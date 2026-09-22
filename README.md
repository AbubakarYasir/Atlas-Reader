# Atlas Reader Native

> Native-performance, local-first reading and research software built around **Index → Reader → Bookmarks**.

**Status:** N0 bootstrap **Accepted**. N1 Windows toolchain + empty-shell baseline is **In progress**. No production index/reader/bookmark implementation has started.

**Current native build:** `2.0.0-alpha.1`

**Primary platform:** Windows 11 first

**Planned platform order:** Windows → Android → Linux → macOS → iOS/iPadOS

**Successor to:** the Flutter Atlas Reader implementation. Flutter remains a behavioral/data reference while the native successor is proven checkpoint-by-checkpoint.

## Why the native successor exists

Atlas's long-term workload is closer to a graphics/document application than an ordinary CRUD UI: large PDF libraries, virtualized document canvases, deeply nested outlines, low-latency pen/touch input, safe PDF mutation, printing, Arabic/RTL, accessibility, and extensive local indexing.

The native architecture is therefore designed around:

- **C++23** shared core and performance-sensitive services;
- **Qt 6 + Qt Quick/QML** GPU-backed cross-platform presentation;
- **SQLite + FTS5** local index/search;
- a replaceable **PDF engine abstraction** with Qt PDF/PDFium rendering candidates and qpdf structural-transformation candidate;
- **CMake**;
- planned **vcpkg manifest mode** for non-Qt native dependencies;
- thin platform adapters for Windows now and Android/Linux/Apple later.

## Product priority

Atlas is not trying to reproduce every function in Acrobat or Foxit. The order is deliberate:

1. **Library / Index** — discover, identify, search, and recover books reliably.
2. **Reader** — render/navigate PDFs quickly and accurately without UI stalls.
3. **Bookmarks / Outlines** — deep research navigation, portable in the PDF when safe and local when necessary.
4. Standard ink/annotations.
5. Printing, covers, portable metadata, migration/backup, and Windows integration.

The core data rule is:

> **Portable when possible, local when necessary, never lost silently.**

A writable ordinary PDF is the preferred authority for PDF-native data. Restricted, read-only, signed/certified, externally locked, conflicted, offline, or otherwise unsafe-to-mutate documents use an explicit Atlas-local overlay rather than bypassing security or losing research work.

## Release program to 2.0

We do **not** call an empty shell a beta.

- N0: `2.0.0-alpha.0` — accepted architecture/bootstrap.
- N1: `2.0.0-alpha.1` — active Windows toolchain + zero-feature performance baseline.
- N2: `2.0.0-alpha.2` — PDF-engine qualification.
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
         PDF engine         SQLite/FTS5      Platform ports
      render / inspect     search / state     filesystem etc.
                │                                 │
      Qt PDF / PDFium / qpdf              Win → Android → ...
```

The shared domain/core must not depend on Win32, Android APIs, Objective-C/Swift, or QML. Platform-specific behavior lives behind interfaces.

## N1: measure before adding features

N1 exists so later regressions have a known zero-feature baseline.

The alpha shell now provides:

- reproducible Visual Studio 2022 x64 CMake presets;
- public Debug + Release CI;
- strict compiler warnings-as-errors in CI;
- privacy-safe logging categories;
- startup/QML-load/first-frame/shutdown metrics;
- deterministic resize/frame-pacing exercise;
- English/LTR ↔ Arabic/RTL shell proof;
- light/dark shell proof;
- repeatable automatic startup/shutdown;
- local PowerShell memory/CPU/startup benchmark harness.

No PDF renderer, SQLite schema, folder scanner, Library, Reader, Bookmark editor, or annotation engine is permitted in N1.

### N1 toolchain

Canonical public alpha baseline:

- Visual Studio 17 2022 x64 / MSVC v143;
- C++23;
- Qt **6.10.3** MSVC 2022 64-bit;
- CMake >= 3.28.

Qt 6.11.2 is the preferred current product-generation line and may be used as an additional compatibility build, but public Windows `aqtinstall` 6.11.x binary installation is currently unreliable. Atlas therefore keeps public alpha CI on the reproducible 6.10.3 pin instead of requiring private Qt-account credentials.

See [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md) and [`docs/decisions/ADR-0003-n1-qt-toolchain-pin.md`](docs/decisions/ADR-0003-n1-qt-toolchain-pin.md).

### N1 evidence

Target-machine performance and manual RTL/scale checks live in:

[`docs/baselines/N1_WINDOWS_BASELINE.md`](docs/baselines/N1_WINDOWS_BASELINE.md)

Raw machine-specific benchmark files are ignored by Git and should not be committed by default.

## Open-source reuse without lock-in

Planned production candidates include Qt, SQLite/FTS5, qpdf, Qt PDF/PDFium, Catch2, nlohmann/json, Google Benchmark, and small utilities only when they solve a measured need.

Development-only tools include Qt QML Profiler, Tracy, RenderDoc, Windows Performance Analyzer/Recorder, Accessibility Insights, clang-format/tidy, GitHub Actions/CodeQL, and CodeGraph.

Every dependency must pass license, maintenance, portability, abstraction, performance, and fixture tests. See [`docs/DEPENDENCIES_AND_TOOLS.md`](docs/DEPENDENCIES_AND_TOOLS.md) and [`docs/UPSTREAM_CATALOG.md`](docs/UPSTREAM_CATALOG.md).

## Repository map

```text
.
├── .codex/                  local agent/MCP configuration
├── .github/workflows/       Windows Debug/Release build/test automation
├── docs/
│   ├── baselines/           checkpoint performance/evidence summaries
│   ├── decisions/           Architecture decision records
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
├── tests/                   native core and later integration tests
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

In short:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
cmake --preset windows-msvc2022
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --build --preset windows-release
ctest --preset windows-release
```

## Documentation hierarchy

Start with [`docs/README.md`](docs/README.md). It points to the authoritative product, architecture, data, toolchain, quality, performance, security, accessibility, dependency, migration, and release documents.

## Current checkpoint

**N1 — Windows toolchain + empty-shell baseline:** **In progress**.

N2 must not begin until N1 has strict Debug/Release CI evidence, real Windows target-machine baseline measurements, RTL/scale manual checks, and explicit owner PASS.

## License

Atlas Reader Native is planned as open-source software under the MIT License. Third-party components retain their own terms; see [`docs/LICENSING.md`](docs/LICENSING.md).
