# Atlas Reader Native — Current Project Notes

**State:** native successor alpha/toolchain qualification

**Release line:** `2.0.0`

**Current checkpoint:** N1 — Windows toolchain + empty-shell baseline

**Current N1 status:** **In progress — corrective performance pass**. The repository/toolchain path is verified, physical attempt 1 proved the shell/RTL/theme path but exposed slow empty-shell startup and a v1 process-sampler defect. N2 remains closed.

**Previous checkpoint:** N0 — **Accepted by owner on 2026-09-22**

**Primary target:** Windows 11

**Future targets:** Android → Linux → macOS → iOS/iPadOS

## What this branch is doing

N1 establishes the cost and reproducibility floor **before** PDF, indexing, or bookmark code can hide framework/toolchain overhead.

It may contain only:

- Windows toolchain/build/CI work;
- empty Qt Quick shell work;
- privacy-safe logging skeleton;
- English/LTR ↔ Arabic/RTL shell-direction proof;
- light/dark shell proof;
- deterministic startup/shutdown controls;
- local numeric startup/frame benchmark instrumentation;
- physical-machine evidence and corrective performance work;
- documentation.

It must not contain production PDF, SQLite, scanner, Library, Reader, Bookmark, annotation, or migration implementation.

## N1 canonical alpha toolchain

- C++23
- Visual Studio 17 2022 x64 generator
- MSVC v143
- public reproducible Qt **6.10.3** MSVC 2022 64-bit baseline
- Qt 6.11.2 allowed as an additional developer compatibility build
- CMake >= 3.28
- Debug + Release CTest
- `/W4 /permissive- /Zc:__cplusplus`; CI adds `/WX`

See `docs/TOOLCHAIN.md` and `docs/decisions/ADR-0003-n1-qt-toolchain-pin.md`.

## Why canonical CI is temporarily Qt 6.10.3

Qt 6.11.2 is the preferred current product-generation line, but the unauthenticated public `aqtinstall` Windows 6.11.x binary repository path is currently unreliable. Atlas will not require private Qt-account credentials for ordinary public CI.

This is a reproducibility decision for the alpha baseline. The Qt pin must be re-evaluated before later release qualification.

## Implemented in N1

- prerelease advanced to `2.0.0-alpha.1`;
- local Visual Studio 2022 x64 presets aligned with CI;
- Debug + Release CI matrix;
- warnings-as-errors CI path;
- privacy-safe Qt logging categories;
- first-frame/QML-load/shutdown metric recorder;
- deterministic empty-shell resize benchmark mode;
- startup/shutdown auto-quit test option;
- shell language/direction switch for English/Arabic proof;
- light/dark shell toggle;
- PowerShell target-machine baseline harness;
- canonical N1 toolchain document and ADR;
- N1 baseline evidence sheet;
- self-contained `windeployqt` Windows qualification artifact;
- PowerShell syntax validation in CI;
- v2 physical harness with first-frame-aware process sampling, sample validity, packaged commit identity, storage/power/DPI capture, and p50/p95 startup statistics;
- staged startup metrics around `QGuiApplication`, QML engine/load, and first frame;
- compile-time `QtQuick.Controls.Basic` shell baseline to remove unnecessary runtime Fusion-style overhead.

## Physical attempt 1 findings

The first owner-machine run used the CI artifact from commit `d76eeef804b06fc3fae47079b8364a1b114a8b26` on Windows 11 Home build 26200 with a Core i7-14650HX, RTX 4070 Laptop GPU, 15.71 GiB RAM and a reported 144 Hz display.

Owner observations:

- English/LTR worked correctly;
- Arabic/RTL worked correctly;
- light and dark modes looked correct;
- no clipped or broken layout was noticed.

Automated findings:

- resize pacing was roughly around the 60 Hz budget on ordinary runs;
- idle shell memory was roughly mid-40 MiB working set / mid-20 MiB private memory in valid samples;
- v1 `WaitForInputIdle` sampling produced one invalid English process-memory sample and has been removed;
- warm first-frame startup remained roughly 1.8–2.2 s on ordinary runs, materially above the provisional **<800 ms p95** N1 target;
- cold candidates were about 2.6–3.1 s, above the provisional **<1.5 s** target.

Therefore N1 is **not** being waved through merely because the shell looked responsive after appearing. The corrective build must re-measure and attribute startup cost before acceptance.

See `docs/baselines/N1_WINDOWS_BASELINE.md` for the durable raw/corrected evidence.

## Evidence still required before N1 can be Accepted

- corrective branch-head Debug CI PASS;
- corrective branch-head Release CI PASS + portable artifact PASS;
- target Windows 11 v2 baseline using the corrected packaged qualification script;
- verify the new Basic-style shell still looks correct in LTR/RTL/light/dark;
- exact DPI/device-pixel-ratio evidence sufficient to qualify 100%/200% scaling;
- keyboard focus check;
- startup target met, or remaining deviation measured/root-caused and explicitly accepted as a tradeoff;
- owner PASS.

## Chosen long-term foundation

- C++23 shared core
- Qt 6 + Qt Quick/QML presentation
- CMake + CTest
- SQLite + FTS5 planned for index/search in N3
- replaceable PDF-engine abstraction
- Qt PDF/PDFium rendering/text evaluation in N2
- qpdf structural/security/transformation evaluation in N2
- vcpkg manifest mode accepted for qualified non-Qt dependencies from N2
- thin OS adapters
- GitHub Actions + CodeGraph/read-only GitHub MCP developer automation

## Product priority

1. Library / Index
2. Reader
3. Bookmarks / Outlines
4. Writing / standard annotations
5. Printing / covers / portable metadata / migration / backup

The research-data rule remains:

**Portable when possible, local when necessary, never lost silently.**

## Release program

- N0: `2.0.0-alpha.0` — Accepted bootstrap
- N1: `2.0.0-alpha.1` — active toolchain/baseline corrective pass
- N2: `2.0.0-alpha.2` — PDF-engine qualification
- N3: first useful native `2.0.0-beta.1`
- N4–N9: incremental `2.0.0-beta.N` milestones
- N10: `2.0.0-rc.N`
- N11: Windows `2.0.0`

## Not implemented

- production PDF loading/rendering/editing;
- SQLite database/schema;
- scanner/watcher;
- production Library UI;
- production Reader;
- bookmarks;
- annotations;
- migration/backup implementation;
- installer.

## Source-of-truth hierarchy

- Running code/tests define Implemented behavior.
- `CHECKPOINTS.md` defines execution/status/release gates.
- `PLAN.md` defines the master north star.
- `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope.
- `docs/TOOLCHAIN.md` defines the active N1 Windows toolchain.
- `docs/baselines/N1_WINDOWS_BASELINE.md` defines current N1 evidence.
- `docs/CORE_WORKFLOWS.md` defines Index/Reader/Bookmark capability/failure/recovery behavior.
- `docs/ARCHITECTURE.md` defines layer/thread/platform boundaries.
- `docs/TECH_STACK.md`, `docs/DEPENDENCIES_AND_TOOLS.md`, and `docs/UPSTREAM_CATALOG.md` define technology/dependency/tool policy.
- `docs/QUALITY_AND_TESTING.md` and `docs/PERFORMANCE.md` define evidence/benchmarks.
- `docs/SECURITY_MODEL.md` defines untrusted-input/privacy/security behavior.
- `docs/UX_ACCESSIBILITY_AND_DESIGN.md` defines UI/a11y/RTL rules.
- `docs/RELEASE_STRATEGY.md` defines alpha/beta/RC/stable semantics.
- `CHANGELOG.md` records repository changes.

No document may claim an unimplemented requirement is shipped.
