# Atlas Reader Native

> Native-performance, local-first reading and research software built around **Index → Reader → Bookmarks**.

**Status:** N0 bootstrap **Accepted**. N1 Windows toolchain + empty-shell baseline **Accepted**. N2 PDF-engine qualification is **In progress**. No production Index/Reader/Bookmark implementation has started.

**Current native build:** `2.0.0-alpha.2`

**Active branch:** `native-v2-n2-pdf-engine-qualification`

**Primary platform:** Windows 11 first

**Planned platform order:** Windows → Android → Linux → macOS → iOS/iPadOS

**Successor to:** the Flutter Atlas Reader implementation. Flutter remains a behavioral/data reference while the native successor is proven checkpoint-by-checkpoint.

## Why the native successor exists

Atlas's long-term workload is closer to a graphics/document application than an ordinary CRUD UI: large PDF libraries, virtualized document canvases, deeply nested outlines, low-latency pen/touch input, safe PDF mutation, printing, Arabic/RTL, accessibility, and extensive local indexing.

The native architecture is therefore designed around:

- **C++23** shared core and performance-sensitive services;
- **Qt 6 + Qt Quick/QML** GPU-backed cross-platform presentation;
- **SQLite + FTS5** local index/search, beginning in N3 rather than N2;
- a replaceable **Atlas-owned PDF abstraction** with Qt PDF/PDFium read-render candidates and qpdf structural-transformation candidate;
- **CMake**;
- **vcpkg manifest mode** for qualified non-Qt native dependencies from N2 onward;
- thin platform adapters for Windows now and Android/Linux/Apple later.

## Product priority

Atlas is not trying to reproduce every function in Acrobat or Foxit. The order is deliberate:

1. **Library / Index** — discover, identify, search, and recover books reliably.
2. **Reader** — render/navigate PDFs quickly and accurately without UI stalls.
3. **Bookmarks / Outlines** — deep research navigation, portable in the PDF when safe and local when necessary.
4. Standard ink/annotations.
5. Printing, covers, portable metadata, migration/backup, and Windows integration.

N2 is a deliberate pre-feature qualification checkpoint: it selects the PDF responsibilities that later Reader/Bookmark work will depend on without building those features early.

The core data rule is:

> **Portable when possible, local when necessary, never lost silently.**

A writable ordinary PDF is the preferred authority for PDF-native data. Restricted, read-only, signed/certified, externally locked, conflicted, offline, or otherwise unsafe-to-mutate documents use an explicit Atlas-local overlay rather than bypassing security or losing research work.

## Release program to 2.0

We do **not** call an empty shell a beta.

- N0: `2.0.0-alpha.0` — **Accepted** architecture/bootstrap.
- N1: `2.0.0-alpha.1` — **Accepted** Windows toolchain + zero-feature performance baseline.
- N2: `2.0.0-alpha.2` — **In progress** PDF-engine qualification.
- N3: `2.0.0-beta.1` — first useful native Library/Index beta.
- N4: `2.0.0-beta.2` — native Reader.
- N5: `2.0.0-beta.3` — complete resilient Bookmarks.
- N6: `2.0.0-beta.4` — ink/annotations.
- N7–N9: further Windows beta qualification.
- N10: `2.0.0-rc.N`.
- N11: stable **`2.0.0`** Windows.

See [`docs/RELEASE_STRATEGY.md`](docs/RELEASE_STRATEGY.md) and [`CHECKPOINTS.md`](CHECKPOINTS.md).

## Native architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                         QML / Qt Quick                       │
│      shell · reader chrome · panels · dialogs · a11y       │
└──────────────────────────────┬──────────────────────────────┘
                               │ controllers / view models
┌──────────────────────────────▼──────────────────────────────┐
│                       Application layer                     │
│       Library · Reader · Bookmarks · Commands · State      │
└──────────────────────────────┬──────────────────────────────┘
                               │ Atlas-owned interfaces
┌──────────────────────────────▼──────────────────────────────┐
│                         C++23 core                          │
│ identity · capability · index · bookmark · reconciliation  │
└───────────────┬────────────────┬────────────────┬───────────┘
                │                │                │
         PDF contracts      SQLite/FTS5      Platform ports
      render / inspect     search / state     filesystem etc.
                │                                 │
     adapters: Qt PDF / PDFium / qpdf      Win → Android → ...
```

The shared domain/core must not depend on Win32, Android APIs, Objective-C/Swift, QML, Qt PDF types, PDFium handles, or qpdf objects. Platform and engine-specific behavior lives behind interfaces/adapters.

## N1: accepted zero-feature baseline

N1 established the reproducible Windows shell/toolchain and performance floor before PDF/index work.

Accepted evidence includes:

- Visual Studio 2022 x64 / MSVC v143 / C++23;
- public Qt 6.10.3 Windows baseline;
- Debug + Release strict CI;
- privacy-safe startup/frame metrics;
- English/LTR, Arabic/RTL and mixed-script proof;
- light/dark proof;
- 100% and 200% scale qualification;
- clean repeated process lifecycle;
- keyboard Tab and Enter/Return activation;
- measured Windows GDI font-backend decision.

The accepted user-qualified runtime is commit `73f567cf2c557f185371d7f944ebf6d69453105a` with artifact digest `sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`.

N1 is frozen. Per repository policy, a passed checkpoint cannot be reopened without new evidence of a user-facing regression.

See [`docs/baselines/N1_FINAL_QUALIFICATION.md`](docs/baselines/N1_FINAL_QUALIFICATION.md).

## N2: qualify PDF responsibilities before building the Reader

N2 is not “pick a favorite PDF library.” It tests responsibility boundaries using the same documented fixtures and normalized Atlas expectations.

### Candidates

- **Qt PDF** — read/page metadata/render/text/search/links/outlines candidate with the lowest expected Qt integration cost.
- **PDFium** — read/render/text/search/navigation candidate with mature low-level APIs, but its upstream public API is not thread-safe and official source integration uses a Chromium-style build stack. These costs are part of the qualification.
- **qpdf** — structure/security/transformation candidate for preservation-sensitive operations; not a page raster engine.

A valid N2 result may deliberately use more than one engine.

### N2 evidence system

- plan: [`docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`](docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md)
- live matrix: [`docs/baselines/N2_PDF_ENGINE_MATRIX.md`](docs/baselines/N2_PDF_ENGINE_MATRIX.md)
- fixture rules: [`tests/fixtures/pdf/README.md`](tests/fixtures/pdf/README.md)
- proposed architecture decision: [`docs/decisions/ADR-0004-pdf-engine-responsibilities.md`](docs/decisions/ADR-0004-pdf-engine-responsibilities.md)

ADR-0004 remains **Proposed**. No engine is production-selected until evidence is complete and the owner explicitly records `N2 PASS`.

### N2 non-scope

N2 does not implement:

- production Reader UI/virtual viewport;
- SQLite/FTS5 library/index;
- scanner/watcher;
- production bookmark editor/local overlay;
- annotations/ink;
- migration/backup;
- installer;
- OCR or AI analysis.

N3 stays closed until N2 Accepted.

## Open-source reuse without lock-in

Planned/qualified candidates include Qt, SQLite/FTS5, qpdf, Qt PDF/PDFium, Catch2, nlohmann/json, Google Benchmark, and small utilities only when they solve a measured need.

Development-only tools include Qt QML Profiler, Tracy, RenderDoc, Windows Performance Analyzer/Recorder, Accessibility Insights, clang-format/tidy, GitHub Actions/CodeQL, and CodeGraph.

Every dependency must pass license, maintenance, portability, abstraction, performance, reproducibility, rollback, and fixture tests. See [`docs/DEPENDENCIES_AND_TOOLS.md`](docs/DEPENDENCIES_AND_TOOLS.md) and [`docs/UPSTREAM_CATALOG.md`](docs/UPSTREAM_CATALOG.md).

## Repository map

```text
.
├── .codex/                  local agent/MCP configuration
├── .github/workflows/       Windows Debug/Release build/test automation
├── docs/
│   ├── baselines/           checkpoint performance/evidence summaries
│   ├── decisions/           Architecture decision records
│   ├── N2_PDF_ENGINE_QUALIFICATION_PLAN.md
│   ├── ARCHITECTURE.md
│   ├── BUILDING.md
│   ├── COMPETITIVE_BASELINE.md
│   ├── CORE_WORKFLOWS.md
│   ├── DATA_MODEL_AND_FORMATS.md
│   ├── DEPENDENCIES_AND_TOOLS.md
│   ├── DEVELOPMENT_WORKFLOW.md
│   ├── FEATURE_SCOPE_2_0.md
│   ├── LICENSING.md
│   ├── MIGRATION_FROM_FLUTTER.md
│   ├── PERFORMANCE.md
│   ├── PLATFORM_ROADMAP.md
│   ├── QUALITY_AND_TESTING.md
│   ├── RELEASE_STRATEGY.md
│   ├── RISK_REGISTER.md
│   ├── SECURITY_MODEL.md
│   ├── SUCCESS_METRICS.md
│   ├── TECH_STACK.md
│   ├── TOOLCHAIN.md
│   ├── UPSTREAM_CATALOG.md
│   └── UX_ACCESSIBILITY_AND_DESIGN.md
├── qml/                     presentation-only QML
├── src/
│   ├── app/                 process/shell/diagnostics infrastructure
│   └── core/                portable C++ contracts/domain logic
├── tests/
│   └── fixtures/pdf/        N2 tracked PDF fixture contract/corpus
├── tools/bench/             local physical-machine benchmark harnesses
├── AGENTS.md
├── CHANGELOG.md
├── CHECKPOINTS.md
├── INFO.md
├── PLAN.md
├── SECURITY.md
└── CMakeLists.txt
```

## Build on Windows

See [`docs/BUILDING.md`](docs/BUILDING.md) for canonical commands.

The N2 branch still builds the inherited N1 shell with the accepted Qt toolchain before any engine-specific probe dependency is enabled:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
cmake --preset windows-msvc2022
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --build --preset windows-release
ctest --preset windows-release
```

Candidate-specific N2 setup commands will be added only when each focused probe is introduced and pinned.

## Documentation hierarchy

Start with [`docs/README.md`](docs/README.md). It points to the authoritative product, architecture, data, toolchain, quality, performance, security, accessibility, dependency, migration, and release documents.

For current work, read `CHECKPOINTS.md` → `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` → `docs/baselines/N2_PDF_ENGINE_MATRIX.md` → Proposed ADR-0004.

## Current checkpoint

**N2 — PDF engine qualification spike:** **In progress**.

All bounded N2 capability blockers now pass. N2 still requires frozen and requalified PDFium/qpdf production routes, exact dependency/provenance/license records, final strict CI, synchronized ADR-0004, and explicit owner `N2 PASS`.

## License

Atlas Reader Native is planned as open-source software under the MIT License. Third-party components retain their own terms; see [`docs/LICENSING.md`](docs/LICENSING.md).
