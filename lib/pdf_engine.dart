import 'dart:io';

import 'package:syncfusion_flutter_pdf/pdf.dart';

class PdfOverwriteException implements Exception {
  PdfOverwriteException(this.message);

  final String message;

  @override
  String toString() => message;
}

class PdfEngine {
  Future<String?> injectBookmark(
    String filePath,
    String bookmarkTitle,
    int pageIndex,
  ) async {
    PdfDocument? document;
    try {
      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      final bookmark = document.bookmarks.add(bookmarkTitle);
      bookmark.destination = PdfDestination(document.pages[pageIndex]);

      final outBytes = await document.save();
      document.dispose();
      document = null;

      await _safeOverwrite(filePath, outBytes);
      return filePath;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      print('PdfEngine.injectBookmark error: $e');
      print(stackTrace);
      return null;
    } finally {
      document?.dispose();
    }
  }

  Future<void> _safeOverwrite(String originalPath, List<int> outBytes) async {
    final tmpPath = '$originalPath.tmp';
    final originalFile = File(originalPath);
    final tmpFile = File(tmpPath);

    await tmpFile.writeAsBytes(outBytes);

    try {
      await originalFile.delete();
      await tmpFile.rename(originalPath);
    } catch (_) {
      if (await tmpFile.exists()) {
        await tmpFile.delete();
      }
      throw PdfOverwriteException(
        'Could not replace the PDF because it is open in another application. '
        'Close the file and try again.',
      );
    }
  }
}
