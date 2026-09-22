# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — evidence pending**  
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Opened:** 2026-09-22

This is the durable comparison sheet for N2. It must be updated when evidence is produced. Empty cells are intentionally `PENDING`; absence of evidence must never be converted into a guessed result.

## Result vocabulary

- **PASS** — requirement satisfied by measured/fixture evidence.
- **PASS WITH LIMITATION** — usable with a named bounded limitation/workaround.
- **FAIL** — unsuitable for that responsibility.
- **BLOCKED** — evidence cannot currently be produced.
- **N/A** — responsibility is outside the engine's intended role.
- **PENDING** — not tested yet.

## Candidate identity

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended N2 role | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version/revision tested | PENDING | PENDING | PENDING |
| Acquisition path | Qt toolchain/module | PENDING | PENDING |
| Compiler/build path | CMake + pinned Qt | PENDING | PENDING |
| Package/archive SHA-256 | N/A/PENDING | PENDING | PENDING |
| Primary upstream license | LGPLv3/GPLv2/commercial module terms | BSD-style + third-party notices | Apache-2.0/MIT upstream/port terms to verify per exact version |
| Production distribution decision | PENDING | PENDING | PENDING |

## Upstream/integration facts recorded at opening

### Qt PDF

- Current Qt documentation exposes page rendering, text access, search model, bookmark model, link model and navigation helpers.
- It integrates through normal Qt CMake targets.
- Atlas will qualify the engine APIs, not adopt the complete Qt viewer as the reader architecture.

### PDFium

- Public embedder APIs cover document/page loading, bitmap rendering, text, bookmarks/destinations/links and related functionality.
- Current upstream public header states PDFium APIs are not thread-safe and embedders must serialize calls.
- Official source build uses Chromium-style depot_tools/gclient + GN/Ninja + Clang rather than Atlas's normal CMake/MSVC-only path.
- Microsoft vcpkg does not currently expose a standard `ports/pdfium` package in the inspected registry tree.
- Community `bblanchon/pdfium-binaries` may be used only as a pinned N2 bootstrap until supply-chain suitability is decided.

### qpdf

- Latest published release visible at N2 opening: 12.4.1 (2026-08-27).
- Current vcpkg curated qpdf port inspected at opening: 12.4.0.
- Current latest documentation may already identify 12.4.2; this discrepancy must not be hidden when recording exact experiment versions.

## Read/open and geometry

| Capability | Qt PDF | PDFium | Notes / fixture IDs |
|---|---|---|---|
| Valid document open | PENDING | PENDING | |
| Invalid/malformed failure typing | PENDING | PENDING | |
| Password-required detection | PENDING | PENDING | |
| Incorrect-password distinction | PENDING | PENDING | |
| Unsupported-security distinction | PENDING | PENDING | |
| Page count | PENDING | PENDING | |
| Page labels | PENDING | PENDING | |
| Media box | PENDING | PENDING | |
| Crop box | PENDING | PENDING | |
| Rotation | PENDING | PENDING | |
| Mixed page sizes | PENDING | PENDING | |
| Image-only page handling | PENDING | PENDING | |

## Rendering

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| 100% nominal raster fidelity | PENDING | PENDING | |
| High-DPI raster fidelity | PENDING | PENDING | |
| Rotation correctness | PENDING | PENDING | |
| Crop-box correctness | PENDING | PENDING | |
| Annotation rendering behavior | PENDING | PENDING | |
| Transparent/background behavior | PENDING | PENDING | |
| Warm render p50 | PENDING | PENDING | ms; same fixture/page/size |
| Warm render p95 | PENDING | PENDING | ms; same fixture/page/size |
| First render after open | PENDING | PENDING | ms |
| Repeated-render memory behavior | PENDING | PENDING | |

## Text extraction and search

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| English Unicode extraction | PENDING | PENDING | |
| Arabic Unicode extraction | PENDING | PENDING | |
| Mixed Arabic/English extraction | PENDING | PENDING | |
| Diacritics/combining marks | PENDING | PENDING | |
| Urdu text | PENDING | PENDING | |
| Extraction bounds/geometry | PENDING | PENDING | |
| English search | PENDING | PENDING | |
| Arabic search | PENDING | PENDING | |
| Mixed-script search | PENDING | PENDING | |
| Hit page/index identity | PENDING | PENDING | |
| Hit geometry/destination | PENDING | PENDING | |
| First extraction time | PENDING | PENDING | ms |
| Repeated extraction time | PENDING | PENDING | ms |
| Search completion time | PENDING | PENDING | ms |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| Internal link detection | PENDING | PENDING | |
| External URI link detection | PENDING | PENDING | |
| Named/explicit destination handling | PENDING | PENDING | |
| Outline hierarchy | PENDING | PENDING | |
| Duplicate outline titles | PENDING | PENDING | |
| Deep outline hierarchy | PENDING | PENDING | |
| Arabic/English outline text | PENDING | PENDING | |
| Destination page normalization | PENDING | PENDING | |
| Destination coordinate normalization | PENDING | PENDING | |

## PDFium concurrency/build-specific evidence

| Check | Result | Evidence |
|---|---|---|
| Public API non-thread-safe constraint acknowledged in adapter design | PENDING | |
| Serialized-call correctness | PENDING | |
| Parallel Atlas tasks do not violate PDFium API contract | PENDING | |
| Acquisition pinned by exact revision/tag | PENDING | |
| Binary/archive checksum pinned | PENDING | |
| Clean CI acquisition reproducible | PENDING | |
| Runtime binary/dependency footprint | PENDING | |
| Upgrade/rollback procedure documented | PENDING | |

## qpdf structural/security evidence

| Capability | qpdf result | Notes / fixture IDs |
|---|---|---|
| Valid document structural open | PENDING | |
| Malformed-but-readable diagnostics | PENDING | |
| Encryption algorithm/revision inspection | PENDING | |
| User/owner password state | PENDING | |
| Permission restriction inspection | PENDING | |
| Page tree/basic geometry access | PENDING | |
| Outline read hierarchy/order | PENDING | |
| Outline destination read | PENDING | |
| Signature/certification visibility needed by Atlas | PENDING | exact available information to document |
| No-op/rewrite reopen check | PENDING | copied fixture only |
| No-op/rewrite unrelated structure preservation | PENDING | |
| Controlled outline transformation | PENDING | synthetic/redistributable fixture only |
| Output `qpdf --check`/library validation | PENDING | |
| Independent-reader reopen | PENDING | |
| Exact qpdf version recorded | PENDING | |
| Exact vcpkg baseline/override recorded | PENDING | |

## Preservation invariants for mutation probes

| Invariant | Expected | qpdf evidence |
|---|---|---|
| Page count | unchanged unless operation explicitly changes pages | PENDING |
| Page dimensions/rotation | unchanged | PENDING |
| Page content streams | unchanged when outline-only operation is under test | PENDING |
| Existing annotations | unchanged | PENDING |
| Existing outline nodes not targeted | unchanged | PENDING |
| Document Info | preserved unless explicitly edited | PENDING |
| XMP presence/content | preserved unless explicitly edited | PENDING |
| Encryption policy | preserved unless explicitly changed | PENDING |
| Signatures/certification consequence | detected and never silently represented as preserved integrity | PENDING |
| Attachments/other unrelated structures | preserved when writer touches document | PENDING |

## Build and maintenance comparison

| Criterion | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Fits existing CMake flow | PENDING | PENDING | PENDING |
| Fits existing MSVC toolchain | PENDING | PENDING | PENDING |
| Extra toolchain required | PENDING | PENDING | PENDING |
| CI setup cost | PENDING | PENDING | PENDING |
| Package size delta | PENDING | PENDING | PENDING |
| Runtime dependency delta | PENDING | PENDING | PENDING |
| Cross-platform path | PENDING | PENDING | PENDING |
| License/notices complexity | PENDING | PENDING | PENDING |
| Reproducible pinning | PENDING | PENDING | PENDING |
| Rollback/replaceability | PENDING | PENDING | PENDING |

## Hard blockers

A candidate/responsibility is not accepted if evidence shows any of the following without an acceptable contained workaround:

- unrelated PDF content can be silently lost/corrupted by the required operation;
- required Arabic/Unicode semantics are not dependable for the responsibility;
- the library/distribution route cannot legally/reproducibly ship with Atlas;
- concurrency requirements force an unsafe architecture;
- encrypted/restricted/signed states cannot be distinguished safely enough for later Atlas workflows;
- engine-specific types leak through the portable Atlas application/domain boundary;
- severe reproducibility/update risk cannot be pinned and rolled back.

## Final responsibility decision

| Responsibility | Selected implementation | Status | Evidence |
|---|---|---|---|
| Document open/read metadata | PENDING | PENDING | |
| Page geometry/labels | PENDING | PENDING | |
| Page raster rendering | PENDING | PENDING | |
| Text extraction | PENDING | PENDING | |
| Search | PENDING | PENDING | |
| Links/navigation | PENDING | PENDING | |
| Outline read | PENDING | PENDING | |
| Security/capability inspection | PENDING | PENDING | |
| Structural transformation/write | PENDING | PENDING | |
| Independent output validation | PENDING | PENDING | |

The final entries above must match accepted ADR-0004. No decision is implied by the initial ordering of candidates.
