# Building Atlas Reader Native

N0 and N1 are Accepted. N2 is the active PDF-engine qualification checkpoint (`2.0.0-alpha.2`).

The base N2 branch initially inherits the accepted N1 shell/toolchain and deliberately adds PDF candidate dependencies only in focused probe commits. A probe dependency compiling successfully does **not** mean that dependency is accepted for production.

## Canonical Windows baseline inherited from N1

See `TOOLCHAIN.md` and the accepted N1 records for the binding Windows baseline.

For the public alpha baseline:

- Windows 11 development machine recommended;
- Visual Studio 2022 with **Desktop development with C++**;
- Visual Studio 17 2022 x64 CMake generator;
- MSVC v143;
- CMake >= 3.28;
- Git;
- Qt **6.10.3** MSVC 2022 64-bit for canonical public parity.

N2 starts from this accepted baseline instead of changing toolchains merely because a candidate library uses different upstream tooling. Candidate-specific tools are documented separately when needed.

Qt Creator is optional. The repository command line remains the source of truth.

## Environment

Canonical public baseline:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
```

Verify:

```powershell
cmake --version
git --version
```

The Visual Studio generator discovers MSVC without requiring a Ninja-specific Developer PowerShell workflow for ordinary Atlas targets.

## Configure once

```powershell
cmake --preset windows-msvc2022
```

The preset identifies the active engineering preview as `2.0.0-alpha.2`.

## Debug

```powershell
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Run:

```powershell
.\build\windows-msvc2022\Debug\atlas_reader.exe
```

## Release

```powershell
cmake --build --preset windows-release
ctest --preset windows-release
```

Run:

```powershell
.\build\windows-msvc2022\Release\atlas_reader.exe
```

If the executable cannot locate Qt runtime DLLs, either launch from a Qt-aware environment or prepend the Qt kit's `bin` directory to `PATH`.

## N1 shell diagnostics retained as regression tooling

The accepted shell still exposes test-only controls:

```text
--language en|ar
--theme system|light|dark
--quit-after-ms N
--metrics-file PATH
--benchmark-shell
```

Examples:

```powershell
.\build\windows-msvc2022\Release\atlas_reader.exe --language ar --theme dark
```

```powershell
.\build\windows-msvc2022\Release\atlas_reader.exe `
  --benchmark-shell `
  --quit-after-ms 4500 `
  --metrics-file .\artifacts\bench\manual.jsonl
```

These remain useful regression tools, but running them during N2 does not reopen or requalify Accepted N1 unless new user-facing regression evidence appears.

## N2 PDF-engine qualification documents

Before building a candidate probe, read:

- `N2_PDF_ENGINE_QUALIFICATION_PLAN.md`;
- `baselines/N2_PDF_ENGINE_MATRIX.md`;
- `decisions/ADR-0004-pdf-engine-responsibilities.md`;
- `../tests/fixtures/pdf/README.md`;
- `DEPENDENCIES_AND_TOOLS.md`.

Each probe must document exact candidate version/revision, acquisition route, checksum where applicable, fixture IDs and evidence output.

## N2.0 baseline build

At N2 opening there is intentionally no Qt PDF, PDFium or qpdf target yet. The first N2 CI baseline proves that documentation/version/CI changes did not break the inherited application.

Canonical commands remain:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
cmake --preset windows-msvc2022
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --build --preset windows-release
ctest --preset windows-release
```

## Candidate-specific build rules

### Qt PDF probe

When N2.1 lands, the repository will add the Qt PDF module only to a focused probe/adapter target. The exact command/module list will be documented in this file in the same commit.

Do not wire Qt PDF directly into QML/application domain code merely to prove it can render a page.

### PDFium probe

PDFium's official upstream source workflow uses Chromium-style tooling (`depot_tools`/`gclient`, GN, Ninja and Clang/clang-cl). Atlas's ordinary build remains CMake/MSVC.

If N2 uses a pinned community prebuilt to accelerate the probe, this file must record:

- exact package/tag/revision;
- archive SHA-256;
- header/binary provenance;
- extraction/install location convention;
- CMake discovery/link command;
- license/notices location;
- how to remove/replace the probe dependency.

The upstream API is not thread-safe; build instructions must not imply that parallel Atlas jobs may call PDFium APIs concurrently without adapter serialization.

### qpdf probe

N2 intends to introduce qpdf through a pinned vcpkg manifest experiment first because a curated qpdf port exists.

When that commit lands, this file must record:

- exact `builtin-baseline`;
- selected qpdf port version/features;
- triplet;
- install/bootstrap command;
- CMake target/link configuration;
- any override needed because the registry version differs from upstream;
- how CI caches or installs it reproducibly.

Do not create a floating “latest” dependency path.

## PDF fixture location

Tracked N2 fixtures belong under:

```text
tests/fixtures/pdf/
```

The fixture contract requires provenance, redistribution permission, SHA-256, expected capabilities and mutation permission.

Private owner fixtures stay outside Git. Their paths, titles, extracted text and passwords must not leak into public CI logs or committed benchmark summaries.

## Benchmark/output location

Machine-specific raw output should live under ignored `artifacts/`, for example:

```text
artifacts/n2/
artifacts/n2/qt-pdf/
artifacts/n2/pdfium/
artifacts/n2/qpdf/
```

Git history receives summarized evidence, exact candidate/fixture IDs and artifact/checksum references, not enormous raw dumps by default.

## Public Windows CI

`.github/workflows/windows-ci.yml` uses a Debug + Release matrix.

Each lane:

1. checks out the exact commit;
2. installs public Qt 6.10.3 MSVC 2022 x64;
3. reports toolchain/checkpoint context;
4. configures with Visual Studio 17 2022 x64 and `ATLAS_PRERELEASE=alpha.2`;
5. enables `ATLAS_WARNINGS_AS_ERRORS=ON`;
6. builds;
7. runs CTest.

Release additionally:

- retains the inherited N1 shell/benchmark regression checks;
- validates the accepted GDI `qt.conf` is still present;
- stages an **N2 Windows engineering artifact** containing the current application plus N2 plan/matrix;
- labels it as engineering/CI smoke, not a user release and not N1 requalification.

Candidate probe CI steps are added in focused commits and must not silently change the inherited shell baseline.

Hosted CI is useful for correctness/reproducibility, but physical rendering/performance evidence must state the actual machine/build conditions when hardware materially affects the result.

## Warnings and formatting

Atlas-owned C++ targets use:

```text
/W4 /permissive- /Zc:__cplusplus
```

Strict CI additionally uses:

```text
/WX
```

Formatting is controlled by the checked-in `.clang-format`.

Examples:

```powershell
clang-format -i `
  src\app\main.cpp `
  src\app\logging\Logging.cpp `
  src\app\logging\Logging.h `
  src\app\diagnostics\ShellMetrics.cpp `
  src\app\diagnostics\ShellMetrics.h
```

As N2 adds adapter/probe sources, include them in formatting/static checks rather than treating spike code as exempt.

## CodeGraph (optional local developer/agent tool)

If installed:

```powershell
npm install --global @colbymchenry/codegraph
codegraph telemetry off
codegraph init
codegraph status
```

`.codegraph/` is ignored and reproducible. Restart Codex/compatible clients after enabling project-local `.codex/config.toml` if connector discovery requires it.

## Troubleshooting

### Qt not found

`CMAKE_PREFIX_PATH` must point to a kit containing `lib\cmake\Qt6`.

### Qt DLLs not found when launching

Add the selected Qt kit's `bin` directory to `PATH`, or run from Qt Creator/a Qt-aware shell.

### QML module errors

Confirm the kit contains Qt Declarative/Quick/Quick Controls.

### A candidate probe cannot be found

Do not “fix” this with an unpinned machine-global install. Read the candidate-specific N2 documentation and use the recorded acquisition path/version.

### PDFium build is awkward in MSVC/CMake

That integration cost is part of the N2 evidence. Do not hide it by silently replacing the official upstream build with an undocumented binary.

### qpdf vcpkg version differs from upstream latest

Record the exact registry port/baseline. If N2 needs a newer upstream qpdf, introduce an explicit override/acquisition change and mark which prior evidence used the older version.

### Benchmark numbers look unexpectedly high

Confirm:

- Release build;
- no debugger attached;
- consistent power plan;
- same display/GPU context where relevant;
- same engine/version/build flags;
- same fixture bytes/SHA-256;
- same requested raster dimensions/search work;
- no heavy background workload;
- same cold/warm interpretation.

## Dependency policy

N2 may add Qt PDF, PDFium and qpdf only as focused qualification dependencies behind Atlas-owned adapters/probes.

SQLite/FTS5, scanner, production Reader, production Bookmarks, annotations/ink, migration and installer dependencies remain closed until their checkpoints.
