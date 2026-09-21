# N1 Windows Empty-Shell Baseline

**Checkpoint:** N1 — Windows toolchain + empty-shell baseline  
**Release:** `2.0.0-alpha.1`  
**Status:** **Final qualification — keyboard activation retest pending**

This file preserves the N1 investigation history and points to the final qualification record in `N1_FINAL_QUALIFICATION.md`.

## Canonical N1 result

The final runtime candidate is built from exact head:

`99b693713d3548fe3c309f37cfdd81be51eae94a`

Windows CI run `35669134924` passed:

- strict Debug Configure → Build → CTest;
- strict Release Configure → Build → CTest;
- qualification-script validation;
- GDI `qt.conf` packaging contract;
- `windeployqt` portable staging;
- canonical artifact upload.

Final artifact:

`atlas-reader-n1-windows-x64-99b693713d3548fe3c309f37cfdd81be51eae94a`

Digest:

`sha256:c2a3248ea6d6332cc0ab92081fc4422591c9fd6826bce693ba68a76985610680`

The final head differs from the fully measured GDI candidate only by the contained keyboard-input correction and documentation. The correction explicitly maps Enter/Return on the two focused N1 buttons to `AbstractButton.animateClick()`; it does not alter layout, font backend, startup path, theme logic, DPI behavior, or product scope.

For the complete final metrics, tradeoff statement, and manual checklist, see `N1_FINAL_QUALIFICATION.md`.

---

## Investigation history

### Attempt 1 — initial packaged shell

The first physical Windows run exposed two separate issues:

1. a benchmark defect: `WaitForInputIdle(5000)` could outlive Atlas auto-shutdown and produce invalid memory/CPU samples;
2. a real startup problem: ordinary warm first frames were roughly 1.8–2.2 seconds, materially above the provisional `<800 ms` warm-p95 target.

The benchmark defect was fixed by waiting for Atlas's own first-frame metric and refusing dead-process samples.

### Corrected full-shell baseline

On corrected exact build `87fdf83acb39b05ca8407ad7c4d8910ac3ea23da`, physical measurements were valid, but startup remained slow:

- English warm first-frame p95: approximately `2424 ms`;
- Arabic warm first-frame p95: approximately `2976 ms`.

Staged metrics showed Atlas reached the QML-load boundary quickly; most delay occurred during first meaningful text/QML initialization.

### Startup isolation

A temporary diagnostic branch compared progressively richer shells on the same executable family:

- bare Qt Quick window p95: approximately `252 ms`;
- one Text item, English: approximately `1476 ms`;
- one Text item, Arabic: approximately `1303 ms`;
- full shell, English: approximately `1399 ms`;
- full shell, Arabic: approximately `1498 ms`.

This ruled out Atlas layout/composition as the dominant cost and showed Arabic shaping was not uniquely responsible.

### Windows font-engine comparison

The physical machine contained 2639 files in the Windows Fonts directory and 13 per-user font registry entries. This was recorded as environment evidence, not claimed as sole cause.

A controlled backend comparison then produced:

| backend | full English p95 | full Arabic p95 |
|---|---:|---:|
| default Qt Windows backend (DirectWrite) | `1513.896 ms` | `3549.836 ms` |
| FreeType | `1478.991 ms` | `1343.749 ms` |
| GDI | **`709.366 ms`** | **`840.160 ms`** |

N1 therefore adopted the officially supported Windows GDI font backend through `qt.conf`, with the decision and future re-evaluation requirements recorded separately in `N1_WINDOWS_FONT_BACKEND_DECISION.md`.

### Final GDI physical measurements

Normal six-run qualification on exact candidate `7c5a305894e30149abe276230f369e48cdf1cb5b` produced:

Arabic/dark:

- cold candidate: `734.472 ms`;
- warm average: `568.395 ms`;
- warm p50: `678.976 ms`;
- warm p95: **`772.875 ms`**;
- warm QML-loaded p95: `460.316 ms`.

English/system initial six-run:

- cold candidate: `929.900 ms`;
- warm average: `891.928 ms`;
- warm p50: `816.553 ms`;
- warm p95: `1247.782 ms`.

Because the English five-warm-sample p95 was too sensitive to one spike, the same artifact received a 22-run confirmation:

- 22 total / 21 warm;
- 21/21 valid warm process samples;
- 22/22 first frames observed;
- cold candidate: `360.000 ms`;
- warm average: `543.145 ms`;
- warm p50: **`365.941 ms`**;
- warm p95: **`1014.827 ms`**;
- warm QML-loaded p95: `518.933 ms`;
- working set: `77.351 MiB`;
- private memory: `97.979 MiB`;
- idle CPU: `1.107%`.

The English confirmation was bimodal: most launches were approximately 312–394 ms, while a contiguous slow window reached approximately 831–1185 ms and then recovered without a code/configuration change. Pre-QML, QML-load, and post-QML-to-first-frame stages all rose together. N1 records this as measured system/workstation launch variance. The strict provisional `<800 ms p95` target therefore requires an explicit owner-approved tradeoff for English.

---

## Manual qualification

The owner reports all manual N1 checks pass except the keyboard activation defect discovered before acceptance:

- [x] English/LTR layout;
- [x] Arabic/RTL mirroring;
- [x] mixed Arabic/English readability;
- [x] light theme;
- [x] dark theme;
- [x] exact 100% scale evidence (`DPR=1`, logical DPI 96);
- [x] manual 200% Windows scale check;
- [x] repeated open/close leaves no Atlas process;
- [x] Tab traversal reaches the controls;
- [ ] post-fix Enter/Return activation retest on both controls.

The Enter/Return defect was found **before N1 acceptance** and is fixed on final head `99b693713d3548fe3c309f37cfdd81be51eae94a`. Previously passed evidence does not need to be repeated for this contained input-only correction.

## Product scope held during N1

N1 contains no:

- PDF engine;
- qpdf;
- SQLite/FTS5;
- scanner;
- production Library/Reader/Bookmarks;
- annotations;
- migration implementation;
- installer implementation.

## N1 acceptance gate

N1 becomes Accepted only when:

1. exact-head Debug + Release CI passes — **PASS**;
2. final portable artifact is produced — **PASS**;
3. physical performance evidence is valid — **PASS**;
4. 100%/200% and LTR/RTL/theme/process manual checks pass — **PASS**;
5. Enter/Return keyboard activation retest passes — **PENDING**;
6. owner explicitly accepts the measured English startup tradeoff — **PENDING**;
7. owner explicitly records `N1 PASS` — **PENDING**.

After explicit acceptance, `docs/DEVELOPMENT_WORKFLOW.md` applies: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.**
