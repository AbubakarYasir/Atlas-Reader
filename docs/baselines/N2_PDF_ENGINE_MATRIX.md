# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — Qt PDF/PDFium core + navigation evidence captured; Unicode/search/security/performance/qpdf pending**  
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

## N2.1 Qt PDF core evidence baseline — 2026-09-22

The first clean Qt PDF evidence baseline is bound to tested implementation SHA `213050ee75882ae5fa53f73b07fe2707bbaea5a8`.

- GitHub Actions run `35679099222` — Debug and Release PASS.
- Artifact ID `10674630141`.
- Artifact digest `sha256:6490e039589984d3190f3e6ac0f0269b9b732f21da581472822d90a5fb51187b`.
- Qt PDF: 6.10.3 via `qtpdf`, MSVC 2022 x64.
- `atlas_reader` and `atlas_core` do not link Qt PDF; only the focused probe does.
- Independent pypdf 5.9.0 fixture validation passes A001–A005.
- Windows checkout originally rewrote ASCII-heavy PDF fixtures through line-ending conversion. Repository-level `*.pdf -text` fixed the corruption. The earlier A003 empty-text symptom is therefore not retained as a Qt PDF defect.
- A001–A005 strict Qt PDF CTests pass.

Single-run timings are smoke evidence only, not benchmarks. Repeated p50/p95 work remains PENDING.

## N2.2 PDFium core evidence baseline — 2026-09-22

The first passing PDFium core evidence is bound to implementation head `7874794e14b9cea54ec0723c15963621f65bebf6`.

- GitHub Actions run `35680738681` — Debug and Release PASS.
- PDFium 156.0.8066.0 / `chromium/8066`.
- Probe distribution: `bblanchon/pdfium-binaries`, source commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac`.
- Asset `pdfium-win-x64.tgz`, ID `579031518`, 3,823,498 bytes.
- Archive SHA-256 `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`, verified before extraction in both CI lanes.
- Artifact ID `10674712819`; digest `sha256:0972d8424b6d35c8f2c8c1612ddbea60b34f7038b59c5d8c5643ab37bfad4803`.
- `atlas_pdfium_probe` is isolated from `atlas_reader`/`atlas_core`.
- All PDFium calls in the probe are serialized on one thread, honoring the upstream non-thread-safe API contract.
- A001–A005 strict PDFium tests pass.
- On A001–A005, Qt PDF and PDFium produce matching page counts, page labels, normalized visible page sizes, and exact page-by-page UTF-8 text SHA-256 values.
- A001 renders non-null through both engines, but raw pixel hashes differ; raster fidelity/equivalence is not inferred.

Detailed acquisition/licensing caveats are in `N2_PDFIUM_PROVENANCE.md`.

## N2 navigation evidence baseline — 2026-09-22

Canonical navigation evidence is bound to implementation head:

`59073e4bd341bd4f5feb86097a8f700a3603be4e`

- GitHub Actions run `35682017265` — Debug and Release PASS.
- PR checkout SHA `f6a68fe10f3268e91ddca292d5f357420569d197` is recorded separately from the implementation head.
- Artifact `atlas-reader-n2-windows-x64-59073e4bd341bd4f5feb86097a8f700a3603be4e`.
- Artifact ID `10675244583`.
- Artifact digest `sha256:280d660a99c44de72eecb65e6b6fcdd530517a6b64fe09e3ec81f0f896e8d3a3`.
- Fixture validation, configure, build, CTest, evidence staging and artifact upload all pass in the canonical run.

### Outline semantics

A003 and A004 produce the same normalized outline semantics in both engines:

| Flattened item | Depth | Destination page |
|---|---:|---:|
| `Chapter 1` | 0 | 0 |
| `Section 1.1` | 1 | 1 |
| `Chapter 2` | 0 | 2 |

This establishes the tested hierarchy/title/order/page-destination path for the current synthetic corpus.

### Link semantics and raw-engine difference

A003 and A005 establish the same required semantic targets in both engines:

- internal link destination: zero-based page `2`;
- external URI: `https://example.com/atlas-n2-fixture`.

Raw row cardinality is deliberately preserved rather than normalized inside the candidate probes:

- PDFium reports the two explicit PDF link annotations: raw count `2`;
- Qt `QPdfLinkModel` reports raw count `3`: the internal destination plus **two rows for the same external URI with different rectangles**.

Therefore Qt external-URI detection is **PASS WITH LIMITATION** at the raw model layer: the URI is correct, but raw link rows require Atlas-side semantic normalization/de-duplication before they can become the portable application contract. PDFium reports the expected two raw annotations for this fixture.

The exploratory run `35681776254` first exposed this cardinality difference and is diagnostic only. Its failure was an overly strict cross-engine raw-row-count assumption; it is not the canonical navigation result.

Destination **page** normalization is now proven on the current fixture. Destination-coordinate normalization remains PENDING because the engines expose coordinate systems/fields differently and no Atlas-normalized coordinate contract has yet been tested.

## Candidate identity

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended N2 role | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version/revision tested | **6.10.3** | **156.0.8066.0 / `chromium/8066`** | PENDING |
| Acquisition path | Qt desktop MSVC 2022 x64 + `qtpdf` | **PASS WITH LIMITATION** — pinned community non-V8 Windows x64 package, probe-only | PENDING |
| Compiler/build path | **PASS** — CMake + MSVC 2022 | **PASS WITH LIMITATION** — published `PDFiumConfig.cmake` + MSVC; official source-build route unresolved | PENDING |
| Package/archive SHA-256 | PENDING — Qt archive hash not separately captured | **PASS** — `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` | PENDING |
| Primary license surface | Qt module terms require release review | PDFium/Chromium third-party notices; distributor repo MIT; production audit PENDING | Apache-2.0/MIT port terms to verify on exact pin |
| Production distribution decision | PENDING | PENDING — community binary remains probe-only | PENDING |

## Read/open and geometry

| Capability | Qt PDF | PDFium | Notes / fixture IDs |
|---|---|---|---|
| Valid document open | **PASS** | **PASS** | A001–A005 |
| Invalid/malformed failure typing | PENDING | PENDING | Error enums mapped; malformed fixtures not yet exercised |
| Password-required detection | PENDING | PENDING | |
| Incorrect-password distinction | PENDING | PENDING | |
| Unsupported-security distinction | PENDING | PENDING | |
| Page count | **PASS** | **PASS** | A001–A005 agree |
| Page labels | **PASS** | **PASS** | Engines agree on all current fixtures |
| Media box | PENDING | PENDING | Raw boxes not yet read |
| Crop box | PENDING | PENDING | A002 visible crop result observed; raw crop box pending |
| Rotation | PENDING | PENDING | A002 normalized rotated dimensions agree; raw rotation pending |
| Mixed page sizes | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Normalized visible sizes match; raw decomposition pending |
| Image-only page handling | PENDING | PENDING | |

## Rendering

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| 100% nominal raster fidelity | PENDING | PENDING | Non-null smoke passes; pixel hashes differ |
| High-DPI raster fidelity | PENDING | PENDING | |
| Rotation correctness | PENDING | PENDING | |
| Crop-box correctness | PENDING | PENDING | |
| Annotation rendering behavior | PENDING | PENDING | |
| Transparent/background behavior | PENDING | PENDING | |
| Warm render p50 | PENDING | PENDING | |
| Warm render p95 | PENDING | PENDING | |
| First render after open | PENDING | PENDING | Existing values are one-shot smoke only |
| Repeated-render memory behavior | PENDING | PENDING | |

## Text extraction and search

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| English Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A001–A005 page text hashes agree exactly; broader Unicode pending |
| Arabic Unicode extraction | PENDING | PENDING | |
| Mixed Arabic/English extraction | PENDING | PENDING | |
| Diacritics/combining marks | PENDING | PENDING | |
| Urdu text | PENDING | PENDING | |
| Extraction bounds/geometry | PENDING | PENDING | |
| English search | PENDING | PENDING | Search APIs not yet exercised |
| Arabic search | PENDING | PENDING | |
| Mixed-script search | PENDING | PENDING | |
| Hit page/index identity | PENDING | PENDING | |
| Hit geometry/destination | PENDING | PENDING | |
| First extraction time | PENDING | PENDING | Existing values are one-shot smoke only |
| Repeated extraction time | PENDING | PENDING | |
| Search completion time | PENDING | PENDING | |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| Internal link detection | **PASS** | **PASS** | A003/A005; zero-based destination page `2` |
| External URI link detection | **PASS WITH LIMITATION** | **PASS** | Correct URI in both; Qt exposes duplicate raw URI row with distinct rectangle |
| Named/explicit destination handling | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Current synthetic named bookmark/link destinations resolve correctly; broader destination forms not yet covered |
| Outline hierarchy | **PASS** | **PASS** | A003/A004 exact titles, order, depths and destination pages |
| Duplicate outline titles | PENDING | PENDING | |
| Deep outline hierarchy | PENDING | PENDING | Current fixture depth only 1 child level |
| Arabic/English outline text | PENDING | PENDING | |
| Destination page normalization | **PASS** | **PASS** | A003/A005 internal target normalized to zero-based page `2`; outline pages `0,1,2` |
| Destination coordinate normalization | PENDING | PENDING | Raw engine coordinate representations differ; no Atlas normalized assertion yet |

## PDFium concurrency/build-specific evidence

| Check | Result | Evidence |
|---|---|---|
| Public API non-thread-safe constraint acknowledged | **PASS** | Probe records `serialized-single-thread` |
| Serialized-call correctness | **PASS WITH LIMITATION** | Current fixture suite passes; concurrent app task-queue stress pending |
| Parallel Atlas tasks respect contract | PENDING | Production adapter/task queue does not exist yet |
| Acquisition pinned by exact revision/tag | **PASS** | `chromium/8066`, distributor commit recorded |
| Binary/archive checksum pinned | **PASS** | SHA-256 verified before extraction |
| Clean CI acquisition reproducible | **PASS** | Multiple Debug/Release runs acquire and verify exact package |
| Runtime binary/dependency footprint | PENDING | Raw `pdfium.dll` size known; product delta/transitive footprint pending |
| Upgrade/rollback procedure documented | **PASS WITH LIMITATION** | Exact pin/remove path documented; production source/update policy unresolved |

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
| Signature/certification visibility needed by Atlas | PENDING | |
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
| Fits existing CMake flow | **PASS** | **PASS WITH LIMITATION** — probe package integrates; official source build differs | PENDING |
| Fits existing MSVC toolchain | **PASS** | **PASS** for pinned Windows probe package | PENDING |
| Extra toolchain required | **PASS** | **PASS WITH LIMITATION** — no extra compiler for binary probe; official source build needs Chromium tooling | PENDING |
| CI setup cost | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | PENDING |
| Package size delta | PENDING | PENDING | PENDING |
| Runtime dependency delta | PENDING | PENDING | PENDING |
| Cross-platform path | PENDING | PENDING | PENDING |
| License/notices complexity | PENDING | PENDING — production notice audit required | PENDING |
| Reproducible pinning | **PASS WITH LIMITATION** | **PASS** for N2 probe | PENDING |
| Rollback/replaceability | **PASS** | **PASS** | PENDING |

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
| Links/navigation | PENDING | PENDING | Candidate evidence exists; no final responsibility selection yet |
| Outline read | PENDING | PENDING | Candidate evidence exists; no final responsibility selection yet |
| Security/capability inspection | PENDING | PENDING | |
| Structural transformation/write | PENDING | PENDING | |
| Independent output validation | PENDING | PENDING | |

The final entries above must match accepted ADR-0004. No decision is implied by candidate ordering or intermediate PASS rows.
