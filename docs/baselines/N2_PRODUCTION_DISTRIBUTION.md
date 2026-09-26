# N2 PDF Production Acquisition / Distribution Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Open — PDFium/qpdf routes proposed; implementation/compliance freeze and requalification pending**
**Opened:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This document separates **probe acquisition** from **production distribution**. A package being reproducible enough for qualification does not automatically approve it for shipping.

This is engineering/compliance planning, not legal advice. Final release compliance should be reviewed against the licenses actually chosen and the exact binaries/notices shipped.

## 1. Cross-cutting release rules

Any PDF responsibility selected for Atlas v2 must have all of the following before N2 can close:

1. exact upstream version/revision/tag;
2. reproducible acquisition/build path;
3. archive/source checksum or immutable commit identity;
4. documented runtime files and package footprint;
5. license files and third-party attribution/NOTICE material staged with the release;
6. an SBOM or equivalent machine-readable dependency inventory where available;
7. security-update procedure;
8. rollback procedure to the previously qualified pin;
9. no candidate-native ABI/type leakage across the Atlas portable boundary;
10. a clear owner for keeping notices and dependency metadata current when upgrading.

## 2. Qt PDF production route

### Qualified probe identity

- Qt: **6.10.3**;
- kit: MSVC 2022 x64;
- module: `qtpdf` / `Qt6::Pdf`;
- acquisition during N2: official Qt module installation used by CI.

### Upstream license surface

Qt PDF is offered under Qt commercial terms and under open-source terms including LGPLv3/GPLv2 for the module. The module includes a PDFium snapshot and additional third-party components, so Qt's own license choice does not replace those third-party obligations.

Qt 6.10.3's published Qt PDF third-party list includes at least:

- Abseil — Apache-2.0;
- FreeType — FTL;
- PDFium — BSD;
- Chromium Project code — BSD-3-Clause;
- fast_float — MIT;
- HarfBuzz — MIT-Modern-Variant;
- ICU — MIT;
- libjpeg-turbo — IJG/BSD-3/Zlib;
- libpng — libpng licenses;
- zlib — zlib license.

Qt 6.8+ publishes SPDX SBOM information for third-party components. If Atlas ships Qt PDF, the release pipeline should preserve the applicable Qt/PDF third-party license material and SBOM data for the exact Qt release.

Official references:

- https://doc.qt.io/qt-6.10/licensing.html
- https://doc.qt.io/qt-6.10/licenses-used-in-qt.html
- https://doc.qt.io/qt-6/qtpdf-licensing.html
- https://www.qt.io/development/open-source-lgpl-obligations

### Proposed shippable route if Qt PDF is selected

Use the **official Qt 6.10.3 dynamic Windows distribution** already compatible with Atlas's Qt application stack. Do not create a private Qt PDF/PDFium fork for N2.

Release work required:

- freeze the exact Qt installer/package provenance used for release builds;
- record the actual `Qt6Pdf` runtime files added by deployment;
- include the applicable Qt license text and third-party attributions;
- if Atlas uses the LGPL route, preserve the relinking/source-offer obligations applicable to the Qt libraries in the actual distribution;
- if Atlas uses a commercial Qt license, keep the commercial entitlement path separate from third-party open-source notices still required by bundled components;
- include exact Qt 6.10.3 SBOM material in the release compliance bundle;
- record the update/rollback pair as `current-qualified Qt pin` ↔ `previous-qualified Qt pin`.

### Current status

**PASS WITH LIMITATION** as a technically shippable route. Final Atlas license choice, exact release-file inventory, source/SBOM/notice bundle and package-size delta remain to be frozen.

## 3. Standalone PDFium production route

### Qualified probe identity

Current N2 probe package:

- PDFium version: `156.0.8066.0`;
- branch/pin: `chromium/8066`;
- distributor: `bblanchon/pdfium-binaries`;
- asset: `pdfium-win-x64.tgz`;
- asset ID: `579031518`;
- compressed size: `3,823,498` bytes;
- SHA-256: `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`.

The distributor repository is an excellent reproducible **qualification accelerator**, but its MIT repository license is not a substitute for auditing the license/notices of the PDFium binary and bundled dependencies.

### Upstream source/build surface

PDFium's upstream license file permits redistribution in source/binary form under BSD-style conditions and also contains licenses/notices for additional included code. Binary redistribution must reproduce the applicable notices in documentation/materials.

Official PDFium development uses Chromium tooling: `depot_tools`, `gclient`, GN and Ninja rather than Atlas's ordinary CMake/MSVC-only dependency flow.

Official references:

- https://pdfium.googlesource.com/pdfium/+/refs/heads/main/LICENSE
- https://pdfium.googlesource.com/pdfium/+/refs/heads/main/README.md

### Proposed shippable route if standalone PDFium is selected

Do **not** promote the community qualification archive to production by default.

Preferred production route:

1. pin an immutable upstream PDFium/Chromium revision corresponding to the selected tested branch;
2. fetch through a documented `depot_tools`/`gclient` manifest;
3. build in CI with GN/Ninja using an explicit minimal feature profile;
4. unless a product requirement proves otherwise, prefer a smaller non-V8/non-XFA configuration rather than enabling script/form surfaces by default;
5. archive the exact GN args and compiler/toolchain identities;
6. produce the Atlas-owned Windows DLL/import-lib package;
7. generate and ship a complete license/NOTICE/SBOM bundle from the exact source tree;
8. record DLL and dependency hashes in release metadata;
9. re-run the N2 correctness/fidelity/coordinate/stress suite against the production-built DLL before replacing the qualification package.

The existing PDFium API serialization rule remains mandatory regardless of packaging route.

### Current status

**PASS WITH LIMITATION** as a candidate production route. The probe binary is reproducible and functionally qualified, but an Atlas-controlled source-build pipeline, notice/SBOM bundle, runtime dependency inventory and production-built-binary requalification are still pending if standalone PDFium is selected.

## 4. qpdf production route

### Qualified identity

- qpdf: **12.4.1**;
- tag: `v12.4.1`;
- first-party asset: `qpdf-12.4.1-msvc64.zip`;
- release asset ID: `533019080`;
- compressed size: `28,165,367` bytes;
- SHA-256: `3cd016cd433ef7232e42f4c13348a49cc14907a3c7278ef4f99120593126f7a6`.

Current N2 structural/security/transformation evidence qualifies the **CLI route**, not a linked `libqpdf` production integration.

qpdf's primary license is Apache-2.0. Current upstream Windows builds support MSVC/CMake; recent qpdf Windows releases use OpenSSL as the default crypto provider while the native provider is also compiled in and selectable.

Official references:

- https://github.com/qpdf/qpdf/releases/tag/v12.4.1
- https://qpdf.readthedocs.io/en/stable/release-notes.html
- https://qpdf.readthedocs.io/en/latest/installation.html

### Proposed shippable route if qpdf owns structural mutation

Keep the **first-party qpdf CLI process boundary** for Atlas v2 unless a measured product need requires linked-library integration.

Reasons this remains the lower-risk N2 route:

- it is already the exact route used by the transformation/security qualification;
- qpdf failures/crashes are process-isolated from the Atlas UI process;
- the executable can be upgraded/rolled back as an independently hashed component;
- Atlas avoids exposing qpdf C++ ABI/types inside its application/domain layers;
- CLI JSON/check output is already part of the normalized evidence path.

Release work required:

- inspect/stage only the runtime files actually required from the official MSVC64 package rather than copying the full 28.2 MB archive blindly;
- preserve qpdf Apache-2.0 and bundled dependency notices;
- record the chosen crypto-provider/runtime DLL set;
- record hashes for `qpdf.exe` and every shipped companion DLL;
- include `qpdf --version`/pin verification in the release smoke test;
- keep the A003/A006 preservation and A009/A010 security-interlock suite as an upgrade gate.

A future switch to linked `libqpdf` is a **new distribution/build baseline**, not an invisible implementation detail.

### Current status

**PASS WITH LIMITATION** for the first-party CLI distribution route. The exact minimal runtime file set and final notice/SBOM bundle still need to be frozen before release packaging.

### Tested minimal Windows runtime candidate

On 2026-09-27, the following self-contained subset from the exact official
12.4.1 MSVC64 archive ran `qpdf --version`, strict A015 checking, and all six
Atlas qpdf structural/permission/signature/title-mutation/outline-breadth/
encrypted-write probes successfully. Total size is **9,104,336 bytes**.

| File | Bytes | SHA-256 |
|---|---:|---|
| `qpdf.exe` | 19,456 | `57c003e868fb66cd343fd5afb91be8c2277f56434eea8635762499731bf9f60d` |
| `qpdf30.dll` | 7,438,848 | `36fe5b2023f244e5785d96b8372dba26b75ea51b63adf4d4f1e66ad0aa1c8a61` |
| `concrt140.dll` | 374,200 | `54716f0738af891f283d213b5c8d11b25896bb8ee3097d301eae718560cf974e` |
| `msvcp140.dll` | 643,512 | `7c26614e1d733892c2deac7e245ce115504b1d80592dd0a01b08e3e5a55f89ca` |
| `msvcp140_1.dll` | 35,768 | `206c931bf90fdad8816de3b5e2ef80b2bcaa9406c89ecc05fe6fddffe251e982` |
| `msvcp140_2.dll` | 274,872 | `d50d7883f20d1dc6191768d3746f52dd9cac89c346ffaed5be1f110c2f34a838` |
| `msvcp140_atomic_wait.dll` | 57,792 | `3d0cbfaa1bf3eecf5a3f4491d2960ee803cb994f30292c6adc4a07c498f60e2b` |
| `msvcp140_codecvt_ids.dll` | 31,160 | `8a65c7596ef2e6938731f5a1058e7e40145b6d97967cc649231a076b9a608d78` |
| `vcruntime140.dll` | 178,616 | `d1f4225df2cd877dbf130d5668a021dce3f94118455ff5ec952061c30afc9ce7` |
| `vcruntime140_1.dll` | 50,112 | `a7146c08f89fe5b04541ab507cdb59ff7b44534d4ba3c668a426c6450a03434e` |

This is a technical runtime candidate, not the completed compliance freeze.
Final CI must reproduce this staging, preserve the applicable qpdf and Microsoft
runtime license material, and emit the release notice/SBOM bundle.

## 5. Security/update ownership

For every selected component, Atlas should maintain a small dependency record containing:

- upstream project;
- exact pin;
- acquisition source;
- checksums;
- build arguments where applicable;
- license/notice bundle path;
- CVE/security advisory source;
- date last reviewed;
- previous qualified pin;
- rollback command/process;
- qualification workflows that must pass before promotion.

An upgrade is not complete merely because the dependency builds. It must run the relevant N2 regression slices again against the candidate pin.

## 6. Current decision boundary

ADR-0004 now proposes standalone PDFium for read/render/text/navigation and the
first-party qpdf CLI for structural/security/write responsibilities. This
document defines what must still be frozen before that proposal can be accepted.

What is now clear:

- Qt PDF has a plausible official-module shipping route, but Atlas must freeze its Qt license/compliance/SBOM packaging for the exact release;
- standalone PDFium has a plausible production route, but Atlas should own a pinned source-build pipeline rather than silently promoting the community qualification binary;
- qpdf has a strong first-party CLI shipping route already aligned with the tested transformation boundary.

## 7. Remaining production-distribution evidence

Before final N2 acceptance:

1. freeze the proposed PDFium upstream source revision/build configuration and runtime-file inventory;
2. build and requalify that Atlas-controlled PDFium binary;
3. measure the selected runtime/package footprint in the Atlas portable package;
4. stage the exact notice/license/SBOM bundle in CI;
5. document security-update monitoring and rollback mechanics;
6. freeze qpdf's minimal runtime file/dependency set;
7. run final exact-head CI with only the selected production routes treated as release dependencies.

N2 remains **Open** and ADR-0004 remains **Proposed**.
