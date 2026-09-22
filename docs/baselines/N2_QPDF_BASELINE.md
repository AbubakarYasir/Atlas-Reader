# N2 qpdf Structural / Security / Transformation Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Canonical candidate evidence — N2 still Open**  
**Recorded:** 2026-09-22  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical qpdf evidence slice for N2. It is candidate/responsibility evidence only; it does not select the final production architecture and it does not constitute `N2 PASS`.

## 1. Exact identity

Canonical implementation head:

`35888d8d09510c2366f68537fd5d2f2d3e05ea2d`

Qualification runs on that exact implementation head:

- `N2 qpdf Qualification` run `35691066745` — PASS;
- normal Windows CI run `35691066727` — Debug and Release PASS, preserving the existing 35 strict Qt PDF/PDFium functional/security tests and Release evidence staging/upload;
- `N2 PDF Performance` run `35691066809` — PASS, confirming the qpdf changes did not disturb the isolated read-engine benchmark pipeline.

Canonical qpdf artifact:

- name: `atlas-reader-n2-qpdf-35888d8d09510c2366f68537fd5d2f2d3e05ea2d`;
- artifact ID: `10678394708`;
- artifact digest: `sha256:fe41e20373ab9bcad2930d2664eb58d848dd1250887d01925b4a736c0457ac99`;
- artifact size: 36,135 bytes.

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

Before qpdf qualification, pypdf 5.9.0 independently validates the fixture corpus. Relevant fixed identities are:

| Fixture | Purpose | SHA-256 |
|---|---|---|
| A003 | outlines + links | `77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5` |
| A006 | deterministic Unicode/Arabic/Urdu semantics | `efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b` |
| A007 | deliberately truncated malformed PDF | `d6e2fb7962bb233083c110593f6e6968b067c155c713ebb9418c19493e48d79f` |
| A008 | deterministic Standard Security Handler V1/R2 RC4-40 password fixture | `62a6b4e332b8296250b9f8d1076e7c5d717a01c54a7471418bc771f0e2d4a08e` |

A008 uses intentionally weak legacy RC4-40 only to exercise deterministic password/security inspection. It is not a production encryption recommendation.

## 4. Structural read evidence

`qpdf --check` returns clean exit `0` for A003 and A006.

qpdf JSON v2 exposes the expected page and outline semantics:

### A003

- page count: 3;
- outline depth-first titles: `Chapter 1`, `Section 1.1`, `Chapter 2`;
- source outline objects in this deterministic input are rooted at `obj:10 0 R`, with outline items `obj:11 0 R`, `obj:12 0 R`, `obj:13 0 R`;
- destination pages correspond to pages 1, 2 and 3 in qpdf's one-based JSON summary / pages 0, 1 and 2 in Atlas/pypdf normalized indexing.

### A006

- page count: 4;
- outline depth-first titles: `العربية`, `مُشَكَّل`, `Mixed العربية English`, `اردو`;
- Unicode outline text is preserved through qpdf JSON and independent pypdf reopen;
- hierarchy and destination pages agree with the previously qualified Qt PDF/PDFium evidence.

Indirect object numbers are **not** treated as portable Atlas identity. qpdf may legitimately renumber objects while rewriting a PDF.

## 5. Malformed and encryption/security inspection

### A007 malformed input

`qpdf --check` returns exit `2`, so the deliberately truncated file is not silently classified as structurally clean.

This is sufficient for the current malformed diagnostic gate. Broader malformed-but-recoverable corpora remain future robustness evidence.

### A008 encrypted input

Scriptable qpdf exit behavior on the canonical fixture:

| Check | Exit | Meaning in this fixture |
|---|---:|---|
| A003 `--is-encrypted` | 2 | unencrypted |
| A008 `--is-encrypted` | 0 | encrypted |
| A008 `--requires-password` without password | 0 | another/correct password required |
| A008 `--requires-password` with `atlas-wrong` | 0 | supplied password is not sufficient |
| A008 `--requires-password` with correct user password `atlas-user` | 3 | encrypted file successfully accessible with supplied password |

`--show-encryption` works even without the correct password and reports for A008:

- incorrect password supplied;
- `R = 2`;
- `P = -4`;
- user password field empty in this diagnostic output;
- extraction/accessibility allowed;
- low/high-resolution printing allowed;
- document assembly, forms, annotations, other modification and unrestricted modification all allowed.

Therefore qpdf **can inspect the tested security revision, permission integer and effective capability flags without opening the document with the correct password**.

Limitations:

- A008 intentionally has permissive permission flags, so a *restricted* permission fixture is still required before restriction inspection is considered fully qualified;
- owner-password semantics are not yet independently qualified;
- unsupported encryption/security algorithms remain PENDING;
- cryptographic signature/certification visibility and mutation consequences remain PENDING.

## 6. No-op rewrite preservation

A003 and A006 were each rewritten by qpdf without a requested semantic change, then re-checked with qpdf and independently reopened with pypdf.

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

This is **PASS WITH LIMITATION** for no-op structural preservation: it is strong deterministic synthetic evidence, not proof for every PDF feature or arbitrary real-world document.

## 7. Controlled outline transformation

qpdf JSON v2 `--update-from-json` is qualified through two deliberately narrow object-level transformations.

The update JSON contains only:

- `jsonversion: 2`;
- exactly one existing outline object copied from qpdf's own source JSON;
- the same object dictionary with only `/Title` changed.

All other source objects are omitted from the update JSON and are therefore not intentionally modified by the operation.

### A003 ASCII title mutation

- source title: `Chapter 2`;
- new title: `Atlas Controlled Outline`;
- source target object: `obj:13 0 R`;
- rewritten target object after qpdf renumbering: `obj:6 0 R`;
- output SHA-256: `7fdeabc0db13200120bf237377ceaaf1476ae843c4c7c906d196a5699c657d71`;
- qpdf mutation exit: 0;
- rewritten `qpdf --check`: 0.

Independent pypdf comparison proves:

- exactly the intended one outline title changed;
- outline depth and destination page remain unchanged;
- page/text/content/annotations/Info/XMP/attachments all remain equal under the same preservation snapshot.

### A006 Unicode title mutation

- source title: `اردو`;
- new title: `اردو — فوائد`;
- source target object: `obj:56 0 R`;
- rewritten target object after qpdf renumbering: `obj:6 0 R`;
- output SHA-256: `2a01085b9f65a2f6e74715ab887911b3214719f366cd7ecbf72c80aab2c17915`;
- qpdf mutation exit: 0;
- rewritten `qpdf --check`: 0.

Independent pypdf comparison proves the same preservation invariants as A003, while additionally establishing that qpdf JSON update can write the tested Unicode Urdu/Arabic outline title correctly.

### Object-renumbering diagnostic

The first mutation run, implementation `dcf02ac57ea723bcea686ce25d420fe4fbf627bb`, failed only an exploratory assertion that indirect object IDs must remain numerically identical after serialization. The PDF mutations themselves had already succeeded and all semantic preservation checks were true.

That raw-ID assertion was correctly removed because PDF indirect object numbers are an implementation/serialization detail, not Atlas domain identity. Canonical implementation `35888d8d...` instead requires semantic hierarchy/destination preservation and successful qpdf/pypdf reopen.

## 8. qpdf capability result for current N2 corpus

| Capability | Current result | Bound limitation |
|---|---|---|
| Exact version/acquisition/checksum pin | **PASS** | CLI distribution qualified; future linked-library route separate |
| Valid structural open/check | **PASS** | A003/A006 synthetic corpus |
| Malformed diagnostics | **PASS WITH LIMITATION** | A007 hard truncation only |
| Encryption state inspection | **PASS** | A003/A008 |
| Security revision / permission integer inspection | **PASS WITH LIMITATION** | A008 R2 only |
| Effective permission capability inspection | **PASS WITH LIMITATION** | A008 permissions are all allowed; restricted fixture pending |
| Password-required state | **PASS** | A008 no/wrong/correct user-password paths |
| Owner-password state | **PENDING** | not independently qualified |
| Page/outline structural JSON | **PASS WITH LIMITATION** | current deterministic corpus |
| No-op rewrite + qpdf re-check | **PASS** | A003/A006 |
| Independent-reader reopen | **PASS** | pypdf 5.9.0 |
| No-op unrelated-structure preservation | **PASS WITH LIMITATION** | explicit synthetic invariant set only |
| Controlled ASCII outline title mutation | **PASS** | existing node title only |
| Controlled Unicode outline title mutation | **PASS** | existing node title only |
| Controlled add/remove/reparent outline nodes | **PENDING** | title mutation does not qualify pointer-tree construction |
| Signature/certification visibility/consequence | **PENDING** | |
| Unsupported-security distinction | **PENDING** | |

## 9. Architectural implications

The current evidence supports keeping qpdf as a **structural/security/transformation candidate** behind an Atlas-owned boundary. It should not leak qpdf object references or numeric object IDs into portable application/domain types.

The current evidence does **not** select qpdf as the production renderer or text/search engine; those responsibilities remain the Qt PDF/PDFium comparison.

Any future mutation path must continue to apply explicit preservation invariants and must treat signatures/certification as a special safety state rather than silently claiming integrity survives a rewrite.

## 10. Remaining qpdf/security work before final N2 decision

1. deterministic **restricted-permission** encrypted fixture and qpdf capability assertions;
2. owner-password/user-password distinction where relevant to Atlas workflows;
3. unsupported-security/encryption classification if reproducibly fixtureable;
4. signature/certification visibility and rewrite consequence detection;
5. add/remove/reparent outline mutation if Atlas v2 requires qpdf to construct outline trees rather than only update existing nodes;
6. production CLI-vs-library packaging decision, runtime/dependency footprint and notice audit.

N2 remains **Open**. This baseline supplies evidence for ADR-0004; it does not accept the ADR or authorize merge by itself.
