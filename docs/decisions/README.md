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
- `ADR-0004-pdf-engine-responsibilities.md` — **Accepted** — N2 evidence-driven assignment of read/render/text/navigation versus structure/security/transformation responsibilities.
- `ADR-0005-document-identity-and-reconciliation.md` — **Accepted** — Atlas-owned document IDs with conservative, reviewable move/copy/offline reconciliation.
- `ADR-0006-interface-languages.md` — **Accepted** — English/Arabic/Urdu interface via checked string tables, live switching, bundled Nastaliq and Naskh fonts.
- `ADR-0007-foxit-workspace-parity.md` — **Accepted** — Foxit PDF Editor ribbon workspace, measurements and shortcuts reproduced with Atlas's own icons, colours and three languages; Foxit features staged before/after 2.0.
- `ADR-0008-bookmark-overlay-and-location-envelope.md` — **Proposed** — bookmark overlay in the profile database (schema 5) with explicit storage states, base snapshot and tombstones, plus the versioned `atlas-location/1` envelope; Accepted with `N5 PASS`.
- `ADR-0009-outline-safe-save.md` — **Proposed** — what Save writes into the PDF (the visible bookmark tree; hidden PDF bookmarks kept unless removal is chosen), qpdf `--update-from-json` with a fresh outline item set that keeps every existing key, destination conversion through crop box and /Rotate, and the proof chain before Embedded; Accepted with `N5 PASS`.

ADR-0004 was Accepted by owner `N2 PASS` on 2026-09-27: Qt PDF 6.10.3 for reading/rendering plus qpdf 12.4.1 CLI for structure/security/writes. ADR-0005 governs identity and reconciliation (implemented in N3, extended by N5). Scope and priority decisions that are not architectural are recorded in the [PLAN.md decision log](../../PLAN.md#9-decision-log).
