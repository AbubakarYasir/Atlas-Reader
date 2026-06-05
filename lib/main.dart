import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'database.dart';
import 'pdf_engine.dart';
import 'sync_engine.dart';
import 'sync_models.dart';

/// Global database instance
late final AppDatabase database;

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  database = AppDatabase();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Atlas UEP PoC',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
      ),
      home: const AtlasHomePage(),
    );
  }
}

class AtlasHomePage extends StatefulWidget {
  const AtlasHomePage({super.key});

  @override
  State<AtlasHomePage> createState() => _AtlasHomePageState();
}

class _AtlasHomePageState extends State<AtlasHomePage> {
  String? _selectedFilePath;
  String _status = 'Waiting';
  String _searchQuery = '';
  bool _isSyncing = false;
  bool _isPushing = false;
  List<BookmarkDiff> _syncDiffs = [];

  final _bookmarkTitleController = TextEditingController();
  final _pageNumberController = TextEditingController();
  final _pdfEngine = PdfEngine();
  final _syncEngine = SyncEngine(database);

  @override
  void dispose() {
    _bookmarkTitleController.dispose();
    _pageNumberController.dispose();
    super.dispose();
  }

  bool checkFileExists(String filePath) {
    return File(filePath).existsSync();
  }

  Future<String?> _resolveMissingFile(String oldPath) async {
    if (checkFileExists(oldPath)) {
      return oldPath;
    }

    final locate = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('File Missing!'),
          content: Text(
            'The file is no longer at $oldPath. Would you like to locate its new home?',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: const Text('Locate'),
            ),
          ],
        );
      },
    );

    if (locate != true) {
      return null;
    }

    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf'],
    );

    if (result == null || result.files.single.path == null) {
      return null;
    }

    final newPath = result.files.single.path!;
    final updatedBookmarks = await database.updateFilePaths(oldPath, newPath);

    if (!mounted) {
      return null;
    }

    setState(() {
      _selectedFilePath = newPath;
      _status = 'File path relinked to $newPath';
    });

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            'Successfully updated $updatedBookmarks bookmarks to the new file path!',
          ),
          backgroundColor: Colors.green,
        ),
      );
    }

    await _calculateSyncDiff();
    return newPath;
  }

  Future<void> _pickPdf() async {
    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf'],
    );

    if (result == null || result.files.single.path == null) {
      return;
    }

    final initialPath = result.files.single.path!;
    final resolvedPath = await _resolveMissingFile(initialPath);
    if (resolvedPath == null) {
      return;
    }

    setState(() {
      _selectedFilePath = resolvedPath;
      _status = 'PDF selected: $_selectedFilePath';
      _syncDiffs = [];
    });

    // Calculate sync diff after file is selected
    await _calculateSyncDiff();
  }

  Future<void> _injectBookmark() async {
    final filePath = _selectedFilePath;
    final bookmarkTitle = _bookmarkTitleController.text.trim();
    final pageNumberText = _pageNumberController.text.trim();

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }
    if (bookmarkTitle.isEmpty) {
      setState(() => _status = 'Please enter a bookmark title.');
      return;
    }
    if (pageNumberText.isEmpty) {
      setState(() => _status = 'Please enter a page number.');
      return;
    }

    final pageNumber = int.tryParse(pageNumberText);
    if (pageNumber == null || pageNumber < 1) {
      setState(() => _status = 'Page number must be a positive integer.');
      return;
    }

    try {
      // Validate page number is within bounds
      final pageCount = await _pdfEngine.getPageCount(filePath);
      if (pageCount == null) {
        setState(() => _status = 'Could not read PDF file.');
        return;
      }
      if (pageNumber > pageCount) {
        setState(
          () => _status = 'Page $pageNumber exceeds document pages ($pageCount).',
        );
        return;
      }

      // STEP 1: Dual-Layer Save - Instantly save to local database
      await database.addBookmark(
        filePath: filePath,
        title: bookmarkTitle,
        pageIndex: pageNumber - 1,
      );

      if (!mounted) return;

      // Show instant UI feedback
      setState(() {
        _status = 'Saved to local DB!';
        _bookmarkTitleController.clear();
        _pageNumberController.clear();
      });

      // STEP 2: Background injection - Do NOT await, let it run in background
      _pdfEngine.injectBookmark(
        filePath,
        bookmarkTitle,
        pageNumber - 1,
      ).then((outputPath) {
        if (!mounted) return;
        if (outputPath != null) {
          setState(() {
            _status = '✓ Synced to PDF! Bookmark embedded.';
          });
        } else {
          // Check console for detailed error logs
          setState(() {
            _status = '⚠ Local DB saved, but PDF injection failed. Check console.';
          });
        }
      }).catchError((e) {
        if (!mounted) return;
        setState(() {
          _status = '⚠ Error: $e (Check console for details)';
        });
      });
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');
    }
  }

  Future<void> _syncFile() async {
    final filePath = _selectedFilePath;

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }

    final resolvedPath = await _resolveMissingFile(filePath);
    if (resolvedPath == null) {
      setState(() {
        _isSyncing = false;
        _status = 'Sync canceled because the file could not be located.';
      });
      return;
    }

    setState(() {
      _isSyncing = true;
      _status = 'Syncing bookmarks...';
    });

    try {
      final bookmarks = await PdfEngine.extractBookmarks(resolvedPath);
      int newBookmarksCount = 0;

      for (final bookmark in bookmarks) {
        final title = bookmark['title'] as String;
        final pageIndex = bookmark['pageIndex'] as int;

        final wasAdded = await database.syncBookmark(
          filePath: resolvedPath,
          title: title,
          pageIndex: pageIndex,
        );

        if (wasAdded) {
          newBookmarksCount++;
        }
      }

      if (!mounted) return;

      setState(() {
        _status = 'Sync Complete: $newBookmarksCount new bookmarks found';
      });
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');
    } finally {
      if (mounted) {
        setState(() {
          _isSyncing = false;
        });
      }
    }
  }

  Future<void> _pushToFile() async {
    final filePath = _selectedFilePath;

    if (filePath == null) {
      setState(() => _status = 'Please select a PDF file first.');
      return;
    }

    final resolvedPath = await _resolveMissingFile(filePath);
    if (resolvedPath == null) {
      setState(() {
        _isPushing = false;
        _status = 'Commit canceled because the file could not be located.';
      });
      return;
    }

    setState(() {
      _isPushing = true;
      _status = 'Committing changes to PDF...';
    });

    // Show snackbar for sync progress
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Syncing changes...'),
          duration: Duration(seconds: 1),
        ),
      );
    }

    try {
      // Use SyncEngine for incremental reconciliation
      final result = await _syncEngine.reconcile(resolvedPath);

      if (!mounted) return;

      if (result != null) {
        final (adds, deletes) = result;
        setState(() {
          _status = 'Successfully committed $adds adds and $deletes deletes';
          _syncDiffs = [];
        });

        // Show success snackbar
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Successfully committed $adds adds and $deletes deletes'),
            backgroundColor: Colors.green,
          ),
        );
      } else {
        setState(() {
          _status = 'Failed to sync bookmarks to PDF.';
        });

        // Show error snackbar
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Failed to sync bookmarks to PDF.'),
            backgroundColor: Colors.red,
          ),
        );
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => _status = 'Error: $e');

      // Show error snackbar
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error: $e'),
          backgroundColor: Colors.red,
        ),
      );
    } finally {
      if (mounted) {
        setState(() {
          _isPushing = false;
        });
      }
    }
  }

  /// Calculate the differences between database and PDF bookmarks
  Future<void> _calculateSyncDiff() async {
    final filePath = _selectedFilePath;
    if (filePath == null) return;

    try {
      // Fetch database bookmarks for this file
      final dbBookmarks = await database.getBookmarksForFile(filePath);

      // Fetch PDF bookmarks
      final pdfBookmarks = await PdfEngine.extractBookmarks(filePath);

      // Create sets of titles for comparison
      final dbTitles = dbBookmarks.map((b) => b.title).toSet();
      final pdfTitles = pdfBookmarks.map((b) => b['title'] as String).toSet();

      final diffs = <BookmarkDiff>[];

      // Identify to add: in DB but not in PDF
      for (final bookmark in dbBookmarks) {
        if (!pdfTitles.contains(bookmark.title)) {
          diffs.add(BookmarkDiff(
            title: bookmark.title,
            pageIndex: bookmark.pageIndex,
            action: SyncAction.add,
          ));
        }
      }

      // Identify to delete: in PDF but not in DB
      for (final bookmark in pdfBookmarks) {
        final title = bookmark['title'] as String;
        final pageIndex = bookmark['pageIndex'] as int;
        if (!dbTitles.contains(title)) {
          diffs.add(BookmarkDiff(
            title: title,
            pageIndex: pageIndex,
            action: SyncAction.delete,
          ));
        }
      }

      // Identify to keep: in both
      for (final bookmark in dbBookmarks) {
        if (pdfTitles.contains(bookmark.title)) {
          diffs.add(BookmarkDiff(
            title: bookmark.title,
            pageIndex: bookmark.pageIndex,
            action: SyncAction.keep,
          ));
        }
      }

      if (mounted) {
        setState(() {
          _syncDiffs = diffs;
        });
      }
    } catch (e) {
      print('Error calculating sync diff: $e');
    }
  }

  Future<void> _showEditBookmarkDialog(Bookmark bookmark) async {
    final titleController = TextEditingController(text: bookmark.title);
    final pageController = TextEditingController(text: '${bookmark.pageIndex + 1}');

    final saved = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Edit Bookmark'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: titleController,
                decoration: const InputDecoration(
                  labelText: 'New Title',
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: pageController,
                decoration: const InputDecoration(
                  labelText: 'New Page Number',
                ),
                keyboardType: TextInputType.number,
                inputFormatters: [FilteringTextInputFormatter.digitsOnly],
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                final newTitle = titleController.text.trim();
                final newPageNumber = int.tryParse(pageController.text.trim());

                if (newTitle.isEmpty || newPageNumber == null || newPageNumber < 1) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(
                      content: Text('Please enter a valid title and page number.'),
                      backgroundColor: Colors.red,
                    ),
                  );
                  return;
                }

                Navigator.of(context).pop(true);
              },
              child: const Text('Save'),
            ),
          ],
        );
      },
    );

    if (saved != true || !mounted) {
      return;
    }

    final newTitle = titleController.text.trim();
    final newPageNumber = int.tryParse(pageController.text.trim());
    if (newTitle.isEmpty || newPageNumber == null || newPageNumber < 1) {
      return;
    }

    await database.updateBookmark(bookmark.id, newTitle, newPageNumber - 1);

    if (!mounted) return;

    final messenger = ScaffoldMessenger.of(context);
    await _calculateSyncDiff();
    setState(() {});

    messenger.showSnackBar(
      const SnackBar(
        content: Text('Bookmark edited locally. Remember to Commit Changes to update the PDF file!'),
        backgroundColor: Colors.green,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Atlas UEP PoC'),
      ),
      body: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  ElevatedButton(
                    onPressed: _pickPdf,
                    child: const Text('Select PDF'),
                  ),
                  if (_selectedFilePath != null) ...[
                    const SizedBox(height: 8),
                    Text(
                      _selectedFilePath!,
                      textAlign: TextAlign.center,
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                  const SizedBox(height: 24),
                  TextField(
                    controller: _bookmarkTitleController,
                    decoration: const InputDecoration(
                      labelText: 'Bookmark Title',
                      border: OutlineInputBorder(),
                    ),
                  ),
                  const SizedBox(height: 16),
                  TextField(
                    controller: _pageNumberController,
                    decoration: const InputDecoration(
                      labelText: 'Page Number',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    inputFormatters: [FilteringTextInputFormatter.digitsOnly],
                  ),
                  const SizedBox(height: 24),
                  Row(
                    children: [
                      Expanded(
                        child: ElevatedButton(
                          onPressed: _injectBookmark,
                          child: const Text('INJECT BOOKMARK'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: OutlinedButton(
                          onPressed: _isSyncing ? null : _syncFile,
                          child: const Text('SYNC FILE'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: OutlinedButton(
                          onPressed: (_isPushing || _syncDiffs.isEmpty) ? null : _pushToFile,
                          child: const Text('COMMIT CHANGES'),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 32),
                  // Sync Preview Section
                  if (_selectedFilePath != null && _syncDiffs.isNotEmpty) ...[
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.grey[200],
                        borderRadius: BorderRadius.circular(8),
                        border: Border.all(color: Colors.grey),
                      ),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              const Text(
                                'Sync Preview',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              Row(
                                children: [
                                  Container(
                                    padding: const EdgeInsets.symmetric(
                                      horizontal: 8,
                                      vertical: 4,
                                    ),
                                    decoration: BoxDecoration(
                                      color: Colors.green[100],
                                      borderRadius: BorderRadius.circular(4),
                                    ),
                                    child: Text(
                                      'Pending Adds: ${_syncDiffs.where((d) => d.action == SyncAction.add).length}',
                                      style: const TextStyle(color: Colors.green),
                                    ),
                                  ),
                                  const SizedBox(width: 8),
                                  Container(
                                    padding: const EdgeInsets.symmetric(
                                      horizontal: 8,
                                      vertical: 4,
                                    ),
                                    decoration: BoxDecoration(
                                      color: Colors.red[100],
                                      borderRadius: BorderRadius.circular(4),
                                    ),
                                    child: Text(
                                      'Pending Deletions: ${_syncDiffs.where((d) => d.action == SyncAction.delete).length}',
                                      style: const TextStyle(color: Colors.red),
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ),
                          const SizedBox(height: 12),
                          SizedBox(
                            height: 150,
                            child: ListView.builder(
                              itemCount: _syncDiffs.length,
                              itemBuilder: (context, index) {
                                final diff = _syncDiffs[index];
                                Color textColor;
                                IconData icon;
                                String actionText;

                                switch (diff.action) {
                                  case SyncAction.add:
                                    textColor = Colors.green;
                                    icon = Icons.add_circle;
                                    actionText = 'ADD';
                                    break;
                                  case SyncAction.delete:
                                    textColor = Colors.red;
                                    icon = Icons.remove_circle;
                                    actionText = 'REMOVE';
                                    break;
                                  case SyncAction.keep:
                                    textColor = Colors.grey;
                                    icon = Icons.check_circle;
                                    actionText = 'KEEP';
                                    break;
                                }

                                return ListTile(
                                  dense: true,
                                  leading: Icon(icon, color: textColor, size: 20),
                                  title: Text(
                                    diff.title,
                                    style: TextStyle(
                                      color: textColor,
                                      fontSize: 14,
                                    ),
                                  ),
                                  trailing: Text(
                                    'Page ${diff.pageIndex + 1}',
                                    style: const TextStyle(fontSize: 12),
                                  ),
                                  subtitle: Text(
                                    actionText,
                                    style: TextStyle(
                                      color: textColor,
                                      fontSize: 11,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                                );
                              },
                            ),
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(height: 16),
                  ],
                  Text(
                    _status,
                    textAlign: TextAlign.center,
                  ),
                ],
              ),
            ),
          ),
          // Bookmarks ListView with FutureBuilder
          Container(
            color: Colors.grey[100],
            child: Column(
              children: [
                const Padding(
                  padding: EdgeInsets.all(16),
                  child: Text(
                    'Saved Bookmarks',
                    style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                  ),
                ),
                // Command Center Search Bar
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  child: TextField(
                    onChanged: (value) {
                      setState(() {
                        _searchQuery = value;
                      });
                    },
                    decoration: InputDecoration(
                      hintText: 'Command Center: Search all bookmarks...',
                      prefixIcon: const Icon(Icons.search),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(8),
                      ),
                      contentPadding: const EdgeInsets.symmetric(vertical: 12),
                    ),
                  ),
                ),
                SizedBox(
                  height: 250,
                  child: FutureBuilder<List<Bookmark>>(
                    future: database.searchBookmarks(_searchQuery),
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return const Center(child: CircularProgressIndicator());
                      }

                      if (snapshot.hasError) {
                        return Center(
                          child: Text('Error: ${snapshot.error}'),
                        );
                      }

                      final bookmarks = snapshot.data ?? [];

                      if (bookmarks.isEmpty) {
                        return Center(
                          child: Text(
                            _searchQuery.isEmpty 
                              ? 'No bookmarks yet' 
                              : 'No bookmarks match "$_searchQuery"',
                          ),
                        );
                      }

                      return ListView.builder(
                        itemCount: bookmarks.length,
                        itemBuilder: (context, index) {
                          final bookmark = bookmarks[index];
                          final fileName = bookmark.filePath.split('/').last;
                          
                          return ListTile(
                            title: Text(bookmark.title),
                            subtitle: Text(
                              'Page ${bookmark.pageIndex + 1} • $fileName',
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                            ),
                            trailing: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                IconButton(
                                  icon: const Icon(Icons.edit),
                                  onPressed: () async {
                                    await _showEditBookmarkDialog(bookmark);
                                  },
                                ),
                                IconButton(
                                  icon: const Icon(Icons.delete),
                                  onPressed: () async {
                                    await database.deleteBookmark(bookmark.id);
                                    if (mounted) {
                                      await _calculateSyncDiff();
                                      setState(() {});
                                    }
                                  },
                                ),
                              ],
                            ),
                          );
                        },
                      );
                    },
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
