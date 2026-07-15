# ============================================================================
# 墨香Reborn - 完整部署指南
# ============================================================================
# 本脚本将自动完成所有部署步骤
# 使用方法：.\deploy_all.ps1
# ============================================================================

param(
    [switch]$SkipDatabase,
    [switch]$SkipServer,
    [switch]$SkipClient,
    [string]$ServerIP = "127.0.0.1"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$baseDir = Split-Path -Parent $scriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  墨香Reborn - 完整部署脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "部署目录: $baseDir" -ForegroundColor Gray
Write-Host "服务器IP: $ServerIP" -ForegroundColor Gray
Write-Host ""

# 检查管理员权限
function Test-Admin {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# ============================================================================
# 阶段一：数据库安装
# ============================================================================
if (-not $SkipDatabase) {
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "  阶段一：数据库安装" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host ""
    
    if (Test-Admin) {
        $dbScript = Join-Path $baseDir "database\install_database.ps1"
        if (Test-Path $dbScript) {
            Write-Host "执行数据库安装脚本..." -ForegroundColor Gray
            & $dbScript
        } else {
            Write-Host "[错误] 未找到数据库安装脚本" -ForegroundColor Red
        }
    } else {
        Write-Host "[警告] 需要管理员权限来安装数据库" -ForegroundColor Yellow
        Write-Host "请以管理员身份重新运行此脚本" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "或者手动安装 SQL Server：" -ForegroundColor White
        Write-Host "  1. 下载 SQL Server Express: https://www.microsoft.com/zh-cn/sql-server/sql-server-downloads" -ForegroundColor White
        Write-Host "  2. 安装后运行: .\database\install_database.ps1" -ForegroundColor White
        Write-Host ""
    }
} else {
    Write-Host "跳过数据库安装" -ForegroundColor Yellow
}

# ============================================================================
# 阶段二：服务端部署
# ============================================================================
if (-not $SkipServer) {
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "  阶段二：服务端部署" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host ""
    
    $serverScript = Join-Path $baseDir "scripts\deploy_server.ps1"
    if (Test-Path $serverScript) {
        Write-Host "执行服务端部署脚本..." -ForegroundColor Gray
        & $serverScript
    } else {
        Write-Host "[错误] 未找到服务端部署脚本" -ForegroundColor Red
    }
} else {
    Write-Host "跳过服务端部署" -ForegroundColor Yellow
}

# ============================================================================
# 阶段三：客户端部署
# ============================================================================
if (-not $SkipClient) {
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "  阶段三：客户端部署" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host ""
    
    $clientScript = Join-Path $baseDir "scripts\deploy_client.ps1"
    if (Test-Path $clientScript) {
        Write-Host "执行客户端部署脚本..." -ForegroundColor Gray
        & $clientScript -ServerIP $ServerIP
    } else {
        Write-Host "[错误] 未找到客户端部署脚本" -ForegroundColor Red
    }
} else {
    Write-Host "跳过客户端部署" -ForegroundColor Yellow
}

# ============================================================================
# 完成
# ============================================================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  部署完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "目录结构：" -ForegroundColor Yellow
Write-Host "  deploy/" -ForegroundColor White
Write-Host "    database\    - 数据库安装脚本" -ForegroundColor Gray
Write-Host "    server\      - 服务端文件" -ForegroundColor Gray
Write-Host "    client\      - 客户端文件" -ForegroundColor Gray
Write-Host "    scripts\     - 部署脚本" -ForegroundColor Gray
Write-Host ""
Write-Host "启动步骤：" -ForegroundColor Yellow
Write-Host "  1. 安装 SQL Server (如果尚未安装)" -ForegroundColor White
Write-Host "  2. 运行 .\database\install_database.ps1" -ForegroundColor White
Write-Host "  3. 运行 .\scripts\start_all.ps1" -ForegroundColor White
Write-Host "  4. 运行 ..\client\启动游戏.bat" -ForegroundColor White
Write-Host ""
Write-Host "服务端口：" -ForegroundColor Yellow
Write-Host "  登录服务器: 9000" -ForegroundColor White
Write-Host "  代理服务器: 10000" -ForegroundColor White
Write-Host "  地图服务器: 10001" -ForegroundColor White
Write-Host "  监控服务器: 10002" -ForegroundColor White
Write-Host ""
Write-Host "停止服务：" -ForegroundColor Yellow
Write-Host "  .\scripts\start_all.ps1 -Stop" -ForegroundColor White
