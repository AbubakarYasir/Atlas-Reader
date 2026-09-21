param(
    [ValidateRange(2, 50)]
    [int]$EnglishIterations = 6,
    [ValidateRange(2, 50)]
    [int]$ArabicIterations = 3,
    [switch]$StartupDiagnostics,
    [ValidateRange(2, 10)]
    [int]$DiagnosticIterations = 3
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$packageRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$appPath = Join-Path $packageRoot "atlas_reader.exe"
$measureScript = Join-Path $packageRoot "tools\bench\measure-windows-shell.ps1"

if (-not (Test-Path $appPath)) {
    throw "Portable Atlas executable not found at '$appPath'. Run this script from the unpacked N1 portable artifact without changing its directory layout."
}

$buildInfoPath = Join-Path $packageRoot "N1_BUILD_INFO.txt"
$buildCommit = ""
if (Test-Path $buildInfoPath) {
    Write-Host "=== Canonical build info ==="
    $buildInfo = @(Get-Content $buildInfoPath)
    $buildInfo | ForEach-Object { Write-Host $_ }
    Write-Host ""

    $commitLine = $buildInfo | Where-Object { $_ -match '^Commit:\s*([0-9a-fA-F]{7,40})\s*$' } | Select-Object -First 1
    if ($null -ne $commitLine -and $commitLine -match '^Commit:\s*([0-9a-fA-F]{7,40})\s*$') {
        $buildCommit = $Matches[1]
    }
}

if ($StartupDiagnostics) {
    function Read-DiagnosticMetrics {
        param([string]$Path)

        $items = @()
        if (-not (Test-Path $Path)) {
            return $items
        }

        foreach ($line in Get-Content $Path -ErrorAction SilentlyContinue) {
            if ([string]::IsNullOrWhiteSpace($line)) {
                continue
            }
            try {
                $items += ($line | ConvertFrom-Json)
            } catch {
                # Ignore only an incomplete final line.
            }
        }
        return $items
    }

    function Get-DiagnosticMetricValue {
        param(
            [AllowNull()]
            [AllowEmptyCollection()]
            [object[]]$Metrics,
            [string]$Name
        )

        if ($null -eq $Metrics -or $Metrics.Count -eq 0) {
            return $null
        }

        $item = $Metrics | Where-Object { $_.metric -eq $Name } | Select-Object -Last 1
        if ($null -eq $item) {
            return $null
        }

        return [double]$item.value
    }

    function Get-DiagnosticPercentile {
        param(
            [object[]]$Values,
            [double]$Fraction
        )

        $numbers = @($Values | Where-Object { $null -ne $_ } | ForEach-Object { [double]$_ } | Sort-Object)
        if ($numbers.Count -eq 0) {
            return $null
        }

        $index = [math]::Ceiling($Fraction * ($numbers.Count - 1))
        return [math]::Round($numbers[[int]$index], 3)
    }

    function Invoke-StartupProbe {
        param(
            [string]$Profile,
            [string]$Language,
            [int]$Iteration,
            [bool]$Recorded
        )

        $tag = if ($Recorded) { "m$Iteration" } else { "prewarm" }
        $metricsPath = Join-Path $env:TEMP "atlas-n1-startup-$PID-$Profile-$Language-$tag.jsonl"
        Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath

        $arguments = @(
            "--quit-after-ms", "1200",
            "--metrics-file", "`"$metricsPath`"",
            "--language", $Language,
            "--theme", "system",
            "--shell-profile", $Profile
        )

        $process = Start-Process -FilePath $appPath -ArgumentList $arguments -PassThru -Wait
        if ($process.ExitCode -ne 0) {
            throw "Atlas startup probe failed: profile=$Profile language=$Language exit=$($process.ExitCode)"
        }

        $metrics = @(Read-DiagnosticMetrics -Path $metricsPath)
        $beforeLoad = Get-DiagnosticMetricValue -Metrics $metrics -Name "startup.before_qml_load_ms"
        $qmlLoaded = Get-DiagnosticMetricValue -Metrics $metrics -Name "startup.qml_loaded_ms"
        $firstFrame = Get-DiagnosticMetricValue -Metrics $metrics -Name "startup.first_frame_ms"

        Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath

        if (-not $Recorded) {
            return $null
        }

        return [pscustomobject]@{
            profile = $Profile
            language = $Language
            iteration = $Iteration
            before_qml_load_ms = $beforeLoad
            qml_loaded_ms = $qmlLoaded
            first_frame_ms = $firstFrame
            qml_load_cost_ms = if ($null -ne $beforeLoad -and $null -ne $qmlLoaded) {
                [math]::Round($qmlLoaded - $beforeLoad, 3)
            } else {
                $null
            }
            post_qml_to_first_frame_ms = if ($null -ne $qmlLoaded -and $null -ne $firstFrame) {
                [math]::Round($firstFrame - $qmlLoaded, 3)
            } else {
                $null
            }
        }
    }

    $scenarios = @(
        [pscustomobject]@{ profile = "bare"; language = "en" },
        [pscustomobject]@{ profile = "text"; language = "en" },
        [pscustomobject]@{ profile = "text"; language = "ar" },
        [pscustomobject]@{ profile = "controls"; language = "en" },
        [pscustomobject]@{ profile = "controls"; language = "ar" },
        [pscustomobject]@{ profile = "full"; language = "en" },
        [pscustomobject]@{ profile = "full"; language = "ar" }
    )

    Write-Host "=== Atlas N1 startup diagnostic matrix ==="
    Write-Host "Executable: $appPath"
    Write-Host "Commit: $buildCommit"
    Write-Host "Warm measured iterations per scenario: $DiagnosticIterations"
    Write-Host ""

    $results = @()

    foreach ($scenario in $scenarios) {
        Write-Host "Prewarm: $($scenario.profile) / $($scenario.language)"
        Invoke-StartupProbe -Profile $scenario.profile -Language $scenario.language -Iteration 0 -Recorded $false | Out-Null

        for ($i = 1; $i -le $DiagnosticIterations; $i++) {
            Write-Host "Measure: $($scenario.profile) / $($scenario.language) [$i/$DiagnosticIterations]"
            $results += Invoke-StartupProbe `
                -Profile $scenario.profile `
                -Language $scenario.language `
                -Iteration $i `
                -Recorded $true
        }
    }

    $summary = @()
    foreach ($scenario in $scenarios) {
        $scenarioRuns = @($results | Where-Object {
            $_.profile -eq $scenario.profile -and $_.language -eq $scenario.language
        })

        $summary += [pscustomobject]@{
            profile = $scenario.profile
            language = $scenario.language
            samples = $scenarioRuns.Count
            first_frame_p50_ms = Get-DiagnosticPercentile ($scenarioRuns | ForEach-Object { $_.first_frame_ms }) 0.50
            first_frame_p95_ms = Get-DiagnosticPercentile ($scenarioRuns | ForEach-Object { $_.first_frame_ms }) 0.95
            qml_load_cost_p50_ms = Get-DiagnosticPercentile ($scenarioRuns | ForEach-Object { $_.qml_load_cost_ms }) 0.50
            qml_load_cost_p95_ms = Get-DiagnosticPercentile ($scenarioRuns | ForEach-Object { $_.qml_load_cost_ms }) 0.95
            post_qml_to_first_frame_p95_ms = Get-DiagnosticPercentile ($scenarioRuns | ForEach-Object { $_.post_qml_to_first_frame_ms }) 0.95
        }
    }

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $outputPath = Join-Path $packageRoot "artifacts\bench\n1-startup-diagnostics-$stamp.json"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null

    $report = [pscustomobject]@{
        schema = "atlas.n1.startup-diagnostics.v1"
        generated_utc = (Get-Date).ToUniversalTime().ToString("o")
        git_sha = if ([string]::IsNullOrWhiteSpace($buildCommit)) { "unknown" } else { $buildCommit }
        note = "Diagnostic-only warm startup matrix. Each scenario is prewarmed once; prewarm runs are not included in the summary. This is not an N1 acceptance report."
        scenarios = $summary
        runs = $results
    }

    $report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $outputPath

    Write-Host ""
    Write-Host "Startup diagnostic report written to:"
    Write-Host $outputPath
    Write-Host ""
    $summary | Format-Table -AutoSize
    return
}

if (-not (Test-Path $measureScript)) {
    throw "Measurement script not found at '$measureScript'. The N1 artifact may be incomplete."
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$englishOutput = Join-Path $packageRoot "artifacts\bench\n1-portable-en-$stamp.json"
$arabicOutput = Join-Path $packageRoot "artifacts\bench\n1-portable-ar-$stamp.json"

Write-Host "=== Atlas N1 portable physical-machine qualification ==="
Write-Host "Executable: $appPath"
Write-Host ""

Write-Host "[1/2] English/LTR system-theme measurement"
& $measureScript `
    -AppPath $appPath `
    -Iterations $EnglishIterations `
    -Language en `
    -Theme system `
    -OutputPath $englishOutput `
    -BuildCommit $buildCommit

Write-Host "[2/2] Arabic/RTL dark-theme measurement"
& $measureScript `
    -AppPath $appPath `
    -Iterations $ArabicIterations `
    -Language ar `
    -Theme dark `
    -OutputPath $arabicOutput `
    -BuildCommit $buildCommit

Write-Host ""
Write-Host "Automated physical-machine measurements completed."
Write-Host "English report: $englishOutput"
Write-Host "Arabic report:  $arabicOutput"
Write-Host ""
Write-Host "Manual checks still required before N1 PASS:"
Write-Host "  [ ] English/LTR layout and keyboard focus"
Write-Host "  [ ] Arabic/RTL mirroring and mixed-script readability"
Write-Host "  [ ] Light and dark themes"
Write-Host "  [ ] Windows display scale at 100%"
Write-Host "  [ ] Windows display scale at 200%"
Write-Host "  [ ] Repeated open/close leaves no Atlas process"
Write-Host ""
Write-Host "Keep both generated JSON reports; they contain the machine/performance evidence needed for the N1 record."
