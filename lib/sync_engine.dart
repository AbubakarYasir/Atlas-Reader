import 'dart:convert';
import 'dart:io';

import 'package:syncfusion_flutter_pdf/pdf.dart';

import 'database.dart';
import 'pdf_engine.dart';

class SyncEngine {
  final AppDatabase database;

  SyncEngine(this.database);

  /// Reconcile PDF bookmarks with database bookmarks incrementally
  /// Returns (addsCount, deletesCount) on success, null on failure
  Future<(int adds, int deletes)?> reconcile(String filePath) async {
    try {
      print('[SYNC] Starting reconciliation for: $filePath');

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

      print('[SYNC] To add: ${toAdd.length}, To delete: ${toDeleteTitles.length}');

      // If no changes, just update the snapshot and return
      if (toAdd.isEmpty && toDeleteTitles.isEmpty) {
        print('[SYNC] No changes detected, updating snapshot only');
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
            print('[SYNC] Deleted bookmark: $titleToDelete');
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
            bookmarkToAdd.pageIndex >= pageCount ? pageCount - 1 : bookmarkToAdd.pageIndex;

        try {
          final bookmark = document.bookmarks.add(bookmarkToAdd.title);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          print('[SYNC] Added bookmark: ${bookmarkToAdd.title} at page $safePageIndex');
        } catch (e) {
          print('[SYNC] Error adding bookmark ${bookmarkToAdd.title}: $e');
        }
      }

      print('[SYNC] Added $addedCount, Deleted $deletedCount bookmarks');

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

      print('[SYNC] File overwritten successfully');

      // Step 8: Update FileSnapshots table with new state
      await database.saveFileSnapshot(
        filePath,
        currentBookmarks.map((b) => b.title).toList(),
      );

      print('[SYNC] Reconciliation complete');
      return (addedCount, deletedCount);
    } catch (e, stackTrace) {
      print('[SYNC] ERROR in reconcile: $e');
      print('[SYNC] Stack trace: $stackTrace');
      return null;
    }
  }
}
