<#
.SYNOPSIS
    墨香（Moxian）服务端一键启动脚本 - 现代化版本。

.DESCRIPTION
    替代原 SWorking 目录下的 105.lnk / 108.lnk 等快捷方式。
    自动按依赖顺序启动 Distribute → Agent → Map → Monitoring。

.PARAMETER Mode
    start   - 按顺序启动所有服务端进程（默认）
    stop    - 停止所有服务端进程
    status  - 显示当前运行状态
    restart - 重启全部进程

.PARAMETER Serverset
    ServerSet 子目录编号（默认 1）

.PARAMETER Path
    服务端工作目录（默认 墨香【源码】\SWorking）

.EXAMPLE
    .\start-server.ps1 -Mode start
    .\start-server.ps1 -Mode stop
    .\start-server.ps1 -Mode status -Serverset 1
#>

[CmdletBinding()]
param(
    [ValidateSet('start', 'stop', 'status', 'restart')]
    [string]$Mode = 'start',

    [int]$Serverset = 1,

    [string]$Path = "$PSScriptRoot\..\墨香【源码】\SWorking"
)

$ErrorActionPreference = 'Stop'

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

function Write-Section([string]$text) {
    Write-Host ''
    Write-Host ('=' * 60) -ForegroundColor Cyan
    Write-Host " $text" -ForegroundColor Cyan
    Write-Host ('=' * 60) -ForegroundColor Cyan
}

function Test-ServerPath {
    if (-not (Test-Path $Path)) {
        throw "Server path not found: $Path"
    }
    Write-Host "Server path: $Path" -ForegroundColor DarkGray
}

function Get-ServerProcesses {
    # Match by exe name (without extension).
    Get-Process | Where-Object {
        $_.ProcessName -in @('DistributeServer', 'AgentServer', 'MapServer', 'MonitoringServer')
    } | Select-Object Id, ProcessName, StartTime, MainWindowTitle | Format-Table -AutoSize
}

function Stop-Servers {
    Write-Section 'Stopping all server processes'

    $names = @('MapServer', 'AgentServer', 'DistributeServer', 'MonitoringServer')
    foreach ($name in $names) {
        $procs = Get-Process -Name $name -ErrorAction SilentlyContinue
        if ($procs) {
            foreach ($p in $procs) {
                Write-Host "Stopping $name (PID $($p.Id))..." -ForegroundColor Yellow
                Stop-Process -Id $p.Id -Force
            }
        } else {
            Write-Host "$name not running." -ForegroundColor DarkGray
        }
    }

    Start-Sleep -Seconds 2
    Write-Section 'All servers stopped'
}

function Start-OneServer {
    param(
        [string]$ExeName,
        [string]$WorkingDir,
        [int]$WaitSeconds = 5
    )

    $exePath = Join-Path $WorkingDir "$ExeName.exe"
    if (-not (Test-Path $exePath)) {
        Write-Host "  [SKIP] $ExeName not found at $exePath" -ForegroundColor DarkGray
        return
    }

    Write-Host "  Starting $ExeName..." -ForegroundColor Green
    $proc = Start-Process -FilePath $exePath `
                           -WorkingDirectory $WorkingDir `
                           -PassThru `
                           -RedirectStandardOutput "$WorkingDir\Log\$ExeName.out.log" `
                           -RedirectStandardError  "$WorkingDir\Log\$ExeName.err.log"

    Write-Host "    PID = $($proc.Id)" -ForegroundColor DarkGray
    Start-Sleep -Seconds $WaitSeconds
    if ($proc.HasExited) {
        Write-Host "    [WARN] $ExeName exited unexpectedly. Check logs." -ForegroundColor Red
    } else {
        Write-Host "    OK." -ForegroundColor Green
    }
}

function Start-Servers {
    Write-Section "Starting servers (serverset=$Serverset)"

    if (-not (Test-Path $Path)) {
        throw "Path not found: $Path"
    }

    # Ensure log dir
    $logDir = Join-Path $Path 'Log'
    if (-not (Test-Path $logDir)) {
        New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    }

    # 1. Monitoring Server first (others will connect to it).
    Start-OneServer -ExeName 'MonitoringServer' -WorkingDir $Path -WaitSeconds 4

    # 2. Distribute (login).
    Start-OneServer -ExeName 'DistributeServer'  -WorkingDir $Path -WaitSeconds 4

    # 3. Agent (world/proxy).
    Start-OneServer -ExeName 'AgentServer'       -WorkingDir $Path -WaitSeconds 4

    # 4. Map servers (one per map). Auto-detect from ServerSet.
    $serverSetDir = Join-Path $Path "ServerSet\$Serverset"
    if (Test-Path $serverSetDir) {
        Write-Host ''
        Write-Host 'Detected map servers in ServerSet dir:' -ForegroundColor Cyan
        Get-ChildItem $serverSetDir -Recurse -Filter '*.txt' |
            Where-Object { $_.Name -match '^Map.*\.txt$' } |
            ForEach-Object { Write-Host "  $($_.Name)" -ForegroundColor DarkGray }
        Write-Host ''
        Write-Host 'Start map servers manually per your serverInfo.ini.' -ForegroundColor Yellow
        Write-Host 'Example:' -ForegroundColor Yellow
        Write-Host "  cd $Path" -ForegroundColor Yellow
        Write-Host "  .\MapServer.exe" -ForegroundColor Yellow
    } else {
        Write-Host "ServerSet\$Serverset not found; skipping map auto-start." -ForegroundColor Yellow
    }

    Write-Section 'Startup complete'
}

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

try {
    Test-ServerPath

    switch ($Mode) {
        'start'   { Start-Servers }
        'stop'    { Stop-Servers }
        'restart' { Stop-Servers; Start-Sleep -Seconds 3; Start-Servers }
        'status'  {
            Write-Section 'Current server status'
            Get-ServerProcesses
        }
    }
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    exit 1
}