# Log-shape checks for Map10 monster count and the fog bypass.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-gui-client-smoke-monster-log.ps1
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
. (Join-Path $repoRoot 'scripts\gui-client-smoke-ephemeral.ps1')
$main = Join-Path $repoRoot 'scripts\gui-client-smoke.ps1'
$fixturePath = Join-Path $PSScriptRoot 'fixtures\gui-smoke-map10-monsters.log'

function Assert-True($condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
}

$fixture = [IO.File]::ReadAllText($fixturePath)
Assert-True ($fixture.Contains('CInGameState: MonsterAdd count=64')) 'fixture keeps the 64 milestone'
Assert-True ($fixture.Contains('CInGameState: MonsterAdd count=128')) 'fixture keeps the 128 milestone'
Assert-True ($fixture.Contains('CInGameState: MonsterAdd count=192')) 'fixture keeps the 192 milestone'
Assert-True ($fixture.Contains('[Map] sent 228 monster adds to player=100000')) 'fixture keeps the map send line'
Assert-True ($fixture.Contains('monster=81/228/147/0')) 'fixture keeps the entity monster field'
Assert-True (-not $fixture.Contains('CInGameState: MonsterAdd object_id=')) 'fixture must not use the old object_id string'

$count = Get-GuiSmokeMonsterCount -LogText $fixture
Assert-True ($count -eq 228) "combined log counted $count, expected 228"

$clientOnly = ($fixture -split "`n" | Where-Object { $_ -notmatch '\[Map\] sent ' }) -join "`n"
$milestoneOnly = Get-GuiSmokeMonsterCount -LogText $clientOnly
Assert-True ($milestoneOnly -eq 192) "client milestones alone are the last cumulative value, got $milestoneOnly"

$entityTotals = @(Get-GuiSmokeEntityMonsterTotals -LogText $fixture)
Assert-True ($entityTotals.Count -eq 1 -and $entityTotals[0] -eq 228) "entity total was $($entityTotals -join ',')"
$withoutEntity = ($fixture -split "`n" | Where-Object { $_ -notmatch '\[entity\] frame ' }) -join "`n"
Assert-True (@(Get-GuiSmokeEntityMonsterTotals -LogText $withoutEntity).Count -eq 0) 'mainline log without [entity] has no second assertion'

$oldStyle = "CInGameState: MonsterAdd object_id=1`nCInGameState: MonsterAdd object_id=2"
Assert-True ((Get-GuiSmokeMonsterCount -LogText $oldStyle) -eq 0) 'old object_id lines are not a count'

$inconsistent = $false
try {
    Get-GuiSmokeMonsterCount -LogText "CInGameState: MonsterAdd count=64`n[Map] sent 228 monster adds" | Out-Null
} catch {
    $inconsistent = $_.Exception.Message -like '*64-boundary 192*'
}
Assert-True $inconsistent 'a client that stopped at count=64 cannot claim 228 sent'

$mainText = [IO.File]::ReadAllText($main)
Assert-True (-not $mainText.Contains('CInGameState: MonsterAdd object_id=')) 'smoke script still matches object_id='
Assert-True ($mainText.Contains('Get-GuiSmokeMonsterCount -LogText $monsterLog')) 'smoke script must count the combined log'
Assert-True ($mainText.Contains('Get-GuiSmokeEntityMonsterTotals -LogText $log')) 'entity total is a second assertion'
Assert-True ($mainText.Contains('Test-GuiSmokeFogBypassActive $log')) 'fog rounds must inspect the bypass line'
Assert-True ($mainText.Contains('GUI smoke fog is on but the client logged FOG diag bypass (MXH_FOG_DISABLE=1)')) 'fog-on round must fail if the bypass line appears'
Assert-True ($mainText.Contains('-Environment $clientEnvironment')) 'client process must receive the environment copy'
Assert-True ($mainText.Contains('map.out.log')) 'map send line is read from map.out.log'

$fogOff = @'
mxh_client: map fog enabled start=1.0 end=30000.0 density=1.00 color=0xff000000 (from BMHM)
mxh_client: FOG diag bypass (MXH_FOG_DISABLE=1); skipping EnableFog (BMHM band start=1.0 end=30000.0 density=1.00 color=0xff000000 would have been applied)
'@
$fogOn = @'
mxh_client: map fog enabled start=1.0 end=30000.0 density=1.00 color=0xff000000 (from BMHM)
mxh_client: staged GameLoading complete map=10
'@
Assert-True (Test-GuiSmokeFogBypassActive $fogOff) 'fog-off log must contain the bypass line'
Assert-True (Test-GuiSmokeFogBypassActive $fixture) 'fixture bypass line must count as fog off'
Assert-True (-not (Test-GuiSmokeFogBypassActive $fogOn)) 'fog-on log must not contain the bypass line'
Assert-True (-not ($fogOn.Contains('FOG diag bypass (MXH_FOG_DISABLE=1)'))) 'fog-on sample must stay free of the bypass line'

Write-Output 'PASS: gui-client-smoke monster and fog log checks'
