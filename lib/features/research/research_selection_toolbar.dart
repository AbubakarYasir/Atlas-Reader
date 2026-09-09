import 'package:flutter/material.dart';

class ResearchSelectionToolbar extends StatelessWidget {
  const ResearchSelectionToolbar({
    super.key,
    required this.selectedText,
    required this.onHighlight,
    required this.onUnderline,
    required this.onStrikethrough,
    required this.onAddNote,
    required this.onCite,
    required this.onCopy,
    required this.onClose,
  });

  final String selectedText;
  final ValueChanged<String> onHighlight; // hex color
  final VoidCallback onUnderline;
  final VoidCallback onStrikethrough;
  final VoidCallback onAddNote;
  final VoidCallback onCite;
  final VoidCallback onCopy;
  final VoidCallback onClose;

  static const highlightColors = [
    ('#FFE066', 'Yellow', Color(0xFFFFE066)),
    ('#8CE99A', 'Green', Color(0xFF8CE99A)),
    ('#74C0FC', 'Blue', Color(0xFF74C0FC)),
    ('#FFA8A8', 'Pink', Color(0xFFFFA8A8)),
  ];

  @override
  Widget build(BuildContext context) {
    return Material(
      elevation: 6,
      borderRadius: BorderRadius.circular(10),
      color: Theme.of(context).colorScheme.surface,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(10),
          border: Border.all(
            color: Theme.of(context).dividerColor.withAlpha(60),
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            ...highlightColors.map(
              (c) => IconButton(
                icon: Container(
                  width: 18,
                  height: 18,
                  decoration: BoxDecoration(
                    color: c.$3,
                    shape: BoxShape.circle,
                    border: Border.all(color: Colors.black26),
                  ),
                ),
                tooltip: 'Highlight in ${c.$2}',
                visualDensity: VisualDensity.compact,
                onPressed: () => onHighlight(c.$1),
              ),
            ),
            const VerticalDivider(width: 16, thickness: 1),
            IconButton(
              icon: const Icon(Icons.format_underlined, size: 18),
              tooltip: 'Underline',
              visualDensity: VisualDensity.compact,
              onPressed: onUnderline,
            ),
            IconButton(
              icon: const Icon(Icons.format_strikethrough, size: 18),
              tooltip: 'Strikethrough',
              visualDensity: VisualDensity.compact,
              onPressed: onStrikethrough,
            ),
            IconButton(
              icon: const Icon(Icons.note_add_outlined, size: 18),
              tooltip: 'Add Sticky Note',
              visualDensity: VisualDensity.compact,
              onPressed: onAddNote,
            ),
            const VerticalDivider(width: 16, thickness: 1),
            IconButton(
              icon: const Icon(Icons.format_quote_outlined, size: 18),
              tooltip: 'Cite Page',
              visualDensity: VisualDensity.compact,
              onPressed: onCite,
            ),
            IconButton(
              icon: const Icon(Icons.copy, size: 18),
              tooltip: 'Copy',
              visualDensity: VisualDensity.compact,
              onPressed: onCopy,
            ),
            IconButton(
              icon: const Icon(Icons.close, size: 16),
              tooltip: 'Dismiss',
              visualDensity: VisualDensity.compact,
              onPressed: onClose,
            ),
          ],
        ),
      ),
    );
  }
}
