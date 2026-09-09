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
  });

  final AppDatabase database;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(String filePath, ReaderBookmarkDraft draft)
  onCreateBookmark;
  final ValueChanged<String>? onSelectPdf;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Library Bookshelf')),
      body: VisualBookshelf(
        database: database,
        fileSystem: fileSystem,
        onCreateBookmark: onCreateBookmark,
        onSelectPdf: onSelectPdf,
      ),
    );
  }
}
