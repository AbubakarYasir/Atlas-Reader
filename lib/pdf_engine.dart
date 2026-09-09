import 'dart:convert';
import 'package:logging/logging.dart';

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

class PdfEngine {
  PdfEngine({DocumentFileSystem? fileSystem})
    : _fileSystem = fileSystem ?? const WindowsDocumentFileSystem(),
      _safeFileWriter = PdfSafeFileWriter(
        fileSystem ?? const WindowsDocumentFileSystem(),
      );

  static final _log = Logger('PdfEngine');
  final DocumentFileSystem _fileSystem;
  final PdfSafeFileWriter _safeFileWriter;

  /// Get the page count of a PDF file
  Future<int?> getPageCount(String filePath) async {
    PdfDocument? document;
    try {
      final bytes = await _fileSystem.readAsBytes(filePath);
      document = PdfDocument(inputBytes: bytes);
      return document.pages.count;
    } catch (e) {
      _log.warning('[PDF] Error reading page count: $e');
      return null;
    } finally {
      document?.dispose();
    }
  }

  Future<String?> injectBookmark(
    String filePath,
    String bookmarkTitle,
    int pageIndex, {
    String? description,
  }) async {
    PdfDocument? document;
    try {
      _log.info('[PDF] Starting bookmark injection for: $bookmarkTitle');
      _log.fine('[PDF] File: $filePath, Page: $pageIndex');

      final bytes = await _fileSystem.readAsBytes(filePath);
      document = PdfDocument(inputBytes: bytes);
      final documentPageCount = document.pages.count;

      // Verify page index is valid
      if (pageIndex >= document.pages.count) {
        throw Exception(
          'Page index $pageIndex exceeds document pages (${document.pages.count})',
        );
      }

      // Add bookmark with Unicode support
      try {
        final bookmark = document.bookmarks.add(bookmarkTitle);
        bookmark.destination = PdfDestination(document.pages[pageIndex]);
        _log.info('[PDF] Bookmark added successfully: $bookmarkTitle');
        _log.fine('[PDF] Bookmarks count in PDF: ${document.bookmarks.count}');
      } catch (e) {
        _log.warning('[PDF] Error adding bookmark: $e');
        _log.fine(
          '[PDF] Attempted title (bytes): ${utf8.encode(bookmarkTitle)}',
        );
        _log.fine('[PDF] Attempted title (length): ${bookmarkTitle.length}');
        rethrow;
      }

      // Update descriptions in custom property if description is provided
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // TODO: Implement alternative metadata storage for descriptions
      if (description != null) {
        _log.fine(
          "[PDF] Description provided but not saved (Syncfusion doesn't support customProperties)",
        );
      }

      final outBytes = await document.save();
      document.dispose();
      document = null;

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
    } finally {
      document?.dispose();
    }
  }

  /// Extract all bookmarks from a PDF file, including nested sub-bookmarks.
  /// Each entry includes a hierarchical [path], [title], optional [pageIndex],
  /// [isFolder], [description], and [tags].
  Future<List<Map<String, dynamic>>> extractBookmarks(String filePath) async {
    PdfDocument? document;
    try {
      _log.info('[PDF] Starting bookmark extraction from: $filePath');

      final bytes = await _fileSystem.readAsBytes(filePath);
      document = PdfDocument(inputBytes: bytes);

      final bookmarksList = <Map<String, dynamic>>[];

      for (int i = 0; i < document.bookmarks.count; i++) {
        _collectBookmarkSubtree(
          document.bookmarks[i],
          document,
          bookmarksList,
          const [],
        );
      }

      _log.info(
        '[PDF] Extraction complete. Found ${bookmarksList.length} bookmarks',
      );
      return bookmarksList;
    } catch (e, stackTrace) {
      _log.severe('[PDF] ERROR in extractBookmarks: $e');
      _log.severe('[PDF] Stack trace: $stackTrace');
      return [];
    } finally {
      document?.dispose();
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

      final bytes = await _fileSystem.readAsBytes(filePath);
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

      final bytes = await _fileSystem.readAsBytes(filePath);
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

      final bytes = await _fileSystem.readAsBytes(filePath);
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
}
