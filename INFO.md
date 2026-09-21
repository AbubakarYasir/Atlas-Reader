# Atlas Reader Native — Current Project Notes

**State:** native successor bootstrap

**Release line:** `2.0.0`

**Current checkpoint:** N0 — Native repository bootstrap

**Current N0 status:** **Ready for owner test** — Windows build path Verified; owner PASS still required before N1.

**Primary target:** Windows 11

**Future targets:** Android → Linux → macOS → iOS/iPadOS

## What this repository/branch is

This is the clean native successor foundation to the Flutter Atlas Reader. It starts as a new architecture, not a line-by-line code translation.

The Flutter implementation remains a product-behavior/data/fixture reference until each native checkpoint is independently Implemented, Verified, and Accepted.

## Chosen foundation

- C++23 shared core
- Qt 6 + Qt Quick/QML presentation
- preferred product line Qt 6.11.x; bootstrap public CI currently Qt 6.10.3 because public aqt Windows 6.11 install is failing upstream
- CMake + CTest; Windows CI pinned to Visual Studio 17 2022 x64
- SQLite + FTS5 planned for index/search
- replaceable PDF-engine abstraction
- Qt PDF/PDFium rendering/text evaluation in N2
- qpdf structural/security/transformation evaluation in N2 (planning baseline 12.4.1)
- vcpkg manifest mode accepted for non-Qt native dependencies from N2
- thin OS adapters
- GitHub Actions + CodeGraph/read-only GitHub MCP developer automation

N0 deliberately does **not** wire qpdf/SQLite/PDFium into the application. Each production dependency must earn its place through the relevant checkpoint.

## Product priority

1. Library / Index
2. Reader
3. Bookmarks / Outlines
4. Writing / standard annotations
5. Printing / covers / portable metadata / migration / backup

The research-data rule is:

**Portable when possible, local when necessary, never lost silently.**

## Release program

- N0–N2: `2.0.0-alpha.N` engineering previews
- N3: first useful native `2.0.0-beta.1`
- N4–N9: incremental `2.0.0-beta.N` milestones
- N10: `2.0.0-rc.N`
- N11: Windows `2.0.0`

See `docs/RELEASE_STRATEGY.md` and `CHECKPOINTS.md`.

## Implemented in N0

- comprehensive documentation hierarchy;
- CMake bootstrap;
- minimal QML window;
- pure C++ capability/PDF/filesystem/index interfaces;
- core smoke test;
- Windows CI;
- project-local CodeGraph/read-only GitHub MCP configuration;
- security/vulnerability policy;
- ADR process and vcpkg dependency-management decision.

## Verified N0 evidence

Windows CI run 27 on commit `bb359876fc67d972b079bdaf643158952fb68638` passed:

- Qt install;
- CMake Configure;
- Release Build;
- CTest/core smoke.

Earlier CI attempts usefully exposed two bootstrap issues that are now corrected: public aqt could not install Windows Qt 6.11.x, and application CMake had incorrectly hard-pinned the exact Qt patch. Public N0 CI therefore uses Qt 6.10.3 compatibility while N1 owns exact Qt 6.11.x product qualification.

## Not implemented

- production PDF loading/rendering/editing;
- SQLite database/schema;
- scanner/watcher;
- production library UI;
- production reader;
- bookmarks;
- annotations;
- migration/backup implementation;
- installer.

## Source-of-truth hierarchy

- Running code/tests define Implemented behavior.
- `CHECKPOINTS.md` defines execution/status/release gates.
- `PLAN.md` defines the master north star.
- `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope.
- `docs/CORE_WORKFLOWS.md` defines Index/Reader/Bookmark capability/failure/recovery behavior.
- `docs/ARCHITECTURE.md` defines layer/thread/platform boundaries.
- `docs/TECH_STACK.md`, `docs/DEPENDENCIES_AND_TOOLS.md`, and `docs/UPSTREAM_CATALOG.md` define technology/dependency/tool policy.
- `docs/QUALITY_AND_TESTING.md` and `docs/PERFORMANCE.md` define evidence/benchmarks.
- `docs/SECURITY_MODEL.md` defines untrusted-input/privacy/security behavior.
- `docs/UX_ACCESSIBILITY_AND_DESIGN.md` defines UI/a11y/RTL rules.
- `docs/RELEASE_STRATEGY.md` defines alpha/beta/RC/stable semantics.
- `CHANGELOG.md` records repository changes.

No document may claim an unimplemented requirement is shipped.