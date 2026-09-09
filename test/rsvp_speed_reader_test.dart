import 'package:atlas_poc/features/speed_reading/rsvp_speed_reader.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('renders Arabic text and exposes speed controls', (tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: RsvpSpeedReader(text: 'بسم الله الرحمن الرحيم', initialWpm: 350),
      ),
    );

    expect(
      find.byWidgetPredicate(
        (widget) =>
            widget is RichText && widget.text.toPlainText().contains('بسم'),
      ),
      findsOneWidget,
    );
    expect(find.text('350 WPM'), findsOneWidget);
    expect(find.byTooltip('Forward 10 words'), findsOneWidget);
  });
}
