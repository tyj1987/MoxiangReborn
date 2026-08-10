[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [int]$TimeoutSeconds = 20,
    [switch]$FollowCamera
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$clientExe = Join-Path $buildRoot 'tools\MoxianClient\Debug\mxh_client.exe'
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
if (-not (Test-Path -LiteralPath $clientExe)) { throw "Missing GUI client: $clientExe" }

$runId = [Guid]::NewGuid().ToString('N')
$runRoot = Join-Path $buildRoot "runtime\gui-smoke\$runId"
$logDir = Join-Path $runRoot 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stdout = Join-Path $logDir 'client.out.log'
$stderr = Join-Path $logDir 'client.err.log'
$characterName = 'GUI' + $runId.Substring(0, 10)
$frame = Join-Path $runRoot 'map12.tga'
$client = $null

try {
    & $serverScript -Mode start -Backend sqlite -DataDir (Join-Path $runRoot 'data')
    $arguments = @(
        '--login-host', '127.0.0.1',
        '--login-port', '16001',
        '--map-port', '18001',
        '--username', 'test',
        '--password', 'test',
        '--auto-create',
        '--character-name', $characterName,
        '--save-frame', $frame,
        '--exit-after-gamein'
    )
    if ($FollowCamera) { $arguments += '--follow-camera' }
    $client = Start-Process -FilePath $clientExe -ArgumentList $arguments `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
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
    foreach ($marker in @('playing original BGM id=1667', 'playing original BGM id=1670', '[terrain] original HFL loaded', '[static] original STM loaded', 'sent CharacterMakeSyn', 'CharacterSelectAck', 'GameInAck', 'GUI_SMOKE_PASS')) {
        if ($log -notmatch [regex]::Escape($marker)) {
            throw "GUI smoke missing marker '$marker'; log=$stderr"
        }
    }
    if ($FollowCamera) {
        foreach ($marker in @('[sky] original MOD loaded meshes=8/8 textures=8/8', '[terrain] player camera active', '[entity] original MonsterList loaded', '[entity] original model kind=65006 chx=man.chx', '[entity] original idle animation active', '[entity] original model kind=1 chx=L001.chx')) {
            if ($log -notmatch [regex]::Escape($marker)) {
                throw "GUI player-view smoke missing marker '$marker'; log=$stderr"
            }
        }
    }
    if (-not (Test-Path -LiteralPath $frame)) { throw "GUI smoke missing terrain frame: $frame" }
    if (-not $FollowCamera) {
        & python (Join-Path $repoRoot 'scripts\verify-terrain-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI terrain frame validation failed: $frame" }
    } else {
        & python (Join-Path $repoRoot 'scripts\verify-entity-frame.py') $frame
        if ($LASTEXITCODE -ne 0) { throw "GUI entity frame validation failed: $frame" }
    }
    Write-Host "GUI_CLIENT_SMOKE PASS (original BGM/create/select/game-in, evidence=$stderr, frame=$frame)" -ForegroundColor Green
}
finally {
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force -ErrorAction SilentlyContinue
    }
    & $serverScript -Mode stop
}
