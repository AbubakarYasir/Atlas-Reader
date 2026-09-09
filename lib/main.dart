import 'package:flutter/material.dart';
import 'package:logging/logging.dart';

import 'database.dart';
export 'features/library/library_screen.dart'
    show LibraryScreen, MyApp, database;

import 'features/library/library_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  Logger.root.level = Level.ALL;
  Logger.root.onRecord.listen((record) {
    debugPrint('${record.level.name}: ${record.loggerName}: ${record.message}');
  });
  database = AppDatabase();
  runApp(const MyApp());
}
