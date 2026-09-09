// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Atlas Reader';

  @override
  String get settings => 'Settings';

  @override
  String get language => 'Language';

  @override
  String get english => 'English';

  @override
  String get arabic => 'Arabic';

  @override
  String get languageDescription =>
      'Choose the app language and layout direction';

  @override
  String get libraryFolders => 'Library folders';

  @override
  String get libraryFoldersDescription =>
      'Choose folders to scan for PDF files';

  @override
  String get browseLibrary => 'Browse library';

  @override
  String get searchBookmarks => 'Search all bookmarks';

  @override
  String get searchHint => 'Bookmark title or PDF file name...';

  @override
  String get commandCenterHint => 'Enter a search term. Press Escape to close.';

  @override
  String get noMatches => 'No matches found.';

  @override
  String get searchLibrary => 'Search bookmarks across your library.';

  @override
  String pageNumber(Object number) {
    return 'Page $number';
  }

  @override
  String pageNumberOf(Object page, Object total) {
    return 'Page $page of $total';
  }

  @override
  String get pdfFile => 'PDF file';

  @override
  String get folder => 'Folder';

  @override
  String get syncing => 'Syncing bookmarks…';

  @override
  String syncComplete(Object count) {
    return 'Sync complete: $count new bookmarks found.';
  }

  @override
  String get saveComplete => 'Saved successfully.';

  @override
  String saveError(Object error) {
    return 'Could not save: $error';
  }

  @override
  String get fileMissing => 'The PDF file could not be found.';

  @override
  String bookmarkSaved(Object page) {
    return 'Bookmark saved for page $page.';
  }

  @override
  String get bookmarkTitle => 'Bookmark title';

  @override
  String get rename => 'Rename';

  @override
  String get remove => 'Remove';

  @override
  String get keyboardTreeHint =>
      'Use Up and Down arrows to move, Right and Left to expand or collapse, Enter to open, F2 to rename, and Delete to remove.';

  @override
  String bookmarkTreeItem(
    Object kind,
    Object level,
    Object page,
    Object title,
  ) {
    return '$title, level $level, $kind, $page';
  }

  @override
  String get bookmarkTreeFolder => 'folder';

  @override
  String get bookmarkTreeBookmark => 'bookmark';

  @override
  String openAtPage(Object page, Object title) {
    return 'Open $title at page $page';
  }

  @override
  String openFile(Object title) {
    return 'Open $title';
  }

  @override
  String get theme => 'Theme';

  @override
  String get themeDescription =>
      'System, light, dark, and high-contrast themes';

  @override
  String get readerNavigation => 'Reader navigation';

  @override
  String get pages => 'Pages';

  @override
  String get outline => 'Outline';

  @override
  String get bookmarks => 'Bookmarks';

  @override
  String get annotations => 'Annotations';

  @override
  String get searchCurrentPanel => 'Search this panel';

  @override
  String get clearSearch => 'Clear search';

  @override
  String get closeNavigationPanel => 'Close reader navigation';

  @override
  String get loadingPages => 'Loading pages…';

  @override
  String get noBookmarks => 'No bookmarks in this document';

  @override
  String get noOutline => 'No outline in this document';

  @override
  String get noAnnotations => 'No annotations in this document';

  @override
  String pageThumbnail(Object page) {
    return 'Page $page thumbnail';
  }

  @override
  String get showNavigation => 'Show reader navigation';

  @override
  String get hideNavigation => 'Hide reader navigation';

  @override
  String zoomPercent(Object percent) {
    return 'Zoom $percent%';
  }

  @override
  String get zoomOut => 'Zoom out';

  @override
  String get zoomIn => 'Zoom in';

  @override
  String get fitPage => 'Fit page';

  @override
  String get fitWidth => 'Fit width';

  @override
  String get write => 'Write';

  @override
  String get openDocumentTab => 'Open document tab';

  @override
  String closeDocumentTab(Object title) {
    return 'Close $title';
  }

  @override
  String get customZoom => 'Custom zoom…';

  @override
  String get zoomRange => 'Zoom (10–6400)';

  @override
  String get apply => 'Apply';

  @override
  String get open => 'Open';

  @override
  String get addToFavorites => 'Add to favorites';

  @override
  String get removeFromFavorites => 'Remove from favorites';

  @override
  String get editLibraryDetails => 'Edit library details';

  @override
  String get refreshCover => 'Refresh cover';

  @override
  String get showInFileExplorer => 'Show in File Explorer';

  @override
  String get copyFullPath => 'Copy full path';

  @override
  String get close => 'Close';

  @override
  String get closeOtherTabs => 'Close other tabs';

  @override
  String get closeTabsToRight => 'Close tabs to the right';

  @override
  String get filePathCopied => 'File path copied.';

  @override
  String couldNotShowFile(Object error) {
    return 'Could not show the file: $error';
  }

  @override
  String get restoreDocumentTabs => 'Restore document tabs on restart';

  @override
  String get restoreDocumentTabsDescription =>
      'Off by default. When disabled, Atlas opens only the document you requested.';
}
