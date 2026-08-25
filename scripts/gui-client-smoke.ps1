[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [int]$TimeoutSeconds = 20,
    [ValidateRange(0, 65535)]
    [int]$MapNumber = 10,
    [int]$MinimumNpcCount = 0,
    [switch]$FollowCamera,
    [switch]$AuditMapDependencies,
    [switch]$AuditNpcDependencies,
    [switch]$ExerciseInventory,
    [switch]$ExerciseSkills
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
$previousGuiSmokeOpenInventory = $env:MXH_GUI_SMOKE_OPEN_INVENTORY
$previousGuiSmokeSkills = $env:MXH_GUI_SMOKE_SKILLS

try {
    if ($AuditMapDependencies) {
        $dependencyAudit = Join-Path $repoRoot 'modern\tools\audit_map_dependencies.py'
        $dependencyOutput = Join-Path $runRoot 'map-dependencies.txt'
        & python $dependencyAudit $MapNumber (Join-Path $repoRoot 'modern\data\PlayDH') `
            --build-dir $buildRoot --output $dependencyOutput
        if ($LASTEXITCODE -ne 0) {
            throw "GUI smoke static map dependency gate failed for Map $MapNumber; report=$dependencyOutput"
        }
    }
    & $serverScript -Mode start -Backend sqlite -DataDir $dataRoot -MapNumber $MapNumber
    $dbTool = Join-Path $buildRoot 'tools\MoxianDbTool\mxh_db_tool.exe'
    if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
        throw "Missing database tool required for isolated GUI smoke: $dbTool"
    }
    $dbConfig = 'sqlite;path=' + (Join-Path $dataRoot 'moxian.db')
    # PowerShell's native stdin pipeline can encode the string as UTF-16 on
    # some hosts, which makes the DB tool receive embedded NULs and reject an
    # otherwise valid password.  Use an explicit ASCII fixture and redirect
    # stdin so the account bootstrap is byte-stable across shells.
    $registerInput = Join-Path $runRoot 'register.stdin'
    [System.IO.File]::WriteAllText(
        $registerInput, "Test1234`n",
        [System.Text.Encoding]::ASCII)
    $register = Start-Process -FilePath $dbTool `
        -ArgumentList @('register', '--db', $dbConfig, 'test') `
        -RedirectStandardInput $registerInput -NoNewWindow -Wait -PassThru
    if ($register.ExitCode -ne 0) { throw "GUI smoke test account registration failed" }
    Remove-Item -LiteralPath $registerInput -Force -ErrorAction SilentlyContinue
    if ($ExerciseInventory) {
        $grantSql = "INSERT INTO modern_item_grant(idempotency_key,character_id,item_id,item_count,status,created_by,reason)VALUES('gui-smoke-$runId',100000,8000,3,'pending','gui-smoke','inventory-visual-gate')"
        & $dbTool exec --db $dbConfig $grantSql
        if ($LASTEXITCODE -ne 0) { throw "GUI smoke inventory grant setup failed" }
    }
    if ($ExerciseSkills) {
        $skillSchemaSql = "CREATE TABLE IF NOT EXISTS modern_player_skill(player_id INTEGER NOT NULL,slot INTEGER NOT NULL,skill_idx INTEGER NOT NULL,level INTEGER NOT NULL DEFAULT 1,updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY(player_id,slot))"
        & $dbTool exec --db $dbConfig $skillSchemaSql
        if ($LASTEXITCODE -ne 0) { throw "GUI smoke skill schema setup failed" }
        $skillSql = "INSERT INTO modern_player_skill(player_id,slot,skill_idx,level)VALUES(100000,0,10,1)"
        & $dbTool exec --db $dbConfig $skillSql
        if ($LASTEXITCODE -ne 0) { throw "GUI smoke skill setup failed" }
    }
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
    if ($ExerciseInventory) { $env:MXH_GUI_SMOKE_OPEN_INVENTORY = '1' }
    if ($ExerciseSkills) { $env:MXH_GUI_SMOKE_SKILLS = '1' }
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
    if ($AuditNpcDependencies) {
        $explorer = Join-Path $buildRoot 'tools\MoxianResourceExplorer\mxh_explorer.exe'
        if (-not (Test-Path -LiteralPath $explorer -PathType Leaf)) {
            throw "Missing resource explorer required for NPC dependency gate: $explorer"
        }
        $catalogLines = & $explorer npc-chx (Join-Path $repoRoot 'modern\data\PlayDH\Resource\Client\NpcChxList.bin')
        $catalog = @{}
        foreach ($line in $catalogLines) {
            if ($line -match '^([0-9]+)\s+(.+)$') { $catalog[[int]$Matches[1]] = $Matches[2].Trim() }
        }
        $packLines = & $explorer list (Join-Path $repoRoot 'modern\data\PlayDH\npc.pak')
        $packNames = @($packLines | ForEach-Object {
            if ($_ -match '^\s*[0-9]+\s+(.+)$') { $Matches[1].Trim() }
        })
        $missingNpcAssets = @{}
        foreach ($match in [regex]::Matches($log, 'NpcAdd id=\d+ kind=(\d+)')) {
            $kind = [int]$match.Groups[1].Value
            if (-not $catalog.ContainsKey($kind)) {
                $missingNpcAssets["kind=$kind"] = 'catalog-entry-missing'
                continue
            }
            $asset = [string]$catalog[$kind]
            if (-not ($packNames -contains $asset)) { $missingNpcAssets["kind=$kind"] = $asset }
        }
        if ($missingNpcAssets.Count -gt 0) {
            $details = ($missingNpcAssets.GetEnumerator() | ForEach-Object { "$($_.Key):$($_.Value)" }) -join ', '
            throw "GUI smoke NPC dependency gate failed for Map ${MapNumber}: $details; log=$stderr"
        }
    }
    if ($log -match 'GameLoading: Unable to enter Map \d+: (.+)') {
        throw "GUI smoke map load failed: $($Matches[1]); log=$stderr"
    }
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
    if ($ExerciseInventory -and $log -notmatch 'GUI_SMOKE_INVENTORY_OPEN') {
        throw "GUI smoke inventory exercise did not open the live inventory UI; log=$stderr"
    }
    if ($ExerciseSkills -and $log -notmatch 'GUI_SMOKE_SKILLS=10') {
        throw "GUI smoke skill exercise did not receive persisted skill 10; log=$stderr"
    }
    if ($log -notmatch "GameInAck .* map=$MapNumber(?:\D|$)") {
        throw "GUI smoke GameInAck map does not match requested map $MapNumber; log=$stderr"
    }
    if ($log -match 'release visual gate failed: placeholderCount=(\d+) failedModelCount=(\d+)') {
        throw "GUI smoke visual gate failed for Map $MapNumber (placeholders=$($Matches[1]) failedModels=$($Matches[2])); log=$stderr"
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
        if ($MapNumber -eq 10 -and
            $log -notmatch '\[entity\] original model object=\d+ type=monster kind=\d+ chx=L\d+\.chx') {
            throw "GUI player-view smoke did not resolve a Map10 monster model; log=$stderr"
        }
        if ($MapNumber -ne 10 -and
            $log -notmatch '\[entity\] original model object=\d+ type=npc kind=\d+ chx=N\d+\.chx') {
            throw "GUI player-view smoke did not resolve an NPC model for Map $MapNumber; log=$stderr"
        }
        if ($log -notmatch 'entity loaded=\d+ failed=0 placeholders=0 monsters=\d+ npcs=\d+') {
            throw "GUI player-view smoke reported missing/placeholder entities for Map $MapNumber; log=$stderr"
        }
    }
    if (-not (Test-Path -LiteralPath $frame)) { throw "GUI smoke missing terrain frame: $frame" }
    $stateFramesDir = Join-Path $runRoot "state-frames"
    if (-not (Test-Path -LiteralPath $stateFramesDir)) { throw "GUI smoke missing state-frames dir: $stateFramesDir" }
    & python (Join-Path $repoRoot 'scripts\verify-state-frames.py') $stateFramesDir --permissive
    if ($LASTEXITCODE -ne 0) { throw "GUI state frames validation failed: $stateFramesDir" }
    if (-not $FollowCamera) {
        $terrainArgs = @($frame)
        if ($ExerciseInventory) { $terrainArgs += '--allow-ui-overlay' }
        & python (Join-Path $repoRoot 'scripts\verify-terrain-frame.py') @terrainArgs
        if ($LASTEXITCODE -ne 0) { throw "GUI terrain frame validation failed: $frame" }
    } else {
        & python (Join-Path $repoRoot 'scripts\verify-entity-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI entity frame validation failed: $frame" }
    }
    Write-Host "GUI_CLIENT_SMOKE PASS (map=$MapNumber, monsters=$monsterCount, npcs=$npcCount, original BGM/create/select/game-in, evidence=$stderr, frame=$frame)" -ForegroundColor Green
}
finally {
    $registerInput = Join-Path $runRoot 'register.stdin'
    Remove-Item -LiteralPath $registerInput -Force -ErrorAction SilentlyContinue
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
    if ($null -eq $previousGuiSmokeOpenInventory) {
        Remove-Item Env:MXH_GUI_SMOKE_OPEN_INVENTORY -ErrorAction SilentlyContinue
    } else {
        $env:MXH_GUI_SMOKE_OPEN_INVENTORY = $previousGuiSmokeOpenInventory
    }
    if ($null -eq $previousGuiSmokeSkills) {
        Remove-Item Env:MXH_GUI_SMOKE_SKILLS -ErrorAction SilentlyContinue
    } else { $env:MXH_GUI_SMOKE_SKILLS = $previousGuiSmokeSkills }
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
    & $serverScript -Mode stop
}
