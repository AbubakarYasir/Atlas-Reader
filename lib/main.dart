import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'database.dart';
import 'pdf_engine.dart';

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

  final _bookmarkTitleController = TextEditingController();
  final _pageNumberController = TextEditingController();
  final _pdfEngine = PdfEngine();

  @override
  void dispose() {
    _bookmarkTitleController.dispose();
    _pageNumberController.dispose();
    super.dispose();
  }

  Future<void> _pickPdf() async {
    final result = await FilePicker.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf'],
    );

    if (result == null || result.files.single.path == null) {
      return;
    }

    setState(() {
      _selectedFilePath = result.files.single.path;
      _status = 'PDF selected: $_selectedFilePath';
    });
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
                  ElevatedButton(
                    onPressed: _injectBookmark,
                    child: const Text('INJECT BOOKMARK'),
                  ),
                  const SizedBox(height: 32),
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
                            trailing: IconButton(
                              icon: const Icon(Icons.delete),
                              onPressed: () async {
                                await database.deleteBookmark(bookmark.id);
                                if (mounted) {
                                  setState(() {});
                                }
                              },
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
