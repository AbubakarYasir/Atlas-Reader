import 'package:flutter/material.dart';

import '../../bookmark_grouping.dart';
import '../../bookmark_tree.dart';
import '../../database.dart';

class CommandCenterResult {
  const CommandCenterResult({
    this.bookmark,
    required this.filePath,
    required this.bookTitle,
    required this.breadcrumb,
    this.pageNumber,
  });

  /// The matched bookmark, or null when the result matched only a PDF file.
  final Bookmark? bookmark;
  final String filePath;
  final String bookTitle;
  final String breadcrumb;

  /// 1-based page number for bookmark matches; null for file matches or
  /// folders (callers fall back to page 1).
  final int? pageNumber;
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
    final fileMatches = await widget.database.searchLibraryFileNames(query);
    final allBookmarks = await widget.database.getAllBookmarks();
    final byId = BookmarkTree.indexById(allBookmarks);

    final results = <CommandCenterResult>[];
    final coveredFilePaths = <String>{};

    for (final bookmark in matches) {
      coveredFilePaths.add(bookmark.filePath);
      final path = BookmarkTree.pathForBookmark(bookmark, byId);
      results.add(
        CommandCenterResult(
          bookmark: bookmark,
          filePath: bookmark.filePath,
          bookTitle: BookmarkGrouping.fileNameFromPath(bookmark.filePath),
          breadcrumb: BookmarkTree.displayPath(path),
          pageNumber: bookmark.isFolder || bookmark.pageIndex == null
              ? null
              : bookmark.pageIndex! + 1,
        ),
      );
    }

    for (final filePath in fileMatches) {
      if (!coveredFilePaths.add(filePath)) continue;
      final fileName = BookmarkGrouping.fileNameFromPath(filePath);
      results.add(
        CommandCenterResult(
          filePath: filePath,
          bookTitle: fileName,
          breadcrumb: filePath,
        ),
      );
    }

    return results;
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
                  hintText: 'Bookmark title or PDF file name...',
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
          return const Center(child: Text('No matches found.'));
        }

        return ListView.builder(
          itemCount: results.length,
          itemBuilder: (context, index) {
            final result = results[index];
            final isFileMatch = result.bookmark == null;
            final page = isFileMatch
                ? 'PDF file'
                : result.pageNumber == null
                ? 'Folder'
                : 'Page ${result.pageNumber}';
            return ListTile(
              leading: Icon(
                isFileMatch ? Icons.menu_book_outlined : Icons.bookmark_outline,
              ),
              title: Text(result.bookTitle),
              subtitle: Text(
                isFileMatch ? result.breadcrumb : result.breadcrumb,
              ),
              isThreeLine: false,
              trailing: Text(page),
              onTap: () => widget.onSelected(result),
            );
          },
        );
      },
    );
  }
}
