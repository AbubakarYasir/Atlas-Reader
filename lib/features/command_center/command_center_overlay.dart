import 'package:flutter/material.dart';

import '../../bookmark_grouping.dart';
import '../../bookmark_tree.dart';
import '../../database.dart';

class CommandCenterResult {
  const CommandCenterResult({
    required this.bookmark,
    required this.bookTitle,
    required this.breadcrumb,
  });

  final Bookmark bookmark;
  final String bookTitle;
  final String breadcrumb;
}

/// A focused, global search dialog opened by Ctrl+K.
class CommandCenterOverlay extends StatefulWidget {
  const CommandCenterOverlay({
    super.key,
    required this.database,
    required this.onSelected,
  });

  final AppDatabase database;
  final ValueChanged<CommandCenterResult> onSelected;

  @override
  State<CommandCenterOverlay> createState() => _CommandCenterOverlayState();
}

class _CommandCenterOverlayState extends State<CommandCenterOverlay> {
  final _controller = TextEditingController();
  final _focusNode = FocusNode();
  Future<List<CommandCenterResult>>? _results;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback(
      (_) => _focusNode.requestFocus(),
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  void _search(String query) {
    setState(() {
      _results = query.trim().isEmpty ? null : _findResults(query);
    });
  }

  Future<List<CommandCenterResult>> _findResults(String query) async {
    final matches = await widget.database.searchBookmarks(query);
    final allBookmarks = await widget.database.getAllBookmarks();
    final byId = BookmarkTree.indexById(allBookmarks);

    return matches.map((bookmark) {
      final path = BookmarkTree.pathForBookmark(bookmark, byId);
      return CommandCenterResult(
        bookmark: bookmark,
        bookTitle: BookmarkGrouping.fileNameFromPath(bookmark.filePath),
        breadcrumb: BookmarkTree.displayPath(path),
      );
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 720, maxHeight: 560),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            children: [
              TextField(
                controller: _controller,
                focusNode: _focusNode,
                onChanged: _search,
                decoration: const InputDecoration(
                  prefixIcon: Icon(Icons.search),
                  labelText: 'Search all bookmarks',
                  hintText: 'Type a bookmark title...',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 12),
              Expanded(child: _buildResults()),
              const SizedBox(height: 8),
              const Text('Enter a search term. Press Esc to close.'),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildResults() {
    if (_results == null) {
      return const Center(child: Text('Search bookmarks across your library.'));
    }

    return FutureBuilder<List<CommandCenterResult>>(
      future: _results,
      builder: (context, snapshot) {
        if (snapshot.connectionState != ConnectionState.done) {
          return const Center(child: CircularProgressIndicator());
        }
        if (snapshot.hasError) {
          return Center(child: Text('Search failed: ${snapshot.error}'));
        }
        final results = snapshot.data ?? const [];
        if (results.isEmpty) {
          return const Center(child: Text('No bookmarks found.'));
        }

        return ListView.builder(
          itemCount: results.length,
          itemBuilder: (context, index) {
            final result = results[index];
            final page = result.bookmark.pageIndex == null
                ? 'Folder'
                : 'Page ${result.bookmark.pageIndex! + 1}';
            return ListTile(
              leading: const Icon(Icons.bookmark_outline),
              title: Text(result.bookmark.title),
              subtitle: Text('${result.bookTitle}\n${result.breadcrumb}'),
              isThreeLine: true,
              trailing: Text(page),
              onTap: () => widget.onSelected(result),
            );
          },
        );
      },
    );
  }
}
