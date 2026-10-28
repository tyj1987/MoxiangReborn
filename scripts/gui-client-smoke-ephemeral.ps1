# gui-client-smoke-ephemeral.ps1
# Helpers for scripts/gui-client-smoke.ps1 -EphemeralLocalServer.
#
# This file does not start servers when it is dot-sourced. The caller starts
# processes only after Get-EphemeralSafetyViolation returns nothing.
#
# Safety gate (any hit aborts before a process is started):
# - Listen and client-connect addresses must be exactly 127.0.0.1.
# - The database must be sqlite in a new directory directly under
#   [IO.Path]::GetTempPath(), file name moxian.db.
# - Server arguments, the database config, client arguments, and the
#   environment block handed to the server are scanned for MSSQL/ODBC
#   connection-string markers (Server=, Data Source=, mssql, odbc) and for
#   any address other than 127.0.0.1.
# - PATH, PATHEXT and PSMODULEPATH are process runtime path lists. They are
#   still scanned for Server=, Data Source= and non-loopback addresses, but
#   not for the bare substrings mssql/odbc (those appear in SQL client
#   install paths and are not connection strings).
# - If MXH_DATABASE_CONFIG is already set and is not sqlite, abort.
# - --login-host must be 127.0.0.1. MXH_LOGIN_HOST, when set, must be the
#   same. A remote target is mutually exclusive with this mode.
# - Cleanup stops only the PIDs this script recorded, then deletes that
#   temp directory. The temp root itself is never deleted.

function Get-EphemeralNormalizedTempRoot {
    $root = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    return $root.TrimEnd('\', '/')
}

function Test-EphemeralDeletableDatabaseDirectory {
    param([string]$Directory)
    if ([string]::IsNullOrWhiteSpace($Directory)) { return $false }
    $tempRoot = Get-EphemeralNormalizedTempRoot
    $full = [IO.Path]::GetFullPath($Directory).TrimEnd('\', '/')
    if ([string]::Equals($full, $tempRoot, [StringComparison]::OrdinalIgnoreCase)) { return $false }
    $parent = [IO.Path]::GetDirectoryName($full)
    if ([string]::IsNullOrWhiteSpace($parent)) { return $false }
    $parent = $parent.TrimEnd('\', '/')
    return [string]::Equals($parent, $tempRoot, [StringComparison]::OrdinalIgnoreCase)
}

function Test-EphemeralSqliteDatabaseConfig {
    param([string]$Value)
    if ([string]::IsNullOrWhiteSpace($Value)) { return $false }
    if ($Value -match '(?i)Server=|Data Source=|mssql|odbc') { return $false }
    return [bool]($Value -match '(?i)(^|;)sqlite(;|$)|backend\s*=\s*sqlite\b')
}

function Find-EphemeralIpv4Addresses {
    param([string]$Text)
    # Exactly four dotted segments, each 0-255. The match cannot sit against
    # a digit or '.' . A letter, '_' or '-' is also a boundary so a version
    # token such as jdk-17.0.10.7-hotspot is not an address; digit and '.'
    # alone would still accept 17.0.10.7 inside that name.
    $found = @()
    if ([string]::IsNullOrEmpty($Text)) { return $found }
    foreach ($match in [regex]::Matches($Text, '(?<![0-9A-Za-z_.-])(?:[0-9]{1,3}\.){3}[0-9]{1,3}(?![0-9A-Za-z_.-])')) {
        $valid = $true
        foreach ($part in $match.Value.Split('.')) {
            $number = 0
            if (-not [int]::TryParse($part, [ref]$number) -or $number -lt 0 -or $number -gt 255) {
                $valid = $false
                break
            }
        }
        if ($valid) { $found += $match.Value }
    }
    return $found
}

function Test-EphemeralNonLoopbackText {
    param([string]$Text)
    if ([string]::IsNullOrEmpty($Text)) { return $false }
    foreach ($address in @(Find-EphemeralIpv4Addresses $Text)) {
        if ($address -ne '127.0.0.1') { return $true }
    }
    if ($Text -match '(?i)(?<![A-Za-z0-9])localhost(?![A-Za-z0-9])') { return $true }
    if ($Text -match '(?<![A-Za-z0-9:])::1(?![A-Za-z0-9:])') { return $true }
    return $false
}

function Test-EphemeralMssqlMarker {
    param(
        [string]$Text,
        [switch]$AssignmentOnly
    )
    if ([string]::IsNullOrEmpty($Text)) { return $false }
    if ($AssignmentOnly) {
        return [bool]($Text -match '(?i)Server=|Data Source=')
    }
    return [bool]($Text -match '(?i)Server=|Data Source=|mssql|odbc')
}

function Get-EphemeralSafetyViolation {
    param(
        [string[]]$ListenAddresses = @(),
        [string[]]$ClientConnectAddresses = @(),
        [string]$DatabaseDirectory = '',
        [string]$DatabaseFile = '',
        [string]$DatabaseConfig = '',
        [string]$RegisterDatabaseArgument = '',
        [string[]]$ServerArguments = @(),
        [hashtable]$ServerEnvironment = @{},
        [string[]]$ClientArguments = @(),
        [AllowEmptyString()][AllowNull()][string]$ExistingDatabaseConfig,
        [AllowEmptyString()][AllowNull()][string]$ExistingLoginHost
    )

    if (@($ListenAddresses).Count -lt 1) { return 'ephemeral safety: non-loopback' }
    foreach ($address in @($ListenAddresses)) {
        if ([string]$address -ne '127.0.0.1') { return 'ephemeral safety: non-loopback' }
    }
    if (@($ClientConnectAddresses).Count -lt 1) { return 'ephemeral safety: non-loopback' }
    foreach ($address in @($ClientConnectAddresses)) {
        if ([string]$address -ne '127.0.0.1') { return 'ephemeral safety: non-loopback' }
    }

    $loginHostSeen = 0
    $loginHost = $null
    $clientArgs = @($ClientArguments)
    for ($i = 0; $i -lt $clientArgs.Count; $i++) {
        if ([string]$clientArgs[$i] -eq '--login-host') {
            $loginHostSeen++
            if ($i + 1 -lt $clientArgs.Count) { $loginHost = [string]$clientArgs[$i + 1] }
        }
    }
    if ($loginHostSeen -ne 1 -or $loginHost -ne '127.0.0.1') {
        return 'ephemeral safety: remote-login-host'
    }
    if (-not [string]::IsNullOrWhiteSpace($ExistingLoginHost) -and $ExistingLoginHost -ne '127.0.0.1') {
        return 'ephemeral safety: remote-login-host'
    }

    # Marker and address scans run before the temp-path equality check so a
    # MSSQL connection string is reported as a marker even when it also is
    # not the temp sqlite file.
    $texts = @([string[]]$ServerArguments) + @([string[]]$ClientArguments) + @($DatabaseConfig, $RegisterDatabaseArgument)
    foreach ($text in $texts) {
        if (Test-EphemeralMssqlMarker ([string]$text)) { return 'ephemeral safety: mssql-marker' }
        if (Test-EphemeralNonLoopbackText ([string]$text)) { return 'ephemeral safety: non-loopback' }
    }
    foreach ($key in @($ServerEnvironment.Keys)) {
        $name = [string]$key
        $value = [string]$ServerEnvironment[$key]
        $runtimePath = $name -match '^(?i:PATH|PATHEXT|PSMODULEPATH)$'
        if ($runtimePath) {
            if (Test-EphemeralMssqlMarker $value -AssignmentOnly) { return "ephemeral safety: mssql-marker ($name)" }
        } elseif (Test-EphemeralMssqlMarker $value) {
            return "ephemeral safety: mssql-marker ($name)"
        }
        if (Test-EphemeralNonLoopbackText $value) { return "ephemeral safety: non-loopback ($name)" }
    }

    if (-not (Test-EphemeralDeletableDatabaseDirectory $DatabaseDirectory)) {
        return 'ephemeral safety: database-not-temp'
    }
    if ([string]::IsNullOrWhiteSpace($DatabaseFile) -or $DatabaseFile.Contains(';') -or $DatabaseFile.Contains('=')) {
        return 'ephemeral safety: database-not-temp'
    }
    $directoryFull = [IO.Path]::GetFullPath($DatabaseDirectory).TrimEnd('\', '/')
    $fileFull = [IO.Path]::GetFullPath($DatabaseFile)
    $prefix = $directoryFull + [IO.Path]::DirectorySeparatorChar
    if (-not $fileFull.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        return 'ephemeral safety: database-not-temp'
    }
    if ([IO.Path]::GetFileName($fileFull) -ne 'moxian.db') {
        return 'ephemeral safety: database-not-temp'
    }
    $expectedConfig = 'backend=sqlite;path=' + $DatabaseFile
    $expectedRegister = 'sqlite;path=' + $DatabaseFile
    if ($DatabaseConfig -ne $expectedConfig -or $RegisterDatabaseArgument -ne $expectedRegister) {
        return 'ephemeral safety: database-not-temp'
    }
    if (-not (Test-EphemeralSqliteDatabaseConfig $DatabaseConfig) -or -not (Test-EphemeralSqliteDatabaseConfig $RegisterDatabaseArgument)) {
        return 'ephemeral safety: database-not-sqlite'
    }
    if (-not [string]::IsNullOrWhiteSpace($ExistingDatabaseConfig) -and -not (Test-EphemeralSqliteDatabaseConfig $ExistingDatabaseConfig)) {
        return 'ephemeral safety: database-not-sqlite'
    }
    return $null
}

function Copy-EphemeralProcessEnvironment {
    $map = @{}
    foreach ($entry in [Environment]::GetEnvironmentVariables('Process').GetEnumerator()) {
        $map[[string]$entry.Key] = [string]$entry.Value
    }
    return $map
}

function Copy-EphemeralEnvironmentForProcess {
    param(
        [hashtable]$Source,
        [string]$ProcessName
    )
    $copy = @{}
    foreach ($key in @($Source.Keys)) {
        $copy[[string]$key] = [string]$Source[$key]
    }
    $copy['MXH_PROCESS'] = $ProcessName
    return $copy
}

function Get-GuiSmokeCollapsedPathText {
    # Text-only Windows path collapse used for the D:\MX check. Does not touch
    # the filesystem. Returns e.g. 'D:\MX\PlayDH' for 'd:/x/../MX/./PlayDH/'.
    param([AllowEmptyString()][string]$Path)
    if ([string]::IsNullOrEmpty($Path)) { return '' }
    $text = $Path -replace '/', '\'
    $text = $text -replace '^\\\\[?.]\\', ''
    $prefix = ''
    if ($text -match '^([A-Za-z]:)(.*)$') {
        $prefix = $Matches[1].ToUpperInvariant()
        $text = $Matches[2]
    } elseif ($text.StartsWith('\\')) {
        $prefix = '\\'
        $text = $text.Substring(2)
    }
    $stack = [System.Collections.Generic.List[string]]::new()
    foreach ($segment in $text.Split('\')) {
        if ($segment -eq '' -or $segment -eq '.') { continue }
        if ($segment -eq '..') {
            if ($stack.Count -gt 0) { $stack.RemoveAt($stack.Count - 1) }
            continue
        }
        $stack.Add($segment)
    }
    if ($prefix -eq '\\') { return '\\' + ($stack -join '\') }
    return $prefix + '\' + ($stack -join '\')
}

function Test-GuiSmokePathUnderOriginalRoot {
    param([AllowEmptyString()][string]$CollapsedPath)
    if ([string]::IsNullOrEmpty($CollapsedPath)) { return $false }
    $trimmed = $CollapsedPath.TrimEnd('\')
    return ($trimmed -ieq 'D:\MX') -or $trimmed.StartsWith('D:\MX\', [StringComparison]::OrdinalIgnoreCase)
}

function Resolve-GuiSmokeResourceRoot {
    # -ResourceRoot on gui-client-smoke.ps1. Empty keeps the repo PlayDH
    # (<RepoRoot>\modern\data\PlayDH), exactly as before. A value must be an
    # absolute path to an existing directory outside D:\MX; it is returned unchanged so the
    # client's --resource-root, the map server's --resource-root and the
    # optional audits all read the same tree (sprite atlas and pak mounts in
    # mxh_client both follow --resource-root).
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        [AllowEmptyString()][AllowNull()][string]$ResourceRoot
    )
    if ([string]::IsNullOrWhiteSpace($ResourceRoot)) {
        return (Join-Path $RepoRoot 'modern\data\PlayDH')
    }
    # D:\MX is the read-only original install. The check runs on a collapsed
    # copy of the text (as clean-deploy -PlayDhRoot does): '/' becomes '\',
    # a \\?\ or \\.\ prefix is dropped, '.' and empty segments are removed,
    # '..' pops a segment, and the compare ignores case. So D:\x\..\MX\PlayDH,
    # d:\mx\ and D:\MX\. are refused, while D:\MX\..\x (= D:\x) passes. It is
    # text-only, so the refusal also happens on a non-Windows host.
    $collapsed = Get-GuiSmokeCollapsedPathText -Path $ResourceRoot
    if (Test-GuiSmokePathUnderOriginalRoot -CollapsedPath $collapsed) {
        throw "[ERROR] gui smoke: -ResourceRoot resolves under D:\MX (read-only original install): $ResourceRoot -> $collapsed"
    }
    $qualifiedMethod = [IO.Path].GetMethod('IsPathFullyQualified', [Type[]]@([string]))
    if ($null -ne $qualifiedMethod) {
        $qualified = [IO.Path]::IsPathFullyQualified($ResourceRoot)
    } else {
        # Windows PowerShell 5.1: rooted, and not \foo or C:foo (drive-relative).
        $qualified = [IO.Path]::IsPathRooted($ResourceRoot) -and ($ResourceRoot -notmatch '^[\\/](?![\\/])') -and ($ResourceRoot -notmatch '^[A-Za-z]:(?![\\/])')
    }
    if (-not $qualified) {
        throw "[ERROR] gui smoke: -ResourceRoot must be an absolute path: $ResourceRoot"
    }
    if (-not (Test-Path -LiteralPath $ResourceRoot -PathType Container)) {
        throw "[ERROR] gui smoke: -ResourceRoot does not exist: $ResourceRoot"
    }
    # A junction or symlink whose target is under D:\MX is refused as well.
    $item = Get-Item -LiteralPath $ResourceRoot -Force
    $target = $null
    if ($item.PSObject.Properties.Name -contains 'LinkTarget') { $target = $item.LinkTarget }
    if (-not [string]::IsNullOrWhiteSpace([string]$target) -and
        (Test-GuiSmokePathUnderOriginalRoot -CollapsedPath (Get-GuiSmokeCollapsedPathText -Path ([string]$target)))) {
        throw "[ERROR] gui smoke: -ResourceRoot resolves under D:\MX (read-only original install): $ResourceRoot -> $target"
    }
    return $ResourceRoot
}

function Get-EphemeralResourceLayout {
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        # Optional explicit PlayDH root from gui-client-smoke.ps1 -ResourceRoot.
        [AllowEmptyString()][AllowNull()][string]$ResourceRoot = ''
    )
    $manifestPath = Join-Path $RepoRoot 'deploy\resource-profiles.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Missing resource profile manifest: $manifestPath"
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding utf8 | ConvertFrom-Json
    $profile = $manifest.profiles.'playdh-current'
    if ($null -eq $profile) { throw 'playdh-current resource profile is missing' }
    if ([string]::IsNullOrWhiteSpace($ResourceRoot)) {
        $resourceRoot = Join-Path $RepoRoot ([string]$profile.source)
    } else {
        $resourceRoot = $ResourceRoot
    }
    $configured = [Environment]::GetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT', 'Process')
    if ([string]::IsNullOrWhiteSpace($configured)) {
        $serverRoot = Join-Path $resourceRoot 'Resource\Server'
    } else {
        $serverRoot = $configured
    }
    [pscustomobject]@{
        ResourceRoot = $resourceRoot
        ServerResourceRoot = $serverRoot
        ResourceProfileId = 'playdh-current'
    }
}

function Resolve-EphemeralServerExecutable {
    param(
        [Parameter(Mandatory)][string]$BuildRoot,
        [Parameter(Mandatory)][string]$Configuration,
        [Parameter(Mandatory)][string]$ToolDirectory,
        [Parameter(Mandatory)][string]$FileName
    )
    $candidates = @(
        (Join-Path $BuildRoot "tools\$ToolDirectory\$FileName"),
        (Join-Path $BuildRoot "tools\$ToolDirectory\$Configuration\$FileName")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Missing $Configuration executable: $FileName (checked $($candidates -join '; '))"
}

function Get-EphemeralReservedPorts {
    # Modern defaults, the r18 triple, and the r2 map range.
    return @(16001, 17001, 18001, 36001, 37001, 38001, 36101, 37101, 38101, 38102, 38103)
}

function Resolve-EphemeralPortPlan {
    param([Parameter(Mandatory)][int]$Base)
    # login = base, agent = base+1000, map = base+2000, map17 = base+2002.
    # 28002 stays free for the optional map12 listener used by other probes.
    if ($Base -lt 1 -or ($Base + 2002) -gt 65535) {
        throw "ephemeral safety: port-out-of-range $Base"
    }
    $login = $Base
    $agent = $Base + 1000
    $map = $Base + 2000
    $map17 = $Base + 2002
    $reserved = Get-EphemeralReservedPorts
    foreach ($port in @($login, $agent, $map, $map17)) {
        if ($reserved -contains $port) {
            throw "ephemeral safety: reserved-port $port"
        }
    }
    [pscustomobject]@{
        LoginPort = $login
        AgentPort = $agent
        MapPort = $map
        Map17Port = $map17
        Ports = @($login, $agent, $map, $map17)
    }
}

function Get-EphemeralActiveTcpListeners {
    # Separate so the SelfTest can make the enumeration fail.
    return @([System.Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveTcpListeners())
}

function Test-EphemeralPortOccupied {
    param([Parameter(Mandatory)][int]$Port)
    # Occupied if either holds:
    # 1. GetActiveTcpListeners() lists the port on 0.0.0.0, 127.0.0.1, :: or ::1
    #    (main check: Windows does not set ExclusiveAddressUse by default, so a
    #    127.0.0.1 bind can still succeed while another socket holds 0.0.0.0).
    #    If the enumeration itself fails, the port is treated as occupied
    #    (fail closed) and a [WARN] line is written;
    # 2. an ExclusiveAddressUse bind of 127.0.0.1 fails.
    # We never bind 0.0.0.0 ourselves: on Windows that can raise a firewall prompt.
    $watched = @(
        [System.Net.IPAddress]::Any,
        [System.Net.IPAddress]::Loopback,
        [System.Net.IPAddress]::IPv6Any,
        [System.Net.IPAddress]::IPv6Loopback
    )
    try {
        $active = Get-EphemeralActiveTcpListeners
    } catch {
        [Console]::Error.WriteLine("[WARN] listener enumeration failed (port $Port treated as in use): $($_.Exception.Message)")
        return $true
    }
    foreach ($endpoint in @($active)) {
        if ($endpoint.Port -ne $Port) { continue }
        foreach ($address in $watched) {
            if ($endpoint.Address.Equals($address)) { return $true }
        }
    }
    $listener = $null
    try {
        $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, $Port)
        $listener.ExclusiveAddressUse = $true
        $listener.Start()
    } catch {
        return $true
    } finally {
        if ($null -ne $listener) {
            try { $listener.Stop() } catch {}
        }
    }
    return $false
}

function Get-EphemeralPortInUseViolation {
    param([Parameter(Mandatory)][int[]]$Ports)
    foreach ($port in @($Ports)) {
        if (Test-EphemeralPortOccupied -Port $port) {
            return "ephemeral safety: port-in-use $port"
        }
    }
    return $null
}

function Get-EphemeralReadinessFailure {
    param([Parameter(Mandatory)]$Servers)
    foreach ($server in @($Servers)) {
        $proc = $server.Process
        if ($null -ne $proc) {
            try { $proc.Refresh() } catch {}
            if ($proc.HasExited) {
                $stderr = [string]$server.Stderr
                $suffix = ''
                if (-not [string]::IsNullOrWhiteSpace($stderr)) { $suffix = "; see $stderr" }
                return "$($server.Name) exited during startup (exit=$($proc.ExitCode)$suffix)"
            }
        }
        $port = [int]$server.Port
        if (-not (Test-EphemeralLoopbackPort -Port $port)) {
            return "$($server.Name) did not accept connections on 127.0.0.1:${port}"
        }
    }
    return $null
}

function New-EphemeralServerLaunchSpec {
    param(
        [Parameter(Mandatory)][string]$MapExecutable,
        [Parameter(Mandatory)][string]$AgentExecutable,
        [Parameter(Mandatory)][string]$LoginExecutable,
        [Parameter(Mandatory)][int]$MapNumber,
        [Parameter(Mandatory)][string]$ResourceRoot,
        [Parameter(Mandatory)][string]$ServerResourceRoot,
        [Parameter(Mandatory)][string]$DatabaseDirectory,
        [string]$ResourceProfileId = 'playdh-current',
        [int]$LoginPort = 26001,
        [int]$AgentPort = 27001,
        [int]$MapPort = 28001,
        [int]$Map17Port = 0,
        [hashtable]$ProcessEnvironment = @{},
        [Parameter(Mandatory)][string[]]$ClientArguments
    )
    if ($Map17Port -eq 0) { $Map17Port = $MapPort + 2 }
    $reserved = Get-EphemeralReservedPorts
    $listenPorts = @($LoginPort, $AgentPort, $MapPort)
    if ($MapNumber -ne 17) { $listenPorts += $Map17Port }
    foreach ($port in @($listenPorts)) {
        if ($port -lt 1 -or $port -gt 65535) {
            throw "ephemeral safety: port-out-of-range $port"
        }
        if ($reserved -contains $port) {
            throw "ephemeral safety: reserved-port $port"
        }
    }
    $distinct = @($listenPorts | Select-Object -Unique)
    if ($distinct.Count -ne @($listenPorts).Count) {
        throw "ephemeral safety: port-out-of-range $Map17Port"
    }

    # Command line matches deploy/scripts/start_modern.ps1 $processes
    # (map / agent / login, about lines 363-379). BindAddress there defaults
    # to 0.0.0.0 (line 15); this mode forces every listen and connect address
    # to 127.0.0.1. Database config matches lines 343-345 and 354.
    # gui-client-smoke.ps1 does not pass -UseHsel or -AllowDevFallbacks, and
    # Locale stays the start_modern.ps1 default CHINA (line 6, exe names 339-340).
    $databaseFile = Join-Path $DatabaseDirectory 'moxian.db'
    $databaseConfig = 'backend=sqlite;path=' + $databaseFile
    $registerArgument = 'sqlite;path=' + $databaseFile
    $commonDb = @('--backend', 'sqlite', '--db-env', 'MXH_DATABASE_CONFIG')
    $legacy = @('--legacy')
    $mapArgs = @(
        '--port', "$MapPort",
        '--map', "$MapNumber",
        '--bind-address', '127.0.0.1',
        '--resource-root', $ResourceRoot,
        '--server-resource-root', $ServerResourceRoot,
        '--resource-profile', $ResourceProfileId
    ) + $commonDb
    $agentArgs = @(
        '--port', "$AgentPort",
        '--bind-address', '127.0.0.1',
        '--map-server', "127.0.0.1:$MapPort",
        '--default-map', "$MapNumber",
        '--start-area', '17'
    ) + $commonDb + $legacy
    if ($MapNumber -ne 17) {
        $agentArgs += @('--map-server-map', "17=127.0.0.1:$Map17Port")
    }
    $map17Args = @(
        '--port', "$Map17Port",
        '--map', '17',
        '--bind-address', '127.0.0.1',
        '--resource-root', $ResourceRoot,
        '--server-resource-root', $ServerResourceRoot,
        '--resource-profile', $ResourceProfileId
    ) + $commonDb
    $loginArgs = @(
        '--port', "$LoginPort",
        '--bind-address', '127.0.0.1',
        '--agent-addr', '127.0.0.1',
        '--agent-port', "$AgentPort"
    ) + $commonDb + $legacy

    $environment = @{}
    $existingDatabaseConfig = ''
    $existingLoginHost = ''
    foreach ($key in @($ProcessEnvironment.Keys)) {
        $name = [string]$key
        $environment[$name] = [string]$ProcessEnvironment[$key]
        if ($name -eq 'MXH_DATABASE_CONFIG') { $existingDatabaseConfig = [string]$ProcessEnvironment[$key] }
        if ($name -eq 'MXH_LOGIN_HOST') { $existingLoginHost = [string]$ProcessEnvironment[$key] }
    }
    $environment['MXH_DATABASE_CONFIG'] = $databaseConfig

    # Map17 is the original new-character StartArea. The primary -MapNumber
    # process stays in place for an existing character saved on that map.
    # start_modern.ps1 is not the local smoke launcher and is left unchanged.
    $processes = @(
        [pscustomobject]@{ Name = 'map'; Executable = $MapExecutable; Port = $MapPort; Arguments = $mapArgs }
    )
    if ($MapNumber -ne 17) {
        $processes += [pscustomobject]@{ Name = 'map17'; Executable = $MapExecutable; Port = $Map17Port; Arguments = $map17Args }
    }
    $processes += @(
        [pscustomobject]@{ Name = 'agent'; Executable = $AgentExecutable; Port = $AgentPort; Arguments = $agentArgs },
        [pscustomobject]@{ Name = 'login'; Executable = $LoginExecutable; Port = $LoginPort; Arguments = $loginArgs }
    )
    $listenAddresses = @($processes | ForEach-Object { '127.0.0.1' })
    $serverArguments = @($mapArgs) + @($agentArgs) + @($loginArgs)
    if ($MapNumber -ne 17) { $serverArguments = @($mapArgs) + @($map17Args) + @($agentArgs) + @($loginArgs) }
    $violation = Get-EphemeralSafetyViolation `
        -ListenAddresses $listenAddresses `
        -ClientConnectAddresses @('127.0.0.1', '127.0.0.1', '127.0.0.1') `
        -DatabaseDirectory $DatabaseDirectory `
        -DatabaseFile $databaseFile `
        -DatabaseConfig $databaseConfig `
        -RegisterDatabaseArgument $registerArgument `
        -ServerArguments $serverArguments `
        -ServerEnvironment $environment `
        -ClientArguments $ClientArguments `
        -ExistingDatabaseConfig $existingDatabaseConfig `
        -ExistingLoginHost $existingLoginHost
    if ($violation) { throw $violation }

    [pscustomobject]@{
        DatabaseDirectory = $DatabaseDirectory
        DatabaseFile = $databaseFile
        DatabaseConfig = $databaseConfig
        RegisterDatabaseArgument = $registerArgument
        Environment = $environment
        Processes = $processes
    }
}

function Format-EphemeralCleanupLine {
    param(
        [bool]$PidsStopped,
        [bool]$DbRemoved
    )
    $pids = if ($PidsStopped) { 'yes' } else { 'no' }
    $database = if ($DbRemoved) { 'yes' } else { 'no' }
    return "[ephemeral] cleanup pids_stopped=$pids db_removed=$database"
}

function Test-EphemeralLoopbackPort {
    param([Parameter(Mandatory)][int]$Port)
    $tcp = [Net.Sockets.TcpClient]::new()
    try {
        $task = $tcp.ConnectAsync('127.0.0.1', $Port)
        if (-not $task.Wait(1000)) { return $false }
        return [bool]$tcp.Connected
    } catch {
        return $false
    } finally {
        $tcp.Dispose()
    }
}

function Invoke-EphemeralLocalCleanup {
    param(
        [int[]]$ProcessIds = @(),
        [string]$DatabaseDirectory = '',
        [string]$LogFile = ''
    )
    $ids = @($ProcessIds | Where-Object { $_ -gt 0 -and $_ -ne $PID })
    foreach ($procId in $ids) {
        $alive = Get-Process -Id $procId -ErrorAction SilentlyContinue
        if ($null -ne $alive) {
            Stop-Process -Id $procId -Force -ErrorAction SilentlyContinue
            try { Wait-Process -Id $procId -Timeout 5 -ErrorAction SilentlyContinue } catch {}
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($DatabaseDirectory) -and (Test-EphemeralDeletableDatabaseDirectory $DatabaseDirectory)) {
        for ($attempt = 0; $attempt -lt 10; $attempt++) {
            if (-not (Test-Path -LiteralPath $DatabaseDirectory)) { break }
            Remove-Item -LiteralPath $DatabaseDirectory -Recurse -Force -ErrorAction SilentlyContinue
            if (-not (Test-Path -LiteralPath $DatabaseDirectory)) { break }
            Start-Sleep -Milliseconds 200
        }
    }
    $pidsStopped = $true
    foreach ($procId in @($ProcessIds)) {
        if ($procId -gt 0 -and $procId -ne $PID -and (Get-Process -Id $procId -ErrorAction SilentlyContinue)) {
            $pidsStopped = $false
        }
    }
    $dbRemoved = $true
    if (-not [string]::IsNullOrWhiteSpace($DatabaseDirectory)) {
        $dbRemoved = -not (Test-Path -LiteralPath $DatabaseDirectory)
    }
    $line = Format-EphemeralCleanupLine -PidsStopped $pidsStopped -DbRemoved $dbRemoved
    Write-Host $line
    if (-not [string]::IsNullOrWhiteSpace($LogFile)) {
        Add-Content -LiteralPath $LogFile -Value $line -Encoding utf8
    }
    [pscustomobject]@{
        Ok = [bool]($pidsStopped -and $dbRemoved)
        PidsStopped = [bool]$pidsStopped
        DbRemoved = [bool]$dbRemoved
        Line = $line
    }
}

function Get-GuiSmokeMonsterCount {
    param([AllowEmptyString()][string]$LogText)
    # CInGameState.cpp logs "MonsterAdd count=<monsters_.size()>" only when
    # (size & 0x3F) == 0. The value is cumulative. A final partial batch
    # (Map10's 193..228) is not logged, so the last client line is count=192.
    # The population that must equal 228 is the mainline map line
    # "[Map] sent N monster adds". The client milestones have to stop on the
    # greatest multiple of 64 that is <= N. [entity] frame lines are not read
    # here; they are optional and come from a different commit.
    $milestones = @()
    foreach ($match in [regex]::Matches([string]$LogText, 'CInGameState: MonsterAdd count=(\d+)')) {
        $milestones += [int]$match.Groups[1].Value
    }
    $sent = @()
    foreach ($match in [regex]::Matches([string]$LogText, '\[Map\] sent (\d+) monster adds')) {
        $sent += [int]$match.Groups[1].Value
    }
    $lastMilestone = $null
    if ($milestones.Count -gt 0) {
        $lastMilestone = [int](($milestones | Measure-Object -Maximum).Maximum)
    }
    if ($sent.Count -gt 0) {
        $population = [int](($sent | Measure-Object -Maximum).Maximum)
        $expectedMilestone = $population - ($population % 64)
        if ($expectedMilestone -gt 0 -and $lastMilestone -ne $expectedMilestone) {
            throw "GUI smoke MonsterAdd count=$lastMilestone is not the cumulative 64-boundary $expectedMilestone for $population sent monsters"
        }
        return $population
    }
    if ($null -ne $lastMilestone) { return $lastMilestone }
    return 0
}

function Get-GuiSmokeEntityMonsterTotals {
    param([AllowEmptyString()][string]$LogText)
    # monster=drawn/total/culled/skipped. The total is the second field.
    # Mainline without the entity-visibility commit has none of these lines.
    $totals = @()
    foreach ($match in [regex]::Matches([string]$LogText, '\[entity\] frame .*monster=(\d+)/(\d+)/(\d+)/(\d+)')) {
        $totals += [int]$match.Groups[2].Value
    }
    return $totals
}

function Test-GuiSmokeFogBypassActive {
    param([AllowEmptyString()][string]$LogText)
    # Render-time switch. The BMHM load line "map fog enabled ... (from BMHM)"
    # is printed whether or not this bypass runs.
    return [bool]([string]$LogText -match 'FOG diag bypass \(MXH_FOG_DISABLE=1\)')
}
