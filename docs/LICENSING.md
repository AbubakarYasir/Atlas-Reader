# Licensing and Dependency Guardrails

This document is engineering guidance, not legal advice. Perform a license review before public binary distribution.

## Atlas source

Planned project license: MIT.

## Qt

Qt uses different licenses across modules and distribution models. The open-source Qt modules selected for Atlas must be reviewed individually.

Engineering rules:

- prefer modules available under terms compatible with Atlas's open-source distribution plan;
- do not add a GPL-only module accidentally to a build intended for different distribution terms;
- keep Qt dynamically linked where that simplifies LGPL compliance and user replacement/relinking requirements;
- ship required notices/license texts with binaries;
- record every added Qt module in the dependency/license manifest before release.

The bootstrap uses Core, Gui, Quick, Qml, and QuickControls2 only.

## qpdf

Planned evaluation baseline: qpdf 12.4.1. qpdf uses the Apache License 2.0 in current releases. Preserve notices/license obligations when distributed.

## SQLite

SQLite is generally published in the public domain, but the exact distributed source/binary provenance must still be recorded.

## PDFium and other candidates

If PDFium is selected, review Chromium/PDFium third-party notices and packaging obligations as part of N2. Do not treat "BSD-like" summaries as sufficient release documentation.

## Dependency acceptance checklist

Before adding a production dependency:

- purpose is documented;
- license is documented from upstream source;
- transitive notices are understood;
- supported target platforms are verified;
- update/security mechanism is known;
- binary size/startup impact is measured where relevant;
- abstraction boundary prevents vendor lock-in when practical.

## Release artifacts

Windows release candidate must contain:

- Atlas license;
- third-party notices/licenses;
- exact dependency versions;
- source/project links required by licenses;
- instructions needed for applicable relinking/replacement rights.

Do not postpone license inventory until the installer checkpoint.