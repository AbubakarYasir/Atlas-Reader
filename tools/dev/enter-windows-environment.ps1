[CmdletBinding()]
param(
    [string]$QtRoot = 'C:\Qt\6.10.3\msvc2022_64',
    [string]$VcpkgRoot = (Join-Path $PSScriptRoot '../../build/vcpkg'),
    [switch]$Persist
)

$ErrorActionPreference = 'Stop'
$qtPath = (Resolve-Path -LiteralPath $QtRoot).Path
$vcpkgPath = (Resolve-Path -LiteralPath $VcpkgRoot).Path
foreach ($required in @(
    (Join-Path $qtPath 'lib/cmake/Qt6Pdf/Qt6PdfConfig.cmake'),
    (Join-Path $vcpkgPath 'scripts/buildsystems/vcpkg.cmake'),
    (Join-Path $vcpkgPath 'vcpkg.exe')
)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required build file missing: $required" }
}
$qtVersion = (& (Join-Path $qtPath 'bin/qmake.exe') -query QT_VERSION).Trim()
if ($LASTEXITCODE -ne 0 -or $qtVersion -ne '6.10.3') { throw 'This checkpoint requires the accepted Qt 6.10.3 kit.' }
$baseline = (git -C $vcpkgPath rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $baseline -ne '9e2895bf6afb246396d85232ba70fcfa1fa67ba1') {
    throw 'vcpkg must use the baseline declared in vcpkg.json.'
}

$env:VCPKG_ROOT = $vcpkgPath
$env:CMAKE_PREFIX_PATH = $qtPath
$qtBin = Join-Path $qtPath 'bin'
if (($env:PATH -split ';') -notcontains $qtBin) { $env:PATH = "$qtBin;$env:PATH" }
if ($Persist) {
    [Environment]::SetEnvironmentVariable('VCPKG_ROOT', $vcpkgPath, 'User')
    [Environment]::SetEnvironmentVariable('CMAKE_PREFIX_PATH', $qtPath, 'User')
}
Write-Host "Atlas environment ready: Qt 6.10.3; vcpkg $baseline."
