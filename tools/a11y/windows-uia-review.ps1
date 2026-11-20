<#
.SYNOPSIS
  Windows keyboard and screen-reader review through UI Automation (N4-04).

  Narrator reads the same UI Automation tree this script reads, so it checks
  what a Narrator user hears and where the keyboard goes, in EN/AR/UR:

  * Bookmarks tree: focus follows the arrow keys (Up/Down, Home/End, open and
    close with the mirrored keys in Arabic and Urdu); every row is a named
    TreeItem with its level in the help text; the native ExpandCollapse
    pattern reports and changes the open/closed state.
  * Editing by keyboard: Ctrl+B, name, Enter, Delete, Ctrl+Z, Alt+Q/Escape,
    Ctrl+S and Enter saves into the PDF.
  * Tab order: Tab walks the window without a trap; every stop is named.
  * Library: focus starts on a book and moves with the arrow keys.
  * About: opens with focus on a named dialog; Escape closes it.

  Uses only Windows' own UI Automation client (.NET Framework), so it runs in
  Windows PowerShell 5.1:

    powershell -NoProfile -File tools/a11y/windows-uia-review.ps1 -Exe <atlas_reader.exe> -Pdf <A003> -Out <dir>
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Pdf,
    [Parameter(Mandatory = $true)][string]$Out,
    [string[]]$Languages = @('en', 'ar', 'ur')
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Windows.Forms
$AE = [System.Windows.Automation.AutomationElement]
$ECP = [System.Windows.Automation.ExpandCollapsePattern]
$Expanded = [System.Windows.Automation.ExpandCollapseState]::Expanded
$Collapsed = [System.Windows.Automation.ExpandCollapseState]::Collapsed
$NamedTypes = @('Button', 'CheckBox', 'Edit', 'ComboBox', 'MenuItem', 'TreeItem', 'ListItem',
                'RadioButton', 'TabItem', 'Hyperlink', 'Slider', 'Spinner', 'SplitButton')

$exePath = (Resolve-Path $Exe).Path
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$work = Join-Path ([IO.Path]::GetTempPath()) ('atlas-uia-' + [guid]::NewGuid().ToString('N'))
$books = Join-Path $work 'books'
New-Item -ItemType Directory -Path $books | Out-Null
$book = Join-Path $books 'Accessibility Sample.pdf'
Copy-Item -LiteralPath $Pdf -Destination $book
Copy-Item -LiteralPath $Pdf -Destination (Join-Path $books 'Second Sample.pdf')
foreach ($l in $Languages) { Copy-Item -LiteralPath $Pdf -Destination (Join-Path $books "Edit $l.pdf") }
$script:failures = New-Object System.Collections.Generic.List[string]
$script:log = New-Object System.Collections.Generic.List[object]
$script:proc = $null

function Quote([string[]]$items) { ($items | ForEach-Object { if ($_ -match '[\s"]') { '"' + $_ + '"' } else { $_ } }) -join ' ' }
function Fail([string]$lang, [string]$message) { $script:failures.Add("${lang}: $message"); Write-Host "  FAIL $message" }

function Focused {
    $el = $AE::FocusedElement
    if ($null -eq $el) { return [pscustomobject]@{ type = $null; name = ''; help = ''; state = $null } }
    $c = $el.Current
    $type = $c.ControlType.ProgrammaticName -replace '^ControlType\.', ''
    $state = $null
    $pattern = $null
    if ($type -eq 'TreeItem' -and $el.TryGetCurrentPattern($ECP::Pattern, [ref]$pattern)) { $state = [string]$pattern.Current.ExpandCollapseState }
    [pscustomobject]@{ type = $type; name = [string]$c.Name; help = [string]$c.HelpText; state = $state; element = $el }
}

function Press([string]$keys) {
    [System.Windows.Forms.SendKeys]::SendWait($keys)
    Start-Sleep -Milliseconds 600
    Focused
}

function WaitFocus([string]$type, [int]$seconds = 12) {
    $deadline = (Get-Date).AddSeconds($seconds)
    $f = Focused
    while ((Get-Date) -lt $deadline -and $f.type -ne $type) { Start-Sleep -Milliseconds 400; $f = Focused }
    $f
}

function Launch([string]$lang, [string[]]$extra) {
    $profileDir = Join-Path $work "profile-$lang"
    if (-not (Test-Path $profileDir)) {
        $p = Start-Process -FilePath $exePath -PassThru -ArgumentList (Quote @('--language', $lang, '--profile-directory', $profileDir, '--add-library-root', $books, '--quit-after-ms', '5000'))
        if (-not $p.WaitForExit(90000)) { $p.Kill() }
    }
    $script:proc = Start-Process -FilePath $exePath -PassThru -ArgumentList (Quote (@('--language', $lang, '--profile-directory', $profileDir, '--quit-after-ms', '180000') + $extra))
    $condition = New-Object System.Windows.Automation.PropertyCondition($AE::ProcessIdProperty, $script:proc.Id)
    $deadline = (Get-Date).AddSeconds(40)
    while ((Get-Date) -lt $deadline) {
        $window = $AE::RootElement.FindFirst([System.Windows.Automation.TreeScope]::Children, $condition)
        if ($window) {
            Start-Sleep -Seconds 3   # let the QA surface open
            try { $window.SetFocus() } catch { }
            Start-Sleep -Seconds 1
            return $window
        }
        Start-Sleep -Milliseconds 500
    }
    $null
}

function Close-App {
    if ($script:proc -and -not $script:proc.HasExited) { $script:proc.Kill(); $script:proc.WaitForExit(20000) | Out-Null }
    $script:proc = $null
}

function Record([string]$lang, [string]$check, [string]$step, $f) {
    $script:log.Add([pscustomobject]@{ lang = $lang; check = $check; step = $step; type = $f.type; name = $f.name; help = $f.help; state = $f.state })
    Write-Host ("  {0,-28} -> {1} '{2}' help='{3}' state={4}" -f $step, $f.type, $f.name, $f.help, $f.state)
}

function Check-Bookmarks([string]$lang) {
    Write-Host "[$lang] bookmarks tree"
    $rtl = $lang -in @('ar', 'ur')
    $deeper = if ($rtl) { '{LEFT}' } else { '{RIGHT}' }
    $shallower = if ($rtl) { '{RIGHT}' } else { '{LEFT}' }
    if (-not (Launch $lang @('--show', 'bookmarks-select:0', $book))) { Fail $lang 'window did not appear'; return }
    try {
        $steps = @(
            @('open on first row', $null, 'Chapter 1', 'Expanded'),
            @('Down', '{DOWN}', 'Section 1.1', $null),
            @('Down', '{DOWN}', 'Chapter 2', $null),
            @('Home', '{HOME}', 'Chapter 1', 'Expanded'),
            @('End', '{END}', 'Chapter 2', $null),
            @('Home', '{HOME}', 'Chapter 1', 'Expanded'),
            @('close (shallower key)', $shallower, 'Chapter 1', 'Collapsed'),
            @('Down skips hidden child', '{DOWN}', 'Chapter 2', $null),
            @('Up', '{UP}', 'Chapter 1', 'Collapsed'),
            @('open (deeper key)', $deeper, 'Chapter 1', 'Expanded'),
            @('deeper key enters child', $deeper, 'Section 1.1', $null),
            @('shallower key to parent', $shallower, 'Chapter 1', 'Expanded'))
        foreach ($s in $steps) {
            $f = if ($s[1]) { Press $s[1] } else { WaitFocus 'TreeItem' }
            Record $lang 'bookmarks' $s[0] $f
            if ($f.type -ne 'TreeItem' -or $f.name -ne $s[2]) { Fail $lang "$($s[0]): focus $($f.type) '$($f.name)', expected TreeItem '$($s[2])'" }
            if (-not $f.help) { Fail $lang "$($s[0]): tree item has no help text (level/page)" }
            elseif ($f.help -notmatch '\d') { Fail $lang "$($s[0]): help text '$($f.help)' has no level or page number" }
            if ($s[3] -and $f.state -ne $s[3]) { Fail $lang "$($s[0]): state $($f.state), expected $($s[3])" }
        }
        # The ExpandCollapse pattern behind Narrator's expand/collapse commands.
        $row = (Focused).element
        $pattern = $null
        if ($row -and $row.TryGetCurrentPattern($ECP::Pattern, [ref]$pattern)) {
            $pattern.Collapse(); Start-Sleep -Milliseconds 800
            $after = [string]$pattern.Current.ExpandCollapseState
            Record $lang 'bookmarks' 'UIA Collapse()' ([pscustomobject]@{ type = 'TreeItem'; name = $row.Current.Name; help = ''; state = $after })
            if ($after -ne 'Collapsed') { Fail $lang "UIA Collapse() left state $after" }
            $pattern.Expand(); Start-Sleep -Milliseconds 800
            $after = [string]$pattern.Current.ExpandCollapseState
            Record $lang 'bookmarks' 'UIA Expand()' ([pscustomobject]@{ type = 'TreeItem'; name = $row.Current.Name; help = ''; state = $after })
            if ($after -ne 'Expanded') { Fail $lang "UIA Expand() left state $after" }
        } else {
            Fail $lang 'expandable row offers no ExpandCollapse pattern'
        }
        Check-TabOrder $lang
    } finally { Close-App }
}

function TreeNames($window) {
    $cond = New-Object System.Windows.Automation.PropertyCondition($AE::ControlTypeProperty, [System.Windows.Automation.ControlType]::TreeItem)
    @($window.FindAll([System.Windows.Automation.TreeScope]::Descendants, $cond) | ForEach-Object { $_.Current.Name })
}

# Editing with the keyboard only (N5 review checks 1 and 4): add, name, delete,
# undo, save with Enter. Regressions this guards: after naming a bookmark the
# hidden rename field kept the keyboard (Delete and Ctrl+Z did nothing); Enter
# did not press Save; the title-bar search kept the keyboard after closing.
function Check-Editing([string]$lang) {
    Write-Host "[$lang] editing with the keyboard"
    $edit = Join-Path $books "Edit $lang.pdf"
    $before = (Get-FileHash -LiteralPath $edit -Algorithm SHA256).Hash
    $window = Launch $lang @('--show', 'bookmarks-select:0', $edit)
    if (-not $window) { Fail $lang 'window did not appear'; return }
    try {
        WaitFocus 'TreeItem' | Out-Null
        $f = Press '^b'
        Record $lang 'edit' 'Ctrl+B' $f
        if ($f.type -ne 'Edit') { Fail $lang "Ctrl+B did not open the name field (focus $($f.type) '$($f.name)')" }
        [System.Windows.Forms.SendKeys]::SendWait('^a'); Start-Sleep -Milliseconds 300
        [System.Windows.Forms.SendKeys]::SendWait('QA'); Start-Sleep -Milliseconds 400
        $f = Press '{ENTER}'
        Record $lang 'edit' 'name + Enter' $f
        if ($f.type -ne 'TreeItem' -or $f.name -ne 'QA') { Fail $lang "after naming, focus is $($f.type) '$($f.name)', expected TreeItem 'QA'" }
        $f = Press '{DELETE}'
        Record $lang 'edit' 'Delete' $f
        if ((TreeNames $window) -contains 'QA') { Fail $lang 'Delete did not remove the new bookmark (keyboard stuck after naming)' }
        $f = Press '^z'
        Record $lang 'edit' 'Ctrl+Z' $f
        if ((TreeNames $window) -notcontains 'QA') { Fail $lang 'Ctrl+Z did not bring the bookmark back' }
        # Title-bar search: Escape hands the keyboard back (not to a hidden field).
        Press '%q' | Out-Null
        $f = Press '{ESC}'
        Record $lang 'edit' 'Alt+Q, Escape' $f
        if ($f.type -eq 'Edit') { Fail $lang 'after closing the title-bar search the keyboard is still in its field' }
        $f = Press '^s'
        Start-Sleep -Milliseconds 800
        $f = Focused
        Record $lang 'edit' 'Ctrl+S' $f
        if (-not $f.name) { Fail $lang "Save dialog opens with focus on unnamed $($f.type)" }
        Press '{ENTER}' | Out-Null
        $deadline = (Get-Date).AddSeconds(25)
        $after = $before
        while ((Get-Date) -lt $deadline -and $after -eq $before) { Start-Sleep -Milliseconds 700; $after = (Get-FileHash -LiteralPath $edit -Algorithm SHA256).Hash }
        Write-Host ("  Save with Enter: file changed = {0}" -f ($after -ne $before))
        if ($after -eq $before) { Fail $lang 'Enter in the Save dialog did not save the bookmarks into the PDF' }
    } finally { Close-App }
}

function Check-TabOrder([string]$lang) {
    $start = Focused
    $startKey = "$($start.type)|$($start.name)"
    $seen = New-Object System.Collections.Generic.List[string]
    $moved = $false
    for ($i = 1; $i -le 60; $i++) {
        $f = Press '{TAB}'
        $key = "$($f.type)|$($f.name)"
        $script:log.Add([pscustomobject]@{ lang = $lang; check = 'tab'; step = "Tab $i"; type = $f.type; name = $f.name; help = $f.help; state = $f.state })
        if ($key -ne $startKey) { $moved = $true }
        if ($f.type -in $NamedTypes -and -not $f.name.Trim()) { Fail $lang "Tab ${i}: unnamed $($f.type)" }
        if ($seen.Count -gt 0 -and $key -eq $seen[0]) { break }   # full cycle, no trap
        $seen.Add($key)
    }
    Write-Host ("  Tab order ({0} stops): {1}" -f $seen.Count, (($seen | ForEach-Object { $_ -replace '\|', ':' }) -join ' | '))
    if (-not $moved) { Fail $lang 'Tab never moved focus (keyboard trap)' }
    # Two different controls with the same name and type sound identical to Narrator.
    $dupes = @($seen | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    foreach ($d in $dupes) { Fail $lang "two Tab stops are announced the same: $($d -replace '\|', ' ')" }
}

function Check-Library([string]$lang) {
    Write-Host "[$lang] library"
    if (-not (Launch $lang @())) { Fail $lang 'window did not appear'; return }
    try {
        $first = WaitFocus 'ListItem'
        Record $lang 'library' 'start' $first
        $second = Press '{DOWN}'
        Record $lang 'library' 'Down' $second
        if ($first.type -ne 'ListItem' -or -not $first.name) { Fail $lang "library starts with focus on $($first.type) '$($first.name)', expected a book" }
        if ($second.type -ne 'ListItem' -or $second.name -eq $first.name) { Fail $lang "Down did not move to the next book ($($second.type) '$($second.name)')" }
    } finally { Close-App }
}

function Check-About([string]$lang) {
    Write-Host "[$lang] About dialog"
    if (-not (Launch $lang @('--show', 'about', $book))) { Fail $lang 'window did not appear'; return }
    try {
        $open = Focused
        Record $lang 'about' 'open' $open
        if (-not $open.name) { Fail $lang "About opens with focus on unnamed $($open.type)" }
        $closed = Press '{ESC}'
        Record $lang 'about' 'Escape' $closed
        if ($closed.name -eq $open.name -and $closed.type -eq $open.type) { Fail $lang 'Escape did not close About' }
    } finally { Close-App }
}

try {
    foreach ($lang in $Languages) {
        Check-Bookmarks $lang
        Check-Editing $lang
        Check-Library $lang
        Check-About $lang
    }
} finally {
    Close-App
    $script:log | ConvertTo-Json -Depth 3 | Out-File (Join-Path $Out 'uia-review.json') -Encoding utf8
    Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
}
if ($script:failures.Count -gt 0) { throw ("UI Automation review failed:`n  " + ($script:failures -join "`n  ")) }
Write-Host 'UI Automation review: all checks pass.'
