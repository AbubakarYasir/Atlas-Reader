# Technology Stack

## Decision summary

Atlas Reader Native uses **C++23 + Qt 6 / Qt Quick** as the primary application stack.

The choice is based on Atlas's actual workload: graphics-heavy PDF rendering, large virtualized documents, stylus/touch interaction, native desktop integration, printing, local filesystem/indexing, and a later mobile path.

## Toolchain baseline

### Product target

Preferred native product line: **Qt 6.11.x**, currently evaluated against Qt 6.11.2 at bootstrap. Qt 6.11 contains changes relevant to fluid UI and rendering, so N1 explicitly re-qualifies the newest 6.11 patch that is reproducibly installable across developer machines and CI.

### Public bootstrap CI pin

Public GitHub CI temporarily uses **Qt 6.10.3** because the public `aqtinstall`/`install-qt-action` path currently fails to install Windows Qt 6.11.x repository metadata reliably. This is an upstream installer/repository limitation, not a product architecture downgrade. We do not require private Qt-account secrets merely to make public CI green.

N1 exit requires one canonical Qt patch version for both local release development and CI; changing it requires the normal dependency qualification gate.

### Compiler/build

- C++23
- MSVC / Visual Studio 2022-compatible Qt toolchain for Windows baseline
- CMake >= 3.28
- Ninja for fast CI/local single-config builds where convenient
- CTest for orchestration
- clang-cl as an additional diagnostic build after baseline qualification

## C++23

Why:

- native performance and predictable control over allocation/lifetime;
- direct interoperability with PDF/database/native libraries;
- one shared compiled core across target platforms;
- mature profiling/sanitizer/toolchain ecosystem;
- suitable for custom rendering and input hot paths.

Rule: C++ is not automatically fast. Atlas still enforces thread, cache, allocation, and benchmark discipline.

Prefer standard C++23 facilities before adding helper libraries when compiler/library support is adequate.

## Qt 6 + Qt Quick / QML

Qt provides:

- Windows, Android, Linux, macOS, and iOS support;
- Qt Quick scene graph with hardware-backed rendering;
- QML for responsive declarative UI;
- window/input/accessibility/i18n primitives;
- platform abstraction without forcing web technologies into the render loop.

### QML boundary

Use QML for:

- shell;
- panels;
- lists/grids;
- reader chrome;
- dialogs;
- animations;
- responsive layouts.

Do **not** put document/database/business rules into QML JavaScript. Controllers/view models expose explicit state/commands from the application layer.

## PDF strategy

No single PDF engine is accepted in N0.

### Qt PDF

Candidate for page rendering, text, search, links, and navigation because it integrates naturally with Qt. It must be benchmarked and fixture-tested before production selection.

### PDFium

Candidate alternative/secondary renderer/inspector if Qt PDF performance, fidelity, text extraction, edge-case behavior, or roadmap is insufficient.

### qpdf

Bootstrap planning baseline: **qpdf 12.4.1**.

Candidate for structural PDF inspection/transformation, encryption/security information, and preservation-sensitive operations. qpdf is not a page renderer; separating these responsibilities is useful.

### Engine rule

Atlas depends on `IPdfEngine`/facades, not vendor object models. Rendering and structural transformation may intentionally use different libraries. Engine choice is finalized only by N2 ADR + benchmark + preservation/Arabic/security fixture evidence.

## SQLite + FTS5

Planned local index/search store.

Bootstrap upstream reference: SQLite 3.53.x line (current upstream release at documentation review: 3.53.4).

Why:

- mature embedded database;
- ACID transactions;
- FTS5 local search;
- stable single-file format;
- broad platform support;
- public-domain core.

Atlas uses direct SQLite/repository abstractions rather than allowing a UI ORM to define the data model.

## Dependency management

### Decision: vcpkg manifest mode for non-Qt native dependencies

Beginning when N2 introduces production native dependencies, check in:

- `vcpkg.json`;
- `vcpkg-configuration.json` when needed;
- a pinned vcpkg baseline.

This gives project-local declarative dependencies, version constraints/overrides, CMake integration, and reproducible CI.

Qt remains installed/pinned independently because its SDK/toolchain distribution is specialized.

Do not mix random `FetchContent`, vendored archives, global vcpkg classic installs, and system libraries without a documented exception.

## Small support libraries

Preferred candidates, added only when needed:

- **Catch2 v3** — C++ unit/contract tests;
- **Google Benchmark** — microbenchmarks;
- **nlohmann/json** — versioned Atlas JSON interchange/backup/bookmark bundles;
- **spdlog** — structured local logging only if Qt logging does not satisfy requirements;
- **xxHash** — fast fingerprint component, never a sole destructive identity signal.

See `DEPENDENCIES_AND_TOOLS.md` for acceptance rules and development-only tooling.

## Profiling stack

Development-only:

- Qt QML Profiler;
- Tracy;
- RenderDoc;
- Visual Studio Profiler;
- Windows Performance Recorder/Analyzer.

We profile measured problems rather than replacing architecture based on intuition.

## Testing

- CTest — orchestration;
- Catch2 — portable core/service tests;
- Qt Test — Qt/QML/event-loop integration;
- Google Benchmark — microbenchmarks;
- Atlas fixture-driven PDF/library integration tests;
- manual Narrator/pen/touch/printer qualification where automation is insufficient.

## Static/security tooling

Planned as source surface grows:

- clang-format;
- curated clang-tidy;
- high MSVC warnings;
- clang-cl diagnostic build;
- sanitizers where supported;
- GitHub CodeQL C/C++;
- secrets scanning;
- dependency/license/SBOM generation for releases.

## Packaging

Installer technology is intentionally unselected until N11. Candidates must be compared for open-source terms, reliable upgrade/uninstall, optional file association, CI packaging, signing support, and maintenance burden.

Do not let installer technology own Atlas settings/data architecture.

## Tools deliberately not chosen

### Flutter

The existing application remains a product/data behavior reference, but the native successor removes framework/rendering constraints for a document/graphics-heavy workload.

### Electron/Tauri/WebView UI

Good general application choices, but Atlas should not place a browser rendering layer between stylus/page rendering and native graphics when the central workload is a document canvas.

### Rust + emerging GUI toolkit

Rust is attractive for safety/performance, but the GUI/mobile/native-integration risk is higher for this roadmap than mature Qt. Isolated Rust components behind a C ABI remain possible only if a measured future need justifies them.

### .NET/Avalonia

Strong alternative, but Atlas would still depend heavily on native PDF/graphics libraries. C++/Qt keeps those boundaries direct in hot paths.

### Proprietary PDF SDK as foundation

Not accepted for Windows 2.0 architecture. A future business decision may evaluate one behind existing Atlas interfaces, but product data/behavior must not become proprietary-SDK locked.

## Version source of truth

Do not duplicate pins arbitrarily. CI/toolchain manifests are implementation pins; documentation explains rationale. Any meaningful version change updates CHANGELOG and, if architectural/data/licensing behavior changes, an ADR.
