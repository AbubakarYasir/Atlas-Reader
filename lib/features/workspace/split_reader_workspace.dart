import 'dart:async';
import 'dart:collection';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../reader/pdf_reader_screen.dart';
import '../reader/reader_models.dart';
import '../reader/reader_navigation_panel.dart';

class WorkspaceTabItem {
  WorkspaceTabItem({
    required this.filePath,
    required this.title,
    this.currentPage = 1,
    this.zoomPercent = 100,
    this.zoomPreset = ReaderZoomPreset.fitWidth,
    this.isWriting = false,
    this.navigationPanel = ReaderNavigationTab.outline,
    this.viewport,
  });

  final String filePath;
  final String title;
  int currentPage;
  double zoomPercent;
  ReaderZoomPreset zoomPreset;
  bool isWriting;
  ReaderNavigationTab navigationPanel;
  String? viewport;
  final ReaderSessionController sessionController = ReaderSessionController();
}

/// Focused desktop document workspace.
///
/// Reader instances stay mounted in an [IndexedStack], so changing tabs does
/// not discard the current page, zoom, navigation panel, or an active writing
/// session. Page changes are persisted after a short debounce to avoid turning
/// scroll events into database write bursts.
class SplitReaderWorkspace extends StatefulWidget {
  const SplitReaderWorkspace({
    super.key,
    required this.initialFilePath,
    required this.database,
    required this.fileSystem,
    required this.onCreateBookmark,
    this.initialPageNumber = 1,
  });

  final String initialFilePath;
  final int initialPageNumber;
  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;

  @override
  State<SplitReaderWorkspace> createState() => _SplitReaderWorkspaceState();
}

class _SplitReaderWorkspaceState extends State<SplitReaderWorkspace> {
  final List<WorkspaceTabItem> _tabs = [];
  int _activeTabIndex = 0;
  bool _loading = true;
  bool _restoreTabs = false;
  Timer? _persistDebounce;
  final LinkedHashSet<String> _loadedPaths = LinkedHashSet<String>();

  @override
  void initState() {
    super.initState();
    unawaited(_initWorkspace());
  }

  @override
  void dispose() {
    _persistDebounce?.cancel();
    super.dispose();
  }

  Future<void> _initWorkspace() async {
    _restoreTabs = await widget.database.getRestoreDocumentTabs();
    final savedTabs = _restoreTabs
        ? await widget.database.getReadingSessionTabs()
        : const <ReadingSessionTab>[];
    for (final saved in savedTabs) {
      if (await widget.fileSystem.exists(saved.filePath)) {
        _tabs.add(
          WorkspaceTabItem(
            filePath: saved.filePath,
            title: _titleFromPath(saved.filePath),
            currentPage: saved.pageNumber,
            zoomPercent: saved.zoomPercent,
            zoomPreset: ReaderZoomPreset.values.firstWhere(
              (value) => value.name == saved.zoomPreset,
              orElse: () => ReaderZoomPreset.fitWidth,
            ),
            isWriting: saved.readerMode == 'write',
            navigationPanel: ReaderNavigationTab.values.firstWhere(
              (value) => value.name == saved.navigationPanel,
              orElse: () => ReaderNavigationTab.outline,
            ),
            viewport: saved.viewport,
          ),
        );
      }
    }

    final requestedIndex = _tabs.indexWhere(
      (tab) => tab.filePath == widget.initialFilePath,
    );
    if (requestedIndex >= 0) {
      _activeTabIndex = requestedIndex;
      _tabs[requestedIndex].currentPage = widget.initialPageNumber;
    } else {
      _tabs.add(
        WorkspaceTabItem(
          filePath: widget.initialFilePath,
          title: _titleFromPath(widget.initialFilePath),
          currentPage: widget.initialPageNumber,
        ),
      );
      _activeTabIndex = _tabs.length - 1;
    }

    if (!mounted) return;
    _touchLoaded(_tabs[_activeTabIndex].filePath);
    setState(() => _loading = false);
    await _persistTabs();
  }

  String _titleFromPath(String path) {
    final name = path.split(RegExp(r'[/\\]')).last;
    return name.replaceAll(RegExp(r'\.[^.]+$'), '');
  }

  Future<void> _persistTabs() async {
    if (!_restoreTabs || _tabs.isEmpty) return;
    await widget.database.saveReadingSessionTabStates([
      for (var index = 0; index < _tabs.length; index++)
        (
          filePath: _tabs[index].filePath,
          pageNumber: _tabs[index].currentPage,
          isActive: index == _activeTabIndex,
          zoomPercent: _tabs[index].zoomPercent,
          zoomPreset: _tabs[index].zoomPreset.name,
          readerMode: _tabs[index].isWriting ? 'write' : 'read',
          navigationPanel: _tabs[index].navigationPanel.name,
          viewport: _tabs[index].viewport,
        ),
    ]);
  }

  void _schedulePersist() {
    _persistDebounce?.cancel();
    _persistDebounce = Timer(
      const Duration(milliseconds: 350),
      () => unawaited(_persistTabs()),
    );
  }

  Future<void> _openNewTab() async {
    final result = await FilePicker.pickFile(
      dialogTitle: AppLocalizations.of(context)!.openDocumentTab,
      type: FileType.custom,
      allowedExtensions: const ['pdf'],
    );
    final path = result?.path;
    if (path == null || !await widget.fileSystem.exists(path) || !mounted) {
      return;
    }

    setState(() {
      final existingIndex = _tabs.indexWhere((tab) => tab.filePath == path);
      if (existingIndex >= 0) {
        _activeTabIndex = existingIndex;
      } else {
        _tabs.add(
          WorkspaceTabItem(filePath: path, title: _titleFromPath(path)),
        );
        _activeTabIndex = _tabs.length - 1;
      }
      _touchLoaded(path);
    });
    await _persistTabs();
  }

  void _touchLoaded(String path) {
    _loadedPaths.remove(path);
    _loadedPaths.add(path);
    while (_loadedPaths.length > 3) {
      final eviction = _loadedPaths.cast<String?>().firstWhere((candidate) {
        if (candidate == null || candidate == path) return false;
        final tab = _tabs.firstWhere((tab) => tab.filePath == candidate);
        return tab.sessionController.snapshot?.hasUnsavedChanges != true;
      }, orElse: () => null);
      if (eviction == null) break;
      _loadedPaths.remove(eviction);
    }
  }

  void _activateTab(int index) {
    if (index == _activeTabIndex) return;
    final started = Stopwatch()..start();
    setState(() {
      _activeTabIndex = index;
      _touchLoaded(_tabs[index].filePath);
    });
    debugPrint(
      '[TABS] activated ${_tabs[index].filePath} in ${started.elapsedMicroseconds / 1000}ms',
    );
    _schedulePersist();
  }

  Future<void> _closeTab(int index) async {
    if (!await _tabs[index].sessionController.requestClose() || !mounted) {
      return;
    }
    if (_tabs.length == 1) {
      Navigator.of(context).maybePop();
      return;
    }
    setState(() {
      _loadedPaths.remove(_tabs[index].filePath);
      _tabs.removeAt(index);
      if (index < _activeTabIndex) {
        _activeTabIndex--;
      } else if (_activeTabIndex >= _tabs.length) {
        _activeTabIndex = _tabs.length - 1;
      }
    });
    _schedulePersist();
  }

  Future<void> _closeOtherTabs(int index) async {
    final keep = _tabs[index];
    for (final tab in List<WorkspaceTabItem>.from(_tabs)) {
      if (identical(tab, keep)) continue;
      if (!await tab.sessionController.requestClose() || !mounted) return;
    }
    setState(() {
      _tabs
        ..clear()
        ..add(keep);
      _activeTabIndex = 0;
      _loadedPaths
        ..clear()
        ..add(keep.filePath);
    });
    _schedulePersist();
  }

  Future<void> _closeTabsToRight(int index) async {
    final closing = _tabs.skip(index + 1).toList(growable: false);
    for (final tab in closing) {
      if (!await tab.sessionController.requestClose() || !mounted) return;
    }
    setState(() {
      for (final tab in closing) {
        _loadedPaths.remove(tab.filePath);
      }
      _tabs.removeRange(index + 1, _tabs.length);
      if (_activeTabIndex > index) _activeTabIndex = index;
      _touchLoaded(_tabs[_activeTabIndex].filePath);
    });
    _schedulePersist();
  }

  Future<void> _revealFile(String path) async {
    try {
      if (!Platform.isWindows) throw UnsupportedError('Windows only');
      await Process.run('explorer.exe', ['/select,$path']);
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not show the file: $error')),
      );
    }
  }

  Future<void> _copyPath(String path) async {
    await Clipboard.setData(ClipboardData(text: path));
    if (!mounted) return;
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('File path copied.')));
  }

  void _recordPage(int index, int page) {
    if (index >= _tabs.length || _tabs[index].currentPage == page) return;
    _tabs[index].currentPage = page;
    _schedulePersist();
  }

  void _recordSession(String path, ReaderSessionSnapshot snapshot) {
    final index = _tabs.indexWhere((tab) => tab.filePath == path);
    if (index < 0) return;
    final tab = _tabs[index];
    tab
      ..currentPage = snapshot.pageNumber
      ..zoomPercent = snapshot.zoomPercent
      ..zoomPreset = snapshot.zoomPreset
      ..isWriting = snapshot.isWriting
      ..navigationPanel = snapshot.navigationPanel
      ..viewport = snapshot.viewport;
    _schedulePersist();
  }

  @override
  Widget build(BuildContext context) {
    if (_loading || _tabs.isEmpty) {
      return const Scaffold(body: Center(child: CircularProgressIndicator()));
    }

    return Scaffold(
      body: Column(
        children: [
          _DocumentTabBar(
            tabs: _tabs,
            activeIndex: _activeTabIndex,
            onActivate: _activateTab,
            onClose: _closeTab,
            onCloseOthers: _closeOtherTabs,
            onCloseRight: _closeTabsToRight,
            onReveal: _revealFile,
            onCopyPath: _copyPath,
            onOpen: _openNewTab,
          ),
          Expanded(
            child: Stack(
              children: [
                for (final path in _loadedPaths)
                  Positioned.fill(
                    child: Offstage(
                      offstage: _tabs[_activeTabIndex].filePath != path,
                      child: TickerMode(
                        enabled: _tabs[_activeTabIndex].filePath == path,
                        child: Builder(
                          builder: (context) {
                            final tab = _tabs.firstWhere(
                              (candidate) => candidate.filePath == path,
                            );
                            return PdfReaderScreen(
                              key: ValueKey(path),
                              filePath: path,
                              fileSystem: widget.fileSystem,
                              database: widget.database,
                              initialPageNumber: tab.currentPage,
                              initialZoomPercent: tab.zoomPercent,
                              initialZoomPreset: tab.zoomPreset,
                              initialWriting: tab.isWriting,
                              initialNavigationPanel: tab.navigationPanel,
                              initialViewport: tab.viewport,
                              sessionController: tab.sessionController,
                              onPageChanged: (page) {
                                final index = _tabs.indexOf(tab);
                                if (index >= 0) _recordPage(index, page);
                              },
                              onSessionChanged: (snapshot) =>
                                  _recordSession(path, snapshot),
                              onCreateBookmark: (draft) =>
                                  widget.onCreateBookmark(path, draft),
                            );
                          },
                        ),
                      ),
                    ),
                  ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _DocumentTabBar extends StatelessWidget {
  const _DocumentTabBar({
    required this.tabs,
    required this.activeIndex,
    required this.onActivate,
    required this.onClose,
    required this.onCloseOthers,
    required this.onCloseRight,
    required this.onReveal,
    required this.onCopyPath,
    required this.onOpen,
  });

  final List<WorkspaceTabItem> tabs;
  final int activeIndex;
  final ValueChanged<int> onActivate;
  final Future<void> Function(int) onClose;
  final Future<void> Function(int) onCloseOthers;
  final Future<void> Function(int) onCloseRight;
  final Future<void> Function(String) onReveal;
  final Future<void> Function(String) onCopyPath;
  final VoidCallback onOpen;

  Future<void> _showTabMenu(
    BuildContext context,
    int index,
    Offset position,
  ) async {
    final strings = AppLocalizations.of(context)!;
    final overlay =
        Overlay.of(context).context.findRenderObject()! as RenderBox;
    final choice = await showMenu<_TabMenuAction>(
      context: context,
      position: RelativeRect.fromRect(
        Rect.fromLTWH(position.dx, position.dy, 0, 0),
        Offset.zero & overlay.size,
      ),
      items: [
        PopupMenuItem(value: _TabMenuAction.close, child: Text(strings.close)),
        PopupMenuItem(
          value: _TabMenuAction.closeOthers,
          enabled: tabs.length > 1,
          child: Text(strings.closeOtherTabs),
        ),
        PopupMenuItem(
          value: _TabMenuAction.closeRight,
          enabled: index < tabs.length - 1,
          child: Text(strings.closeTabsToRight),
        ),
        const PopupMenuDivider(),
        PopupMenuItem(
          value: _TabMenuAction.reveal,
          child: Text(strings.showInFileExplorer),
        ),
        PopupMenuItem(
          value: _TabMenuAction.copyPath,
          child: Text(strings.copyFullPath),
        ),
      ],
    );
    switch (choice) {
      case _TabMenuAction.close:
        await onClose(index);
      case _TabMenuAction.closeOthers:
        await onCloseOthers(index);
      case _TabMenuAction.closeRight:
        await onCloseRight(index);
      case _TabMenuAction.reveal:
        await onReveal(tabs[index].filePath);
      case _TabMenuAction.copyPath:
        await onCopyPath(tabs[index].filePath);
      case null:
        break;
    }
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    return Material(
      color: Theme.of(context).colorScheme.surfaceContainerHighest,
      child: SafeArea(
        bottom: false,
        child: SizedBox(
          height: 44,
          child: Row(
            children: [
              Expanded(
                child: ListView.builder(
                  key: const ValueKey('document-tab-strip'),
                  scrollDirection: Axis.horizontal,
                  itemCount: tabs.length,
                  itemBuilder: (context, index) {
                    final tab = tabs[index];
                    final selected = index == activeIndex;
                    return FocusableActionDetector(
                      shortcuts: const {
                        SingleActivator(LogicalKeyboardKey.f10, shift: true):
                            ActivateIntent(),
                        SingleActivator(LogicalKeyboardKey.contextMenu):
                            ActivateIntent(),
                      },
                      actions: {
                        ActivateIntent: CallbackAction<ActivateIntent>(
                          onInvoke: (_) {
                            final box =
                                context.findRenderObject() as RenderBox?;
                            if (box != null) {
                              _showTabMenu(
                                context,
                                index,
                                box.localToGlobal(
                                  box.size.bottomLeft(Offset.zero),
                                ),
                              );
                            }
                            return null;
                          },
                        ),
                      },
                      child: Semantics(
                        selected: selected,
                        button: true,
                        label: tab.title,
                        child: InkWell(
                          key: ValueKey('document-tab-${tab.filePath}'),
                          onTap: () => onActivate(index),
                          onSecondaryTapUp: (details) => _showTabMenu(
                            context,
                            index,
                            details.globalPosition,
                          ),
                          child: Container(
                            constraints: const BoxConstraints(
                              minWidth: 120,
                              maxWidth: 220,
                            ),
                            padding: const EdgeInsetsDirectional.only(
                              start: 12,
                            ),
                            decoration: BoxDecoration(
                              color: selected
                                  ? Theme.of(context).colorScheme.surface
                                  : Colors.transparent,
                              border: BorderDirectional(
                                end: BorderSide(
                                  color: Theme.of(context).dividerColor,
                                ),
                                bottom: BorderSide(
                                  color: selected
                                      ? Theme.of(context).colorScheme.primary
                                      : Colors.transparent,
                                  width: 3,
                                ),
                              ),
                            ),
                            child: Row(
                              children: [
                                const Icon(
                                  Icons.picture_as_pdf_outlined,
                                  size: 17,
                                ),
                                const SizedBox(width: 7),
                                Expanded(
                                  child: Text(
                                    tab.title,
                                    maxLines: 1,
                                    overflow: TextOverflow.ellipsis,
                                  ),
                                ),
                                IconButton(
                                  tooltip: strings.closeDocumentTab(tab.title),
                                  visualDensity: VisualDensity.compact,
                                  onPressed: () => onClose(index),
                                  icon: const Icon(Icons.close, size: 16),
                                ),
                              ],
                            ),
                          ),
                        ),
                      ),
                    );
                  },
                ),
              ),
              IconButton(
                tooltip: strings.openDocumentTab,
                onPressed: onOpen,
                icon: const Icon(Icons.add),
              ),
              const SizedBox(width: 4),
            ],
          ),
        ),
      ),
    );
  }
}

enum _TabMenuAction { close, closeOthers, closeRight, reveal, copyPath }
