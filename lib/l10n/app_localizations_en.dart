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
}
