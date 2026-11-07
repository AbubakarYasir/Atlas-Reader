<#
.SYNOPSIS
  Windows rendered review (UI-01, N1-02, N4-04): screenshots of the main
  surfaces and states in EN/AR/UR at 100 % and 200 %, wide and narrow, and
  with Windows high contrast switched on, for a person to review.

  200 %: switches the Windows display scale itself (the per-monitor DPI the
  Settings app sets) and confirms from the screenshot size that Qt drew at
  2x. Only if Windows refuses does it fall back to QT_SCALE_FACTOR=2, and the
  summary says which one ran. High contrast: switches Windows high contrast on
  (SPI_SETHIGHCONTRAST) and off again afterwards.

    pwsh -NoProfile -File tools/review/windows-visual-review.ps1 -Exe <atlas_reader.exe> -Fixtures tests/fixtures/pdf/generated -Out <dir>
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Fixtures,
    [Parameter(Mandatory = $true)][string]$Out,
    [string[]]$Languages = @('en', 'ar', 'ur')
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing, System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class AtlasDisplay {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct HIGHCONTRAST { public uint cbSize; public uint dwFlags; public string lpszDefaultScheme; }
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool SystemParametersInfo(uint action, uint param, ref HIGHCONTRAST value, uint flags);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SystemParametersInfo(uint action, int param, IntPtr value, uint flags);
    public static bool HighContrast(bool on) {
        var hc = new HIGHCONTRAST();
        hc.cbSize = (uint)Marshal.SizeOf(typeof(HIGHCONTRAST));
        hc.dwFlags = on ? 1u : 0u;              // HCF_HIGHCONTRASTON
        hc.lpszDefaultScheme = "High Contrast Black";
        return SystemParametersInfo(0x0043, hc.cbSize, ref hc, 3); // SPI_SETHIGHCONTRAST
    }
    // Relative scale step from the recommended value (+4 = 200 % on a 100 % screen).
    public static bool ScaleSteps(int steps) { return SystemParametersInfo(0x009F, steps, IntPtr.Zero, 1); }
}
'@

$exePath = (Resolve-Path $Exe).Path
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$work = Join-Path ([IO.Path]::GetTempPath()) ('atlas-visual-' + [guid]::NewGuid().ToString('N'))
$books = Join-Path $work 'books'
New-Item -ItemType Directory -Path $books | Out-Null
function Book([string]$fixture, [string]$name) {
    $path = Join-Path $books $name
    Copy-Item -LiteralPath (Join-Path $Fixtures $fixture) -Destination $path
    $path
}
$outline = Book 'A003_outlines_links.pdf' 'Outline Sample.pdf'
$headings = Book 'A001_simple_text.pdf' 'Heading Sample.pdf'
$restricted = Book 'A009_restricted_permissions_rc4_40.pdf' 'Restricted Sample.pdf'
$password = Book 'A015_encrypted_outline_rc4_40.pdf' 'Password Sample.pdf'
$malformed = Book 'A016_malformed_outline.pdf' 'Damaged Outline Sample.pdf'

function Quote([string[]]$items) { ($items | ForEach-Object { if ($_ -match '[\s"]') { '"' + $_ + '"' } else { $_ } }) -join ' ' }

$summary = New-Object System.Collections.Generic.List[string]
function Shot([string]$name, [string]$lang, [string[]]$extra, [int]$w, [int]$h, [int]$delay = 2500, [string]$profileKind = 'books') {
    $profileDir = Join-Path $work "profile-$profileKind-$lang"
    if (-not (Test-Path $profileDir)) {
        $seed = @('--language', $lang, '--profile-directory', $profileDir, '--quit-after-ms', '5000')
        if ($profileKind -eq 'books') { $seed += @('--add-library-root', $books) }
        $p = Start-Process -FilePath $exePath -PassThru -ArgumentList (Quote $seed)
        if (-not $p.WaitForExit(90000)) { $p.Kill() }
    }
    $file = Join-Path $Out "$name.png"
    $argList = @('--language', $lang, '--profile-directory', $profileDir, '--window-width', "$w", '--window-height', "$h",
                 '--screenshot-file', $file, '--screenshot-delay-ms', "$delay", '--quit-after-ms', "$($delay + 4000)") + $extra
    $p = Start-Process -FilePath $exePath -PassThru -ArgumentList (Quote $argList)
    if (-not $p.WaitForExit(90000)) { $p.Kill() }
    if (Test-Path $file) {
        $img = [System.Drawing.Image]::FromFile($file)
        $size = "$($img.Width)x$($img.Height)"
        $img.Dispose()
        $summary.Add("$name $size")
        return $size
    }
    $summary.Add("$name MISSING")
    Write-Warning "No screenshot for $name"
    $null
}

# Surfaces and states: name, --show value, book, delay
$surfaces = @(
    @('library', $null, $null, 2500),
    @('library-empty', $null, $null, 2500),
    @('bookmarks', 'bookmarks-select:0', $outline, 2500),
    @('menu', 'bookmarks-menu', $outline, 2500),
    @('preferences', 'preferences', $outline, 2500),
    @('about', 'about', $outline, 2500),
    @('autocreate', 'autobookmarks', $headings, 4000),
    @('save', 'bookmarks-autocreate-save', $headings, 5000),
    @('export', 'bookmarks-export', $outline, 2500),
    @('backstage', 'backstage', $outline, 2500),
    @('restricted', 'capability', $restricted, 3000),
    @('password', $null, $password, 2500),
    @('damaged-outline', 'bookmarks', $malformed, 3000))
$narrow = @('bookmarks', 'preferences', 'autocreate', 'save', 'menu', 'library')

function Pass([string]$tag, [int]$w, [int]$h, [string[]]$only) {
    foreach ($lang in $Languages) {
        foreach ($s in $surfaces) {
            if ($only -and $s[0] -notin $only) { continue }
            $extra = @()
            if ($s[1]) { $extra += @('--show', $s[1]) }
            if ($s[2]) { $extra += $s[2] }
            $kind = if ($s[0] -eq 'library-empty') { 'empty' } else { 'books' }
            Shot "$tag-$lang-$($s[0])" $lang $extra $w $h $s[3] $kind | Out-Null
        }
    }
}

try {
    $screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $summary.Add("screen $($screen.Width)x$($screen.Height)")

    # 100 %: wide and narrow (the minimum window size).
    Pass 'p100-wide' 1280 820 $null
    Pass 'p100-narrow' 760 560 $narrow

    # 200 %: Windows display scale, confirmed by the screenshot's pixel size.
    $real = [AtlasDisplay]::ScaleSteps(4)
    Start-Sleep -Seconds 3
    $probe = Shot 'p200-probe' 'en' @() 760 560 2500
    $mode = if ($real -and $probe -eq '1520x1120') { 'windows-200' } else { 'qt-scale-2' }
    if ($mode -ne 'windows-200') {
        [AtlasDisplay]::ScaleSteps(0) | Out-Null
        $env:QT_SCALE_FACTOR = '2'
    }
    $summary.Add("200% mode: $mode (probe $probe)")
    Pass "p200-$mode" 760 560 $narrow
    Remove-Item Env:QT_SCALE_FACTOR -ErrorAction SilentlyContinue
    [AtlasDisplay]::ScaleSteps(0) | Out-Null
    Start-Sleep -Seconds 2

    # Windows high contrast.
    $hc = [AtlasDisplay]::HighContrast($true)
    Start-Sleep -Seconds 3
    $summary.Add("high contrast switched on: $hc")
    Pass 'hc-wide' 1280 820 @('library', 'bookmarks', 'preferences', 'save', 'menu')
} finally {
    [AtlasDisplay]::HighContrast($false) | Out-Null
    [AtlasDisplay]::ScaleSteps(0) | Out-Null
    $summary | Out-File (Join-Path $Out 'summary.txt') -Encoding utf8
    Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
}
$summary | ForEach-Object { Write-Host $_ }
$missing = @($summary | Where-Object { $_ -match 'MISSING' })
if ($missing.Count -gt 0) { throw "Missing screenshots: $($missing -join ', ')" }
