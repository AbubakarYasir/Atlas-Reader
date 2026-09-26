[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ignoredSegments = @('\build\', '\out\', '\vendor\', '\third_party\')
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
    'CHECKPOINTS.md',
    'PLAN.md',
    'README.md',
    'INFO.md',
    'CHANGELOG.md',
    'AGENTS.md',
    'docs/README.md',
    'docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md',
    'docs/DEPENDENCIES_AND_TOOLS.md'
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

$checkpointText = Get-Content -LiteralPath (Join-Path $repoRoot 'CHECKPOINTS.md') -Raw
foreach ($number in 3..11) {
    $sectionMatch = [regex]::Match(
        $checkpointText,
        "(?ms)^## N$number\b.*?(?=^## N$($number + 1)\b|^## Platform checkpoints|\z)"
    )
    if (-not $sectionMatch.Success) {
        $errors.Add("Missing checkpoint section N$number in CHECKPOINTS.md")
        continue
    }

    $section = $sectionMatch.Value
    foreach ($requiredHeading in @('Automated QA', 'Owner test', 'Stop gate')) {
        if ($section -notmatch "(?m)^### $([regex]::Escape($requiredHeading))\s*$" -and
            $section -notmatch "(?m)^\*\*$([regex]::Escape($requiredHeading)):\*\*") {
            $errors.Add("N$number is missing '$requiredHeading' in CHECKPOINTS.md")
        }
    }
}

$requiredReferences = @(
    @{ File = 'docs/README.md'; Text = 'CHECKPOINT_QA_MATRIX.md' },
    @{ File = 'docs/README.md'; Text = 'GIT_WORKFLOW.md' },
    @{ File = 'docs/DEVELOPMENT_WORKFLOW.md'; Text = 'GIT_WORKFLOW.md' },
    @{ File = 'docs/QUALITY_AND_TESTING.md'; Text = 'CHECKPOINT_QA_MATRIX.md' }
)
foreach ($reference in $requiredReferences) {
    $text = Get-Content -LiteralPath (Join-Path $repoRoot $reference.File) -Raw
    if (-not $text.Contains($reference.Text)) {
        $errors.Add("$($reference.File) must reference $($reference.Text)")
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    throw "Markdown governance validation failed with $($errors.Count) error(s)."
}

Write-Host "Markdown governance PASS: $($markdownFiles.Count) files, status '$expectedStatus'."
