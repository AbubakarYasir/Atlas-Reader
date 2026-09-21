param(
    [string]$AppPath = "",
    [string]$QtBin = "",
    [ValidateRange(2, 50)]
    [int]$Iterations = 6,
    [ValidateSet("en", "ar")]
    [string]$Language = "en",
    [ValidateSet("system", "light", "dark")]
    [string]$Theme = "system",
    [string]$OutputPath = "",
    [string]$BuildCommit = ""
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

function Read-MetricsSafe {
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
            # The app flushes complete JSON lines, but a live read can still
            # race the final write. Ignore only the incomplete live line.
        }
    }
    return $items
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

function Get-Percentile {
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

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem
$gpuControllers = @(Get-CimInstance Win32_VideoController)
$activeGpu = $gpuControllers | Where-Object { $null -ne $_.CurrentHorizontalResolution } | Select-Object -First 1
if ($null -eq $activeGpu) {
    $activeGpu = $gpuControllers | Select-Object -First 1
}
$logicalProcessors = [Math]::Max(1, [int]$computer.NumberOfLogicalProcessors)

$powerScheme = "unknown"
try {
    $powerScheme = ((& powercfg /getactivescheme 2>$null) | Out-String).Trim()
} catch {
    $powerScheme = "unknown"
}

$physicalDisks = @()
try {
    $physicalDisks = @(
        Get-PhysicalDisk | ForEach-Object {
            [pscustomobject]@{
                friendly_name = $_.FriendlyName
                media_type = [string]$_.MediaType
                bus_type = [string]$_.BusType
                size_gib = [math]::Round($_.Size / 1GB, 1)
            }
        }
    )
} catch {
    $physicalDisks = @()
}

$gitSha = $BuildCommit.Trim()
if ([string]::IsNullOrWhiteSpace($gitSha)) {
    try {
        $gitSha = (git -C $repoRoot rev-parse HEAD 2>$null).Trim()
    } catch {
        $gitSha = "unknown"
    }
}
if ([string]::IsNullOrWhiteSpace($gitSha)) {
    $gitSha = "unknown"
}

$runs = @()

for ($i = 1; $i -le $Iterations; $i++) {
    $metricsPath = Join-Path $env:TEMP "atlas-n1-metrics-$PID-$i.jsonl"
    Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath

    $arguments = @(
        "--benchmark-shell",
        "--quit-after-ms", "6500",
        "--metrics-file", "`"$metricsPath`"",
        "--language", $Language,
        "--theme", $Theme
    )

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process -FilePath $AppPath -ArgumentList $arguments -PassThru

    # Wait for Atlas's own first-frame metric instead of WaitForInputIdle().
    # Qt Quick does not provide a useful/consistent input-idle signal here and
    # the old call could block until after auto-shutdown, corrupting memory data.
    $firstFrameObserved = $false
    $firstFrameDeadline = [DateTime]::UtcNow.AddSeconds(8)
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $firstFrameDeadline) {
        $liveMetrics = @(Read-MetricsSafe -Path $metricsPath)
        if ($null -ne (Get-MetricValue -Metrics $liveMetrics -Name "startup.first_frame_ms")) {
            $firstFrameObserved = $true
            break
        }
        Start-Sleep -Milliseconds 50
        $process.Refresh()
    }

    $workingSetMiB = $null
    $privateMiB = $null
    $normalizedCpuPercent = $null
    $processSampleValid = $false

    if ($firstFrameObserved -and -not $process.HasExited) {
        Start-Sleep -Milliseconds 350
        $process.Refresh()

        if (-not $process.HasExited) {
            $workingSetMiB = [math]::Round($process.WorkingSet64 / 1MB, 3)
            $privateMiB = [math]::Round($process.PrivateMemorySize64 / 1MB, 3)
            $cpuStartMs = $process.TotalProcessorTime.TotalMilliseconds

            Start-Sleep -Milliseconds 1000
            $process.Refresh()
            if (-not $process.HasExited) {
                $cpuDeltaMs = $process.TotalProcessorTime.TotalMilliseconds - $cpuStartMs
                $normalizedCpuPercent = [math]::Round((($cpuDeltaMs / 1000.0) / $logicalProcessors) * 100.0, 3)
                $processSampleValid = $true
            }
        }
    }

    $process.WaitForExit()
    $stopwatch.Stop()

    $metricObjects = @(Read-MetricsSafe -Path $metricsPath)

    $runs += [pscustomobject]@{
        iteration = $i
        cache_class = if ($i -eq 1) { "cold_candidate" } else { "warm_candidate" }
        process_runtime_ms = [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 3)
        first_frame_observed = $firstFrameObserved
        process_sample_valid = $processSampleValid
        working_set_mib = $workingSetMiB
        private_memory_mib = $privateMiB
        idle_cpu_percent_one_second = $normalizedCpuPercent
        qgui_application_ready_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.qgui_application_ready_ms"
        arguments_ready_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.arguments_ready_ms"
        metrics_ready_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.metrics_ready_ms"
        qml_engine_ready_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.qml_engine_ready_ms"
        before_qml_load_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.before_qml_load_ms"
        qml_loaded_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.qml_loaded_ms"
        first_frame_ms = Get-MetricValue -Metrics $metricObjects -Name "startup.first_frame_ms"
        device_pixel_ratio = Get-MetricValue -Metrics $metricObjects -Name "display.device_pixel_ratio"
        logical_dpi = Get-MetricValue -Metrics $metricObjects -Name "display.logical_dpi"
        physical_dpi = Get-MetricValue -Metrics $metricObjects -Name "display.physical_dpi"
        measured_refresh_hz = Get-MetricValue -Metrics $metricObjects -Name "display.refresh_hz"
        resize_frame_samples = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_samples"
        resize_frame_p50_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p50_ms"
        resize_frame_p95_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p95_ms"
        resize_frame_p99_ms = Get-MetricValue -Metrics $metricObjects -Name "resize.frame_p99_ms"
        shutdown_ms = Get-MetricValue -Metrics $metricObjects -Name "lifecycle.shutdown_ms"
    }

    Remove-Item -Force -ErrorAction SilentlyContinue $metricsPath
}

$warmRuns = @($runs | Where-Object { $_.cache_class -eq "warm_candidate" })
$validWarmProcessRuns = @($warmRuns | Where-Object { $_.process_sample_valid })

$report = [pscustomobject]@{
    schema = "atlas.n1.windows-shell-baseline.v2"
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    git_sha = $gitSha
    app_path = $AppPath
    language = $Language
    theme = $Theme
    note = "The first launch is a cold-cache candidate, not a guaranteed laboratory cold-cache run. Compare like-for-like runs on the same machine. Process memory/CPU summaries include only samples captured after Atlas emitted its own first-frame metric while the process was still alive."
    machine = [pscustomobject]@{
        os_caption = $os.Caption
        os_version = $os.Version
        os_build = $os.BuildNumber
        cpu = $cpu.Name
        logical_processors = $logicalProcessors
        ram_gib = [math]::Round($computer.TotalPhysicalMemory / 1GB, 2)
        gpu = if ($null -ne $activeGpu) { $activeGpu.Name } else { "unknown" }
        gpu_all = @($gpuControllers | ForEach-Object { $_.Name })
        display_resolution = if ($null -ne $activeGpu -and $null -ne $activeGpu.CurrentHorizontalResolution) { "$($activeGpu.CurrentHorizontalResolution)x$($activeGpu.CurrentVerticalResolution)" } else { "unknown" }
        refresh_hz = if ($null -ne $activeGpu) { $activeGpu.CurrentRefreshRate } else { $null }
        power_scheme = $powerScheme
        physical_disks = $physicalDisks
    }
    measurement_quality = [pscustomobject]@{
        total_runs = $runs.Count
        warm_runs = $warmRuns.Count
        valid_warm_process_samples = $validWarmProcessRuns.Count
        first_frame_observed_runs = @($runs | Where-Object { $_.first_frame_observed }).Count
    }
    summary = [pscustomobject]@{
        cold_first_frame_ms = if ($runs.Count -gt 0) { $runs[0].first_frame_ms } else { $null }
        warm_first_frame_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.first_frame_ms })
        warm_first_frame_ms_p50 = Get-Percentile ($warmRuns | ForEach-Object { $_.first_frame_ms }) 0.50
        warm_first_frame_ms_p95 = Get-Percentile ($warmRuns | ForEach-Object { $_.first_frame_ms }) 0.95
        warm_qml_loaded_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.qml_loaded_ms })
        warm_qml_loaded_ms_p95 = Get-Percentile ($warmRuns | ForEach-Object { $_.qml_loaded_ms }) 0.95
        warm_working_set_mib_average = Get-Average ($validWarmProcessRuns | ForEach-Object { $_.working_set_mib })
        warm_private_memory_mib_average = Get-Average ($validWarmProcessRuns | ForEach-Object { $_.private_memory_mib })
        warm_idle_cpu_percent_average = Get-Average ($validWarmProcessRuns | ForEach-Object { $_.idle_cpu_percent_one_second })
        warm_resize_p95_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.resize_frame_p95_ms })
        warm_resize_p99_ms_average = Get-Average ($warmRuns | ForEach-Object { $_.resize_frame_p99_ms })
        device_pixel_ratio = if ($runs.Count -gt 0) { $runs[0].device_pixel_ratio } else { $null }
        logical_dpi = if ($runs.Count -gt 0) { $runs[0].logical_dpi } else { $null }
        measured_refresh_hz = if ($runs.Count -gt 0) { $runs[0].measured_refresh_hz } else { $null }
    }
    runs = $runs
}

$report | ConvertTo-Json -Depth 10 | Set-Content -Encoding UTF8 $OutputPath

Write-Host "Atlas N1 shell baseline written to: $OutputPath"
$report.measurement_quality | Format-List
$report.summary | Format-List
