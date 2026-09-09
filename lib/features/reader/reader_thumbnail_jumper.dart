import 'package:flutter/material.dart';

class ReaderThumbnailJumper extends StatefulWidget {
  const ReaderThumbnailJumper({
    super.key,
    required this.currentPage,
    required this.pageCount,
    required this.onPageSelected,
    this.bookmarkedPages = const {},
    this.chapterPages = const {},
    this.pageOffset = 0,
  });

  final int currentPage;
  final int pageCount;
  final ValueChanged<int> onPageSelected;
  final Set<int> bookmarkedPages;
  final Set<int> chapterPages;
  final int pageOffset;

  @override
  State<ReaderThumbnailJumper> createState() => _ReaderThumbnailJumperState();
}

class _ReaderThumbnailJumperState extends State<ReaderThumbnailJumper> {
  late final TextEditingController _jumpController;
  late final ScrollController _scrollController;

  @override
  void initState() {
    super.initState();
    _jumpController = TextEditingController(text: '${widget.currentPage}');
    _scrollController = ScrollController();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients && widget.pageCount > 0) {
        final targetIndex = (widget.currentPage - 1).clamp(
          0,
          widget.pageCount - 1,
        );
        final itemWidth = 100.0;
        final targetOffset = (targetIndex * itemWidth) - 150.0;
        _scrollController.jumpTo(
          targetOffset.clamp(0.0, _scrollController.position.maxScrollExtent),
        );
      }
    });
  }

  @override
  void dispose() {
    _jumpController.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  void _submitJump() {
    final parsed = int.tryParse(_jumpController.text.trim());
    if (parsed != null && parsed >= 1 && parsed <= widget.pageCount) {
      widget.onPageSelected(parsed);
      Navigator.pop(context);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 650, maxHeight: 480),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    'Go to Page',
                    style: Theme.of(context).textTheme.titleLarge?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  IconButton(
                    icon: const Icon(Icons.close),
                    onPressed: () => Navigator.pop(context),
                  ),
                ],
              ),
              const SizedBox(height: 12),
              Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _jumpController,
                      keyboardType: TextInputType.number,
                      autofocus: true,
                      decoration: InputDecoration(
                        labelText:
                            'Enter page number (1 - ${widget.pageCount})',
                        border: const OutlineInputBorder(),
                        suffixIcon: IconButton(
                          icon: const Icon(Icons.arrow_forward),
                          onPressed: _submitJump,
                        ),
                      ),
                      onSubmitted: (_) => _submitJump(),
                    ),
                  ),
                  const SizedBox(width: 12),
                  FilledButton(
                    onPressed: _submitJump,
                    child: const Text('Jump'),
                  ),
                ],
              ),
              const SizedBox(height: 16),
              Align(
                alignment: Alignment.centerLeft,
                child: Text(
                  'Surrounding Pages',
                  style: Theme.of(context).textTheme.titleSmall?.copyWith(
                    color: Theme.of(context).hintColor,
                  ),
                ),
              ),
              const SizedBox(height: 8),
              Expanded(
                child: widget.pageCount <= 0
                    ? const Center(child: Text('No pages available'))
                    : GridView.builder(
                        controller: _scrollController,
                        gridDelegate:
                            const SliverGridDelegateWithFixedCrossAxisCount(
                              crossAxisCount: 5,
                              childAspectRatio: 0.72,
                              crossAxisSpacing: 10,
                              mainAxisSpacing: 10,
                            ),
                        itemCount: widget.pageCount,
                        itemBuilder: (context, index) {
                          final pageNum = index + 1;
                          final isCurrent = pageNum == widget.currentPage;
                          final isBookmarked = widget.bookmarkedPages.contains(
                            pageNum,
                          );
                          final isChapter = widget.chapterPages.contains(
                            pageNum,
                          );
                          final printedNum = pageNum + widget.pageOffset;

                          return InkWell(
                            onTap: () {
                              widget.onPageSelected(pageNum);
                              Navigator.pop(context);
                            },
                            borderRadius: BorderRadius.circular(8),
                            child: Container(
                              decoration: BoxDecoration(
                                color: isCurrent
                                    ? Theme.of(
                                        context,
                                      ).colorScheme.primaryContainer
                                    : Theme.of(
                                        context,
                                      ).colorScheme.surfaceContainerHigh,
                                borderRadius: BorderRadius.circular(8),
                                border: Border.all(
                                  color: isCurrent
                                      ? Theme.of(context).colorScheme.primary
                                      : isChapter
                                      ? Colors.amber
                                      : Theme.of(
                                          context,
                                        ).dividerColor.withAlpha(50),
                                  width: isCurrent ? 2 : 1,
                                ),
                              ),
                              child: Stack(
                                children: [
                                  Center(
                                    child: Column(
                                      mainAxisAlignment:
                                          MainAxisAlignment.center,
                                      children: [
                                        Icon(
                                          Icons.article_outlined,
                                          size: 28,
                                          color: isCurrent
                                              ? Theme.of(
                                                  context,
                                                ).colorScheme.primary
                                              : Theme.of(context).hintColor,
                                        ),
                                        const SizedBox(height: 4),
                                        Text(
                                          '$pageNum',
                                          style: TextStyle(
                                            fontWeight: isCurrent
                                                ? FontWeight.bold
                                                : FontWeight.normal,
                                            fontSize: 13,
                                          ),
                                        ),
                                        if (widget.pageOffset != 0)
                                          Text(
                                            '($printedNum)',
                                            style: TextStyle(
                                              fontSize: 10,
                                              color: Theme.of(
                                                context,
                                              ).hintColor,
                                            ),
                                          ),
                                      ],
                                    ),
                                  ),
                                  if (isBookmarked)
                                    const Positioned(
                                      top: 4,
                                      right: 4,
                                      child: Icon(
                                        Icons.bookmark,
                                        size: 14,
                                        color: Colors.blueAccent,
                                      ),
                                    ),
                                  if (isChapter)
                                    const Positioned(
                                      top: 4,
                                      left: 4,
                                      child: Icon(
                                        Icons.flag_outlined,
                                        size: 12,
                                        color: Colors.amber,
                                      ),
                                    ),
                                ],
                              ),
                            ),
                          );
                        },
                      ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
