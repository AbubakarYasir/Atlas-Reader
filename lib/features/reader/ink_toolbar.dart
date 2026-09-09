import 'package:dart_pdf_editor/dart_pdf_editor.dart';
import 'package:flutter/material.dart';

/// Atlas Reader's intentionally small writing surface. Drawing geometry and
/// pressure are handled by the PDF editor in native page points; this widget
/// only exposes the four tools people need while reading.
class InkToolbar extends StatelessWidget {
  const InkToolbar({
    super.key,
    required this.controller,
    required this.saving,
    required this.savedRevisionId,
    required this.onSave,
    required this.onDone,
  });

  final PdfEditingController controller;
  final bool saving;
  final int savedRevisionId;
  final VoidCallback onSave;
  final VoidCallback onDone;

  static const _colors = <Color>[
    Colors.black,
    Color(0xFF1565C0),
    Color(0xFFC62828),
    Color(0xFF2E7D32),
    Color(0xFFF9A825),
  ];
  static const _strokeWidths = <double>[1, 2, 4, 8, 12, 18];
  static const _eraserRadii = <double>[4, 8, 16, 24];

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Theme.of(context).colorScheme.surfaceContainerHigh,
      elevation: 2,
      child: SafeArea(
        bottom: false,
        child: SizedBox(
          height: 52,
          child: ListenableBuilder(
            listenable: controller,
            builder: (context, _) => SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              padding: const EdgeInsets.symmetric(horizontal: 8),
              child: Row(
                children: [
                  _toolButton(
                    context,
                    tool: PdfEditTool.select,
                    icon: Icons.near_me_outlined,
                    label: 'Select stroke (Delete removes it)',
                  ),
                  _toolButton(
                    context,
                    tool: PdfEditTool.ink,
                    icon: Icons.edit,
                    label: 'Pen',
                  ),
                  _toolButton(
                    context,
                    tool: PdfEditTool.highlight,
                    icon: Icons.border_color,
                    label: 'Highlighter',
                  ),
                  _toolButton(
                    context,
                    tool: PdfEditTool.eraser,
                    icon: Icons.auto_fix_normal,
                    label: 'Area eraser',
                  ),
                  const VerticalDivider(indent: 8, endIndent: 8),
                  if (controller.toolUsesColor)
                    for (final color in _colors)
                      Semantics(
                        button: true,
                        label: '${_colorName(color)} ink color',
                        selected:
                            controller.color.toARGB32() == color.toARGB32(),
                        child: Tooltip(
                          message: _colorName(color),
                          child: InkWell(
                            customBorder: const CircleBorder(),
                            onTap: () => controller.color = color,
                            child: Container(
                              width: 30,
                              height: 30,
                              margin: const EdgeInsets.symmetric(horizontal: 2),
                              decoration: BoxDecoration(
                                shape: BoxShape.circle,
                                color: color,
                                border: Border.all(
                                  color:
                                      controller.color.toARGB32() ==
                                          color.toARGB32()
                                      ? Theme.of(context).colorScheme.primary
                                      : Theme.of(context).dividerColor,
                                  width:
                                      controller.color.toARGB32() ==
                                          color.toARGB32()
                                      ? 3
                                      : 1,
                                ),
                              ),
                            ),
                          ),
                        ),
                      ),
                  const SizedBox(width: 4),
                  PopupMenuButton<double>(
                    tooltip: controller.tool == PdfEditTool.eraser
                        ? 'Eraser area size'
                        : 'Line thickness',
                    icon: const Icon(Icons.line_weight),
                    onSelected: (value) {
                      if (controller.tool == PdfEditTool.eraser) {
                        controller.preferences.eraserRadius = value;
                      } else {
                        controller.preferences.strokeWidth = value;
                      }
                    },
                    itemBuilder: (context) => [
                      for (final value
                          in controller.tool == PdfEditTool.eraser
                              ? _eraserRadii
                              : _strokeWidths)
                        PopupMenuItem(
                          value: value,
                          child: Text('${value.toStringAsFixed(0)} pt'),
                        ),
                    ],
                  ),
                  IconButton(
                    tooltip: 'Undo (Ctrl+Z)',
                    onPressed: controller.canUndo ? controller.undo : null,
                    icon: const Icon(Icons.undo),
                  ),
                  IconButton(
                    tooltip: 'Redo (Ctrl+Shift+Z)',
                    onPressed: controller.canRedo ? controller.redo : null,
                    icon: const Icon(Icons.redo),
                  ),
                  const VerticalDivider(indent: 8, endIndent: 8),
                  FilledButton.icon(
                    onPressed:
                        saving || controller.revisionId == savedRevisionId
                        ? null
                        : onSave,
                    icon: saving
                        ? const SizedBox.square(
                            dimension: 16,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(Icons.save_outlined, size: 18),
                    label: const Text('Save'),
                  ),
                  const SizedBox(width: 6),
                  TextButton.icon(
                    onPressed: saving ? null : onDone,
                    icon: const Icon(Icons.done, size: 18),
                    label: const Text('Done'),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _toolButton(
    BuildContext context, {
    required PdfEditTool tool,
    required IconData icon,
    required String label,
  }) {
    final selected = controller.tool == tool;
    return Semantics(
      button: true,
      selected: selected,
      label: label,
      child: IconButton.filledTonal(
        tooltip: label,
        isSelected: selected,
        onPressed: () => controller.tool = tool,
        icon: Icon(icon),
      ),
    );
  }

  static String _colorName(Color color) => switch (color.toARGB32()) {
    0xFF000000 => 'Black',
    0xFF1565C0 => 'Blue',
    0xFFC62828 => 'Red',
    0xFF2E7D32 => 'Green',
    _ => 'Yellow',
  };
}
