# N2 PDFium Provenance and Probe Pin

**Checkpoint:** N2.2 — PDFium qualification  
**Status:** Core smoke captured — broader qualification pending; not a production dependency decision  
**Branch:** `native-v2-n2-pdf-engine-qualification`  
**Recorded:** 2026-09-22

## Purpose

This file freezes the exact PDFium package used for the first N2.2 Windows probe so CI evidence can be reproduced later. It does **not** approve this community binary distribution for production shipping. ADR-0004 remains Proposed until N2 is explicitly passed.

## Core evidence binding

The first passing PDFium core smoke is bound to implementation head:

`7874794e14b9cea54ec0723c15963621f65bebf6`

GitHub Actions run `35680738681` passed Debug and Release. The Release engineering artifact has ID `10674712819` and digest:

`sha256:0972d8424b6d35c8f2c8c1612ddbea60b34f7038b59c5d8c5643ab37bfad4803`

Because this was a pull-request-triggered run, GitHub checked out a synthetic merge commit and `${{ github.sha }}` was `df4f45e1a301508b81a4389a90b2d00d156730b6`; the workflow run's actual branch head is the implementation SHA above. Future artifact metadata must record implementation head and checkout SHA separately.

A001–A005 passed strict PDFium tests. On the current English corpus, PDFium and Qt PDF returned identical page labels, normalized visible sizes, and page-by-page UTF-8 text SHA-256 values. A001 also returned a non-null 612×792 raster. Search, bookmarks, links/destinations, Arabic/Urdu/combining-mark semantics, password/security, malformed-input behavior, raw page-box/rotation decomposition, raster fidelity, and repeatable performance remain pending.

## Upstream API constraint

Current PDFium public embedder headers state that PDFium APIs are **not thread-safe**. Calls are expected from one thread, or the embedder must serialize calls so only one PDFium API call is active at a time.

Atlas therefore treats serialization as part of the candidate contract. N2 must not benchmark an unsupported parallel-call pattern and then claim that as representative PDFium behavior.

The current probe records `serialized-single-thread` and makes PDFium calls serially. A later Atlas adapter/task-queue stress test is still required to prove concurrent application workloads never create simultaneous PDFium API calls.

## Probe distribution

For the first Windows x64 N2.2 probe, Atlas uses the prebuilt non-V8 package published by `bblanchon/pdfium-binaries`.

This project is a community binary distribution and explicitly states that it is not affiliated with Google or Foxit. It provides shared/dynamic PDFium libraries and CMake integration for consumers.

| Field | Pin |
|---|---|
| Distribution repository | `bblanchon/pdfium-binaries` |
| Release tag | `chromium/8066` |
| Release name | `PDFium 156.0.8066.0` |
| Distribution source commit | `f2e9a1c45bb17b85b540abf1af30146ef65416ac` |
| Release published | `2026-09-21T12:48:25Z` |
| Windows asset | `pdfium-win-x64.tgz` |
| Asset ID | `579031518` |
| Asset size | `3823498` bytes |
| Asset SHA-256 | `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020` |
| V8 enabled | No |
| Intended scope | N2 probe only |

The release also publishes an attestation artifact (`pdfium-attestation.json`), but Atlas does not treat the existence of that file alone as production supply-chain approval.

## Licensing scope

The pinned `bblanchon/pdfium-binaries` repository itself carries the MIT License at the pinned distribution commit. That describes the distributor repository's own code/packaging material; it is **not** sufficient evidence that distributing the bundled PDFium binary requires only that MIT notice.

PDFium and its bundled third-party components have their own upstream licenses/notices. Before any production selection, Atlas must inventory the exact notices supplied or required for the pinned engine build and determine the distribution obligations for the platforms Atlas intends to ship. Until that audit is recorded in `docs/LICENSING.md` and the final ADR, the community binary route remains probe-only.

## Acquisition rule

Qualification CI must download the **tagged** asset URL for `chromium/8066`, calculate SHA-256 locally, and fail before extraction if it does not equal:

`739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`

Do not use `/releases/latest/` in accepted N2 evidence.

Run `35680738681` demonstrated this acquisition rule in both Debug and Release lanes before configure/build.

## Integration rule

The community distribution documents CMake consumption through `find_package(PDFium)` after setting `PDFium_DIR` to the extracted package. On Windows, `pdfium.dll` must be placed beside the probe executable or otherwise be on `PATH`.

For N2:

- PDFium links only to a focused `atlas_pdfium_probe` target;
- `atlas_reader` and `atlas_core` remain free of PDFium linkage;
- PDFium handles/types must not cross into Atlas domain/application contracts;
- CI copies the runtime DLL only for the probe/evidence artifact;
- all PDFium calls in Atlas-owned probe/adapter code remain serialized;
- replacing/removing PDFium must not require rewriting the product shell.

The current probe artifact contains `pdfium.dll` at 7,380,992 bytes. This is useful raw footprint evidence but is **not** yet a production package-size delta because the engineering artifact intentionally duplicates probe/runtime files.

## Production caveat

Before PDFium could be selected for production, N2 must separately decide whether Atlas should:

1. build PDFium from official upstream source with Chromium tooling,
2. consume a pinned third-party binary distribution,
3. use another packaging route,
4. or not ship PDFium at all.

That decision must include license/notices, provenance, update/rollback mechanics, cross-platform parity, runtime footprint, and security maintenance. This probe pin answers none of those questions by itself.

## Evidence links inside the repository

- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md`
- `docs/DEPENDENCIES_AND_TOOLS.md`
- `docs/LICENSING.md`
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md`
