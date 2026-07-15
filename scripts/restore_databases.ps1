# 还原墨香游戏数据库备份到SQL Server
# 使用方法: .\restore_databases.ps1

$ErrorActionPreference = "Stop"

# 数据库备份文件路径
$backupDir = "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\数据库"
$databases = @(
    @{Name="MHCMEMBER"; BakFile="$backupDir\MHCMEMBER.bak"},
    @{Name="MHGAME"; BakFile="$backupDir\MHGAME.bak"},
    @{Name="MHLOG"; BakFile="$backupDir\MHLOG.bak"}
)

# SQL Server实例名称
$serverInstance = "(local)"

Write-Host "=== 墨香游戏数据库还原脚本 ===" -ForegroundColor Cyan
Write-Host ""

# 检查备份文件是否存在
foreach ($db in $databases) {
    if (-not (Test-Path $db.BakFile)) {
        Write-Host "错误: 找不到备份文件 $($db.BakFile)" -ForegroundColor Red
        exit 1
    }
    $size = (Get-Item $db.BakFile).Length / 1MB
    Write-Host "$($db.Name): $($db.BakFile) ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
}

Write-Host ""
Write-Host "开始还原数据库..." -ForegroundColor Yellow

# 还原每个数据库
foreach ($db in $databases) {
    Write-Host ""
    Write-Host "正在还原 $($db.Name)..." -ForegroundColor Yellow
    
    # 构建还原SQL
    $restoreSql = @"
USE [master];
GO

-- 如果数据库已存在，先断开连接
IF DB_ID('$($db.Name)') IS NOT BEGIN
    ALTER DATABASE [$($db.Name)] SET SINGLE_USER WITH ROLLBACK IMMEDIATE;
    ALTER DATABASE [$($db.Name)] SET MULTI_USER;
END
GO

-- 还原数据库
RESTORE DATABASE [$($db.Name)] 
FROM DISK = N'$($db.BakFile)' 
WITH 
    FILE = 1,
    NOUNLOAD,
    REPLACE,
    STATS = 10;
GO

PRINT '数据库 $($db.Name) 还原完成';
GO
"@
    
    # 执行还原
    try {
        $tempSqlFile = [System.IO.Path]::GetTempFileName() + ".sql"
        $restoreSql | Out-File -FilePath $tempSqlFile -Encoding UTF8
        
        # 使用sqlcmd执行
        $process = Start-Process -FilePath "sqlcmd" -ArgumentList "-S", $serverInstance, "-i", $tempSqlFile, "-o", "$env:TEMP\restore_$($db.Name).log" -Wait -PassThru -NoNewWindow
        
        if ($process.ExitCode -eq 0) {
            Write-Host "$($db.Name) 还原成功!" -ForegroundColor Green
        } else {
            Write-Host "$($db.Name) 还原失败，退出码: $($process.ExitCode)" -ForegroundColor Red
            Write-Host "查看日志: $env:TEMP\restore_$($db.Name).log" -ForegroundColor Yellow
        }
        
        # 清理临时文件
        Remove-Item -Path $tempSqlFile -ErrorAction SilentlyContinue
    }
    catch {
        Write-Host "还原 $($db.Name) 时发生错误: $_" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "=== 数据库还原完成 ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "验证数据库状态:" -ForegroundColor Yellow

# 验证数据库
foreach ($db in $databases) {
    $checkSql = "SELECT name, state_desc FROM sys.databases WHERE name = '$($db.Name)'"
    $tempCheckFile = [System.IO.Path]::GetTempFileName() + ".sql"
    $checkSql | Out-File -FilePath $tempCheckFile -Encoding UTF8
    
    $result = & sqlcmd -S $serverInstance -i $tempCheckFile -h -1 -W 2>&1
    Remove-Item -Path $tempCheckFile -ErrorAction SilentlyContinue
    
    if ($result -match $db.Name) {
        Write-Host "$($db.Name): 在线" -ForegroundColor Green
    } else {
        Write-Host "$($db.Name): 未找到" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "数据库还原脚本执行完成!" -ForegroundColor Cyan
