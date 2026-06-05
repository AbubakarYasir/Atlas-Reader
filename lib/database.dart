import 'dart:convert';
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

/// FileSnapshots table for tracking PDF state for reconciliation
@DataClassName('FileSnapshot')
class FileSnapshots extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get filePath => text()();
  TextColumn get lastKnownState => text()(); // JSON list of bookmark titles
  DateTimeColumn get updatedAt => dateTime().withDefault(currentDateAndTime)();
}

/// AppDatabase class extending _$AppDatabase
@DriftDatabase(tables: [Bookmarks, FileSnapshots])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 2;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (Migrator m) async {
        await m.createAll();
        // Create FTS5 virtual table for full-text search
        await customStatement('''
          CREATE VIRTUAL TABLE IF NOT EXISTS bookmarks_fts 
          USING fts5(title, content=bookmarks, content_rowid=id);
        ''');
        // Populate FTS table with existing data
        await customStatement('''
          INSERT INTO bookmarks_fts(rowid, title)
          SELECT id, title FROM bookmarks;
        ''');
        // Create triggers to keep FTS in sync
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS bookmarks_ai AFTER INSERT ON bookmarks BEGIN
            INSERT INTO bookmarks_fts(rowid, title) VALUES (new.id, new.title);
          END;
        ''');
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS bookmarks_ad AFTER DELETE ON bookmarks BEGIN
            INSERT INTO bookmarks_fts(bookmarks_fts, rowid, title) VALUES('delete', old.id, old.title);
          END;
        ''');
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS bookmarks_au AFTER UPDATE ON bookmarks BEGIN
            INSERT INTO bookmarks_fts(bookmarks_fts, rowid, title) VALUES('delete', old.id, old.title);
            INSERT INTO bookmarks_fts(rowid, title) VALUES (new.id, new.title);
          END;
        ''');
      },
      onUpgrade: (Migrator m, int from, int to) async {
        // For PoC, we'll drop and recreate
        if (from < 2) {
          await customStatement('DROP TABLE IF EXISTS bookmarks');
          await customStatement('DROP TABLE IF EXISTS file_snapshots');
          await customStatement('DROP TABLE IF EXISTS bookmarks_fts');
          await m.createAll();
          // Create FTS5 virtual table
          await customStatement('''
            CREATE VIRTUAL TABLE IF NOT EXISTS bookmarks_fts 
            USING fts5(title, content=bookmarks, content_rowid=id);
          ''');
          // Create triggers
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS bookmarks_ai AFTER INSERT ON bookmarks BEGIN
              INSERT INTO bookmarks_fts(rowid, title) VALUES (new.id, new.title);
            END;
          ''');
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS bookmarks_ad AFTER DELETE ON bookmarks BEGIN
              INSERT INTO bookmarks_fts(bookmarks_fts, rowid, title) VALUES('delete', old.id, old.title);
            END;
          ''');
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS bookmarks_au AFTER UPDATE ON bookmarks BEGIN
              INSERT INTO bookmarks_fts(bookmarks_fts, rowid, title) VALUES('delete', old.id, old.title);
              INSERT INTO bookmarks_fts(rowid, title) VALUES (new.id, new.title);
            END;
          ''');
        }
      },
    );
  }

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

  /// Get all bookmarks for a specific file
  Future<List<Bookmark>> getBookmarksForFile(String filePath) async {
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

  /// Sync bookmark from external source (PDF extraction)
  /// Adds bookmark only if filePath + title combination doesn't exist
  /// Returns true if new bookmark was added, false if it already exists
  Future<bool> syncBookmark({
    required String filePath,
    required String title,
    required int pageIndex,
  }) async {
    // Check if bookmark with exact filePath and title already exists
    final existing = await (select(bookmarks)
          ..where((tbl) => 
            tbl.filePath.equals(filePath) & 
            tbl.title.equals(title)))
        .getSingleOrNull();
    
    if (existing == null) {
      // Bookmark doesn't exist, insert it
      await into(bookmarks).insert(
        BookmarksCompanion(
          filePath: Value(filePath),
          title: Value(title),
          pageIndex: Value(pageIndex),
        ),
      );
      return true;
    }
    
    // Bookmark already exists
    return false;
  }

  /// Search bookmarks by title using FTS5
  /// If query is empty, returns all bookmarks
  Future<List<Bookmark>> searchBookmarks(String query) async {
    final trimmedQuery = query.trim();
    
    if (trimmedQuery.isEmpty) {
      return getAllBookmarks();
    }
    
    // Use FTS5 for lightning-fast full-text search
    final results = await customSelect(
      'SELECT b.* FROM bookmarks b '
      'INNER JOIN bookmarks_fts fts ON b.id = fts.rowid '
      'WHERE bookmarks_fts MATCH ? '
      'ORDER BY rank',
      variables: [Variable.withString(trimmedQuery)],
    ).map((row) => Bookmark.fromData(row.data)).get();
    
    return results;
  }

  /// Save or update file snapshot for reconciliation
  Future<void> saveFileSnapshot(String filePath, List<String> bookmarkTitles) async {
    final jsonState = jsonEncode(bookmarkTitles);
    
    final existing = await (select(fileSnapshots)
          ..where((tbl) => tbl.filePath.equals(filePath)))
        .getSingleOrNull();
    
    if (existing != null) {
      await (update(fileSnapshots)..where((tbl) => tbl.id.equals(existing.id)))
          .write(FileSnapshotsCompanion(
        lastKnownState: Value(jsonState),
        updatedAt: Value(DateTime.now()),
      ));
    } else {
      await into(fileSnapshots).insert(FileSnapshotsCompanion(
        filePath: Value(filePath),
        lastKnownState: Value(jsonState),
      ));
    }
  }

  /// Get file snapshot for reconciliation
  Future<FileSnapshot?> getFileSnapshot(String filePath) async {
    return (select(fileSnapshots)
          ..where((tbl) => tbl.filePath.equals(filePath)))
        .getSingleOrNull();
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
