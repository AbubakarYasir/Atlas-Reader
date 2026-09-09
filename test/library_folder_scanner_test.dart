import 'dart:io';

import 'package:atlas_poc/features/library/library_folder_scanner.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late Directory tempDir;
  late LibraryFolderScanner scanner;

  setUp(() {
    tempDir = Directory.systemTemp.createTempSync('atlas_library_scan_');
    scanner = LibraryFolderScanner();
  });

  tearDown(() {
    if (tempDir.existsSync()) {
      tempDir.deleteSync(recursive: true);
    }
  });

  test('finds PDF files recursively and ignores non-PDF files', () async {
    File('${tempDir.path}/root.pdf').writeAsStringSync('not a real pdf');
    final nested = Directory('${tempDir.path}/sub');
    nested.createSync();
    File('${nested.path}/chapter.pdf').writeAsStringSync('also not a pdf');
    File('${tempDir.path}/notes.txt').writeAsStringSync('ignored');
    File('${tempDir.path}/image.png').writeAsStringSync('ignored');

    final scanned = await scanner.scanFolder(tempDir.path);

    expect(scanned, hasLength(2));
    final names = scanned.map((pdf) => pdf.fileName).toSet();
    expect(names, {'root.pdf', 'chapter.pdf'});
    expect(scanned.every((pdf) => pdf.filePath.contains(tempDir.path)), isTrue);
  });

  test('reports zero files for a missing folder', () async {
    final scanned = await scanner.scanFolder('${tempDir.path}/does_not_exist');
    expect(scanned, isEmpty);
  });

  test('skips unreadable files without failing the whole scan', () async {
    File('${tempDir.path}/broken.pdf').writeAsStringSync('garbage, not a PDF');

    final scanned = await scanner.scanFolder(tempDir.path);

    // Extraction fails gracefully, so the file is still indexed with 0
    // bookmarks rather than aborting the scan.
    expect(scanned, hasLength(1));
    expect(scanned.single.bookmarkCount, 0);
  });
}
