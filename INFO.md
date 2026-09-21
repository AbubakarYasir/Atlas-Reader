# Atlas Reader Native — Current Project Notes

**State:** native successor alpha/toolchain qualification

**Release line:** `2.0.0`

**Current checkpoint:** N1 — Windows toolchain + empty-shell baseline

**Current N1 status:** **In progress** — implementation/toolchain work is on branch `native-v2-n1-toolchain-baseline`; strict Debug/Release CI and physical Windows baseline evidence are required before owner review.

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
- documentation and evidence.

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

## Implemented in N1 so far

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
- N1 baseline evidence sheet.

## Evidence still required before N1 can be Accepted

- current branch-head Debug CI PASS;
- current branch-head Release CI PASS;
- target Windows 11 Release baseline using `tools/bench/measure-windows-shell.ps1`;
- English/LTR and Arabic/RTL manual shell check;
- 100% and 200% Windows scale check;
- clean repeated startup/shutdown check;
- owner PASS.

See `docs/baselines/N1_WINDOWS_BASELINE.md`.

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
- N1: `2.0.0-alpha.1` — active toolchain/baseline
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
