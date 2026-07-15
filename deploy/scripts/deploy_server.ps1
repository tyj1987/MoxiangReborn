# ============================================================================
# 墨香Reborn - 服务端部署脚本
# ============================================================================
# 使用方法：.\deploy_server.ps1
# ============================================================================

param(
    [string]$SourceDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking",
    [string]$DeployDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\deploy\server",
    [string]$ResourceDir = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码配套资源】\PlayDH"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  墨香Reborn - 服务端部署脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 创建部署目录结构
Write-Host "[1/5] 创建目录结构..." -ForegroundColor Yellow
$dirs = @(
    "$DeployDir\Distribute",
    "$DeployDir\Agent",
    "$DeployDir\Map",
    "$DeployDir\MurimNet",
    "$DeployDir\RMTool",
    "$DeployDir\Monitoring",
    "$DeployDir\Map\serverset",
    "$DeployDir\Map\serverset\1",
    "$DeployDir\Map\Resource",
    "$DeployDir\Map\Resource\Map",
    "$DeployDir\Map\Resource\Client",
    "$DeployDir\Map\Resource\QuestScript",
    "$DeployDir\Map\Resource\SkillArea"
)

foreach ($dir in $dirs) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}
Write-Host "  目录结构创建完成" -ForegroundColor Green

# 复制服务器可执行文件
Write-Host "[2/5] 复制服务器可执行文件..." -ForegroundColor Yellow
$serverFiles = @(
    @{Source="DistributeServer.exe"; Dest="Distribute\DistributeServer.exe"},
    @{Source="AgentServer.exe"; Dest="Agent\AgentServer.exe"},
    @{Source="MapServer.exe"; Dest="Map\MapServer.exe"},
    @{Source="RMToolServer.exe"; Dest="RMTool\RMToolServer.exe"},
    @{Source="MonitoringServer.exe"; Dest="Monitoring\MonitoringServer.exe"},
    @{Source="BaseNetwork.dll"; Dest="Agent\BaseNetwork.dll"},
    @{Source="4DyuchiGXGFunc.dll"; Dest="Map\4DyuchiGXGFunc.dll"},
    @{Source="RainFTP.dll"; Dest="Agent\RainFTP.dll"},
    @{Source="AntiCpSvr.dll"; Dest="Agent\AntiCpSvr.dll"},
    @{Source="dbghelp.dll"; Dest="Map\dbghelp.dll"},
    @{Source="FreeImage.dll"; Dest="Map\FreeImage.dll"},
    @{Source="SoundLib.dll"; Dest="Map\SoundLib.dll"},
    @{Source="mss32.dll"; Dest="Map\mss32.dll"},
    @{Source="SS3DAudio_muk.dll"; Dest="Map\SS3DAudio_muk.dll"},
    @{Source="SS3DExecutiveForMuk.dll"; Dest="Map\SS3DExecutiveForMuk.dll"},
    @{Source="SS3DFileStorage.dll"; Dest="Map\SS3DFileStorage.dll"},
    @{Source="SS3DGeometryForMuk.dll"; Dest="Map\SS3DGeometryForMuk.dll"},
    @{Source="SS3DGFunc.dll"; Dest="Map\SS3DGFunc.dll"},
    @{Source="SS3DGFunc1.dll"; Dest="Map\SS3DGFunc1.dll"},
    @{Source="SS3DGFuncN.dll"; Dest="Map\SS3DGFuncN.dll"},
    @{Source="SS3DGFuncSSE.dll"; Dest="Map\SS3DGFuncSSE.dll"},
    @{Source="SS3DRendererForMuk.dll"; Dest="Map\SS3DRendererForMuk.dll"}
)

foreach ($file in $serverFiles) {
    $src = Join-Path $SourceDir $file.Source
    $dst = Join-Path $DeployDir $file.Dest
    if (Test-Path $src) {
        Copy-Item -Path $src -Destination $dst -Force
        Write-Host "  复制: $($file.Source)" -ForegroundColor Gray
    } else {
        Write-Host "  [警告] 未找到: $($file.Source)" -ForegroundColor Yellow
    }
}

# 复制配置文件
Write-Host "[3/5] 复制配置文件..." -ForegroundColor Yellow
$configFiles = @(
    "serverset.txt",
    "AgentDBInfo.txt",
    "DistributeDBInfo.txt",
    "MapDBInfo.txt",
    "MHVerInfo.ver",
    "InitedDate.txt",
    "mapchangefail.txt"
)

foreach ($file in $configFiles) {
    $src = Join-Path $SourceDir $file
    $dst = Join-Path $DeployDir "Map\$file"
    if (Test-Path $src) {
        Copy-Item -Path $src -Destination $dst -Force
        Write-Host "  复制: $file" -ForegroundColor Gray
    }
}

# 复制游戏资源
Write-Host "[4/5] 复制游戏资源..." -ForegroundColor Yellow
$resourceDirs = @(
    @{Source="Resource\Map"; Dest="Map\Resource\Map"},
    @{Source="Resource\Client"; Dest="Map\Resource\Client"},
    @{Source="Resource\QuestScript"; Dest="Map\Resource\QuestScript"},
    @{Source="Resource\SkillArea"; Dest="Map\Resource\SkillArea"},
    @{Source="Resource\EffectScript"; Dest="Map\Resource\EffectScript"},
    @{Source="Data"; Dest="Map\Data"},
    @{Source="Image"; Dest="Map\Image"},
    @{Source="Sound"; Dest="Map\Sound"}
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

# 创建服务器配置文件
Write-Host "[5/5] 生成服务器配置文件..." -ForegroundColor Yellow

# Agent 服务器配置
$agentIni = Join-Path $DeployDir "Agent\AgentServer.ini"
"[Network]" | Set-Content -Path $agentIni -Encoding UTF8
"Port=10000" | Add-Content -Path $agentIni
"MaxConnections=2000" | Add-Content -Path $agentIni
"RecvBufferSize=65536" | Add-Content -Path $agentIni
"SendBufferSize=65536" | Add-Content -Path $agentIni
"" | Add-Content -Path $agentIni
"[Database]" | Add-Content -Path $agentIni
"DSN=MHCMEMBER" | Add-Content -Path $agentIni
"AdminDSN=MHGAME" | Add-Content -Path $agentIni
"ThreadCount=2" | Add-Content -Path $agentIni
"MaxQueries=512" | Add-Content -Path $agentIni
"" | Add-Content -Path $agentIni
"[Server]" | Add-Content -Path $agentIni
"ServerSet=1" | Add-Content -Path $agentIni
"DistributeIP=127.0.0.1" | Add-Content -Path $agentIni
"DistributePort=9000" | Add-Content -Path $agentIni

# Distribute 服务器配置
$distributeIni = Join-Path $DeployDir "Distribute\DistributeServer.ini"
"[Network]" | Set-Content -Path $distributeIni -Encoding UTF8
"Port=9000" | Add-Content -Path $distributeIni
"MaxConnections=1000" | Add-Content -Path $distributeIni
"RecvBufferSize=32768" | Add-Content -Path $distributeIni
"SendBufferSize=32768" | Add-Content -Path $distributeIni
"" | Add-Content -Path $distributeIni
"[Database]" | Add-Content -Path $distributeIni
"DSN=MHCMEMBER" | Add-Content -Path $distributeIni
"AdminDSN=MHGAME" | Add-Content -Path $distributeIni
"ThreadCount=2" | Add-Content -Path $distributeIni
"MaxQueries=512" | Add-Content -Path $distributeIni
"" | Add-Content -Path $distributeIni
"[Server]" | Add-Content -Path $distributeIni
"ServerName=MoxianReborn" | Add-Content -Path $distributeIni
"MaxServers=10" | Add-Content -Path $distributeIni

# Map 服务器配置
$mapIni = Join-Path $DeployDir "Map\MapServer.ini"
"[Network]" | Set-Content -Path $mapIni -Encoding UTF8
"Port=10001" | Add-Content -Path $mapIni
"MaxConnections=500" | Add-Content -Path $mapIni
"RecvBufferSize=65536" | Add-Content -Path $mapIni
"SendBufferSize=65536" | Add-Content -Path $mapIni
"" | Add-Content -Path $mapIni
"[Database]" | Add-Content -Path $mapIni
"DSN=MHGAME" | Add-Content -Path $mapIni
"AdminDSN=MHGAME" | Add-Content -Path $mapIni
"ThreadCount=2" | Add-Content -Path $mapIni
"MaxQueries=1024" | Add-Content -Path $mapIni
"" | Add-Content -Path $mapIni
"[Server]" | Add-Content -Path $mapIni
"ServerSet=1" | Add-Content -Path $mapIni
"MapIndex=1" | Add-Content -Path $mapIni
"MapName=首阳" | Add-Content -Path $mapIni
"AgentIP=127.0.0.1" | Add-Content -Path $mapIni
"AgentPort=10000" | Add-Content -Path $mapIni
"" | Add-Content -Path $mapIni
"[Game]" | Add-Content -Path $mapIni
"MaxPlayers=200" | Add-Content -Path $mapIni
"MonsterRespawn=300" | Add-Content -Path $mapIni
"ItemDropRate=1.0" | Add-Content -Path $mapIni
"ExpRate=1.0" | Add-Content -Path $mapIni

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  服务端部署完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Deploy Dir: $DeployDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "Server Structure:" -ForegroundColor Yellow
Write-Host "  Distribute\  - Login Server (Port 9000)" -ForegroundColor White
Write-Host "  Agent\       - Agent Server (Port 10000)" -ForegroundColor White
Write-Host "  Map\         - Map Server (Port 10001)" -ForegroundColor White
Write-Host "  RMTool\      - GM Tool Server" -ForegroundColor White
Write-Host "  Monitoring\  - Monitor Server" -ForegroundColor White
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Ensure database is installed (run install_database.ps1)" -ForegroundColor White
Write-Host "  2. Run .\start_server.ps1 to start servers" -ForegroundColor White
