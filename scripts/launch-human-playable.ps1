[CmdletBinding()]
param(
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9.-]+$')] [string]$LoginHost = '192.168.2.244',
    [ValidateRange(1, 65535)] [int]$LoginPort = 16001,
    [ValidateRange(1, 65535)] [int]$AgentPort = 17001,
    [ValidateRange(1, 65535)] [int]$MapPort = 18001,
    [ValidateRange(0, 64)] [int]$MinimumEvidenceFrames = 8
)

# Compatibility entry point. The former implementation launched the client
# directly, accepted the read-only reference profile, and used broad process
# cleanup. Keep the familiar filename but route every invocation through the
# hardened launcher-driven acceptance flow.
$runner = Join-Path $PSScriptRoot 'run-human-acceptance.ps1'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $runner `
    -ResourceProfileId playdh-current -BuildDirectory $BuildDirectory `
    -LoginHost $LoginHost -LoginPort $LoginPort -AgentPort $AgentPort `
    -MapPort $MapPort -MinimumEvidenceFrames $MinimumEvidenceFrames
exit $LASTEXITCODE
