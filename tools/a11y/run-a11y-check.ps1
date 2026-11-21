<#
.SYNOPSIS
  Windows accessibility sweep: opens each surface in EN/AR/UR, dumps the
  accessibility tree (what Narrator reaches through UI Automation) and fails on
  unnamed controls or a Bookmarks tree without named rows.

    pwsh tools/a11y/run-a11y-check.ps1 -Exe <atlas_reader.exe> -Pdf <book with outline>
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Pdf,
    [string]$HeadingPdf = '',
    [string[]]$Languages = @('en', 'ar', 'ur'),
    [string[]]$Surfaces = @('library', 'bookmarks', 'preferences', 'backstage', 'about', 'command', 'autobookmarks', 'tab-autocreate-save', 'tab-export', 'tab-menu')
)
$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path $Exe).Path
$work = Join-Path ([IO.Path]::GetTempPath()) ("atlas-a11y-" + [guid]::NewGuid().ToString('N'))
$books = Join-Path $work 'books'
New-Item -ItemType Directory -Path $books | Out-Null
$book = Join-Path $books 'Accessibility Sample.pdf'  # a space on purpose: paths must survive quoting
Copy-Item -LiteralPath $Pdf -Destination $book
$checker = Join-Path $PSScriptRoot 'check_accessibility.py'
# Where keyboard and screen-reader focus must be when each surface opens.
$expectFocus = @{
    'library' = 'ListItem,EditableText'; 'bookmarks' = 'TreeItem'; 'preferences' = 'Dialog'
    'backstage' = 'MenuItem'; 'about' = 'Dialog'; 'command' = 'EditableText'; 'autobookmarks' = 'Dialog'
    'tab-autocreate-save' = 'Dialog'; 'tab-export' = 'Dialog'; 'tab-menu' = 'PopupMenu,MenuItem'
}
# Auto Create needs a book with headings (A003 has an outline, not headings).
$headingBook = $null
if ($HeadingPdf) {
    $headingBook = Join-Path $books 'Heading Sample.pdf'
    Copy-Item -LiteralPath $HeadingPdf -Destination $headingBook
}
$failed = @()
# Start-Process joins -ArgumentList without quoting, so quote anything with spaces.
function Join-Args([string[]]$items) { ($items | ForEach-Object { if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ } }) -join ' ' }
foreach ($lang in $Languages) {
    $profileDir = Join-Path $work "profile-$lang"
    # First run indexes the folder so the book has a library identity (bookmarks need it).
    $p = Start-Process -FilePath $exePath -PassThru -WindowStyle Hidden -ArgumentList (Join-Args @('--language', $lang, '--profile-directory', $profileDir, '--add-library-root', $books, '--quit-after-ms', '4000'))
    if (-not $p.WaitForExit(60000)) { $p.Kill() }
    foreach ($surface in $Surfaces) {
        $dump = Join-Path $work "$lang-$surface.jsonl"
        $argList = @('--language', $lang, '--profile-directory', $profileDir, '--show', "a11y:$surface@$dump", '--quit-after-ms', '6000')
        if ($surface -ne 'library') { $argList += $(if ($headingBook -and $surface -like '*autocreate*') { $headingBook } else { $book }) }
        $p = Start-Process -FilePath $exePath -PassThru -WindowStyle Hidden -ArgumentList (Join-Args $argList)
        if (-not $p.WaitForExit(60000)) { $p.Kill() }
        if (-not (Test-Path -LiteralPath $dump)) { $failed += "$lang/$surface (no dump)"; Write-Host "MISSING dump $lang/$surface"; continue }
        $checkArgs = @($checker, $dump)
        if ($surface -eq 'bookmarks') { $checkArgs += '--expect-tree' }
        if ($expectFocus.ContainsKey($surface)) { $checkArgs += @('--expect-focus', $expectFocus[$surface]) }
        & python @checkArgs
        if ($LASTEXITCODE -ne 0) { $failed += "$lang/$surface" }
    }
}
Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
if ($failed.Count -gt 0) { throw "Accessibility check failed: $($failed -join ', ')" }
Write-Host 'Accessibility sweep: all surfaces pass.'
