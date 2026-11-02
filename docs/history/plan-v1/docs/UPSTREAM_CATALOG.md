# Atlas Reader Native — Upstream Project Catalog

This is the short operational catalog of projects/tools Atlas may rely on or use during development. Inclusion here does **not** mean every project is already a runtime dependency. `DEPENDENCIES_AND_TOOLS.md`, `TECH_STACK.md`, ADRs, and the active checkpoint determine adoption status.

## N2 upstream snapshot — 2026-09-22

N2 deliberately distinguishes an upstream project, a package-manager port, and a third-party binary distribution. They are not interchangeable provenance.

- **Qt PDF** is part of the Qt 6 ecosystem and is qualified through a focused Atlas adapter/probe rather than by adopting Qt's complete viewer UI.
- **PDFium** official source uses Chromium-style `depot_tools`/`gclient`, GN, Ninja and Clang/clang-cl tooling. Its public embedder API states that PDFium APIs are not thread-safe; Atlas experiments must serialize calls instead of benchmarking unsupported concurrent API use.
- The inspected Microsoft vcpkg registry does **not** expose a standard `ports/pdfium` package. A community prebuilt can therefore be useful to bootstrap the N2 spike, but it does not become trusted production provenance merely by being convenient.
- `bblanchon/pdfium-binaries` is an optional N2-only bootstrap candidate for pinned precompiled PDFium packages. Any use must record exact package/tag, embedded PDFium revision when available, SHA-256 and third-party license/notices. Final supply-chain suitability remains an ADR decision.
- **qpdf** has a current Microsoft vcpkg port. At N2 opening the inspected port reports `12.4.0`; the latest published qpdf release visible in the upstream release feed is `12.4.1` dated 2026-08-27, while current generated documentation may already identify `12.4.2`. Atlas records the exact version surface actually used instead of silently treating these as the same.

These notes are qualification inputs, not engine-selection conclusions.

## Runtime / production candidates

| Project | Upstream | Role in Atlas | Adoption gate |
|---|---|---|---|
| Qt 6 | https://www.qt.io/ / https://code.qt.io/ | Cross-platform UI, input, accessibility, platform infrastructure | Chosen foundation; N1 baseline Accepted |
| Qt PDF | https://doc.qt.io/qt-6/qtpdf-index.html | PDF render/text/search/navigation candidate | **Active N2 qualification** |
| PDFium | https://pdfium.googlesource.com/pdfium/ | PDF render/text/search/inspection candidate | **Active N2 qualification**; non-thread-safe API contract must be respected |
| qpdf | https://github.com/qpdf/qpdf | PDF structure/security/transformation candidate | **Active N2 qualification** |
| SQLite | https://sqlite.org/ | Local transactional index/state | N3 implementation; closed during N2 |
| FTS5 | https://sqlite.org/fts5.html | Local full-text search | N3 implementation; closed during N2 |
| nlohmann/json | https://github.com/nlohmann/json | Atlas bookmark/backup JSON candidate | N5/N8 when serialization lands |
| xxHash | https://github.com/Cyan4973/xxHash | Fast fingerprint/cache component candidate | N3 identity/cache only if useful |
| spdlog | https://github.com/gabime/spdlog | Structured logging candidate | Add only if Qt/native logging proves insufficient |

## N2 PDF acquisition candidates

| Candidate | Provenance | Intended N2 use | Current policy |
|---|---|---|---|
| Qt PDF module | Qt distribution/toolchain | Qt PDF adapter/probe | Allowed for N2 qualification; exact Qt kit recorded |
| Official PDFium source | Google PDFium source | Canonical upstream/reference build path | Preferred provenance; integration/toolchain cost is part of the bake-off |
| `bblanchon/pdfium-binaries` | Community automated binary distribution | Fast N2 Windows probe bootstrap | Spike-only unless later approved; exact archive/revision/hash/notices required |
| qpdf via vcpkg | Microsoft curated vcpkg port sourcing qpdf | qpdf N2 adapter/probe | Preferred first reproducible experiment path; exact vcpkg baseline and qpdf port version required |
| qpdf upstream release/source | qpdf upstream | Cross-check/version override if needed | Allowed when justified and pinned; document divergence from vcpkg |

A convenience bootstrap must never become an undocumented production dependency.

## Build / dependency management

| Project | Upstream | Role | Gate |
|---|---|---|---|
| CMake | https://cmake.org/ | Canonical Atlas native build system | Chosen |
| Ninja | https://ninja-build.org/ | Fast local/CI builds; also part of official PDFium source workflow | Atlas convenience / PDFium upstream requirement as applicable |
| vcpkg | https://github.com/microsoft/vcpkg | Manifest-mode non-Qt dependency management | **Introduce in N2 when first accepted non-Qt probe dependency requires it; pin baseline** |
| depot_tools / gclient / GN | Chromium/PDFium infrastructure | Official PDFium source acquisition/configuration | Development/probe tooling only unless final PDFium supply chain requires it |
| install-qt-action | https://github.com/jurplel/install-qt-action | Public CI Qt installation | Existing CI tool |

Qt itself remains separately pinned/installed; do not force Qt into the vcpkg graph merely for uniformity.

## Testing / benchmarking

| Project | Upstream | Role | Gate |
|---|---|---|---|
| CTest | https://cmake.org/cmake/help/latest/manual/ctest.1.html | Test orchestration | Chosen |
| Catch2 | https://github.com/catchorg/Catch2 | C++ unit/contract testing | Introduce when N2 adapter contract surface justifies it |
| Qt Test | https://doc.qt.io/qt-6/qttest-index.html | Qt/event-loop integration tests | Qt-specific adapter/UI behavior |
| Google Benchmark | https://github.com/google/benchmark | Repeatable microbenchmarks | Candidate for engine microbenchmarks if the Atlas fixture runner alone is insufficient |
| Atlas PDF fixture runner | this repository | Cross-engine normalized evidence | **Required N2 work** |

## Profiling / graphics diagnostics

| Project/tool | Upstream | Use |
|---|---|---|
| Qt QML Profiler | https://doc.qt.io/qt-6/qtquick-profiling.html | QML creation/bindings/animation/frame analysis |
| Tracy | https://github.com/wolfpld/tracy | C++ CPU/thread/task/lock/timing profiling |
| RenderDoc | https://github.com/baldurk/renderdoc | GPU frame/resource/overdraw inspection |
| Windows Performance Recorder / Analyzer | Microsoft Windows ADK | Startup, I/O, scheduling, memory, system-level traces |
| Visual Studio Profiler | Visual Studio | Native CPU/memory/debug diagnostics |

N2 should prefer simple repeatable timing in the fixture runner before bringing in a profiler. Profilers answer a measured anomaly; they are not substitutes for benchmarks.

## Accessibility / UX diagnostics

| Tool | Upstream | Use |
|---|---|---|
| Accessibility Insights for Windows | https://github.com/microsoft/accessibility-insights-windows | UI Automation/FastPass/manual a11y checks |
| Windows Narrator | Windows | Manual screen-reader acceptance |
| Qt accessibility APIs | https://doc.qt.io/qt-6/accessible.html | Runtime semantic roles/names/states |

N2 is mostly non-production probe UI/CLI work, but Arabic/Unicode document semantics remain a mandatory engine criterion.

## Code quality / security

| Tool | Upstream | Use |
|---|---|---|
| clang-format | LLVM | Canonical formatting |
| clang-tidy | LLVM | Curated correctness/performance/static checks |
| clang-cl | LLVM | Secondary diagnostic Windows compiler; also relevant to official PDFium build model |
| AddressSanitizer / UBSan | compiler toolchains | Memory/undefined-behavior diagnostics where supported |
| GitHub CodeQL | GitHub | C/C++ security/static analysis once source surface is meaningful |
| GitHub secret scanning / push protection | GitHub | Prevent credential leakage |
| Syft (candidate) | https://github.com/anchore/syft | SBOM generation candidate for RC/release |

## Agent / repository intelligence

| Tool | Upstream / configuration | Use |
|---|---|---|
| CodeGraph | https://github.com/colbymchenry/codegraph | Local symbol/caller/impact graph; database remains machine-local |
| GitHub MCP/connector | project-local `.codex/config.toml` | Repository/diff/CI context and explicit scoped mutations |
| `AGENTS.md` | this repository | Durable rules for coding agents |

AI/agent tooling never becomes a runtime dependency and never lowers review/test requirements.

## IDE/editor conveniences

No mandatory IDE. Recommended choices:

- **Qt Creator** — QML tooling, QML Profiler, Qt-native project diagnostics.
- **Visual Studio** — MSVC, native debugger/profiler, Windows integration.
- **VS Code** — CMake Tools, clangd/C++ tooling, QML extension/language tooling, EditorConfig, Git/GitHub.

The repository remains buildable from documented command-line tools so no editor plugin becomes part of the architecture.

## Packaging candidates — intentionally unresolved

Evaluate only at N11:

- CPack;
- NSIS;
- WiX after current licensing/maintenance terms are reviewed;
- other Windows installer technology only if it improves upgrade/uninstall/file-association/signing behavior measurably.

## Explicitly not adopted for Windows 2.0 foundation

- Electron/Tauri/WebView as UI runtime;
- proprietary PDF SDK as core architecture;
- MuPDF without a separate licensing/distribution decision;
- cloud telemetry/crash-upload SDKs;
- OCR engines;
- AI document-analysis SDKs;
- full office-conversion stacks.

## Rule

Before adding anything from this catalog to production:

1. identify the active checkpoint problem;
2. verify upstream maintenance/version/license;
3. place it behind the correct Atlas boundary;
4. add a pinned/reproducible dependency entry;
5. add fixture/tests/benchmarks relevant to its job;
6. update notices/SBOM plan;
7. record an ADR if the choice materially constrains architecture or data formats.

During N2, “added to a probe” and “accepted for production” are explicitly different states. The final production responsibility assignment comes only from accepted ADR-0004 after owner `N2 PASS`.
