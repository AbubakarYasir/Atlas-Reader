import 'dart:io';

import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/scanned_pdf.dart';
import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late AppDatabase db;

  setUp(() {
    db = AppDatabase.forTesting(NativeDatabase.memory());
  });

  tearDown(() async {
    await db.close();
  });

  test('library folders register idempotently and list by path', () async {
    final added = await db.addLibraryFolder(r'C:\Books');
    await db.addLibraryFolder(r'C:\Research');
    final duplicate = await db.addLibraryFolder(r'C:\Books');

    expect(added.path, r'C:\Books');
    expect(
      duplicate.id,
      added.id,
      reason: 're-adding the same path returns the same row',
    );

    final folders = await db.getLibraryFolders();
    expect(folders.map((f) => f.path), [r'C:\Books', r'C:\Research']);
  });

  test('upsert inserts, updates, and removes library files', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');

    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\a.pdf',
        fileName: 'a.pdf',
        bookmarkCount: 3,
        lastModified: DateTime(2026, 1, 1),
      ),
      ScannedPdf(
        filePath: r'C:\Books\b.pdf',
        fileName: 'b.pdf',
        bookmarkCount: 0,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);

    var files = await db.getLibraryFiles(folderId: folder.id);
    expect(files, hasLength(2));

    // Re-scan updates bookmark counts in place...
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\a.pdf',
        fileName: 'a.pdf',
        bookmarkCount: 7,
        lastModified: DateTime(2026, 2, 1),
      ),
    ]);

    files = await db.getLibraryFiles(folderId: folder.id);
    expect(files, hasLength(1));
    expect(files.single.bookmarkCount, 7);

    // ...and files that disappear on disk are removed.
    final folder2 = await db.addLibraryFolder(r'C:\Other');
    await db.upsertLibraryFiles(folder2.id, [
      ScannedPdf(
        filePath: r'C:\Other\c.pdf',
        fileName: 'c.pdf',
        bookmarkCount: 1,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);
    await db.upsertLibraryFiles(folder2.id, const []);
    expect(await db.getLibraryFiles(folderId: folder2.id), isEmpty);
  });

  test('progressive scan batches never remove books not reached yet', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    final modified = DateTime(2026, 1, 1);
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\a.pdf',
        fileName: 'a.pdf',
        bookmarkCount: 1,
        fileSizeBytes: 100,
        lastModified: modified,
      ),
      ScannedPdf(
        filePath: r'C:\Books\b.pdf',
        fileName: 'b.pdf',
        bookmarkCount: 1,
        fileSizeBytes: 200,
        lastModified: modified,
      ),
    ]);

    await db.upsertLibraryFiles(
      folder.id,
      [
        ScannedPdf(
          filePath: r'C:\Books\a.pdf',
          fileName: 'a.pdf',
          bookmarkCount: 1,
          fileSizeBytes: 100,
          lastModified: modified,
        ),
      ],
      removeMissing: false,
      reconcileRenames: false,
    );

    expect(await db.getLibraryFiles(folderId: folder.id), hasLength(2));
  });

  test(
    'directly opened PDFs appear in recents without a visible folder',
    () async {
      final modified = DateTime(2026, 1, 1);
      await db.rememberExternalFile(
        ScannedPdf(
          filePath: r'C:\Downloads\outside.pdf',
          fileName: 'outside.pdf',
          title: 'Outside the Library',
          author: 'Direct Open',
          bookmarkCount: 2,
          pageCount: 40,
          fileSizeBytes: 500,
          lastModified: modified,
        ),
        currentPage: 7,
      );

      expect(await db.getLibraryFolders(), isEmpty);
      final recents = await db.getFilteredLibraryFiles(
        onlyOpened: true,
        sortBy: LibrarySortBy.lastOpened,
        ascending: false,
      );
      expect(recents.single.title, 'Outside the Library');
      expect(recents.single.currentPage, 7);
    },
  );

  test(
    'upgrade tolerates columns already present in an older-version DB',
    () async {
      final file = File(
        '${Directory.systemTemp.path}${Platform.pathSeparator}'
        'atlas-reader-migration-${DateTime.now().microsecondsSinceEpoch}.sqlite',
      );
      final current = AppDatabase.forTesting(NativeDatabase(file));
      await current.getLibraryFolders();
      await current.customStatement('PRAGMA user_version = 5');
      await current.close();

      final upgraded = AppDatabase.forTesting(NativeDatabase(file));
      addTearDown(() async {
        await upgraded.close();
        if (await file.exists()) await file.delete();
      });

      await upgraded.rememberExternalFile(
        ScannedPdf(
          filePath: r'C:\Downloads\migration-safe.pdf',
          fileName: 'migration-safe.pdf',
          bookmarkCount: 0,
          lastModified: DateTime(2026, 1, 1),
        ),
      );

      expect(
        await upgraded.getFilteredLibraryFiles(onlyOpened: true),
        hasLength(1),
      );
    },
  );

  test('file name query is case-insensitive and filters by folder', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\Intro to Islam.pdf',
        fileName: 'Intro to Islam.pdf',
        bookmarkCount: 2,
        lastModified: DateTime(2026, 1, 1),
      ),
      ScannedPdf(
        filePath: r'C:\Books\Atlas.pdf',
        fileName: 'Atlas.pdf',
        bookmarkCount: 5,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);

    final matches = await db.searchLibraryFileNames('atl');
    expect(
      matches.map((p) => p.toLowerCase()),
      contains(endsWith('atlas.pdf')),
    );

    final filtered = await db.getLibraryFiles(query: 'ISLAM');
    expect(filtered, hasLength(1));
    expect(filtered.single.fileName, 'Intro to Islam.pdf');
  });

  test('quick-open searches title, author, and file name', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\usul.pdf',
        fileName: 'usul.pdf',
        title: 'أصول الفقه',
        author: 'وليد السعيدان',
        bookmarkCount: 3,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);

    expect(await db.searchLibraryFiles('أصول'), hasLength(1));
    expect(await db.searchLibraryFiles('السعيدان'), hasLength(1));
    expect(await db.searchLibraryFiles('USUL'), hasLength(1));
  });

  test('bookmark FTS maps SQLite bools and timestamps safely', () async {
    await db.addBookmark(
      filePath: r'C:\Books\usul.pdf',
      title: 'مبحث القياس Analogy',
      pageIndex: 4,
    );

    final results = await db.searchBookmarks('القياس');
    expect(results, hasLength(1));
    expect(results.single.isFolder, isFalse);
    expect(results.single.pageIndex, 4);
    expect(results.single.createdAt, isA<DateTime>());
  });

  test('removing a folder cascades its scanned files', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\a.pdf',
        fileName: 'a.pdf',
        bookmarkCount: 1,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);

    await db.removeLibraryFolder(folder.id);

    expect(await db.getLibraryFiles(folderId: folder.id), isEmpty);
    expect(await db.getLibraryFolders(), isEmpty);
  });

  test('renamed files retain their local bookmark association', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    final modified = DateTime(2026, 1, 1, 12);
    const oldPath = r'C:\Books\old-name.pdf';
    const newPath = r'C:\Books\renamed.pdf';

    await db.addBookmark(
      filePath: oldPath,
      title: 'Saved locally',
      pageIndex: 3,
    );
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: oldPath,
        fileName: 'old-name.pdf',
        bookmarkCount: 1,
        lastModified: modified,
      ),
    ]);

    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: newPath,
        fileName: 'renamed.pdf',
        bookmarkCount: 1,
        lastModified: modified,
      ),
    ]);

    final files = await db.getLibraryFiles(folderId: folder.id);
    final bookmarks = await db.getBookmarksForFile(newPath);
    expect(files.single.filePath, newPath);
    expect(bookmarks.single.title, 'Saved locally');
  });

  test('academic page offsets persist for a scanned library file', () async {
    final folder = await db.addLibraryFolder(r'C:\Books');
    const filePath = r'C:\Books\scholarly-text.pdf';
    await db.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: filePath,
        fileName: 'scholarly-text.pdf',
        bookmarkCount: 4,
        lastModified: DateTime(2026, 1, 1),
      ),
    ]);

    await db.updatePageOffset(filePath, -12);

    final files = await db.getLibraryFiles(folderId: folder.id);
    expect(files.single.pageOffset, -12);
  });

  test('renaming a folder preserves its folder state and page', () async {
    final folderId = await db.addBookmark(
      filePath: r'C:\Books\outline.pdf',
      title: 'Part one',
      pageIndex: 0,
      isFolder: true,
    );

    await db.renameBookmark(folderId, 'Part I');

    final bookmarks = await db.getBookmarksForFile(r'C:\Books\outline.pdf');
    expect(bookmarks.single.title, 'Part I');
    expect(bookmarks.single.isFolder, isTrue);
    expect(bookmarks.single.pageIndex, 0);
  });
}
