import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../reader/pdf_reader_screen.dart';
import 'research_scratchpad.dart';

enum WorkspaceSplitMode { single, sideBySideBooks, bookAndScratchpad }

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

class SplitReaderWorkspace extends StatefulWidget {
  const SplitReaderWorkspace({
    super.key,
    required this.initialFilePath,
    required this.database,
    required this.fileSystem,
    required this.onCreateBookmark,
  });

  final String initialFilePath;
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
  WorkspaceSplitMode _splitMode = WorkspaceSplitMode.single;
  String? _secondaryFilePath;
  int _primaryPage = 1;

  @override
  void initState() {
    super.initState();
    _initWorkspace();
  }

  Future<void> _initWorkspace() async {
    final savedTabs = await widget.database.getReadingSessionTabs();
    if (savedTabs.isNotEmpty) {
      for (final t in savedTabs) {
        if (await widget.fileSystem.exists(t.filePath)) {
          final title = _titleFromPath(t.filePath);
          _tabs.add(
            WorkspaceTabItem(
              filePath: t.filePath,
              title: title,
              currentPage: t.pageNumber,
            ),
          );
        }
      }
    }

    if (_tabs.isEmpty) {
      final title = _titleFromPath(widget.initialFilePath);
      _tabs.add(
        WorkspaceTabItem(filePath: widget.initialFilePath, title: title),
      );
    }

    if (mounted) {
      setState(() {
        _activeTabIndex = 0;
      });
    }
  }

  String _titleFromPath(String path) {
    final name = path.split(RegExp(r'[/\\]')).last;
    return name.replaceAll(RegExp(r'\.[^.]+$'), '');
  }

  Future<void> _persistTabs() async {
    final sessionList = <({String filePath, int pageNumber, bool isActive})>[];
    for (var i = 0; i < _tabs.length; i++) {
      final tab = _tabs[i];
      sessionList.add((
        filePath: tab.filePath,
        pageNumber: tab.currentPage,
        isActive: i == _activeTabIndex,
      ));
    }
    await widget.database.saveReadingSessionTabs(sessionList);
  }

  Future<void> _openNewTab() async {
    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf', 'epub'],
    );
    if (result.isNotEmpty && result.single.path != null) {
      final path = result.single.path!;
      final title = _titleFromPath(path);
      setState(() {
        final existingIndex = _tabs.indexWhere((t) => t.filePath == path);
        if (existingIndex >= 0) {
          _activeTabIndex = existingIndex;
        } else {
          _tabs.add(WorkspaceTabItem(filePath: path, title: title));
          _activeTabIndex = _tabs.length - 1;
        }
      });
      await _persistTabs();
    }
  }

  void _closeTab(int index) {
    if (_tabs.length <= 1) return;
    setState(() {
      _tabs.removeAt(index);
      if (_activeTabIndex >= _tabs.length) {
        _activeTabIndex = _tabs.length - 1;
      }
    });
    _persistTabs();
  }

  Future<void> _pickSecondaryBook() async {
    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf', 'epub'],
    );
    if (result.isNotEmpty && result.single.path != null) {
      setState(() {
        _secondaryFilePath = result.single.path;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_tabs.isEmpty) {
      return const Scaffold(body: Center(child: CircularProgressIndicator()));
    }

    final activeTab = _tabs[_activeTabIndex];

    return Scaffold(
      body: Column(
        children: [
          _buildTabBar(context),
          Expanded(child: _buildWorkspaceContent(activeTab)),
        ],
      ),
    );
  }

  Widget _buildTabBar(BuildContext context) {
    return Container(
      height: 42,
      decoration: BoxDecoration(
        color: Theme.of(
          context,
        ).colorScheme.surfaceContainerHighest.withAlpha(90),
        border: Border(
          bottom: BorderSide(
            color: Theme.of(context).dividerColor.withAlpha(80),
          ),
        ),
      ),
      child: Row(
        children: [
          Expanded(
            child: ListView.builder(
              scrollDirection: Axis.horizontal,
              itemCount: _tabs.length,
              itemBuilder: (context, index) {
                final tab = _tabs[index];
                final isActive = index == _activeTabIndex;

                return InkWell(
                  onTap: () {
                    setState(() => _activeTabIndex = index);
                    _persistTabs();
                  },
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 12),
                    decoration: BoxDecoration(
                      color: isActive
                          ? Theme.of(context).colorScheme.surface
                          : Colors.transparent,
                      border: Border(
                        right: BorderSide(
                          color: Theme.of(context).dividerColor.withAlpha(50),
                        ),
                        bottom: BorderSide(
                          color: isActive
                              ? Theme.of(context).colorScheme.primary
                              : Colors.transparent,
                          width: 2.5,
                        ),
                      ),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Icon(Icons.menu_book, size: 14),
                        const SizedBox(width: 6),
                        ConstrainedBox(
                          constraints: const BoxConstraints(maxWidth: 160),
                          child: Text(
                            tab.title,
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            style: TextStyle(
                              fontSize: 12,
                              fontWeight: isActive
                                  ? FontWeight.bold
                                  : FontWeight.normal,
                            ),
                          ),
                        ),
                        if (_tabs.length > 1) ...[
                          const SizedBox(width: 4),
                          InkWell(
                            onTap: () => _closeTab(index),
                            child: const Icon(Icons.close, size: 14),
                          ),
                        ],
                      ],
                    ),
                  ),
                );
              },
            ),
          ),
          IconButton(
            icon: const Icon(Icons.add, size: 18),
            tooltip: 'Open new document tab',
            visualDensity: VisualDensity.compact,
            onPressed: _openNewTab,
          ),
          const VerticalDivider(width: 16, indent: 8, endIndent: 8),
          SegmentedButton<WorkspaceSplitMode>(
            segments: const [
              ButtonSegment(
                value: WorkspaceSplitMode.single,
                icon: Icon(Icons.crop_square, size: 16),
                tooltip: 'Single View',
              ),
              ButtonSegment(
                value: WorkspaceSplitMode.sideBySideBooks,
                icon: Icon(Icons.view_column_outlined, size: 16),
                tooltip: 'Side-by-Side Books',
              ),
              ButtonSegment(
                value: WorkspaceSplitMode.bookAndScratchpad,
                icon: Icon(Icons.vertical_split_outlined, size: 16),
                tooltip: 'Book & Scratchpad',
              ),
            ],
            selected: {_splitMode},
            onSelectionChanged: (set) {
              setState(() {
                _splitMode = set.first;
                if (_splitMode == WorkspaceSplitMode.sideBySideBooks &&
                    _secondaryFilePath == null) {
                  _pickSecondaryBook();
                }
              });
            },
          ),
          const SizedBox(width: 8),
        ],
      ),
    );
  }

  Widget _buildWorkspaceContent(WorkspaceTabItem activeTab) {
    switch (_splitMode) {
      case WorkspaceSplitMode.single:
        return PdfReaderScreen(
          key: ValueKey(activeTab.filePath),
          filePath: activeTab.filePath,
          fileSystem: widget.fileSystem,
          database: widget.database,
          initialPageNumber: activeTab.currentPage,
          onCreateBookmark: (draft) =>
              widget.onCreateBookmark(activeTab.filePath, draft),
        );

      case WorkspaceSplitMode.sideBySideBooks:
        return Row(
          children: [
            Expanded(
              child: PdfReaderScreen(
                key: ValueKey('left_${activeTab.filePath}'),
                filePath: activeTab.filePath,
                fileSystem: widget.fileSystem,
                database: widget.database,
                initialPageNumber: activeTab.currentPage,
                onCreateBookmark: (draft) =>
                    widget.onCreateBookmark(activeTab.filePath, draft),
              ),
            ),
            const VerticalDivider(width: 1),
            Expanded(
              child: _secondaryFilePath != null
                  ? PdfReaderScreen(
                      key: ValueKey('right_$_secondaryFilePath'),
                      filePath: _secondaryFilePath!,
                      fileSystem: widget.fileSystem,
                      database: widget.database,
                      onCreateBookmark: (draft) =>
                          widget.onCreateBookmark(_secondaryFilePath!, draft),
                    )
                  : Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const Icon(
                            Icons.compare_arrows,
                            size: 48,
                            color: Colors.grey,
                          ),
                          const SizedBox(height: 12),
                          const Text(
                            'Select second book for side-by-side comparison',
                          ),
                          const SizedBox(height: 12),
                          FilledButton.icon(
                            onPressed: _pickSecondaryBook,
                            icon: const Icon(Icons.folder_open),
                            label: const Text('Open Companion Book'),
                          ),
                        ],
                      ),
                    ),
            ),
          ],
        );

      case WorkspaceSplitMode.bookAndScratchpad:
        return Row(
          children: [
            Expanded(
              child: PdfReaderScreen(
                key: ValueKey('main_${activeTab.filePath}'),
                filePath: activeTab.filePath,
                fileSystem: widget.fileSystem,
                database: widget.database,
                initialPageNumber: activeTab.currentPage,
                onCreateBookmark: (draft) =>
                    widget.onCreateBookmark(activeTab.filePath, draft),
              ),
            ),
            ResearchScratchpad(
              filePath: activeTab.filePath,
              database: widget.database,
              currentPage: _primaryPage,
              onJumpToPage: (page) => setState(() => _primaryPage = page),
              onClose: () =>
                  setState(() => _splitMode = WorkspaceSplitMode.single),
            ),
          ],
        );
    }
  }
}
