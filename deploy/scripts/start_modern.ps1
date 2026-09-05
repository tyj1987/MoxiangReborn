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
    [switch]$AllowDevFallbacks,
    [switch]$SkipResourceIntegrity,
    [string]$RunId = ''
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
if ($null -eq $profile.required) {
    throw "Profile '$ResourceProfileId' has no required-resource list"
}
$seenProfilePaths = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($entry in @($profile.required)) {
    $manifestPath = [string]$entry.path
    $manifestHash = [string]$entry.sha256
    if ([string]::IsNullOrWhiteSpace($manifestPath) -or [IO.Path]::IsPathRooted($manifestPath) -or $manifestPath.Replace('\', '/').Split('/') -contains '..') {
        throw "Profile '$ResourceProfileId' contains an unsafe manifest path: '$manifestPath'"
    }
    if ($manifestHash -notmatch '^[0-9a-fA-F]{64}$') {
        throw "Profile '$ResourceProfileId' contains an invalid SHA-256 for '$manifestPath'"
    }
    if (-not $seenProfilePaths.Add($manifestPath)) {
        throw "Profile '$ResourceProfileId' contains a duplicate manifest path: '$manifestPath'"
    }
}
if ([string]::IsNullOrWhiteSpace([string]$profile.hashManifest)) {
    throw "Profile '$ResourceProfileId' has no hash manifest"
}
$hashManifestPath = Join-Path $repoRoot ([string]$profile.hashManifest)
if (-not (Test-Path -LiteralPath $hashManifestPath -PathType Leaf)) {
    throw "Profile '$ResourceProfileId' hash manifest is missing: $hashManifestPath"
}
$hashManifest = Get-Content -LiteralPath $hashManifestPath -Raw | ConvertFrom-Json
if ([string]$hashManifest.profileId -ne $ResourceProfileId) {
    throw "Profile '$ResourceProfileId' hash manifest profile mismatch: $hashManifestPath"
}
if ($null -eq $profile.inventory -or
    [int64]$hashManifest.fileCount -ne [int64]$profile.inventory.fileCount -or
    [int64]$hashManifest.byteCount -ne [int64]$profile.inventory.byteCount) {
    throw "Profile '$ResourceProfileId' inventory mismatch between resource profile and $hashManifestPath"
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

if ($SkipResourceIntegrity -and -not $AllowDevFallbacks) {
    throw '-SkipResourceIntegrity is only permitted together with -AllowDevFallbacks; release startup must verify the complete profile'
}

function Assert-ResourceProfileIntegrity {
    param([string]$Root, $Manifest, [string]$ProfileId)
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        throw "Profile '$ProfileId' resource root is missing: $Root"
    }
    if ($null -eq $Manifest.files -or @($Manifest.files).Count -eq 0) {
        throw "Profile '$ProfileId' hash manifest has no file entries"
    }
    $expected = @{}
    foreach ($entry in @($Manifest.files)) {
        $relative = ([string]$entry.path).Replace('/', '\')
        if ([string]::IsNullOrWhiteSpace($relative) -or [IO.Path]::IsPathRooted($relative) -or $relative.Split('\') -contains '..') {
            throw "Profile '$ProfileId' hash manifest contains an unsafe path: $($entry.path)"
        }
        $key = $relative.Replace('\', '/').ToLowerInvariant()
        if ($expected.ContainsKey($key)) { throw "Profile '$ProfileId' hash manifest contains duplicate path: $relative" }
        if ([string]$entry.sha256 -notmatch '^[0-9a-fA-F]{64}$') { throw "Profile '$ProfileId' hash manifest contains invalid SHA-256: $relative" }
        $expected[$key] = [pscustomobject]@{ path = $relative; bytes = [int64]$entry.bytes; sha256 = ([string]$entry.sha256).ToLowerInvariant() }
    }
    $actualFiles = @(Get-ChildItem -LiteralPath $Root -File -Recurse)
    if ($actualFiles.Count -ne [int64]$Manifest.fileCount) {
        throw "Profile '$ProfileId' file count mismatch: expected $($Manifest.fileCount), found $($actualFiles.Count)"
    }
    $actualBytes = [int64](($actualFiles | Measure-Object -Property Length -Sum).Sum)
    if ($actualBytes -ne [int64]$Manifest.byteCount) {
        throw "Profile '$ProfileId' byte count mismatch: expected $($Manifest.byteCount), found $actualBytes"
    }
    foreach ($file in $actualFiles) {
        $relative = $file.FullName.Substring($Root.Length + 1).Replace('\', '/')
        $key = $relative.ToLowerInvariant()
        if (-not $expected.ContainsKey($key)) { throw "Profile '$ProfileId' contains an unregistered resource: $relative" }
        $entry = $expected[$key]
        if ([int64]$file.Length -ne $entry.bytes) { throw "Profile '$ProfileId' size mismatch: $relative" }
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
        if ($hash -ne $entry.sha256) { throw "Profile '$ProfileId' hash mismatch: $relative" }
        $expected.Remove($key)
    }
    if ($expected.Count -ne 0) { throw "Profile '$ProfileId' hash manifest contains files missing from the resource root: $($expected.Values.path -join ', ')" }
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

if (-not $SkipResourceIntegrity) {
    Assert-ResourceProfileIntegrity -Root $ResourceRoot -Manifest $hashManifest -ProfileId $ResourceProfileId
} else {
    Write-Warning "Skipping complete resource integrity verification for development fallback profile '$ResourceProfileId'"
}

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

$mapResourcePath = Join-Path $ServerResourceRoot "Monster_$MapNumber.bin"
if (-not (Test-Path -LiteralPath $mapResourcePath -PathType Leaf) -and
    $MapNumber -ge 0 -and $MapNumber -le 99) {
    $legacyPaddedPath = Join-Path $ServerResourceRoot (
        "Monster_{0:D2}.bin" -f $MapNumber)
    if (Test-Path -LiteralPath $legacyPaddedPath -PathType Leaf) {
        $mapResourcePath = $legacyPaddedPath
    }
}

$requiredResources = @(
    (Join-Path $ResourceRoot 'Resource\ItemList.bin'),
    (Join-Path $ResourceRoot 'Resource\CharacterExpPoint.bin'),
    $mapResourcePath
)
foreach ($resource in $requiredResources) {
    if (-not (Test-Path -LiteralPath $resource -PathType Leaf)) {
        throw "Required runtime resource is missing: $resource"
    }
}

function Get-ServerResourceProfileDiagnostic {
    param([string]$ResourcePath, [string]$ProfileId, [string]$Encoding)
    $info = [ordered]@{
        profile_id = $ProfileId
        encoding = $Encoding
        path = $ResourcePath
        exists = $false
        byte_length = 0
        first_u32_le = $null
        size_prefixed_marker = $false
        status = 'missing'
    }
    if (-not (Test-Path -LiteralPath $ResourcePath -PathType Leaf)) {
        return [pscustomobject]$info
    }
    $bytes = [IO.File]::ReadAllBytes($ResourcePath)
    $info.exists = $true
    $info.byte_length = $bytes.Length
    if ($bytes.Length -ge 4) {
        $info.first_u32_le = [BitConverter]::ToUInt32($bytes, 0)
    }
    $info.size_prefixed_marker = ($bytes.Length -ge 5 -and $bytes.Length -le (256MB) -and $info.first_u32_le -eq $bytes.Length)
    if ($Encoding -eq 'server-size-prefixed-opaque-v1') {
        $info.status = if (-not $info.size_prefixed_marker) {
            'profile-format-mismatch'
        } elseif ($bytes.Length -lt 32) {
            'decoder-invalid'
        } else {
            'decoder-ready'
        }
    } else {
        $info.status = 'present-unverified'
    }
    return [pscustomobject]$info
}

$serverResourceDiagnostic = Get-ServerResourceProfileDiagnostic -ResourcePath $mapResourcePath -ProfileId $ResourceProfileId -Encoding ([string]$profile.encoding)
$knownEmptyMonsterMap = $serverResourceDiagnostic.byte_length -eq 14 -and
    $serverResourceDiagnostic.first_u32_le -eq 20040309
if ($serverResourceDiagnostic.status -eq 'profile-format-mismatch' -and -not $knownEmptyMonsterMap) {
    throw "Profile '$ResourceProfileId' declares '$($profile.encoding)' but $mapResourcePath has no size-prefixed marker (bytes=$($serverResourceDiagnostic.byte_length), first_u32_le=$($serverResourceDiagnostic.first_u32_le)); refusing positional decode"
}
if ($serverResourceDiagnostic.status -eq 'decoder-invalid' -and -not $knownEmptyMonsterMap) {
    throw "Profile '$ResourceProfileId' resource encoding '$($profile.encoding)' is structurally detected at $mapResourcePath but is too short for the recovered opaque container header (bytes=$($serverResourceDiagnostic.byte_length)); refusing to start"
}
if ($knownEmptyMonsterMap) {
    Write-Host "Map $MapNumber uses the canonical 14-byte empty monster table; battle spawn validation is disabled for this map."
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

$commonDbArgs = @('--backend', $Backend, '--db-env', $DatabaseConfigEnv)
$legacyArgs = @('--legacy')
$hselArgs = if ($UseHsel) { @('--use-hsel') } else { @() }
$fallbackArgs = if ($AllowDevFallbacks) { @('--allow-dev-fallbacks') } else { @() }
# Cross-process run identifier.  When non-empty, every server
# process and the client receive MXH_RUN_ID so logs from
# concurrent processes can be correlated.  The capture tool
# requires a fresh run id per attempt so we do not mix stale
# evidence with the current run.
$runIdArg = if (-not [string]::IsNullOrWhiteSpace($RunId)) { @('--run-id', $RunId) } else { @() }
$processes = @(
    [ordered]@{
        name = 'map'; exe = $mapExe; port = $MapPort
        args = @('--port', $MapPort, '--map', $MapNumber, '--bind-address', $MapBindAddress,
            '--resource-root', $ResourceRoot, '--server-resource-root', $ServerResourceRoot,
            '--resource-profile', $ResourceProfileId) + $runIdArg + $commonDbArgs + $hselArgs + $fallbackArgs
    },
    [ordered]@{
        name = 'agent'; exe = $agentExe; port = $AgentPort
        args = @('--port', $AgentPort, '--bind-address', $BindAddress,
            '--map-server', "${MapEndpointAddress}:$MapPort", '--default-map', $MapNumber) + $runIdArg + $commonDbArgs + $legacyArgs + $hselArgs
    },
    [ordered]@{
        name = 'login'; exe = $loginExe; port = $LoginPort
        args = @('--port', $LoginPort, '--bind-address', $BindAddress,
            '--agent-addr', $AdvertisedAgentAddress, '--agent-port', $AgentPort) + $runIdArg + $commonDbArgs + $legacyArgs + $hselArgs
    }
)

try {
    if ($DryRun) {
        Write-Host "Modern server dry-run (backend=$Backend locale=$Locale config=$Config profile=$ResourceProfileId encoding=$($profile.encoding) db-env=$DatabaseConfigEnv)" -ForegroundColor Cyan
        Write-Host "server-resource preflight: status=$($serverResourceDiagnostic.status) bytes=$($serverResourceDiagnostic.byte_length) first_u32_le=$($serverResourceDiagnostic.first_u32_le) path=$($serverResourceDiagnostic.path)" -ForegroundColor Yellow
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
        run_id = $RunId
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
        $procEnv = [System.Collections.Generic.Dictionary[string,string]]::new()
        foreach ($kv in [Environment]::GetEnvironmentVariables('Process').GetEnumerator()) {
            $procEnv[[string]$kv.Key] = [string]$kv.Value
        }
        $procEnv['MXH_PROCESS'] = $item.name
        if (-not [string]::IsNullOrWhiteSpace($RunId)) {
            $procEnv['MXH_RUN_ID'] = $RunId
        }
        $process = Start-Process -FilePath $item.exe -ArgumentList $item.args -WorkingDirectory (Split-Path -Parent $item.exe) -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -WindowStyle Hidden -Environment $procEnv
        $entry = [ordered]@{ name = $item.name; pid = $process.Id; port = $item.port; exe = $item.exe; run_id = $RunId }
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
