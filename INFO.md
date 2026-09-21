# Atlas Reader Native — Current Project Notes

**State:** native successor bootstrap

**Current checkpoint:** N0 — Native repository bootstrap

**Primary target:** Windows 11

**Future targets:** Android → Linux → macOS → iOS/iPadOS

## What this repository is

This is the planned native successor to the existing Flutter Atlas Reader. It starts as a clean architecture rather than a code translation.

The original application remains the product-behavior and fixture reference until each native checkpoint is independently verified and accepted.

## Chosen foundation

- C++23 shared core
- Qt 6.11.x
- Qt Quick/QML presentation
- CMake
- SQLite + FTS5 planned for indexing
- replaceable PDF-engine abstraction
- Qt PDF/PDFium evaluation planned for rendering/text
- qpdf planned for structural-transformation evaluation
- thin OS adapters

At bootstrap time, upstream verification found Qt 6.11.2 and qpdf 12.4.1 as current releases. The repository deliberately does not wire qpdf/SQLite into N0 so the skeleton remains a minimal, reproducible Qt application.

## Product priority

1. Library/Index
2. Reader
3. Bookmarks/Outlines
4. Writing/annotations
5. Printing/metadata/secondary utilities

The rule for document-associated research data is **portable when possible, local when necessary, never lost silently**.

## Implementation status

Implemented in N0:

- documentation hierarchy;
- CMake bootstrap;
- minimal QML window;
- pure C++ capability/PDF/filesystem/index interfaces;
- smoke test;
- Windows CI definition.

Not implemented:

- PDF loading/rendering/editing;
- SQLite database;
- folder scanner/watcher;
- reader UI;
- bookmarks;
- migration;
- installer.

## Source-of-truth order

- Running code/tests define implemented behavior.
- `CHECKPOINTS.md` defines delivery status/order.
- `PLAN.md` defines the north star.
- `docs/CORE_WORKFLOWS.md` defines core product behavior/failure states.
- Architecture/stack/performance/platform docs define engineering contracts.
- `CHANGELOG.md` records repository changes.

No document may claim an unimplemented requirement is shipped.