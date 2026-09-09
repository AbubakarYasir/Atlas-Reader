import 'dart:io';
import 'dart:isolate';

import 'package:logging/logging.dart';

import '../../bookmark_grouping.dart';
import '../../core/file_system/document_file_system.dart';
import '../../core/file_system/windows_document_file_system.dart';
import '../../pdf_engine.dart';
import '../../scanned_pdf.dart';

/// Recursively finds PDF files under a library folder and indexes their
/// outlines (bookmark counts) off the main isolate.
class LibraryFolderScanner {
  LibraryFolderScanner({DocumentFileSystem? fileSystem})
    : _fileSystem = fileSystem ?? const WindowsDocumentFileSystem();

  static final _log = Logger('LibraryFolderScanner');
  final DocumentFileSystem _fileSystem;
  late final PdfEngine _pdfEngine = PdfEngine(fileSystem: _fileSystem);

  /// Walks [folderPath] for `.pdf` files and extracts each outline count.
  /// Unreadable or corrupt files are skipped with a warning.
  Future<List<ScannedPdf>> scanFolder(String folderPath) async {
    final paths = await _findPdfPaths(folderPath);
    final results = <ScannedPdf>[];

    for (final path in paths) {
      final scanned = await _scanOne(path);
      if (scanned != null) {
        results.add(scanned);
      }
    }

    _log.info('[SCAN] $folderPath returned ${results.length} PDF files');
    return results;
  }

  /// Lists every `.pdf` file under [folderPath] in a background isolate.
  Future<List<String>> _findPdfPaths(String folderPath) async {
    return Isolate.run(() {
      final directory = Directory(folderPath);
      if (!directory.existsSync()) return const <String>[];

      final pdfs = <String>[];
      for (final entity in directory.listSync(
        recursive: true,
        followLinks: false,
      )) {
        if (entity is File && entity.path.toLowerCase().endsWith('.pdf')) {
          pdfs.add(entity.path);
        }
      }
      pdfs.sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));
      return pdfs;
    });
  }

  Future<ScannedPdf?> _scanOne(String filePath) async {
    try {
      final stat = await FileStat.stat(filePath);
      final extracted = await _pdfEngine.extractBookmarks(filePath);
      return ScannedPdf(
        filePath: filePath,
        fileName: BookmarkGrouping.fileNameFromPath(filePath),
        bookmarkCount: extracted.length,
        lastModified: stat.modified,
      );
    } catch (e) {
      _log.warning('[SCAN] Skipping unreadable PDF $filePath: $e');
      return null;
    }
  }
}
