import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class RenameTreeItemIntent extends Intent {
  const RenameTreeItemIntent();
}

class DeleteTreeItemIntent extends Intent {
  const DeleteTreeItemIntent();
}

class NextTreeItemIntent extends Intent {
  const NextTreeItemIntent();
}

class PreviousTreeItemIntent extends Intent {
  const PreviousTreeItemIntent();
}

/// A [ListTile] with an ink surface, full screen-reader description, and
/// keyboard operations used by the bookmark outline.
class AccessibleBookmarkTile extends StatelessWidget {
  const AccessibleBookmarkTile({
    super.key,
    required this.title,
    this.subtitle,
    this.leading,
    this.trailing,
    this.onTap,
    this.onRename,
    this.onDelete,
    this.semanticLabel,
    this.semanticHint,
    this.contentPadding,
    this.dense = false,
  });

  final Widget title;
  final Widget? subtitle;
  final Widget? leading;
  final Widget? trailing;
  final VoidCallback? onTap;
  final VoidCallback? onRename;
  final VoidCallback? onDelete;
  final String? semanticLabel;
  final String? semanticHint;
  final EdgeInsetsGeometry? contentPadding;
  final bool dense;

  @override
  Widget build(BuildContext context) {
    return FocusableActionDetector(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.arrowDown): NextTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.arrowUp): PreviousTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.f2): RenameTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.delete): DeleteTreeItemIntent(),
      },
      actions: {
        NextTreeItemIntent: CallbackAction<NextTreeItemIntent>(
          onInvoke: (_) {
            FocusScope.of(context).nextFocus();
            return null;
          },
        ),
        PreviousTreeItemIntent: CallbackAction<PreviousTreeItemIntent>(
          onInvoke: (_) {
            FocusScope.of(context).previousFocus();
            return null;
          },
        ),
        RenameTreeItemIntent: CallbackAction<RenameTreeItemIntent>(
          onInvoke: (_) {
            onRename?.call();
            return null;
          },
        ),
        DeleteTreeItemIntent: CallbackAction<DeleteTreeItemIntent>(
          onInvoke: (_) {
            onDelete?.call();
            return null;
          },
        ),
      },
      child: Semantics(
        button: onTap != null || onRename != null || onDelete != null,
        label: semanticLabel,
        hint: semanticHint,
        child: ExcludeSemantics(
          excluding: semanticLabel != null,
          child: Material(
            color: Colors.transparent,
            child: ListTile(
              dense: dense,
              contentPadding: contentPadding,
              leading: ExcludeSemantics(child: leading ?? const SizedBox()),
              title: title,
              subtitle: subtitle,
              trailing: trailing,
              onTap: onTap,
            ),
          ),
        ),
      ),
    );
  }
}

/// An accessible expansion row. Right/Left expand or collapse its branch and
/// Up/Down continue through the surrounding outline in visual order.
class AccessibleExpansionTile extends StatefulWidget {
  const AccessibleExpansionTile({
    super.key,
    this.leading,
    required this.title,
    this.subtitle,
    this.tilePadding,
    required this.children,
    this.semanticLabel,
    this.semanticHint,
    this.onRename,
    this.onDelete,
  });

  final Widget? leading;
  final Widget title;
  final Widget? subtitle;
  final EdgeInsetsGeometry? tilePadding;
  final List<Widget> children;
  final String? semanticLabel;
  final String? semanticHint;
  final VoidCallback? onRename;
  final VoidCallback? onDelete;

  @override
  State<AccessibleExpansionTile> createState() =>
      _AccessibleExpansionTileState();
}

class _AccessibleExpansionTileState extends State<AccessibleExpansionTile> {
  final _controller = ExpansibleController();
  var _expanded = false;

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return FocusableActionDetector(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.arrowDown): NextTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.arrowUp): PreviousTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.arrowRight): ActivateIntent(),
        SingleActivator(LogicalKeyboardKey.arrowLeft): DismissIntent(),
        SingleActivator(LogicalKeyboardKey.f2): RenameTreeItemIntent(),
        SingleActivator(LogicalKeyboardKey.delete): DeleteTreeItemIntent(),
      },
      actions: {
        NextTreeItemIntent: CallbackAction<NextTreeItemIntent>(
          onInvoke: (_) {
            FocusScope.of(context).nextFocus();
            return null;
          },
        ),
        PreviousTreeItemIntent: CallbackAction<PreviousTreeItemIntent>(
          onInvoke: (_) {
            FocusScope.of(context).previousFocus();
            return null;
          },
        ),
        ActivateIntent: CallbackAction<ActivateIntent>(
          onInvoke: (_) {
            if (!_expanded) _controller.expand();
            return null;
          },
        ),
        DismissIntent: CallbackAction<DismissIntent>(
          onInvoke: (_) {
            if (_expanded) _controller.collapse();
            return null;
          },
        ),
        RenameTreeItemIntent: CallbackAction<RenameTreeItemIntent>(
          onInvoke: (_) {
            widget.onRename?.call();
            return null;
          },
        ),
        DeleteTreeItemIntent: CallbackAction<DeleteTreeItemIntent>(
          onInvoke: (_) {
            widget.onDelete?.call();
            return null;
          },
        ),
      },
      child: Material(
        color: Colors.transparent,
        child: Theme(
          data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
          child: Semantics(
            label: widget.semanticLabel,
            hint: widget.semanticHint,
            expanded: _expanded,
            child: ExpansionTile(
              controller: _controller,
              tilePadding: widget.tilePadding,
              leading: ExcludeSemantics(
                child: widget.leading ?? const SizedBox(),
              ),
              title: widget.title,
              subtitle: widget.subtitle,
              onExpansionChanged: (expanded) =>
                  setState(() => _expanded = expanded),
              children: widget.children,
            ),
          ),
        ),
      ),
    );
  }
}
