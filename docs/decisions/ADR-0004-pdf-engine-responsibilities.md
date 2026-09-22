# ADR-0004 — PDF Engine Responsibility Split

**Status:** Proposed  
**Date proposed:** 2026-09-22  
**Checkpoint:** N2 — PDF engine qualification spike (`2.0.0-alpha.2`)

## Context

Atlas needs PDF capabilities across several materially different responsibilities:

- opening and inspecting documents;
- page geometry and labels;
- raster rendering;
- Unicode text extraction and search;
- links, outlines and destinations;
- encryption/security/capability inspection;
- preservation-sensitive structural mutation.

Choosing one library for every responsibility merely because it can perform some of them would unnecessarily couple Atlas to one object model, performance profile, threading model, build system and preservation behavior.

The accepted architecture already requires Atlas-owned PDF contracts and explicitly permits rendering and structural mutation to use different engines.

N2 therefore compares Qt PDF, PDFium and qpdf with real fixtures and repeatable evidence before assigning production responsibilities.

## Proposed decision shape

**No engine is selected by this ADR yet.**

The decision N2 is testing is a responsibility split behind Atlas-owned ports:

```text
Atlas domain/application
        |
        v
normalized Atlas PDF contracts
        |
        +----------------------+----------------------+
        |                      |                      |
        v                      v                      v
read/render/text          navigation/outline     structure/write
Qt PDF or PDFium          Qt PDF or PDFium            qpdf
```

A single engine may ultimately cover more than one column if evidence supports it, but application/domain code must not depend directly on Qt PDF, PDFium or qpdf types.

## Candidates

### Qt PDF

Qualify for:

- document/page metadata;
- raster rendering;
- text extraction/search;
- links/navigation;
- outline reading/destinations.

Primary advantages to test:

- lowest integration friction with the existing Qt toolchain;
- existing Qt object/lifecycle integration;
- cross-platform availability aligned with Atlas's UI stack.

Primary risks/limitations to test:

- fidelity/performance on the Atlas fixture corpus;
- Arabic/mixed-script extraction/search semantics;
- API coverage for normalized destinations/geometry;
- whether its convenience APIs hide distinctions Atlas needs later.

### PDFium

Qualify for the same read/render/text/navigation responsibilities as Qt PDF.

Primary advantages to test:

- low-level rendering/text/navigation APIs;
- mature Chromium PDF engine behavior.

Primary risks/limitations to test:

- current public API contract is not thread-safe, so Atlas must serialize calls safely;
- official source builds require Chromium-style depot_tools/gclient + GN/Ninja + Clang tooling rather than Atlas's normal CMake/MSVC-only dependency path;
- no standard Microsoft vcpkg `pdfium` port was found at N2 opening;
- production binary provenance/update policy is not yet decided.

A pinned community prebuilt may be used for N2 measurement only if its exact tag/revision and SHA-256 are recorded. That does not pre-approve it as the production supply chain.

### qpdf

Qualify primarily for:

- structural inspection;
- encryption/security information;
- outline/object-level transformation needed by later bookmark workflows;
- preservation-sensitive document rewriting/validation support.

qpdf is not being evaluated as the primary page raster engine.

## Decision criteria

The final responsibility assignment must be based on the durable matrix in `docs/baselines/N2_PDF_ENGINE_MATRIX.md`.

A candidate may be selected for a responsibility only when the relevant evidence addresses:

1. correctness on representative fixtures;
2. Arabic/Unicode semantics where applicable;
3. page/destination/geometry normalization;
4. encrypted/restricted/malformed states;
5. preservation behavior for mutation responsibilities;
6. measured Release performance and memory where hot-path relevant;
7. threading/concurrency constraints;
8. reproducible build/acquisition and rollback;
9. license/notices/distribution requirements;
10. cross-platform viability and replaceability.

Correctness, safety, preservation and redistributability are hard gates. A faster engine does not win if it fails them.

## Alternatives considered

### One universal PDF engine

Not accepted as an assumption. It remains possible only if one candidate independently proves it is the best practical choice for all required responsibilities without creating unacceptable coupling or preservation risk.

### Qt PDF only

Not accepted before evidence. Integration simplicity is valuable but not sufficient.

### PDFium only

Not accepted before evidence. Rendering maturity does not by itself prove suitable structural mutation/preservation or supply-chain fit.

### qpdf only

Rejected as the intended universal engine because qpdf is fundamentally a structural/transformation library rather than Atlas's page-raster renderer.

### Proprietary PDF SDK

Outside N2 unless an explicit later business/licensing decision changes the open architecture policy.

### MuPDF

Remains deferred pending a separate licensing/distribution decision; it is not silently introduced into N2.

## Consequences if the split is accepted

Positive:

- Atlas can select the strongest tool per responsibility;
- reader/render decisions stay replaceable;
- structural writes can be tested independently for preservation;
- application/domain code remains engine-neutral;
- a future platform can swap an adapter without rewriting domain rules.

Costs:

- normalization must be explicit and tested;
- two libraries may mean larger packages and more notices;
- destination/page/security semantics may disagree and require contract tests;
- concurrency/lifetime rules become Atlas infrastructure responsibilities.

## Revisit conditions

After N2 acceptance, revisit the responsibility split only when there is concrete evidence such as:

- user-facing rendering/text/navigation regression;
- preservation or security failure;
- upstream abandonment/security issue/license change;
- unacceptable package/performance impact measured in later checkpoints;
- platform port evidence showing the selected adapter is impractical;
- a substantially better engine becomes available and passes the same qualification contract.

A preference change alone is not enough to reopen an accepted checkpoint decision.

## Acceptance condition

This ADR remains **Proposed** throughout the bake-off.

It becomes **Accepted** only when:

- the N2 matrix contains sufficient evidence;
- exact versions/acquisition paths/licenses are recorded;
- the final responsibility table is filled;
- strict N2 CI passes;
- the owner explicitly records `N2 PASS`.

If N2 rejects one or more candidates, this ADR will record the rejection and the selected alternative rather than deleting the evaluation history.

## Related documents

- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md`
- `docs/ARCHITECTURE.md`
- `docs/DEPENDENCIES_AND_TOOLS.md`
- `docs/QUALITY_AND_TESTING.md`
- `docs/LICENSING.md`
- `CHECKPOINTS.md`
