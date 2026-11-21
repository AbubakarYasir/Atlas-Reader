<#
.SYNOPSIS
  Measures Atlas startup (time to first frame) on Windows and shows where the
  time goes. Used by Windows CI and runnable on any PC:

    pwsh tools/perf/measure-startup.ps1 -Exe build\...\atlas_reader.exe

  Each language is launched once to warm caches, then -Runs measured times.
  One extra traced run logs Qt font and QML disk-cache events with process
  timestamps and prints the largest gaps (N1-03 investigation).
  Target (docs/QUALITY.md): warm first frame under 800 ms.

  Font check (does not depend on the speed of the machine): Qt on Windows reads
  every installed font family when a font request is inexact or a character is
  missing from its font; with thousands of fonts that costs half a second or
  more. For each language the app is started on a library whose folder, file
  and title contain Chinese, emoji and direction marks, once showing the
  library and once with that book open. Each run must read fewer than
  -MaxFontFamilies families and never start Qt's all-fonts alias pass. The
  check runs with ATLAS_FONT_WARMUP=hold, so nothing but the first screens
  themselves can ask for a font. Failures are printed, written to
  -FontCheckFile when given, and thrown with -EnforceFonts.

  -ProfileTemplate <folder> measures with a copy of an existing profile (a real
  library) instead of an empty one; the folder itself is never written to.
  A last table shows when the idle font read finished and what characters from
  other scripts cost afterwards, compared with leaving it all to Qt.
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [int]$Runs = 3,
    [string[]]$Languages = @('en', 'ar', 'ur'),
    [double]$TargetMs = 800,
    [switch]$Enforce,
    [string]$Document = '',
    [int]$MaxFontFamilies = 40,
    [string]$FontCheckFile = '',
    [switch]$EnforceFonts,
    [string]$ProfileTemplate = ''
)
$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path $Exe).Path
$work = Join-Path ([IO.Path]::GetTempPath()) ("atlas-startup-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null

function Invoke-Atlas([string]$Language, [string]$MetricsFile, [hashtable]$ExtraEnv = @{}, [string]$LogFile = $null,
                      [string]$OpenDocument = '', [string]$ProfileName = '', [string]$AddLibraryRoot = '') {
    $profileDir = Join-Path $work $(if ($ProfileName) { $ProfileName } else { "profile-$Language" })
    $argList = @('--language', $Language, '--profile-directory', ('"' + $profileDir + '"'), '--quit-after-ms', '2500', '--metrics-file', ('"' + $MetricsFile + '"'))
    if ($AddLibraryRoot) { $argList += @('--add-library-root', ('"' + $AddLibraryRoot + '"')) }
    if ($OpenDocument) { $argList += ('"' + $OpenDocument + '"') }
    $saved = @{}
    foreach ($k in $ExtraEnv.Keys) { $saved[$k] = [Environment]::GetEnvironmentVariable($k); [Environment]::SetEnvironmentVariable($k, $ExtraEnv[$k]) }
    try {
        if ($LogFile) {
            $p = Start-Process -FilePath $exePath -ArgumentList $argList -PassThru -WindowStyle Hidden -RedirectStandardError $LogFile -RedirectStandardOutput "$LogFile.out"
        } else {
            $p = Start-Process -FilePath $exePath -ArgumentList $argList -PassThru -WindowStyle Hidden
        }
        if (-not $p.WaitForExit(60000)) { $p.Kill(); throw "Atlas did not exit within 60 s ($Language)" }
    } finally {
        foreach ($k in $saved.Keys) { [Environment]::SetEnvironmentVariable($k, $saved[$k]) }
    }
}

function Read-Metric([string]$File, [string]$Name) {
    foreach ($line in Get-Content -LiteralPath $File) {
        $m = $line | ConvertFrom-Json
        if ($m.metric -eq $Name) { return [double]$m.value }
    }
    return $null
}

if ($ProfileTemplate) {
    $templatePath = (Resolve-Path -LiteralPath $ProfileTemplate).Path
    foreach ($lang in $Languages) { Copy-Item -LiteralPath $templatePath -Destination (Join-Path $work "profile-$lang") -Recurse }
    Write-Host "Profiles are copies of $templatePath"
}

$rows = @()
foreach ($lang in $Languages) {
    Invoke-Atlas $lang (Join-Path $work "warm-$lang.jsonl")   # warm-up, not counted
    for ($i = 1; $i -le $Runs; $i++) {
        $file = Join-Path $work "$lang-$i.jsonl"
        Invoke-Atlas $lang $file
        $rows += [pscustomobject]@{
            Language   = $lang
            Run        = $i
            Fonts      = Read-Metric $file 'startup.fonts_registered_ms'
            QmlLoaded  = Read-Metric $file 'startup.qml_loaded_ms'
            FirstFrame = Read-Metric $file 'startup.first_frame_ms'
        }
    }
}
$rows | Format-Table -AutoSize | Out-String | Write-Host

$worst = ($rows | Measure-Object -Property FirstFrame -Maximum).Maximum
Write-Host ("Slowest warm first frame: {0:N1} ms (target {1} ms)" -f $worst, $TargetMs)

# Where does the time go? Font and QML cache events with process timestamps.
$log = Join-Path $work 'trace.log'
Invoke-Atlas 'en' (Join-Path $work 'trace.jsonl') @{
    QT_LOGGING_RULES   = 'qt.qpa.fonts.debug=true;qt.text.font.db.debug=true;qt.qml.diskcache.debug=true;qt.scenegraph.general=true'
    QT_MESSAGE_PATTERN = '%{time process} %{category}: %{message}'
    QT_FORCE_STDERR_LOGGING = '1'
} $log
$events = foreach ($line in Get-Content -LiteralPath $log -ErrorAction SilentlyContinue) {
    if ($line -match '^\s*(\d+\.\d+)\s+(.*)$') { [pscustomobject]@{ T = [double]$Matches[1]; Text = $Matches[2] } }
}
Write-Host ("Trace: {0} timestamped events" -f @($events).Count)
$gaps = for ($i = 1; $i -lt @($events).Count; $i++) {
    [pscustomobject]@{ GapMs = [math]::Round(($events[$i].T - $events[$i - 1].T) * 1000, 1); At = $events[$i - 1].T; Before = $events[$i - 1].Text; After = $events[$i].Text }
}
Write-Host 'Largest gaps between logged events (time spent between them):'
$gaps | Sort-Object GapMs -Descending | Select-Object -First 12 | ForEach-Object {
    $b = $_.Before; if ($b.Length -gt 110) { $b = $b.Substring(0, 110) }
    $a = $_.After; if ($a.Length -gt 110) { $a = $a.Substring(0, 110) }
    Write-Host ("{0,8} ms at {1:N3}s`n   after:  {2}`n   before: {3}" -f $_.GapMs, $_.At, $b, $a)
}
$counts = @{}
foreach ($e in $events) { $cat = ($e.Text -split ':')[0]; $counts[$cat] = 1 + [int]$counts[$cat] }
Write-Host 'Events per category:'
$counts.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object { Write-Host ("  {0,6}  {1}" -f $_.Value, $_.Key) }

# Font check: how many font families did Qt read, and did it start the all-fonts pass?
if (-not $Document) {
    $candidate = Join-Path $PSScriptRoot '..\..\tests\fixtures\pdf\generated\A003_outlines_links.pdf'
    if (Test-Path -LiteralPath $candidate) { $Document = (Resolve-Path -LiteralPath $candidate).Path }
}
$fontFailures = @()
$fontRows = @()
$hostileBook = ''
$hostileRoot = ''
if ($Document) {
    # Folder, file name: Han, CJK brackets, an emoji, Arabic, and isolate marks around "x".
    $han = [string][char]0x66F8
    $hostileRoot = Join-Path $work ($han + ' folder')
    New-Item -ItemType Directory -Path $hostileRoot | Out-Null
    $name = [string][char]0x300A + [char]0x6E2C + [char]0x8A66 + [char]0x300B + ' ' + [char]::ConvertFromUtf32(0x1F600) + ' ' +
            [char]0x0643 + [char]0x062A + [char]0x0627 + [char]0x0628 + ' ' + [char]0x2068 + 'x' + [char]0x2069 + '.pdf'
    $hostileBook = Join-Path $hostileRoot $name
    Copy-Item -LiteralPath $Document -Destination $hostileBook
} else {
    Write-Host 'Font check: no sample document found; only an empty library is checked.'
}
foreach ($lang in $Languages) {
    $fontProfile = "fonts-$lang"
    if ($hostileRoot) { Invoke-Atlas $lang (Join-Path $work "fonts-$lang-index.jsonl") @{} $null '' $fontProfile $hostileRoot }
    $scenarios = @(@{ Name = 'library'; Doc = '' })
    if ($hostileBook) { $scenarios += @{ Name = 'document'; Doc = $hostileBook } }
    foreach ($scenario in $scenarios) {
        $fontLog = Join-Path $work ("fonts-$lang-" + $scenario.Name + '.log')
        Invoke-Atlas $lang (Join-Path $work ("fonts-$lang-" + $scenario.Name + '.jsonl')) @{
            QT_LOGGING_RULES        = 'qt.qpa.fonts.debug=true;qt.text.font.match.debug=true'
            QT_MESSAGE_PATTERN      = '%{time process} %{category}: %{message}'
            QT_FORCE_STDERR_LOGGING = '1'
            ATLAS_FONT_WARMUP       = 'hold'
        } $fontLog $scenario.Doc $fontProfile
        $families = New-Object System.Collections.Generic.HashSet[string]
        $aliasPass = 0
        $fallbackPass = 0
        foreach ($line in Get-Content -LiteralPath $fontLog -ErrorAction SilentlyContinue) {
            if ($line -match '^\s*[\d.]+\s+qt\.qpa\.fonts: "([^"]*)"\s*$' -and $Matches[1] -notmatch '^addFontToDatabase ') { [void]$families.Add($Matches[1]) }
            elseif ($line -match 'qt\.text\.font\.match: Alias match pass') { $aliasPass++ }
            elseif ($line -match 'qt\.text\.font\.match: Fallbacks pass') { $fallbackPass++ }
        }
        $fontRows += [pscustomobject]@{ Language = $lang; Scenario = $scenario.Name; FamiliesRead = $families.Count; AliasPass = $aliasPass; FallbackPass = $fallbackPass }
        $label = "$lang/" + $scenario.Name
        if ($families.Count -ge $MaxFontFamilies) {
            $sample = (@($families) | Select-Object -First 12) -join ', '
            $fontFailures += "${label}: Qt read $($families.Count) font families (limit $MaxFontFamilies). First: $sample"
        }
        if ($aliasPass -gt 0) { $fontFailures += "${label}: an inexact font request started Qt's all-fonts alias pass ($aliasPass)." }
        if ($families.Count -lt $MaxFontFamilies) { Write-Host ("  {0}: {1}" -f $label, ((@($families) | Sort-Object) -join ', ')) }
    }
}
$fontRows | Format-Table -AutoSize | Out-String | Write-Host
if ($fontFailures.Count -gt 0) {
    Write-Host 'Font check FAILED:'
    $fontFailures | ForEach-Object { Write-Host "  $_" }
} else {
    Write-Host "Font check passed: every run read fewer than $MaxFontFamilies font families and none started the all-fonts pass."
}
# Idle font read: when it finished, and the cost of characters from other scripts afterwards.
$probeRows = @()
foreach ($mode in @('', '0')) {
    $probeFile = Join-Path $work ("probe-" + $(if ($mode) { 'off' } else { 'default' }) + '.jsonl')
    Invoke-Atlas $Languages[0] $probeFile @{ ATLAS_FONT_WARMUP = $mode; ATLAS_FONT_PROBE = '1' }
    $probeRows += [pscustomobject]@{
        Mode            = $(if ($mode -eq '') { 'default (idle read after the first frame)' } else { 'off (Qt reads when first needed)' })
        FirstFrame      = Read-Metric $probeFile 'startup.first_frame_ms'
        ReadFinished    = Read-Metric $probeFile 'fonts.warmup_finished_ms'
        Families        = Read-Metric $probeFile 'fonts.warmup_families'
        OtherScriptCost = Read-Metric $probeFile 'fonts.fallback_probe_ms'
    }
}
Write-Host 'Idle font read (ms; OtherScriptCost = laying out Korean, Hindi and Thai text once on the interface thread):'
$probeRows | Format-Table -AutoSize | Out-String | Write-Host

if ($FontCheckFile) { Set-Content -LiteralPath $FontCheckFile -Value ($fontFailures -join "`n") -Encoding utf8 }

Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
if ($EnforceFonts -and $fontFailures.Count -gt 0) { throw "Startup font check failed ($($fontFailures.Count))." }
if ($Enforce -and $worst -gt $TargetMs) { throw "Startup missed the $TargetMs ms target ($worst ms)." }
