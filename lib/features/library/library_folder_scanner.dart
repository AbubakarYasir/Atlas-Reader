import 'dart:io';
import 'dart:isolate';

import 'package:logging/logging.dart';

import '../../bookmark_grouping.dart';
import '../../core/covers/cover_cache_manager.dart';
import '../../core/epub/epub_engine.dart';
import '../../core/file_system/document_file_system.dart';
import '../../core/file_system/windows_document_file_system.dart';
import '../../pdf_engine.dart';
import '../../scanned_pdf.dart';

/// Recursively finds PDF and EPUB files under a library folder and indexes their
/// metadata, covers, and outlines off the main isolate.
class LibraryFolderScanner {
  LibraryFolderScanner({DocumentFileSystem? fileSystem})
    : _fileSystem = fileSystem ?? const WindowsDocumentFileSystem();

  static final _log = Logger('LibraryFolderScanner');
  final DocumentFileSystem _fileSystem;
  late final PdfEngine _pdfEngine = PdfEngine(fileSystem: _fileSystem);
  static const _epubEngine = EpubEngine();

  /// Walks [folderPath] for supported book files and extracts metadata.
  Future<List<ScannedPdf>> scanFolder(String folderPath) async {
    final paths = await _findDocumentPaths(folderPath);
    final results = <ScannedPdf>[];

    for (final path in paths) {
      final scanned = await _scanOne(path);
      if (scanned != null) {
        results.add(scanned);
      }
    }

    _log.info('[SCAN] $folderPath returned ${results.length} document files');
    return results;
  }

  /// Lists every `.pdf` and `.epub` file under [folderPath] in a background isolate.
  Future<List<String>> _findDocumentPaths(String folderPath) async {
    return Isolate.run(() {
      final directory = Directory(folderPath);
      if (!directory.existsSync()) return const <String>[];

      final docs = <String>[];
      for (final entity in directory.listSync(
        recursive: true,
        followLinks: false,
      )) {
        if (entity is File) {
          final p = entity.path.toLowerCase();
          if (p.endsWith('.pdf') || p.endsWith('.epub')) {
            docs.add(entity.path);
          }
        }
      }
      docs.sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));
      return docs;
    });
  }

  Future<ScannedPdf?> _scanOne(String filePath) async {
    try {
      final stat = await FileStat.stat(filePath);
      final isEpub = filePath.toLowerCase().endsWith('.epub');

      if (isEpub) {
        EpubMetadata? meta;
        String? coverPath;
        try {
          meta = await _epubEngine.extractMetadata(filePath);
          coverPath = await CoverCacheManager.instance.extractAndCacheCover(filePath);
        } catch (_) {}
        return ScannedPdf(
          filePath: filePath,
          fileName: BookmarkGrouping.fileNameFromPath(filePath),
          bookmarkCount: meta?.chapters.length ?? 0,
          lastModified: stat.modified,
          title: meta?.title,
          author: meta?.author,
          format: 'EPUB',
          pageCount: meta?.chapters.length ?? 1,
          fileSizeBytes: stat.size,
          coverPath: coverPath,
        );
      }

      var bookmarkCount = 0;
      try {
        final extracted = await _pdfEngine.extractBookmarks(filePath);
        bookmarkCount = extracted.length;
      } catch (_) {}

      ({int pageCount, String? title, String? author})? meta;
      try {
        meta = await _pdfEngine.getDocumentMetadata(filePath);
      } catch (_) {}

      return ScannedPdf(
        filePath: filePath,
        fileName: BookmarkGrouping.fileNameFromPath(filePath),
        bookmarkCount: bookmarkCount,
        lastModified: stat.modified,
        title: meta?.title,
        author: meta?.author,
        format: 'PDF',
        pageCount: meta?.pageCount ?? 0,
        fileSizeBytes: stat.size,
      );
    } catch (e) {
      _log.warning('[SCAN] Skipping unreadable document $filePath: $e');
      return null;
    }
  }
}
