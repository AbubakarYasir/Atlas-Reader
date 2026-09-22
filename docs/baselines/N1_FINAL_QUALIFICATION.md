# N1 Final Qualification — Windows Native Empty Shell

**Checkpoint:** N1 — Windows toolchain + empty-shell baseline  
**Release:** `2.0.0-alpha.1`  
**Status:** **Accepted — owner PASS recorded 2026-09-22**

This file is the final qualification and acceptance record for N1. Earlier attempts and diagnostic history remain in `N1_WINDOWS_BASELINE.md`, `N1_STARTUP_DIAGNOSTICS.md`, and `N1_WINDOWS_FONT_BACKEND_DECISION.md`.

## Accepted exact-head artifact

The owner-accepted post-fix artifact is:

- commit: `73f567cf2c557f185371d7f944ebf6d69453105a`;
- Windows CI push run: `35669313160`;
- artifact: `atlas-reader-n1-windows-x64-73f567cf2c557f185371d7f944ebf6d69453105a`;
- artifact digest: `sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`;
- Qt: 6.10.3 / MSVC 2022 x64;
- Controls: compile-time Basic;
- Windows font backend: GDI via `qt.conf`.

Strict Debug + Release Configure → Build → CTest, qualification-script validation, portable staging, and artifact upload all passed on that exact head.

N1 deliberately contains no PDF engine, qpdf, SQLite/FTS5, scanner, production Library/Reader/Bookmarks, annotations, migration, or installer implementation.

## Physical qualification machine

- Windows 11 Home build 26200;
- Intel Core i7-14650HX / 24 logical processors;
- 15.71 GiB RAM;
- NVIDIA GeForce RTX 4070 Laptop GPU;
- 1920x1080 / 144 Hz;
- SKHynix NVMe SSD (~953.9 GiB);
- Balanced power scheme;
- qualification scale: 100% (`DPR=1`, logical DPI 96), plus owner manual 200% check.

## Final measured startup evidence

### Arabic / dark — normal six-run qualification

- cold candidate first frame: `734.472 ms`;
- warm average: `568.395 ms`;
- warm p50: `678.976 ms`;
- warm p95: `772.875 ms`;
- warm QML-loaded p95: `460.316 ms`;
- warm working set: `78.809 MiB`;
- warm private memory: `100.191 MiB`;
- warm idle CPU: `1.627%`.

Arabic meets the provisional `<800 ms` warm-first-frame p95 target.

### English / system — normal six-run qualification

- cold candidate first frame: `929.900 ms`;
- warm average: `891.928 ms`;
- warm p50: `816.553 ms`;
- warm p95: `1247.782 ms`;
- warm QML-loaded p95: `497.661 ms`.

Because five warm samples were too small to distinguish a one-off spike from repeatable variance, the exact same measured runtime candidate received a 22-run confirmation.

### English / system — 22-run confirmation

- total runs: 22;
- warm runs: 21;
- valid warm process samples: 21/21;
- first frame observed: 22/22;
- cold candidate first frame: `360.000 ms`;
- warm average: `543.145 ms`;
- warm p50: `365.941 ms`;
- warm p95: `1014.827 ms`;
- warm QML-loaded p95: `518.933 ms`;
- warm working set: `77.351 MiB`;
- warm private memory: `97.979 MiB`;
- warm idle CPU: `1.107%`.

The confirmation is bimodal: 14/21 warm launches are approximately 312–394 ms, followed by a contiguous seven-run slower window of approximately 831–1185 ms, after which startup returns to approximately 394/366 ms without an application change. Pre-QML, QML-load, and post-QML-to-first-frame timings rise together in the slower window. N1 records the remaining English tail as measured workstation/system launch variance rather than a single remaining Atlas-specific stage.

The strict provisional `<800 ms` English p95 target is not met in this uncontrolled Balanced-power run. The owner explicitly accepted the measured tradeoff on 2026-09-22: median approximately 366 ms, p95 approximately 1.015 s, worst observed warm approximately 1.185 s in the confirmation run.

## Manual qualification — PASS

The owner confirmed all final manual N1 checks pass:

- **PASS:** English/LTR layout;
- **PASS:** Arabic/RTL mirroring;
- **PASS:** mixed Arabic/English readability;
- **PASS:** light theme;
- **PASS:** dark theme;
- **PASS:** exact 100% scale evidence;
- **PASS:** manual 200% scale check;
- **PASS:** repeated open/close leaves no Atlas process;
- **PASS:** Tab traversal reaches the controls;
- **PASS:** post-fix focused keyboard activation.

## Keyboard activation correction

A focused-button Enter activation defect was discovered **before N1 acceptance**, so correcting it did not reopen a passed checkpoint.

The product fix explicitly handles both `Qt.Key_Return` and `Qt.Key_Enter` on the two N1 buttons and calls `AbstractButton.animateClick()` for non-auto-repeat key presses. Existing built-in Space activation remains untouched.

The owner retested the final exact-head artifact and confirmed: **Keyboard activation passes.**

## Owner acceptance

On 2026-09-22 the owner explicitly recorded:

> Keyboard activation passes. I accept the measured English startup tradeoff. N1 PASS.

Therefore every N1 acceptance condition is satisfied:

1. strict Debug + Release CI on the post-fix exact head — **PASS**;
2. post-fix portable artifact produced successfully — **PASS**;
3. focused Enter/Return activation retest — **PASS**;
4. measured English startup tradeoff explicitly accepted — **PASS**;
5. explicit `N1 PASS` — **PASS**.

## Acceptance-record CI

The acceptance-record branch head after updating the baseline, final qualification record, and checkpoint ledger is:

`3c2892e16ecd3c4813e72ad02dfdd2c7260883b9`

Pull-request Windows CI run `35671096980` passed both Debug and Release lanes, including Release qualification-script validation, Configure → Build → CTest, portable staging, and artifact upload. These commits are documentation-only acceptance records; the physically tested and owner-qualified runtime remains `73f567cf2c557f185371d7f944ebf6d69453105a`.

## Final state

**N1 is Accepted.** The accepted user-qualified runtime artifact remains the exact-head package `73f567cf2c557f185371d7f944ebf6d69453105a`. Subsequent acceptance-record commits are documentation-only and do not redefine the physically tested runtime.

Per `docs/DEVELOPMENT_WORKFLOW.md`: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.** New preferences, theoretical risks, retrospective stricter criteria, or unrelated toolchain churn do not invalidate N1; they belong to later checkpoints unless concrete regression evidence appears.

N2 may begin only as a new checkpoint after this accepted N1 state; no N2 implementation is included in N1.
