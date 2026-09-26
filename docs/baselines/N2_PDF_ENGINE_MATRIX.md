# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — bounded capability blockers pass; production-route freeze/requalification and final acceptance remain pending**
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Opened:** 2026-09-22  
**Last evidence refresh:** 2026-09-27

This is the binding comparison sheet for N2. Detailed evidence remains in the focused baseline files linked below. Intermediate PASS rows are responsibility evidence only and do **not** constitute `N2 PASS`.

## Result vocabulary

- **PASS** — requirement satisfied by measured fixture/evidence.
- **PASS WITH LIMITATION** — usable with a named bounded limitation/workaround.
- **FAIL** — unsuitable for that responsibility.
- **BLOCKED** — evidence cannot currently be produced.
- **N/A** — outside the candidate's intended role.
- **PENDING** — not yet qualified or not yet selected.

## Canonical evidence ledger

| Slice | Physically tested implementation SHA | Run / artifact | Result |
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
| stress / concurrency | `72109de91aad5496a9e2fe16d5741581243a649f` | stress `35801464463`; artifact `10726420090` | functional PASS; memory trends PASS WITH LIMITATION |
| image-only + real-font Arabic/Urdu content fidelity | `6ac322d9ddb4c9f53da158acf69265d65cdd5106` | content fidelity `36274881956`; artifact `10917256207` | A013 PASS; corrected A014 automated + owner visual PASS |
| permitted encrypted outline mutation | `28a072bbedf07808773068941fc8c7ed22252ba8` | encrypted write `36271839766`; artifact `10916290827` | PASS |

Focused sources:

- `docs/baselines/N2_PDFIUM_PROVENANCE.md`
- `docs/baselines/N2_QPDF_BASELINE.md`
- `docs/baselines/N2_QPDF_PROVENANCE.md`
- `docs/baselines/N2_RENDERING_FIDELITY.md`
- `docs/baselines/N2_COORDINATE_NORMALIZATION.md`
- `docs/baselines/N2_STRESS_CONCURRENCY.md`
- `docs/baselines/N2_PRODUCTION_DISTRIBUTION.md`
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md` — remains **Proposed**.

Later documentation-only commits do not redefine which binaries/probes were physically tested. Each evidence row above remains bound to its recorded implementation SHA.

## Candidate identity / acquisition

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended responsibility | read/render/text/search/navigation candidate | read/render/text/search/navigation candidate | structural/security/transformation candidate |
| Exact version tested | **6.10.3** | **156.0.8066.0 / `chromium/8066`** | **12.4.1** |
| Qualification acquisition | official Qt 6.10.3 MSVC 2022 x64 + `qtpdf` | pinned `bblanchon/pdfium-binaries` non-V8 x64 package | official first-party qpdf MSVC64 ZIP |
| Exact package hash | Qt release/module provenance still needs release freeze | **PASS** — `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` | **PASS** — `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6` |
| Probe build integration | **PASS** | **PASS WITH LIMITATION** — community prebuilt probe package | **PASS WITH LIMITATION** — CLI route |
| Proposed production route | official dynamic Qt distribution if selected | Atlas-owned pinned upstream source build if selected; community binary remains probe-only | first-party qpdf CLI process boundary unless a measured need justifies `libqpdf` |
| Production route frozen? | **PENDING** | **PENDING** | **PENDING** — route defined, minimal runtime/notice bundle not frozen |
| Replaceability in N2 | **PASS** — isolated probe | **PASS** — isolated probe | **PASS** — isolated CLI workflow |

No candidate is linked into the production `atlas_reader`/portable domain boundary by N2 qualification work.

## Read / open / page geometry

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Valid document open | **PASS** | **PASS** | A001–A006; A008 correct-password open |
| Invalid/malformed failure typing | **PASS** | **PASS** | A007: Qt `invalid-file-format`; PDFium `format` |
| Password-required / wrong password | **PASS** | **PASS** | A008; candidate enums differ, Atlas semantic state required |
| Supported encrypted open | **PASS** | **PASS** | A008 user password |
| Page count | **PASS** | **PASS** | deterministic corpus |
| Page labels | **PASS** | **PASS** | deterministic corpus |
| Raw MediaBox/CropBox/Rotate oracle | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | independently proven by pypdf on A011; read APIs expose different normalized surfaces |
| Effective visible page size | **PASS** | **PASS** | A002/A011 normal, cropped, rotated pages |
| CropBox normalization | **PASS** | **PASS** | A011 200×150 effective visible page |
| 90° inherent rotation normalization | **PASS** | **PASS** | A011 300×200 effective page |
| Image-only page handling | **PASS** | **PASS** | A013 opens/renders at 1x/2x and safely reports no text/search layer |

## Rendering fidelity

A011 deterministic vector fixture SHA-256:

`b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| Nominal 1× vector semantic fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | stable semantic color landmarks; synthetic vector corpus |
| 2× / high-density semantic fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | same landmarks at 2× |
| CropBox render correctness | **PASS** | **PASS** | A011 |
| 90° rotation render correctness | **PASS** | **PASS** | A011 |
| Annotation off/on behavior | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | explicit normal appearance Link annotation |
| Native blank-background behavior | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Qt untouched pixels transparent; PDFium qualification bitmap prefilled white; Atlas policy required |
| Cross-engine byte-identical pixels | **N/A** | **N/A** | deliberately not a requirement |
| Real Arabic/Urdu shaped-font fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Corrected A014 uses Noto Naskh Arabic + Noto Nastaliq Urdu; full-layout joining/marks, 1x/2x right-edge checks and owner visual review pass; bounded corpus |
| Image/gradient/transparency-group fidelity | **POST-N2 HARDENING** | **POST-N2 HARDENING** | bounded N2 image-only evidence is sufficient for selection |
| Broad annotation subtype fidelity | **N6/P1 EVIDENCE** | **N6/P1 EVIDENCE** | current explicit-appearance Link establishes only the N2 policy boundary |

Detailed evidence: `N2_RENDERING_FIDELITY.md`.

## Text extraction / Unicode / search

A006 deterministic logical-Unicode fixture SHA-256:

`efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b`

| Capability | Qt PDF | PDFium | Evidence / limitation |
|---|---|---|---|
| English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | synthetic corpus |
| Plain Arabic extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | exact A006 logical text |
| Mixed Arabic/English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 mixed page; arbitrary bidi layouts remain scope review |
| Urdu extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | exact A006 logical text |
| Combining marks / tashkīl | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | all tested code points preserved; raw mark order requires Unicode normalization |
| Unicode outlines | **PASS** | **PASS** | Arabic/Urdu A006 hierarchy/pages agree |
| English search | **PASS** | **PASS** | A001 + A006 mixed page |
| Plain Arabic search | **PASS** | **PASS** | A006 pages 0/2 |
| Urdu search | **PASS** | **PASS** | A006 page 3 |
| Fully vocalized Arabic query | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | source-order form gets 0 hits; canonical-equivalent normalized/raw form hits |
| Hit page/per-page ordinal | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | shared portable identity; native indexes differ |
| Search-hit rectangles — normal/crop/90° | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 cross-engine maximum observed field delta 0.43 pt |
| Multi-line/real-font/bidi geometry | **POST-N2 HARDENING** | **POST-N2 HARDENING** | current geometry corpus is simple/synthetic; not selection-changing |

Atlas must Unicode-normalize before equality, indexing and user-query comparison. Qt and PDFium search timing APIs represent different execution models; timing evidence must not be treated as a pure internal algorithm ratio.

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
| Search rectangle normalization | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Qt raw rotated QRect can have negative width and is canonicalized before portable exposure |
| `/XYZ` destination — normal target | **PASS** | **PASS** | A012 expected `(40,50)` |
| `/XYZ` destination — 90° target | **PASS WITH LIMITATION** | **PASS** | Qt needs external inherent-rotation metadata to transform `(40,50)` into Atlas `(250,40)`; PDFium normalizes directly |
| Fit/FitH/FitV destination modes | **POST-N2 HARDENING** | **POST-N2 HARDENING** | not required at engine-selection time; preserve as later navigation coverage |

A012 SHA-256:

`0c710f3f6e4457a44b8d2dbaa42a476a5c5b429764a0a7569002b66222ef069d`

Detailed evidence: `N2_COORDINATE_NORMALIZATION.md`.

## Reader security / structural security

| Capability | Qt PDF | PDFium | qpdf / structural layer |
|---|---|---|---|
| Malformed hard failure | **PASS** | **PASS** | **PASS WITH LIMITATION** on A007 |
| Password-required / wrong-password state | **PASS** | **PASS** | inspectable on A008/A009 |
| Correct supported encrypted open | **PASS** | **PASS** | N/A as reader responsibility |
| Unsupported-security distinction | **POST-N2 HARDENING** | **POST-N2 HARDENING** | malformed/password states already qualify selection; later app state remains required |
| User-vs-owner password identity | N/A / not selected | N/A / not selected | **PASS WITH LIMITATION** on R2 fixtures |
| Restricted permission inspection | N/A / not selected | N/A / not selected | **PASS WITH LIMITATION** on A009 `/P=-64` |
| Signature/DocMDP structural visibility | N/A / not selected | N/A / not selected | **PASS WITH LIMITATION** on synthetic A010 |
| Pre-mutation signed/certified interlock | N/A | N/A | **PASS WITH LIMITATION**; crypto validity not qualified |

Portable Atlas state must not expose candidate numeric error enums. Actual cryptographic signature verification is a separate responsibility if Atlas v2 ever claims signature validity.

## PDFium concurrency-specific evidence

Canonical stress implementation:

`72109de91aad5496a9e2fe16d5741581243a649f`

Stress run `35801464463`; artifact `10726420090`; digest `sha256:6fa983cc89745950b7d73c75d8aea9061dfa0723a348724c6063dac12058f34a`.

| Check | Result | Evidence / limitation |
|---|---|---|
| Public non-thread-safe constraint acknowledged | **PASS** | qualification contract never permits concurrent public PDFium API execution |
| 500 complete PDFium open/render/text/close lifetimes | **PASS** | 500/500; zero operation failures |
| Concurrent Atlas producers routed through one PDFium lane | **PASS** | 4 producers × 250 requests = 1,000 jobs |
| Producer-side PDFium API calls | **PASS** | `0` |
| Actual PDFium worker thread count | **PASS** | `1` |
| Maximum simultaneously active PDFium API executions | **PASS** | exactly `1` |
| Queue completion | **PASS** | submitted = completed = 1,000; failed = 0; clean shutdown |
| Production queue cancellation/back-pressure policy | **PENDING — later adapter design** | not an N2 engine-safety blocker |
| Exact package pin/checksum | **PASS** | `chromium/8066`; archive SHA recorded above |
| Production source/package route | **PASS WITH LIMITATION — route defined, not frozen** | proposed Atlas-owned upstream source build if selected |

The stress queue proves the serialized-executor architecture is viable; it does **not** prove simultaneous PDFium calls are safe, and N2 deliberately never attempts them.

## Lifetime / memory stress

Canonical detailed source: `N2_STRESS_CONCURRENCY.md`.

| Evidence | Qt PDF | PDFium |
|---|---:|---:|
| complete lifetimes | 500/500 | 500/500 |
| render successes | 500 | 500 |
| text successes | 500 | 500 |
| operation failures | 0 | 0 |
| start working set | 10.03125 MiB | 8.71484 MiB |
| end working set | 18.56641 MiB | 11.84375 MiB |
| start→end delta | +8.53516 MiB | +3.12891 MiB |
| first→last checkpoint delta | +4.30469 MiB | +0.59375 MiB |
| fitted checkpoint slope | ~9.94 KiB/iteration | ~1.23 KiB/iteration |

Result:

- repeated lifetime correctness: **PASS** both;
- resource/memory interpretation: **PASS WITH LIMITATION / measured evidence**;
- PDFium is nearly flat after initial growth in this synthetic run;
- Qt shows a measurable upward working-set trend in this one hosted-runner series;
- the Qt trend is a follow-up signal for any later real-document soak test, **not** an automatic leak verdict.

## qpdf structural / transformation evidence

Current canonical qpdf implementation:

`1d645e13487215f29a13d528603683229a5140d9`

Exact first-party qpdf package:

- qpdf `12.4.1`, tag `v12.4.1`;
- official `qpdf-12.4.1-msvc64.zip`;
- asset ID `533019080`;
- compressed size `28,165,367` bytes;
- SHA-256 `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6`.

| Capability | qpdf result | Evidence / limitation |
|---|---|---|
| Valid structural check | **PASS WITH LIMITATION** | A003/A006 synthetic corpus |
| Hard malformed diagnosis | **PASS WITH LIMITATION** | A007 returns exit 2 |
| Encryption/user-owner/permission inspection | **PASS WITH LIMITATION** | A008/A009 R2 corpus |
| Page/outline structural JSON | **PASS** | A003/A006 |
| No-op rewrite + qpdf re-check | **PASS** | A003/A006 |
| Unrelated semantic preservation | **PASS WITH LIMITATION** | explicit synthetic invariant set |
| Controlled ASCII outline title mutation | **PASS** | `Chapter 2` → `Atlas Controlled Outline` |
| Controlled Unicode outline title mutation | **PASS** | `اردو` → `اردو — فوائد` |
| Signature/DocMDP structural visibility | **PASS WITH LIMITATION** | A010 synthetic structure; not crypto verification |
| Signed/certified pre-mutation safety interlock | **PASS WITH LIMITATION** | source state detectable; rewrite bytes change while signature dictionaries can remain |
| Add/remove/reparent outline nodes | **PASS** | A003 breadth probe also covers delete/reorder/nest/unnest, Unicode and duplicate titles |
| Encrypted rewrite preservation | **PASS WITH LIMITATION** | A015 permitted R2 fixture; encryption/password/permission identities and unrelated invariants preserved; restricted writes remain forbidden |
| Linked-library production route | **N/A for current proposed route** | current proposed production route remains first-party CLI process boundary |

Atlas must never use qpdf indirect object numbers as portable document identity because qpdf may renumber objects during serialization.

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

These hosted synthetic timings are engineering evidence, not a final engine verdict. Qt asynchronous `QPdfSearchModel` and PDFium synchronous search do not perform identical internal workloads.

## Production acquisition / licensing / distribution

Detailed source: `N2_PRODUCTION_DISTRIBUTION.md`.

This section records **defined candidate shipping routes**, not final responsibility selection and not legal advice.

| Surface | Current production-route evidence | Status |
|---|---|---|
| Qt PDF | official Qt 6.10.3 dynamic module route; Qt commercial or open-source licensing surface identified; module includes PDFium + multiple third-party components; Qt SBOM/third-party material available | **PASS WITH LIMITATION — route defined, exact Atlas release provenance/files/notices/SBOM not frozen** |
| standalone PDFium | upstream BSD-style + third-party obligations identified; official source build uses Chromium `depot_tools`/`gclient`/GN/Ninja | **PASS WITH LIMITATION — proposed Atlas-owned pinned upstream source build if selected; production-built DLL still needs requalification** |
| `bblanchon/pdfium-binaries` | exact qualification asset pinned and hashed | **PROBE-ONLY — not production approval** |
| qpdf | official 12.4.1 MSVC64 package qualified; Apache-2.0 primary license; CLI process boundary already matches tested architecture | **PASS WITH LIMITATION — proposed first-party CLI route; minimal runtime DLL set and notice/SBOM bundle not frozen** |
| Cross-platform acquisition | N2 runtime evidence is Windows-focused | **PENDING — not required to select Windows N2 responsibilities unless product scope changes** |

Production selection still requires, for each selected responsibility:

1. exact production package/source provenance;
2. actual shipped runtime-file inventory and footprint;
3. license/NOTICE/SBOM bundle;
4. security-update owner/procedure;
5. previous-qualified-pin rollback path;
6. final regression qualification against the production route.

## Proposed final responsibility decision — acceptance pending

| Responsibility | Selected implementation | Status | Evidence still needed before selection |
|---|---|---|---|
| Document open/read metadata | PDFium | PROPOSED | freeze/requalify Atlas-controlled production build |
| Page geometry/labels | PDFium + Atlas normalization | PROPOSED | freeze/requalify production build |
| Page raster rendering | PDFium + Atlas background/compositing policy | PROPOSED | freeze/requalify production build; broader graphics corpus is later hardening |
| Text extraction | PDFium + Atlas Unicode normalization | PROPOSED | freeze/requalify production build |
| Search | PDFium + Atlas normalization/index layer | PROPOSED | freeze/requalify production build |
| Links/navigation | PDFium + Atlas normalized contracts | PROPOSED | freeze/requalify production build |
| Outline read | PDFium | PROPOSED | freeze/requalify production build |
| Security/capability inspection | qpdf 12.4.1 CLI adapter | PROPOSED | freeze minimal runtime/notices/SBOM bundle |
| Structural transformation/write | qpdf 12.4.1 CLI adapter | PROPOSED | freeze minimal runtime/notices/SBOM bundle |
| Independent output validation | qpdf `--check`; independent oracle in qualification only | PROPOSED | production app must not depend on pypdf |

No ordering above implies a selected candidate.

## Recorded scope decisions

The scope review records the following as later hardening or later-checkpoint
evidence rather than N2 selection blockers:

- broader image/gradient/transparency and multi-line/bidi geometry corpora;
- Fit/FitH/FitV view-mode fidelity;
- unsupported-security-scheme regression fixtures;
- broad annotation subtype fidelity (N6/P1);
- cryptographic signature validity (out of Windows 2.0 scope).

B1 image-only rendering, corrected B2 real-font Arabic/Urdu rendering, B3 full
outline breadth, and B4 permitted encrypted mutation are complete and are not
deferred by this classification.

## Remaining N2 gates

Before ADR-0004 can move from **Proposed** to **Accepted** and before owner `N2 PASS`:

1. **Production route freeze:** choose the actual selected components and freeze exact shippable provenance, runtime files/footprint, licenses/notices/SBOM, security-update and rollback procedure.
2. **Responsibility freeze:** synchronize ADR-0004 with the requalified production routes and retain explicit Atlas-owned normalization/serialization boundaries and rollback.
3. **Final strict CI:** all selected production-route probes/builds green on one implementation head.
4. **Owner acceptance:** explicit `N2 PASS`.

N2 is **Open**. N3 is **Not started**. PR #4 remains **draft/open/unmerged** until explicit N2 acceptance.
