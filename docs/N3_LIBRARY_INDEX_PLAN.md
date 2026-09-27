# N3 — Library and Index Foundation

<!-- atlas-status: N3|in-progress -->

**Status:** In progress, begun 2026-09-27 after N2 owner PASS and closure.
**Branch:** `native-v2-n3-library-index-foundation`
**Starting base:** accepted native `main`, `84a9b45b10b24ece595bb7069136fefd2ed2cc5b`.
**Current active subgates:** N3.2 final scanner evidence and N3.3 identity/reconciliation. N3.1 storage/schema and the corrected `x64-windows-static-md` CRT linkage are verified on local Windows and final CI head `e7fe595c5ffb228f48b63c17a960559a8b8b8a22`.
**Planned milestone:** `2.0.0-beta.1`; a version string alone does not mean N3 is a useful or accepted beta.

This is the binding N3 implementation sequence and evidence plan. The [checkpoint ledger](../CHECKPOINTS.md) and [QA matrix](CHECKPOINT_QA_MATRIX.md) define its mandatory gates. The [N2 acceptance record](baselines/N2_ACCEPTANCE.md) defines the frozen predecessor.

## User outcome

A person can add several book folders, let Atlas build a searchable PDF library without freezing the interface, find the right book using English, Arabic or Urdu metadata, and keep a book's identity and research state when a drive is temporarily offline. Scanning an unavailable or partly readable location must never make its books disappear from the index.

This outcome is a plan; the complete library is not implemented at N3 start.

## Scope and sequence

### N3.1 — Versioned storage and repositories

- SQLite is pinned as `3.53.4#1` at vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, with FTS5 enabled and `x64-windows-static-md` so its static library uses Qt's dynamic MSVC runtime. A runtime mismatch warning found during local/CI integration was fixed and verified by local and final Windows Debug/Release builds plus selected-route CI. See the [N3 evidence ledger](baselines/N3_LIBRARY_INDEX_EVIDENCE.md).
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
- Multi-root coordination now retains a separate outcome for every configured root and suppresses duplicate paths emitted by overlapping/nested roots. The implementation at `609c8b9` is verified locally and in final branch CI head `66f13464d8f7fed903360d048beea8d5bfd60902`; later named-storage benchmarks remain required. Treat permission-denied children independently from successful enumeration elsewhere.
- Make reparse/junction/symlink behavior conservative and cycle-safe. Do not follow paths outside configured meaning or loop indefinitely.
- Treat roots and file entries as changing during a scan. Distinguish successful, partial, cancelled and unavailable root outcomes; only a successfully completed root scan can propose deletion reconciliation.
- Persisted root scan generations are active at `352c5d0`: an older scan cannot overwrite a newer one, newly observed paths remain unlinked until identity reconciliation, and partial/offline/interrupted scans preserve the preceding successful completion evidence and indexed books.
- Scanner/index coordination at `38d3cab`, verified through final branch head `e871ecab257008fc3f100927f0411e6e0832b84f`, marks known locations in their current root generation and publishes unknown paths in bounded, cross-root-deduplicated batches for the later identity stage. It does not create document IDs, relink books or infer deletions. Complete, partial, unavailable and cancelled outcomes retain separate safe persistence rules.
- Coalesce watcher events if/when watchers are introduced; never let event storms create unbounded queued work or a writer queue.
- Cover long/Unicode paths, inaccessible subfolders, nested roots, disappearing files, cancellation and encrypted/corrupt PDF identification.
- PDF inspection at `30137a1` classifies readable, password-locked, malformed/unreadable, unsupported-security and missing files, preserves Unicode paths, and collects safe metadata/revision signals where available. It does not bypass passwords or make identity/relink decisions.

### N3.3 — Identity and reconciliation

- Keep stable Atlas document identity separate from any current path, filename, title, or PDF engine object.
- ADR-0005 freezes the conservative policy: only a previous path proven missing by a complete scan plus preserved filesystem identity may authorize automatic move relinking. A surviving original is a distinct copy; content/PDF metadata matches alone are ambiguous; offline and incomplete scans never prove a relationship.
- Schema v4 at `f278bec` persists the last successfully completed scan generation, opaque filesystem identity evidence and reviewable reconciliation proposals. Proposal creation never changes a location; application revalidates the completed scans, unchanged evidence and unclaimed target in one transaction. Pending/applied/dismissed audit state and stable document identity survive restart.
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
