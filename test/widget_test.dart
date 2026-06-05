import 'package:flutter_test/flutter_test.dart';

import 'package:atlas_poc/main.dart';

void main() {
  testWidgets('shows initial waiting status', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    expect(find.text('Atlas UEP PoC'), findsOneWidget);
    expect(find.text('Waiting'), findsOneWidget);
    expect(find.text('INJECT BOOKMARK'), findsOneWidget);
  });
}
