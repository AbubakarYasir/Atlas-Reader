import 'dart:async';
import 'package:flutter/material.dart';

import '../../core/accessibility/accessibility_announcer.dart';

class RsvpSpeedReader extends StatefulWidget {
  const RsvpSpeedReader({super.key, required this.text, this.initialWpm = 350});

  final String text;
  final int initialWpm;

  @override
  State<RsvpSpeedReader> createState() => _RsvpSpeedReaderState();
}

class _RsvpSpeedReaderState extends State<RsvpSpeedReader> {
  late final List<String> _words;
  late int _wpm;
  int _currentIndex = 0;
  bool _isPlaying = false;
  Timer? _timer;
  double _fontSize = 44.0;
  Color _backgroundColor = const Color(0xFF1E1E1E);
  Color _textColor = Colors.white;

  @override
  void initState() {
    super.initState();
    _wpm = widget.initialWpm;
    _words = widget.text
        .split(RegExp(r'\s+'))
        .map((w) => w.trim())
        .where((w) => w.isNotEmpty)
        .toList();
    if (_words.isEmpty) {
      _words.add('No');
      _words.add('readable');
      _words.add('text');
      _words.add('found');
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  void _start() {
    if (_words.isEmpty) return;
    setState(() => _isPlaying = true);
    _scheduleNextWord();
    AccessibilityAnnouncer.announce(
      context,
      'Speed reader playing at $_wpm words per minute',
    );
  }

  void _pause() {
    _timer?.cancel();
    setState(() => _isPlaying = false);
    AccessibilityAnnouncer.announce(context, 'Speed reader paused');
  }

  void _scheduleNextWord() {
    _timer?.cancel();
    if (!_isPlaying || !mounted) return;

    if (_currentIndex >= _words.length - 1) {
      setState(() => _isPlaying = false);
      return;
    }

    final intervalMs = (60000 / _wpm).round();
    _timer = Timer(Duration(milliseconds: intervalMs), () {
      if (!mounted) return;
      setState(() {
        _currentIndex++;
      });
      _scheduleNextWord();
    });
  }

  void _skipWords(int delta) {
    setState(() {
      _currentIndex = (_currentIndex + delta).clamp(0, _words.length - 1);
    });
  }

  ({String prefix, String focal, String suffix}) _splitWord(String word) {
    if (word.isEmpty) return (prefix: '', focal: '', suffix: '');
    if (word.length == 1) return (prefix: '', focal: word, suffix: '');

    final focalIndex = (word.length * 0.35).floor().clamp(0, word.length - 1);
    final prefix = word.substring(0, focalIndex);
    final focal = word.substring(focalIndex, focalIndex + 1);
    final suffix = word.substring(focalIndex + 1);
    return (prefix: prefix, focal: focal, suffix: suffix);
  }

  @override
  Widget build(BuildContext context) {
    final currentWord = _words[_currentIndex];
    final parts = _splitWord(currentWord);
    final isRtl = Bidi.isRtlLanguage(currentWord);

    return Dialog.fullscreen(
      child: Scaffold(
        backgroundColor: _backgroundColor,
        appBar: AppBar(
          backgroundColor: Colors.transparent,
          elevation: 0,
          leading: IconButton(
            icon: const Icon(Icons.close, color: Colors.white70),
            onPressed: () => Navigator.pop(context),
          ),
          title: Text(
            'RSVP Speed Reader (${_currentIndex + 1} / ${_words.length} words)',
            style: const TextStyle(color: Colors.white70, fontSize: 14),
          ),
          actions: [
            IconButton(
              icon: const Icon(Icons.palette_outlined, color: Colors.white70),
              tooltip: 'Theme',
              onPressed: () {
                setState(() {
                  if (_backgroundColor == const Color(0xFF1E1E1E)) {
                    _backgroundColor = Colors.black; // OLED
                    _textColor = Colors.white;
                  } else if (_backgroundColor == Colors.black) {
                    _backgroundColor = const Color(0xFFF7F1E5); // Sepia
                    _textColor = const Color(0xFF2B2B2B);
                  } else {
                    _backgroundColor = const Color(0xFF1E1E1E);
                    _textColor = Colors.white;
                  }
                });
              },
            ),
          ],
        ),
        body: Column(
          children: [
            LinearProgressIndicator(
              value: (_currentIndex + 1) / _words.length,
              minHeight: 4,
              backgroundColor: Colors.white12,
              valueColor: const AlwaysStoppedAnimation<Color>(Colors.redAccent),
            ),
            Expanded(
              child: Center(
                child: Directionality(
                  textDirection: isRtl ? TextDirection.rtl : TextDirection.ltr,
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 24,
                      vertical: 32,
                    ),
                    child: RichText(
                      textAlign: TextAlign.center,
                      text: TextSpan(
                        style: TextStyle(
                          fontSize: _fontSize,
                          color: _textColor,
                          fontWeight: FontWeight.w500,
                          fontFamily: 'Segoe UI',
                        ),
                        children: [
                          TextSpan(text: parts.prefix),
                          TextSpan(
                            text: parts.focal,
                            style: const TextStyle(
                              color: Colors.redAccent,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                          TextSpan(text: parts.suffix),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ),
            Container(
              padding: const EdgeInsets.all(20),
              decoration: BoxDecoration(
                color: Colors.black26,
                border: Border(
                  top: BorderSide(color: Colors.white.withAlpha(20)),
                ),
              ),
              child: Column(
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      IconButton(
                        icon: const Icon(Icons.replay_10, color: Colors.white),
                        iconSize: 28,
                        tooltip: 'Rewind 10 words',
                        onPressed: () => _skipWords(-10),
                      ),
                      const SizedBox(width: 16),
                      FilledButton.icon(
                        style: FilledButton.styleFrom(
                          backgroundColor: Colors.redAccent,
                          padding: const EdgeInsets.symmetric(
                            horizontal: 24,
                            vertical: 12,
                          ),
                        ),
                        icon: Icon(_isPlaying ? Icons.pause : Icons.play_arrow),
                        label: Text(_isPlaying ? 'PAUSE' : 'START'),
                        onPressed: _isPlaying ? _pause : _start,
                      ),
                      const SizedBox(width: 16),
                      IconButton(
                        icon: const Icon(Icons.forward_10, color: Colors.white),
                        iconSize: 28,
                        tooltip: 'Forward 10 words',
                        onPressed: () => _skipWords(10),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  Row(
                    children: [
                      const Icon(Icons.speed, color: Colors.white70, size: 20),
                      const SizedBox(width: 8),
                      Text(
                        '$_wpm WPM',
                        style: const TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      Expanded(
                        child: Slider(
                          value: _wpm.toDouble(),
                          min: 200,
                          max: 800,
                          divisions: 12,
                          label: '$_wpm WPM',
                          onChanged: (val) {
                            setState(() => _wpm = val.round());
                            if (_isPlaying) _scheduleNextWord();
                          },
                        ),
                      ),
                    ],
                  ),
                  Row(
                    children: [
                      const Icon(
                        Icons.format_size,
                        color: Colors.white70,
                        size: 20,
                      ),
                      const SizedBox(width: 8),
                      Text(
                        '${_fontSize.round()} pt',
                        style: const TextStyle(
                          color: Colors.white70,
                          fontSize: 12,
                        ),
                      ),
                      Expanded(
                        child: Slider(
                          value: _fontSize,
                          min: 28,
                          max: 72,
                          divisions: 22,
                          onChanged: (val) => setState(() => _fontSize = val),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class Bidi {
  static bool isRtlLanguage(String text) {
    for (final char in text.runes) {
      if ((char >= 0x0600 && char <= 0x06FF) ||
          (char >= 0x0750 && char <= 0x077F) ||
          (char >= 0x08A0 && char <= 0x08FF) ||
          (char >= 0xFB50 && char <= 0xFDFF) ||
          (char >= 0xFE70 && char <= 0xFEFF) ||
          (char >= 0x0590 && char <= 0x05FF)) {
        return true;
      }
    }
    return false;
  }
}
