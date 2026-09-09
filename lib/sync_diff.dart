import 'bookmark_tree.dart';
import 'database.dart';

/// Result of comparing database bookmarks against a PDF outline extract.
class SyncDiffResult {
  SyncDiffResult({
    required this.toAddToPdf,
    required this.toDeleteFromPdf,
    required this.toAddToDb,
  });

  final Set<String> toAddToPdf;
  final Set<String> toDeleteFromPdf;
  final Set<String> toAddToDb;

  bool get hasChanges =>
      toAddToPdf.isNotEmpty ||
      toDeleteFromPdf.isNotEmpty ||
      toAddToDb.isNotEmpty;
}

/// Pure helpers for path-key diff calculations (testable without I/O).
class SyncDiffCalculator {
  SyncDiffCalculator._();

  static Set<String> pathKeysFromBookmarks(List<Bookmark> bookmarks) {
    final byId = BookmarkTree.indexById(bookmarks);
    return bookmarks
        .map(
          (bookmark) => BookmarkTree.pathKey(
            BookmarkTree.pathForBookmark(bookmark, byId),
          ),
        )
        .toSet();
  }

  static Set<String> pathKeysFromPdfExtract(
    List<Map<String, dynamic>> extracted,
  ) {
    return extracted
        .map(
          (entry) =>
              BookmarkTree.pathKey(List<String>.from(entry['path'] as List)),
        )
        .toSet();
  }

  static SyncDiffResult calculate({
    required Set<String> dbPathKeys,
    required Set<String> pdfPathKeys,
  }) {
    return SyncDiffResult(
      toAddToPdf: dbPathKeys.difference(pdfPathKeys),
      toDeleteFromPdf: pdfPathKeys.difference(dbPathKeys),
      toAddToDb: pdfPathKeys.difference(dbPathKeys),
    );
  }

  static SyncDiffResult calculateFromSources({
    required List<Bookmark> dbBookmarks,
    required List<Map<String, dynamic>> pdfExtracted,
  }) {
    return calculate(
      dbPathKeys: pathKeysFromBookmarks(dbBookmarks),
      pdfPathKeys: pathKeysFromPdfExtract(pdfExtracted),
    );
  }
}
