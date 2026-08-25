[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [int]$TimeoutSeconds = 20,
    [ValidateRange(1, 65535)]
    [int]$MapNumber = 10,
    [int]$MinimumNpcCount = 0,
    [switch]$FollowCamera
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$clientExe = Join-Path $buildRoot 'tools\MoxianClient\mxh_client.exe'
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
if (-not (Test-Path -LiteralPath $clientExe)) { throw "Missing GUI client: $clientExe" }

$runId = [Guid]::NewGuid().ToString('N')
$runRoot = Join-Path $buildRoot "runtime\gui-smoke\$runId"
$logDir = Join-Path $runRoot 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stdout = Join-Path $logDir 'client.out.log'
$stderr = Join-Path $logDir 'client.err.log'
$characterName = 'GUI' + $runId.Substring(0, 10)
$frame = Join-Path $runRoot "map${MapNumber}.tga"
$dataRoot = Join-Path $runRoot 'data'
$client = $null
$previousGuiSmokePassword = $env:MXH_GUI_SMOKE_PASSWORD
$previousGuiSmokeExit = $env:MXH_GUI_SMOKE_EXIT
$previousGuiSmokeRenderEntities = $env:MXH_GUI_SMOKE_RENDER_ENTITIES

try {
    & $serverScript -Mode start -Backend sqlite -DataDir $dataRoot -MapNumber $MapNumber
    $dbTool = Join-Path $buildRoot 'tools\MoxianDbTool\mxh_db_tool.exe'
    if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
        throw "Missing database tool required for isolated GUI smoke: $dbTool"
    }
    $dbConfig = 'sqlite;path=' + (Join-Path $dataRoot 'moxian.db')
    'Test1234' | & $dbTool register --db $dbConfig test 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "GUI smoke test account registration failed" }
    $arguments = @(
        '--login-host', '127.0.0.1',
        '--login-port', '16001',
        '--map-port', '18001',
        '--username', 'test',
        '--password-env', 'MXH_GUI_SMOKE_PASSWORD',
        '--auto-login',
        '--auto-create',
        '--character-name', $characterName,
        '--save-frame', $frame,
        '--state-frames-dir', (Join-Path $runRoot 'state-frames'),
        '--smoke-settle-frames', '20',
        '--exit-after-gamein',
        '--resource-root', (Join-Path $repoRoot 'modern\data\PlayDH')
    )
    if ($FollowCamera) { $arguments += '--follow-camera' }
    $env:MXH_GUI_SMOKE_PASSWORD = 'Test1234'
    $env:MXH_GUI_SMOKE_EXIT = '1'
    if ($FollowCamera) { $env:MXH_GUI_SMOKE_RENDER_ENTITIES = '1' }
    $client = Start-Process -FilePath $clientExe -ArgumentList $arguments `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru `
        -WorkingDirectory (Join-Path $buildRoot 'tools\MoxianClient')
    if (-not $client.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
        throw "GUI client did not reach GameIn within ${TimeoutSeconds}s; log=$stderr"
    }
    $client.WaitForExit()
    $client.Refresh()
    $exitCode = $client.ExitCode
    if ($null -ne $exitCode -and $exitCode -ne 0) {
        throw "GUI client exited with code $exitCode; log=$stderr"
    }
    $log = Get-Content -LiteralPath $stderr -Raw
    foreach ($marker in @('playing original BGM id=1667', 'SFX manifest ready', '[terrain] original HFL loaded', '[static] original STM loaded', 'CharacterSelectAck', 'GameInAck', 'GUI_SMOKE_PASS')) {
        if ($log -notmatch [regex]::Escape($marker)) {
            throw "GUI smoke missing marker '$marker'; log=$stderr"
        }
    }
    $expectedMapBgm = if ($MapNumber -eq 1) { 1671 } elseif ($MapNumber -eq 12) { 1670 } else { 0 }
    if ($expectedMapBgm -ne 0 -and $log -notmatch [regex]::Escape("playing original BGM id=$expectedMapBgm")) {
        throw "GUI smoke missing Map $MapNumber BGM id=$expectedMapBgm; log=$stderr"
    }
    if ($log -notmatch [regex]::Escape('sent CharacterMakeSyn') -and
        $log -notmatch [regex]::Escape('first valid chrid=')) {
        throw "GUI smoke neither created nor loaded a character; log=$stderr"
    }
    if ($log -notmatch "CharacterSelectAck chrid=\d+ map_num=$MapNumber") {
        throw "GUI smoke character map does not match requested map $MapNumber; log=$stderr"
    }
    if ($log -notmatch "GameInAck .* map=$MapNumber(?:\D|$)") {
        throw "GUI smoke GameInAck map does not match requested map $MapNumber; log=$stderr"
    }
    $npcCount = ([regex]::Matches($log, 'CInGameState: NpcAdd ')).Count
    if ($npcCount -lt $MinimumNpcCount) {
        throw "GUI smoke received $npcCount NPCs, expected at least $MinimumNpcCount; log=$stderr"
    }
    $monsterCount = ([regex]::Matches($log, 'CInGameState: MonsterAdd object_id=')).Count
    if ($MapNumber -eq 10 -and $monsterCount -ne 228) {
        throw "GUI smoke received $monsterCount Map10 monsters, expected exactly 228; log=$stderr"
    }
    if ($FollowCamera) {
        foreach ($marker in @('[sky] original MOD loaded meshes=8/8 textures=8/8', '[terrain] player camera active', '[entity] original MonsterList loaded', '[entity] original animation active')) {
            if ($log -notmatch [regex]::Escape($marker)) {
                throw "GUI player-view smoke missing marker '$marker'; log=$stderr"
            }
        }
        if ($log -notmatch '\[entity\] original model object=\d+ type=player kind=\d+ chx=man\.chx') {
            throw "GUI player-view smoke did not resolve the local player model; log=$stderr"
        }
        if ($log -notmatch '\[entity\] original model object=\d+ type=monster kind=\d+ chx=L\d+\.chx') {
            throw "GUI player-view smoke did not resolve a monster model; log=$stderr"
        }
        if ($log -notmatch 'entity loaded=\d+ failed=0 placeholders=0 monsters=228') {
            throw "GUI player-view smoke reported missing/placeholder Map10 entities; log=$stderr"
        }
    }
    if (-not (Test-Path -LiteralPath $frame)) { throw "GUI smoke missing terrain frame: $frame" }
    $stateFramesDir = Join-Path $runRoot "state-frames"
    if (-not (Test-Path -LiteralPath $stateFramesDir)) { throw "GUI smoke missing state-frames dir: $stateFramesDir" }
    & python (Join-Path $repoRoot 'scripts\verify-state-frames.py') $stateFramesDir --permissive
    if ($LASTEXITCODE -ne 0) { throw "GUI state frames validation failed: $stateFramesDir" }
    if (-not $FollowCamera) {
        & python (Join-Path $repoRoot 'scripts\verify-terrain-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI terrain frame validation failed: $frame" }
    } else {
        & python (Join-Path $repoRoot 'scripts\verify-entity-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI entity frame validation failed: $frame" }
    }
    Write-Host "GUI_CLIENT_SMOKE PASS (map=$MapNumber, monsters=$monsterCount, npcs=$npcCount, original BGM/create/select/game-in, evidence=$stderr, frame=$frame)" -ForegroundColor Green
}
finally {
    if ($null -eq $previousGuiSmokePassword) {
        Remove-Item Env:MXH_GUI_SMOKE_PASSWORD -ErrorAction SilentlyContinue
    } else {
        $env:MXH_GUI_SMOKE_PASSWORD = $previousGuiSmokePassword
    }
    if ($null -eq $previousGuiSmokeExit) {
        Remove-Item Env:MXH_GUI_SMOKE_EXIT -ErrorAction SilentlyContinue
    } else {
        $env:MXH_GUI_SMOKE_EXIT = $previousGuiSmokeExit
    }
    if ($null -eq $previousGuiSmokeRenderEntities) {
        Remove-Item Env:MXH_GUI_SMOKE_RENDER_ENTITIES -ErrorAction SilentlyContinue
    } else {
        $env:MXH_GUI_SMOKE_RENDER_ENTITIES = $previousGuiSmokeRenderEntities
    }
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
    & $serverScript -Mode stop
}
