import 'dart:io';
import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/features/research/citation_generator.dart';
import 'package:atlas_poc/pdf_engine.dart';
import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:syncfusion_flutter_pdf/pdf.dart';

void main() {
  late AppDatabase db;
  late Directory tempDir;
  late String pdfPath;
  late PdfEngine pdfEngine;
  const fileSystem = WindowsDocumentFileSystem();

  setUp(() async {
    db = AppDatabase.forTesting(NativeDatabase.memory());
    tempDir = await Directory.systemTemp.createTemp('atlas_stage9_');
    pdfPath = '${tempDir.path}${Platform.pathSeparator}scholarly_treatise.pdf';
    pdfEngine = PdfEngine(fileSystem: fileSystem);

    final document = PdfDocument();
    document.pages.add();
    document.pages.add();
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
    'Stage 9: Database and PDF engine create, persist, and extract standard PDF annotations',
    () async {
      // 1. Add highlight to DB
      final id1 = await db.addAnnotation(
        filePath: pdfPath,
        pageNumber: 1,
        type: 'highlight',
        selectedText: 'الضروريات الخمس في مقاصد الشريعة',
        note: 'Foundational principle',
        colorHex: '#FFE066',
      );
      expect(id1, isPositive);

      // 2. Add sticky note to DB
      final id2 = await db.addAnnotation(
        filePath: pdfPath,
        pageNumber: 2,
        type: 'note',
        selectedText: 'Ibn Ashur treatise',
        note: 'Compare with Al-Shatibi Muwafaqat',
        colorHex: '#8CE99A',
      );
      expect(id2, isPositive);

      // 3. Query DB annotations
      final allAnnots = await db.getAnnotationsForFile(pdfPath);
      expect(allAnnots, hasLength(2));
      expect(allAnnots.first.selectedText, 'الضروريات الخمس في مقاصد الشريعة');

      // 4. Query page-specific DB annotations
      final page1Annots = await db.getAnnotationsForFile(
        pdfPath,
        pageNumber: 1,
      );
      expect(page1Annots, hasLength(1));

      // 5. Test FTS full-text search on annotations
      final ftsMatches = await db.searchAnnotationsFts('مقاصد');
      expect(ftsMatches, hasLength(1));
      expect(ftsMatches.first.id, id1);

      // 6. Save standard PDF annotations directly into PDF stream
      final successHighlight = await pdfEngine.addAnnotationToPdf(
        pdfPath,
        pageIndex: 0,
        type: 'highlight',
        text: 'الضروريات الخمس في مقاصد الشريعة',
        note: 'Foundational principle',
        colorHex: '#FFE066',
      );
      expect(successHighlight, isTrue);

      final successNote = await pdfEngine.addAnnotationToPdf(
        pdfPath,
        pageIndex: 1,
        type: 'note',
        text: 'Ibn Ashur treatise',
        note: 'Compare with Al-Shatibi Muwafaqat',
      );
      expect(successNote, isTrue);

      // 7. Extract annotations directly from PDF stream (verifying external reader compatibility)
      final extracted = await pdfEngine.extractAnnotationsFromPdf(pdfPath);
      expect(extracted, hasLength(2));
      expect(
        extracted.any((a) => a['text'] == 'الضروريات الخمس في مقاصد الشريعة'),
        isTrue,
      );
      expect(extracted.any((a) => a['type'] == 'note'), isTrue);
    },
  );

  test(
    'Stage 9: Citation generator produces accurate APA, Chicago, MLA, and BibTeX citations',
    () {
      const title = 'The Muqaddimah: An Introduction to History';
      const author = 'Ibn Khaldun';
      const year = '1377';
      const publisher = 'Princeton University Press';
      const page = 42;

      final apa = CitationGenerator.generate(
        format: CitationFormat.apa,
        title: title,
        author: author,
        pageNumber: page,
        year: year,
        publisher: publisher,
      );
      expect(
        apa,
        'Ibn Khaldun. (1377). The Muqaddimah: An Introduction to History (p. 42). Princeton University Press.',
      );

      final chicago = CitationGenerator.generate(
        format: CitationFormat.chicago,
        title: title,
        author: author,
        pageNumber: page,
        year: year,
        publisher: publisher,
      );
      expect(
        chicago,
        'Ibn Khaldun. The Muqaddimah: An Introduction to History. Princeton University Press, 1377, p. 42.',
      );

      final mla = CitationGenerator.generate(
        format: CitationFormat.mla,
        title: title,
        author: author,
        pageNumber: page,
        year: year,
        publisher: publisher,
      );
      expect(
        mla,
        'Ibn Khaldun. The Muqaddimah: An Introduction to History. Princeton University Press, 1377, p. 42.',
      );

      final bibtex = CitationGenerator.generate(
        format: CitationFormat.bibtex,
        title: title,
        author: author,
        pageNumber: page,
        year: year,
        publisher: publisher,
      );
      expect(bibtex, contains('@inbook{IbnKhaldun1377_p42,'));
      expect(bibtex, contains('pages = {42}'));
    },
  );
}
