import 'package:atlas_poc/widgets/accessible_bookmark_tile.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('F2 and Delete invoke bookmark tree operations', (tester) async {
    var renamed = false;
    var deleted = false;

    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: AccessibleBookmarkTile(
            title: const Text('مقدمة Introduction'),
            semanticLabel: 'مقدمة Introduction, level 1, bookmark, page 4',
            onRename: () => renamed = true,
            onDelete: () => deleted = true,
          ),
        ),
      ),
    );

    await tester.sendKeyEvent(LogicalKeyboardKey.tab);
    await tester.sendKeyEvent(LogicalKeyboardKey.f2);
    await tester.sendKeyEvent(LogicalKeyboardKey.delete);

    expect(renamed, isTrue);
    expect(deleted, isTrue);
    expect(
      tester.getSemantics(find.text('مقدمة Introduction')),
      matchesSemantics(
        label: 'مقدمة Introduction, level 1, bookmark, page 4',
        isButton: true,
        isFocusable: true,
        hasFocusAction: true,
      ),
    );
  });
}
