# ADR-0010 — Screen-reader interfaces on Qt 6.10 (Qt Quick private API)

- **Status:** Proposed (implemented in the N5 repair pass; becomes Accepted with `N5 PASS`)
- **Date:** 2026-11-07
- **Checkpoint:** N5 repair pass (closure ledger N4-04)

## Context

Windows UI Automation, which Narrator reads, showed three defects in Atlas on
Qt 6.10.3. None could be fixed from QML alone:

1. **Tree open/closed state.** QML can set an expanded state only from Qt 6.12
   (`Accessible.expanded`). The 6.10 Windows bridge already maps a tree item's
   `expandable/expanded` state to the ExpandCollapse pattern; QML cannot set it.
2. **Focus inside lists.** Qt Quick reports a focused ListView or GridView as
   the focused element, never its current item. Narrator therefore said
   nothing while the arrow keys moved through bookmarks or books.
3. **Dialog names.** When a popup opens, focus goes to the popup's own
   container. Qt names that container only if accessibility was already on
   when the popup was created, so Narrator often said just "dialog".

Moving to Qt 6.12 fixes only the first defect. It also re-opens the N2 engine
qualification that is pinned to 6.10.3 ([ADR-0003](ADR-0003-n1-qt-toolchain-pin.md),
[ADR-0004](ADR-0004-pdf-engine-responsibilities.md)).

## Decision

Keep Qt 6.10.3. Add `src/app/accessibility/TreeItemAccessible.*`, installed
before any QML is created. It is an accessibility interface factory built on
Qt Quick's `QAccessibleQuickItem` (private header, `Qt6::QuickPrivate`):

- A row that declares `a11yTreeRow: true` reports native
  `expandable/expanded/collapsed`. Its show-menu action, which UI Automation
  Expand and Collapse call, runs the row's `a11yToggle()`.
- A focused `QQuickItemView` reports its current item as the focused child and
  sends a focus event when the current item changes.
- A popup container without a name takes the name of the Dialog it contains.

The interfaces keep the Locale attribute that Qt Quick Controls provide,
because Narrator uses it to choose an Arabic or Urdu voice. Qt Quick's own
rule for which items are accessible is unchanged.

## Alternatives considered

- **Upgrade to Qt 6.12.** This fixes only the tree state and re-qualifies the
  pinned PDF engine stack. Deferred to a planned toolchain update.
- **Put the state in the description** (the previous approach). Narrator read
  it as text, but it offered no expand/collapse command and still said nothing
  inside lists.
- **Move focus into each row or dialog from QML.** This splits focus between
  the list and its rows and breaks the panel's key handling. It would also need
  a change in every dialog and would still leave the container unnamed.

## Consequences

- Private API is tied to the Qt version. A Qt upgrade must rebuild and
  rerun `atlas_tree_item_accessible` (which fails without the factory), the
  accessibility sweep and the Windows UI Automation review. If the private
  class changes, the factory is the only file to adapt.
- On Qt 6.12, rows can use `Accessible.expanded` and the tree part of the
  factory can be removed. The list-focus and dialog-name parts stay until Qt
  fixes those defects.
- Evidence: `tests/app/tree_item_accessible_tests.cpp`,
  `tools/a11y/check_accessibility.py` (focus checks),
  `tools/a11y/windows-uia-review.ps1` (real keys, EN/AR/UR, Windows CI).

## Revisit conditions

- The Qt upgrade to 6.12 or later (use `Accessible.expanded`; recheck the
  list-focus and popup-name defects).
- A failure of `atlas_tree_item_accessible` or the Windows UI Automation
  review after any Qt change.

## Related

[DESIGN.md](../DESIGN.md) accessibility rules · [QUALITY.md](../QUALITY.md) ·
[closure ledger](../baselines/N1_N5_CLOSURE.md) N4-04 ·
[ADR-0003](ADR-0003-n1-qt-toolchain-pin.md)
