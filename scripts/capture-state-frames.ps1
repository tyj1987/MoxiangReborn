# scripts/capture-state-frames.ps1
#
# Phase 3 of human-acceptance: launch mxh_client.exe against the running
# modern server, drive it through the state machine, and capture TGA frames
# for each state. Mirrors gui-client-smoke.ps1 but:
#   - exits on first state frame OR after --TimeoutSeconds, whichever first
#   - writes frames to a caller-supplied -FramesDir
#   - does NOT enforce release-gate markers; verification is a separate step
#
# Usage (typical human-acceptance run):
#   pwsh -File scripts\capture-state-frames.ps1 `
#        -AccountName ha_user01 -Password 'Test1234' `
#        -FramesDir modern\scratch\2026-08-31-acceptance\frames `
#        -MapNumber 10 -TimeoutSeconds 60
#
# Requires: server already running (start_modern.ps1 -Mode start) AND
#           the account already registered (register-test-account.ps1).

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$AccountName,

    [Parameter(Mandatory = $true)]
    [string]$Password,

    [Parameter(Mandatory = $true)]
    [string]$FramesDir,

    [int]$MapNumber = 10,

    [int]$LoginPort = 16001,

    [int]$MapPort = 18001,

    [string]$LoginHost = '127.0.0.1',

    [int]$Width = 800,

    [int]$Height = 600,

    [int]$TimeoutSeconds = 60,

    [string]$CharacterName = '',

    [string]$CharacterNamePrefix = 'HA',

    [string]$ClientExe = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ([string]::IsNullOrWhiteSpace($ClientExe)) {
    $ClientExe = Join-Path $repoRoot 'modern\build\tools\MoxianClient\mxh_client.exe'
}
if (-not (Test-Path -LiteralPath $ClientExe -PathType Leaf)) {
    throw "Missing mxh_client.exe: $ClientExe  (rebuild: scripts\build-modern.bat Debug mxh_client)"
}

if (-not (Test-Path -LiteralPath $FramesDir)) {
    New-Item -ItemType Directory -Force -Path $FramesDir | Out-Null
}

if ([string]::IsNullOrWhiteSpace($CharacterName)) {
    $CharacterName = $CharacterNamePrefix + '_' + (Get-Date -Format 'MMdd_HHmmss')
}

# Stash any prior env so we can restore in finally.
$prevPassword = $env:MXH_GUI_SMOKE_PASSWORD
$prevExit      = $env:MXH_GUI_SMOKE_EXIT
$prevFramesDir = $env:MXH_STATE_FRAMES_DIR

# The client reads the password from the env var named by --password-env.
$env:MXH_GUI_SMOKE_PASSWORD = $Password
$env:MXH_GUI_SMOKE_EXIT = '1'
$env:MXH_STATE_FRAMES_DIR = $FramesDir

$clientOutLog = Join-Path $FramesDir 'client.out.log'
$clientErrLog = Join-Path $FramesDir 'client.err.log'
$terrainFrame = Join-Path $FramesDir ("map${MapNumber}.tga")

$arguments = @(
    '--login-host', $LoginHost,
    '--login-port', "$LoginPort",
    '--map-port', "$MapPort",
    '--username', $AccountName,
    '--password-env', 'MXH_GUI_SMOKE_PASSWORD',
    '--auto-login',
    '--auto-create',
    '--character-name', $CharacterName,
    '--save-frame', $terrainFrame,
    '--state-frames-dir', $FramesDir,
    '--smoke-settle-frames', '20',
    '--exit-after-gamein',
    '--resource-root', (Join-Path $repoRoot 'modern\data\PlayDH'),
    '--width', "$Width",
    '--height', "$Height"
)

$client = $null
try {
    $client = Start-Process -FilePath $ClientExe -ArgumentList $arguments `
        -RedirectStandardOutput $clientOutLog `
        -RedirectStandardError $clientErrLog `
        -PassThru `
        -WorkingDirectory (Split-Path -Parent $ClientExe)

    if (-not $client.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
        throw "Client did not reach GameIn within ${TimeoutSeconds}s.  err=$clientErrLog"
    }
    $client.WaitForExit()
    $client.Refresh()
    $exitCode = $client.ExitCode

    $frames = @(Get-ChildItem -LiteralPath $FramesDir -Filter 'state-*.tga' -ErrorAction SilentlyContinue)
    $terrainOk = Test-Path -LiteralPath $terrainFrame

    if (($null -ne $exitCode -and $exitCode -ne 0) -or $frames.Count -eq 0 -or -not $terrainOk) {
        $summary = "exit=$exitCode state_frames=$($frames.Count) terrain_frame=$terrainOk"
        throw "Capture incomplete: $summary  err=$clientErrLog"
    }

    $frameNames = ($frames | ForEach-Object { $_.Name }) -join ','
    Write-Host "capture OK: $frameNames | map${MapNumber}.tga" -ForegroundColor Green
}
finally {
    if ($null -eq $prevPassword) { Remove-Item Env:MXH_GUI_SMOKE_PASSWORD -ErrorAction SilentlyContinue } else { $env:MXH_GUI_SMOKE_PASSWORD = $prevPassword }
    if ($null -eq $prevExit)      { Remove-Item Env:MXH_GUI_SMOKE_EXIT      -ErrorAction SilentlyContinue } else { $env:MXH_GUI_SMOKE_EXIT      = $prevExit }
    if ($null -eq $prevFramesDir) { Remove-Item Env:MXH_STATE_FRAMES_DIR    -ErrorAction SilentlyContinue } else { $env:MXH_STATE_FRAMES_DIR    = $prevFramesDir }
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
}
