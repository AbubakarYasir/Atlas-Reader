import 'package:flutter/material.dart';

import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../reader/pdf_reader_screen.dart';
import '../../widgets/accessible_bookmark_tile.dart';

/// Browse every PDF discovered across the registered library folders.
class LibraryFilesScreen extends StatefulWidget {
  const LibraryFilesScreen({
    super.key,
    required this.database,
    required this.fileSystem,
    required this.onCreateBookmark,
  });

  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;

  @override
  State<LibraryFilesScreen> createState() => _LibraryFilesScreenState();
}

class _LibraryFilesScreenState extends State<LibraryFilesScreen> {
  String _query = '';

  Future<List<LibraryFile>> _load() {
    return widget.database.getLibraryFiles(query: _query);
  }

  Future<void> _openFile(LibraryFile file) async {
    if (!await widget.fileSystem.exists(file.filePath)) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('The PDF file is missing from disk.'),
          backgroundColor: Colors.red,
        ),
      );
      return;
    }

    if (!mounted) return;
    await Navigator.of(context).push<void>(
      MaterialPageRoute(
        builder: (_) => PdfReaderScreen(
          filePath: file.filePath,
          fileSystem: widget.fileSystem,
          onCreateBookmark: (draft) =>
              widget.onCreateBookmark(file.filePath, draft),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Library')),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(16),
            child: TextField(
              onChanged: (value) => setState(() => _query = value),
              decoration: const InputDecoration(
                prefixIcon: Icon(Icons.search),
                labelText: 'Filter by file name',
                border: OutlineInputBorder(),
              ),
            ),
          ),
          Expanded(
            child: FutureBuilder<List<LibraryFile>>(
              future: _load(),
              builder: (context, snapshot) {
                if (snapshot.connectionState != ConnectionState.done) {
                  return const Center(child: CircularProgressIndicator());
                }
                if (snapshot.hasError) {
                  return Center(child: Text('Error: ${snapshot.error}'));
                }
                final files = snapshot.data ?? const [];
                if (files.isEmpty) {
                  return const Center(
                    child: Text(
                      'No PDFs found. Add library folders in Settings '
                      'to scan for documents.',
                    ),
                  );
                }

                return ListView.builder(
                  itemCount: files.length,
                  itemBuilder: (context, index) {
                    final file = files[index];
                    final lastScanned = file.lastScanned;
                    return AccessibleBookmarkTile(
                      leading: const Icon(Icons.menu_book_outlined),
                      title: Text(file.fileName),
                      subtitle: Text(
                        '${file.bookmarkCount} bookmark'
                        '${file.bookmarkCount == 1 ? '' : 's'} · '
                        'scanned ${lastScanned.toLocal().toString().substring(0, 16)}',
                      ),
                      onTap: () => _openFile(file),
                    );
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
