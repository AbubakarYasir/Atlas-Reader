import 'package:flutter/material.dart';

import '../../database.dart';
import '../../l10n/app_localizations.dart';
import '../library/library_folder_manager.dart';
import '../library/library_folders_screen.dart';

/// Settings entry point. Persisted preferences arrive in a later stage.
class SettingsScreen extends StatelessWidget {
  const SettingsScreen({
    super.key,
    required this.database,
    required this.manager,
    required this.onLocaleChanged,
  });

  final AppDatabase database;
  final LibraryFolderManager manager;
  final ValueChanged<Locale> onLocaleChanged;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context)!;
    return Scaffold(
      appBar: AppBar(title: Text(strings.settings)),
      body: ListView(
        children: [
          ListTile(
            leading: Icon(Icons.folder_outlined),
            title: Text(strings.libraryFolders),
            subtitle: Text(strings.libraryFoldersDescription),
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
            title: Text(strings.theme),
            subtitle: Text(strings.themeDescription),
          ),
          ListTile(
            leading: Icon(Icons.language_outlined),
            title: Text(strings.language),
            subtitle: Text(strings.languageDescription),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => _showLanguagePicker(context, strings),
          ),
        ],
      ),
    );
  }

  Future<void> _showLanguagePicker(
    BuildContext context,
    AppLocalizations strings,
  ) async {
    final locale = await showDialog<Locale>(
      context: context,
      builder: (dialogContext) => AlertDialog(
        title: Text(strings.language),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              title: Text(strings.english),
              onTap: () => Navigator.pop(dialogContext, const Locale('en')),
            ),
            ListTile(
              title: Text(strings.arabic),
              onTap: () => Navigator.pop(dialogContext, const Locale('ar')),
            ),
          ],
        ),
      ),
    );
    if (locale != null) onLocaleChanged(locale);
  }
}
