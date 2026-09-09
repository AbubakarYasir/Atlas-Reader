import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import 'reader_models.dart';

class ReaderZoomControls extends StatelessWidget {
  const ReaderZoomControls({
    super.key,
    required this.percent,
    required this.preset,
    required this.onZoomOut,
    required this.onZoomIn,
    required this.onPresetSelected,
    this.compact = false,
  });

  final int percent;
  final ReaderZoomPreset preset;
  final VoidCallback onZoomOut;
  final VoidCallback onZoomIn;
  final ValueChanged<(ReaderZoomPreset, int?)> onPresetSelected;
  final bool compact;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    final menu = PopupMenuButton<(ReaderZoomPreset, int?)>(
      key: const ValueKey('reader-zoom-menu'),
      tooltip: strings.zoomPercent(percent),
      onSelected: (selection) {
        if (selection.$1 == ReaderZoomPreset.custom && selection.$2 == null) {
          _promptCustomZoom(context);
        } else {
          onPresetSelected(selection);
        }
      },
      padding: EdgeInsets.zero,
      itemBuilder: (context) => [
        PopupMenuItem(
          value: const (ReaderZoomPreset.fitPage, null),
          child: _MenuChoice(
            selected: preset == ReaderZoomPreset.fitPage,
            icon: Icons.fit_screen,
            label: strings.fitPage,
          ),
        ),
        PopupMenuItem(
          value: const (ReaderZoomPreset.fitWidth, null),
          child: _MenuChoice(
            selected: preset == ReaderZoomPreset.fitWidth,
            icon: Icons.fit_screen_outlined,
            label: strings.fitWidth,
          ),
        ),
        const PopupMenuDivider(),
        for (final value in const [10, 25, 50, 75, 100, 125, 200, 400, 800])
          PopupMenuItem(
            value: (ReaderZoomPreset.custom, value),
            child: _MenuChoice(
              selected: preset == ReaderZoomPreset.custom && percent == value,
              icon: Icons.zoom_in,
              label: strings.zoomPercent(value),
            ),
          ),
        const PopupMenuDivider(),
        PopupMenuItem(
          value: const (ReaderZoomPreset.custom, null),
          child: _MenuChoice(
            selected: false,
            icon: Icons.edit_outlined,
            label: strings.customZoom,
          ),
        ),
      ],
      icon: Semantics(
        label: strings.zoomPercent(percent),
        child: ConstrainedBox(
          constraints: const BoxConstraints(minWidth: 42, minHeight: 40),
          child: Center(
            child: Text(
              '$percent%',
              maxLines: 1,
              overflow: TextOverflow.fade,
              style: Theme.of(context).textTheme.labelLarge,
            ),
          ),
        ),
      ),
    );

    if (compact) return menu;
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        IconButton(
          tooltip: strings.zoomOut,
          onPressed: onZoomOut,
          icon: const Icon(Icons.zoom_out),
        ),
        menu,
        IconButton(
          tooltip: strings.zoomIn,
          onPressed: onZoomIn,
          icon: const Icon(Icons.zoom_in),
        ),
      ],
    );
  }

  Future<void> _promptCustomZoom(BuildContext context) async {
    final strings = AppLocalizations.of(context)!;
    var value = '$percent';
    final selected = await showDialog<int>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(strings.customZoom),
        content: TextFormField(
          initialValue: value,
          autofocus: true,
          keyboardType: TextInputType.number,
          decoration: InputDecoration(
            labelText: strings.zoomRange,
            suffixText: '%',
            border: const OutlineInputBorder(),
          ),
          onFieldSubmitted: (value) {
            final parsed = int.tryParse(value);
            if (parsed != null && parsed >= 10 && parsed <= 6400) {
              Navigator.pop(context, parsed);
            }
          },
          onChanged: (updated) => value = updated,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: Text(MaterialLocalizations.of(context).cancelButtonLabel),
          ),
          FilledButton(
            onPressed: () {
              final parsed = int.tryParse(value);
              if (parsed != null && parsed >= 10 && parsed <= 6400) {
                Navigator.pop(context, parsed);
              }
            },
            child: Text(strings.apply),
          ),
        ],
      ),
    );
    if (selected != null) {
      onPresetSelected((ReaderZoomPreset.custom, selected));
    }
  }
}

class _MenuChoice extends StatelessWidget {
  const _MenuChoice({
    required this.selected,
    required this.icon,
    required this.label,
  });

  final bool selected;
  final IconData icon;
  final String label;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(selected ? Icons.check : icon, size: 18),
        const SizedBox(width: 12),
        Text(label),
      ],
    );
  }
}
