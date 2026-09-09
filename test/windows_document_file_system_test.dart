import 'dart:io';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test(
    'WindowsDocumentFileSystem reads, moves, and deletes a document file',
    () async {
      final directory = await Directory.systemTemp.createTemp(
        'atlas_file_system_',
      );
      final originalPath =
          '${directory.path}${Platform.pathSeparator}source.pdf';
      final movedPath = '${directory.path}${Platform.pathSeparator}moved.pdf';
      final fileSystem = const WindowsDocumentFileSystem();

      addTearDown(() => directory.delete(recursive: true));

      await fileSystem.writeAsBytes(originalPath, [1, 2, 3], flush: true);
      expect(await fileSystem.exists(originalPath), isTrue);
      expect(await fileSystem.readAsBytes(originalPath), [1, 2, 3]);

      await fileSystem.writeAsBytesInBackground(originalPath, [
        4,
        5,
        6,
      ], flush: true);
      expect(await fileSystem.readAsBytesInBackground(originalPath), [4, 5, 6]);

      await fileSystem.rename(originalPath, movedPath);
      expect(await fileSystem.exists(originalPath), isFalse);
      expect(await fileSystem.exists(movedPath), isTrue);

      await fileSystem.delete(movedPath);
      expect(await fileSystem.exists(movedPath), isFalse);
    },
  );
}
