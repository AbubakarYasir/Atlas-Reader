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
  Future<List<ScannedPdf>> scanFolder(
    String folderPath, {
    Future<void> Function(List<ScannedPdf> batch, int indexed, int total)?
    onBatch,
  }) async {
    final paths = await _findDocumentPaths(folderPath);
    final results = <ScannedPdf>[];

    // Keep disk pressure bounded while allowing metadata parsing and cover
    // extraction to overlap. Six concurrent books is fast on SSDs without
    // starving the reader's renderer or flooding slower drives.
    const batchSize = 6;
    for (var offset = 0; offset < paths.length; offset += batchSize) {
      final end = (offset + batchSize).clamp(0, paths.length);
      final batch = await Future.wait(paths.sublist(offset, end).map(_scanOne));
      final completed = <ScannedPdf>[];
      for (final scanned in batch) {
        if (scanned != null) {
          results.add(scanned);
          completed.add(scanned);
        }
      }
      if (onBatch != null && completed.isNotEmpty) {
        await onBatch(completed, results.length, paths.length);
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
          coverPath = await CoverCacheManager.instance.extractAndCacheCover(
            filePath,
          );
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

      ({int pageCount, int bookmarkCount, String? title, String? author})? meta;
      try {
        meta = await _pdfEngine.inspectForLibrary(filePath);
      } catch (_) {}

      return ScannedPdf(
        filePath: filePath,
        fileName: BookmarkGrouping.fileNameFromPath(filePath),
        bookmarkCount: meta?.bookmarkCount ?? 0,
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
