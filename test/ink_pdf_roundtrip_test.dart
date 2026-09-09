import 'dart:io';
import 'dart:typed_data';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/pdf_engine.dart';
import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pdf_document/pdf_document.dart' as native_pdf;
import 'package:syncfusion_flutter_pdf/pdf.dart' as syncfusion;

void main() {
  late Directory tempDirectory;
  late String pdfPath;
  late AppDatabase database;
  const fileSystem = WindowsDocumentFileSystem();

  setUp(() async {
    tempDirectory = await Directory.systemTemp.createTemp('atlas_ink_test_');
    pdfPath = '${tempDirectory.path}${Platform.pathSeparator}ink.pdf';
    database = AppDatabase.forTesting(NativeDatabase.memory());

    final document = syncfusion.PdfDocument();
    document.documentInformation
      ..title = 'Atlas Ink Integrity'
      ..author = 'Atlas Reader Tests'
      ..subject = 'Nested outline preservation';
    final firstPage = document.pages.add();
    final secondPage = document.pages.add();
    final root = document.bookmarks.add('المجلد الأول Volume 1');
    root.destination = syncfusion.PdfDestination(firstPage);
    final child = root.add('الفصل 2 Chapter');
    child.destination = syncfusion.PdfDestination(secondPage);
    await File(pdfPath).writeAsBytes(document.saveSync(), flush: true);
    document.dispose();
  });

  tearDown(() async {
    await database.close();
    await tempDirectory.delete(recursive: true);
  });

  test(
    'standard ink keeps PDF coordinates, page isolation, outline, and metadata',
    () async {
      final original = Uint8List.fromList(await File(pdfPath).readAsBytes());
      final parsed = native_pdf.PdfDocument.open(original);
      final editor = native_pdf.PdfEditor(parsed);
      editor.addInk(
        1,
        const [
          [(72.0, 90.0), (144.0, 180.0), (216.0, 270.0)],
        ],
        color: 0x1565C0,
        strokeWidth: 4,
        pressures: const [
          [0.2, 0.6, 1.0],
        ],
        name: 'atlas-page-two-ink',
      );

      final engine = PdfEngine(fileSystem: fileSystem);
      await engine.saveEditedPdfRevision(pdfPath, editor.save());

      final saved = Uint8List.fromList(await File(pdfPath).readAsBytes());
      final reopened = native_pdf.PdfDocument.open(saved);
      expect(
        reopened.page(0).annotations.where((a) => a.subtype == 'Ink'),
        isEmpty,
      );
      final ink = reopened
          .page(1)
          .annotations
          .singleWhere((annotation) => annotation.subtype == 'Ink');
      expect(ink.name, 'atlas-page-two-ink');
      expect(ink.inkList!.single, const [
        (72.0, 90.0),
        (144.0, 180.0),
        (216.0, 270.0),
      ]);

      final external = syncfusion.PdfDocument(inputBytes: saved);
      expect(external.pages.count, 2);
      expect(external.documentInformation.title, 'Atlas Ink Integrity');
      expect(external.documentInformation.author, 'Atlas Reader Tests');
      expect(
        external.documentInformation.subject,
        'Nested outline preservation',
      );
      expect(external.bookmarks.count, 1);
      expect(external.bookmarks[0].title, 'المجلد الأول Volume 1');
      expect(external.bookmarks[0].count, 1);
      expect(external.bookmarks[0][0].title, 'الفصل 2 Chapter');
      external.dispose();
    },
  );

  test(
    'ink annotations rebuild the local SQLite projection without duplicates',
    () async {
      final parsed = native_pdf.PdfDocument.open(
        Uint8List.fromList(await File(pdfPath).readAsBytes()),
      );
      final editor = native_pdf.PdfEditor(parsed)
        ..addInk(
          0,
          const [
            [(20.0, 30.0), (80.0, 90.0)],
          ],
          color: 0xF9A825,
          strokeWidth: 12,
          opacity: .35,
          name: 'highlight-1',
        );
      final bytes = editor.save();
      final engine = PdfEngine(fileSystem: fileSystem);
      final index = await engine.extractInkAnnotationIndex(bytes);

      Future<void> replace() => database.replaceInkAnnotationIndex(
        pdfPath,
        index
            .map(
              (entry) => (
                pageNumber: entry.pageNumber,
                type: entry.type,
                annotationName: entry.annotationName,
                colorHex: entry.colorHex,
                left: entry.left,
                bottom: entry.bottom,
                width: entry.width,
                height: entry.height,
              ),
            )
            .toList(),
      );

      await replace();
      await replace();
      final rows = await database.getAnnotationsForFile(pdfPath);
      expect(rows, hasLength(1));
      expect(rows.single.pageNumber, 1);
      expect(rows.single.type, 'ink_highlighter');
      expect(rows.single.selectedText, 'highlight-1');
      expect(rows.single.rectY, greaterThanOrEqualTo(0));
    },
  );
}
