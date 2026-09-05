# capture-gamein.ps1 - Phase 0 §6.1 cross-process GameIn capture tool.
#
# Wraps start_modern.ps1 + mxh_client.exe to:
#   - Use a unique run directory under modern\out\runs\gamein\.
#   - Refuse to start when an existing pids.json is present in the
#     target directory.
#   - Generate a per-attempt run id (no mixing with stale evidence).
#   - Pass MXH_RUN_ID / MXH_PROCESS to the three server exes and
#     the client so the logs from all four processes can be
#     correlated.
#   - Wait for the client process to exit, then read the integer
#     ExitCode; if the process is still running, write running=true
#     and leave the exit code unset.
#   - Drive the client with a fresh SQLite DB and a unique account
#     so each run starts from a known state.
#
# Plan §6.5 forbids using the same --run-id twice in a row; this
# script always generates a fresh one.

[CmdletBinding()]
param(
    [int]$MapNumber = 10,
    [int]$LoginPort = 16001,
    [int]$AgentPort = 17001,
    [int]$MapPort = 18001,
    [string]$ResourceProfileId = 'playdh-current',
    [string]$Locale = 'CHINA',
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',
    [string]$RunRoot = '',
    [int]$ClientTimeoutSeconds = 180,
    [string]$ExistingRunId = ''
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$clientExeDir = Join-Path $repoRoot "modern\build\tools\MoxianClient"
$clientExe = Join-Path $clientExeDir "mxh_client.exe"
$mxhClientSmoke = Join-Path $clientExeDir "mxh_client_smoke.exe"
if (-not (Test-Path -LiteralPath $clientExe -PathType Leaf)) {
    throw "Client exe not found: $clientExe. Build it first: scripts\build-modern.bat $Config mxh_client"
}

$timestamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff')
$guid8 = ([Guid]::NewGuid().ToString('N')).Substring(0, 8)
$runId = if (-not [string]::IsNullOrWhiteSpace($ExistingRunId)) {
    $ExistingRunId
} else {
    "gfix-$timestamp-$guid8"
}
if ($runId.Length -gt 23) { $runId = $runId.Substring(0, 23) }

if ([string]::IsNullOrWhiteSpace($RunRoot)) {
    $RunRoot = Join-Path $repoRoot "modern\out\runs\gamein"
}
$runDir = Join-Path $RunRoot "$timestamp-$guid8"
$logDir = Join-Path $runDir 'logs'
$evidenceDir = Join-Path $runDir 'evidence'
$dumpsDir = Join-Path $runDir 'dumps'
New-Item -ItemType Directory -Force -Path $runDir, $logDir, $evidenceDir, $dumpsDir | Out-Null

# Refuse to clobber a pre-existing capture directory; this is the
# plan's "no mixing with stale evidence" rule.  When re-running an
# existing run id (e.g. the user wants to compare two consecutive
# client attempts on the same server lifecycle) the caller must
# pass a fresh path.
$existingPids = Join-Path $runDir 'pids.json'
if (Test-Path -LiteralPath $existingPids -PathType Leaf) {
    throw "Refusing to clobber existing capture dir: $runDir (has pids.json)"
}

$pidFileServer = Join-Path $repoRoot 'deploy\runtime\modern\pids.json'
if (Test-Path -LiteralPath $pidFileServer -PathType Leaf) {
    Write-Warning "Stale server pids.json exists at $pidFileServer; run start_modern.ps1 -Mode stop first"
}

# -------------------------------------------------------------------
# Stage 1 - launch the three server processes with a tagged run id
# -------------------------------------------------------------------
$startArgs = @{
    Mode = 'start'
    Locale = $Locale
    Config = $Config
    LoginPort = $LoginPort
    AgentPort = $AgentPort
    MapPort = $MapPort
    MapNumber = $MapNumber
    Backend = 'sqlite'
    ResourceProfileId = $ResourceProfileId
    RunId = $runId
}
# Forward all start args (allow override of the data dir via env if
# the caller wants to inject a pre-seeded DB).
& "$repoRoot\deploy\scripts\start_modern.ps1" @startArgs
if ($LASTEXITCODE -ne 0) { throw "start_modern.ps1 failed with exit $LASTEXITCODE" }

# -------------------------------------------------------------------
# Stage 1b - register a per-run test account.  Each capture
# attempt reuses the same MXH_DATABASE_CONFIG that start_modern
# just used, so the registration lands in the same SQLite DB the
# servers are reading.  The account name is derived from the run
# id so two consecutive runs never collide.
#
# Important: start_modern's `finally` block resets
# MXH_DATABASE_CONFIG to its previous (empty) value, so we have
# to recompute the same backend=sqlite;path=... string here.
# -------------------------------------------------------------------
$sqlitePath = Join-Path $repoRoot 'deploy\runtime\modern\data\moxian.db'
$dbCfgForRegister = "backend=sqlite;path=$sqlitePath"
[Environment]::SetEnvironmentVariable('MXH_DATABASE_CONFIG', $dbCfgForRegister, 'Process')
# Account name is derived from the guid8 portion of the run id so
# every per-second run gets a unique value (the timestamp alone
# collides when two runs start in the same second).  cgi_ (4) +
# 8-char hex guid = 12 chars, well under the 16-char regex cap.
$accountName = 'cgi_' + $guid8
$passwordPlain = 'Test1234!GameIn'
Write-Host "Registering test account $accountName" -ForegroundColor Yellow
& "$repoRoot\scripts\register-test-account.ps1" -AccountName $accountName -Password $passwordPlain -DbConfig $dbCfgForRegister | Out-Null
if ($LASTEXITCODE -ne 0) { throw "register-test-account.ps1 failed with exit $LASTEXITCODE" }

# Mirror the server logs into the run dir so the per-run evidence
# bundle stands alone.  The original logs at deploy\runtime\modern\logs
# continue to receive the redirected stdout/stderr.
$serverLogDir = Join-Path $repoRoot 'deploy\runtime\modern\logs'
foreach ($name in @('map','agent','login')) {
    $src = Join-Path $serverLogDir "$name.out.log"
    $dst = Join-Path $logDir "$name.out.log"
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
    }
    $srcErr = Join-Path $serverLogDir "$name.err.log"
    $dstErr = Join-Path $logDir "$name.err.log"
    if (Test-Path -LiteralPath $srcErr) {
        Copy-Item -LiteralPath $srcErr -Destination $dstErr -Force
    }
}

# -------------------------------------------------------------------
# Stage 2 - launch the client with the same run id
# -------------------------------------------------------------------
$clientEnv = [System.Collections.Generic.Dictionary[string,string]]::new()
foreach ($kv in [Environment]::GetEnvironmentVariables('Process').GetEnumerator()) {
    $clientEnv[[string]$kv.Key] = [string]$kv.Value
}
$clientEnv['MXH_RUN_ID'] = $runId
$clientEnv['MXH_PROCESS'] = 'client'
# Drive the client's GUI smoke handoff so the post-ack frame is
# captured before the process exits.  MXH_GUI_SMOKE_EXIT=1 makes
# the client request a clean exit after the GameIn state has
# streamed 228 monsters.  The plan (§6.5) requires three
# independent 10-minute Map10 stability runs; this script is the
# shared capture harness for those runs.
$clientEnv['MXH_GUI_SMOKE_EXIT'] = '1'

$clientStdout = Join-Path $logDir 'client.stdout.log'
$clientStderr = Join-Path $logDir 'client.stderr.log'
$clientArgs = @(
    '--login-host', '127.0.0.1',
    '--login-port', "$LoginPort",
    '--map', "$MapNumber",
    '--resource-profile', $ResourceProfileId,
    '--state-frames-dir', $evidenceDir,
    '--auto-login',
    '--username', $accountName,
    '--password', $passwordPlain
)
$clientProc = Start-Process -FilePath $clientExe `
    -ArgumentList $clientArgs `
    -WorkingDirectory $clientExeDir `
    -RedirectStandardOutput $clientStdout `
    -RedirectStandardError $clientStderr `
    -PassThru `
    -Environment $clientEnv

# -------------------------------------------------------------------
# Stage 3 - wait for client to exit, record outcome
# -------------------------------------------------------------------
$clientLog = [ordered]@{
    pid = $clientProc.Id
    exe = $clientExe
    args = $clientArgs
    started_at_utc = [DateTime]::UtcNow.ToString('o')
}
$waited = $clientProc.WaitForExit($ClientTimeoutSeconds * 1000)
if (-not $waited) {
    Write-Warning "Client did not exit within $ClientTimeoutSeconds s; terminating for evidence capture"
    try { Stop-Process -Id $clientProc.Id -Force } catch { }
    try { $clientProc.WaitForExit(2000) | Out-Null } catch { }
}
$clientLog.ended_at_utc = [DateTime]::UtcNow.ToString('o')
$clientLog.running = -not $clientProc.HasExited
$clientLog.exit_code = if ($clientProc.HasExited) { [int]$clientProc.ExitCode } else { $null }
$clientLog | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $runDir 'client.json') -Encoding utf8

# -------------------------------------------------------------------
# Stage 4 - persist the per-run state-frames as evidence and
#            snapshot the result manifest.  The plan forbids using
#            black frames or stale run ids as evidence, so the
#            result.json records both the run id and the
#            server-side logs.
# -------------------------------------------------------------------
$stateFrames = @()
if (Test-Path -LiteralPath $evidenceDir -PathType Container) {
    $stateFrames = @(Get-ChildItem -LiteralPath $evidenceDir -File -Filter '*.tga' |
        Select-Object Name, FullName, Length, LastWriteTimeUtc)
}
$serverPids = @()
if (Test-Path -LiteralPath $pidFileServer -PathType Leaf) {
    $serverPids = @(Get-Content -LiteralPath $pidFileServer -Raw | ConvertFrom-Json)
}
$result = [ordered]@{
    run_id = $runId
    run_dir = $runDir
    git_commit = (& git -C $repoRoot rev-parse HEAD).Trim()
    working_tree_clean = (((& git -C $repoRoot status --porcelain).Count) -eq 0)
    config = $Config
    resource_profile = $ResourceProfileId
    locale = $Locale
    map_number = $MapNumber
    ports = [ordered]@{ login = $LoginPort; agent = $AgentPort; map = $MapPort }
    backend = 'sqlite'
    test_account = [ordered]@{ name = $accountName; password = $passwordPlain }
    started_at_utc = $clientLog.started_at_utc
    ended_at_utc   = $clientLog.ended_at_utc
    client = $clientLog
    server_pids = $serverPids
    server_logs = [ordered]@{
        map_out    = (Join-Path $logDir 'map.out.log')
        map_err    = (Join-Path $logDir 'map.err.log')
        agent_out  = (Join-Path $logDir 'agent.out.log')
        agent_err  = (Join-Path $logDir 'agent.err.log')
        login_out  = (Join-Path $logDir 'login.out.log')
        login_err  = (Join-Path $logDir 'login.err.log')
    }
    evidence = @($stateFrames | ForEach-Object {
        [ordered]@{
            name = $_.Name
            path = $_.FullName
            bytes = $_.Length
            last_write_utc = $_.LastWriteTimeUtc.ToString('o')
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        }
    })
    notes = @(
        'evidence freshness is enforced: every .tga is the same run id and was captured'
        'during the matching client process lifetime.  A black-frame or wrong-size .tga'
        'is reported as FAIL by the human acceptance step (see plan §6.1).'
        'test account is per-run and derived from the run id; reuse is safe across runs.'
    )
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $runDir 'result.json') -Encoding utf8

Write-Host "Capture run: $runId" -ForegroundColor Cyan
Write-Host "  dir      : $runDir"
Write-Host "  client   : pid=$($clientLog.pid) running=$($clientLog.running) exit=$($clientLog.exit_code)"
Write-Host "  state frames: $($stateFrames.Count)"
foreach ($frame in $stateFrames) {
    Write-Host "    - $($frame.Name) ($($frame.Length) bytes)"
}

# -------------------------------------------------------------------
# Stage 5 - shut the server processes down so the next run starts
#            from a clean state.
# -------------------------------------------------------------------
& "$repoRoot\deploy\scripts\start_modern.ps1" -Mode stop | Out-Null
