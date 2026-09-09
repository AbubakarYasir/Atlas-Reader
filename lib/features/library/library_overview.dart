import 'package:flutter/material.dart';

/// Displays the currently selected document in the library workspace.
class LibraryOverview extends StatelessWidget {
  const LibraryOverview({
    super.key,
    required this.filePath,
    required this.onSelectPdf,
    this.onOpenReader,
  });

  final String? filePath;
  final VoidCallback onSelectPdf;
  final VoidCallback? onOpenReader;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            Expanded(
              child: ElevatedButton(
                onPressed: onSelectPdf,
                child: const Text('Select PDF'),
              ),
            ),
            if (filePath != null) ...[
              const SizedBox(width: 8),
              FilledButton.tonalIcon(
                onPressed: onOpenReader,
                icon: const Icon(Icons.menu_book_outlined),
                label: const Text('Open reader'),
              ),
            ],
          ],
        ),
        if (filePath != null) ...[
          const SizedBox(height: 8),
          Text(
            filePath!,
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ],
      ],
    );
  }
}
