import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:syncfusion_flutter_pdfviewer/pdfviewer.dart';

import '../../core/accessibility/accessibility_announcer.dart';
import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../../pdf_engine.dart';
import '../research/annotations_drawer.dart';
import '../research/citation_generator.dart';
import '../research/research_selection_toolbar.dart';
import '../speed_reading/auto_scroll_overlay.dart';
import '../speed_reading/rsvp_speed_reader.dart';
import 'reader_models.dart';
import 'reader_outline_sidebar.dart';
import 'reader_scrub_bar.dart';
import 'reader_settings_dialog.dart';
import 'reader_thumbnail_jumper.dart';

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

/// Full-screen PDF reader with continuous scrolling, dual interface (Basic vs
/// Research mode), outline sidebar, annotations drawer, visual thumbnail jumper,
/// in-text markup (highlights, notes), citation generator, and margin cropping.
class PdfReaderScreen extends StatefulWidget {
  const PdfReaderScreen({
    super.key,
    required this.filePath,
    required this.fileSystem,
    required this.onCreateBookmark,
    this.initialPageNumber = 1,
    this.database,
  });

  final String filePath;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(ReaderBookmarkDraft draft) onCreateBookmark;
  final int initialPageNumber;
  final AppDatabase? database;

  @override
  State<PdfReaderScreen> createState() => _PdfReaderScreenState();
}

class _PdfReaderScreenState extends State<PdfReaderScreen> {
  final PdfViewerController _viewerController = PdfViewerController();
  late final Future<List<int>> _documentBytes;
  late final PdfEngine _pdfEngine;
  AppDatabase? _localDatabase;

  late var _activePage = widget.initialPageNumber;
  var _pageCount = 0;
  var _savingBookmark = false;
  var _showOutlineSidebar = false;
  var _showAnnotationsDrawer = false;
  var _showAutoScroll = false;
  double _autoScrollDistance = 0;

  String? _selectedText;
  ReaderPreferences _preferences = const ReaderPreferences();
  List<ReaderOutlineItem> _outline = [];
  Set<int> _chapterPages = {};
  final Set<int> _bookmarkedPages = {};

  AppDatabase get _db => widget.database ?? (_localDatabase ??= AppDatabase());

  @override
  void initState() {
    super.initState();
    _pdfEngine = PdfEngine(fileSystem: widget.fileSystem);
    _documentBytes = widget.fileSystem.readAsBytesInBackground(widget.filePath);
    _loadOutline();
  }

  Future<void> _loadOutline() async {
    try {
      final bookmarks = await _pdfEngine.extractBookmarks(widget.filePath);
      final items = <ReaderOutlineItem>[];
      final chapters = <int>{};

      for (final b in bookmarks) {
        final title = (b['title'] as String?) ?? 'Untitled';
        final pageIndex = (b['pageIndex'] as int?) ?? 0;
        final pageNum = pageIndex + 1;
        chapters.add(pageNum);
        items.add(ReaderOutlineItem(title: title, pageNumber: pageNum));
      }

      if (mounted) {
        setState(() {
          _outline = items;
          _chapterPages = chapters;
        });
      }
    } catch (_) {}
  }

  @override
  void dispose() {
    _viewerController.dispose();
    _localDatabase?.close();
    super.dispose();
  }

  void _jumpToPage(int pageNumber) {
    if (_pageCount <= 0) return;
    final target = pageNumber.clamp(1, _pageCount);
    _viewerController.jumpToPage(target);
    setState(() => _activePage = target);
    _db.updateReadingProgress(widget.filePath, target, pageCount: _pageCount);
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
        builder: (ctx, setDialogState) => ReaderSettingsDialog(
          preferences: _preferences,
          onChanged: (updated) {
            setDialogState(() => _preferences = updated);
            setState(() => _preferences = updated);
            if (updated.pageOffset != _preferences.pageOffset) {
              _db.updatePageOffset(widget.filePath, updated.pageOffset);
            }
          },
        ),
      ),
    );
  }

  void _openCitationDialog() {
    final printedPage = _activePage + _preferences.pageOffset;
    final fileName = widget.filePath.split(RegExp(r'[/\\]')).last;
    final title = fileName.replaceAll(RegExp(r'\.[^.]+$'), '');
    showDialog<void>(
      context: context,
      builder: (_) => CitationDialog(title: title, pageNumber: printedPage),
    );
  }

  Future<void> _openSpeedReader() async {
    final text = await _pdfEngine.extractPageText(
      widget.filePath,
      _activePage - 1,
    );
    if (!mounted) return;
    if (text.trim().isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('No selectable text was found on this page.'),
        ),
      );
      AccessibilityAnnouncer.announce(
        context,
        'No selectable text was found on this page.',
      );
      return;
    }
    await showDialog<void>(
      context: context,
      builder: (_) => RsvpSpeedReader(text: text),
    );
  }

  void _handleAutoScrollTick(double delta) {
    // Syncfusion exposes the current scroll position but not a public setter.
    // In page layouts, advancing after a measured reading distance is the
    // supported, hands-free equivalent and works for both page directions.
    _autoScrollDistance += delta;
    const pageAdvanceDistance = 680.0;
    if (_autoScrollDistance >= pageAdvanceDistance &&
        _activePage < _pageCount) {
      _autoScrollDistance = 0;
      _viewerController.nextPage();
    } else if (_autoScrollDistance <= -pageAdvanceDistance && _activePage > 1) {
      _autoScrollDistance = 0;
      _viewerController.previousPage();
    }
  }

  Future<void> _createAnnotation({
    required String type,
    String? note,
    String colorHex = '#FFE066',
  }) async {
    final text = _selectedText;
    if (text == null || text.isEmpty) return;

    await _db.addAnnotation(
      filePath: widget.filePath,
      pageNumber: _activePage,
      type: type,
      selectedText: text,
      note: note,
      colorHex: colorHex,
    );

    // Save directly into standard PDF annotation stream
    await _pdfEngine.addAnnotationToPdf(
      widget.filePath,
      pageIndex: _activePage - 1,
      type: type,
      text: text,
      note: note,
      colorHex: colorHex,
    );

    setState(() {
      _selectedText = null;
    });

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            '${type[0].toUpperCase()}${type.substring(1)} saved to PDF and database!',
          ),
        ),
      );
      AccessibilityAnnouncer.announce(context, '$type annotation saved');
    }
  }

  Future<void> _promptAddNote() async {
    final controller = TextEditingController();
    final note = await showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Add Sticky Note'),
        content: TextField(
          controller: controller,
          maxLines: 3,
          autofocus: true,
          decoration: const InputDecoration(
            hintText: 'Enter your research note...',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, controller.text.trim()),
            child: const Text('Save'),
          ),
        ],
      ),
    );

    if (note != null && note.isNotEmpty) {
      await _createAnnotation(type: 'note', note: note);
    }
  }

  ColorFilter? _getColorFilter(ReaderThemeMode theme) {
    switch (theme) {
      case ReaderThemeMode.day:
        return null;
      case ReaderThemeMode.warmParchment:
        return const ColorFilter.mode(Color(0x18795548), BlendMode.multiply);
      case ReaderThemeMode.night:
        return const ColorFilter.matrix([
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
        ]);
      case ReaderThemeMode.oled:
        return const ColorFilter.matrix([
          -1.0,
          0,
          0,
          0,
          255,
          0,
          -1.0,
          0,
          0,
          255,
          0,
          0,
          -1.0,
          0,
          255,
          0,
          0,
          0,
          1,
          0,
        ]);
    }
  }

  Color _getThemeBackgroundColor(ReaderThemeMode theme) {
    switch (theme) {
      case ReaderThemeMode.day:
        return const Color(0xFFFBFBFB);
      case ReaderThemeMode.warmParchment:
        return const Color(0xFFF7F1E5);
      case ReaderThemeMode.night:
        return const Color(0xFF1E1E1E);
      case ReaderThemeMode.oled:
        return Colors.black;
    }
  }

  Widget _wrapViewerWithMarginCrop(Widget child) {
    if (_preferences.marginCrop <= 0.0) return child;
    return ClipRect(
      child: Transform.scale(
        scale: 1.0 + (_preferences.marginCrop * 1.5),
        child: child,
      ),
    );
  }

  Future<void> _openBookmarkDialog() async {
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
                decoration: const InputDecoration(
                  labelText: 'Note (optional)',
                  border: OutlineInputBorder(),
                ),
                maxLines: 2,
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
      _bookmarkedPages.add(draft.pageNumber);
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Bookmark saved for page ${draft.pageNumber}.')),
      );
      AccessibilityAnnouncer.announce(
        context,
        'Bookmark saved for page ${draft.pageNumber}.',
      );
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Could not save bookmark: $error'),
          backgroundColor: Colors.red,
        ),
      );
      AccessibilityAnnouncer.announce(
        context,
        'Could not save bookmark: $error',
      );
    } finally {
      if (mounted) setState(() => _savingBookmark = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    final isResearch = _preferences.isResearchMode;
    final printedPageNum = _activePage + _preferences.pageOffset;

    final titleText = _preferences.pageOffset != 0
        ? 'Page $_activePage (Book p. $printedPageNum) of $_pageCount'
        : (_pageCount == 0
              ? strings.pageNumber(_activePage)
              : strings.pageNumberOf(_activePage, _pageCount));

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
                  _openBookmarkDialog();
                  return null;
                },
              ),
        },
        child: Focus(
          autofocus: true,
          child: Scaffold(
            appBar: AppBar(
              title: Text(titleText),
              actions: [
                if (isResearch) ...[
                  IconButton(
                    tooltip: 'Table of Contents',
                    icon: Icon(
                      _showOutlineSidebar
                          ? Icons.format_list_bulleted
                          : Icons.menu_book,
                    ),
                    onPressed: () => setState(() {
                      _showOutlineSidebar = !_showOutlineSidebar;
                      if (_showOutlineSidebar) _showAnnotationsDrawer = false;
                    }),
                  ),
                  IconButton(
                    tooltip: 'Annotations & Notes',
                    icon: Icon(
                      _showAnnotationsDrawer ? Icons.draw : Icons.draw_outlined,
                    ),
                    onPressed: () => setState(() {
                      _showAnnotationsDrawer = !_showAnnotationsDrawer;
                      if (_showAnnotationsDrawer) _showOutlineSidebar = false;
                    }),
                  ),
                  IconButton(
                    tooltip: 'Cite Page',
                    icon: const Icon(Icons.format_quote_rounded),
                    onPressed: _openCitationDialog,
                  ),
                  IconButton(
                    tooltip: 'Go to page (Thumbnails)',
                    icon: const Icon(Icons.grid_on),
                    onPressed: _openThumbnailJumper,
                  ),
                  IconButton(
                    tooltip: 'Reading and Display settings',
                    icon: const Icon(Icons.tune_outlined),
                    onPressed: _openSettingsDialog,
                  ),
                  IconButton(
                    tooltip: _showAutoScroll
                        ? 'Stop hands-free auto-scroll'
                        : 'Start hands-free auto-scroll',
                    icon: Icon(
                      _showAutoScroll
                          ? Icons.pause_circle_outline
                          : Icons.play_circle_outline,
                    ),
                    onPressed: () => setState(() {
                      _showAutoScroll = !_showAutoScroll;
                      _autoScrollDistance = 0;
                    }),
                  ),
                  IconButton(
                    tooltip: 'Open speed reader for this page',
                    icon: const Icon(Icons.speed),
                    onPressed: _openSpeedReader,
                  ),
                ],
                IconButton(
                  tooltip: 'Zoom out',
                  icon: const Icon(Icons.zoom_out),
                  onPressed: () {
                    _viewerController.zoomLevel =
                        (_viewerController.zoomLevel - .25).clamp(1, 3);
                  },
                ),
                IconButton(
                  tooltip: 'Zoom in',
                  icon: const Icon(Icons.zoom_in),
                  onPressed: () {
                    _viewerController.zoomLevel =
                        (_viewerController.zoomLevel + .25).clamp(1, 3);
                  },
                ),
                IconButton(
                  tooltip: 'Bookmark active page (Ctrl+B)',
                  icon: _savingBookmark
                      ? const SizedBox.square(
                          dimension: 18,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.bookmark_add_outlined),
                  onPressed: _savingBookmark ? null : _openBookmarkDialog,
                ),
                IconButton(
                  tooltip: isResearch
                      ? 'Switch to Basic Mode'
                      : 'Switch to Research Mode',
                  icon: Icon(
                    isResearch ? Icons.desktop_windows : Icons.menu_open,
                  ),
                  color: isResearch
                      ? Theme.of(context).colorScheme.primary
                      : null,
                  onPressed: () {
                    setState(() {
                      _preferences = _preferences.copyWith(
                        isResearchMode: !isResearch,
                      );
                      if (!_preferences.isResearchMode) {
                        _showOutlineSidebar = false;
                        _showAnnotationsDrawer = false;
                      }
                    });
                  },
                ),
              ],
            ),
            body: Column(
              children: [
                Expanded(
                  child: Row(
                    children: [
                      if (isResearch && _showOutlineSidebar)
                        ReaderOutlineSidebar(
                          outline: _outline,
                          currentPage: _activePage,
                          onSelectPage: _jumpToPage,
                          onClose: () =>
                              setState(() => _showOutlineSidebar = false),
                        ),
                      Expanded(
                        child: Container(
                          color: _getThemeBackgroundColor(_preferences.theme),
                          child: Stack(
                            fit: StackFit.expand,
                            children: [
                              _wrapViewerWithMarginCrop(
                                ColorFiltered(
                                  colorFilter:
                                      _getColorFilter(_preferences.theme) ??
                                      const ColorFilter.mode(
                                        Colors.transparent,
                                        BlendMode.dst,
                                      ),
                                  child: FutureBuilder<List<int>>(
                                    future: _documentBytes,
                                    builder: (context, snapshot) {
                                      if (snapshot.connectionState !=
                                          ConnectionState.done) {
                                        return const Center(
                                          child: CircularProgressIndicator(),
                                        );
                                      }
                                      if (snapshot.hasError ||
                                          !snapshot.hasData) {
                                        return Center(
                                          child: Padding(
                                            padding: const EdgeInsets.all(24),
                                            child: Text(
                                              'Could not open this PDF: ${snapshot.error}',
                                            ),
                                          ),
                                        );
                                      }

                                      final isSingle =
                                          _preferences.mode ==
                                          ReadingMode.singlePage;

                                      return SfPdfViewer.memory(
                                        Uint8List.fromList(snapshot.data!),
                                        controller: _viewerController,
                                        enableDoubleTapZooming: true,
                                        enableTextSelection: true,
                                        pageLayoutMode: isSingle
                                            ? PdfPageLayoutMode.single
                                            : PdfPageLayoutMode.continuous,
                                        scrollDirection: isSingle
                                            ? PdfScrollDirection.horizontal
                                            : PdfScrollDirection.vertical,
                                        onTextSelectionChanged: (details) {
                                          setState(() {
                                            _selectedText = details.selectedText
                                                ?.trim();
                                          });
                                        },
                                        onPageChanged: (details) {
                                          setState(
                                            () => _activePage =
                                                details.newPageNumber,
                                          );
                                          _db.updateReadingProgress(
                                            widget.filePath,
                                            details.newPageNumber,
                                            pageCount: _pageCount,
                                          );
                                        },
                                        onDocumentLoaded: (details) {
                                          final targetPage = widget
                                              .initialPageNumber
                                              .clamp(
                                                1,
                                                details.document.pages.count,
                                              );
                                          _viewerController.jumpToPage(
                                            targetPage,
                                          );
                                          setState(() {
                                            _activePage = targetPage;
                                            _pageCount =
                                                details.document.pages.count;
                                          });
                                        },
                                        onDocumentLoadFailed: (details) {
                                          AccessibilityAnnouncer.announce(
                                            context,
                                            'Could not render PDF: ${details.description}',
                                          );
                                          ScaffoldMessenger.of(
                                            context,
                                          ).showSnackBar(
                                            SnackBar(
                                              content: Text(
                                                'Could not render PDF: ${details.description}',
                                              ),
                                              backgroundColor: Colors.red,
                                            ),
                                          );
                                        },
                                      );
                                    },
                                  ),
                                ),
                              ),
                              if (_preferences.brightness < 1.0)
                                Positioned.fill(
                                  child: IgnorePointer(
                                    child: Container(
                                      color: Colors.black.withValues(
                                        alpha: 1.0 - _preferences.brightness,
                                      ),
                                    ),
                                  ),
                                ),
                              if (_selectedText != null &&
                                  _selectedText!.isNotEmpty)
                                Positioned(
                                  top: 16,
                                  left: 0,
                                  right: 0,
                                  child: Center(
                                    child: ResearchSelectionToolbar(
                                      selectedText: _selectedText!,
                                      onHighlight: (color) => _createAnnotation(
                                        type: 'highlight',
                                        colorHex: color,
                                      ),
                                      onUnderline: () => _createAnnotation(
                                        type: 'underline',
                                        colorHex: '#74C0FC',
                                      ),
                                      onStrikethrough: () => _createAnnotation(
                                        type: 'strikethrough',
                                        colorHex: '#FFA8A8',
                                      ),
                                      onAddNote: _promptAddNote,
                                      onCite: _openCitationDialog,
                                      onCopy: () {
                                        Clipboard.setData(
                                          ClipboardData(text: _selectedText!),
                                        );
                                        ScaffoldMessenger.of(
                                          context,
                                        ).showSnackBar(
                                          const SnackBar(
                                            content: Text(
                                              'Selected text copied',
                                            ),
                                          ),
                                        );
                                        setState(() => _selectedText = null);
                                      },
                                      onClose: () =>
                                          setState(() => _selectedText = null),
                                    ),
                                  ),
                                ),
                              if (_showAutoScroll)
                                Positioned(
                                  right: 20,
                                  bottom: 20,
                                  child: AutoScrollOverlay(
                                    onScrollTick: _handleAutoScrollTick,
                                    onClose: () => setState(() {
                                      _showAutoScroll = false;
                                      _autoScrollDistance = 0;
                                    }),
                                  ),
                                ),
                            ],
                          ),
                        ),
                      ),
                      if (isResearch && _showAnnotationsDrawer)
                        AnnotationsDrawer(
                          filePath: widget.filePath,
                          database: _db,
                          currentPage: _activePage,
                          onJumpToPage: _jumpToPage,
                          onClose: () =>
                              setState(() => _showAnnotationsDrawer = false),
                        ),
                    ],
                  ),
                ),
                ReaderScrubBar(
                  currentPage: _activePage,
                  pageCount: _pageCount,
                  chapterPages: _chapterPages,
                  isBasicMode: !isResearch,
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
}
