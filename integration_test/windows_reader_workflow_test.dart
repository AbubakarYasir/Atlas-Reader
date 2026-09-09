import 'dart:io';
import 'dart:typed_data';

import 'package:atlas_poc/core/file_system/windows_document_file_system.dart';
import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/features/command_center/command_center_overlay.dart';
import 'package:atlas_poc/features/library/library_files_screen.dart';
import 'package:atlas_poc/features/library/library_folder_manager.dart';
import 'package:atlas_poc/features/reader/ink_toolbar.dart';
import 'package:atlas_poc/features/reader/pdf_reader_screen.dart';
import 'package:atlas_poc/main.dart' as atlas_app;
import 'package:dart_pdf_editor/dart_pdf_editor.dart' as editor;
import 'package:drift/native.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:pdf_document/pdf_document.dart' as native_pdf;
import 'package:syncfusion_flutter_pdf/pdf.dart' as syncfusion;

import 'package:atlas_poc/l10n/app_localizations.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('Windows launch argument opens a PDF outside the library', (
    tester,
  ) async {
    final tempDirectory = await Directory.systemTemp.createTemp(
      'atlas_external_pdf_',
    );
    final pdfPath =
        '${tempDirectory.path}${Platform.pathSeparator}external-book.pdf';
    _writeBook(
      pdfPath,
      title: 'External PDF',
      author: 'Outside Library',
      outlineTitle: 'Direct open',
    );
    atlas_app.database = AppDatabase.forTesting(NativeDatabase.memory());
    addTearDown(() async {
      await atlas_app.database.close();
      if (await tempDirectory.exists()) {
        await tempDirectory.delete(recursive: true);
      }
    });

    await tester.pumpWidget(atlas_app.MyApp(initialFilePath: pdfPath));
    await _pumpFor(tester, const Duration(seconds: 4));

    expect(find.byType(PdfReaderScreen), findsOneWidget);
    expect(find.text('Write'), findsOneWidget);
    final recents = await atlas_app.database.getFilteredLibraryFiles(
      onlyOpened: true,
      sortBy: LibrarySortBy.lastOpened,
      ascending: false,
    );
    expect(recents.single.filePath, pdfPath);
    expect(await atlas_app.database.getLibraryFolders(), isEmpty);
  });

  testWidgets('Windows reader opens, draws, saves, and reopens standard ink', (
    tester,
  ) async {
    final tempDirectory = await Directory.systemTemp.createTemp(
      'atlas_windows_reader_',
    );
    final pdfPath =
        '${tempDirectory.path}${Platform.pathSeparator}reader-roundtrip.pdf';
    final database = AppDatabase.forTesting(NativeDatabase.memory());
    addTearDown(() async {
      await database.close();
      if (await tempDirectory.exists()) {
        await tempDirectory.delete(recursive: true);
      }
    });

    final fixture = syncfusion.PdfDocument();
    final firstPage = fixture.pages.add();
    final secondPage = fixture.pages.add();
    fixture.documentInformation
      ..title = 'Windows ink round-trip'
      ..author = 'Atlas Reader';
    final root = fixture.bookmarks.add('المجلد الأول Volume 1');
    root.destination = syncfusion.PdfDestination(firstPage);
    final child = root.add('الفصل الأول Chapter 1');
    child.destination = syncfusion.PdfDestination(secondPage);
    await File(pdfPath).writeAsBytes(fixture.saveSync(), flush: true);
    fixture.dispose();

    await tester.pumpWidget(
      MaterialApp(
        supportedLocales: AppLocalizations.supportedLocales,
        localizationsDelegates: const [
          AppLocalizations.delegate,
          editor.DartPdfEditorLocalizations.delegate,
          GlobalMaterialLocalizations.delegate,
          GlobalWidgetsLocalizations.delegate,
          GlobalCupertinoLocalizations.delegate,
        ],
        home: PdfReaderScreen(
          filePath: pdfPath,
          fileSystem: const WindowsDocumentFileSystem(),
          database: database,
          onCreateBookmark: (_) async {},
        ),
      ),
    );
    await _pumpFor(tester, const Duration(seconds: 4));
    expect(find.text('Write'), findsOneWidget);

    await tester.tap(find.text('Write'));
    await _pumpFor(tester, const Duration(seconds: 5));
    expect(
      find.byKey(const ValueKey('ink-canvas-repaint-boundary')),
      findsOneWidget,
    );
    expect(find.byTooltip('Pen'), findsOneWidget);

    expect(find.byType(editor.PdfPageView), findsWidgets);
    final firstPageRect = tester.getRect(find.byType(editor.PdfPageView).first);
    final start = firstPageRect.center - const Offset(70, 30);
    final gesture = await tester.createGesture(kind: PointerDeviceKind.mouse);
    await gesture.down(start);
    for (var step = 1; step <= 12; step++) {
      await gesture.moveTo(start + Offset(step * 10, step * 5));
      await tester.pump(const Duration(milliseconds: 16));
    }
    await gesture.up();
    await tester.pump(const Duration(milliseconds: 500));

    final inkToolbar = tester.widget<InkToolbar>(find.byType(InkToolbar));
    inkToolbar.controller.finishInk();
    expect(inkToolbar.controller.revisionId, greaterThan(0));
    await tester.pump();

    await tester.tap(find.widgetWithText(FilledButton, 'Save'));
    await _pumpFor(tester, const Duration(seconds: 2));

    final saved = native_pdf.PdfDocument.open(
      Uint8List.fromList(await File(pdfPath).readAsBytes()),
    );
    expect(
      saved
          .page(0)
          .annotations
          .where((annotation) => annotation.subtype == 'Ink'),
      isNotEmpty,
    );
    expect(
      saved
          .page(1)
          .annotations
          .where((annotation) => annotation.subtype == 'Ink'),
      isEmpty,
    );
    expect(native_pdf.PdfOutline.of(saved).items.single.children, hasLength(1));
    expect(saved.info['Title'], 'Windows ink round-trip');
  });

  testWidgets(
    'Windows library scans folders, filters books, and finds a bookmark',
    (tester) async {
      final root = await Directory.systemTemp.createTemp(
        'atlas_windows_library_',
      );
      final arabicFolder = await Directory(
        '${root.path}${Platform.pathSeparator}arabic',
      ).create();
      final englishFolder = await Directory(
        '${root.path}${Platform.pathSeparator}english',
      ).create();
      final arabicPath =
          '${arabicFolder.path}${Platform.pathSeparator}usul.pdf';
      final englishPath =
          '${englishFolder.path}${Platform.pathSeparator}history.pdf';
      _writeBook(
        arabicPath,
        title: 'أصول الفقه',
        author: 'وليد السعيدان',
        outlineTitle: 'الباب الأول Chapter 1',
      );
      _writeBook(
        englishPath,
        title: 'A Short History',
        author: 'English Author',
        outlineTitle: 'Opening Chapter',
      );

      final database = AppDatabase.forTesting(NativeDatabase.memory());
      final manager = LibraryFolderManager(
        database: database,
        fileSystem: const WindowsDocumentFileSystem(),
      );
      addTearDown(() async {
        await manager.dispose();
        await database.close();
        if (await root.exists()) await root.delete(recursive: true);
      });
      await manager.addFolder(arabicFolder.path);
      await manager.addFolder(englishFolder.path);
      final report = await manager.rescanAll();
      expect(report.foldersScanned, 2);
      expect(report.filesFound, 2);

      final bookmarkId = await database.addBookmark(
        filePath: arabicPath,
        title: 'مبحث القياس Analogy',
        pageIndex: 0,
      );
      await database.addTagToBookmark(bookmarkId, 'فقه');

      await tester.pumpWidget(
        _localizedApp(
          LibraryFilesScreen(
            database: database,
            fileSystem: const WindowsDocumentFileSystem(),
            onCreateBookmark: (_, _) async {},
          ),
        ),
      );
      await _pumpFor(tester, const Duration(seconds: 2));
      expect(find.text('أصول الفقه'), findsWidgets);
      expect(find.text('A Short History'), findsWidgets);

      await tester.enterText(find.byType(TextField).first, 'السعيدان');
      await _pumpFor(tester, const Duration(seconds: 1));
      expect(find.text('أصول الفقه'), findsWidgets);
      expect(find.text('A Short History'), findsNothing);

      CommandCenterResult? selected;
      await tester.pumpWidget(
        _localizedApp(
          Scaffold(
            body: CommandCenterOverlay(
              database: database,
              onSelected: (result) => selected = result,
            ),
          ),
        ),
      );
      await tester.pump();
      await tester.enterText(find.byType(TextField), 'القياس');
      await _pumpFor(tester, const Duration(seconds: 1));
      expect(find.text('مبحث القياس Analogy'), findsOneWidget);
      await tester.tap(find.text('مبحث القياس Analogy'));
      expect(selected?.filePath, arabicPath);
      expect(selected?.pageNumber, 1);
      expect(selected?.breadcrumb, 'مبحث القياس Analogy');
    },
  );
}

Widget _localizedApp(Widget home) {
  return MaterialApp(
    supportedLocales: AppLocalizations.supportedLocales,
    localizationsDelegates: const [
      AppLocalizations.delegate,
      editor.DartPdfEditorLocalizations.delegate,
      GlobalMaterialLocalizations.delegate,
      GlobalWidgetsLocalizations.delegate,
      GlobalCupertinoLocalizations.delegate,
    ],
    home: home,
  );
}

void _writeBook(
  String path, {
  required String title,
  required String author,
  required String outlineTitle,
}) {
  final document = syncfusion.PdfDocument();
  final page = document.pages.add();
  document.documentInformation
    ..title = title
    ..author = author;
  document.bookmarks.add(outlineTitle).destination = syncfusion.PdfDestination(
    page,
  );
  File(path).writeAsBytesSync(document.saveSync(), flush: true);
  document.dispose();
}

Future<void> _pumpFor(WidgetTester tester, Duration duration) async {
  final end = DateTime.now().add(duration);
  while (DateTime.now().isBefore(end)) {
    await tester.pump(const Duration(milliseconds: 100));
  }
}
