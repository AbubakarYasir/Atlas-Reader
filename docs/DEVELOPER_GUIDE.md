# Atlas Reader — Developer Guide

[Project introduction](../README.md) · [Full documentation index](README.md)

## Current state

Native engineering version: `2.0.0-beta.1`. N0–N3 are Accepted; N4 has not started in this accepted snapshot. See the [N3 acceptance record](baselines/N3_ACCEPTANCE.md), [N3 plan](N3_LIBRARY_INDEX_PLAN.md), [evidence ledger](baselines/N3_LIBRARY_INDEX_EVIDENCE.md), and [owner test](N3_OWNER_TEST.md). The built-in Reader, bookmark editor and annotations are not complete.

Permanent branches are `main` (accepted native work) and `legacy/flutter` (stopped, obsolete backup). The completed N3 branch is merged and deleted only after exact-main verification. N4 work must use `native-v2-n4-reader-foundation` from that accepted merge; do not revive Flutter.

## Before changing anything

Read [agent/contributor rules](../AGENTS.md), the [checkpoint ledger](../CHECKPOINTS.md), and the relevant stage plan. Follow the [development workflow](DEVELOPMENT_WORKFLOW.md) and [Git/documentation policy](GIT_WORKFLOW.md). Every completed change must update affected documentation and include verification evidence.

## Architecture and dependencies

- C++23 shared core with Qt 6 / Qt Quick presentation and platform adapters.
- Accepted Windows toolchain: Visual Studio 2022 x64 / MSVC v143 and Qt 6.10.3.
- Accepted PDF route: official dynamic Qt PDF 6.10.3 for reading/rendering/text/navigation; first-party qpdf 12.4.1 CLI for structural/security/write operations.
- Standalone PDFium remains qualification/comparison evidence, not part of the selected production package.
- Production SQLite/FTS5 indexing, scanning and the Library foundation were Accepted in N3.
- CMake and pinned dependency acquisition keep builds reproducible. Engine/platform types remain behind Atlas-owned interfaces.

[Architecture](ARCHITECTURE.md) · [Toolchain](TOOLCHAIN.md) · [Dependencies](DEPENDENCIES_AND_TOOLS.md) · [Accepted PDF decision](decisions/ADR-0004-pdf-engine-responsibilities.md) · [Licensing](LICENSING.md)

## Build and verify

[BUILDING.md](BUILDING.md) owns installation, exact dependency pins, shell build commands, PDF probes and package verification. A shell build alone does not exercise all PDF probes.

For documentation changes, run from the repository root:

```powershell
pwsh -NoProfile -File tools/docs/check-markdown.ps1
git diff --check
```

Behavior changes also require the tests specified by [quality and testing](QUALITY_AND_TESTING.md) and the [checkpoint QA matrix](CHECKPOINT_QA_MATRIX.md). Windows CI checks Debug and Release. Manual owner acceptance remains separate from automated success.

## Find code and evidence

- `src/app/`: application shell and diagnostics.
- `src/core/`: portable contracts and domain code.
- `qml/`: presentation.
- `tests/fixtures/pdf/`: documented PDF fixtures and provenance.
- `tools/`: build, qualification, benchmark and documentation helpers.
- `.github/workflows/`: automated checks.
- `docs/baselines/`: measured checkpoint evidence.
- `docs/decisions/`: architecture decisions.

[N1 accepted baseline](baselines/N1_FINAL_QUALIFICATION.md) · [N2 evidence matrix](baselines/N2_PDF_ENGINE_MATRIX.md) · [Release rules](RELEASE_STRATEGY.md) · [Security](../SECURITY.md)

The [documentation index](README.md) links the full product, data, performance, accessibility, migration and security contracts. Moving technical details out of the public homepage does not weaken or replace those requirements.
