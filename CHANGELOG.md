# Changelog

All notable changes to Atlas Reader Native are documented here.

## [Unreleased]

### N1 — Windows toolchain + empty-shell baseline (`2.0.0-alpha.1`)

- N0 was explicitly Accepted by the owner on 2026-09-22; N1 is now the active checkpoint.
- Created dedicated branch `native-v2-n1-toolchain-baseline`.
- Advanced native prerelease identifier from `alpha.0` to `alpha.1`.
- Aligned local CMake presets with the Visual Studio 17 2022 x64 generator used by CI instead of using a separate Ninja path.
- Added a Debug + Release Windows CI matrix.
- Added strict Atlas-owned compiler policy: `/W4 /permissive- /Zc:__cplusplus`, with `/WX` in CI.
- Added privacy-safe Qt logging categories for startup, UI, and performance diagnostics.
- Added local-only numeric metrics for QML load, first swapped frame, resize frame intervals, and clean shutdown.
- Added deterministic `--benchmark-shell` resize exercise and `--quit-after-ms` lifecycle test option.
- Added `--language en|ar`, `--theme system|light|dark`, and `--metrics-file` shell test controls.
- Added live English/LTR ↔ Arabic/RTL direction proof in the empty shell.
- Added light/dark shell proof without introducing a production theming system.
- Added `tools/bench/measure-windows-shell.ps1` for physical-machine startup, memory, CPU, and frame baseline measurement.
- Added ignored `artifacts/` output for machine-specific benchmark data.
- Added `docs/TOOLCHAIN.md` and `docs/baselines/N1_WINDOWS_BASELINE.md`.
- Added ADR-0003 documenting the reproducible public Qt 6.10.3/MSVC 2022 alpha pin while Qt 6.11.2 remains the preferred newer compatibility target.
- Added a self-contained Release qualification artifact produced with `windeployqt` so owner hardware testing does not require a local compiler/Qt install.
- Added CI syntax validation for the PowerShell qualification scripts before packaging.
- Physical attempt 1 on Windows 11 / Core i7-14650HX / RTX 4070 Laptop / 144 Hz verified English/LTR, Arabic/RTL, light/dark presentation, and no observed clipping/broken layout.
- Physical attempt 1 also exposed two N1 blockers before PDF/index work begins:
  - the v1 `WaitForInputIdle` process sampler could outlive Atlas auto-shutdown and corrupt memory/CPU readings;
  - empty-shell first-frame startup was materially above the provisional N1 target (roughly 1.8–2.2 s on ordinary warm runs versus the <800 ms p95 engineering target).
- Reworked the physical harness to wait for Atlas's own first-frame metric, reject dead-process memory/CPU samples, report validity counts, capture power/storage/DPI context, preserve packaged commit identity, and report startup p50/p95.
- Added startup-stage instrumentation around `QGuiApplication`, argument parsing, QML-engine creation/load, and first frame so remaining startup cost can be attributed rather than guessed.
- Switched the N1 shell from runtime-selected Fusion controls to compile-time `QtQuick.Controls.Basic`, Qt's lightweight/max-performance Controls baseline, while retaining Atlas-owned palette/RTL behavior.
- Recorded attempt-1 raw and corrected evidence in `docs/baselines/N1_WINDOWS_BASELINE.md`; N1 remains In progress until corrective physical evidence satisfies or explicitly resolves the startup gate.
- Updated README, INFO, BUILDING, docs index, and AGENTS rules for the active N1 checkpoint.
- Kept PDF engines, qpdf, SQLite/FTS5, scanner, production Library/Reader/Bookmarks, annotations, migration, and installer code out of N1.

### N0 — Native 2.0 bootstrap program (`2.0.0-alpha.0`)

- Established C++23 + Qt Quick/QML + CMake successor architecture.
- Added pure C++ contracts for document capabilities, PDF engine, filesystem, and library index.
- Added minimal Qt Quick application shell and native smoke test.
- Added Windows CI and corrected its public Qt/toolchain configuration after early bootstrap runs exposed reproducibility issues.
- Added project-local CodeGraph and read-only GitHub MCP configuration.
- Defined the native release line through `2.0.0`: N0–N2 alphas, N3–N9 betas, N10 RC, N11 stable Windows 2.0.
- Expanded `CHECKPOINTS.md` into explicit subgates, owner evidence, and release mapping.
- Added complete Windows 2.0 product scope with P0/P1/P2/deferred boundaries.
- Added open-source dependency/tool/plugin policy including vcpkg manifest direction, PDF-engine qualification, testing/profiling/accessibility tooling, AI-agent guardrails, and a concrete upstream project catalog.
- Added quality/testing/preservation/failure-injection strategy.
- Added security/privacy threat model and vulnerability-reporting policy.
- Added UX/accessibility/Arabic/RTL design contract.
- Added competitive baseline/differentiation strategy covering mature PDF readers/editors without turning feature-count parity into the product goal.
- Added release strategy, ADR process, vcpkg dependency ADR, and development workflow documentation.
- Expanded licensing/third-party/SBOM guardrails.
- Added measurable provisional performance budgets and benchmark hygiene.
- Added Atlas-owned data/format contracts covering document identity, destinations, bookmark overlay states, versioned interchange, backups, and migration boundaries.
- Added a product/engineering success scorecard so beta progression is measured by safety, correctness, responsiveness, accessibility, interoperability, and resource use rather than feature count.
- Added a living risk register for engine, corruption, licensing, C++ safety, portability, accessibility, migration, packaging, scope, and performance risks.
- Added `docs/N0_HANDOFF.md` as the explicit owner-review/acceptance checklist.
- Explicitly kept production PDF engines, SQLite, scanning, reader features, bookmarks, annotations, migration, and installer code out of N0.

### N0 CI findings and verification

Early CI runs exposed two useful setup defects:

1. Public `aqtinstall` could resolve Qt 6.11.2 but could not locate its Windows repository XML, so public CI moved to a reproducibly installable Qt 6.10.3 pin.
2. `CMakeLists.txt` incorrectly hard-required the exact Qt 6.11.2 patch while CI intentionally used a compatible patch. Application source now states the Qt 6.10 API floor while exact patch selection belongs to the toolchain policy.

The Windows runner was pinned to Windows 2022 with the Visual Studio 17 2022 x64 generator to match the MSVC 2022 Qt kit instead of inheriting changing `windows-latest` toolchains.

N0 passed branch-head Windows Qt installation → CMake Configure → Release Build → CTest and was explicitly Accepted by the owner on 2026-09-22.

## Versioning

The native successor belongs to the `2.0.0` release line.

- `2.0.0-alpha.0` — N0 bootstrap — **Accepted**
- `2.0.0-alpha.1` — N1 Windows/toolchain baseline — **In progress**
- `2.0.0-alpha.2` — N2 PDF-engine qualification
- `2.0.0-beta.1` onward — usable feature checkpoints from N3
- `2.0.0-rc.N` — release qualification
- `2.0.0` — accepted stable Windows release

See `docs/RELEASE_STRATEGY.md` for the binding rules.
