# Changelog

All notable changes to Atlas Reader Native are documented here.

## [Unreleased]

### Native 2.0 bootstrap program

- Established C++23 + Qt Quick/QML + CMake successor architecture.
- Added pure C++ contracts for document capabilities, PDF engine, filesystem, and library index.
- Added minimal Qt Quick application shell and native smoke test.
- Added Windows CI skeleton and corrected its public Qt pin after the first run exposed an upstream aqt Windows Qt 6.11.x repository-install failure.
- Added project-local CodeGraph and read-only GitHub MCP configuration.
- Defined the native release line through `2.0.0`: N0–N2 alphas, N3–N9 betas, N10 RC, N11 stable Windows 2.0.
- Expanded `CHECKPOINTS.md` into explicit subgates, owner evidence, and release mapping.
- Added complete Windows 2.0 product scope with P0/P1/P2/deferred boundaries.
- Added open-source dependency/tool/plugin policy including vcpkg manifest direction, PDF-engine qualification, testing/profiling/accessibility tooling, and AI-agent guardrails.
- Added quality/testing/preservation/failure-injection strategy.
- Added UX/accessibility/Arabic/RTL design contract.
- Added competitive baseline/differentiation strategy covering mature PDF readers/editors without turning feature-count parity into the product goal.
- Added release strategy and development workflow documentation.
- Expanded licensing/third-party/SBOM guardrails.
- Updated master plan, README, INFO, build guide, stack guide, and AGENTS rules around the completed 2.0 program.
- Explicitly kept production PDF engines, SQLite, scanning, reader features, bookmarks, annotations, migration, and installer code out of N0.

### CI finding

The first native Windows run failed in the Qt install step before Configure/Build/Test: public `aqtinstall` could resolve Qt 6.11.2 but failed to locate its Windows repository XML. N0 therefore remained unverified. Bootstrap CI now uses Qt 6.10.3 publicly while N1 owns canonical Qt 6.11.x qualification and CI/local convergence.

## Versioning

The native successor belongs to the `2.0.0` release line.

- `2.0.0-alpha.0` — N0 bootstrap (engineering-only)
- `2.0.0-alpha.1` — N1 Windows/toolchain baseline
- `2.0.0-alpha.2` — N2 PDF-engine qualification
- `2.0.0-beta.1` onward — usable feature checkpoints from N3
- `2.0.0-rc.N` — release qualification
- `2.0.0` — accepted stable Windows release

See `docs/RELEASE_STRATEGY.md` for the binding rules.