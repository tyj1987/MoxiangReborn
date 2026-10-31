# Start one release directory on an explicit public host.
# Does not deploy files and does not read a hard-coded host.
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$PublicHost,
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
    [int]$MapNumber = 10,
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'release_common.ps1')

if ($DryRun) { $WhatIfPreference = $true }
$planOnly = [bool]$WhatIfPreference -or [bool]$DryRun

Assert-MxhPublicHost -PublicHost $PublicHost
$ports = Get-MxhReleasePorts -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort `
    -Map12Port $Map12Port -Map2Port $Map2Port -EnableMap17 ([bool]$EnableMap17) -Map17Port $Map17Port
if (-not (Test-Path -LiteralPath $ReleaseDir -PathType Container)) {
    throw ('release directory is missing: ' + $ReleaseDir)
}
$releaseFull = [IO.Path]::GetFullPath($ReleaseDir)
$layout = Get-MxhReleaseLayout -ReleaseDir $releaseFull

if (-not $planOnly) {
    Assert-MxhPortsFree -Ports $ports
}

$backend = $null
if (-not $planOnly) {
    $backend = Get-MxhDatabaseBackend -EnvName $DatabaseConfigEnv
    foreach ($binary in @($layout.Map, $layout.Agent, $layout.Login)) {
        if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
            throw ('release executable is missing: ' + $binary)
        }
    }
    if (Test-Path -LiteralPath $layout.StatePath -PathType Leaf) {
        throw ('run-state json already exists: ' + $layout.StatePath)
    }
} else {
    $backend = 'planned'
}

$dbArgs = @('--backend', [string]$backend, '--db-env', $DatabaseConfigEnv)
$planned = New-Object System.Collections.Generic.List[object]
$mapNumbers = @(
    [pscustomobject]@{ Name = 'map10'; Port = $MapPort; Map = $MapNumber },
    [pscustomobject]@{ Name = 'map12'; Port = $Map12Port; Map = 12 },
    [pscustomobject]@{ Name = 'map2'; Port = $Map2Port; Map = 2 }
)
if ($EnableMap17) {
    $mapNumbers += [pscustomobject]@{ Name = 'map17'; Port = $Map17Port; Map = 17 }
}
foreach ($map in $mapNumbers) {
    $planned.Add([pscustomobject]@{
        Name = $map.Name
        Exe = $layout.Map
        Port = [int]$map.Port
        Args = @(
            '--port', [string]$map.Port,
            '--map', [string]$map.Map,
            '--bind-address', $PublicHost,
            '--resource-root', $layout.ResourceRoot,
            '--server-resource-root', $layout.ServerResourceRoot,
            '--resource-profile', 'playdh-current'
        ) + $dbArgs
    }) | Out-Null
}
$agentArgs = @(
    '--port', [string]$AgentPort,
    '--bind-address', $PublicHost,
    '--map-server', ($PublicHost + ':' + $MapPort),
    '--map-server-map', ('12=' + $PublicHost + ':' + $Map12Port),
    '--map-server-map', ('2=' + $PublicHost + ':' + $Map2Port)
)
if ($EnableMap17) {
    $agentArgs += @('--map-server-map', ('17=' + $PublicHost + ':' + $Map17Port))
}
$agentArgs += @('--default-map', [string]$MapNumber)
if ($EnableMap17) {
    $agentArgs += @('--start-area', '17')
}
$agentArgs += $dbArgs + @('--legacy')
$planned.Add([pscustomobject]@{
    Name = 'agent'
    Exe = $layout.Agent
    Port = [int]$AgentPort
    Args = $agentArgs
}) | Out-Null
$planned.Add([pscustomobject]@{
    Name = 'login'
    Exe = $layout.Login
    Port = [int]$LoginPort
    Args = @(
        '--port', [string]$LoginPort,
        '--bind-address', $PublicHost,
        '--agent-addr', $PublicHost,
        '--agent-port', [string]$AgentPort
    ) + $dbArgs + @('--legacy')
}) | Out-Null

if ($planOnly) {
    foreach ($item in $planned) {
        Write-Host ('WhatIf: start ' + $item.Name + ' ' + $item.Exe + ' ' + ($item.Args -join ' '))
    }
    return
}

New-Item -ItemType Directory -Force -Path $layout.LogDir | Out-Null
$state = [ordered]@{
    schema = 1
    publicHost = $PublicHost
    releaseDir = $releaseFull
    databaseConfigEnv = $DatabaseConfigEnv
    startedAtUtc = [DateTime]::UtcNow.ToString('o')
    processes = @()
}
$rows = @()
try {
    foreach ($item in $planned) {
        $stdout = Join-Path $layout.LogDir ($item.Name + '.out.log')
        $stderr = Join-Path $layout.LogDir ($item.Name + '.err.log')
        $startParams = @{
            FilePath = $item.Exe
            ArgumentList = $item.Args
            WorkingDirectory = (Split-Path -Parent $item.Exe)
            RedirectStandardOutput = $stdout
            RedirectStandardError = $stderr
            PassThru = $true
        }
        if ($env:OS -eq 'Windows_NT') {
            $startParams['WindowStyle'] = 'Hidden'
        }
        $process = Start-Process @startParams
        Start-Sleep -Milliseconds 500
        if ($process.HasExited) {
            throw ($item.Name + ' exited during startup: ' + $process.ExitCode)
        }
        $hash = (Get-FileHash -LiteralPath $item.Exe -Algorithm SHA256).Hash.ToLowerInvariant()
        $rows += [pscustomobject]@{
            name = $item.Name
            pid = [int]$process.Id
            exe = $item.Exe
            port = [int]$item.Port
            sha256 = $hash
            startUtc = [DateTime]::UtcNow.ToString('o')
        }
        $state.processes = @($rows)
        Write-MxhRunState -Path $layout.StatePath -State $state
    }
} catch {
    foreach ($row in @($rows)) {
        $decision = Test-MxhOwnedProcess -Entry $row -ReleaseDir $releaseFull
        if ($decision.Owned) {
            try { Stop-MxhOwnedProcess -ProcessId ([int]$row.pid) -ExpectedImage ([string]$row.exe) } catch { }
        }
    }
    if (Test-Path -LiteralPath $layout.StatePath) {
        Remove-Item -LiteralPath $layout.StatePath -Force -ErrorAction SilentlyContinue
    }
    throw
}

Write-Host ('release started host=' + $PublicHost + ' count=' + $rows.Count + ' state=' + $layout.StatePath)
