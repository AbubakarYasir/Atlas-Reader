import 'dart:async';
import 'dart:io';

import 'package:logging/logging.dart';

/// Watches registered library folders for PDF add/rename/move/delete events
/// and reports them through a debounced callback so a rescan covers batches
/// of macOS/Windows filesystem notifications with one pass.
class LibraryFileWatcher {
  LibraryFileWatcher({required this.onChange});

  static final _log = Logger('LibraryFileWatcher');
  final void Function() onChange;

  final Map<String, StreamSubscription<FileSystemEvent>> _subscriptions = {};
  Timer? _debounce;

  /// Starts watching [folderPath] recursively. Idempotent per path.
  void watch(String folderPath) {
    if (_subscriptions.containsKey(folderPath)) return;

    try {
      final subscription = Directory(folderPath)
          .watch(recursive: true, events: FileSystemEvent.all)
          .listen((event) => _handleEvent(event));
      _subscriptions[folderPath] = subscription;
      _log.fine('[WATCH] Watching $folderPath');
    } catch (e) {
      _log.warning('[WATCH] Could not watch $folderPath: $e');
    }
  }

  /// Stops watching [folderPath].
  void unwatch(String folderPath) {
    final subscription = _subscriptions.remove(folderPath);
    if (subscription != null) {
      subscription.cancel();
      _log.fine('[WATCH] Stopped watching $folderPath');
    }
  }

  void _handleEvent(FileSystemEvent event) {
    final path = event.path.toLowerCase();
    final isPdf = path.endsWith('.pdf');

    if (!isPdf &&
        event.type != FileSystemEvent.create &&
        event.type != FileSystemEvent.delete) {
      return;
    }

    _debounce?.cancel();
    _debounce = Timer(const Duration(milliseconds: 750), onChange);
  }

  /// Cancels every subscription. Safe to call more than once.
  void dispose() {
    _debounce?.cancel();
    _debounce = null;
    for (final subscription in _subscriptions.values) {
      subscription.cancel();
    }
    _subscriptions.clear();
  }
}
