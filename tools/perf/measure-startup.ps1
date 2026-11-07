<#
.SYNOPSIS
  Measures Atlas startup (time to first frame) on Windows and shows where the
  time goes. Used by Windows CI and runnable on any PC:

    pwsh tools/perf/measure-startup.ps1 -Exe build\...\atlas_reader.exe

  Each language is launched once to warm caches, then -Runs measured times.
  One extra traced run logs Qt font and QML disk-cache events with process
  timestamps and prints the largest gaps (N1-03 investigation).
  Target (docs/QUALITY.md): warm first frame under 800 ms.
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [int]$Runs = 3,
    [string[]]$Languages = @('en', 'ar', 'ur'),
    [double]$TargetMs = 800,
    [switch]$Enforce
)
$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path $Exe).Path
$work = Join-Path ([IO.Path]::GetTempPath()) ("atlas-startup-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null

function Invoke-Atlas([string]$Language, [string]$MetricsFile, [hashtable]$ExtraEnv = @{}, [string]$LogFile = $null) {
    $profileDir = Join-Path $work "profile-$Language"
    $argList = @('--language', $Language, '--profile-directory', $profileDir, '--quit-after-ms', '2500', '--metrics-file', $MetricsFile)
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

Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
if ($Enforce -and $worst -gt $TargetMs) { throw "Startup missed the $TargetMs ms target ($worst ms)." }
