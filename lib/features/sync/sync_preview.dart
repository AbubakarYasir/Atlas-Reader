import 'package:flutter/material.dart';

import '../../sync_models.dart';
import '../../widgets/accessible_bookmark_tile.dart';

/// Read-only preview of the changes that will be written during a sync.
class SyncPreview extends StatelessWidget {
  const SyncPreview({super.key, required this.diffs});

  final List<BookmarkDiff> diffs;

  @override
  Widget build(BuildContext context) {
    final adds = diffs.where((diff) => diff.action == SyncAction.add).length;
    final deletes = diffs.length - adds;

    return Material(
      color: Colors.grey[200],
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(8),
        side: BorderSide(color: Colors.grey[400]!),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Sync Preview',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            Text('Pending adds: $adds • Pending deletes: $deletes'),
            const SizedBox(height: 8),
            ...diffs.map(
              (diff) => AccessibleBookmarkTile(
                dense: true,
                leading: Icon(
                  diff.action == SyncAction.add ? Icons.add : Icons.remove,
                  color: diff.action == SyncAction.add
                      ? Colors.green
                      : Colors.red,
                ),
                title: Text(diff.title),
                subtitle: diff.pageIndex == null
                    ? null
                    : Text('Page ${diff.pageIndex! + 1}'),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
