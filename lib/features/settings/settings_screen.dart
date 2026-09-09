import 'package:flutter/material.dart';

/// Settings entry point. Persisted preferences are introduced in a later stage.
class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListView(
        children: [
          ListTile(
            leading: Icon(Icons.palette_outlined),
            title: Text('Theme'),
            subtitle: Text('System, light, dark, and high-contrast themes'),
          ),
          ListTile(
            leading: Icon(Icons.folder_outlined),
            title: Text('Library folders'),
            subtitle: Text('Manage folders to scan in a future release'),
          ),
          ListTile(
            leading: Icon(Icons.language_outlined),
            title: Text('Language'),
            subtitle: Text('English and Arabic/RTL support are planned'),
          ),
        ],
      ),
    );
  }
}
