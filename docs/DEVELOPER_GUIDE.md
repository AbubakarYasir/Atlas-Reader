# Atlas Reader — Developer Guide

[Project introduction](../README.md) · [Full documentation index](README.md)

## Current state

Native engineering version: `2.0.0-alpha.2`. N0, N1 and N2 are Accepted; N3 Library/index foundation is In progress. N3.1 storage/schema is CI-verified; N3.2 safe scanning is active. See the [N3 plan](N3_LIBRARY_INDEX_PLAN.md), [N3 evidence ledger](baselines/N3_LIBRARY_INDEX_EVIDENCE.md), and [N2 acceptance record](baselines/N2_ACCEPTANCE.md). The full Library, Reader, bookmark editor and annotations are not complete.

Permanent branches are `main` (accepted native work) and `legacy/flutter` (stopped, obsolete backup). N3 work uses `native-v2-n3-library-index-foundation` from the exact accepted `main` base. Follow the N3 subgate and owner checks; do not revive Flutter.

## Before changing anything

Read [agent/contributor rules](../AGENTS.md), the [checkpoint ledger](../CHECKPOINTS.md), and the relevant stage plan. Follow the [development workflow](DEVELOPMENT_WORKFLOW.md) and [Git/documentation policy](GIT_WORKFLOW.md). Every completed change must update affected documentation and include verification evidence.

## Architecture and dependencies

- C++23 shared core with Qt 6 / Qt Quick presentation and platform adapters.
- Accepted Windows toolchain: Visual Studio 2022 x64 / MSVC v143 and Qt 6.10.3.
- Accepted PDF route: official dynamic Qt PDF 6.10.3 for reading/rendering/text/navigation; first-party qpdf 12.4.1 CLI for structural/security/write operations.
- Standalone PDFium remains qualification/comparison evidence, not part of the selected production package.
- Production SQLite/FTS5 indexing is N3 work. The N3.1 schema, repository and search foundation is implemented and Windows CI verified; the scanner and production Library remain unfinished N3 gates.
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
