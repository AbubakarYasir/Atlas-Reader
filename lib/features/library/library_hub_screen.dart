import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../bookmark_grouping.dart';
import '../../bookmark_tree.dart';
import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../reader/pdf_reader_screen.dart';
import 'library_folder_manager.dart';
import 'visual_bookshelf.dart';

enum _HubDestination {
  library,
  recents,
  bookmarks,
  favorites,
  folders,
  settings,
}

/// The primary Windows workspace. Every core destination stays one click away,
/// while the reader remains distraction-free after a book is opened.
class LibraryHubScreen extends StatefulWidget {
  const LibraryHubScreen({
    super.key,
    required this.database,
    required this.fileSystem,
    required this.folderManager,
    required this.onCreateBookmark,
    required this.onOpenPdf,
    required this.onOpenFile,
    required this.onOpenAdvancedBookmarks,
    required this.onOpenCommandCenter,
    required this.themeMode,
    required this.onThemeModeChanged,
    required this.onLocaleChanged,
  });

  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final LibraryFolderManager folderManager;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;
  final VoidCallback onOpenPdf;
  final Future<void> Function(String filePath, {int pageNumber}) onOpenFile;
  final VoidCallback onOpenAdvancedBookmarks;
  final VoidCallback onOpenCommandCenter;
  final ThemeMode themeMode;
  final ValueChanged<ThemeMode> onThemeModeChanged;
  final ValueChanged<Locale> onLocaleChanged;

  @override
  State<LibraryHubScreen> createState() => _LibraryHubScreenState();
}

class _LibraryHubScreenState extends State<LibraryHubScreen> {
  _HubDestination _destination = _HubDestination.library;
  String _bookmarkQuery = '';
  LibraryScanProgress? _scanProgress;
  bool _scanning = false;

  @override
  void initState() {
    super.initState();
    widget.folderManager.onProgress = _handleScanProgress;
  }

  @override
  void dispose() {
    widget.folderManager.onProgress = null;
    super.dispose();
  }

  void _handleScanProgress(LibraryScanProgress progress) {
    if (mounted) setState(() => _scanProgress = progress);
  }

  String get _title => switch (_destination) {
    _HubDestination.library => 'Library',
    _HubDestination.recents => 'Recents',
    _HubDestination.bookmarks => 'Bookmarks',
    _HubDestination.favorites => 'Favorites',
    _HubDestination.folders => 'Folders',
    _HubDestination.settings => 'Settings',
  };

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final wide = constraints.maxWidth >= 900;
        return Scaffold(
          appBar: AppBar(
            title: Text(_title),
            actions: [
              FilledButton.tonalIcon(
                onPressed: widget.onOpenPdf,
                icon: const Icon(Icons.file_open_outlined),
                label: const Text('Open PDF'),
              ),
              const SizedBox(width: 8),
              IconButton(
                tooltip: 'Quick open and bookmark search (Ctrl+K)',
                onPressed: widget.onOpenCommandCenter,
                icon: const Icon(Icons.manage_search_outlined),
              ),
              const SizedBox(width: 8),
            ],
          ),
          drawer: wide ? null : Drawer(child: _buildNavigation(false)),
          body: Row(
            children: [
              if (wide) _buildNavigation(true),
              if (wide) const VerticalDivider(width: 1),
              Expanded(child: _buildDestination()),
            ],
          ),
        );
      },
    );
  }

  Widget _buildNavigation(bool rail) {
    const destinations = [
      (Icons.local_library_outlined, Icons.local_library, 'Library'),
      (Icons.history_outlined, Icons.history, 'Recents'),
      (Icons.bookmarks_outlined, Icons.bookmarks, 'Bookmarks'),
      (Icons.star_outline, Icons.star, 'Favorites'),
      (Icons.folder_outlined, Icons.folder, 'Folders'),
      (Icons.settings_outlined, Icons.settings, 'Settings'),
    ];
    if (rail) {
      return NavigationRail(
        selectedIndex: _destination.index,
        labelType: NavigationRailLabelType.all,
        leading: Padding(
          padding: const EdgeInsets.only(bottom: 8),
          child: Semantics(
            label: 'Atlas Reader',
            child: const Icon(Icons.auto_stories_outlined, size: 30),
          ),
        ),
        destinations: [
          for (final item in destinations)
            NavigationRailDestination(
              icon: Icon(item.$1),
              selectedIcon: Icon(item.$2),
              label: Text(item.$3),
            ),
        ],
        onDestinationSelected: (index) =>
            setState(() => _destination = _HubDestination.values[index]),
      );
    }

    return SafeArea(
      child: NavigationDrawer(
        selectedIndex: _destination.index,
        onDestinationSelected: (index) {
          setState(() => _destination = _HubDestination.values[index]);
          Navigator.of(context).pop();
        },
        children: [
          const Padding(
            padding: EdgeInsets.fromLTRB(28, 20, 16, 12),
            child: Text('Atlas Reader', style: TextStyle(fontSize: 20)),
          ),
          for (final item in destinations)
            NavigationDrawerDestination(
              icon: Icon(item.$1),
              selectedIcon: Icon(item.$2),
              label: Text(item.$3),
            ),
        ],
      ),
    );
  }

  Widget _buildDestination() => switch (_destination) {
    _HubDestination.library => _bookshelf(key: const ValueKey('library')),
    _HubDestination.recents => _bookshelf(
      key: const ValueKey('recents'),
      sortBy: LibrarySortBy.lastOpened,
      ascending: false,
      onlyOpened: true,
    ),
    _HubDestination.bookmarks => _buildBookmarks(),
    _HubDestination.favorites => _bookshelf(
      key: const ValueKey('favorites'),
      favorites: true,
    ),
    _HubDestination.folders => _buildFolders(),
    _HubDestination.settings => _buildSettings(),
  };

  Widget _bookshelf({
    required Key key,
    LibrarySortBy sortBy = LibrarySortBy.title,
    bool ascending = true,
    bool favorites = false,
    bool onlyOpened = false,
  }) {
    return VisualBookshelf(
      key: key,
      database: widget.database,
      fileSystem: widget.fileSystem,
      onCreateBookmark: widget.onCreateBookmark,
      initialSortBy: sortBy,
      initialSortAscending: ascending,
      initialOnlyFavorites: favorites,
      onlyOpened: onlyOpened,
    );
  }

  Widget _buildBookmarks() {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              Expanded(
                child: TextField(
                  decoration: const InputDecoration(
                    prefixIcon: Icon(Icons.search),
                    hintText: 'Search bookmarks in every book…',
                    border: OutlineInputBorder(),
                    isDense: true,
                  ),
                  onChanged: (value) => setState(() => _bookmarkQuery = value),
                ),
              ),
              const SizedBox(width: 8),
              OutlinedButton.icon(
                onPressed: widget.onOpenAdvancedBookmarks,
                icon: const Icon(Icons.account_tree_outlined),
                label: const Text('Organize & sync'),
              ),
            ],
          ),
        ),
        Expanded(
          child: FutureBuilder<(List<Bookmark>, List<LibraryFile>)>(
            future:
                Future.wait<Object>([
                  widget.database.getAllBookmarks(),
                  widget.database.getLibraryFiles(),
                ]).then(
                  (items) => (
                    items[0] as List<Bookmark>,
                    items[1] as List<LibraryFile>,
                  ),
                ),
            builder: (context, snapshot) {
              if (!snapshot.hasData) {
                return const Center(child: CircularProgressIndicator());
              }
              final all = snapshot.data!.$1;
              final query = _bookmarkQuery.trim().toLowerCase();
              final matches = query.isEmpty
                  ? all
                  : all.where((bookmark) {
                      return bookmark.title.toLowerCase().contains(query) ||
                          (bookmark.description ?? '').toLowerCase().contains(
                            query,
                          ) ||
                          bookmark.filePath.toLowerCase().contains(query);
                    }).toList();
              if (matches.isEmpty) {
                return const Center(child: Text('No bookmarks found.'));
              }
              final byId = BookmarkTree.indexById(all);
              final books = {
                for (final book in snapshot.data!.$2) book.filePath: book,
              };
              return ListView.builder(
                itemCount: matches.length,
                itemBuilder: (context, index) {
                  final bookmark = matches[index];
                  final page = bookmark.pageIndex == null
                      ? null
                      : bookmark.pageIndex! + 1;
                  final book = books[bookmark.filePath];
                  final title = book?.title?.trim().isNotEmpty == true
                      ? book!.title!
                      : BookmarkGrouping.fileNameFromPath(bookmark.filePath);
                  final breadcrumb = BookmarkTree.displayPath(
                    BookmarkTree.pathForBookmark(bookmark, byId),
                  );
                  return Material(
                    color: Colors.transparent,
                    child: ListTile(
                      leading: Icon(
                        bookmark.isFolder
                            ? Icons.folder_outlined
                            : Icons.bookmark_outline,
                      ),
                      title: Text(bookmark.title),
                      subtitle: Text('$title  •  $breadcrumb'),
                      trailing: page == null ? null : Text('Page $page'),
                      enabled: page != null,
                      onTap: page == null
                          ? null
                          : () => widget.onOpenFile(
                              bookmark.filePath,
                              pageNumber: page,
                            ),
                    ),
                  );
                },
              );
            },
          ),
        ),
      ],
    );
  }

  Future<void> _scanFolders() async {
    if (_scanning) return;
    setState(() {
      _scanning = true;
      _scanProgress = null;
    });
    try {
      await widget.folderManager.rescanAll(restartAfterCurrent: true);
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _addFolder() async {
    final path = await FilePicker.getDirectoryPath(
      dialogTitle: 'Choose a folder containing PDF or EPUB books',
    );
    if (path == null) return;
    await widget.folderManager.addFolder(path);
    await _scanFolders();
  }

  Future<void> _removeFolder(LibraryFolder folder) async {
    final remove = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Remove library folder?'),
        content: Text(
          'Atlas Reader will remove this folder from its local index. '
          'No book or annotation files will be deleted.\n\n${folder.path}',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Remove'),
          ),
        ],
      ),
    );
    if (remove == true) {
      await widget.folderManager.removeFolder(folder.id, folder.path);
    }
  }

  Widget _buildFolders() {
    final progress = _scanProgress;
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              FilledButton.icon(
                onPressed: _addFolder,
                icon: const Icon(Icons.create_new_folder_outlined),
                label: const Text('Add folder'),
              ),
              const SizedBox(width: 8),
              OutlinedButton.icon(
                onPressed: _scanning ? null : _scanFolders,
                icon: _scanning
                    ? const SizedBox.square(
                        dimension: 16,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.refresh),
                label: const Text('Scan now'),
              ),
              if (progress != null) ...[
                const SizedBox(width: 16),
                Expanded(
                  child: Text(
                    'Indexed ${progress.indexed} of ${progress.total} in '
                    '${_fileName(progress.folderPath)}',
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ],
          ),
        ),
        Expanded(
          child: FutureBuilder<List<LibraryFolder>>(
            future: widget.database.getLibraryFolders(),
            builder: (context, snapshot) {
              if (!snapshot.hasData) {
                return const Center(child: CircularProgressIndicator());
              }
              final folders = snapshot.data!;
              if (folders.isEmpty) {
                return const Center(
                  child: Text(
                    'No library folders yet. Add one above, or use Open PDF '
                    'to read a document without adding its folder.',
                    textAlign: TextAlign.center,
                  ),
                );
              }
              return ListView(
                padding: const EdgeInsets.fromLTRB(12, 0, 12, 16),
                children: [for (final folder in folders) _folderTile(folder)],
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _folderTile(LibraryFolder folder) {
    return Material(
      color: Colors.transparent,
      child: ExpansionTile(
        leading: const Icon(Icons.folder_outlined),
        title: Text(folder.path),
        trailing: IconButton(
          tooltip: 'Remove folder from library',
          onPressed: () => _removeFolder(folder),
          icon: const Icon(Icons.remove_circle_outline),
        ),
        children: [
          FutureBuilder<List<LibraryFile>>(
            future: widget.database.getLibraryFiles(folderId: folder.id),
            builder: (context, snapshot) {
              final files = snapshot.data ?? const <LibraryFile>[];
              if (!snapshot.hasData) {
                return const Padding(
                  padding: EdgeInsets.all(12),
                  child: LinearProgressIndicator(),
                );
              }
              if (files.isEmpty) {
                return const ListTile(title: Text('No indexed books'));
              }
              return Column(
                children: [
                  for (final file in files)
                    Material(
                      color: Colors.transparent,
                      child: ListTile(
                        contentPadding: const EdgeInsetsDirectional.only(
                          start: 52,
                          end: 16,
                        ),
                        leading: Icon(
                          file.format.toUpperCase() == 'PDF'
                              ? Icons.picture_as_pdf_outlined
                              : Icons.book_outlined,
                        ),
                        title: Text(file.title ?? file.fileName),
                        subtitle: Text(file.filePath),
                        onTap: file.format.toUpperCase() == 'PDF'
                            ? () => widget.onOpenFile(
                                file.filePath,
                                pageNumber: file.currentPage,
                              )
                            : null,
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildSettings() {
    return ListView(
      padding: const EdgeInsets.all(20),
      children: [
        _settingsSection(
          'Library & storage',
          'Control indexed locations and document-safe storage.',
          [
            ListTile(
              leading: const Icon(Icons.folder_copy_outlined),
              title: const Text('Library folders'),
              subtitle: const Text('Add, remove, inspect, or rescan folders'),
              trailing: const Icon(Icons.chevron_right),
              onTap: () =>
                  setState(() => _destination = _HubDestination.folders),
            ),
            ListTile(
              leading: const Icon(Icons.file_open_outlined),
              title: const Text('Open an external PDF'),
              subtitle: const Text('No library registration required'),
              onTap: widget.onOpenPdf,
            ),
          ],
        ),
        _settingsSection(
          'Appearance & language',
          'Choose the app appearance and interface direction.',
          [
            Padding(
              padding: const EdgeInsets.all(16),
              child: SegmentedButton<ThemeMode>(
                segments: const [
                  ButtonSegment(value: ThemeMode.system, label: Text('System')),
                  ButtonSegment(value: ThemeMode.light, label: Text('Light')),
                  ButtonSegment(value: ThemeMode.dark, label: Text('Dark')),
                ],
                selected: {widget.themeMode},
                onSelectionChanged: (value) =>
                    widget.onThemeModeChanged(value.first),
              ),
            ),
            ListTile(
              leading: const Icon(Icons.language_outlined),
              title: const Text('Interface language'),
              subtitle: const Text(
                'English or العربية with mirrored RTL layout',
              ),
              trailing: Wrap(
                spacing: 4,
                children: [
                  TextButton(
                    onPressed: () => widget.onLocaleChanged(const Locale('en')),
                    child: const Text('English'),
                  ),
                  TextButton(
                    onPressed: () => widget.onLocaleChanged(const Locale('ar')),
                    child: const Text('العربية'),
                  ),
                ],
              ),
            ),
          ],
        ),
        _settingsSection(
          'Reader & writing',
          'Per-document controls stay in the reader so each book can be tuned.',
          const [
            ListTile(
              leading: Icon(Icons.chrome_reader_mode_outlined),
              title: Text('Display controls'),
              subtitle: Text(
                'Day, Night, OLED, Warm Parchment, brightness, margin crop, '
                'zoom, continuous scrolling, and page navigation',
              ),
            ),
            ListTile(
              leading: Icon(Icons.draw_outlined),
              title: Text('Pen and highlighter'),
              subtitle: Text(
                'Colors, thickness, stroke/area eraser, undo, redo, and safe PDF save',
              ),
            ),
          ],
        ),
        _settingsSection(
          'Accessibility & keyboard',
          'Core actions remain available without a mouse.',
          const [
            ListTile(
              leading: Icon(Icons.keyboard_outlined),
              title: Text('Shortcuts'),
              subtitle: Text(
                'Ctrl+K quick open • Ctrl+B bookmark • Ctrl+S save • '
                'Ctrl+Z undo • Ctrl+Shift+Z redo',
              ),
            ),
            ListTile(
              leading: Icon(Icons.accessibility_new_outlined),
              title: Text('Assistive technology'),
              subtitle: Text(
                'Semantic labels, focus navigation, live save and error announcements',
              ),
            ),
          ],
        ),
        _settingsSection(
          'Data & safety',
          'Your documents remain portable and authoritative.',
          const [
            ListTile(
              leading: Icon(Icons.verified_user_outlined),
              title: Text('Document is the database'),
              subtitle: Text(
                'Bookmarks and standard PDF ink are embedded in the PDF; '
                'the local SQLite index accelerates search and can be backed up separately.',
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _settingsSection(
    String title,
    String subtitle,
    List<Widget> children,
  ) {
    return Card(
      margin: const EdgeInsets.only(bottom: 14),
      clipBehavior: Clip.antiAlias,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 16, 16, 8),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(title, style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 2),
                Text(subtitle, style: Theme.of(context).textTheme.bodySmall),
              ],
            ),
          ),
          ...children,
        ],
      ),
    );
  }

  String _fileName(String path) {
    final normalized = path.replaceAll('\\', '/');
    final parts = normalized
        .split('/')
        .where((part) => part.isNotEmpty)
        .toList();
    return parts.isEmpty ? path : parts.last;
  }
}
