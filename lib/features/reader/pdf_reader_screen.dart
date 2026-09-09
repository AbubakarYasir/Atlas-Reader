import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:syncfusion_flutter_pdfviewer/pdfviewer.dart';

import '../../core/file_system/document_file_system.dart';

class ReaderBookmarkDraft {
  const ReaderBookmarkDraft({
    required this.title,
    required this.pageNumber,
    this.description,
    this.tags,
  });

  final String title;
  final int pageNumber;
  final String? description;
  final String? tags;
}

class _CreateReaderBookmarkIntent extends Intent {
  const _CreateReaderBookmarkIntent();
}

/// Full-screen PDF reader with continuous scrolling, zoom, and active-page
/// bookmarking. File bytes are loaded through [DocumentFileSystem] before the
/// viewer is mounted, keeping file I/O off the interaction path.
class PdfReaderScreen extends StatefulWidget {
  const PdfReaderScreen({
    super.key,
    required this.filePath,
    required this.fileSystem,
    required this.onCreateBookmark,
  });

  final String filePath;
  final DocumentFileSystem fileSystem;
  final Future<void> Function(ReaderBookmarkDraft draft) onCreateBookmark;

  @override
  State<PdfReaderScreen> createState() => _PdfReaderScreenState();
}

class _PdfReaderScreenState extends State<PdfReaderScreen> {
  final PdfViewerController _viewerController = PdfViewerController();
  late final Future<List<int>> _documentBytes;
  var _activePage = 1;
  var _pageCount = 0;
  var _savingBookmark = false;

  @override
  void initState() {
    super.initState();
    _documentBytes = widget.fileSystem.readAsBytesInBackground(widget.filePath);
  }

  @override
  void dispose() {
    _viewerController.dispose();
    super.dispose();
  }

  Future<void> _openBookmarkDialog() async {
    final titleController = TextEditingController();
    final descriptionController = TextEditingController();
    final tagsController = TextEditingController();

    final draft = await showDialog<ReaderBookmarkDraft>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Bookmark this page'),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Page $_activePage${_pageCount == 0 ? '' : ' of $_pageCount'}',
              ),
              const SizedBox(height: 12),
              TextField(
                controller: titleController,
                autofocus: true,
                decoration: const InputDecoration(
                  labelText: 'Bookmark title',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: descriptionController,
                decoration: const InputDecoration(
                  labelText: 'Note (optional)',
                  border: OutlineInputBorder(),
                ),
                maxLines: 2,
              ),
              const SizedBox(height: 12),
              TextField(
                controller: tagsController,
                decoration: const InputDecoration(
                  labelText: 'Tags (optional, comma-separated)',
                  border: OutlineInputBorder(),
                ),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              final title = titleController.text.trim();
              if (title.isEmpty) return;
              Navigator.pop(
                context,
                ReaderBookmarkDraft(
                  title: title,
                  pageNumber: _activePage,
                  description: descriptionController.text.trim(),
                  tags: tagsController.text.trim(),
                ),
              );
            },
            child: const Text('Save bookmark'),
          ),
        ],
      ),
    );

    titleController.dispose();
    descriptionController.dispose();
    tagsController.dispose();

    if (draft == null || !mounted) return;

    setState(() => _savingBookmark = true);
    try {
      await widget.onCreateBookmark(draft);
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Bookmark saved for page ${draft.pageNumber}.')),
      );
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Could not save bookmark: $error'),
          backgroundColor: Colors.red,
        ),
      );
    } finally {
      if (mounted) setState(() => _savingBookmark = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Shortcuts(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.keyB, control: true):
            _CreateReaderBookmarkIntent(),
      },
      child: Actions(
        actions: {
          _CreateReaderBookmarkIntent:
              CallbackAction<_CreateReaderBookmarkIntent>(
                onInvoke: (_) {
                  _openBookmarkDialog();
                  return null;
                },
              ),
        },
        child: Focus(
          autofocus: true,
          child: Scaffold(
            appBar: AppBar(
              title: Text(
                'Reader - page $_activePage${_pageCount == 0 ? '' : ' of $_pageCount'}',
              ),
              actions: [
                IconButton(
                  tooltip: 'Zoom out',
                  icon: const Icon(Icons.zoom_out),
                  onPressed: () {
                    _viewerController.zoomLevel =
                        (_viewerController.zoomLevel - .25).clamp(1, 3);
                  },
                ),
                IconButton(
                  tooltip: 'Zoom in',
                  icon: const Icon(Icons.zoom_in),
                  onPressed: () {
                    _viewerController.zoomLevel =
                        (_viewerController.zoomLevel + .25).clamp(1, 3);
                  },
                ),
                IconButton(
                  tooltip: 'Bookmark active page (Ctrl+B)',
                  icon: _savingBookmark
                      ? const SizedBox.square(
                          dimension: 18,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.bookmark_add_outlined),
                  onPressed: _savingBookmark ? null : _openBookmarkDialog,
                ),
              ],
            ),
            body: FutureBuilder<List<int>>(
              future: _documentBytes,
              builder: (context, snapshot) {
                if (snapshot.connectionState != ConnectionState.done) {
                  return const Center(child: CircularProgressIndicator());
                }
                if (snapshot.hasError || !snapshot.hasData) {
                  return Center(
                    child: Padding(
                      padding: const EdgeInsets.all(24),
                      child: Text('Could not open this PDF: ${snapshot.error}'),
                    ),
                  );
                }

                return SfPdfViewer.memory(
                  Uint8List.fromList(snapshot.data!),
                  controller: _viewerController,
                  enableDoubleTapZooming: true,
                  enableTextSelection: true,
                  onPageChanged: (details) {
                    setState(() => _activePage = details.newPageNumber);
                  },
                  onDocumentLoaded: (details) {
                    setState(() => _pageCount = details.document.pages.count);
                  },
                  onDocumentLoadFailed: (details) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        content: Text(
                          'Could not render PDF: ${details.description}',
                        ),
                        backgroundColor: Colors.red,
                      ),
                    );
                  },
                );
              },
            ),
          ),
        ),
      ),
    );
  }
}
