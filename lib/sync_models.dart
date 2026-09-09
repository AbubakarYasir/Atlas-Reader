/// Enum representing the sync action for a bookmark
enum SyncAction { add, delete, keep }

/// Class representing a bookmark difference for sync preview
class BookmarkDiff {
  final String title;
  final int? pageIndex;
  final SyncAction action;

  BookmarkDiff({
    required this.title,
    required this.pageIndex,
    required this.action,
  });

  @override
  String toString() {
    return 'BookmarkDiff(title: $title, pageIndex: $pageIndex, action: $action)';
  }
}
