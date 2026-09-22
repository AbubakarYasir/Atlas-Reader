# Atlas Reader Native — Dependencies, Tools, Plugins, and Reuse Policy

## Purpose

Atlas should reuse mature open-source work where it clearly improves reliability or development speed, but third-party code must not control the product architecture. This document separates what ships in Atlas from what only helps us build it.

## N2 status — 2026-09-22

N2 is now active. **No PDF engine has been accepted for production yet.**

The durable qualification plan is `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`, the evidence sheet is `docs/baselines/N2_PDF_ENGINE_MATRIX.md`, and the architecture decision remains Proposed in `docs/decisions/ADR-0004-pdf-engine-responsibilities.md`.

Current integration facts that must not be lost during experiments:

- Qt PDF is qualified through the existing Qt toolchain as a read/render/text/search/navigation candidate. Atlas will not adopt the complete Qt viewer widget architecture as its reader.
- The first clean Qt PDF core evidence is bound to implementation SHA `213050ee75882ae5fa53f73b07fe2707bbaea5a8`; broader search/navigation/Arabic/security/performance rows remain open.
- PDFium's current public API contract states that PDFium APIs are not thread-safe; Atlas must serialize calls and measure the consequence rather than run unsupported parallel API calls.
- Official PDFium source builds use Chromium-style `depot_tools`/`gclient`, GN, Ninja and Clang/clang-cl tooling. The inspected Microsoft vcpkg registry has no standard `ports/pdfium` package.
- The active N2.2 Windows probe pin is PDFium **156.0.8066.0 / `chromium/8066`** from `bblanchon/pdfium-binaries`, distribution source commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac`, asset `pdfium-win-x64.tgz`, SHA-256 `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`.
- That community PDFium package is **probe-only**. Exact production supply-chain/license/notices/source-build strategy remains undecided and is tracked in `docs/baselines/N2_PDFIUM_PROVENANCE.md`.
- qpdf is the structure/security/transformation candidate. At N2 opening, upstream release, generated documentation and Microsoft vcpkg currently expose different version surfaces; every experiment must record the exact qpdf version it actually uses.
- The inspected vcpkg `qpdf` port is `12.4.0` with `Apache-2.0 AND MIT` metadata. The upstream release feed visible at N2 opening shows `12.4.1` published 2026-08-27, while current generated docs may identify `12.4.2`.

The facts above are inputs to the bake-off, not winner declarations.

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

For N2, a dependency can be **probe-only** without being accepted for production. Probe-only code/configuration must be clearly labeled and remain removable.

## 2. Production dependencies — planned/qualified

### Qt 6 + Qt Quick/QML

**Role:** windowing, input, GPU-backed scene graph, accessibility/i18n primitives, platform abstraction, QML presentation.

**Policy:** use only modules whose open-source licensing/distribution obligations are understood. Avoid GPL-only modules unless the project license/distribution decision explicitly permits them. Dynamic linking and relinkability/license notices must be handled correctly when distributing LGPL Qt builds.

**Boundary:** presentation/application infrastructure only; Atlas domain rules do not depend on QML/Qt GUI types.

**Status:** chosen foundation. N1 Windows baseline Accepted.

### SQLite + FTS5

**Role:** local index, search, app-local state, migrations, transactional persistence.

**Status:** intended production dependency, but **closed during N2**. Implementation belongs to N3.

**Why:** stable embedded transactional database, mature C API, FTS5, cross-platform availability, public-domain core.

**Boundary:** repositories/schema layer. UI never executes SQL directly.

### qpdf

**Role candidate:** structural PDF inspection/transformation, encryption/security information, preservation-sensitive operations.

**Status:** **active N2 qualification; not yet production-selected.**

**Boundary:** PDF infrastructure adapter only. qpdf object types do not escape into Atlas domain/application interfaces.

**N2 acquisition policy:** prefer a pinned vcpkg experiment first because a curated port exists, but record the exact vcpkg baseline, qpdf port version, feature set and transitive libraries. If Atlas needs a newer upstream release than the registry provides, use an explicit override/custom acquisition rather than silently claiming the registry provided it.

### Qt PDF

**Role candidate:** page rendering, text extraction/search, links/navigation, outlines/destinations and document/page inspection.

**Status:** **active N2 bake-off; no winner assumed.** Core A001–A005 smoke is captured; Arabic/search/navigation/security/repeated-performance evidence remains pending.

**Boundary:** infrastructure adapter/probe. Qt PDF types must not become Atlas domain/application types.

**Important:** Atlas owns the future reader viewport. Qualifying Qt PDF does not authorize adopting the full Qt PDF viewer UI as the product architecture.

### PDFium

**Role candidate:** page rendering, text extraction/search, links/navigation, outlines/destinations and low-level inspection.

**Status:** **active N2 bake-off; no winner assumed.** The first N2.2 core Windows smoke is captured on implementation SHA `7874794e14b9cea54ec0723c15963621f65bebf6`: A001–A005 open/page-count/page-label/normalized-visible-size/text expectations pass, and extracted text hashes match Qt PDF page-for-page. Broader navigation/Arabic/security/performance evidence remains pending.

**N2.2 probe pin:** PDFium `156.0.8066.0`, tag `chromium/8066`, `bblanchon/pdfium-binaries` source commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac`, Windows x64 non-V8 archive SHA-256 `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`. The exact provenance contract lives in `docs/baselines/N2_PDFIUM_PROVENANCE.md`.

**Boundary:** infrastructure adapter/probe. PDFium handles/types do not escape into Atlas domain/application interfaces. `atlas_reader` and `atlas_core` do not link PDFium.

**Concurrency constraint:** current upstream public API says PDFium APIs are not thread-safe. The N2 probe uses a serialized single-thread call model. A later Atlas adapter/task-queue test must preserve that contract under concurrent app workloads.

**Supply-chain constraint:** official source integration brings Chromium-style tooling. The current community prebuilt accelerates the spike only because exact tag/source commit/asset/checksum are pinned. Its distributor repository is MIT-licensed, but that does not replace PDFium/third-party notice obligations. Production distribution remains undecided until ADR-0004 is accepted.

Atlas may intentionally use different engines for rendering and structural mutation.

## 3. Small support libraries — preferred candidates

These are not automatically added. Add only when the relevant checkpoint needs them.

| Candidate | Intended role | License/notes | Policy |
|---|---|---|---|
| Catch2 v3 | pure C++ unit/contract tests | Boost Software License 1.0 | Preferred test framework candidate |
| Google Benchmark | repeatable microbenchmarks | permissive upstream license | Use if the Atlas N2 fixture runner needs a dedicated microbenchmark layer |
| spdlog | structured local logging | MIT; uses fmt | Use if Qt logging is insufficient; never log passwords/document text by default |
| nlohmann/json | Atlas bookmark/backup JSON serialization | MIT | Strong candidate for versioned interchange formats; do not add early merely for N2 output if a smaller solution suffices |
| xxHash | fast non-cryptographic identity/cache fingerprint component | BSD-2-Clause | May be one signal in guarded document identity; never sole destructive identity signal |

Use C++23 standard facilities first (`std::expected`, chrono, filesystem, ranges, etc.) before importing replacements.

## 4. Dependency manager

### Chosen direction: vcpkg manifest mode for non-Qt C/C++ dependencies

From N2 onward, Atlas uses a checked-in `vcpkg.json` plus a pinned vcpkg baseline for **accepted or deliberately pinned probe** third-party native libraries where reliable ports exist.

Why:

- project-local declarative dependencies;
- versioning and overrides;
- reproducible CI/dev setup;
- CMake integration;
- Windows-first strength while retaining cross-platform triplets.

Qt itself remains installed/pinned separately because Qt distribution/toolchain handling is specialized and should not be coupled to the vcpkg dependency graph without a measured reason.

N2 rules:

- do not create an empty/decorative manifest merely to say vcpkg is present;
- introduce the manifest with the first real pinned non-Qt probe dependency (expected first candidate: qpdf) or with explicit baseline-only infrastructure if CI needs that before the probe;
- record `builtin-baseline` exactly;
- avoid floating registry head in accepted evidence;
- overrides must state why Atlas differs from the curated registry version;
- PDFium is not forced through vcpkg if no suitable standard port exists.

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

N2 should not profile before a repeatable fixture benchmark shows a question that profiling can answer.

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
- Catch2 — domain/service/adapter contract tests when justified.
- Qt Test — Qt/QML/native event-loop integration where Qt-specific behavior is under test.
- Google Benchmark — optional microbenchmarks.
- **Atlas PDF fixture runner** — required N2 cross-engine evidence tool for real copied/synthetic PDFs and normalized results.

Golden image tests may be used for deterministic rendering components, but visual correctness still needs representative real-document/manual review.

N2 fixture policy is binding in `tests/fixtures/pdf/README.md`.

## 8. Accessibility tools

### Accessibility Insights for Windows

Use Windows UI Automation inspection/FastPass/manual workflows against every user-facing Windows checkpoint.

### Narrator

Manual screen-reader acceptance remains required; automated accessibility checks do not prove a usable reading workflow.

### Qt accessibility inspection/tests

Automated tests should verify semantic names/roles/states for critical custom controls where possible.

N2 is primarily an engine qualification checkpoint, but Arabic/Unicode correctness is mandatory because accessibility/i18n cannot be repaired later if the engine loses semantics at extraction time.

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

N2 engine downloads used in CI must be pinned and checksummed where practical. Do not curl a floating “latest” binary into qualification CI.

Avoid adding cloud CI/review vendors that duplicate GitHub Actions/CodeQL without measurable value.

## 10. Local AI/agent developer tooling

AI can accelerate repository navigation, boilerplate, tests, migration work, and documentation, but generated code has no exemption from evidence gates.

### CodeGraph

Reuse the existing Atlas approach: local code graph indexed from the checkout. It helps agents/developers resolve symbols/callers/impact without repeatedly grepping the repository. The machine-local graph database is never committed.

### GitHub connector/MCP

Use repository/diff/issue/PR/CI context through authorized connectors. Mutation must be explicit and tied to the active task. Credentials stay in environment/secret storage, never files.

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

Do not accept speculative refactors solely because an AI says they are cleaner/faster.

## 11. IDE/editor plugins

No IDE is mandatory. Recommended developer conveniences:

### Visual Studio / VS Code / Qt Creator
