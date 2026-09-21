# ADR-0003 — N1 Qt / Windows Toolchain Pin

**Status:** Accepted for N1 alpha baseline  
**Date:** 2026-09-22  
**Checkpoint:** N1

## Context

Atlas wants the current Qt 6.11 generation, but a build foundation is useful only when it can be reproduced in public CI without hidden credentials. Qt 6.11.2 is available through Qt's official installer/source channels, while the unauthenticated `aqtinstall` Windows 6.11.x binary repository path is currently unreliable.

The N0 bootstrap demonstrated that public Qt 6.10.3 MSVC 2022 binaries install reproducibly on GitHub Actions and that the Atlas shell uses no API newer than the Qt 6.10 floor.

## Decision

For `2.0.0-alpha.1`:

- canonical public CI Qt is **6.10.3 MSVC 2022 64-bit**;
- canonical generator is **Visual Studio 17 2022 x64**;
- canonical compiler family is **MSVC v143 / C++23**;
- application source declares a **Qt 6.10 API floor**;
- Qt 6.11.2 is an allowed additional developer compatibility build, not the canonical N1 CI pin;
- Atlas will not store Qt account credentials merely to make public CI use 6.11.2;
- no 6.11-only API may enter while public canonical CI remains 6.10.3;
- the Qt pin must be re-evaluated before later performance/release qualification, with a preference for the current secure supported patch that is reproducibly buildable.

## Consequences

Positive:

- every contributor can reproduce the public alpha build;
- CI does not rely on private account state;
- Debug/Release and local/CI generator behavior remain comparable;
- Atlas can continue architecture work without coupling source to a transient installer outage.

Tradeoffs:

- N1 does not exercise 6.11.2-specific fixes in its canonical public lane;
- framework performance numbers must name the exact Qt patch;
- moving to 6.11.x later requires a benchmark comparison and documentation update.

## Alternatives considered

### Require Qt 6.11.2 through the authenticated official installer

Rejected for the public alpha baseline because it makes ordinary CI depend on private Qt-account secrets.

### Build Qt 6.11.2 from source in every CI job

Rejected for N1 because the cost and build time are disproportionate to an empty-shell checkpoint.

### Downgrade the architecture to a different UI framework

Rejected. The installer/repository limitation does not invalidate the already accepted native-stack decision.

### Hard-code exactly Qt 6.10.3 in application `find_package`

Rejected. Exact patch selection belongs to the toolchain/reproducibility layer; application source intentionally states the compatible API floor.

## Revisit trigger

Revisit when any of the following becomes true:

- public Windows Qt 6.11.x packages become reproducibly installable again;
- a newer Qt generation is selected before a feature checkpoint;
- a security issue requires an immediate Qt move;
- an Atlas requirement genuinely needs a newer Qt API;
- release packaging requires an exact newer supported patch.
