# Atlas Reader Native — Dependencies, Tools, Plugins, and Reuse Policy

## Purpose

Atlas should reuse mature open-source work where it clearly improves reliability or development speed, but third-party code must not control the product architecture. This document separates what ships in Atlas from what only helps us build it.

## 1. Dependency rules

Every proposed dependency must answer:

1. What exact problem does it solve better than a small Atlas implementation?
2. Is the project actively maintained and usable on the planned platforms?
3. Is its license compatible with Atlas distribution?
4. Can it be isolated behind an Atlas-owned interface?
5. What is its binary/CPU/memory/startup cost?
6. What happens if the dependency is abandoned or changes license?
7. Can we test its behavior with fixtures rather than trust marketing/docs?

A dependency is rejected when it duplicates the standard library/Qt without material benefit, expands the attack surface for convenience, forces product data into a proprietary format, or makes Android/Apple portability materially harder.

## 2. Production dependencies — planned/qualified

### Qt 6 + Qt Quick/QML

**Role:** windowing, input, GPU-backed scene graph, accessibility/i18n primitives, platform abstraction, QML presentation.

**Policy:** use only modules whose open-source licensing/distribution obligations are understood. Avoid GPL-only modules unless the project license/distribution decision explicitly permits them. Dynamic linking and relinkability/license notices must be handled correctly when distributing LGPL Qt builds.

**Boundary:** presentation/application infrastructure only; Atlas domain rules do not depend on QML/Qt GUI types.

### SQLite + FTS5

**Role:** local index, search, app-local state, migrations, transactional persistence.

**Status:** intended production dependency.

**Why:** stable embedded transactional database, mature C API, FTS5, cross-platform availability, public-domain core.

**Boundary:** repositories/schema layer. UI never executes SQL directly.

### qpdf

**Role candidate:** structural PDF inspection/transformation, encryption/security information, preservation-sensitive operations.

**Status:** qualify in N2 before production use.

**Boundary:** PDF infrastructure adapter only. qpdf object types do not escape into Atlas domain/application interfaces.

### Qt PDF and PDFium

**Role candidates:** page rendering, text extraction/search/link/navigation/inspection.

**Status:** N2 bake-off. No winner is assumed before benchmark/fidelity/Arabic/edge-case evidence.

Atlas may intentionally use different engines for rendering and structural mutation.

## 3. Small support libraries — preferred candidates

These are not automatically added in N0. Add only when the relevant checkpoint needs them.

| Candidate | Intended role | License/notes | Policy |
|---|---|---|---|
| Catch2 v3 | pure C++ unit/contract tests | Boost Software License 1.0 | Preferred test framework candidate |
| Google Benchmark | repeatable microbenchmarks | permissive upstream license | Preferred microbenchmark harness |
| spdlog | structured local logging | MIT; uses fmt | Use if Qt logging is insufficient; never log passwords/document text by default |
| nlohmann/json | Atlas bookmark/backup JSON serialization | MIT | Strong candidate for versioned interchange formats |
| xxHash | fast non-cryptographic identity/cache fingerprint component | BSD-2-Clause | May be one signal in guarded document identity; never sole destructive identity signal |

Use C++23 standard facilities first (`std::expected`, `std::format` where toolchain support is acceptable, chrono, filesystem, ranges, etc.) before importing replacements.

## 4. Dependency manager

### Chosen direction: vcpkg manifest mode for non-Qt C/C++ dependencies

From N2 onward, Atlas should use a checked-in `vcpkg.json` plus a pinned vcpkg baseline for third-party native libraries where reliable ports exist.

Why:

- project-local declarative dependencies;
- versioning and overrides;
- reproducible CI/dev setup;
- CMake integration;
- Windows-first strength while retaining cross-platform triplets.

Qt itself remains installed/pinned separately because Qt distribution/toolchain handling is specialized and should not be coupled to the vcpkg dependency graph without a measured reason.

If a future mobile dependency cannot be handled cleanly by vcpkg, isolate that exception rather than replacing the whole dependency strategy casually.

## 5. Profiling and diagnostics — development only

These tools do **not** ship as runtime dependencies.

### Qt QML Profiler

Use for bindings, QML creation, JavaScript, animations, frame timing, and QML-side stalls. A QML performance issue should be profiled before being rewritten blindly.

### Tracy Profiler

Use for C++ CPU zones, thread interaction, task queues, locks, frame timing, allocations where useful. Instrument Atlas-owned hot paths, not user document contents.

### RenderDoc

Use to inspect Direct3D/Vulkan/OpenGL frame captures for Atlas's own rendering, page texture behavior, overdraw, resource churn, and GPU mistakes.

### Windows Performance Recorder / Analyzer

Use for system-level startup, file I/O, CPU scheduling, memory, disk, and input latency investigations on Windows.

### Visual Studio Profiler / Diagnostics Tools

Use for native CPU/memory diagnostics and debugger integration.

## 6. Static analysis and code hygiene

Planned gates:

- `clang-format` — canonical formatting;
- `clang-tidy` — correctness/performance/modernize checks with curated rules;
- MSVC warnings at high level and warnings-as-errors in CI after baseline cleanup;
- optional clang-cl diagnostic build;
- CMake configure warnings treated as actionable;
- include-what-you-use considered after architecture stabilizes, not before;
- sanitizer jobs where supported and meaningful (ASan/UBSan on suitable platforms/toolchains).

Do not enable hundreds of noisy static rules merely to display a badge. A rule belongs in CI only when the team intends to fix violations.

## 7. Testing tools

- CTest — orchestration/source of truth for native test execution.
- Catch2 — domain/service/adapter contract tests.
- Qt Test — Qt/QML/native event-loop integration where Qt-specific behavior is under test.
- Google Benchmark — microbenchmarks and repeatable isolated performance tests.
- fixture runner — Atlas-owned executable/tests for real copied PDFs and library trees.

Golden image tests may be used for deterministic rendering components, but visual correctness still needs representative real-document/manual review.

## 8. Accessibility tools

### Accessibility Insights for Windows

Use Windows UI Automation inspection/FastPass/manual workflows against every user-facing Windows checkpoint.

### Narrator

Manual screen-reader acceptance remains required; automated accessibility checks do not prove a usable reading workflow.

### Qt accessibility inspection/tests

Automated tests should verify semantic names/roles/states for critical custom controls where possible.

## 9. GitHub automation

### Required

- GitHub Actions — Windows build/test now; later Linux/macOS/platform matrices.
- release workflow — build/package/checksum/SBOM/notices from tags only.
- dependency/security review at update checkpoints.
- CodeQL or equivalent GitHub-native code scanning once the C++ source surface is meaningful.
- secret scanning / push protection where available.

### Recommended later

- artifact retention for benchmark reports, test logs, and candidate packages;
- GitHub issue/PR templates tied to checkpoint evidence;
- changelog/release-note generation only after PR labeling becomes consistent.

Avoid adding cloud CI/review vendors that duplicate GitHub Actions/CodeQL without measurable value.

## 10. Local AI/agent developer tooling

AI can accelerate repository navigation, boilerplate, tests, migration work, and documentation, but generated code has no exemption from evidence gates.

### CodeGraph

Reuse the existing Atlas approach: local code graph indexed from the checkout. It helps agents/developers resolve symbols/callers/impact without repeatedly grepping the repository. The machine-local graph database is never committed.

### GitHub read-only connector/MCP

Use for repository, diff, issue, PR, and CI inspection. Credentials stay in environment/secret storage, never files.

### AGENTS.md

`AGENTS.md` is the durable contract for coding agents: architecture boundaries, current checkpoint, required docs, security rules, and commands. Agents must not skip checkpoints or mark planned work complete.

### Rule for generated patches

An AI-generated change is accepted only when it:

- fits the current checkpoint;
- compiles;
- passes tests/static analysis;
- includes tests for new behavior;
- respects architecture/dependency policy;
- updates documentation when behavior/contracts change;
- contains no secrets or copied incompatible code.

## 11. IDE/editor plugins

No IDE is mandatory. Recommended developer conveniences:

### Visual Studio / VS Code / Qt Creator

- CMake integration;
- clangd/C++ language services;
- QML language tooling;
- CMake Tools where applicable;
- EditorConfig;
- clang-format integration;
- Git/GitHub integration.

Qt Creator is especially useful for QML Profiler and Qt diagnostics; Visual Studio remains useful for MSVC/native Windows profiling/debugging. We should not force a single editor when the build is command-line reproducible.

## 12. Packaging tools

Do not lock a Windows installer technology before N11. Evaluate at packaging time against licensing, update/repair/uninstall behavior, file association, signing, CI automation, and maintenance cost.

WiX is a known option but current WiX project terms/maintenance requirements must be reviewed at selection time. CPack/NSIS/other approaches remain candidates. Packaging technology must never own application configuration/data formats.

## 13. Tools/repositories intentionally rejected or deferred

- Electron/Tauri/WebView as the Atlas UI runtime — rejected for the core document canvas architecture.
- proprietary PDF SDK as an architectural dependency — rejected unless an explicit later business decision changes licensing/distribution and an open abstraction remains.
- MuPDF integration — deferred because licensing/distribution needs a separate deliberate review; do not casually add it merely for performance.
- cloud telemetry/crash upload — deferred; Atlas is local-first and must not silently transmit user/document data.
- OCR engines — out of Windows 2.0 scope.
- AI document analysis — out of Windows 2.0 scope.

## 14. Dependency update policy

Dependency updates are not background churn. Each update PR/checkpoint records:

- old/new version;
- security relevance;
- license changes;
- API/ABI impact;
- binary size change where material;
- benchmark delta for hot-path dependencies;
- fixture/regression result;
- rollback plan.

Major dependency upgrades require an ADR when they change architecture or data behavior.

## References checked at bootstrap

- Qt 6.11 licensing documentation (LGPLv3/GPL/commercial; some modules GPL-only).
- Microsoft vcpkg manifest-mode documentation (recommended mode with versioning/custom registries).
- qpdf 12.4.1 release.
- SQLite public-domain/copyright documentation and FTS5 docs.
- Catch2 BSL-1.0 licensing.
- Tracy BSD-3-Clause licensing.
- RenderDoc MIT/open-source project.
- Accessibility Insights for Windows open-source project.