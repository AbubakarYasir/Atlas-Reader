<#
.SYNOPSIS
  UI-SIZE-01 reference measurement: opens a test PDF in Foxit PDF Reader (the
  free reader, installed only on a disposable CI machine), right-clicks the
  first bookmark and records the bookmark menu's item sizes from UI
  Automation plus a screenshot, at Windows 100 %. Measurement only: nothing
  from Foxit is copied into Atlas (FOXIT_PARITY.md).

    pwsh -NoProfile -File tools/review/capture-foxit-bookmark-menu.ps1 -Pdf <A003> -Out <dir>
#>
param(
    [Parameter(Mandatory = $true)][string]$Pdf,
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$Reader = ''
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Drawing, System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class AtlasInput {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
    public static void RightClick(int x, int y) {
        SetCursorPos(x, y);
        mouse_event(0x0008, 0, 0, 0, UIntPtr.Zero); // right down
        mouse_event(0x0010, 0, 0, 0, UIntPtr.Zero); // right up
    }
    public static void Click(int x, int y) {
        SetCursorPos(x, y);
        mouse_event(0x0002, 0, 0, 0, UIntPtr.Zero);
        mouse_event(0x0004, 0, 0, 0, UIntPtr.Zero);
    }
}
'@
$AE = [System.Windows.Automation.AutomationElement]
$TS = [System.Windows.Automation.TreeScope]
New-Item -ItemType Directory -Force -Path $Out | Out-Null

function Shot([string]$name) {
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $bmp.Save((Join-Path $Out "$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
}

function Find-All([System.Windows.Automation.AutomationElement]$root, $type) {
    $cond = New-Object System.Windows.Automation.PropertyCondition($AE::ControlTypeProperty, $type)
    $root.FindAll($TS::Descendants, $cond)
}

if (-not $Reader) {
    $Reader = @(Get-ChildItem 'C:\Program Files*\Foxit Software' -Recurse -Filter 'FoxitPDFReader.exe' -ErrorAction SilentlyContinue)[0].FullName
}
if (-not $Reader) { throw 'Foxit PDF Reader is not installed.' }
$book = Join-Path $env:RUNNER_TEMP 'Outline Sample.pdf'
Copy-Item -LiteralPath $Pdf -Destination $book -Force
$proc = Start-Process -FilePath $Reader -ArgumentList "`"$book`"" -PassThru
$report = [ordered]@{ reader = (Get-Item $Reader).VersionInfo.ProductVersion; screen = "$([System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Width)x$([System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Height)" }
try {
    Start-Sleep -Seconds 25
    Shot '01-opened'
    $window = $null
    $deadline = (Get-Date).AddSeconds(60)
    while (-not $window -and (Get-Date) -lt $deadline) {
        foreach ($w in $AE::RootElement.FindAll($TS::Children, [System.Windows.Automation.Condition]::TrueCondition)) {
            if ($w.Current.Name -match 'Outline Sample' -or $w.Current.ProcessId -eq $proc.Id) { $window = $w; break }
        }
        if (-not $window) { Start-Sleep -Seconds 2 }
    }
    if (-not $window) { throw 'Foxit window not found' }
    $report.window = $window.Current.Name
    try { $window.SetFocus() } catch { }
    # Dismiss first-run panels with Escape, then find the bookmark rows.
    [System.Windows.Forms.SendKeys]::SendWait('{ESC}'); Start-Sleep -Seconds 2
    $items = Find-All $window ([System.Windows.Automation.ControlType]::TreeItem)
    $target = $null
    foreach ($i in $items) { if ($i.Current.Name -match 'Chapter') { $target = $i; break } }
    $wr = $window.Current.BoundingRectangle
    $report.windowRect = [ordered]@{ x = $wr.X; y = $wr.Y; w = $wr.Width; h = $wr.Height }
    if (-not $target) {
        # Open the Bookmarks panel: the first icon of the left navigation bar
        # (Foxit gives it no UI Automation name), 22 px in and 231 px down.
        [AtlasInput]::Click([int]($wr.X + 22), [int]($wr.Y + 231)); Start-Sleep -Seconds 3
        $items = Find-All $window ([System.Windows.Automation.ControlType]::TreeItem)
        foreach ($i in $items) { if ($i.Current.Name -match 'Chapter') { $target = $i; break } }
    }
    Shot '02-bookmarks'
    # Everything Foxit exposes, for calibrating this script.
    $all = $window.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
    $report.elements = @(foreach ($e in $all) {
        $b = $e.Current.BoundingRectangle
        if ($b.Width -gt 0) { '{0} | {1} | {2},{3} {4}x{5}' -f ($e.Current.ControlType.ProgrammaticName -replace 'ControlType\.', ''), $e.Current.Name, $b.X, $b.Y, $b.Width, $b.Height }
    })
    if ($target) {
        $r = $target.Current.BoundingRectangle
        $report.row = [ordered]@{ name = $target.Current.Name; x = $r.X; y = $r.Y; w = $r.Width; h = $r.Height }
        [AtlasInput]::RightClick([int]($r.X + 30), [int]($r.Y + $r.Height / 2))
    } else {
        # Rows not exposed: right-click where the first row sits in the panel.
        $report.row = 'not exposed; right-clicked at the first row position'
        [AtlasInput]::RightClick([int]($wr.X + 90), [int]($wr.Y + 262))
    }
    Start-Sleep -Seconds 2
    Shot '03-menu'
    $menuItems = Find-All $AE::RootElement ([System.Windows.Automation.ControlType]::MenuItem)
    $rows = @()
    foreach ($m in $menuItems) {
        $b = $m.Current.BoundingRectangle
        if ($b.Width -gt 0) { $rows += [ordered]@{ name = $m.Current.Name; x = $b.X; y = $b.Y; w = $b.Width; h = $b.Height } }
    }
    $menus = Find-All $AE::RootElement ([System.Windows.Automation.ControlType]::Menu)
    $report.menus = @(foreach ($m in $menus) { $b = $m.Current.BoundingRectangle; [ordered]@{ name = $m.Current.Name; x = $b.X; y = $b.Y; w = $b.Width; h = $b.Height } })
    $report.menuItems = $rows
} finally {
    $report | ConvertTo-Json -Depth 5 | Out-File (Join-Path $Out 'foxit-bookmark-menu.json') -Encoding utf8
    if ($proc -and -not $proc.HasExited) { $proc.Kill() }
}
$report | ConvertTo-Json -Depth 5
