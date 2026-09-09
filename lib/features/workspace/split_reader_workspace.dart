import 'dart:async';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../reader/pdf_reader_screen.dart';

class WorkspaceTabItem {
  WorkspaceTabItem({
    required this.filePath,
    required this.title,
    this.currentPage = 1,
  });

  final String filePath;
  final String title;
  int currentPage;
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
  Timer? _persistDebounce;

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
    final savedTabs = await widget.database.getReadingSessionTabs();
    for (final saved in savedTabs) {
      if (await widget.fileSystem.exists(saved.filePath)) {
        _tabs.add(
          WorkspaceTabItem(
            filePath: saved.filePath,
            title: _titleFromPath(saved.filePath),
            currentPage: saved.pageNumber,
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
    setState(() => _loading = false);
    await _persistTabs();
  }

  String _titleFromPath(String path) {
    final name = path.split(RegExp(r'[/\\]')).last;
    return name.replaceAll(RegExp(r'\.[^.]+$'), '');
  }

  Future<void> _persistTabs() async {
    if (_tabs.isEmpty) return;
    await widget.database.saveReadingSessionTabs([
      for (var index = 0; index < _tabs.length; index++)
        (
          filePath: _tabs[index].filePath,
          pageNumber: _tabs[index].currentPage,
          isActive: index == _activeTabIndex,
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
    });
    await _persistTabs();
  }

  void _activateTab(int index) {
    if (index == _activeTabIndex) return;
    setState(() => _activeTabIndex = index);
    _schedulePersist();
  }

  void _closeTab(int index) {
    if (_tabs.length == 1) {
      Navigator.of(context).maybePop();
      return;
    }
    setState(() {
      _tabs.removeAt(index);
      if (index < _activeTabIndex) {
        _activeTabIndex--;
      } else if (_activeTabIndex >= _tabs.length) {
        _activeTabIndex = _tabs.length - 1;
      }
    });
    _schedulePersist();
  }

  void _recordPage(int index, int page) {
    if (index >= _tabs.length || _tabs[index].currentPage == page) return;
    _tabs[index].currentPage = page;
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
            onOpen: _openNewTab,
          ),
          Expanded(
            child: IndexedStack(
              index: _activeTabIndex,
              children: [
                for (var index = 0; index < _tabs.length; index++)
                  PdfReaderScreen(
                    key: ValueKey(_tabs[index].filePath),
                    filePath: _tabs[index].filePath,
                    fileSystem: widget.fileSystem,
                    database: widget.database,
                    initialPageNumber: _tabs[index].currentPage,
                    onPageChanged: (page) => _recordPage(index, page),
                    onCreateBookmark: (draft) =>
                        widget.onCreateBookmark(_tabs[index].filePath, draft),
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
    required this.onOpen,
  });

  final List<WorkspaceTabItem> tabs;
  final int activeIndex;
  final ValueChanged<int> onActivate;
  final ValueChanged<int> onClose;
  final VoidCallback onOpen;

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
                    return Semantics(
                      selected: selected,
                      button: true,
                      label: tab.title,
                      child: InkWell(
                        key: ValueKey('document-tab-${tab.filePath}'),
                        onTap: () => onActivate(index),
                        child: Container(
                          constraints: const BoxConstraints(
                            minWidth: 120,
                            maxWidth: 220,
                          ),
                          padding: const EdgeInsetsDirectional.only(start: 12),
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
