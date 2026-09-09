import 'dart:convert';

import 'bookmark_tree.dart';
import 'database.dart';

/// Serialized bookmark outline state stored in [FileSnapshots.lastKnownState].
class FileSnapshotState {
  FileSnapshotState({required this.version, required this.pathKeys});

  static const int currentVersion = 2;

  final int version;
  final List<String> pathKeys;

  /// Builds a v2 snapshot from database bookmarks using full hierarchical paths.
  factory FileSnapshotState.fromBookmarks(List<Bookmark> bookmarks) {
    final byId = BookmarkTree.indexById(bookmarks);
    final keys =
        bookmarks
            .map(
              (bookmark) => BookmarkTree.pathKey(
                BookmarkTree.pathForBookmark(bookmark, byId),
              ),
            )
            .toList()
          ..sort();
    return FileSnapshotState(version: currentVersion, pathKeys: keys);
  }

  /// Decodes stored JSON, including legacy flat title lists (v1).
  factory FileSnapshotState.decode(String jsonState) {
    final decoded = jsonDecode(jsonState);

    if (decoded is Map<String, dynamic>) {
      final version = decoded['version'] as int? ?? 1;
      if (version >= currentVersion) {
        return FileSnapshotState(
          version: currentVersion,
          pathKeys: List<String>.from(decoded['paths'] as List? ?? const []),
        );
      }
    }

    if (decoded is List) {
      return FileSnapshotState(
        version: 1,
        pathKeys: decoded.map((entry) => entry.toString()).toList(),
      );
    }

    throw FormatException('Unsupported file snapshot format: $jsonState');
  }

  String encode() {
    return jsonEncode({'version': currentVersion, 'paths': pathKeys});
  }
}
