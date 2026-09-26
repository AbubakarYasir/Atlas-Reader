# N2 — PDF Engine Qualification Plan

**Checkpoint:** N2 — PDF engine qualification spike  
**Planned version:** `2.0.0-alpha.2`  
**Status:** **In progress — planning and fixture contract**  
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Base:** accepted N1 integration commit `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`

## 1. Purpose

N2 exists to choose PDF **responsibilities** using measured evidence before Atlas builds the real reader, indexing, bookmarks, annotations, or safe-save pipeline around the wrong engine assumptions.

N2 is a qualification spike, not a reader implementation checkpoint.

The question is not “which single library wins?” The permitted outcome is a split architecture, for example:

- one engine for page rendering/text/search/link/navigation;
- qpdf for structural inspection, security/encryption information, and preservation-sensitive transformation;
- Atlas-owned normalization types between them.

No engine object model may become Atlas's public domain model.

## 2. Frozen predecessor

N1 is Accepted and immutable unless new evidence demonstrates a user-facing N1 regression. N2 must not reinterpret N1 performance or reopen the accepted Windows shell baseline.

The N2 branch starts from the N1 merge commit above. `main` remains untouched.

## 3. Explicit N2 scope

### In scope

- Qt PDF qualification;
- PDFium qualification;
- qpdf structural/security/transformation qualification;
- minimal throwaway/adaptor prototypes behind Atlas-owned interfaces;
- real PDF fixture corpus and manifest;
- rendering fidelity and timing measurements;
- text extraction/search/link/outline comparison;
- page geometry, crop/media box, rotation and destination normalization;
- password/encryption/restriction behavior;
- malformed-input containment;
- preservation-focused structural mutation experiments on copied fixtures;
- build/reproducibility/integration cost;
- runtime/binary footprint evidence where material;
- licensing/notices/supply-chain assessment;
- final ADR assigning PDF responsibilities.

### Explicitly out of scope

- production Reader UI;
- virtualized viewport/page texture cache;
- SQLite/FTS5 library index;
- production bookmarks editor/local overlay;
- annotation/ink implementation;
- production safe-save workflow;
- migration from Flutter;
- installer/release packaging;
- OCR;
- AI document analysis.

A spike may expose enough code to measure an engine, but N2 must not smuggle N3/N4/N5 implementation into the repository.

## 4. Candidate facts checked at N2 opening — 2026-09-22

These facts are inputs to qualification, not conclusions.

### Qt PDF

Current Qt 6 documentation exposes:

- `QPdfDocument` for loading, page count/size/labels, text access and rendering;
- `QPdfPageRenderer` for queued rendering;
- `QPdfSearchModel` for search;
- `QPdfBookmarkModel` for document outlines;
- `QPdfLinkModel` for links/navigation;
- Qt Quick PDF viewer building blocks.

Qt PDF is documented under commercial terms and, for open-source use, LGPLv3 or GPLv2 terms. Atlas must keep the same license-compliance discipline already applied to Qt.

N2 does **not** use the full Qt PDF viewer component as the future Atlas reader architecture. Atlas owns its viewport; Qt PDF is qualified as an engine/adapter candidate.

### PDFium

PDFium's public embedder API provides document loading, page rendering, text APIs, bookmarks/destinations/links and related low-level functions.

Important upstream constraint: the current public API header explicitly states that PDFium APIs are **not thread-safe** and embedders must prevent concurrent PDFium API calls. N2 must measure the consequences instead of assuming parallel calls are safe.

Official PDFium source builds use the Chromium toolchain model: depot_tools/gclient, GN, Ninja, and Clang/clang-cl rather than the Atlas MSVC/CMake-only path. This build/integration burden is part of the comparison.

PDFium source is under a BSD-style license, with third-party notices/dependencies that still require inventory if selected.

### qpdf

qpdf is evaluated for structure/security/transformation, not page rasterization.

At N2 opening:

- official GitHub releases identify `12.4.1` (2026-08-27) as the latest published release visible in the release feed;
- current latest documentation may already identify `12.4.2`, so Atlas must distinguish published release artifacts from documentation built ahead of release;
- the current Microsoft vcpkg curated `qpdf` port inspected by Atlas is `12.4.0`, licensed as `Apache-2.0 AND MIT` in the port metadata.

N2 will not silently pretend those three version surfaces are identical. The exact qpdf version used by each experiment must be recorded.

qpdf 12.x also provides JSON v2 object-level inspection/update facilities, encryption/security handling, and the C++ library API. These are useful diagnostic capabilities even if production code ultimately uses direct helper classes instead of JSON.

### PDFium binary bootstrap option

The official PDFium build is deliberately non-trivial. For **N2 spike speed only**, Atlas may use the community `bblanchon/pdfium-binaries` project to bootstrap a reproducible Windows x64 PDFium probe, provided:

- an exact release/tag is pinned;
- the downloaded archive SHA-256 is recorded;
- headers and binary come from the same pinned package;
- the package is treated as a third-party distribution, not Google-official binary provenance;
- the final ADR separately decides whether this distribution route is acceptable for production.

Its project reports automated cross-platform builds and CMake package support. Using it in a spike does not pre-decide the long-term PDFium supply chain.

## 5. Architecture boundary under test

The existing `src/core/pdf/IPdfEngine.h` is intentionally tiny. N2 must evolve the contract only as required to compare engines, and keep it portable.

Target conceptual boundary:

```text
Atlas application/domain
        |
        v
Atlas PDF contracts / normalized types
        |
        +--------------------+
        |                    |
        v                    v
read/render adapter     structural adapter
(Qt PDF/PDFium)              (qpdf)
```

Normalized Atlas types must own semantics for:

- page index versus page label;
- page point size;
- media/crop boxes;
- rotation;
- render request/output metadata;
- extracted Unicode text and character/range bounds;
- search hit ranges/geometry;
- links/actions/destinations;
- outline hierarchy/destinations;
- password-required / incorrect-password / unsupported-security states;
- permissions/capabilities;
- malformed-but-readable versus fatal-open failure.

N2 should prefer adding focused contracts (`IPdfReadEngine`, normalized data records, structural inspection port) over turning `IPdfEngine` into one enormous abstraction.

## 6. Qualification phases

### N2.0 — plan, fixture contract, upstream inventory

Deliverables:

- this plan;
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md`;
- fixture manifest rules under `tests/fixtures/pdf/`;
- proposed ADR-0004;
- N2 checkpoint status/version documentation;
- CI still green before any engine is introduced.

### N2.1 — Qt PDF probe

Build a small non-production adapter/probe using the same pinned Qt family as Atlas.

Measure/record:

- open/close;
- page count, labels, sizes;
- render correctness and timing at representative output sizes;
- Unicode text extraction;
- Arabic/English search;
- links;
- outline hierarchy/destinations;
- encrypted/password behavior;
- malformed fixture behavior;
- process memory and package delta where meaningful.

Do not adopt Qt's ready-made viewer as the Atlas reader.

### N2.2 — PDFium probe

Use the same normalized fixture outputs as N2.1.

Measure the same capabilities plus:

- impact of the non-thread-safe API contract;
- safe serialization/mutex model;
- separate-document concurrency experiment only if it can be done without violating the upstream API contract;
- binary/dependency footprint;
- reproducible acquisition/build path;
- packaging complexity.

If a community prebuilt is used, record exact tag/hash separately from the upstream PDFium revision embedded in it.

### N2.3 — qpdf structural/security probe

Use copied fixtures only.

Measure/record:

- page tree/basic geometry inspection needed by Atlas;
- outline/object access required for later bookmark work;
- encryption/password/restriction reporting;
- malformed-file warnings/recovery behavior;
- signature/certification visibility available to Atlas;
- controlled no-op/rewrite preservation observations;
- controlled outline-level transformation on synthetic/redistributable fixtures;
- reopen/check output after mutation;
- exact qpdf/vcpkg versions.

N2 mutations are experiments, never performed on user originals.

### N2.4 — normalized cross-engine comparison

The same fixture runner produces comparable JSON/CSV-style evidence for Qt PDF and PDFium wherever capabilities overlap.

Required comparison classes:

- document/page metadata;
- rendering;
- text extraction;
- search;
- links;
- outlines/destinations;
- password/security states;
- malformed input;
- performance/memory;
- integration/packaging.

Differences must be recorded as either:

- engine bug/limitation;
- Atlas normalization bug;
- fixture ambiguity;
- expected engine semantic difference;
- unresolved blocker.

### N2.5 — preservation/security comparison

Use qpdf and independent readers/tools to test the transformations Atlas will eventually depend on.

At minimum record relevant before/after invariants:

- page count;
- page dimensions/rotation;
- page content streams when the operation should not touch them;
- existing annotations;
- outline hierarchy/order/destinations;
- document information/XMP presence;
- encryption policy;
- signature/certification consequences;
- attachments/other structures if touched by the writer.

### N2.6 — decision and owner handoff

Complete ADR-0004 with an evidence-backed responsibility assignment.

Possible valid outcomes include:

- Qt PDF read/render + qpdf structure/write;
- PDFium read/render + qpdf structure/write;
- mixed use where one read engine has a narrowly better capability and complexity remains justified;
- rejection/blocking of a candidate due to reproducibility, licensing, correctness, preservation or performance.

There is no predetermined winner.

## 7. Fixture classes

N2 requires a small but discriminating corpus, not thousands of random PDFs.

Minimum tracked classes:

1. simple one-page text;
2. multi-page page-label document;
3. Arabic-only text with searchable/extractable Unicode;
4. mixed Arabic/English text;
5. Arabic/English outline hierarchy;
6. duplicate outline titles under different parents;
7. deep outline hierarchy;
8. internal link/destination fixture;
9. external URI link fixture;
10. rotated pages;
11. mixed page sizes;
12. non-default crop/media boxes;
13. image-only page;
14. existing annotations;
15. metadata/XMP-rich document;
16. open-password encrypted document;
17. permission-restricted document;
18. malformed-but-readable document;
19. signed/certified test document where redistribution permits;
20. long document for render/text timing.

Each tracked fixture must state source/provenance, redistribution permission, checksum, expected capabilities and whether mutation is allowed.

Private owner PDFs can supplement the corpus, but their bytes, paths, titles and extracted text must not enter Git history, public CI logs, or ordinary benchmark artifacts.

## 8. Measurement rules

### Rendering

For comparable render work:

- use Release builds;
- same physical fixture/page;
- same requested pixel dimensions/background assumptions;
- record warm and cold context separately;
- at least 10 measured warm renders for timing comparisons after one unmeasured warm-up unless a fixture is intentionally cold-load focused;
- record median and p95, not only best time;
- hash the produced raster bytes after canonicalizing format when practical;
- use visual/golden comparison only with documented tolerance.

A faster incorrect raster loses.

### Open/text/search

Record separately:

- document open-to-ready;
- first text extraction for a page;
- repeated extraction;
- search completion and hit count;
- Unicode result text/hash plus hit geometry/destination where relevant.

### Memory

Measure:

- process delta after engine initialization;
- opened-document steady state;
- repeated render/search loop;
- release after document close.

The spike should detect obvious leaks/unbounded caches; production cache tuning belongs to N4/N10.

### Build/integration cost

Record:

- acquisition mechanism;
- exact revision/version;
- clean build/setup steps;
- CI time delta;
- binary/package delta;
- compiler/toolchain exceptions;
- transitive license/notices inventory;
- update/rollback path.

Integration complexity is a real engineering cost and can break ties, but cannot outweigh correctness.

## 9. Scoring policy

N2 does **not** use one opaque weighted score that can hide fatal failures.

Each candidate gets per-capability states:

- **PASS** — evidence meets Atlas requirement;
- **PASS WITH LIMITATION** — usable with a named, bounded limitation/workaround;
- **FAIL** — unsuitable for that responsibility;
- **BLOCKED** — evidence cannot yet be produced;
- **N/A** — responsibility is outside that engine's intended role.

Hard blockers override speed:

- corrupts or silently loses unrelated PDF content in the operation under test;
- cannot preserve required Unicode/Arabic semantics for a responsibility;
- cannot be redistributed under Atlas's chosen distribution model;
- build/acquisition cannot be pinned/reproduced sufficiently;
- unsafe concurrency assumptions would force unacceptable architecture;
- critical encrypted/restricted/signed states cannot be distinguished safely enough for later workflows.

## 10. Initial hypotheses to test, not assumptions to keep

- Qt PDF may be the lowest-integration-cost read/render option because it is already in the Qt ecosystem.
- PDFium may provide stronger low-level rendering/text behavior but carries supply-chain/build and serialization complexity.
- qpdf is likely complementary rather than a raster engine and may be the strongest structural/preservation tool.

These are hypotheses only. The matrix and ADR decide.

## 11. Stop gate

N2 cannot be Accepted until:

1. Qt PDF and PDFium have comparable evidence for the read/render capabilities Atlas needs, or one is explicitly blocked/rejected with documented evidence;
2. qpdf has preservation/security/structural evidence on the required copied fixtures;
3. Arabic/Unicode, page labels, outlines/destinations, rotation/boxes, links, encryption and malformed cases are represented;
4. build/reproducibility/license implications are recorded;
5. adapter boundaries keep engine types out of portable domain/application code;
6. ADR-0004 assigns responsibilities and rollback/replaceability rules;
7. strict CI passes on the final N2 branch head;
8. owner reviews the evidence and explicitly records `N2 PASS`.

Until then N3 remains Not started.

## 12. Documentation discipline

Every meaningful N2 experiment updates GitHub Markdown in the same branch:

- matrix result;
- exact dependency/revision;
- command/build path;
- fixture checksum/provenance;
- measurements;
- limitations/failures;
- decision impact.

Raw machine output may stay under ignored `artifacts/` when large, but summarized evidence and exact artifact/checksum references belong in Git history.
