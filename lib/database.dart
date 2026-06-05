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
  IntColumn get pageIndex => integer().nullable()();
  TextColumn get description => text().nullable()();
  IntColumn get parentId => integer().nullable().references(Bookmarks, #id, onDelete: KeyAction.cascade)();
  BoolColumn get isFolder => boolean().withDefault(const Constant(false))();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
  DateTimeColumn get modifiedAt =>
      dateTime().withDefault(currentDateAndTime)();
}

/// Tags table definition
@DataClassName('Tag')
class Tags extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get name => text().unique()();
}

/// BookmarkTags join table for many-to-many relationship
@DataClassName('BookmarkTag')
class BookmarkTags extends Table {
  IntColumn get bookmarkId => integer().references(Bookmarks, #id, onDelete: KeyAction.cascade)();
  IntColumn get tagId => integer().references(Tags, #id, onDelete: KeyAction.cascade)();
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
@DriftDatabase(tables: [Bookmarks, Tags, BookmarkTags, FileSnapshots])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 3;

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
        if (from < 3) {
          await customStatement('DROP TABLE IF EXISTS bookmarks');
          await customStatement('DROP TABLE IF EXISTS tags');
          await customStatement('DROP TABLE IF EXISTS bookmark_tags');
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
    int? pageIndex,
    String? description,
    int? parentId,
    bool isFolder = false,
  }) async {
    return into(bookmarks).insert(
      BookmarksCompanion(
        filePath: Value(filePath),
        title: Value(title),
        pageIndex: Value(pageIndex),
        description: Value(description),
        parentId: Value(parentId),
        isFolder: Value(isFolder),
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

  /// Update a bookmark's title and page index using an efficient drift update statement.
  Future<int> updateBookmark(int id, String newTitle, int newPageIndex) async {
    return await (update(bookmarks)
          ..where((tbl) => tbl.id.equals(id)))
        .write(BookmarksCompanion(
      title: Value(newTitle),
      pageIndex: Value(newPageIndex),
      modifiedAt: Value(DateTime.now()),
    ));
  }

  /// Delete a bookmark and cascade delete nested children
  Future<int> deleteBookmark(int id) async {
    return await transaction(() async {
      // First, delete all child bookmarks (those with parentId = id)
      await (delete(bookmarks)..where((tbl) => tbl.parentId.equals(id))).go();
      // Then delete the bookmark itself
      return (delete(bookmarks)..where((tbl) => tbl.id.equals(id))).go();
    });
  }

  /// Delete all bookmarks for a specific file
  Future<int> deleteBookmarksByFile(String filePath) async {
    return (delete(bookmarks)..where((tbl) => tbl.filePath.equals(filePath)))
        .go();
  }

  /// Update every bookmark and file snapshot matching an old path to the new path in one transaction
  Future<int> updateFilePaths(String oldPath, String newPath) async {
    return await transaction(() async {
      final bookmarkChanges = await (update(bookmarks)
            ..where((tbl) => tbl.filePath.equals(oldPath)))
          .write(BookmarksCompanion(filePath: Value(newPath)));

      await (update(fileSnapshots)
            ..where((tbl) => tbl.filePath.equals(oldPath)))
          .write(FileSnapshotsCompanion(filePath: Value(newPath)));

      return bookmarkChanges;
    });
  }

  /// Sync bookmark from external source (PDF extraction)
  /// Adds bookmark only if filePath + title combination doesn't exist
  /// Returns true if new bookmark was added, false if it already exists
  Future<bool> syncBookmark({
    required String filePath,
    required String title,
    int? pageIndex,
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
    ).map((row) {
      return Bookmark(
        id: row.data['id'] as int,
        filePath: row.data['file_path'] as String,
        title: row.data['title'] as String,
        pageIndex: row.data['page_index'] as int?,
        description: row.data['description'] as String?,
        parentId: row.data['parent_id'] as int?,
        isFolder: row.data['is_folder'] as bool,
        createdAt: row.data['created_at'] as DateTime,
        modifiedAt: row.data['modified_at'] as DateTime,
      );
    }).get();

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

  /// Fetch all tags for a specific bookmark
  Future<List<Tag>> getTagsForBookmark(int bookmarkId) async {
    final query = select(tags).join([
      innerJoin(bookmarkTags, bookmarkTags.tagId.equalsExp(tags.id)),
    ]);
    query.where(bookmarkTags.bookmarkId.equals(bookmarkId));
    return query.map((row) => row.readTable(tags)).get();
  }

  /// Add a tag to a bookmark (transaction helper)
  /// Creates the tag if it doesn't exist, then links it to the bookmark
  Future<void> addTagToBookmark(int bookmarkId, String tagName) async {
    return await transaction(() async {
      // Check if tag already exists
      final existingTag = await (select(tags)
            ..where((tbl) => tbl.name.equals(tagName)))
          .getSingleOrNull();

      int tagId;
      if (existingTag != null) {
        tagId = existingTag.id;
      } else {
        // Create new tag
        tagId = await into(tags).insert(
          TagsCompanion(name: Value(tagName)),
        );
      }

      // Check if the bookmark-tag relationship already exists
      final existingRelation = await (select(bookmarkTags)
            ..where((tbl) =>
                tbl.bookmarkId.equals(bookmarkId) &
                tbl.tagId.equals(tagId)))
          .getSingleOrNull();

      // Only insert if the relationship doesn't exist
      if (existingRelation == null) {
        await into(bookmarkTags).insert(
          BookmarkTagsCompanion(
            bookmarkId: Value(bookmarkId),
            tagId: Value(tagId),
          ),
        );
      }
    });
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
