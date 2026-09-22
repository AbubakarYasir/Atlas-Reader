# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — N2.1 Qt PDF core smoke captured; broader qualification pending**  
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

## N2.1 Qt PDF evidence baseline — 2026-09-22

The first clean Qt PDF evidence baseline is bound to the **tested implementation SHA** `213050ee75882ae5fa53f73b07fe2707bbaea5a8`.

- GitHub Actions run: `35679099222` — Debug and Release both PASS.
- Windows engineering artifact: `atlas-reader-n2-windows-x64-213050ee75882ae5fa53f73b07fe2707bbaea5a8`.
- Artifact ID: `10674630141`.
- Artifact digest: `sha256:6490e039589984d3190f3e6ac0f0269b9b732f21da581472822d90a5fb51187b`.
- Qt PDF under test: **Qt 6.10.3**, `qtpdf`, MSVC 2022 x64.
- Atlas product shell still does **not** link Qt PDF; the candidate remains isolated in `atlas_qt_pdf_probe`.
- Independent fixture validator: **pypdf 5.9.0**. All five A001–A005 files matched the manifest SHA-256 values and independently passed expected page/text/outline/link checks applicable to each fixture.
- Repository integrity finding: without `.gitattributes`, Windows checkout rewrote LF bytes in ASCII-heavy PDF files, changing all fixture SHA-256 values and invalidating `startxref` offsets. `*.pdf -text` was added at the tested SHA, after which all manifest hashes passed exactly. The earlier A003 empty-text symptom is therefore **not** retained as a Qt PDF defect.
- Qt PDF A001–A005 strict CTest expectations all passed. A003 no longer has any `WILL_FAIL`/expected-failure waiver.

Single-run CI smoke timings are evidence of successful execution, **not** performance benchmarks: open time ranged from about **0.585–0.853 ms** across A001–A005; first-page text extraction was about **2.05–2.12 ms**; A001 rendered a non-null 612×792 image in **0.895 ms** with pixel SHA-256 `ed2ce000dc1bc40cb8d1b2b340d268b638eb961d5aa81b95394e938aefa2d434`. Repeated warm p50/p95 measurements remain PENDING.

Coverage boundary: the current Qt probe calls `QPdfDocument::load`, `pageCount`, `pageLabel`, `pagePointSize`, `getAllText`, and one raster `render`. It does **not yet** call Qt bookmark, link, destination, search-model, password-flow, malformed-input, annotation, or repeated-performance APIs. Those rows remain PENDING even where the fixture itself contains the relevant structure.

## Candidate identity

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended N2 role | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version/revision tested | **6.10.3** | PENDING | PENDING |
| Acquisition path | `jurplel/install-qt-action@v4`, Qt desktop MSVC 2022 x64 + `qtpdf` | PENDING | PENDING |
| Compiler/build path | **PASS** — CMake 3.31.6 + MSVC 19.44.35228 on `windows-2022` | PENDING | PENDING |
| Package/archive SHA-256 | PENDING — Qt package archive hash not separately captured | PENDING | PENDING |
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
| Valid document open | **PASS** | PENDING | A001–A005; strict CI |
| Invalid/malformed failure typing | PENDING | PENDING | Qt error enum is mapped by probe, malformed fixture not yet exercised |
| Password-required detection | PENDING | PENDING | |
| Incorrect-password distinction | PENDING | PENDING | |
| Unsupported-security distinction | PENDING | PENDING | |
| Page count | **PASS** | PENDING | A001–A005 |
| Page labels | **PASS** | PENDING | A001 `1`; A002 `i,ii,1`; A003–A005 `1,2,3` |
| Media box | PENDING | PENDING | Current probe records normalized `pagePointSize`, not raw boxes |
| Crop box | PENDING | PENDING | A002 visible size changes are observed but raw crop box is not exposed by current probe |
| Rotation | PENDING | PENDING | A002 rotated page reports normalized dimensions; rotation value itself not read |
| Mixed page sizes | **PASS WITH LIMITATION** | PENDING | A002 yields distinct normalized visible sizes; raw media/crop/rotation decomposition pending |
| Image-only page handling | PENDING | PENDING | |

## Rendering

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| 100% nominal raster fidelity | PENDING | PENDING | A001 render smoke PASS/non-null at 612×792, but no independent pixel/reference fidelity comparison yet |
| High-DPI raster fidelity | PENDING | PENDING | |
| Rotation correctness | PENDING | PENDING | |
| Crop-box correctness | PENDING | PENDING | |
| Annotation rendering behavior | PENDING | PENDING | |
| Transparent/background behavior | PENDING | PENDING | |
| Warm render p50 | PENDING | PENDING | ms; same fixture/page/size |
| Warm render p95 | PENDING | PENDING | ms; same fixture/page/size |
| First render after open | PENDING | PENDING | single A001 smoke = 0.895 ms; not a benchmark |
| Repeated-render memory behavior | PENDING | PENDING | |

## Text extraction and search

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| English Unicode extraction | **PASS WITH LIMITATION** | PENDING | A001–A005 English/ASCII text extracted correctly; broader Latin/Unicode corpus still pending |
| Arabic Unicode extraction | PENDING | PENDING | |
| Mixed Arabic/English extraction | PENDING | PENDING | |
| Diacritics/combining marks | PENDING | PENDING | |
| Urdu text | PENDING | PENDING | |
| Extraction bounds/geometry | PENDING | PENDING | Current probe records text only, not glyph/selection bounds |
| English search | PENDING | PENDING | `QPdfSearchModel` not yet exercised |
| Arabic search | PENDING | PENDING | |
| Mixed-script search | PENDING | PENDING | |
| Hit page/index identity | PENDING | PENDING | |
| Hit geometry/destination | PENDING | PENDING | |
| First extraction time | PENDING | PENDING | single first-page smoke ≈2.05–2.12 ms; benchmark pending |
| Repeated extraction time | PENDING | PENDING | later-page smoke ≈0.04–0.08 ms; not repeated benchmark evidence |
| Search completion time | PENDING | PENDING | ms |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| Internal link detection | PENDING | PENDING | A003/A005 independently contain links, but Qt link API not yet called |
| External URI link detection | PENDING | PENDING | A003/A005 URI independently verified by pypdf; Qt link API not yet called |
| Named/explicit destination handling | PENDING | PENDING | |
| Outline hierarchy | PENDING | PENDING | A003/A004 outline hierarchy independently verified; Qt bookmark API not yet called |
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
| Fits existing CMake flow | **PASS** — focused `Qt6::Pdf` target integrates directly | PENDING | PENDING |
| Fits existing MSVC toolchain | **PASS** — MSVC 2022 x64 CI | PENDING | PENDING |
| Extra toolchain required | **PASS** — no extra compiler/build system beyond installing the Qt `qtpdf` module | PENDING | PENDING |
| CI setup cost | **PASS WITH LIMITATION** — one extra Qt module plus deployment/evidence staging | PENDING | PENDING |
| Package size delta | PENDING | PENDING | PENDING |
| Runtime dependency delta | PENDING | PENDING | Current engineering artifact duplicates probe/runtime files, so it is not a clean product-size delta measurement |
| Cross-platform path | PENDING | PENDING | PENDING |
| License/notices complexity | PENDING | PENDING | PENDING |
| Reproducible pinning | **PASS WITH LIMITATION** — Qt 6.10.3 pinned and CI reproducible; underlying Qt archive SHA not separately recorded | PENDING | PENDING |
| Rollback/replaceability | **PASS** — probe isolated; `atlas_reader` product shell does not link Qt PDF | PENDING | PENDING |

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
