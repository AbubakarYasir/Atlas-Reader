# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — N2.1/N2.2 core smoke captured; broader qualification pending**  
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

## N2.2 PDFium core evidence baseline — 2026-09-22

The first passing PDFium core evidence is bound to **implementation head SHA** `7874794e14b9cea54ec0723c15963621f65bebf6`.

- GitHub Actions run: `35680738681` — Debug and Release both PASS.
- PDFium under test: **156.0.8066.0**, release tag `chromium/8066`, non-V8 Windows x64 package from `bblanchon/pdfium-binaries`.
- Distribution source commit: `f2e9a1c45bb17b85b540abf1af30146ef65416ac`.
- Downloaded asset: `pdfium-win-x64.tgz`, asset ID `579031518`, 3,823,498 bytes.
- Archive SHA-256 verified in both CI lanes **before extraction**: `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`.
- Release engineering artifact ID: `10674712819`; digest `sha256:0972d8424b6d35c8f2c8c1612ddbea60b34f7038b59c5d8c5643ab37bfad4803`.
- This PR-triggered run used GitHub's synthetic merge checkout SHA `df4f45e1a301508b81a4389a90b2d00d156730b6` in the artifact name/build-info while the workflow run's head SHA is `7874794e14b9cea54ec0723c15963621f65bebf6`. That is a CI identity-labeling issue, not an engine result. It must be corrected before later evidence artifacts are treated as canonical so future artifacts record implementation head and checkout SHA separately.
- `atlas_pdfium_probe` is isolated from `atlas_reader` and `atlas_core`. All PDFium calls in this probe are serialized on one thread, honoring the upstream non-thread-safe API contract.
- A001–A005 passed strict PDFium CTests and Release evidence runs. The same independently validated fixture bytes were used for Qt PDF and PDFium.
- Cross-engine text semantics are identical on the current English fixture corpus: **every page of A001–A005 has the same UTF-8 text SHA-256 from Qt PDF and PDFium**, with matching extracted text lengths.
- Cross-engine normalized visible page geometry and page labels also match exactly on A001–A005. For A002 both report `612×792`, rotated `841.8897705×595.2755737`, and cropped `360×560` visible page sizes, with labels `i,ii,1`.
- A001 rendered non-null at 612×792 through both engines. The raw pixel hashes differ, so raster fidelity/equivalence is **not** inferred from the smoke test and remains PENDING.

Single-run PDFium CI smoke timings are **not** performance rankings: open time ranged about **0.078–0.112 ms** across A001–A005; first-page text extraction about **0.044–0.070 ms**; A001 raster render about **0.440 ms**. The corresponding Qt PDF sample in the same artifact was slower, but repeated same-process/cold-warm benchmark distributions are required before any performance conclusion.

Coverage boundary: the current PDFium probe calls document load, page count, page labels, normalized page width/height, text-page extraction, and one bitmap render. It does **not yet** exercise bookmarks, links/destinations, search, raw page-box/rotation decomposition, password/security paths, malformed-input typing beyond the mapped error enum, annotation behavior, or repeated performance. Those rows remain PENDING.

Detailed package pin and production caveats are recorded in `docs/baselines/N2_PDFIUM_PROVENANCE.md`.

## Candidate identity

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended N2 role | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version/revision tested | **6.10.3** | **156.0.8066.0 / `chromium/8066`** | PENDING |
| Acquisition path | `jurplel/install-qt-action@v4`, Qt desktop MSVC 2022 x64 + `qtpdf` | **PASS WITH LIMITATION** — pinned `bblanchon/pdfium-binaries` non-V8 Windows x64 package for N2 probe only | PENDING |
| Compiler/build path | **PASS** — CMake 3.31.6 + MSVC 19.44.35228 on `windows-2022` | **PASS WITH LIMITATION** — published `PDFiumConfig.cmake` imported target + MSVC 2022; production source-build route unresolved | PENDING |
| Package/archive SHA-256 | PENDING — Qt package archive hash not separately captured | **PASS** — `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` | PENDING |
| Primary upstream license | LGPLv3/GPLv2/commercial module terms | BSD-style/third-party engine notices; distributor repository MIT; exact production package notice audit PENDING | Apache-2.0/MIT upstream/port terms to verify per exact version |
| Production distribution decision | PENDING | PENDING — community binary is probe-only | PENDING |

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
- Community `bblanchon/pdfium-binaries` is used only as a pinned N2 bootstrap until supply-chain suitability is decided.

### qpdf

- Latest published release visible at N2 opening: 12.4.1 (2026-08-27).
- Current vcpkg curated qpdf port inspected at opening: 12.4.0.
- Current latest documentation may already identify 12.4.2; this discrepancy must not be hidden when recording exact experiment versions.

## Read/open and geometry

| Capability | Qt PDF | PDFium | Notes / fixture IDs |
|---|---|---|---|
| Valid document open | **PASS** | **PASS** | A001–A005; strict CI |
| Invalid/malformed failure typing | PENDING | PENDING | Both probes map engine error enums; malformed fixture not yet exercised |
| Password-required detection | PENDING | PENDING | |
| Incorrect-password distinction | PENDING | PENDING | |
| Unsupported-security distinction | PENDING | PENDING | |
| Page count | **PASS** | **PASS** | A001–A005 agree |
| Page labels | **PASS** | **PASS** | A001 `1`; A002 `i,ii,1`; A003–A005 `1,2,3`; engines agree |
| Media box | PENDING | PENDING | Current probes record normalized visible page size, not raw boxes |
| Crop box | PENDING | PENDING | A002 visible crop result observed; raw crop box not read |
| Rotation | PENDING | PENDING | A002 normalized rotated dimensions agree; rotation value itself not read |
| Mixed page sizes | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A002 normalized visible sizes match exactly; raw media/crop/rotation decomposition pending |
| Image-only page handling | PENDING | PENDING | |

## Rendering

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| 100% nominal raster fidelity | PENDING | PENDING | A001 non-null render smoke passes in both; pixel hashes differ, so no fidelity/equivalence claim yet |
| High-DPI raster fidelity | PENDING | PENDING | |
| Rotation correctness | PENDING | PENDING | |
| Crop-box correctness | PENDING | PENDING | |
| Annotation rendering behavior | PENDING | PENDING | |
| Transparent/background behavior | PENDING | PENDING | |
| Warm render p50 | PENDING | PENDING | ms; same fixture/page/size |
| Warm render p95 | PENDING | PENDING | ms; same fixture/page/size |
| First render after open | PENDING | PENDING | single A001 smoke: Qt ≈0.911 ms, PDFium ≈0.440 ms in run `35680738681`; not benchmark evidence |
| Repeated-render memory behavior | PENDING | PENDING | |

## Text extraction and search

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| English Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A001–A005 current English text matches page-for-page by exact UTF-8 SHA-256 across both engines; broader Unicode corpus pending |
| Arabic Unicode extraction | PENDING | PENDING | |
| Mixed Arabic/English extraction | PENDING | PENDING | |
| Diacritics/combining marks | PENDING | PENDING | |
| Urdu text | PENDING | PENDING | |
| Extraction bounds/geometry | PENDING | PENDING | Current probes record text only, not glyph/selection bounds |
| English search | PENDING | PENDING | Search APIs not yet exercised |
| Arabic search | PENDING | PENDING | |
| Mixed-script search | PENDING | PENDING | |
| Hit page/index identity | PENDING | PENDING | |
| Hit geometry/destination | PENDING | PENDING | |
| First extraction time | PENDING | PENDING | one-shot same-artifact smoke: Qt ≈2.09–2.88 ms first pages, PDFium ≈0.044–0.070 ms; repeatable benchmark pending |
| Repeated extraction time | PENDING | PENDING | later-page smoke exists but is not repeated benchmark evidence |
| Search completion time | PENDING | PENDING | ms |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| Internal link detection | PENDING | PENDING | A003/A005 independently contain links, but candidate link APIs not yet called |
| External URI link detection | PENDING | PENDING | A003/A005 URI independently verified by pypdf; candidate link APIs not yet called |
| Named/explicit destination handling | PENDING | PENDING | |
| Outline hierarchy | PENDING | PENDING | A003/A004 outline hierarchy independently verified; candidate bookmark APIs not yet called |
| Duplicate outline titles | PENDING | PENDING | |
| Deep outline hierarchy | PENDING | PENDING | |
| Arabic/English outline text | PENDING | PENDING | |
| Destination page normalization | PENDING | PENDING | |
| Destination coordinate normalization | PENDING | PENDING | |

## PDFium concurrency/build-specific evidence

| Check | Result | Evidence |
|---|---|---|
| Public API non-thread-safe constraint acknowledged in adapter design | **PASS** | `N2_PDFIUM_PROVENANCE.md`, probe records `serialized-single-thread`; no unsupported parallel calls |
| Serialized-call correctness | **PASS WITH LIMITATION** | A001–A005 pass through single-thread serialized probe; future Atlas task-queue/adapter stress still pending |
| Parallel Atlas tasks do not violate PDFium API contract | PENDING | No production adapter/task queue exists in N2 yet |
| Acquisition pinned by exact revision/tag | **PASS** | `chromium/8066`, distribution commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac` |
| Binary/archive checksum pinned | **PASS** | `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`, verified before extraction |
| Clean CI acquisition reproducible | **PASS** | Debug + Release run `35680738681` independently acquired/verified/configured package |
| Runtime binary/dependency footprint | PENDING | Probe artifact records `pdfium.dll` = 7,380,992 bytes; clean production package delta/transitive footprint not yet measured |
| Upgrade/rollback procedure documented | **PASS WITH LIMITATION** | Exact tag/hash pinned and probe is removable; production update policy/source-build route remains undecided |

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
| Fits existing CMake flow | **PASS** — focused `Qt6::Pdf` target integrates directly | **PASS WITH LIMITATION** — pinned probe package exposes `PDFiumConfig.cmake`/`pdfium`; official source build is a separate Chromium-style path | PENDING |
| Fits existing MSVC toolchain | **PASS** — MSVC 2022 x64 CI | **PASS** for pinned Windows probe package + MSVC 2022 import library | PENDING |
| Extra toolchain required | **PASS** — no extra compiler/build system beyond installing the Qt `qtpdf` module | **PASS WITH LIMITATION** — no extra compiler for pinned binary probe; official production source build would require depot_tools/GN/Ninja/Clang | PENDING |
| CI setup cost | **PASS WITH LIMITATION** — one extra Qt module plus deployment/evidence staging | **PASS WITH LIMITATION** — tagged archive download + SHA verification + extraction + DLL staging; source-build cost not measured | PENDING |
| Package size delta | PENDING | PENDING | PENDING |
| Runtime dependency delta | PENDING | PENDING — current probe `pdfium.dll` is 7,380,992 bytes; full product delta not measured | Current engineering artifact duplicates probe/runtime files, so it is not a clean product-size delta measurement |
| Cross-platform path | PENDING | PENDING | PENDING |
| License/notices complexity | PENDING | PENDING — distributor repo is MIT but PDFium/third-party production notices still require audit | PENDING |
| Reproducible pinning | **PASS WITH LIMITATION** — Qt 6.10.3 pinned and CI reproducible; underlying Qt archive SHA not separately recorded | **PASS** for N2 probe — immutable tag + source commit + asset ID + SHA-256 recorded | PENDING |
| Rollback/replaceability | **PASS** — probe isolated; `atlas_reader` product shell does not link Qt PDF | **PASS** — probe isolated behind build option; `atlas_reader`/`atlas_core` do not link PDFium | PENDING |

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
