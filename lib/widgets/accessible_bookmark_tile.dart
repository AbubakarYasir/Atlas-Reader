import 'package:flutter/material.dart';

/// A [ListTile] wrapped in a transparent [Material] so ink splashes and
/// backgrounds render correctly inside [ExpansionTile] and other parents
/// that do not provide a Material ancestor.
class AccessibleBookmarkTile extends StatelessWidget {
  const AccessibleBookmarkTile({
    super.key,
    required this.title,
    this.subtitle,
    this.leading,
    this.trailing,
    this.onTap,
    this.contentPadding,
    this.dense = false,
  });

  final Widget title;
  final Widget? subtitle;
  final Widget? leading;
  final Widget? trailing;
  final VoidCallback? onTap;
  final EdgeInsetsGeometry? contentPadding;
  final bool dense;

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: ListTile(
        dense: dense,
        contentPadding: contentPadding,
        leading: leading,
        title: title,
        subtitle: subtitle,
        trailing: trailing,
        onTap: onTap,
      ),
    );
  }
}

/// An [ExpansionTile] wrapped in a transparent [Material] for correct ink
/// and splash rendering when nested inside other expansion panels.
class AccessibleExpansionTile extends StatelessWidget {
  const AccessibleExpansionTile({
    super.key,
    this.leading,
    required this.title,
    this.subtitle,
    this.tilePadding,
    required this.children,
  });

  final Widget? leading;
  final Widget title;
  final Widget? subtitle;
  final EdgeInsetsGeometry? tilePadding;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: Theme(
        data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
        child: ExpansionTile(
          tilePadding: tilePadding,
          leading: leading,
          title: title,
          subtitle: subtitle,
          children: children,
        ),
      ),
    );
  }
}
