# Technology Stack

## Decision summary

Atlas Reader Native uses **C++23 + Qt 6/Qt Quick** as the primary application stack.

The choice is based on Atlas's actual workload: graphics-heavy PDF rendering, large virtualized documents, stylus/touch interaction, native desktop integration, printing, local filesystem/indexing, and a later mobile path.

## C++23

Why:

- native performance and predictable control over allocation/lifetime;
- direct interoperability with PDF/database/native libraries;
- one shared compiled core across target platforms;
- mature profiling/sanitizer/toolchain ecosystem;
- suitable for custom rendering and input hot paths.

Rule: C++ is not automatically fast. Atlas still enforces thread, cache, allocation, and benchmark discipline.

## Qt 6.11.x

Bootstrap target: **Qt 6.11.2**.

Qt provides:

- Windows, Android, Linux, macOS, and iOS support;
- Qt Quick scene graph with hardware-backed rendering;
- QML for responsive declarative UI;
- window/input/accessibility/i18n primitives;
- platform abstraction without forcing web technologies into the render loop.

Version policy: pin a tested minor/patch for CI/release. Update only through a dependency checkpoint with tests and benchmarks.

## Qt Quick / QML

Use for visual composition only:

- shell;
- panels;
- lists/grids;
- reader chrome;
- dialogs;
- animations;
- responsive layouts.

Do not put document/database/business rules into QML JavaScript.

## PDF strategy

No single engine is selected in N0.

### Qt PDF

Candidate for initial page rendering, text, search, links, and navigation because it integrates naturally with Qt. It must be benchmarked and fixture-tested before becoming the production backend.

### PDFium

Candidate alternative/secondary rendering engine if Qt PDF performance, fidelity, annotation, or edge-case behavior is insufficient.

### qpdf

Bootstrap planning baseline: **qpdf 12.4.1**.

Candidate for structural PDF inspection/transformation, encryption/security information, and preservation-sensitive operations. qpdf is not a page renderer; that separation is useful.

### Rule

`IPdfEngine`/facades isolate Atlas from all of them. Engine selection may differ by responsibility.

## SQLite + FTS5

Planned local index/search store.

Why:

- mature embedded database;
- strong transactional semantics;
- FTS5 for fast local search;
- easy backup/inspection;
- available across every target platform.

Atlas should use direct SQLite/repository abstractions rather than allowing a UI framework ORM to define data architecture.

## CMake

Repository requires CMake >= 3.28; bootstrap development is compatible with current CMake 4.x. The documented upstream current release at bootstrap is 4.4.3.

Why:

- Qt's first-class build integration;
- cross-platform native standard;
- IDE/CI support;
- target-based dependency management.

## Windows compiler

Visual Studio 2022 MSVC toolchain is the initial baseline because Qt supplies matching Windows binaries and it integrates well with Win32 tooling/profiling.

Clang-cl may later be added as an additional diagnostic build, not as a replacement before qualification.

## Dependency management

N0 intentionally avoids a package-manager commitment for qpdf/SQLite. N2 will compare vcpkg/system/source strategies against:

- reproducibility;
- Windows developer setup;
- Android/iOS cross-builds;
- license/notice generation;
- CI cache behavior;
- patchability/security response.

Do not add a dependency manager merely to make the tree look complete.

## Testing

- CTest for orchestration;
- lightweight pure C++ tests for core;
- Qt Test or Catch2/GoogleTest may be introduced after N1/N2 based on need;
- fixture-driven PDF engine tests;
- native platform integration tests;
- benchmark executables with machine/build metadata.

## Tools deliberately not chosen

### Flutter

The existing application demonstrated value but the native successor is intended to remove framework/rendering constraints for a document/graphics-heavy workload.

### Electron/Tauri/WebView UI

Good general application choices, but Atlas should not place a browser rendering layer between stylus/page rendering and native graphics when the central workload is a document canvas.

### Rust + emerging GUI toolkit

Rust is attractive for safety/performance, but the GUI/mobile/native-integration ecosystem would add product risk compared with Qt for this specific roadmap. Rust components remain possible behind C ABI boundaries later if justified.

### .NET/Avalonia

Strong alternative and likely productive, but Atlas would still depend heavily on native PDF/graphics libraries. The chosen C++/Qt architecture keeps those boundaries direct and minimizes managed/native crossings in hot paths.

## Version source of truth

Do not duplicate dependency versions across arbitrary files. CI/toolchain files are implementation pins; this document explains rationale. When versions change, CHANGELOG and relevant decision record must explain why.