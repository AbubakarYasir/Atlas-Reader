# Atlas Reader Native — Checkpoints to Windows 2.0

This file controls implementation order. Only one checkpoint is active at a time. A checkpoint may be **Not started**, **In progress**, **Ready for owner test**, **Accepted**, or **Blocked**.

`PLAN.md` defines the north star; `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope; `docs/QUALITY_AND_TESTING.md` defines evidence; this file defines execution order and stop gates.

## Current checkpoint

| Field | Value |
|---|---|
| Checkpoint | **N0 — Native repository bootstrap** |
| Planned version | `2.0.0-alpha.0` (engineering bootstrap; not normal user release) |
| Status | **In progress — documentation/toolchain verification** |
| Scope | Documentation, CMake/Qt shell, core interfaces, smoke test, CI and agent/tooling skeleton |
| Explicitly excluded | PDF engine integration, SQLite implementation, indexing, reader, bookmarks |
| Primary platform | Windows 11 |

N0 is not Accepted until Windows CI has successfully configured, built, and run tests on the bootstrap tree and the owner accepts the architecture/documentation program.

## Operating contract

1. Do not begin the next checkpoint until the current checkpoint is **Accepted**.
2. Every checkpoint has written exit criteria and an owner test/evidence handoff.
3. Performance claims require benchmark evidence on named hardware/build configuration.
4. Core logic remains portable; Windows-specific behavior goes behind adapters.
5. User-facing work includes English/Arabic resources and accessibility from its checkpoint onward, not as a late retrofit.
6. PDF writes never bypass security, silently invalidate signed originals, or overwrite newer external revisions.
7. Planned/Implemented/Verified/Accepted/Released are distinct states.
8. Migration work never destructively modifies the Flutter profile or user PDFs.
9. Dependencies/tooling follow `docs/DEPENDENCIES_AND_TOOLS.md` and licensing gates.
10. P0 Windows 2.0 scope in `docs/FEATURE_SCOPE_2_0.md` cannot be silently deferred; changing scope requires an explicit docs/owner decision.

## Release ledger

| ID | Planned release | Checkpoint | Status |
|---|---|---|---|
| N0 | `2.0.0-alpha.0` | Native repository bootstrap | **In progress** |
| N1 | `2.0.0-alpha.1` | Windows toolchain + empty-shell baseline | Not started |
| N2 | `2.0.0-alpha.2` | PDF engine qualification spike | Not started |
| N3 | `2.0.0-beta.1` | Library/index foundation | Not started |
| N4 | `2.0.0-beta.2` | Native reader foundation | Not started |
| N5 | `2.0.0-beta.3` | Core resilience + complete bookmarks | Not started |
| N6 | `2.0.0-beta.4` | Ink + standard annotations | Not started |
| N7 | `2.0.0-beta.5` | Printing, covers + portable metadata | Not started |
| N8 | `2.0.0-beta.6` | Flutter migration + backup/restore | Not started |
| N9 | `2.0.0-beta.7` | Arabic/RTL/accessibility qualification | Not started |
| N10 | `2.0.0-rc.1` | Performance/hardware + release qualification | Not started |
| N11 | `2.0.0` | Windows stable release | Not started |
| P1 | post-2.0 | Android adaptation | Not started |
| P2 | post-2.0 | Linux adaptation | Not started |
| P3 | post-2.0 | macOS adaptation | Not started |
| P4 | post-2.0 | iOS/iPadOS adaptation | Not started |

Corrective builds may increment beta/RC identifiers/build metadata. Never reuse a version tag for different bytes.

---

## N0 — Native repository bootstrap

**Goal:** Establish a clean, documented native architecture without prematurely implementing product features.

### Exit criteria

- C++23/CMake project exists;
- minimal Qt Quick shell exists;
- portable core interfaces exist without Qt GUI/Win32 types;
- pure C++ smoke test compiles/runs;
- Windows CI configures/builds/tests successfully;
- `.codex`/AGENTS agent-navigation rules exist;
- architecture, feature scope, stack/dependencies, build, release strategy, performance, quality/testing, UX/accessibility, licensing, migration, competitive baseline, platform, and core-workflow docs exist;
- no Flutter product source copied into native tree;
- owner accepts repository naming/separation and program plan.

### Owner test

Review documentation hierarchy and source tree; build the shell on a Windows dev machine when available; confirm there is no accidental feature implementation or Flutter coupling.

**Stop gate:** N1 does not begin until CI is green and owner records PASS.

---

## N1 — Windows toolchain + empty-shell baseline

**Goal:** Prove the chosen toolchain is reproducible and establish zero-feature performance measurements before feature code hides framework/architecture overhead.

### Work

- select one canonical Qt 6.11.x patch if public/local toolchain can reproduce it; otherwise document/justify compatible pin;
- canonical Visual Studio/MSVC/Windows SDK/CMake/Ninja versions;
- Debug and Release builds;
- CI/local parity;
- app startup/shutdown loop;
- initial theme + English/Arabic shell direction switch;
- logging skeleton with privacy rules;
- clang-format and initial compiler warning policy;
- benchmark harness introduced.

### Evidence

Measure Release build:

- cold/warm startup;
- idle CPU;
- idle working set/private bytes;
- empty-shell frame pacing/resizing;
- 100% and 200% scale;
- shutdown cleanliness.

**Stop gate:** baseline report committed/attached; no PDF dependency yet.

---

## N2 — PDF engine qualification spike

**Goal:** Select PDF responsibilities using real fixtures/benchmarks, not preference.

### Evaluate

- Qt PDF;
- PDFium;
- qpdf for structure/security/transformation;
- combinations where rendering and structural mutation use separate engines.

### Fixture classes

- Arabic/English mixed outlines;
- deep/duplicate outlines;
- encrypted/restricted;
- signed/certified;
- rotated/mixed-size/crop boxes;
- image-only;
- unusual page labels;
- malformed outlines;
- long/large PDFs;
- existing annotations/XMP.

### Outputs

- adapter prototypes only;
- benchmark/render-fidelity/text/search results;
- coordinate/destination normalization findings;
- preservation/security findings;
- license/notices assessment;
- ADR selecting responsibilities;
- vcpkg manifest/baseline introduced for accepted non-Qt native dependencies.

**Do not build the final reader here.**

**Stop gate:** engine decision accepted with rollback/replaceability preserved.

---

## N3 — Library/index foundation (`2.0.0-beta.1`)

**Goal:** First useful native beta: durable identity/index/search before reader complexity.

### Subgates

**N3.1 Storage/schema** — SQLite migrations, repositories, FTS, serialized writes, profile paths.

**N3.2 Scanner** — multiple roots, progressive bounded scan, cancellation, permission-denied isolation, overlapping roots, reparse safety.

**N3.3 Identity/reconciliation** — rename/move, copies, offline roots, missing/unreadable/encrypted states, no destructive unavailable-root purge.

**N3.4 Library UX/search** — grid/list/folders, Recents/Favorites, filtering/sorting, Command Center book results, Arabic metadata.

### Owner test

Use a representative multi-folder library including Arabic/English, nested roots, encrypted/corrupt files, a removable/offline root, rename/move and copied PDF. Confirm responsiveness and correct retained state.

**Stop gate:** publish first native beta only after library/index is genuinely useful without the Flutter app.

---

## N4 — Native reader foundation (`2.0.0-beta.2`)

**Goal:** Build the Atlas-owned GPU-backed virtualized reader.

### Subgates

**N4.1 Session/open model** — direct open, tabs, immutable/read session, errors.

**N4.2 Virtual viewport** — page geometry, visible/near-visible texture cache, cancellation/priorities, memory bounds.

**N4.3 Navigation** — scroll/page, zoom/fit, page jump/labels/academic offset, thumbnails.

**N4.4 Outline/text/search/links** — exact navigation and search where text exists.

**N4.5 Workspace** — navigation panel, context menus, session restoration option, keyboard/a11y/Arabic.

### Evidence

- large document navigation;
- repeated tab switch/open/close;
- bounded memory/cache behavior;
- UI-thread responsiveness;
- 200% scale;
- image-only PDF behavior;
- mixed page sizes/rotations.

**Stop gate:** reader is comfortable enough for daily PDF reading before bookmark editing begins.

---

## N5 — Core resilience + complete bookmarks (`2.0.0-beta.3`)

**Goal:** Make deep research navigation the signature Atlas capability.

Implements the full contract in `docs/CORE_WORKFLOWS.md`.

### Subgates

**N5.1 Capability preflight** — open vs mutation capability, password distinctions, filesystem read-only, signed/certified, lock, offline, external revision.

**N5.2 Local overlay** — Embedded/Local/Pending/Override/Hidden/Conflict state persisted/recoverable.

**N5.3 Complete editor** — create/rename/delete/destination/move/reparent/reorder/nest, duplicate names, deep Unicode tree, keyboard semantics.

**N5.4 Reconciliation/safe save** — diff/merge/conflicts, temp→validate→backup/replace→reopen; no blind overwrite.

**N5.5 Portability** — Atlas JSON import/export, Markdown, CSV, subtree/local-only export.

### Owner fixtures

Normal writable, permission-restricted, read-only filesystem, transient lock, signed/certified, external outline change, offline/missing, Arabic/English deep tree, duplicate names, 10k-node search.

**Stop gate:** no tested protected/conflicted state loses research or falsely reports embedding; writable PDF round-trip agrees externally.

---

## N6 — Ink + standard annotations (`2.0.0-beta.4`)

**Goal:** Add low-latency writing without coupling pointer input to PDF serialization.

### Subgates

- live GPU stroke layer;
- pen/highlighter presets;
- pressure/hardware mapping where verified;
- eraser/select/delete/undo/redo;
- pan/zoom gesture separation;
- page isolation/coordinate normalization;
- standard PDF ink round-trip;
- text highlight/underline/strikeout/sticky note where engine support passes interop tests;
- restricted/signed/conflict behavior reuses N5 local/safe-save rules.

**Stop gate:** low-latency interaction + independent-reader round-trip + no collateral PDF loss.

---

## N7 — Printing, covers + portable metadata (`2.0.0-beta.5`)

**Goal:** Complete the essential desktop document utilities without turning Atlas into a PDF office suite.

### Work

- queued/invalidation-safe covers;
- useful document details;
- Windows print/preview/ranges/layout with source hash unchanged;
- selected metadata/XMP editing with before/after preview and preservation checks;
- capability/safe-save integration.

**Stop gate:** core P1 utilities pass without reader/index regression.

---

## N8 — Flutter migration + backup/restore (`2.0.0-beta.6`)

**Goal:** Make V2 safe for existing users and app-local research data.

- versioned Atlas backup/export + restore;
- legacy Flutter DB/profile read-only importer;
- identity preview/unresolved items;
- idempotent/repeatable migration behavior;
- source PDFs excluded from backup by default;
- no filename-only destructive relinking;
- documented rollback/side-by-side behavior.

**Stop gate:** clean-profile restore and representative legacy migration pass.

---

## N9 — Arabic/RTL/accessibility qualification (`2.0.0-beta.7`)

**Goal:** Close cross-cutting gaps across everything implemented; this is qualification, not the first time accessibility/Arabic is added.

### Matrix

Complete Library, Reader, Bookmarks, protected/local fallback, conflict, import/export, ink, metadata, print, backup/migration using:

- Arabic UI;
- mixed Arabic/English/Urdu data;
- keyboard only;
- Narrator;
- Accessibility Insights;
- 100%/200% scale;
- high contrast;
- reduced motion where applicable.

**Stop gate:** no core workflow untested/failed in the published matrix.

---

## N10 — Performance/hardware + release qualification (`2.0.0-rc.1`)

**Goal:** Prove feature-complete Windows 2.0 on representative real hardware.

### Qualification

- large libraries;
- long/large PDFs;
- 10,000+ bookmark trees;
- SSD + representative HDD/external drive;
- pen + touch;
- multiple DPI/monitor scenarios;
- physical printer;
- interrupted saves/conflicts/offline roots;
- prolonged session/memory behavior;
- install/upgrade candidate packaging;
- SBOM/notices/security/dependency review.

**Stop gate:** any missing required hardware evidence or failed P0 threshold blocks stable.

---

## N11 — Windows stable 2.0

**Goal:** Promote an accepted RC artifact to **`2.0.0`**.

### Exit

- clean install/upgrade/uninstall;
- optional `.pdf` association user-controlled;
- migration/backup guidance;
- no user books/local research deleted by uninstall;
- release notes + known limitations;
- source/tag/artifact/checksum/SBOM/notices archived;
- final RC receives explicit owner PASS;
- stable artifact is byte-for-byte/source-equivalent to accepted candidate except publication metadata that does not alter executable/package payload.

---

## Platform checkpoints after Windows 2.0

### P1 — Android

Add SAF/document-provider storage, lifecycle, sharing/open-with, mobile responsive shell, stylus/touch qualification; reuse core rules.

### P2 — Linux

Wayland/X11, portals, filesystem/watch differences, printing, packaging; no forked domain behavior.

### P3 — macOS

Finder/sandbox/security-scoped behavior as applicable, menus, printing, Metal qualification, packaging/notarization path.

### P4 — iOS/iPadOS

Document picker/security-scoped URLs, lifecycle/sharing, Pencil, mobile shell; reuse the same document/bookmark capability semantics.

Platform adaptation does not reopen Windows 2.0 architecture casually. If a port exposes a flawed abstraction, change it through an ADR and cross-platform tests rather than creating a platform-specific duplicate core.