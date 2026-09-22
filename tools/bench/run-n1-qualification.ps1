param(
    [string]$QtRoot = "C:\Qt\6.10.3\msvc2022_64",
    [ValidateRange(2, 50)]
    [int]$EnglishIterations = 6,
    [ValidateRange(2, 50)]
    [int]$ArabicIterations = 3
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$qtCmake = Join-Path $QtRoot "lib\cmake\Qt6"
$qtBin = Join-Path $QtRoot "bin"

if (-not (Test-Path $qtCmake)) {
    throw "Qt kit not found at '$QtRoot'. Install/use the MSVC 2022 x64 Qt kit or pass -QtRoot explicitly."
}

$env:CMAKE_PREFIX_PATH = $QtRoot

Write-Host "=== Atlas N1 qualification ==="
Write-Host "Repository: $repoRoot"
Write-Host "Qt root:    $QtRoot"
Write-Host ""

Push-Location $repoRoot
try {
    Write-Host "[1/5] Configure Visual Studio 2022 x64"
    cmake --preset windows-msvc2022
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

    Write-Host "[2/5] Build Release"
    cmake --build --preset windows-release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Release build failed." }

    Write-Host "[3/5] Run Release tests"
    ctest --preset windows-release
    if ($LASTEXITCODE -ne 0) { throw "Release tests failed." }

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $englishOutput = Join-Path $repoRoot "artifacts\bench\n1-en-$stamp.json"
    $arabicOutput = Join-Path $repoRoot "artifacts\bench\n1-ar-$stamp.json"

    Write-Host "[4/5] Measure English/LTR system-theme shell"
    & (Join-Path $PSScriptRoot "measure-windows-shell.ps1") `
        -Iterations $EnglishIterations `
        -Language en `
        -Theme system `
        -QtBin $qtBin `
        -OutputPath $englishOutput
    if ($LASTEXITCODE -ne 0) { throw "English baseline failed." }

    Write-Host "[5/5] Measure Arabic/RTL dark-theme shell"
    & (Join-Path $PSScriptRoot "measure-windows-shell.ps1") `
        -Iterations $ArabicIterations `
        -Language ar `
        -Theme dark `
        -QtBin $qtBin `
        -OutputPath $arabicOutput
    if ($LASTEXITCODE -ne 0) { throw "Arabic baseline failed." }

    Write-Host ""
    Write-Host "Automated N1 local evidence completed."
    Write-Host "English report: $englishOutput"
    Write-Host "Arabic report:  $arabicOutput"
    Write-Host ""
    Write-Host "Manual checks still required before N1 PASS:"
    Write-Host "  [ ] English/LTR layout and keyboard focus"
    Write-Host "  [ ] Arabic/RTL mirroring and mixed-script readability"
    Write-Host "  [ ] Light and dark themes"
    Write-Host "  [ ] Windows display scale at 100%"
    Write-Host "  [ ] Windows display scale at 200%"
    Write-Host "  [ ] Repeated open/close leaves no Atlas process"
    Write-Host ""
    Write-Host "Use docs/baselines/N1_WINDOWS_BASELINE.md to record the accepted evidence."
} finally {
    Pop-Location
}
