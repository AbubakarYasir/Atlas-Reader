/// A document discovered by a library-folder scan, ready for persistence.
class ScannedPdf {
  const ScannedPdf({
    required this.filePath,
    required this.fileName,
    required this.bookmarkCount,
    required this.lastModified,
    this.title,
    this.author,
    this.format = 'PDF',
    this.pageCount = 0,
    this.fileSizeBytes = 0,
    this.coverPath,
    this.tags,
    this.series,
  });

  final String filePath;
  final String fileName;
  final int bookmarkCount;
  final DateTime lastModified;
  final String? title;
  final String? author;
  final String format;
  final int pageCount;
  final int fileSizeBytes;
  final String? coverPath;
  final String? tags;
  final String? series;
}
