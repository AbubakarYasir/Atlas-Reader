# ADR-0002 — vcpkg Manifest Mode for Non-Qt Native Dependencies

**Status:** Accepted

**Date:** 2026-09-22

## Context

Atlas Native will eventually use native C/C++ libraries such as qpdf, a test framework, benchmark tooling, JSON serialization, and potentially PDFium-related components. Ad-hoc global installs, unpinned source downloads, or mixing multiple dependency systems would make Windows CI, future mobile builds, license inventory, and reproducibility harder.

Qt is a specialized SDK/toolchain and is already installed/versioned separately.

## Decision

Use **vcpkg manifest mode** for accepted non-Qt native production/development C/C++ dependencies where suitable ports exist.

Beginning in N2 when the first such dependencies enter the real build, Atlas checks in:

- `vcpkg.json`;
- `vcpkg-configuration.json` when required;
- a pinned vcpkg baseline;
- explicit overrides only when necessary and documented.

Qt remains outside the vcpkg graph unless a later ADR demonstrates that moving Qt into it materially improves the project without harming toolchain/mobile/release handling.

CMake remains the build-system source of truth and integrates with the vcpkg toolchain through documented presets/CI configuration.

## Alternatives considered

### Global/system dependencies

Rejected as the normal strategy because developer/CI versions become implicit and non-reproducible, especially on Windows.

### Vendoring every library in the repository

Rejected as a default because it increases repository size, update burden, license maintenance, and patch divergence. Vendoring remains an explicit exception when upstream/package-manager support cannot satisfy a critical requirement.

### CMake FetchContent for every dependency

Rejected as the default because version resolution, transitive dependency policy, binary caching, and license inventory become fragmented across CMake files. Small header-only exceptions may be considered only with justification.

### Conan

A viable native package manager, but rejected for the initial program because vcpkg integrates naturally with CMake/Windows, has project manifest/versioning support, and is sufficient for Atlas's expected dependency set. Revisit if mobile/platform or package availability evidence shows a material limitation.

## Consequences

### Positive

- project-local dependency declaration;
- pinned/reviewable baselines;
- easier Windows developer/CI parity;
- CMake integration;
- cross-platform triplets for future ports;
- clearer dependency/SBOM/license inventory;
- simpler rollback of dependency changes.

### Costs

- vcpkg itself becomes a toolchain dependency;
- some upstream libraries/patches may lag in ports;
- large libraries such as PDFium may need a specialized build path if vcpkg is not a good fit;
- mobile triplets still require qualification rather than assumption.

## Exceptions

An exception must document:

1. why vcpkg cannot meet the requirement;
2. how the alternative is pinned/reproducible;
3. how license/SBOM notices are generated;
4. cross-platform consequences;
5. update/rollback path.

Do not replace the entire dependency strategy merely because one dependency needs an exception.

## Revisit conditions

Revisit this ADR if:

- Android/iOS builds reveal a systemic vcpkg blocker;
- critical production dependencies cannot be maintained reproducibly through it;
- another solution provides materially better reproducibility/security/license management across all Atlas targets.

A change requires a superseding ADR and migration plan.

## Related

- `docs/DEPENDENCIES_AND_TOOLS.md`
- `docs/TECH_STACK.md`
- `docs/LICENSING.md`
- N2 — PDF engine qualification