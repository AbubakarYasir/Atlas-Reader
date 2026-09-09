import 'dart:async';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:dart_pdf_editor/dart_pdf_editor.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:logging/logging.dart';

import '../../bookmark_grouping.dart';
import '../../bookmark_tree.dart';
import '../../core/accessibility/accessibility_announcer.dart';
import '../../core/file_system/windows_document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../../features/command_center/command_center_search_field.dart';
import '../../features/command_center/command_center_overlay.dart';
import '../../features/command_center/command_center_shortcuts.dart';
import '../../features/reader/bookmark_composer.dart';
import '../../features/reader/pdf_reader_screen.dart';
import '../../features/settings/settings_screen.dart';
import '../../pdf_engine.dart';
import '../../scanned_pdf.dart';
import '../../sync_engine.dart';
import '../../sync_diff.dart';
import '../../sync_models.dart';
import '../../widgets/accessible_bookmark_tile.dart';
import 'library_files_screen.dart';
import 'library_hub_screen.dart';
import 'library_folder_manager.dart';
import 'library_overview.dart';

/// Global database instance
late final AppDatabase database;

class _CommitChangesIntent extends Intent {
  const _CommitChangesIntent();
}

class MyApp extends StatefulWidget {
  const MyApp({super.key, this.initialFilePath});

  /// Optional Windows launch argument used by Explorer's "Open with" flow.
  final String? initialFilePath;

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  static final _navigatorKey = GlobalKey<NavigatorState>();
  static const _fileSystem = WindowsDocumentFileSystem();
  Locale? _locale;
  ThemeMode _themeMode = ThemeMode.system;
  late final LibraryFolderManager _libraryManager = LibraryFolderManager(
    database: database,
    fileSystem: _fileSystem,
  );

  @override
  void initState() {
    super.initState();
    _libraryManager.onChanged = () {
      if (mounted) setState(() {});
    };
    unawaited(_libraryManager.startWatching());
    unawaited(_libraryManager.rescanAll());
    if (widget.initialFilePath != null) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        unawaited(_openPdfPath(widget.initialFilePath!));
      });
    }
  }

  @override
  void dispose() {
    unawaited(_libraryManager.dispose());
    super.dispose();
  }

  void _setLocale(Locale locale) => setState(() => _locale = locale);

  void _setThemeMode(ThemeMode mode) => setState(() => _themeMode = mode);

  Future<void> _openPdfPicker() async {
    final selection = await FilePicker.pickFile(
      dialogTitle: 'Open a PDF',
      type: FileType.custom,
      allowedExtensions: const ['pdf'],
    );
    final path = selection?.path;
    if (path != null) await _openPdfPath(path);
  }

  Future<void> _openPdfPath(String filePath, {int pageNumber = 1}) async {
    final navigator = _navigatorKey.currentState;
    final context = _navigatorKey.currentContext;
    if (navigator == null) return;
    if (!filePath.toLowerCase().endsWith('.pdf') ||
        !await _fileSystem.exists(filePath)) {
      if (context != null && context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('The selected PDF could not be opened.'),
          ),
        );
      }
      return;
    }

    unawaited(_rememberExternalPdf(filePath, pageNumber));

    await navigator.push<void>(
      MaterialPageRoute(
        builder: (_) => PdfReaderScreen(
          filePath: filePath,
          fileSystem: _fileSystem,
          database: database,
          initialPageNumber: pageNumber,
          onCreateBookmark: (draft) =>
              _createBookmarkFromReader(filePath, draft),
        ),
      ),
    );
    if (mounted) setState(() {});
  }

  Future<void> _rememberExternalPdf(String filePath, int pageNumber) async {
    try {
      final results = await Future.wait<Object?>([
        FileStat.stat(filePath),
        PdfEngine(fileSystem: _fileSystem).inspectForLibrary(filePath),
      ]);
      final stat = results[0]! as FileStat;
      final metadata =
          results[1]
              as ({
                int pageCount,
                int bookmarkCount,
                String? title,
                String? author,
              })?;
      await database.rememberExternalFile(
        ScannedPdf(
          filePath: filePath,
          fileName: BookmarkGrouping.fileNameFromPath(filePath),
          title: metadata?.title,
          author: metadata?.author,
          format: 'PDF',
          bookmarkCount: metadata?.bookmarkCount ?? 0,
          pageCount: metadata?.pageCount ?? 0,
          fileSizeBytes: stat.size,
          lastModified: stat.modified,
        ),
        currentPage: pageNumber,
      );
      if (mounted) setState(() {});
    } catch (_) {
      // Direct reading remains available even if the optional recent index
      // cannot inspect a damaged or access-restricted file.
    }
  }

  Future<void> _openBookmarkLibrary() async {
    final navigator = _navigatorKey.currentState;
    if (navigator == null) return;
    await navigator.push<void>(
      MaterialPageRoute(
        builder: (_) => LibraryScreen(onLocaleChanged: _setLocale),
      ),
    );
    if (mounted) setState(() {});
  }

  Future<void> _openCommandCenter() async {
    final context = _navigatorKey.currentContext;
    if (context == null) return;

    await showDialog<void>(
      context: context,
      builder: (dialogContext) => CommandCenterOverlay(
        database: database,
        onSelected: (result) {
          Navigator.of(dialogContext).pop();
          _openSearchResult(result);
        },
      ),
    );
  }

  Future<void> _openSearchResult(CommandCenterResult result) async {
    final context = _navigatorKey.currentContext;
    if (!await _fileSystem.exists(result.filePath)) {
      if (context != null && context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('The PDF file could not be found.')),
        );
      }
      return;
    }
    await _openPdfPath(result.filePath, pageNumber: result.pageNumber ?? 1);
  }

  Future<void> _createBookmarkFromReader(
    String filePath,
    ReaderBookmarkDraft draft,
  ) async {
    // The PDF is authoritative. Commit its outline safely first so a file
    // replacement failure can never leave a local-only bookmark that appears
    // saved but disappears in every external reader.
    final output = await PdfEngine(fileSystem: _fileSystem).injectBookmark(
      filePath,
      draft.title,
      draft.pageNumber - 1,
      description: draft.description,
    );
    if (output == null) {
      throw StateError('The bookmark could not be written to the PDF.');
    }

    final bookmarkId = await database.addBookmark(
      filePath: filePath,
      title: draft.title,
      pageIndex: draft.pageNumber - 1,
      description: draft.description?.isEmpty ?? true
          ? null
          : draft.description,
    );
    final tags = draft.tags?.split(',') ?? const <String>[];
    for (final tag
        in tags.map((tag) => tag.trim()).where((tag) => tag.isNotEmpty)) {
      await database.addTagToBookmark(bookmarkId, tag);
    }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      navigatorKey: _navigatorKey,
      onGenerateTitle: (context) => AppLocalizations.of(context)!.appTitle,
      locale: _locale,
      supportedLocales: AppLocalizations.supportedLocales,
      localizationsDelegates: const [
        AppLocalizations.delegate,
        DartPdfEditorLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
      ),
      darkTheme: ThemeData(
        brightness: Brightness.dark,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.deepPurple,
          brightness: Brightness.dark,
        ),
      ),
      themeMode: _themeMode,
      builder: (context, child) => CommandCenterShortcuts(
        onOpen: _openCommandCenter,
        child: child ?? const SizedBox.shrink(),
      ),
      home: LibraryHubScreen(
        database: database,
        fileSystem: _fileSystem,
        folderManager: _libraryManager,
        onCreateBookmark: _createBookmarkFromReader,
        onOpenPdf: _openPdfPicker,
        onOpenFile: _openPdfPath,
        onOpenAdvancedBookmarks: _openBookmarkLibrary,
        onOpenCommandCenter: _openCommandCenter,
        themeMode: _themeMode,
        onThemeModeChanged: _setThemeMode,
        onLocaleChanged: _setLocale,
      ),
    );
  }
}

class LibraryScreen extends StatefulWidget {
  const LibraryScreen({super.key, this.onLocaleChanged});

  final ValueChanged<Locale>? onLocaleChanged;

  @override
  State<LibraryScreen> createState() => _LibraryScreenState();
}

class _LibraryScreenState extends State<LibraryScreen> {
  final _log = Logger('Main');
  String? _selectedFilePath;
  String _status = 'Waiting';
  String _searchQuery = '';
  bool _isSyncing = false;
  bool _isPushing = false;
  List<BookmarkDiff> _syncDiffs = [];
  BookmarkSort _selectedSort = BookmarkSort.pageNumber;
  BookmarkViewGroup _selectedViewGroup = BookmarkViewGroup.book;

  final _bookmarkTitleController = TextEditingController();
  final _pageNumberController = TextEditingController();
  final _descriptionController = TextEditingController();
  final _tagsController = TextEditingController();
  final _fileSystem = const WindowsDocumentFileSystem();
  late final _pdfEngine = PdfEngine(fileSystem: _fileSystem);
  late final _syncEngine = SyncEngine(database, pdfEngine: _pdfEngine);
  late final LibraryFolderManager _folderManager = LibraryFolderManager(
    database: database,
    fileSystem: _fileSystem,
  );

  @override
  void initState() {
    super.initState();
    _folderManager.startWatching();
    _folderManager.rescanAll().then((report) {
      if (!mounted) return;
      setState(() {
        _status = report.foldersScanned == 0
            ? 'Waiting — add a library folder in Settings to scan books.'
            : 'Library ready: ${report.filesFound} books indexed.';
      });
    });
  }

  @override
  void dispose() {
    _bookmarkTitleController.dispose();
    _pageNumberController.dispose();
    _descriptionController.dispose();
    _tagsController.dispose();
    _folderManager.dispose();
    super.dispose();
  }

  Future<String?> _resolveMissingFile(String oldPath) async {
    if (await _fileSystem.exists(oldPath)) {
      return oldPath;
    }
    if (!mounted) return null;

    final locate = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('File Missing!'),
          content: Text(
            'The file is no longer at $oldPath. Would you like to locate its new home?',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: const Text('Locate'),
            ),
          ],
        );
      },
    );

    if (locate != true) {
      return null;
    }

    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf'],
    );

    if (result.isEmpty || result.single.path == null) {
      return null;
    }

    final newPath = result.single.path!;
    final updatedBookmarks = await database.updateFilePaths(oldPath, newPath);

    if (!mounted) {
      return null;
    }

    setState(() {
      _selectedFilePath = newPath;
      _status = 'File path relinked to $newPath';
    });

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            'Successfully updated $updatedBookmarks bookmarks to the new file path!',
          ),
          backgroundColor: Colors.green,
        ),
      );
    }

    await _calculateSyncDiff();
    return newPath;
  }

  Future<void> _pickPdf() async {
    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf'],
    );

    if (result.isEmpty || result.single.path == null) {
      return;
    }

    final initialPath = result.single.path!;
    final resolvedPath = await _resolveMissingFile(initialPath);
    if (resolvedPath == null) {
      return;
    }

    setState(() {
      _selectedFilePath = resolvedPath;
      _status = 'PDF selected: $_selectedFilePath';
      _syncDiffs = [];
    });

    // Calculate sync diff after file is selected
    await _calculateSyncDiff();
  }

  Future<void> _injectBookmark() async {
    final filePath = _selectedFilePath;
    final bookmarkTitle = _bookmarkTitleController.text.trim();
    final pageNumberText = _pageNumberController.text.trim();
    final description = _descriptionController.text.trim();
    final tagsText = _tagsController.text.trim();

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }
    if (bookmarkTitle.isEmpty) {
      setState(() => _status = 'Please enter a bookmark title.');
      return;
    }

    if (pageNumberText.isEmpty) {
      setState(() => _status = 'Please enter a page number.');
      return;
    }

    final pageNumber = int.tryParse(pageNumberText);
    if (pageNumber == null || pageNumber < 1) {
      setState(() => _status = 'Page number must be a positive integer.');
      return;
    }

    try {
      final pageCount = await _pdfEngine.getPageCount(filePath);
      if (pageCount == null) {
        setState(() => _status = 'Could not read PDF file.');
        return;
      }
      if (pageNumber > pageCount) {
        setState(
          () =>
              _status = 'Page $pageNumber exceeds document pages ($pageCount).',
        );
        return;
      }

      // STEP 1: Dual-Layer Save - Instantly save to local database
      final bookmarkId = await database.addBookmark(
        filePath: filePath,
        title: bookmarkTitle,
        pageIndex: pageNumber - 1,
        description: description.isNotEmpty ? description : null,
      );

      // Add tags if provided
      if (tagsText.isNotEmpty) {
        final tags = tagsText
            .split(',')
            .map((t) => t.trim())
            .where((t) => t.isNotEmpty)
            .toList();
        for (final tag in tags) {
          await database.addTagToBookmark(bookmarkId, tag);
        }
      }

      if (!mounted) return;

      // Show instant UI feedback
      setState(() {
        _status = 'Saved to local DB!';
        _bookmarkTitleController.clear();
        _pageNumberController.clear();
        _descriptionController.clear();
        _tagsController.clear();
      });
      AccessibilityAnnouncer.announce(context, 'Bookmark saved locally.');

      // STEP 2: Background injection
      _pdfEngine
          .injectBookmark(
            filePath,
            bookmarkTitle,
            pageNumber - 1,
            description: description.isNotEmpty ? description : null,
          )
          .then((outputPath) {
            if (!mounted) return;
            if (outputPath != null) {
              setState(() {
                _status = '✓ Synced to PDF! Bookmark embedded.';
              });
              AccessibilityAnnouncer.announce(
                context,
                'Bookmark saved and embedded in the PDF.',
              );
            } else {
              setState(() {
                _status =
                    '⚠ Local DB saved, but PDF injection failed. Check console.';
              });
              AccessibilityAnnouncer.announce(
                context,
                'Bookmark was saved locally, but could not be written to the PDF.',
              );
            }
          })
          .catchError((e) {
            if (!mounted) return;
            setState(() {
              _status = '⚠ Error: $e (Check console for details)';
            });
          });
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');
    }
  }

  /// Dual-layer save for a bookmark created while browsing the library list.
  Future<void> _createBookmarkFromLibrary(
    String filePath,
    ReaderBookmarkDraft draft,
  ) async {
    final bookmarkId = await database.addBookmark(
      filePath: filePath,
      title: draft.title,
      pageIndex: draft.pageNumber - 1,
      description: draft.description?.isEmpty ?? true
          ? null
          : draft.description,
    );
    for (final tag
        in (draft.tags ?? '')
            .split(',')
            .map((t) => t.trim())
            .where((t) => t.isNotEmpty)) {
      await database.addTagToBookmark(bookmarkId, tag);
    }

    final output = await _pdfEngine.injectBookmark(
      filePath,
      draft.title,
      draft.pageNumber - 1,
      description: draft.description,
    );
    if (output == null) {
      throw StateError(
        'The bookmark was saved locally but could not be written to the PDF.',
      );
    }

    if (mounted) await _calculateSyncDiff();
  }

  Future<void> _openReader() async {
    final filePath = _selectedFilePath;
    if (filePath == null) return;

    final resolvedPath = await _resolveMissingFile(filePath);
    if (resolvedPath == null || !mounted) return;

    await Navigator.of(context).push<void>(
      MaterialPageRoute(
        builder: (_) => PdfReaderScreen(
          filePath: resolvedPath,
          fileSystem: _fileSystem,
          onCreateBookmark: (draft) async {
            _bookmarkTitleController.text = draft.title;
            _pageNumberController.text = draft.pageNumber.toString();
            _descriptionController.text = draft.description ?? '';
            _tagsController.text = draft.tags ?? '';
            await _injectBookmark();
          },
        ),
      ),
    );

    if (mounted) await _calculateSyncDiff();
  }

  Future<void> _syncFile() async {
    final filePath = _selectedFilePath;

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }

    final resolvedPath = await _resolveMissingFile(filePath);
    if (!mounted) return;
    if (resolvedPath == null) {
      setState(() {
        _isSyncing = false;
        _status = 'Sync canceled because the file could not be located.';
      });
      return;
    }

    setState(() {
      _isSyncing = true;
      _status = 'Syncing bookmarks...';
    });
    AccessibilityAnnouncer.announce(context, 'Syncing bookmarks.');

    try {
      final bookmarks = await _pdfEngine.extractBookmarks(resolvedPath);
      final newBookmarksCount = await database.syncBookmarkHierarchy(
        resolvedPath,
        bookmarks,
      );

      if (!mounted) return;

      setState(() {
        _status = 'Sync Complete: $newBookmarksCount new bookmarks found';
      });
      AccessibilityAnnouncer.announce(
        context,
        'Sync complete: $newBookmarksCount new bookmarks found.',
      );
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');
    } finally {
      if (mounted) {
        setState(() {
          _isSyncing = false;
        });
      }
    }
  }

  Future<void> _pushToFile() async {
    final filePath = _selectedFilePath;

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }

    final resolvedPath = await _resolveMissingFile(filePath);
    if (!mounted) return;
    if (resolvedPath == null) {
      setState(() {
        _isPushing = false;
        _status = 'Commit canceled because the file could not be located.';
      });
      return;
    }

    setState(() {
      _isPushing = true;
      _status = 'Committing changes to PDF...';
    });
    AccessibilityAnnouncer.announce(context, 'Saving bookmark changes to PDF.');

    // Show snackbar for sync progress
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Syncing changes...'),
          duration: Duration(seconds: 1),
        ),
      );
    }

    try {
      // Use SyncEngine for incremental reconciliation
      final result = await _syncEngine.reconcile(resolvedPath);

      if (!mounted) return;

      if (result != null) {
        final (adds, deletes) = result;
        setState(() {
          _status = 'Successfully committed $adds adds and $deletes deletes';
          _syncDiffs = [];
        });
        AccessibilityAnnouncer.announce(
          context,
          'Saved successfully: $adds additions and $deletes removals.',
        );

        // Show success snackbar
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              'Successfully committed $adds adds and $deletes deletes',
            ),
            backgroundColor: Colors.green,
          ),
        );
      } else {
        setState(() {
          _status = 'Failed to sync bookmarks to PDF.';
        });
        AccessibilityAnnouncer.announce(
          context,
          'Could not save bookmark changes to the PDF.',
        );

        // Show error snackbar
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Failed to sync bookmarks to PDF.'),
            backgroundColor: Colors.red,
          ),
        );
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');
      AccessibilityAnnouncer.announce(context, 'Error saving changes: $e');

      // Show error snackbar
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error: $e'), backgroundColor: Colors.red),
      );
    } finally {
      if (mounted) {
        setState(() {
          _isPushing = false;
        });
      }
    }
  }

  /// Calculate the differences between database and PDF bookmarks
  Future<void> _calculateSyncDiff() async {
    final filePath = _selectedFilePath;
    if (filePath == null) return;

    try {
      // Fetch database bookmarks for this file
      final dbBookmarks = await database.getBookmarksForFile(filePath);

      // Fetch PDF bookmarks
      final pdfBookmarks = await _pdfEngine.extractBookmarks(filePath);

      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: dbBookmarks,
        pdfExtracted: pdfBookmarks,
      );
      final byId = BookmarkTree.indexById(dbBookmarks);
      final diffs = <BookmarkDiff>[];

      for (final bookmark in dbBookmarks) {
        final path = BookmarkTree.pathForBookmark(bookmark, byId);
        final pathKey = BookmarkTree.pathKey(path);
        if (diff.toAddToPdf.contains(pathKey)) {
          diffs.add(
            BookmarkDiff(
              title: BookmarkTree.displayPath(path),
              pageIndex: bookmark.pageIndex,
              action: SyncAction.add,
            ),
          );
        }
      }

      for (final bookmark in pdfBookmarks) {
        final path = List<String>.from(bookmark['path'] as List);
        final pathKey = BookmarkTree.pathKey(path);
        if (diff.toDeleteFromPdf.contains(pathKey)) {
          diffs.add(
            BookmarkDiff(
              title: BookmarkTree.displayPath(path),
              pageIndex: bookmark['pageIndex'] as int?,
              action: SyncAction.delete,
            ),
          );
        }
      }

      if (mounted) {
        setState(() {
          _syncDiffs = diffs;
        });
      }
    } catch (e) {
      _log.warning('Error calculating sync diff: $e');
    }
  }

  Future<void> _showEditBookmarkDialog(Bookmark bookmark) async {
    if (bookmark.isFolder) {
      await _showRenameFolderDialog(bookmark);
      return;
    }
    final titleController = TextEditingController(text: bookmark.title);
    final pageController = TextEditingController(
      text: '${(bookmark.pageIndex ?? 0) + 1}',
    );
    final descriptionController = TextEditingController(
      text: bookmark.description ?? '',
    );
    final tagsController = TextEditingController();

    // Load existing tags
    final existingTags = await database.getTagsForBookmark(bookmark.id);
    tagsController.text = existingTags.map((t) => t.name).join(', ');

    if (!mounted) return;

    final saved = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Edit Bookmark'),
          content: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                TextField(
                  controller: titleController,
                  decoration: const InputDecoration(labelText: 'New Title'),
                ),
                const SizedBox(height: 12),
                TextField(
                  controller: pageController,
                  decoration: const InputDecoration(
                    labelText: 'New Page Number',
                  ),
                  keyboardType: TextInputType.number,
                  inputFormatters: [FilteringTextInputFormatter.digitsOnly],
                ),
                const SizedBox(height: 12),
                TextField(
                  controller: descriptionController,
                  decoration: const InputDecoration(
                    labelText: 'Description (Markdown comments)',
                    hintText: 'Optional: Add notes about this bookmark...',
                  ),
                  maxLines: 3,
                ),
                const SizedBox(height: 12),
                TextField(
                  controller: tagsController,
                  decoration: const InputDecoration(
                    labelText: 'Tags (comma-separated)',
                    hintText: 'Optional: project, important, research...',
                  ),
                ),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                final newTitle = titleController.text.trim();
                final newPageNumber = int.tryParse(pageController.text.trim());

                if (newTitle.isEmpty ||
                    newPageNumber == null ||
                    newPageNumber < 1) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(
                      content: Text(
                        'Please enter a valid title and page number.',
                      ),
                      backgroundColor: Colors.red,
                    ),
                  );
                  return;
                }

                Navigator.of(context).pop(true);
              },
              child: const Text('Save'),
            ),
          ],
        );
      },
    );

    if (saved != true || !mounted) {
      return;
    }

    final newTitle = titleController.text.trim();
    final newPageNumber = int.tryParse(pageController.text.trim());
    final newDescription = descriptionController.text.trim();
    final newTagsText = tagsController.text.trim();

    if (newTitle.isEmpty || newPageNumber == null || newPageNumber < 1) {
      return;
    }

    // Update bookmark basic info
    await database.updateBookmark(
      bookmark.id,
      newTitle,
      newPageNumber - 1,
      description: newDescription.isNotEmpty ? newDescription : null,
    );

    // Handle tags - clear existing and add new ones
    await database.clearTagsForBookmark(bookmark.id);

    // Add new tags
    if (newTagsText.isNotEmpty) {
      final newTags = newTagsText
          .split(',')
          .map((t) => t.trim())
          .where((t) => t.isNotEmpty)
          .toList();
      for (final tag in newTags) {
        await database.addTagToBookmark(bookmark.id, tag);
      }
    }

    if (!mounted) return;

    final messenger = ScaffoldMessenger.of(context);
    await _calculateSyncDiff();
    setState(() {});

    messenger.showSnackBar(
      const SnackBar(
        content: Text(
          'Bookmark edited locally. Remember to Commit Changes to update the PDF file!',
        ),
        backgroundColor: Colors.green,
      ),
    );
  }

  Future<void> _showRenameFolderDialog(Bookmark bookmark) async {
    final titleController = TextEditingController(text: bookmark.title);
    final renamed = await showDialog<bool>(
      context: context,
      builder: (dialogContext) => AlertDialog(
        title: const Text('Rename folder'),
        content: TextField(
          controller: titleController,
          autofocus: true,
          decoration: const InputDecoration(labelText: 'Folder title'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(dialogContext, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(dialogContext, true),
            child: const Text('Rename'),
          ),
        ],
      ),
    );
    final title = titleController.text.trim();
    titleController.dispose();
    if (renamed != true || title.isEmpty || !mounted) return;

    await database.renameBookmark(bookmark.id, title);
    if (!mounted) return;
    await _calculateSyncDiff();
    if (!mounted) return;
    setState(() {});
    AccessibilityAnnouncer.announce(context, '$title renamed.');
  }

  Future<({List<Bookmark> bookmarks, Map<int, List<Tag>> tagsByBookmarkId})>
  _loadBookmarksWithTags() async {
    final matches = await database.searchBookmarks(_searchQuery);
    final bookmarks = _searchQuery.isEmpty
        ? matches
        : BookmarkTree.includeWithAncestors(
            matches,
            await database.getAllBookmarks(),
          );
    final tagsByBookmarkId = await database.getTagsByBookmarkIds(
      bookmarks.map((b) => b.id),
    );
    return (bookmarks: bookmarks, tagsByBookmarkId: tagsByBookmarkId);
  }

  Future<void> _confirmRemoveBook(String filePath) async {
    final fileName = BookmarkGrouping.fileNameFromPath(filePath);
    final bookmarkCount = (await database.getBookmarksForFile(filePath)).length;

    if (!mounted) return;

    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Remove book from bookmarks?'),
        content: Text(
          'Remove all $bookmarkCount bookmark${bookmarkCount == 1 ? '' : 's'} '
          'for "$fileName" from your local list?\n\n'
          'This does not change the PDF file until you commit changes.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Remove all'),
          ),
        ],
      ),
    );

    if (confirmed != true || !mounted) return;

    final messenger = ScaffoldMessenger.of(context);
    await database.deleteBookmarksByFile(filePath);
    if (!mounted) return;

    await _calculateSyncDiff();
    setState(() {});
    messenger.showSnackBar(
      SnackBar(
        content: Text('Removed all bookmarks for $fileName'),
        backgroundColor: Colors.orange,
      ),
    );
  }

  Widget _buildBookmarkTile(
    Bookmark bookmark,
    List<Tag> tags, {
    int depth = 0,
    bool showFileName = true,
    String? titleOverride,
  }) {
    final fileName = BookmarkGrouping.fileNameFromPath(bookmark.filePath);

    return AccessibleBookmarkTile(
      contentPadding: EdgeInsetsDirectional.only(
        start: 16 + (depth * 20.0),
        end: 16,
      ),
      leading: bookmark.isFolder
          ? Icon(Icons.folder_outlined, color: Colors.amber[800], size: 20)
          : Icon(Icons.bookmark_outline, color: Colors.blue[700], size: 20),
      title: Text(titleOverride ?? bookmark.title),
      semanticLabel: _bookmarkSemanticLabel(bookmark, depth),
      semanticHint:
          'Press Enter to open. Press F2 to rename. Press Delete to remove.',
      onTap: bookmark.isFolder ? null : () => _showEditBookmarkDialog(bookmark),
      onRename: () => _showEditBookmarkDialog(bookmark),
      onDelete: () => _deleteBookmark(bookmark),
      subtitle: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (!bookmark.isFolder && bookmark.pageIndex != null)
            Text(
              showFileName
                  ? 'Page ${bookmark.pageIndex! + 1} • $fileName'
                  : 'Page ${bookmark.pageIndex! + 1}',
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            )
          else if (showFileName)
            Text(fileName, maxLines: 1, overflow: TextOverflow.ellipsis),
          if (bookmark.description != null && bookmark.description!.isNotEmpty)
            Text(
              bookmark.description!,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                fontSize: 12,
                color: Colors.grey[600],
                fontStyle: FontStyle.italic,
              ),
            ),
          if (tags.isNotEmpty)
            Wrap(
              spacing: 4,
              runSpacing: 4,
              children: tags.map((tag) {
                return Chip(
                  label: Text(tag.name, style: const TextStyle(fontSize: 10)),
                  backgroundColor: Colors.blue[100],
                  padding: const EdgeInsets.symmetric(
                    horizontal: 4,
                    vertical: 0,
                  ),
                  materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
                );
              }).toList(),
            ),
        ],
      ),
      trailing: bookmark.isFolder
          ? IconButton(
              icon: const Icon(Icons.delete_outline),
              tooltip: 'Remove folder and sub-bookmarks',
              onPressed: () => _deleteBookmark(bookmark),
            )
          : Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                IconButton(
                  icon: const Icon(Icons.edit),
                  onPressed: () async {
                    await _showEditBookmarkDialog(bookmark);
                  },
                ),
                IconButton(
                  icon: const Icon(Icons.delete),
                  onPressed: () => _deleteBookmark(bookmark),
                ),
              ],
            ),
    );
  }

  List<Widget> _buildBookmarkTreeNodes(
    List<BookmarkTreeNode> nodes,
    Map<int, List<Tag>> tagsByBookmarkId, {
    int depth = 0,
    bool showFileName = true,
  }) {
    return nodes.map((node) {
      final bookmark = node.bookmark;
      final hasChildren = node.children.isNotEmpty;

      if (!hasChildren) {
        return _buildBookmarkTile(
          bookmark,
          tagsByBookmarkId[bookmark.id] ?? [],
          depth: depth,
          showFileName: showFileName,
        );
      }

      return AccessibleExpansionTile(
        tilePadding: EdgeInsets.only(left: 8 + (depth * 20.0), right: 8),
        leading: Icon(
          bookmark.isFolder ? Icons.folder_outlined : Icons.bookmark_outline,
          color: bookmark.isFolder ? Colors.amber[800] : Colors.blue[700],
          size: 20,
        ),
        title: Text(bookmark.title),
        semanticLabel: _bookmarkSemanticLabel(bookmark, depth),
        semanticHint:
            'Use Right and Left arrows to expand or collapse. Press F2 to rename. Press Delete to remove.',
        onRename: () => _showEditBookmarkDialog(bookmark),
        onDelete: () => _deleteBookmark(bookmark),
        subtitle: !bookmark.isFolder && bookmark.pageIndex != null
            ? Text('Page ${bookmark.pageIndex! + 1}')
            : null,
        children: [
          if (!bookmark.isFolder)
            _buildBookmarkTile(
              bookmark,
              tagsByBookmarkId[bookmark.id] ?? [],
              depth: depth + 1,
              showFileName: showFileName,
            ),
          ..._buildBookmarkTreeNodes(
            node.children,
            tagsByBookmarkId,
            depth: depth + 1,
            showFileName: showFileName,
          ),
        ],
      );
    }).toList();
  }

  String _bookmarkSemanticLabel(Bookmark bookmark, int depth) {
    final kind = bookmark.isFolder ? 'folder' : 'bookmark';
    final page = bookmark.pageIndex == null
        ? 'no page'
        : 'page ${bookmark.pageIndex! + 1}';
    return '${bookmark.title}, level ${depth + 1}, $kind, $page';
  }

  Future<void> _deleteBookmark(Bookmark bookmark) async {
    await database.deleteBookmark(bookmark.id);
    if (!mounted) return;
    await _calculateSyncDiff();
    if (!mounted) return;
    setState(() {});
    AccessibilityAnnouncer.announce(context, '${bookmark.title} removed.');
  }

  Widget _buildGroupedBookmarksList(
    List<Bookmark> bookmarks,
    Map<int, List<Tag>> tagsByBookmarkId,
  ) {
    if (_selectedViewGroup == BookmarkViewGroup.flat) {
      final byId = BookmarkTree.indexById(bookmarks);
      final sorted = BookmarkGrouping.flatList(bookmarks);
      BookmarkGrouping.sortBookmarks(sorted, _selectedSort);

      return ListView.builder(
        itemCount: sorted.length,
        itemBuilder: (context, index) {
          final bookmark = sorted[index];
          return _buildBookmarkTile(
            bookmark,
            tagsByBookmarkId[bookmark.id] ?? [],
            depth: BookmarkTree.depth(bookmark, byId),
          );
        },
      );
    }

    if (_selectedViewGroup == BookmarkViewGroup.book) {
      final groups = BookmarkGrouping.groupByBook(bookmarks);

      return ListView(
        children: groups.entries.map((entry) {
          final headerLabel = BookmarkGrouping.bookGroupLabel(entry.key);
          final roots = BookmarkTree.rootsForFile(bookmarks, entry.key);
          final count = entry.value.length;

          return AccessibleExpansionTile(
            leading: const Icon(Icons.menu_book, size: 20),
            title: Text(headerLabel),
            subtitle: Text('$count bookmark${count == 1 ? '' : 's'}'),
            children: [
              AccessibleBookmarkTile(
                leading: Icon(Icons.delete_sweep, color: Colors.red[700]),
                title: const Text('Remove all bookmarks for this book'),
                onTap: () => _confirmRemoveBook(entry.key),
              ),
              ..._buildBookmarkTreeNodes(
                roots,
                tagsByBookmarkId,
                showFileName: false,
              ),
            ],
          );
        }).toList(),
      );
    }

    final groups = BookmarkGrouping.groupByTag(bookmarks, tagsByBookmarkId);
    final sortedGroups = BookmarkGrouping.sortedGroups(groups, _selectedSort);
    final byId = BookmarkTree.indexById(bookmarks);

    return ListView(
      children: sortedGroups.entries.map((entry) {
        final count = entry.value.length;

        return AccessibleExpansionTile(
          leading: const Icon(Icons.label, size: 20),
          title: Text(entry.key),
          subtitle: Text('$count bookmark${count == 1 ? '' : 's'}'),
          children: entry.value.map((bookmark) {
            final path = BookmarkTree.pathForBookmark(bookmark, byId);
            return _buildBookmarkTile(
              bookmark,
              tagsByBookmarkId[bookmark.id] ?? [],
              showFileName: true,
              titleOverride: path.length > 1
                  ? BookmarkTree.displayPath(path)
                  : bookmark.title,
            );
          }).toList(),
        );
      }).toList(),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Shortcuts(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.keyS, control: true):
            _CommitChangesIntent(),
      },
      child: Actions(
        actions: {
          _CommitChangesIntent: CallbackAction<_CommitChangesIntent>(
            onInvoke: (_) {
              _pushToFile();
              return null;
            },
          ),
        },
        child: Focus(
          autofocus: true,
          child: Scaffold(
            appBar: AppBar(
              title: const Text('Bookmarks & PDF Sync'),
              actions: [
                IconButton(
                  tooltip: 'Browse library',
                  icon: const Icon(Icons.menu_book_outlined),
                  onPressed: () {
                    Navigator.of(context).push(
                      MaterialPageRoute<void>(
                        builder: (_) => LibraryFilesScreen(
                          database: database,
                          fileSystem: _fileSystem,
                          onCreateBookmark: _createBookmarkFromLibrary,
                          onSelectPdf: (path) {
                            setState(() {
                              _selectedFilePath = path;
                            });
                            _calculateSyncDiff();
                          },
                        ),
                      ),
                    );
                  },
                ),
                IconButton(
                  tooltip: 'Settings',
                  icon: const Icon(Icons.settings_outlined),
                  onPressed: () => Navigator.of(context).push(
                    MaterialPageRoute<void>(
                      builder: (_) => SettingsScreen(
                        database: database,
                        manager: _folderManager,
                        onLocaleChanged: widget.onLocaleChanged ?? (_) {},
                      ),
                    ),
                  ),
                ),
              ],
            ),
            body: Column(
              children: [
                Expanded(
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.all(24),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        LibraryOverview(
                          filePath: _selectedFilePath,
                          onSelectPdf: _pickPdf,
                          onOpenReader: _openReader,
                        ),
                        const SizedBox(height: 24),
                        BookmarkComposer(
                          titleController: _bookmarkTitleController,
                          pageController: _pageNumberController,
                          descriptionController: _descriptionController,
                          tagsController: _tagsController,
                          onInject: _injectBookmark,
                          onSync: _syncFile,
                          onCommit: _pushToFile,
                          isSyncing: _isSyncing,
                          isPushing: _isPushing,
                          canCommit: _syncDiffs.isNotEmpty,
                        ),
                        const SizedBox(height: 32),
                        // Sync Preview Section
                        if (_selectedFilePath != null &&
                            _syncDiffs.isNotEmpty) ...[
                          Material(
                            color: Colors.grey[200],
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(8),
                              side: BorderSide(color: Colors.grey[400]!),
                            ),
                            child: Padding(
                              padding: const EdgeInsets.all(16),
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Row(
                                    mainAxisAlignment:
                                        MainAxisAlignment.spaceBetween,
                                    children: [
                                      const Text(
                                        'Sync Preview',
                                        style: TextStyle(
                                          fontSize: 16,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                      Row(
                                        children: [
                                          Container(
                                            padding: const EdgeInsets.symmetric(
                                              horizontal: 8,
                                              vertical: 4,
                                            ),
                                            decoration: BoxDecoration(
                                              color: Colors.green[100],
                                              borderRadius:
                                                  BorderRadius.circular(4),
                                            ),
                                            child: Text(
                                              'Pending Adds: ${_syncDiffs.where((d) => d.action == SyncAction.add).length}',
                                              style: const TextStyle(
                                                color: Colors.green,
                                              ),
                                            ),
                                          ),
                                          const SizedBox(width: 8),
                                          Container(
                                            padding: const EdgeInsets.symmetric(
                                              horizontal: 8,
                                              vertical: 4,
                                            ),
                                            decoration: BoxDecoration(
                                              color: Colors.red[100],
                                              borderRadius:
                                                  BorderRadius.circular(4),
                                            ),
                                            child: Text(
                                              'Pending Deletions: ${_syncDiffs.where((d) => d.action == SyncAction.delete).length}',
                                              style: const TextStyle(
                                                color: Colors.red,
                                              ),
                                            ),
                                          ),
                                        ],
                                      ),
                                    ],
                                  ),
                                  const SizedBox(height: 12),
                                  SizedBox(
                                    height: 150,
                                    child: Builder(
                                      builder: (context) {
                                        final visibleDiffs = _syncDiffs
                                            .where(
                                              (d) =>
                                                  d.action != SyncAction.keep,
                                            )
                                            .toList();
                                        return ListView.builder(
                                          itemCount: visibleDiffs.length,
                                          itemBuilder: (context, index) {
                                            final diff = visibleDiffs[index];
                                            Color textColor;
                                            IconData icon;
                                            String actionText;

                                            switch (diff.action) {
                                              case SyncAction.add:
                                                textColor = Colors.green;
                                                icon = Icons.add_circle;
                                                actionText = 'ADD';
                                                break;
                                              case SyncAction.delete:
                                                textColor = Colors.red;
                                                icon = Icons.remove_circle;
                                                actionText = 'REMOVE';
                                                break;
                                              case SyncAction.keep:
                                                textColor = Colors.grey;
                                                icon = Icons.check_circle;
                                                actionText = 'KEEP';
                                                break;
                                            }

                                            return AccessibleBookmarkTile(
                                              dense: true,
                                              leading: Icon(
                                                icon,
                                                color: textColor,
                                                size: 20,
                                              ),
                                              title: Text(
                                                diff.title.isNotEmpty
                                                    ? diff.title
                                                    : '/',
                                                style: TextStyle(
                                                  color: textColor,
                                                  fontSize: 14,
                                                ),
                                              ),
                                              trailing: Text(
                                                diff.pageIndex != null
                                                    ? 'Page ${diff.pageIndex! + 1}'
                                                    : 'No page',
                                                style: const TextStyle(
                                                  fontSize: 12,
                                                ),
                                              ),
                                              subtitle: Text(
                                                actionText,
                                                style: TextStyle(
                                                  color: textColor,
                                                  fontSize: 11,
                                                  fontWeight: FontWeight.bold,
                                                ),
                                              ),
                                            );
                                          },
                                        );
                                      },
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ),
                          const SizedBox(height: 16),
                        ],
                        AccessibilityStatus(message: _status),
                      ],
                    ),
                  ),
                ),
                // Bookmarks ListView with FutureBuilder
                Material(
                  color: Colors.grey[100],
                  child: Column(
                    children: [
                      Padding(
                        padding: const EdgeInsets.fromLTRB(16, 16, 16, 0),
                        child: Row(
                          children: [
                            const Text(
                              'Saved Bookmarks',
                              style: TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                            const Spacer(),
                            Icon(Icons.sort, size: 18, color: Colors.grey[600]),
                            const SizedBox(width: 4),
                            DropdownButton<BookmarkSort>(
                              value: _selectedSort,
                              underline: const SizedBox.shrink(),
                              items: BookmarkSort.values.map((sort) {
                                return DropdownMenuItem(
                                  value: sort,
                                  child: Text(
                                    sort.label,
                                    style: const TextStyle(fontSize: 13),
                                  ),
                                );
                              }).toList(),
                              onChanged: (value) {
                                if (value == null) return;
                                setState(() => _selectedSort = value);
                              },
                            ),
                          ],
                        ),
                      ),
                      Padding(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 4,
                        ),
                        child: Row(
                          children: [
                            Icon(
                              Icons.view_list,
                              size: 18,
                              color: Colors.grey[600],
                            ),
                            const SizedBox(width: 8),
                            const Text(
                              'Group by:',
                              style: TextStyle(fontSize: 13),
                            ),
                            const SizedBox(width: 8),
                            Expanded(
                              child: SegmentedButton<BookmarkViewGroup>(
                                segments: BookmarkViewGroup.values.map((group) {
                                  return ButtonSegment(
                                    value: group,
                                    label: Text(
                                      group.label,
                                      style: const TextStyle(fontSize: 11),
                                    ),
                                    icon: Icon(switch (group) {
                                      BookmarkViewGroup.book => Icons.menu_book,
                                      BookmarkViewGroup.tag => Icons.label,
                                      BookmarkViewGroup.flat => Icons.list,
                                    }, size: 16),
                                  );
                                }).toList(),
                                selected: {_selectedViewGroup},
                                onSelectionChanged: (selection) {
                                  setState(
                                    () => _selectedViewGroup = selection.first,
                                  );
                                },
                              ),
                            ),
                          ],
                        ),
                      ),
                      // Command Center Search Bar
                      Padding(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 8,
                        ),
                        child: CommandCenterSearchField(
                          value: _searchQuery,
                          onChanged: (value) =>
                              setState(() => _searchQuery = value),
                        ),
                      ),
                      SizedBox(
                        height: 300,
                        child:
                            FutureBuilder<
                              ({
                                List<Bookmark> bookmarks,
                                Map<int, List<Tag>> tagsByBookmarkId,
                              })
                            >(
                              future: _loadBookmarksWithTags(),
                              builder: (context, snapshot) {
                                if (snapshot.connectionState ==
                                    ConnectionState.waiting) {
                                  return const Center(
                                    child: CircularProgressIndicator(),
                                  );
                                }

                                if (snapshot.hasError) {
                                  return Center(
                                    child: Text('Error: ${snapshot.error}'),
                                  );
                                }

                                final data = snapshot.data;
                                final bookmarks = data?.bookmarks ?? [];
                                final tagsByBookmarkId =
                                    data?.tagsByBookmarkId ?? {};

                                if (bookmarks.isEmpty) {
                                  return Center(
                                    child: Text(
                                      _searchQuery.isEmpty
                                          ? 'No bookmarks yet'
                                          : 'No bookmarks match "$_searchQuery"',
                                    ),
                                  );
                                }

                                return _buildGroupedBookmarksList(
                                  bookmarks,
                                  tagsByBookmarkId,
                                );
                              },
                            ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
