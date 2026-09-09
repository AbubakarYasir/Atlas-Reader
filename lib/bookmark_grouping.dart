import 'database.dart';

/// How bookmarks are sorted inside each group or flat list.
enum BookmarkSort { pageNumber, dateCreated, dateModified, alphabetical }

/// How bookmarks are grouped in the list UI.
enum BookmarkViewGroup { book, tag, flat }

extension BookmarkSortLabel on BookmarkSort {
  String get label => switch (this) {
    BookmarkSort.pageNumber => 'Page number',
    BookmarkSort.dateCreated => 'Date created',
    BookmarkSort.dateModified => 'Date modified',
    BookmarkSort.alphabetical => 'Alphabetical (A–Z)',
  };
}

extension BookmarkViewGroupLabel on BookmarkViewGroup {
  String get label => switch (this) {
    BookmarkViewGroup.book => 'By book',
    BookmarkViewGroup.tag => 'By tag',
    BookmarkViewGroup.flat => 'Flat list',
  };
}

/// In-memory grouping and sorting helpers for bookmark list UI.
class BookmarkGrouping {
  BookmarkGrouping._();

  static String fileNameFromPath(String filePath) {
    final parts = filePath.split(RegExp(r'[/\\]'));
    return parts.isNotEmpty ? parts.last : filePath;
  }

  /// Group bookmarks by source PDF file path.
  static Map<String, List<Bookmark>> groupByBook(List<Bookmark> bookmarks) {
    final groups = <String, List<Bookmark>>{};
    for (final bookmark in bookmarks) {
      groups.putIfAbsent(bookmark.filePath, () => []).add(bookmark);
    }
    return groups;
  }

  /// Group bookmarks by tag name. Bookmarks with multiple tags appear under each tag.
  /// Untagged bookmarks are placed under the [untaggedLabel] key.
  static Map<String, List<Bookmark>> groupByTag(
    List<Bookmark> bookmarks,
    Map<int, List<Tag>> tagsByBookmarkId, {
    String untaggedLabel = 'Untagged',
  }) {
    final groups = <String, List<Bookmark>>{};

    for (final bookmark in bookmarks) {
      final tags = tagsByBookmarkId[bookmark.id] ?? [];
      if (tags.isEmpty) {
        groups.putIfAbsent(untaggedLabel, () => []).add(bookmark);
        continue;
      }

      for (final tag in tags) {
        groups.putIfAbsent(tag.name, () => []).add(bookmark);
      }
    }

    return groups;
  }

  /// Returns a flat copy of all bookmarks (unsorted).
  static List<Bookmark> flatList(List<Bookmark> bookmarks) {
    return List<Bookmark>.from(bookmarks);
  }

  /// Sorts [bookmarks] in place according to [sort].
  static void sortBookmarks(List<Bookmark> bookmarks, BookmarkSort sort) {
    bookmarks.sort((a, b) {
      switch (sort) {
        case BookmarkSort.pageNumber:
          if (a.pageIndex == null && b.pageIndex == null) return 0;
          if (a.pageIndex == null) return 1;
          if (b.pageIndex == null) return -1;
          return a.pageIndex!.compareTo(b.pageIndex!);
        case BookmarkSort.dateCreated:
          return b.createdAt.compareTo(a.createdAt);
        case BookmarkSort.dateModified:
          return b.updatedAt.compareTo(a.updatedAt);
        case BookmarkSort.alphabetical:
          return a.title.toLowerCase().compareTo(b.title.toLowerCase());
      }
    });
  }

  /// Sorts each group's bookmarks and returns groups with alphabetically sorted keys.
  static Map<String, List<Bookmark>> sortedGroups(
    Map<String, List<Bookmark>> groups,
    BookmarkSort sort,
  ) {
    final sorted = <String, List<Bookmark>>{};
    final keys = groups.keys.toList()
      ..sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));

    for (final key in keys) {
      final list = List<Bookmark>.from(groups[key]!);
      sortBookmarks(list, sort);
      sorted[key] = list;
    }

    return sorted;
  }

  /// Display label for a group header when grouping by book.
  static String bookGroupLabel(String filePath) {
    return fileNameFromPath(filePath);
  }
}
