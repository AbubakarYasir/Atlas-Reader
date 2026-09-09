/// Platform-neutral operations needed to read and safely replace user documents.
///
/// Android can provide a Storage Access Framework implementation without
/// changing PDF parsing, sync, or UI code.
abstract interface class DocumentFileSystem {
  Future<bool> exists(String path);

  Future<List<int>> readAsBytes(String path);

  /// Reads a document without competing with scrolling or animation work.
  Future<List<int>> readAsBytesInBackground(String path);

  Future<void> writeAsBytes(String path, List<int> bytes, {bool flush = false});

  Future<void> delete(String path);

  Future<void> rename(String fromPath, String toPath);
}
