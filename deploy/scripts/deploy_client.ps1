# ============================================================================
# 墨香Reborn - 客户端部署脚本
# ============================================================================
# 使用方法：.\deploy_client.ps1
# ============================================================================

param(
    [string]$SourceDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\cworking",
    [string]$DeployDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\client",
    [string]$ResourceDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码配套资源】\PlayDH",
    [string]$ServerIP = "127.0.0.1"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  墨香Reborn - 客户端部署脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 创建部署目录结构
Write-Host "[1/5] 创建目录结构..." -ForegroundColor Yellow
$dirs = @(
    "$DeployDir\Resource",
    "$DeployDir\Resource\Client",
    "$DeployDir\Resource\Map",
    "$DeployDir\Resource\EffectScript",
    "$DeployDir\Resource\QuestScript",
    "$DeployDir\Resource\SkillArea",
    "$DeployDir\Image",
    "$DeployDir\Sound",
    "$DeployDir\Data",
    "$DeployDir\Ini",
    "$DeployDir\Log"
)

foreach ($dir in $dirs) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}
Write-Host "  目录结构创建完成" -ForegroundColor Green

# 复制客户端可执行文件
Write-Host "[2/5] 复制客户端可执行文件..." -ForegroundColor Yellow
$clientExe = Join-Path $SourceDir "MHClient-Connect.exe"
if (Test-Path $clientExe) {
    Copy-Item -Path $clientExe -Destination "$DeployDir\MoxianReborn.exe" -Force
    Write-Host "  复制: MHClient-Connect.exe -> MoxianReborn.exe" -ForegroundColor Gray
} else {
    Write-Host "  [错误] 未找到客户端可执行文件" -ForegroundColor Red
    exit 1
}

# 复制游戏资源
Write-Host "[3/5] 复制游戏资源..." -ForegroundColor Yellow
$resourceDirs = @(
    @{Source="Resource\Client"; Dest="Resource\Client"},
    @{Source="Resource\Map"; Dest="Resource\Map"},
    @{Source="Resource\EffectScript"; Dest="Resource\EffectScript"},
    @{Source="Resource\QuestScript"; Dest="Resource\QuestScript"},
    @{Source="Resource\SkillArea"; Dest="Resource\SkillArea"},
    @{Source="Image"; Dest="Image"},
    @{Source="Sound"; Dest="Sound"},
    @{Source="Data"; Dest="Data"},
    @{Source="Ini"; Dest="Ini"}
)

foreach ($res in $resourceDirs) {
    $src = Join-Path $ResourceDir $res.Source
    $dst = Join-Path $DeployDir $res.Dest
    if (Test-Path $src) {
        if (-not (Test-Path $dst)) {
            New-Item -ItemType Directory -Path $dst -Force | Out-Null
        }
        Copy-Item -Path "$src\*" -Destination $dst -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  复制: $($res.Source)" -ForegroundColor Gray
    }
}

# 生成客户端配置文件
Write-Host "[4/5] 生成客户端配置文件..." -ForegroundColor Yellow

# MHVerInfo.ver - 版本信息
$verContent = "DHCV20240101"
$verContent | Out-File -FilePath "$DeployDir\MHVerInfo.ver" -Encoding ASCII -NoNewline
Write-Host "  生成: MHVerInfo.ver" -ForegroundColor Gray

# MHVerInfo.bin - 服务器列表（二进制格式，这里用文本模拟）
$serverList = @"
[MoxianReborn]
IP=$ServerIP
Port=9000
"@
$serverList | Out-File -FilePath "$DeployDir\MHVerInfo.bin" -Encoding UTF8
Write-Host "  生成: MHVerInfo.bin" -ForegroundColor Gray

# ServerList.bin - 服务器列表
$serverListBin = @"
[ServerList]
Count=1
Server1_Name=墨香Reborn
Server1_IP=$ServerIP
Server1_Port=9000
Server1_Online=0
Server1_Max=1000
Server1_Status=0
"@
$serverListBin | Out-File -FilePath "$DeployDir\Ini\ServerList.bin" -Encoding UTF8
Write-Host "  生成: Ini\ServerList.bin" -ForegroundColor Gray

# GameDesc.bin - 游戏描述
$gameDesc = @"
[Game]
Name=墨香Reborn
Version=1.0.0
Build=20240101
"@
$gameDesc | Out-File -FilePath "$DeployDir\Ini\GameDesc.bin" -Encoding UTF8
Write-Host "  生成: Ini\GameDesc.bin" -ForegroundColor Gray

# GameOption.opt - 游戏选项
$gameOption = @"
[Option]
MusicVolume=80
SoundVolume=80
Brightness=50
Gamma=50
CameraAngle=0
ShowName=1
ShowHP=1
ShowDamage=1
"@
$gameOption | Out-File -FilePath "$DeployDir\Ini\GameOption.opt" -Encoding UTF8
Write-Host "  生成: Ini\GameOption.opt" -ForegroundColor Gray

# wonuke.ini - 配置
$wonuke = @"
[Network]
ServerIP=$ServerIP
ServerPort=9000
"@
$wonuke | Out-File -FilePath "$DeployDir\Ini\wonuke.ini" -Encoding UTF8
Write-Host "  生成: Ini\wonuke.ini" -ForegroundColor Gray

# 创建启动器
Write-Host "[5/5] 创建启动器..." -ForegroundColor Yellow
$launcher = @"
@echo off
chcp 65001 >nul
title 墨香Reborn - 启动器
echo.
echo ========================================
echo   墨香Reborn - 游戏启动器
echo ========================================
echo.
echo 正在启动游戏...
echo.

cd /d "%~dp0"

:: 检查服务器连接
echo 检查服务器连接...
ping -n 1 $ServerIP >nul 2>&1
if errorlevel 1 (
    echo [警告] 无法连接到服务器 $ServerIP
    echo 请确保服务器已启动
    pause
)

:: 启动游戏
start "" "%~dp0MoxianReborn.exe"

echo 游戏已启动
echo.
pause
"@
$launcher | Out-File -FilePath "$DeployDir\启动游戏.bat" -Encoding OEM
Write-Host "  生成: 启动游戏.bat" -ForegroundColor Gray

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  客户端部署完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "部署目录: $DeployDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "文件结构：" -ForegroundColor Yellow
Write-Host "  MoxianReborn.exe    - 游戏主程序" -ForegroundColor White
Write-Host "  启动游戏.bat        - 游戏启动器" -ForegroundColor White
Write-Host "  MHVerInfo.ver       - 版本信息" -ForegroundColor White
Write-Host "  Resource\           - 游戏资源" -ForegroundColor White
Write-Host "  Image\              - 图片资源" -ForegroundColor White
Write-Host "  Sound\              - 音效资源" -ForegroundColor White
Write-Host "  Data\               - 游戏数据" -ForegroundColor White
Write-Host "  Ini\                - 配置文件" -ForegroundColor White
Write-Host ""
Write-Host "服务器地址: $ServerIP:9000" -ForegroundColor Cyan
Write-Host ""
Write-Host "下一步：" -ForegroundColor Yellow
Write-Host "  1. 确保服务器已启动" -ForegroundColor White
Write-Host "  2. 双击 启动游戏.bat 或运行 MoxianReborn.exe" -ForegroundColor White
