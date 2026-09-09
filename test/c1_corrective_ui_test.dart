import 'package:atlas_poc/core/file_system/document_file_system.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/features/library/visual_bookshelf.dart';
import 'package:atlas_poc/l10n/app_localizations.dart';
import 'package:atlas_poc/scanned_pdf.dart';
import 'package:drift/native.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late AppDatabase database;

  setUp(() => database = AppDatabase.forTesting(NativeDatabase.memory()));
  tearDown(() => database.close());

  Widget localized(Widget child, {Locale locale = const Locale('en')}) {
    return MaterialApp(
      locale: locale,
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: Scaffold(body: child),
    );
  }

  test(
    'extended reader session state round-trips with compatible defaults',
    () async {
      await database.saveReadingSessionTabStates([
        (
          filePath: r'C:\Books\Atlas.pdf',
          pageNumber: 7,
          isActive: true,
          zoomPercent: 37.5,
          zoomPreset: 'custom',
          readerMode: 'write',
          navigationPanel: 'bookmarks',
          viewport: '{"page":6,"top":0.2,"zoom":0.375}',
        ),
      ]);

      final tab = (await database.getReadingSessionTabs()).single;
      expect(tab.zoomPercent, 37.5);
      expect(tab.zoomPreset, 'custom');
      expect(tab.readerMode, 'write');
      expect(tab.navigationPanel, 'bookmarks');
      expect(tab.viewport, contains('0.375'));
    },
  );

  test(
    'tab restoration is opt-in and disabling it clears saved tabs',
    () async {
      expect(await database.getRestoreDocumentTabs(), isFalse);

      await database.setRestoreDocumentTabs(true);
      expect(await database.getRestoreDocumentTabs(), isTrue);
      await database.saveReadingSessionTabs([
        (filePath: r'C:\Books\Atlas.pdf', pageNumber: 3, isActive: true),
      ]);
      expect(await database.getReadingSessionTabs(), hasLength(1));

      await database.setRestoreDocumentTabs(false);
      expect(await database.getRestoreDocumentTabs(), isFalse);
      expect(await database.getReadingSessionTabs(), isEmpty);
    },
  );

  testWidgets('book right-click exposes fast actions in English and Arabic', (
    tester,
  ) async {
    final folder = await database.addLibraryFolder(r'C:\Books');
    await database.upsertLibraryFiles(folder.id, [
      ScannedPdf(
        filePath: r'C:\Books\Atlas.pdf',
        fileName: 'Atlas.pdf',
        bookmarkCount: 0,
        lastModified: DateTime(2026),
        title: 'Atlas',
        format: 'PDF',
        pageCount: 10,
        fileSizeBytes: 100,
      ),
    ]);

    Future<void> pump(Locale locale) async {
      await tester.pumpWidget(
        localized(
          VisualBookshelf(
            database: database,
            fileSystem: const _ExistingFileSystem(),
            onCreateBookmark: (_, _) async {},
          ),
          locale: locale,
        ),
      );
      await tester.pumpAndSettle();
    }

    await pump(const Locale('en'));
    await tester.tap(find.text('Atlas').first, buttons: kSecondaryMouseButton);
    await tester.pumpAndSettle();
    expect(find.text('Refresh cover'), findsOneWidget);
    expect(find.text('Copy full path'), findsOneWidget);
    await tester.tapAt(const Offset(2, 2));
    await tester.pumpAndSettle();

    await pump(const Locale('ar'));
    await tester.tap(find.text('Atlas').first, buttons: kSecondaryMouseButton);
    await tester.pumpAndSettle();
    expect(find.text('تحديث الغلاف'), findsOneWidget);
    expect(find.text('نسخ المسار الكامل'), findsOneWidget);
  });
}

class _ExistingFileSystem implements DocumentFileSystem {
  const _ExistingFileSystem();

  @override
  Future<bool> exists(String path) async => true;
  @override
  Future<List<int>> readAsBytes(String path) async => const [];
  @override
  Future<List<int>> readAsBytesInBackground(String path) async => const [];
  @override
  Future<void> writeAsBytes(
    String path,
    List<int> bytes, {
    bool flush = false,
  }) async {}
  @override
  Future<void> writeAsBytesInBackground(
    String path,
    List<int> bytes, {
    bool flush = false,
  }) async {}
  @override
  Future<void> delete(String path) async {}
  @override
  Future<void> rename(String fromPath, String toPath) async {}
  @override
  Future<List<String>> listDirectory(String path) async => const [];
}
