# Atlas Reader Native — Master Plan to 2.0

**Status:** N2 PDF-engine qualification — In progress

**Current engineering preview:** `2.0.0-alpha.2`

**Accepted predecessors:** N0 and N1

**Primary implementation target:** Windows 11

**Future platform order:** Android → Linux → macOS → iOS/iPadOS

**Core stack:** C++23 · Qt 6/Qt Quick/QML · SQLite/FTS5 from N3 · replaceable Atlas-owned PDF contracts · Qt PDF/PDFium/qpdf qualification in N2 · CMake · vcpkg manifest for qualified non-Qt dependencies

## I. North Star

Atlas Reader is a fast, local-first reading and research environment where a user can find a book, get to the exact place, preserve deep research navigation, and keep ownership of their data without being trapped by a proprietary cloud database.

The native successor exists to make that promise compatible with graphics-heavy PDF workloads, low-latency pen input, very large local libraries, safe PDF mutation, and long-term desktop/mobile deployment.

### Non-negotiable principles

| Principle | Meaning |
|---|---|
| **Index → Reader → Bookmarks** | Core product work is prioritized in that order |
| **Native responsiveness** | Expensive work never executes synchronously on the UI/render thread |
| **Portable when possible** | PDF-native data belongs in the PDF when safe and permitted |
| **Local when necessary** | Non-writable/unsafe documents retain research usability through explicit local overlays |
| **Never lost silently** | No false save success, blind overwrite, ambiguous relinking, or destructive unavailable-root reconciliation |
| **Core is portable** | Shared domain/application rules contain no Windows-only assumptions |
| **Engine replaceability** | PDF vendor libraries remain behind Atlas-owned interfaces |
| **Arabic/RTL first-class** | Unicode, bidi, Arabic UI, mixed-script metadata/outlines are release requirements |
| **Accessibility first** | Keyboard, Narrator/UIA, visible focus, text scale, high contrast are architectural gates |
| **Evidence before claims** | Planned, Implemented, Verified, Accepted, Released are distinct |
| **Measure before optimize** | Profilers/benchmarks identify bottlenecks; “C++” alone is not a performance guarantee |
| **Scope beats feature-count** | Atlas competes by excellence in chosen workflows, not by cloning every Acrobat/Foxit function |
| **Checkpoint immutability** | A passed checkpoint is not reopened without new evidence of a user-facing regression |

## II. Current execution state

### N0 — Accepted

Native repository/bootstrap architecture accepted on 2026-09-22.

### N1 — Accepted

Windows toolchain + empty-shell baseline accepted on 2026-09-22 after physical performance, Arabic/RTL/theme/scale/lifecycle and keyboard qualification.

Accepted user-qualified N1 runtime:

`73f567cf2c557f185371d7f944ebf6d69453105a`

N1 is frozen unless concrete user-facing regression evidence appears.

### N2 — Active

N2 was deliberately opened only after N1 acceptance and branched from accepted N1 integration commit:

`f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`

N2 exists to decide PDF **responsibilities** before product Reader/Index/Bookmark code depends on one engine assumption.

Binding N2 documents:

- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`;
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md`;
- `tests/fixtures/pdf/README.md`;
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md`.

ADR-0004 remains **Proposed** and no PDF engine is production-selected while N2 is In progress.

N3 remains closed until explicit owner `N2 PASS`.

## III. Windows 2.0 product pillars

### Pillar 1 — Library and Index (P0)

- explicit library roots plus direct-open PDFs;
- progressive bounded scanning;
- SQLite/FTS5 metadata/bookmark search;
- guarded identity across safe moves/renames;
- unavailable/offline/restricted/unreadable states;
- no destructive root purge when enumeration fails;
- Arabic/Unicode paths/metadata;
- Recents/Favorites/Folders and Command Center;
- measurable responsiveness at large scale.

Implementation begins in N3, after N2 PDF responsibilities are accepted.

### Pillar 2 — Reader (P0)

- Atlas-owned GPU-backed virtualized page viewport;
- visible/near-visible page texture priority;
- bounded caches/memory;
- continuous/page reading modes;
- free zoom + fit page/width;
- page index/label/academic-offset separation;
- thumbnails, outline, text/search/links where supported;
- multi-document workspace;
- byte-backed/read-session behavior where useful;
- no PDF parse/render/save or large SQL on UI thread.

The production reader begins in N4. N2 only qualifies the engines/adapters it may use.

### Pillar 3 — Bookmarks and Outlines (P0 / strategic differentiator)

- deep hierarchy, duplicate visible names, exact destinations/breadcrumb identity;
- complete create/edit/move/reorder/nest/delete;
- library-wide search/Command Center;
- embedded PDF outline when safe;
- first-class local overlay for restricted/read-only/signed/offline/conflicted documents;
- security/password capability model;
- external-revision conflict/reconciliation;
- safe promotion into PDF;
- lossless Atlas JSON + Markdown/CSV import/export;
- backup/recovery.

Production bookmark implementation begins in N5. N2 may inspect/transform outlines only as fixture-based engine qualification.

### Pillar 4 — Standard Writing and Annotations (P1)

After the first three pillars are dependable: low-latency live ink, highlighter, selection/erase/undo/redo, standard markup/notes where interop passes. Live drawing is a GPU/UI overlay first; PDF persistence happens asynchronously through the same safe-save capability model.

### Pillar 5 — Essential Desktop Utilities (P1)

Printing, cover generation, useful document details, selected portable metadata editing, Windows shell integration, installer, backup/migration.

### Pillar 6 — Platform Quality

Windows reaches full 2.0 release quality first. Later platforms adapt storage/input/shell/printing/lifecycle/UI while reusing the core rules and most QML components.

## IV. Scope contract

The detailed Windows 2.0 feature contract lives in `docs/FEATURE_SCOPE_2_0.md`. P0 features cannot be silently dropped during implementation.

Explicitly deferred from Windows 2.0 unless owner scope changes:

- OCR;
- cloud accounts/sync;
- AI document chat/analysis;
- PDF password cracking/restriction bypass;
- arbitrary PDF text/object editing;
- forms/signature-authoring workflow;
- office conversion;
- multimedia/3D;
- full EPUB rendering/annotation.

A competitor having one of these does not make Atlas 2.0 a failure. Core quality has priority over feature-count vanity.

## V. Architectural constraints

### Pure shared core

Reusable domain/application logic uses standard C++ types where practical and never exposes Qt GUI, Win32, Java/Kotlin, Swift/Objective-C, Windows HANDLEs, Android URIs, Apple security-scoped handles, Qt PDF objects, PDFium handles, or qpdf object types as domain identity.

### UI boundary

QML handles presentation/interaction composition. Business rules do not live in QML JavaScript. Controllers/view models translate to application commands/state.

### PDF boundary

Application code depends on Atlas PDF contracts/facades, never directly on Qt PDF, PDFium, or qpdf.

N2 intentionally permits responsibility splitting. The working conceptual boundary is:

```text
Atlas application/domain
        |
        v
Atlas normalized PDF contracts
        |
        +----------------------+----------------------+
        |                      |                      |
        v                      v                      v
read/render/text          navigation/outline     structure/write
Qt PDF or PDFium          Qt PDF or PDFium            qpdf
```

This is a hypothesis shape, not a final selection. ADR-0004 decides only after evidence.

### Storage boundary

SQLite + FTS5 remain behind schema/migrations/repositories. Writes are serialized/batched as appropriate. UI never executes arbitrary SQL. SQLite implementation is closed until N3.

### Platform boundary

At minimum:

- filesystem/storage/document-reference port;
- watcher;
- picker;
- printing;
- stylus/touch specialization;
- shell/open-with/share integration;
- lifecycle/power/session behavior;
- secure credential facility only if a future ADR introduces persistent secret storage.

See `docs/ARCHITECTURE.md`.

## VI. N2 PDF-engine qualification contract

### Qt PDF

Qualify as a candidate for:

- open/page metadata;
- page labels/geometry;
- raster rendering;
- Unicode text extraction/search;
- links/navigation;
- outlines/destinations.

Atlas qualifies engine APIs, not Qt's complete viewer UI.

### PDFium

Qualify against the same normalized read/render/text/navigation expectations.

N2 treats as evidence-relevant constraints:

- current public PDFium API is documented as non-thread-safe, so adapter calls must be serialized correctly;
- official source builds use Chromium-style `depot_tools`/`gclient`, GN/Ninja and Clang/clang-cl tooling;
- no standard Microsoft vcpkg `pdfium` port was found in the inspected registry;
- any community prebuilt used for a spike requires exact pin, SHA-256, provenance and notices and does not pre-approve production distribution.

### qpdf

Qualify primarily for:

- structural inspection;
- encryption/security/restriction information;
- outline/object-level operations needed by later workflows;
- preservation-sensitive transformations/validation.

At N2 opening, current upstream release/docs/vcpkg version surfaces are not identical. Exact qpdf version must be recorded for each experiment rather than inferred from “latest.”

### N2 evidence vocabulary

Per-capability results are:

- PASS;
- PASS WITH LIMITATION;
- FAIL;
- BLOCKED;
- N/A;
- PENDING.

There is no aggregate score that can hide a fatal correctness, Arabic/Unicode, preservation, licensing, concurrency or reproducibility failure.

### N2 fixture discipline

Committed fixtures require stable IDs, provenance, redistribution permission, SHA-256, expected behavior and mutation permission. Private owner PDFs remain outside Git and public logs.

Mutation probes always use disposable copies.

### N2 stop gate

N2 requires:

- comparable Qt PDF/PDFium evidence or explicit evidence-backed rejection/blocking;
- qpdf structural/security/preservation evidence;
- Arabic/Unicode, labels, links, outlines/destinations, rotations/boxes, encryption and malformed fixtures;
- exact versions/acquisition/licenses/build paths;
- adapter boundary proof;
- final responsibility table;
- Accepted ADR-0004;
- strict final CI;
- explicit owner `N2 PASS`.

Until then N3 is Not started.

## VII. Threading/performance model

The GUI/render thread must not perform:

- directory traversal;
- PDF parsing/structural validation;
- CPU page rasterization;
- cover generation;
- large hashing/fingerprinting;
- large SQL queries;
- PDF serialization/save;
- large import/export;
- expensive tree reconciliation.

Bounded queues prioritize interactive viewport/search work over prefetch/background scan work. Cancellation/reprioritization is first-class.

Performance budgets/evidence rules live in `docs/PERFORMANCE.md`; profiling/tooling policy lives in `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/QUALITY_AND_TESTING.md`.

N2 timing comparisons use Release builds, the same fixture/work request, declared warm/cold context, median/p95 where appropriate, and correctness before speed.

## VIII. Safety and data ownership

Every production PDF mutation will eventually recheck:

1. document identity/location;
2. external source revision;
3. filesystem writability;
4. PDF security/permission capability;
5. signed/certified consequence;
6. destination/storage availability;
7. preservation invariants.

Future commit model: **temp → validate → backup/replace → reopen → re-index → mark committed**.

N2 does not implement the production save pipeline. It uses copied fixtures to discover what each candidate preserves/changes and to inform later safe-save implementation.

Local data is never labeled Embedded before successful validated re-read.

A non-writable document is not a broken research workflow; Atlas keeps local state clearly and exportably.

See `docs/CORE_WORKFLOWS.md`.

## IX. Accessibility and internationalization

From the first user-facing beta:

- English + Arabic resources;
- RTL layout/bidi isolation;
- Arabic/English/Urdu mixed fixtures;
- keyboard-only operation;
- Narrator/UI Automation semantics;
- visible focus;
- 200% UI/text testing;
- system high-contrast behavior;
- non-color-only state;
- reduced motion where relevant.

N9 is final qualification, not the first implementation pass.

N2 must already include Arabic/Unicode text/search/outline evidence because an engine that loses those semantics cannot be repaired by UI polish later.

See `docs/UX_ACCESSIBILITY_AND_DESIGN.md`.

## X. Quality model

A feature or engine responsibility is Done only when relevant layers have evidence:

- domain/unit tests;
- adapter/component tests;
- integration/fixture tests;
- preservation/round-trip tests;
- Release build;
- benchmark budget for hot paths;
- Arabic/RTL/accessibility evidence where applicable;
- failure/recovery evidence;
- documentation status update;
- owner checkpoint acceptance.

Critical defects found in beta/owner testing receive regression tests or a documented manual checklist item.

See `docs/QUALITY_AND_TESTING.md`.

## XI. Dependency/open-source strategy

Atlas deliberately reuses mature components where quality improves:

- Qt for cross-platform native UI/platform infrastructure;
- SQLite/FTS5 for local transactional search/index beginning N3;
- qpdf candidate for PDF structure/transformation in N2;
- Qt PDF/PDFium candidates for rendering/inspection in N2;
- Catch2/Qt Test for testing when justified;
- Google Benchmark for performance tests where the fixture runner needs dedicated microbenchmarks;
- nlohmann/json candidate for versioned Atlas JSON interchange;
- xxHash candidate as one guarded fingerprint signal;
- spdlog only if needed;
- vcpkg manifest mode for qualified non-Qt native dependency reproducibility.

Development-only quality tools include QML Profiler, Tracy, RenderDoc, WPA/WPR, Visual Studio profiler, Accessibility Insights, clang-format/tidy, CodeQL, CodeGraph, and GitHub Actions.

No dependency bypasses license/performance/portability/abstraction/reproducibility review.

Probe-only adoption and production adoption are distinct states.

See `docs/DEPENDENCIES_AND_TOOLS.md`, `docs/UPSTREAM_CATALOG.md`, and `docs/LICENSING.md`.

## XII. Competitive strategy

Adobe/Foxit define mature security/interoperability/desktop expectations; Librera informs reader/library flow; Xournal++ demonstrates pen/local-layer value; SumatraPDF reinforces startup/minimal-latency expectations; Okular is a useful open-source cross-platform reference.

Atlas differentiation is the combination of:

- serious local library;
- fast native reader;
- deep first-class bookmarks;
- protected-document local fallback;
- local-first ownership;
- Arabic/RTL;
- accessibility;
- transparent conflict/recovery.

See `docs/COMPETITIVE_BASELINE.md`.

## XIII. Release program

Native V2 uses:

- N0: `2.0.0-alpha.0` — Accepted;
- N1: `2.0.0-alpha.1` — Accepted;
- N2: `2.0.0-alpha.2` — In progress engineering qualification;
- N3 onward: `2.0.0-beta.N` usable milestone builds;
- N10: `2.0.0-rc.N`;
- N11: stable `2.0.0` Windows.

Later Android/Linux/macOS/iOS adaptations occur after Windows 2.0 rather than delaying Windows indefinitely.

See `docs/RELEASE_STRATEGY.md` and `CHECKPOINTS.md`.

## XIV. Success definition for Windows 2.0

Atlas 2.0 succeeds when:

- all P0 Windows scope is Accepted;
- no known core path silently loses research data;
- protected/signed/read-only/offline/conflicted PDFs have explicit tested behavior;
- large libraries remain usable while indexing;
- large PDFs navigate with bounded memory and responsive UI;
- 10k bookmark search/navigation meets recorded target;
- embedded portable data round-trips in independent readers for accepted fixtures;
- backup/restore and legacy migration are tested;
- Arabic/RTL/Narrator matrix has no untested core workflow;
- installer/upgrade/uninstall preserve user books/data;
- exact dependency/toolchain/SBOM/notices and release evidence are archived;
- owner explicitly accepts the release candidate.

## XV. Migration rule

The Flutter application is a **behavior/data reference**, not a source-code template. Port contracts deliberately; do not recreate its widget/service structure in C++/QML.

Native and Flutter builds may run side-by-side through beta. PDFs are the strongest portable bridge. Legacy app-local migration is implemented only after the native schema/identity model is stable enough to avoid repeated destructive converters.

## XVI. Documentation map

- `README.md` — entry point/current status.
- `PLAN.md` — this master plan.
- `CHECKPOINTS.md` — execution order/releases/stop gates.
- `INFO.md` — compact orientation.
- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` — active N2 execution plan.
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md` — active N2 evidence sheet.
- `tests/fixtures/pdf/README.md` — N2 fixture contract.
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` — Proposed N2 architecture decision.
- `docs/FEATURE_SCOPE_2_0.md` — exact Windows 2.0 product scope.
- `docs/ARCHITECTURE.md` — boundaries/data/threading.
- `docs/CORE_WORKFLOWS.md` — capability/local-overlay/save/conflict contract.
- `docs/TECH_STACK.md` — technical choices and alternatives.
- `docs/DEPENDENCIES_AND_TOOLS.md` — open-source/tool/plugin/agent policy.
- `docs/UPSTREAM_CATALOG.md` — concrete upstream candidates/current integration facts.
- `docs/PERFORMANCE.md` — performance budgets/benchmarks.
- `docs/QUALITY_AND_TESTING.md` — test/evidence/security quality gates.
- `docs/UX_ACCESSIBILITY_AND_DESIGN.md` — UX/a11y/RTL contract.
- `docs/COMPETITIVE_BASELINE.md` — competitor research/differentiation.
- `docs/RELEASE_STRATEGY.md` — alpha/beta/RC/stable rules.
- `docs/DEVELOPMENT_WORKFLOW.md` — branches/PRs/ADRs/agent workflow.
- `docs/PLATFORM_ROADMAP.md` — Windows-first portability program.
- `docs/MIGRATION_FROM_FLUTTER.md` — legacy migration.
- `docs/BUILDING.md` — canonical build instructions.
- `docs/LICENSING.md` — license/distribution guardrails.
- `docs/decisions/` — ADRs.
