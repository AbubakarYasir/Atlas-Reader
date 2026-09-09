import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../database.dart';
import '../../widgets/accessible_bookmark_tile.dart';
import 'library_folder_manager.dart';

/// Manage the folders that are scanned to build the library.
class LibraryFoldersScreen extends StatefulWidget {
  const LibraryFoldersScreen({
    super.key,
    required this.database,
    required this.manager,
  });

  final AppDatabase database;
  final LibraryFolderManager manager;

  @override
  State<LibraryFoldersScreen> createState() => _LibraryFoldersScreenState();
}

class _LibraryFoldersScreenState extends State<LibraryFoldersScreen> {
  bool _isScanning = false;
  String _status = '';

  @override
  void initState() {
    super.initState();
    widget.manager.onProgress = (progress) {
      if (!mounted) return;
      setState(() {
        _isScanning = true;
        _status =
            'Indexed ${progress.indexed} of ${progress.total} books in '
            '${_folderName(progress.folderPath)}…';
      });
    };
  }

  @override
  void dispose() {
    widget.manager.onProgress = null;
    super.dispose();
  }

  Future<List<LibraryFolder>> _loadFolders() {
    return widget.database.getLibraryFolders();
  }

  Future<void> _addFolder() async {
    final path = await FilePicker.getDirectoryPath(
      dialogTitle: 'Choose a library folder',
    );
    if (path == null || !mounted) return;

    await widget.manager.addFolder(path);
    await _rescan();
    if (!mounted) return;
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('Library folder added.')));
    setState(() {});
  }

  Future<void> _removeFolder(LibraryFolder folder) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Remove library folder?'),
        content: Text(
          'This removes the folder from Atlas Reader’s local index. '
          'It will not delete any books or annotations.\n\n${folder.path}',
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
    if (confirmed != true) return;
    await widget.manager.removeFolder(folder.id, folder.path);
    if (!mounted) return;
    setState(() {});
  }

  Future<void> _rescan() async {
    setState(() {
      _isScanning = true;
      _status = 'Scanning folders...';
    });
    final report = await widget.manager.rescanAll(restartAfterCurrent: true);
    if (!mounted) return;
    setState(() {
      _isScanning = false;
      _status = report.foldersScanned == 0
          ? 'Add a folder to start building your library.'
          : 'Scanned ${report.foldersScanned} folder'
                '${report.foldersScanned == 1 ? '' : 's'} — '
                '${report.filesFound} books indexed.';
    });
  }

  String _folderName(String path) {
    final parts = path
        .replaceAll('\\', '/')
        .split('/')
        .where((part) => part.isNotEmpty)
        .toList();
    return parts.isEmpty ? path : parts.last;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Library folders')),
      body: FutureBuilder<List<LibraryFolder>>(
        future: _loadFolders(),
        builder: (context, snapshot) {
          final folders = snapshot.data ?? const <LibraryFolder>[];

          return ListView(
            padding: const EdgeInsets.all(16),
            children: [
              Row(
                children: [
                  Expanded(
                    child: FilledButton.icon(
                      onPressed: _addFolder,
                      icon: const Icon(Icons.create_new_folder_outlined),
                      label: const Text('Add folder'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: OutlinedButton.icon(
                      onPressed: _isScanning ? null : _rescan,
                      icon: _isScanning
                          ? const SizedBox.square(
                              dimension: 16,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : const Icon(Icons.refresh),
                      label: const Text('Scan now'),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 12),
              if (_status.isNotEmpty)
                Text(
                  _status,
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.bodySmall,
                ),
              const SizedBox(height: 20),
              if (folders.isEmpty)
                Padding(
                  padding: const EdgeInsets.all(24),
                  child: Text(
                    'No folders yet. Add a folder containing PDFs or EPUBs to start '
                    'indexing your library.',
                    textAlign: TextAlign.center,
                  ),
                )
              else
                ...folders.map((folder) {
                  return AccessibleBookmarkTile(
                    leading: const Icon(Icons.folder_outlined),
                    title: Text(folder.path),
                    subtitle: Text('Added ${folder.addedAt.toLocal()}'),
                    trailing: IconButton(
                      icon: const Icon(Icons.delete_outline),
                      tooltip: 'Remove folder',
                      onPressed: () => _removeFolder(folder),
                    ),
                  );
                }),
            ],
          );
        },
      ),
    );
  }
}
