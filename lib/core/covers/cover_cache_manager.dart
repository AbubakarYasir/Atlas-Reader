import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:crypto/crypto.dart';
import 'package:logging/logging.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';

import '../epub/epub_engine.dart';

class CoverCacheManager {
  CoverCacheManager._();
  static final CoverCacheManager instance = CoverCacheManager._();

  static final _log = Logger('CoverCacheManager');
  static Directory? _cacheDir;
  final Map<String, Uint8List?> _memoryCache = {};

  Future<Directory> _getCacheDirectory() async {
    if (_cacheDir != null && await _cacheDir!.exists()) {
      return _cacheDir!;
    }
    if (Platform.environment.containsKey('FLUTTER_TEST')) {
      final dir = Directory(
        p.join(Directory.systemTemp.path, 'atlas_covers_test'),
      );
      if (!await dir.exists()) {
        await dir.create(recursive: true);
      }
      _cacheDir = dir;
      return dir;
    }
    try {
      final appDir = await getApplicationSupportDirectory();
      final dir = Directory(p.join(appDir.path, 'atlas_covers'));
      if (!await dir.exists()) {
        await dir.create(recursive: true);
      }
      _cacheDir = dir;
      return dir;
    } catch (_) {
      final tempDir = Directory.systemTemp;
      final dir = Directory(p.join(tempDir.path, 'atlas_covers'));
      if (!await dir.exists()) {
        await dir.create(recursive: true);
      }
      _cacheDir = dir;
      return dir;
    }
  }

  String _hashPath(String filePath) {
    return md5.convert(utf8.encode(filePath)).toString();
  }

  Future<String?> getCachedCoverPath(String filePath) async {
    final cacheDir = await _getCacheDirectory();
    final hash = _hashPath(filePath);
    for (final ext in ['jpg', 'jpeg', 'png', 'webp']) {
      final candidate = File(p.join(cacheDir.path, '$hash.$ext'));
      if (await candidate.exists()) {
        return candidate.path;
      }
    }
    return null;
  }

  Future<Uint8List?> loadCoverBytes(String? coverPath) async {
    if (coverPath == null) return null;
    if (_memoryCache.containsKey(coverPath)) {
      return _memoryCache[coverPath];
    }
    try {
      final file = File(coverPath);
      if (await file.exists()) {
        final bytes = await file.readAsBytes();
        _memoryCache[coverPath] = bytes;
        return bytes;
      }
    } catch (e) {
      _log.warning('Could not load cover bytes: $e');
    }
    return null;
  }

  Future<String?> saveCoverBytes(
    String filePath,
    Uint8List bytes, {
    String extension = 'jpg',
  }) async {
    try {
      final cacheDir = await _getCacheDirectory();
      final hash = _hashPath(filePath);
      final targetFile = File(p.join(cacheDir.path, '$hash.$extension'));
      await targetFile.writeAsBytes(bytes, flush: true);
      _memoryCache[targetFile.path] = bytes;
      return targetFile.path;
    } catch (e) {
      _log.warning('Failed to save cover cache: $e');
      return null;
    }
  }

  Future<String?> extractAndCacheCover(String filePath) async {
    final existing = await getCachedCoverPath(filePath);
    if (existing != null) return existing;

    final lower = filePath.toLowerCase();
    if (lower.endsWith('.epub')) {
      final epubMeta = await const EpubEngine().extractMetadata(filePath);
      if (epubMeta?.coverImageBytes != null) {
        return saveCoverBytes(
          filePath,
          epubMeta!.coverImageBytes!,
          extension: epubMeta.coverImageExtension ?? 'jpg',
        );
      }
    }
    return null;
  }
}
