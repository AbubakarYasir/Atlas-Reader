import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:archive/archive.dart';
import 'package:logging/logging.dart';
import 'package:xml/xml.dart';

class EpubMetadata {
  const EpubMetadata({
    this.title,
    this.author,
    this.publisher,
    this.language,
    this.coverImageBytes,
    this.coverImageExtension,
    this.chapters = const [],
  });

  final String? title;
  final String? author;
  final String? publisher;
  final String? language;
  final Uint8List? coverImageBytes;
  final String? coverImageExtension;
  final List<EpubChapter> chapters;
}

class EpubChapter {
  const EpubChapter({
    required this.title,
    required this.content,
    this.subChapters = const [],
    this.playOrder,
  });

  final String title;
  final String content;
  final List<EpubChapter> subChapters;
  final int? playOrder;
}

class EpubEngine {
  const EpubEngine();

  static final _log = Logger('EpubEngine');

  Future<EpubMetadata?> extractMetadata(String filePath) async {
    try {
      final file = File(filePath);
      if (!await file.exists()) return null;
      final bytes = await file.readAsBytes();
      return Isolate.run(() => _parseEpubBytes(bytes));
    } catch (e, st) {
      _log.warning('[EPUB] Failed to extract metadata from $filePath: $e\n$st');
      return null;
    }
  }

  static EpubMetadata? _parseEpubBytes(Uint8List bytes) {
    try {
      final archive = ZipDecoder().decodeBytes(bytes);

      ArchiveFile? containerFile;
      for (final f in archive) {
        if (f.name == 'META-INF/container.xml') {
          containerFile = f;
          break;
        }
      }
      if (containerFile == null) return null;

      final containerXml = utf8.decode(containerFile.content as List<int>);
      final containerDoc = XmlDocument.parse(containerXml);
      final rootfileElem = containerDoc.findAllElements('rootfile').firstOrNull;
      if (rootfileElem == null) return null;

      final opfPath = rootfileElem.getAttribute('full-path');
      if (opfPath == null) return null;

      ArchiveFile? opfFile;
      for (final f in archive) {
        if (f.name == opfPath) {
          opfFile = f;
          break;
        }
      }
      if (opfFile == null) return null;

      final opfXml = utf8.decode(opfFile.content as List<int>);
      final opfDoc = XmlDocument.parse(opfXml);

      final title = opfDoc
          .findAllElements('dc:title')
          .firstOrNull
          ?.innerText
          .trim();
      final author = opfDoc
          .findAllElements('dc:creator')
          .firstOrNull
          ?.innerText
          .trim();
      final publisher = opfDoc
          .findAllElements('dc:publisher')
          .firstOrNull
          ?.innerText
          .trim();
      final language = opfDoc
          .findAllElements('dc:language')
          .firstOrNull
          ?.innerText
          .trim();

      final opfDir = opfPath.contains('/')
          ? opfPath.substring(0, opfPath.lastIndexOf('/') + 1)
          : '';

      String? coverHref;
      final metaCoverElem = opfDoc
          .findAllElements('meta')
          .firstWhereOrNull(
            (e) => e.getAttribute('name')?.toLowerCase() == 'cover',
          );
      if (metaCoverElem != null) {
        final coverId = metaCoverElem.getAttribute('content');
        if (coverId != null) {
          final manifestItem = opfDoc
              .findAllElements('item')
              .firstWhereOrNull((e) => e.getAttribute('id') == coverId);
          coverHref = manifestItem?.getAttribute('href');
        }
      }

      if (coverHref == null) {
        final manifestItem = opfDoc.findAllElements('item').firstWhereOrNull((
          e,
        ) {
          final props = e.getAttribute('properties') ?? '';
          final id = e.getAttribute('id')?.toLowerCase() ?? '';
          final href = e.getAttribute('href')?.toLowerCase() ?? '';
          return props.contains('cover-image') ||
              id == 'cover' ||
              id == 'cover-image' ||
              href.contains('cover');
        });
        coverHref = manifestItem?.getAttribute('href');
      }

      Uint8List? coverBytes;
      String? coverExt;
      if (coverHref != null) {
        final fullCoverPath = _normalizePath('$opfDir$coverHref');
        for (final f in archive) {
          if (f.name == fullCoverPath ||
              f.name.toLowerCase() == fullCoverPath.toLowerCase()) {
            coverBytes = Uint8List.fromList(f.content as List<int>);
            coverExt =
                fullCoverPath.split('.').lastOrNull?.toLowerCase() ?? 'jpg';
            break;
          }
        }
      }

      final chapters = <EpubChapter>[];
      final ncxItem = opfDoc.findAllElements('item').firstWhereOrNull((e) {
        return e.getAttribute('media-type') == 'application/x-dtbncx+xml' ||
            e.getAttribute('id') == 'ncx';
      });

      if (ncxItem != null) {
        final ncxHref = ncxItem.getAttribute('href');
        if (ncxHref != null) {
          final fullNcxPath = _normalizePath('$opfDir$ncxHref');
          ArchiveFile? ncxFile;
          for (final f in archive) {
            if (f.name == fullNcxPath) {
              ncxFile = f;
              break;
            }
          }
          if (ncxFile != null) {
            final ncxXml = utf8.decode(ncxFile.content as List<int>);
            final ncxDoc = XmlDocument.parse(ncxXml);
            for (final navPoint in ncxDoc.findAllElements('navPoint')) {
              final text = navPoint
                  .findAllElements('text')
                  .firstOrNull
                  ?.innerText
                  .trim();
              if (text != null && text.isNotEmpty) {
                chapters.add(EpubChapter(title: text, content: ''));
              }
            }
          }
        }
      }

      return EpubMetadata(
        title: title,
        author: author,
        publisher: publisher,
        language: language,
        coverImageBytes: coverBytes,
        coverImageExtension: coverExt,
        chapters: chapters,
      );
    } catch (e) {
      return null;
    }
  }

  static String _normalizePath(String raw) {
    final parts = raw.split('/');
    final out = <String>[];
    for (final p in parts) {
      if (p == '.' || p.isEmpty) continue;
      if (p == '..') {
        if (out.isNotEmpty) out.removeLast();
      } else {
        out.add(p);
      }
    }
    return out.join('/');
  }
}

extension on Iterable<XmlElement> {
  XmlElement? firstWhereOrNull(bool Function(XmlElement) test) {
    for (final element in this) {
      if (test(element)) return element;
    }
    return null;
  }
}
