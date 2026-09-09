# Changelog

All notable changes are documented here. This project follows a staged
hardening approach rather than formal releases.

## Focused core workflow — Library, bookmarks, and PDF ink (2026-09-09)

### Added

- A distraction-free **Write** mode with pressure-sensitive pen, freehand
  highlighter, five standard colors, line sizes, area eraser, whole-stroke
  selection/deletion, and `Ctrl+Z` / `Ctrl+Shift+Z` undo and redo.
- Standard PDF `/Ink` persistence using native 72-point page coordinates,
  bottom-left PDF origin conversion, and strict page-index isolation.
- A `RepaintBoundary` around the writing surface and byte-backed editing so
  live strokes do not repaint the normal reader or keep Windows file locks.
- Local SQLite ink projection rebuilt from the authoritative PDF after saves.
- In-book outline and bookmark search that preserves unlimited nested paths.

### Changed

- The primary reader now exposes only outline/bookmarks, display controls,
  page navigation, bookmarking, and writing; research/speed controls are no
  longer part of the focused reading surface.
- Library scanning overlaps metadata work in bounded batches and global search
  now matches book title and author as well as file name.
- The visual library opens PDFs directly in the focused reader and labels its
  third layout **Compact List**.
- Updated `file_picker` for the current Windows dependency stack and added the
  native PDF editor dependencies.

### Safety and verification

- Ink saves are rejected before replacement if page count, nested outline
  path/destination structure, document information, or XMP presence changes.
- Added deterministic tests for PDF-coordinate ink, page isolation, nested
  Arabic/English outline preservation, metadata preservation, ink re-indexing,
  unlimited outline construction, and title/author/file quick-open search.
- Final full-suite and Windows UI verification will be recorded when this
  milestone is signed off.

## Stage 11 — Speed reading and hands-free mode (2026-09-09)

### Added

- Hands-free reader overlay with pause, direction, speed, and stop controls.
- RSVP speed reader for selectable current-page PDF text, with 200–800 WPM,
  readable focus highlighting, display themes, and Arabic-aware word direction.
- Background page-text extraction for the speed reader plus widget and engine
  regression tests.

## Stage 10 — Multi-document workspace (2026-09-09)

### Added

- Split reader workspace, tabbed reading sessions, persisted tab/page state,
  and a linked Markdown research scratchpad.

## Stage 9 — Research annotations and citations (2026-09-09)

### Added

- Standard PDF highlights, underlines, strikethrough, sticky notes, local
  annotation indexing, annotations drawer, and citations.

## Stage 8 — Advanced reading engine (2026-09-09)

### Added

- Basic/Research interface modes, reading layouts, display themes, margin
  crop, brightness, page offset, outline sidebar, thumbnail jump grid, and
  chapter-aware scrub bar.

## Stage 7 — Library discovery and visual bookshelf (2026-09-09)

### Added

- Cover caching, visual grid and list presentations, rich library metadata,
  favorites, reading progress, filters, EPUB discovery, and folder scanning.

## Stage 6 — Arabic PDF round-trip verification (2026-09-09)

### Added

- A nested Arabic outline fixture that verifies safe PDF bookmark writes,
  tag persistence, external-outline changes, and database synchronization.

## Stabilization audit — Stages 1–11 (2026-09-09)

### Fixed

- Reader page-number offsets now persist when changed in Reader Settings;
  previously the value was compared after state replacement and never saved.

### Verified

- `flutter analyze` with zero issues and the full automated test suite.
- Mixed Arabic-English path-key regression coverage.
- Read-only real-file smoke coverage for Arabic nested outlines and English
  PDFs, configured without storing personal paths in source control.

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

## Stage 3 — PDF reader integration (completed)

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

- Initial tree, snapshot, diff, and widget coverage was added; current
  verification status is recorded in the stabilization audit above.
