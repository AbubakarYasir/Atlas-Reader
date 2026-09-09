import 'package:flutter/material.dart';

import 'reader_models.dart';

class ReaderSettingsDialog extends StatelessWidget {
  const ReaderSettingsDialog({
    super.key,
    required this.preferences,
    required this.onChanged,
  });

  final ReaderPreferences preferences;
  final ValueChanged<ReaderPreferences> onChanged;

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 500),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Expanded(
                      child: Text(
                        'Reading & Display Settings',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                    ),
                    IconButton(
                      icon: const Icon(Icons.close),
                      onPressed: () => Navigator.pop(context),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Text('Display Theme', style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(height: 8),
                SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  child: SegmentedButton<ReaderThemeMode>(
                    segments: const [
                      ButtonSegment(
                        value: ReaderThemeMode.day,
                        label: Text('Day'),
                        icon: Icon(Icons.light_mode_outlined, size: 16),
                      ),
                      ButtonSegment(
                        value: ReaderThemeMode.warmParchment,
                        label: Text('Sepia'),
                        icon: Icon(Icons.menu_book, size: 16),
                      ),
                      ButtonSegment(
                        value: ReaderThemeMode.night,
                        label: Text('Night'),
                        icon: Icon(Icons.dark_mode_outlined, size: 16),
                      ),
                      ButtonSegment(
                        value: ReaderThemeMode.oled,
                        label: Text('OLED'),
                        icon: Icon(Icons.contrast, size: 16),
                      ),
                    ],
                    selected: {preferences.theme},
                    onSelectionChanged: (set) => onChanged(preferences.copyWith(theme: set.first)),
                  ),
                ),
                const SizedBox(height: 16),
                Text('Reading Mode', style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    ChoiceChip(
                      label: const Text('Vertical Scroll'),
                      selected: preferences.mode == ReadingMode.continuousVertical,
                      onSelected: (_) => onChanged(preferences.copyWith(mode: ReadingMode.continuousVertical)),
                    ),
                    ChoiceChip(
                      label: const Text('Single Page'),
                      selected: preferences.mode == ReadingMode.singlePage,
                      onSelected: (_) => onChanged(preferences.copyWith(mode: ReadingMode.singlePage)),
                    ),
                    ChoiceChip(
                      label: const Text('Two-Page Spread'),
                      selected: preferences.mode == ReadingMode.twoPageSpread,
                      onSelected: (_) => onChanged(preferences.copyWith(mode: ReadingMode.twoPageSpread)),
                    ),
                    ChoiceChip(
                      label: const Text('Two-Column Split'),
                      selected: preferences.mode == ReadingMode.twoColumnSplit,
                      onSelected: (_) => onChanged(preferences.copyWith(mode: ReadingMode.twoColumnSplit)),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('White Margin Cropping', style: Theme.of(context).textTheme.titleSmall),
                    Text('${(preferences.marginCrop * 100).round()}%'),
                  ],
                ),
                Slider(
                  value: preferences.marginCrop,
                  min: 0.0,
                  max: 0.30,
                  divisions: 30,
                  label: '${(preferences.marginCrop * 100).round()}%',
                  onChanged: (val) => onChanged(preferences.copyWith(marginCrop: val)),
                ),
                const SizedBox(height: 12),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('Screen Brightness', style: Theme.of(context).textTheme.titleSmall),
                    Text('${(preferences.brightness * 100).round()}%'),
                  ],
                ),
                Slider(
                  value: preferences.brightness,
                  min: 0.3,
                  max: 1.0,
                  divisions: 14,
                  label: '${(preferences.brightness * 100).round()}%',
                  onChanged: (val) => onChanged(preferences.copyWith(brightness: val)),
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    Expanded(
                      child: Text('Academic Page Offset', style: Theme.of(context).textTheme.titleSmall),
                    ),
                    SizedBox(
                      width: 100,
                      child: TextFormField(
                        initialValue: '${preferences.pageOffset}',
                        keyboardType: TextInputType.number,
                        decoration: const InputDecoration(
                          hintText: 'e.g. -12',
                          isDense: true,
                          border: OutlineInputBorder(),
                        ),
                        onChanged: (val) {
                          final offset = int.tryParse(val.trim()) ?? 0;
                          onChanged(preferences.copyWith(pageOffset: offset));
                        },
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('Font Size', style: Theme.of(context).textTheme.titleSmall),
                    Text('${preferences.fontSize.round()} pt'),
                  ],
                ),
                Slider(
                  value: preferences.fontSize,
                  min: 12.0,
                  max: 32.0,
                  divisions: 20,
                  label: '${preferences.fontSize.round()}',
                  onChanged: (val) => onChanged(preferences.copyWith(fontSize: val)),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
