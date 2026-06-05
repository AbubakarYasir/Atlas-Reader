import 'dart:io';

import 'package:syncfusion_flutter_pdf/pdf.dart';
import 'package:logging/logging.dart';

import 'database.dart';
import 'pdf_engine.dart';

class SyncEngine {
  final AppDatabase database;

  SyncEngine(this.database);
  final _log = Logger('SyncEngine');

  /// Reconcile PDF bookmarks with database bookmarks incrementally
  /// Returns (addsCount, deletesCount) on success, null on failure
  Future<(int adds, int deletes)?> reconcile(String filePath) async {
    try {
      _log.info('[SYNC] Starting reconciliation for: $filePath');

      // Step 1: Fetch current bookmarks from DB
      final currentBookmarks = await database.getBookmarksForFile(filePath);
      final currentDbTitles = currentBookmarks.map((b) => b.title).toSet();

      // Step 2: Fetch current bookmarks from PDF
      final pdfBookmarks = await PdfEngine.extractBookmarks(filePath);
      final pdfTitles = pdfBookmarks.map((b) => b['title'] as String).toSet();

      // Step 3: Identify toAdd and toDelete by comparing DB vs PDF
      final toAdd = currentBookmarks
          .where((bookmark) => !pdfTitles.contains(bookmark.title))
          .toList();

      final toDeleteTitles = pdfBookmarks
          .map((b) => b['title'] as String)
          .where((title) => !currentDbTitles.contains(title))
          .toList();

      // Step 3.5: Identify bookmarks to sync from PDF to DB (new bookmarks in PDF)
      final toSyncFromPdf = pdfBookmarks
          .where((b) => !currentDbTitles.contains(b['title'] as String))
          .toList();

      _log.info('[SYNC] To add: ${toAdd.length}, To delete: ${toDeleteTitles.length}');

      // If no changes, just update the snapshot and return
      if (toAdd.isEmpty && toDeleteTitles.isEmpty) {
        _log.info('[SYNC] No changes detected, updating snapshot only');
        await database.saveFileSnapshot(
          filePath,
          currentBookmarks.map((b) => b.title).toList(),
        );
        return (0, 0);
      }

      // Step 4: Load PDF
      final bytes = await File(filePath).readAsBytes();
      final document = PdfDocument(inputBytes: bytes);

      // Step 5: Perform deletions
      int deletedCount = 0;
      for (final titleToDelete in toDeleteTitles) {
        // Find the bookmark by title
        for (int i = 0; i < document.bookmarks.count; i++) {
          final bookmark = document.bookmarks[i];
          if (bookmark.title == titleToDelete) {
            document.bookmarks.removeAt(i);
            deletedCount++;
            _log.info('[SYNC] Deleted bookmark: $titleToDelete');
            // Adjust index since we removed an item
            i--;
            break;
          }
        }
      }

      // Step 6: Perform additions
      int addedCount = 0;
      final pageCount = document.pages.count;
      for (final bookmarkToAdd in toAdd) {
        final safePageIndex =
            bookmarkToAdd.pageIndex != null && bookmarkToAdd.pageIndex! >= pageCount
                ? pageCount - 1
                : bookmarkToAdd.pageIndex ?? 0;

        try {
          // Fetch tags for this bookmark
          final tags = await database.getTagsForBookmark(bookmarkToAdd.id);

          // Construct title with tags if present
          String titleToWrite = bookmarkToAdd.title;
          if (tags.isNotEmpty) {
            final tagString = tags.map((t) => '#${t.name}').join(', ');
            titleToWrite = '$titleToWrite - $tagString';
          }

          final bookmark = document.bookmarks.add(titleToWrite);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          _log.info('[SYNC] Added bookmark: $titleToWrite at page $safePageIndex');
        } catch (e) {
          _log.warning('[SYNC] Error adding bookmark ${bookmarkToAdd.title}: $e');
          // Continue without tags if there's an error
          try {
            final bookmark = document.bookmarks.add(bookmarkToAdd.title);
            bookmark.destination = PdfDestination(document.pages[safePageIndex]);
            addedCount++;
            _log.info('[SYNC] Added bookmark without tags: ${bookmarkToAdd.title} at page $safePageIndex');
          } catch (e2) {
            _log.warning('[SYNC] Error adding bookmark without tags ${bookmarkToAdd.title}: $e2');
          }
        }
      }

      _log.info('[SYNC] Added $addedCount, Deleted $deletedCount bookmarks');

      // Step 7: Save PDF using Safe Overwrite
      final outBytes = await document.save();
      document.dispose();

      // Safe overwrite logic
      final tmpPath = '$filePath.tmp';
      final originalFile = File(filePath);
      final tmpFile = File(tmpPath);

      await tmpFile.writeAsBytes(outBytes);

      try {
        await originalFile.delete();
        await tmpFile.rename(filePath);
      } catch (_) {
        if (await tmpFile.exists()) {
          await tmpFile.delete();
        }
        throw Exception(
          'Could not replace the PDF because it is open in another application. '
          'Close the file and try again.',
        );
      }

      _log.info('[SYNC] File overwritten successfully');

      // Step 8: Sync new bookmarks from PDF to DB with tags
      int syncedFromPdfCount = 0;
      for (final pdfBookmark in toSyncFromPdf) {
        final title = pdfBookmark['title'] as String;
        final pageIndex = pdfBookmark['pageIndex'] as int?;
        final description = pdfBookmark['description'] as String?;
        final tags = pdfBookmark['tags'] as List<String>?;

        try {
          // Insert bookmark into database
          final bookmarkId = await database.addBookmark(
            filePath: filePath,
            title: title,
            pageIndex: pageIndex,
            description: description,
          );

          // Link tags to bookmark
          if (tags != null && tags.isNotEmpty) {
            for (final tag in tags) {
              // Clean up tag (remove # prefix if present)
              final cleanTag = tag.startsWith('#') ? tag.substring(1) : tag;
              await database.addTagToBookmark(bookmarkId, cleanTag);
            }
          }

          syncedFromPdfCount++;
          _log.info('[SYNC] Synced bookmark from PDF: $title with ${tags?.length ?? 0} tags');
        } catch (e) {
          _log.warning('[SYNC] Error syncing bookmark $title: $e');
        }
      }

      // Step 9: Update FileSnapshots table with new state
      await database.saveFileSnapshot(
        filePath,
        currentBookmarks.map((b) => b.title).toList(),
      );

      _log.info('[SYNC] Synced $syncedFromPdfCount bookmarks from PDF to DB');
      return (addedCount, deletedCount);
    } catch (e, stackTrace) {
      _log.severe('[SYNC] ERROR in reconcile: $e');
      _log.severe('[SYNC] Stack trace: $stackTrace');
      return null;
    }
  }
}
