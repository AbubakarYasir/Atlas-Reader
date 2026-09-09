# Atlas Reader — Current Project Notes

**Release candidate:** `0.8.0-beta.2` (build 4)

**Latest published release:** `0.8.0-beta.1` (build 2)

**Verified target:** Windows 11

**Source of truth:** the repository and running application

This file is a compact orientation note. User installation and operation live
in `README.md`; implemented stages, acceptance criteria, and deferred work live
in `PLAN.md`; delivery order and owner stop gates live in `CHECKPOINTS.md`;
release history lives in `CHANGELOG.md`.

## Current product focus

Atlas Reader is an offline-first Flutter desktop application built around three
core workflows:

1. Discover and search PDF/EPUB books across explicit library folders.
2. Create unlimited nested PDF bookmarks and jump to their exact pages locally
   or across the whole library.
3. Read and write on PDFs with standard, page-isolated `/Ink` annotations.

The next planned product layer is a fuller desktop PDF workspace: page/outline/
bookmark/annotation navigation, complete bookmark management, standard markup
and note tools, and local Windows printing with page-range and layout controls.
Librera Reader and Noteful are workflow benchmarks for the library and writing
surfaces. Full Arabic/RTL support is a release requirement, including localized
errors and controls, mixed-script metadata, search, accessibility, and PDF
round trips.
The plan intentionally excludes conversion, signing, forms, OCR, cloud services,
multimedia, and 3D features.

The owner accepted **C0 — Baseline acceptance** on 2026-09-09. Execution is now
limited to **C1 — Reader workspace**, with corrected build 4 at **Ready for
owner test**. Work stops until the owner records PASS or reports defects.

The primary shell contains Library, Recents, Bookmarks, Favorites, Folders, and
Settings. Any PDF can also be opened directly or supplied as a Windows launch
argument without enrolling its parent directory.

## Storage contract

- PDF outline entries and `/Ink` annotations are portable document data.
- SQLite is a fast searchable projection for library metadata, bookmarks, ink,
  favorites, tags, and reading progress.
- PDF replacement uses a validated `.tmp` file plus a recoverable
  `.atlas-backup` promotion sequence.
- Page count, nested outline paths/destinations, document information, and XMP
  presence must survive an ink edit before replacement is allowed.
- Bookmark descriptions and tags are currently SQLite-only and remain a
  documented pre-1.0 portability gap.

## Architecture at a glance

- `lib/features/library/`: responsive shell, shelf layouts, folder explorer,
  background progressive scanner, and file watching.
- `lib/features/reader/`: PDF display, nested outline/search, exact page jumps,
  bookmark dialog, and isolated pen/highlighter editing.
- `lib/features/command_center/`: global `Ctrl+K` book/bookmark search.
- `lib/database.dart`: Drift schema, FTS, library state, and local projections.
- `lib/pdf_engine.dart`: outline and annotation read/write plus integrity checks.
- `lib/core/file_system/`: platform-neutral document operations and Windows
  implementation, preserving a future Android storage boundary.

## Performance model

- Library discovery runs directory enumeration in an isolate, inspects up to
  six books concurrently, and publishes every completed batch immediately.
- Intermediate batches never remove unseen records; final reconciliation alone
  detects removals and safe renames.
- Search and navigation use SQLite/FTS rather than reparsing documents.
- PDF reads, writes, validation, and ink indexing run away from the UI isolate.
- The viewer virtualizes pages; live drawing is isolated by `RepaintBoundary`.
- CI caches Flutter/Pub and Windows native output so local agents can consume a
  compact result instead of repeated compiler logs.

## Developer knowledge and automation

- CodeGraph 1.6.0 indexes this checkout locally in `.codegraph/codegraph.db`.
  The database is ignored and never leaves the machine.
- `.codex/config.toml` enables CodeGraph and GitHub's hosted read-only MCP
  endpoint. GitHub credentials must stay in `GITHUB_PAT_TOKEN` at user level.
- `.github/workflows/test.yml` verifies format, analysis, tests, and coverage.
- `.github/workflows/build-windows.yml` creates a downloadable beta bundle.

## Verification

Analysis passed with zero issues and the full unit/widget suite passed 56 tests
with 2 optional personal-file audits skipped. The native Windows integration
suite passed all 3 scenarios, and the Windows release build succeeded. The two
skipped audits still require copied personal Arabic and English PDFs and do not
count as passing evidence.

Commands for reproducing verification:

```powershell
flutter analyze
flutter test
flutter test integration_test/windows_reader_workflow_test.dart -d windows
flutter build windows --release
codegraph status
```

Stage 12 text-to-speech, cloud services, and other secondary expansion are not
part of this beta focus. Historical modules may remain in source but are not on
the primary reading surface.
