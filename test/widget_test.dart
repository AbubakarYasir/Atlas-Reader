import 'package:drift/native.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:atlas_poc/database.dart';
import 'package:atlas_poc/main.dart';

void main() {
  testWidgets('shows initial waiting status', (WidgetTester tester) async {
    database = AppDatabase.forTesting(NativeDatabase.memory());
    addTearDown(database.close);

    await tester.pumpWidget(const MyApp());

    expect(find.text('Atlas UEP PoC'), findsOneWidget);
    expect(find.text('Waiting'), findsOneWidget);
    expect(find.text('INJECT BOOKMARK'), findsOneWidget);
  });
}
