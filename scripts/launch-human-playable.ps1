[CmdletBinding()]
param(
    [ValidateRange(1, 65535)] [int]$LoginPort = 26101,
    [ValidateRange(1, 65535)] [int]$AgentPort = 27101,
    [ValidateRange(1, 65535)] [int]$MapPort = 28101,
    [ValidateRange(0, 255)] [int]$MapNumber = 10,
    [ValidateRange(0, 64)] [int]$MinimumEvidenceFrames = 8,
    [switch]$SkipServers
)

# Compatibility entry point. The former implementation launched the client
# directly, accepted the read-only reference profile, and used broad process
# cleanup. Keep the familiar filename but route every invocation through the
# hardened launcher-driven acceptance flow.
$runner = Join-Path $PSScriptRoot 'run-human-acceptance.ps1'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $runner `
    -ResourceProfileId playdh-current -LoginPort $LoginPort -AgentPort $AgentPort `
    -MapPort $MapPort -MapNumber $MapNumber `
    -MinimumEvidenceFrames $MinimumEvidenceFrames -SkipServers:$SkipServers
exit $LASTEXITCODE
