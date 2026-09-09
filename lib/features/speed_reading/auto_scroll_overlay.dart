import 'dart:async';
import 'package:flutter/material.dart';

import '../../core/accessibility/accessibility_announcer.dart';

class AutoScrollOverlay extends StatefulWidget {
  const AutoScrollOverlay({
    super.key,
    required this.onScrollTick,
    required this.onClose,
  });

  /// Callback called on every tick with the scroll delta (positive = down, negative = up)
  final ValueChanged<double> onScrollTick;
  final VoidCallback onClose;

  @override
  State<AutoScrollOverlay> createState() => _AutoScrollOverlayState();
}

class _AutoScrollOverlayState extends State<AutoScrollOverlay> {
  bool _isPlaying = true;
  double _speedPxPerSec = 40.0;
  bool _scrollDown = true;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _startTimer();
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  void _startTimer() {
    _timer?.cancel();
    const tickIntervalMs = 50;
    _timer = Timer.periodic(const Duration(milliseconds: tickIntervalMs), (_) {
      if (!_isPlaying || !mounted) return;
      final delta =
          (_speedPxPerSec * (tickIntervalMs / 1000.0)) *
          (_scrollDown ? 1.0 : -1.0);
      widget.onScrollTick(delta);
    });
  }

  void _togglePlayPause() {
    setState(() => _isPlaying = !_isPlaying);
    AccessibilityAnnouncer.announce(
      context,
      _isPlaying
          ? 'Auto-scroll resumed at ${_speedPxPerSec.round()} pixels per second'
          : 'Auto-scroll paused',
    );
  }

  @override
  Widget build(BuildContext context) {
    return Material(
      elevation: 8,
      borderRadius: BorderRadius.circular(24),
      color: Theme.of(
        context,
      ).colorScheme.surfaceContainerHighest.withAlpha(230),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(24),
          border: Border.all(
            color: Theme.of(context).dividerColor.withAlpha(80),
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            IconButton(
              icon: Icon(
                _isPlaying
                    ? Icons.pause_circle_filled
                    : Icons.play_circle_filled,
              ),
              iconSize: 30,
              color: Theme.of(context).colorScheme.primary,
              tooltip: _isPlaying ? 'Pause Auto-scroll' : 'Resume Auto-scroll',
              onPressed: _togglePlayPause,
            ),
            IconButton(
              icon: Icon(
                _scrollDown ? Icons.arrow_downward : Icons.arrow_upward,
              ),
              iconSize: 20,
              tooltip: _scrollDown ? 'Scrolling Down' : 'Scrolling Up',
              onPressed: () => setState(() => _scrollDown = !_scrollDown),
            ),
            const SizedBox(width: 4),
            Text(
              '${_speedPxPerSec.round()} px/s',
              style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold),
            ),
            SizedBox(
              width: 140,
              child: Slider(
                value: _speedPxPerSec,
                min: 10,
                max: 150,
                divisions: 14,
                onChanged: (val) => setState(() => _speedPxPerSec = val),
              ),
            ),
            IconButton(
              icon: const Icon(Icons.close, size: 18),
              tooltip: 'Stop Auto-scroll',
              onPressed: () {
                _timer?.cancel();
                widget.onClose();
              },
            ),
          ],
        ),
      ),
    );
  }
}
