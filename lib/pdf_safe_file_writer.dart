import 'package:syncfusion_flutter_pdf/pdf.dart';

import 'core/file_system/document_file_system.dart';
import 'pdf_engine.dart';

/// Writes PDF bytes to disk using a temporary file, validation, and atomic replace.
class PdfSafeFileWriter {
  PdfSafeFileWriter(this._fileSystem);

  final DocumentFileSystem _fileSystem;

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
  Future<void> replacePdfFile(
    String originalPath,
    List<int> bytes, {
    int? expectedPageCount,
  }) async {
    _validatePdfBytes(bytes, expectedPageCount: expectedPageCount);

    final tmpPath = '$originalPath.tmp';
    final backupPath = '$originalPath.atlas-backup';
    if (await _fileSystem.exists(tmpPath)) {
      await _fileSystem.delete(tmpPath);
    }

    if (!await _fileSystem.exists(originalPath)) {
      throw PdfOverwriteException(
        'The original PDF could not be found, so no changes were made.',
      );
    }

    if (await _fileSystem.exists(backupPath)) {
      throw PdfOverwriteException(
        'A previous PDF recovery file already exists at "$backupPath". '
        'Your original file was not changed. Please recover or rename that '
        'file before trying again.',
      );
    }

    try {
      await _fileSystem.writeAsBytes(tmpPath, bytes, flush: true);
      await _validateWrittenFile(tmpPath, expectedPageCount: expectedPageCount);

      // Windows does not reliably rename over an existing file. Move the
      // original aside first, then restore it if promotion fails.
      await _fileSystem.rename(originalPath, backupPath);
      await _fileSystem.rename(tmpPath, originalPath);
      await _fileSystem.delete(backupPath);
    } on PdfOverwriteException {
      await _cleanupTempFile(tmpPath);
      rethrow;
    } catch (_) {
      await _restoreOriginalIfNeeded(originalPath, backupPath);
      await _cleanupTempFile(tmpPath);
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

  Future<void> _validateWrittenFile(
    String tmpPath, {
    int? expectedPageCount,
  }) async {
    final written = await _fileSystem.readAsBytes(tmpPath);
    _validatePdfBytes(written, expectedPageCount: expectedPageCount);
  }

  Future<void> _cleanupTempFile(String tmpPath) async {
    if (await _fileSystem.exists(tmpPath)) {
      await _fileSystem.delete(tmpPath);
    }
  }

  Future<void> _restoreOriginalIfNeeded(
    String originalPath,
    String backupPath,
  ) async {
    if (!await _fileSystem.exists(originalPath) &&
        await _fileSystem.exists(backupPath)) {
      await _fileSystem.rename(backupPath, originalPath);
    }
  }
}
