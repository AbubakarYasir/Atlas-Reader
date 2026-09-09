import 'dart:io';

import 'package:syncfusion_flutter_pdf/pdf.dart';

import 'pdf_engine.dart';

/// Writes PDF bytes to disk using a temporary file, validation, and atomic replace.
class PdfSafeFileWriter {
  PdfSafeFileWriter._();

  static const _pdfHeader = '%PDF';
  static const _minPdfBytes = 64;

  /// Validates [bytes], writes to [originalPath].tmp, then replaces the original.
  ///
  /// The original is first moved to a sibling recovery file and is restored if
  /// the temporary file cannot be promoted. This prevents a failed replacement
  /// from ever leaving the user without their source document.
  ///
  /// When [expectedPageCount] is supplied, the temporary PDF must have exactly
  /// that number of pages before it can replace the original.
  static Future<void> replacePdfFile(
    String originalPath,
    List<int> bytes, {
    int? expectedPageCount,
  }) async {
    _validatePdfBytes(bytes, expectedPageCount: expectedPageCount);

    final tmpPath = '$originalPath.tmp';
    final backupPath = '$originalPath.atlas-backup';
    final originalFile = File(originalPath);
    final tmpFile = File(tmpPath);
    final backupFile = File(backupPath);

    if (await tmpFile.exists()) {
      await tmpFile.delete();
    }

    if (!await originalFile.exists()) {
      throw PdfOverwriteException(
        'The original PDF could not be found, so no changes were made.',
      );
    }

    if (await backupFile.exists()) {
      throw PdfOverwriteException(
        'A previous PDF recovery file already exists at "$backupPath". '
        'Your original file was not changed. Please recover or rename that '
        'file before trying again.',
      );
    }

    try {
      await tmpFile.writeAsBytes(bytes, flush: true);
      await _validateWrittenFile(tmpPath, expectedPageCount: expectedPageCount);

      // Windows does not reliably rename over an existing file. Move the
      // original aside first, then restore it if promotion fails.
      await originalFile.rename(backupPath);
      await tmpFile.rename(originalPath);
      await backupFile.delete();
    } on PdfOverwriteException {
      await _cleanupTempFile(tmpFile);
      rethrow;
    } catch (_) {
      await _restoreOriginalIfNeeded(originalFile, backupFile);
      await _cleanupTempFile(tmpFile);
      throw PdfOverwriteException(
        'Could not safely replace the PDF. Your original file was restored '
        'when possible. Close the file in other applications and try again.',
      );
    }
  }

  static void _validatePdfBytes(List<int> bytes, {int? expectedPageCount}) {
    if (bytes.isEmpty) {
      throw PdfOverwriteException(
        'The updated PDF could not be saved because it is empty. '
        'Your original file was not changed.',
      );
    }

    if (bytes.length < _minPdfBytes) {
      throw PdfOverwriteException(
        'The updated PDF could not be saved because the file data is too small. '
        'Your original file was not changed.',
      );
    }

    final header = String.fromCharCodes(bytes.take(_pdfHeader.length));
    if (header != _pdfHeader) {
      throw PdfOverwriteException(
        'The updated PDF could not be saved because the file data is invalid. '
        'Your original file was not changed.',
      );
    }

    PdfDocument? document;
    try {
      document = PdfDocument(inputBytes: bytes);
      if (expectedPageCount != null &&
          document.pages.count != expectedPageCount) {
        throw PdfOverwriteException(
          'The updated PDF could not be saved because its page count changed '
          'during validation. Your original file was not changed.',
        );
      }
    } on PdfOverwriteException {
      rethrow;
    } catch (_) {
      throw PdfOverwriteException(
        'The updated PDF could not be saved because it failed validation. '
        'Your original file was not changed.',
      );
    } finally {
      document?.dispose();
    }
  }

  static Future<void> _validateWrittenFile(
    String tmpPath, {
    int? expectedPageCount,
  }) async {
    final written = await File(tmpPath).readAsBytes();
    _validatePdfBytes(written, expectedPageCount: expectedPageCount);
  }

  static Future<void> _cleanupTempFile(File tmpFile) async {
    if (await tmpFile.exists()) {
      await tmpFile.delete();
    }
  }

  static Future<void> _restoreOriginalIfNeeded(
    File originalFile,
    File backupFile,
  ) async {
    if (!await originalFile.exists() && await backupFile.exists()) {
      await backupFile.rename(originalFile.path);
    }
  }
}
