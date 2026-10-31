# Report HEALTH OK or HEALTH FAIL. Exit 1 when any line is FAIL.
# Optional portal check runs only when -PortalUrl is set.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ReleaseDir,
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
    [string]$PortalUrl = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'release_common.ps1')

$failed = $false
$ports = Get-MxhReleasePorts -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort `
    -Map12Port $Map12Port -Map2Port $Map2Port -EnableMap17 ([bool]$EnableMap17) -Map17Port $Map17Port

if (-not (Test-Path -LiteralPath $ReleaseDir -PathType Container)) {
    Write-Host ('HEALTH FAIL release-dir missing ' + $ReleaseDir)
    exit 1
}
$releaseFull = [IO.Path]::GetFullPath($ReleaseDir)

foreach ($item in @($ports)) {
    if (Test-MxhTcpPortListening -Port ([int]$item.Port)) {
        Write-Host ('HEALTH OK port ' + $item.Port + ' listening')
    } else {
        Write-Host ('HEALTH FAIL port ' + $item.Port + ' not-listening')
        $failed = $true
    }
}

$statePath = Get-MxhRunStatePath -ReleaseDir $releaseFull
if (Test-Path -LiteralPath $statePath -PathType Leaf) {
    $state = Read-MxhRunState -Path $statePath
    $rows = @(Get-MxhProcessRows -State $state)
    if ($rows.Count -eq 0) {
        Write-Host 'HEALTH FAIL process run-state-empty'
        $failed = $true
    }
    foreach ($entry in $rows) {
        $decision = Test-MxhOwnedProcess -Entry $entry -ReleaseDir $releaseFull
        if ($decision.Owned) {
            Write-Host ('HEALTH OK process ' + $entry.name + ' pid=' + $entry.pid)
        } else {
            Write-Host ('HEALTH FAIL process ' + $entry.name + ' pid=' + $entry.pid + ' ' + $decision.Reason)
            $failed = $true
        }
    }
} else {
    Write-Host 'HEALTH FAIL process run-state-missing'
    $failed = $true
}

$logDir = Join-Path (Join-Path $releaseFull 'run') 'logs'
$sawMatch = $false
$sawMismatch = $false
if (Test-Path -LiteralPath $logDir -PathType Container) {
    $logs = @(Get-ChildItem -LiteralPath $logDir -File -Recurse -ErrorAction SilentlyContinue)
    foreach ($log in $logs) {
        $text = ''
        try { $text = [IO.File]::ReadAllText($log.FullName) } catch { $text = '' }
        if ($text.Contains('PROTOCOL_MISMATCH')) { $sawMismatch = $true }
        if ($text.Contains('PROTOCOL_MATCH')) { $sawMatch = $true }
    }
}
if ($sawMismatch) {
    Write-Host 'HEALTH FAIL protocol PROTOCOL_MISMATCH'
    $failed = $true
} elseif ($sawMatch) {
    Write-Host 'HEALTH OK protocol PROTOCOL_MATCH'
}

if (-not [string]::IsNullOrWhiteSpace($PortalUrl)) {
    $url = $PortalUrl.Trim()
    if ($url -notmatch '/api/healthz($|\?)') {
        $url = $url.TrimEnd('/') + '/api/healthz'
    }
    $portalOk = $false
    $client = $null
    try {
        $client = New-Object System.Net.Http.HttpClient
        $client.Timeout = [TimeSpan]::FromSeconds(3)
        $response = $client.GetAsync($url).GetAwaiter().GetResult()
        $portalOk = ([int]$response.StatusCode -eq 200)
    } catch {
        $portalOk = $false
    } finally {
        if ($null -ne $client) { $client.Dispose() }
    }
    if ($portalOk) {
        Write-Host 'HEALTH OK portal'
    } else {
        Write-Host 'HEALTH FAIL portal'
        $failed = $true
    }
}

if ($failed) { exit 1 }
Write-Host 'HEALTH OK summary'
exit 0
