import 'dart:io';

import 'package:drift/drift.dart';
import 'package:drift/native.dart';
import 'package:path/path.dart' as path;
import 'package:path_provider/path_provider.dart';

/// Creates the desktop SQLite connection for Atlas's local index.
///
/// This is infrastructure-only: feature and sync code access the database
/// through [AppDatabase], never by opening filesystem paths themselves.
LazyDatabase openAppDatabaseConnection() {
  return LazyDatabase(() async {
    final dbFolder = await getApplicationDocumentsDirectory();
    final databaseFile = File(path.join(dbFolder.path, 'atlas_db.sqlite'));
    return NativeDatabase(databaseFile, logStatements: true);
  });
}
