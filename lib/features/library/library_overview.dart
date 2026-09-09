import 'package:flutter/material.dart';

/// Displays the currently selected document in the library workspace.
class LibraryOverview extends StatelessWidget {
  const LibraryOverview({
    super.key,
    required this.filePath,
    required this.onSelectPdf,
  });

  final String? filePath;
  final VoidCallback onSelectPdf;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        ElevatedButton(onPressed: onSelectPdf, child: const Text('Select PDF')),
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
