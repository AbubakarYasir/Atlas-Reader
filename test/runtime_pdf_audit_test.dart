import 'dart:io';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/pdf_engine.dart';
import 'package:flutter_test/flutter_test.dart';

/// Opt-in smoke coverage for locally available, real documents. The paths are
/// supplied through environment variables so no personal file location enters
/// source control. This test only reads PDFs; it never modifies either file.
void main() {
  final arabicPath = Platform.environment['ATLAS_AUDIT_ARABIC_PDF'] ?? '';
  final englishPath = Platform.environment['ATLAS_AUDIT_ENGLISH_PDF'] ?? '';
  final skipArabic = arabicPath.isEmpty
      ? 'Set ATLAS_AUDIT_ARABIC_PDF to run the real-file audit.'
      : false;
  final skipEnglish = englishPath.isEmpty
      ? 'Set ATLAS_AUDIT_ENGLISH_PDF to run the real-file audit.'
      : false;

  test(
    'runtime audit reads a real Arabic nested-outline PDF without changing it',
    () async {
      expect(await File(arabicPath).exists(), isTrue);
      final engine = PdfEngine(fileSystem: const WindowsDocumentFileSystem());

      final pageCount = await engine.getPageCount(arabicPath);
      final outline = await engine.extractBookmarks(arabicPath);

      expect(pageCount, greaterThan(0));
      expect(outline, isNotEmpty);
      expect(
        outline.any(
          (bookmark) => RegExp(
            r'[\u0600-\u06FF]',
          ).hasMatch(bookmark['title'] as String? ?? ''),
        ),
        isTrue,
        reason: 'the selected fixture should prove Arabic outline decoding',
      );
    },
    skip: skipArabic,
  );

  test(
    'runtime audit reads a real English PDF without changing it',
    () async {
      expect(await File(englishPath).exists(), isTrue);
      final engine = PdfEngine(fileSystem: const WindowsDocumentFileSystem());

      expect(await engine.getPageCount(englishPath), greaterThan(0));
    },
    skip: skipEnglish,
  );
}
