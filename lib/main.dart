import 'package:flutter/material.dart';
import 'package:logging/logging.dart';

import 'database.dart';
export 'features/library/library_screen.dart'
    show LibraryScreen, MyApp, database;

import 'features/library/library_screen.dart';

void main(List<String> arguments) async {
  WidgetsFlutterBinding.ensureInitialized();
  Logger.root.level = Level.ALL;
  Logger.root.onRecord.listen((record) {
    debugPrint('${record.level.name}: ${record.loggerName}: ${record.message}');
  });
  database = AppDatabase();
  String? initialPdf;
  for (final argument in arguments) {
    if (argument.toLowerCase().endsWith('.pdf')) {
      initialPdf = argument;
      break;
    }
  }
  runApp(MyApp(initialFilePath: initialPdf));
}
