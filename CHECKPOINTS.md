# Atlas Reader Native — Checkpoints to Windows 2.0

This file controls implementation order. Only one checkpoint is active at a time. A checkpoint may be **Not started**, **In progress**, **Ready for owner test**, **Accepted**, or **Blocked**.

`PLAN.md` defines the north star; `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope; `docs/QUALITY_AND_TESTING.md` defines evidence; this file defines execution order and stop gates.

## Current checkpoint

| Field | Value |
|---|---|
| Checkpoint | **N2 — PDF engine qualification spike** |
| Planned version | `2.0.0-alpha.2` (engineering alpha; not normal user release) |
| Status | **In progress — capability qualification complete; production-route freeze and owner handoff remain** |
| Previous checkpoint | **N1 Accepted by owner on 2026-09-22** |
| Branch | `native-v2-n2-pdf-engine-qualification` |
| Base | accepted N1 integration commit `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2` |
| Scope | Qt PDF/PDFium read-render-text-navigation qualification; qpdf structure/security/transformation qualification; normalized contracts; fixtures; benchmarks; preservation/licensing/reproducibility evidence; final ADR |
| Explicitly excluded | Production Reader UI/viewport, SQLite/FTS5 index, scanner, production bookmark editor/local overlay, annotations/ink, migration, installer, OCR, AI document analysis |
| Primary platform | Windows 11, while preserving cross-platform adapter boundaries |
| Inherited toolchain | accepted N1 Qt 6.10.3/MSVC 2022/C++23 baseline; candidate-specific probe tooling must be documented separately |
| Plan | `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` |
| Evidence sheet | `docs/baselines/N2_PDF_ENGINE_MATRIX.md` |
| Proposed decision | `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` |

N2 was deliberately opened on 2026-09-22 after explicit N1 acceptance. No PDF engine is pre-selected. N2 may end with a split architecture where one engine owns read/render/text/navigation and qpdf owns structure/security/transformation.

**N3 remains Not started until N2 has sufficient evidence, final strict CI, an Accepted ADR-0004, and explicit owner `N2 PASS`.**

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
13. N2 candidate experiments must use Atlas-owned normalized contracts; engine-native types cannot leak into portable domain/application code.
14. N2 probe adoption is not production adoption: exact version, provenance, license, checksum/build path and rollback must be recorded before an engine responsibility can be accepted.

## Release ledger

| ID | Planned release | Checkpoint | Status |
|---|---|---|---|
| N0 | `2.0.0-alpha.0` | Native repository bootstrap | **Accepted** |
| N1 | `2.0.0-alpha.1` | Windows toolchain + empty-shell baseline | **Accepted** |
| N2 | `2.0.0-alpha.2` | PDF engine qualification spike | **In progress** |
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

Qt 6.10.3 is the alpha reproducibility pin because unauthenticated public `aqtinstall` Windows 6.11.x binaries were not reproducibly available during N1. This is not a claim that 6.10.3 is preferable to newer Qt; re-qualification is required before release.

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

N2 was deliberately opened only after this acceptance and inherits N1 as its frozen baseline.

---

## N2 — PDF engine qualification spike (`2.0.0-alpha.2`) — In progress

**Goal:** Select PDF responsibilities using real fixtures/benchmarks, not preference, before Atlas builds product features around an engine assumption.

### Binding N2 documents

- plan: `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`;
- evidence matrix: `docs/baselines/N2_PDF_ENGINE_MATRIX.md`;
- fixture contract: `tests/fixtures/pdf/README.md`;
- proposed architecture decision: `docs/decisions/ADR-0004-pdf-engine-responsibilities.md`;
- dependency/upstream rules: `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/UPSTREAM_CATALOG.md`.

### Evaluate

#### Qt PDF

Qualify for:

- document/page open and metadata;
- page labels/sizes/geometry;
- raster rendering;
- Unicode text extraction/search;
- links/navigation;
- outlines/destinations;
- encrypted/malformed behavior relevant to read workflows;
- performance/memory/package impact.

Qt's ready-made PDF viewer UI is **not** the future Atlas reader architecture. Atlas owns its viewport.

#### PDFium

Qualify against the same normalized read/render/text/navigation responsibilities and fixtures as Qt PDF.

N2 must additionally record:

- impact of PDFium's public non-thread-safe API contract;
- the serialization/mutex model required by an Atlas adapter;
- official source build/acquisition complexity;
- any community binary bootstrap provenance/hash separately from upstream source revision;
- runtime/package footprint and update/rollback route.

#### qpdf

Qualify for:

- structural inspection;
- encryption/password/restriction information;
- outline/object-level access needed by later bookmark workflows;
- malformed-file diagnostics/recovery observations;
- preservation-sensitive no-op/rewrite and controlled structural transformation on copied fixtures;
- validation/reopen behavior;
- exact upstream/vcpkg version used.

qpdf is not being evaluated as Atlas's page raster engine.

### N2 phases

**N2.0 — planning/fixture/upstream contract**

- [x] branch from accepted N1 integration state;
- [x] qualification plan;
- [x] durable evidence matrix;
- [x] Proposed ADR-0004;
- [x] fixture corpus contract;
- [x] N2 dependency/upstream rules;
- [x] advance prerelease identifier to `alpha.2` without adding an engine dependency;
- [x] strict Debug + Release CI on the N2.0 baseline head.

**N2.1 — Qt PDF probe**

- [x] focused adapter/probe target;
- [x] normalized fixture runner output;
- [x] open/page/label/geometry evidence;
- [x] render evidence/timing;
- [x] English/Arabic/mixed Unicode extraction and search;
- [x] links/outlines/destinations;
- [x] encrypted/malformed states;
- [x] package/memory notes;
- [x] matrix update.

**N2.2 — PDFium probe**

- [x] exact upstream/binary revision and acquisition provenance for qualification;
- [x] checksum where binary/archive is used;
- [x] focused adapter/probe target;
- [x] same normalized fixture expectations as Qt PDF;
- [x] serialized-call correctness under non-thread-safe API contract;
- [x] render/text/search/navigation evidence;
- [x] encrypted/malformed states;
- [x] qualification build/package/memory cost;
- [x] matrix update.

**N2.3 — qpdf structural/security probe**

- [x] pinned qpdf/version path;
- [x] structural/security inspection evidence;
- [x] outline/destination read evidence needed for later workflows;
- [x] copied-fixture no-op/rewrite preservation evidence;
- [x] controlled structural transformation evidence;
- [x] validation/independent reopen evidence;
- [x] matrix update.

**N2.4 — normalized cross-engine comparison**

- [x] resolve/record meaningful Qt PDF versus PDFium semantic differences;
- [x] classify disagreements as engine limitation, Atlas normalization bug, fixture ambiguity, expected difference, or unresolved blocker;
- [x] compare qualification integration/reproducibility/footprint costs.

**N2.5 — preservation/security comparison**

- [x] verify relevant page/content/annotation/outline/metadata/XMP/encryption/signature/attachment invariants on mutation probes;
- [x] independent validation where practical;
- [x] no unsafe claim that signed integrity remains valid after mutation.

**N2.6 — decision/owner handoff**

- [x] fill proposed final responsibility table in matrix;
- [x] revise Proposed ADR-0004 with selected responsibilities and rejected alternatives/limitations;
- [ ] document exact dependency versions, licenses/notices and rollback route;
- [ ] strict final CI;
- [ ] owner evidence review;
- [ ] explicit owner `N2 PASS`.

### Minimum fixture classes

- simple text;
- page labels;
- Arabic-only text;
- mixed Arabic/English text;
- Arabic/English outlines;
- duplicate/deep outlines;
- internal/external links and destinations;
- rotated/mixed-size/non-default box pages;
- image-only;
- annotations;
- metadata/XMP;
- password/restriction encryption;
- malformed-but-readable;
- signed/certified test document where redistributable;
- long document for timing/memory.

Each tracked fixture requires provenance, redistribution permission, SHA-256, expected behavior and mutation permission. Private owner PDFs may supplement local qualification but never enter public Git/CI logs with personal metadata or passwords.

### Evidence policy

Candidate rows use only:

- **PASS**;
- **PASS WITH LIMITATION**;
- **FAIL**;
- **BLOCKED**;
- **N/A**;
- **PENDING**.

N2 does not use an opaque aggregate score that can hide a correctness or preservation failure.

Hard blockers include silent unrelated-content loss, unreliable required Arabic/Unicode semantics, unacceptable legal/reproducibility constraints, unsafe concurrency assumptions, inability to distinguish critical security states, or engine-native types leaking across the portable Atlas boundary.

### Stop gate

N2 cannot be Accepted until:

1. Qt PDF and PDFium have comparable evidence for required read/render capabilities, or one is explicitly rejected/blocked with documented evidence;
2. qpdf has structural/security/preservation evidence on copied fixtures;
3. Arabic/Unicode, labels, outlines/destinations, rotations/boxes, links, encryption and malformed cases are represented;
4. exact build/acquisition/version/license implications are recorded;
5. engine types remain behind Atlas-owned normalized ports;
6. ADR-0004 assigns responsibilities and rollback/replaceability rules;
7. strict Debug + Release CI passes on the final N2 branch head;
8. the owner explicitly records `N2 PASS`.

**Do not build the final Reader here. N3 does not begin before N2 Accepted.**

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
