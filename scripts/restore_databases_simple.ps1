# 还原墨香游戏数据库备份到SQL Server
$ErrorActionPreference = "Stop"

$backupDir = "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\数据库"
$serverInstance = "(local)"

Write-Host "=== 墨香游戏数据库还原脚本 ==="
Write-Host ""

# 检查备份文件
$files = @("MHCMEMBER.bak", "MHGAME.bak", "MHLOG.bak")
foreach ($f in $files) {
    $path = Join-Path $backupDir $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length / 1MB
        Write-Host "$f - $([math]::Round($size, 2)) MB"
    } else {
        Write-Host "错误: 找不到 $path"
        exit 1
    }
}

Write-Host ""
Write-Host "开始还原数据库..."

# 还原MHCMEMBER
Write-Host "正在还原 MHCMEMBER..."
$backupFile = Join-Path $backupDir "MHCMEMBER.bak"
$sql = "RESTORE DATABASE [MHCMEMBER] FROM DISK = N'$backupFile' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 10"
& sqlcmd -S $serverInstance -Q $sql
if ($LASTEXITCODE -eq 0) {
    Write-Host "MHCMEMBER 还原成功!"
} else {
    Write-Host "MHCMEMBER 还原失败!"
}

# 还原MHGAME
Write-Host "正在还原 MHGAME..."
$backupFile = Join-Path $backupDir "MHGAME.bak"
$sql = "RESTORE DATABASE [MHGAME] FROM DISK = N'$backupFile' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 10"
& sqlcmd -S $serverInstance -Q $sql
if ($LASTEXITCODE -eq 0) {
    Write-Host "MHGAME 还原成功!"
} else {
    Write-Host "MHGAME 还原失败!"
}

# 还原MHLOG
Write-Host "正在还原 MHLOG..."
$backupFile = Join-Path $backupDir "MHLOG.bak"
$sql = "RESTORE DATABASE [MHLOG] FROM DISK = N'$backupFile' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 10"
& sqlcmd -S $serverInstance -Q $sql
if ($LASTEXITCODE -eq 0) {
    Write-Host "MHLOG 还原成功!"
} else {
    Write-Host "MHLOG 还原失败!"
}

Write-Host ""
Write-Host "=== 数据库还原完成 ==="

# 验证数据库
Write-Host "验证数据库状态:"
& sqlcmd -S $serverInstance -Q "SELECT name, state_desc FROM sys.databases WHERE name IN ('MHCMEMBER', 'MHGAME', 'MHLOG')"
