import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../core/accessibility/accessibility_announcer.dart';

enum CitationFormat { apa, chicago, mla, bibtex }

class CitationGenerator {
  const CitationGenerator._();

  static String generate({
    required CitationFormat format,
    required String title,
    required String? author,
    required int pageNumber,
    String? publisher,
    String? year,
  }) {
    final cleanAuthor = (author != null && author.trim().isNotEmpty)
        ? author.trim()
        : 'Unknown Author';
    final cleanTitle = title.trim().isNotEmpty ? title.trim() : 'Untitled Work';
    final cleanYear = (year != null && year.trim().isNotEmpty)
        ? year.trim()
        : DateTime.now().year.toString();
    final cleanPublisher = (publisher != null && publisher.trim().isNotEmpty)
        ? publisher.trim()
        : 'Publisher';

    switch (format) {
      case CitationFormat.apa:
        return '$cleanAuthor. ($cleanYear). $cleanTitle (p. $pageNumber). $cleanPublisher.';
      case CitationFormat.chicago:
        return '$cleanAuthor. $cleanTitle. $cleanPublisher, $cleanYear, p. $pageNumber.';
      case CitationFormat.mla:
        return '$cleanAuthor. $cleanTitle. $cleanPublisher, $cleanYear, p. $pageNumber.';
      case CitationFormat.bibtex:
        final citeKey = _generateCiteKey(cleanAuthor, cleanYear, pageNumber);
        return '''@inbook{$citeKey,
  author = {$cleanAuthor},
  title = {$cleanTitle},
  pages = {$pageNumber},
  year = {$cleanYear},
  publisher = {$cleanPublisher}
}''';
    }
  }

  static String _generateCiteKey(String author, String year, int page) {
    var namePart = author.replaceAll(RegExp(r'[^a-zA-Z0-9]'), '');
    if (namePart.isEmpty) namePart = 'book';
    return '$namePart$year'
        '_p$page';
  }
}

class CitationDialog extends StatefulWidget {
  const CitationDialog({
    super.key,
    required this.title,
    this.author,
    required this.pageNumber,
    this.publisher,
    this.year,
  });

  final String title;
  final String? author;
  final int pageNumber;
  final String? publisher;
  final String? year;

  @override
  State<CitationDialog> createState() => _CitationDialogState();
}

class _CitationDialogState extends State<CitationDialog> {
  CitationFormat _selectedFormat = CitationFormat.apa;

  void _copyCitation(String citation) {
    Clipboard.setData(ClipboardData(text: citation));
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Citation copied to clipboard!')),
    );
    AccessibilityAnnouncer.announce(context, 'Citation copied to clipboard');
  }

  @override
  Widget build(BuildContext context) {
    final citation = CitationGenerator.generate(
      format: _selectedFormat,
      title: widget.title,
      author: widget.author,
      pageNumber: widget.pageNumber,
      publisher: widget.publisher,
      year: widget.year,
    );

    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 540),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.format_quote_rounded, size: 24),
                      const SizedBox(width: 8),
                      Text(
                        'Generate Citation',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                    ],
                  ),
                  IconButton(
                    icon: const Icon(Icons.close),
                    onPressed: () => Navigator.pop(context),
                  ),
                ],
              ),
              const SizedBox(height: 16),
              SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: SegmentedButton<CitationFormat>(
                  segments: const [
                    ButtonSegment(
                      value: CitationFormat.apa,
                      label: Text('APA 7th'),
                    ),
                    ButtonSegment(
                      value: CitationFormat.chicago,
                      label: Text('Chicago'),
                    ),
                    ButtonSegment(
                      value: CitationFormat.mla,
                      label: Text('MLA 9th'),
                    ),
                    ButtonSegment(
                      value: CitationFormat.bibtex,
                      label: Text('BibTeX'),
                    ),
                  ],
                  selected: {_selectedFormat},
                  onSelectionChanged: (set) =>
                      setState(() => _selectedFormat = set.first),
                ),
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(14),
                decoration: BoxDecoration(
                  color: Theme.of(
                    context,
                  ).colorScheme.surfaceContainerHighest.withAlpha(100),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(
                    color: Theme.of(context).dividerColor.withAlpha(60),
                  ),
                ),
                child: SelectableText(
                  citation,
                  style: const TextStyle(
                    fontFamily: 'Courier',
                    fontSize: 13,
                    height: 1.4,
                  ),
                ),
              ),
              const SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Close'),
                  ),
                  const SizedBox(width: 8),
                  FilledButton.icon(
                    onPressed: () => _copyCitation(citation),
                    icon: const Icon(Icons.copy, size: 18),
                    label: const Text('Copy Citation'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
