# ============================================================================
# 墨香Reborn - 服务端部署脚本
# ============================================================================
# 使用方法：.\deploy_server.ps1
# ============================================================================

param(
    [string]$SourceDir = "",
    [string]$DeployDir = "",
    [string]$ResourceDir = "",
    [switch]$ResolveOnly,
    [switch]$SelfTest,
    [string]$PackageScanRoot = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot 'playdh_package_guard.ps1')

function Copy-MxhServerResourceTrees {
    param(
        [Parameter(Mandatory)][string]$ResourceDir,
        [Parameter(Mandatory)][string]$DeployDir,
        [Parameter(Mandatory)][string[]]$Patterns
    )
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
        if (Test-Path -LiteralPath $src) {
            if (-not (Test-Path -LiteralPath $dst)) {
                New-Item -ItemType Directory -Path $dst -Force | Out-Null
            }
            Copy-MxhPlayDhFiltered -SourceRoot $ResourceDir -SourcePath $src -DestinationRoot $dst -Patterns $Patterns
        }
    }
}

if (-not [string]::IsNullOrWhiteSpace($PackageScanRoot)) {
    $mxhProfilePath = Join-Path (Get-MxhPlayDhRepoRoot) 'modern/config/resource-profiles/playdh-current.json'
    $mxhPatterns = @(Get-MxhPlayDhExcludePatterns -ProfilePath $mxhProfilePath)
    exit (Invoke-MxhPlayDhPackageScan -Root $PackageScanRoot -Patterns $mxhPatterns)
}

if ($SelfTest) {
    $mxhFailed = $false
    $mxhParent = if (-not [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
    $mxhTemp = Join-Path $mxhParent ('mxh-server-exclude-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $mxhTemp | Out-Null
    try {
        $mxhProfilePath = Join-Path (Get-MxhPlayDhRepoRoot) 'modern/config/resource-profiles/playdh-current.json'
        $mxhPatterns = @(Get-MxhPlayDhExcludePatterns -ProfilePath $mxhProfilePath)
        $mxhSource = Join-Path $mxhTemp 'source'
        $mxhPackage = Join-Path $mxhTemp 'package'
        New-Item -ItemType Directory -Path $mxhSource | Out-Null
        $mxhKept = Join-Path (Join-Path (Join-Path $mxhSource 'Resource') 'EffectScript') 'kept.txt'
        New-Item -ItemType Directory -Path (Split-Path -Parent $mxhKept) -Force | Out-Null
        [System.IO.File]::WriteAllBytes($mxhKept, [Text.Encoding]::ASCII.GetBytes('synthetic-kept'))
        foreach ($mxhPattern in $mxhPatterns) {
            $mxhNormalized = (($mxhPattern -replace '\\','/').Trim())
            if ($mxhNormalized.EndsWith('/')) {
                $mxhSample = 'Resource/EffectScript/' + $mxhNormalized + 'synthetic.txt'
            } else {
                $mxhSample = 'Resource/EffectScript/nested/' + $mxhNormalized.Replace('*', 'Synth')
            }
            $mxhToken = ($mxhNormalized -replace '[^A-Za-z0-9]+','-').Trim('-')
            if (-not (Test-MxhPlayDhExcludedRelativePath -RelativePath $mxhSample -Patterns $mxhPatterns)) {
                Write-Host "PS_GATE_CASE server-exclude-$mxhToken exit=1 FAIL"
                $mxhFailed = $true
                continue
            }
            $mxhSamplePath = Join-Path $mxhSource ($mxhSample -replace '/',[IO.Path]::DirectorySeparatorChar)
            $mxhSampleParent = Split-Path -Parent $mxhSamplePath
            if (-not (Test-Path -LiteralPath $mxhSampleParent)) {
                New-Item -ItemType Directory -Path $mxhSampleParent -Force | Out-Null
            }
            [System.IO.File]::WriteAllBytes($mxhSamplePath, [Text.Encoding]::ASCII.GetBytes('synthetic-forbidden'))
            Write-Host "PS_GATE_CASE server-exclude-$mxhToken exit=0 OK"
        }
        $mxhMapBak = Join-Path (Join-Path (Join-Path $mxhSource 'Resource') 'Map') 'x.bin_bak'
        New-Item -ItemType Directory -Path (Split-Path -Parent $mxhMapBak) -Force | Out-Null
        [System.IO.File]::WriteAllBytes($mxhMapBak, [Text.Encoding]::ASCII.GetBytes('synthetic-forbidden'))
        Copy-MxhServerResourceTrees -ResourceDir $mxhSource -DeployDir $mxhPackage -Patterns $mxhPatterns
        $mxhLeaked = @(Get-MxhPlayDhPackageForbidden -Root $mxhPackage -Patterns $mxhPatterns)
        $mxhKeptCopied = Test-Path -LiteralPath (Join-Path (Join-Path (Join-Path (Join-Path $mxhPackage 'Map') 'Resource') 'EffectScript') 'kept.txt')
        if ($mxhLeaked.Count -eq 0 -and $mxhKeptCopied) {
            Write-Host 'PS_GATE_CASE server-copy-excludes exit=0 OK'
        } else {
            Write-Host 'PS_GATE_CASE server-copy-excludes exit=1 FAIL'
            foreach ($mxhHit in $mxhLeaked) { Write-Host "PLAYDH_PACKAGE_FORBIDDEN $mxhHit" }
            $mxhFailed = $true
        }
        $mxhMapBakDest = Join-Path (Join-Path (Join-Path (Join-Path $mxhPackage 'Map') 'Resource') 'Map') 'x.bin_bak'
        if (Test-Path -LiteralPath $mxhMapBakDest) {
            Write-Host 'PS_GATE_CASE server-exclude-underscore-bak exit=1 FAIL'
            $mxhFailed = $true
        } else {
            Write-Host 'PS_GATE_CASE server-exclude-underscore-bak exit=0 OK'
        }
        $mxhPwsh = (Get-Process -Id $PID).Path
        function Invoke-MxhServerGateCase {
            param($Name, $Root, $ExpectZero, $ExpectText)
            $mxhOutput = & $mxhPwsh -NoProfile -File $PSCommandPath -PackageScanRoot $Root 2>&1 | Out-String
            $mxhCode = $LASTEXITCODE
            $mxhTextOk = $mxhOutput.Contains($ExpectText)
            $mxhExitOk = if ($ExpectZero) { $mxhCode -eq 0 } else { $mxhCode -ne 0 }
            if (-not $mxhTextOk -or -not $mxhExitOk) {
                Write-Host "PS_GATE_CASE $Name exit=$mxhCode FAIL"
                throw "server package gate case $Name failed"
            }
            Write-Host "PS_GATE_CASE $Name exit=$mxhCode OK"
        }
        Invoke-MxhServerGateCase -Name 'server-scan-clean' -Root $mxhPackage -ExpectZero $true -ExpectText 'PLAYDH_PACKAGE_SCAN OK'
        $mxhInjected = Join-Path (Join-Path (Join-Path (Join-Path $mxhPackage 'Map') 'Resource') 'EffectScript') 'MHClientInjected.exe'
        New-Item -ItemType Directory -Path (Split-Path -Parent $mxhInjected) -Force | Out-Null
        [System.IO.File]::WriteAllBytes($mxhInjected, [Text.Encoding]::ASCII.GetBytes('synthetic-injected'))
        Invoke-MxhServerGateCase -Name 'server-scan-injected' -Root $mxhPackage -ExpectZero $false -ExpectText 'PLAYDH_PACKAGE_FORBIDDEN Map/Resource/EffectScript/MHClientInjected.exe'
    } finally {
        if (Test-Path -LiteralPath $mxhTemp) {
            Remove-Item -LiteralPath $mxhTemp -Recurse -Force
        }
    }
    if ($mxhFailed) { exit 1 }
    exit 0
}

$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($ResourceDir)) {
    if (-not [string]::IsNullOrWhiteSpace($env:MXH_PLAYDH_ROOT)) {
        $ResourceDir = $env:MXH_PLAYDH_ROOT
    } else {
        Write-Host 'MXH_PLAYDH_ROOT is not set and no --playdh-root/-ResourceDir was given'
        exit 1
    }
}
if ($ResolveOnly) {
    if (-not (Test-Path -LiteralPath $ResourceDir -PathType Container)) {
        Write-Host ("MXH_PLAYDH_ROOT is set to `"" + $ResourceDir + "`" but that directory does not exist")
        exit 1
    }
    $ResourceDir = (Resolve-Path -LiteralPath $ResourceDir).Path
    Write-Output "RESOURCE_DIR=$ResourceDir"
    exit 0
}
if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $SourceDir = Join-Path $repoRoot 'modern\build\tools'
}
if ([string]::IsNullOrWhiteSpace($DeployDir)) {
    $DeployDir = Join-Path $repoRoot 'deploy\server'
}
if (-not (Test-Path -LiteralPath $ResourceDir -PathType Container)) {
    Write-Host ("MXH_PLAYDH_ROOT is set to `"" + $ResourceDir + "`" but that directory does not exist")
    exit 1
}

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

# 复制游戏资源。派生路径和 MHClient*.exe 不进入包，打包后再扫一遍。
Write-Host "[4/5] 复制游戏资源..." -ForegroundColor Yellow
$mxhExcludeProfile = Join-Path $repoRoot 'modern/config/resource-profiles/playdh-current.json'
$mxhExcludePatterns = @(Get-MxhPlayDhExcludePatterns -ProfilePath $mxhExcludeProfile)
Copy-MxhServerResourceTrees -ResourceDir $ResourceDir -DeployDir $DeployDir -Patterns $mxhExcludePatterns
Write-Host "  复制: Resource/Map Client QuestScript SkillArea EffectScript Data Image Sound" -ForegroundColor Gray

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
$mxhPackageHits = @(Get-MxhPlayDhPackageForbidden -Root $DeployDir -Patterns $mxhExcludePatterns)
if ($mxhPackageHits.Count -gt 0) {
    foreach ($mxhPackageHit in $mxhPackageHits) {
        Write-Host "PLAYDH_PACKAGE_FORBIDDEN $mxhPackageHit"
    }
    exit 1
}
Write-Host ("PLAYDH_PACKAGE_SCAN OK files=" + @(Get-ChildItem -LiteralPath $DeployDir -Recurse -File -Force).Count)
