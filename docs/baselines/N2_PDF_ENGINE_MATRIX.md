# N2 PDF Engine Qualification Matrix

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — Qt PDF/PDFium core, navigation, Unicode/search, basic reader security, repeated synthetic performance, and qpdf structural/security/transformation including restricted permissions and signature/DocMDP mutation-safety evidence captured; fidelity, coordinate normalization, stress, production packaging/licensing and final responsibility selection remain pending**  
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Opened:** 2026-09-22  
**Last evidence refresh:** 2026-09-23

This is the durable comparison sheet for N2. It must be updated when evidence is produced. Empty cells are intentionally `PENDING`; absence of evidence must never be converted into a guessed result. Intermediate PASS rows are responsibility evidence only and do **not** constitute `N2 PASS`.

## Result vocabulary

- **PASS** — requirement satisfied by measured/fixture evidence.
- **PASS WITH LIMITATION** — usable with a named bounded limitation/workaround.
- **FAIL** — unsuitable for that responsibility.
- **BLOCKED** — evidence cannot currently be produced.
- **N/A** — responsibility is outside the engine's intended role.
- **PENDING** — not tested yet.

---

## Canonical evidence chronology

### N2.1 — Qt PDF core

Implementation: `213050ee75882ae5fa53f73b07fe2707bbaea5a8`

- GitHub Actions run `35679099222` — Debug and Release PASS.
- Artifact ID `10674630141`.
- Artifact digest `sha256:6490e039589984d3190f3e6ac0f0269b9b732f21da581472822d90a5fb51187b`.
- Qt PDF 6.10.3 via `qtpdf`, MSVC 2022 x64.
- `atlas_reader` and `atlas_core` do not link Qt PDF; only focused qualification probes do.
- Independent pypdf 5.9.0 fixture validation passes A001–A005.
- A001–A005 strict Qt PDF CTests pass.
- An earlier A003 empty-text symptom was traced to Windows checkout line-ending corruption of a PDF fixture, not Qt PDF. Repository binary handling and later source-generated deterministic fixtures prevent that class of false engine failure.

Single-run timings from the original core probe remain smoke evidence only. Canonical repeated timing evidence is recorded below.

### N2.2 — PDFium core

Implementation: `7874794e14b9cea54ec0723c15963621f65bebf6`

- GitHub Actions run `35680738681` — PASS.
- PDFium `156.0.8066.0` / `chromium/8066`.
- Probe distribution: `bblanchon/pdfium-binaries`, source commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac`.
- Asset `pdfium-win-x64.tgz`, ID `579031518`, size 3,823,498 bytes.
- Archive SHA-256 `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`, verified before extraction.
- Artifact ID `10674712819`; digest `sha256:0972d8424b6d35c8f2c8c1612ddbea60b34f7038b59c5d8c5643ab37bfad4803`.
- `atlas_pdfium_probe` is isolated from `atlas_reader`/`atlas_core`.
- All public PDFium calls in qualification remain serialized on one thread, honoring the upstream non-thread-safe API contract.
- A001–A005 strict PDFium tests pass.
- On A001–A005, Qt PDF and PDFium agree on page counts, page labels, normalized visible page sizes and page-by-page UTF-8 text SHA-256 values.
- A001 renders non-null through both engines, but raw pixel hashes differ; fidelity/equivalence is not inferred.

Detailed acquisition/licensing caveats are in `N2_PDFIUM_PROVENANCE.md`.

### Navigation

Implementation: `59073e4bd341bd4f5feb86097a8f700a3603be4e`

- GitHub Actions run `35682017265` — Debug and Release PASS.
- PR checkout SHA `f6a68fe10f3268e91ddca292d5f357420569d197` recorded separately.
- Artifact ID `10675244583`.
- Artifact digest `sha256:280d660a99c44de72eecb65e6b6fcdd530517a6b64fe09e3ec81f0f896e8d3a3`.

A003/A004 normalized outline semantics agree in both engines:

| Item | Depth | Destination page |
|---|---:|---:|
| `Chapter 1` | 0 | 0 |
| `Section 1.1` | 1 | 1 |
| `Chapter 2` | 0 | 2 |

A003/A005 link semantics also agree:

- internal destination: zero-based page `2`;
- external URI: `https://example.com/atlas-n2-fixture`.

Raw engine difference deliberately preserved:

- PDFium reports the two explicit PDF link annotations: raw count `2`;
- Qt `QPdfLinkModel` reports raw count `3`: the internal destination plus two rows for the same external URI with different rectangles.

Therefore Qt external-URI detection is **PASS WITH LIMITATION** at the raw model layer and requires Atlas-side semantic normalization/de-duplication. Destination-page normalization is proven; destination-coordinate normalization remains PENDING.

### Unicode + search

Implementation: `57029562525fc90f0fb8fb80ce32d5781705b054`

- GitHub Actions run `35686938500` — Debug and Release PASS with 27 strict CTests.
- PR checkout SHA `a098a24dbfd9ee59a1406396db7bd0e3f01fa189` recorded separately.
- Artifact ID `10676978221`.
- Artifact digest `sha256:28f465d787936b8fc943f7f57ce3806f58a5853325bf38140097267da388e120`.
- A006 deterministic SHA-256 `efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b`.

A006 is a logical-Unicode Type3/ToUnicode fixture and **not** Arabic/Urdu visual-shaping evidence.

Qt PDF and PDFium produce identical A006 page text lengths and UTF-8 hashes:

| Page | Purpose | UTF-16 length | UTF-8 SHA-256 |
|---:|---|---:|---|
| 0 | plain Arabic | 13 | `9262a0a791605071a500c1a15bef2d5efcc6c8f198567105e9ab364811377e9f` |
| 1 | fully vocalized Arabic | 38 | `376cdb244082d82602d5f60ab1edff730450a55d53b5b9bcb2eaf2755ce76cb0` |
| 2 | English + Arabic runs | 28 | `5a83c4017aff1dd32778fd24fe25ea52d85ca1fe741574daf52473b72d8efc69` |
| 3 | Urdu | 14 | `273b0d5f90d7e562bc5ccf57666fc9e511d43b60343049b534e10e39af5b0d84` |

Unicode outline semantics also agree: `العربية` -> page 0; child `مُشَكَّل` -> page 1; `Mixed العربية English` -> page 2; `اردو` -> page 3.

#### Combining-mark limitation

The source vocalized line SHA-256 is `43ffde22c1f7320e3683a8b883168d6bfdd1678779e4349cd9833ab80229734f`. Both engines preserve all 38 UTF-16 code units but expose a different raw combining-mark order inside affected clusters; raw full-line hash is `376cdb244082d82602d5f60ab1edff730450a55d53b5b9bcb2eaf2755ce76cb0`.

The source and raw forms are canonically equivalent after Unicode normalization. Atlas must normalize Unicode before user-facing equality, indexing or search comparison.

#### Search semantics

Strict evidence agrees in both engines:

- A001 English: 1 hit, page 0, ordinal 0;
- A006 plain Arabic: 2 hits, pages 0 and 2, ordinal 0 on each;
- A006 English on mixed page: 1 hit, page 2, ordinal 0;
- A006 Urdu: 1 hit, page 3, ordinal 0;
- A006 raw-order fully vocalized Arabic: 1 hit, page 1, ordinal 0.

The source-order fully vocalized Arabic query returns **0 hits in both engines**, while the canonically equivalent raw-order query hits. This is a shared normalization requirement, not an engine differentiator. Qt `IndexOnPage`, PDFium character start/count and vendor-native geometry are retained as native evidence; cross-engine hit geometry remains PENDING.

### Reader malformed/password security

Implementation: `ba673ff0d539f38f44136c6757ecc1220c709865`

- GitHub Actions run `35687950134` — Debug and Release PASS with 35 strict CTests.
- Artifact ID `10677830518`.
- Artifact digest `sha256:0b7c519a81fbab87808a85169806918188cb6b33462f0496df63e38e5a352d30`.
- A007 malformed SHA-256 `d6e2fb7962bb233083c110593f6e6968b067c155c713ebb9418c19493e48d79f`.
- A008 password SHA-256 `62a6b4e332b8296250b9f8d1076e7c5d717a01c54a7471418bc771f0e2d4a08e`.

A008 uses weak RC4-40 **only as deterministic test data**.

| Case | Qt PDF 6.10.3 | PDFium `chromium/8066` | Result |
|---|---|---|---|
| A007 malformed | `invalid-file-format`, code `4` | `format`, code `3` | PASS both |
| A008 no password | `incorrect-password`, code `5` | `password`, code `4` | PASS both |
| A008 wrong password | `incorrect-password`, code `5` | `password`, code `4` | PASS both |
| A008 correct user password | `none`, code `0`; page 1/label `1` | `success`, code `0`; page 1/label `1` | PASS both |

The portable Atlas contract must therefore express semantic states instead of exposing candidate-engine numeric enums.

### Repeated synthetic performance

Implementation: `ab5b00ed8b573145cd065ead2ad61163e6f2c232`

- `N2 PDF Performance` run `35689957375` — PASS.
- Same-head normal Windows CI `35689957385` — Debug/Release PASS.
- Artifact ID `10678183252`.
- Artifact digest `sha256:398854eec6bf88e8c6b3e2c47fb37b1f02eb14f54c6631013351c95780e3969e`.
- Environment: GitHub-hosted Windows Server 2022, MSVC 2022 x64 Release.
- Protocol: 3 excluded warmups + 31 measured iterations per operation; nearest-rank p50/p95; in-memory open input; 612 x 792 render.

Warm p50/p95 ms:

| Fixture | Operation | Qt PDF | PDFium |
|---|---|---:|---:|
| A003 | open | 0.4461 / 0.4625 | 0.0102 / 0.0178 |
| A003 | extract all pages | 0.1188 / 0.1214 | 0.0579 / 0.0598 |
| A003 | known-hit search | 109.8045 / 125.0793 | 0.0627 / 0.0642 |
| A003 | render 612 x 792 | 0.3176 / 0.3213 | 0.3290 / 0.3621 |
| A006 | open | 0.9745 / 1.0490 | 0.0116 / 0.0189 |
| A006 | extract all pages | 0.1079 / 0.1152 | 0.0376 / 0.0389 |
| A006 | known-hit search | 328.7500 / 344.5818 | 0.0409 / 0.0414 |
| A006 | render 612 x 792 | 0.4082 / 0.4411 | 0.3104 / 0.3486 |

First-operation ms:

| Fixture | Operation | Qt PDF | PDFium |
|---|---|---:|---:|
| A003 | open | 0.6992 | 0.1834 |
| A003 | extract all pages | 54.2246 | 2.1143 |
| A003 | known-hit search | 100.9263 | 0.0787 |
| A003 | render | 0.9956 | 0.7940 |
| A006 | open | 1.1194 | 0.0810 |
| A006 | extract all pages | 0.5575 | 0.5111 |
| A006 | known-hit search | 359.3860 | 0.0499 |
| A006 | render | 0.5745 | 0.4184 |

Peak process working set signal:

| Fixture | Qt PDF MiB | PDFium MiB |
|---|---:|---:|
| A003 | 17.4414 | 13.7930 |
| A006 | 15.3711 | 11.4492 |

These hosted synthetic figures are engineering evidence, **not a final responsibility choice**. Qt asynchronous `QPdfSearchModel` and PDFium synchronous text search are not identical internal workloads. Lifetime-symmetric memory-growth/leak stress remains PENDING.

### qpdf structural/security/transformation

Current canonical implementation: `1d645e13487215f29a13d528603683229a5140d9`

Exact-head gates:

- `N2 qpdf Qualification` run `35793085186` — PASS;
- normal Windows CI run `35793085026` — Debug and Release PASS, including deterministic A007–A010 regeneration and independent ten-fixture validation;
- `N2 PDF Performance` run `35793085077` — PASS.

Canonical qpdf artifact:

- name `atlas-reader-n2-qpdf-1d645e13487215f29a13d528603683229a5140d9`;
- artifact ID `10722283322`;
- digest `sha256:a1ea89237b98505b56b987dd637d623a0672857a6749a995af1f3f3ae1a9a3eb`;
- size 48,626 bytes.

Same-head Windows engineering artifact:

- ID `10723115687`;
- digest `sha256:3a9a71bbfab08c95920587fea89aa4fc568770fab110df952d1bf82455d31795`.

Pinned qpdf distribution:

- qpdf 12.4.1, tag `v12.4.1`;
- official MSVC64 ZIP, asset ID `533019080`, size 28,165,367 bytes;
- SHA-256 `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6` verified before extraction;
- primary upstream license Apache-2.0;
- current qualification covers the first-party CLI route only. A linked-library production route requires its own build/dependency baseline.

Current canonical qpdf evidence:

- A003/A006 `qpdf --check` clean and JSON v2 exposes current page/outline structures, including Arabic/Urdu titles.
- A007 hard-truncated malformed input returns non-clean exit `2`.
- A008 permissive R2 fixture: user/owner password identities independently qualified; `/P=-4`; effective capabilities all allowed.
- A009 restricted R2 fixture SHA `0db18b77d4dc8f43ee728fd22eb897f1072f630444a41fef3a8fff12b83400b2`: pypdf decrypt states wrong/user/owner = `0/1/2`; qpdf sees `/P=-64`; extraction, print, accessibility and all exposed modify classes false.
- A003/A006 no-op rewrites re-check clean and independently preserve page count/labels, text hashes, MediaBox/CropBox/rotation, decoded page content, outlines/destinations, annotations, Info, XMP and attachments.
- Controlled A003 title mutation `Chapter 2` -> `Atlas Controlled Outline` passes with unrelated preservation invariants intact.
- Controlled A006 Unicode title mutation `اردو` -> `اردو — فوائد` passes with unrelated preservation invariants intact.
- qpdf may renumber indirect objects during serialization; numeric PDF object IDs are explicitly **not** Atlas portable/domain identity.

#### A010 signed/certified structural-safety evidence

A010 deterministic SHA-256: `95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668`.

A010 is deliberately **not cryptographically valid**. It contains synthetic signature bytes and sentinel `/ByteRange [0 0 0 0]`. It qualifies structural detection and mutation-safety policy only.

Independent pypdf and qpdf evidence confirms:

- signature form field `Atlas Certification`;
- `/Type /Sig`;
- `/Filter /Adobe.PPKLite`;
- `/SubFilter /adbe.pkcs7.detached`;
- `/Reference` to `/TransformMethod /DocMDP`;
- `/DigestMethod /SHA256`;
- transform params `/P=2`, `/V /1.2`;
- catalog `/Perms /DocMDP`.

A qpdf rewrite changes the file SHA to `fd410d246825f74da3614c9b0ed1c7b066b76cfe16d881d387946a9495e067a4`, re-checks cleanly, and still contains the signature/DocMDP dictionaries. Therefore the presence of `/Sig` after rewrite **cannot** be treated as evidence that integrity remains valid.

Atlas safety contract established by this evidence:

1. signature/DocMDP structure must be detected before structural mutation;
2. silent mutation of signed/certified input is forbidden;
3. Atlas may not claim signature integrity survives a rewrite merely because signature dictionaries remain present;
4. actual cryptographic validity requires a separate qualified verifier and, where needed, re-signing.

This is **PASS WITH LIMITATION** for structural signed/certified visibility and the pre-mutation safety interlock. It is not cryptographic-verification evidence.

Focused qpdf details remain in `N2_QPDF_BASELINE.md` and `N2_QPDF_PROVENANCE.md`.

---

## Candidate identity / acquisition

| Item | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Intended N2 role | read/render/text/search/navigation | read/render/text/search/navigation | structural/security/transformation |
| Exact version/revision | **6.10.3** | **156.0.8066.0 / `chromium/8066`** | **12.4.1** |
| Acquisition path | Qt desktop MSVC 2022 x64 + `qtpdf` | **PASS WITH LIMITATION** — pinned community non-V8 Windows x64 package, probe-only | **PASS WITH LIMITATION** — pinned first-party MSVC64 CLI ZIP; linked-library route not qualified |
| Compiler/build path | **PASS** — CMake + MSVC 2022 | **PASS WITH LIMITATION** — published `PDFiumConfig.cmake` + MSVC; official source build differs | **N/A for current CLI slice**; library path pending if selected |
| Package/archive SHA-256 | PENDING — Qt archive hash not separately captured | **PASS** — `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` | **PASS** — `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6` |
| Primary license surface | Qt module terms require release review | PDFium/Chromium notices; distributor repo MIT; production audit PENDING | Apache-2.0 upstream; binary notices/dependencies audit PENDING |
| Production distribution decision | PENDING | PENDING — community binary is probe-only | PENDING — CLI-vs-library + notices/runtime deps unresolved |

## Read/open and geometry

| Capability | Qt PDF | PDFium | Notes |
|---|---|---|---|
| Valid document open | **PASS** | **PASS** | A001–A006 + A008 correct-password |
| Invalid/malformed failure typing | **PASS** | **PASS** | A007 |
| Password-required detection | **PASS** | **PASS** | A008 |
| Incorrect-password distinction | **PASS** | **PASS** | A008 |
| Supported encrypted open | **PASS** | **PASS** | A008 user password |
| Unsupported-security distinction | PENDING | PENDING | no deterministic unsupported scheme qualified |
| Page count | **PASS** | **PASS** | current valid reader corpus |
| Page labels | **PASS** | **PASS** | current valid reader corpus |
| Raw MediaBox | PENDING | PENDING | preservation snapshot exists through pypdf/qpdf, reader API normalization pending |
| Raw CropBox | PENDING | PENDING | A002 visible crop result observed; reader raw box normalization pending |
| Raw rotation | PENDING | PENDING | A002 normalized rotated dimensions agree; raw rotation normalization pending |
| Mixed page sizes | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | normalized visible sizes match |
| Image-only page handling | PENDING | PENDING | |

## Rendering

| Capability | Qt PDF | PDFium | Notes |
|---|---|---|---|
| 100% nominal raster fidelity | PENDING | PENDING | non-null smoke passes; raw pixel hashes differ |
| High-DPI raster fidelity | PENDING | PENDING | |
| Rotation correctness | PENDING | PENDING | |
| Crop-box correctness | PENDING | PENDING | |
| Annotation rendering behavior | PENDING | PENDING | |
| Transparent/background behavior | PENDING | PENDING | |
| Warm render p50/p95 | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | repeated A003/A006 synthetic benchmark |
| First render on loaded document | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | first-operation values recorded separately |
| Repeated-render memory behavior | PENDING | PENDING | lifetime-symmetric stress required |

## Text extraction and search

| Capability | Qt PDF | PDFium | Notes |
|---|---|---|---|
| English Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | synthetic corpus |
| Arabic Unicode extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006; real-font corpus pending |
| Mixed Arabic/English extraction | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 mixed page; broader bidi ordering pending |
| Diacritics/combining marks | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | code points preserved; normalization required |
| Urdu text | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A006 synthetic ToUnicode |
| Extraction bounds/geometry | PENDING | PENDING | |
| English search | **PASS** | **PASS** | A001 + A006 mixed page |
| Arabic search | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | plain Arabic passes; vocalized source order requires normalization |
| Mixed-script search | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | current mixed-page cases |
| Hit page/index identity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | page/per-page ordinal normalized; vendor indices differ |
| Hit geometry/destination | PENDING | PENDING | raw geometry captured; normalization pending |
| First extraction time | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | synthetic tiny corpus |
| Repeated extraction time | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | 31 warm measured samples after 3 warmups |
| Search completion time | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Qt async vs PDFium sync internal semantics differ |

## Links, outlines and destinations

| Capability | Qt PDF | PDFium | Notes |
|---|---|---|---|
| Internal link detection | **PASS** | **PASS** | A003/A005 -> page 2 |
| External URI detection | **PASS WITH LIMITATION** | **PASS** | Qt duplicate raw URI row |
| Named/explicit destination handling | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | current fixture forms only |
| Outline hierarchy | **PASS** | **PASS** | A003/A004 |
| Duplicate outline titles | PENDING | PENDING | |
| Deep outline hierarchy | PENDING | PENDING | only one child depth currently |
| Arabic/English outline text | **PASS** | **PASS** | A006 |
| Destination page normalization | **PASS** | **PASS** | tested current corpus |
| Destination coordinate normalization | PENDING | PENDING | |

## PDFium concurrency/build-specific evidence

| Check | Result | Evidence |
|---|---|---|
| Public API non-thread-safe constraint acknowledged | **PASS** | qualification records `serialized-single-thread` |
| Serialized-call correctness | **PASS WITH LIMITATION** | fixture suite passes; app task-queue stress pending |
| Parallel Atlas tasks respect contract | PENDING | production adapter/task queue not built |
| Exact revision/tag pin | **PASS** | `chromium/8066` |
| Binary/archive checksum | **PASS** | verified before extraction |
| Clean CI acquisition | **PASS** | repeated Debug/Release/performance runs |
| Runtime binary/dependency footprint | PENDING | product delta/transitives pending |
| Upgrade/rollback procedure | **PASS WITH LIMITATION** | exact pin/remove path documented; production source/update policy unresolved |

## qpdf structural/security evidence

| Capability | qpdf result | Notes |
|---|---|---|
| Valid structural open/check | **PASS WITH LIMITATION** | A003/A006 synthetic corpus |
| Malformed diagnostics | **PASS WITH LIMITATION** | A007 hard truncation only |
| Encryption algorithm/revision inspection | **PASS WITH LIMITATION** | A008/A009 R2; broader handlers pending |
| User/owner password state | **PASS WITH LIMITATION** | independently qualified wrong/user/owner = 0/1/2 on R2 fixtures |
| Permission restriction inspection | **PASS WITH LIMITATION** | A008 all-allowed + A009 all-restricted R2 states |
| Page tree/basic geometry access | **PASS WITH LIMITATION** | structural/preservation corpus; broader raw geometry pending |
| Outline read hierarchy/order | **PASS** | A003/A006 |
| Outline destination read | **PASS WITH LIMITATION** | current destination forms |
| Signature/certification structural visibility | **PASS WITH LIMITATION** | A010 `/Sig` + DocMDP P=2; crypto validity not qualified |
| Pre-mutation signed/certified safety interlock | **PASS WITH LIMITATION** | structural detection + byte-changing rewrite consequence established; production adapter enforcement pending |
| Cryptographic signature verification | **N/A for current qpdf evidence** | separate verifier responsibility if Atlas needs validity reporting |
| No-op/rewrite reopen | **PASS** | A003/A006 qpdf check + pypdf reopen |
| Unrelated-structure preservation | **PASS WITH LIMITATION** | explicit invariant set on A003/A006 |
| Controlled outline transformation | **PASS WITH LIMITATION** | existing ASCII + Unicode title-only mutations; add/remove/reparent pending |
| Output qpdf validation | **PASS** | rewritten/mutated outputs re-check clean |
| Independent-reader reopen | **PASS** | pypdf 5.9.0 |
| Exact qpdf version | **PASS** | 12.4.1 |
| Exact vcpkg baseline/override | **N/A for current CLI slice** | linked-library path would require separate baseline |
| Unsupported-security distinction | PENDING | no deterministic unsupported-scheme fixture yet |

## Preservation invariants for mutation probes

| Invariant | Expected | qpdf evidence |
|---|---|---|
| Page count | unchanged unless operation changes pages | **PASS** — A003/A006 no-op/title mutation |
| Page dimensions/rotation | unchanged | **PASS** |
| Page content streams | unchanged for outline-only operation | **PASS** — decoded stream hashes equal |
| Existing annotations | unchanged | **PASS** |
| Existing outline nodes not targeted | unchanged | **PASS** |
| Document Info | preserved unless edited | **PASS** |
| XMP presence/content | preserved unless edited | **PASS** |
| Encryption policy | preserved unless explicitly changed | **PASS WITH LIMITATION** — A003/A006 are unencrypted; encrypted-write preservation not qualified |
| Signatures/certification consequence | detected and never silently represented as preserved integrity | **PASS WITH LIMITATION** — A010 structural interlock proven; cryptographic validity not qualified |
| Attachments/other unrelated structures | preserved | **PASS WITH LIMITATION** — current deterministic fixture set |

## Build and maintenance comparison

| Criterion | Qt PDF | PDFium | qpdf |
|---|---|---|---|
| Fits existing CMake/workflow | **PASS** | **PASS WITH LIMITATION** — probe package integrates; official source build differs | **PASS WITH LIMITATION** — isolated CLI workflow fits qualification; linked library not qualified |
| Fits existing MSVC toolchain | **PASS** | **PASS** for probe package | **PASS** for official MSVC64 CLI package |
| Extra toolchain required | **PASS** | **PASS WITH LIMITATION** — official source build needs Chromium tooling | **PASS WITH LIMITATION** — none for CLI probe; library build may differ |
| CI setup cost | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** |
| Package size delta | PENDING | PENDING | **PASS WITH LIMITATION** — qualification ZIP known; product delta pending |
| Runtime dependency delta | PENDING | PENDING | PENDING |
| Cross-platform path | PENDING | PENDING | PENDING — current qpdf evidence Windows CLI only |
| License/notices complexity | PENDING | PENDING — production notice audit required | **PASS WITH LIMITATION** — Apache-2.0 upstream; binary notices/dependency audit pending |
| Reproducible pinning | **PASS WITH LIMITATION** | **PASS** for N2 probe | **PASS** |
| Rollback/replaceability | **PASS** | **PASS** | **PASS** — isolated, no product linkage |

---

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
| Document open/read metadata | PENDING | PENDING | candidate correctness + synthetic performance evidence exists; no final selection |
| Page geometry/labels | PENDING | PENDING | raw box/rotation contract still incomplete |
| Page raster rendering | PENDING | PENDING | timing exists; fidelity/high-DPI/annotation evidence pending |
| Text extraction | PENDING | PENDING | Unicode evidence exists; real-font/geometry evidence incomplete |
| Search | PENDING | PENDING | Unicode/search evidence exists; normalization + geometry contract incomplete |
| Links/navigation | PENDING | PENDING | semantic navigation evidence exists; coordinate normalization incomplete |
| Outline read | PENDING | PENDING | Qt/PDFium semantic + qpdf structural evidence exists |
| Security/capability inspection | PENDING | PENDING | Qt/PDFium malformed/password + qpdf encryption/user-owner/permissions/signature-structure evidence exists; unsupported schemes, crypto-verifier responsibility and production packaging remain unresolved |
| Structural transformation/write | PENDING | PENDING | qpdf preservation + ASCII/Unicode title mutation + signed-state interlock evidence exists; broader mutation/encrypted-write/product integration remain unresolved |
| Independent output validation | PENDING | PENDING | qpdf `--check` + independent pypdf reopen proven on current outputs; final architecture not selected |

## Remaining evidence before final N2 decision

1. **Rendering fidelity / geometry:** normalized raw MediaBox/CropBox/rotation contract, raster fidelity at nominal and high DPI, annotation/background behavior.
2. **Coordinate normalization:** extraction/search hit geometry and destination coordinates across read candidates.
3. **Stress/concurrency:** lifetime-symmetric repeated open/render/extract memory-growth checks and PDFium serialized application task-queue stress.
4. **Production acquisition/licensing:** Qt PDF shipping terms, PDFium production source/binary route and notices, qpdf CLI-vs-library/runtime dependency and notice decision.
5. **Security follow-up where product-relevant:** deterministic unsupported-security typing if practical; select/qualify an independent cryptographic signature verifier only if Atlas v2 must report signature validity rather than merely protect mutation safety.
6. **Broader qpdf mutation only if required by v2 scope:** add/remove/reparent outline nodes and encrypted-write preservation.
7. Final responsibility assignment in ADR-0004, final strict CI, rollback/replaceability confirmation and explicit owner `N2 PASS`.

ADR-0004 remains **Proposed**. PR #4 must remain draft/open/unmerged during active qualification. N3 remains **Not started**. No decision is implied by candidate ordering or intermediate PASS rows.