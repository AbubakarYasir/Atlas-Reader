import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Reader-side form for creating a bookmark at a document page.
class BookmarkComposer extends StatelessWidget {
  const BookmarkComposer({
    super.key,
    required this.titleController,
    required this.pageController,
    required this.descriptionController,
    required this.tagsController,
    required this.onInject,
    required this.onSync,
    required this.onCommit,
    required this.isSyncing,
    required this.isPushing,
    required this.canCommit,
  });

  final TextEditingController titleController;
  final TextEditingController pageController;
  final TextEditingController descriptionController;
  final TextEditingController tagsController;
  final VoidCallback onInject;
  final VoidCallback onSync;
  final VoidCallback onCommit;
  final bool isSyncing;
  final bool isPushing;
  final bool canCommit;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        TextField(
          controller: titleController,
          decoration: const InputDecoration(
            labelText: 'Bookmark Title',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 16),
        TextField(
          controller: pageController,
          decoration: const InputDecoration(
            labelText: 'Page Number',
            border: OutlineInputBorder(),
          ),
          keyboardType: TextInputType.number,
          inputFormatters: [FilteringTextInputFormatter.digitsOnly],
        ),
        const SizedBox(height: 16),
        TextField(
          controller: descriptionController,
          decoration: const InputDecoration(
            labelText: 'Description (Markdown comments)',
            border: OutlineInputBorder(),
            hintText: 'Optional: Add notes about this bookmark...',
          ),
          maxLines: 2,
        ),
        const SizedBox(height: 16),
        TextField(
          controller: tagsController,
          decoration: const InputDecoration(
            labelText: 'Tags (comma-separated)',
            border: OutlineInputBorder(),
            hintText: 'Optional: project, important, research...',
          ),
        ),
        const SizedBox(height: 24),
        Row(
          children: [
            Expanded(
              child: ElevatedButton(
                onPressed: onInject,
                child: const Text('INJECT BOOKMARK'),
              ),
            ),
            const SizedBox(width: 8),
            Expanded(
              child: OutlinedButton(
                onPressed: isSyncing ? null : onSync,
                child: const Text('SYNC FILE'),
              ),
            ),
            const SizedBox(width: 8),
            Expanded(
              child: OutlinedButton(
                onPressed: isPushing || !canCommit ? null : onCommit,
                child: const Text('COMMIT CHANGES'),
              ),
            ),
          ],
        ),
      ],
    );
  }
}
