# Atlas Reader Native — Current Project Notes

**State:** native successor PDF-engine qualification

**Release line:** `2.0.0`

**Current checkpoint:** N2 — PDF engine qualification spike

**Current N2 status:** **In progress — bounded capability blockers pass; PDFium/qpdf responsibility assignment is Proposed pending production-route freeze, final CI and owner acceptance.**

**Current build:** `2.0.0-alpha.2`

**Current branch:** `native-v2-n2-pdf-engine-qualification`

**Previous checkpoint:** N1 — **Accepted by owner on 2026-09-22**

**N2 base:** accepted N1 integration merge `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`

**Primary target:** Windows 11

**Future targets:** Android → Linux → macOS → iOS/iPadOS

## Accepted predecessor state

N0 and N1 are Accepted.

The accepted N1 user-qualified runtime is:

- commit `73f567cf2c557f185371d7f944ebf6d69453105a`;
- artifact digest `sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`.

N1 established the reproducible Windows shell/toolchain baseline, Arabic/RTL/theme/scale/keyboard qualification, clean lifecycle evidence, and measured GDI font-backend decision. The owner explicitly accepted the measured English startup tail and recorded `N1 PASS`.

Repository rule: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.** N2 inherits N1; it does not retune it for convenience.

## What this branch is doing

N2 selects PDF **responsibilities** before Atlas builds the production Reader, Index or Bookmarks around one engine assumption.

It may contain only what is needed for PDF-engine qualification:

- Atlas-owned normalized PDF contracts;
- redistributable/synthetic fixture corpus and sanitized private-fixture hooks;
- Qt PDF probe/adapter experiments;
- PDFium probe/adapter experiments;
- qpdf structure/security/transformation experiments;
- rendering/text/search/link/outline/destination measurements;
- encryption/malformed/security-state observations;
- preservation experiments on disposable fixture copies;
- build/reproducibility/package/memory evidence;
- dependency/license/notices analysis;
- benchmark/fixture tooling;
- ADR-0004 and supporting Markdown evidence.

It must **not** contain production Reader UI/viewport, SQLite/FTS5 index, scanner, production bookmark editor/local overlay, annotations/ink, migration, installer, OCR, or AI document analysis.

## N2 binding documents

Read these in order:

1. `CHECKPOINTS.md` — active checkpoint/status/stop gate;
2. `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` — detailed experiment program;
3. `docs/baselines/N2_PDF_ENGINE_MATRIX.md` — durable live evidence;
4. `tests/fixtures/pdf/README.md` — fixture/provenance/privacy/mutation contract;
5. `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` — Proposed responsibility decision;
6. `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/UPSTREAM_CATALOG.md` — acquisition/tool policy.

ADR-0004 remains **Proposed** until N2 evidence is complete and the owner records `N2 PASS`.

## N2 candidates

### Qt PDF

Candidate for:

- document/page metadata;
- page labels/geometry;
- raster rendering;
- Unicode text extraction/search;
- links/navigation;
- outlines/destinations.

Atlas is qualifying the engine APIs, not adopting Qt's complete PDF viewer as the future Atlas reader. Atlas owns the viewport in N4.

### PDFium

Candidate for the same read/render/text/navigation responsibilities as Qt PDF.

N2 must account for two important integration facts rather than hiding them:

- current PDFium public API documentation states that its APIs are not thread-safe, so Atlas must serialize calls according to upstream requirements;
- official source integration uses Chromium-style `depot_tools`/`gclient`, GN, Ninja and Clang/clang-cl tooling instead of Atlas's normal CMake/MSVC-only dependency path.

The inspected Microsoft vcpkg registry does not expose a standard `ports/pdfium` package. A pinned community prebuilt may accelerate N2 probing only when exact package/revision, SHA-256 and third-party notices are recorded. That does not select a production supply chain.

### qpdf

Candidate primarily for:

- structural inspection;
- encryption/password/restriction information;
- outline/object-level work required by future bookmark workflows;
- preservation-sensitive transformations and validation.

qpdf is not being evaluated as the page raster engine.

At N2 opening the exact version surfaces differ and must be recorded honestly:

- inspected Microsoft vcpkg qpdf port: `12.4.0`;
- upstream published release feed visible at opening: `12.4.1` dated 2026-08-27;
- current generated documentation may already identify `12.4.2`.

Each experiment records the exact surface it actually uses.

## N2 outcome model

There is no single predetermined winner.

A valid final architecture may be:

```text
Atlas-owned normalized PDF contracts
        |
        +----------------------+----------------------+
        |                      |                      |
        v                      v                      v
read/render/text          navigation/outline     structure/write
Qt PDF or PDFium          Qt PDF or PDFium            qpdf
```

Engine-native types may not become Atlas domain/application types.

The matrix uses per-capability states:

- PASS;
- PASS WITH LIMITATION;
- FAIL;
- BLOCKED;
- N/A;
- PENDING.

N2 deliberately avoids a single weighted score that could make a fast but unsafe/corrupting candidate appear to “win.”

## N2 fixture contract

Tracked fixtures require:

- stable fixture ID;
- public/synthetic provenance;
- redistribution permission/license;
- SHA-256;
- expected behavior;
- mutation permission;
- test-only password policy where applicable.

Required classes include Arabic-only/mixed text, Arabic/English outlines, duplicate/deep outlines, page labels, links/destinations, rotation, mixed page sizes, CropBox/MediaBox, image-only, annotations, XMP/metadata, encryption/restrictions, malformed-but-readable, signed/certified where redistributable, and long timing documents.

Private owner PDFs may supplement local testing but their bytes, personal paths, titles, extracted text and passwords do not enter public Git history or ordinary CI logs.

## Measurement rules

For comparable hot-path evidence:

- Release builds;
- same fixture/page/request;
- warm/cold context declared;
- median and p95 rather than best-case only;
- enough repetitions to reduce noise;
- output correctness/fidelity checked before speed is accepted;
- memory/package/build-tool costs recorded where material.

A faster wrong render/search result loses.

## N2 stop gate

N2 cannot be Accepted until:

- Qt PDF and PDFium have comparable evidence for required read/render responsibilities, or one is explicitly rejected/blocked with evidence;
- qpdf has structural/security/preservation evidence;
- Arabic/Unicode, labels, outlines/destinations, boxes/rotation, links, encryption and malformed cases are represented;
- exact dependency versions/acquisition/license implications are recorded;
- Atlas-owned adapter boundaries remain intact;
- ADR-0004 assigns final responsibilities and rollback/replaceability rules;
- strict Debug + Release final CI passes;
- owner explicitly records `N2 PASS`.

N3 remains closed until then.

## Inherited canonical Windows toolchain

N2 begins from the accepted N1 toolchain rather than silently changing it:

- C++23;
- Visual Studio 17 2022 x64 generator;
- MSVC v143;
- public reproducible Qt 6.10.3 MSVC 2022 64-bit baseline;
- CMake >= 3.28;
- Debug + Release CTest;
- `/W4 /permissive- /Zc:__cplusplus`; CI adds `/WX`.

Candidate-specific tools are allowed only when the candidate requires them and their exact role/provenance is documented.

## Chosen long-term foundation

- C++23 shared core;
- Qt 6 + Qt Quick/QML presentation;
- CMake + CTest;
- SQLite + FTS5 planned for index/search beginning N3;
- replaceable Atlas-owned PDF contracts;
- Qt PDF/PDFium read/render qualification in N2;
- qpdf structural/security/transformation qualification in N2;
- vcpkg manifest mode accepted for qualified non-Qt dependencies from N2;
- thin OS adapters;
- GitHub Actions + CodeGraph/GitHub connector developer automation.

## Product priority

1. Library / Index
2. Reader
3. Bookmarks / Outlines
4. Writing / standard annotations
5. Printing / covers / portable metadata / migration / backup

The research-data rule remains:

**Portable when possible, local when necessary, never lost silently.**

## Release program

- N0: `2.0.0-alpha.0` — **Accepted** bootstrap
- N1: `2.0.0-alpha.1` — **Accepted** Windows/toolchain baseline
- N2: `2.0.0-alpha.2` — **In progress** PDF-engine qualification
- N3: first useful native `2.0.0-beta.1` — Not started
- N4–N9: incremental `2.0.0-beta.N` milestones
- N10: `2.0.0-rc.N`
- N11: Windows `2.0.0`

## Not implemented as product features

N2 probes do not mean these product features are implemented:

- production PDF Reader;
- SQLite database/schema;
- scanner/watcher;
- production Library UI;
- production bookmarks/local overlay;
- annotations/ink;
- migration/backup implementation;
- installer.

## Source-of-truth hierarchy

- Running code/tests define Implemented behavior.
- `CHECKPOINTS.md` defines execution/status/release gates.
- `PLAN.md` defines the master north star.
- `docs/FEATURE_SCOPE_2_0.md` defines Windows 2.0 scope.
- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` defines current N2 execution details.
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md` defines current N2 evidence.
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` records the Proposed/accepted PDF responsibility decision.
- `docs/CORE_WORKFLOWS.md` defines Index/Reader/Bookmark capability/failure/recovery behavior.
- `docs/ARCHITECTURE.md` defines layer/thread/platform boundaries.
- `docs/TECH_STACK.md`, `docs/DEPENDENCIES_AND_TOOLS.md`, and `docs/UPSTREAM_CATALOG.md` define technology/dependency/tool policy.
- `docs/QUALITY_AND_TESTING.md` and `docs/PERFORMANCE.md` define evidence/benchmarks.
- `docs/SECURITY_MODEL.md` defines untrusted-input/privacy/security behavior.
- `docs/UX_ACCESSIBILITY_AND_DESIGN.md` defines UI/a11y/RTL rules.
- `docs/RELEASE_STRATEGY.md` defines alpha/beta/RC/stable semantics.
- `CHANGELOG.md` records repository changes.

No document may claim an unimplemented requirement is shipped.
