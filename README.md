# Atlas Reader Native

> Native-performance, local-first reading and research software built around **Index → Reader → Bookmarks**.

**Status:** bootstrap only — architecture, documentation, build skeleton, and interfaces. No production reader/index/bookmark implementation has started.

**Primary platform:** Windows 11 first

**Planned platform order:** Windows → Android → Linux → macOS → iOS/iPadOS

**Successor to:** `AbubakarYasir/Atlas-Reader` (Flutter implementation). The existing repository remains the behavioral/reference implementation while this native successor is proven checkpoint-by-checkpoint.

## Why this repository exists

Atlas Reader has outgrown the assumptions of a general application UI toolkit. Its long-term workload is closer to a graphics/document application: large PDF libraries, deeply nested outlines, responsive continuous rendering, pen/touch input, safe PDF mutation, printing, Arabic/RTL, and extensive local indexing.

The native successor is therefore designed around:

- **C++23** for the shared core and performance-sensitive services.
- **Qt 6.11.x + Qt Quick/QML** for GPU-backed cross-platform UI.
- **SQLite + FTS5** for local indexing/search.
- A replaceable **PDF engine abstraction**; Qt PDF/PDFium are rendering candidates and qpdf is the preferred structural-transformation candidate.
- **CMake** as the cross-platform build system.
- Thin platform adapters for Windows now and Android/Linux/Apple platforms later.

As of the bootstrap date, Qt **6.11.2** and qpdf **12.4.1** are the verified current upstream releases used by the planning documents. Dependency versions are not silently upgraded; each change must pass the relevant benchmark and compatibility gates.

## Product priority

Atlas is not trying to reproduce every feature in Acrobat or Foxit. The product order is deliberate:

1. **Library / Index** — discover, identify, search, and track books reliably.
2. **Reader** — render and navigate PDFs quickly, accurately, and without UI stalls.
3. **Bookmarks / Outlines** — deep hierarchical research navigation, portable in the PDF when safe and local when necessary.
4. Standard annotations/writing.
5. Printing, richer metadata, and secondary workflows.

The core data rule is:

> **Portable when possible, local when necessary, never lost silently.**

A writable ordinary PDF is the preferred authority for PDF-native data. Restricted, read-only, signed/certified, externally locked, conflicted, offline, or otherwise unsafe-to-mutate documents use an explicit Atlas-local overlay rather than bypassing security or losing research work.

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
                               │ pure interfaces
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

## What exists in this bootstrap

- clean CMake/C++23 project;
- a minimal Qt Quick window proving the application boundary;
- pure C++ interfaces for PDF, filesystem, library index, and document capability;
- a tiny core smoke test;
- Windows CI skeleton;
- architecture, performance, platform, licensing, build, migration, and core-workflow documentation;
- checkpoint-based delivery rules similar to the original Atlas repository.

No PDF renderer, scanner, SQLite schema, bookmark editor, or migration code is intentionally implemented yet.

## Repository map

```text
.
├── .github/workflows/       Windows build/test automation
├── docs/
│   ├── decisions/           Architecture decision records
│   ├── ARCHITECTURE.md
│   ├── BUILDING.md
│   ├── CORE_WORKFLOWS.md
│   ├── LICENSING.md
│   ├── MIGRATION_FROM_FLUTTER.md
│   ├── PERFORMANCE.md
│   ├── PLATFORM_ROADMAP.md
│   └── TECH_STACK.md
├── qml/                     Presentation-only QML
├── src/
│   ├── app/                 Process/bootstrap layer
│   └── core/                Portable C++ contracts and domain logic
├── tests/                   Native core and later integration tests
├── AGENTS.md
├── CHANGELOG.md
├── CHECKPOINTS.md
├── INFO.md
├── PLAN.md
└── CMakeLists.txt
```

## Build the bootstrap on Windows

See [`docs/BUILDING.md`](docs/BUILDING.md) for the canonical instructions.

Short version from a Visual Studio 2022 Developer PowerShell with Qt 6.11.2 available:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.11.2\msvc2022_64'
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
.\build\windows-debug\atlas_reader.exe
```

The exact Qt installation directory may differ.

## Documentation hierarchy

- [`PLAN.md`](PLAN.md) — product/engineering north star and non-negotiable principles.
- [`CHECKPOINTS.md`](CHECKPOINTS.md) — implementation order, evidence, and stop gates.
- [`INFO.md`](INFO.md) — compact orientation/status note.
- [`docs/CORE_WORKFLOWS.md`](docs/CORE_WORKFLOWS.md) — Index/Reader/Bookmark capability and failure-state contract.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — boundaries, components, threading, and data flow.
- [`docs/TECH_STACK.md`](docs/TECH_STACK.md) — why each technology exists and what may replace it.
- [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) — budgets and benchmark rules.
- [`docs/PLATFORM_ROADMAP.md`](docs/PLATFORM_ROADMAP.md) — Windows-first without Windows-only architecture.
- [`docs/MIGRATION_FROM_FLUTTER.md`](docs/MIGRATION_FROM_FLUTTER.md) — migration policy and compatibility strategy.
- [`docs/LICENSING.md`](docs/LICENSING.md) — dependency/license guardrails.
- [`CHANGELOG.md`](CHANGELOG.md) — repository history.

## Current checkpoint

**N0 — Native repository bootstrap.** Architecture and boilerplate only. N1 does not begin until this bootstrap is reviewed and accepted.

## License

Atlas Reader Native is planned as open-source software under the MIT License. Third-party components retain their own licenses; see [`docs/LICENSING.md`](docs/LICENSING.md).