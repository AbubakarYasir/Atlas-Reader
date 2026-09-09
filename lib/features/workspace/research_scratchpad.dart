import 'dart:async';
import 'package:flutter/material.dart';

import '../../core/accessibility/accessibility_announcer.dart';
import '../../database.dart';

class ResearchScratchpad extends StatefulWidget {
  const ResearchScratchpad({
    super.key,
    required this.filePath,
    required this.database,
    required this.currentPage,
    required this.onJumpToPage,
    this.selectedText,
    this.onClose,
  });

  final String filePath;
  final AppDatabase database;
  final int currentPage;
  final ValueChanged<int> onJumpToPage;
  final String? selectedText;
  final VoidCallback? onClose;

  @override
  State<ResearchScratchpad> createState() => _ResearchScratchpadState();
}

class _ResearchScratchpadState extends State<ResearchScratchpad> {
  late final TextEditingController _controller;
  Timer? _debounceTimer;
  bool _previewMode = false;
  bool _saved = true;

  @override
  void initState() {
    super.initState();
    _controller = TextEditingController();
    _loadNote();
  }

  Future<void> _loadNote() async {
    final note = await widget.database.getScratchpadNote(widget.filePath);
    if (note != null && mounted) {
      setState(() {
        _controller.text = note;
      });
    }
  }

  @override
  void dispose() {
    _debounceTimer?.cancel();
    _controller.dispose();
    super.dispose();
  }

  void _onTextChanged(String text) {
    setState(() => _saved = false);
    _debounceTimer?.cancel();
    _debounceTimer = Timer(const Duration(milliseconds: 600), () async {
      await widget.database.saveScratchpadNote(widget.filePath, text);
      if (mounted) setState(() => _saved = true);
    });
  }

  void _insertFormatting(String prefix, [String suffix = '']) {
    final text = _controller.text;
    final selection = _controller.selection;
    final start = selection.start >= 0 ? selection.start : text.length;
    final end = selection.end >= 0 ? selection.end : text.length;
    final selected = text.substring(start, end);

    final replacement = '$prefix$selected$suffix';
    final newText = text.replaceRange(start, end, replacement);

    _controller.value = TextEditingValue(
      text: newText,
      selection: TextSelection.collapsed(
        offset: start + prefix.length + selected.length,
      ),
    );
    _onTextChanged(newText);
  }

  void _insertQuote() {
    final quoteText = widget.selectedText?.trim();
    final quote = (quoteText != null && quoteText.isNotEmpty)
        ? quoteText
        : 'Scholarly excerpt';
    final fileName = widget.filePath.split(RegExp(r'[/\\]')).last;
    final bookTitle = fileName.replaceAll(RegExp(r'\.[^.]+$'), '');

    final snippet =
        '\n> "$quote"\n> — *$bookTitle*, [Page ${widget.currentPage}](atlas://page/${widget.currentPage})\n\n';
    _insertFormatting(snippet);
    AccessibilityAnnouncer.announce(
      context,
      'Quote inserted from page ${widget.currentPage}',
    );
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 360,
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(left: BorderSide(color: Theme.of(context).dividerColor)),
      ),
      child: Column(
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
            decoration: BoxDecoration(
              color: Theme.of(
                context,
              ).colorScheme.surfaceContainerHighest.withAlpha(90),
              border: Border(
                bottom: BorderSide(
                  color: Theme.of(context).dividerColor.withAlpha(60),
                ),
              ),
            ),
            child: Row(
              children: [
                const Icon(Icons.edit_note, size: 20),
                const SizedBox(width: 6),
                Expanded(
                  child: Row(
                    children: [
                      Flexible(
                        child: Text(
                          'Research Scratchpad',
                          overflow: TextOverflow.ellipsis,
                          style: Theme.of(context).textTheme.titleSmall
                              ?.copyWith(fontWeight: FontWeight.bold),
                        ),
                      ),
                      const SizedBox(width: 4),
                      if (!_saved)
                        const SizedBox.square(
                          dimension: 10,
                          child: CircularProgressIndicator(strokeWidth: 1.5),
                        )
                      else
                        const Icon(Icons.check, size: 14, color: Colors.green),
                    ],
                  ),
                ),
                IconButton(
                  icon: Icon(
                    _previewMode
                        ? Icons.edit_outlined
                        : Icons.remove_red_eye_outlined,
                    size: 18,
                  ),
                  tooltip: _previewMode ? 'Edit' : 'Preview',
                  visualDensity: VisualDensity.compact,
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(
                    minWidth: 32,
                    minHeight: 32,
                  ),
                  onPressed: () => setState(() => _previewMode = !_previewMode),
                ),
                if (widget.onClose != null)
                  IconButton(
                    icon: const Icon(Icons.close, size: 18),
                    tooltip: 'Close scratchpad',
                    visualDensity: VisualDensity.compact,
                    padding: EdgeInsets.zero,
                    constraints: const BoxConstraints(
                      minWidth: 32,
                      minHeight: 32,
                    ),
                    onPressed: widget.onClose,
                  ),
              ],
            ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              border: Border(
                bottom: BorderSide(
                  color: Theme.of(context).dividerColor.withAlpha(40),
                ),
              ),
            ),
            child: Wrap(
              spacing: 2,
              runSpacing: 2,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                IconButton(
                  icon: const Icon(Icons.format_bold, size: 18),
                  tooltip: 'Bold',
                  visualDensity: VisualDensity.compact,
                  onPressed: () => _insertFormatting('**', '**'),
                ),
                IconButton(
                  icon: const Icon(Icons.format_italic, size: 18),
                  tooltip: 'Italic',
                  visualDensity: VisualDensity.compact,
                  onPressed: () => _insertFormatting('*', '*'),
                ),
                IconButton(
                  icon: const Icon(Icons.title, size: 18),
                  tooltip: 'Heading',
                  visualDensity: VisualDensity.compact,
                  onPressed: () => _insertFormatting('### '),
                ),
                IconButton(
                  icon: const Icon(Icons.format_list_bulleted, size: 18),
                  tooltip: 'Bullet List',
                  visualDensity: VisualDensity.compact,
                  onPressed: () => _insertFormatting('- '),
                ),
                IconButton(
                  icon: const Icon(Icons.code, size: 18),
                  tooltip: 'Code',
                  visualDensity: VisualDensity.compact,
                  onPressed: () => _insertFormatting('`', '`'),
                ),
                FilledButton.tonalIcon(
                  style: FilledButton.styleFrom(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 8,
                      vertical: 4,
                    ),
                    visualDensity: VisualDensity.compact,
                  ),
                  icon: const Icon(Icons.format_quote, size: 14),
                  label: Text(
                    'Quote p.${widget.currentPage}',
                    style: const TextStyle(fontSize: 11),
                  ),
                  onPressed: _insertQuote,
                ),
              ],
            ),
          ),
          Expanded(
            child: _previewMode
                ? _buildMarkdownPreview()
                : Padding(
                    padding: const EdgeInsets.all(12),
                    child: TextField(
                      controller: _controller,
                      maxLines: null,
                      expands: true,
                      onChanged: _onTextChanged,
                      style: const TextStyle(
                        fontFamily: 'Courier',
                        fontSize: 13,
                        height: 1.45,
                      ),
                      decoration: const InputDecoration(
                        hintText:
                            'Record research notes, quotes, and reflections here (Markdown supported)...',
                        border: InputBorder.none,
                      ),
                    ),
                  ),
          ),
        ],
      ),
    );
  }

  Widget _buildMarkdownPreview() {
    final text = _controller.text;
    if (text.trim().isEmpty) {
      return const Center(
        child: Text(
          'Empty note. Switch back to edit and write your research notes.',
        ),
      );
    }

    final lines = text.split('\n');

    return ListView.builder(
      padding: const EdgeInsets.all(14),
      itemCount: lines.length,
      itemBuilder: (context, index) {
        final line = lines[index];
        if (line.startsWith('### ')) {
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 6),
            child: Text(
              line.substring(4),
              style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
            ),
          );
        }
        if (line.startsWith('## ')) {
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 8),
            child: Text(
              line.substring(3),
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
          );
        }
        if (line.startsWith('# ')) {
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 10),
            child: Text(
              line.substring(2),
              style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
          );
        }
        if (line.startsWith('> ')) {
          return Container(
            margin: const EdgeInsets.symmetric(vertical: 4),
            padding: const EdgeInsets.only(left: 10, top: 4, bottom: 4),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(
                  color: Theme.of(context).colorScheme.primary,
                  width: 3,
                ),
              ),
              color: Theme.of(context).colorScheme.primary.withAlpha(15),
            ),
            child: _buildRichLine(line.substring(2)),
          );
        }
        if (line.startsWith('- ')) {
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 2),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text('• ', style: TextStyle(fontWeight: FontWeight.bold)),
                Expanded(child: _buildRichLine(line.substring(2))),
              ],
            ),
          );
        }
        if (line.trim().isEmpty) {
          return const SizedBox(height: 8);
        }
        return Padding(
          padding: const EdgeInsets.symmetric(vertical: 2),
          child: _buildRichLine(line),
        );
      },
    );
  }

  Widget _buildRichLine(String text) {
    final pageLinkRegex = RegExp(r'\[Page\s+(\d+)\]\(atlas:\/\/page\/(\d+)\)');
    final match = pageLinkRegex.firstMatch(text);

    if (match != null) {
      final pageNum = int.tryParse(match.group(1) ?? '') ?? 1;
      final before = text.substring(0, match.start);
      final after = text.substring(match.end);

      return Wrap(
        crossAxisAlignment: WrapCrossAlignment.center,
        children: [
          Text(before),
          ActionChip(
            avatar: const Icon(Icons.link, size: 14),
            label: Text('Page $pageNum'),
            visualDensity: VisualDensity.compact,
            padding: EdgeInsets.zero,
            onPressed: () => widget.onJumpToPage(pageNum),
          ),
          Text(after),
        ],
      );
    }

    return Text(text, style: const TextStyle(fontSize: 13, height: 1.4));
  }
}
