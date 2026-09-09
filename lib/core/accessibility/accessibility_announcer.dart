import 'package:flutter/material.dart';
import 'package:flutter/semantics.dart';

/// Announces important asynchronous outcomes to platform screen readers.
///
/// The visible status text should also use [AccessibilityStatus] so users who
/// prefer a screen reader get updates without moving keyboard focus.
class AccessibilityAnnouncer {
  const AccessibilityAnnouncer._();

  static void announce(BuildContext context, String message) {
    SemanticsService.sendAnnouncement(
      View.of(context),
      message,
      Directionality.of(context),
    );
  }
}

class AccessibilityStatus extends StatelessWidget {
  const AccessibilityStatus({super.key, required this.message});

  final String message;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      liveRegion: true,
      label: message,
      child: Text(message, textAlign: TextAlign.center),
    );
  }
}
