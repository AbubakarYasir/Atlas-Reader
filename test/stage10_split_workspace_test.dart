import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/features/workspace/research_scratchpad.dart';
import 'package:drift/native.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late AppDatabase db;

  setUp(() {
    db = AppDatabase.forTesting(NativeDatabase.memory());
  });

  tearDown(() async {
    await db.close();
  });

  test('Stage 10: Reading session tabs are persisted and restored from SQLite', () async {
    final tabsToSave = [
      (filePath: 'C:\\Books\\Arabic_Original.pdf', pageNumber: 15, isActive: true),
      (filePath: 'C:\\Books\\English_Translation.pdf', pageNumber: 22, isActive: false),
      (filePath: 'C:\\Books\\Commentary.pdf', pageNumber: 5, isActive: false),
    ];

    await db.saveReadingSessionTabs(tabsToSave);

    final restored = await db.getReadingSessionTabs();
    expect(restored, hasLength(3));
    expect(restored[0].filePath, 'C:\\Books\\Arabic_Original.pdf');
    expect(restored[0].pageNumber, 15);
    expect(restored[0].isActive, isTrue);

    expect(restored[1].filePath, 'C:\\Books\\English_Translation.pdf');
    expect(restored[1].pageNumber, 22);
    expect(restored[1].isActive, isFalse);

    // Save scratchpad note and retrieve
    await db.saveScratchpadNote(
      'C:\\Books\\Arabic_Original.pdf',
      '# Chapter 1 Analysis\nKey textual comparison with manuscript A.',
    );

    final savedNote = await db.getScratchpadNote('C:\\Books\\Arabic_Original.pdf');
    expect(savedNote, contains('# Chapter 1 Analysis'));
    expect(savedNote, contains('Key textual comparison with manuscript A.'));
  });

  testWidgets('Stage 10: ResearchScratchpad supports formatting, quotes, and page links', (tester) async {
    int? jumpedPage;

    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: ResearchScratchpad(
            filePath: 'C:\\Books\\Philosophy.pdf',
            database: db,
            currentPage: 42,
            selectedText: 'I think, therefore I am',
            onJumpToPage: (page) => jumpedPage = page,
          ),
        ),
      ),
    );

    expect(find.text('Research Scratchpad'), findsOneWidget);
    expect(find.text('Quote p.42'), findsOneWidget);

    // Click quote button to insert quote with link
    await tester.tap(find.text('Quote p.42'));
    await tester.pump();

    final textField = tester.widget<TextField>(find.byType(TextField));
    expect(textField.controller?.text, contains('I think, therefore I am'));
    expect(textField.controller?.text, contains('[Page 42](atlas://page/42)'));

    // Toggle preview mode
    await tester.tap(find.byIcon(Icons.remove_red_eye_outlined));
    await tester.pump();

    expect(find.text('Page 42'), findsOneWidget);
    await tester.tap(find.text('Page 42'));
    await tester.pump();

    expect(jumpedPage, 42);
  });
}
