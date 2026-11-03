[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$QtRoot,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$ImplementationSha = '',
    [string]$CheckoutSha = '',
    [string]$PackageName = 'atlas-reader-review-windows-x64',
    [string]$QpdfBin = '',
    [string]$QpdfCompliance = '',
    [string]$Version = '2.0.0-beta.2',
    [string]$Checkpoint = 'N4',
    [string]$Purpose = 'Owner review build'
)

$ErrorActionPreference = 'Stop'
if ($Checkpoint -eq 'N5' -and (-not $QpdfBin -or -not $QpdfCompliance)) { throw 'N5 requires the qpdf runtime and notices.' }
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$sourceExe = (Resolve-Path -LiteralPath $Executable).Path
if ((Get-Item -LiteralPath $sourceExe).VersionInfo.ProductVersion -ne $Version) { throw 'Package version must match the built executable.' }
$qtPath = (Resolve-Path -LiteralPath $QtRoot).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Use a new output directory; existing review builds are preserved.' }
if (-not $ImplementationSha) { $ImplementationSha = (git -C $repo rev-parse HEAD).Trim() }
if (-not $CheckoutSha) { $CheckoutSha = $ImplementationSha }
if ($ImplementationSha -notmatch '^[a-f0-9]{40}$' -or $CheckoutSha -notmatch '^[a-f0-9]{40}$') {
    throw 'Full implementation and checkout commit IDs are required.'
}

$stage = Join-Path $output $PackageName
$docs = Join-Path $stage 'docs'
$samples = Join-Path $stage 'samples'
$notices = Join-Path $stage 'notices'
New-Item -ItemType Directory -Path $docs,$samples,$notices -Force | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination (Join-Path $stage 'atlas_reader.exe')
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE') -Destination (Join-Path $stage 'LICENSE.txt')
$buildDirectory = Split-Path (Split-Path $sourceExe)
$sqliteNotice = Join-Path $buildDirectory 'vcpkg_installed/x64-windows-static-md/share/sqlite3/copyright'
if (-not (Test-Path -LiteralPath $sqliteNotice)) { throw 'Pinned SQLite notice is missing from this build.' }
Copy-Item -LiteralPath $sqliteNotice -Destination (Join-Path $notices 'sqlite3-copyright.txt')
# Bundled interface assets (compiled into the executable): fonts (OFL-1.1) and icons (MIT).
Copy-Item -LiteralPath (Join-Path $repo 'assets/fonts/OFL.txt') -Destination (Join-Path $notices 'noto-fonts-OFL.txt')
Copy-Item -LiteralPath (Join-Path $repo 'assets/fonts/README.md') -Destination (Join-Path $notices 'noto-fonts-README.md')
Copy-Item -LiteralPath (Join-Path $repo 'assets/icons/LICENSE.txt') -Destination (Join-Path $notices 'fluentui-system-icons-LICENSE.txt')
$reviewGuide = if ($Checkpoint -eq 'N5') { 'N5_REVIEW.md' } else { 'N4_REVIEW.md' }
foreach ($doc in @($reviewGuide, 'DEPENDENCIES.md')) {
    Copy-Item -LiteralPath (Join-Path $repo "docs/$doc") -Destination $docs
}
Copy-Item -LiteralPath (Join-Path $repo "docs/$reviewGuide") -Destination (Join-Path $stage 'START-HERE.md')
foreach ($fixture in @('A001_simple_text.pdf','A002_geometry_labels.pdf','A003_outlines_links.pdf','A006_unicode_semantics.pdf','A008_password_rc4_40.pdf','A010_certification_structure.pdf')) {
    Copy-Item -LiteralPath (Join-Path $repo "tests/fixtures/pdf/generated/$fixture") -Destination $samples
}
Copy-Item -LiteralPath (Join-Path $repo 'tests/fixtures/pdf/README.md') -Destination (Join-Path $samples 'PROVENANCE.md')
Copy-Item -LiteralPath (Join-Path $repo 'tests/fixtures/pdf/manifest.json') -Destination $samples

& (Join-Path $qtPath 'bin/windeployqt.exe') --release --no-translations --compiler-runtime --qmldir (Join-Path $repo 'qml') (Join-Path $stage 'atlas_reader.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
# windeployqt may create qt.conf; restore the accepted font-backend setting last.
Copy-Item -LiteralPath (Join-Path $repo 'qt.conf') -Destination (Join-Path $stage 'qt.conf') -Force
foreach ($dll in @('Qt6Core.dll','Qt6Gui.dll','Qt6Quick.dll','Qt6Pdf.dll','Qt6Svg.dll','platforms/qwindows.dll','imageformats/qsvg.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $dll))) { throw "Missing packaged runtime: $dll" }
}
if (Test-Path -LiteralPath (Join-Path $stage 'pdfium.dll')) { throw 'Standalone PDFium is qualification-only.' }

# First-party qpdf 12.4.1 CLI (structure and security checks, N5), in its own folder.
if ($QpdfBin) {
    $qpdfStage = Join-Path $stage 'qpdf'
    New-Item -ItemType Directory -Force -Path $qpdfStage | Out-Null
    foreach ($name in @('qpdf.exe', 'qpdf30.dll', 'concrt140.dll', 'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll',
                        'msvcp140_atomic_wait.dll', 'msvcp140_codecvt_ids.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
        $source = Join-Path $QpdfBin $name
        if (-not (Test-Path -LiteralPath $source)) { throw "qpdf runtime file missing: $source" }
        Copy-Item -LiteralPath $source -Destination $qpdfStage
    }
    Copy-Item -LiteralPath (Join-Path $QpdfCompliance 'LICENSE.txt') -Destination (Join-Path $notices 'qpdf-LICENSE.txt')
    Copy-Item -LiteralPath (Join-Path $QpdfCompliance 'NOTICE.md') -Destination (Join-Path $notices 'qpdf-NOTICE.md')
}

# Preserve the same pinned Qt source-license route used by the accepted N2 lane.
$licenseSources = @(
    @{ Module = 'qtbase'; Commit = '7ddbc87d8e14ce51d2957ea72d0a6077593d5ff4' },
    @{ Module = 'qtsvg'; Commit = '77765f981c067cec8f250e13688e5be75ee94a1b' },
    @{ Module = 'qtwebengine'; Commit = '9d3164c951523dfe151cc3799b6a8945de666171' }
)
$licenseManifest = @()
foreach ($source in $licenseSources) {
    $destination = Join-Path $notices $source.Module
    New-Item -ItemType Directory -Path $destination | Out-Null
    $url = "https://api.github.com/repos/qt/$($source.Module)/contents/LICENSES?ref=$($source.Commit)"
    $files = Invoke-RestMethod -Uri $url
    foreach ($file in $files) {
        if ($file.type -ne 'file') { continue }
        $target = Join-Path $destination $file.name
        $downloadUrl = "https://raw.githubusercontent.com/qt/$($source.Module)/$($source.Commit)/LICENSES/$($file.name)"
        Invoke-WebRequest -Uri $downloadUrl -OutFile $target
        $licenseManifest += [ordered]@{ file = "$($source.Module)/$($file.name)"; source = $downloadUrl; sha256 = (Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
}
$pdfNotices = Join-Path $notices 'qtpdf-attributions'
New-Item -ItemType Directory -Path $pdfNotices | Out-Null
$index = Join-Path $pdfNotices 'qtpdf-licensing.html'
Invoke-WebRequest -Uri 'https://doc.qt.io/qt-6.10/qtpdf-licensing.html' -OutFile $index
$pages = @([regex]::Matches((Get-Content -LiteralPath $index -Raw), 'href="([^"]*qtpdf-3rdparty-[^"]+\.html)"') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
if ($pages.Count -ne 10) { throw 'Qt PDF attribution list changed; review before packaging.' }
foreach ($page in $pages) { Invoke-WebRequest -Uri "https://doc.qt.io/qt-6.10/$page" -OutFile (Join-Path $pdfNotices $page) }
$licenseManifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $notices 'source-manifest.json') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $qtPath 'sbom') -Destination (Join-Path $notices 'qt-sbom') -Recurse

$buildInfo = [ordered]@{
    schemaVersion = 1; checkpoint = $Checkpoint; purpose = $Purpose; publisher = 'DerekSol (https://dereksol.com)'
    version = $Version; implementationSha = $ImplementationSha; checkoutSha = $CheckoutSha
    configuration = 'Release'; qt = '6.10.3'; sqlite = '3.53.4#1'
    sourceExeSha256 = (Get-FileHash -LiteralPath $sourceExe -Algorithm SHA256).Hash.ToLowerInvariant()
    builtUtc = [DateTime]::UtcNow.ToString('o'); pageRendering = $true; pdfWrites = ($Checkpoint -eq 'N5')
}
$buildInfo | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'BUILD-INFO.json') -Encoding utf8

$smokeProfile = Join-Path $output 'smoke-profile'
$smokeArgs = '--profile-directory "{0}" --quit-after-ms 1800' -f $smokeProfile
$smokeLog = Join-Path $output 'smoke-stderr.log'
$savedEnvironment = @{}
foreach ($name in @('PATH','QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','ATLAS_QPDF','QT_FORCE_STDERR_LOGGING')) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_FORCE_STDERR_LOGGING = "1"
    foreach ($name in @('QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','ATLAS_QPDF')) {
        [Environment]::SetEnvironmentVariable($name, $null, 'Process')
    }
    $smoke = Start-Process -FilePath (Join-Path $stage 'atlas_reader.exe') -ArgumentList $smokeArgs -PassThru -WindowStyle Hidden -RedirectStandardError $smokeLog
    if (-not $smoke.WaitForExit(30000)) { $smoke.Kill(); throw 'Packaged startup timed out.' }
    if ($smoke.ExitCode -ne 0) { throw "Packaged startup failed: $($smoke.ExitCode)" }
} finally {
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
if ((Get-Content -LiteralPath $smokeLog -Raw) -match 'ReferenceError|TypeError|failed to load component|is not installed') {
    throw 'Packaged startup reported a QML/runtime error; inspect smoke-stderr.log.'
}

Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object {
    $relative = [IO.Path]::GetRelativePath($stage, $_.FullName).Replace('\','/')
    "$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant())  $relative"
} | Set-Content -LiteralPath (Join-Path $stage 'SHA256SUMS.txt') -Encoding utf8
$archive = Join-Path $output "$PackageName.zip"
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $PackageName.zip" | Set-Content -LiteralPath "$archive.sha256" -Encoding utf8
Write-Host "Review package: $archive"
Write-Host "SHA-256: $hash"
