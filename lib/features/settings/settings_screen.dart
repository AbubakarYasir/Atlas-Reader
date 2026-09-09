import 'package:flutter/material.dart';

import '../../database.dart';
import '../library/library_folder_manager.dart';
import '../library/library_folders_screen.dart';

/// Settings entry point. Persisted preferences arrive in a later stage.
class SettingsScreen extends StatelessWidget {
  const SettingsScreen({
    super.key,
    required this.database,
    required this.manager,
  });

  final AppDatabase database;
  final LibraryFolderManager manager;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListView(
        children: [
          ListTile(
            leading: Icon(Icons.folder_outlined),
            title: Text('Library folders'),
            subtitle: Text('Choose folders to scan for PDF files'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => Navigator.of(context).push(
              MaterialPageRoute<void>(
                builder: (_) =>
                    LibraryFoldersScreen(database: database, manager: manager),
              ),
            ),
          ),
          ListTile(
            leading: Icon(Icons.palette_outlined),
            title: Text('Theme'),
            subtitle: Text('System, light, dark, and high-contrast themes'),
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
