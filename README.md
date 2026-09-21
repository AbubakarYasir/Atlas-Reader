# Atlas Reader Native

> Native-performance, local-first reading and research software built around **Index → Reader → Bookmarks**.

**Status:** N0 bootstrap/documentation/toolchain verification. No production index/reader/bookmark implementation has started.

**Native release line:** `2.0.0`

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

- N0–N2: `2.0.0-alpha.N` engineering/toolchain/PDF-engine qualification.
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

## Open-source reuse without lock-in

Planned production candidates include Qt, SQLite/FTS5, qpdf, Qt PDF/PDFium, Catch2, nlohmann/json, Google Benchmark, and small utilities only when they solve a measured need.

Development-only tools include Qt QML Profiler, Tracy, RenderDoc, Windows Performance Analyzer/Recorder, Accessibility Insights, clang-format/tidy, GitHub Actions/CodeQL, and CodeGraph.

Every dependency must pass license, maintenance, portability, abstraction, performance, and fixture tests. See [`docs/DEPENDENCIES_AND_TOOLS.md`](docs/DEPENDENCIES_AND_TOOLS.md).

## What exists in N0

- clean CMake/C++23 project;
- minimal Qt Quick window proving the application boundary;
- pure C++ interfaces for PDF, filesystem, library index, and document capability;
- core smoke test;
- Windows GitHub Actions workflow;
- project-local CodeGraph/GitHub read-only MCP configuration;
- comprehensive architecture/product/release/quality documentation;
- checkpoint delivery rules.

No PDF renderer, scanner, SQLite schema, bookmark editor, or migration code is intentionally implemented yet.

## Repository map

```text
.
├── .codex/                  local agent/MCP configuration
├── .github/workflows/       Windows build/test automation
├── docs/
│   ├── decisions/           Architecture decision records
│   ├── ARCHITECTURE.md
│   ├── BUILDING.md
│   ├── COMPETITIVE_BASELINE.md
│   ├── CORE_WORKFLOWS.md
│   ├── DEPENDENCIES_AND_TOOLS.md
│   ├── DEVELOPMENT_WORKFLOW.md
│   ├── FEATURE_SCOPE_2_0.md
│   ├── LICENSING.md
│   ├── MIGRATION_FROM_FLUTTER.md
│   ├── PERFORMANCE.md
│   ├── PLATFORM_ROADMAP.md
│   ├── QUALITY_AND_TESTING.md
│   ├── RELEASE_STRATEGY.md
│   ├── TECH_STACK.md
│   └── UX_ACCESSIBILITY_AND_DESIGN.md
├── qml/                     presentation-only QML
├── src/
│   ├── app/                 process/bootstrap layer
│   └── core/                portable C++ contracts/domain logic
├── tests/                   native core and later integration tests
├── AGENTS.md
├── CHANGELOG.md
├── CHECKPOINTS.md
├── INFO.md
├── PLAN.md
└── CMakeLists.txt
```

## Build the bootstrap on Windows

See [`docs/BUILDING.md`](docs/BUILDING.md) for canonical instructions.

The preferred product line is Qt 6.11.x, but public bootstrap CI currently uses Qt 6.10.3 because the public aqt Windows 6.11 repository installation path is failing upstream. N1 must establish one reproducible canonical Qt patch across local development and CI before feature work.

## Documentation hierarchy

Start here:

1. [`PLAN.md`](PLAN.md) — master product/engineering plan.
2. [`CHECKPOINTS.md`](CHECKPOINTS.md) — exact execution order and alpha/beta/RC gates.
3. [`docs/FEATURE_SCOPE_2_0.md`](docs/FEATURE_SCOPE_2_0.md) — what Windows 2.0 includes/excludes.
4. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architectural boundaries/threading/data flow.
5. [`docs/CORE_WORKFLOWS.md`](docs/CORE_WORKFLOWS.md) — Index/Reader/Bookmark capability/failure/recovery contract.
6. [`docs/TECH_STACK.md`](docs/TECH_STACK.md) + [`docs/DEPENDENCIES_AND_TOOLS.md`](docs/DEPENDENCIES_AND_TOOLS.md) — stack, open-source reuse, plugins/tools/agents.
7. [`docs/QUALITY_AND_TESTING.md`](docs/QUALITY_AND_TESTING.md) + [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) — measurable acceptance/evidence.
8. [`docs/UX_ACCESSIBILITY_AND_DESIGN.md`](docs/UX_ACCESSIBILITY_AND_DESIGN.md) — UX, keyboard, Narrator, RTL design contract.
9. [`docs/COMPETITIVE_BASELINE.md`](docs/COMPETITIVE_BASELINE.md) — how we learn from giants without becoming a clone.
10. [`docs/RELEASE_STRATEGY.md`](docs/RELEASE_STRATEGY.md) — version/release rules.
11. [`docs/DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md) — branches/PRs/ADRs/AI-agent workflow.
12. [`docs/PLATFORM_ROADMAP.md`](docs/PLATFORM_ROADMAP.md), [`docs/MIGRATION_FROM_FLUTTER.md`](docs/MIGRATION_FROM_FLUTTER.md), [`docs/LICENSING.md`](docs/LICENSING.md), [`docs/BUILDING.md`](docs/BUILDING.md).

## Current checkpoint

**N0 — Native repository bootstrap.** It remains **In progress** until the corrected public Windows CI successfully builds/tests and the owner accepts this architecture/documentation program. N1 must not start before that.

## License

Atlas Reader Native is planned as open-source software under the MIT License. Third-party components retain their own terms; see [`docs/LICENSING.md`](docs/LICENSING.md).