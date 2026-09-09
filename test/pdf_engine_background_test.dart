import 'dart:io';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/pdf_engine.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:syncfusion_flutter_pdf/pdf.dart';

void main() {
  test(
    'PDF engine reads, extracts, and writes bookmarks off the UI isolate',
    () async {
      final directory = await Directory.systemTemp.createTemp(
        'atlas_pdf_engine_',
      );
      final path = '${directory.path}${Platform.pathSeparator}sample.pdf';
      final fileSystem = const WindowsDocumentFileSystem();
      final engine = PdfEngine(fileSystem: fileSystem);
      final document = PdfDocument();
      final page = document.pages.add();
      page.graphics.drawString(
        'Atlas Reader page text',
        PdfStandardFont(PdfFontFamily.helvetica, 12),
      );
      final originalBytes = document.saveSync();
      document.dispose();

      addTearDown(() => directory.delete(recursive: true));
      await fileSystem.writeAsBytesInBackground(
        path,
        originalBytes,
        flush: true,
      );

      expect(await engine.getPageCount(path), 1);
      expect(await engine.extractBookmarks(path), isEmpty);
      expect(await engine.extractPageText(path, 0), contains('Atlas Reader'));

      expect(await engine.injectBookmark(path, 'Important page', 0), path);
      final bookmarks = await engine.extractBookmarks(path);
      expect(bookmarks, hasLength(1));
      expect(bookmarks.single['title'], 'Important page');
      expect(bookmarks.single['pageIndex'], 0);
    },
  );
}
