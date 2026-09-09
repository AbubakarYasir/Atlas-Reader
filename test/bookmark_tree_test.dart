import 'package:atlas_poc/bookmark_tree.dart';
import 'package:atlas_poc/database.dart';
import 'package:flutter_test/flutter_test.dart';

Bookmark _bookmark({
  required int id,
  required String title,
  int? parentId,
  bool isFolder = false,
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
    isFolder: isFolder,
    createdAt: now,
    updatedAt: now,
  );
}

void main() {
  group('BookmarkTree.pathKey', () {
    test('joins segments with separator', () {
      expect(
        BookmarkTree.pathKey(['Volume 1', 'Chapter 2', 'Section A']),
        'Volume 1${bookmarkPathSeparator}Chapter 2${bookmarkPathSeparator}Section A',
      );
    });

    test('distinguishes identical titles under different parents', () {
      final first = BookmarkTree.pathKey(['Volume 1', 'Index']);
      final second = BookmarkTree.pathKey(['Volume 2', 'Index']);
      expect(first, isNot(equals(second)));
    });

    test('preserves Arabic, Latin text, and punctuation in a path key', () {
      const path = ['أصول الفقه', 'Chapter 2: الأدلة', 'Section (A)'];

      expect(
        BookmarkTree.pathKey(path),
        'أصول الفقه${bookmarkPathSeparator}Chapter 2: الأدلة'
        '${bookmarkPathSeparator}Section (A)',
      );
    });
  });

  group('BookmarkTree.pathForBookmark', () {
    test('builds full path from root to leaf', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Chapter 2', parentId: 1),
        _bookmark(id: 3, title: 'Section A', parentId: 2, pageIndex: 10),
      ];
      final byId = BookmarkTree.indexById(bookmarks);

      expect(BookmarkTree.pathForBookmark(bookmarks[2], byId), [
        'Volume 1',
        'Chapter 2',
        'Section A',
      ]);
    });
  });

  group('BookmarkTree.buildForest', () {
    test('nests children under parents', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Root'),
        _bookmark(id: 2, title: 'Child', parentId: 1),
        _bookmark(id: 3, title: 'Grandchild', parentId: 2, pageIndex: 4),
      ];

      final forest = BookmarkTree.buildForest(bookmarks);

      expect(forest, hasLength(1));
      expect(forest.first.bookmark.title, 'Root');
      expect(forest.first.children, hasLength(1));
      expect(forest.first.children.first.bookmark.title, 'Child');
      expect(forest.first.children.first.children, hasLength(1));
      expect(
        forest.first.children.first.children.first.bookmark.title,
        'Grandchild',
      );
    });

    test('returns multiple roots for sibling top-level bookmarks', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Volume 1'),
        _bookmark(id: 2, title: 'Volume 2'),
      ];

      final forest = BookmarkTree.buildForest(bookmarks);
      expect(forest, hasLength(2));
    });
  });

  group('BookmarkTree.includeWithAncestors', () {
    test('includes parent folders for search matches', () {
      final bookmarks = [
        _bookmark(id: 1, title: 'Volume 1', isFolder: true),
        _bookmark(id: 2, title: 'Chapter', parentId: 1, isFolder: true),
        _bookmark(id: 3, title: 'Section A', parentId: 2, pageIndex: 1),
      ];

      final included = BookmarkTree.includeWithAncestors([
        bookmarks[2],
      ], bookmarks);

      expect(included.map((bookmark) => bookmark.id).toSet(), {1, 2, 3});
    });
  });
}
