import 'dart:io';
import 'dart:isolate';

import 'document_file_system.dart';

/// Windows and desktop implementation backed by `dart:io`.
class WindowsDocumentFileSystem implements DocumentFileSystem {
  const WindowsDocumentFileSystem();

  @override
  Future<void> delete(String path) => File(path).delete();

  @override
  Future<bool> exists(String path) => File(path).exists();

  @override
  Future<List<int>> readAsBytes(String path) => File(path).readAsBytes();

  @override
  Future<List<int>> readAsBytesInBackground(String path) {
    return Isolate.run(() => File(path).readAsBytes());
  }

  @override
  Future<void> rename(String fromPath, String toPath) async {
    await File(fromPath).rename(toPath);
  }

  @override
  Future<void> writeAsBytes(
    String path,
    List<int> bytes, {
    bool flush = false,
  }) async {
    await File(path).writeAsBytes(bytes, flush: flush);
  }

  @override
  Future<void> writeAsBytesInBackground(
    String path,
    List<int> bytes, {
    bool flush = false,
  }) {
    return Isolate.run(() => File(path).writeAsBytes(bytes, flush: flush));
  }
}
