# Building Atlas Reader

The command line is the source of truth; no IDE is required.

## 1. Canonical Windows toolchain

| Component | Version |
|---|---|
| OS | Windows 11 x64 (development and primary target) |
| Compiler | Visual Studio 2022, **Desktop development with C++**, MSVC v143, generator `Visual Studio 17 2022` x64 |
| Language | C++23 |
| CMake | ≥ 3.28 (actual version printed in every CI run) |
| Qt | **6.10.3**, MSVC 2022 64-bit, including **Qt PDF** |
| vcpkg | Manifest mode, baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1` (SQLite 3.53.4#1 + FTS5, triplet `x64-windows-static-md`) |
| CI runner | GitHub Actions `windows-2022` (observed MSVC 19.44, Windows SDK 10.0.26100.0) |

Visual Studio 2026 is allowed **locally** through the `windows-msvc2026` preset, with the same Qt 6.10.3 `msvc2022_64` kit. It doesn't change the shipping toolchain.

**Drift policy.** Never change the canonical Qt version, generator family, architecture, minimum CMake or C++ level just because a machine has something newer. A change needs a reason, clean Debug + Release CI, a benchmark comparison where performance could shift, a license/security review, doc updates and, if architectural, an ADR ([ADR-0003](decisions/ADR-0003-n1-qt-toolchain-pin.md)). Before the release candidate, Atlas moves to the current secure supported Qt that CI can reproduce.

## 2. One-time setup

```powershell
# From the repository root (skip clone/bootstrap if build\vcpkg already exists at the pinned baseline)
git clone https://github.com/microsoft/vcpkg.git .\build\vcpkg
git -C .\build\vcpkg checkout 9e2895bf6afb246396d85232ba70fcfa1fa67ba1
.\build\vcpkg\bootstrap-vcpkg.bat -disableMetrics

# Set Qt + vcpkg for this PowerShell session, validated (add -Persist to save for your user)
. ./tools/dev/enter-windows-environment.ps1
```

The helper checks the Qt kit, the toolchain files and the vcpkg baseline, then adds Qt's `bin` folder to this process's `PATH`. A `.git` folder alone doesn't prove a usable vcpkg; `vcpkg.exe` and `scripts/buildsystems/vcpkg.cmake` must both exist. If a saved `VCPKG_ROOT` points at an old checkout, re-run the helper with `-Persist`.

## 3. Test fixtures

Generated fixtures are created locally and never committed:

```powershell
python -m pip install --disable-pip-version-check -r tests/fixtures/pdf/requirements-validation.txt
python tests/fixtures/pdf/source/generate_a006_unicode_semantics.py
python tests/fixtures/pdf/source/generate_a007_a008_security_semantics.py
python tests/fixtures/pdf/source/generate_a009_restricted_permissions.py
python tests/fixtures/pdf/source/generate_a010_certification_structure.py
python tests/fixtures/pdf/validate_n2_fixtures.py --output build/local-fixture-validation.json
```

The validator must report `"passed": true`. See the [fixture contract](../tests/fixtures/pdf/README.md).

## 4. Configure, build, test

```powershell
cmake --preset windows-msvc2022          # VS 2026: windows-msvc2026
cmake --build --preset windows-debug     # VS 2026: windows-2026-debug
ctest  --preset windows-debug
cmake --build --preset windows-release   # VS 2026: windows-2026-release
ctest  --preset windows-release
```

Run: `.\build\windows-msvc2022\Release\atlas_reader.exe` (or `build\windows-msvc2026\…`). The preset sets `ATLAS_PRERELEASE=beta.1` until N4 earns beta.2.

Both Debug and Release must pass. CI adds `ATLAS_WARNINGS_AS_ERRORS=ON` (`/W4 /permissive- /Zc:__cplusplus /WX` on Atlas targets); accepted code must pass that strict configuration.

## 5. Useful app flags (diagnostics)

```text
--language en|ar   --theme system|light|dark   --profile-directory PATH
--quit-after-ms N  --metrics-file PATH         --benchmark-shell
```

```powershell
.\build\windows-msvc2022\Release\atlas_reader.exe --language ar --theme dark
```

Machine-specific raw output goes in ignored `artifacts/`. Git gets summaries with commit, fixture IDs and checksums, never large raw dumps or private paths.

## 6. Owner review package

After a successful Release build and test:

```powershell
./tools/release/package-n4-review.ps1 `
  -Executable build/windows-msvc2022/Release/atlas_reader.exe `
  -QtRoot $env:CMAKE_PREFIX_PATH `
  -OutputDirectory artifacts/n4-review-local
```

The script refuses to overwrite an existing output. It deploys the Qt runtime, restores the accepted `qt.conf`, adds public samples with their provenance, the [review guide](N4_REVIEW.md) as `START-HERE.md`, Atlas/Qt/SQLite notices and the Qt SBOM, records build identity, runs an isolated-profile startup check, and writes file and ZIP SHA-256 checksums. It needs Internet only to gather notices; the app itself runs offline. It rejects a standalone `pdfium.dll`. CI on the N4 branch uploads the same package for 30 days.

## 7. CI

`.github/workflows/windows-ci.yml` runs on every push with a Debug + Release matrix: pinned Qt 6.10.3 and vcpkg → configure (strict warnings) → build → CTest. Release also runs: the Markdown governance check (`tools/docs/check-markdown.ps1`), PowerShell syntax validation, N1 shell regression checks, the `qt.conf` check and review-package staging. The seven `n2-*.yml` workflows replay the PDF qualification suites (fidelity, coordinates, stress, content, production route, qpdf). Hosted CI proves correctness and reproducibility; physical performance claims must name the real machine.

Historical N2 probe build instructions (Qt PDF, PDFium, qpdf probes) are in [history](history/plan-v1/docs/BUILDING.md).

## 8. Formatting and static checks

`.clang-format` is canonical. Format the files you touch:

```powershell
clang-format -i (git diff --name-only -- '*.cpp' '*.h')
```

A clang-format check and a curated clang-tidy set become CI gates during N4.2 ([QUALITY.md §8](QUALITY.md#8-static-and-build-gates)). Documentation:

```powershell
pwsh -NoProfile -File tools/docs/check-markdown.ps1
git diff --check
```

## 9. CodeGraph (optional)

```powershell
npm install --global @colbymchenry/codegraph
codegraph telemetry off
codegraph init
codegraph status
```

`.codegraph/` is ignored. The project-local `.codex/config.toml` defines a read-only GitHub MCP endpoint; keep `GITHUB_PAT_TOKEN` in your user environment, never in files.

## 10. Troubleshooting

| Problem | Fix |
|---|---|
| Qt not found | `CMAKE_PREFIX_PATH` must point to a kit containing `lib\cmake\Qt6`; re-run the environment helper |
| Qt DLLs missing at launch | Add the kit's `bin` folder to `PATH` (the helper does this) or use the review package |
| QML module errors | The kit must include Qt Declarative, Quick, Quick Controls and Qt PDF |
| vcpkg toolchain not found | `VCPKG_ROOT` points at an old or incomplete checkout; bootstrap `build\vcpkg` and re-run the helper |
| Benchmark numbers look wrong | Release build, no debugger, same power plan, display and GPU, same fixture SHA-256, same warm/cold state, no heavy background load |
| Tempted to install something globally to make it work | Don't. Use the pinned route in this file, or propose a change through [DEPENDENCIES.md](DEPENDENCIES.md) |
