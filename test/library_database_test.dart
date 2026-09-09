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
}
