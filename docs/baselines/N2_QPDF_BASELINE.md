# N2 qpdf Structural / Security / Transformation Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Canonical candidate evidence — N2 still Open**  
**Recorded:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical qpdf evidence slice for N2. It is candidate/responsibility evidence only; it does not select the final production architecture and it does not constitute `N2 PASS`.

## 1. Exact identity

Canonical implementation head:

`1d645e13487215f29a13d528603683229a5140d9`

Qualification runs on that exact implementation head:

- `N2 qpdf Qualification` run `35793085186` — PASS;
- normal Windows CI run `35793085026` — Debug and Release PASS, including deterministic A007–A010 regeneration, independent validation of the ten-fixture corpus, existing strict Qt PDF/PDFium tests, Release evidence staging and upload;
- `N2 PDF Performance` run `35793085077` — PASS, confirming the fixture/safety additions did not disturb the isolated read-engine benchmark pipeline.

Canonical qpdf artifact:

- name: `atlas-reader-n2-qpdf-1d645e13487215f29a13d528603683229a5140d9`;
- artifact ID: `10722283322`;
- artifact digest: `sha256:a1ea89237b98505b56b987dd637d623a0672857a6749a995af1f3f3ae1a9a3eb`;
- artifact size: 48,626 bytes.

Same-head Windows engineering artifact:

- name: `atlas-reader-n2-windows-x64-1d645e13487215f29a13d528603683229a5140d9`;
- artifact ID: `10723115687`;
- artifact digest: `sha256:3a9a71bbfab08c95920587fea89aa4fc568770fab110df952d1bf82455d31795`.

Historical qpdf heads remain valid evidence for the narrower slices they first qualified:

- `35888d8d09510c2366f68537fd5d2f2d3e05ea2d` — no-op preservation + controlled ASCII/Unicode outline-title mutation;
- `d794d4f09b238099b4b136704cc192f40fa3d1cb` — restricted-permission + user/owner-password evidence.

`1d645e13...` supersedes them as the current qpdf N2 baseline by retaining those checks and adding deterministic signature/DocMDP structural-safety evidence plus the generic Windows fixture-regeneration gate.

## 2. qpdf pin and acquisition

The qualification uses the first-party qpdf Windows MSVC 64-bit distribution:

- qpdf version: **12.4.1**;
- release tag: `v12.4.1`;
- release ID: `378185910`;
- asset: `qpdf-12.4.1-msvc64.zip`;
- asset ID: `533019080`;
- asset size: `28,165,367` bytes;
- expected and verified SHA-256: `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6`.

The workflow verifies the ZIP hash before extraction and requires `qpdf --version` to report `12.4.1` before any PDF is processed. No floating `latest` reference is used.

This evidence qualifies the official CLI distribution only. A future linked qpdf library integration would require its own exact CMake/compiler/package baseline and dependency audit.

Primary upstream license recorded for this pin: Apache License 2.0. Production redistribution/third-party notice review remains separate and pending.

## 3. Independently validated fixtures

Before qpdf qualification, pypdf 5.9.0 independently validates the complete ten-fixture N2 corpus. Relevant fixed identities are:

| Fixture | Purpose | SHA-256 |
|---|---|---|
| A003 | outlines + links | `77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5` |
| A006 | deterministic Unicode/Arabic/Urdu semantics | `efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b` |
| A007 | deliberately truncated malformed PDF | `d6e2fb7962bb233083c110593f6e6968b067c155c713ebb9418c19493e48d79f` |
| A008 | Standard Security Handler V1/R2 RC4-40 permissive password fixture | `62a6b4e332b8296250b9f8d1076e7c5d717a01c54a7471418bc771f0e2d4a08e` |
| A009 | Standard Security Handler V1/R2 RC4-40 restricted-permission fixture (`P=-64`) | `0db18b77d4dc8f43ee728fd22eb897f1072f630444a41fef3a8fff12b83400b2` |
| A010 | structural `/Sig` + `/DocMDP` certification-safety fixture (`P=2`) | `95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668` |

A008/A009 use intentionally weak legacy RC4-40 only because the format is compact and deterministic enough for security-state qualification. They are test data, not a production encryption recommendation.

A010 is intentionally **not cryptographically valid**. Its `/Contents` bytes are synthetic and `/ByteRange` is a sentinel `[0 0 0 0]`. It qualifies structural signature/certification detection and mutation safety only; it must never be cited as evidence of cryptographic signature verification.

Public A008/A009 fixture credentials are deliberately non-secret:

- user password: `atlas-user`;
- owner password: `atlas-owner`;
- wrong-password sentinel: `atlas-wrong`.

Independent pypdf 5.9.0 evidence proves for both A008 and A009:

- encrypted state is present;
- page access without decryption is blocked;
- wrong-password result = `0`;
- user-password result = `1`;
- owner-password result = `2`.

It separately confirms signed permission integers:

- A008: `/P = -4`;
- A009: `/P = -64`.

For A010, independent pypdf validation confirms:

- one signature form field named `Atlas Certification`;
- `/Type /Sig` and `/SubFilter /adbe.pkcs7.detached`;
- sentinel `/ByteRange [0 0 0 0]` and signature contents present;
- catalog `/Perms /DocMDP` is present;
- one `/TransformMethod /DocMDP` reference;
- transform permission `/P = 2`;
- `cryptographic_validity_qualified=false` and fixture `cryptographically_valid=false`.

The generic Windows CI now regenerates and SHA-verifies A007–A010 before independent validation. This is intentional: generated PDF byte identity must come from checked-in source + pinned SHA, not from checkout line-ending behavior or an opaque stored blob.

## 4. Structural read evidence

`qpdf --check` returns clean exit `0` for A003 and A006.

qpdf JSON v2 exposes the expected page and outline semantics:

### A003

- page count: 3;
- outline depth-first titles: `Chapter 1`, `Section 1.1`, `Chapter 2`;
- source outline objects in this deterministic input are rooted at `obj:10 0 R`, with outline items `obj:11 0 R`, `obj:12 0 R`, `obj:13 0 R`;
- destination pages correspond to pages 1, 2 and 3 in qpdf's one-based source references / pages 0, 1 and 2 in Atlas/pypdf normalized indexing.

### A006

- page count: 4;
- outline depth-first titles: `العربية`, `مُشَكَّل`, `Mixed العربية English`, `اردو`;
- Unicode outline text is preserved through qpdf JSON and independent pypdf reopen;
- hierarchy and destination pages agree with the previously qualified Qt PDF/PDFium evidence.

Indirect object numbers are **not** treated as portable Atlas identity. qpdf may legitimately renumber objects while rewriting a PDF.

## 5. Malformed, encryption, permission and certification inspection

### A007 malformed input

`qpdf --check` returns exit `2`, so the deliberately truncated file is not silently classified as structurally clean.

This is sufficient for the current hard-malformed diagnostic gate. Broader malformed-but-recoverable corpora remain future robustness evidence.

### A008 permissive encrypted input

qpdf's scriptable password contract is stable on A008:

| Check | Exit |
|---|---:|
| `--is-encrypted` | 0 |
| `--requires-password` without password | 0 |
| `--requires-password` with `atlas-wrong` | 0 |
| `--requires-password` with user password | 3 |
| `--requires-password` with owner password | 3 |

qpdf JSON v2 with the user password reports:

- `encrypted=true`;
- `userpasswordmatched=true`;
- `ownerpasswordmatched=false`;
- `R=2`, `V=1`, 40-bit RC4;
- `P=-4`;
- all exposed capability flags true.

With the owner password it reports the inverse password identity (`ownerpasswordmatched=true`, `userpasswordmatched=false`) while retaining the same encryption parameters.

### A009 restricted-permission encrypted input

A009 uses the same deterministic public credentials and R2 handler, but `/P=-64`, clearing the four Revision-2 permission classes: printing, modification, copy/extraction and annotations.

The same qpdf password exit contract holds. qpdf JSON v2 with the user password reports:

- `encrypted=true`;
- `userpasswordmatched=true`, `ownerpasswordmatched=false`;
- `R=2`, `V=1`, 40-bit RC4;
- `P=-64`;
- `extract=false`;
- `printlow=false`, `printhigh=false`;
- `modify=false`;
- `modifyannotations=false`;
- `modifyassembly=false`;
- `modifyforms=false`;
- `modifyother=false`;
- `accessibility=false`.

With the owner password qpdf correctly switches to `ownerpasswordmatched=true` and retains the same underlying security parameters/capability description.

Therefore qpdf can distinguish the tested user/owner password identities and expose both an all-allowed and an all-restricted effective permission state for the deterministic R2 corpus.

### A010 signature / DocMDP structural-safety evidence

A010 deliberately contains a PDF signature field and certification policy structure but **not a valid cryptographic signature**.

qpdf 12.4.1 JSON v2 detects the source structure exactly:

- signature field title `Atlas Certification`;
- signature dictionary `/Type /Sig`;
- filter `/Adobe.PPKLite`;
- subfilter `/adbe.pkcs7.detached`;
- `/ByteRange [0 0 0 0]`;
- signature contents and `/Reference` present;
- `/TransformMethod /DocMDP`;
- digest method `/SHA256`;
- transform parameters `/P = 2`, `/V /1.2`;
- catalog `/Perms /DocMDP` present.

A qpdf rewrite of A010:

- succeeds;
- changes the file SHA-256 from `95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668` to `fd410d246825f74da3614c9b0ed1c7b066b76cfe16d881d387946a9495e067a4` in the canonical probe evidence;
- re-checks cleanly;
- still exposes the signature/DocMDP structures after rewrite;
- renumbers indirect objects during serialization, reinforcing that object IDs are not portable identity.

This establishes the required Atlas safety policy:

1. signature-field and DocMDP/certification structure detection is required **before** any structural mutation;
2. silent mutation of a signed/certified document is not allowed;
3. Atlas must never represent a rewritten document as retaining verified signature integrity merely because the `/Sig` dictionary remains present;
4. any claim about actual signature validity after or before mutation requires an independent cryptographic verifier and, where appropriate, re-signing.

This is **PASS WITH LIMITATION** for signature/certification visibility and rewrite-safety interlock. It is **not** cryptographic signature verification evidence.

Bound security limitations remain:

- broader Standard Security Handler revisions/AES variants and reproducible unsupported-security typing are still pending;
- permission enforcement by viewer applications is distinct from capability inspection and is not inferred here;
- a real cryptographically valid signed/certified corpus and verifier responsibility are not yet qualified.

## 6. No-op rewrite preservation

A003 and A006 are each rewritten by qpdf without a requested semantic change, then re-checked with qpdf and independently reopened with pypdf.

For both fixtures:

- qpdf rewrite succeeds;
- rewritten `qpdf --check` exit is `0`;
- output bytes differ from source bytes, which is expected for a structural rewrite;
- every asserted semantic preservation invariant is true.

The independent preservation snapshot proves equality for:

1. encryption state;
2. page count;
3. page labels;
4. page-by-page extracted UTF-8 text hashes;
5. MediaBox, CropBox and rotation;
6. decoded page-content-stream hashes;
7. outline titles/hierarchy/page destinations;
8. semantic annotations including subtype, rectangle, URI/action and destination;
9. Document Info;
10. raw XMP metadata stream hash when present;
11. attachment names/content hashes when present.

This is **PASS WITH LIMITATION** for no-op structural preservation: it is strong deterministic synthetic evidence, not proof for every PDF feature or arbitrary real-world document. Encrypted rewrite-preservation remains outside this specific no-op preservation corpus.

A010 is deliberately different: its rewrite is used to demonstrate the signed/certified safety interlock, **not** to claim signature integrity is preserved.

## 7. Controlled outline transformation

qpdf JSON v2 `--update-from-json` is qualified through two deliberately narrow object-level transformations.

The update JSON contains only `jsonversion: 2` and exactly one existing outline object copied from qpdf's own source JSON with only `/Title` changed. All other source objects are omitted from the update JSON and are therefore not intentionally modified by the operation.

### A003 ASCII title mutation

- source title: `Chapter 2`;
- new title: `Atlas Controlled Outline`;
- source target object: `obj:13 0 R`;
- qpdf mutation exit: 0;
- rewritten `qpdf --check`: 0.

Independent pypdf comparison proves exactly the intended one outline title changed, outline depth and destination page remain unchanged, and page/text/content/annotations/Info/XMP/attachments remain equal under the same preservation snapshot.

### A006 Unicode title mutation

- source title: `اردو`;
- new title: `اردو — فوائد`;
- source target object: `obj:56 0 R`;
- qpdf mutation exit: 0;
- rewritten `qpdf --check`: 0.

Independent pypdf comparison proves the same preservation invariants as A003 while additionally establishing that qpdf JSON update writes the tested Unicode Urdu/Arabic outline title correctly.

### Object-renumbering diagnostic

The first mutation run, implementation `dcf02ac57ea723bcea686ce25d420fe4fbf627bb`, failed only an exploratory assertion that indirect object IDs must remain numerically identical after serialization. The PDF mutations themselves had already succeeded and all semantic preservation checks were true.

That raw-ID assertion was removed because PDF indirect object numbers are serialization detail, not Atlas domain identity. Canonical probes require semantic hierarchy/destination preservation and successful qpdf/pypdf reopen instead.

## 8. qpdf capability result for current N2 corpus

| Capability | Current result | Bound limitation |
|---|---|---|
| Exact version/acquisition/checksum pin | **PASS** | CLI distribution qualified; future linked-library route separate |
| Valid structural open/check | **PASS** | A003/A006 synthetic corpus |
| Malformed diagnostics | **PASS WITH LIMITATION** | A007 hard truncation only |
| Encryption state inspection | **PASS** | A008/A009 R2 fixtures |
| Security revision / permission integer inspection | **PASS WITH LIMITATION** | R2 permissive + restricted states; broader handlers pending |
| Effective permission capability inspection | **PASS WITH LIMITATION** | A008 all-allowed and A009 all-restricted R2 states qualified |
| Password-required state | **PASS** | no/wrong/user/owner paths qualified on A008/A009 |
| User-vs-owner password state | **PASS WITH LIMITATION** | independently qualified on R2 fixtures only |
| Page/outline structural JSON | **PASS WITH LIMITATION** | current deterministic corpus |
| No-op rewrite + qpdf re-check | **PASS** | A003/A006 |
| Independent-reader reopen | **PASS** | pypdf 5.9.0 |
| No-op unrelated-structure preservation | **PASS WITH LIMITATION** | explicit synthetic invariant set only |
| Controlled ASCII outline title mutation | **PASS** | existing node title only |
| Controlled Unicode outline title mutation | **PASS** | existing node title only |
| Controlled add/remove/reparent outline nodes | **PENDING** | title mutation does not qualify pointer-tree construction |
| Signature/certification structural visibility | **PASS WITH LIMITATION** | A010 synthetic `/Sig` + DocMDP P=2; cryptographic validity not qualified |
| Pre-mutation signed/certified safety interlock | **PASS WITH LIMITATION** | policy/evidence established structurally; production enforcement adapter not built yet |
| Cryptographic signature verification | **N/A for current qpdf evidence** | independent verifier responsibility still to be selected/qualified |
| Unsupported-security distinction | **PENDING** | no deterministic unsupported-scheme fixture yet |

## 9. Architectural implications

The current evidence supports keeping qpdf as a **structural/security/transformation candidate** behind an Atlas-owned boundary. It should not leak qpdf object references or numeric object IDs into portable application/domain types.

The current evidence does **not** select qpdf as the production renderer or text/search engine; those responsibilities remain the Qt PDF/PDFium comparison.

Atlas security state must be semantic rather than qpdf-enum/string based. At minimum the future domain boundary must be able to represent encrypted/password-required, user-vs-owner credential state where relevant, effective permission restrictions, malformed/invalid structure, and signed/certified structure requiring a mutation interlock.

Any future mutation path must continue to apply explicit preservation invariants. Signed/certified input is a special safety state: a rewrite that leaves signature dictionaries in place must still be treated as invalidating any unverified integrity claim unless an independent cryptographic verifier proves otherwise.

## 10. Remaining qpdf/security work before final N2 decision

1. reproducible unsupported-security/encryption classification if practical and relevant to Atlas error semantics;
2. select/qualify the independent cryptographic-signature verification responsibility if Atlas v2 needs signature validity reporting rather than only mutation safety;
3. add/remove/reparent outline mutation if Atlas v2 requires qpdf to construct outline trees rather than only update existing nodes;
4. encrypted rewrite-preservation if Atlas intends to mutate encrypted PDFs rather than block/defer that workflow;
5. production CLI-vs-library packaging decision, runtime/dependency footprint and notice audit.

N2 remains **Open**. This baseline supplies evidence for ADR-0004; it does not accept the ADR or authorize merge by itself.