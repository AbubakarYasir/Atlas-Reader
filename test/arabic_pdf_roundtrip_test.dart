import 'dart:io';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/pdf_engine.dart';
import 'package:atlas_poc/sync_diff.dart';
import 'package:atlas_poc/sync_engine.dart';
import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:syncfusion_flutter_pdf/pdf.dart';

void main() {
  late Directory tempDir;
  late String pdfPath;
  late AppDatabase db;
  late PdfEngine pdfEngine;
  late SyncEngine syncEngine;
  const fileSystem = WindowsDocumentFileSystem();

  setUp(() async {
    tempDir = await Directory.systemTemp.createTemp('atlas_arabic_test_');
    pdfPath =
        '${tempDir.path}${Platform.pathSeparator}arabic_scholarly_book.pdf';
    db = AppDatabase.forTesting(NativeDatabase.memory());
    pdfEngine = PdfEngine(fileSystem: fileSystem);
    syncEngine = SyncEngine(db, pdfEngine: pdfEngine);

    final document = PdfDocument();
    final p1 = document.pages.add();
    final p2 = document.pages.add();
    final p3 = document.pages.add();
    final p4 = document.pages.add();

    final b1 = document.bookmarks.add('أصول الفقه على منهج أهل السنة');
    b1.destination = PdfDestination(p1);

    final b1Sub1 = b1.add('المقدمة في تعريف أصول الفقه');
    b1Sub1.destination = PdfDestination(p2);

    final b1Sub2 = b1.add('الباب الأول: الأدلة الشرعية');
    b1Sub2.destination = PdfDestination(p3);

    final b2 = document.bookmarks.add('المصادر والمراجع');
    b2.destination = PdfDestination(p4);

    final bytes = document.saveSync();
    document.dispose();

    await fileSystem.writeAsBytesInBackground(pdfPath, bytes, flush: true);
  });

  tearDown(() async {
    await db.close();
    if (await tempDir.exists()) {
      await tempDir.delete(recursive: true);
    }
  });

  test(
    'Stage 6: Arabic PDF Round-Trip with nested TOC, tags, and bi-directional sync',
    () async {
      final extractedInitial = await pdfEngine.extractBookmarks(pdfPath);
      expect(extractedInitial, hasLength(4));

      expect(extractedInitial[0]['title'], 'أصول الفقه على منهج أهل السنة');
      expect(extractedInitial[0]['pageIndex'], 0);
      expect(extractedInitial[0]['path'], ['أصول الفقه على منهج أهل السنة']);

      expect(extractedInitial[1]['title'], 'المقدمة في تعريف أصول الفقه');
      expect(extractedInitial[1]['pageIndex'], 1);
      expect(extractedInitial[1]['path'], [
        'أصول الفقه على منهج أهل السنة',
        'المقدمة في تعريف أصول الفقه',
      ]);

      expect(extractedInitial[2]['title'], 'الباب الأول: الأدلة الشرعية');
      expect(extractedInitial[2]['pageIndex'], 2);

      expect(extractedInitial[3]['title'], 'المصادر والمراجع');
      expect(extractedInitial[3]['pageIndex'], 3);

      final importedCount = await db.syncBookmarkHierarchy(
        pdfPath,
        extractedInitial,
      );
      expect(importedCount, 4);

      final dbBookmarks = await db.getBookmarksForFile(pdfPath);
      expect(dbBookmarks, hasLength(4));

      final parentBookmark = dbBookmarks.firstWhere(
        (b) => b.title == 'أصول الفقه على منهج أهل السنة',
      );
      final childBookmark = dbBookmarks.firstWhere(
        (b) => b.title == 'المقدمة في تعريف أصول الفقه',
      );
      expect(childBookmark.parentId, parentBookmark.id);

      final newBookmarkId = await db.addBookmark(
        filePath: pdfPath,
        title: 'الفصل الأول: القرآن الكريم والسنة النبوية',
        pageIndex: 2,
        parentId: parentBookmark.id,
        description: 'دراسة استدلالية متعمقة',
      );
      await db.addTagToBookmark(newBookmarkId, 'دراسات');
      await db.addTagToBookmark(newBookmarkId, 'فقه');

      final tags = await db.getTagsForBookmark(newBookmarkId);
      expect(tags.map((t) => t.name).toList(), containsAll(['دراسات', 'فقه']));

      final updatedDbBookmarks = await db.getBookmarksForFile(pdfPath);
      expect(updatedDbBookmarks, hasLength(5));

      final tagsByBookmarkId = <int, List<String>>{
        newBookmarkId: ['دراسات', 'فقه'],
      };

      final overwriteSuccess = await pdfEngine.overwriteBookmarkTree(
        pdfPath,
        updatedDbBookmarks,
        tagsByBookmarkId: tagsByBookmarkId,
      );
      expect(overwriteSuccess, isTrue);

      final externalBytes = await fileSystem.readAsBytesInBackground(pdfPath);
      final externalDoc = PdfDocument(inputBytes: externalBytes);
      expect(externalDoc.bookmarks.count, 2);
      final extRoot1 = externalDoc.bookmarks[0];
      expect(extRoot1.title, 'أصول الفقه على منهج أهل السنة');
      expect(extRoot1.count, 3);

      final extSubtitles = [
        extRoot1[0].title,
        extRoot1[1].title,
        extRoot1[2].title,
      ];
      expect(extSubtitles, contains('المقدمة في تعريف أصول الفقه'));
      expect(extSubtitles, contains('الباب الأول: الأدلة الشرعية'));
      expect(
        extSubtitles,
        contains('الفصل الأول: القرآن الكريم والسنة النبوية - #دراسات, #فقه'),
      );

      final extPage = extRoot1[2].destination?.page;
      expect(extPage, isNotNull);
      final extPageIndex = externalDoc.pages.indexOf(extPage!);
      expect(extPageIndex, 2);

      final extNewChild = extRoot1.add('خاتمة البحث والتوصيات');
      extNewChild.destination = PdfDestination(externalDoc.pages[3]);
      final modifiedBytes = externalDoc.saveSync();
      externalDoc.dispose();

      await fileSystem.writeAsBytesInBackground(
        pdfPath,
        modifiedBytes,
        flush: true,
      );

      final externalBookmarks = await pdfEngine.extractBookmarks(pdfPath);
      expect(
        externalBookmarks.any((b) => b['title'] == 'خاتمة البحث والتوصيات'),
        isTrue,
      );

      final currentDb = await db.getBookmarksForFile(pdfPath);
      final diff = SyncDiffCalculator.calculateFromSources(
        dbBookmarks: currentDb,
        pdfExtracted: externalBookmarks,
      );
      expect(diff.toAddToDb, hasLength(1));
      expect(diff.toAddToDb.first, contains('خاتمة البحث والتوصيات'));

      final reconcileResult = await syncEngine.reconcile(pdfPath);
      expect(reconcileResult, isNotNull);

      final syncedDbBookmarks = await db.getBookmarksForFile(pdfPath);
      expect(
        syncedDbBookmarks.any((b) => b.title == 'خاتمة البحث والتوصيات'),
        isTrue,
      );
    },
  );
}
