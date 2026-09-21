# N1 Startup Diagnostic Matrix

**Branch:** `native-v2-n1-startup-diagnostics`  
**Purpose:** isolate the physical-Windows startup bottleneck discovered after the corrected N1 qualification run.  
**Status:** diagnostic-only; never merge the probe surface into the product without an explicit decision.

The canonical N1 branch remains `native-v2-n1-toolchain-baseline`. This branch starts from its corrected-harness head and adds temporary shell profiles so one physical run can attribute the startup delay instead of guessing.

## Why this exists

The corrected physical qualification showed:

- pre-QML application initialization in only tens of milliseconds;
- first-frame startup still well above the provisional `<800 ms` warm p95 target;
- the dominant interval inside `QQmlApplicationEngine::loadFromModule()`;
- Arabic/full-shell startup slower than English/full-shell startup;
- good subjective UI responsiveness after the window appears.

The probe matrix therefore decomposes the same executable into:

1. `bare/en` — `QtQuick.Window` plus a background color, no text, no Controls;
2. `text/en` — Qt Quick `Text`, English;
3. `text/ar` — Qt Quick `Text`, Arabic;
4. `controls/en` — `QtQuick.Controls.Basic` Label/Button, English;
5. `controls/ar` — Basic Controls, Arabic;
6. `full/en` — the current N1 shell, English;
7. `full/ar` — the current N1 shell, Arabic.

Each scenario is prewarmed once and then measured repeatedly. Prewarm runs are intentionally excluded from the summary because the immediate question is the reproducible warm-start cost, not Windows extraction/Defender cold-start noise.

## Run

From the unpacked CI artifact:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\bench\run-n1-packaged-qualification.ps1 -StartupDiagnostics
```

Upload the single `artifacts\bench\n1-startup-diagnostics-*.json` report.

## Interpretation

- `bare` slow → core Qt Quick/QML module startup or packaging/import lookup is the first suspect.
- `bare` fast, `text/ar` slow → Arabic font lookup/shaping/text creation is implicated.
- text fast, Controls slow → Qt Quick Controls Basic import/type creation dominates.
- Controls fast, full slow → Atlas shell composition/layout is the dominant cost.
- all probes fast but full acceptance run remains noisy → investigate external Windows cold-cache/Defender/power-state variance separately.

The probes are not product features and do not advance N2.
