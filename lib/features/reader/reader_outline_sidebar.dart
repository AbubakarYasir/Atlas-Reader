import 'package:flutter/material.dart';

class ReaderOutlineItem {
  const ReaderOutlineItem({
    required this.title,
    required this.pageNumber,
    this.children = const [],
  });

  final String title;
  final int pageNumber;
  final List<ReaderOutlineItem> children;
}

class ReaderOutlineSidebar extends StatefulWidget {
  const ReaderOutlineSidebar({
    super.key,
    required this.outline,
    required this.currentPage,
    required this.onSelectPage,
    required this.onClose,
  });

  final List<ReaderOutlineItem> outline;
  final int currentPage;
  final ValueChanged<int> onSelectPage;
  final VoidCallback onClose;

  @override
  State<ReaderOutlineSidebar> createState() => _ReaderOutlineSidebarState();
}

class _ReaderOutlineSidebarState extends State<ReaderOutlineSidebar> {
  String _filter = '';

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 320,
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(right: BorderSide(color: Theme.of(context).dividerColor)),
      ),
      child: Column(
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.surfaceContainerHighest.withAlpha(80),
              border: Border(bottom: BorderSide(color: Theme.of(context).dividerColor.withAlpha(60))),
            ),
            child: Row(
              children: [
                const Icon(Icons.list_alt, size: 20),
                const SizedBox(width: 8),
                Text('Table of Contents', style: Theme.of(context).textTheme.titleSmall),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.close, size: 18),
                  tooltip: 'Close sidebar',
                  onPressed: widget.onClose,
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(8),
            child: TextField(
              decoration: InputDecoration(
                hintText: 'Filter chapters...',
                prefixIcon: const Icon(Icons.search, size: 18),
                isDense: true,
                contentPadding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
              ),
              onChanged: (val) => setState(() => _filter = val.trim().toLowerCase()),
            ),
          ),
          Expanded(
            child: widget.outline.isEmpty
                ? const Center(child: Text('No outline found in document'))
                : ListView(
                    children: _buildOutlineList(widget.outline, 0),
                  ),
          ),
        ],
      ),
    );
  }

  List<Widget> _buildOutlineList(List<ReaderOutlineItem> items, int depth) {
    final list = <Widget>[];

    for (final item in items) {
      if (_filter.isNotEmpty && !item.title.toLowerCase().contains(_filter) && !_hasMatchingChild(item, _filter)) {
        continue;
      }

      final isCurrent = widget.currentPage == item.pageNumber;

      if (item.children.isEmpty) {
        list.add(
          ListTile(
            dense: true,
            contentPadding: EdgeInsetsDirectional.only(
              start: 16.0 + (depth * 16.0),
              end: 16.0,
            ),
            selected: isCurrent,
            selectedTileColor: Theme.of(context).colorScheme.primaryContainer.withAlpha(100),
            title: Text(
              item.title,
              style: TextStyle(
                fontSize: 13,
                fontWeight: isCurrent ? FontWeight.bold : FontWeight.normal,
              ),
            ),
            trailing: Text(
              '${item.pageNumber}',
              style: TextStyle(fontSize: 11, color: Theme.of(context).hintColor),
            ),
            onTap: () => widget.onSelectPage(item.pageNumber),
          ),
        );
      } else {
        list.add(
          Theme(
            data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
            child: ExpansionTile(
              initiallyExpanded: true,
              tilePadding: EdgeInsetsDirectional.only(
                start: 16.0 + (depth * 16.0),
                end: 16.0,
              ),
              title: Text(
                item.title,
                style: TextStyle(
                  fontSize: 13,
                  fontWeight: isCurrent ? FontWeight.bold : FontWeight.w600,
                ),
              ),
              trailing: InkWell(
                onTap: () => widget.onSelectPage(item.pageNumber),
                child: Text(
                  '${item.pageNumber}',
                  style: TextStyle(fontSize: 11, color: Theme.of(context).hintColor),
                ),
              ),
              children: _buildOutlineList(item.children, depth + 1),
            ),
          ),
        );
      }
    }

    return list;
  }

  bool _hasMatchingChild(ReaderOutlineItem item, String filter) {
    for (final child in item.children) {
      if (child.title.toLowerCase().contains(filter) || _hasMatchingChild(child, filter)) {
        return true;
      }
    }
    return false;
  }
}
