# N2 PDF Fixture Corpus

This directory defines the tracked fixture contract for the N2 PDF engine qualification spike.

Do not add arbitrary personal PDFs here. Every committed fixture must have deliberate provenance, redistribution permission, a checksum, and an expected-behavior entry.

## Goals

The corpus exists to make Qt PDF, PDFium and qpdf experiments comparable and repeatable across developers/CI.

A small discriminating corpus is preferred over a large undocumented dump.

## Required fixture manifest fields

Every committed PDF must have a corresponding manifest entry containing:

```text
id                  stable Atlas fixture ID
file                repository-relative filename
sha256              SHA-256 of exact bytes
source              origin/project or "synthetic-atlas"
source_url          public origin when applicable
license             redistribution license/permission
created_by          tool/script when synthetic
purpose             capability under test
expected_pages      expected page count when known
expected_labels     expected page labels when relevant
expected_text       only short non-sensitive expected strings/hashes
expected_outline    summary or companion expected-data file
password_fixture    yes/no
password_reference  test-only secret source; never a personal password
mutation_allowed    yes/no
notes               known ambiguity/limitations
```

Additional fields may narrow fixture scope. For example, `visual_fidelity_fixture: false` explicitly prevents a semantic-only PDF from being cited as rendering/shaping evidence.

## Minimum fixture classes

N2 should establish fixtures for:

1. simple one-page text;
2. multi-page page labels;
3. Arabic-only searchable/extractable Unicode;
4. mixed Arabic/English text;
5. Arabic/English outline hierarchy;
6. duplicate outline titles under different parents;
7. deep outline hierarchy;
8. internal links/destinations;
9. external URI link;
10. rotated pages;
11. mixed page sizes;
12. non-default CropBox/MediaBox;
13. image-only page;
14. existing standard annotations;
15. metadata/XMP-rich PDF;
16. open-password encryption;
17. permission-restricted encryption;
18. malformed-but-readable PDF;
19. signed/certified test document when redistribution allows;
20. long document for timing/memory loops.

One PDF may cover several classes when doing so does not make the expected result ambiguous.

## A006 — Unicode semantics, not visual shaping

`A006_unicode_semantics.pdf` is deliberately **font-free at the external dependency level**. Its generator builds a synthetic Type3 font and a deterministic ToUnicode CMap with pypdf 5.9.0.

Its purpose is to isolate logical Unicode behavior for:

- Arabic extraction;
- Arabic combining marks/tashkīl;
- mixed Arabic + English content on one page;
- Urdu extraction;
- Unicode outline titles/hierarchy;
- engine search semantics.

The visible Type3 glyphs are simple synthetic rectangles. They do not resemble Arabic or Urdu letters and therefore provide **zero evidence** about shaping, joining, typography, fallback, or raster fidelity.

This separation is intentional: a text/search engine should first prove that it preserves the correct logical Unicode mapping without the result being confounded by HarfBuzz/font/shaping differences.

A later visual-shaping fixture may embed an open font only after its exact upstream revision, redistribution license, file/archive SHA-256, shaping/generator versions, and source are documented. Never substitute a system font and call the fixture reproducible.

A006 generator:

`tests/fixtures/pdf/source/generate_a006_unicode_semantics.py`

Expected SHA-256:

`1b69b6ce646ad3052d0a8f967efb4d2540569641e3f946ca8e533d9c9655d019`

## Public versus private fixtures

### Public tracked fixtures

May be committed only when redistribution is clearly allowed. Record exact provenance and license.

### Synthetic Atlas fixtures

Preferred where possible. Commit the generator/source inputs alongside the PDF when practical so expected structure is reproducible.

The generator must not depend on proprietary desktop software merely to recreate a core CI fixture.

### Private owner fixtures

Useful for real-world validation, especially Arabic research PDFs, but **must not be committed** unless redistribution rights are confirmed.

For private fixtures:

- keep bytes outside Git;
- use a local manifest mapping a non-sensitive fixture ID to the private path;
- never put personal paths, document titles, extracted text, passwords, or hashes intended to identify private material into public CI logs;
- benchmark outputs should refer to sanitized fixture IDs;
- mutation tests operate on disposable copies.

## Password fixtures

Passwords must be synthetic/test-only values. Never use or request a personal document password for a committed test.

A tracked encrypted fixture may use a plainly documented test password in a dedicated test-data file if that password has no security value outside the fixture.

Do not print passwords in ordinary test logs.

## Mutation safety

A fixture manifest must explicitly say whether mutation is allowed.

N2 mutation probes:

- work on temporary copies;
- never overwrite the source fixture in place;
- retain before/after SHA-256;
- validate output after writing;
- compare preservation invariants;
- reopen through an independent implementation where practical.

Signed/certified fixtures are inspection fixtures by default. If a mutation experiment is intentionally performed on a signed test copy, the expected invalidation/consequence must be documented and must never be presented as preserved signature integrity.

## Expected-data policy

Do not make one engine the oracle for another.

Expected page count, labels, boxes, outlines and text should come from fixture construction/source specification or independent inspection where possible.

When two engines disagree and the PDF specification/fixture intent does not make the correct interpretation obvious, record the case as **fixture ambiguity** until independently resolved.

## Raster/golden evidence

Golden images can help detect regressions, but byte-identical raster output is not required across different engines.

If golden comparison is used, record:

- requested pixel dimensions;
- page box and rotation assumptions;
- background/alpha handling;
- color-space expectations;
- tolerance/diff method;
- independent visual inspection for material differences.

A visually wrong render cannot pass merely because it is fast.

A006 is explicitly excluded from visual/golden evidence because its glyphs are synthetic semantic markers.

## Checksums

Use SHA-256 for fixture identity and produced mutation artifacts.

If fixture bytes change intentionally:

1. explain why in the commit;
2. update the checksum;
3. rerun every N2 result that depended on those exact bytes;
4. do not silently reuse old benchmark numbers.

## Repository layout target

```text
tests/fixtures/pdf/
├─ README.md
├─ manifest.json
├─ generated/                    # redistributable synthetic fixtures
├─ source/                       # source/generator inputs
└─ expected/                     # normalized expected records/golden metadata
```

Directories need not be created empty in Git. They appear as fixtures are added.

## Qualification linkage

Every result added to `docs/baselines/N2_PDF_ENGINE_MATRIX.md` should name the fixture IDs and exact engine revision/version used.

N2 cannot be accepted with undocumented PDFs as the sole evidence for a core capability.
