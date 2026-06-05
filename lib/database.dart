import 'dart:io';
import 'package:drift/drift.dart';
import 'package:drift/native.dart';
import 'package:path_provider/path_provider.dart';
import 'package:path/path.dart' as p;

part 'database.g.dart';

/// Bookmarks table definition
@DataClassName('Bookmark')
class Bookmarks extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get filePath => text()();
  TextColumn get title => text()();
  IntColumn get pageIndex => integer()();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
  DateTimeColumn get modifiedAt =>
      dateTime().withDefault(currentDateAndTime)();
}

/// AppDatabase class extending _$AppDatabase
@DriftDatabase(tables: [Bookmarks])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 1;

  /// Add a new bookmark to the database
  Future<int> addBookmark({
    required String filePath,
    required String title,
    required int pageIndex,
  }) async {
    return into(bookmarks).insert(
      BookmarksCompanion(
        filePath: Value(filePath),
        title: Value(title),
        pageIndex: Value(pageIndex),
      ),
    );
  }

  /// Get all bookmarks for a specific file
  Future<List<Bookmark>> getBookmarksByFile(String filePath) async {
    return (select(bookmarks)
          ..where((tbl) => tbl.filePath.equals(filePath)))
        .get();
  }

  /// Get all bookmarks
  Future<List<Bookmark>> getAllBookmarks() async {
    return select(bookmarks).get();
  }

  /// Update a bookmark
  Future<bool> updateBookmark({
    required int id,
    required String title,
    required int pageIndex,
  }) async {
    return update(bookmarks).replace(
      Bookmark(
        id: id,
        filePath: (await (select(bookmarks)
              ..where((tbl) => tbl.id.equals(id)))
            .getSingleOrNull())
            ?.filePath ??
            '',
        title: title,
        pageIndex: pageIndex,
        createdAt: (await (select(bookmarks)
              ..where((tbl) => tbl.id.equals(id)))
            .getSingleOrNull())
            ?.createdAt ??
            DateTime.now(),
        modifiedAt: DateTime.now(),
      ),
    );
  }

  /// Delete a bookmark
  Future<int> deleteBookmark(int id) async {
    return (delete(bookmarks)..where((tbl) => tbl.id.equals(id))).go();
  }

  /// Delete all bookmarks for a specific file
  Future<int> deleteBookmarksByFile(String filePath) async {
    return (delete(bookmarks)..where((tbl) => tbl.filePath.equals(filePath)))
        .go();
  }

  /// Search bookmarks by title
  Future<List<Bookmark>> searchBookmarks(String query) async {
    return (select(bookmarks)
          ..where((tbl) => tbl.title.like('%$query%')))
        .get();
  }
}

/// Open connection to the SQLite database
LazyDatabase _openConnection() {
  return LazyDatabase(() async {
    final dbFolder = await getApplicationDocumentsDirectory();
    final file = File(p.join(dbFolder.path, 'atlas_db.sqlite'));
    return NativeDatabase(file, logStatements: true);
  });
}
