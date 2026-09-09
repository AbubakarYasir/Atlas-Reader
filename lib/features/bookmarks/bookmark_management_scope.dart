import 'package:flutter/widgets.dart';

/// Shared callback contract for bookmark-list widgets and edit dialogs.
class BookmarkManagementScope extends InheritedWidget {
  const BookmarkManagementScope({
    super.key,
    required this.refreshBookmarks,
    required super.child,
  });

  final VoidCallback refreshBookmarks;

  static BookmarkManagementScope? maybeOf(BuildContext context) =>
      context.dependOnInheritedWidgetOfExactType<BookmarkManagementScope>();

  @override
  bool updateShouldNotify(BookmarkManagementScope oldWidget) =>
      refreshBookmarks != oldWidget.refreshBookmarks;
}
