import 'dart:isolate';
import 'dart:typed_data';
import 'dart:ui' show Rect;
import 'package:logging/logging.dart';
import 'package:pdf_document/pdf_document.dart' as native_pdf;

import 'package:syncfusion_flutter_pdf/pdf.dart';

import 'bookmark_tree.dart';
import 'core/file_system/document_file_system.dart';
import 'core/file_system/windows_document_file_system.dart';
import 'database.dart';
import 'pdf_safe_file_writer.dart';

class PdfOverwriteException implements Exception {
  PdfOverwriteException(this.message);

  final String message;

  @override
  String toString() => message;
}

/// A compact, searchable SQLite projection of one standard PDF /Ink annotation.
/// The PDF remains authoritative; these rows are rebuilt after each save.
class IndexedInkAnnotation {
  const IndexedInkAnnotation({
    required this.pageNumber,
    required this.type,
    required this.annotationName,
    required this.colorHex,
    required this.left,
    required this.bottom,
    required this.width,
    required this.height,
  });

  final int pageNumber;
  final String type;
  final String? annotationName;
  final String colorHex;
  final double left;
  final double bottom;
  final double width;
  final double height;
}

class _PdfIntegritySnapshot {
  const _PdfIntegritySnapshot({
    required this.pageCount,
    required this.documentInfo,
    required this.hasXmpMetadata,
    required this.outlineKeys,
  });

  final int pageCount;
  final Map<String, String> documentInfo;
  final bool hasXmpMetadata;
  final List<String> outlineKeys;
}

class PdfEngine {
  PdfEngine({DocumentFileSystem? fileSystem})
    : _fileSystem = fileSystem ?? const WindowsDocumentFileSystem(),
      _safeFileWriter = PdfSafeFileWriter(
        fileSystem ?? const WindowsDocumentFileSystem(),
      );

  static final _log = Logger('PdfEngine');
  final DocumentFileSystem _fileSystem;
  final PdfSafeFileWriter _safeFileWriter;

  /// Validates and persists a byte-backed editor revision without keeping an
  /// operating-system file handle open. The incremental editor works entirely
  /// from memory, so by the time the Windows rename starts both readers have
  /// released the source file. Existing page count, nested outline structure,
  /// document information, and XMP-metadata presence must survive unchanged.
  Future<void> saveEditedPdfRevision(
    String filePath,
    Uint8List editedBytes,
  ) async {
    final originalBytes = await _fileSystem.readAsBytesInBackground(filePath);
    final snapshots = await Future.wait([
      Isolate.run(
        () => _readIntegritySnapshot(Uint8List.fromList(originalBytes)),
      ),
      Isolate.run(() => _readIntegritySnapshot(editedBytes)),
    ]);
    final before = snapshots[0];
    final after = snapshots[1];

    if (before.pageCount != after.pageCount) {
      throw PdfOverwriteException(
        'The edited PDF changed its page count, so the original was left untouched.',
      );
    }
    if (!_sameStrings(before.outlineKeys, after.outlineKeys)) {
      throw PdfOverwriteException(
        'The edited PDF did not preserve its nested chapter outline, so the original was left untouched.',
      );
    }
    if (!_sameStringMap(before.documentInfo, after.documentInfo) ||
        before.hasXmpMetadata != after.hasXmpMetadata) {
      throw PdfOverwriteException(
        'The edited PDF did not preserve its document metadata, so the original was left untouched.',
      );
    }

    await _safeFileWriter.replacePdfFile(
      filePath,
      editedBytes,
      expectedPageCount: before.pageCount,
    );
  }

  /// Reads every standard /Ink annotation from [bytes]. Coordinates remain in
  /// native PDF user space (72 points per inch, origin at bottom-left).
  Future<List<IndexedInkAnnotation>> extractInkAnnotationIndex(
    Uint8List bytes,
  ) async {
    final rows = await Isolate.run(() => _extractInkRows(bytes));
    return rows
        .map(
          (row) => IndexedInkAnnotation(
            pageNumber: row['pageNumber']! as int,
            type: row['type']! as String,
            annotationName: row['annotationName'] as String?,
            colorHex: row['colorHex']! as String,
            left: row['left']! as double,
            bottom: row['bottom']! as double,
            width: row['width']! as double,
            height: row['height']! as double,
          ),
        )
        .toList(growable: false);
  }

  /// Get the page count of a PDF file
  Future<int?> getPageCount(String filePath) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      return Isolate.run(() => _pageCountFromBytes(bytes));
    } catch (e) {
      _log.warning('[PDF] Error reading page count: $e');
      return null;
    }
  }

  /// Get page count and metadata (title, author) of a PDF file
  Future<({int pageCount, String? title, String? author})?> getDocumentMetadata(
    String filePath,
  ) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      return Isolate.run(() {
        final document = PdfDocument(inputBytes: bytes);
        try {
          final count = document.pages.count;
          final t = document.documentInformation.title.trim();
          final a = document.documentInformation.author.trim();
          return (
            pageCount: count,
            title: t.isEmpty ? null : t,
            author: a.isEmpty ? null : a,
          );
        } finally {
          document.dispose();
        }
      });
    } catch (e) {
      _log.warning('[PDF] Error reading document metadata: $e');
      return null;
    }
  }

  /// Reads library metadata and outline count in one file read and one parse.
  /// Folder scanning uses this instead of reopening every PDF for each field.
  Future<({int pageCount, int bookmarkCount, String? title, String? author})?>
  inspectForLibrary(String filePath) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      return Isolate.run(() {
        final document = PdfDocument(inputBytes: bytes);
        try {
          final title = document.documentInformation.title.trim();
          final author = document.documentInformation.author.trim();
          return (
            pageCount: document.pages.count,
            bookmarkCount: _countBookmarkBase(document.bookmarks),
            title: title.isEmpty ? null : title,
            author: author.isEmpty ? null : author,
          );
        } finally {
          document.dispose();
        }
      });
    } catch (error) {
      _log.warning('[PDF] Error inspecting library document: $error');
      return null;
    }
  }

  Future<String?> injectBookmark(
    String filePath,
    String bookmarkTitle,
    int pageIndex, {
    String? description,
  }) async {
    try {
      _log.info('[PDF] Starting bookmark injection for: $bookmarkTitle');
      _log.fine('[PDF] File: $filePath, Page: $pageIndex');

      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      final generated = await Isolate.run(
        () => _injectBookmarkIntoBytes(bytes, bookmarkTitle, pageIndex),
      );
      final outBytes = List<int>.from(generated['bytes']! as List);
      final documentPageCount = generated['pageCount']! as int;
      _log.info('[PDF] Bookmark added successfully: $bookmarkTitle');

      // Update descriptions in custom property if description is provided
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // TODO: Implement alternative metadata storage for descriptions
      if (description != null) {
        _log.fine(
          "[PDF] Description provided but not saved (Syncfusion doesn't support customProperties)",
        );
      }

      _log.fine('[PDF] Document saved, size: ${outBytes.length} bytes');

      await _safeFileWriter.replacePdfFile(
        filePath,
        outBytes,
        expectedPageCount: documentPageCount,
      );
      _log.info('[PDF] File overwritten successfully');
      return filePath;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in injectBookmark: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return null;
    }
  }

  /// Extract all bookmarks from a PDF file, including nested sub-bookmarks.
  /// Each entry includes a hierarchical [path], [title], optional [pageIndex],
  /// [isFolder], [description], and [tags].
  Future<List<Map<String, dynamic>>> extractBookmarks(String filePath) async {
    try {
      _log.info('[PDF] Starting bookmark extraction from: $filePath');

      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      final bookmarksList = await Isolate.run(
        () => _extractBookmarksFromBytes(bytes),
      );

      _log.info(
        '[PDF] Extraction complete. Found ${bookmarksList.length} bookmarks',
      );
      return bookmarksList;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in extractBookmarks: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return [];
    }
  }

  /// Extracts selectable text from one PDF page without blocking the UI.
  /// Returns an empty string for scanned or image-only pages.
  Future<String> extractPageText(String filePath, int pageIndex) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      return Isolate.run(() => _extractPageTextFromBytes(bytes, pageIndex));
    } catch (e, stackTrace) {
      _log.warning('[PDF] Error extracting page text: $e', stackTrace);
      return '';
    }
  }

  static int _pageCountFromBytes(List<int> bytes) {
    final document = PdfDocument(inputBytes: bytes);
    try {
      return document.pages.count;
    } finally {
      document.dispose();
    }
  }

  static int _countBookmarkBase(PdfBookmarkBase base) {
    var count = 0;
    for (var index = 0; index < base.count; index++) {
      count += 1 + _countBookmarkBase(base[index]);
    }
    return count;
  }

  static String _extractPageTextFromBytes(List<int> bytes, int pageIndex) {
    final document = PdfDocument(inputBytes: bytes);
    try {
      if (pageIndex < 0 || pageIndex >= document.pages.count) return '';
      return PdfTextExtractor(
        document,
      ).extractText(startPageIndex: pageIndex, endPageIndex: pageIndex);
    } finally {
      document.dispose();
    }
  }

  static Map<String, Object> _injectBookmarkIntoBytes(
    List<int> bytes,
    String bookmarkTitle,
    int pageIndex,
  ) {
    final document = PdfDocument(inputBytes: bytes);
    try {
      final pageCount = document.pages.count;
      if (pageIndex < 0 || pageIndex >= pageCount) {
        throw StateError(
          'Page index $pageIndex exceeds document pages ($pageCount)',
        );
      }
      final bookmark = document.bookmarks.add(bookmarkTitle);
      bookmark.destination = PdfDestination(document.pages[pageIndex]);
      return {'bytes': document.saveSync(), 'pageCount': pageCount};
    } finally {
      document.dispose();
    }
  }

  static List<Map<String, dynamic>> _extractBookmarksFromBytes(
    List<int> bytes,
  ) {
    final document = PdfDocument(inputBytes: bytes);
    try {
      final bookmarksList = <Map<String, dynamic>>[];
      for (var index = 0; index < document.bookmarks.count; index++) {
        _collectBookmarkSubtree(
          document.bookmarks[index],
          document,
          bookmarksList,
          const [],
        );
      }
      return bookmarksList;
    } finally {
      document.dispose();
    }
  }

  static void _collectBookmarkSubtree(
    PdfBookmark bookmark,
    PdfDocument document,
    List<Map<String, dynamic>> out,
    List<String> pathPrefix,
  ) {
    try {
      final rawTitle = bookmark.title;
      final parsed = _parseBookmarkTitle(rawTitle);
      final cleanTitle = parsed.title;
      final tags = parsed.tags;
      final path = [...pathPrefix, cleanTitle];

      int? pageIndex;
      final page = bookmark.destination?.page;
      if (page != null) {
        final index = document.pages.indexOf(page);
        if (index >= 0) {
          pageIndex = index;
        }
      }

      final hasChildren = bookmark.count > 0;
      final isFolder = hasChildren && pageIndex == null;

      out.add({
        'path': path,
        'title': cleanTitle,
        'pageIndex': pageIndex,
        'isFolder': isFolder,
        'description': null,
        'tags': tags,
      });

      _log.fine(
        '[PDF] Extracted bookmark: "${BookmarkTree.displayPath(path)}" '
        '${pageIndex != null ? 'at page ${pageIndex + 1}' : '(folder)'}',
      );

      for (int i = 0; i < bookmark.count; i++) {
        _collectBookmarkSubtree(bookmark[i], document, out, path);
      }
    } catch (e) {
      _log.warning('[PDF] Error processing bookmark "${bookmark.title}": $e');
    }
  }

  static ({String title, List<String> tags}) _parseBookmarkTitle(
    String rawTitle,
  ) {
    final tagPattern = RegExp(r'\s-\s*#([^\s,]+(?:,\s*#[^\s,]+)*)$');
    final match = tagPattern.firstMatch(rawTitle);

    if (match == null) {
      return (title: rawTitle, tags: const []);
    }

    final cleanTitle = rawTitle.substring(0, match.start);
    final tagString = match.group(1) ?? '';
    final tags = tagString.split(RegExp(r',\s*')).map((t) => t.trim()).toList();
    return (title: cleanTitle, tags: tags);
  }

  static String _titleWithTags(String title, List<String> tags) {
    if (tags.isEmpty) return title;
    final tagString = tags.map((tag) => '#$tag').join(', ');
    return '$title - $tagString';
  }

  /// Replaces all PDF bookmarks with the hierarchical tree from the database.
  Future<bool> overwriteBookmarkTree(
    String filePath,
    List<Bookmark> dbBookmarks, {
    Map<int, List<String>> tagsByBookmarkId = const {},
  }) async {
    PdfDocument? document;
    try {
      _log.info(
        '[PDF] Starting hierarchical bookmark overwrite for: $filePath',
      );

      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      document = PdfDocument(inputBytes: bytes);
      document.bookmarks.clear();

      final forest = BookmarkTree.buildForest(dbBookmarks);
      final pageCount = document.pages.count;

      for (final root in forest) {
        _writeBookmarkNode(
          document.bookmarks,
          document,
          root,
          tagsByBookmarkId,
          pageCount,
        );
      }

      final outBytes = await document.save();
      document.dispose();
      document = null;

      await _safeFileWriter.replacePdfFile(
        filePath,
        outBytes,
        expectedPageCount: pageCount,
      );
      _log.info('[PDF] Hierarchical bookmark tree written successfully');
      return true;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in overwriteBookmarkTree: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return false;
    } finally {
      document?.dispose();
    }
  }

  static void _writeBookmarkNode(
    PdfBookmarkBase parent,
    PdfDocument document,
    BookmarkTreeNode node,
    Map<int, List<String>> tagsByBookmarkId,
    int pageCount,
  ) {
    final bookmark = node.bookmark;
    final tags = tagsByBookmarkId[bookmark.id] ?? const [];
    final title = _titleWithTags(bookmark.title, tags);
    final pdfBookmark = parent.add(title);

    if (!bookmark.isFolder && bookmark.pageIndex != null) {
      final safePageIndex = bookmark.pageIndex! >= pageCount
          ? pageCount - 1
          : bookmark.pageIndex!;
      pdfBookmark.destination = PdfDestination(document.pages[safePageIndex]);
    }

    for (final child in node.children) {
      _writeBookmarkNode(
        pdfBookmark,
        document,
        child,
        tagsByBookmarkId,
        pageCount,
      );
    }
  }

  /// Inject multiple bookmarks into a PDF file in a single operation
  /// Returns true if successful, false if it fails
  Future<bool> injectBookmarksBatch(
    String filePath,
    List<Map<String, dynamic>> newBookmarks,
  ) async {
    PdfDocument? document;
    try {
      _log.info('[PDF] Starting batch bookmark injection for: $filePath');
      _log.fine('[PDF] Number of bookmarks to inject: ${newBookmarks.length}');

      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      document = PdfDocument(inputBytes: bytes);

      final pageCount = document.pages.count;
      int addedCount = 0;

      // Loop through newBookmarks and add each one
      for (final bookmarkData in newBookmarks) {
        final title = bookmarkData['title'] as String;
        final pageIndex = bookmarkData['pageIndex'] as int;

        // Handle out-of-bounds pages by defaulting to the last page
        final safePageIndex = pageIndex >= pageCount
            ? pageCount - 1
            : pageIndex;

        try {
          final bookmark = document.bookmarks.add(title);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          _log.fine('[PDF] Added bookmark: "$title" at page $safePageIndex');
        } catch (e) {
          _log.warning('[PDF] Error adding bookmark "$title": $e');
          // Continue to next bookmark
          continue;
        }
      }

      _log.info(
        '[PDF] Successfully added $addedCount out of ${newBookmarks.length} bookmarks',
      );

      final outBytes = await document.save();
      document.dispose();
      document = null;

      await _safeFileWriter.replacePdfFile(
        filePath,
        outBytes,
        expectedPageCount: pageCount,
      );

      _log.info('[PDF] File overwritten successfully');
      return true;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in injectBookmarksBatch: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return false;
    } finally {
      document?.dispose();
    }
  }

  /// Overwrite all bookmarks in a PDF file with a new set of bookmarks
  /// Returns true if successful, false if it fails
  Future<bool> overwriteAllBookmarks(
    String filePath,
    List<Map<String, dynamic>> finalBookmarks, {
    List<String>? toRemove,
    List<String>? toAdd,
    Map<String, String>? descriptions,
  }) async {
    PdfDocument? document;
    try {
      _log.info('[PDF] Starting bookmark overwrite for: $filePath');
      _log.fine('[PDF] Number of bookmarks to set: ${finalBookmarks.length}');

      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      document = PdfDocument(inputBytes: bytes);

      // Clear all existing bookmarks
      _log.fine('[PDF] Clearing existing bookmarks...');
      document.bookmarks.clear();

      final pageCount = document.pages.count;
      int addedCount = 0;

      // Loop through finalBookmarks and add each one
      for (final bookmarkData in finalBookmarks) {
        final title = bookmarkData['title'] as String;
        final pageIndex = bookmarkData['pageIndex'] as int;

        // Handle out-of-bounds pages by defaulting to the last page
        final safePageIndex = pageIndex >= pageCount
            ? pageCount - 1
            : pageIndex;

        try {
          final bookmark = document.bookmarks.add(title);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          _log.fine('[PDF] Added bookmark: "$title" at page $safePageIndex');
        } catch (e) {
          _log.warning('[PDF] Error adding bookmark "$title": $e');
          // Continue to next bookmark
          continue;
        }
      }

      _log.info(
        '[PDF] Successfully added $addedCount out of ${finalBookmarks.length} bookmarks',
      );

      // Write descriptions to custom property if provided
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // TODO: Implement alternative metadata storage for descriptions
      if (descriptions != null && descriptions.isNotEmpty) {
        _log.fine(
          "[PDF] Descriptions provided but not saved (Syncfusion doesn't support customProperties)",
        );
      }

      final outBytes = await document.save();
      document.dispose();
      document = null;

      await _safeFileWriter.replacePdfFile(
        filePath,
        outBytes,
        expectedPageCount: pageCount,
      );

      _log.info('[PDF] File overwritten successfully');

      // Sync History Log Entry
      if (toRemove != null || toAdd != null) {
        final timestamp = DateTime.now().toIso8601String();
        final removedStr = toRemove != null && toRemove.isNotEmpty
            ? 'Removed: [${toRemove.join(", ")}]'
            : 'Removed: []';
        final addedStr = toAdd != null && toAdd.isNotEmpty
            ? 'Added: [${toAdd.join(", ")}]'
            : 'Added: []';
        _log.fine(
          '[SYNC HISTORY] Version pushed at $timestamp: $removedStr and $addedStr',
        );
      }

      return true;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in overwriteAllBookmarks: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return false;
    } finally {
      document?.dispose();
    }
  }

  /// Injects standard PDF highlight, underline, strikethrough, or note annotation.
  /// Saved directly into standard PDF annotation streams so it opens in Adobe Acrobat.
  Future<bool> addAnnotationToPdf(
    String filePath, {
    required int pageIndex,
    required String type,
    required String text,
    String? note,
    String colorHex = '#FFE066',
    Rect? bounds,
  }) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      final outBytes = await Isolate.run(() {
        final doc = PdfDocument(inputBytes: bytes);
        try {
          if (pageIndex < 0 || pageIndex >= doc.pages.count) {
            return null;
          }
          final page = doc.pages[pageIndex];
          final color = _hexToPdfColor(colorHex);
          final rect = bounds ?? const Rect.fromLTWH(50, 50, 200, 20);

          if (type == 'note') {
            final popup = PdfPopupAnnotation(
              rect,
              note ?? text,
              icon: PdfPopupIcon.note,
            );
            page.annotations.add(popup);
          } else {
            PdfTextMarkupAnnotationType markupType;
            switch (type) {
              case 'underline':
                markupType = PdfTextMarkupAnnotationType.underline;
                break;
              case 'strikethrough':
                markupType = PdfTextMarkupAnnotationType.strikethrough;
                break;
              case 'highlight':
              default:
                markupType = PdfTextMarkupAnnotationType.highlight;
                break;
            }

            final markup = PdfTextMarkupAnnotation(
              rect,
              text,
              color,
              textMarkupAnnotationType: markupType,
              subject: note,
            );
            page.annotations.add(markup);
          }

          final saved = doc.saveSync();
          return saved;
        } finally {
          doc.dispose();
        }
      });

      if (outBytes == null) return false;

      await _safeFileWriter.replacePdfFile(filePath, outBytes);
      return true;
    } catch (e, st) {
      _log.warning('[PDF] Failed to add annotation to PDF: $e\n$st');
      return false;
    }
  }

  /// Extracts standard PDF text markup and popup annotations from PDF stream
  Future<List<Map<String, dynamic>>> extractAnnotationsFromPdf(
    String filePath,
  ) async {
    try {
      final bytes = await _fileSystem.readAsBytesInBackground(filePath);
      return Isolate.run(() {
        final doc = PdfDocument(inputBytes: bytes);
        final results = <Map<String, dynamic>>[];
        try {
          for (var p = 0; p < doc.pages.count; p++) {
            final page = doc.pages[p];
            for (var a = 0; a < page.annotations.count; a++) {
              final annot = page.annotations[a];
              if (annot is PdfTextMarkupAnnotation) {
                results.add({
                  'pageIndex': p,
                  'type': annot.textMarkupAnnotationType.name,
                  'text': annot.text,
                  'note': annot.subject,
                  'bounds': [
                    annot.bounds.left,
                    annot.bounds.top,
                    annot.bounds.width,
                    annot.bounds.height,
                  ],
                });
              } else if (annot is PdfPopupAnnotation) {
                results.add({
                  'pageIndex': p,
                  'type': 'note',
                  'text': annot.text,
                  'note': annot.text,
                  'bounds': [
                    annot.bounds.left,
                    annot.bounds.top,
                    annot.bounds.width,
                    annot.bounds.height,
                  ],
                });
              }
            }
          }
          return results;
        } finally {
          doc.dispose();
        }
      });
    } catch (e) {
      return [];
    }
  }

  static PdfColor _hexToPdfColor(String hex) {
    var clean = hex.replaceAll('#', '');
    if (clean.length == 6) {
      clean = 'FF$clean';
    }
    final val = int.tryParse(clean, radix: 16) ?? 0xFFFFEB3B;
    final r = (val >> 16) & 0xFF;
    final g = (val >> 8) & 0xFF;
    final b = val & 0xFF;
    return PdfColor(r, g, b);
  }

  static _PdfIntegritySnapshot _readIntegritySnapshot(Uint8List bytes) {
    final document = native_pdf.PdfDocument.open(bytes);
    final outlineKeys = <String>[];
    for (final item in native_pdf.PdfOutline.of(document).items) {
      _collectNativeOutlineKeys(item, const [], outlineKeys);
    }
    return _PdfIntegritySnapshot(
      pageCount: document.pageCount,
      documentInfo: Map<String, String>.from(document.info),
      hasXmpMetadata: document.catalog.containsKey('Metadata'),
      outlineKeys: outlineKeys,
    );
  }

  static void _collectNativeOutlineKeys(
    native_pdf.PdfOutlineItem item,
    List<String> parentPath,
    List<String> output,
  ) {
    final path = [...parentPath, item.title];
    output.add(
      '${path.join('\u001f')}\u001e${item.destination?.pageIndex ?? -1}',
    );
    for (final child in item.children) {
      _collectNativeOutlineKeys(child, path, output);
    }
  }

  static List<Map<String, Object?>> _extractInkRows(Uint8List bytes) {
    final document = native_pdf.PdfDocument.open(bytes);
    final rows = <Map<String, Object?>>[];
    for (var pageIndex = 0; pageIndex < document.pageCount; pageIndex++) {
      for (final annotation in document.page(pageIndex).annotations) {
        if (annotation.subtype != 'Ink' || annotation.inkList == null) continue;
        final rgb = annotation.color ?? 0x000000;
        final opacity = annotation.appearanceOpacity;
        final width = annotation.borderWidth ?? 1;
        final isHighlighter = opacity < .75 && width >= 4;
        rows.add({
          'pageNumber': pageIndex + 1,
          'type': isHighlighter ? 'ink_highlighter' : 'ink',
          'annotationName': annotation.name,
          'colorHex': '#${rgb.toRadixString(16).padLeft(6, '0').toUpperCase()}',
          'left': annotation.rect.left,
          'bottom': annotation.rect.bottom,
          'width': annotation.rect.width,
          'height': annotation.rect.height,
        });
      }
    }
    return rows;
  }

  static bool _sameStrings(List<String> a, List<String> b) {
    if (a.length != b.length) return false;
    for (var index = 0; index < a.length; index++) {
      if (a[index] != b[index]) return false;
    }
    return true;
  }

  static bool _sameStringMap(Map<String, String> a, Map<String, String> b) {
    if (a.length != b.length) return false;
    for (final entry in a.entries) {
      if (b[entry.key] != entry.value) return false;
    }
    return true;
  }
}
