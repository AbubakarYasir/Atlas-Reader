import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/features/reader/reader_models.dart';
import 'package:atlas_poc/features/reader/reader_navigation_panel.dart';
import 'package:atlas_poc/features/reader/reader_outline_sidebar.dart';
import 'package:atlas_poc/features/reader/reader_zoom_controls.dart';
import 'package:atlas_poc/l10n/app_localizations.dart';
import 'package:drift/native.dart';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late AppDatabase database;

  setUp(() {
    database = AppDatabase.forTesting(NativeDatabase.memory());
  });

  tearDown(() async {
    await database.close();
  });

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

  testWidgets('reader navigation searches panels and jumps to exact pages', (
    tester,
  ) async {
    const filePath = r'C:\Books\Mixed Arabic English.pdf';
    await database.addAnnotation(
      filePath: filePath,
      pageNumber: 4,
      type: 'highlight',
      selectedText: 'مقاصد الشريعة',
      note: 'Key passage',
    );
    final selectedPages = <int>[];

    await tester.pumpWidget(
      localized(
        ReaderNavigationPanel(
          filePath: filePath,
          database: database,
          outline: const [
            ReaderOutlineItem(
              title: 'Introduction مقدمة',
              pageNumber: 2,
              breadcrumb: 'Introduction مقدمة',
            ),
            ReaderOutlineItem(
              title: 'Saved place',
              pageNumber: 5,
              breadcrumb: 'Saved place',
              isUserBookmark: true,
            ),
          ],
          currentPage: 2,
          pageCount: 6,
          chapterPages: const {2},
          bookmarkedPages: const {5},
          pageOffset: 10,
          onSelectPage: selectedPages.add,
          onClose: () {},
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.text('Introduction مقدمة'), findsOneWidget);
    await tester.tap(find.text('Pages'));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const ValueKey('reader-page-thumbnail-4')));
    expect(selectedPages, [4]);

    await tester.tap(find.text('Outline'));
    await tester.pumpAndSettle();
    await tester.enterText(find.byType(TextField), 'مقدمة');
    await tester.pump();
    expect(find.text('Introduction مقدمة'), findsOneWidget);
    expect(find.text('Saved place'), findsNothing);

    await tester.tap(find.text('Bookmarks'));
    await tester.pumpAndSettle();
    expect(find.text('Saved place'), findsOneWidget);
    await tester.tap(find.text('Saved place'));
    expect(selectedPages.last, 5);

    final annotationsTab = find.byIcon(Icons.comment_outlined).first;
    await tester.ensureVisible(annotationsTab);
    await tester.tap(annotationsTab);
    await tester.pumpAndSettle();
    expect(find.text('highlight'), findsOneWidget);
    expect(find.textContaining('Key passage'), findsOneWidget);
    await tester.tap(find.text('highlight'));
    expect(selectedPages.last, 4);
  });

  testWidgets('zoom controls expose fit modes and exact percentages', (
    tester,
  ) async {
    final selections = <(ReaderZoomPreset, int?)>[];
    await tester.pumpWidget(
      localized(
        Center(
          child: ReaderZoomControls(
            percent: 125,
            preset: ReaderZoomPreset.custom,
            onZoomOut: () {},
            onZoomIn: () {},
            onPresetSelected: selections.add,
          ),
        ),
      ),
    );

    await tester.tap(find.byKey(const ValueKey('reader-zoom-menu')));
    await tester.pumpAndSettle();
    expect(find.text('Fit page'), findsOneWidget);
    expect(find.text('Zoom 200%'), findsOneWidget);
    await tester.tap(find.text('Zoom 200%'));
    await tester.pumpAndSettle();
    expect(selections.single, (ReaderZoomPreset.custom, 200));

    await tester.pumpWidget(
      localized(
        Center(
          child: ReaderZoomControls(
            percent: 200,
            preset: ReaderZoomPreset.custom,
            onZoomOut: () {},
            onZoomIn: () {},
            onPresetSelected: selections.add,
          ),
        ),
      ),
    );
    await tester.tap(find.byKey(const ValueKey('reader-zoom-menu')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Fit page'));
    await tester.pumpAndSettle();
    expect(selections.last, (ReaderZoomPreset.fitPage, null));

    await tester.tap(find.byKey(const ValueKey('reader-zoom-menu')));
    await tester.pumpAndSettle();
    await tester.ensureVisible(find.text('Custom zoom…'));
    await tester.tap(find.text('Custom zoom…'));
    await tester.pumpAndSettle();
    await tester.enterText(find.byType(TextField), '37');
    await tester.tap(find.text('Apply'));
    await tester.pumpAndSettle();
    expect(selections.last, (ReaderZoomPreset.custom, 37));
  });

  testWidgets('reader navigation remains usable in Arabic at 200% scale', (
    tester,
  ) async {
    tester.view.physicalSize = const Size(900, 900);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);

    await tester.pumpWidget(
      MediaQuery(
        data: const MediaQueryData(textScaler: TextScaler.linear(2)),
        child: localized(
          ReaderNavigationPanel(
            filePath: r'C:\Books\Arabic.pdf',
            database: database,
            outline: const [],
            currentPage: 1,
            pageCount: 2,
            chapterPages: const {},
            bookmarkedPages: const {},
            pageOffset: 0,
            width: 450,
            onSelectPage: (_) {},
            onClose: () {},
          ),
          locale: const Locale('ar'),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(tester.takeException(), isNull);
    expect(find.text('الصفحات'), findsOneWidget);
    await tester.tap(find.text('الصفحات'));
    await tester.pumpAndSettle();
    expect(
      find.byKey(const ValueKey('reader-page-thumbnail-1')),
      findsOneWidget,
    );
  });
}
