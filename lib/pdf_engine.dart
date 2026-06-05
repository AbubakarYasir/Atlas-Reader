import 'dart:io';
import 'dart:convert';

import 'package:syncfusion_flutter_pdf/pdf.dart';

class PdfOverwriteException implements Exception {
  PdfOverwriteException(this.message);

  final String message;

  @override
  String toString() => message;
}

class PdfEngine {
  /// Get the page count of a PDF file
  Future<int?> getPageCount(String filePath) async {
    PdfDocument? document;
    try {
      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);
      return document.pages.count;
    } catch (e) {
      print('[PDF] Error reading page count: $e');
      return null;
    } finally {
      document?.dispose();
    }
  }

  Future<String?> injectBookmark(
    String filePath,
    String bookmarkTitle,
    int pageIndex,
  ) async {
    PdfDocument? document;
    try {
      print('[PDF] Starting bookmark injection for: $bookmarkTitle');
      print('[PDF] File: $filePath, Page: $pageIndex');
      
      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      // Verify page index is valid
      if (pageIndex >= document.pages.count) {
        throw Exception('Page index $pageIndex exceeds document pages (${document.pages.count})');
      }

      // Add bookmark with Unicode support
      try {
        final bookmark = document.bookmarks.add(bookmarkTitle);
        bookmark.destination = PdfDestination(document.pages[pageIndex]);
        print('[PDF] Bookmark added successfully: $bookmarkTitle');
        print('[PDF] Bookmarks count in PDF: ${document.bookmarks.count}');
      } catch (e) {
        print('[PDF] Error adding bookmark: $e');
        print('[PDF] Attempted title (bytes): ${utf8.encode(bookmarkTitle)}');
        print('[PDF] Attempted title (length): ${bookmarkTitle.length}');
        rethrow;
      }

      final outBytes = await document.save();
      document.dispose();
      document = null;

      print('[PDF] Document saved, size: ${outBytes.length} bytes');

      await _safeOverwrite(filePath, outBytes);
      print('[PDF] File overwritten successfully');
      return filePath;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      print('[PDF] ERROR in injectBookmark: $e');
      print('[PDF] Stack trace: $stackTrace');
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

  /// Extract all bookmarks from a PDF file
  /// Returns a list of maps with 'title' and 'pageIndex' keys
  static Future<List<Map<String, dynamic>>> extractBookmarks(
    String filePath,
  ) async {
    PdfDocument? document;
    try {
      print('[PDF] Starting bookmark extraction from: $filePath');

      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      final bookmarksList = <Map<String, dynamic>>[];

      // Loop through all bookmarks in the document
      for (int i = 0; i < document.bookmarks.count; i++) {
        final bookmark = document.bookmarks[i];

        try {
          // Get bookmark title
          final title = bookmark.title ?? 'Untitled Bookmark';

          // Try to get the page index from the bookmark's destination
          int pageIndex = -1;
          if (bookmark.destination != null && bookmark.destination!.page != null) {
            // Find the page index using indexOf
            pageIndex = document.pages.indexOf(bookmark.destination!.page!);
          }

          if (pageIndex >= 0) {
            bookmarksList.add({
              'title': title,
              'pageIndex': pageIndex,
            });
            print('[PDF] Extracted bookmark: "$title" at page $pageIndex');
          } else {
            print('[PDF] Skipped bookmark "$title" - destination page not found');
          }
        } catch (e) {
          print('[PDF] Error processing bookmark $i: $e');
          // Continue to next bookmark
          continue;
        }
      }

      print('[PDF] Extraction complete. Found ${bookmarksList.length} bookmarks');
      return bookmarksList;
    } catch (e, stackTrace) {
      print('[PDF] ERROR in extractBookmarks: $e');
      print('[PDF] Stack trace: $stackTrace');
      return [];
    } finally {
      document?.dispose();
    }
  }
}
