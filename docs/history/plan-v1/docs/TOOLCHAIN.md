# Atlas Reader Native — N1 Windows Toolchain

This document defines the reproducible Windows toolchain for the native `2.0.0-alpha.1` baseline. It is intentionally narrower than the eventual Windows 2.0 release matrix.

## Canonical N1 public baseline

| Component | N1 baseline |
|---|---|
| Host/target | Windows x64; Windows 11 is the primary product target |
| CI runner family | GitHub Actions `windows-2022` |
| Generator | Visual Studio 17 2022, x64 |
| Compiler family | MSVC v143 |
| CI-observed compiler | MSVC 19.44.35228.0 / toolset 14.44.35207 |
| CI-observed Windows SDK | 10.0.26100.0 |
| C++ language level | C++23 |
| CMake | minimum 3.28; actual version is printed in every CI run |
| Qt public reproducible pin | **6.10.3**, MSVC 2022 64-bit |
| Atlas prerelease | `2.0.0-alpha.1` |

Exact runner image patch numbers may evolve inside the pinned `windows-2022` family. CI logs are therefore evidence and must retain the compiler/SDK/CMake information for every accepted checkpoint.

## Why Qt 6.10.3 is the N1 public pin

Qt 6.11.2 is the preferred current product-generation line and is available through Qt's official installer/source channels. However, the public `aqtinstall` Windows 6.11.x binary repository path is currently not reliably installable. Atlas will not make public CI depend on private Qt-account credentials merely to consume a newer patch.

Therefore N1 uses this rule:

1. **6.10.3 is the canonical public alpha baseline.**
2. Source code requires only the Qt 6.10 API floor during N1.
3. Developers may additionally build the same source with Qt 6.11.2 for compatibility qualification.
4. No 6.11-only API may enter until the canonical build lane can reproduce it or an explicit ADR changes the policy.
5. Before the Windows release candidate, Atlas must re-evaluate and move to the current secure supported Qt patch that can be reproduced for release builds.

This is a reproducibility decision, not a claim that 6.10.3 is technically preferable to 6.11.2.

## Local parity

The canonical local path is the same CMake generator used in CI:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.10.3\msvc2022_64'
cmake --preset windows-msvc2022
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --build --preset windows-release
ctest --preset windows-release
```

A developer using Qt 6.11.2 may point `CMAKE_PREFIX_PATH` to that kit and run the same presets. That build is compatibility evidence, not the canonical N1 public baseline.

## Debug and Release are both required

Every N1 change must compile and pass CTest in:

- Debug;
- Release.

CI enables `ATLAS_WARNINGS_AS_ERRORS=ON` and `/W4 /permissive- /Zc:__cplusplus /WX` for Atlas-owned C++ targets. Local builds may leave warnings-as-errors off while iterating, but accepted code must pass the strict CI configuration.

## Toolchain drift policy

Do not silently change any of these because a developer machine happens to contain something newer:

- Qt major/minor/patch used by canonical CI;
- Visual Studio generator family;
- architecture;
- minimum CMake version;
- C++ language level.

A change requires:

1. reason;
2. clean Debug + Release CI;
3. benchmark comparison when framework/compiler changes could affect performance;
4. license/security review when relevant;
5. documentation update;
6. ADR when the change materially affects architecture or platform support.

## What is intentionally not in N1

- vcpkg production dependency graph;
- PDFium;
- qpdf;
- SQLite/FTS5;
- Catch2/Google Benchmark runtime dependency;
- Windows installer/signing toolchain;
- Android/Linux/Apple build matrix.

Those enter only at their documented checkpoints.
