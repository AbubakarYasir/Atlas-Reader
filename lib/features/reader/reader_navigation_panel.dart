import 'package:flutter/material.dart';

import '../../database.dart';
import '../../l10n/app_localizations.dart';
import 'reader_outline_sidebar.dart';

enum ReaderNavigationTab { outline, pages, bookmarks, annotations }

class ReaderNavigationPanel extends StatefulWidget {
  const ReaderNavigationPanel({
    super.key,
    required this.filePath,
    required this.database,
    required this.outline,
    required this.currentPage,
    required this.pageCount,
    required this.chapterPages,
    required this.bookmarkedPages,
    required this.pageOffset,
    required this.onSelectPage,
    required this.onClose,
    this.annotationRevision = 0,
    this.pagePreviewBuilder,
    this.width = 336,
    this.initialTab = ReaderNavigationTab.outline,
    this.onTabChanged,
  });

  final String filePath;
  final AppDatabase database;
  final List<ReaderOutlineItem> outline;
  final int currentPage;
  final int pageCount;
  final Set<int> chapterPages;
  final Set<int> bookmarkedPages;
  final int pageOffset;
  final ValueChanged<int> onSelectPage;
  final VoidCallback onClose;
  final int annotationRevision;
  final Widget Function(BuildContext context, int pageNumber)?
  pagePreviewBuilder;
  final double width;
  final ReaderNavigationTab initialTab;
  final ValueChanged<ReaderNavigationTab>? onTabChanged;

  @override
  State<ReaderNavigationPanel> createState() => _ReaderNavigationPanelState();
}

class _ReaderNavigationPanelState extends State<ReaderNavigationPanel>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;
  late final TextEditingController _searchController;
  late Future<List<DocumentAnnotation>> _annotations;
  String _query = '';

  @override
  void initState() {
    super.initState();
    _tabController = TabController(
      length: ReaderNavigationTab.values.length,
      vsync: this,
      initialIndex: ReaderNavigationTab.values.indexOf(widget.initialTab),
    )..addListener(_handleTabChanged);
    _searchController = TextEditingController();
    _annotations = widget.database.getAnnotationsForFile(widget.filePath);
  }

  @override
  void didUpdateWidget(covariant ReaderNavigationPanel oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.filePath != widget.filePath ||
        oldWidget.annotationRevision != widget.annotationRevision) {
      _annotations = widget.database.getAnnotationsForFile(widget.filePath);
    }
  }

  @override
  void dispose() {
    _tabController
      ..removeListener(_handleTabChanged)
      ..dispose();
    _searchController.dispose();
    super.dispose();
  }

  void _handleTabChanged() {
    if (_tabController.indexIsChanging) return;
    _searchController.clear();
    if (mounted) setState(() => _query = '');
    widget.onTabChanged?.call(ReaderNavigationTab.values[_tabController.index]);
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    return Semantics(
      container: true,
      label: strings.readerNavigation,
      child: SizedBox(
        width: widget.width,
        child: Material(
          color: Theme.of(context).colorScheme.surface,
          child: Column(
            children: [
              _PanelHeader(onClose: widget.onClose),
              TabBar(
                controller: _tabController,
                isScrollable: true,
                tabAlignment: TabAlignment.start,
                tabs: [
                  Tab(
                    icon: const Icon(Icons.account_tree),
                    text: strings.outline,
                  ),
                  Tab(icon: const Icon(Icons.grid_view), text: strings.pages),
                  Tab(
                    icon: const Icon(Icons.bookmarks_outlined),
                    text: strings.bookmarks,
                  ),
                  Tab(
                    icon: const Icon(Icons.comment_outlined),
                    text: strings.annotations,
                  ),
                ],
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(12, 10, 12, 8),
                child: TextField(
                  controller: _searchController,
                  decoration: InputDecoration(
                    hintText: strings.searchCurrentPanel,
                    prefixIcon: const Icon(Icons.search),
                    suffixIcon: _query.isEmpty
                        ? null
                        : IconButton(
                            tooltip: strings.clearSearch,
                            onPressed: () {
                              _searchController.clear();
                              setState(() => _query = '');
                            },
                            icon: const Icon(Icons.close),
                          ),
                    isDense: true,
                    border: const OutlineInputBorder(),
                  ),
                  onChanged: (value) =>
                      setState(() => _query = value.trim().toLowerCase()),
                ),
              ),
              Expanded(
                child: TabBarView(
                  controller: _tabController,
                  children: [
                    _OutlineTab(
                      items: widget.outline,
                      currentPage: widget.currentPage,
                      query: _query,
                      bookmarksOnly: false,
                      onSelectPage: widget.onSelectPage,
                    ),
                    _PagesTab(widget: widget, query: _query),
                    _OutlineTab(
                      items: widget.outline,
                      currentPage: widget.currentPage,
                      query: _query,
                      bookmarksOnly: true,
                      onSelectPage: widget.onSelectPage,
                    ),
                    _AnnotationsTab(
                      annotations: _annotations,
                      query: _query,
                      currentPage: widget.currentPage,
                      onSelectPage: widget.onSelectPage,
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _PanelHeader extends StatelessWidget {
  const _PanelHeader({required this.onClose});

  final VoidCallback onClose;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    return Container(
      height: 48,
      padding: const EdgeInsetsDirectional.only(start: 16, end: 4),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceContainerHighest,
        border: Border(
          bottom: BorderSide(color: Theme.of(context).dividerColor),
        ),
      ),
      child: Row(
        children: [
          Expanded(
            child: Text(
              strings.readerNavigation,
              style: Theme.of(context).textTheme.titleSmall,
            ),
          ),
          IconButton(
            tooltip: strings.closeNavigationPanel,
            onPressed: onClose,
            icon: const Icon(Icons.close),
          ),
        ],
      ),
    );
  }
}

class _PagesTab extends StatelessWidget {
  const _PagesTab({required this.widget, required this.query});

  final ReaderNavigationPanel widget;
  final String query;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    final pages = [
      for (var page = 1; page <= widget.pageCount; page++)
        if (query.isEmpty ||
            '$page'.contains(query) ||
            '${page + widget.pageOffset}'.contains(query))
          page,
    ];
    if (widget.pageCount == 0) {
      return Center(child: Text(strings.loadingPages));
    }
    if (pages.isEmpty) return Center(child: Text(strings.noMatches));

    return GridView.builder(
      key: const PageStorageKey('reader-pages-grid'),
      padding: const EdgeInsets.all(12),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 2,
        childAspectRatio: .72,
        crossAxisSpacing: 12,
        mainAxisSpacing: 12,
      ),
      itemCount: pages.length,
      itemBuilder: (context, index) {
        final page = pages[index];
        final selected = page == widget.currentPage;
        final chapter = widget.chapterPages.contains(page);
        final bookmarked = widget.bookmarkedPages.contains(page);
        final printedPage = page + widget.pageOffset;
        return Semantics(
          button: true,
          selected: selected,
          label: strings.pageThumbnail(page),
          child: InkWell(
            key: ValueKey('reader-page-thumbnail-$page'),
            borderRadius: BorderRadius.circular(8),
            onTap: () => widget.onSelectPage(page),
            child: AnimatedContainer(
              duration: const Duration(milliseconds: 120),
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.surfaceContainerLow,
                borderRadius: BorderRadius.circular(8),
                border: Border.all(
                  color: selected
                      ? Theme.of(context).colorScheme.primary
                      : Theme.of(context).dividerColor,
                  width: selected ? 2 : 1,
                ),
              ),
              child: Stack(
                children: [
                  Positioned.fill(
                    child: Padding(
                      padding: const EdgeInsets.fromLTRB(14, 16, 14, 34),
                      child:
                          widget.pagePreviewBuilder?.call(context, page) ??
                          const _PagePreviewPlaceholder(),
                    ),
                  ),
                  PositionedDirectional(
                    start: 8,
                    end: 8,
                    bottom: 7,
                    child: Row(
                      children: [
                        if (chapter) const Icon(Icons.flag_outlined, size: 14),
                        if (bookmarked) const Icon(Icons.bookmark, size: 14),
                        const Spacer(),
                        Text(
                          widget.pageOffset == 0
                              ? '$page'
                              : '$page · $printedPage',
                          style: Theme.of(context).textTheme.labelMedium,
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }
}

class _PagePreviewPlaceholder extends StatelessWidget {
  const _PagePreviewPlaceholder();

  @override
  Widget build(BuildContext context) {
    final color = Theme.of(context).colorScheme.onSurfaceVariant.withAlpha(45);
    return DecoratedBox(
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border.all(color: color),
      ),
      child: Padding(
        padding: const EdgeInsets.all(10),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Container(height: 5, color: color),
            const SizedBox(height: 7),
            Container(height: 3, color: color),
            const SizedBox(height: 5),
            Container(height: 3, color: color),
            const SizedBox(height: 5),
            FractionallySizedBox(
              widthFactor: .72,
              alignment: AlignmentDirectional.centerStart,
              child: Container(height: 3, color: color),
            ),
          ],
        ),
      ),
    );
  }
}

class _OutlineTab extends StatelessWidget {
  const _OutlineTab({
    required this.items,
    required this.currentPage,
    required this.query,
    required this.bookmarksOnly,
    required this.onSelectPage,
  });

  final List<ReaderOutlineItem> items;
  final int currentPage;
  final String query;
  final bool bookmarksOnly;
  final ValueChanged<int> onSelectPage;

  List<ReaderOutlineItem> _visibleItems(List<ReaderOutlineItem> source) {
    final visible = <ReaderOutlineItem>[];
    for (final item in source) {
      final children = _visibleItems(item.children);
      final kindMatches = !bookmarksOnly || item.isUserBookmark;
      final queryMatches =
          query.isEmpty ||
          item.title.toLowerCase().contains(query) ||
          item.breadcrumb.toLowerCase().contains(query);
      if ((kindMatches && queryMatches) || children.isNotEmpty) {
        visible.add(
          ReaderOutlineItem(
            title: item.title,
            pageNumber: item.pageNumber,
            breadcrumb: item.breadcrumb,
            isUserBookmark: item.isUserBookmark,
            children: children,
          ),
        );
      }
    }
    return visible;
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    final visible = _visibleItems(items);
    if (visible.isEmpty) {
      return Center(
        child: Text(
          query.isNotEmpty
              ? strings.noMatches
              : bookmarksOnly
              ? strings.noBookmarks
              : strings.noOutline,
        ),
      );
    }
    return ListView(
      key: PageStorageKey(
        bookmarksOnly ? 'reader-bookmarks' : 'reader-outline',
      ),
      padding: const EdgeInsets.symmetric(vertical: 6),
      children: [for (final item in visible) _buildItem(context, item, 0)],
    );
  }

  Widget _buildItem(BuildContext context, ReaderOutlineItem item, int depth) {
    final selected = item.pageNumber == currentPage;
    final leading = item.isUserBookmark
        ? Icons.bookmark_outline
        : item.children.isEmpty
        ? Icons.article_outlined
        : Icons.folder_outlined;
    if (item.children.isNotEmpty) {
      return ExpansionTile(
        key: PageStorageKey('outline-${item.breadcrumb}'),
        initiallyExpanded: selected || query.isNotEmpty,
        leading: Icon(leading, size: 19),
        title: Text(item.title, maxLines: 2, overflow: TextOverflow.ellipsis),
        subtitle: item.pageNumber == null
            ? null
            : Text(AppLocalizations.of(context)!.pageNumber(item.pageNumber!)),
        onExpansionChanged: (_) {},
        childrenPadding: const EdgeInsetsDirectional.only(start: 14),
        children: [
          for (final child in item.children)
            _buildItem(context, child, depth + 1),
        ],
      );
    }
    return Semantics(
      selected: selected,
      button: item.pageNumber != null,
      child: ListTile(
        contentPadding: EdgeInsetsDirectional.only(
          start: 16.0 + depth * 12,
          end: 12,
        ),
        selected: selected,
        leading: Icon(leading, size: 19),
        title: Text(item.title, maxLines: 2, overflow: TextOverflow.ellipsis),
        subtitle: item.pageNumber == null
            ? null
            : Text(AppLocalizations.of(context)!.pageNumber(item.pageNumber!)),
        onTap: item.pageNumber == null
            ? null
            : () => onSelectPage(item.pageNumber!),
      ),
    );
  }
}

class _AnnotationsTab extends StatelessWidget {
  const _AnnotationsTab({
    required this.annotations,
    required this.query,
    required this.currentPage,
    required this.onSelectPage,
  });

  final Future<List<DocumentAnnotation>> annotations;
  final String query;
  final int currentPage;
  final ValueChanged<int> onSelectPage;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    return FutureBuilder<List<DocumentAnnotation>>(
      future: annotations,
      builder: (context, snapshot) {
        if (snapshot.connectionState != ConnectionState.done) {
          return const Center(child: CircularProgressIndicator());
        }
        final items = (snapshot.data ?? const <DocumentAnnotation>[])
            .where((item) {
              if (query.isEmpty) return true;
              return item.type.toLowerCase().contains(query) ||
                  (item.note?.toLowerCase().contains(query) ?? false) ||
                  (item.selectedText?.toLowerCase().contains(query) ?? false) ||
                  '${item.pageNumber}'.contains(query);
            })
            .toList(growable: false);
        if (items.isEmpty) {
          return Center(
            child: Text(
              query.isEmpty ? strings.noAnnotations : strings.noMatches,
            ),
          );
        }
        return ListView.builder(
          key: const PageStorageKey('reader-annotations'),
          itemCount: items.length,
          itemBuilder: (context, index) {
            final item = items[index];
            final detail = item.note?.trim().isNotEmpty == true
                ? item.note!.trim()
                : item.selectedText?.trim();
            return Semantics(
              button: true,
              selected: item.pageNumber == currentPage,
              child: ListTile(
                leading: const Icon(Icons.comment_outlined),
                title: Text(item.type.replaceAll('_', ' ')),
                subtitle: Text(
                  detail?.isNotEmpty == true
                      ? '${strings.pageNumber(item.pageNumber)} · $detail'
                      : strings.pageNumber(item.pageNumber),
                  maxLines: 3,
                  overflow: TextOverflow.ellipsis,
                ),
                onTap: () => onSelectPage(item.pageNumber),
              ),
            );
          },
        );
      },
    );
  }
}
