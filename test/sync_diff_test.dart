import 'package:atlas_poc/bookmark_tree.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/sync_diff.dart';
import 'package:flutter_test/flutter_test.dart';

Bookmark _bookmark({
  required int id,
  required String title,
  int? parentId,
  int? pageIndex,
}) {
  final now = DateTime(2026, 1, 1);
  return Bookmark(
    id: id,
    filePath: '/books/sample.pdf',
    title: title,
    pageIndex: pageIndex,
    description: null,
    parentId: parentId,
    isFolder: false,
    createdAt: now,
    updatedAt: now,
  );
}

void main() {
  group('SyncDiffCalculator', () {
    test('detects bookmarks to add to PDF', () {
      final dbBookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Chapter 2', parentId: 1, pageIndex: 3),
      ];
      final pdfExtracted = [
        {
          'path': ['Volume 1'],
          'pageIndex': null,
          'isFolder': true,
        },
      ];

      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: dbBookmarks,
        pdfExtracted: pdfExtracted,
      );

      expect(diff.toAddToPdf, hasLength(1));
      expect(
        diff.toAddToPdf.single,
        BookmarkTree.pathKey(['Volume 1', 'Chapter 2']),
      );
      expect(diff.toDeleteFromPdf, isEmpty);
    });

    test('detects bookmarks to remove from PDF', () {
      final dbBookmarks = [_bookmark(id: 1, title: 'Volume 1')];
      final pdfExtracted = [
        {
          'path': ['Volume 1'],
          'pageIndex': null,
          'isFolder': true,
        },
        {
          'path': ['Volume 1', 'Legacy Section'],
          'pageIndex': 8,
          'isFolder': false,
        },
      ];

      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: dbBookmarks,
        pdfExtracted: pdfExtracted,
      );

      expect(diff.toDeleteFromPdf, hasLength(1));
      expect(
        diff.toDeleteFromPdf.single,
        BookmarkTree.pathKey(['Volume 1', 'Legacy Section']),
      );
    });

    test('does not confuse duplicate titles under different parents', () {
      final dbBookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Index', parentId: 1, pageIndex: 1),
        _bookmark(id: 3, title: 'Volume 2'),
        _bookmark(id: 4, title: 'Index', parentId: 3, pageIndex: 2),
      ];
      final pdfExtracted = [
        {
          'path': ['Volume 1'],
          'pageIndex': null,
          'isFolder': true,
        },
        {
          'path': ['Volume 1', 'Index'],
          'pageIndex': 1,
          'isFolder': false,
        },
        {
          'path': ['Volume 2'],
          'pageIndex': null,
          'isFolder': true,
        },
        {
          'path': ['Volume 2', 'Index'],
          'pageIndex': 2,
          'isFolder': false,
        },
      ];

      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: dbBookmarks,
        pdfExtracted: pdfExtracted,
      );

      expect(diff.hasChanges, isFalse);
    });

    test('reports no changes when path sets match', () {
      final dbBookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Chapter', parentId: 1, pageIndex: 0),
      ];
      final pdfExtracted = [
        {
          'path': ['Volume 1'],
          'pageIndex': null,
          'isFolder': true,
        },
        {
          'path': ['Volume 1', 'Chapter'],
          'pageIndex': 0,
          'isFolder': false,
        },
      ];

      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: dbBookmarks,
        pdfExtracted: pdfExtracted,
      );

      expect(diff.hasChanges, isFalse);
    });
  });
}
