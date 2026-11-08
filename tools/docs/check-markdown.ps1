[CmdletBinding()]
param()

# Documentation governance for Atlas Reader.
# 1. Every relative Markdown link resolves (frozen docs/history is skipped).
# 2. Live status markers agree.
# 3. Closed evidence records keep their historical status.
# 4. CHECKPOINTS.md keeps Automated QA / Owner review / Stop gate for N4-N11.
# 5. Key cross-references exist.

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$sep = [IO.Path]::DirectorySeparatorChar
$ignoredSegments = @('artifacts', 'build', 'out', 'vendor', 'third_party', 'docs/history') |
    ForEach-Object { $sep + ($_ -replace '/', $sep) + $sep }
$markdownFiles = Get-ChildItem -LiteralPath $repoRoot -Recurse -File -Filter '*.md' |
    Where-Object {
        $path = $_.FullName
        -not ($ignoredSegments | Where-Object { $path.Contains($_) })
    }

$errors = [System.Collections.Generic.List[string]]::new()

foreach ($file in $markdownFiles) {
    $text = Get-Content -LiteralPath $file.FullName -Raw
    foreach ($match in [regex]::Matches($text, '\[[^\]]*\]\(([^)]+)\)')) {
        $target = $match.Groups[1].Value.Trim()
        if ($target -match '^(https?://|mailto:|#|codex:)' -or $target -match '^<.*>$') {
            continue
        }

        $pathPart = ($target -split '#', 2)[0]
        if ([string]::IsNullOrWhiteSpace($pathPart)) {
            continue
        }

        $decoded = [uri]::UnescapeDataString($pathPart)
        $resolved = Join-Path $file.DirectoryName $decoded
        if (-not (Test-Path -LiteralPath $resolved)) {
            $relative = [IO.Path]::GetRelativePath($repoRoot, $file.FullName)
            $errors.Add("Broken Markdown link: $relative -> $target")
        }
    }
}

$liveStatusFiles = @(
    'README.md',
    'PLAN.md',
    'CHECKPOINTS.md',
    'AGENTS.md',
    'CHANGELOG.md',
    'docs/README.md',
    'docs/baselines/N5_BOOKMARKS_SAFETY_EVIDENCE.md',
    'docs/N5_REVIEW.md',
    'docs/baselines/N5_UI_SIZE_COMPARISON.md'
)

$expectedStatus = $null
foreach ($relativePath in $liveStatusFiles) {
    $path = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $path)) {
        $errors.Add("Missing live-status document: $relativePath")
        continue
    }

    $text = Get-Content -LiteralPath $path -Raw
    $match = [regex]::Match($text, '<!--\s*atlas-status:\s*([^>]+?)\s*-->')
    if (-not $match.Success) {
        $errors.Add("Missing atlas-status marker: $relativePath")
        continue
    }

    $status = $match.Groups[1].Value.Trim()
    if ($null -eq $expectedStatus) {
        $expectedStatus = $status
    }
    elseif ($status -ne $expectedStatus) {
        $errors.Add("Divergent atlas-status marker: $relativePath has '$status', expected '$expectedStatus'")
    }
}

# Closed evidence records keep their historical status when the active stage advances.
$acceptedHistoryFiles = @{
    'docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md' = 'N3|accepted'
    'docs/baselines/N4_READER_FOUNDATION_EVIDENCE.md' = 'N4|accepted'
    'docs/N4_REVIEW.md' = 'N4|accepted'
}
foreach ($acceptedHistory in $acceptedHistoryFiles.GetEnumerator()) {
    $acceptedHistoryText = Get-Content -LiteralPath (Join-Path $repoRoot $acceptedHistory.Key) -Raw
    $acceptedHistoryMatch = [regex]::Match(
        $acceptedHistoryText,
        '<!--\s*atlas-status:\s*([^>]+?)\s*-->'
    )
    if (-not $acceptedHistoryMatch.Success -or $acceptedHistoryMatch.Groups[1].Value.Trim() -ne $acceptedHistory.Value) {
        $errors.Add("Closed evidence record must retain historical status $($acceptedHistory.Value): $($acceptedHistory.Key)")
    }
}

$checkpointText = Get-Content -LiteralPath (Join-Path $repoRoot 'CHECKPOINTS.md') -Raw
foreach ($number in 4..11) {
    $sectionMatch = [regex]::Match(
        $checkpointText,
        "(?ms)^## N$number\b.*?(?=^## |\z)"
    )
    if (-not $sectionMatch.Success) {
        $errors.Add("Missing stage section N$number in CHECKPOINTS.md")
        continue
    }

    $section = $sectionMatch.Value
    foreach ($requiredHeading in @('Automated QA', 'Owner review', 'Stop gate')) {
        if ($section -notmatch "(?m)^### $([regex]::Escape($requiredHeading))\s*$") {
            $errors.Add("N$number is missing '### $requiredHeading' in CHECKPOINTS.md")
        }
    }
    if ($section -notmatch "N$number PASS") {
        $errors.Add("N$number stop gate must name the explicit owner result 'N$number PASS'")
    }
}

$requiredReferences = @(
    @{ File = 'README.md'; Text = 'CHECKPOINTS.md' },
    @{ File = 'PLAN.md'; Text = 'AFTER_2_0.md' },
    @{ File = 'CHECKPOINTS.md'; Text = 'N4_REVIEW.md' },
    @{ File = 'CHECKPOINTS.md'; Text = 'N5_REVIEW.md' },
    @{ File = 'AGENTS.md'; Text = 'DESIGN.md' },
    @{ File = 'docs/README.md'; Text = 'WORKFLOW.md' },
    @{ File = 'docs/README.md'; Text = 'QUALITY.md' },
    @{ File = 'docs/QUALITY.md'; Text = 'CHECKPOINTS.md' }
)
foreach ($reference in $requiredReferences) {
    $text = Get-Content -LiteralPath (Join-Path $repoRoot $reference.File) -Raw
    if (-not $text.Contains($reference.Text)) {
        $errors.Add("$($reference.File) must reference $($reference.Text)")
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ -ErrorAction Continue }
    throw "Markdown governance validation failed with $($errors.Count) error(s)."
}

Write-Host "Markdown governance PASS: $($markdownFiles.Count) files, status '$expectedStatus'."
