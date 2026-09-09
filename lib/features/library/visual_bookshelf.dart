import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../core/accessibility/accessibility_announcer.dart';
import '../../core/covers/cover_cache_manager.dart';
import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../reader/pdf_reader_screen.dart';
import '../workspace/split_reader_workspace.dart';

enum BookshelfViewMode { coverGrid, detailedList, condensedList }

class VisualBookshelf extends StatefulWidget {
  const VisualBookshelf({
    super.key,
    required this.database,
    required this.fileSystem,
    required this.onCreateBookmark,
    this.onSelectPdf,
    this.initialSortBy = LibrarySortBy.title,
    this.initialSortAscending = true,
    this.initialOnlyFavorites = false,
    this.onlyOpened = false,
  });

  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;
  final ValueChanged<String>? onSelectPdf;
  final LibrarySortBy initialSortBy;
  final bool initialSortAscending;
  final bool initialOnlyFavorites;
  final bool onlyOpened;

  @override
  State<VisualBookshelf> createState() => _VisualBookshelfState();
}

enum _BookMenuAction { open, favorite, edit, refreshCover, reveal, copyPath }

class _ContextMenuRegion extends StatefulWidget {
  const _ContextMenuRegion({
    required this.semanticLabel,
    required this.onRequested,
    required this.child,
  });

  final String semanticLabel;
  final ValueChanged<Offset> onRequested;
  final Widget child;

  @override
  State<_ContextMenuRegion> createState() => _ContextMenuRegionState();
}

class _ContextMenuRegionState extends State<_ContextMenuRegion> {
  final GlobalKey _key = GlobalKey();

  void _showFromKeyboard() {
    final box = _key.currentContext?.findRenderObject() as RenderBox?;
    if (box == null) return;
    widget.onRequested(box.localToGlobal(box.size.center(Offset.zero)));
  }

  @override
  Widget build(BuildContext context) {
    return FocusableActionDetector(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.f10, shift: true): ActivateIntent(),
        SingleActivator(LogicalKeyboardKey.contextMenu): ActivateIntent(),
      },
      actions: {
        ActivateIntent: CallbackAction<ActivateIntent>(
          onInvoke: (_) {
            _showFromKeyboard();
            return null;
          },
        ),
      },
      child: Semantics(
        label: widget.semanticLabel,
        child: GestureDetector(
          key: _key,
          behavior: HitTestBehavior.translucent,
          onSecondaryTapUp: (details) =>
              widget.onRequested(details.globalPosition),
          child: widget.child,
        ),
      ),
    );
  }
}

class _VisualBookshelfState extends State<VisualBookshelf> {
  BookshelfViewMode _viewMode = BookshelfViewMode.coverGrid;
  late LibrarySortBy _sortBy;
  late bool _sortAscending;
  String _searchQuery = '';
  String? _selectedAuthor;
  String? _selectedTag;
  String? _selectedSeries;
  late bool _onlyFavorites;
  String? _selectedFormat;

  Map<String, int> _authors = {};
  Map<String, int> _tags = {};
  Map<String, int> _series = {};
  bool _showAuthorIndex = false;
  bool _showTagIndex = false;

  @override
  void initState() {
    super.initState();
    _sortBy = widget.initialSortBy;
    _sortAscending = widget.initialSortAscending;
    _onlyFavorites = widget.initialOnlyFavorites;
    _refreshFacets();
  }

  Future<void> _refreshFacets() async {
    final a = await widget.database.getDistinctAuthors();
    final t = await widget.database.getDistinctTags();
    final s = await widget.database.getDistinctSeries();
    if (mounted) {
      setState(() {
        _authors = a;
        _tags = t;
        _series = s;
      });
    }
  }

  Future<List<LibraryFile>> _loadBooks() {
    return widget.database.getFilteredLibraryFiles(
      query: _searchQuery,
      author: _selectedAuthor,
      tag: _selectedTag,
      series: _selectedSeries,
      format: _selectedFormat,
      onlyFavorites: _onlyFavorites ? true : null,
      onlyOpened: widget.onlyOpened,
      sortBy: _sortBy,
      ascending: _sortAscending,
    );
  }

  Future<void> _toggleFavorite(LibraryFile book) async {
    final isFav = await widget.database.toggleFavorite(book.id);
    if (!mounted) return;
    setState(() {});
    AccessibilityAnnouncer.announce(
      context,
      isFav
          ? '${book.fileName} added to favorites'
          : '${book.fileName} removed from favorites',
    );
  }

  Future<void> _openBook(LibraryFile book) async {
    if (!await widget.fileSystem.exists(book.filePath)) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Document file is missing from disk.'),
          backgroundColor: Colors.red,
        ),
      );
      return;
    }

    widget.onSelectPdf?.call(book.filePath);

    if (book.format.toUpperCase() != 'PDF') {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text(
            'EPUB discovery and search are available. Page reading and ink currently require a PDF.',
          ),
        ),
      );
      return;
    }

    if (!mounted) return;
    await Navigator.of(context).push<void>(
      MaterialPageRoute(
        builder: (_) => SplitReaderWorkspace(
          initialFilePath: book.filePath,
          fileSystem: widget.fileSystem,
          database: widget.database,
          initialPageNumber: book.currentPage,
          onCreateBookmark: widget.onCreateBookmark,
        ),
      ),
    );
    if (mounted) {
      setState(() {});
    }
  }

  Future<void> _refreshCover(LibraryFile book) async {
    final path = await CoverCacheManager.instance.enqueueCover(
      book.filePath,
      refresh: true,
    );
    await widget.database.updateLibraryCover(book.filePath, path);
    if (mounted) setState(() {});
  }

  Future<void> _revealBook(LibraryFile book) async {
    try {
      if (!Platform.isWindows) throw UnsupportedError('Windows only');
      await Process.run('explorer.exe', ['/select,${book.filePath}']);
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not show the file: $error')),
      );
    }
  }

  Future<void> _copyBookPath(LibraryFile book) async {
    await Clipboard.setData(ClipboardData(text: book.filePath));
    if (!mounted) return;
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('File path copied.')));
  }

  Future<void> _showBookMenu(LibraryFile book, Offset position) async {
    final strings = AppLocalizations.of(context)!;
    final overlay =
        Overlay.of(context).context.findRenderObject()! as RenderBox;
    final action = await showMenu<_BookMenuAction>(
      context: context,
      position: RelativeRect.fromRect(
        Rect.fromLTWH(position.dx, position.dy, 0, 0),
        Offset.zero & overlay.size,
      ),
      items: [
        PopupMenuItem(value: _BookMenuAction.open, child: Text(strings.open)),
        PopupMenuItem(
          value: _BookMenuAction.favorite,
          child: Text(
            book.isFavorite
                ? strings.removeFromFavorites
                : strings.addToFavorites,
          ),
        ),
        PopupMenuItem(
          value: _BookMenuAction.edit,
          child: Text(strings.editLibraryDetails),
        ),
        PopupMenuItem(
          value: _BookMenuAction.refreshCover,
          child: Text(strings.refreshCover),
        ),
        const PopupMenuDivider(),
        PopupMenuItem(
          value: _BookMenuAction.reveal,
          child: Text(strings.showInFileExplorer),
        ),
        PopupMenuItem(
          value: _BookMenuAction.copyPath,
          child: Text(strings.copyFullPath),
        ),
      ],
    );
    switch (action) {
      case _BookMenuAction.open:
        await _openBook(book);
      case _BookMenuAction.favorite:
        await _toggleFavorite(book);
      case _BookMenuAction.edit:
        await _editMetadata(book);
      case _BookMenuAction.refreshCover:
        await _refreshCover(book);
      case _BookMenuAction.reveal:
        await _revealBook(book);
      case _BookMenuAction.copyPath:
        await _copyBookPath(book);
      case null:
        break;
    }
  }

  Widget _withBookContextMenu(LibraryFile book, Widget child) {
    return _ContextMenuRegion(
      semanticLabel: book.title ?? book.fileName,
      onRequested: (position) => _showBookMenu(book, position),
      child: child,
    );
  }

  Future<void> _editMetadata(LibraryFile book) async {
    final titleCtrl = TextEditingController(text: book.title ?? book.fileName);
    final authorCtrl = TextEditingController(text: book.author ?? '');
    final seriesCtrl = TextEditingController(text: book.series ?? '');
    final tagsCtrl = TextEditingController(text: book.tags ?? '');

    final saved = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Edit Book Metadata'),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: titleCtrl,
                decoration: const InputDecoration(labelText: 'Title'),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: authorCtrl,
                decoration: const InputDecoration(labelText: 'Author'),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: seriesCtrl,
                decoration: const InputDecoration(labelText: 'Series'),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: tagsCtrl,
                decoration: const InputDecoration(
                  labelText: 'Tags (comma separated)',
                ),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Save'),
          ),
        ],
      ),
    );

    if (saved == true) {
      await widget.database.updateBookMetadata(
        book.id,
        title: titleCtrl.text.trim().isEmpty ? null : titleCtrl.text.trim(),
        author: authorCtrl.text.trim().isEmpty ? null : authorCtrl.text.trim(),
        series: seriesCtrl.text.trim().isEmpty ? null : seriesCtrl.text.trim(),
        tags: tagsCtrl.text.trim().isEmpty ? null : tagsCtrl.text.trim(),
      );
      await _refreshFacets();
      if (mounted) setState(() {});
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        _buildToolbar(context),
        if (_hasActiveFilters()) _buildFilterSummaryBar(context),
        if (_showAuthorIndex) _buildAuthorIndexView(context),
        if (_showTagIndex) _buildTagIndexView(context),
        Expanded(
          child: FutureBuilder<List<LibraryFile>>(
            future: _loadBooks(),
            builder: (context, snapshot) {
              if (snapshot.connectionState != ConnectionState.done) {
                return const Center(child: CircularProgressIndicator());
              }
              final books = snapshot.data ?? const [];
              if (books.isEmpty) {
                return Center(
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const Icon(
                        Icons.menu_book_outlined,
                        size: 64,
                        color: Colors.grey,
                      ),
                      const SizedBox(height: 16),
                      Text(
                        _hasActiveFilters()
                            ? 'No books match your current filters.'
                            : 'No books found. Add library folders in Settings.',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      if (_hasActiveFilters()) ...[
                        const SizedBox(height: 12),
                        OutlinedButton(
                          onPressed: () {
                            setState(() {
                              _searchQuery = '';
                              _selectedAuthor = null;
                              _selectedTag = null;
                              _selectedSeries = null;
                              _selectedFormat = null;
                              _onlyFavorites = false;
                            });
                          },
                          child: const Text('Clear Filters'),
                        ),
                      ],
                    ],
                  ),
                );
              }

              switch (_viewMode) {
                case BookshelfViewMode.coverGrid:
                  return _buildCoverGrid(books);
                case BookshelfViewMode.detailedList:
                  return _buildDetailedList(books);
                case BookshelfViewMode.condensedList:
                  return _buildCondensedList(books);
              }
            },
          ),
        ),
      ],
    );
  }

  bool _hasActiveFilters() {
    return _selectedAuthor != null ||
        _selectedTag != null ||
        _selectedSeries != null ||
        _selectedFormat != null ||
        _onlyFavorites ||
        _searchQuery.isNotEmpty;
  }

  Widget _buildToolbar(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: Theme.of(
          context,
        ).colorScheme.surfaceContainerHighest.withAlpha(80),
        border: Border(
          bottom: BorderSide(
            color: Theme.of(context).dividerColor.withAlpha(60),
          ),
        ),
      ),
      child: Column(
        children: [
          LayoutBuilder(
            builder: (context, constraints) {
              if (constraints.maxWidth < 600) {
                return Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    _buildSearchField(),
                    const SizedBox(height: 8),
                    Align(
                      alignment: AlignmentDirectional.centerEnd,
                      child: _buildViewSelector(),
                    ),
                  ],
                );
              }
              return Row(
                children: [
                  Expanded(child: _buildSearchField()),
                  const SizedBox(width: 8),
                  _buildViewSelector(),
                ],
              );
            },
          ),
          const SizedBox(height: 8),
          Align(
            alignment: AlignmentDirectional.centerStart,
            child: Wrap(
              spacing: 6,
              runSpacing: 4,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                PopupMenuButton<LibrarySortBy>(
                  initialValue: _sortBy,
                  tooltip: 'Sort by',
                  onSelected: (val) => setState(() => _sortBy = val),
                  itemBuilder: (ctx) => [
                    const PopupMenuItem(
                      value: LibrarySortBy.title,
                      child: Text('Sort by Title'),
                    ),
                    const PopupMenuItem(
                      value: LibrarySortBy.author,
                      child: Text('Sort by Author'),
                    ),
                    const PopupMenuItem(
                      value: LibrarySortBy.dateAdded,
                      child: Text('Sort by Date Added'),
                    ),
                    const PopupMenuItem(
                      value: LibrarySortBy.lastOpened,
                      child: Text('Sort by Last Opened'),
                    ),
                    const PopupMenuItem(
                      value: LibrarySortBy.fileSize,
                      child: Text('Sort by File Size'),
                    ),
                    const PopupMenuItem(
                      value: LibrarySortBy.progress,
                      child: Text('Sort by Reading Progress'),
                    ),
                  ],
                  child: Chip(
                    avatar: const Icon(Icons.sort, size: 16),
                    label: Text('Sort: ${_sortLabel(_sortBy)}'),
                  ),
                ),
                IconButton(
                  icon: Icon(
                    _sortAscending ? Icons.arrow_upward : Icons.arrow_downward,
                    size: 18,
                  ),
                  tooltip: _sortAscending ? 'Ascending' : 'Descending',
                  onPressed: () =>
                      setState(() => _sortAscending = !_sortAscending),
                ),
                FilterChip(
                  label: const Text('Favorites'),
                  avatar: Icon(
                    _onlyFavorites ? Icons.star : Icons.star_border,
                    size: 16,
                    color: Colors.amber[700],
                  ),
                  selected: _onlyFavorites,
                  onSelected: (val) => setState(() => _onlyFavorites = val),
                ),
                ActionChip(
                  avatar: const Icon(Icons.person_outline, size: 16),
                  label: Text('Authors (${_authors.length})'),
                  backgroundColor: _showAuthorIndex
                      ? Theme.of(context).colorScheme.primaryContainer
                      : null,
                  onPressed: () {
                    setState(() {
                      _showAuthorIndex = !_showAuthorIndex;
                      if (_showAuthorIndex) _showTagIndex = false;
                    });
                  },
                ),
                ActionChip(
                  avatar: const Icon(Icons.tag_outlined, size: 16),
                  label: Text(
                    'Tags & Series (${_tags.length + _series.length})',
                  ),
                  backgroundColor: _showTagIndex
                      ? Theme.of(context).colorScheme.primaryContainer
                      : null,
                  onPressed: () {
                    setState(() {
                      _showTagIndex = !_showTagIndex;
                      if (_showTagIndex) _showAuthorIndex = false;
                    });
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSearchField() {
    return TextField(
      decoration: InputDecoration(
        hintText: 'Search title, author, file...',
        prefixIcon: const Icon(Icons.search, size: 20),
        suffixIcon: _searchQuery.isNotEmpty
            ? IconButton(
                icon: const Icon(Icons.clear, size: 18),
                onPressed: () => setState(() => _searchQuery = ''),
              )
            : null,
        isDense: true,
        contentPadding: const EdgeInsets.symmetric(
          horizontal: 12,
          vertical: 10,
        ),
        border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
      ),
      onChanged: (value) => setState(() => _searchQuery = value),
    );
  }

  Widget _buildViewSelector() {
    return SegmentedButton<BookshelfViewMode>(
      segments: const [
        ButtonSegment(
          value: BookshelfViewMode.coverGrid,
          icon: Icon(Icons.grid_view_rounded, size: 18),
          tooltip: 'Cover Grid',
        ),
        ButtonSegment(
          value: BookshelfViewMode.detailedList,
          icon: Icon(Icons.view_list_rounded, size: 18),
          tooltip: 'Detailed List',
        ),
        ButtonSegment(
          value: BookshelfViewMode.condensedList,
          icon: Icon(Icons.table_rows_rounded, size: 18),
          tooltip: 'Compact List',
        ),
      ],
      selected: {_viewMode},
      onSelectionChanged: (selection) =>
          setState(() => _viewMode = selection.first),
    );
  }

  String _sortLabel(LibrarySortBy sort) {
    switch (sort) {
      case LibrarySortBy.title:
        return 'Title';
      case LibrarySortBy.author:
        return 'Author';
      case LibrarySortBy.dateAdded:
        return 'Date Added';
      case LibrarySortBy.lastOpened:
        return 'Last Opened';
      case LibrarySortBy.fileSize:
        return 'File Size';
      case LibrarySortBy.progress:
        return 'Reading Progress';
    }
  }

  Widget _buildFilterSummaryBar(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      color: Theme.of(context).colorScheme.primary.withAlpha(20),
      child: Row(
        children: [
          const Text(
            'Active filters: ',
            style: TextStyle(fontWeight: FontWeight.w600, fontSize: 12),
          ),
          Expanded(
            child: Wrap(
              spacing: 6,
              runSpacing: 4,
              children: [
                if (_selectedAuthor != null)
                  InputChip(
                    label: Text('Author: $_selectedAuthor'),
                    onDeleted: () => setState(() => _selectedAuthor = null),
                  ),
                if (_selectedTag != null)
                  InputChip(
                    label: Text('#$_selectedTag'),
                    onDeleted: () => setState(() => _selectedTag = null),
                  ),
                if (_selectedSeries != null)
                  InputChip(
                    label: Text('Series: $_selectedSeries'),
                    onDeleted: () => setState(() => _selectedSeries = null),
                  ),
                if (_selectedFormat != null)
                  InputChip(
                    label: Text(_selectedFormat!),
                    onDeleted: () => setState(() => _selectedFormat = null),
                  ),
                if (_onlyFavorites)
                  InputChip(
                    label: const Text('Favorites Only'),
                    onDeleted: () => setState(() => _onlyFavorites = false),
                  ),
                if (_searchQuery.isNotEmpty)
                  InputChip(
                    label: Text('Query: $_searchQuery'),
                    onDeleted: () => setState(() => _searchQuery = ''),
                  ),
              ],
            ),
          ),
          TextButton(
            onPressed: () {
              setState(() {
                _searchQuery = '';
                _selectedAuthor = null;
                _selectedTag = null;
                _selectedSeries = null;
                _selectedFormat = null;
                _onlyFavorites = false;
              });
            },
            child: const Text('Reset', style: TextStyle(fontSize: 12)),
          ),
        ],
      ),
    );
  }

  Widget _buildAuthorIndexView(BuildContext context) {
    final sortedAuthors = _authors.keys.toList()
      ..sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));

    return Container(
      height: 160,
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(
          bottom: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Author Index',
                style: Theme.of(context).textTheme.titleSmall,
              ),
              IconButton(
                icon: const Icon(Icons.close, size: 16),
                onPressed: () => setState(() => _showAuthorIndex = false),
              ),
            ],
          ),
          Expanded(
            child: sortedAuthors.isEmpty
                ? const Center(child: Text('No authors found.'))
                : ListView.builder(
                    scrollDirection: Axis.horizontal,
                    itemCount: sortedAuthors.length,
                    itemBuilder: (ctx, index) {
                      final author = sortedAuthors[index];
                      final count = _authors[author] ?? 0;
                      final initial = author.isNotEmpty
                          ? author.characters.first.toUpperCase()
                          : '?';
                      final isSelected = _selectedAuthor == author;

                      return Padding(
                        padding: const EdgeInsets.only(right: 8),
                        child: InkWell(
                          onTap: () {
                            setState(() {
                              _selectedAuthor = isSelected ? null : author;
                              _showAuthorIndex = false;
                            });
                          },
                          borderRadius: BorderRadius.circular(8),
                          child: Container(
                            width: 110,
                            padding: const EdgeInsets.all(8),
                            decoration: BoxDecoration(
                              color: isSelected
                                  ? Theme.of(
                                      context,
                                    ).colorScheme.primaryContainer
                                  : Theme.of(
                                      context,
                                    ).colorScheme.surfaceContainerHigh,
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(
                                color: isSelected
                                    ? Theme.of(context).colorScheme.primary
                                    : Theme.of(
                                        context,
                                      ).dividerColor.withAlpha(40),
                              ),
                            ),
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                CircleAvatar(
                                  radius: 18,
                                  backgroundColor: Theme.of(
                                    context,
                                  ).colorScheme.primary.withAlpha(40),
                                  child: Text(
                                    initial,
                                    style: TextStyle(
                                      fontWeight: FontWeight.bold,
                                      color: Theme.of(
                                        context,
                                      ).colorScheme.primary,
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 6),
                                Text(
                                  author,
                                  maxLines: 2,
                                  overflow: TextOverflow.ellipsis,
                                  textAlign: TextAlign.center,
                                  style: const TextStyle(
                                    fontSize: 11,
                                    fontWeight: FontWeight.w600,
                                  ),
                                ),
                                const SizedBox(height: 2),
                                Text(
                                  '$count ${count == 1 ? 'book' : 'books'}',
                                  style: TextStyle(
                                    fontSize: 10,
                                    color: Theme.of(context).hintColor,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }

  Widget _buildTagIndexView(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(
          bottom: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Tags and Series',
                style: Theme.of(context).textTheme.titleSmall,
              ),
              IconButton(
                icon: const Icon(Icons.close, size: 16),
                onPressed: () => setState(() => _showTagIndex = false),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 6,
            children: [
              ..._tags.entries.map((e) {
                final isSelected = _selectedTag == e.key;
                return FilterChip(
                  label: Text('#${e.key} (${e.value})'),
                  selected: isSelected,
                  onSelected: (val) {
                    setState(() {
                      _selectedTag = val ? e.key : null;
                      _showTagIndex = false;
                    });
                  },
                );
              }),
              ..._series.entries.map((e) {
                final isSelected = _selectedSeries == e.key;
                return FilterChip(
                  avatar: const Icon(
                    Icons.collections_bookmark_outlined,
                    size: 14,
                  ),
                  label: Text('${e.key} (${e.value})'),
                  selected: isSelected,
                  onSelected: (val) {
                    setState(() {
                      _selectedSeries = val ? e.key : null;
                      _showTagIndex = false;
                    });
                  },
                );
              }),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildCoverGrid(List<LibraryFile> books) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final crossAxisCount = (constraints.maxWidth / 200).clamp(2, 6).floor();
        return GridView.builder(
          padding: const EdgeInsets.all(16),
          gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
            crossAxisCount: crossAxisCount,
            childAspectRatio: 0.62,
            crossAxisSpacing: 16,
            mainAxisSpacing: 16,
          ),
          itemCount: books.length,
          itemBuilder: (context, index) {
            final book = books[index];
            return _buildBookCard(book);
          },
        );
      },
    );
  }

  Widget _buildBookCard(LibraryFile book) {
    final title = book.title ?? book.fileName;
    final progress = book.pageCount > 0
        ? (book.currentPage / book.pageCount).clamp(0.0, 1.0)
        : 0.0;
    final isPdf = book.format.toUpperCase() == 'PDF';

    return _withBookContextMenu(
      book,
      Card(
        clipBehavior: Clip.antiAlias,
        elevation: 2,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
        child: InkWell(
          onTap: () => _openBook(book),
          onLongPress: () => _editMetadata(book),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Expanded(
                child: Stack(
                  fit: StackFit.expand,
                  children: [
                    _buildBookCoverVisual(book),
                    Positioned(
                      top: 6,
                      left: 6,
                      child: Container(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 6,
                          vertical: 2,
                        ),
                        decoration: BoxDecoration(
                          color: isPdf
                              ? Colors.indigo.withAlpha(220)
                              : Colors.teal.withAlpha(220),
                          borderRadius: BorderRadius.circular(4),
                        ),
                        child: Text(
                          book.format.toUpperCase(),
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 10,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),
                    ),
                    Positioned(
                      top: 2,
                      right: 2,
                      child: IconButton(
                        icon: Icon(
                          book.isFavorite ? Icons.star : Icons.star_border,
                          color: book.isFavorite
                              ? Colors.amber[400]
                              : Colors.white70,
                          size: 20,
                        ),
                        tooltip: book.isFavorite ? 'Unfavorite' : 'Favorite',
                        onPressed: () => _toggleFavorite(book),
                      ),
                    ),
                    if (book.pageCount > 0)
                      Positioned(
                        bottom: 6,
                        right: 6,
                        child: Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 6,
                            vertical: 2,
                          ),
                          decoration: BoxDecoration(
                            color: Colors.black.withAlpha(180),
                            borderRadius: BorderRadius.circular(4),
                          ),
                          child: Text(
                            '${book.pageCount}p',
                            style: const TextStyle(
                              color: Colors.white,
                              fontSize: 10,
                            ),
                          ),
                        ),
                      ),
                  ],
                ),
              ),
              LinearProgressIndicator(
                value: progress,
                minHeight: 4,
                backgroundColor: Colors.black12,
                valueColor: AlwaysStoppedAnimation<Color>(
                  progress >= 1.0
                      ? Colors.green
                      : Theme.of(context).colorScheme.primary,
                ),
              ),
              SizedBox(
                height: 88,
                child: Padding(
                  padding: const EdgeInsets.all(8),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        title,
                        maxLines: 2,
                        overflow: TextOverflow.ellipsis,
                        style: const TextStyle(
                          fontWeight: FontWeight.bold,
                          fontSize: 12,
                        ),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        book.author ?? 'Unknown Author',
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: TextStyle(
                          fontSize: 11,
                          color: Theme.of(context).hintColor,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Text(
                            '${(progress * 100).round()}% read',
                            style: TextStyle(
                              fontSize: 10,
                              color: Theme.of(context).hintColor,
                            ),
                          ),
                          InkWell(
                            onTap: () => _editMetadata(book),
                            child: const Icon(Icons.more_vert, size: 14),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildBookCoverVisual(LibraryFile book) {
    if (book.coverPath != null && File(book.coverPath!).existsSync()) {
      return ColoredBox(
        color: const Color(0xFFE7E8ED),
        child: Image.file(
          File(book.coverPath!),
          fit: book.format.toUpperCase() == 'PDF'
              ? BoxFit.contain
              : BoxFit.cover,
        ),
      );
    }

    final hash = book.filePath.hashCode.abs();
    final hue = (hash % 360).toDouble();
    final bgColor = HSLColor.fromAHSL(1.0, hue, 0.45, 0.28).toColor();
    final spineColor = HSLColor.fromAHSL(1.0, hue, 0.55, 0.20).toColor();

    return Container(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.centerLeft,
          end: Alignment.centerRight,
          colors: [spineColor, bgColor],
          stops: const [0.08, 0.15],
        ),
      ),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(height: 2, width: 30, color: Colors.amber.withAlpha(200)),
          const SizedBox(height: 10),
          Text(
            book.title ?? book.fileName,
            maxLines: 4,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              color: Colors.white,
              fontWeight: FontWeight.bold,
              fontSize: 13,
              height: 1.25,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            book.author ?? '',
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
            style: TextStyle(
              color: Colors.white.withAlpha(200),
              fontSize: 11,
              fontStyle: FontStyle.italic,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDetailedList(List<LibraryFile> books) {
    return ListView.separated(
      padding: const EdgeInsets.all(16),
      itemCount: books.length,
      separatorBuilder: (_, _) => const Divider(height: 1),
      itemBuilder: (context, index) {
        final book = books[index];
        final title = book.title ?? book.fileName;
        final progress = book.pageCount > 0
            ? (book.currentPage / book.pageCount).clamp(0.0, 1.0)
            : 0.0;
        final sizeMb = (book.fileSizeBytes / (1024 * 1024)).toStringAsFixed(1);

        return _withBookContextMenu(
          book,
          ListTile(
            contentPadding: const EdgeInsets.symmetric(
              horizontal: 8,
              vertical: 6,
            ),
            leading: ClipRRect(
              borderRadius: BorderRadius.circular(4),
              child: SizedBox(
                width: 44,
                height: 60,
                child: _buildBookCoverVisual(book),
              ),
            ),
            title: Row(
              children: [
                Expanded(
                  child: Text(
                    title,
                    style: const TextStyle(fontWeight: FontWeight.w600),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                Container(
                  margin: const EdgeInsets.symmetric(horizontal: 6),
                  padding: const EdgeInsets.symmetric(
                    horizontal: 6,
                    vertical: 2,
                  ),
                  decoration: BoxDecoration(
                    color: book.format.toUpperCase() == 'PDF'
                        ? Colors.indigo.withAlpha(30)
                        : Colors.teal.withAlpha(30),
                    borderRadius: BorderRadius.circular(4),
                  ),
                  child: Text(
                    book.format.toUpperCase(),
                    style: TextStyle(
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                      color: book.format.toUpperCase() == 'PDF'
                          ? Colors.indigo
                          : Colors.teal,
                    ),
                  ),
                ),
              ],
            ),
            subtitle: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const SizedBox(height: 2),
                Text(
                  '${book.author ?? 'Unknown Author'} · ${book.pageCount} pages · $sizeMb MB · ${book.bookmarkCount} bookmarks',
                  style: const TextStyle(fontSize: 11),
                ),
                const SizedBox(height: 6),
                Row(
                  children: [
                    Expanded(
                      child: ClipRRect(
                        borderRadius: BorderRadius.circular(2),
                        child: LinearProgressIndicator(
                          value: progress,
                          minHeight: 4,
                          backgroundColor: Colors.black12,
                        ),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Text(
                      '${(progress * 100).round()}%',
                      style: const TextStyle(fontSize: 10),
                    ),
                  ],
                ),
              ],
            ),
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                IconButton(
                  icon: Icon(
                    book.isFavorite ? Icons.star : Icons.star_border,
                    color: book.isFavorite ? Colors.amber[700] : null,
                  ),
                  tooltip: 'Favorite',
                  onPressed: () => _toggleFavorite(book),
                ),
                IconButton(
                  icon: const Icon(Icons.edit_outlined, size: 18),
                  tooltip: 'Edit Metadata',
                  onPressed: () => _editMetadata(book),
                ),
              ],
            ),
            onTap: () => _openBook(book),
          ),
        );
      },
    );
  }

  Widget _buildCondensedList(List<LibraryFile> books) {
    return ListView.builder(
      itemCount: books.length,
      itemBuilder: (context, index) {
        final book = books[index];
        final title = book.title ?? book.fileName;
        final progress = book.pageCount > 0
            ? (book.currentPage / book.pageCount).clamp(0.0, 1.0)
            : 0.0;

        return _withBookContextMenu(
          book,
          ListTile(
            dense: true,
            leading: Icon(
              book.format.toUpperCase() == 'PDF'
                  ? Icons.picture_as_pdf_outlined
                  : Icons.book_outlined,
              size: 18,
            ),
            title: Text(title, maxLines: 1, overflow: TextOverflow.ellipsis),
            subtitle: Text(
              '${book.author ?? 'Unknown'} · ${book.pageCount}p · ${(progress * 100).round()}%',
              style: const TextStyle(fontSize: 10),
            ),
            trailing: IconButton(
              icon: Icon(
                book.isFavorite ? Icons.star : Icons.star_border,
                size: 16,
                color: book.isFavorite ? Colors.amber[700] : null,
              ),
              onPressed: () => _toggleFavorite(book),
            ),
            onTap: () => _openBook(book),
          ),
        );
      },
    );
  }
}
