[CmdletBinding()]
param(
    [ValidateSet('start', 'stop', 'status', 'restart')]
    [string]$Mode = 'start',
    [ValidateSet('CHINA', 'KOR', 'HK', 'JAPAN', 'TL')]
    [string]$Locale = 'CHINA',
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',
    [int]$LoginPort = 16001,
    [int]$AgentPort = 17001,
    [int]$MapPort = 18001,
    [int]$MapNumber = 10,
    [ValidateSet('sqlite', 'mssql_odbc')]
    [string]$Backend = 'sqlite',
    [string]$BindAddress = '0.0.0.0',
    [string]$AdvertisedAgentAddress = '127.0.0.1',
    [string]$MapBindAddress = '127.0.0.1',
    [string]$MapEndpointAddress = '127.0.0.1',
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG',
    [string]$DataDir = '',
    [ValidateSet('playdh-current', 'sworking-2008-reference')]
    [string]$ResourceProfileId = 'playdh-current',
    [string]$ResourceRoot = '',
    [string]$ServerResourceRoot = '',
    [switch]$DryRun,
    [switch]$UseHsel,
    [switch]$AllowDevFallbacks
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$profileManifestPath = Join-Path $repoRoot 'deploy\resource-profiles.json'
$profileManifest = Get-Content -LiteralPath $profileManifestPath -Raw | ConvertFrom-Json
$buildRoot = Join-Path $repoRoot 'modern\build\tools'
$stateDir = Join-Path $repoRoot 'deploy\runtime\modern'
$pidFile = Join-Path $stateDir 'pids.json'
$manifestFile = Join-Path $stateDir 'deployment.json'
$logDir = Join-Path $stateDir 'logs'

if ([string]::IsNullOrWhiteSpace($DataDir)) {
    $DataDir = Join-Path $stateDir 'data'
}
if ($null -eq $profileManifest.profiles.PSObject.Properties[$ResourceProfileId]) {
    throw "Unknown resource profile '$ResourceProfileId' (manifest: $profileManifestPath)"
}
$profile = $profileManifest.profiles.$ResourceProfileId
if (-not [bool]$profile.releaseAllowed -and -not $AllowDevFallbacks) {
    throw "Profile '$ResourceProfileId' is not release-enabled"
}
if ([string]::IsNullOrWhiteSpace([string]$profile.encoding)) {
    throw "Profile '$ResourceProfileId' has no declared resource encoding"
}
if ([string]::IsNullOrWhiteSpace($ResourceRoot)) {
    $ResourceRoot = Join-Path $repoRoot ([string]$profile.source)
}
if ([string]::IsNullOrWhiteSpace($ServerResourceRoot)) {
    $configuredServerRoot = [Environment]::GetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT')
    if (-not [string]::IsNullOrWhiteSpace($configuredServerRoot)) {
        $ServerResourceRoot = $configuredServerRoot
    } else {
        $ServerResourceRoot = Join-Path $ResourceRoot 'Resource\Server'
    }
}

function Resolve-ModernBinary {
    param([string]$ToolDirectory, [string]$FileName)
    $toolRoot = Join-Path $buildRoot $ToolDirectory
    $candidates = @(
        (Join-Path $toolRoot $FileName),
        (Join-Path (Join-Path $toolRoot $Config) $FileName)
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Missing $Config executable: $FileName (checked single- and multi-config paths under $toolRoot)"
}

function Read-ModernState {
    if (-not (Test-Path -LiteralPath $pidFile -PathType Leaf)) { return @() }
    $parsed = Get-Content -LiteralPath $pidFile -Raw | ConvertFrom-Json
    if ($parsed -is [Array]) {
        $parsed | ForEach-Object { Write-Output $_ }
        return
    }
    Write-Output $parsed
}

function Test-ExpectedProcess {
    param($Entry)
    $process = Get-CimInstance Win32_Process -Filter "ProcessId=$([int]$Entry.pid)" -ErrorAction SilentlyContinue
    if ($null -eq $process -or [string]::IsNullOrWhiteSpace([string]$process.ExecutablePath)) {
        # Some managed Windows environments deny CIM process-path queries.
        # Process.Path still gives us an exact executable identity without
        # falling back to a dangerous process-name kill.
        $process = Get-Process -Id ([int]$Entry.pid) -ErrorAction SilentlyContinue
        if ($null -eq $process -or [string]::IsNullOrWhiteSpace([string]$process.Path)) { return $false }
        $processPath = [string]$process.Path
    } else {
        $processPath = [string]$process.ExecutablePath
    }
    try {
        $actual = (Resolve-Path -LiteralPath $processPath -ErrorAction Stop).Path
        $expected = (Resolve-Path -LiteralPath ([string]$Entry.exe) -ErrorAction Stop).Path
        return [string]::Equals($actual, $expected, [StringComparison]::OrdinalIgnoreCase)
    } catch {
        return $false
    }
}

function Stop-Modern {
    $state = @(Read-ModernState)
    foreach ($entry in $state) {
        if (Test-ExpectedProcess $entry) {
            Stop-Process -Id ([int]$entry.pid) -Force
        } elseif (Get-Process -Id ([int]$entry.pid) -ErrorAction SilentlyContinue) {
            Write-Warning "Refusing to stop stale PID $($entry.pid): executable path no longer matches $($entry.exe)"
        }
    }
    Remove-Item -LiteralPath $pidFile -Force -ErrorAction SilentlyContinue
    Write-Host 'Modern servers stopped' -ForegroundColor Green
}

function Test-LocalPort {
    param([int]$Port)
    $client = [Net.Sockets.TcpClient]::new()
    try {
        $task = $client.ConnectAsync('127.0.0.1', $Port)
        if (-not $task.Wait(1000)) { return $false }
        return $client.Connected
    } catch {
        return $false
    } finally {
        $client.Dispose()
    }
}

if ($Mode -eq 'stop') { Stop-Modern; exit 0 }
if ($Mode -eq 'restart') { Stop-Modern; Start-Sleep -Seconds 1 }
if ($Mode -eq 'status') {
    foreach ($entry in @(Read-ModernState)) {
        $running = Test-ExpectedProcess $entry
        Write-Host "$($entry.name): $(if ($running) { 'running' } else { 'stopped' }) pid=$($entry.pid)"
    }
    foreach ($port in @($LoginPort, $AgentPort, $MapPort)) {
        Write-Host "port ${port}: $(Test-LocalPort $port)"
    }
    exit 0
}

New-Item -ItemType Directory -Force -Path $stateDir, $DataDir, $logDir | Out-Null
$ResourceRoot = (Resolve-Path -LiteralPath $ResourceRoot).Path
$ServerResourceRoot = (Resolve-Path -LiteralPath $ServerResourceRoot).Path

foreach ($entry in @($profile.required)) {
    $requiredPath = Join-Path $ResourceRoot ([string]$entry.path)
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Profile '$ResourceProfileId' is missing manifest resource: $requiredPath"
    }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $requiredPath).Hash.ToLowerInvariant()
    if ($actualHash -ne ([string]$entry.sha256).ToLowerInvariant()) {
        throw "Profile '$ResourceProfileId' hash mismatch: $requiredPath"
    }
}

$requiredResources = @(
    (Join-Path $ResourceRoot 'Resource\ItemList.bin'),
    (Join-Path $ResourceRoot 'Resource\CharacterExpPoint.bin'),
    (Join-Path $ServerResourceRoot "Monster_$MapNumber.bin")
)
foreach ($resource in $requiredResources) {
    if (-not (Test-Path -LiteralPath $resource -PathType Leaf)) {
        throw "Required runtime resource is missing: $resource"
    }
}

$dbTool = Resolve-ModernBinary 'MoxianDbTool' 'mxh_db_tool.exe'
$loginExe = Resolve-ModernBinary 'MoxianLoginServer' 'mxh_login_server.exe'
$agentExe = Resolve-ModernBinary 'MoxianAgentServer' "mxh_agent_server_$Locale.exe"
$mapExe = Resolve-ModernBinary 'MoxianMapServer' "mxh_map_server_$Locale.exe"

$previousDatabaseConfig = [Environment]::GetEnvironmentVariable($DatabaseConfigEnv, 'Process')
if ($Backend -eq 'sqlite') {
    $databasePath = Join-Path $DataDir 'moxian.db'
    $databaseConfig = "backend=sqlite;path=$databasePath"
} else {
    $databaseConfig = $previousDatabaseConfig
    if ([string]::IsNullOrWhiteSpace($databaseConfig)) {
        throw "Set process environment variable $DatabaseConfigEnv to the MSSQL ODBC config before starting"
    }
}
[Environment]::SetEnvironmentVariable($DatabaseConfigEnv, $databaseConfig, 'Process')

$commonDbArgs = @('--backend', $Backend, '--db-env', $DatabaseConfigEnv, '--legacy')
$hselArgs = if ($UseHsel) { @('--use-hsel') } else { @() }
$fallbackArgs = if ($AllowDevFallbacks) { @('--allow-dev-fallbacks') } else { @() }
$processes = @(
    [ordered]@{
        name = 'map'; exe = $mapExe; port = $MapPort
        args = @('--port', $MapPort, '--map', $MapNumber, '--bind-address', $MapBindAddress,
            '--resource-root', $ResourceRoot, '--server-resource-root', $ServerResourceRoot) + $commonDbArgs + $hselArgs + $fallbackArgs
    },
    [ordered]@{
        name = 'agent'; exe = $agentExe; port = $AgentPort
        args = @('--port', $AgentPort, '--bind-address', $BindAddress,
            '--map-server', "${MapEndpointAddress}:$MapPort", '--default-map', $MapNumber) + $commonDbArgs + $hselArgs
    },
    [ordered]@{
        name = 'login'; exe = $loginExe; port = $LoginPort
        args = @('--port', $LoginPort, '--bind-address', $BindAddress,
            '--agent-addr', $AdvertisedAgentAddress, '--agent-port', $AgentPort) + $commonDbArgs + $hselArgs
    }
)

try {
    if ($DryRun) {
        Write-Host "Modern server dry-run (backend=$Backend locale=$Locale config=$Config profile=$ResourceProfileId encoding=$($profile.encoding) db-env=$DatabaseConfigEnv)" -ForegroundColor Cyan
        foreach ($item in $processes) {
            Write-Host "$($item.name): $($item.exe) $($item.args -join ' ')"
        }
        exit 0
    }

    & $dbTool migrate --db-env $DatabaseConfigEnv
    if ($LASTEXITCODE -ne 0) { throw "Database migration failed with exit code $LASTEXITCODE" }

    $gitCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
    $manifest = [ordered]@{
        schema = 1
        git_commit = $gitCommit
        locale = $Locale
        config = $Config
        backend = $Backend
        database_source = "environment:$DatabaseConfigEnv"
        bind_address = $BindAddress
        advertised_agent_address = $AdvertisedAgentAddress
        map_bind_address = $MapBindAddress
        map_endpoint = "${MapEndpointAddress}:$MapPort"
        map_number = $MapNumber
        resource_profile_id = $ResourceProfileId
        resource_encoding = [string]$profile.encoding
        resource_root = $ResourceRoot
        server_resource_root = $ServerResourceRoot
        allow_dev_fallbacks = [bool]$AllowDevFallbacks
        started_at_utc = [DateTime]::UtcNow.ToString('o')
    }
    $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestFile -Encoding utf8

    $state = @()
    foreach ($item in $processes) {
        $stdout = Join-Path $logDir "$($item.name).out.log"
        $stderr = Join-Path $logDir "$($item.name).err.log"
        $process = Start-Process -FilePath $item.exe -ArgumentList $item.args -WorkingDirectory (Split-Path -Parent $item.exe) -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -WindowStyle Hidden
        $entry = [ordered]@{ name = $item.name; pid = $process.Id; port = $item.port; exe = $item.exe }
        $state += $entry
        $state | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $pidFile -Encoding utf8
        Start-Sleep -Milliseconds 500
        if ($process.HasExited) { throw "$($item.name) exited during startup (see $stderr)" }
    }

    Start-Sleep -Seconds 1
    foreach ($port in @($MapPort, $AgentPort, $LoginPort)) {
        if (-not (Test-LocalPort $port)) { throw "Modern server port is not healthy: $port" }
    }
    Write-Host "Modern servers started: Login=$LoginPort Agent=$AgentPort Map=$MapPort" -ForegroundColor Green
} catch {
    if (Test-Path -LiteralPath $pidFile) { Stop-Modern }
    throw
} finally {
    [Environment]::SetEnvironmentVariable($DatabaseConfigEnv, $previousDatabaseConfig, 'Process')
}
