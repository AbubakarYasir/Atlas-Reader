import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_ar.dart';
import 'app_localizations_en.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations? of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations);
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[
    Locale('ar'),
    Locale('en'),
  ];

  /// No description provided for @appTitle.
  ///
  /// In en, this message translates to:
  /// **'Atlas Reader'**
  String get appTitle;

  /// No description provided for @settings.
  ///
  /// In en, this message translates to:
  /// **'Settings'**
  String get settings;

  /// No description provided for @language.
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get language;

  /// No description provided for @english.
  ///
  /// In en, this message translates to:
  /// **'English'**
  String get english;

  /// No description provided for @arabic.
  ///
  /// In en, this message translates to:
  /// **'Arabic'**
  String get arabic;

  /// No description provided for @languageDescription.
  ///
  /// In en, this message translates to:
  /// **'Choose the app language and layout direction'**
  String get languageDescription;

  /// No description provided for @libraryFolders.
  ///
  /// In en, this message translates to:
  /// **'Library folders'**
  String get libraryFolders;

  /// No description provided for @libraryFoldersDescription.
  ///
  /// In en, this message translates to:
  /// **'Choose folders to scan for PDF files'**
  String get libraryFoldersDescription;

  /// No description provided for @browseLibrary.
  ///
  /// In en, this message translates to:
  /// **'Browse library'**
  String get browseLibrary;

  /// No description provided for @searchBookmarks.
  ///
  /// In en, this message translates to:
  /// **'Search all bookmarks'**
  String get searchBookmarks;

  /// No description provided for @searchHint.
  ///
  /// In en, this message translates to:
  /// **'Bookmark title or PDF file name...'**
  String get searchHint;

  /// No description provided for @commandCenterHint.
  ///
  /// In en, this message translates to:
  /// **'Enter a search term. Press Escape to close.'**
  String get commandCenterHint;

  /// No description provided for @noMatches.
  ///
  /// In en, this message translates to:
  /// **'No matches found.'**
  String get noMatches;

  /// No description provided for @searchLibrary.
  ///
  /// In en, this message translates to:
  /// **'Search bookmarks across your library.'**
  String get searchLibrary;

  /// No description provided for @pageNumber.
  ///
  /// In en, this message translates to:
  /// **'Page {number}'**
  String pageNumber(Object number);

  /// No description provided for @pageNumberOf.
  ///
  /// In en, this message translates to:
  /// **'Page {page} of {total}'**
  String pageNumberOf(Object page, Object total);

  /// No description provided for @pdfFile.
  ///
  /// In en, this message translates to:
  /// **'PDF file'**
  String get pdfFile;

  /// No description provided for @folder.
  ///
  /// In en, this message translates to:
  /// **'Folder'**
  String get folder;

  /// No description provided for @syncing.
  ///
  /// In en, this message translates to:
  /// **'Syncing bookmarks…'**
  String get syncing;

  /// No description provided for @syncComplete.
  ///
  /// In en, this message translates to:
  /// **'Sync complete: {count} new bookmarks found.'**
  String syncComplete(Object count);

  /// No description provided for @saveComplete.
  ///
  /// In en, this message translates to:
  /// **'Saved successfully.'**
  String get saveComplete;

  /// No description provided for @saveError.
  ///
  /// In en, this message translates to:
  /// **'Could not save: {error}'**
  String saveError(Object error);

  /// No description provided for @fileMissing.
  ///
  /// In en, this message translates to:
  /// **'The PDF file could not be found.'**
  String get fileMissing;

  /// No description provided for @bookmarkSaved.
  ///
  /// In en, this message translates to:
  /// **'Bookmark saved for page {page}.'**
  String bookmarkSaved(Object page);

  /// No description provided for @bookmarkTitle.
  ///
  /// In en, this message translates to:
  /// **'Bookmark title'**
  String get bookmarkTitle;

  /// No description provided for @rename.
  ///
  /// In en, this message translates to:
  /// **'Rename'**
  String get rename;

  /// No description provided for @remove.
  ///
  /// In en, this message translates to:
  /// **'Remove'**
  String get remove;

  /// No description provided for @keyboardTreeHint.
  ///
  /// In en, this message translates to:
  /// **'Use Up and Down arrows to move, Right and Left to expand or collapse, Enter to open, F2 to rename, and Delete to remove.'**
  String get keyboardTreeHint;

  /// No description provided for @bookmarkTreeItem.
  ///
  /// In en, this message translates to:
  /// **'{title}, level {level}, {kind}, {page}'**
  String bookmarkTreeItem(Object kind, Object level, Object page, Object title);

  /// No description provided for @bookmarkTreeFolder.
  ///
  /// In en, this message translates to:
  /// **'folder'**
  String get bookmarkTreeFolder;

  /// No description provided for @bookmarkTreeBookmark.
  ///
  /// In en, this message translates to:
  /// **'bookmark'**
  String get bookmarkTreeBookmark;

  /// No description provided for @openAtPage.
  ///
  /// In en, this message translates to:
  /// **'Open {title} at page {page}'**
  String openAtPage(Object page, Object title);

  /// No description provided for @openFile.
  ///
  /// In en, this message translates to:
  /// **'Open {title}'**
  String openFile(Object title);

  /// No description provided for @theme.
  ///
  /// In en, this message translates to:
  /// **'Theme'**
  String get theme;

  /// No description provided for @themeDescription.
  ///
  /// In en, this message translates to:
  /// **'System, light, dark, and high-contrast themes'**
  String get themeDescription;

  /// No description provided for @readerNavigation.
  ///
  /// In en, this message translates to:
  /// **'Reader navigation'**
  String get readerNavigation;

  /// No description provided for @pages.
  ///
  /// In en, this message translates to:
  /// **'Pages'**
  String get pages;

  /// No description provided for @outline.
  ///
  /// In en, this message translates to:
  /// **'Outline'**
  String get outline;

  /// No description provided for @bookmarks.
  ///
  /// In en, this message translates to:
  /// **'Bookmarks'**
  String get bookmarks;

  /// No description provided for @annotations.
  ///
  /// In en, this message translates to:
  /// **'Annotations'**
  String get annotations;

  /// No description provided for @searchCurrentPanel.
  ///
  /// In en, this message translates to:
  /// **'Search this panel'**
  String get searchCurrentPanel;

  /// No description provided for @clearSearch.
  ///
  /// In en, this message translates to:
  /// **'Clear search'**
  String get clearSearch;

  /// No description provided for @closeNavigationPanel.
  ///
  /// In en, this message translates to:
  /// **'Close reader navigation'**
  String get closeNavigationPanel;

  /// No description provided for @loadingPages.
  ///
  /// In en, this message translates to:
  /// **'Loading pages…'**
  String get loadingPages;

  /// No description provided for @noBookmarks.
  ///
  /// In en, this message translates to:
  /// **'No bookmarks in this document'**
  String get noBookmarks;

  /// No description provided for @noOutline.
  ///
  /// In en, this message translates to:
  /// **'No outline in this document'**
  String get noOutline;

  /// No description provided for @noAnnotations.
  ///
  /// In en, this message translates to:
  /// **'No annotations in this document'**
  String get noAnnotations;

  /// No description provided for @pageThumbnail.
  ///
  /// In en, this message translates to:
  /// **'Page {page} thumbnail'**
  String pageThumbnail(Object page);

  /// No description provided for @showNavigation.
  ///
  /// In en, this message translates to:
  /// **'Show reader navigation'**
  String get showNavigation;

  /// No description provided for @hideNavigation.
  ///
  /// In en, this message translates to:
  /// **'Hide reader navigation'**
  String get hideNavigation;

  /// No description provided for @zoomPercent.
  ///
  /// In en, this message translates to:
  /// **'Zoom {percent}%'**
  String zoomPercent(Object percent);

  /// No description provided for @zoomOut.
  ///
  /// In en, this message translates to:
  /// **'Zoom out'**
  String get zoomOut;

  /// No description provided for @zoomIn.
  ///
  /// In en, this message translates to:
  /// **'Zoom in'**
  String get zoomIn;

  /// No description provided for @fitPage.
  ///
  /// In en, this message translates to:
  /// **'Fit page'**
  String get fitPage;

  /// No description provided for @fitWidth.
  ///
  /// In en, this message translates to:
  /// **'Fit width'**
  String get fitWidth;

  /// No description provided for @write.
  ///
  /// In en, this message translates to:
  /// **'Write'**
  String get write;

  /// No description provided for @openDocumentTab.
  ///
  /// In en, this message translates to:
  /// **'Open document tab'**
  String get openDocumentTab;

  /// No description provided for @closeDocumentTab.
  ///
  /// In en, this message translates to:
  /// **'Close {title}'**
  String closeDocumentTab(Object title);
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) =>
      <String>['ar', 'en'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'ar':
      return AppLocalizationsAr();
    case 'en':
      return AppLocalizationsEn();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
