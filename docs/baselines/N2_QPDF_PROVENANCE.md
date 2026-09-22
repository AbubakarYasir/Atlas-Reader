# N2 qpdf Qualification Provenance

**Checkpoint:** N2 — structural/security/transformation qualification  
**Status:** Pinned qualification dependency; production integration decision pending  
**Recorded:** 2026-09-22

## Exact qualification pin

Atlas N2 uses the first-party qpdf **12.4.1** Windows MSVC 64-bit release asset for the initial structural/security CLI qualification.

- Upstream repository: `qpdf/qpdf`
- Release tag: `v12.4.1`
- Release name: `qpdf version 12.4.1`
- Release ID: `378185910`
- Published: `2026-08-27T23:56:39Z`
- Asset: `qpdf-12.4.1-msvc64.zip`
- Asset ID: `533019080`
- Asset size: `28,165,367` bytes
- Asset SHA-256: `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6`
- Upstream release also publishes `qpdf-12.4.1.sha256` and a Sigstore artifact; N2 still verifies the selected ZIP directly against the release API digest before extraction.

No floating `latest` identifier is used anywhere in the canonical qualification workflow.

## Acquisition route

The N2 Windows qualification downloads the exact official GitHub release ZIP above, verifies its SHA-256 before extraction, recursively locates the packaged `qpdf.exe`, and then requires `qpdf --version` to report `12.4.1` before any fixture is processed.

This first qpdf slice intentionally qualifies the **official CLI distribution**, not a package-manager reconstruction and not a locally built qpdf library. Therefore:

- a vcpkg baseline/override is **N/A for this CLI evidence slice**;
- if Atlas later links qpdf as a production library, that library acquisition/build route must receive its own exact baseline, compiler configuration, transitive dependency and checksum/provenance record before N2 can accept it for shipping.

## License surface

The `v12.4.1` upstream `LICENSE.txt` is Apache License 2.0. This records qpdf's primary upstream license for qualification purposes only.

A production distribution decision still requires review of the exact binary package's bundled third-party notices/runtime dependencies and Atlas's notice/redistribution obligations. Passing the N2 CLI probe does not itself approve production redistribution.

## Intended Atlas role

qpdf is **not** being introduced as a competing page renderer in this checkpoint. Its intended N2 responsibilities are structural/security inspection and controlled transformation/validation, especially where Atlas must reason about PDF object structure or preserve unrelated content while modifying portable research metadata.

The initial evidence slice covers:

1. structural check/JSON inspection on existing deterministic fixtures;
2. encrypted/password-state inspection on A008;
3. malformed-input diagnostics on A007;
4. no-op rewrite and independent semantic-preservation checks on A003/A006;
5. qpdf re-check and independent pypdf reopen of rewritten outputs.

Controlled outline mutation is a separate follow-up slice. qpdf JSON v2 supports bidirectional PDF/JSON and object-level updates, but Atlas will not mutate outline objects until the exact v12.4.1 object layout has been captured from our fixtures and explicit preservation assertions are in place.

## Rollback / replaceability

The qualification integration is deliberately isolated from `atlas_reader` and `atlas_core`. Removing the qpdf workflow/probe/provenance files removes the dependency without changing the product shell or the already-qualified Qt PDF/PDFium probes.
