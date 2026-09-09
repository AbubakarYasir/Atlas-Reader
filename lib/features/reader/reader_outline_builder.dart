import 'reader_outline_sidebar.dart';

class ReaderOutlineRecord {
  const ReaderOutlineRecord({
    required this.path,
    required this.pageNumber,
    this.isUserBookmark = false,
  });

  final List<String> path;
  final int? pageNumber;
  final bool isUserBookmark;
}

/// Builds a nested, full-path keyed tree. Same-titled chapters under different
/// parents remain distinct, while a local bookmark already embedded in the PDF
/// is merged with its matching outline node.
List<ReaderOutlineItem> buildReaderOutline(
  Iterable<ReaderOutlineRecord> records,
) {
  final roots = <_MutableOutlineNode>[];
  final nodesByPath = <String, _MutableOutlineNode>{};

  for (final record in records) {
    if (record.path.isEmpty) continue;
    final cleanPath = record.path
        .map((part) => part.trim())
        .where((part) => part.isNotEmpty)
        .toList(growable: false);
    if (cleanPath.isEmpty) continue;

    _MutableOutlineNode? parent;
    for (var depth = 0; depth < cleanPath.length; depth++) {
      final partialPath = cleanPath.take(depth + 1).join('\u001f');
      final isLeaf = depth == cleanPath.length - 1;
      final page = isLeaf ? record.pageNumber : null;
      final key = partialPath;
      var node = nodesByPath[key];
      if (node == null) {
        node = _MutableOutlineNode(
          title: cleanPath[depth],
          path: cleanPath.take(depth + 1).toList(growable: false),
          pageNumber: page,
        );
        nodesByPath[key] = node;
        if (parent == null) {
          roots.add(node);
        } else {
          parent.children.add(node);
        }
      } else if (isLeaf && node.pageNumber == null) {
        node.pageNumber = page;
      }
      if (isLeaf && record.isUserBookmark) node.isUserBookmark = true;
      parent = node;
    }
  }

  return roots.map((node) => node.freeze()).toList(growable: false);
}

class _MutableOutlineNode {
  _MutableOutlineNode({
    required this.title,
    required this.path,
    required this.pageNumber,
  });

  final String title;
  final List<String> path;
  int? pageNumber;
  bool isUserBookmark = false;
  final List<_MutableOutlineNode> children = [];

  ReaderOutlineItem freeze() => ReaderOutlineItem(
    title: title,
    pageNumber: pageNumber,
    breadcrumb: path.join(' > '),
    isUserBookmark: isUserBookmark,
    children: children.map((child) => child.freeze()).toList(growable: false),
  );
}
