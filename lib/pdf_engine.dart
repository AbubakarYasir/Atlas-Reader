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
    int pageIndex, {
    String? description,
  }) async {
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

      // Update descriptions in custom property if description is provided
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // TODO: Implement alternative metadata storage for descriptions
      if (description != null) {
        print("[PDF] Description provided but not saved (Syncfusion doesn't support customProperties)");
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
  /// Returns a list of maps with 'title', 'pageIndex', 'description', and 'tags' keys
  static Future<List<Map<String, dynamic>>> extractBookmarks(
    String filePath,
  ) async {
    PdfDocument? document;
    try {
      print('[PDF] Starting bookmark extraction from: $filePath');

      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      final bookmarksList = <Map<String, dynamic>>[];

      // Read custom property for descriptions
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // Descriptions will need to be stored in a separate metadata file or alternative approach
      Map<String, String> descriptionsMap = {};
      // TODO: Implement alternative metadata storage for descriptions

      // Loop through all bookmarks in the document
      for (int i = 0; i < document.bookmarks.count; i++) {
        final bookmark = document.bookmarks[i];

        try {
          // Get bookmark title
          final rawTitle = bookmark.title ?? 'Untitled Bookmark';

          // Parse tags from title (format: "Title - #tag1, #tag2")
          String cleanTitle = rawTitle;
          List<String> tags = [];

          final tagPattern = RegExp(r'\s-\s*#([^\s,]+(?:,\s*#[^\s,]+)*)$');
          final match = tagPattern.firstMatch(rawTitle);

          if (match != null) {
            cleanTitle = rawTitle.substring(0, match.start);
            final tagString = match.group(1) ?? '';
            tags = tagString.split(RegExp(r',\s*')).map((t) => t.trim()).toList();
            print('[PDF] Parsed tags from title: $tags');
          }

          // Try to get the page index from the bookmark's destination
          int pageIndex = -1;
          if (bookmark.destination != null && bookmark.destination!.page != null) {
            // Find the page index using indexOf
            pageIndex = document.pages.indexOf(bookmark.destination!.page!);
          }

          if (pageIndex >= 0) {
            bookmarksList.add({
              'title': cleanTitle,
              'pageIndex': pageIndex,
              'description': descriptionsMap[cleanTitle],
              'tags': tags,
            });
            print('[PDF] Extracted bookmark: "$cleanTitle" at page $pageIndex');
          } else {
            print('[PDF] Skipped bookmark "$cleanTitle" - destination page not found');
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

  /// Inject multiple bookmarks into a PDF file in a single operation
  /// Returns true if successful, false if it fails
  static Future<bool> injectBookmarksBatch(
    String filePath,
    List<Map<String, dynamic>> newBookmarks,
  ) async {
    PdfDocument? document;
    try {
      print('[PDF] Starting batch bookmark injection for: $filePath');
      print('[PDF] Number of bookmarks to inject: ${newBookmarks.length}');

      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      final pageCount = document.pages.count;
      int addedCount = 0;

      // Loop through newBookmarks and add each one
      for (final bookmarkData in newBookmarks) {
        final title = bookmarkData['title'] as String;
        final pageIndex = bookmarkData['pageIndex'] as int;

        // Handle out-of-bounds pages by defaulting to the last page
        final safePageIndex = pageIndex >= pageCount ? pageCount - 1 : pageIndex;

        try {
          final bookmark = document.bookmarks.add(title);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          print('[PDF] Added bookmark: "$title" at page $safePageIndex');
        } catch (e) {
          print('[PDF] Error adding bookmark "$title": $e');
          // Continue to next bookmark
          continue;
        }
      }

      print('[PDF] Successfully added $addedCount out of ${newBookmarks.length} bookmarks');

      final outBytes = await document.save();
      document.dispose();
      document = null;

      print('[PDF] Document saved, size: ${outBytes.length} bytes');

      // Safe overwrite logic
      final tmpPath = '$filePath.tmp';
      final originalFile = File(filePath);
      final tmpFile = File(tmpPath);

      await tmpFile.writeAsBytes(outBytes);

      try {
        await originalFile.delete();
        await tmpFile.rename(filePath);
      } catch (_) {
        if (await tmpFile.exists()) {
          await tmpFile.delete();
        }
        throw PdfOverwriteException(
          'Could not replace the PDF because it is open in another application. '
          'Close the file and try again.',
        );
      }

      print('[PDF] File overwritten successfully');
      return true;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      print('[PDF] ERROR in injectBookmarksBatch: $e');
      print('[PDF] Stack trace: $stackTrace');
      return false;
    } finally {
      document?.dispose();
    }
  }

  /// Overwrite all bookmarks in a PDF file with a new set of bookmarks
  /// Returns true if successful, false if it fails
  static Future<bool> overwriteAllBookmarks(
    String filePath,
    List<Map<String, dynamic>> finalBookmarks, {
    List<String>? toRemove,
    List<String>? toAdd,
    Map<String, String>? descriptions,
  }) async {
    PdfDocument? document;
    try {
      print('[PDF] Starting bookmark overwrite for: $filePath');
      print('[PDF] Number of bookmarks to set: ${finalBookmarks.length}');

      final bytes = await File(filePath).readAsBytes();
      document = PdfDocument(inputBytes: bytes);

      // Clear all existing bookmarks
      print('[PDF] Clearing existing bookmarks...');
      document.bookmarks.clear();

      final pageCount = document.pages.count;
      int addedCount = 0;

      // Loop through finalBookmarks and add each one
      for (final bookmarkData in finalBookmarks) {
        final title = bookmarkData['title'] as String;
        final pageIndex = bookmarkData['pageIndex'] as int;

        // Handle out-of-bounds pages by defaulting to the last page
        final safePageIndex = pageIndex >= pageCount ? pageCount - 1 : pageIndex;

        try {
          final bookmark = document.bookmarks.add(title);
          bookmark.destination = PdfDestination(document.pages[safePageIndex]);
          addedCount++;
          print('[PDF] Added bookmark: "$title" at page $safePageIndex');
        } catch (e) {
          print('[PDF] Error adding bookmark "$title": $e');
          // Continue to next bookmark
          continue;
        }
      }

      print('[PDF] Successfully added $addedCount out of ${finalBookmarks.length} bookmarks');

      // Write descriptions to custom property if provided
      // Note: Syncfusion PDF doesn't support customProperties, so we skip this for now
      // TODO: Implement alternative metadata storage for descriptions
      if (descriptions != null && descriptions.isNotEmpty) {
        print("[PDF] Descriptions provided but not saved (Syncfusion doesn't support customProperties)");
      }

      final outBytes = await document.save();
      document.dispose();
      document = null;

      print('[PDF] Document saved, size: ${outBytes.length} bytes');

      // Safe overwrite logic
      final tmpPath = '$filePath.tmp';
      final originalFile = File(filePath);
      final tmpFile = File(tmpPath);

      await tmpFile.writeAsBytes(outBytes);

      try {
        await originalFile.delete();
        await tmpFile.rename(filePath);
      } catch (_) {
        if (await tmpFile.exists()) {
          await tmpFile.delete();
        }
        throw PdfOverwriteException(
          'Could not replace the PDF because it is open in another application. '
          'Close the file and try again.',
        );
      }

      print('[PDF] File overwritten successfully');

      // Sync History Log Entry
      if (toRemove != null || toAdd != null) {
        final timestamp = DateTime.now().toIso8601String();
        final removedStr = toRemove != null && toRemove.isNotEmpty
            ? 'Removed: [${toRemove.join(", ")}]'
            : 'Removed: []';
        final addedStr = toAdd != null && toAdd.isNotEmpty
            ? 'Added: [${toAdd.join(", ")}]'
            : 'Added: []';
        print('[SYNC HISTORY] Version pushed at $timestamp: $removedStr and $addedStr');
      }

      return true;
    } on PdfOverwriteException {
      rethrow;
    } catch (e, stackTrace) {
      print('[PDF] ERROR in overwriteAllBookmarks: $e');
      print('[PDF] Stack trace: $stackTrace');
      return false;
    } finally {
      document?.dispose();
    }
  }
}
