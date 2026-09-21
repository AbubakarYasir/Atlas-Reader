# Atlas Reader — Current Project Notes

**Release candidate:** `0.8.0-beta.2` (build 4)

**Latest accepted release:** `0.8.0-beta.1` (build 2)

**Verified target:** Windows 11

**Source of truth for implemented behavior:** the repository and running application

This file is a compact orientation note. User installation and currently implemented operation live in `README.md`; broad product and engineering requirements live in `PLAN.md`; the detailed core Library/Index → Reader → Bookmarks capability, fallback, conflict, import/export, and recovery contract lives in `docs/CORE_WORKFLOWS.md`; delivery order and owner stop gates live in `CHECKPOINTS.md`; release history lives in `CHANGELOG.md`.

`docs/CORE_WORKFLOWS.md` is a **requirements document**, not an implementation-status claim. In particular, its planned local-only bookmark overlay, protected/signed-document fallbacks, bookmark export/import, and conflict flows must not be described as shipped until their checkpoint is implemented and verified.

## Current product focus

Atlas Reader is an offline-first Flutter desktop application built around three core workflows, in this priority order:

1. Discover, identify, index, and search PDF/EPUB books across explicit library folders.
2. Read and navigate PDFs responsively and accurately.
3. Create, organize, search, and jump through deep PDF bookmarks/outlines, with portable storage when the document can be changed safely and a planned first-class local fallback when it cannot.

The product rule for the road to 1.0 is **portable when possible, local when necessary, never lost silently**. A writable ordinary PDF remains the preferred portable authority for its outline and standard annotations. A restricted, filesystem-read-only, signed/certified, externally locked, conflicted, unavailable, or otherwise unsafe-to-mutate document must not turn bookmarking into a dead end: the planned core contract keeps research data in an explicit Atlas overlay and offers later promotion, Save Copy, export, or conflict resolution instead of bypassing security or risking the source.

The owner accepted **C0 — Baseline acceptance** on 2026-09-09. Execution is now limited to **C1 — Reader workspace**, with corrected build 4 at **Ready for owner test**. Work stops until the owner records PASS or reports defects. The new core-workflow requirements do not reopen C1; after C1 is accepted they refine C2 and become cross-cutting save/sync requirements for later checkpoints.

The primary shell contains Library, Recents, Bookmarks, Favorites, Folders, and Settings. Any PDF can also be opened directly or supplied as a Windows launch argument without enrolling its parent directory.

## Core workflow contract

The 1.0 plan treats document capability as state, not as a generic save error. Future work must explicitly distinguish at least:

- normal writable PDFs;
- open-password encryption;
- permissions-restricted PDFs;
- filesystem read-only files/folders/media;
- transient Windows sharing locks;
- signed/certified documents where mutation may invalidate or violate signature state;
- malformed/partially recoverable documents;
- unsupported security handlers;
- missing/offline/removable/cloud-placeholder sources;
- PDFs changed outside Atlas while open.

For bookmark work, the planned storage states are **Embedded**, **Local only**, **Pending embed**, **Local override**, **Locally hidden**, and **Conflict**. Passwords are never part of the database/export contract; pre-1.0 may retain a successful password only in process memory for the current session.

The complete requirements and C2 internal order are in `docs/CORE_WORKFLOWS.md`.

## Storage contract

### Current verified beta

- PDF outline entries and `/Ink` annotations are portable document data.
- SQLite is a fast searchable projection for library metadata, bookmarks, ink, favorites, tags, and reading progress.
- PDF replacement uses a validated `.tmp` file plus a recoverable `.atlas-backup` promotion sequence.
- Page count, nested outline paths/destinations, document information, and XMP presence must survive an ink edit before replacement is allowed.
- Bookmark descriptions and tags are currently SQLite-only and remain a documented pre-1.0 portability gap.

### Planned 1.0 refinement

- PDF data remains preferred when it can be changed safely and legitimately.
- Atlas-local bookmark overlays become a first-class fallback when PDF mutation is unavailable or unsafe.
- Local edits are not reported as embedded until a validated PDF commit is re-read successfully.
- External document revisions block blind overwrite and require reconciliation.
- Local-only research data requires explicit export/backup and safe relinking rather than depending indefinitely on a hidden database file.

## Architecture at a glance

- `lib/features/library/`: responsive shell, shelf layouts, folder explorer, background progressive scanner, and file watching.
- `lib/features/reader/`: PDF display, nested outline/search, exact page jumps, bookmark dialog, and isolated pen/highlighter editing.
- `lib/features/command_center/`: global `Ctrl+K` book/bookmark search.
- `lib/database.dart`: Drift schema, FTS, library state, and local projections.
- `lib/pdf_engine.dart`: outline and annotation read/write plus integrity checks.
- `lib/core/file_system/`: platform-neutral document operations and Windows implementation, preserving a future Android storage boundary.

The planned capability/overlay work should be implemented behind these existing boundaries rather than by adding permission/password/save logic directly to presentation widgets.

## Performance model

- Library discovery runs directory enumeration in an isolate, inspects up to six books concurrently, and publishes every completed batch immediately.
- Intermediate batches never remove unseen records; final reconciliation alone detects removals and safe renames.
- Search and navigation use SQLite/FTS rather than reparsing documents.
- PDF reads, writes, validation, and ink indexing run away from the UI isolate.
- The viewer virtualizes pages; live drawing is isolated by `RepaintBoundary`.
- CI caches Flutter/Pub and Windows native output so local agents can consume a compact result instead of repeated compiler logs.

The core requirements add further performance/correctness cases for overlapping roots, reparse-point cycles, unavailable removable/network roots, cloud placeholders, files changing during scan, locked/encrypted/corrupt documents, guarded identity matching, and large bookmark trees. These are requirements until separately verified.

## Developer knowledge and automation

- CodeGraph 1.6.0 indexes this checkout locally in `.codegraph/codegraph.db`. The database is ignored and never leaves the machine.
- `.codex/config.toml` enables CodeGraph and GitHub's hosted read-only MCP endpoint. GitHub credentials must stay in `GITHUB_PAT_TOKEN` at user level.
- `.github/workflows/test.yml` verifies format, analysis, tests, and coverage.
- `.github/workflows/build-windows.yml` creates a downloadable beta bundle.
- `AGENTS.md` requires agents touching the three core workflows or document mutation to read `docs/CORE_WORKFLOWS.md` first.

## Verification

Analysis passed with zero issues and the full unit/widget suite passed 56 tests with 2 optional personal-file audits skipped. The native Windows integration suite passed all 3 scenarios, and the Windows release build succeeded. The two skipped audits still require copied personal Arabic and English PDFs and do not count as passing evidence.

Commands for reproducing current verification:

```powershell
flutter analyze
flutter test
flutter test integration_test/windows_reader_workflow_test.dart -d windows
flutter build windows --release
codegraph status
```

Stage 12 text-to-speech, cloud services, and other secondary expansion are not part of this beta focus. Historical modules may remain in source but are not on the primary reading surface. The 1.0 scope also does not include password cracking/restriction bypass, OCR, arbitrary PDF text/object editing, forms/signing workflows, or an attempt to clone the complete Acrobat/Foxit feature set.
