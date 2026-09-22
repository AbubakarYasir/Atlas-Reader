# Changelog

All notable changes to Atlas Reader Native are documented here.

## [Unreleased]

### N2 — PDF engine qualification spike (`2.0.0-alpha.2`)

- N1 was explicitly Accepted by the owner on 2026-09-22 and merged into the accepted integration line at `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`.
- Opened dedicated branch `native-v2-n2-pdf-engine-qualification` from that exact accepted N1 state; `main` remains untouched.
- Advanced native prerelease identifier from `alpha.1` to `alpha.2` without adding a PDF engine dependency yet.
- Added `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` defining N2 scope, exclusions, phases, normalized responsibilities, fixture classes, performance rules, hard blockers and owner stop gate.
- Added `docs/baselines/N2_PDF_ENGINE_MATRIX.md` as the durable evidence table. All untested capabilities remain explicitly `PENDING`; no candidate is pre-ranked or pre-selected.
- Added Proposed `ADR-0004-pdf-engine-responsibilities.md`; it cannot become Accepted before sufficient matrix evidence, strict final CI and explicit owner `N2 PASS`.
- Added `tests/fixtures/pdf/README.md` defining fixture provenance, redistribution, SHA-256, private-fixture privacy, password, mutation-safety and expected-data rules.
- Updated `AGENTS.md` so coding agents operate under N2 boundaries and cannot drift into production Reader, SQLite/index, bookmarks, annotations, migration or installer work.
- Refreshed `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/UPSTREAM_CATALOG.md` with the N2 upstream/integration snapshot.
- Recorded current PDFium constraint that its public API is not thread-safe and qualification must serialize API calls rather than benchmark unsupported concurrency.
- Recorded that official PDFium source integration uses Chromium-style `depot_tools`/`gclient`, GN/Ninja and Clang/clang-cl tooling, and that the inspected Microsoft vcpkg registry does not expose a standard `ports/pdfium` package.
- Allowed a pinned community PDFium binary distribution only as an N2 spike bootstrap when exact package/revision, SHA-256 and notices are recorded; this does not pre-approve the production supply chain.
- Recorded qpdf version-surface differences visible at N2 opening instead of collapsing them: current inspected vcpkg port `12.4.0`, upstream published release feed `12.4.1` (2026-08-27), and generated docs that may already identify `12.4.2`.
- Preserved the split-engine possibility: Qt PDF or PDFium may own read/render/text/navigation while qpdf may own structure/security/transformation, all behind Atlas-owned normalized contracts.
- Kept N3 and all later product-feature checkpoints closed until N2 is explicitly Accepted.

### N1 — Windows toolchain + empty-shell baseline (`2.0.0-alpha.1`) — Accepted

- N0 was explicitly Accepted by the owner on 2026-09-22.
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
- Added `docs/TOOLCHAIN.md` and N1 baseline/qualification records.
- Added ADR-0003 documenting the reproducible public Qt 6.10.3/MSVC 2022 alpha pin.
- Added a self-contained Release qualification artifact produced with `windeployqt` so owner hardware testing does not require a local compiler/Qt install.
- Added CI syntax/runtime validation for PowerShell qualification scripts before packaging.
- Physical testing on Windows 11 / Core i7-14650HX / RTX 4070 Laptop / 144 Hz verified English/LTR, Arabic/RTL, mixed script, light/dark, 100%/200% scale and clean startup/shutdown.
- Initial physical measurements exposed a `WaitForInputIdle` sampler defect and empty-shell startup around 1.8–2.2 seconds, so N1 remained open while the issue was attributed instead of normalized.
- Reworked the physical harness to wait for Atlas's own first-frame metric, reject dead-process samples, capture power/storage/DPI context, preserve packaged commit identity, and report startup p50/p95.
- Added staged startup instrumentation around application, QML engine/load and first frame.
- Switched the N1 shell to compile-time `QtQuick.Controls.Basic` while retaining Atlas palette/RTL behavior.
- Isolated first meaningful text/font initialization as the dominant Windows startup cost and compared Windows font engines on a temporary diagnostic branch.
- Adopted `fontengine=gdi` through packaged `qt.conf` for the N1 Windows shell after measured comparison; documented it as a qualified Windows decision with later re-evaluation conditions rather than a universal permanent claim.
- Final Arabic warm first-frame p95 measured `772.875 ms`.
- English 22-run confirmation measured warm p50 `365.941 ms`, p95 `1014.827 ms`, with a temporary system-wide slow cluster and recovery; owner explicitly accepted this measured English startup tradeoff.
- Fixed a focused keyboard activation defect discovered before acceptance by handling Enter/Return on the N1 buttons; owner physically retested and passed the correction.
- Accepted user-qualified runtime commit: `73f567cf2c557f185371d7f944ebf6d69453105a`, artifact digest `sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`.
- Owner explicitly recorded `N1 PASS` on 2026-09-22.
- Added the repository rule: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.**
- Merged N1 PR #3 into `native-v2-bootstrap` at `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`.
- PDF engines, qpdf, SQLite/FTS5, scanner, production Library/Reader/Bookmarks, annotations, migration, and installer implementation remained outside N1.

### N0 — Native 2.0 bootstrap program (`2.0.0-alpha.0`) — Accepted

- Established C++23 + Qt Quick/QML + CMake successor architecture.
- Added pure C++ contracts for document capabilities, PDF engine, filesystem, and library index.
- Added minimal Qt Quick application shell and native smoke test.
- Added Windows CI and corrected its public Qt/toolchain configuration after early bootstrap runs exposed reproducibility issues.
- Added project-local CodeGraph and GitHub connector/MCP configuration.
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
- `2.0.0-alpha.1` — N1 Windows/toolchain baseline — **Accepted**
- `2.0.0-alpha.2` — N2 PDF-engine qualification — **In progress**
- `2.0.0-beta.1` onward — usable feature checkpoints from N3
- `2.0.0-rc.N` — release qualification
- `2.0.0` — accepted stable Windows release

See `docs/RELEASE_STRATEGY.md` for the binding rules.
