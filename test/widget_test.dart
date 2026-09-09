import 'package:drift/native.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/main.dart';

void main() {
  testWidgets('opens on the library hub with every primary destination', (
    WidgetTester tester,
  ) async {
    database = AppDatabase.forTesting(NativeDatabase.memory());
    addTearDown(database.close);

    await tester.pumpWidget(const MyApp());

    expect(find.text('Library'), findsOneWidget);
    expect(find.text('Search title, author, file...'), findsOneWidget);
    expect(find.text('Open PDF'), findsOneWidget);
    expect(
      find.byTooltip('Quick open and bookmark search (Ctrl+K)'),
      findsOneWidget,
    );

    await tester.tap(find.byIcon(Icons.menu));
    await tester.pumpAndSettle();
    for (final destination in const [
      'Library',
      'Recents',
      'Bookmarks',
      'Favorites',
      'Folders',
      'Settings',
    ]) {
      expect(find.text(destination), findsWidgets);
    }
  });
}
