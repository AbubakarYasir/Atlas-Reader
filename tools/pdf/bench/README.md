# N2 PDF Engine Performance Benchmark Contract

This directory is an **engineering qualification harness**, not product code and not an end-user performance promise.

## Purpose

Measure repeated Qt PDF and PDFium operations under the same narrow synthetic workload so N2 can compare candidate read engines without treating one-shot functional-probe timings as benchmark evidence.

The benchmark covers:

- document open from already-loaded in-memory PDF bytes;
- full-document text extraction on one persistent loaded document;
- known-hit full-document search;
- single-page raster rendering;
- process working-set change across the repeated workload.

## Measurement protocol

Canonical CI runs use:

- Windows Server 2022 GitHub-hosted runner;
- MSVC 2022 x64 Release builds;
- Qt PDF 6.10.3;
- PDFium 156.0.8066.0 / `chromium/8066` from the existing pinned probe package;
- 3 excluded warm-up iterations per operation;
- 31 measured warm iterations per operation;
- nearest-rank p50 and p95;
- render size 612 x 792 pixels;
- serialized, single-thread PDFium calls;
- exact known search-hit counts as a correctness guard.

Each scenario records the first operation separately from the warm sample distribution. First-operation values are descriptive only; they are not mixed into warm p50/p95.

## Canonical scenarios

### A003 — ordinary English/navigation fixture

- Query: `Atlas N2 Fixture A003 page 1`
- Expected hits: 1
- Render page: 0

This gives a small ordinary-text/navigation workload using the existing independently validated fixture.

### A006 — Unicode/Arabic fixture

- Query: `مرحبا بالعالم`
- Expected hits: 2
- Render page: 0

A006 is regenerated deterministically before the benchmark. Its Type3 glyphs make it suitable for **logical Unicode/search workload comparison only**; it is not representative Arabic visual-render fidelity evidence.

## Interpretation rules

1. Benchmark JSON must report `passed: true`; failed correctness guards invalidate the timing sample.
2. p50/p95 values are compared only for the same fixture, operation, build type and run environment.
3. GitHub-hosted runner values are comparative engineering evidence, not a prediction of the user's physical Windows machine.
4. The synthetic corpus is intentionally small. It can expose fixed engine overhead and obvious regressions, but it does **not** replace a later large/real-world PDF workload.
5. Search timings include each engine's own search model/API semantics. They are useful for observed end-to-end candidate behavior but must not be mistaken for identical internal work.
6. Memory evidence is process working set before vs. after the workload and peak working set. It is a coarse leak/regression signal, not heap attribution.
7. No absolute PASS threshold is invented during this slice. N2 records measured evidence first; responsibility selection happens only after correctness, preservation, security, licensing/provenance and performance evidence are considered together.

## Isolation

The benchmark uses its own mini-CMake project and workflow. `atlas_reader` and the product/domain targets remain unlinked to both Qt PDF and PDFium during N2 qualification.
