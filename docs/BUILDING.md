# Building Atlas Reader Native

N0 is intentionally a small Qt Quick/C++ build with no PDF/database dependency.

## Windows prerequisites

- Windows 11 recommended
- Visual Studio 2022 with **Desktop development with C++**
- Qt 6.11.2 desktop kit for **MSVC 2022 64-bit**
- CMake >= 3.28 (current bootstrap planning environment recognizes CMake 4.4.3)
- Ninja
- Git

Qt Creator is optional; command-line builds are the canonical reproducible path.

## Environment

Example:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.11.2\msvc2022_64'
```

Use a **Developer PowerShell for VS 2022** so `cl.exe` is available when using Ninja.

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
- `atlas_core_smoke` passes.

It does **not** mean PDF/index/bookmark behavior exists.

## Formatting

```powershell
clang-format -i src\app\main.cpp src\core\document\DocumentCapabilities.h src\core\pdf\IPdfEngine.h src\core\platform\IFileSystem.h src\core\index\ILibraryIndex.h tests\core_smoke.cpp
```

A later checkpoint may add automated formatting/static-analysis enforcement after the toolchain baseline is accepted.

## Troubleshooting

### Qt not found

Confirm `CMAKE_PREFIX_PATH` points to the Qt kit directory containing `lib\cmake\Qt6`.

### Compiler not found

Open Visual Studio Developer PowerShell or call the VS developer environment before configuring Ninja.

### QML module errors

Confirm the Qt installation includes Qt Declarative/Quick/Quick Controls for the selected desktop kit.

## Dependency policy

Do not add qpdf/PDFium/SQLite to this bootstrap merely to test installation. Engine/dependency integration belongs to N2/N3 so each dependency enters with fixtures, licensing notes, and performance evidence.