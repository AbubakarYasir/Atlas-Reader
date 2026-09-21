# Atlas Reader Native — Upstream Project Catalog

This is the short operational catalog of projects/tools Atlas may rely on or use during development. Inclusion here does **not** mean every project is already a runtime dependency. `DEPENDENCIES_AND_TOOLS.md`, `TECH_STACK.md`, ADRs, and the active checkpoint determine adoption status.

## Runtime / production candidates

| Project | Upstream | Role in Atlas | Adoption gate |
|---|---|---|---|
| Qt 6 | https://www.qt.io/ / https://code.qt.io/ | Cross-platform UI, input, accessibility, platform infrastructure | Chosen foundation; exact patch in N1 |
| Qt PDF | https://doc.qt.io/qt-6/qtpdf-index.html | PDF render/text/search/navigation candidate | N2 qualification |
| PDFium | https://pdfium.googlesource.com/pdfium/ | PDF render/text/search/inspection candidate | N2 qualification |
| qpdf | https://github.com/qpdf/qpdf | PDF structure/security/transformation candidate | N2 qualification |
| SQLite | https://sqlite.org/ | Local transactional index/state | N3 implementation |
| FTS5 | https://sqlite.org/fts5.html | Local full-text search | N3 implementation |
| nlohmann/json | https://github.com/nlohmann/json | Atlas bookmark/backup JSON candidate | N5/N8 when serialization lands |
| xxHash | https://github.com/Cyan4973/xxHash | Fast fingerprint/cache component candidate | N3 identity/cache only if useful |
| spdlog | https://github.com/gabime/spdlog | Structured logging candidate | N1 only if Qt/native logging is insufficient |

## Build / dependency management

| Project | Upstream | Role | Gate |
|---|---|---|---|
| CMake | https://cmake.org/ | Canonical native build system | Chosen |
| Ninja | https://ninja-build.org/ | Fast local/CI builds | Chosen convenience |
| vcpkg | https://github.com/microsoft/vcpkg | Manifest-mode non-Qt dependency management | Introduce N2 with pinned baseline |
| install-qt-action | https://github.com/jurplel/install-qt-action | Public CI Qt installation | Bootstrap/N1; tool only |

## Testing / benchmarking

| Project | Upstream | Role | Gate |
|---|---|---|---|
| CTest | https://cmake.org/cmake/help/latest/manual/ctest.1.html | Test orchestration | Chosen |
| Catch2 | https://github.com/catchorg/Catch2 | C++ unit/contract testing | Introduce when N1/N2 test surface justifies it |
| Qt Test | https://doc.qt.io/qt-6/qttest-index.html | Qt/QML/event-loop integration tests | User-facing Qt checkpoints |
| Google Benchmark | https://github.com/google/benchmark | Repeatable microbenchmarks | N1 performance harness |

## Profiling / graphics diagnostics

| Project/tool | Upstream | Use |
|---|---|---|
| Qt QML Profiler | https://doc.qt.io/qt-6/qtquick-profiling.html | QML creation/bindings/animation/frame analysis |
| Tracy | https://github.com/wolfpld/tracy | C++ CPU/thread/task/lock/timing profiling |
| RenderDoc | https://github.com/baldurk/renderdoc | GPU frame/resource/overdraw inspection |
| Windows Performance Recorder / Analyzer | Microsoft Windows ADK | Startup, I/O, scheduling, memory, system-level traces |
| Visual Studio Profiler | Visual Studio | Native CPU/memory/debug diagnostics |

## Accessibility / UX diagnostics

| Tool | Upstream | Use |
|---|---|---|
| Accessibility Insights for Windows | https://github.com/microsoft/accessibility-insights-windows | UI Automation/FastPass/manual a11y checks |
| Windows Narrator | Windows | Manual screen-reader acceptance |
| Qt accessibility APIs | https://doc.qt.io/qt-6/accessible.html | Runtime semantic roles/names/states |

## Code quality / security

| Tool | Upstream | Use |
|---|---|---|
| clang-format | LLVM | Canonical formatting |
| clang-tidy | LLVM | Curated correctness/performance/static checks |
| clang-cl | LLVM | Secondary diagnostic Windows compiler |
| AddressSanitizer / UBSan | compiler toolchains | Memory/undefined-behavior diagnostics where supported |
| GitHub CodeQL | GitHub | C/C++ security/static analysis once source surface is meaningful |
| GitHub secret scanning / push protection | GitHub | Prevent credential leakage |
| Syft (candidate) | https://github.com/anchore/syft | SBOM generation candidate for RC/release |

## Agent / repository intelligence

| Tool | Upstream / configuration | Use |
|---|---|---|
| CodeGraph | https://github.com/colbymchenry/codegraph | Local symbol/caller/impact graph; database remains machine-local |
| GitHub MCP/connector | project-local `.codex/config.toml` | Read-only repository/diff/CI context by default |
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