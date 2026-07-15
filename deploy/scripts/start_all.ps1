# ============================================================================
# 墨香Reborn - 一键启动所有服务
# ============================================================================
# 使用方法：.\start_all.ps1
# ============================================================================

param(
    [string]$ServerDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\server",
    [switch]$Stop
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  墨香Reborn - 服务管理器" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 停止所有服务
if ($Stop) {
    Write-Host "正在停止所有服务..." -ForegroundColor Yellow
    
    $processes = @("DistributeServer", "AgentServer", "MapServer", "RMToolServer", "MonitoringServer")
    foreach ($proc in $processes) {
        $running = Get-Process -Name $proc -ErrorAction SilentlyContinue
        if ($running) {
            Stop-Process -Name $proc -Force
            Write-Host "  已停止: $proc" -ForegroundColor Gray
        }
    }
    
    Write-Host "所有服务已停止" -ForegroundColor Green
    exit 0
}

# 检查数据库连接
Write-Host "[1/5] 检查数据库连接..." -ForegroundColor Yellow
try {
    $sqlcmdPath = Get-ChildItem -Path "C:\Program Files\Microsoft SQL Server" -Recurse -Filter "sqlcmd.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($sqlcmdPath) {
        $result = & $sqlcmdPath.FullName -S "." -Q "SELECT @@VERSION" -E -h1 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  数据库连接正常" -ForegroundColor Green
        } else {
            Write-Host "  [警告] 数据库连接失败" -ForegroundColor Yellow
            Write-Host "  请确保 SQL Server 服务正在运行" -ForegroundColor Yellow
        }
    }
} catch {
    Write-Host "  [警告] 无法检查数据库连接" -ForegroundColor Yellow
}

# 启动登录服务器 (Distribute)
Write-Host "[2/5] 启动登录服务器 (Distribute)..." -ForegroundColor Yellow
$distributeExe = Join-Path $ServerDir "Distribute\DistributeServer.exe"
if (Test-Path $distributeExe) {
    $distributeDir = Split-Path $distributeExe
    Start-Process -FilePath $distributeExe -WorkingDirectory $distributeDir -WindowStyle Minimized
    Start-Sleep -Seconds 2
    Write-Host "  登录服务器已启动" -ForegroundColor Green
} else {
    Write-Host "  [错误] 未找到 DistributeServer.exe" -ForegroundColor Red
}

# 启动代理服务器 (Agent)
Write-Host "[3/5] 启动代理服务器 (Agent)..." -ForegroundColor Yellow
$agentExe = Join-Path $ServerDir "Agent\AgentServer.exe"
if (Test-Path $agentExe) {
    $agentDir = Split-Path $agentExe
    Start-Process -FilePath $agentExe -WorkingDirectory $agentDir -WindowStyle Minimized
    Start-Sleep -Seconds 2
    Write-Host "  代理服务器已启动" -ForegroundColor Green
} else {
    Write-Host "  [错误] 未找到 AgentServer.exe" -ForegroundColor Red
}

# 启动地图服务器 (Map)
Write-Host "[4/5] 启动地图服务器 (Map)..." -ForegroundColor Yellow
$mapExe = Join-Path $ServerDir "Map\MapServer.exe"
if (Test-Path $mapExe) {
    $mapDir = Split-Path $mapExe
    Start-Process -FilePath $mapExe -WorkingDirectory $mapDir -WindowStyle Minimized
    Start-Sleep -Seconds 3
    Write-Host "  地图服务器已启动" -ForegroundColor Green
} else {
    Write-Host "  [错误] 未找到 MapServer.exe" -ForegroundColor Red
}

# 启动监控服务器 (Monitoring)
Write-Host "[5/5] 启动监控服务器 (Monitoring)..." -ForegroundColor Yellow
$monitorExe = Join-Path $ServerDir "Monitoring\MonitoringServer.exe"
if (Test-Path $monitorExe) {
    $monitorDir = Split-Path $monitorExe
    Start-Process -FilePath $monitorExe -WorkingDirectory $monitorDir -WindowStyle Minimized
    Start-Sleep -Seconds 1
    Write-Host "  监控服务器已启动" -ForegroundColor Green
} else {
    Write-Host "  [警告] 未找到 MonitoringServer.exe" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  所有服务已启动！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "服务状态：" -ForegroundColor Yellow
Write-Host "  登录服务器 (Distribute) - 端口 9000" -ForegroundColor White
Write-Host "  代理服务器 (Agent)      - 端口 10000" -ForegroundColor White
Write-Host "  地图服务器 (Map)        - 端口 10001" -ForegroundColor White
Write-Host "  监控服务器 (Monitoring)  - 端口 10002" -ForegroundColor White
Write-Host ""
Write-Host "启动客户端：" -ForegroundColor Yellow
Write-Host "  运行 ..\client\启动游戏.bat" -ForegroundColor White
Write-Host ""
Write-Host "停止服务：" -ForegroundColor Yellow
Write-Host "  .\start_all.ps1 -Stop" -ForegroundColor White
