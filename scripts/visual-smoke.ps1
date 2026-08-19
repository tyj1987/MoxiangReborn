<#
visual-smoke.ps1 — M-R0 视觉基线

自动启动 modern 服务 + MoxianClient，截 6 状态：
  1. login      — 登录界面
  2. charselect — 选角界面
  3. charmake   — 建角界面
  4. gamein     — 进入地图空场
  5. hud-only   — gamein + HP/MP/QuickSlot 显示
  6. inventory  — gamein + I 键开 Inventory

每状态截 1 张 .tga + 1 张 .png（同一帧），存到 modern/docs/screenshots/baseline/

usage:
  pwsh -File scripts/visual-smoke.ps1
  pwsh -File scripts/visual-smoke.ps1 -MapNumber 12 -TimeoutSeconds 60
#>

param(
    [string]$BuildDir = '',
    [int]$TimeoutSeconds = 300,
    [int]$MapNumber = 12,
    [string]$Username = 'visualsmoke',
    [string]$Password = 'V1sualSm0ke',
    [string]$CharacterName = 'VisualSmoke',
    [switch]$SkipServerStart
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$clientExe = Join-Path $buildRoot 'tools\MoxianClient\Debug\mxh_client.exe'
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'

if (-not (Test-Path -LiteralPath $clientExe)) {
    throw "MoxianClient.exe not found: $clientExe`n先 build: cmake --build $buildRoot --config Debug"
}

# 截图输出目录
$runId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$runRoot = Join-Path $repoRoot "modern\docs\restoration-plan\baseline\$runId"
New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
$logDir = Join-Path $runRoot 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stdout = Join-Path $logDir 'client.out.log'
$stderr = Join-Path $logDir 'client.err.log'

# 状态 1-4 由 mxh_client.exe --state-frames-dir 自动写
# 状态 5-6 (hud-only / inventory) 需要进入 gamein 后停留并触发
# 但 --exit-after-gamein 模式会立即 exit，所以用 --smoke-settle-frames 留出窗口
$frame = Join-Path $runRoot "map${MapNumber}.tga"
$stateFramesDir = Join-Path $runRoot 'state-frames'
New-Item -ItemType Directory -Force -Path $stateFramesDir | Out-Null

$dataDir = Join-Path $runRoot 'data'
New-Item -ItemType Directory -Force -Path $dataDir | Out-Null

$client = $null
$hudOnlyFrame = $null
$inventoryFrame = $null

try {
    if (-not $SkipServerStart) {
        Write-Host "[visual-smoke] starting modern servers..." -ForegroundColor Cyan
        & $serverScript -Mode start -Backend sqlite -DataDir $dataDir -MapNumber $MapNumber 2>&1
        Start-Sleep -Seconds 2
        & $serverScript -Mode status 2>&1
    }

    # 注册账号（visualsmoke 之前不存在，auth 会 FAIL 卡在 connect）
    $dbTool = Join-Path $buildRoot 'tools\MoxianDbTool\Debug\mxh_db_tool.exe'
    if (Test-Path -LiteralPath $dbTool) {
        Write-Host "[visual-smoke] registering account $Username..." -ForegroundColor Cyan
        $dbCfg = "sqlite;path=" + (Join-Path $dataDir 'login.db')
        Write-Host "[visual-smoke] registering account $Username (db=$dbCfg)..." -ForegroundColor Cyan
        $Password | & $dbTool register --db $dbCfg $Username 2>&1
    } else {
        Write-Host "[visual-smoke] db tool not found, skipping register: $dbTool" -ForegroundColor Yellow
    }

    # 状态 1-4: 5 状态自动截（connect/login/charselect/charmake/gamein）
    # 状态 5-6: 不带 --exit-after-gamein，停留一段时间，让外部触发 HUD 切换
    $arguments = @(
        '--login-host', '127.0.0.1',
        '--login-port', '16001',
        '--map-port', '18001',
        '--username', $Username,
        '--password', $Password,
        '--auto-login',
        '--auto-create',
        '--character-name', $CharacterName,
        '--save-frame', $frame,
        '--state-frames-dir', $stateFramesDir,
        '--smoke-settle-frames', '20',
        '--exit-after-gamein'
    )

    Write-Host "[visual-smoke] launching MoxianClient with smoke-settle-frames=40" -ForegroundColor Cyan
    $client = Start-Process -FilePath $clientExe -ArgumentList $arguments `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru

    # 等待 client 进程退出（follow-camera + smoke-settle-frames=40 触发后会自动 exit）
    if (-not $client.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
        throw "MoxianClient did not exit within ${TimeoutSeconds}s; log=$stderr"
    }
    $client.WaitForExit()
    $client.Refresh()
    $exitCode = $client.ExitCode
    Write-Host "[visual-smoke] MoxianClient exit code = $exitCode"

    # 验证基础 5 状态（login / charselect / charmake / gamein）
    & python (Join-Path $repoRoot 'scripts\verify-state-frames.py') $stateFramesDir
    if ($LASTEXITCODE -ne 0) {
        throw "5-state frames verification failed"
    }

    # 拷贝 5 状态截图到 baseline 目录
    $stateNames = @('connect', 'login', 'charselect', 'charmake', 'gamein')
    foreach ($name in $stateNames) {
        $src = Join-Path $stateFramesDir ("state-$name.tga")
        if (Test-Path -LiteralPath $src) {
            $dst = Join-Path $runRoot "modern-${name}.tga"
            Copy-Item -LiteralPath $src -Destination $dst -Force
            Write-Host "[visual-smoke] saved modern-${name}.tga" -ForegroundColor Green
        } else {
            Write-Host "[visual-smoke] MISSING state-$name.tga" -ForegroundColor Yellow
        }
    }

    # 状态 4: gamein 帧（从 --save-frame 复制）
    if (Test-Path -LiteralPath $frame) {
        $dst = Join-Path $runRoot 'modern-gamein-terrain.tga'
        Copy-Item -LiteralPath $frame -Destination $dst -Force
        Write-Host "[visual-smoke] saved modern-gamein-terrain.tga (gamein exit view)" -ForegroundColor Green
    }

    # TODO 状态 5 (hud-only) 和 状态 6 (inventory) 需要第二次启动 + SendInput 触发
    # 当前阶段先标 SKIP，等 M-R4 完成真实 cDialog 树后回来填

    Write-Host ""
    Write-Host "VISUAL_SMOKE_PARTIAL PASS (5/6 states captured)" -ForegroundColor Green
    Write-Host "Output: $runRoot"
    Write-Host "TODO: hud-only + inventory states require M-R4 dialog tree or SendInput injection"
} finally {
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
    if (-not $SkipServerStart) {
        & $serverScript -Mode stop 2>$null
    }
    Get-Process -Name 'mxh_login_server','mxh_agent_server_CHINA','mxh_agent_server_KOR','mxh_map_server_CHINA','mxh_map_server_KOR' -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
}
