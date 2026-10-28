[CmdletBinding()]
param(
    [switch]$Start,
    [ValidatePattern('^[A-Za-z_][A-Za-z0-9_]*$')]
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG',
    [string]$DatabaseHost = '',
    [ValidateRange(1, 65535)] [int]$LoginPort = 36001,
    [ValidateRange(1, 65535)] [int]$AgentPort = 37001,
    [ValidateRange(1, 65535)] [int]$MapPort = 38001,
    [ValidateRange(0, 255)] [int]$MapNumber = 10,
    # Map17 listens on 38004 only when this switch is set. Default off.
    # --start-area 17 is passed only in that branch. Occupied 38004 refuses
    # the start; there is no fallback port.
    [switch]$EnableMap17,
    [string]$AdvertisedAgentAddress = '192.168.2.244',
    [string]$ResourceRoot = 'C:\Moxiang\original\stage-r18-20261009\modern\data\PlayDH',
    [string]$ServerResourceRoot = 'C:\Moxiang\original\stage-r18-20261009\modern\data\PlayDH\Resource\Server'
)

$ErrorActionPreference = 'Stop'
$stageRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')).TrimEnd('\')
$expectedStage = 'C:\Moxiang\original\stage-r18-20261009'
if (-not [StringComparer]::OrdinalIgnoreCase.Equals($stageRoot, $expectedStage)) {
    throw "This wrapper is limited to VM112 stage $expectedStage; found $stageRoot"
}
if ($LoginPort -eq $AgentPort -or $LoginPort -eq $MapPort -or $AgentPort -eq $MapPort) {
    throw 'Login, Agent and Map ports must be distinct.'
}
if ($LoginPort -ne 36001 -or $AgentPort -ne 37001 -or $MapPort -ne 38001) {
    throw 'This isolated r18 stage requires its assigned 36001/37001/38001 ports.'
}
$listenPorts = @($LoginPort, $AgentPort, $MapPort)
$Map17Port = 38004
if ($EnableMap17) {
    if ($Map17Port -eq $LoginPort -or $Map17Port -eq $AgentPort -or $Map17Port -eq $MapPort) {
        throw 'Map17 port 38004 collides with an r18 listener.'
    }
    $listenPorts += $Map17Port
}
if (-not [StringComparer]::OrdinalIgnoreCase.Equals(
        [IO.Path]::GetFullPath($ResourceRoot).TrimEnd('\'),
        (Join-Path $expectedStage 'modern\data\PlayDH')) -or
    -not [StringComparer]::OrdinalIgnoreCase.Equals(
        [IO.Path]::GetFullPath($ServerResourceRoot).TrimEnd('\'),
        (Join-Path $expectedStage 'modern\data\PlayDH\Resource\Server'))) {
    throw 'Resource roots must be the canonical copy inside this r18 stage.'
}
if ($AdvertisedAgentAddress -ne '192.168.2.244') {
    throw 'VM112 must advertise 192.168.2.244 in LoginAck.'
}
$localAddresses = @(Get-NetIPAddress -AddressFamily IPv4 -ErrorAction Stop | Select-Object -ExpandProperty IPAddress)
if ($localAddresses -notcontains '192.168.2.244') {
    throw 'This host is not VM112 at 192.168.2.244.'
}

$manifestPath = Join-Path $stageRoot 'stage-manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Stage manifest is missing.' }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([int]$manifest.schema -ne 1 -or [string]$manifest.stage -ne $expectedStage) {
    throw 'Stage manifest identity mismatch.'
}
$writerContract = $manifest.writerContract
if ($null -eq $writerContract -or
    [string]$writerContract.id -ne 'original-vm112-r18-mssql-v3' -or
    [string]$writerContract.stageBuild -ne 'china-x86-debug-20261009' -or
    [string]$writerContract.databaseHost -ne '192.168.2.238' -or
    [string]$writerContract.database -ne 'MoxiangOriginalR18Test_20261009' -or
    [int]$writerContract.schemaVersion -ne 4 -or
    [string]$writerContract.vitalsContract -ne 'original-vitals-cas-v2' -or
    [string]$writerContract.progressionContract -ne 'original-exp-points-v3' -or
    [int]$manifest.listenerPorts.login -ne $LoginPort -or
    [int]$manifest.listenerPorts.agent -ne $AgentPort -or
    [int]$manifest.listenerPorts.map -ne $MapPort -or
    [string]$writerContract.numericAttributes -ne 'base-4-int-nullable-checked-v1' -or
    [string]$writerContract.resourceProfile -ne 'playdh-current' -or
    [string]$manifest.sourceHead -notmatch '^[0-9a-f]{40}$' -or
    [string]$manifest.sourcePatchSha256 -notmatch '^[0-9a-f]{64}$') {
    throw 'Stage writer contract identity mismatch.'
}
foreach ($entry in @($manifest.files)) {
    $relative = [string]$entry.path
    if ([IO.Path]::IsPathRooted($relative) -or $relative.Split('/') -contains '..') {
        throw "Unsafe stage path: $relative"
    }
    $path = Join-Path $stageRoot $relative.Replace('/', '\')
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Stage file missing: $relative" }
    $file = Get-Item -LiteralPath $path
    if ($file.Length -ne [int64]$entry.bytes -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine [string]$entry.sha256) {
        throw "Stage file hash mismatch: $relative"
    }
}

if (-not (Test-Path -LiteralPath $ResourceRoot -PathType Container) -or
    -not (Test-Path -LiteralPath $ServerResourceRoot -PathType Container)) {
    throw 'PlayDH or server resource root is missing.'
}
$resourceManifestPath = Join-Path $stageRoot 'reference\manifests\playdh-current.sha256.json'
if ((Get-FileHash -LiteralPath $resourceManifestPath -Algorithm SHA256).Hash -ine
    [string]$writerContract.resourceManifestSha256) {
    throw 'Stage writer contract resource digest mismatch.'
}
$resourceManifest = Get-Content -LiteralPath $resourceManifestPath -Raw | ConvertFrom-Json
if ([string]$resourceManifest.profileId -ne 'playdh-current' -or
    @($resourceManifest.files).Count -ne [int]$resourceManifest.fileCount) {
    throw 'PlayDH resource manifest is invalid.'
}
$actualResourceFiles = @(Get-ChildItem -LiteralPath $ResourceRoot -File -Recurse)
$actualResourceBytes = [int64](($actualResourceFiles | Measure-Object -Property Length -Sum).Sum)
if ($actualResourceFiles.Count -ne [int]$resourceManifest.fileCount -or
    $actualResourceBytes -ne [int64]$resourceManifest.byteCount) {
    throw 'PlayDH resource file count or byte count mismatch.'
}
foreach ($entry in @($resourceManifest.files)) {
    $relative = [string]$entry.path
    if ([IO.Path]::IsPathRooted($relative) -or $relative.Split('/') -contains '..') {
        throw "Unsafe PlayDH path: $relative"
    }
    $path = Join-Path $ResourceRoot $relative.Replace('/', '\')
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "PlayDH file missing: $relative" }
    $file = Get-Item -LiteralPath $path
    if ($file.Length -ne [int64]$entry.bytes -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine [string]$entry.sha256) {
        throw "PlayDH hash mismatch: $relative"
    }
}

$bin = Join-Path $stageRoot 'modern\build\tools'
$runtime = Join-Path $stageRoot 'runtime'
$mapExe = Join-Path $bin 'MoxianMapServer\mxh_map_server_CHINA.exe'
$agentExe = Join-Path $bin 'MoxianAgentServer\mxh_agent_server_CHINA.exe'
$loginExe = Join-Path $bin 'MoxianLoginServer\mxh_login_server.exe'
$agentArgs = @(
    '--port', $AgentPort,
    '--bind-address', '192.168.2.244',
    '--map-server', "192.168.2.244:$MapPort",
    '--default-map', $MapNumber,
    '--backend', 'mssql_odbc',
    '--db-env', $DatabaseConfigEnv,
    '--legacy'
)
$serverSpecs = @(
    [pscustomobject]@{ name = 'map'; exe = $mapExe; port = $MapPort; args = @('--port', $MapPort, '--map', $MapNumber, '--bind-address', '192.168.2.244', '--resource-root', $ResourceRoot, '--server-resource-root', $ServerResourceRoot, '--resource-profile', 'playdh-current', '--backend', 'mssql_odbc', '--db-env', $DatabaseConfigEnv) }
)
if ($EnableMap17) {
    $agentArgs = @(
        '--port', $AgentPort,
        '--bind-address', '192.168.2.244',
        '--map-server', "192.168.2.244:$MapPort",
        '--map-server-map', "17=192.168.2.244:$Map17Port",
        '--default-map', $MapNumber,
        '--start-area', '17',
        '--backend', 'mssql_odbc',
        '--db-env', $DatabaseConfigEnv,
        '--legacy'
    )
    $serverSpecs += [pscustomobject]@{ name = 'map17'; exe = $mapExe; port = $Map17Port; args = @('--port', $Map17Port, '--map', 17, '--bind-address', '192.168.2.244', '--resource-root', $ResourceRoot, '--server-resource-root', $ServerResourceRoot, '--resource-profile', 'playdh-current', '--backend', 'mssql_odbc', '--db-env', $DatabaseConfigEnv) }
}
$serverSpecs += @(
    [pscustomobject]@{ name = 'agent'; exe = $agentExe; port = $AgentPort; args = $agentArgs },
    [pscustomobject]@{ name = 'login'; exe = $loginExe; port = $LoginPort; args = @('--port', $LoginPort, '--bind-address', '192.168.2.244', '--agent-addr', $AdvertisedAgentAddress, '--agent-port', $AgentPort, '--backend', 'mssql_odbc', '--db-env', $DatabaseConfigEnv, '--legacy') }
)

if (-not $Start) {
    [ordered]@{
        mode = 'plan-only'
        stage = $stageRoot
        sourceHead = [string]$manifest.sourceHead
        sourcePatchSha256 = [string]$manifest.sourcePatchSha256
        resourceProfile = 'playdh-current'
        resourceRoot = $ResourceRoot
        resourceFilesVerified = @($resourceManifest.files).Count
        resourcePaksVerified = @($manifest.resourcePaks).Count
        databaseHostRequired = $true
        databaseConfigEnv = $DatabaseConfigEnv
        startsProcesses = $false
        runsMigration = $false
        map17Enabled = [bool]$EnableMap17
        servers = @($serverSpecs | ForEach-Object { [ordered]@{ name = $_.name; port = $_.port; exe = $_.exe; args = $_.args } })
    } | ConvertTo-Json -Depth 5
    return
}

if ($DatabaseHost -ne '192.168.2.238') {
    throw 'This VM112 stage may only use the isolated CT930 database host 192.168.2.238.'
}
$databaseConfig = [Environment]::GetEnvironmentVariable($DatabaseConfigEnv, 'Process')
if ([string]::IsNullOrWhiteSpace($databaseConfig)) {
    throw "Protected process environment $DatabaseConfigEnv is missing."
}
function Assert-OriginalProtectedConfigValue {
    param([string]$Config, [string]$Name, [string]$Expected)
    $pattern = '(?i)(?:^|;)\s*' + [regex]::Escape($Name) + '\s*=\s*([^;]+)'
    $found = [regex]::Matches($Config, $pattern)
    if ($found.Count -ne 1 -or
        -not [StringComparer]::OrdinalIgnoreCase.Equals(
            $found[0].Groups[1].Value.Trim(), $Expected)) {
        throw "Protected database configuration has a missing, duplicate, or mismatched $Name value."
    }
}
Assert-OriginalProtectedConfigValue $databaseConfig 'backend' 'mssql_odbc'
Assert-OriginalProtectedConfigValue $databaseConfig 'host' $DatabaseHost
Assert-OriginalProtectedConfigValue $databaseConfig 'database' 'MoxiangOriginalR18Test_20261009'
Assert-OriginalProtectedConfigValue $databaseConfig 'encrypt' 'yes'
Assert-OriginalProtectedConfigValue $databaseConfig 'trust_server_certificate' 'no'
foreach ($port in $listenPorts) {
    if (@(Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue).Count -gt 0) {
        throw "Port $port is already listening; refusing to take over an existing service."
    }
}

$statePath = Join-Path $stageRoot 'run\pids.json'
if (Test-Path -LiteralPath $statePath -PathType Leaf) {
    throw 'Existing stage PID manifest found; inspect it before starting another run.'
}
$env:PATH = "$runtime;$env:PATH"
& (Join-Path $PSScriptRoot 'assert_original_writer_contract.ps1') `
    -DbToolPath (Join-Path $bin 'MoxianDbTool\mxh_db_tool.exe') `
    -DatabaseConfigEnv $DatabaseConfigEnv
if (-not $?) {
    throw 'Isolated MSSQL writer contract check failed.'
}
$logRoot = Join-Path $stageRoot 'run\logs'
$null = New-Item -ItemType Directory -Path $logRoot -Force
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class OriginalOwnedProcessImage {
    [DllImport("kernel32.dll", EntryPoint="QueryFullProcessImageNameW", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern bool QueryFullProcessImageName(IntPtr process, int flags, StringBuilder path, ref int length);
}
'@
function Assert-OriginalOwnedProcessImage {
    param([Diagnostics.Process]$Process, [string]$ExpectedPath)
    $lastErrorCode = 0
    for ($attempt = 0; $attempt -lt 10; $attempt++) {
        if ($Process.HasExited) {
            throw "Owned process $($Process.Id) exited before image check; exit code $($Process.ExitCode)."
        }
        $path = [Text.StringBuilder]::new(1024)
        $length = 1024
        if ([OriginalOwnedProcessImage]::QueryFullProcessImageName($Process.Handle, 0, $path, [ref]$length)) {
            if (-not [StringComparer]::OrdinalIgnoreCase.Equals($path.ToString(), $ExpectedPath)) {
                throw "Owned process $($Process.Id) image path differs from its fixed stage executable."
            }
            return
        }
        $lastErrorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        Start-Sleep -Milliseconds 50
    }
    throw "Owned process $($Process.Id) image path remained unreadable; Win32 error $lastErrorCode."
}
function Write-OriginalOwnedPidManifest {
    param([string]$Path, [object[]]$Entries, [bool]$Initial)
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes((ConvertTo-Json -InputObject @($Entries) -Depth 4) + "`n")
    if ($Initial) {
        $stream = [IO.FileStream]::new($Path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
        try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) }
        finally { $stream.Dispose() }
    } else {
        $suffix = [Guid]::NewGuid().ToString('N')
        $temp = $Path + '.' + $suffix + '.tmp'
        $backup = $Path + '.' + $suffix + '.bak'
        $stream = [IO.FileStream]::new($temp, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
        try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) }
        finally { $stream.Dispose() }
        [IO.File]::Replace($temp, $Path, $backup)
    }
}
function Stop-OriginalOwnedProcess {
    param([Diagnostics.Process]$Process)
    if (-not $Process.HasExited) {
        try { $Process.Kill() }
        catch { if (-not $Process.HasExited) { throw } }
        if (-not $Process.WaitForExit(30000) -or -not $Process.HasExited) {
            throw "Owned process $($Process.Id) did not exit."
        }
    }
}
$owned = @()
$started = @()
try {
    foreach ($spec in $serverSpecs) {
        $process = Start-Process -FilePath $spec.exe -ArgumentList $spec.args -WorkingDirectory (Split-Path -Parent $spec.exe) -RedirectStandardOutput (Join-Path $logRoot "$($spec.name).out.log") -RedirectStandardError (Join-Path $logRoot "$($spec.name).err.log") -WindowStyle Hidden -PassThru
        $started += $process
        $null = $process.Handle
        Assert-OriginalOwnedProcessImage -Process $process -ExpectedPath $spec.exe
        $owned += [ordered]@{ name = $spec.name; pid = $process.Id; exe = $spec.exe; port = $spec.port;
            startUtcTicks = $process.StartTime.ToUniversalTime().Ticks;
            exeSha256 = (Get-FileHash -LiteralPath $spec.exe -Algorithm SHA256).Hash.ToLowerInvariant() }
        Write-OriginalOwnedPidManifest -Path $statePath -Entries $owned -Initial ($owned.Count -eq 1)
        Start-Sleep -Milliseconds 500
        if ($process.HasExited) { throw "$($spec.name) exited during startup; inspect stage logs." }
    }
} catch {
    $startError = $_
    for ($index = $started.Count - 1; $index -ge 0; $index--) {
        $process = $started[$index]
        try { Stop-OriginalOwnedProcess -Process $process }
        catch { Write-Warning "Owned startup cleanup failed for $($process.Id): $($_.FullyQualifiedErrorId)" }
    }
    throw $startError
}
Write-Host "VM112 original three-service stack started; PID manifest: $statePath"
