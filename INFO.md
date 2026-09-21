# Atlas Reader Native — Current Project Notes

**State:** native successor bootstrap

**Release line:** `2.0.0`

**Current checkpoint:** N0 — Native repository bootstrap

**Current N0 status:** In progress until corrected public Windows CI successfully configures/builds/tests and owner accepts the program plan.

**Primary target:** Windows 11

**Future targets:** Android → Linux → macOS → iOS/iPadOS

## What this repository/branch is

This is the clean native successor foundation to the Flutter Atlas Reader. It starts as a new architecture, not a line-by-line code translation.

The Flutter implementation remains a product-behavior/data/fixture reference until each native checkpoint is independently Implemented, Verified, and Accepted.

## Chosen foundation

- C++23 shared core
- Qt 6 + Qt Quick/QML presentation
- preferred product line Qt 6.11.x; bootstrap public CI currently Qt 6.10.3 because public aqt Windows 6.11 install is failing upstream
- CMake + Ninja/CTest
- SQLite + FTS5 planned for index/search
- replaceable PDF-engine abstraction
- Qt PDF/PDFium rendering/text evaluation in N2
- qpdf structural/security/transformation evaluation in N2 (current planning baseline 12.4.1)
- vcpkg manifest mode planned for accepted non-Qt native dependencies from N2
- thin OS adapters
- GitHub Actions + CodeGraph/read-only GitHub MCP developer automation

N0 deliberately does **not** wire qpdf/SQLite/PDFium into the application. Setup is minimal until each production dependency earns its place through the relevant checkpoint.

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
- Windows CI definition;
- project-local CodeGraph/read-only GitHub MCP configuration.

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

## Current toolchain issue discovered by CI

The first CI run failed **before Configure** because public `aqtinstall` could resolve Qt 6.11.2 but could not locate/download its Windows repository XML. The workflow has been corrected to use reproducibly public Qt 6.10.3 for bootstrap CI while N1 owns final Qt 6.11.x qualification. N0 remains unverified until a corrected run reaches Configure, Build, and Test successfully.

## Source-of-truth hierarchy

- Running code/tests define Implemented behavior.
- `CHECKPOINTS.md` defines execution/status/release gates.
- `PLAN.md` defines the master north star.
- `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope.
- `docs/CORE_WORKFLOWS.md` defines Index/Reader/Bookmark capability/failure/recovery behavior.
- `docs/ARCHITECTURE.md` defines layer/thread/platform boundaries.
- `docs/TECH_STACK.md` and `docs/DEPENDENCIES_AND_TOOLS.md` define technology/dependency/tool policy.
- `docs/QUALITY_AND_TESTING.md` and `docs/PERFORMANCE.md` define evidence/benchmarks.
- `docs/UX_ACCESSIBILITY_AND_DESIGN.md` defines UI/a11y/RTL rules.
- `docs/RELEASE_STRATEGY.md` defines alpha/beta/RC/stable semantics.
- `CHANGELOG.md` records repository changes.

No document may claim an unimplemented requirement is shipped.