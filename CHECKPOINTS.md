# Atlas Reader Native — Checkpoints

This file controls implementation order. Only one checkpoint is active at a time. A checkpoint may be **Not started**, **In progress**, **Ready for owner test**, **Accepted**, or **Blocked**.

## Current checkpoint

| Field | Value |
|---|---|
| Checkpoint | **N0 — Native repository bootstrap** |
| Status | **Ready for owner review** |
| Scope | Documentation, CMake/Qt shell, core interfaces, smoke test, CI skeleton |
| Explicitly excluded | PDF engine integration, SQLite implementation, indexing, reader, bookmarks |
| Primary platform | Windows 11 |

## Operating contract

1. Do not begin the next checkpoint until the current checkpoint is accepted.
2. Performance claims require benchmark evidence on named hardware/build configuration.
3. Core logic must remain portable; Windows-specific behavior goes behind adapters.
4. User-facing work must include English/Arabic resources and accessibility from its checkpoint onward.
5. PDF writes never bypass security, invalidate signed originals silently, or overwrite newer external revisions.
6. Planned behavior and shipped behavior must be clearly distinguished in README/INFO/CHANGELOG.
7. Migration work must not modify the existing Flutter repository or user PDFs destructively.

## Ledger

| ID | Checkpoint | Status |
|---|---|---|
| N0 | Native repository bootstrap | **Ready for owner review** |
| N1 | Windows toolchain + empty-shell baseline | Not started |
| N2 | PDF engine qualification spike | Not started |
| N3 | Library/index foundation | Not started |
| N4 | Native reader foundation | Not started |
| N5 | Core resilience + complete bookmarks | Not started |
| N6 | Ink/standard annotations | Not started |
| N7 | Printing, covers, and metadata | Not started |
| N8 | Migration + backup/restore | Not started |
| N9 | Arabic/RTL/accessibility qualification | Not started |
| N10 | Performance/hardware qualification | Not started |
| N11 | Windows release candidate | Not started |
| P1 | Android adaptation | Not started |
| P2 | Linux adaptation | Not started |
| P3 | macOS adaptation | Not started |
| P4 | iOS/iPadOS adaptation | Not started |

## N0 — Native repository bootstrap

**Goal:** Establish a clean, documented native architecture without prematurely implementing product features.

**Exit criteria:**

- C++23/CMake project exists;
- Qt Quick shell source exists;
- portable core interfaces exist without Qt GUI/Win32 dependencies;
- smoke test demonstrates core headers compile;
- Windows CI workflow exists;
- architecture, stack, build, performance, licensing, migration, platform, and core-workflow docs exist;
- no Flutter product source is copied into the native tree.

**Owner review:** confirm the architecture, repository naming, documentation hierarchy, and checkpoint order before N1.

## N1 — Windows toolchain + empty-shell baseline

**Goal:** Prove the chosen toolchain is reproducible and establish zero-feature performance measurements.

**Required evidence:**

- documented Visual Studio/Qt/CMake versions;
- Debug and Release builds on Windows;
- CI passes;
- app opens/closes cleanly repeatedly;
- startup, idle CPU, idle memory, frame pacing, resize behavior, 100%/200% scale measured;
- English/Arabic shell direction switching proof;
- no PDF dependency yet.

**Stop gate:** baseline measurements are recorded before feature code can hide architectural overhead.

## N2 — PDF engine qualification spike

**Goal:** Select rendering/inspection/transformation engines using fixtures and benchmarks, not preference.

Evaluate Qt PDF and PDFium for render/text/search/navigation, and qpdf for structure/security/preservation work. Use Arabic/English mixed-outline PDFs, encrypted/restricted samples, signed/certified samples, rotated/mixed-page-size documents, scanned/image-only PDFs, malformed outlines, and very long documents.

**Do not build the final reader here.** Produce an engine decision record, adapter prototypes, performance data, license review, and round-trip preservation results.

## N3 — Library/index foundation

**Goal:** Implement the durable identity/index/search layer before reader complexity.

Includes roots, progressive scan, watcher queue, availability states, guarded identity, SQLite/FTS5, direct-open registration, search, rename/move reconciliation, overlapping roots, reparse-point safety, offline roots, and corrupt/encrypted-file isolation.

## N4 — Native reader foundation

**Goal:** Build the GPU-backed virtualized reader with strict thread/performance rules.

Includes visible-page prioritization, zoom/fit modes, navigation, page labels, outline display, text/search/link layer where supported, thumbnails, tab/session state, accessibility, and exact-page commands.

## N5 — Core resilience + complete bookmarks

**Goal:** Implement the complete capability/local-overlay/bookmark contract in `docs/CORE_WORKFLOWS.md`.

Includes password/permission distinctions, read-only/signed/locked/conflict states, Embedded/Local/Pending/Override/Hidden/Conflict storage states, complete tree editing, reconciliation, safe promotion, Atlas JSON export/import, Markdown/CSV export, and local data recovery.

## N6 — Ink and standard annotations

**Goal:** Add low-latency live pen/highlighter/markup using a rendering overlay independent of PDF serialization.

Persist only through the same capability/conflict/safe-save system established earlier.

## N7 — Printing, covers, and metadata

**Goal:** Complete core desktop document utilities without compromising reader responsiveness or source integrity.

## N8 — Migration + backup/restore

**Goal:** Support safe migration from the Flutter Atlas database/profile where feasible and provide first-class native backup/restore.

Migration is previewed, repeatable, versioned, and never matches ambiguous documents by filename alone.

## N9 — Arabic/RTL/accessibility qualification

**Goal:** Close every cross-cutting gap across all implemented features rather than adding a late cosmetic translation pass.

## N10 — Performance/hardware qualification

**Goal:** Prove the Windows product under large libraries/documents, 10,000+ bookmarks, pen/touch, SSD/HDD, external drives, interrupted saves, and physical printing.

## N11 — Windows release candidate

**Goal:** Package a reproducible Windows release with installer, optional file association, licenses/notices, migration/backup guidance, and complete regression evidence.

## Platform checkpoints P1–P4

Each platform starts only after Windows architecture has demonstrated that the shared core is truly portable. Platform work adapts ports and responsive UI; it must not fork core product rules into platform-specific copies.