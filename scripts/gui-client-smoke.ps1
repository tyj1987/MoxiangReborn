<#
.SYNOPSIS
  GUI client smoke against the modern login/agent/map stack.

.DESCRIPTION
  Default (no -EphemeralLocalServer): unchanged. This script calls
  deploy/scripts/start_modern.ps1, which only mutates servers on VM112.

  -EphemeralLocalServer: do not call start_modern.ps1. This script starts
  login/agent/map itself, loopback only, sqlite only, and stops those PIDs
  when the run ends. Requires PowerShell 7 (Start-Process -Environment,
  same as start_modern.ps1).

  ROG example (PowerShell 7.4+). Default ports are 26001/27001/28001.
  Map17 uses base+2002, which is 28003, only when StartArea is 17.
  28003 is an allowed smoke port.
  16001/17001/18001 and 36001/37001/38001 stay rejected.
  -MapNumber 10 enters a character pre-saved on Map10. -LiveCharacterBirth
  creates a character on the configured StartArea. verify-map17-birth.py
  runs only when that StartArea is 17.
    pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\gui-client-smoke.ps1 -EphemeralLocalServer -BuildDir modern\build

  mxh_client dials only --login-host/--login-port. With the switch on,
  that login port is EphemeralPortBase (default 26001) and --login-host
  stays 127.0.0.1. With the switch off, the client arguments stay
  127.0.0.1:16001 and --map-port 18001. Agent host:port is not a client
  socket: login puts it in LoginAck (--agent-addr 127.0.0.1 and
  --agent-port on the login process; the login binary otherwise
  advertises port 7001). The client does not open a map socket; the
  agent dials --map-server 127.0.0.1:<map port>. --map-port is only
  logged by mxh_client and follows the same map port when the switch
  is on. --agent-port is not parsed by today's client and is still
  passed so a later build can see the configured agent port.

  Safety rules live in gui-client-smoke-ephemeral.ps1. Client exit-code
  handling is unchanged (any non-zero, including 4, fails the smoke).
  MXH_GUI_SMOKE_RENDER_ENTITIES is still set here. On PowerShell 7.4+
  the client is started with an explicit copy of the process environment,
  so MXH_FOG_DISABLE and MXH_STM_LIGHTING reach that process. The client
  fog switch is MXH_FOG_DISABLE=1 (no command-line flag). --resource-root
  is still passed.

.PARAMETER TimeoutSeconds
  Seconds to wait for the GUI client process to exit. Default 90.
  On the gate machine a Map10 scene with 228 monsters can still be
  running at 20s; 90s covers both fog-off and fog-on. -ExerciseCombat
  still raises a shorter value to 180 and -ExerciseMovement raises a
  shorter value to 60. If the wait expires, the thrown error says
  "timeout" and prints the seconds actually used.

.PARAMETER ResourceRoot
  PlayDH root passed as the client --resource-root, as the ephemeral map
  server --resource-root (server root <ResourceRoot>\Resource\Server unless
  MXH_SERVER_RESOURCE_ROOT is set) and to -AuditMapDependencies /
  -AuditNpcDependencies. Empty reads MXH_PLAYDH_ROOT. When neither
  -ResourceRoot nor MXH_PLAYDH_ROOT is set, the script exits 1.
  A value must be an absolute path to an existing directory, for example
  the -PlayDhRoot given to clean-deploy. The script prints
  resource_root=<path> once.
#>
[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [int]$TimeoutSeconds = 90,
    [ValidateRange(0, 65535)]
    [int]$MapNumber = 10,
    [int]$MinimumNpcCount = 0,
    [switch]$FollowCamera,
    [switch]$AuditMapDependencies,
    [switch]$AuditNpcDependencies,
    [switch]$ExerciseInventory,
    [switch]$ExerciseSkills,
    [switch]$RotateAppearance,
    [switch]$ExerciseCombat,
    [switch]$ExerciseMovement,
    # Opt out of MXH_GUI_SMOKE_RENDER_ENTITIES. This script always sets
    # MXH_GUI_SMOKE_EXIT=1, so -NoEntities leaves the entity list empty.
    [switch]$NoEntities,
    # Loopback-only throwaway login/agent/map. Does not call start_modern.ps1.
    [switch]$EphemeralLocalServer,
    # Live CharacterMake. The saved map must equal the configured StartArea
    # (-StartArea when passed, otherwise -MapNumber, which the agent receives
    # as --default-map). Map17 and verify-map17-birth.py run only when that
    # value is 17. Cannot be combined with -MapNumber 10.
    [switch]$LiveCharacterBirth,
    [ValidateRange(0, 255)]
    [int]$StartArea = 0,
    # login=base, agent=base+1000, map=base+2000. Ignored unless -EphemeralLocalServer.
    # Default avoids 16001/17001/18001, 36001/37001/38001 and 36101/37101/38101-38103.
    [ValidateRange(1, 65533)]
    [int]$EphemeralPortBase = 26001,
    # PlayDH root for the client --resource-root, the ephemeral map server and
    # the optional audits. Empty reads MXH_PLAYDH_ROOT. Otherwise an
    # absolute path to an existing directory, e.g. a clean-deploy -PlayDhRoot.
    [string]$ResourceRoot = '',
    # Print resource_root= and exit 0 after the root is accepted.
    [switch]$ResolveOnly
)

$ErrorActionPreference = 'Stop'
$startAreaExplicit = $PSBoundParameters.ContainsKey('StartArea')
$configuredStartArea = if ($startAreaExplicit) { $StartArea } else { $MapNumber }
if ($LiveCharacterBirth -and $MapNumber -eq 10) {
    throw "-MapNumber 10 enters the pre-saved Map10 character. -LiveCharacterBirth checks the configured StartArea and cannot be combined with -MapNumber 10."
}
if ($LiveCharacterBirth -and $configuredStartArea -eq 17 -and $MapNumber -ne 17 -and -not $EphemeralLocalServer) {
    throw "start_modern.ps1 does not register Map17. Pass -EphemeralLocalServer with -LiveCharacterBirth -StartArea 17."
}
if ($EphemeralLocalServer) {
    # Start-Process -Environment requires Windows PowerShell's successor 7.4.
    # Checked before dot-sourcing so an older host fails with this message
    # and does not change the switch-off path.
    $ephemeralHostVersion = $PSVersionTable.PSVersion
    if ($null -eq $ephemeralHostVersion -or $ephemeralHostVersion.Major -lt 7 -or
        ($ephemeralHostVersion.Major -eq 7 -and $ephemeralHostVersion.Minor -lt 4)) {
        Write-Output "EphemeralLocalServer requires PowerShell 7.4 or newer (Start-Process -Environment). Found $ephemeralHostVersion."
        exit 1
    }
    # First line of script output. Also written to ephemeral.log below.
    Write-Output 'server=local-ephemeral'
}
. (Join-Path $PSScriptRoot 'gui-client-smoke-ephemeral.ps1')
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
# PowerShell names are case-insensitive: $ResourceRoot is the parameter, so
# the resolved value lives in $smokeResourceRoot.
$smokeResourceRoot = Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $ResourceRoot
Write-Output "resource_root=$smokeResourceRoot"
if ($ResolveOnly) { exit 0 }
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$clientExe = Join-Path $buildRoot 'tools\MoxianClient\mxh_client.exe'
$clientWorkingDir = Join-Path $buildRoot 'tools\MoxianClient'
if (-not (Test-Path -LiteralPath $clientExe -PathType Leaf)) {
    $clientExe = Join-Path $buildRoot ("tools\MoxianClient\$Configuration\mxh_client.exe")
    $clientWorkingDir = Split-Path -Parent $clientExe
}
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
if (-not (Test-Path -LiteralPath $clientExe)) { throw "Missing GUI client: $clientExe" }
. (Join-Path $PSScriptRoot 'protocol_manifest_gate.ps1')

$runId = [Guid]::NewGuid().ToString('N')
$runRoot = Join-Path $buildRoot "runtime\gui-smoke\$runId"
$logDir = Join-Path $runRoot 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$ephemeralLog = $null
$ephemeralServerPids = [System.Collections.Generic.List[int]]::new()
$ephemeralDbDirectory = $null
$ephemeralSpec = $null
$ephemeralCleanupFailed = $false
$ephemeralCleanupLine = $null
$ephemeralPreviousDatabaseConfig = $null
$ephemeralDatabaseConfigTouched = $false
if ($EphemeralLocalServer) {
    $ephemeralLog = Join-Path $logDir 'ephemeral.log'
    Set-Content -LiteralPath $ephemeralLog -Value 'server=local-ephemeral' -Encoding utf8NoBOM
}
$stdout = Join-Path $logDir 'client.out.log'
$stderr = Join-Path $logDir 'client.err.log'
$characterName = 'GUI' + $runId.Substring(0, 10)
$frame = Join-Path $runRoot "map${MapNumber}.tga"
$dataRoot = Join-Path $runRoot 'data'
$client = $null
$effectiveTimeoutSeconds = if ($ExerciseCombat -and $TimeoutSeconds -lt 180) {
    # Full original-resource DX11 rendering can take hundreds of milliseconds
    # per frame.  The combat gate may need several real kills because the
    # MonsterList drop profile legitimately awards no item on many deaths.
    180
} else {
    if ($ExerciseMovement -and $TimeoutSeconds -lt 60) { 60 } else { $TimeoutSeconds }
}
$previousGuiSmokePassword = $env:MXH_GUI_SMOKE_PASSWORD
$previousGuiSmokeExit = $env:MXH_GUI_SMOKE_EXIT
$previousGuiSmokeRenderEntities = $env:MXH_GUI_SMOKE_RENDER_ENTITIES
$previousGuiSmokeOpenInventory = $env:MXH_GUI_SMOKE_OPEN_INVENTORY
$previousGuiSmokeSkills = $env:MXH_GUI_SMOKE_SKILLS
$previousGuiSmokeRotateAppearance = $env:MXH_GUI_SMOKE_ROTATE_APPEARANCE
$previousGuiSmokeCombat = $env:MXH_GUI_SMOKE_COMBAT
$previousGuiSmokeMovement = $env:MXH_GUI_SMOKE_MOVEMENT

try {
    $clientLoginPort = '16001'
    $clientMapPort = '18001'
    $ephemeralPortPlan = $null
    if ($EphemeralLocalServer) {
        $ephemeralPortPlan = Resolve-EphemeralPortPlan -Base $EphemeralPortBase
        $clientLoginPort = [string]$ephemeralPortPlan.LoginPort
        $clientMapPort = [string]$ephemeralPortPlan.MapPort
    }
    $arguments = @(
        '--login-host', '127.0.0.1',
        '--login-port', $clientLoginPort,
        '--map-port', $clientMapPort,
        '--username', 'test',
        '--password-env', 'MXH_GUI_SMOKE_PASSWORD',
        '--auto-login',
        '--auto-create',
        '--character-name', $characterName,
        '--save-frame', $frame,
        '--state-frames-dir', (Join-Path $runRoot 'state-frames'),
        '--smoke-settle-frames', '20',
        '--exit-after-gamein',
        '--resource-root', $smokeResourceRoot
    )
    if ($FollowCamera) { $arguments += '--follow-camera' }
    if ($EphemeralLocalServer) {
        # Today's parser ignores this flag. LoginAck carries the agent
        # endpoint; the flag is here for a build that starts honoring it.
        $arguments += @('--agent-port', [string]$ephemeralPortPlan.AgentPort)
    }
    if ($EphemeralLocalServer) {
        # Safety gate before any process, including the optional python audit.
        $layout = Get-EphemeralResourceLayout -RepoRoot $repoRoot -ResourceRoot $smokeResourceRoot
        $mapExe = Resolve-EphemeralServerExecutable -BuildRoot $buildRoot -Configuration $Configuration -ToolDirectory 'MoxianMapServer' -FileName 'mxh_map_server_CHINA.exe'
        Assert-MxhItemProtocolMatch -MapExecutable $mapExe -ClientExecutable $clientExe
        $agentExe = Resolve-EphemeralServerExecutable -BuildRoot $buildRoot -Configuration $Configuration -ToolDirectory 'MoxianAgentServer' -FileName 'mxh_agent_server_CHINA.exe'
        $loginExe = Resolve-EphemeralServerExecutable -BuildRoot $buildRoot -Configuration $Configuration -ToolDirectory 'MoxianLoginServer' -FileName 'mxh_login_server.exe'
        $dbTool = Join-Path $buildRoot 'tools\MoxianDbTool\mxh_db_tool.exe'
        if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
            $dbTool = Join-Path $buildRoot ("tools\MoxianDbTool\$Configuration\mxh_db_tool.exe")
        }
        if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
            throw "Missing database tool required for isolated GUI smoke: $dbTool"
        }
        $plannedDbDirectory = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
        $registerMap17 = $LiveCharacterBirth -and $configuredStartArea -eq 17
        $ephemeralSpec = New-EphemeralServerLaunchSpec `
            -MapExecutable $mapExe -AgentExecutable $agentExe -LoginExecutable $loginExe `
            -MapNumber $MapNumber `
            -ResourceRoot $layout.ResourceRoot -ServerResourceRoot $layout.ServerResourceRoot `
            -ResourceProfileId $layout.ResourceProfileId `
            -DatabaseDirectory $plannedDbDirectory `
            -LoginPort $ephemeralPortPlan.LoginPort -AgentPort $ephemeralPortPlan.AgentPort -MapPort $ephemeralPortPlan.MapPort `
            -RegisterMap17:$registerMap17 `
            -ProcessEnvironment (Copy-EphemeralProcessEnvironment) `
            -ClientArguments $arguments
        # Before any server process. A listener on 0.0.0.0 would still accept 127.0.0.1.
        $boundPorts = @($ephemeralSpec.Processes | ForEach-Object { [int]$_.Port })
        $map17Port = @($ephemeralSpec.Processes | Where-Object { $_.Name -eq 'map17' } | ForEach-Object { [int]$_.Port })
        if ($map17Port.Count -eq 1) {
            $map17Busy = Get-EphemeralPortInUseViolation -Ports @([int]$map17Port[0])
            if ($map17Busy) {
                throw "Map17 listen port $($map17Port[0]) is occupied; refusing to fall back to another port. $map17Busy"
            }
        }
        $portInUse = Get-EphemeralPortInUseViolation -Ports $boundPorts
        if ($portInUse) { throw $portInUse }
    }
    if ($AuditMapDependencies) {
        $dependencyAudit = Join-Path $repoRoot 'modern\tools\audit_map_dependencies.py'
        $dependencyOutput = Join-Path $runRoot 'map-dependencies.txt'
        & python $dependencyAudit $MapNumber $smokeResourceRoot `
            --build-dir $buildRoot --output $dependencyOutput
        if ($LASTEXITCODE -ne 0) {
            throw "GUI smoke static map dependency gate failed for Map $MapNumber; report=$dependencyOutput"
        }
    }
    if ($EphemeralLocalServer) {
        New-Item -ItemType Directory -Force -Path $ephemeralSpec.DatabaseDirectory | Out-Null
        $ephemeralDbDirectory = $ephemeralSpec.DatabaseDirectory
        if ($ephemeralLog) {
            foreach ($item in @($ephemeralSpec.Processes)) {
                Add-Content -LiteralPath $ephemeralLog -Value "$($item.Name): $($item.Executable) $($item.Arguments -join ' ')" -Encoding utf8
            }
        }
        $ephemeralPreviousDatabaseConfig = [Environment]::GetEnvironmentVariable('MXH_DATABASE_CONFIG', 'Process')
        [Environment]::SetEnvironmentVariable('MXH_DATABASE_CONFIG', $ephemeralSpec.DatabaseConfig, 'Process')
        $ephemeralDatabaseConfigTouched = $true
        & $dbTool migrate --db-env MXH_DATABASE_CONFIG
        if ($LASTEXITCODE -ne 0) { throw "ephemeral database migration failed with exit code $LASTEXITCODE" }
        $ephemeralStarted = @()
        foreach ($item in @($ephemeralSpec.Processes)) {
            $serverStdout = Join-Path $logDir "$($item.Name).out.log"
            $serverStderr = Join-Path $logDir "$($item.Name).err.log"
            $procEnv = Copy-EphemeralEnvironmentForProcess -Source $ephemeralSpec.Environment -ProcessName $item.Name
            $serverProcess = Start-Process -FilePath $item.Executable -ArgumentList @($item.Arguments) `
                -WorkingDirectory (Split-Path -Parent $item.Executable) `
                -RedirectStandardOutput $serverStdout -RedirectStandardError $serverStderr `
                -PassThru -WindowStyle Hidden -Environment $procEnv
            [void]$ephemeralServerPids.Add([int]$serverProcess.Id)
            $ephemeralStarted += [pscustomobject]@{
                Name = $item.Name; Process = $serverProcess; Port = [int]$item.Port; Stderr = $serverStderr
            }
            Start-Sleep -Milliseconds 500
            $serverProcess.Refresh()
            if ($serverProcess.HasExited) {
                throw "$($item.Name) exited during startup (exit=$($serverProcess.ExitCode); see $serverStderr)"
            }
        }
        Start-Sleep -Seconds 1
        $readinessFailure = Get-EphemeralReadinessFailure -Servers $ephemeralStarted
        if ($readinessFailure) { throw $readinessFailure }
        $dbConfig = $ephemeralSpec.RegisterDatabaseArgument
    } else {
        $gateTools = Join-Path $buildRoot 'tools'
        $gateMap = Resolve-MxhBuiltExecutable -ToolsRoot $gateTools -ToolDirectory 'MoxianMapServer' -FileName 'mxh_map_server_CHINA.exe' -Config $Configuration
        Assert-MxhItemProtocolMatch -MapExecutable $gateMap -ClientExecutable $clientExe
        & $serverScript -Mode start -Backend sqlite -DataDir $dataRoot -MapNumber $MapNumber
        $dbTool = Join-Path $buildRoot 'tools\MoxianDbTool\mxh_db_tool.exe'
        if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
            $dbTool = Join-Path $buildRoot ("tools\MoxianDbTool\$Configuration\mxh_db_tool.exe")
        }
        if (-not (Test-Path -LiteralPath $dbTool -PathType Leaf)) {
            throw "Missing database tool required for isolated GUI smoke: $dbTool"
        }
        $dbConfig = 'sqlite;path=' + (Join-Path $dataRoot 'moxian.db')
    }
    # PowerShell's native stdin pipeline can encode the string as UTF-16 on
    # some hosts, which makes the DB tool receive embedded NULs and reject an
    # otherwise valid password.  Use an explicit ASCII fixture and redirect
    # stdin so the account bootstrap is byte-stable across shells.
    $registerInput = Join-Path $runRoot 'register.stdin'
    if ([string]::IsNullOrWhiteSpace($env:MXH_SMOKE_PASSWORD)) {
        Write-Error "MXH_SMOKE_PASSWORD is unset or empty" -ErrorAction Continue
        exit 1
    }
    [System.IO.File]::WriteAllText(
        $registerInput, ($env:MXH_SMOKE_PASSWORD + "`n"),
        [System.Text.Encoding]::ASCII)
    $register = Start-Process -FilePath $dbTool `
        -ArgumentList @('register', '--db', $dbConfig, 'test') `
        -RedirectStandardInput $registerInput -NoNewWindow -Wait -PassThru
    if ($register.ExitCode -ne 0) { throw "GUI smoke test account registration failed" }
    Remove-Item -LiteralPath $registerInput -Force -ErrorAction SilentlyContinue
    if (-not $LiveCharacterBirth) {
        # Deterministic MapNumber enter. The client --auto-create path selects
        # this row and must not submit CharacterMakeSyn. userid 1 is the first
        # modern_account_identity row for account 'test'.
        $savedMapSql = @(
            "INSERT INTO modern_account_identity(account_id, user_idx) VALUES('test', 1)",
            "INSERT INTO character_info(chrid,charname,userid,sex_type,hair_type,face_type,body_type,start_area,height,width,level,map_num,standing_idx,base_gengol,base_minchub,base_cheryuk,base_simmek) VALUES(100000,'MapSmoke','1',0,1,1,0,$MapNumber,1.0,1.0,1,$MapNumber,0,12,12,12,12)",
            "INSERT INTO modern_player_state(player_id,money,level,exp,max_level,exp_flag,level_up_point,hp_mp_flag,updated_at) VALUES(100000,0,1,0,1,16,0,0,CURRENT_TIMESTAMP)",
            "INSERT INTO modern_player_vitals(player_id,hp,shield,mp,dead,initialized,revision,updated_at) VALUES(100000,125,0,20,0,1,1,CURRENT_TIMESTAMP)"
        )
        foreach ($statement in $savedMapSql) {
            & $dbTool exec --db $dbConfig $statement
            if ($LASTEXITCODE -ne 0) { throw "GUI smoke failed to save the Map $MapNumber character" }
        }
    }
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
    if ($RotateAppearance) {
        $equipmentSchemaSql = "CREATE TABLE IF NOT EXISTS modern_character_equipment(chrid INTEGER NOT NULL,slot INTEGER NOT NULL,item_idx INTEGER NOT NULL,updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY(chrid,slot))"
        & $dbTool exec --db $dbConfig $equipmentSchemaSql
        if ($LASTEXITCODE -ne 0) { throw "GUI smoke equipment schema setup failed" }
    }
    # Copy the process environment after the smoke variables are set, then
    # hand that copy to the client. MXH_FOG_DISABLE (the only fog switch,
    # read by getenv in the client) is included when the caller set it.
    # PowerShell 7.4+ Start-Process -Environment replaces the block, so the
    # copy must be the full environment. Older hosts keep inheritance.
    if ([string]::IsNullOrWhiteSpace($env:MXH_SMOKE_PASSWORD)) {
        Write-Error "MXH_SMOKE_PASSWORD is unset or empty" -ErrorAction Continue
        exit 1
    }
    $env:MXH_GUI_SMOKE_PASSWORD = $env:MXH_SMOKE_PASSWORD
    $env:MXH_GUI_SMOKE_EXIT = '1'
    if ($ExerciseInventory) { $env:MXH_GUI_SMOKE_OPEN_INVENTORY = '1' }
    if ($ExerciseSkills) { $env:MXH_GUI_SMOKE_SKILLS = '1' }
    if ($RotateAppearance) { $env:MXH_GUI_SMOKE_ROTATE_APPEARANCE = '1' }
    if ($ExerciseCombat) { $env:MXH_GUI_SMOKE_COMBAT = '1' }
    if ($ExerciseMovement) { $env:MXH_GUI_SMOKE_MOVEMENT = '1' }
    # EXIT=1 skips the entity list unless RENDER_ENTITIES=1. Default on
    # even without -FollowCamera; -NoEntities is the terrain-only opt-out.
    if ($NoEntities) {
        Remove-Item Env:MXH_GUI_SMOKE_RENDER_ENTITIES -ErrorAction SilentlyContinue
    } else {
        $env:MXH_GUI_SMOKE_RENDER_ENTITIES = '1'
    }
    $entitiesEnabled = -not $NoEntities
    if ($NoEntities) {
        Write-Warning "Entity rendering disabled (-NoEntities). Map screenshots omit monsters, NPCs and the player."
    }
    $captureMeta = [ordered]@{
        in_game_map = $true
        map_number = $MapNumber
        frame = "map$MapNumber.tga"
        entities_enabled = [bool]$entitiesEnabled
        entities_opt_out_requested = [bool]$NoEntities
        mxh_gui_smoke_exit = '1'
        mxh_gui_smoke_render_entities = $(if ($entitiesEnabled) { '1' } else { '' })
        mxh_fog_disable = $(if ([string]::IsNullOrEmpty($env:MXH_FOG_DISABLE)) { '' } else { [string]$env:MXH_FOG_DISABLE })
    }
    $captureMeta | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $runRoot 'capture-metadata.json') -Encoding utf8
    Write-Host "entities_enabled=$entitiesEnabled"
    $clientEnvironment = Copy-EphemeralProcessEnvironment
    $clientHostVersion = $PSVersionTable.PSVersion
    $passClientEnvironment = $null -ne $clientHostVersion -and (
        $clientHostVersion.Major -gt 7 -or
        ($clientHostVersion.Major -eq 7 -and $clientHostVersion.Minor -ge 4))
    if ($passClientEnvironment) {
        $client = Start-Process -FilePath $clientExe -ArgumentList $arguments `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru `
            -WorkingDirectory $clientWorkingDir -Environment $clientEnvironment
    } else {
        $client = Start-Process -FilePath $clientExe -ArgumentList $arguments `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru `
            -WorkingDirectory $clientWorkingDir
    }
    if (-not $client.WaitForExit($effectiveTimeoutSeconds * 1000)) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
        throw "GUI client timeout: process still running after ${effectiveTimeoutSeconds}s; log=$stderr"
    }
    $client.WaitForExit()
    $client.Refresh()
    $exitCode = $client.ExitCode
    if ($null -ne $exitCode -and $exitCode -ne 0) {
        $earlyLog = Get-Content -LiteralPath $stderr -Raw -ErrorAction SilentlyContinue
        if ($earlyLog -match 'release visual gate failed: (stmVclMismatch|stmVclMissing|stmVclBufferMismatch|stmVclShaderFailed)') {
            throw "GUI smoke visual gate failed for Map $MapNumber ($($Matches[1]), exit=$exitCode); log=$stderr"
        }
        throw "GUI client exited with code $exitCode; log=$stderr"
    }
    $log = Get-Content -LiteralPath $stderr -Raw
    # MXH_FOG_DISABLE=1 skips EnableFog and logs "FOG diag bypass".
    # The BMHM load line "map fog enabled ... (from BMHM)" is printed either way.
    $fogBypassActive = Test-GuiSmokeFogBypassActive $log
    if ($env:MXH_FOG_DISABLE -eq '1') {
        if (-not $fogBypassActive) {
            throw "GUI smoke MXH_FOG_DISABLE=1 did not reach the client (missing FOG diag bypass); log=$stderr"
        }
    } elseif ($fogBypassActive) {
        throw "GUI smoke fog is on but the client logged FOG diag bypass (MXH_FOG_DISABLE=1); log=$stderr"
    }
    if ($AuditNpcDependencies) {
        $explorer = Join-Path $buildRoot 'tools\MoxianResourceExplorer\mxh_explorer.exe'
        if (-not (Test-Path -LiteralPath $explorer -PathType Leaf)) {
            throw "Missing resource explorer required for NPC dependency gate: $explorer"
        }
        $catalogLines = & $explorer npc-chx (Join-Path $smokeResourceRoot 'Resource\Client\NpcChxList.bin')
        $catalog = @{}
        foreach ($line in $catalogLines) {
            if ($line -match '^([0-9]+)\s+(.+)$') { $catalog[[int]$Matches[1]] = $Matches[2].Trim() }
        }
        $packLines = & $explorer list (Join-Path $smokeResourceRoot 'npc.pak')
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
    # Fatal log markers: a hit fails the smoke even when the client exits 0.
    $fatalLogMarkers = @(
        'ERROR [map] obfuscated .map not supported',
        'ERROR [map] malformed .map'
    )
    foreach ($marker in $fatalLogMarkers) {
        if ($log -match [regex]::Escape($marker)) {
            throw "GUI smoke hit fatal log marker '$marker'; log=$stderr"
        }
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
    if ($log -notmatch 'CharacterSelectAck chrid=\d+ map_num=(\d+)') {
        throw "GUI smoke missing CharacterSelectAck saved map; log=$stderr"
    }
    $savedMap = [int]$Matches[1]
    if ($LiveCharacterBirth) {
        if ($log -notmatch [regex]::Escape('sent CharacterMakeSyn')) {
            throw "LiveCharacterBirth did not create a character; log=$stderr"
        }
        if ($savedMap -ne $configuredStartArea) {
            throw "LiveCharacterBirth entered map $savedMap; expected configured StartArea $configuredStartArea; log=$stderr"
        }
    } else {
        if ($log -match [regex]::Escape('sent CharacterMakeSyn')) {
            throw "GUI smoke -MapNumber $MapNumber must enter the pre-saved character and must not live-create; log=$stderr"
        }
        if ($log -notmatch 'auto-create selected existing chrid=') {
            throw "GUI smoke did not select the pre-saved Map $MapNumber character; log=$stderr"
        }
        if ($savedMap -ne $MapNumber) {
            throw "GUI smoke entered map $savedMap; -MapNumber $MapNumber requires that saved map; log=$stderr"
        }
    }
    if ($ExerciseInventory -and $log -notmatch 'GUI_SMOKE_INVENTORY_OPEN') {
        throw "GUI smoke inventory exercise did not open the live inventory UI; log=$stderr"
    }
    if ($ExerciseSkills -and $log -notmatch 'GUI_SMOKE_SKILLS=10') {
        throw "GUI smoke skill exercise did not receive persisted skill 10; log=$stderr"
    }
    if ($RotateAppearance -and $log -notmatch 'GUI_SMOKE_APPEARANCE_ROTATED') {
        throw "GUI smoke appearance rotation did not reach CharMake; log=$stderr"
    }
    if ($RotateAppearance -and $log -notmatch "GameInAck .*gender=1 face=0 hair=0 equip2=23010") {
        throw "GUI smoke created appearance was not persisted into GameInAck; log=$stderr"
    }
    if ($ExerciseCombat) {
        foreach ($marker in @('GUI_SMOKE_COMBAT_TARGET_DEAD', 'GUI_SMOKE_COMBAT_DROP_SEEN', 'GUI_SMOKE_COMBAT_PICKUP_CONFIRMED')) {
            if ($log -notmatch [regex]::Escape($marker)) {
                throw "GUI smoke combat exercise missing marker '$marker'; log=$stderr"
            }
        }
    }
    if ($ExerciseMovement -and $log -notmatch 'GUI_SMOKE_MOVEMENT_CONFIRMED') {
        throw "GUI smoke movement exercise did not receive an authoritative position change; log=$stderr"
    }
    if ($log -match 'release visual gate failed: (stmVclMismatch|stmVclMissing|stmVclBufferMismatch|stmVclShaderFailed)') {
        throw "GUI smoke visual gate failed for Map $MapNumber ($($Matches[1]): .vcl missing/mismatched, colour stream not built, or vcl shader unavailable); log=$stderr"
    }
    if ($log -notmatch "GameInAck .* map=$savedMap(?:\D|$)") {
        throw "GUI smoke GameInAck map does not match the character's saved map $savedMap; log=$stderr"
    }
    if ($log -match 'release visual gate failed: placeholderCount=(\d+) failedModelCount=(\d+)') {
        throw "GUI smoke visual gate failed for Map $MapNumber (placeholders=$($Matches[1]) failedModels=$($Matches[2])); log=$stderr"
    }
    if ($log -match 'release visual gate failed: entitySceneThrow count=(\d+)') {
        throw "GUI smoke entity scene threw $($Matches[1]) time(s) for Map $MapNumber; log=$stderr"
    }
    $npcCount = ([regex]::Matches($log, 'CInGameState: NpcAdd ')).Count
    if ($npcCount -lt $MinimumNpcCount) {
        throw "GUI smoke received $npcCount NPCs, expected at least $MinimumNpcCount; log=$stderr"
    }
    $monsterLog = [string]$log
    if ($EphemeralLocalServer) {
        $mapLogPath = Join-Path $logDir 'map.out.log'
    } else {
        $mapLogPath = Join-Path $repoRoot 'deploy\runtime\modern\logs\map.out.log'
    }
    if (Test-Path -LiteralPath $mapLogPath) {
        $monsterLog += "`n" + [string](Get-Content -LiteralPath $mapLogPath -Raw)
    }
    try {
        $monsterCount = Get-GuiSmokeMonsterCount -LogText $monsterLog
    } catch {
        throw "$($_.Exception.Message); log=$stderr"
    }
    if (-not $LiveCharacterBirth -and $MapNumber -eq 10 -and $monsterCount -ne 228) {
        throw "GUI smoke received $monsterCount Map10 monsters, expected exactly 228; log=$stderr"
    }
    $entityTotals = @(Get-GuiSmokeEntityMonsterTotals -LogText $log)
    if (-not $LiveCharacterBirth -and $MapNumber -eq 10 -and $entityTotals.Count -gt 0) {
        $entityMonsterTotal = [int](($entityTotals | Measure-Object -Maximum).Maximum)
        if ($entityMonsterTotal -ne 228) {
            throw "GUI smoke entity frame monster total $entityMonsterTotal, expected exactly 228; log=$stderr"
        }
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
        if ($savedMap -eq 10 -and
            $log -notmatch '\[entity\] original model object=\d+ type=monster kind=\d+ chx=L\d+\.chx') {
            throw "GUI player-view smoke did not resolve a Map10 monster model; log=$stderr"
        }
        if ($savedMap -ne 10 -and
            $log -notmatch '\[entity\] original model object=\d+ type=npc kind=\d+ chx=N\d+\.chx') {
            throw "GUI player-view smoke did not resolve an NPC model for saved map $savedMap; log=$stderr"
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
    if ($LiveCharacterBirth -and $configuredStartArea -eq 17) {
        & python (Join-Path $repoRoot 'scripts\verify-map17-birth.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI Map17 birth frame validation failed: $frame" }
    } elseif (-not $FollowCamera) {
        $terrainArgs = @($frame)
        if ($ExerciseInventory) { $terrainArgs += '--allow-ui-overlay' }
        if ($MapNumber -eq 12) { $terrainArgs += '--allow-sparse' }
        & python (Join-Path $repoRoot 'scripts\verify-terrain-frame.py') @terrainArgs
        if ($LASTEXITCODE -ne 0) { throw "GUI terrain frame validation failed: $frame" }
    } else {
        & python (Join-Path $repoRoot 'scripts\verify-entity-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI entity frame validation failed: $frame" }
    }
    Write-Host "GUI_CLIENT_SMOKE PASS (map=$MapNumber, entities_enabled=$entitiesEnabled, monsters=$monsterCount, npcs=$npcCount, original BGM/create/select/game-in, evidence=$stderr, frame=$frame)" -ForegroundColor Green
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
    if ($null -eq $previousGuiSmokeRotateAppearance) {
        Remove-Item Env:MXH_GUI_SMOKE_ROTATE_APPEARANCE -ErrorAction SilentlyContinue
    } else { $env:MXH_GUI_SMOKE_ROTATE_APPEARANCE = $previousGuiSmokeRotateAppearance }
    if ($null -eq $previousGuiSmokeCombat) {
        Remove-Item Env:MXH_GUI_SMOKE_COMBAT -ErrorAction SilentlyContinue
    } else { $env:MXH_GUI_SMOKE_COMBAT = $previousGuiSmokeCombat }
    if ($null -eq $previousGuiSmokeMovement) {
        Remove-Item Env:MXH_GUI_SMOKE_MOVEMENT -ErrorAction SilentlyContinue
    } else { $env:MXH_GUI_SMOKE_MOVEMENT = $previousGuiSmokeMovement }
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
    if ($EphemeralLocalServer) {
        if ($ephemeralDatabaseConfigTouched) {
            if ($null -eq $ephemeralPreviousDatabaseConfig) {
                Remove-Item Env:MXH_DATABASE_CONFIG -ErrorAction SilentlyContinue
            } else {
                [Environment]::SetEnvironmentVariable('MXH_DATABASE_CONFIG', $ephemeralPreviousDatabaseConfig, 'Process')
            }
        }
        $recordedPids = $ephemeralServerPids.ToArray()
        $ephemeralCleanup = Invoke-EphemeralLocalCleanup -ProcessIds $recordedPids `
            -DatabaseDirectory $ephemeralDbDirectory -LogFile $ephemeralLog
        if (-not $ephemeralCleanup.Ok) {
            $ephemeralCleanupFailed = $true
            $ephemeralCleanupLine = $ephemeralCleanup.Line
        }
    } else {
        & $serverScript -Mode stop
    }
}
if ($ephemeralCleanupFailed) {
    throw "ephemeral cleanup verification failed: $ephemeralCleanupLine"
}
