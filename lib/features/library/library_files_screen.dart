import 'package:flutter/material.dart';

import '../../core/file_system/document_file_system.dart';
import '../../database.dart';
import '../reader/pdf_reader_screen.dart';
import 'visual_bookshelf.dart';

/// Browse documents discovered across the registered library folders
/// using the visual bookshelf with cover grid, detailed list, and filters.
class LibraryFilesScreen extends StatelessWidget {
  const LibraryFilesScreen({
    super.key,
    required this.database,
    required this.fileSystem,
    required this.onCreateBookmark,
    this.onSelectPdf,
    this.onOpenSettings,
    this.onOpenBookmarks,
  });

  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;
  final ValueChanged<String>? onSelectPdf;
  final VoidCallback? onOpenSettings;
  final VoidCallback? onOpenBookmarks;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Atlas Reader'),
        actions: [
          if (onOpenBookmarks != null)
            IconButton(
              tooltip: 'Browse all bookmarks and sync',
              onPressed: onOpenBookmarks,
              icon: const Icon(Icons.bookmarks_outlined),
            ),
          if (onOpenSettings != null)
            IconButton(
              tooltip: 'Settings and library folders',
              onPressed: onOpenSettings,
              icon: const Icon(Icons.settings_outlined),
            ),
          const SizedBox(width: 8),
        ],
      ),
      body: VisualBookshelf(
        database: database,
        fileSystem: fileSystem,
        onCreateBookmark: onCreateBookmark,
        onSelectPdf: onSelectPdf,
      ),
    );
  }
}
