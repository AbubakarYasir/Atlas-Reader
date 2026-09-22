# Atlas Reader Native — Checkpoints to Windows 2.0

This file controls implementation order. Only one checkpoint is active at a time. A checkpoint may be **Not started**, **In progress**, **Ready for owner test**, **Accepted**, or **Blocked**.

`PLAN.md` defines the north star; `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope; `docs/QUALITY_AND_TESTING.md` defines evidence; this file defines execution order and stop gates.

## Current checkpoint

| Field | Value |
|---|---|
| Checkpoint | **N1 — Windows toolchain + empty-shell baseline** |
| Planned version | `2.0.0-alpha.1` (engineering alpha; not normal user release) |
| Status | **Accepted — owner PASS recorded 2026-09-22** |
| Previous checkpoint | **N0 Accepted by owner on 2026-09-22** |
| Scope | Canonical Windows build/CI, empty shell, RTL/theme proof, privacy-safe logging, lifecycle/benchmark harness, zero-feature measurements |
| Explicitly excluded | PDF engines, qpdf, SQLite/FTS5, scanner, production Library/Reader/Bookmarks, annotations, migration, installer |
| Primary platform | Windows 11 |
| Canonical public alpha toolchain | Windows 2022 CI family · Visual Studio 17 2022 x64 · MSVC v143 · Qt 6.10.3 MSVC 2022 64-bit · C++23 |
| Evidence sheet | `docs/baselines/N1_WINDOWS_BASELINE.md` |

N1 is accepted. The owner passed the final keyboard activation retest, explicitly accepted the measured English startup tradeoff, and recorded `N1 PASS` on 2026-09-22. The accepted user-qualified artifact is `73f567cf2c557f185371d7f944ebf6d69453105a`. N2 remains **Not started** until it is deliberately opened as the next checkpoint.

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
11. Canonical toolchain changes follow `docs/TOOLCHAIN.md` and require recorded evidence rather than silent workstation drift.
12. A passed checkpoint cannot be reopened without new evidence of a user-facing regression.

## Release ledger

| ID | Planned release | Checkpoint | Status |
|---|---|---|---|
| N0 | `2.0.0-alpha.0` | Native repository bootstrap | **Accepted** |
| N1 | `2.0.0-alpha.1` | Windows toolchain + empty-shell baseline | **Accepted** |
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

## N0 — Native repository bootstrap — Accepted

**Goal:** Establish a clean, documented native architecture without prematurely implementing product features.

### Exit criteria completed

- C++23/CMake project exists;
- minimal Qt Quick shell exists;
- portable core interfaces exist without Qt GUI/Win32 types;
- pure C++ smoke test compiles/runs;
- Windows CI configures/builds/tests successfully;
- `.codex`/AGENTS agent-navigation rules exist;
- architecture, feature scope, stack/dependencies, build, release strategy, performance, quality/testing, UX/accessibility, security, licensing, migration, competitive baseline, platform, upstream-tool catalog, data-format, risk, success-metric, and core-workflow docs exist;
- no Flutter product source copied into native tree;
- owner accepted repository separation, architecture, product priorities, and checkpoint program.

### Verification

Branch-head bootstrap CI passed public Qt installation, CMake Configure, Release Build, and CTest/core smoke. Earlier failed runs were retained as useful evidence that toolchain assumptions were corrected rather than hidden.

### Acceptance

**Owner PASS recorded 2026-09-22.** N0 is frozen as the accepted `2.0.0-alpha.0` foundation. Later fixes may update historical documentation for correctness but must not retroactively imply N0 contained product features.

---

## N1 — Windows toolchain + empty-shell baseline (`2.0.0-alpha.1`) — Accepted

**Goal:** Prove the chosen Windows build is reproducible and establish zero-feature performance measurements before feature code hides framework/architecture overhead.

### Toolchain decision

Binding details are in `docs/TOOLCHAIN.md` and ADR-0003.

For N1 public reproducibility:

- Windows 2022 GitHub runner family;
- Visual Studio 17 2022 x64 generator;
- MSVC v143 / C++23;
- Qt 6.10.3 MSVC 2022 64-bit canonical public alpha pin;
- Qt 6.11.2 allowed as additional developer compatibility evidence;
- CMake >= 3.28;
- Debug + Release;
- warnings-as-errors in CI.

Qt 6.10.3 is the alpha reproducibility pin because unauthenticated public `aqtinstall` Windows 6.11.x binaries are currently unreliable. This is not a claim that 6.10.3 is preferable to newer Qt; re-qualification is required before release.

### Implemented work

- [x] prerelease identifier `alpha.1`;
- [x] local Visual Studio 2022 x64 presets aligned with CI;
- [x] Debug + Release CI matrix;
- [x] `/W4 /permissive- /Zc:__cplusplus`, strict CI `/WX`;
- [x] privacy-safe startup/UI/performance logging categories;
- [x] deterministic startup/shutdown option (`--quit-after-ms`);
- [x] first-frame/QML-load/shutdown metric recorder;
- [x] deterministic empty-shell resize/frame-pacing exercise (`--benchmark-shell`);
- [x] explicit local metrics output (`--metrics-file`), no telemetry;
- [x] English/LTR and Arabic/RTL shell-direction proof;
- [x] light/dark shell proof;
- [x] local PowerShell process memory/CPU/startup measurement harness;
- [x] canonical toolchain document + ADR;
- [x] target-machine evidence template;
- [x] self-contained portable Release qualification artifact;
- [x] physical attempt 1 recorded with owner visual evidence;
- [x] v1 sampler defect identified and replaced by first-frame-aware v2 sampling;
- [x] staged startup breakdown and display DPI/DPR measurement;
- [x] compile-time `QtQuick.Controls.Basic` corrective shell baseline;
- [x] PowerShell qualification-script syntax validation in CI;
- [x] Windows GDI font-backend decision measured and documented;
- [x] 100% and manual 200% scale qualification;
- [x] English/LTR, Arabic/RTL, mixed-script, light/dark qualification;
- [x] repeated clean startup/shutdown;
- [x] keyboard Tab traversal and Enter/Return activation.

### Physical investigation and accepted result

Attempt 1 used the canonical `d76eeef...` Release artifact on the owner Windows 11 machine. It verified the visible shell/RTL/theme behavior but exposed startup around 1.8–2.2 seconds and a v1 sampler defect.

The corrected v2 harness produced valid measurements, and startup isolation showed the dominant Windows cost appeared with first meaningful text/font initialization rather than Atlas layout composition. A backend comparison led to the N1 GDI `qt.conf` candidate.

The final measured GDI evidence includes:

- Arabic warm first-frame p95: `772.875 ms`;
- English 22-run confirmation warm p50: `365.941 ms`;
- English 22-run confirmation warm p95: `1014.827 ms`;
- English worst observed warm in that confirmation: approximately `1.185 s`.

The English confirmation showed a temporary contiguous system-wide slow window affecting pre-QML, QML-load, and post-QML first-frame delivery together, followed by recovery without an application change. The owner explicitly accepted this measured startup tradeoff.

A focused Enter activation defect was then found before acceptance, corrected, rebuilt, and physically retested. The accepted user-qualified artifact is:

`73f567cf2c557f185371d7f944ebf6d69453105a`

with digest:

`sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`

See `docs/baselines/N1_WINDOWS_BASELINE.md` and `docs/baselines/N1_FINAL_QUALIFICATION.md` for the complete evidence record.

### Acceptance

**Owner PASS recorded 2026-09-22.** The owner confirmed keyboard activation passes, explicitly accepted the measured English startup tradeoff, and recorded `N1 PASS`.

N1 is frozen as the accepted `2.0.0-alpha.1` Windows empty-shell/toolchain baseline. Per the repository operating contract, it cannot be reopened without new evidence of a user-facing regression.

**N2 remains Not started until deliberately opened as the next checkpoint.**

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
