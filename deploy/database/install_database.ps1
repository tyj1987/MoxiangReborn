# ============================================================================
# 墨香Reborn - SQL Server 安装和数据库恢复脚本
# ============================================================================
# 使用方法：以管理员身份运行 PowerShell，执行此脚本
# .\install_database.ps1
# ============================================================================

param(
    [string]$SqlServerInstance = "MSSQLSERVER",
    [string]$SqlServerVersion = "2022",
    [string]$BackupDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\数据库",
    [string]$DataDir = "C:\Program Files\Microsoft SQL Server\MSSQL$SqlServerVersion.$SqlServerInstance\MSSQL\Data",
    [string]$LogDir = "C:\Program Files\Microsoft SQL Server\MSSQL$SqlServerVersion.$SqlServerInstance\MSSQL\LOG"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  墨香Reborn - 数据库安装脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查管理员权限
function Test-Admin {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    Write-Host "[错误] 请以管理员身份运行此脚本！" -ForegroundColor Red
    exit 1
}

# 检查 SQL Server 是否安装
Write-Host "[1/6] 检查 SQL Server 安装状态..." -ForegroundColor Yellow
$sqlservrPath = "C:\Program Files\Microsoft SQL Server\MSSQL$SqlServerVersion.$SqlServerInstance\MSSQL\Binn\sqlservr.exe"
if (-not (Test-Path $sqlservrPath)) {
    Write-Host "[警告] 未找到 SQL Server $SqlServerVersion 实例" -ForegroundColor Yellow
    Write-Host "请先安装 SQL Server 2022 Express 或更高版本" -ForegroundColor Yellow
    Write-Host "下载地址: https://www.microsoft.com/zh-cn/sql-server/sql-server-downloads" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "安装时请选择："
    Write-Host "  - 基本安装 或 自定义安装"
    Write-Host "  - 实例名称: MSSQLSERVER (默认实例)"
    Write-Host "  - 身份验证模式: 混合模式 (SQL Server 和 Windows)"
    Write-Host "  - SA 密码: [请设置强密码]"
    Write-Host ""
    
    $continue = Read-Host "是否继续安装数据库？(Y/N)"
    if ($continue -ne "Y") {
        exit 0
    }
}

# 检查 sqlcmd 是否可用
Write-Host "[2/6] 检查 sqlcmd 工具..." -ForegroundColor Yellow
$sqlcmdPath = Get-ChildItem -Path "C:\Program Files\Microsoft SQL Server" -Recurse -Filter "sqlcmd.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $sqlcmdPath) {
    Write-Host "[警告] 未找到 sqlcmd 工具" -ForegroundColor Yellow
    Write-Host "请安装 SQL Server Command Line Utilities" -ForegroundColor Yellow
    Write-Host "下载地址: https://learn.microsoft.com/zh-cn/sql/tools/sqlcmd-utility" -ForegroundColor Yellow
    exit 1
}
$sqlcmd = $sqlcmdPath.FullName
Write-Host "  sqlcmd: $sqlcmd" -ForegroundColor Gray

# 测试数据库连接
Write-Host "[3/6] 测试数据库连接..." -ForegroundColor Yellow
$testResult = & $sqlcmd -S ".\$SqlServerInstance" -Q "SELECT @@VERSION" -E -h1 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[错误] 无法连接到 SQL Server 实例" -ForegroundColor Red
    Write-Host "请确保 SQL Server 服务正在运行" -ForegroundColor Red
    Write-Host "运行: net start MSSQL$SqlServerInstance" -ForegroundColor Yellow
    exit 1
}
Write-Host "  连接成功" -ForegroundColor Green

# 创建数据库
Write-Host "[4/6] 创建数据库..." -ForegroundColor Yellow
$databases = @("MHCMEMBER", "MHGAME", "MHLOG")

foreach ($db in $databases) {
    Write-Host "  创建数据库: $db" -ForegroundColor Gray
    $createDbSql = @"
IF NOT EXISTS (SELECT name FROM sys.databases WHERE name = '$db')
BEGIN
    CREATE DATABASE [$db]
    PRINT '数据库 $db 创建成功'
END
ELSE
BEGIN
    PRINT '数据库 $db 已存在'
END
"@
    & $sqlcmd -S ".\$SqlServerInstance" -Q $createDbSql -E 2>&1 | Out-Null
}

# 恢复数据库备份
Write-Host "[5/6] 恢复数据库备份..." -ForegroundColor Yellow
$backupFiles = @{
    "MHCMEMBER" = "MHCMEMBER.bak"
    "MHGAME" = "MHGAME.bak"
    "MHLOG" = "MHLOG.bak"
}

foreach ($db in $backupFiles.Keys) {
    $bakFile = Join-Path $BackupDir $backupFiles[$db]
    if (Test-Path $bakFile) {
        Write-Host "  恢复 $db 从 $bakFile" -ForegroundColor Gray
        $restoreSql = @"
USE master;
ALTER DATABASE [$db] SET SINGLE_USER WITH ROLLBACK IMMEDIATE;
RESTORE DATABASE [$db] FROM DISK = '$bakFile' WITH REPLACE,
    MOVE '$db' TO '$DataDir\$db.mdf',
    MOVE '${db}_log' TO '$LogDir\$db_log.ldf';
ALTER DATABASE [$db] SET MULTI_USER;
PRINT '数据库 $db 恢复成功';
"@
        & $sqlcmd -S ".\$SqlServerInstance" -Q $restoreSql -E 2>&1
    } else {
        Write-Host "  [警告] 未找到备份文件: $bakFile" -ForegroundColor Yellow
    }
}

# 创建 ODBC 数据源
Write-Host "[6/6] 配置 ODBC 数据源..." -ForegroundColor Yellow
$odbcNames = @("MHCMEMBER", "MHGAME", "MHLOG")

foreach ($dsn in $odbcNames) {
    Write-Host "  创建系统 DSN: $dsn" -ForegroundColor Gray
    $odbcRegPath = "HKLM:\SOFTWARE\ODBC\ODBC.INI\$dsn"
    if (-not (Test-Path $odbcRegPath)) {
        New-Item -Path $odbcRegPath -Force | Out-Null
    }
    Set-ItemProperty -Path $odbcRegPath -Name "Driver" -Value "C:\Windows\System32\SQLSRV32.dll"
    Set-ItemProperty -Path $odbcRegPath -Name "Server" -Value ".\$SqlServerInstance"
    Set-ItemProperty -Path $odbcRegPath -Name "Database" -Value $dsn
    
    # 添加到 ODBC 数据源列表
    $odbcIniPath = "HKLM:\SOFTWARE\ODBC\ODBC.INI\ODBC Data Sources"
    if (-not (Test-Path $odbcIniPath)) {
        New-Item -Path $odbcIniPath -Force | Out-Null
    }
    Set-ItemProperty -Path $odbcIniPath -Name $dsn -Value "SQL Server"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  数据库安装完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "已创建数据库: MHCMEMBER, MHGAME, MHLOG" -ForegroundColor Cyan
Write-Host "已配置 ODBC 数据源" -ForegroundColor Cyan
Write-Host ""
Write-Host "下一步：" -ForegroundColor Yellow
Write-Host "  1. 运行 .\deploy_server.ps1 部署服务端" -ForegroundColor White
Write-Host "  2. 运行 .\deploy_client.ps1 部署客户端" -ForegroundColor White
Write-Host "  3. 运行 .\start_all.ps1 启动所有服务" -ForegroundColor White
