# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — core read/navigation/Unicode/search/security, repeated synthetic performance, rendering fidelity, coordinate normalization, and qpdf structural/security/transformation evidence captured; stress, production distribution/licensing and final responsibility selection remain pending**  
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Opened:** 2026-09-22  
**Last evidence refresh:** 2026-09-23

This is the binding comparison sheet for N2. Detailed evidence remains in the focused baseline files linked below. Intermediate PASS rows are responsibility evidence only and do **not** constitute `N2 PASS`.

## Result vocabulary

- **PASS** — requirement satisfied by measured fixture/evidence.
- **PASS WITH LIMITATION** — usable with a named bounded limitation/workaround.
- **FAIL** — unsuitable for that responsibility.
- **BLOCKED** — evidence cannot currently be produced.
- **N/A** — outside the candidate's intended role.
- **PENDING** — not yet qualified.

## Canonical evidence ledger

| Slice | Implementation SHA | Run / artifact | Result |
|---|---|---|---|
| Qt PDF core | `213050ee75882ae5fa53f73b07fe2707bbaea5a8` | CI `35679099222`; artifact `10674630141` | Debug/Release PASS |
| PDFium core | `7874794e14b9cea54ec0723c15963621f65bebf6` | CI `35680738681`; artifact `10674712819` | Debug/Release PASS |
| navigation | `59073e4bd341bd4f5feb86097a8f700a3603be4e` | CI `35682017265`; artifact `10675244583` | PASS |
| Unicode + search | `57029562525fc90f0fb8fb80ce32d5781705b054` | CI `35686938500`; artifact `10676978221` | PASS with documented normalization limits |
| malformed/password read security | `ba673ff0d539f38f44136c6757ecc1220c709865` | CI `35687950134`; artifact `10677830518` | PASS |
| repeated performance | `ab5b00ed8b573145cd065ead2ad61163e6f2c232` | run `35689957375`; artifact `10678183252` | PASS WITH LIMITATION |
| qpdf structural/security/transformation | `1d645e13487215f29a13d528603683229a5140d9` | qpdf `35793085186`; artifact `10722283322` | PASS / PASS WITH LIMITATION by capability |
| rendering fidelity / geometry | `d9daf12cf3b1a7efc7118733279536585f8831fc` | fidelity `35796991213`; artifact `10724751540` | PASS / PASS WITH LIMITATION by capability |
| coordinate normalization + explicit `/XYZ` | `0bbe1132872a946e9869945be20b8f9f174a1785` | coordinates `35800654969`; artifact `10725573435` | PASS; Qt rotated destination limitation recorded |

Focused sources:

- `docs/baselines/N2_PDFIUM_PROVENANCE.md`
- `docs/baselines/N2_QPDF_BASELINE.md`
- `docs/baselines/N2_QPDF_PROVENANCE.md`
- `docs/baselines/N2_RENDERING_FIDELITY.md`
- `docs/baselines/N2_COORDINATE_NORMALIZATION.md`
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` — remains **Proposed**.

## Candidate identity / acquisition

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended responsibility | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version tested | **6.10.3** | **156.0.8066.0 / `chromium/8066`** | **12.4.1** |
| Current acquisition | Qt 6.10.3 MSVC 2022 x64 + `qtpdf` | pinned `bblanchon/pdfium-binaries` non-V8 x64 package | official qpdf MSVC64 ZIP |
| Exact package hash | Qt package archive hash not yet independently frozen | **PASS** — `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` | **PASS** — `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6` |
| Probe build integration | **PASS** | **PASS WITH LIMITATION** — community prebuilt probe package | **PASS WITH LIMITATION** — CLI probe route |
| Production route approved? | **PENDING** | **PENDING** | **PENDING** |
| Replaceability in N2 | **PASS** — isolated probe | **PASS** — isolated probe | **PASS** — isolated CLI workflow |

No candidate is linked into the production `atlas_reader`/portable domain boundary by N2 qualification work.

## Read / open / page geometry

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Valid document open | **PASS** | **PASS** | A001–A006; A008 correct-password open |
| Invalid/malformed failure typing | **PASS** | **PASS** | A007: Qt `invalid-file-format`; PDFium `format` |
| Password-required / wrong password | **PASS** | **PASS** | A008; candidate enums differ, Atlas semantic state required |
| Supported encrypted open | **PASS** | **PASS** | A008 user password |
| Page count | **PASS** | **PASS** | current deterministic corpus |
| Page labels | **PASS** | **PASS** | current deterministic corpus |
| Raw MediaBox/CropBox/Rotate oracle | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | independently proven by pypdf on A011; not exposed identically by both read APIs |
| Effective visible page size | **PASS** | **PASS** | A002/A011 normal, cropped, rotated pages |
| CropBox normalization | **PASS** | **PASS** | A011 200×150 effective visible page |
| 90° inherent rotation normalization | **PASS** | **PASS** | A011 300×200 effective page |
| Image-only page handling | **PENDING** | **PENDING** | no dedicated fixture yet |

## Rendering fidelity

A011 deterministic vector fixture SHA-256:

`b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Nominal 1× vector semantic fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | stable color landmarks; synthetic vector corpus |
| 2× / high-density semantic fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | same landmarks at 2×; not arbitrary high-DPI corpus |
| CropBox render correctness | **PASS** | **PASS** | A011 page 1 |
| 90° rotation render correctness | **PASS** | **PASS** | A011 page 2 orientation |
| Annotation off/on behavior | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | one Link annotation with explicit normal appearance |
| Native blank-background behavior | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Qt untouched pixels transparent; PDFium probe prefilled opaque white; Atlas policy required |
| Cross-engine byte-identical pixels | **N/A** | **N/A** | deliberately not a requirement |
| Real Arabic/Urdu shaped-font fidelity | **PENDING** | **PENDING** | A006 is logical Unicode, not visual shaping evidence |
| Image/gradient/transparency-group fidelity | **PENDING** | **PENDING** | not covered by A011 |
| Broad annotation subtype fidelity | **PENDING** | **PENDING** | current explicit-appearance Link only |

Detailed evidence: `N2_RENDERING_FIDELITY.md`.

## Text extraction and Unicode

A006 deterministic logical-Unicode fixture SHA-256:

`efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b`

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | synthetic corpus |
| Plain Arabic extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | exact A006 text; real-font corpus still useful |
| Mixed Arabic/English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 mixed page; arbitrary bidi layouts pending |
| Urdu extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | exact A006 text |
| Combining marks / tashkīl | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | all tested code points preserved; raw mark ordering requires Unicode normalization |
| Unicode outlines | **PASS** | **PASS** | Arabic/Urdu A006 titles/hierarchy/pages agree |

Source-order fully vocalized text and raw engine output are canonically equivalent after Unicode normalization. Atlas must normalize before equality, indexing and user-query comparison.

## Search

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| English search | **PASS** | **PASS** | A001 + A006 mixed page |
| Plain Arabic search | **PASS** | **PASS** | A006 pages 0/2 |
| Urdu search | **PASS** | **PASS** | A006 page 3 |
| Fully vocalized Arabic query | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | source-order form gets 0 hits; normalized/raw-order form hits |
| Hit page/per-page ordinal | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | shared portable identity; native indexes differ |
| Search-hit rectangles — normal/crop/90° | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 cross-engine max observed field delta 0.43 pt |
| Multi-line/real-font/bidi geometry | **PENDING** | **PENDING** | current geometry corpus simple/synthetic |

Qt and PDFium search timing APIs represent different execution models; timing rows below must not be interpreted as pure internal algorithm ratios.

## Links / outlines / destination geometry

Atlas coordinate contract:

> effective visible page; origin upper-left; X right; Y down; units PDF points.

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Internal link detection | **PASS** | **PASS** | A003/A005 destination page 2 |
| External URI detection | **PASS WITH LIMITATION** | **PASS** | Qt exposes duplicate raw URI row; semantic de-duplication required |
| Outline hierarchy/order | **PASS** | **PASS** | A003/A004 |
| Arabic/English/Urdu outline text | **PASS** | **PASS** | A006 |
| Destination page normalization | **PASS** | **PASS** | current outline/link corpus |
| Source link rectangle normalization | **PASS** | **PASS** | A003 exact normalized rectangles |
| Search rectangle normalization | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 simple text; Qt raw rotated QRect may have negative width and must be canonicalized |
| `/XYZ` destination — normal target | **PASS** | **PASS** | A012 expected `(40,50)` |
| `/XYZ` destination — 90° target | **PASS WITH LIMITATION** | **PASS** | Qt returns unrotated top-left `(40,50)` and needs external inherent-rotation metadata to reach Atlas `(250,40)`; PDFium directly normalizes raw `(40,250)` to `(250,40)` |
| Fit/FitH/FitV destination modes | **PENDING** | **PENDING** | not required by current fixture set |

A012 SHA-256:

`0c710f3f6e4457a44b8d2dbaa42a476a5c5b429764a0a7569002b66222ef069d`

Detailed evidence: `N2_COORDINATE_NORMALIZATION.md`.

## Reader security/error semantics

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Malformed hard failure | **PASS** | **PASS** | A007 |
| Password-required state | **PASS** | **PASS** | A008 |
| Wrong-password state | **PASS** | **PASS** | A008 |
| Correct supported encrypted open | **PASS** | **PASS** | A008 |
| Unsupported-security distinction | **PENDING** | **PENDING** | no deterministic unsupported-scheme fixture yet |
| Permission capability inspection | **N/A / not qualified** | **N/A / not qualified** | qpdf is structural/security candidate for this responsibility |

Portable Atlas state must not expose candidate numeric error enums.

## PDFium concurrency-specific evidence

| Check | Result | Evidence / limitation |
|---|---|---|
| Public non-thread-safe constraint acknowledged | **PASS** | all qualification calls serialized |
| Serialized-call correctness | **PASS WITH LIMITATION** | all current probes pass |
| Concurrent Atlas workloads routed through one PDFium execution lane | **PENDING** | dedicated adapter/task-queue stress still required if PDFium is selected |
| Exact package pin/checksum | **PASS** | `chromium/8066`; SHA recorded above |
| Production source/package route | **PENDING** | community binary is probe-only |

## qpdf structural / security / transformation

Current canonical qpdf evidence head:

`1d645e13487215f29a13d528603683229a5140d9`

Exact first-party qpdf package:

- version `12.4.1`;
- official `qpdf-12.4.1-msvc64.zip`;
- asset ID `533019080`;
- SHA-256 `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6`.

| Capability | qpdf result | Evidence / limitation |
|---|---|---|
| Valid structural check | **PASS WITH LIMITATION** | A003/A006 synthetic corpus |
| Hard malformed diagnosis | **PASS WITH LIMITATION** | A007 returns exit 2 |
| Encryption revision/state inspection | **PASS WITH LIMITATION** | A008/A009 R2 corpus |
| User-vs-owner password state | **PASS WITH LIMITATION** | pypdf 0/1/2 independent oracle + qpdf identity flags |
| Restricted permission inspection | **PASS WITH LIMITATION** | A009 `/P=-64`, all tested capabilities false |
| Page/outline structural JSON | **PASS** | A003/A006 |
| No-op rewrite + qpdf re-check | **PASS** | A003/A006 |
| Unrelated semantic preservation | **PASS WITH LIMITATION** | explicit synthetic invariant set |
| Controlled ASCII outline title mutation | **PASS** | `Chapter 2` → `Atlas Controlled Outline` |
| Controlled Unicode outline title mutation | **PASS** | `اردو` → `اردو — فوائد` |
| Signature/DocMDP structural visibility | **PASS WITH LIMITATION** | A010 synthetic structure; not cryptographic verification |
| Signed/certified pre-mutation safety interlock | **PASS WITH LIMITATION** | source state detectable; rewrite bytes change while signature dictionaries may remain |
| Cryptographic signature validity verification | **N/A / PENDING separate verifier** | A010 deliberately invalid cryptographically |
| Add/remove/reparent outline nodes | **PENDING** | only existing-title mutation qualified |
| Encrypted rewrite preservation | **PENDING** | only if Atlas v2 intends to mutate encrypted PDFs |
| Linked-library production route | **PENDING** | current qualification is CLI route |

Atlas must never use qpdf object numbers as portable document identity because qpdf may renumber indirect objects during serialization.

## Preservation invariants for structural mutation

| Invariant | Current qpdf evidence |
|---|---|
| Page count | **PASS** — unchanged in tested no-op/title mutations |
| MediaBox/CropBox/rotation | **PASS** — snapshot equality |
| Decoded page contents | **PASS** — hashes equal |
| Existing annotations | **PASS** — semantic snapshots equal |
| Untargeted outline nodes/destinations | **PASS** |
| Document Info | **PASS** |
| XMP | **PASS WITH LIMITATION** — hash equality when present |
| Attachments | **PASS WITH LIMITATION** — current fixture snapshot |
| Encryption policy | **PENDING for write path** | unencrypted mutation corpus |
| Signature/certification consequence | **PASS WITH LIMITATION** — structural detection + interlock, not crypto verification |

## Repeated synthetic performance

Protocol: GitHub-hosted Windows Server 2022, Release, 3 excluded warmups + 31 measured iterations per operation.

### Warm p50 / p95 ms

| Fixture | Operation | Qt PDF | PDFium |
|---|---|---:|---:|
| A003 | open | 0.4461 / 0.4625 | 0.0102 / 0.0178 |
| A003 | extract all pages | 0.1188 / 0.1214 | 0.0579 / 0.0598 |
| A003 | known-hit search | 109.8045 / 125.0793 | 0.0627 / 0.0642 |
| A003 | render 612×792 | 0.3176 / 0.3213 | 0.3290 / 0.3621 |
| A006 | open | 0.9745 / 1.0490 | 0.0116 / 0.0189 |
| A006 | extract all pages | 0.1079 / 0.1152 | 0.0376 / 0.0389 |
| A006 | known-hit search | 328.7500 / 344.5818 | 0.0409 / 0.0414 |
| A006 | render 612×792 | 0.4082 / 0.4411 | 0.3104 / 0.3486 |

Important limitations:

- tiny deterministic synthetic corpus;
- Qt search includes asynchronous `QPdfSearchModel` completion/stability behavior while PDFium search is synchronous;
- hosted runner data does not establish user-machine latency or large-document scaling;
- no absolute N2 performance threshold is invented from these samples.

### Peak working-set signal

| Fixture | Qt PDF | PDFium |
|---|---:|---:|
| A003 | 17.4414 MiB | 13.7930 MiB |
| A006 | 15.3711 MiB | 11.4492 MiB |

Lifetime-symmetric repeated-render/open-close growth remains PENDING.

## Licensing / distribution state

| Surface | Current evidence | Status |
|---|---|---|
| Qt PDF module | LGPLv3/GPLv2-or-commercial; embeds PDFium snapshot + third-party components | **PASS WITH LIMITATION** — terms identified; Atlas shipping-compliance plan still pending |
| PDFium upstream | BSD-style license + third-party dependencies | **PASS WITH LIMITATION** — upstream license identified |
| `bblanchon/pdfium-binaries` repo | MIT packaging repository; shared-library distribution; build pipeline stages licenses | **PASS WITH LIMITATION** — does not by itself approve DLL redistribution |
| PDFium exact production notices/SBOM | not yet frozen for selected route | **PENDING** |
| qpdf | Apache-2.0 primary upstream; official CLI package qualified | **PASS WITH LIMITATION** — exact binary notice/dependency audit + CLI-vs-library decision pending |
| Cross-platform acquisition | Windows evidence only | **PENDING** |

Production selection requires an explicit shipping route, notices/SBOM, update/rollback procedure and runtime/package footprint for every selected responsibility.

## Hard blockers

A responsibility cannot be accepted if evidence shows any of these without a contained workaround:

- required PDF content can be silently lost/corrupted;
- required Arabic/Unicode semantics are not dependable;
- the selected distribution route cannot be legally/reproducibly shipped;
- concurrency requirements force unsafe use;
- encrypted/restricted/signed states cannot be distinguished safely enough for Atlas workflows;
- engine-native types or object IDs leak through the portable Atlas boundary;
- coordinate/background/Unicode quirks remain implicit rather than normalized at the adapter;
- rollback/update provenance cannot be pinned.

## Final responsibility decision — intentionally not selected yet

| Responsibility | Selected implementation | Status | Evidence still needed before selection |
|---|---|---|---|
| Document open/read metadata | PENDING | PENDING | production acquisition + stress context |
| Page geometry/labels | PENDING | PENDING | candidate correctness strong; final split decision pending |
| Page raster rendering | PENDING | PENDING | synthetic fidelity now measured; real-font/image-heavy evidence only if judged necessary |
| Text extraction | PENDING | PENDING | Unicode correctness exists; final selection + production route pending |
| Search | PENDING | PENDING | semantics/geometry measured; normalization contract + production route pending |
| Links/navigation | PENDING | PENDING | Qt duplicate rows + rotated `/XYZ` metadata dependency must be weighed against PDFium |
| Outline read | PENDING | PENDING | Qt/PDFium + qpdf evidence exists |
| Security/capability inspection | PENDING | PENDING | qpdf evidence strong; production integration route pending |
| Structural transformation/write | PENDING | PENDING | qpdf preservation/title mutation/signature interlock proven; scope-dependent broader mutations may be unnecessary |
| Independent output validation | PENDING | PENDING | qpdf check + independent pypdf reopen proven; final architecture pending |

No ordering above implies a preferred candidate.

## Remaining N2 gates

Before ADR-0004 can move from **Proposed** to **Accepted** and before owner `N2 PASS`:

1. **Stress/concurrency:** lifetime-symmetric repeated open/render/close evidence and, if PDFium remains a serious candidate, serialized Atlas task-queue stress.
2. **Production acquisition/licensing:** choose and document shippable Qt/PDFium/qpdf routes, required notices/SBOM, runtime/package footprint, security-update and rollback procedure.
3. **Scope review:** decide whether real-world shaped Arabic/image-heavy rendering, non-XYZ destination modes, unsupported-security fixtures, broader qpdf outline mutation, or encrypted-write preservation are actual v2 blockers or later hardening tasks.
4. **Responsibility assignment:** fill the final table with explicit Atlas-owned normalization boundaries and rollback path.
5. **Final strict CI:** all selected responsibility probes/builds green on one implementation head.
6. **Owner acceptance:** explicit `N2 PASS`.

N2 is **Open**. N3 is **Not started**.
