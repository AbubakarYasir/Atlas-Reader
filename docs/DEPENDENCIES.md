# Atlas Reader — Dependencies, Tools and Licensing

Atlas reuses mature open-source work when it clearly improves reliability or speed, but third-party code never controls the product architecture or data. This is engineering guidance, not legal advice; a release-specific license review happens before every public binary.

## 1. What ships today

| Component | Version / pin | Role | License | Boundary |
|---|---|---|---|---|
| Qt (Core, Gui, Qml, Quick, QuickControls2, Pdf) | 6.10.3, dynamic | UI, platform, PDF reading | LGPLv3 (per module; verify each) | Presentation/infrastructure; Qt PDF only inside adapters |
| qpdf CLI | 12.4.1, first-party build route | PDF structure/security/writes (used from N5) | Apache-2.0 | PDF infrastructure adapter |
| SQLite + FTS5 | 3.53.4#1, vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, `x64-windows-static-md` | Index, search, local state | Public domain | Repository layer only |
| Noto Nastaliq Urdu, Noto Naskh Arabic | pinned files, see `assets/fonts/README.md` | Urdu interface; Arabic/Urdu content | OFL-1.1 | Loaded at startup from bundled resources |
| Fluent UI System Icons | commit `9eb184e9`, 40 SVGs, see `assets/icons/README.md` | Interface icons | MIT | Tinted at runtime; one family only |

The production distribution route, notices and SBOM are recorded in [N2_PRODUCTION_DISTRIBUTION](baselines/N2_PRODUCTION_DISTRIBUTION.md) and [ADR-0004](decisions/ADR-0004-pdf-engine-responsibilities.md). **PDFium** (`156.0.8066.0`, community `bblanchon/pdfium-binaries` build, see [provenance](baselines/N2_PDFIUM_PROVENANCE.md)) is qualification and comparison evidence only. A standalone `pdfium.dll` must never ship; the review packager enforces this.

## 2. Admission rule

Before adding anything to production, answer and record:

1. What exact problem does it solve better than a small Atlas implementation or Qt/the standard library?
2. Is it maintained and usable on Windows now and Android/Linux/macOS/iOS later?
3. Is its license compatible, including transitive notices, fonts and codecs? (Free of charge does not mean open source. GPL/AGPL/proprietary terms need an explicit owner decision.)
4. Can it sit behind an Atlas-owned interface?
5. What does it cost in binary size, CPU, memory and startup?
6. What happens if it's abandoned or relicensed? What's the exit?
7. Can its behavior be tested with our fixtures, rather than trusted from its docs?

Then: pin the exact version and source, add it through vcpkg manifest mode (Qt stays separately installed), add fixtures/tests/benchmarks for its job, update notices and SBOM, and write an ADR if it constrains architecture or data formats. "Added to a probe" and "accepted for production" are different states. Never curl a floating "latest" binary into CI.

## 3. Candidates (not yet added)

| Candidate | Purpose | License | When |
|---|---|---|---|
| nlohmann/json | Versioned bookmark/backup/annotation JSON | MIT | N5 |
| Catch2 v3 | Portable unit/contract tests | BSL-1.0 | When Qt Test is awkward for pure core tests |
| Google Benchmark | Microbenchmarks | Apache-2.0 | N4 reader benchmarks, if the fixture runner isn't enough |
| xxHash | One fast identity/cache signal | BSD-2-Clause | Only if identity/cache work needs it |
| Noto Sans Arabic UI | Arabic UI font, if Segoe's Arabic loses the rendered comparison | OFL-1.1 | Only if needed |
| spdlog | Structured logging | MIT | Only if Qt logging proves insufficient |
| Syft (or similar) | SBOM generation | Apache-2.0 | N10 |

**Explicitly not adopted:** Electron/Tauri/WebView UI, proprietary PDF SDKs, MuPDF (needs a separate licensing decision), telemetry or crash-upload SDKs, OCR engines, AI document SDKs, Office conversion stacks.

## 4. Licensing rules

- **Atlas source:** MIT. It doesn't override bundled libraries' obligations.
- **Qt:** record every module used. Prefer LGPLv3 modules and never add a GPL-only module without a licensing decision. Deploy dynamically. Ship license texts, notices, source/relinking information and a Qt module inventory. Tool licenses (Qt Creator etc.) are separate from runtime library licenses.
- **qpdf:** keep notices and record the exact version and source.
- **SQLite:** public domain; still record provenance for the SBOM and security maintenance. No proprietary extensions.
- **PDFium (if ever reconsidered):** a pinned official build route, an audit of Chromium third-party notices, codecs and fonts, and a new ADR. The distributor repository's MIT license is not PDFium's notice set.
- **Fonts, icons, samples, screenshots:** commit or ship only with known redistribution rights. Private PDFs never enter Git. Competitor screenshots are reference material only.
- **Brand mark:** the owner-selected mark in `assets/branding` (2026-09-27); see its README for checksums and the no-modification rule.

**Every RC/stable ships:** the Atlas MIT license, third-party licenses and notices, exact dependency versions, required source links or offers, Qt LGPL compliance material, an SBOM (SPDX or CycloneDX), checksums and build provenance. License inventory never waits until installer week.

## 5. Development-only tools

These never become runtime dependencies.

| Tool | Use |
|---|---|
| Qt QML Profiler | Bindings, QML creation, frame timing |
| Tracy | C++ zones, queues, locks, thread timing |
| RenderDoc | GPU frames, textures, overdraw |
| WPR/WPA, Visual Studio profiler | Startup, I/O, scheduling, memory, native CPU |
| Accessibility Insights for Windows + Narrator | UI Automation checks and manual screen-reader acceptance |
| clang-format, clang-tidy, clang-cl, sanitizers, CodeQL | Format, static analysis, memory diagnostics, security scanning |
| CodeGraph (`.codegraph/`, ignored) | Symbol/caller/impact navigation for agents |
| GitHub Actions, secret scanning | CI; credentials stay in environment/secret storage |

IDE choice is free (Qt Creator for QML profiling, Visual Studio for MSVC debugging, VS Code with CMake Tools/clangd/QML tooling). The command-line build is the source of truth.

## 6. Packaging

The installer is chosen in N11 (candidates: CPack, NSIS, WiX after a terms review). It is compared on upgrade/repair/uninstall, optional file association, signing, CI automation and maintenance. Packaging never owns settings or data formats.

## 7. Updates

Dependency updates are deliberate, never background churn. Record old and new versions, security relevance, license changes, API/ABI impact, size change, benchmark delta, fixture results and rollback. Major upgrades that change architecture or data behavior need an ADR. Changing a version invalidates the measurements taken with it until they are re-run.
