import 'package:atlas_poc/bookmark_tree.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/file_snapshot_state.dart';
import 'package:flutter_test/flutter_test.dart';

Bookmark _bookmark({required int id, required String title, int? parentId}) {
  final now = DateTime(2026, 1, 1);
  return Bookmark(
    id: id,
    filePath: '/books/sample.pdf',
    title: title,
    pageIndex: null,
    description: null,
    parentId: parentId,
    isFolder: true,
    createdAt: now,
    updatedAt: now,
  );
}

void main() {
  group('FileSnapshotState', () {
    test('encodes hierarchical path keys', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Chapter 2', parentId: 1),
        _bookmark(id: 3, title: 'Section A', parentId: 2),
      ];

      final snapshot = FileSnapshotState.fromBookmarks(bookmarks);
      final decoded = FileSnapshotState.decode(snapshot.encode());

      expect(decoded.version, FileSnapshotState.currentVersion);
      expect(decoded.pathKeys, contains(BookmarkTree.pathKey(['Volume 1'])));
      expect(
        decoded.pathKeys,
        contains(BookmarkTree.pathKey(['Volume 1', 'Chapter 2', 'Section A'])),
      );
    });

    test('decodes legacy flat title snapshots', () {
      final legacy = '["Volume 1","Index","Chapter 2"]';
      final decoded = FileSnapshotState.decode(legacy);

      expect(decoded.version, 1);
      expect(decoded.pathKeys, ['Volume 1', 'Index', 'Chapter 2']);
    });

    test('distinguishes duplicate titles via full paths', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Index', parentId: 1),
        _bookmark(id: 3, title: 'Volume 2'),
        _bookmark(id: 4, title: 'Index', parentId: 3),
      ];

      final keys = FileSnapshotState.fromBookmarks(bookmarks).pathKeys;

      expect(keys, hasLength(4));
      expect(keys.where((key) => key.endsWith('Index')).length, 2);
      expect(keys, isNot(contains('Index')));
    });
  });
}
