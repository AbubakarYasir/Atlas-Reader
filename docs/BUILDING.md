# Building Atlas Reader Native

N1 is still an empty-shell/toolchain checkpoint. It intentionally contains no production PDF/database/index/bookmark dependency.

## Canonical Windows baseline

See `TOOLCHAIN.md` for the binding N1 toolchain policy.

For the public alpha baseline:

- Windows 11 development machine recommended;
- Visual Studio 2022 with **Desktop development with C++**;
- Visual Studio 17 2022 x64 CMake generator;
- MSVC v143;
- CMake >= 3.28;
- Git;
- Qt **6.10.3** MSVC 2022 64-bit for canonical public parity.

Qt 6.11.2 may also be used as an additional developer compatibility build. It is not the public N1 canonical pin because the unauthenticated `aqtinstall` Windows 6.11.x binary path is currently unreliable.

Qt Creator is optional. The repository command line remains the source of truth.

## Environment

Canonical N1 public baseline:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
```

Optional Qt 6.11.2 compatibility build:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.11.2\msvc2022_64'
```

Verify:

```powershell
cmake --version
git --version
```

The Visual Studio generator discovers MSVC without requiring a Ninja-specific Developer PowerShell workflow.

## Configure once

```powershell
cmake --preset windows-msvc2022
```

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

## N1 shell command-line options

The alpha shell exposes test-only controls so startup/RTL/theme behavior is reproducible:

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

The metrics file contains only explicit N1 numeric lifecycle/frame measurements. Ordinary product/document content must never be written into it.

## Windows baseline harness

Run:

```powershell
.\tools\bench\measure-windows-shell.ps1 `
  -Iterations 6 `
  -Language en `
  -Theme system `
  -QtBin C:\Qt\6.10.3\msvc2022_64\bin
```

See `baselines/N1_WINDOWS_BASELINE.md` for the required evidence and interpretation.

Raw benchmark JSON is written under ignored `artifacts/bench/` by default. Do not commit machine-specific raw files merely to prove a number.

## Public Windows CI

`.github/workflows/windows-ci.yml` uses a matrix for:

- Debug;
- Release.

Each lane:

1. checks out the exact commit;
2. installs public Qt 6.10.3 MSVC 2022 x64;
3. prints toolchain information;
4. configures with Visual Studio 17 2022 x64;
5. enables `ATLAS_WARNINGS_AS_ERRORS=ON`;
6. builds;
7. runs CTest.

CI intentionally does **not** run GUI performance numbers on hosted virtual hardware and then pretend they represent the user's Windows machine. Hardware/UI measurements belong to the local N1 baseline harness.

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

A repository-wide format-check script may be added once the source surface is large enough to justify it. N1 does not add a tool dependency solely for a badge.

## CodeGraph (optional local developer/agent tool)

If installed:

```powershell
npm install --global @colbymchenry/codegraph
codegraph telemetry off
codegraph init
codegraph status
```

`.codegraph/` is ignored and reproducible. Restart Codex/compatible clients after enabling project-local `.codex/config.toml` if MCP discovery requires it.

## Troubleshooting

### Qt not found

`CMAKE_PREFIX_PATH` must point to a kit containing `lib\cmake\Qt6`.

### Qt DLLs not found when launching

Add the selected Qt kit's `bin` directory to `PATH`, or run from Qt Creator/a Qt-aware shell.

### QML module errors

Confirm the kit contains Qt Declarative/Quick/Quick Controls.

### Public CI cannot install Qt 6.11.x

This is a known upstream repository/automation limitation. Do not add private Qt credentials to a public workflow just to hide the problem. Update the canonical pin only through `TOOLCHAIN.md`/ADR/checkpoint review.

### Benchmark numbers look unexpectedly high

Confirm:

- Release build;
- no debugger attached;
- consistent power plan;
- same display refresh/scale;
- same Qt/compiler build;
- no heavy background workload;
- same cold/warm interpretation.

## Dependency policy

Do not add qpdf, PDFium, SQLite, FTS5, or unrelated libraries during N1. N2/N3 introduce production dependencies only with licensing, fixtures, benchmarks, and pinned dependency policy.
