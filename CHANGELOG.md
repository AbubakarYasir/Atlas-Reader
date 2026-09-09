# Changelog

All notable changes are documented here. This project follows a staged
hardening approach rather than formal releases.

## Stage 11 — Speed reading and hands-free mode (2026-09-09)

### Added

- Hands-free reader overlay with pause, direction, speed, and stop controls.
- RSVP speed reader for selectable current-page PDF text, with 200–800 WPM,
  readable focus highlighting, display themes, and Arabic-aware word direction.
- Background page-text extraction for the speed reader plus widget and engine
  regression tests.

## Stages 6–10 — Arabic verification, research, and workspace (2026-09-09)

### Added

- Arabic nested-outline round-trip coverage, safe bookmark injection, tag
  persistence, and bi-directional PDF/database synchronization.
- Visual bookshelf discovery, rich library metadata, covers, EPUB discovery,
  filters, and reading-progress tracking.
- Basic and Research reading modes, display themes, page controls, thumbnail
  jumping, annotations, citations, split reading, tabbed sessions, and linked
  research scratchpads.

## Stage 5 — Accessibility and Arabic language support (2026-09-09)

### Added

- English and Arabic Flutter localization resources, app-level locale choice,
  and Material RTL layout support.
- Keyboard outline controls: Up/Down traversal, Right/Left expansion,
  `F2` edit, `Delete` removal, and `Ctrl+S` for saving pending PDF changes.
- Semantic tree labels that report bookmark title, depth, type, and page,
  including mixed Arabic-English titles.
- Live screen-reader announcements for bookmark saves, sync outcomes, PDF
  commits, reader errors, and visible status updates.
- Accessibility test coverage for F2/Delete tree actions and mixed-direction
  semantic labels.

### Changed

- Command Center results now expose explicit spoken actions, breadcrumb context,
  and localized page/file/folder labels.
- Settings has an English/Arabic language picker instead of a planned-feature
  placeholder.

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
