import 'dart:async';
import 'package:dart_pdf_editor/dart_pdf_editor.dart' as editor;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:syncfusion_flutter_pdfviewer/pdfviewer.dart' as syncfusion;

import '../../bookmark_tree.dart';
import '../../core/accessibility/accessibility_announcer.dart';
import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../../pdf_engine.dart';
import 'ink_toolbar.dart';
import 'reader_models.dart';
import 'reader_navigation_panel.dart';
import 'reader_outline_builder.dart';
import 'reader_outline_sidebar.dart';
import 'reader_scrub_bar.dart';
import 'reader_settings_dialog.dart';
import 'reader_thumbnail_jumper.dart';
import 'reader_zoom_controls.dart';

class ReaderBookmarkDraft {
  const ReaderBookmarkDraft({
    required this.title,
    required this.pageNumber,
    this.description,
    this.tags,
  });

  final String title;
  final int pageNumber;
  final String? description;
  final String? tags;
}

class _CreateReaderBookmarkIntent extends Intent {
  const _CreateReaderBookmarkIntent();
}

/// Focused PDF reader with a fast read-only surface and a separate byte-backed
/// writing surface. The editor maps pointer input to PDF points (72/inch),
/// flips Flutter's top-left Y axis into PDF bottom-left user space, and commits
/// one standard /Ink annotation per source page.
class PdfReaderScreen extends StatefulWidget {
  const PdfReaderScreen({
    super.key,
    required this.filePath,
    required this.fileSystem,
    required this.onCreateBookmark,
    this.initialPageNumber = 1,
    this.database,
    this.onPageChanged,
  });

  final String filePath;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(ReaderBookmarkDraft draft) onCreateBookmark;
  final int initialPageNumber;
  final AppDatabase? database;
  final ValueChanged<int>? onPageChanged;

  @override
  State<PdfReaderScreen> createState() => _PdfReaderScreenState();
}

class _PdfReaderScreenState extends State<PdfReaderScreen> {
  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey<ScaffoldState>();
  final syncfusion.PdfViewerController _viewerController =
      syncfusion.PdfViewerController();
  late Future<List<int>> _documentBytes;
  late final PdfEngine _pdfEngine;
  AppDatabase? _localDatabase;

  editor.PdfEditingController? _editingController;
  editor.PdfViewerController? _editingViewerController;
  editor.PdfDocument? _thumbnailDocument;
  int _savedEditingRevision = 0;
  bool _savingInk = false;

  late int _activePage = widget.initialPageNumber;
  int _pageCount = 0;
  bool _savingBookmark = false;
  bool _showNavigationPanel = true;
  int _annotationRevision = 0;
  int _zoomPercent = 100;
  int _writingViewGeneration = 0;
  ReaderZoomPreset _zoomPreset = ReaderZoomPreset.fitWidth;

  ReaderPreferences _preferences = const ReaderPreferences();
  List<ReaderOutlineItem> _outline = [];
  Set<int> _chapterPages = {};
  final Set<int> _bookmarkedPages = {};

  AppDatabase get _db => widget.database ?? (_localDatabase ??= AppDatabase());
  bool get _isWriting => _editingController != null;

  @override
  void initState() {
    super.initState();
    _pdfEngine = PdfEngine(fileSystem: widget.fileSystem);
    _documentBytes = widget.fileSystem.readAsBytesInBackground(widget.filePath);
    _viewerController.addListener(_handleReadViewport);
    unawaited(_loadThumbnailDocument());
    unawaited(_loadOutline());
    unawaited(_indexExistingInk());
  }

  @override
  void dispose() {
    _viewerController.removeListener(_handleReadViewport);
    _viewerController.dispose();
    _disposeEditingSession();
    _localDatabase?.close();
    super.dispose();
  }

  Future<void> _loadOutline() async {
    try {
      final results = await Future.wait<Object>([
        _pdfEngine.extractBookmarks(widget.filePath),
        _db.getBookmarksForFile(widget.filePath),
      ]);
      final pdfBookmarks = results[0] as List<Map<String, dynamic>>;
      final localBookmarks = results[1] as List<Bookmark>;
      final records = <ReaderOutlineRecord>[];

      for (final item in pdfBookmarks) {
        final rawPath = item['path'];
        final path = rawPath is List
            ? rawPath.map((part) => part.toString()).toList(growable: false)
            : <String>[(item['title'] as String?) ?? 'Untitled'];
        final pageIndex = item['pageIndex'] as int?;
        records.add(
          ReaderOutlineRecord(
            path: path,
            pageNumber: pageIndex == null ? null : pageIndex + 1,
          ),
        );
      }

      final byId = BookmarkTree.indexById(localBookmarks);
      for (final bookmark in localBookmarks) {
        records.add(
          ReaderOutlineRecord(
            path: BookmarkTree.pathForBookmark(bookmark, byId),
            pageNumber: bookmark.pageIndex == null
                ? null
                : bookmark.pageIndex! + 1,
            isUserBookmark: true,
          ),
        );
      }

      final outline = buildReaderOutline(records);
      final chapterPages = <int>{};
      void collectPages(List<ReaderOutlineItem> items) {
        for (final item in items) {
          if (item.pageNumber != null) chapterPages.add(item.pageNumber!);
          collectPages(item.children);
        }
      }

      collectPages(outline);

      if (!mounted) return;
      setState(() {
        _outline = outline;
        _chapterPages = chapterPages;
        _bookmarkedPages
          ..clear()
          ..addAll(
            localBookmarks
                .where((bookmark) => bookmark.pageIndex != null)
                .map((bookmark) => bookmark.pageIndex! + 1),
          );
      });
    } catch (_) {
      // A malformed outline must never prevent the document itself opening.
    }
  }

  Future<void> _loadThumbnailDocument() async {
    try {
      final bytes = Uint8List.fromList(await _documentBytes);
      final document = editor.PdfDocument.open(bytes);
      if (!mounted) return;
      setState(() {
        _thumbnailDocument = document;
        if (_pageCount == 0) _pageCount = document.pageCount;
      });
    } catch (_) {
      // Page navigation remains available with lightweight placeholders when
      // a malformed document cannot be parsed by the thumbnail renderer.
    }
  }

  Future<void> _indexExistingInk() async {
    try {
      final bytes = Uint8List.fromList(await _documentBytes);
      await _replaceInkIndex(bytes);
    } catch (_) {
      // The PDF remains usable even if its optional local search index fails.
    }
  }

  Future<void> _replaceInkIndex(Uint8List bytes) async {
    final entries = await _pdfEngine.extractInkAnnotationIndex(bytes);
    await _db.replaceInkAnnotationIndex(
      widget.filePath,
      entries
          .map(
            (entry) => (
              pageNumber: entry.pageNumber,
              type: entry.type,
              annotationName: entry.annotationName,
              colorHex: entry.colorHex,
              left: entry.left,
              bottom: entry.bottom,
              width: entry.width,
              height: entry.height,
            ),
          )
          .toList(growable: false),
    );
    if (mounted) setState(() => _annotationRevision++);
  }

  void _jumpToPage(int pageNumber) {
    if (_pageCount <= 0) return;
    final target = pageNumber.clamp(1, _pageCount);
    if (_isWriting) {
      unawaited(_editingViewerController?.jumpToPage(target - 1));
    } else {
      _viewerController.jumpToPage(target);
    }
    setState(() => _activePage = target);
    widget.onPageChanged?.call(target);
    unawaited(
      _db.updateReadingProgress(widget.filePath, target, pageCount: _pageCount),
    );
  }

  void _handleReadViewport() {
    if (!mounted || _isWriting) return;
    final percent = (_viewerController.zoomLevel * 100).round();
    if (percent != _zoomPercent) setState(() => _zoomPercent = percent);
  }

  void _changeZoom(double delta) {
    final editingViewer = _editingViewerController;
    if (_isWriting && editingViewer != null) {
      final zoom = (editingViewer.zoom + delta).clamp(.5, 4).toDouble();
      editingViewer.setZoom(zoom);
      setState(() {
        _zoomPreset = ReaderZoomPreset.custom;
        _zoomPercent = (zoom * 100).round();
      });
      return;
    }
    final zoom = (_viewerController.zoomLevel + delta).clamp(1, 4).toDouble();
    _viewerController.zoomLevel = zoom;
    setState(() {
      _zoomPreset = ReaderZoomPreset.custom;
      _zoomPercent = (zoom * 100).round();
    });
  }

  void _applyZoomSelection((ReaderZoomPreset, int?) selection) {
    final (preset, requestedPercent) = selection;
    if (preset == ReaderZoomPreset.custom && requestedPercent != null) {
      final minimum = _isWriting ? 50 : 100;
      final percent = requestedPercent.clamp(minimum, 400).toInt();
      final zoom = percent / 100;
      if (_isWriting) {
        _editingViewerController?.setZoom(zoom);
      } else {
        _viewerController.zoomLevel = zoom;
      }
      setState(() {
        _zoomPreset = ReaderZoomPreset.custom;
        _zoomPercent = percent;
      });
      return;
    }

    setState(() {
      _zoomPreset = preset;
      _zoomPercent = 100;
      if (_isWriting) _writingViewGeneration++;
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      if (!_isWriting) {
        _viewerController.zoomLevel = 1;
        _viewerController.jumpToPage(_activePage);
      }
    });
  }

  void _openThumbnailJumper() {
    showDialog<void>(
      context: context,
      builder: (_) => ReaderThumbnailJumper(
        currentPage: _activePage,
        pageCount: _pageCount,
        chapterPages: _chapterPages,
        bookmarkedPages: _bookmarkedPages,
        pageOffset: _preferences.pageOffset,
        onPageSelected: _jumpToPage,
      ),
    );
  }

  void _openSettingsDialog() {
    showDialog<void>(
      context: context,
      builder: (_) => StatefulBuilder(
        builder: (context, setDialogState) => ReaderSettingsDialog(
          preferences: _preferences,
          onChanged: (updated) {
            final previousPageOffset = _preferences.pageOffset;
            setDialogState(() => _preferences = updated);
            setState(() => _preferences = updated);
            if (updated.pageOffset != previousPageOffset) {
              unawaited(
                _db.updatePageOffset(widget.filePath, updated.pageOffset),
              );
            }
          },
        ),
      ),
    );
  }

  Future<void> _enterWritingMode() async {
    if (_isWriting) return;
    try {
      final bytes = Uint8List.fromList(await _documentBytes);
      if (!mounted) return;
      final editingController = editor.PdfEditingController(bytes);
      final editingViewer = editor.PdfViewerController();
      editingController.preferences.fingerDrawsInk = true;
      editingController.tool = editor.PdfEditTool.ink;
      editingViewer.addListener(_handleEditingViewport);
      setState(() {
        _editingController = editingController;
        _editingViewerController = editingViewer;
        _savedEditingRevision = editingController.revisionId;
      });
      WidgetsBinding.instance.addPostFrameCallback((_) {
        unawaited(editingViewer.jumpToPage(_activePage - 1));
        if (_zoomPreset == ReaderZoomPreset.custom) {
          editingViewer.setZoom(_zoomPercent / 100);
        }
      });
    } catch (error) {
      if (mounted) _showError('Could not start writing mode: $error');
    }
  }

  void _handleEditingViewport() {
    final viewer = _editingViewerController;
    if (viewer == null || !mounted || viewer.pageCount == 0) return;
    final page = viewer.currentPage + 1;
    if (page == _activePage && viewer.pageCount == _pageCount) return;
    setState(() {
      _activePage = page;
      _pageCount = viewer.pageCount;
      _zoomPercent = (viewer.zoom * 100).round();
    });
    widget.onPageChanged?.call(page);
    unawaited(
      _db.updateReadingProgress(
        widget.filePath,
        page,
        pageCount: viewer.pageCount,
      ),
    );
  }

  Future<bool> _saveInk() async {
    final controller = _editingController;
    if (controller == null || _savingInk) return false;
    controller.finishInk();
    if (controller.revisionId == _savedEditingRevision) return true;

    setState(() => _savingInk = true);
    try {
      final bytes = Uint8List.fromList(controller.bytes);
      await _pdfEngine.saveEditedPdfRevision(widget.filePath, bytes);
      await _replaceInkIndex(bytes);
      _documentBytes = Future.value(bytes);
      if (!mounted) return true;
      setState(() => _savedEditingRevision = controller.revisionId);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Ink saved safely into the PDF.')),
      );
      AccessibilityAnnouncer.announce(context, 'Ink saved into the PDF.');
      return true;
    } catch (error) {
      if (mounted) _showError(error.toString());
      return false;
    } finally {
      if (mounted) setState(() => _savingInk = false);
    }
  }

  Future<void> _leaveWritingMode() async {
    final controller = _editingController;
    if (controller == null) return;
    controller.finishInk();
    if (controller.revisionId != _savedEditingRevision) {
      final action = await showDialog<String>(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text('Save your writing?'),
          content: const Text(
            'Your pen and highlighter changes have not been saved into the PDF yet.',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context, 'cancel'),
              child: const Text('Keep editing'),
            ),
            TextButton(
              onPressed: () => Navigator.pop(context, 'discard'),
              child: const Text('Discard'),
            ),
            FilledButton(
              onPressed: () => Navigator.pop(context, 'save'),
              child: const Text('Save'),
            ),
          ],
        ),
      );
      if (action == null || action == 'cancel') return;
      if (action == 'save' && !await _saveInk()) return;
    }

    _disposeEditingSession();
    if (mounted) setState(() {});
  }

  void _disposeEditingSession() {
    _editingViewerController?.removeListener(_handleEditingViewport);
    _editingViewerController?.dispose();
    _editingController?.dispose();
    _editingViewerController = null;
    _editingController = null;
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message), backgroundColor: Colors.red),
    );
    AccessibilityAnnouncer.announce(context, message);
  }

  Future<void> _openBookmarkDialog() async {
    if (_isWriting) return;
    final titleController = TextEditingController();
    final descriptionController = TextEditingController();
    final tagsController = TextEditingController();
    final draft = await showDialog<ReaderBookmarkDraft>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Bookmark this page'),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Page $_activePage${_pageCount == 0 ? '' : ' of $_pageCount'}',
              ),
              const SizedBox(height: 12),
              TextField(
                controller: titleController,
                autofocus: true,
                decoration: const InputDecoration(
                  labelText: 'Bookmark title',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: descriptionController,
                maxLines: 2,
                decoration: const InputDecoration(
                  labelText: 'Description (optional)',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: tagsController,
                decoration: const InputDecoration(
                  labelText: 'Tags (optional, comma-separated)',
                  border: OutlineInputBorder(),
                ),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              final title = titleController.text.trim();
              if (title.isEmpty) return;
              Navigator.pop(
                context,
                ReaderBookmarkDraft(
                  title: title,
                  pageNumber: _activePage,
                  description: descriptionController.text.trim(),
                  tags: tagsController.text.trim(),
                ),
              );
            },
            child: const Text('Save bookmark'),
          ),
        ],
      ),
    );
    titleController.dispose();
    descriptionController.dispose();
    tagsController.dispose();
    if (draft == null || !mounted) return;

    setState(() => _savingBookmark = true);
    try {
      await widget.onCreateBookmark(draft);
      await _loadOutline();
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Bookmark saved for page ${draft.pageNumber}.')),
      );
      AccessibilityAnnouncer.announce(
        context,
        'Bookmark saved for page ${draft.pageNumber}.',
      );
    } catch (error) {
      if (mounted) _showError('Could not save bookmark: $error');
    } finally {
      if (mounted) setState(() => _savingBookmark = false);
    }
  }

  ColorFilter? _colorFilter(ReaderThemeMode theme) => switch (theme) {
    ReaderThemeMode.day => null,
    ReaderThemeMode.warmParchment => const ColorFilter.mode(
      Color(0x18795548),
      BlendMode.multiply,
    ),
    ReaderThemeMode.night => const ColorFilter.matrix([
      -0.85,
      0,
      0,
      0,
      230,
      0,
      -0.85,
      0,
      0,
      230,
      0,
      0,
      -0.85,
      0,
      230,
      0,
      0,
      0,
      1,
      0,
    ]),
    ReaderThemeMode.oled => const ColorFilter.matrix([
      -1,
      0,
      0,
      0,
      255,
      0,
      -1,
      0,
      0,
      255,
      0,
      0,
      -1,
      0,
      255,
      0,
      0,
      0,
      1,
      0,
    ]),
  };

  Color _backgroundColor(ReaderThemeMode theme) => switch (theme) {
    ReaderThemeMode.day => const Color(0xFFFBFBFB),
    ReaderThemeMode.warmParchment => const Color(0xFFF7F1E5),
    ReaderThemeMode.night => const Color(0xFF1E1E1E),
    ReaderThemeMode.oled => Colors.black,
  };

  Widget _withMarginCrop(Widget child) {
    if (_preferences.marginCrop <= 0) return child;
    return ClipRect(
      child: Transform.scale(
        scale: 1 + (_preferences.marginCrop * 1.5),
        child: child,
      ),
    );
  }

  ReaderNavigationPanel _navigationPanel({
    required VoidCallback onClose,
    ValueChanged<int>? onSelectPage,
    double width = 336,
  }) {
    return ReaderNavigationPanel(
      filePath: widget.filePath,
      database: _db,
      outline: _outline,
      currentPage: _activePage,
      pageCount: _pageCount,
      chapterPages: _chapterPages,
      bookmarkedPages: _bookmarkedPages,
      pageOffset: _preferences.pageOffset,
      annotationRevision: _annotationRevision,
      pagePreviewBuilder: _buildPagePreview,
      width: width,
      onSelectPage: onSelectPage ?? _jumpToPage,
      onClose: onClose,
    );
  }

  Widget _buildPagePreview(BuildContext context, int pageNumber) {
    final document = _thumbnailDocument;
    if (document == null || pageNumber < 1 || pageNumber > document.pageCount) {
      return const SizedBox.shrink();
    }
    final page = document.page(pageNumber - 1);
    final box = page.cropBox;
    final rotated = page.rotation == 90 || page.rotation == 270;
    final width = rotated ? box.height : box.width;
    final height = rotated ? box.width : box.height;
    return ClipRect(
      child: FittedBox(
        fit: BoxFit.contain,
        child: SizedBox(
          width: width,
          height: height,
          child: IgnorePointer(
            child: editor.PdfPageView(
              page: page,
              scale: .3,
              showAnnotations: true,
              qualityVisible: false,
              onScreen: true,
            ),
          ),
        ),
      ),
    );
  }

  Future<void> _openNavigationSheet() async {
    await showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      useSafeArea: true,
      builder: (sheetContext) => FractionallySizedBox(
        heightFactor: .88,
        child: _navigationPanel(
          width: MediaQuery.sizeOf(sheetContext).width,
          onClose: () => Navigator.of(sheetContext).pop(),
          onSelectPage: (page) {
            Navigator.of(sheetContext).pop();
            _jumpToPage(page);
          },
        ),
      ),
    );
  }

  Widget _buildReaderWorkspace(bool persistentNavigation) {
    final canvas = Expanded(
      child: Container(
        color: _backgroundColor(_preferences.theme),
        child: _isWriting ? _buildWritingSurface() : _buildReadingSurface(),
      ),
    );
    if (!persistentNavigation || !_showNavigationPanel) {
      return Row(children: [canvas]);
    }
    return Row(
      children: [
        _navigationPanel(
          onClose: () => setState(() => _showNavigationPanel = false),
        ),
        VerticalDivider(width: 1, color: Theme.of(context).dividerColor),
        canvas,
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    final compactActions =
        MediaQuery.sizeOf(context).width < 760 ||
        MediaQuery.textScalerOf(context).scale(16) >= 28;
    final persistentNavigation = MediaQuery.sizeOf(context).width >= 900;
    final printedPage = _activePage + _preferences.pageOffset;
    final title = _preferences.pageOffset != 0
        ? 'Page $_activePage (Book p. $printedPage) of $_pageCount'
        : _pageCount == 0
        ? strings.pageNumber(_activePage)
        : strings.pageNumberOf(_activePage, _pageCount);

    return Shortcuts(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.keyB, control: true):
            _CreateReaderBookmarkIntent(),
      },
      child: Actions(
        actions: {
          _CreateReaderBookmarkIntent:
              CallbackAction<_CreateReaderBookmarkIntent>(
                onInvoke: (_) {
                  unawaited(_openBookmarkDialog());
                  return null;
                },
              ),
        },
        child: Focus(
          autofocus: true,
          child: Scaffold(
            key: _scaffoldKey,
            appBar: AppBar(
              title: Text(title),
              actions: [
                IconButton(
                  tooltip: persistentNavigation && _showNavigationPanel
                      ? strings.hideNavigation
                      : strings.showNavigation,
                  onPressed: persistentNavigation
                      ? () => setState(
                          () => _showNavigationPanel = !_showNavigationPanel,
                        )
                      : _openNavigationSheet,
                  icon: Icon(
                    persistentNavigation && _showNavigationPanel
                        ? Icons.menu_open
                        : Icons.view_sidebar_outlined,
                  ),
                ),
                ReaderZoomControls(
                  percent: _zoomPercent,
                  preset: _zoomPreset,
                  compact: compactActions,
                  onZoomOut: () => _changeZoom(-.25),
                  onZoomIn: () => _changeZoom(.25),
                  onPresetSelected: _applyZoomSelection,
                ),
                if (!_isWriting) ...[
                  IconButton(
                    tooltip: 'Bookmark active page (Ctrl+B)',
                    onPressed: _savingBookmark ? null : _openBookmarkDialog,
                    icon: _savingBookmark
                        ? const SizedBox.square(
                            dimension: 18,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(Icons.bookmark_add_outlined),
                  ),
                  if (compactActions)
                    IconButton(
                      tooltip: strings.write,
                      onPressed: _enterWritingMode,
                      icon: const Icon(Icons.draw_outlined),
                    )
                  else
                    FilledButton.tonalIcon(
                      onPressed: _enterWritingMode,
                      icon: const Icon(Icons.draw_outlined),
                      label: Text(strings.write),
                    ),
                ],
                IconButton(
                  tooltip: 'Go to page',
                  onPressed: _openThumbnailJumper,
                  icon: const Icon(Icons.grid_on),
                ),
                IconButton(
                  tooltip: 'Reading and display settings',
                  onPressed: _openSettingsDialog,
                  icon: const Icon(Icons.tune_outlined),
                ),
                const SizedBox(width: 8),
              ],
            ),
            body: Column(
              children: [
                if (_isWriting)
                  InkToolbar(
                    controller: _editingController!,
                    saving: _savingInk,
                    savedRevisionId: _savedEditingRevision,
                    onSave: () => unawaited(_saveInk()),
                    onDone: () => unawaited(_leaveWritingMode()),
                  ),
                Expanded(child: _buildReaderWorkspace(persistentNavigation)),
                ReaderScrubBar(
                  currentPage: _activePage,
                  pageCount: _pageCount,
                  chapterPages: _chapterPages,
                  isBasicMode: false,
                  onPageChanged: _jumpToPage,
                  onTapJumper: _openThumbnailJumper,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildWritingSurface() {
    return _withDesktopPageGutters(
      RepaintBoundary(
        key: const ValueKey('ink-canvas-repaint-boundary'),
        child: editor.PdfEditorView(
          key: ValueKey('writing-${_zoomPreset.name}-$_writingViewGeneration'),
          controller: _editingController!,
          viewerController: _editingViewerController!,
          documentId: widget.filePath,
          showSaveButton: false,
          onSave: (_) => unawaited(_saveInk()),
          backgroundColor: _backgroundColor(_preferences.theme),
          pageLayout: const editor.PdfPageLayout.verticalContinuous(),
          initialFit: _zoomPreset == ReaderZoomPreset.fitPage
              ? editor.PdfViewerFit.page
              : editor.PdfViewerFit.width,
          features: const editor.PdfEditorFeatures(
            headerBar: false,
            search: false,
            searchResultsPanel: false,
            pageNumber: false,
            author: false,
            authorEditable: false,
            viewOptions: false,
            reflowView: false,
            pageColorEditable: false,
            thumbnails: false,
            bookmarks: false,
            pageEditing: false,
            annotationSidebar: false,
            annotationLibrary: false,
            propertiesPanel: false,
            toolbar: false,
            markup: false,
            undoRedo: true,
            colorControls: true,
            styleControls: true,
            flatten: false,
            colorProcessing: false,
            pencilEraserToggle: false,
            tools: {
              editor.PdfEditTool.select,
              editor.PdfEditTool.ink,
              editor.PdfEditTool.highlight,
              editor.PdfEditTool.eraser,
            },
          ),
        ),
      ),
    );
  }

  Widget _buildReadingSurface() {
    return _withDesktopPageGutters(
      Stack(
        fit: StackFit.expand,
        children: [
          _withMarginCrop(
            ColorFiltered(
              colorFilter:
                  _colorFilter(_preferences.theme) ??
                  const ColorFilter.mode(Colors.transparent, BlendMode.dst),
              child: FutureBuilder<List<int>>(
                future: _documentBytes,
                builder: (context, snapshot) {
                  if (snapshot.connectionState != ConnectionState.done) {
                    return const Center(child: CircularProgressIndicator());
                  }
                  if (snapshot.hasError || !snapshot.hasData) {
                    return Center(
                      child: Padding(
                        padding: const EdgeInsets.all(24),
                        child: Text(
                          'Could not open this PDF: ${snapshot.error}',
                        ),
                      ),
                    );
                  }
                  final single =
                      _zoomPreset == ReaderZoomPreset.fitPage ||
                      _preferences.mode == ReadingMode.singlePage;
                  return syncfusion.SfPdfViewer.memory(
                    Uint8List.fromList(snapshot.data!),
                    controller: _viewerController,
                    enableDoubleTapZooming: true,
                    enableTextSelection: true,
                    maxZoomLevel: 4,
                    pageLayoutMode: single
                        ? syncfusion.PdfPageLayoutMode.single
                        : syncfusion.PdfPageLayoutMode.continuous,
                    scrollDirection: single
                        ? syncfusion.PdfScrollDirection.horizontal
                        : syncfusion.PdfScrollDirection.vertical,
                    onPageChanged: (details) {
                      setState(() => _activePage = details.newPageNumber);
                      widget.onPageChanged?.call(details.newPageNumber);
                      unawaited(
                        _db.updateReadingProgress(
                          widget.filePath,
                          details.newPageNumber,
                          pageCount: _pageCount,
                        ),
                      );
                    },
                    onDocumentLoaded: (details) {
                      final target = widget.initialPageNumber.clamp(
                        1,
                        details.document.pages.count,
                      );
                      _viewerController.jumpToPage(target);
                      setState(() {
                        _activePage = target;
                        _pageCount = details.document.pages.count;
                      });
                      widget.onPageChanged?.call(target);
                    },
                    onDocumentLoadFailed: (details) {
                      _showError(
                        'Could not render PDF: ${details.description}',
                      );
                    },
                  );
                },
              ),
            ),
          ),
          if (_preferences.brightness < 1)
            Positioned.fill(
              child: IgnorePointer(
                child: ColoredBox(
                  color: Colors.black.withValues(
                    alpha: 1 - _preferences.brightness,
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  Widget _withDesktopPageGutters(Widget child) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final horizontal = constraints.maxWidth >= 1400
            ? 64.0
            : constraints.maxWidth >= 900
            ? 32.0
            : 8.0;
        return ColoredBox(
          color: switch (_preferences.theme) {
            ReaderThemeMode.day => const Color(0xFFE7E8ED),
            ReaderThemeMode.warmParchment => const Color(0xFFD8CDBB),
            ReaderThemeMode.night => const Color(0xFF202124),
            ReaderThemeMode.oled => Colors.black,
          },
          child: Padding(
            padding: EdgeInsets.symmetric(horizontal: horizontal, vertical: 8),
            child: child,
          ),
        );
      },
    );
  }
}
