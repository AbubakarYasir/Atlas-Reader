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

ADR-0004 remains **Proposed** while N2 is Ready for owner test. The frozen route
is Qt PDF 6.10.3 for read/render/text/navigation plus qpdf 12.4.1 CLI for
structure/security/write; PDFium remains qualification evidence. ADR-0004
becomes Accepted only after explicit owner `N2 PASS`.
