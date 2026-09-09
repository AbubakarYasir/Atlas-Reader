import 'package:flutter/material.dart';

class ReaderScrubBar extends StatelessWidget {
  const ReaderScrubBar({
    super.key,
    required this.currentPage,
    required this.pageCount,
    required this.onPageChanged,
    this.chapterPages = const {},
    this.isBasicMode = true,
    this.onTapJumper,
  });

  final int currentPage;
  final int pageCount;
  final ValueChanged<int> onPageChanged;
  final Set<int> chapterPages;
  final bool isBasicMode;
  final VoidCallback? onTapJumper;

  @override
  Widget build(BuildContext context) {
    if (pageCount <= 0) return const SizedBox.shrink();

    final progress = (currentPage / pageCount).clamp(0.0, 1.0);

    if (isBasicMode) {
      return Container(
        height: 24,
        padding: const EdgeInsets.symmetric(horizontal: 16),
        color: Colors.black.withAlpha(20),
        child: Row(
          children: [
            Expanded(
              child: ClipRRect(
                borderRadius: BorderRadius.circular(2),
                child: LinearProgressIndicator(
                  value: progress,
                  minHeight: 3,
                  backgroundColor: Colors.grey.withAlpha(40),
                ),
              ),
            ),
            const SizedBox(width: 8),
            InkWell(
              onTap: onTapJumper,
              child: Text(
                '$currentPage / $pageCount',
                style: const TextStyle(fontSize: 10, fontWeight: FontWeight.w500),
              ),
            ),
          ],
        ),
      );
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(top: BorderSide(color: Theme.of(context).dividerColor.withAlpha(50))),
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            children: [
              IconButton(
                icon: const Icon(Icons.navigate_before, size: 20),
                tooltip: 'Previous Page',
                onPressed: currentPage > 1 ? () => onPageChanged(currentPage - 1) : null,
              ),
              Expanded(
                child: Stack(
                  alignment: Alignment.center,
                  children: [
                    SliderTheme(
                      data: SliderTheme.of(context).copyWith(
                        trackHeight: 4,
                        thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 6),
                        overlayShape: const RoundSliderOverlayShape(overlayRadius: 12),
                      ),
                      child: Slider(
                        value: currentPage.toDouble().clamp(1.0, pageCount.toDouble()),
                        min: 1.0,
                        max: pageCount.toDouble(),
                        onChanged: (val) => onPageChanged(val.round()),
                      ),
                    ),
                    if (chapterPages.isNotEmpty)
                      Positioned.fill(
                        child: IgnorePointer(
                          child: CustomPaint(
                            painter: _ChapterTicksPainter(
                              chapterPages: chapterPages,
                              pageCount: pageCount,
                              color: Theme.of(context).colorScheme.primary.withAlpha(120),
                            ),
                          ),
                        ),
                      ),
                  ],
                ),
              ),
              IconButton(
                icon: const Icon(Icons.navigate_next, size: 20),
                tooltip: 'Next Page',
                onPressed: currentPage < pageCount ? () => onPageChanged(currentPage + 1) : null,
              ),
              InkWell(
                onTap: onTapJumper,
                borderRadius: BorderRadius.circular(4),
                child: Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
                  child: Text(
                    '$currentPage / $pageCount (${(progress * 100).round()}%)',
                    style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold),
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _ChapterTicksPainter extends CustomPainter {
  const _ChapterTicksPainter({
    required this.chapterPages,
    required this.pageCount,
    required this.color,
  });

  final Set<int> chapterPages;
  final int pageCount;
  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    if (pageCount <= 1) return;
    final paint = Paint()
      ..color = color
      ..strokeWidth = 2.0;

    for (final page in chapterPages) {
      if (page >= 1 && page <= pageCount) {
        final fraction = (page - 1) / (pageCount - 1);
        final dx = fraction * size.width;
        canvas.drawLine(Offset(dx, size.height / 2 - 4), Offset(dx, size.height / 2 + 4), paint);
      }
    }
  }

  @override
  bool shouldRepaint(covariant _ChapterTicksPainter oldDelegate) {
    return oldDelegate.chapterPages != chapterPages ||
        oldDelegate.pageCount != pageCount ||
        oldDelegate.color != color;
  }
}
