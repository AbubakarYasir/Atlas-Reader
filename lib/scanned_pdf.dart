/// A PDF discovered by a library-folder scan, ready for persistence.
class ScannedPdf {
  const ScannedPdf({
    required this.filePath,
    required this.fileName,
    required this.bookmarkCount,
    required this.lastModified,
  });

  final String filePath;
  final String fileName;
  final int bookmarkCount;
  final DateTime lastModified;
}
