import 'package:atlas_poc/features/reader/reader_models.dart';
import 'package:atlas_poc/features/reader/reader_scrub_bar.dart';
import 'package:atlas_poc/features/reader/reader_settings_dialog.dart';
import 'package:atlas_poc/features/reader/reader_thumbnail_jumper.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test(
    'Stage 8: ReaderPreferences supports reading modes, themes, offsets, and cropping',
    () {
      const prefs = ReaderPreferences();
      expect(prefs.mode, ReadingMode.continuousVertical);
      expect(prefs.theme, ReaderThemeMode.day);
      expect(prefs.isResearchMode, isFalse);
      expect(prefs.pageOffset, 0);
      expect(prefs.brightness, 1.0);
      expect(prefs.marginCrop, 0.0);

      final updated = prefs.copyWith(
        mode: ReadingMode.singlePage,
        theme: ReaderThemeMode.warmParchment,
        isResearchMode: true,
        pageOffset: -12,
        brightness: 0.8,
        marginCrop: 0.15,
      );

      expect(updated.mode, ReadingMode.singlePage);
      expect(updated.theme, ReaderThemeMode.warmParchment);
      expect(updated.isResearchMode, isTrue);
      expect(updated.pageOffset, -12);
      expect(updated.brightness, 0.8);
      expect(updated.marginCrop, 0.15);
    },
  );

  testWidgets(
    'Stage 8: ReaderThumbnailJumper displays page grid and direct jump field',
    (tester) async {
      int? selectedPage;

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: ReaderThumbnailJumper(
              currentPage: 5,
              pageCount: 20,
              chapterPages: const {1, 10},
              bookmarkedPages: const {5},
              pageOffset: -2,
              onPageSelected: (page) => selectedPage = page,
            ),
          ),
        ),
      );

      expect(find.text('Go to Page'), findsOneWidget);
      expect(find.text('5'), findsAtLeastNWidgets(1));
      expect(find.byType(TextField), findsOneWidget);

      await tester.enterText(find.byType(TextField), '12');
      await tester.tap(find.text('Jump'));
      await tester.pump();

      expect(selectedPage, 12);
    },
  );

  testWidgets('Stage 8: ReaderScrubBar reflects progress, mode, and jumps', (
    tester,
  ) async {
    int? navigatedPage;

    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: ReaderScrubBar(
            currentPage: 15,
            pageCount: 30,
            chapterPages: const {1, 10, 20},
            isBasicMode: false,
            onPageChanged: (page) => navigatedPage = page,
          ),
        ),
      ),
    );

    expect(find.text('15 / 30 (50%)'), findsOneWidget);
    expect(find.byType(Slider), findsOneWidget);

    final sliderBounds = tester.getRect(find.byType(Slider));
    await tester.tapAt(
      Offset(
        sliderBounds.left + sliderBounds.width * .8,
        sliderBounds.center.dy,
      ),
    );
    await tester.pump();
    expect(navigatedPage, greaterThan(15));

    await tester.tap(find.byIcon(Icons.navigate_next));
    await tester.pump();

    expect(navigatedPage, 16);
  });

  testWidgets(
    'Stage 8: ReaderSettingsDialog updates theme, mode, and margin cropping',
    (tester) async {
      var prefs = const ReaderPreferences();

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: ReaderSettingsDialog(
              preferences: prefs,
              onChanged: (updated) => prefs = updated,
            ),
          ),
        ),
      );

      expect(find.text('Reading & Display Settings'), findsOneWidget);
      expect(find.text('Display Theme'), findsOneWidget);
      expect(find.text('White Margin Cropping'), findsOneWidget);

      await tester.tap(find.text('Sepia'));
      await tester.pump();
      expect(prefs.theme, ReaderThemeMode.warmParchment);

      await tester.tap(find.text('Single Page'));
      await tester.pump();
      expect(prefs.mode, ReadingMode.singlePage);
    },
  );
}
