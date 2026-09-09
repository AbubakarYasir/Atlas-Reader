import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/scanned_pdf.dart';
import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late AppDatabase db;

  setUp(() async {
    db = AppDatabase.forTesting(NativeDatabase.memory());
  });

  tearDown(() async {
    await db.close();
  });

  test(
    'Stage 7: Library database supports rich metadata, favorites, progress, and sorting',
    () async {
      final folder = await db.addLibraryFolder('C:\\Books');
      expect(folder.id, isPositive);

      final scannedBooks = [
        ScannedPdf(
          filePath: 'C:\\Books\\hadith.pdf',
          fileName: 'hadith.pdf',
          bookmarkCount: 12,
          lastModified: DateTime.now().subtract(const Duration(days: 2)),
          title: 'Sahih Al-Bukhari',
          author: 'Imam Al-Bukhari',
          format: 'PDF',
          pageCount: 500,
          fileSizeBytes: 25000000,
          tags: 'hadith, sunnah',
          series: 'Kutub al-Sittah',
        ),
        ScannedPdf(
          filePath: 'C:\\Books\\history.epub',
          fileName: 'history.epub',
          bookmarkCount: 8,
          lastModified: DateTime.now().subtract(const Duration(days: 1)),
          title: 'Muqaddimah',
          author: 'Ibn Khaldun',
          format: 'EPUB',
          pageCount: 350,
          fileSizeBytes: 12000000,
          tags: 'history, sociology',
          series: 'Classical Thought',
        ),
        ScannedPdf(
          filePath: 'C:\\Books\\jurisprudence.pdf',
          fileName: 'jurisprudence.pdf',
          bookmarkCount: 5,
          lastModified: DateTime.now(),
          title: 'Al-Risala',
          author: 'Imam Al-Shafi',
          format: 'PDF',
          pageCount: 200,
          fileSizeBytes: 8000000,
          tags: 'law, usul',
          series: 'Classical Thought',
        ),
      ];

      await db.upsertLibraryFiles(folder.id, scannedBooks);

      final allFiles = await db.getFilteredLibraryFiles();
      expect(allFiles, hasLength(3));

      final hadithBook = allFiles.firstWhere((b) => b.fileName == 'hadith.pdf');
      expect(hadithBook.title, 'Sahih Al-Bukhari');
      expect(hadithBook.author, 'Imam Al-Bukhari');
      expect(hadithBook.format, 'PDF');
      expect(hadithBook.pageCount, 500);
      expect(hadithBook.isFavorite, isFalse);

      final isFav = await db.toggleFavorite(hadithBook.id);
      expect(isFav, isTrue);

      final favFiles = await db.getFilteredLibraryFiles(onlyFavorites: true);
      expect(favFiles, hasLength(1));
      expect(favFiles.first.id, hadithBook.id);

      await db.updateReadingProgress(hadithBook.filePath, 250, pageCount: 500);
      final updatedHadith = (await db.getFilteredLibraryFiles()).firstWhere(
        (b) => b.id == hadithBook.id,
      );
      expect(updatedHadith.currentPage, 250);
      expect(updatedHadith.lastOpened, isNotNull);

      final authors = await db.getDistinctAuthors();
      expect(
        authors.keys,
        containsAll(['Imam Al-Bukhari', 'Ibn Khaldun', 'Imam Al-Shafi']),
      );
      expect(authors['Imam Al-Bukhari'], 1);

      final tags = await db.getDistinctTags();
      expect(
        tags.keys,
        containsAll([
          'hadith',
          'sunnah',
          'history',
          'sociology',
          'law',
          'usul',
        ]),
      );

      final series = await db.getDistinctSeries();
      expect(
        series.keys,
        containsAll(['Kutub al-Sittah', 'Classical Thought']),
      );
      expect(series['Classical Thought'], 2);

      final historyFiltered = await db.getFilteredLibraryFiles(tag: 'history');
      expect(historyFiltered, hasLength(1));
      expect(historyFiltered.first.title, 'Muqaddimah');

      final seriesFiltered = await db.getFilteredLibraryFiles(
        series: 'Classical Thought',
      );
      expect(seriesFiltered, hasLength(2));

      final authorFiltered = await db.getFilteredLibraryFiles(
        author: 'Imam Al-Shafi',
      );
      expect(authorFiltered, hasLength(1));
      expect(authorFiltered.first.title, 'Al-Risala');

      final sortedByTitle = await db.getFilteredLibraryFiles(
        sortBy: LibrarySortBy.title,
        ascending: true,
      );
      expect(sortedByTitle.first.title, 'Al-Risala');
      expect(sortedByTitle.last.title, 'Sahih Al-Bukhari');

      final sortedByProgress = await db.getFilteredLibraryFiles(
        sortBy: LibrarySortBy.progress,
        ascending: false,
      );
      expect(sortedByProgress.first.id, hadithBook.id);
    },
  );
}
