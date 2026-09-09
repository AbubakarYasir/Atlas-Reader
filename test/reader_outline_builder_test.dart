import 'package:atlas_poc/features/reader/reader_outline_builder.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('builds unlimited nested paths and keeps duplicate titles isolated', () {
    final outline = buildReaderOutline(const [
      ReaderOutlineRecord(path: ['المجلد الأول Volume 1'], pageNumber: 1),
      ReaderOutlineRecord(
        path: ['المجلد الأول Volume 1', 'الفصل 2 Chapter', 'المبحث A'],
        pageNumber: 8,
      ),
      ReaderOutlineRecord(
        path: ['المجلد الثاني Volume 2', 'الفصل 2 Chapter', 'المبحث A'],
        pageNumber: 48,
        isUserBookmark: true,
      ),
    ]);

    expect(outline, hasLength(2));
    expect(outline.first.children.single.children.single.pageNumber, 8);
    final secondLeaf = outline.last.children.single.children.single;
    expect(secondLeaf.pageNumber, 48);
    expect(secondLeaf.isUserBookmark, isTrue);
    expect(
      secondLeaf.breadcrumb,
      'المجلد الثاني Volume 2 > الفصل 2 Chapter > المبحث A',
    );
  });

  test(
    'merges the local bookmark marker into the matching PDF outline node',
    () {
      final outline = buildReaderOutline(const [
        ReaderOutlineRecord(path: ['Volume', 'Chapter'], pageNumber: 12),
        ReaderOutlineRecord(
          path: ['Volume', 'Chapter'],
          pageNumber: 12,
          isUserBookmark: true,
        ),
      ]);

      expect(outline.single.children, hasLength(1));
      expect(outline.single.children.single.isUserBookmark, isTrue);
    },
  );
}
