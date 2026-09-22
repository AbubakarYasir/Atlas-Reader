# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — Qt PDF/PDFium core, navigation, Unicode, search and basic malformed/password security evidence captured; performance, unsupported-security and qpdf pending**  
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

## N2 Unicode + search evidence baseline — 2026-09-22

Canonical Unicode/search evidence is bound to implementation head:

`57029562525fc90f0fb8fb80ce32d5781705b054`

- GitHub Actions run `35686938500` — Debug and Release PASS.
- PR checkout SHA `a098a24dbfd9ee59a1406396db7bd0e3f01fa189` is recorded separately from the implementation head.
- Artifact `atlas-reader-n2-windows-x64-57029562525fc90f0fb8fb80ce32d5781705b054`.
- Artifact ID `10676978221`.
- Artifact digest `sha256:28f465d787936b8fc943f7f57ce3806f58a5853325bf38140097267da388e120`.
- A006 is regenerated from checked-in source and must match SHA-256 `efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b` before validation/build/test.
- Independent pypdf 5.9.0 validation, 27 strict CTests, evidence staging and artifact upload all pass.
- A006 is a logical-Unicode fixture using synthetic Type3 glyphs plus ToUnicode mappings. It is **not** visual Arabic/Urdu shaping or raster-fidelity evidence.

### Unicode extraction

Qt PDF and PDFium produce identical A006 page text lengths and UTF-8 hashes:

| Page | Purpose | UTF-16 length | UTF-8 SHA-256 |
|---:|---|---:|---|
| 0 | plain Arabic | 13 | `9262a0a791605071a500c1a15bef2d5efcc6c8f198567105e9ab364811377e9f` |
| 1 | fully vocalized Arabic | 38 | `376cdb244082d82602d5f60ab1edff730450a55d53b5b9bcb2eaf2755ce76cb0` |
| 2 | English + Arabic runs | 28 | `5a83c4017aff1dd32778fd24fe25ea52d85ca1fe741574daf52473b72d8efc69` |
| 3 | Urdu | 14 | `273b0d5f90d7e562bc5ccf57666fc9e511d43b60343049b534e10e39af5b0d84` |

Plain Arabic and Urdu expected substrings pass exactly in both engines. Page 2 proves English and Arabic runs coexist and extract identically in both engines, but it does not yet qualify arbitrary contiguous bidi ordering. Unicode outline titles/hierarchy/pages also agree exactly: `العربية` -> page 0; child `مُشَكَّل` -> page 1; `Mixed العربية English` -> page 2; `اردو` -> page 3.

### Combining-mark limitation

The fixture source vocalized line has UTF-8 SHA-256:

`43ffde22c1f7320e3683a8b883168d6bfdd1678779e4349cd9833ab80229734f`

Both engines preserve all 38 UTF-16 code units, but their raw extracted representation reverses the order of combining marks within affected base-letter clusters. The resulting raw full-line hash is:

`376cdb244082d82602d5f60ab1edff730450a55d53b5b9bcb2eaf2755ce76cb0`

The source and raw forms are canonically equivalent after Unicode decomposition/normalization; no tested code point is lost. Atlas must therefore normalize Unicode before user-facing equality, indexing or search-query comparison rather than using raw engine strings as canonical text.

### Search semantics

Separate focused search probes preserve vendor-native fields while comparing a portable contract of zero-based page + per-page hit ordinal.

Strict evidence agrees in both engines:

- A001 English query: 1 hit, page 0, ordinal 0;
- A006 plain Arabic query: 2 hits, page 0 and page 2, ordinal 0 on each page;
- A006 English query on the mixed-script page: 1 hit, page 2, ordinal 0;
- A006 Urdu query: 1 hit, page 3, ordinal 0;
- A006 raw-order fully vocalized Arabic query: 1 hit, page 1, ordinal 0.

The source-order fully vocalized Arabic query is deliberately diagnostic rather than pre-assumed. It returns **0 hits in both Qt PDF and PDFium**, while the canonically equivalent raw-order query returns the expected hit. This confirms a shared normalization requirement for tashkīl-sensitive search; it is not evidence favoring one read engine over the other.

Qt `IndexOnPage` and PDFium character start/count are retained as engine-native evidence and are not treated as equivalent types. PDFium raw text rectangles and Qt search locations are also captured, but normalized cross-engine hit geometry remains PENDING.

The probe's one-shot `search_ms` values are **not performance evidence**. In particular, the Qt search probe intentionally waits for a minimum observation/stability window before declaring the asynchronous model settled. Repeated performance work must use a dedicated benchmark contract.

## N2 malformed + password security evidence baseline — 2026-09-22

Canonical basic security/open evidence is bound to implementation head:

`ba673ff0d539f38f44136c6757ecc1220c709865`

- GitHub Actions run `35687950134` — Debug and Release PASS.
- PR checkout SHA `72b7b95bc8a7a99b2f151719c3d8325b1994c7ba` is recorded separately from the implementation head.
- Artifact `atlas-reader-n2-windows-x64-ba673ff0d539f38f44136c6757ecc1220c709865`.
- Artifact ID `10677830518`.
- Artifact digest `sha256:0b7c519a81fbab87808a85169806918188cb6b33462f0496df63e38e5a352d30`.
- Independent pypdf 5.9.0 validation passes all 8 registered fixtures before engine qualification.
- 35 strict CTests pass in both Debug and Release, including eight focused security cases.

### Deterministic security fixtures

A007 is deliberately truncated malformed input and must match SHA-256:

`d6e2fb7962bb233083c110593f6e6968b067c155c713ebb9418c19493e48d79f`

Independent pypdf validation rejects it with `PdfStreamError`.

A008 is a deterministic one-page Standard Security Handler V1/R2 RC4-40 password fixture and must match SHA-256:

`62a6b4e332b8296250b9f8d1076e7c5d717a01c54a7471418bc771f0e2d4a08e`

The fixture uses public test credentials only. Independent validation proves: encrypted state detected; page access without a password is blocked; the wrong password is rejected; the correct user password opens the one-page document. **RC4-40 is intentionally weak legacy cryptography and exists only to exercise deterministic password-control flow. It is not a production crypto recommendation.**

### Engine error mapping

| Case | Qt PDF 6.10.3 | PDFium `chromium/8066` | Result |
|---|---|---|---|
| A007 malformed | `invalid-file-format`, code `4` | `format`, code `3` | PASS both |
| A008 no password | `incorrect-password`, code `5` | `password`, code `4` | PASS both |
| A008 wrong password | `incorrect-password`, code `5` | `password`, code `4` | PASS both |
| A008 correct password | `none`, code `0`; page 1/label `1` | `success`, code `0`; page 1/label `1` | PASS both |

The portable Atlas contract should therefore express semantic states such as malformed/invalid, password-required-or-invalid, and open-success rather than exposing candidate-engine numeric enums. The current fixture does **not** exercise an unsupported encryption/security scheme, permission restrictions, signatures, certification, or owner-password semantics; those remain PENDING.

The security probes record whether a password was supplied but never emit the password itself. Their one-shot `open_ms` values are diagnostic only and are not performance evidence.

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
| Valid document open | **PASS** | **PASS** | A001–A006 plus A008 correct-password open |
| Invalid/malformed failure typing | **PASS** | **PASS** | A007: Qt `invalid-file-format`; PDFium `format` |
| Password-required detection | **PASS** | **PASS** | A008 without password: Qt `incorrect-password`; PDFium `password` |
| Incorrect-password distinction | **PASS** | **PASS** | A008 wrong password rejected by both; candidate enums differ |
| Supported encrypted open with correct password | **PASS** | **PASS** | A008 correct user password; page count/label verified |
| Unsupported-security distinction | PENDING | PENDING | A008 is a supported legacy R2 fixture, not an unsupported-scheme fixture |
| Page count | **PASS** | **PASS** | A001–A006 agree; A008 succeeds with correct password |
| Page labels | **PASS** | **PASS** | Engines agree on all current valid fixtures |
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
| English Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A001–A006 hashes agree; current evidence remains synthetic |
| Arabic Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 plain Arabic exact in both; real-world font/ligature corpus still needed |
| Mixed Arabic/English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 page 2 English + Arabic runs agree; arbitrary contiguous bidi ordering not yet qualified |
| Diacritics/combining marks | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | All tested code points preserved, but raw mark order differs; Unicode normalization required |
| Urdu text | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 Urdu exact in both; synthetic ToUnicode fixture |
| Extraction bounds/geometry | PENDING | PENDING | |
| English search | **PASS** | **PASS** | A001 count/page identity passes; A006 mixed-page English also passes |
| Arabic search | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Plain Arabic exact search passes; vocalized source-order search requires normalization |
| Mixed-script search | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 Arabic hit on mixed page + English hit on same page; broader bidi/query variants pending |
| Hit page/index identity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Shared page/per-page ordinal contract passes; vendor-native index semantics differ |
| Hit geometry/destination | PENDING | PENDING | Raw locations/rectangles captured; coordinate normalization/equivalence not yet asserted |
| First extraction time | PENDING | PENDING | Existing values are one-shot smoke only |
| Repeated extraction time | PENDING | PENDING | |
| Search completion time | PENDING | PENDING | Current search timing is one-shot probe plumbing, not benchmark evidence |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes / evidence |
|---|---|---|---|
| Internal link detection | **PASS** | **PASS** | A003/A005; zero-based destination page `2` |
| External URI link detection | **PASS WITH LIMITATION** | **PASS** | Correct URI in both; Qt exposes duplicate raw URI row with distinct rectangle |
| Named/explicit destination handling | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Current synthetic named bookmark/link destinations resolve correctly; broader destination forms not yet covered |
| Outline hierarchy | **PASS** | **PASS** | A003/A004 exact titles, order, depths and destination pages |
| Duplicate outline titles | PENDING | PENDING | |
| Deep outline hierarchy | PENDING | PENDING | Current fixture depth only 1 child level |
| Arabic/English outline text | **PASS** | **PASS** | A006 exact Unicode titles, hierarchy and destination pages |
| Destination page normalization | **PASS** | **PASS** | A003/A005 internal target normalized to zero-based page `2`; outline pages `0,1,2`; A006 outline pages `0,1,2,3` |
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
| Text extraction | PENDING | PENDING | Candidate Unicode evidence exists; no final responsibility selection yet |
| Search | PENDING | PENDING | Candidate Unicode/search evidence exists; normalization contract still required |
| Links/navigation | PENDING | PENDING | Candidate evidence exists; no final responsibility selection yet |
| Outline read | PENDING | PENDING | Candidate evidence exists; no final responsibility selection yet |
| Security/capability inspection | PENDING | PENDING | Basic malformed/password reader evidence exists; unsupported-security/permissions/signatures and qpdf inspection remain pending |
| Structural transformation/write | PENDING | PENDING | |
| Independent output validation | PENDING | PENDING | |

The final entries above must match accepted ADR-0004. No decision is implied by candidate ordering or intermediate PASS rows.