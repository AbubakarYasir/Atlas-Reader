param(
    [ValidateRange(2, 50)]
    [int]$EnglishIterations = 6,
    [ValidateRange(2, 50)]
    [int]$ArabicIterations = 3
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$packageRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$appPath = Join-Path $packageRoot "atlas_reader.exe"
$measureScript = Join-Path $packageRoot "tools\bench\measure-windows-shell.ps1"

if (-not (Test-Path $appPath)) {
    throw "Portable Atlas executable not found at '$appPath'. Run this script from the unpacked N1 portable artifact without changing its directory layout."
}

if (-not (Test-Path $measureScript)) {
    throw "Measurement script not found at '$measureScript'. The N1 artifact may be incomplete."
}

$buildInfoPath = Join-Path $packageRoot "N1_BUILD_INFO.txt"
if (Test-Path $buildInfoPath) {
    Write-Host "=== Canonical build info ==="
    Get-Content $buildInfoPath | ForEach-Object { Write-Host $_ }
    Write-Host ""
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$englishOutput = Join-Path $packageRoot "artifacts\bench\n1-portable-en-$stamp.json"
$arabicOutput = Join-Path $packageRoot "artifacts\bench\n1-portable-ar-$stamp.json"

Write-Host "=== Atlas N1 portable physical-machine qualification ==="
Write-Host "Executable: $appPath"
Write-Host ""

Write-Host "[1/2] English/LTR system-theme measurement"
& $measureScript `
    -AppPath $appPath `
    -Iterations $EnglishIterations `
    -Language en `
    -Theme system `
    -OutputPath $englishOutput

Write-Host "[2/2] Arabic/RTL dark-theme measurement"
& $measureScript `
    -AppPath $appPath `
    -Iterations $ArabicIterations `
    -Language ar `
    -Theme dark `
    -OutputPath $arabicOutput

Write-Host ""
Write-Host "Automated physical-machine measurements completed."
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
Write-Host "Keep both generated JSON reports; they contain the machine/performance evidence needed for the N1 record."
