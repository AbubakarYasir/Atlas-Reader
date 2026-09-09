# Changelog

All notable changes are documented here. This project follows a staged
hardening approach rather than formal releases.

## Stage 4 — Command Center and library management (2026-09-09)

### Added

- A global `Ctrl+K` Command Center overlay. Bookmark results show the book,
  breadcrumb path, and page number; selecting one opens the PDF reader at the
  matching page.
- Library-folder settings for adding, removing, and manually rescanning one or
  more folders.
- Recursive, isolate-backed PDF folder scanning and a Library Files view for
  opening discovered documents.
- Directory change watching while Atlas is open, so added, changed, renamed,
  and deleted PDFs refresh the local index.
- Drift library-folder and library-file tables, plus tests for scan results,
  searching, folder removal, and renamed-file reconciliation.

### Changed

- A safely matched in-folder PDF rename carries local bookmarks and the PDF
  snapshot to its new path instead of treating it as a separate book.

## Stage 3 — PDF reader integration (in progress)

### Added

- Windows PDF reader route with virtual page scrolling, zoom controls, text
  selection, active-page tracking, and `Ctrl+B` in-reader bookmarking.
- Background document-byte loading through `DocumentFileSystem` before the
  reader viewer mounts.
- Background-isolate document reads and generated-file writes for PDF engine
  operations, including the temporary-file safety workflow.
- Isolate-backed page-count and outline extraction, plus isolate-backed
  single-bookmark PDF generation.

## Stage 2 — Architecture foundations (2026-09-09)

### Added

- Feature-oriented folders for library, reader, bookmark management, command
  center, sync preview, and settings UI components.
- Reusable library document selector, bookmark composer, command-center search
  field, sync preview, bookmark-management scope, and settings screen.
- `DocumentFileSystem` interface and `WindowsDocumentFileSystem` adapter.
- Core database connection factory, keeping filesystem setup out of database
  and feature code.

### Changed

- `PdfEngine`, `PdfSafeFileWriter`, and missing-file resolution depend on the
  document-file abstraction rather than opening user documents directly.
- `SyncEngine` now receives the same injected `PdfEngine` instance used by the
  workspace, keeping platform dependencies at the application edge.

## Stage 1 — Data safety and current-defect fixes (2026-09-09)

### Fixed

- Wrapped bookmark and expansion rows in transparent `Material` widgets, so
  nested outline rows have a valid ink surface and no longer emit the Flutter
  `ListTile background color or ink splashes may be invisible` warning.
- Replaced flat-title reconciliation logic with a shared, path-key diff
  calculator. Equal titles in separate sections are now distinct bookmarks.
- Replaced flat `file_snapshots.last_known_state` lists with version 2 JSON
  snapshots containing full hierarchical path keys. Legacy title-list snapshots
  remain readable.
- Corrected the widget test setup to supply an in-memory Drift database.

### Added

- `PdfSafeFileWriter`, which validates generated output, writes and validates
  `filename.pdf.tmp`, verifies page count, and preserves the original through a
  recovery-file swap.
- Plain-language `PdfOverwriteException` messages for absent originals,
  invalid output, existing recovery files, and failed promotion.
- Unit tests for tree construction, path-key identity, legacy snapshot decoding,
  and reconciliation differences.

### Verified

- `flutter test` — 14 tests passing.
- `flutter analyze` — no issues.
