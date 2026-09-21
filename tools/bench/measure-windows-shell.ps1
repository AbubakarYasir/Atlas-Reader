param(
    [string]$AppPath = "",
    [string]$QtBin = "",
    [ValidateRange(2, 50)]
    [int]$Iterations = 6,
    [ValidateSet("en", "ar")]
    [string]$Language = "en",
    [ValidateSet("system", "light", "dark")]
    [string]$Theme = "system",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path

if ([string]::IsNullOrWhiteSpace($AppPath)) {
    $AppPath = Join-Path $repoRoot "build/windows-msvc2022/Release/atlas_reader.exe"
}

$AppPath = (Resolve-Path $AppPath).Path

if (-not [string]::IsNullOrWhiteSpace($QtBin)) {
    $QtBin = (Resolve-Path $QtBin).Path
    $env:PATH = "$QtBin;$env:PATH"
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $repoRoot "artifacts/bench/n1-windows-shell-$stamp.json"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

function Get-MetricValue {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Metrics,
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $item = $Metrics | Where-Object { $_.metric -eq $Name } | Select-Object -Last 1
    if ($null -eq $item) {
        return $null
    }
    return [double]$item.value
}

function Get-Average {
    param([object[]]$Values)
    $numbers = @($Values | Where-Object { $null -ne $_ } | ForEach-Object { [double]$_ })
    if ($numbers.Count -eq 0) {
        return $null
    }
    return [math]::Round(($numbers | Measure-Object -Average).Average, 3)
}

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem
$gpu = Get-CimInstance Win32_VideoController | Select-Object -First 1
$logicalProcessors = [Math]::Max(1, [int]$computer.NumberOfLogicalProcessors)

$gitSha = "unknown"
try {
    $gitSha = (git -C $repoRoot rev-parse HEAD 2>$null).Trim()
} catch {
    $gitSha = "unknown"
}

$runs = @()

for ($i = 1; $i -le $Iterations; $i++) {
    $metricsPath = Join-Path $env:TEMP "atlas-n1-metrics-$PID-$i.jsonl"
    Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath

    $arguments = @(
        "--benchmark-shell",
        "--quit-after-ms", "4500",
        "--metrics-file", "`"$metricsPath`"",
        "--language", $Language,
        "--theme", $Theme
    )

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process -FilePath $AppPath -ArgumentList $arguments -PassThru

    $inputIdleMs = $null
    try {
        if ($process.WaitForInputIdle(5000)) {
            $inputIdleMs = [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 3)
        }
    } catch {
        $inputIdleMs = $null
    }

    Start-Sleep -Milliseconds 1200
    $process.Refresh()

    $workingSetMiB = [math]::Round($process.WorkingSet64 / 1MB, 3)
    $privateMiB = [math]::Round($process.PrivateMemorySize64 / 1MB, 3)
    $cpuStartMs = $process.TotalProcessorTime.TotalMilliseconds

    Start-Sleep -Milliseconds 1000
    $process.Refresh()
    $cpuDeltaMs = $process.TotalProcessorTime.TotalMilliseconds - $cpuStartMs
    $normalizedCpuPercent = [math]::Round((($cpuDeltaMs / 1000.0) / $logicalProcessors) * 100.0, 3)

    $process.WaitForExit()
    $stopwatch.Stop()

    $metricObjects = @()
    if (Test-Path $metricsPath) {
        $metricObjects = @(
            Get-Content $metricsPath |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                ForEach-Object { $_ | ConvertFrom-Json }
        )
    }

    $runs += [pscustomobject]@{
        iteration = $i
        cache_class = if ($i -eq 1) { "cold_candidate" } else { "warm_candidate" }
        process_runtime_ms = [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 3)
        input_idle_ms = $inputIdleMs
        working_set_mib = $workingSetMiB
        private_memory_mib = $privateMiB
        idle_cpu_percent_one_second = $normalizedCpuPercent
        qml_loaded_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.qml_loaded_ms"
        first_frame_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.first_frame_ms"
        resize_frame_samples = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_samples"
        resize_frame_p50_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p50_ms"
        resize_frame_p95_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p95_ms"
        resize_frame_p99_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p99_ms"
        shutdown_ms = Get-MetricValue -Metrics $metricObjects -Name "lifecycle.shutdown_ms"
    }

    Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath
}

$warmRuns = @($runs | Where-Object { $_.cache_class -eq "warm_candidate" })

$report = [pscustomobject]@{
    schema = "atlas.n1.windows-shell-baseline.v1"
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    git_sha = $gitSha
    app_path = $AppPath
    language = $Language
    theme = $Theme
    note = "The first launch is a cold-cache candidate, not a guaranteed laboratory cold-cache run. Compare like-for-like runs on the same machine."
    machine = [pscustomobject]@{
        os_caption = $os.Caption
        os_version = $os.Version
        os_build = $os.BuildNumber
        cpu = $cpu.Name
        logical_processors = $logicalProcessors
        ram_gib = [math]::Round($computer.TotalPhysicalMemory / 1GB, 2)
        gpu = $gpu.Name
        refresh_hz = $gpu.CurrentRefreshRate
    }
    summary = [pscustomobject]@{
        cold_first_frame_ms = if ($runs.Count -gt 0) { $runs[0].first_frame_ms } else { $null }
        warm_first_frame_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.first_frame_ms })
        warm_working_set_mib_average = Get-Average ($warmRuns | ForEach-Object { $_.working_set_mib })
        warm_private_memory_mib_average = Get-Average ($warmRuns | ForEach-Object { $_.private_memory_mib })
        warm_idle_cpu_percent_average = Get-Average ($warmRuns | ForEach-Object { $_.idle_cpu_percent_one_second })
        warm_resize_p95_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.resize_frame_p95_ms })
        warm_resize_p99_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.resize_frame_p99_ms })
    }
    runs = $runs
}

$report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

Write-Host "Atlas N1 shell baseline written to: $OutputPath"
$report.summary | Format-List
