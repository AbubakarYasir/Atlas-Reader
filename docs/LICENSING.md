# Licensing and Dependency Guardrails

This document is engineering guidance, not legal advice. Perform a release-specific license review before public binary distribution.

## Atlas source

Planned project license: **MIT**.

Atlas source licensing does not override the obligations of libraries/tools bundled with release artifacts.

## Qt

Qt is offered under commercial licenses and open-source LGPLv3/GPLv3 terms depending on module. Some Qt modules are GPL-only in the open-source distribution.

Engineering rules:

- record every Qt module used;
- prefer modules available under LGPLv3 or otherwise compatible with the chosen Atlas distribution model;
- do not accidentally add a GPL-only module without an explicit licensing decision;
- do not assume “Qt is LGPL” applies uniformly to all modules/tools;
- when distributing LGPL Qt, satisfy applicable relinking/replacement, notice, license-text, source/offer requirements as required by the final packaging model;
- avoid static-linking decisions until obligations are reviewed; dynamic deployment is the default engineering assumption;
- record Qt version/module inventory in release evidence;
- review Qt third-party notices shipped inside chosen modules.

Bootstrap modules: Core, Gui, Qml, Quick, QuickControls2. Any additional module receives license review before becoming a production dependency.

Qt tools used only during development may have different terms from runtime modules; do not conflate tool licensing with application-library licensing.

## qpdf

Evaluation baseline: qpdf 12.4.1. Current qpdf is Apache License 2.0.

If distributed:

- preserve required notices/license;
- record exact version/source;
- review bundled/transitive components from the actual distribution method;
- isolate behind Atlas PDF infrastructure interfaces.

## SQLite

SQLite core is published in the public domain. Record exact source/binary provenance and version anyway for reproducibility/SBOM/security maintenance.

Do not silently substitute proprietary SQLite extensions.

## PDFium

N2.2 currently uses a **probe-only** Windows x64 prebuilt from `bblanchon/pdfium-binaries`:

- PDFium `156.0.8066.0` / tag `chromium/8066`;
- distributor source commit `f2e9a1c45bb17b85b540abf1af30146ef65416ac`;
- archive SHA-256 `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`;
- exact provenance: `docs/baselines/N2_PDFIUM_PROVENANCE.md`.

The pinned distributor repository itself is MIT-licensed. **That MIT license applies to the distributor repository's own code/packaging material; it is not a substitute for PDFium/Chromium third-party notices or a production distribution clearance.** The N2 engineering artifact may carry `pdfium.dll` solely as qualification evidence; it is not a user release.

If PDFium is selected in N2 for production responsibility:

- decide explicitly whether Atlas builds official/mirrored upstream source, ships a pinned third-party binary distribution, or uses another vetted route;
- pin the exact PDFium/Chromium revision and build configuration;
- audit PDFium/Chromium third-party notices and bundled codecs/fonts/libraries for that exact build;
- generate/ship required third-party notices;
- record update, rollback, security-response and cross-platform provenance;
- treat build configuration as part of license/security reproducibility.

A “BSD-style” summary or the distributor repository's MIT license is not sufficient release documentation.

## Candidate support libraries

Expected current licenses (re-verify exact versions before distribution):

| Component | Expected license | Runtime? |
|---|---|---:|
| Catch2 | Boost Software License 1.0 | Test only |
| Google Benchmark | Apache-2.0-style/per upstream project; verify pinned release | Benchmark only |
| nlohmann/json | MIT | Possible runtime/header dependency |
| spdlog | MIT (and fmt-related notices where applicable) | Possible runtime |
| xxHash | BSD-2-Clause | Possible runtime |
| Tracy | BSD-3-Clause | Development/profiling only unless explicitly configured otherwise |
| RenderDoc | MIT | Development only |
| Accessibility Insights for Windows | upstream Microsoft open-source terms | Development/manual QA only |

The table is a planning aid, not permission to skip release-time verification.

## Dependency manager and SBOM

vcpkg manifest mode is planned for non-Qt native dependencies from N2. Pin a baseline and preserve lock/revision data.

Release builds generate:

- exact dependency inventory;
- SPDX/CycloneDX or equivalent SBOM where practical;
- third-party notices bundle;
- source/project URLs;
- applicable license texts;
- Qt deployment/module inventory.

Potential SBOM generators may include Syft or another open tool, but the specific generator is chosen/qualified before release packaging and must not upload private project/user data.

## Packaging/installer licensing

Installer tooling is selected in N11, not assumed now.

WiX remains a technical candidate but current WiX project licensing/maintenance-fee terms must be reviewed at decision time. CPack/NSIS/other options remain candidates. Do not commit the project to installer tooling solely because it is popular.

## GPL/AGPL/proprietary dependencies

A dependency with GPL/AGPL/proprietary terms is not automatically forbidden, but it requires an explicit owner/legal/distribution decision before inclusion.

In particular, do not casually add a PDF library with copyleft/commercial dual licensing to solve a performance issue. First determine whether Qt PDF/PDFium/qpdf or Atlas-owned code can satisfy the requirement.

## Fonts/assets

Do not commit/distribute fonts, icons, sample PDFs, screenshots, or test fixtures unless redistribution rights are known.

- prefer system fonts or properly licensed bundled fonts;
- record fixture provenance/license;
- personal/private PDFs remain opt-in local fixtures and are never committed;
- competitor assets/UI screenshots are reference material, not distributable product assets unless permission/license permits.

## Dependency acceptance checklist

Before adding a production dependency:

- exact problem/purpose documented;
- exact version/revision documented;
- upstream project/release source documented;
- license verified from upstream source;
- transitive notices understood;
- supported target platforms verified;
- maintenance/security response assessed;
- update/rollback mechanism known;
- binary/startup/runtime impact measured where relevant;
- abstraction boundary prevents vendor lock-in when practical;
- release-notice/SBOM plan updated.

## Release artifacts

Windows RC/stable must contain or provide as legally appropriate:

- Atlas MIT license;
- third-party notices/licenses;
- exact dependency versions/revisions;
- required source/project links/offers;
- applicable Qt LGPL compliance materials/instructions;
- SBOM/dependency inventory;
- checksum and build provenance.

Do not postpone license inventory until installer week.