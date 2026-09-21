# Building Atlas Reader Native

N0 is intentionally a small Qt Quick/C++ build with no PDF/database dependency.

## Windows prerequisites

- Windows 11 recommended
- Visual Studio 2022 with **Desktop development with C++**
- CMake >= 3.28
- Ninja
- Git
- Qt desktop kit for **MSVC 2022 64-bit**

### Qt version note

Preferred product-generation line: Qt **6.11.x**. Local experimentation may use 6.11.2 while N1 qualifies the final canonical patch.

Public bootstrap CI currently pins **Qt 6.10.3** because the public aqt/install-qt-action Windows 6.11 repository path is failing upstream. N1 must converge CI/local release development onto one reproducible canonical Qt patch before feature implementation.

Qt Creator is optional; command-line builds are the canonical reproducible path.

## Environment

Example for a local 6.11.2 installation:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.11.2\msvc2022_64'
```

Or for 6.10.3:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
```

Use a **Developer PowerShell for VS 2022** so `cl.exe` is available with Ninja.

Verify:

```powershell
cmake --version
ninja --version
cl
```

## Configure/build/test

Debug:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Release:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

Run (path can vary by generator/configuration):

```powershell
.\build\windows-debug\atlas_reader.exe
```

## What success means in N0

- CMake configures against Qt;
- `atlas_reader` compiles;
- minimal QML window opens;
- `atlas_core_smoke` passes;
- the same configure/build/test path passes in public Windows CI.

It does **not** mean PDF/index/bookmark behavior exists.

## Public Windows CI

`.github/workflows/windows-ci.yml` performs:

1. checkout;
2. public Qt installation;
3. CMake Release configure with Ninja;
4. build;
5. CTest.

The CI workflow requires no private Qt account credentials. If a newer Qt release cannot be installed through the public automation path, record that constraint rather than adding repository secrets or silently changing the product claim.

## Formatting

```powershell
clang-format -i src\app\main.cpp src\core\document\DocumentCapabilities.h src\core\pdf\IPdfEngine.h src\core\platform\IFileSystem.h src\core\index\ILibraryIndex.h tests\core_smoke.cpp
```

N1 introduces the canonical formatting/warning/static-analysis commands. N2 introduces vcpkg manifest mode when accepted non-Qt native dependencies first enter the build.

## CodeGraph (optional local developer/agent tool)

If installed:

```powershell
npm install --global @colbymchenry/codegraph
codegraph telemetry off
codegraph init
codegraph status
```

The generated `.codegraph/` directory is ignored and reproducible. Restart Codex/compatible client after enabling project-local `.codex/config.toml` if MCP discovery requires it.

## Troubleshooting

### Qt not found

Confirm `CMAKE_PREFIX_PATH` points to the Qt kit directory containing `lib\cmake\Qt6`.

### Compiler not found

Open Visual Studio Developer PowerShell or call the Visual Studio developer environment before configuring Ninja.

### QML module errors

Confirm the Qt installation includes Qt Declarative/Quick/Quick Controls for the selected desktop kit.

### CI Qt install fails before configure

This can be an upstream aqt/Qt repository availability issue rather than an Atlas source failure. Inspect the install step logs, verify the selected version is publicly installable, and update the documented bootstrap/canonical pin only through the toolchain gate. Do not mark N0 verified if Configure/Build/Test never ran.

## Dependency policy

Do not add qpdf/PDFium/SQLite to N0 merely to test installation. Engine/dependency integration belongs to N2/N3 so each dependency enters with fixtures, licensing notes, performance evidence, and a pinned vcpkg baseline where appropriate.