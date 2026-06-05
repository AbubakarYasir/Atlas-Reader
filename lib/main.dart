import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'pdf_engine.dart';

void main() {
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

    setState(() => _status = 'Processing...');

    try {
      final outputPath = await _pdfEngine.injectBookmark(
        filePath,
        bookmarkTitle,
        pageNumber - 1,
      );

      if (!mounted) return;

      setState(() {
        if (outputPath != null) {
          _status = 'Success! File saved at $outputPath';
        } else {
          _status = 'Failed to inject bookmark. Check console for details.';
        }
      });
    } on PdfOverwriteException catch (e) {
      if (!mounted) return;
      setState(() => _status = e.message);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Atlas UEP PoC'),
      ),
      body: Center(
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
    );
  }
}
