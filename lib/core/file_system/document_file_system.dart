/// Platform-neutral operations needed to read and safely replace user documents.
///
/// Android can provide a Storage Access Framework implementation without
/// changing PDF parsing, sync, or UI code.
abstract interface class DocumentFileSystem {
  Future<bool> exists(String path);

  Future<List<int>> readAsBytes(String path);

  Future<void> writeAsBytes(String path, List<int> bytes, {bool flush = false});

  Future<void> delete(String path);

  Future<void> rename(String fromPath, String toPath);
}
