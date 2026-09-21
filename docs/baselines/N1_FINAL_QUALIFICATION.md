# N1 Final Qualification — Windows Native Empty Shell

**Checkpoint:** N1 — Windows toolchain + empty-shell baseline  
**Release:** `2.0.0-alpha.1`  
**Status:** **Pending keyboard-activation retest and explicit owner PASS**

This file is the final qualification record for N1. Earlier attempts and diagnostic history remain in `N1_WINDOWS_BASELINE.md`, `N1_STARTUP_DIAGNOSTICS.md`, and `N1_WINDOWS_FONT_BACKEND_DECISION.md`.

## Candidate lineage

The measured GDI candidate before the keyboard fix was:

- commit: `7c5a305894e30149abe276230f369e48cdf1cb5b`;
- Windows CI run: `35666962902`;
- artifact: `atlas-reader-n1-windows-x64-7c5a305894e30149abe276230f369e48cdf1cb5b`;
- artifact digest: `sha256:329465ae8f285f4cd1de40b92351418b5035d072fe473d512830d1cd0210b853`;
- Qt: 6.10.3 / MSVC 2022 x64;
- Controls: compile-time Basic;
- Windows font backend: GDI via `qt.conf`.

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

Because five warm samples were too small to distinguish a one-off spike from repeatable variance, the exact same artifact received a 22-run confirmation.

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

The confirmation is bimodal: 14/21 warm launches are approximately 312–394 ms, followed by a contiguous seven-run slower window of approximately 831–1185 ms, after which startup returns to approximately 394/366 ms without an application change. Pre-QML, QML-load, and post-QML-to-first-frame timings rise together in the slower window. N1 therefore records the remaining English tail as measured workstation/system launch variance rather than a single remaining Atlas-specific stage.

The strict provisional `<800 ms` English p95 target is not met in this uncontrolled Balanced-power run. N1 acceptance therefore requires explicit owner acceptance of the measured tradeoff: median approximately 366 ms, p95 approximately 1.015 s, worst observed warm approximately 1.185 s in the confirmation run.

## Manual qualification

Owner reports that all final manual N1 checks pass except one keyboard-activation defect discovered before acceptance:

- **PASS:** English/LTR layout;
- **PASS:** Arabic/RTL mirroring;
- **PASS:** mixed Arabic/English readability;
- **PASS:** light theme;
- **PASS:** dark theme;
- **PASS:** exact 100% scale evidence;
- **PASS:** manual 200% scale check;
- **PASS:** repeated open/close leaves no Atlas process;
- **PASS:** Tab traversal reaches the controls;
- **DEFECT FOUND:** focused buttons did not activate with Enter.

## Keyboard activation correction

The defect was found **before N1 acceptance**, so correcting it does not reopen a passed checkpoint.

The product fix explicitly handles both `Qt.Key_Return` and `Qt.Key_Enter` on the two N1 buttons and calls `AbstractButton.animateClick()` for non-auto-repeat key presses. Existing built-in Space activation remains untouched.

Only the affected behavior must be retested on the post-fix artifact:

1. use Tab to focus the language button;
2. press Enter and confirm language toggles;
3. use Tab to focus the theme button;
4. press Enter and confirm theme toggles;
5. optionally confirm numpad Enter/Return and Space still work.

Previously passed manual/performance evidence does not need to be repeated solely because of this contained keyboard-input fix.

## Acceptance rule

N1 may be accepted when:

1. strict Debug + Release CI passes on the post-fix exact head;
2. the post-fix portable artifact is produced successfully;
3. the focused Enter/Return activation retest passes;
4. the owner explicitly accepts the measured English startup tradeoff;
5. the owner explicitly records `N1 PASS`.

After explicit acceptance, the repository rule in `docs/DEVELOPMENT_WORKFLOW.md` applies: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.**
