import 'package:logging/logging.dart';

import 'database.dart';
import 'pdf_engine.dart';
import 'sync_diff.dart';

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
      final pdfBookmarks = await PdfEngine.extractBookmarks(filePath);
      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: currentBookmarks,
        pdfExtracted: pdfBookmarks,
      );

      _log.info(
        '[SYNC] To add to PDF: ${diff.toAddToPdf.length}, '
        'To delete from PDF: ${diff.toDeleteFromPdf.length}, '
        'To add to DB: ${diff.toAddToDb.length}',
      );

      if (!diff.hasChanges) {
        _log.info('[SYNC] No changes detected, updating snapshot only');
        await database.saveFileSnapshot(filePath, currentBookmarks);
        return (0, 0);
      }

      if (diff.toAddToPdf.isNotEmpty || diff.toDeleteFromPdf.isNotEmpty) {
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

      if (diff.toAddToDb.isNotEmpty) {
        final missingFromDb = pdfBookmarks.where((bookmark) {
          final pathKey = SyncDiffCalculator.pathKeysFromPdfExtract([bookmark]).single;
          return diff.toAddToDb.contains(pathKey);
        }).toList();

        await database.syncBookmarkHierarchy(filePath, missingFromDb);
      }

      final refreshedBookmarks = await database.getBookmarksForFile(filePath);
      await database.saveFileSnapshot(filePath, refreshedBookmarks);

      return (diff.toAddToPdf.length, diff.toDeleteFromPdf.length);
    } on PdfOverwriteException catch (e) {
      _log.severe('[SYNC] PDF overwrite blocked: $e');
      return null;
    } catch (e, stackTrace) {
      _log.severe('[SYNC] ERROR in reconcile: $e');
      _log.severe('[SYNC] Stack trace: $stackTrace');
      return null;
    }
  }
}
