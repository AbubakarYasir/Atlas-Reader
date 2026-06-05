import 'package:logging/logging.dart';

import 'bookmark_tree.dart';
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

      final currentBookmarks = await database.getBookmarksForFile(filePath);
      final byId = BookmarkTree.indexById(currentBookmarks);
      final dbPathKeys = currentBookmarks
          .map((bookmark) => BookmarkTree.pathKey(BookmarkTree.pathForBookmark(bookmark, byId)))
          .toSet();

      final pdfBookmarks = await PdfEngine.extractBookmarks(filePath);
      final pdfPathKeys = pdfBookmarks
          .map((bookmark) => BookmarkTree.pathKey(List<String>.from(bookmark['path'] as List)))
          .toSet();

      final toAddToPdf = dbPathKeys.difference(pdfPathKeys);
      final toDeleteFromPdf = pdfPathKeys.difference(dbPathKeys);
      final toAddToDb = pdfPathKeys.difference(dbPathKeys);

      _log.info(
        '[SYNC] To add to PDF: ${toAddToPdf.length}, '
        'To delete from PDF: ${toDeleteFromPdf.length}, '
        'To add to DB: ${toAddToDb.length}',
      );

      if (toAddToPdf.isEmpty && toDeleteFromPdf.isEmpty && toAddToDb.isEmpty) {
        _log.info('[SYNC] No changes detected, updating snapshot only');
        await database.saveFileSnapshot(
          filePath,
          currentBookmarks.map((b) => b.title).toList(),
        );
        return (0, 0);
      }

      if (toAddToPdf.isNotEmpty || toDeleteFromPdf.isNotEmpty) {
        final tagsByBookmarkId = <int, List<String>>{};
        for (final bookmark in currentBookmarks) {
          final tags = await database.getTagsForBookmark(bookmark.id);
          if (tags.isNotEmpty) {
            tagsByBookmarkId[bookmark.id] = tags.map((tag) => tag.name).toList();
          }
        }

        final success = await PdfEngine.overwriteBookmarkTree(
          filePath,
          currentBookmarks,
          tagsByBookmarkId: tagsByBookmarkId,
        );

        if (!success) {
          return null;
        }
      }

      if (toAddToDb.isNotEmpty) {
        final missingFromDb = pdfBookmarks.where((bookmark) {
          final pathKey =
              BookmarkTree.pathKey(List<String>.from(bookmark['path'] as List));
          return toAddToDb.contains(pathKey);
        }).toList();

        await database.syncBookmarkHierarchy(filePath, missingFromDb);
      }

      final refreshedBookmarks = await database.getBookmarksForFile(filePath);
      await database.saveFileSnapshot(
        filePath,
        refreshedBookmarks.map((b) => b.title).toList(),
      );

      return (toAddToPdf.length, toDeleteFromPdf.length);
    } catch (e, stackTrace) {
      _log.severe('[SYNC] ERROR in reconcile: $e');
      _log.severe('[SYNC] Stack trace: $stackTrace');
      return null;
    }
  }
}
