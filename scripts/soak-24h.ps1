<#
.SYNOPSIS
    Moxiang-Reborn 24h stability harness (M6-B local-actionable gate).

.DESCRIPTION
    Drives N synthetic clients against the modern Login/Agent/Map three-process
    chain for a configurable duration. Each client runs mxh_client_e2e.exe
    end-to-end (login -> character create -> enter map -> exit) with a unique
    character name per cycle. While clients are cycling, the harness samples
    every server process for memory / CPU / handle counts at a fixed interval.
    Fills the ROADMAP section 3 M6-B TODO and the section 5.E stability gate.

.PARAMETER DurationHours
    Total wall-clock time to run. Default 24. Use 0.0833 (~5 min) for dry-run.
.PARAMETER Concurrency
    Number of concurrent client slots. Default 4.
.PARAMETER Backend
    Database backend handed to start_modern.ps1. sqlite (default) or mssql_odbc.
.PARAMETER BuildDir
    CMake build directory. Default <repo>/modern/build.
.PARAMETER ReportDir
    Where to drop summary.json + samples.csv. Default <BuildDir>/runtime/soak-<runId>.
.PARAMETER SampleIntervalSeconds
    Server memory/CPU sample cadence. Default 5.
.PARAMETER CycleTimeoutSeconds
    Max wall time per client cycle before the harness kills the slot. Default 60.
.PARAMETER MaxClientFailures
    Failure budget: if total cycle failures exceed this many, the run is
    declared unstable and stops early. Default 20.
.PARAMETER SkipServerStart
    Assume start_modern.ps1 has already been started.
.PARAMETER PassThruExtraArgs
    Additional arguments appended to every mxh_client_e2e invocation.

.NOTES
    Exit codes:
      0  PASS, 1 bad args, 2 preflight, 3 server start, 4 client harness,
      5 stability failure, 130 interrupted
#>

[CmdletBinding()]
param(
    [double]$DurationHours = 24,
    [int]$Concurrency = 4,
    [ValidateSet('sqlite','mssql_odbc')]
    [string]$Backend = 'sqlite',
    [string]$BuildDir = '',
    [string]$ReportDir = '',
    [string]$DbRoot = '',
    [int]$SampleIntervalSeconds = 5,
    [int]$CycleTimeoutSeconds = 60,
    [int]$MaxClientFailures = 20,
    [switch]$SkipServerStart,
    [switch]$DryRun,
    [string]$PassThruExtraArgs = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$startServerScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
$clientExe = Join-Path $buildRoot 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
$runId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
if ([string]::IsNullOrWhiteSpace($ReportDir)) {
    $ReportDir = Join-Path $buildRoot ('runtime\soak-' + $runId)
}
$clientLogDir = Join-Path $ReportDir 'clients'
$serverLogDir = Join-Path $ReportDir 'server'
$crashDir = Join-Path $ReportDir 'crash'
$samplesCsv = Join-Path $ReportDir 'samples.csv'
$summaryJson = Join-Path $ReportDir 'summary.json'

function Write-Summary($state) {
    $json = $state | ConvertTo-Json -Depth 6
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllBytes($summaryJson, $utf8.GetBytes($json))
}

function Sample-Server($samples) {
    $row = [ordered]@{ ts = (Get-Date).ToString('o') }
    foreach ($name in @('mxh_login_server','mxh_agent_server_CHINA','mxh_map_server_CHINA')) {
        $procs = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
        $p = if ($procs.Count -gt 0) { $procs[0] } else { $null }
        if ($p) {
            $row[$name + '.rss_mb'] = [math]::Round($p.WorkingSet64 / 1MB, 1)
            $row[$name + '.cpu_s'] = [math]::Round($p.TotalProcessorTime.TotalSeconds, 1)
            $row[$name + '.handles'] = $p.HandleCount
        } else {
            $row[$name + '.rss_mb'] = 0
            $row[$name + '.alive'] = 0
        }
    }
    $samples.Add([pscustomobject]$row) | Out-Null
    return $row
}

function Stop-All($startedServer) {
    foreach ($k in @($script:clients.Keys)) {
        $slot = $script:clients[$k]
        if ($slot -and $slot.Proc -and -not $slot.Proc.HasExited) {
            try { Stop-Process -Id $slot.Proc.Id -Force -ErrorAction SilentlyContinue } catch {}
        }
    }
    # always kill any lingering server processes (handles orphan from prior runs)
    foreach ($n in $script:serverNames) { Get-Process -Name $n -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
    if ($startedServer -and -not $SkipServerStart) {        try { & $startServerScript -Mode stop 2>$null } catch {}
    }
}

# ---- preflight ----
if ($DurationHours -le 0) { Write-Error '-DurationHours must be > 0'; exit 1 }
if ($Concurrency -lt 1) { Write-Error '-Concurrency must be >= 1'; exit 1 }
if (-not (Test-Path -LiteralPath $clientExe)) { Write-Error ('Missing client exe: ' + $clientExe); exit 2 }
if (-not (Test-Path -LiteralPath $startServerScript)) { Write-Error ('Missing start script: ' + $startServerScript); exit 2 }
New-Item -ItemType Directory -Force -Path $ReportDir,$clientLogDir,$serverLogDir,$crashDir | Out-Null

if ($DryRun) {
    Write-Host ('DRY-RUN duration=' + $DurationHours + 'h concurrency=' + $Concurrency + ' backend=' + $Backend + ' build=' + $buildRoot) -ForegroundColor Cyan
    Write-Host ('  client: ' + $clientExe)
    Write-Host ('  report: ' + $ReportDir)
    exit 0
}

# ---- start servers ----
$startedServer = $false
if (-not $SkipServerStart) {
    $startArgs = @{ Mode = 'start'; Backend = $Backend; Locale = 'CHINA'; DataDir = (Join-Path $ReportDir 'data') }
    if ($Backend -eq 'mssql_odbc' -and -not [string]::IsNullOrWhiteSpace($DbRoot)) {
        $startArgs['DbRoot'] = $DbRoot
    }
    # start_modern.ps1 throws on failure; we trust the return
    & $startServerScript @startArgs *> $null
    $startedServer = $true
}

# mirror server logs
$srcServerLogs = Join-Path $repoRoot 'deploy\runtime\modern\logs'
if (Test-Path -LiteralPath $srcServerLogs) {
    Copy-Item -LiteralPath (Join-Path $srcServerLogs '*') -Destination $serverLogDir -Force -ErrorAction SilentlyContinue
}

# ---- state ----
$script:clients = @{}
$slotSeq = 0
$cycleTotal = 0
$cycleOk = 0
$cycleFail = 0
$serverCrashObserved = $false
$samples = New-Object 'System.Collections.Generic.List[object]'
$startTime = Get-Date
$endTime = $startTime.AddHours($DurationHours)
$interrupt = $false
$interruptReason = ''
$initialRss = @{}
$script:serverNames = @('mxh_login_server','mxh_agent_server_CHINA','mxh_map_server_CHINA')

$handler = [ConsoleCancelEventHandler]{
    param($sender, $e)
    $script:interrupt = $true
    $script:interruptReason = ('cancel=' + $e.Cancel + ' / key=' + $e.SpecialKey)
    $e.Cancel = $true
}
[Console]::add_CancelKeyPress($handler)

Write-Host ('SOAK start duration=' + $DurationHours + 'h concurrency=' + $Concurrency + ' backend=' + $Backend + ' build=' + $buildRoot) -ForegroundColor Cyan
Write-Host ('  report: ' + $ReportDir)

# ---- main loop ----
$nextSample = $startTime
try {
    while (($endTime -gt (Get-Date)) -and -not $interrupt -and -not $serverCrashObserved -and ($cycleFail -lt $MaxClientFailures)) {
        # refill slots
        while ($script:clients.Count -lt $Concurrency -and -not $interrupt) {
            $slotSeq += 1
            $charName = 'soak' + $runId + ('{0:D5}' -f $slotSeq)
            $logPath = Join-Path $clientLogDir ('client-' + ('{0:D4}' -f $slotSeq) + '.log')
            $clientArgs = @()
            if (-not [string]::IsNullOrWhiteSpace($PassThruExtraArgs)) {
                $clientArgs += @($PassThruExtraArgs -split ' ')
            }
            try {
                $splat = @{}
                $splat['FilePath'] = $clientExe
                $splat['WorkingDirectory'] = Split-Path -Parent $clientExe
                $splat['RedirectStandardOutput'] = $logPath
                $splat['RedirectStandardError'] = ($logPath -replace '.log', '.err.log')
                $splat['PassThru'] = $true
                $splat['WindowStyle'] = 'Hidden'
                if ($clientArgs.Count -gt 0) { $splat['ArgumentList'] = $clientArgs }
                $proc = Start-Process @splat
            } catch {
                Write-Error ('Failed to start client: ' + $_.Exception.Message); exit 4
            }
            $script:clients[$slotSeq] = [PSCustomObject]@{ Proc = $proc; Started = (Get-Date) }
        }
        # poll clients
        $finished = @()
        foreach ($k in @($script:clients.Keys)) {
            $slot = $script:clients[$k]
            $proc = $slot.Proc
            if ($proc.HasExited) {
                $finished += $k
                $cycleTotal += 1
                $exit = $proc.ExitCode
                $logBase = Join-Path $clientLogDir ('client-' + ('{0:D4}' -f $k))
                $e2eLogPath = $logBase + '.err.log'
                $e2eOk = ($null -ne $exit -and $exit -eq 0)
                if (-not $e2eOk -and (Test-Path -LiteralPath $e2eLogPath)) {
                    $e2eContent = Get-Content -LiteralPath $e2eLogPath -Raw -ErrorAction SilentlyContinue
                    if ($e2eContent -match 'all 5 protocol steps passed') { $e2eOk = $true }
                }
                if ($e2eOk) { $cycleOk += 1 }
                else {
                    $cycleFail += 1
                    $clientLog = Join-Path $clientLogDir ('client-' + ('{0:D4}' -f $k) + '.log')
                    if (Test-Path -LiteralPath $clientLog) {
                        Copy-Item -LiteralPath $clientLog -Destination (Join-Path $crashDir ('client-' + ('{0:D4}' -f $k) + '.log')) -Force
                    }
                }
                continue
            }
            $elapsed = ((Get-Date) - $slot.Started).TotalSeconds
            if ($elapsed -gt $CycleTimeoutSeconds) {
                try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch {}
                $finished += $k
                $cycleTotal += 1
                $cycleFail += 1
            }
        }
        foreach ($k in $finished) { $script:clients.Remove($k) | Out-Null }
        # server liveness + leak heuristic
        $dead = @()
        foreach ($name in @('mxh_login_server','mxh_agent_server_CHINA','mxh_map_server_CHINA')) {
            $procs = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
            $p = if ($procs.Count -gt 0) { $procs[0] } else { $null }
            if (-not $p) { $dead += $name; continue }
            $rss = [math]::Round($p.WorkingSet64 / 1MB, 1)
            if (-not $initialRss.ContainsKey($name)) { $initialRss[$name] = $rss }
            $peak = $rss
            foreach ($s in $samples) {
                $v = $s.($name + '.rss_mb')
                if ($null -ne $v -and [double]$v -gt $peak) { $peak = [double]$v }
            }
            if ($peak -gt 4 * [double]$initialRss[$name] -and [double]$initialRss[$name] -gt 50) {
                $serverCrashObserved = $true
                'Server ' + $name + ' leaked: initial=' + $initialRss[$name] + ' MB peak=' + $peak + ' MB at ' + (Get-Date -Format o) |
                    Set-Content -LiteralPath (Join-Path $crashDir ('server-' + $name + '.rss-leak.txt')) -Encoding utf8
            }
        }
        if ($dead.Count -gt 0) {
            $serverCrashObserved = $true
            ('Servers exited unexpectedly: ' + ($dead -join ',') + ' at ' + (Get-Date -Format o)) |
                Set-Content -LiteralPath (Join-Path $crashDir 'server-dead.txt') -Encoding utf8
        }

        # sample
        if ((Get-Date) -ge $nextSample) {
            $row = Sample-Server $samples
            $line = ($row.Keys -join ',') + [Environment]::NewLine + (($row.Values | ForEach-Object { '' + $_ }) -join ',')
            Add-Content -LiteralPath $samplesCsv -Value ([string[]]$line) -Encoding utf8
            $nextSample = (Get-Date).AddSeconds($SampleIntervalSeconds)
        }

        Start-Sleep -Milliseconds 200
    }
}
finally {
    Stop-All $startedServer
}

# ---- summary ----
$endStamp = Get-Date
$verdict = 'PASS'
$exitCode = 0
if ($interrupt) { $verdict = 'INTERRUPTED'; $exitCode = 130 }
if ($serverCrashObserved) { $verdict = 'FAIL_CRASH_OR_LEAK'; $exitCode = 5 }
elseif ($cycleFail -ge $MaxClientFailures) { $verdict = 'FAIL_ERROR_RATE'; $exitCode = 5 }
elseif ($cycleTotal -eq 0) { $verdict = 'FAIL_NO_CYCLES'; $exitCode = 4 }

$state = [ordered]@{
    verdict = $verdict
    exit_code = $exitCode
    run_id = $runId
    started_at = $startTime.ToString('o')
    ended_at = $endStamp.ToString('o')
    duration_hours_planned = $DurationHours
    duration_hours_actual = [math]::Round(($endStamp - $startTime).TotalHours, 4)
    backend = $Backend
    concurrency = $Concurrency
    build_dir = $buildRoot
    cycle_total = $cycleTotal
    cycle_ok = $cycleOk
    cycle_fail = $cycleFail
    cycle_success_rate = if ($cycleTotal -gt 0) { [math]::Round($cycleOk / $cycleTotal, 4) } else { 0 }
    server_crash_observed = $serverCrashObserved
    interrupt = $interrupt
    interrupt_reason = $interruptReason
    initial_rss_mb = $initialRss
    sample_count = $samples.Count
    report_dir = $ReportDir
}
Write-Summary $state
$fg = if ($exitCode -eq 0) { 'Green' } else { 'Red' }
Write-Host ('SOAK verdict=' + $verdict + ' cycles=' + $cycleTotal + ' ok=' + $cycleOk + ' fail=' + $cycleFail + ' report=' + $ReportDir) -ForegroundColor $fg
exit $exitCode
