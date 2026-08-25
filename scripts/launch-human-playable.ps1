[CmdletBinding()]
param(
    [ValidateSet('playdh-current', 'sworking-2008-reference')]
    [string]$ResourceProfileId = 'playdh-current',
    [ValidateRange(0, 65535)] [int]$LoginPort = 16001,
    [ValidateRange(0, 65535)] [int]$AgentPort = 17001,
    [ValidateRange(0, 65535)] [int]$MapPort = 18001,
    [ValidateRange(0, 255)] [int]$MapNumber = 10,
    [switch]$SkipServers
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$runId = Get-Date -Format 'yyyyMMdd-HHmmss'
$runRoot = Join-Path $repoRoot "modern\out\runs\human\$runId"
$logRoot = Join-Path $runRoot 'logs'
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

function Find-Executable {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Executable not found: $($Candidates -join ', ')"
}

$resourceRoot = if ($ResourceProfileId -eq 'playdh-current') {
    Join-Path $repoRoot 'modern\data\PlayDH'
} else {
    Join-Path $repoRoot 'reference\legacy-source\4dddd9a6\SWorking'
}
if (-not (Test-Path -LiteralPath $resourceRoot -PathType Container)) {
    throw "Resource profile root not found: $resourceRoot"
}

$clientExe = Find-Executable @(
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\mxh_client.exe'),
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\Debug\mxh_client.exe')
)
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
$owned = [System.Collections.Generic.List[object]]::new()

try {
    if (-not $SkipServers) {
        $serverArgs = @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $serverScript,
            '-Mode', 'start', '-ResourceProfileId', $ResourceProfileId,
            '-LoginPort', $LoginPort, '-AgentPort', $AgentPort, '-MapPort', $MapPort,
            '-MapNumber', $MapNumber
        )
        $server = Start-Process -FilePath 'powershell.exe' -ArgumentList $serverArgs -WorkingDirectory $repoRoot -PassThru -Wait
        if ($server.ExitCode -ne 0) { throw "Modern server startup failed with exit code $($server.ExitCode)" }
    }

    $clientArgs = @(
        '--login-port', $LoginPort,
        '--map-port', $MapPort,
        '--resource-profile', $ResourceProfileId,
        '--resource-root', $resourceRoot,
        '--width', 800,
        '--height', 600
    )
    $client = Start-Process -FilePath $clientExe -ArgumentList $clientArgs -WorkingDirectory $repoRoot -PassThru
    $owned.Add($client)

    Write-Host ''
    Write-Host "Human acceptance run: $runId" -ForegroundColor Cyan
    Write-Host 'No credentials or synthetic input are supplied by this script.'
    Write-Host 'Manually complete: launcher/settings → login → display transition → character select/create → Map10 → movement → NPC/UI → combat/skill → loot/pickup → map change → relog.'
    Write-Host "Logs and run metadata: $runRoot"
    Write-Host 'Press Enter after the human scenario is complete, or Ctrl+C to abort.'
    [Console]::ReadLine() | Out-Null
}
finally {
    foreach ($process in $owned) {
        if ($null -ne $process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
    }
    if (-not $SkipServers) {
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $serverScript -Mode stop | Out-File -LiteralPath (Join-Path $logRoot 'server-stop.txt') -Encoding utf8
    }
}
