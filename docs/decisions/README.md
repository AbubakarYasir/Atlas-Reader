# Architecture Decision Records

ADRs record decisions that materially constrain Atlas architecture, data formats, dependencies, portability, security, or release engineering.

## Status values

- **Proposed** — being evaluated; not binding.
- **Accepted** — current project decision.
- **Superseded** — replaced by a newer ADR; history retained.
- **Rejected** — evaluated and intentionally not chosen.

## Naming

`ADR-####-short-title.md`

Never renumber/remove an accepted ADR. Supersede it with a new record.

## Required sections

1. Status/date
2. Context/problem
3. Decision
4. Alternatives considered
5. Consequences/tradeoffs
6. Revisit conditions
7. Related checkpoints/docs

## Decisions that normally require an ADR

- application language/UI framework;
- PDF engine responsibility split;
- database/search architecture;
- dependency manager;
- canonical build/toolchain policy when it affects reproducibility/platform support;
- persisted interchange/backup format;
- document identity strategy;
- threading/task architecture;
- installer/update architecture;
- runtime plugin/extension system;
- licensing/distribution changes;
- major cross-platform storage abstraction changes.

Ordinary implementation details do not need an ADR.

## Current records

- `ADR-0001-native-stack.md` — **Accepted** — C++23 + Qt Quick native successor architecture.
- `ADR-0002-vcpkg-manifest.md` — **Accepted** — vcpkg manifest mode for non-Qt native production dependencies.
- `ADR-0003-n1-qt-toolchain-pin.md` — **Accepted** — public N1 Qt 6.10.3/MSVC 2022 baseline while newer compatible Qt kits may be separately qualified.
- `ADR-0004-pdf-engine-responsibilities.md` — **Proposed** — N2 evidence-driven assignment of read/render/text/navigation versus structure/security/transformation responsibilities.

ADR-0004 must remain **Proposed** while N2 is in progress. The candidate order in the qualification plan/matrix is not a ranking and does not select a winner. It becomes Accepted only after sufficient evidence, final responsibility assignment, strict CI, and explicit owner `N2 PASS`.
