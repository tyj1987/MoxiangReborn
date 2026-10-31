# Stop only recorded PIDs whose image is inside -ReleaseDir, then start -PreviousReleaseDir.
# Follows deploy/VM112_R2_STAGE.md: image path, start time, and SHA-256. Never by process name.
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$PublicHost,
    [Parameter(Mandatory = $true)]
    [string]$ReleaseDir,
    [Parameter(Mandatory = $true)]
    [string]$PreviousReleaseDir,
    [ValidateRange(1, 65535)]
    [int]$LoginPort = 16001,
    [ValidateRange(1, 65535)]
    [int]$AgentPort = 17001,
    [ValidateRange(1, 65535)]
    [int]$MapPort = 18001,
    [ValidateRange(1, 65535)]
    [int]$Map12Port = 18002,
    [ValidateRange(1, 65535)]
    [int]$Map2Port = 18003,
    [switch]$EnableMap17,
    [ValidateRange(1, 65535)]
    [int]$Map17Port = 18004,
    [int]$MapNumber = 10,
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'release_common.ps1')

if ($DryRun) { $WhatIfPreference = $true }
$planOnly = [bool]$WhatIfPreference -or [bool]$DryRun

Assert-MxhPublicHost -PublicHost $PublicHost
$null = Get-MxhReleasePorts -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort `
    -Map12Port $Map12Port -Map2Port $Map2Port -EnableMap17 ([bool]$EnableMap17) -Map17Port $Map17Port
if (-not (Test-Path -LiteralPath $ReleaseDir -PathType Container)) {
    throw ('release directory is missing: ' + $ReleaseDir)
}
if (-not (Test-Path -LiteralPath $PreviousReleaseDir -PathType Container)) {
    throw ('previous release directory is missing: ' + $PreviousReleaseDir)
}
$releaseFull = [IO.Path]::GetFullPath($ReleaseDir)
$previousFull = [IO.Path]::GetFullPath($PreviousReleaseDir)
if ([string]::Equals($releaseFull, $previousFull, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'PreviousReleaseDir must be a different directory'
}
$statePath = Get-MxhRunStatePath -ReleaseDir $releaseFull
$state = Read-MxhRunState -Path $statePath
$rows = @(Get-MxhProcessRows -State $state)

foreach ($entry in $rows) {
    $decision = Test-MxhOwnedProcess -Entry $entry -ReleaseDir $releaseFull
    if (-not $decision.Owned) {
        Write-Host ('skip pid=' + $entry.pid + ' reason=' + $decision.Reason)
        continue
    }
    if ($planOnly) {
        Write-Host ('WhatIf: stop pid=' + $entry.pid + ' exe=' + $decision.Image)
        continue
    }
    Stop-MxhOwnedProcess -ProcessId ([int]$entry.pid) -ExpectedImage $decision.Image
    Write-Host ('stopped pid=' + $entry.pid + ' exe=' + $decision.Image)
}

if ($planOnly) {
    Write-Host ('WhatIf: start ' + $previousFull)
    return
}

$archived = Join-Path $releaseFull 'run-state.rolled-back.json'
if (Test-Path -LiteralPath $archived) {
    Remove-Item -LiteralPath $archived -Force
}
Move-Item -LiteralPath $statePath -Destination $archived

$startScript = Join-Path $PSScriptRoot 'start_release.ps1'
$startParams = @{
    PublicHost = $PublicHost
    ReleaseDir = $previousFull
    LoginPort = [int]$LoginPort
    AgentPort = [int]$AgentPort
    MapPort = [int]$MapPort
    Map12Port = [int]$Map12Port
    Map2Port = [int]$Map2Port
    MapNumber = [int]$MapNumber
    DatabaseConfigEnv = $DatabaseConfigEnv
}
if ($EnableMap17) {
    $startParams.EnableMap17 = $true
    $startParams.Map17Port = [int]$Map17Port
}
& $startScript @startParams
Write-Host ('rollback started previous=' + $previousFull)
