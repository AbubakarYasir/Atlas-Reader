import 'package:logging/logging.dart';

import '../../core/file_system/document_file_system.dart';
import '../../core/file_system/windows_document_file_system.dart';
import '../../database.dart';
import 'library_file_watcher.dart';
import 'library_folder_scanner.dart';

/// Outcome of a library-folder rescan, used for user-facing status text.
class LibraryScanReport {
  const LibraryScanReport({
    required this.foldersScanned,
    required this.filesFound,
  });

  final int foldersScanned;
  final int filesFound;

  bool get isEmpty => filesFound == 0;
}

class LibraryScanProgress {
  const LibraryScanProgress({
    required this.folderPath,
    required this.indexed,
    required this.total,
  });

  final String folderPath;
  final int indexed;
  final int total;
}

/// Owns folder registration, background scanning, and file watching. The UI
/// calls addFolder/removeFolder/rescanAll and reacts to [onChanged] callbacks.
class LibraryFolderManager {
  LibraryFolderManager({
    required this._database,
    DocumentFileSystem? fileSystem,
  }) : _scanner = LibraryFolderScanner(
         fileSystem: fileSystem ?? const WindowsDocumentFileSystem(),
       ) {
    _watcher = LibraryFileWatcher(onChange: _onWatchChange);
  }

  static final _log = Logger('LibraryFolderManager');
  final AppDatabase _database;
  final LibraryFolderScanner _scanner;
  late final LibraryFileWatcher _watcher;

  /// Called after any scan completes so UI can refresh its file list.
  void Function()? onChanged;
  void Function(LibraryScanProgress progress)? onProgress;
  Future<LibraryScanReport>? _activeScan;

  Future<void> dispose() {
    _watcher.dispose();
    return Future.value();
  }

  /// Registers a folder and starts watching it.
  Future<LibraryFolder> addFolder(String path) async {
    final folder = await _database.addLibraryFolder(path);
    _watcher.watch(folder.path);
    _log.info('[LIBRARY] Registered folder: $path');
    return folder;
  }

  /// Unregisters a folder and stops watching it. Its scanned files cascade.
  Future<void> removeFolder(int id, String path) async {
    await _database.removeLibraryFolder(id);
    _watcher.unwatch(path);
    _log.info('[LIBRARY] Removed folder: $path');
    onChanged?.call();
  }

  /// Re-registers every stored folder for watching. Safe to call at startup.
  Future<void> startWatching() async {
    final folders = await _database.getLibraryFolders();
    for (final folder in folders) {
      _watcher.watch(folder.path);
    }
  }

  /// Scans every registered folder and reconciles the results in the database.
  Future<LibraryScanReport> rescanAll({
    bool restartAfterCurrent = false,
  }) async {
    final active = _activeScan;
    if (active != null) {
      await active;
      if (!restartAfterCurrent) return active;
    }
    final operation = _performRescan();
    _activeScan = operation;
    try {
      return await operation;
    } finally {
      if (identical(_activeScan, operation)) _activeScan = null;
    }
  }

  Future<LibraryScanReport> _performRescan() async {
    var filesFound = 0;
    final folders = await _database.getLibraryFolders();
    for (final folder in folders) {
      final scanned = await _scanner.scanFolder(
        folder.path,
        onBatch: (batch, indexed, total) async {
          await _database.upsertLibraryFiles(
            folder.id,
            batch,
            removeMissing: false,
            reconcileRenames: false,
          );
          onProgress?.call(
            LibraryScanProgress(
              folderPath: folder.path,
              indexed: indexed,
              total: total,
            ),
          );
          onChanged?.call();
        },
      );
      filesFound += scanned.length;
      await _database.upsertLibraryFiles(folder.id, scanned);
    }
    _log.info(
      '[LIBRARY] Rescan complete: ${folders.length} folders, $filesFound files',
    );
    onChanged?.call();
    return LibraryScanReport(
      foldersScanned: folders.length,
      filesFound: filesFound,
    );
  }

  void _onWatchChange() {
    _log.fine('[LIBRARY] Watch change detected; rescannning');
    rescanAll();
  }
}
