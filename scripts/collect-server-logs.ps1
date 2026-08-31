# scripts/collect-server-logs.ps1
#
# Phase 4 of human-acceptance: copy the modern server log directory into a
# caller-supplied -OutDir, and emit a one-line marker summary so the human
# reviewer can confirm the server saw the expected protocol transitions
# (LoginAck, CharacterSelectAck, GameInAck, MonsterAdd, etc.).
#
# Usage:
#   pwsh -File scripts\collect-server-logs.ps1 `
#        -OutDir modern\scratch\2026-08-31-acceptance\server-logs `
#        -LogsDir deploy\runtime\modern\logs
#
# The default -LogsDir matches the start_modern.ps1 output path.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutDir,

    [string]$LogsDir = '',

    [string]$MapNumber = '10'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ([string]::IsNullOrWhiteSpace($LogsDir)) {
    $LogsDir = Join-Path $repoRoot 'deploy\runtime\modern\logs'
}

if (-not (Test-Path -LiteralPath $LogsDir)) {
    Write-Host "collect-logs WARN: source log dir missing: $LogsDir" -ForegroundColor Yellow
    return
}

if (-not (Test-Path -LiteralPath $OutDir)) {
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}

Copy-Item -Path (Join-Path $LogsDir '*') -Destination $OutDir -Recurse -Force

$summaryFile = Join-Path $OutDir 'marker-summary.txt'
$markers = @(
    @{ name = 'LoginAck';            pattern = 'LoginAck\b' },
    @{ name = 'CharacterSelectAck';  pattern = 'CharacterSelectAck\b' },
    @{ name = 'CharacterMakeSyn';    pattern = 'sent CharacterMakeSyn\b' },
    @{ name = 'GameInAck';           pattern = 'GameInAck\b' },
    @{ name = 'MonsterAdd';          pattern = 'MonsterAdd object_id=' },
    @{ name = 'NpcAdd';              pattern = 'NpcAdd id=' },
    @{ name = 'BgmPlayer.loaded';    pattern = 'playing original BGM id=' },
    @{ name = 'SfxPlayer.manifest';  pattern = 'SFX manifest ready' },
    @{ name = 'terrain.HFL';         pattern = '\[terrain\] original HFL loaded' },
    @{ name = 'static.STM';          pattern = '\[static\] original STM loaded' }
)

$mapErr = Join-Path $LogsDir 'map.err.log'
$agentErr = Join-Path $LogsDir 'agent.err.log'
$loginErr = Join-Path $LogsDir 'login.err.log'
$mapOut  = Join-Path $LogsDir 'map.out.log'
$combined = ''
foreach ($f in @($mapErr,$agentErr,$loginErr,$mapOut)) {
    if (Test-Path -LiteralPath $f) { $combined += (Get-Content -LiteralPath $f -Raw) + "`n" }
}

$lines = @('# server log marker summary', "# logs_dir = $LogsDir", "# captured_at = $(Get-Date -Format o)", "")
foreach ($m in $markers) {
    $count = ([regex]::Matches($combined, $m.pattern)).Count
    $line = "{0,-22} = {1}" -f $m.name, $count
    Write-Host $line
    $lines += $line
}
$mapMonsters = ([regex]::Matches($combined, 'CInGameState: MonsterAdd object_id=')).Count
$mapMonsterLine = "Map$MapNumber-monsters-received = $mapMonsters  (expected 228 for Map10)"
Write-Host $mapMonsterLine
$lines += $mapMonsterLine

$lines | Set-Content -LiteralPath $summaryFile -Encoding UTF8
Write-Host "collect-logs OK: $OutDir  (summary=$summaryFile)" -ForegroundColor Green
