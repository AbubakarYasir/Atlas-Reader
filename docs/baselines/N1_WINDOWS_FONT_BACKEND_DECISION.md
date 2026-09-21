# N1 Windows font-backend decision

**Checkpoint:** N1 — Windows toolchain + empty-shell baseline  
**Status:** candidate configuration pending final physical qualification  
**Candidate:** Qt Windows QPA `fontengine=gdi` through `qt.conf`

## Why this decision exists

The corrected N1 physical baseline on the owner workstation showed that Atlas reaches the QML-load boundary in roughly 20–40 ms, while creating the first meaningful Qt Quick text dominates startup. A diagnostic matrix isolated the transition:

| Profile | Language | first-frame p95 |
|---|---:|---:|
| bare Qt Quick | English | 252.485 ms |
| one Text item | English | 1476.148 ms |
| one Text item | Arabic | 1302.685 ms |
| full Atlas N1 shell | English | 1399.308 ms |
| full Atlas N1 shell | Arabic | 1498.388 ms |

The full shell therefore was not the primary source of the delay, and Arabic shaping itself was not uniquely responsible.

The workstation also has a font-heavy environment: 2639 files in the Windows Fonts directory plus 13 per-user font registry entries. This is evidence about the test environment, not proof that font count alone causes the delay.

## Font-engine comparison

The same executable and full/text profiles were then tested after one prewarm per scenario with three measured warm iterations for each Windows font backend:

| Backend | Profile | Language | first-frame p95 | QML-load-cost p95 |
|---|---|---|---:|---:|
| default (DirectWrite) | text | English | 1292.376 ms | 1076.926 ms |
| default (DirectWrite) | text | Arabic | 1224.842 ms | 1069.797 ms |
| default (DirectWrite) | full | English | 1513.896 ms | 1367.786 ms |
| default (DirectWrite) | full | Arabic | 3549.836 ms | 3105.987 ms |
| GDI | text | English | 544.283 ms | 226.366 ms |
| GDI | text | Arabic | 569.016 ms | 246.329 ms |
| GDI | full | English | 709.366 ms | 360.917 ms |
| GDI | full | Arabic | 840.160 ms | 395.023 ms |
| FreeType | text | English | 1235.744 ms | 847.100 ms |
| FreeType | text | Arabic | 1196.967 ms | 768.109 ms |
| FreeType | full | English | 1478.991 ms | 1000.373 ms |
| FreeType | full | Arabic | 1343.749 ms | 900.435 ms |

The three-sample diagnostic p95 is effectively the maximum sample and is diagnostic evidence, not the final acceptance measurement.

## Candidate decision

Use Qt's officially supported Windows `fontengine=gdi` platform argument for the N1 Windows candidate, configured in repository-root `qt.conf`:

```ini
[Platforms]
WindowsArguments = fontengine=gdi
```

CMake copies the file next to local Windows builds. CI also packages it next to the portable qualification executable and verifies the setting text before building.

## Why this is not an unconditional permanent product decision

Qt 6.8 changed Windows to DirectWrite by default because DirectWrite enables modern font capabilities that the legacy GDI backend does not fully provide. In particular, variable-font functionality is one example that requires DirectWrite or FreeType on Windows.

Therefore:

- GDI is accepted here only as the **Windows N1 application-shell candidate** based on measured startup evidence;
- the future PDF page renderer remains a separate subsystem and must not be conflated with Qt UI font rendering;
- before Atlas adds any text-heavy non-PDF reader path that depends on modern font capabilities, the backend decision must be re-evaluated;
- before the Windows 2.0 release candidate, Atlas must re-test current Qt/DirectWrite behavior on representative machines and revisit this decision;
- if GDI produces inferior Arabic shaping, accessibility, DPI, glyph fallback, color-font, variable-font, or UI rendering behavior in later checkpoints, performance alone is not sufficient reason to keep it.

## Remaining N1 proof

This decision does **not** make N1 pass by itself. The canonical GDI candidate must still:

1. pass strict Debug + Release CI, CTest, script validation and portable staging on one exact branch head;
2. be measured again through the normal N1 qualification harness, not the diagnostic harness;
3. receive a final English/LTR + Arabic/RTL + light/dark visual sanity check under GDI;
4. record keyboard focus and exact 100%/200% scale evidence;
5. either satisfy the startup gate or have the small remaining measured deviation explicitly accepted by the owner;
6. receive explicit owner N1 PASS before N2 begins.
