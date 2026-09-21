# Atlas Reader Native — Master Plan to 2.0

**Status:** Native successor bootstrap / N0

**Primary implementation target:** Windows 11

**Future platform order:** Android → Linux → macOS → iOS/iPadOS

**Core stack:** C++23 · Qt 6/Qt Quick/QML · SQLite/FTS5 · replaceable PDF-engine layer · qpdf candidate · CMake · vcpkg manifest for non-Qt dependencies

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

## II. Windows 2.0 product pillars

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

### Pillar 4 — Standard Writing and Annotations (P1)

After the first three pillars are dependable: low-latency live ink, highlighter, selection/erase/undo/redo, standard markup/notes where interop passes. Live drawing is a GPU/UI overlay first; PDF persistence happens asynchronously through the same safe-save capability model.

### Pillar 5 — Essential Desktop Utilities (P1)

Printing, cover generation, useful document details, selected portable metadata editing, Windows shell integration, installer, backup/migration.

### Pillar 6 — Platform Quality

Windows reaches full 2.0 release quality first. Later platforms adapt storage/input/shell/printing/lifecycle/UI while reusing the core rules and most QML components.

## III. Scope contract

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

## IV. Architectural constraints

### Pure shared core

Reusable domain/application logic uses standard C++ types where practical and never exposes Qt GUI, Win32, Java/Kotlin, Swift/Objective-C, Windows HANDLEs, Android URIs, or Apple security-scoped handles as domain identity.

### UI boundary

QML handles presentation/interaction composition. Business rules do not live in QML JavaScript. Controllers/view models translate to application commands/state.

### PDF boundary

Application code depends on Atlas PDF contracts/facades, never directly on Qt PDF, PDFium, or qpdf. N2 selects responsibilities by fixture/benchmark/interop evidence.

### Storage boundary

SQLite + FTS5 behind schema/migrations/repositories. Writes are serialized/batched as appropriate. UI never executes arbitrary SQL.

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

## V. Threading/performance model

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

## VI. Safety and data ownership

Every PDF mutation rechecks:

1. document identity/location;
2. external source revision;
3. filesystem writability;
4. PDF security/permission capability;
5. signed/certified consequence;
6. destination/storage availability;
7. preservation invariants.

Commit model: **temp → validate → backup/replace → reopen → re-index → mark committed**.

Local data is never labeled Embedded before successful validated re-read.

A non-writable document is not a broken research workflow; Atlas keeps local state clearly and exportably.

See `docs/CORE_WORKFLOWS.md`.

## VII. Accessibility and internationalization

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

See `docs/UX_ACCESSIBILITY_AND_DESIGN.md`.

## VIII. Quality model

A feature is Done only when relevant layers have evidence:

- domain/unit tests;
- adapter/component tests;
- integration/fixture tests;
- preservation/round-trip tests;
- Release build;
- benchmark budget for hot paths;
- Arabic/RTL/accessibility evidence;
- failure/recovery evidence;
- documentation status update;
- owner checkpoint acceptance.

Critical defects found in beta/owner testing receive regression tests or a documented manual checklist item.

See `docs/QUALITY_AND_TESTING.md`.

## IX. Dependency/open-source strategy

Atlas deliberately reuses mature components where quality improves:

- Qt for cross-platform native UI/platform infrastructure;
- SQLite/FTS5 for local transactional search/index;
- qpdf candidate for PDF structure/transformation;
- Qt PDF/PDFium candidates for rendering/inspection;
- Catch2/Qt Test for testing;
- Google Benchmark for performance tests;
- nlohmann/json candidate for versioned Atlas JSON interchange;
- xxHash candidate as one guarded fingerprint signal;
- spdlog only if needed;
- vcpkg manifest mode for non-Qt native dependency reproducibility.

Development-only quality tools include QML Profiler, Tracy, RenderDoc, WPA/WPR, Visual Studio profiler, Accessibility Insights, clang-format/tidy, CodeQL, CodeGraph, and GitHub Actions.

No dependency bypasses license/performance/portability/abstraction review.

See `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/LICENSING.md`.

## X. Competitive strategy

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

## XI. Release program

Native V2 uses:

- N0–N2: `2.0.0-alpha.N` engineering previews;
- N3 onward: `2.0.0-beta.N` usable milestone builds;
- N10: `2.0.0-rc.N`;
- N11: stable `2.0.0` Windows.

Later Android/Linux/macOS/iOS adaptations occur after Windows 2.0 rather than delaying Windows indefinitely.

See `docs/RELEASE_STRATEGY.md` and `CHECKPOINTS.md`.

## XII. Success definition for Windows 2.0

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

## XIII. Migration rule

The Flutter application is a **behavior/data reference**, not a source-code template. Port contracts deliberately; do not recreate its widget/service structure in C++/QML.

Native and Flutter builds may run side-by-side through beta. PDFs are the strongest portable bridge. Legacy app-local migration is implemented only after the native schema/identity model is stable enough to avoid repeated destructive converters.

## XIV. Documentation map

- `README.md` — entry point/current status.
- `PLAN.md` — this master plan.
- `CHECKPOINTS.md` — execution order/releases/stop gates.
- `INFO.md` — compact orientation.
- `docs/FEATURE_SCOPE_2_0.md` — exact Windows 2.0 product scope.
- `docs/ARCHITECTURE.md` — boundaries/data/threading.
- `docs/CORE_WORKFLOWS.md` — capability/local-overlay/save/conflict contract.
- `docs/TECH_STACK.md` — technical choices and alternatives.
- `docs/DEPENDENCIES_AND_TOOLS.md` — open-source/tool/plugin/agent policy.
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
