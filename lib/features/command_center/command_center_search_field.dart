import 'package:flutter/material.dart';

/// Search entry point for the global bookmark command center.
class CommandCenterSearchField extends StatelessWidget {
  const CommandCenterSearchField({
    super.key,
    required this.value,
    required this.onChanged,
  });

  final String value;
  final ValueChanged<String> onChanged;

  @override
  Widget build(BuildContext context) {
    return TextField(
      controller: TextEditingController.fromValue(
        TextEditingValue(
          text: value,
          selection: TextSelection.collapsed(offset: value.length),
        ),
      ),
      onChanged: onChanged,
      decoration: const InputDecoration(
        prefixIcon: Icon(Icons.search),
        labelText: 'Search all bookmarks',
        border: OutlineInputBorder(),
      ),
    );
  }
}
