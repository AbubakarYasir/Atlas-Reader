import 'database.dart';

/// Separator used when encoding a bookmark's hierarchical path as a single key.
const bookmarkPathSeparator = '\x1e';

/// A node in the bookmark hierarchy tree.
class BookmarkTreeNode {
  BookmarkTreeNode({required this.bookmark, List<BookmarkTreeNode>? children})
    : children = children ?? [];

  final Bookmark bookmark;
  final List<BookmarkTreeNode> children;
}

/// Helpers for building and comparing hierarchical bookmark trees.
class BookmarkTree {
  BookmarkTree._();

  static String pathKey(List<String> path) => path.join(bookmarkPathSeparator);

  static String displayPath(List<String> path) => path.join(' › ');

  /// Builds a lookup map from bookmark id to bookmark.
  static Map<int, Bookmark> indexById(Iterable<Bookmark> bookmarks) {
    return {for (final bookmark in bookmarks) bookmark.id: bookmark};
  }

  /// Returns the full title path from root to [bookmark].
  static List<String> pathForBookmark(
    Bookmark bookmark,
    Map<int, Bookmark> byId,
  ) {
    final segments = <String>[bookmark.title];
    var current = bookmark;

    while (current.parentId != null) {
      final parent = byId[current.parentId];
      if (parent == null) break;
      segments.insert(0, parent.title);
      current = parent;
    }

    return segments;
  }

  /// Returns the depth of [bookmark] in its tree (0 = root).
  static int depth(Bookmark bookmark, Map<int, Bookmark> byId) {
    var level = 0;
    var current = bookmark;

    while (current.parentId != null) {
      final parent = byId[current.parentId];
      if (parent == null) break;
      level++;
      current = parent;
    }

    return level;
  }

  /// Builds root-level tree nodes from a flat bookmark list.
  static List<BookmarkTreeNode> buildForest(List<Bookmark> bookmarks) {
    final nodes = <int, BookmarkTreeNode>{
      for (final bookmark in bookmarks)
        bookmark.id: BookmarkTreeNode(bookmark: bookmark),
    };

    final roots = <BookmarkTreeNode>[];

    for (final bookmark in bookmarks) {
      final node = nodes[bookmark.id]!;
      final parentId = bookmark.parentId;

      if (parentId != null && nodes.containsKey(parentId)) {
        nodes[parentId]!.children.add(node);
      } else {
        roots.add(node);
      }
    }

    _sortNodes(roots);
    return roots;
  }

  /// Roots for a single file, preserving PDF outline order via id.
  static List<BookmarkTreeNode> rootsForFile(
    List<Bookmark> bookmarks,
    String filePath,
  ) {
    final fileBookmarks = bookmarks
        .where((bookmark) => bookmark.filePath == filePath)
        .toList();
    return buildForest(fileBookmarks);
  }

  /// Includes ancestor folders so search results can render in context.
  static List<Bookmark> includeWithAncestors(
    List<Bookmark> matches,
    List<Bookmark> allBookmarks,
  ) {
    final byId = indexById(allBookmarks);
    final included = <int, Bookmark>{};

    for (final bookmark in matches) {
      var current = bookmark;
      while (true) {
        included[current.id] = current;
        if (current.parentId == null) break;
        final parent = byId[current.parentId];
        if (parent == null) break;
        current = parent;
      }
    }

    return included.values.toList();
  }

  static void _sortNodes(List<BookmarkTreeNode> nodes) {
    nodes.sort((a, b) => a.bookmark.id.compareTo(b.bookmark.id));
    for (final node in nodes) {
      _sortNodes(node.children);
    }
  }
}
