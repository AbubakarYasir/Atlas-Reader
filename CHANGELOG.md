# Changelog

<!-- atlas-status: N3|in-progress -->

All notable changes to Atlas Reader Native are documented here.

## [Unreleased]

### N3 start — 2026-09-27

- Began N3 after N2 owner acceptance, merge, final CI, local/remote synchronization and cleanup were verified. Created `native-v2-n3-library-index-foundation` from accepted native `main` commit `84a9b45b10b24ece595bb7069136fefd2ed2cc5b`.
- Set N3 to In progress and N3.1 storage/schema as its first subgate. Added the binding N3 plan for database/repository/search, scanning, identity/reconciliation, Library UX, QA, owner review and beta stop gate, plus an evidence ledger with no tests/results claimed at start.
- At the initial N3 start record, product code and dependency pins had not yet been established. The engineering version remains alpha.2 until the N3 acceptance/first-useful-beta gates are met.
- Chosen and declared SQLite `3.53.4#1`/FTS5 on pinned vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`; added a private SQLite-backed repository adapter with versioned transactional schema migrations, root/book/favorite/recents persistence, FTS triggers/rebuild and isolated CTest coverage. Windows Debug/Release builds and tests pass on CI. Existing root IDs and book locations cannot be reassigned to different paths/documents without an explicit reconciliation decision.
- First CI exposed two integration issues before N3.1 acceptance: an Arabic FTS assertion assumed stemming that `unicode61` does not provide, and the inherited selected-production-route job lacked the new pinned vcpkg setup. Corrected the fixture and configured that job with the exact baseline/toolchain; reruns are green. N3.1 storage/schema is verified, including the corrected runtime linkage in local and final Windows CI Debug/Release builds. N3.2 safe scanning is active. Search currently matches indexed tokens and does not provide Arabic morphological stemming.
- Started N3.2 with a path-only scanner and streaming native directory enumerator. It publishes bounded batches, supports cancellation and explicit unavailable/partial/limit outcomes, skips symlink entries and enforces depth/directory/file caps. Added deterministic fixture-based coverage plus a real Unicode-path filesystem smoke test; local and Windows CI verification pass. Scans do not mutate the index or remove missing records.
- Added explicit multi-root scanning at `609c8b9`: overlapping and nested roots publish each physical PDF path once, duplicate suppression is reported, root IDs are validated before scanning, and an unavailable root keeps its own outcome without invalidating successful roots. Local Visual Studio 2026 Debug and Release builds each pass 20/20 tests; final branch Debug/Release and inherited PDF checks also pass.
- Activated the database's scan-generation safety fields at `352c5d0`. Every root scan receives a durable generation; superseded scans cannot mark locations or overwrite newer outcomes; only a complete scan records a new completion time. Partial, interrupted and offline scans preserve the last trusted completion evidence and all indexed books. Local and Windows CI Debug/Release suites pass.
- Integrated bounded scans with durable root state at `38d3cab`. Existing locations are marked in the active generation; unknown paths stream forward for later identity inspection without creating or relinking documents; overlapping unknown paths are published once. Complete, partial, offline and cancelled outcomes update only the state they can prove. Local Visual Studio 2026 Debug and Release suites pass 20/20 each; final branch Debug/Release and inherited PDF checks pass.
- Added Qt PDF-backed library inspection at `30137a1`. Discovered files now have explicit readable, password-locked, malformed/unreadable, unsupported-security and missing states, with filename/metadata, page count and filesystem revision signals collected where available. The inspector never supplies or bypasses a password. Local Debug and Release suites pass 21/21 each; final branch CI is pending.
- Started N3.3 with an engine-independent identity policy and ADR-0005. Atlas may automatically relink a move only when a complete scan proved the old path missing and filesystem identity is preserved; surviving originals remain distinct copies, content-only matches remain ambiguous, and offline or incomplete roots never authorize guesses. Local Visual Studio 2026 Debug and Release suites pass 22/22 each. Durable reconciliation proposals/apply and restart coverage remain before this subgate is complete.
- Added SQLite schema v4 and durable reconciliation at `f278bec`. Atlas records completed scan generations and opaque filesystem identity, persists a reviewable pending proposal without changing the book, and revalidates every safety condition transactionally before a proven move is applied. Restart tests prove stable document identity and audit state; ambiguous offline proposals cannot apply and can be explicitly dismissed. Local Debug and Release suites pass 22/22 each. Native filesystem identity collection and remaining integration/owner evidence are still required.
- Added the native filesystem identity adapter at `fb84173`. On Windows it records the volume/file identifier via read-attributes-only handles with read/write/delete sharing; the portability boundary also has a POSIX device/inode implementation. Real filesystem tests prove stability across rename, a different identity for a byte-identical copy, Unicode paths and explicit missing-path state. Local Debug and Release suites pass 23/23 each.
- Integrated native identity with bounded scanning at `0ee0237`. Known paths initialize or confirm their stored native identity; a different filesystem object at the same path is emitted as a replacement candidate carrying the preceding Atlas document ID and is not silently marked current, reassigned or deleted. Local Debug and Release remain 23/23 green.
- Added the safe ingestion orchestrator at `8516cd0`. Completed discoveries now become either newly inspected stable documents or durable pending reconciliation proposals; proposals are never applied implicitly. A real Windows filesystem flow proves new ingestion, Unicode rename preview, explicit move application with the same Atlas ID, distinct byte-identical copies and ambiguous same-path replacement. Local Debug and Release suites pass 24/24 each.
- Configured this PC for native work: added the installed Visual Studio CMake bin to the current user's PATH, bootstrapped vcpkg at the exact pinned baseline, and installed Qt 6.10.3 MSVC 2022 x64 with Qt PDF. Added the VS 2026 configure/build/test preset without changing the VS 2022 CI baseline. The initially self-referential `CMAKE_PREFIX_PATH` preset macro was corrected to inherit the parent environment. Local fixture setup now follows the same generators/validator as CI; full local Debug and Release builds each pass 20/20 tests.
- Local and GitHub link logs exposed MSVC `LNK4098` on SQLite test executables because `x64-windows-static` used a static CRT against Qt's dynamic CRT. Switched to the pinned vcpkg `x64-windows-static-md` triplet across presets, CI, and dependency/build documentation. Corrected local Debug/Release builds and final GitHub PR/push checks each pass all 20 tests; the final GitHub logs contain no linker warning. N3.1 remains verified and N3.2 scanning remains active.
- Updated live status pages and the document checker for checkpoint transitions; closed N2 records retain their historical `N2|accepted` status.

### N2 owner acceptance — 2026-09-27

- Moved seven PDF regression workflow push triggers from the retired N2 branch to permanent `main`, preserving their PR triggers. Performance and qpdf qualification retain PR/manual triggers. This keeps accepted PDF checks active after branch cleanup.

- Rewrote the public README with a reader-focused introduction, explicitly planned benefits, honest current availability, next stages and ways to follow the project. Added `docs/DEVELOPER_GUIDE.md` for technical orientation and linked the existing authoritative build, architecture, QA and evidence documents. No product scope or acceptance requirement changed.

- Adopted permanent branch names at the owner's request: native development is `main`, and the discontinued Flutter backup is `legacy/flutter`. Renamed both existing branches without rewriting commits; synchronized local tracking, public notices and workflow documentation. Future checkpoints do not rename either permanent branch.

- Merged N2 PR #4 after all 20 final-head checks passed. Removed four obsolete remote branches and the completed local N2 branch; preserved two unmerged historical branches through explicitly non-release `archive/*` tags. Only native integration and the obsolete Flutter backup remain. Exact recovery references and worktree purposes are recorded in `docs/baselines/N2_ACCEPTANCE.md`.

- Repository landing page and default file tree now target native development (`main`); Flutter `legacy/flutter` is a secondary obsolete backup/reference, with development stopped.

- Removed unreferenced `DO_NOT_USE.tmp` and obsolete encrypted A013 PDF/generator; canonical A013 is image-only and encrypted-write coverage uses manifest/CI-qualified A015. Deleted files remain recoverable from Git history.
- Closed the superseded Flutter roadmap PR #2 after publishing the discontinued-development notice.

- Owner explicitly tested and approved N2 with `N2 PASS`; ADR-0004 is Accepted.
- Added `docs/baselines/N2_ACCEPTANCE.md` with evidence identity, integration route, future obligations and the stop before N3.
- Corrected Git policy: native checkpoint PRs target `main`; `legacy/flutter` retains the discontinued Flutter reference.
- Published that Flutter development has stopped and C++/Qt native development is active.
- At N2 closeout N3 was Not started. The engineering version remained `2.0.0-alpha.2`.

- Audited every tracked Markdown plan/evidence file for status, route and
  checkpoint-ownership drift; corrected the live N2 state and the final Qt
  PDF/qpdf responsibility route without rewriting historical measurements.
- Added a binding N0–N11 QA/owner-test matrix, assigned every N2 limitation to a
  named implementation checkpoint and release blocker, and filled missing QA
  gates for N3–N11 including Windows integration, settings and diagnostics.
- Added an explicit Git/documentation workflow plus automated Markdown link,
  live-status and checkpoint-gate validation in CI.

### N2 — PDF engine qualification spike (`2.0.0-alpha.2`)

- N1 was explicitly Accepted by the owner on 2026-09-22 and merged into the accepted integration line at `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`.
- Opened dedicated branch `native-v2-n2-pdf-engine-qualification` from that exact accepted N1 state; `main` remains untouched.
- Advanced native prerelease identifier from `alpha.1` to `alpha.2` without adding a production PDF engine dependency.
- Added `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` defining N2 scope, exclusions, phases, normalized responsibilities, fixture classes, performance rules, hard blockers and owner stop gate.
- Added `docs/baselines/N2_PDF_ENGINE_MATRIX.md` as the binding evidence table. Untested or undecided capabilities remain explicit rather than guessed.
- Added Proposed `ADR-0004-pdf-engine-responsibilities.md`; its frozen decision now selects official dynamic Qt PDF 6.10.3 plus the qpdf 12.4.1 CLI, but it cannot become Accepted before final strict CI and explicit owner `N2 PASS`.
- Added deterministic PDF fixture provenance/privacy/checksum/mutation rules under `tests/fixtures/pdf/`.
- Updated `AGENTS.md` so coding agents operate under N2 boundaries and cannot drift into production Reader, SQLite/index, bookmarks, annotations, migration or installer work.
- Refreshed dependency/upstream catalogs with Qt PDF, PDFium and qpdf qualification constraints.
- Preserved the split-engine possibility: read/render/search/navigation and structural/security/transformation responsibilities may be assigned separately behind Atlas-owned normalized contracts.
- Qualified Qt PDF 6.10.3 core read/text/render behavior on the deterministic corpus without linking Qt PDF into the product shell/domain boundary.
- Qualified PDFium `156.0.8066.0` / `chromium/8066` through a checksum-pinned non-V8 Windows x64 probe package; retained the upstream rule that public PDFium API execution must be serialized.
- Qualified outline/link semantics in both read engines; recorded Qt's duplicate raw external-URI row and required Atlas semantic de-duplication.
- Added deterministic A006 Unicode evidence covering Arabic, tashkīl, mixed Arabic/English, Urdu and Unicode outlines/search. Both engines preserve tested code points but require Unicode normalization before equality/index/search comparison for the vocalized source-order case.
- Added malformed/password fixtures and semantic error-state qualification for Qt PDF/PDFium.
- Added repeated Release performance probes with 3 warmups + 31 measured iterations, while explicitly avoiding a false winner claim from non-equivalent Qt/PDFium search execution models.
- Added qpdf 12.4.1 structural/security/transformation qualification from the first-party MSVC64 package with exact asset checksum and preservation assertions.
- Qualified qpdf user/owner password state and restricted permission inspection on deterministic R2 fixtures; legacy RC4 exists only as test data.
- Qualified qpdf no-op rewrites and controlled ASCII/Unicode existing-outline title mutation while preserving explicit unrelated-document invariants.
- Recorded that qpdf may renumber indirect objects, so PDF object numbers are not Atlas portable identity.
- Added A010 synthetic `/Sig` + `/DocMDP` evidence and established the signed/certified pre-mutation interlock: Atlas must detect before mutation and must never equate surviving signature dictionaries with preserved cryptographic validity.
- Added A011 rendering-fidelity evidence covering effective CropBox geometry, 90° rotation, semantic vector landmarks at 1×/2×, annotation off/on behavior and engine-specific native blank backgrounds. Atlas owns background/compositing policy.
- Added the Atlas portable page-space contract: effective visible page, upper-left origin, X right, Y down, units PDF points.
- Qualified link/search rectangle normalization across normal, cropped and rotated pages; recorded that Qt raw rotated search rectangles can have negative width and must be canonicalized.
- Added A012 explicit `/XYZ` destination evidence. Normal targets pass in both engines; PDFium directly normalizes the 90° target, while Qt requires structural rotation metadata external to `QPdfLink` to complete portable destination normalization.
- Added dedicated stress/concurrency evidence: 500 complete open/render/text/close lifetimes per read engine and a 4-producer × 250-request PDFium queue with exactly one PDFium worker/API lane. All 1,000 queued jobs completed, producer-side PDFium calls remained zero, maximum active PDFium API execution remained one, and shutdown was clean.
- Recorded working-set trends from the 500-cycle stress run without inventing a universal leak threshold. Qt showed a larger upward working-set trend than PDFium in this one synthetic hosted-runner series; it is retained as follow-up evidence rather than declared a leak.
- Added `docs/baselines/N2_STRESS_CONCURRENCY.md` with exact stress implementation/run/artifact identities and architectural serialization conclusions.
- Added `docs/baselines/N2_PRODUCTION_DISTRIBUTION.md` separating qualification packages from shippable routes: official dynamic Qt if selected; Atlas-owned pinned upstream PDFium source build if selected; first-party qpdf CLI process boundary as the current lower-coupling structural route.
- Recorded production compliance requirements for exact provenance, runtime-file inventory, license/NOTICE/SBOM bundle, security-update ownership and rollback before any selected component is promoted to a release dependency.
- Closed B1 image-only rendering, B2 real-font Arabic/Urdu rendering, B3 full outline mutation breadth, and B4 permitted encrypted-write preservation with bounded CI evidence.
- Repaired A015's invalid missing resource dictionary, added strict source validation, and corrected the qpdf permission assertion to read the JSON v2 `encrypt.parameters.P` field.
- Froze the selected production route as official dynamic Qt PDF 6.10.3 for read/render/text/navigation plus the first-party qpdf 12.4.1 CLI for structural/security/write; standalone PDFium remains qualification-only because an official Atlas-controlled source-build route was not frozen.
- Added selected-route CI that disables standalone PDFium, runs the Qt PDF and six qpdf regression slices, stages and smoke-tests the actual Windows runtime, enforces qpdf file hashes, rejects `pdfium.dll`, and emits deployed-file, SPDX, license, attribution, update and rollback evidence.
- Measured the staged qualification package at 64,069,704 bytes across 30 files: 54,965,368 bytes for the deployed Qt/read route and 9,104,336 bytes for qpdf.
- Strict CI passed on the synchronized implementation/documentation head; owner review and explicit `N2 PASS` followed on 2026-09-27.
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
- `2.0.0-alpha.2` — N2 PDF-engine qualification — **Accepted**
- `2.0.0-beta.1` onward — usable feature checkpoints from N3
- `2.0.0-rc.N` — release qualification
- `2.0.0` — accepted stable Windows release

See `docs/RELEASE_STRATEGY.md` for the binding rules.
