# N3 — Library and Index Foundation

<!-- atlas-status: N3|in-progress -->

**Status:** In progress, begun 2026-09-27 after N2 owner PASS and closure.
**Branch:** `native-v2-n3-library-index-foundation`
**Starting base:** accepted native `main`, `84a9b45b10b24ece595bb7069136fefd2ed2cc5b`.
**Current active subgate:** N3.2 safe scanning. N3.1 storage/schema implementation is complete; its Windows verification is being refreshed after correcting SQLite's CRT linkage to match Qt.
**Planned milestone:** `2.0.0-beta.1`; a version string alone does not mean N3 is a useful or accepted beta.

This is the binding N3 implementation sequence and evidence plan. The [checkpoint ledger](../CHECKPOINTS.md) and [QA matrix](CHECKPOINT_QA_MATRIX.md) define its mandatory gates. The [N2 acceptance record](baselines/N2_ACCEPTANCE.md) defines the frozen predecessor.

## User outcome

A person can add several book folders, let Atlas build a searchable PDF library without freezing the interface, find the right book using English, Arabic or Urdu metadata, and keep a book's identity and research state when a drive is temporarily offline. Scanning an unavailable or partly readable location must never make its books disappear from the index.

This outcome is a plan; the complete library is not implemented at N3 start.

## Scope and sequence

### N3.1 — Versioned storage and repositories

- SQLite is pinned as `3.53.4#1` at vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, with FTS5 enabled and `x64-windows-static-md` so its static library uses Qt's dynamic MSVC runtime. A runtime mismatch warning found during local/CI integration was fixed; reverify Windows Debug/Release and selected-route compatibility before reaffirming this subgate. See the [N3 evidence ledger](baselines/N3_LIBRARY_INDEX_EVIDENCE.md).
- Keep database operations behind typed repositories/services; UI and filesystem code never issue arbitrary SQL.
- Keep a separately located, versioned app-local profile database. Resolve profile paths through an application/platform service, not a hard-coded profile in portable domain code.
- A persisted library-root ID cannot be silently rebound to another source path. Root removal, replacement and path relocation require explicit policy and must not detach books or erase associated research.
- Add ordered, transactional schema migrations. Record `user_version` only after each migration succeeds. A newer or corrupt database fails with an actionable error; never reset, truncate or silently recreate profile state.
- Separate source-location/root state, document identity and observed revisions, metadata/index projection, user favorites/recents/progress, and derived full-text index state.
- Use FTS5 as rebuildable derived search data. Define and test explicit metadata-update and full-rebuild behavior. Source disappearance must preserve documents and their indexed research state; scans cannot hard-delete index rows. Search queries are bound parameters, not assembled SQL.
- Commit initial migration, repository round trips, rollback/version tests, FTS availability and consistency tests before N3.1 is complete.

### N3.2 — Safe, bounded library scanning

- Add explicit multiple library roots; direct-open behavior is covered only as specified by the [core workflow contract](CORE_WORKFLOWS.md).
- Enumerate on bounded background work, publish progress in batches, and support cancellation/restart without interpreting unseen files as removals.
- Initial scanner implementation streams path-only PDF results in batches of at most 64 by default, with configurable depth, directory and file caps. It skips symbolic links/reparse points and marks inaccessible/incomplete walks partial. These defaults are safety limits, not performance claims; they require Windows CI and later named-storage benchmarks.
- Handle overlapping/nested roots without duplicate logical entries. Treat permission-denied children independently from successful enumeration elsewhere.
- Make reparse/junction/symlink behavior conservative and cycle-safe. Do not follow paths outside configured meaning or loop indefinitely.
- Treat roots and file entries as changing during a scan. Distinguish successful, partial, cancelled and unavailable root outcomes; only a successfully completed root scan can propose deletion reconciliation.
- Coalesce watcher events if/when watchers are introduced; never let event storms create unbounded queued work or a writer queue.
- Cover long/Unicode paths, inaccessible subfolders, nested roots, disappearing files, cancellation and encrypted/corrupt PDF identification.

### N3.3 — Identity and reconciliation

- Keep stable Atlas document identity separate from any current path, filename, title, or PDF engine object.
- A matching filename or content fingerprint alone does not prove two copies are the same logical research item. Copies remain distinct absent user-approved relinking.
- Preserve an offline root and its indexed entries. Missing from a disconnected or partially enumerated root does not mean deleted.
- Retain research state for a sufficiently confident move/rename; leave ambiguous candidates separate and visible for resolution.
- Represent available, locked/encrypted, restricted/read-only, missing/offline, unreadable/corrupt, changed and unsupported states explicitly.
- Verify reconciliation as a diff before changes are committed. Include reopen/restart persistence tests for each durable transition.

### N3.4 — Useful library, filtering and search

- Deliver useful list/grid and folder/library navigation, filtering/sorting, Recents and Favorites, plus book results in Command Center.
- Make selection, scan state, failures and unavailable-root retention clear, keyboard-accessible and operable at 100% and 200% scale.
- Include Arabic and Urdu metadata, right-to-left paths/text, mixed-script title/author, diacritics, normalization, and query/index round trips in the written search contract and fixtures.
- Do not promise OCR: image-only PDFs remain discoverable/indexable by available metadata only.
- Measure named library sizes and SSD plus slower/removable storage; correctness and UI responsiveness must be reported alongside throughput.

## Cross-cutting rules

- Preserve the portable C++23 core and Atlas-owned repository/index contracts; platform and Qt APIs stay behind adapters.
- Do not access SQLite synchronously from the UI thread. Bound scan, parse, search, thumbnail, queue and writer work.
- Never use real private libraries or irreplaceable PDFs as mutation fixtures. Use synthetic/copied data and remove personal paths/text from public CI.
- Never add a dependency without exact provenance, baseline/version, licensing, required build features and reproducible Debug/Release CI evidence.
- N2 and N1 remain frozen. No Reader viewport (N4), complete bookmark authoring/local overlay (N5), migration/backup (N8), or later scope is pulled into N3.
- Update the changelog, checkpoint status, acceptance/evidence ledger and affected build/architecture/dependency/test docs with each completed subgate.

## Required verification

Automated gates follow `CHECKPOINTS.md` and `CHECKPOINT_QA_MATRIX.md`. At minimum, N3 requires:

- Debug and Release native build and CTest on Windows CI;
- ordered migration-upgrade tests, interrupted/failing migration rollback, newer-schema rejection, corrupt-database error handling and no-destructive-reset checks;
- repository create/update/read/delete round trips and database reopen tests;
- FTS5 existence/operation, transactional metadata-index consistency, rebuild and deletion correctness;
- scanner and root-outcome tests for bounded publication, cancellation, overlap/deduplication, reparse loops, permissions, offline roots, moving/copying/changing files and encrypted/corrupt documents;
- guarded identity and reconciliation tests for moves, copies and ambiguity;
- Arabic/Urdu/Unicode path, metadata and search correctness;
- keyboard/focus/accessibility and 100%/200% manual proof for Library interactions;
- named SSD and slower/removable-storage responsiveness/performance observations.

Every claim must identify the tested source commit, workflow/artifact, fixtures and known limits. Automated success is not owner acceptance.

## Owner test and stop gate

The owner test uses a representative multi-folder Arabic/English PDF library with nested roots, encrypted and corrupt PDFs, rename/move/copy cases, and a removable or offline root. The report must show that results remain findable and saved research stays attached to the correct item when a root is unavailable, a file moves, or a copy exists.

Compare the same library/search task against relevant mature library/search software. Record product/version/date, common task, measured or observed result, and the improvement Atlas still needs. Make no unsupported “fastest” or “best” claim.

**N3 may be marked Ready for owner test only after all subgates and the owner test checklist are complete. N3 may be Accepted and beta.1 called useful only after automated checks, exact packaged build/checksum, performance and accessibility evidence, competitor comparison, documented limitations and explicit owner `N3 PASS`.** No later checkpoint begins before that handoff.
