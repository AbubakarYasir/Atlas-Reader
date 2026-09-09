import 'package:drift/drift.dart';

import 'bookmark_tree.dart';
import 'core/database/app_database_connection.dart';
import 'file_snapshot_state.dart';
import 'scanned_pdf.dart';

part 'database.g.dart';

/// Bookmarks table definition
@DataClassName('Bookmark')
class Bookmarks extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get filePath => text()();
  TextColumn get title => text()();
  IntColumn get pageIndex => integer().nullable()();
  TextColumn get description => text().nullable()();
  IntColumn get parentId => integer().nullable().references(
    Bookmarks,
    #id,
    onDelete: KeyAction.cascade,
  )();
  BoolColumn get isFolder => boolean().withDefault(const Constant(false))();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
  DateTimeColumn get updatedAt => dateTime().withDefault(currentDateAndTime)();
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
  IntColumn get bookmarkId =>
      integer().references(Bookmarks, #id, onDelete: KeyAction.cascade)();
  IntColumn get tagId =>
      integer().references(Tags, #id, onDelete: KeyAction.cascade)();
}

/// FileSnapshots table for tracking PDF state for reconciliation
@DataClassName('FileSnapshot')
class FileSnapshots extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get filePath => text()();
  TextColumn get lastKnownState =>
      text()(); // JSON v2 path keys (+ legacy titles)
  DateTimeColumn get updatedAt => dateTime().withDefault(currentDateAndTime)();
}

/// Registered library folders that are scanned for PDF files.
@DataClassName('LibraryFolder')
class LibraryFolders extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get path => text().unique()();
  DateTimeColumn get addedAt => dateTime().withDefault(currentDateAndTime)();
}

/// A document discovered by a library-folder scan, with metadata and reading state.
@DataClassName('LibraryFile')
class LibraryFiles extends Table {
  IntColumn get id => integer().autoIncrement()();
  IntColumn get folderId =>
      integer().references(LibraryFolders, #id, onDelete: KeyAction.cascade)();
  TextColumn get filePath => text().unique()();
  TextColumn get fileName => text()();
  TextColumn get title => text().nullable()();
  TextColumn get author => text().nullable()();
  TextColumn get format => text().withDefault(const Constant('PDF'))();
  IntColumn get bookmarkCount => integer().withDefault(const Constant(0))();
  IntColumn get pageCount => integer().withDefault(const Constant(0))();
  IntColumn get currentPage => integer().withDefault(const Constant(1))();
  IntColumn get fileSizeBytes => integer().withDefault(const Constant(0))();
  BoolColumn get isFavorite => boolean().withDefault(const Constant(false))();
  TextColumn get coverPath => text().nullable()();
  DateTimeColumn get lastOpened => dateTime().nullable()();
  TextColumn get series => text().nullable()();
  TextColumn get tags => text().nullable()();
  IntColumn get pageOffset => integer().withDefault(const Constant(0))();
  DateTimeColumn get lastModified => dateTime()();
  DateTimeColumn get lastScanned =>
      dateTime().withDefault(currentDateAndTime)();
}

/// In-text highlights, notes, underlines, and strikethroughs
@DataClassName('DocumentAnnotation')
class Annotations extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get filePath => text()();
  IntColumn get pageNumber => integer()();
  TextColumn get type => text()(); // highlight, underline, strikethrough, note
  TextColumn get selectedText => text().nullable()();
  TextColumn get note => text().nullable()();
  TextColumn get colorHex => text().withDefault(const Constant('#FFE066'))();
  RealColumn get rectX => real().withDefault(const Constant(0.0))();
  RealColumn get rectY => real().withDefault(const Constant(0.0))();
  RealColumn get rectWidth => real().withDefault(const Constant(0.0))();
  RealColumn get rectHeight => real().withDefault(const Constant(0.0))();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
}

/// AppDatabase class extending _$AppDatabase
@DriftDatabase(
  tables: [
    Bookmarks,
    Tags,
    BookmarkTags,
    FileSnapshots,
    LibraryFolders,
    LibraryFiles,
    Annotations,
  ],
)
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(openAppDatabaseConnection());

  /// Creates an in-memory or otherwise caller-provided database for tests.
  AppDatabase.forTesting(super.executor);

  @override
  int get schemaVersion => 7;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (Migrator m) async {
        await m.createAll();
        // Create FTS5 virtual table for bookmarks
        await customStatement('''
          CREATE VIRTUAL TABLE IF NOT EXISTS bookmarks_fts 
          USING fts5(title, content=bookmarks, content_rowid=id);
        ''');
        await customStatement('''
          INSERT INTO bookmarks_fts(rowid, title)
          SELECT id, title FROM bookmarks;
        ''');
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

        // Create FTS5 virtual table for annotations (highlights, notes)
        await customStatement('''
          CREATE VIRTUAL TABLE IF NOT EXISTS annotations_fts 
          USING fts5(selected_text, note, content=annotations, content_rowid=id);
        ''');
        await customStatement('''
          INSERT INTO annotations_fts(rowid, selected_text, note)
          SELECT id, selected_text, note FROM annotations;
        ''');
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS annotations_ai AFTER INSERT ON annotations BEGIN
            INSERT INTO annotations_fts(rowid, selected_text, note) VALUES (new.id, new.selected_text, new.note);
          END;
        ''');
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS annotations_ad AFTER DELETE ON annotations BEGIN
            INSERT INTO annotations_fts(annotations_fts, rowid, selected_text, note) VALUES('delete', old.id, old.selected_text, old.note);
          END;
        ''');
        await customStatement('''
          CREATE TRIGGER IF NOT EXISTS annotations_au AFTER UPDATE ON annotations BEGIN
            INSERT INTO annotations_fts(annotations_fts, rowid, selected_text, note) VALUES('delete', old.id, old.selected_text, old.note);
            INSERT INTO annotations_fts(rowid, selected_text, note) VALUES (new.id, new.selected_text, new.note);
          END;
        ''');
      },
      onUpgrade: (Migrator m, int from, int to) async {
        if (from < 4 && from >= 3) {
          await customStatement(
            'ALTER TABLE bookmarks RENAME COLUMN modified_at TO updated_at',
          );
        }
        if (from < 5) {
          await m.createTable(libraryFolders);
          await m.createTable(libraryFiles);
        }
        if (from < 6) {
          await m.addColumn(libraryFiles, libraryFiles.title);
          await m.addColumn(libraryFiles, libraryFiles.author);
          await m.addColumn(libraryFiles, libraryFiles.format);
          await m.addColumn(libraryFiles, libraryFiles.pageCount);
          await m.addColumn(libraryFiles, libraryFiles.currentPage);
          await m.addColumn(libraryFiles, libraryFiles.fileSizeBytes);
          await m.addColumn(libraryFiles, libraryFiles.isFavorite);
          await m.addColumn(libraryFiles, libraryFiles.coverPath);
          await m.addColumn(libraryFiles, libraryFiles.lastOpened);
          await m.addColumn(libraryFiles, libraryFiles.series);
          await m.addColumn(libraryFiles, libraryFiles.tags);
        }
        if (from < 7) {
          await m.addColumn(libraryFiles, libraryFiles.pageOffset);
          await m.createTable(annotations);
          await customStatement('''
            CREATE VIRTUAL TABLE IF NOT EXISTS annotations_fts 
            USING fts5(selected_text, note, content=annotations, content_rowid=id);
          ''');
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS annotations_ai AFTER INSERT ON annotations BEGIN
              INSERT INTO annotations_fts(rowid, selected_text, note) VALUES (new.id, new.selected_text, new.note);
            END;
          ''');
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS annotations_ad AFTER DELETE ON annotations BEGIN
              INSERT INTO annotations_fts(annotations_fts, rowid, selected_text, note) VALUES('delete', old.id, old.selected_text, old.note);
            END;
          ''');
          await customStatement('''
            CREATE TRIGGER IF NOT EXISTS annotations_au AFTER UPDATE ON annotations BEGIN
              INSERT INTO annotations_fts(annotations_fts, rowid, selected_text, note) VALUES('delete', old.id, old.selected_text, old.note);
              INSERT INTO annotations_fts(rowid, selected_text, note) VALUES (new.id, new.selected_text, new.note);
            END;
          ''');
        }
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
    return (select(
      bookmarks,
    )..where((tbl) => tbl.filePath.equals(filePath))).get();
  }

  /// Get all bookmarks for a specific file
  Future<List<Bookmark>> getBookmarksForFile(String filePath) async {
    return (select(
      bookmarks,
    )..where((tbl) => tbl.filePath.equals(filePath))).get();
  }

  /// Get all bookmarks
  Future<List<Bookmark>> getAllBookmarks() async {
    return select(bookmarks).get();
  }

  /// Update a bookmark's title, page index, and optional description.
  Future<int> updateBookmark(
    int id,
    String newTitle,
    int newPageIndex, {
    String? description,
  }) async {
    return await (update(bookmarks)..where((tbl) => tbl.id.equals(id))).write(
      BookmarksCompanion(
        title: Value(newTitle),
        pageIndex: Value(newPageIndex),
        description: description != null
            ? Value(description)
            : const Value.absent(),
        updatedAt: Value(DateTime.now()),
      ),
    );
  }

  /// Renames a bookmark or outline folder without changing its page or type.
  Future<int> renameBookmark(int id, String newTitle) {
    return (update(bookmarks)..where((tbl) => tbl.id.equals(id))).write(
      BookmarksCompanion(
        title: Value(newTitle),
        updatedAt: Value(DateTime.now()),
      ),
    );
  }

  /// Remove all tag links for a bookmark.
  Future<void> clearTagsForBookmark(int bookmarkId) async {
    await (delete(
      bookmarkTags,
    )..where((tbl) => tbl.bookmarkId.equals(bookmarkId))).go();
  }

  /// Delete a bookmark. Child rows cascade via foreign key.
  Future<int> deleteBookmark(int id) async {
    return (delete(bookmarks)..where((tbl) => tbl.id.equals(id))).go();
  }

  /// Delete all bookmarks for a specific file
  Future<int> deleteBookmarksByFile(String filePath) async {
    return (delete(
      bookmarks,
    )..where((tbl) => tbl.filePath.equals(filePath))).go();
  }

  /// Update every bookmark and file snapshot matching an old path to the new path in one transaction
  Future<int> updateFilePaths(String oldPath, String newPath) async {
    return await transaction(() async {
      final bookmarkChanges =
          await (update(bookmarks)
                ..where((tbl) => tbl.filePath.equals(oldPath)))
              .write(BookmarksCompanion(filePath: Value(newPath)));

      await (update(fileSnapshots)
            ..where((tbl) => tbl.filePath.equals(oldPath)))
          .write(FileSnapshotsCompanion(filePath: Value(newPath)));

      return bookmarkChanges;
    });
  }

  /// Finds a bookmark by file, parent, and title.
  Future<Bookmark?> findBookmark({
    required String filePath,
    required String title,
    int? parentId,
  }) async {
    return (select(bookmarks)..where((tbl) {
          final parentMatches = parentId == null
              ? tbl.parentId.isNull()
              : tbl.parentId.equals(parentId);
          return tbl.filePath.equals(filePath) &
              tbl.title.equals(title) &
              parentMatches;
        }))
        .getSingleOrNull();
  }

  /// Sync bookmark from external source (PDF extraction)
  /// Adds bookmark only if filePath + parent + title combination doesn't exist
  /// Returns true if new bookmark was added, false if it already exists
  Future<bool> syncBookmark({
    required String filePath,
    required String title,
    int? pageIndex,
    int? parentId,
    bool isFolder = false,
  }) async {
    final existing = await findBookmark(
      filePath: filePath,
      title: title,
      parentId: parentId,
    );

    if (existing == null) {
      await into(bookmarks).insert(
        BookmarksCompanion(
          filePath: Value(filePath),
          title: Value(title),
          pageIndex: pageIndex != null
              ? Value(pageIndex)
              : const Value.absent(),
          parentId: Value(parentId),
          isFolder: Value(isFolder),
        ),
      );
      return true;
    }

    return false;
  }

  /// Syncs a full extracted bookmark hierarchy into the database.
  /// Returns the number of newly added bookmarks.
  Future<int> syncBookmarkHierarchy(
    String filePath,
    List<Map<String, dynamic>> extracted,
  ) async {
    final sorted = List<Map<String, dynamic>>.from(extracted)
      ..sort(
        (a, b) =>
            (a['path'] as List).length.compareTo((b['path'] as List).length),
      );

    final pathToId = <String, int>{};
    var addedCount = 0;

    for (final item in sorted) {
      final path = List<String>.from(item['path'] as List);
      final title = path.last;
      final parentKey = path.length > 1
          ? BookmarkTree.pathKey(path.sublist(0, path.length - 1))
          : null;
      final parentId = parentKey != null ? pathToId[parentKey] : null;
      final pageIndex = item['pageIndex'] as int?;
      final isFolder = item['isFolder'] as bool? ?? false;
      final description = item['description'] as String?;
      final tags = (item['tags'] as List?)?.cast<String>() ?? const <String>[];

      final existing = await findBookmark(
        filePath: filePath,
        title: title,
        parentId: parentId,
      );

      if (existing != null) {
        pathToId[BookmarkTree.pathKey(path)] = existing.id;
        continue;
      }

      final bookmarkId = await addBookmark(
        filePath: filePath,
        title: title,
        pageIndex: pageIndex,
        description: description,
        parentId: parentId,
        isFolder: isFolder,
      );

      for (final tag in tags) {
        final cleanTag = tag.startsWith('#') ? tag.substring(1) : tag;
        if (cleanTag.isNotEmpty) {
          await addTagToBookmark(bookmarkId, cleanTag);
        }
      }

      pathToId[BookmarkTree.pathKey(path)] = bookmarkId;
      addedCount++;
    }

    return addedCount;
  }

  /// Search bookmarks by title using FTS5
  /// If query is empty, returns all bookmarks
  Future<List<Bookmark>> searchBookmarks(String query) async {
    final trimmedQuery = query.trim();

    if (trimmedQuery.isEmpty) {
      return getAllBookmarks();
    }

    // Use FTS5 for lightning-fast full-text search
    final results =
        await customSelect(
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
            updatedAt: row.data['updated_at'] as DateTime,
          );
        }).get();

    return results;
  }

  /// Save or update file snapshot using full hierarchical path keys.
  Future<void> saveFileSnapshot(
    String filePath,
    List<Bookmark> bookmarks,
  ) async {
    final jsonState = FileSnapshotState.fromBookmarks(bookmarks).encode();

    final existing = await (select(
      fileSnapshots,
    )..where((tbl) => tbl.filePath.equals(filePath))).getSingleOrNull();

    if (existing != null) {
      await (update(
        fileSnapshots,
      )..where((tbl) => tbl.id.equals(existing.id))).write(
        FileSnapshotsCompanion(
          lastKnownState: Value(jsonState),
          updatedAt: Value(DateTime.now()),
        ),
      );
    } else {
      await into(fileSnapshots).insert(
        FileSnapshotsCompanion(
          filePath: Value(filePath),
          lastKnownState: Value(jsonState),
        ),
      );
    }
  }

  /// Reads the stored path keys for a file, migrating legacy title snapshots.
  Future<List<String>> getFileSnapshotPathKeys(String filePath) async {
    final snapshot = await getFileSnapshot(filePath);
    if (snapshot == null) {
      return [];
    }
    return FileSnapshotState.decode(snapshot.lastKnownState).pathKeys;
  }

  /// Get file snapshot for reconciliation
  Future<FileSnapshot?> getFileSnapshot(String filePath) async {
    return (select(
      fileSnapshots,
    )..where((tbl) => tbl.filePath.equals(filePath))).getSingleOrNull();
  }

  /// Fetch tags for many bookmarks in one query (bookmark id -> tags).
  Future<Map<int, List<Tag>>> getTagsByBookmarkIds(
    Iterable<int> bookmarkIds,
  ) async {
    final ids = bookmarkIds.toList();
    if (ids.isEmpty) return {};

    final query = select(
      tags,
    ).join([innerJoin(bookmarkTags, bookmarkTags.tagId.equalsExp(tags.id))]);
    query.where(bookmarkTags.bookmarkId.isIn(ids));

    final map = <int, List<Tag>>{};
    for (final row in await query.get()) {
      final tag = row.readTable(tags);
      final bookmarkId = row.readTable(bookmarkTags).bookmarkId;
      map.putIfAbsent(bookmarkId, () => []).add(tag);
    }
    return map;
  }

  /// Fetch all tags for a specific bookmark
  Future<List<Tag>> getTagsForBookmark(int bookmarkId) async {
    final query = select(
      tags,
    ).join([innerJoin(bookmarkTags, bookmarkTags.tagId.equalsExp(tags.id))]);
    query.where(bookmarkTags.bookmarkId.equals(bookmarkId));
    return query.map((row) => row.readTable(tags)).get();
  }

  /// Add a tag to a bookmark (transaction helper)
  /// Creates the tag if it doesn't exist, then links it to the bookmark
  Future<void> addTagToBookmark(int bookmarkId, String tagName) async {
    return await transaction(() async {
      // Check if tag already exists
      final existingTag = await (select(
        tags,
      )..where((tbl) => tbl.name.equals(tagName))).getSingleOrNull();

      int tagId;
      if (existingTag != null) {
        tagId = existingTag.id;
      } else {
        // Create new tag
        tagId = await into(tags).insert(TagsCompanion(name: Value(tagName)));
      }

      // Check if the bookmark-tag relationship already exists
      final existingRelation =
          await (select(bookmarkTags)..where(
                (tbl) =>
                    tbl.bookmarkId.equals(bookmarkId) & tbl.tagId.equals(tagId),
              ))
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

  // --- Library folder management ---

  /// Registers [path] as a library folder to scan. Idempotent: an already
  /// registered path returns the existing row without inserting a duplicate.
  Future<LibraryFolder> addLibraryFolder(String path) async {
    final existing = await (select(
      libraryFolders,
    )..where((tbl) => tbl.path.equals(path))).getSingleOrNull();
    if (existing != null) {
      return existing;
    }
    final id = await into(
      libraryFolders,
    ).insert(LibraryFoldersCompanion(path: Value(path)));
    return (select(
      libraryFolders,
    )..where((tbl) => tbl.id.equals(id))).getSingle();
  }

  /// Returns every registered library folder, ordered by path.
  Future<List<LibraryFolder>> getLibraryFolders() async {
    return (select(
      libraryFolders,
    )..orderBy([(tbl) => OrderingTerm.asc(tbl.path)])).get();
  }

  /// Removes a library folder; its scanned files cascade away.
  Future<int> removeLibraryFolder(int id) async {
    return transaction(() async {
      await (delete(
        libraryFiles,
      )..where((tbl) => tbl.folderId.equals(id))).go();
      return (delete(libraryFolders)..where((tbl) => tbl.id.equals(id))).go();
    });
  }

  /// Reconciles a scan result against the recorded files for [folderId].
  /// Updates bookmark counts for known files, inserts new files, and removes
  /// any tracked file that no longer exists on disk.
  Future<void> upsertLibraryFiles(
    int folderId,
    List<ScannedPdf> scanned,
  ) async {
    await transaction(() async {
      final tracked = await (select(
        libraryFiles,
      )..where((tbl) => tbl.folderId.equals(folderId))).get();
      final livePaths = scanned.map((pdf) => pdf.filePath).toSet();
      final missingTracked = tracked
          .where((file) => !livePaths.contains(file.filePath))
          .toList();
      final reconciledIds = <int>{};

      for (final pdf in scanned) {
        final existing = await (select(
          libraryFiles,
        )..where((tbl) => tbl.filePath.equals(pdf.filePath))).getSingleOrNull();

        if (existing != null) {
          await (update(
            libraryFiles,
          )..where((tbl) => tbl.id.equals(existing.id))).write(
            LibraryFilesCompanion(
              fileName: Value(pdf.fileName),
              title: pdf.title != null ? Value(pdf.title) : const Value.absent(),
              author: pdf.author != null ? Value(pdf.author) : const Value.absent(),
              format: Value(pdf.format),
              bookmarkCount: Value(pdf.bookmarkCount),
              pageCount: pdf.pageCount > 0 ? Value(pdf.pageCount) : const Value.absent(),
              fileSizeBytes: pdf.fileSizeBytes > 0 ? Value(pdf.fileSizeBytes) : const Value.absent(),
              coverPath: pdf.coverPath != null ? Value(pdf.coverPath) : const Value.absent(),
              tags: pdf.tags != null ? Value(pdf.tags) : const Value.absent(),
              series: pdf.series != null ? Value(pdf.series) : const Value.absent(),
              lastModified: Value(pdf.lastModified),
              lastScanned: Value(DateTime.now()),
            ),
          );
        } else {
          // A rename or move inside the same library folder produces a new
          // path and a missing old path. Match the stable outline signature
          // (bookmark count + modification time) to retain local bookmarks.
          final moved = missingTracked.where((file) {
            return !reconciledIds.contains(file.id) &&
                file.bookmarkCount == pdf.bookmarkCount &&
                file.lastModified.isAtSameMomentAs(pdf.lastModified);
          }).firstOrNull;

          if (moved != null) {
            reconciledIds.add(moved.id);
            await (update(
              libraryFiles,
            )..where((tbl) => tbl.id.equals(moved.id))).write(
              LibraryFilesCompanion(
                folderId: Value(folderId),
                filePath: Value(pdf.filePath),
                fileName: Value(pdf.fileName),
                title: pdf.title != null ? Value(pdf.title) : const Value.absent(),
                author: pdf.author != null ? Value(pdf.author) : const Value.absent(),
                format: Value(pdf.format),
                pageCount: pdf.pageCount > 0 ? Value(pdf.pageCount) : const Value.absent(),
                fileSizeBytes: pdf.fileSizeBytes > 0 ? Value(pdf.fileSizeBytes) : const Value.absent(),
                coverPath: pdf.coverPath != null ? Value(pdf.coverPath) : const Value.absent(),
                tags: pdf.tags != null ? Value(pdf.tags) : const Value.absent(),
                series: pdf.series != null ? Value(pdf.series) : const Value.absent(),
                lastScanned: Value(DateTime.now()),
              ),
            );
            await (update(bookmarks)
                  ..where((tbl) => tbl.filePath.equals(moved.filePath)))
                .write(BookmarksCompanion(filePath: Value(pdf.filePath)));
            await (update(fileSnapshots)
                  ..where((tbl) => tbl.filePath.equals(moved.filePath)))
                .write(FileSnapshotsCompanion(filePath: Value(pdf.filePath)));
            continue;
          }

          await into(libraryFiles).insert(
            LibraryFilesCompanion(
              folderId: Value(folderId),
              filePath: Value(pdf.filePath),
              fileName: Value(pdf.fileName),
              title: Value(pdf.title),
              author: Value(pdf.author),
              format: Value(pdf.format),
              bookmarkCount: Value(pdf.bookmarkCount),
              pageCount: Value(pdf.pageCount),
              currentPage: const Value(1),
              fileSizeBytes: Value(pdf.fileSizeBytes),
              isFavorite: const Value(false),
              coverPath: Value(pdf.coverPath),
              tags: Value(pdf.tags),
              series: Value(pdf.series),
              lastModified: Value(pdf.lastModified),
              lastScanned: Value(DateTime.now()),
            ),
          );
        }
      }

      for (final file in tracked) {
        if (!livePaths.contains(file.filePath) &&
            !reconciledIds.contains(file.id)) {
          await (delete(
            libraryFiles,
          )..where((tbl) => tbl.id.equals(file.id))).go();
        }
      }
    });
  }

  /// Lists scanned library files, optionally filtered by [folderId] and a
  /// case-insensitive [query] against the file name.
  Future<List<LibraryFile>> getLibraryFiles({
    int? folderId,
    String? query,
  }) async {
    final q = select(libraryFiles)
      ..orderBy([(tbl) => OrderingTerm.asc(tbl.fileName)]);

    if (folderId != null) {
      q.where((tbl) => tbl.folderId.equals(folderId));
    }
    final trimmedQuery = query?.trim() ?? '';
    if (trimmedQuery.isNotEmpty) {
      q.where(
        (tbl) => tbl.fileName.lower().contains(trimmedQuery.toLowerCase()),
      );
    }

    return q.get();
  }

  /// Returns the menu-book header path and bookmark-count stats for a folder's
  /// scanned files (used by the library folders screen).
  Future<({List<LibraryFile> files, int folderFileCount})> libraryFolderStats(
    int folderId,
  ) async {
    final files = await getLibraryFiles(folderId: folderId);
    return (files: files, folderFileCount: files.length);
  }

  /// Finds distinct PDF paths whose file name matches [query] (case-insensitive).
  /// Used so the Command Center can surface matching books even without
  /// bookmarks.
  Future<List<String>> searchLibraryFileNames(String query) async {
    final trimmed = query.trim();
    if (trimmed.isEmpty) return const [];

    final files =
        await (select(libraryFiles)..where(
              (tbl) => tbl.fileName.lower().contains(trimmed.toLowerCase()),
            ))
            .get();

    final seen = <String>{};
    final paths = <String>[];
    for (final file in files) {
      if (seen.add(file.filePath)) {
        paths.add(file.filePath);
      }
    }
    return paths;
  }

  /// Toggle favorite status of a book
  Future<bool> toggleFavorite(int fileId) async {
    final file = await (select(
      libraryFiles,
    )..where((tbl) => tbl.id.equals(fileId))).getSingleOrNull();
    if (file == null) return false;
    final nextFav = !file.isFavorite;
    await (update(
      libraryFiles,
    )..where((tbl) => tbl.id.equals(fileId))).write(
      LibraryFilesCompanion(isFavorite: Value(nextFav)),
    );
    return nextFav;
  }

  /// Update active reading progress
  Future<void> updateReadingProgress(
    String filePath,
    int currentPage, {
    int? pageCount,
  }) async {
    final existing = await (select(
      libraryFiles,
    )..where((tbl) => tbl.filePath.equals(filePath))).getSingleOrNull();
    if (existing != null) {
      await (update(
        libraryFiles,
      )..where((tbl) => tbl.id.equals(existing.id))).write(
        LibraryFilesCompanion(
          currentPage: Value(currentPage),
          pageCount: pageCount != null && pageCount > 0
              ? Value(pageCount)
              : const Value.absent(),
          lastOpened: Value(DateTime.now()),
        ),
      );
    }
  }

  /// Update book metadata
  Future<void> updateBookMetadata(
    int fileId, {
    String? title,
    String? author,
    String? series,
    String? tags,
    String? coverPath,
  }) async {
    await (update(
      libraryFiles,
    )..where((tbl) => tbl.id.equals(fileId))).write(
      LibraryFilesCompanion(
        title: title != null ? Value(title) : const Value.absent(),
        author: author != null ? Value(author) : const Value.absent(),
        series: series != null ? Value(series) : const Value.absent(),
        tags: tags != null ? Value(tags) : const Value.absent(),
        coverPath: coverPath != null ? Value(coverPath) : const Value.absent(),
      ),
    );
  }

  /// Returns map of distinct authors to book counts
  Future<Map<String, int>> getDistinctAuthors() async {
    final files = await select(libraryFiles).get();
    final counts = <String, int>{};
    for (final file in files) {
      final author = file.author?.trim();
      if (author != null && author.isNotEmpty) {
        counts[author] = (counts[author] ?? 0) + 1;
      }
    }
    return counts;
  }

  /// Returns map of distinct tags to book counts
  Future<Map<String, int>> getDistinctTags() async {
    final files = await select(libraryFiles).get();
    final counts = <String, int>{};
    for (final file in files) {
      final fileTags = file.tags?.split(',') ?? const [];
      for (final t in fileTags) {
        final clean = t.trim().replaceAll(RegExp(r'^#'), '');
        if (clean.isNotEmpty) {
          counts[clean] = (counts[clean] ?? 0) + 1;
        }
      }
    }
    return counts;
  }

  /// Returns map of distinct series to book counts
  Future<Map<String, int>> getDistinctSeries() async {
    final files = await select(libraryFiles).get();
    final counts = <String, int>{};
    for (final file in files) {
      final series = file.series?.trim();
      if (series != null && series.isNotEmpty) {
        counts[series] = (counts[series] ?? 0) + 1;
      }
    }
    return counts;
  }

  /// Filtered, searchable, sorted library files list
  Future<List<LibraryFile>> getFilteredLibraryFiles({
    int? folderId,
    String? query,
    String? author,
    String? tag,
    String? series,
    String? format,
    bool? onlyFavorites,
    LibrarySortBy sortBy = LibrarySortBy.title,
    bool ascending = true,
  }) async {
    final q = select(libraryFiles);
    if (folderId != null) {
      q.where((tbl) => tbl.folderId.equals(folderId));
    }
    if (onlyFavorites == true) {
      q.where((tbl) => tbl.isFavorite.equals(true));
    }
    if (format != null && format.isNotEmpty) {
      q.where((tbl) => tbl.format.equals(format));
    }
    if (author != null && author.isNotEmpty) {
      q.where((tbl) => tbl.author.equals(author));
    }
    if (series != null && series.isNotEmpty) {
      q.where((tbl) => tbl.series.equals(series));
    }

    final trimmedQuery = query?.trim() ?? '';
    if (trimmedQuery.isNotEmpty) {
      final lower = trimmedQuery.toLowerCase();
      q.where(
        (tbl) =>
            tbl.fileName.lower().contains(lower) |
            tbl.title.lower().contains(lower) |
            tbl.author.lower().contains(lower),
      );
    }

    var list = await q.get();

    if (tag != null && tag.isNotEmpty) {
      final cleanTag = tag.trim().replaceAll(RegExp(r'^#'), '').toLowerCase();
      list = list.where((file) {
        final tags = file.tags?.toLowerCase() ?? '';
        return tags
            .split(',')
            .map((t) => t.trim().replaceAll(RegExp(r'^#'), ''))
            .contains(cleanTag);
      }).toList();
    }

    list.sort((a, b) {
      int cmp = 0;
      switch (sortBy) {
        case LibrarySortBy.title:
          final aName = (a.title?.isNotEmpty ?? false) ? a.title! : a.fileName;
          final bName = (b.title?.isNotEmpty ?? false) ? b.title! : b.fileName;
          cmp = aName.toLowerCase().compareTo(bName.toLowerCase());
          break;
        case LibrarySortBy.author:
          final aAuthor = a.author ?? '';
          final bAuthor = b.author ?? '';
          cmp = aAuthor.toLowerCase().compareTo(bAuthor.toLowerCase());
          break;
        case LibrarySortBy.dateAdded:
          cmp = a.lastScanned.compareTo(b.lastScanned);
          break;
        case LibrarySortBy.lastOpened:
          final aTime = a.lastOpened ?? DateTime.fromMillisecondsSinceEpoch(0);
          final bTime = b.lastOpened ?? DateTime.fromMillisecondsSinceEpoch(0);
          cmp = aTime.compareTo(bTime);
          break;
        case LibrarySortBy.fileSize:
          cmp = a.fileSizeBytes.compareTo(b.fileSizeBytes);
          break;
        case LibrarySortBy.progress:
          final aProg = a.pageCount > 0 ? (a.currentPage / a.pageCount) : 0.0;
          final bProg = b.pageCount > 0 ? (b.currentPage / b.pageCount) : 0.0;
          cmp = aProg.compareTo(bProg);
          break;
      }
      return ascending ? cmp : -cmp;
    });

    return list;
  }

  /// Add a research annotation (highlight, underline, strikethrough, note)
  Future<int> addAnnotation({
    required String filePath,
    required int pageNumber,
    required String type,
    String? selectedText,
    String? note,
    String colorHex = '#FFE066',
    double rectX = 0.0,
    double rectY = 0.0,
    double rectWidth = 0.0,
    double rectHeight = 0.0,
  }) async {
    return into(annotations).insert(
      AnnotationsCompanion(
        filePath: Value(filePath),
        pageNumber: Value(pageNumber),
        type: Value(type),
        selectedText: Value(selectedText),
        note: Value(note),
        colorHex: Value(colorHex),
        rectX: Value(rectX),
        rectY: Value(rectY),
        rectWidth: Value(rectWidth),
        rectHeight: Value(rectHeight),
      ),
    );
  }

  /// Get all annotations for a document, optionally scoped to a single page
  Future<List<DocumentAnnotation>> getAnnotationsForFile(
    String filePath, {
    int? pageNumber,
  }) async {
    final q = select(annotations)..where((tbl) => tbl.filePath.equals(filePath));
    if (pageNumber != null) {
      q.where((tbl) => tbl.pageNumber.equals(pageNumber));
    }
    q.orderBy([
      (tbl) => OrderingTerm.asc(tbl.pageNumber),
      (tbl) => OrderingTerm.asc(tbl.createdAt),
    ]);
    return q.get();
  }

  /// Delete an annotation by ID
  Future<int> deleteAnnotation(int id) async {
    return (delete(annotations)..where((tbl) => tbl.id.equals(id))).go();
  }

  /// Update the note of an annotation
  Future<int> updateAnnotationNote(int id, String note) async {
    return (update(annotations)..where((tbl) => tbl.id.equals(id))).write(
      AnnotationsCompanion(note: Value(note)),
    );
  }

  /// Update academic page offset for a library file
  Future<void> updatePageOffset(String filePath, int pageOffset) async {
    await (update(libraryFiles)..where((tbl) => tbl.filePath.equals(filePath)))
        .write(LibraryFilesCompanion(pageOffset: Value(pageOffset)));
  }

  /// Full-text search across all highlights and notes
  Future<List<DocumentAnnotation>> searchAnnotationsFts(String query) async {
    final trimmed = query.trim();
    if (trimmed.isEmpty) return const [];
    final sanitized = trimmed.replaceAll("'", "''");
    final rows = await customSelect(
      '''
      SELECT annotations.* FROM annotations
      JOIN annotations_fts ON annotations.id = annotations_fts.rowid
      WHERE annotations_fts MATCH '$sanitized*'
      ORDER BY annotations.created_at DESC
      ''',
      readsFrom: {annotations},
    ).get();

    return rows.map((row) => annotations.map(row.data)).toList();
  }
}

enum LibrarySortBy {
  title,
  author,
  dateAdded,
  lastOpened,
  fileSize,
  progress,
}
