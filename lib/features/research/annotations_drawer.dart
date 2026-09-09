import 'package:flutter/material.dart';

import '../../database.dart';

class AnnotationsDrawer extends StatefulWidget {
  const AnnotationsDrawer({
    super.key,
    required this.filePath,
    required this.database,
    required this.currentPage,
    required this.onJumpToPage,
    required this.onClose,
  });

  final String filePath;
  final AppDatabase database;
  final int currentPage;
  final ValueChanged<int> onJumpToPage;
  final VoidCallback onClose;

  @override
  State<AnnotationsDrawer> createState() => _AnnotationsDrawerState();
}

class _AnnotationsDrawerState extends State<AnnotationsDrawer> {
  bool _onlyCurrentPage = false;
  String _search = '';

  Future<List<DocumentAnnotation>> _loadAnnotations() {
    return widget.database.getAnnotationsForFile(
      widget.filePath,
      pageNumber: _onlyCurrentPage ? widget.currentPage : null,
    );
  }

  Future<void> _editNote(DocumentAnnotation annot) async {
    final controller = TextEditingController(text: annot.note ?? '');
    final updated = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Edit Note'),
        content: TextField(
          controller: controller,
          maxLines: 3,
          autofocus: true,
          decoration: const InputDecoration(
            hintText: 'Enter your research note...',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Save'),
          ),
        ],
      ),
    );

    if (updated == true) {
      await widget.database.updateAnnotationNote(
        annot.id,
        controller.text.trim(),
      );
      if (mounted) setState(() {});
    }
  }

  Future<void> _deleteAnnotation(DocumentAnnotation annot) async {
    await widget.database.deleteAnnotation(annot.id);
    if (mounted) setState(() {});
  }

  Color _parseColor(String hex) {
    var clean = hex.replaceAll('#', '');
    if (clean.length == 6) clean = 'FF$clean';
    final val = int.tryParse(clean, radix: 16) ?? 0xFFFFEB3B;
    return Color(val);
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 340,
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(left: BorderSide(color: Theme.of(context).dividerColor)),
      ),
      child: Column(
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
            decoration: BoxDecoration(
              color: Theme.of(
                context,
              ).colorScheme.surfaceContainerHighest.withAlpha(80),
              border: Border(
                bottom: BorderSide(
                  color: Theme.of(context).dividerColor.withAlpha(60),
                ),
              ),
            ),
            child: Row(
              children: [
                const Icon(Icons.draw_outlined, size: 20),
                const SizedBox(width: 8),
                Text(
                  'Annotations & Notes',
                  style: Theme.of(context).textTheme.titleSmall,
                ),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.close, size: 18),
                  tooltip: 'Close annotations',
                  onPressed: widget.onClose,
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    decoration: InputDecoration(
                      hintText: 'Search notes...',
                      prefixIcon: const Icon(Icons.search, size: 18),
                      isDense: true,
                      contentPadding: const EdgeInsets.symmetric(
                        horizontal: 8,
                        vertical: 8,
                      ),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(8),
                      ),
                    ),
                    onChanged: (val) =>
                        setState(() => _search = val.trim().toLowerCase()),
                  ),
                ),
                const SizedBox(width: 6),
                FilterChip(
                  label: Text('p.${widget.currentPage}'),
                  selected: _onlyCurrentPage,
                  onSelected: (val) => setState(() => _onlyCurrentPage = val),
                ),
              ],
            ),
          ),
          Expanded(
            child: FutureBuilder<List<DocumentAnnotation>>(
              future: _loadAnnotations(),
              builder: (context, snapshot) {
                if (snapshot.connectionState != ConnectionState.done) {
                  return const Center(child: CircularProgressIndicator());
                }
                var list = snapshot.data ?? const [];
                if (_search.isNotEmpty) {
                  list = list.where((a) {
                    final t = (a.selectedText ?? '').toLowerCase();
                    final n = (a.note ?? '').toLowerCase();
                    return t.contains(_search) || n.contains(_search);
                  }).toList();
                }

                if (list.isEmpty) {
                  return const Center(
                    child: Padding(
                      padding: EdgeInsets.all(24),
                      child: Text(
                        'No annotations or notes yet.\nSelect text on any page to highlight or add notes.',
                        textAlign: TextAlign.center,
                        style: TextStyle(fontSize: 12, color: Colors.grey),
                      ),
                    ),
                  );
                }

                return ListView.separated(
                  padding: const EdgeInsets.all(12),
                  itemCount: list.length,
                  separatorBuilder: (_, _) => const SizedBox(height: 8),
                  itemBuilder: (context, index) {
                    final item = list[index];
                    final color = _parseColor(item.colorHex);

                    return InkWell(
                      onTap: () => widget.onJumpToPage(item.pageNumber),
                      borderRadius: BorderRadius.circular(8),
                      child: Container(
                        padding: const EdgeInsets.all(10),
                        decoration: BoxDecoration(
                          color: Theme.of(
                            context,
                          ).colorScheme.surfaceContainerHigh.withAlpha(120),
                          borderRadius: BorderRadius.circular(8),
                          border: Border(
                            left: BorderSide(color: color, width: 4),
                          ),
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Text(
                                  'Page ${item.pageNumber} · ${item.type.toUpperCase()}',
                                  style: TextStyle(
                                    fontSize: 10,
                                    fontWeight: FontWeight.bold,
                                    color: Theme.of(context).hintColor,
                                  ),
                                ),
                                Row(
                                  mainAxisSize: MainAxisSize.min,
                                  children: [
                                    InkWell(
                                      onTap: () => _editNote(item),
                                      child: const Icon(
                                        Icons.edit_outlined,
                                        size: 14,
                                      ),
                                    ),
                                    const SizedBox(width: 8),
                                    InkWell(
                                      onTap: () => _deleteAnnotation(item),
                                      child: const Icon(
                                        Icons.delete_outline,
                                        size: 14,
                                        color: Colors.red,
                                      ),
                                    ),
                                  ],
                                ),
                              ],
                            ),
                            if (item.selectedText != null &&
                                item.selectedText!.isNotEmpty) ...[
                              const SizedBox(height: 4),
                              Text(
                                '"${item.selectedText}"',
                                maxLines: 3,
                                overflow: TextOverflow.ellipsis,
                                style: const TextStyle(
                                  fontSize: 12,
                                  fontStyle: FontStyle.italic,
                                ),
                              ),
                            ],
                            if (item.note != null && item.note!.isNotEmpty) ...[
                              const SizedBox(height: 4),
                              Container(
                                padding: const EdgeInsets.all(6),
                                decoration: BoxDecoration(
                                  color: Colors.amber.withAlpha(30),
                                  borderRadius: BorderRadius.circular(4),
                                ),
                                child: Row(
                                  children: [
                                    const Icon(
                                      Icons.sticky_note_2_outlined,
                                      size: 12,
                                      color: Colors.amber,
                                    ),
                                    const SizedBox(width: 4),
                                    Expanded(
                                      child: Text(
                                        item.note!,
                                        style: const TextStyle(
                                          fontSize: 11,
                                          fontWeight: FontWeight.w500,
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ],
                        ),
                      ),
                    );
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
