<#
.SYNOPSIS
    24h stability harness for the modern Login, Agent, and Map servers.

.DESCRIPTION
    Starts mxh_login_server, mxh_agent_server_CHINA, and mxh_map_server_CHINA
    once, inside this process, the same way the ephemeral local server does.
    Does not call the shared modern service launcher and does not copy pak bytes.

    Every live check, sample, and stop uses the PID plus process start time
    recorded at launch. A PID whose start time no longer matches is treated
    as dead and is not killed. Cleanup stops only that owned process tree.

    Reconnect, when the client flags are enabled, means a full re-login
    after a forced disconnect. There is no auto-resume.
    Compare-SoakStateDump checks the existing mxh_client_e2e --state-dump JSON.

    Launch on ROG with scripts\soak-24h.cmd. Files are written with
    System.IO APIs as UTF-8 without a BOM.

.NOTES
    Exit 0 only when every configured check passes. A skipped check is a
    failure. The process prints SOAK PASS or SOAK FAIL <reasons>, then
    EXIT=<n> on its own line.
#>
[CmdletBinding()]
param(
    [string]$ServerBinDir = '',
    [string]$ClientExe = '',
    [string]$BuildRoot = '',
    [string]$RunDir = '',
    [string]$RunId = '',
    [double]$DurationHours = 24,
    [double]$WarmupHours = 1,
    [int]$SampleSeconds = 60,
    [int]$Clients = 2,
    [string[]]$CharacterPool = @(),
    [string]$MssqlDb = '',
    [string]$WatchRoot = '',
    [long]$WatchExpectedFiles = -1,
    [long]$WatchExpectedBytes = -1,
    [double]$MinFreeGB = 10,
    [string]$GuardDrive = '',
    [string[]]$GuardPaths = @(),
    [switch]$IncludeSqlDataGuard,
    [string]$PlayDhRoot = '',
    [int]$LoginPort = 26001,
    [int]$AgentPort = 27001,
    [int]$MapPort = 28001,
    [int]$Map12Port = 28002,
    [int]$MapNumber = 10,
    [int]$MaxSampleGapSeconds = 120,
    [int]$CycleTimeoutSeconds = 180,
    [int]$StartupTimeoutSeconds = 30,
    [int]$HoldSeconds = 0,
    [string]$ExpectPeer = '',
    [int]$DropSocketAfterSeconds = 0,
    [string]$StateDumpPath = '',
    [switch]$AssumeWindowsPlatform
)

$soakHelperImportError = ''
try {
    $soakCommercialPath = Join-Path $PSScriptRoot 'commercial-smoke.ps1'
    $soakHelperTokens = $null
    $soakHelperErrors = $null
    $soakHelperAst = [System.Management.Automation.Language.Parser]::ParseFile(
        $soakCommercialPath, [ref]$soakHelperTokens, [ref]$soakHelperErrors)
    if ($soakHelperErrors -and @($soakHelperErrors).Count -gt 0) {
        throw ('commercial-smoke.ps1 parse error: ' + $soakHelperErrors[0].ToString())
    }
    $soakHelperNames = @(
        'Test-CommercialSmokeWindowsHost',
        'Get-CommercialSmokeKernelStart',
        'Get-CommercialSmokeOwnedIdentity',
        'Test-CommercialSmokeLiveIdentity',
        'Add-CommercialSmokeProcStatChildren',
        'Get-CommercialSmokeChildProcessIds',
        'Add-CommercialSmokeOwnedProcess',
        'Add-CommercialSmokeOwnedDescendants',
        'Stop-CommercialSmokeOwnedProcessFallback',
        'Wait-CommercialSmokeOwnedExit',
        'Stop-CommercialSmokeOwnedProcess',
        'Stop-CommercialSmokeOwnedProcesses',
        'Split-CommercialSmokeConnectionSegments',
        'Test-CommercialSmokeDatabaseName',
        'Test-CommercialSmokeConnectionStringLiteral',
        'Test-CommercialSmokeLocalNamedInstance',
        'Resolve-CommercialSmokeMssqlConnection',
        'Get-CommercialSmokeSqlCmdLines',
        'Test-CommercialSmokeDatabaseListed',
        'Get-CommercialSmokeSqlCmdPath'
    )
    foreach ($soakHelperName in $soakHelperNames) {
        $quoted = "'" + $soakHelperName + "'"
        $predicate = [scriptblock]::Create(
            'param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq ' + $quoted)
        $defs = @($soakHelperAst.FindAll($predicate, $true))
        if ($defs.Count -ne 1) { throw ('missing helper ' + $soakHelperName) }
        . ([scriptblock]::Create($defs[0].Extent.Text))
    }
} catch {
    $soakHelperImportError = $_.Exception.Message
}
$script:soakHelperImportError = $soakHelperImportError

function Write-SoakLine {
    param([string]$Text)
    [Console]::Out.WriteLine($Text)
}

function Write-SoakText {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Text,
        [switch]$Append
    )
    $utf8 = New-Object System.Text.UTF8Encoding $false
    $dir = [IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrEmpty($dir) -and -not [IO.Directory]::Exists($dir)) {
        [void][IO.Directory]::CreateDirectory($dir)
    }
    if ($Append) {
        [IO.File]::AppendAllText($Path, $Text, $utf8)
    } else {
        [IO.File]::WriteAllText($Path, $Text, $utf8)
    }
}

function Format-SoakNumber {
    param($Value, [string]$Picture = '0.0000')
    if ($null -eq $Value) { return 'n/a' }
    return ([double]$Value).ToString($Picture, [Globalization.CultureInfo]::InvariantCulture)
}

function Format-SoakUtc {
    param([datetime]$Value)
    $utc = $Value
    if ($utc.Kind -ne [DateTimeKind]::Utc) { $utc = $utc.ToUniversalTime() }
    return $utc.ToString('yyyy-MM-ddTHH:mm:ss.fffZ', [Globalization.CultureInfo]::InvariantCulture)
}

function Test-SoakHostAtLeast74 {
    $version = $PSVersionTable.PSVersion
    if ($null -eq $version) { return $false }
    if ($version.Major -gt 7) { return $true }
    if ($version.Major -eq 7 -and $version.Minor -ge 4) { return $true }
    return $false
}

function Test-SoakWindows {
    param([bool]$Assume)
    if ($Assume) { return $true }
    return [bool](Test-CommercialSmokeWindowsHost)
}

function Get-SoakForbiddenPorts {
    return @(16001, 17001, 18001, 36001, 37001, 38001, 26011, 27011, 28011)
}

function Get-SoakPortViolation {
    param(
        [int]$LoginPort,
        [int]$AgentPort,
        [int]$MapPort,
        [int]$Map12Port
    )
    $ports = @($LoginPort, $AgentPort, $MapPort, $Map12Port)
    $seen = @{}
    foreach ($port in $ports) {
        if ($port -lt 1 -or $port -gt 65535) { return ('port ' + $port + ' is outside 1-65535') }
        foreach ($blocked in @(Get-SoakForbiddenPorts)) {
            if ($port -eq $blocked) { return ('port ' + $port + ' is forbidden') }
        }
        $key = [string]$port
        if ($seen.ContainsKey($key)) { return ('port ' + $port + ' is duplicated') }
        $seen[$key] = $true
    }
    return ''
}

function Test-SoakRunDirPolicy {
    param(
        [string]$Path,
        [bool]$ForWindows,
        [bool]$IsDefault
    )
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return [pscustomobject]@{ Allowed = $false; Reason = 'RunDir is empty' }
    }
    $text = $Path.Replace('/', '\')
    if ($text -match '^[A-Za-z]:') {
        $drive = $text.Substring(0, 1).ToUpperInvariant()
        if ($ForWindows -and $drive -eq 'C') {
            return [pscustomobject]@{ Allowed = $false; Reason = 'RunDir under C:\ is refused' }
        }
        if ($IsDefault -and $drive -eq 'E') {
            return [pscustomobject]@{ Allowed = $false; Reason = 'default RunDir must not be on E:' }
        }
        return [pscustomobject]@{ Allowed = $true; Reason = '' }
    }
    if ($text.StartsWith('\\')) {
        return [pscustomobject]@{ Allowed = $true; Reason = '' }
    }
    if ($Path.StartsWith('/')) {
        return [pscustomobject]@{ Allowed = $true; Reason = '' }
    }
    return [pscustomobject]@{ Allowed = $false; Reason = 'RunDir must be an absolute path' }
}

function Join-SoakPath {
    param([string]$Root, [string]$Child)
    # Windows drive paths must stay textual so a Linux test can simulate
    # the C:\ and E: rules without a C: or E: PSDrive.
    if ($Root -match '^[A-Za-z]:' -or $Root.StartsWith('\\')) {
        return $Root.TrimEnd('\', '/') + '\' + $Child.TrimStart('\', '/')
    }
    return Join-Path $Root $Child
}

function Resolve-SoakRunDirectory {
    param(
        [string]$RunDir,
        [string]$BuildRoot,
        [string]$ServerBinDir,
        [string]$RunId,
        [bool]$ForWindows
    )
    $isDefault = [string]::IsNullOrWhiteSpace($RunDir)
    $path = $RunDir
    if ($isDefault) {
        $root = $BuildRoot
        if ([string]::IsNullOrWhiteSpace($root)) { $root = $ServerBinDir }
        if ([string]::IsNullOrWhiteSpace($root)) {
            return [pscustomobject]@{
                Path = ''
                IsDefault = $true
                Refusal = 'Pass -BuildRoot or -RunDir'
            }
        }
        $path = Join-SoakPath -Root (Join-SoakPath -Root $root -Child 'runtime') -Child ('soak-' + $RunId)
    }
    $policy = Test-SoakRunDirPolicy -Path $path -ForWindows $ForWindows -IsDefault $isDefault
    return [pscustomobject]@{
        Path = $path
        IsDefault = $isDefault
        Refusal = $(if ($policy.Allowed) { '' } else { $policy.Reason })
    }
}

function Test-SoakPathNested {
    param([string]$Child, [string]$Parent)
    if ([string]::IsNullOrWhiteSpace($Child) -or [string]::IsNullOrWhiteSpace($Parent)) { return $false }
    $c = $Child.Replace('/', '\').TrimEnd('\')
    $p = $Parent.Replace('/', '\').TrimEnd('\')
    if ([string]::Equals($c, $p, [StringComparison]::OrdinalIgnoreCase)) { return $true }
    return $c.StartsWith($p + '\', [StringComparison]::OrdinalIgnoreCase)
}

function Test-SoakNameToken {
    param([string]$Value)
    return [regex]::IsMatch([string]$Value, '^[A-Za-z0-9_]{1,16}$')
}

function Get-SoakCharacterSlots {
    param(
        [string[]]$CharacterPool,
        [int]$Clients,
        [string]$RunId
    )
    $slots = New-Object System.Collections.Generic.List[object]
    $source = @($CharacterPool | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    if ($source.Count -eq 0) {
        for ($i = 0; $i -lt $Clients; $i++) {
            $name = 'a' + $RunId + [string]$i
            $slots.Add([pscustomobject]@{ Account = $name; Character = $name })
        }
        return @($slots.ToArray())
    }
    foreach ($entry in $source) {
        $text = ([string]$entry).Trim()
        $account = $text
        $character = $text
        $idx = $text.IndexOf(':')
        if ($idx -gt 0) {
            $account = $text.Substring(0, $idx)
            $character = $text.Substring($idx + 1)
        }
        $slots.Add([pscustomobject]@{ Account = $account; Character = $character })
    }
    return @($slots.ToArray())
}

function Resolve-SoakMssqlTarget {
    param([string]$ConnectionString)
    $resolved = Resolve-CommercialSmokeMssqlConnection -ConnectionString $ConnectionString
    if (-not $resolved.Ok) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = [string]$resolved.Message
        }
    }
    if (-not [string]::Equals([string]$resolved.Server, '.\SQLEXPRESS', [StringComparison]::OrdinalIgnoreCase)) {
        return [pscustomobject]@{
            Ok = $false
            Server = [string]$resolved.Server
            Database = [string]$resolved.Database
            Message = 'MSSQL host must be .\SQLEXPRESS'
        }
    }
    return [pscustomobject]@{
        Ok = $true
        Server = [string]$resolved.Server
        Database = [string]$resolved.Database
        Message = ''
    }
}

function Get-SoakMssqlCreateBatch {
    param([string]$DatabaseName)
    if (-not (Test-CommercialSmokeDatabaseName -Name $DatabaseName)) {
        throw ('refusing to create database ' + $DatabaseName)
    }
    return ('CREATE DATABASE [' + $DatabaseName + ']; ALTER DATABASE [' + $DatabaseName + '] SET RECOVERY SIMPLE;')
}

function Get-SoakMssqlDropBatch {
    param([string]$DatabaseName)
    if (-not (Test-CommercialSmokeDatabaseName -Name $DatabaseName)) {
        throw ('refusing to drop database ' + $DatabaseName)
    }
    return ('IF DB_ID(N''' + $DatabaseName + ''') IS NOT NULL BEGIN ALTER DATABASE [' + $DatabaseName + '] SET SINGLE_USER WITH ROLLBACK IMMEDIATE; DROP DATABASE [' + $DatabaseName + ']; END')
}

function Get-SoakDatabaseCleanupPlan {
    param([bool]$CreatedByThisRun)
    return [pscustomobject]@{ Drop = [bool]$CreatedByThisRun }
}

function Get-SoakSqlDataPathQuery {
    return "SET NOCOUNT ON; SELECT CONVERT(nvarchar(4000), SERVERPROPERTY('InstanceDefaultDataPath')); SELECT CONVERT(nvarchar(4000), SERVERPROPERTY('InstanceDefaultLogPath'));"
}

function Get-SoakDatabaseListQuery {
    return 'SET NOCOUNT ON; SELECT name FROM sys.databases ORDER BY name;'
}

function Get-SoakGuardPathList {
    param(
        [string]$GuardDrive,
        [string[]]$GuardPaths
    )
    $list = New-Object System.Collections.Generic.List[string]
    foreach ($path in @($GuardPaths)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$path)) { $list.Add(([string]$path).Trim()) }
    }
    if (-not [string]::IsNullOrWhiteSpace($GuardDrive)) {
        $drive = $GuardDrive.Trim().TrimEnd('\', '/')
        if ($drive -match '^[A-Za-z]$') { $list.Add($drive + ':\') }
        elseif ($drive -match '^[A-Za-z]:$') { $list.Add($drive + '\') }
        else { $list.Add($GuardDrive.Trim()) }
    }
    return @($list.ToArray())
}

function Test-SoakGuardSpace {
    param(
        [string[]]$Paths,
        [double]$MinFreeGB,
        [hashtable]$FreeBytesByPath
    )
    $list = @($Paths | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    if ($list.Count -eq 0) {
        return [pscustomobject]@{ Status = 'skip'; Reason = 'no guard paths'; Breaches = @() }
    }
    $minBytes = [int64]($MinFreeGB * 1073741824.0)
    $breaches = New-Object System.Collections.Generic.List[string]
    foreach ($path in $list) {
        if ($null -eq $FreeBytesByPath -or -not $FreeBytesByPath.ContainsKey($path)) {
            $breaches.Add('unreadable:' + $path)
            continue
        }
        $free = [int64]$FreeBytesByPath[$path]
        if ($free -lt $minBytes) { $breaches.Add('low:' + $path) }
    }
    if ($breaches.Count -gt 0) {
        return [pscustomobject]@{ Status = 'fail'; Reason = ($breaches.ToArray() -join ','); Breaches = @($breaches.ToArray()) }
    }
    return [pscustomobject]@{ Status = 'pass'; Reason = ''; Breaches = @() }
}

function Get-SoakFreeBytes {
    param([string]$Path)
    $probe = $Path
    if (-not [IO.Directory]::Exists($probe) -and -not [IO.File]::Exists($probe)) {
        $parent = [IO.Path]::GetDirectoryName($probe)
        while (-not [string]::IsNullOrEmpty($parent) -and -not [IO.Directory]::Exists($parent)) {
            $next = [IO.Path]::GetDirectoryName($parent)
            if ([string]::IsNullOrEmpty($next) -or $next -eq $parent) { break }
            $parent = $next
        }
        if (-not [string]::IsNullOrEmpty($parent)) { $probe = $parent }
    }
    $root = [IO.Path]::GetPathRoot($probe)
    if ([string]::IsNullOrEmpty($root)) { $root = $probe }
    $drive = [IO.DriveInfo]::new($root)
    return [int64]$drive.AvailableFreeSpace
}

function Get-SoakTreeCensus {
    param([string]$Root)
    if ([string]::IsNullOrWhiteSpace($Root) -or -not [IO.Directory]::Exists($Root)) { return $null }
    $count = [int64]0
    $bytes = [int64]0
    $options = [IO.EnumerationOptions]::new()
    $options.RecurseSubdirectories = $true
    $options.AttributesToSkip = [IO.FileAttributes]::ReparsePoint
    foreach ($file in [IO.Directory]::EnumerateFiles($Root, '*', $options)) {
        $count++
        $info = [IO.FileInfo]::new($file)
        $bytes += [int64]$info.Length
    }
    return [pscustomobject]@{ Files = $count; Bytes = $bytes }
}

function Get-SoakWatchStatus {
    param(
        $Start,
        $End,
        [long]$ExpectedFiles = -1,
        [long]$ExpectedBytes = -1
    )
    if ($null -eq $Start -or $null -eq $End) {
        return [pscustomobject]@{ Status = 'fail'; Reason = 'watch census missing' }
    }
    if ($ExpectedFiles -ge 0 -and [int64]$Start.Files -ne $ExpectedFiles) {
        return [pscustomobject]@{ Status = 'fail'; Reason = 'watch start file count differs' }
    }
    if ($ExpectedBytes -ge 0 -and [int64]$Start.Bytes -ne $ExpectedBytes) {
        return [pscustomobject]@{ Status = 'fail'; Reason = 'watch start byte count differs' }
    }
    if ([int64]$Start.Files -ne [int64]$End.Files -or [int64]$Start.Bytes -ne [int64]$End.Bytes) {
        return [pscustomobject]@{ Status = 'fail'; Reason = 'watch root changed' }
    }
    return [pscustomobject]@{ Status = 'pass'; Reason = '' }
}

function Get-SoakSlope {
    param(
        [object[]]$X,
        [object[]]$Y
    )
    # Index casts such as [double]($series[0]) can box as Object[] in
    # Windows PowerShell and some pwsh builds. Convert.ToDouble avoids that.
    $hours = @($X)
    $levels = @($Y)
    if ($hours.Count -ne $levels.Count -or $hours.Count -lt 2) { return $null }
    $count = [double]$hours.Count
    $sumHour = [double]0
    $sumLevel = [double]0
    $sumProduct = [double]0
    $sumHourSq = [double]0
    for ($index = 0; $index -lt $hours.Count; $index++) {
        $hour = [System.Convert]::ToDouble($hours[$index])
        $level = [System.Convert]::ToDouble($levels[$index])
        $sumHour = $sumHour + $hour
        $sumLevel = $sumLevel + $level
        $sumProduct = $sumProduct + ($hour * $level)
        $sumHourSq = $sumHourSq + ($hour * $hour)
    }
    $denominator = ($count * $sumHourSq) - ($sumHour * $sumHour)
    if ([math]::Abs($denominator) -lt 1e-9) { return $null }
    return (($count * $sumProduct) - ($sumHour * $sumLevel)) / $denominator
}

function Test-SoakLeak {
    param(
        [object[]]$Samples,
        [double]$WarmupHours,
        [double]$PrivateSlopeLimitMbPerHour = 1.0,
        [double]$HandleSlopeLimitPerHour = 10.0
    )
    $mb = 1048576.0
    $post = @($Samples | Where-Object { $null -ne $_ -and ([double]$_.THours -ge $WarmupHours) } | Sort-Object { [double]$_.THours })
    if ($post.Count -lt 2) {
        return [pscustomobject]@{
            Status = 'skip'
            Reasons = @('fewer than 2 post-warmup samples')
            PrivateSlopeMbPerHour = $null
            HandleSlopePerHour = $null
            PrivateBaselineBytes = $null
            PrivateFinalBytes = $null
            PrivateCeilingBytes = $null
            HandleBaseline = $null
            HandleFinal = $null
            HandleCeiling = $null
        }
    }
    $xs = New-Object System.Collections.Generic.List[double]
    $priv = New-Object System.Collections.Generic.List[double]
    $handles = New-Object System.Collections.Generic.List[double]
    foreach ($sample in $post) {
        $xs.Add([double]$sample.THours)
        $priv.Add([double]$sample.PrivateBytes)
        $handles.Add([double]$sample.Handles)
    }
    $privateSlopeBytes = Get-SoakSlope -X $xs.ToArray() -Y $priv.ToArray()
    $handleSlope = Get-SoakSlope -X $xs.ToArray() -Y $handles.ToArray()
    if ($null -eq $privateSlopeBytes -or $null -eq $handleSlope) {
        return [pscustomobject]@{
            Status = 'skip'
            Reasons = @('slope undefined')
            PrivateSlopeMbPerHour = $null
            HandleSlopePerHour = $null
            PrivateBaselineBytes = $null
            PrivateFinalBytes = $null
            PrivateCeilingBytes = $null
            HandleBaseline = $null
            HandleFinal = $null
            HandleCeiling = $null
        }
    }
    $privateSlope = [double]$privateSlopeBytes / $mb
    $baselinePrivate = [double]($post[0].PrivateBytes)
    $finalPrivate = [double]($post[$post.Count - 1].PrivateBytes)
    $baselineHandles = [double]($post[0].Handles)
    $finalHandles = [double]($post[$post.Count - 1].Handles)
    $privateCeiling = [math]::Max(($baselinePrivate * 1.10), ($baselinePrivate + (20.0 * $mb)))
    $handleCeiling = [math]::Max(($baselineHandles * 1.10), ($baselineHandles + 50.0))
    $reasons = New-Object System.Collections.Generic.List[string]
    if (-not ($privateSlope -lt $PrivateSlopeLimitMbPerHour)) { $reasons.Add('private_slope') }
    if (-not ($finalPrivate -le ($privateCeiling + 1e-6))) { $reasons.Add('private_final') }
    if (-not ($handleSlope -lt $HandleSlopeLimitPerHour)) { $reasons.Add('handle_slope') }
    if (-not ($finalHandles -le ($handleCeiling + 1e-6))) { $reasons.Add('handle_final') }
    $status = $(if ($reasons.Count -eq 0) { 'pass' } else { 'fail' })
    return [pscustomobject]@{
        Status = $status
        Reasons = @($reasons.ToArray())
        PrivateSlopeMbPerHour = $privateSlope
        HandleSlopePerHour = [double]$handleSlope
        PrivateBaselineBytes = $baselinePrivate
        PrivateFinalBytes = $finalPrivate
        PrivateCeilingBytes = $privateCeiling
        HandleBaseline = $baselineHandles
        HandleFinal = $finalHandles
        HandleCeiling = $handleCeiling
    }
}

function Test-SoakSampleGaps {
    param(
        [datetime[]]$TimestampsUtc,
        [int]$MaxGapSeconds = 120
    )
    $stamps = @($TimestampsUtc)
    $maxGap = 0.0
    if ($stamps.Count -lt 2) {
        return [pscustomobject]@{ Status = 'skip'; MaxGapSeconds = 0.0; Reason = 'fewer than 2 samples' }
    }
    $failed = $false
    for ($i = 1; $i -lt $stamps.Count; $i++) {
        $gap = ($stamps[$i] - $stamps[$i - 1]).TotalSeconds
        if ($gap -gt $maxGap) { $maxGap = $gap }
        if ($gap -gt $MaxGapSeconds) { $failed = $true }
    }
    if ($failed) {
        return [pscustomobject]@{ Status = 'fail'; MaxGapSeconds = $maxGap; Reason = 'sample gap' }
    }
    return [pscustomobject]@{ Status = 'pass'; MaxGapSeconds = $maxGap; Reason = '' }
}

function Get-SoakMember {
    param($Object, [string[]]$Names)
    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        foreach ($name in $Names) {
            foreach ($key in @($Object.Keys)) {
                if ([string]$key -eq $name) { return $Object[$key] }
            }
        }
        foreach ($name in $Names) {
            foreach ($key in @($Object.Keys)) {
                if ([string]::Equals([string]$key, $name, [StringComparison]::OrdinalIgnoreCase)) { return $Object[$key] }
            }
        }
        return $null
    }
    foreach ($name in $Names) {
        $prop = $Object.PSObject.Properties[$name]
        if ($null -ne $prop) { return $prop.Value }
    }
    foreach ($prop in @($Object.PSObject.Properties)) {
        foreach ($name in $Names) {
            if ([string]::Equals([string]$prop.Name, $name, [StringComparison]::OrdinalIgnoreCase)) {
                return $prop.Value
            }
        }
    }
    return $null
}

function Convert-SoakStateObject {
    param($Value)
    if ($null -eq $Value) {
        return [pscustomobject]@{ Ok = $false; Value = $null }
    }
    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value)) {
            return [pscustomobject]@{ Ok = $false; Value = $null }
        }
        try {
            $parsed = $Value | ConvertFrom-Json
        } catch {
            return [pscustomobject]@{ Ok = $false; Value = $null }
        }
        if ($null -eq $parsed -or $parsed -is [System.Array]) {
            return [pscustomobject]@{ Ok = $false; Value = $null }
        }
        return [pscustomobject]@{ Ok = $true; Value = $parsed }
    }
    if ($Value -is [System.Array]) {
        return [pscustomobject]@{ Ok = $false; Value = $null }
    }
    return [pscustomobject]@{ Ok = $true; Value = $Value }
}

function Test-SoakHasProperty {
    param($Object, [string]$Name)
    if ($null -eq $Object) { return $false }
    if ($Object -is [System.Collections.IDictionary]) {
        foreach ($key in @($Object.Keys)) {
            if ([string]$key -ceq $Name) { return $true }
        }
        return $false
    }
    return $null -ne $Object.PSObject.Properties[$Name]
}

function Test-SoakField {
    param($Object, [string]$Name)
    # A missing property and a JSON null are both absent. Zero is present.
    # Two absent values are not equal.
    if (-not (Test-SoakHasProperty $Object $Name)) {
        return [pscustomobject]@{ Present = $false; Value = $null }
    }
    $value = Get-SoakMember $Object @($Name)
    if ($null -eq $value) {
        return [pscustomobject]@{ Present = $false; Value = $null }
    }
    return [pscustomobject]@{ Present = $true; Value = $value }
}

function Add-SoakReason {
    param($List, [string]$Reason)
    if (-not $List.Contains($Reason)) { [void]$List.Add($Reason) }
}

function Resolve-SoakSnapshot {
    param($Document, [string]$Side)
    # A session dump has entry and exit. Drop-time is exit. The
    # post-reconnect snapshot is entry. A bare snapshot is used as-is.
    $wrapped = (Test-SoakHasProperty $Document 'entry') -or (Test-SoakHasProperty $Document 'exit')
    if (-not $wrapped) {
        return [pscustomobject]@{ Ok = $true; Snapshot = $Document; Reason = '' }
    }
    $field = Test-SoakField $Document $Side
    if (-not $field.Present) {
        return [pscustomobject]@{ Ok = $false; Snapshot = $null; Reason = ('missing:' + $Side) }
    }
    return [pscustomobject]@{ Ok = $true; Snapshot = $field.Value; Reason = '' }
}

function Get-SoakInventoryCompare {
    param($Snapshot)
    $reasons = New-Object System.Collections.Generic.List[string]
    $inventory = Test-SoakField $Snapshot 'inventory'
    if (-not $inventory.Present) {
        Add-SoakReason $reasons 'missing:inventory'
        return [pscustomobject]@{ Ok = $false; Keys = @(); Reasons = @($reasons.ToArray()) }
    }
    $keys = New-Object System.Collections.Generic.List[string]
    foreach ($item in @($inventory.Value)) {
        if ($null -eq $item) {
            Add-SoakReason $reasons 'missing:inventory'
            continue
        }
        $slot = Test-SoakField $item 'slot'
        $itemIndex = Test-SoakField $item 'item_index'
        $count = Test-SoakField $item 'count'
        if (-not $slot.Present) { Add-SoakReason $reasons 'missing:slot' }
        if (-not $itemIndex.Present) { Add-SoakReason $reasons 'missing:item_index' }
        if (-not $count.Present) { Add-SoakReason $reasons 'missing:count' }
        if ($slot.Present -and $itemIndex.Present -and $count.Present) {
            $keys.Add(([string]$slot.Value) + '|' + ([string]$itemIndex.Value) + '|' + ([string]$count.Value))
        }
    }
    $sorted = @($keys.ToArray() | Sort-Object)
    return [pscustomobject]@{
        Ok = ($reasons.Count -eq 0)
        Keys = $sorted
        Reasons = @($reasons.ToArray())
    }
}

function Compare-SoakStateDump {
    param(
        $Before,
        $After,
        [double]$PositionTolerance = 32
    )
    # mxh_client_e2e --state-dump JSON is {entry, exit}. Reconnect is a
    # full re-login, not a resume: compare the drop-time exit snapshot
    # with the post-reconnect entry snapshot. Map, hp, money, and
    # inventory (slot, item_index, count) must match. Position distance
    # is on x/y, and y is the ground axis, within 32. A missing field
    # fails, including when both sides omit it.
    $parsedBefore = Convert-SoakStateObject $Before
    $parsedAfter = Convert-SoakStateObject $After
    if (-not $parsedBefore.Ok -or -not $parsedAfter.Ok) {
        return [pscustomobject]@{ Pass = $false; Reasons = @('malformed') }
    }
    $drop = Resolve-SoakSnapshot -Document $parsedBefore.Value -Side 'exit'
    $reconnect = Resolve-SoakSnapshot -Document $parsedAfter.Value -Side 'entry'
    $reasons = New-Object System.Collections.Generic.List[string]
    if (-not $drop.Ok) { Add-SoakReason $reasons $drop.Reason }
    if (-not $reconnect.Ok) { Add-SoakReason $reasons $reconnect.Reason }
    if ($drop.Ok -and $reconnect.Ok) {
        $left = $drop.Snapshot
        $right = $reconnect.Snapshot
        foreach ($name in @('hp', 'mp', 'shield', 'money', 'inventory', 'position')) {
            $onLeft = Test-SoakField $left $name
            $onRight = Test-SoakField $right $name
            if (-not $onLeft.Present -or -not $onRight.Present) {
                Add-SoakReason $reasons ('missing:' + $name)
            }
        }
        $hpLeft = Test-SoakField $left 'hp'
        $hpRight = Test-SoakField $right 'hp'
        if ($hpLeft.Present -and $hpRight.Present -and ([int64]$hpLeft.Value -ne [int64]$hpRight.Value)) {
            Add-SoakReason $reasons 'hp'
        }
        $moneyLeft = Test-SoakField $left 'money'
        $moneyRight = Test-SoakField $right 'money'
        if ($moneyLeft.Present -and $moneyRight.Present -and ([int64]$moneyLeft.Value -ne [int64]$moneyRight.Value)) {
            Add-SoakReason $reasons 'money'
        }
        $posLeft = Test-SoakField $left 'position'
        $posRight = Test-SoakField $right 'position'
        if ($posLeft.Present -and $posRight.Present) {
            $axesOk = $true
            foreach ($axis in @('x', 'y', 'map')) {
                $axisLeft = Test-SoakField $posLeft.Value $axis
                $axisRight = Test-SoakField $posRight.Value $axis
                if (-not $axisLeft.Present -or -not $axisRight.Present) {
                    Add-SoakReason $reasons ('missing:' + $axis)
                    $axesOk = $false
                }
            }
            if ($axesOk) {
                $mapLeft = Get-SoakMember $posLeft.Value @('map')
                $mapRight = Get-SoakMember $posRight.Value @('map')
                if ([string]$mapLeft -ne [string]$mapRight) { Add-SoakReason $reasons 'map' }
                $xLeft = [System.Convert]::ToDouble((Get-SoakMember $posLeft.Value @('x')))
                $xRight = [System.Convert]::ToDouble((Get-SoakMember $posRight.Value @('x')))
                $yLeft = [System.Convert]::ToDouble((Get-SoakMember $posLeft.Value @('y')))
                $yRight = [System.Convert]::ToDouble((Get-SoakMember $posRight.Value @('y')))
                $dx = $xLeft - $xRight
                $dy = $yLeft - $yRight
                $distance = [math]::Sqrt(($dx * $dx) + ($dy * $dy))
                if ($distance -gt $PositionTolerance) { Add-SoakReason $reasons 'position' }
            }
        }
        $invLeft = Get-SoakInventoryCompare $left
        $invRight = Get-SoakInventoryCompare $right
        foreach ($reason in @($invLeft.Reasons)) { Add-SoakReason $reasons $reason }
        foreach ($reason in @($invRight.Reasons)) { Add-SoakReason $reasons $reason }
        if ($invLeft.Ok -and $invRight.Ok) {
            $same = @($invLeft.Keys).Count -eq @($invRight.Keys).Count
            if ($same) {
                for ($i = 0; $i -lt @($invLeft.Keys).Count; $i++) {
                    if ([string]($invLeft.Keys[$i]) -ne [string]($invRight.Keys[$i])) { $same = $false }
                }
            }
            if (-not $same) { Add-SoakReason $reasons 'inventory' }
        }
    }
    return [pscustomobject]@{
        Pass = ($reasons.Count -eq 0)
        Reasons = @($reasons.ToArray())
    }
}

function Get-SoakVerdict {
    param(
        [double]$ElapsedHours,
        [double]$DurationHours,
        [double]$WarmupHours,
        [object[]]$Servers,
        [int]$CycleTotal,
        [int]$CycleFailed,
        [string]$WatchStatus,
        [string]$GuardStatus,
        [string]$GapStatus,
        [string[]]$ExtraReasons = @()
    )
    # Failure rate uses integers: failed/total > 0.001 exactly when
    # failed * 1000 > total. A rate of 0.1% passes. A missing cycle
    # count is a skip, and a skip is not a pass.
    $reasons = New-Object System.Collections.Generic.List[string]
    $skipped = New-Object System.Collections.Generic.List[string]
    if ($ElapsedHours -lt $DurationHours) { $reasons.Add('elapsed') }
    $serverRows = New-Object System.Collections.Generic.List[object]
    foreach ($server in @($Servers)) {
        if ($null -eq $server) { continue }
        $name = [string]$server.Name
        if ([bool]$server.ExitedEarly) { $reasons.Add('server_exit:' + $name) }
        if ([bool]$server.Reused) { $reasons.Add('server_reused:' + $name) }
        $leak = Test-SoakLeak -Samples @($server.Samples) -WarmupHours $WarmupHours
        if ($leak.Status -eq 'skip') {
            $skipped.Add('leak:' + $name)
            $reasons.Add('skipped:leak:' + $name)
        } elseif ($leak.Status -eq 'fail') {
            $reasons.Add('leak:' + $name)
        }
        $serverRows.Add([pscustomobject]@{
            Name = $name
            Status = $leak.Status
            ExitedEarly = [bool]$server.ExitedEarly
            Reused = [bool]$server.Reused
            PrivateSlopeMbPerHour = $leak.PrivateSlopeMbPerHour
            HandleSlopePerHour = $leak.HandleSlopePerHour
            PrivateBaselineBytes = $leak.PrivateBaselineBytes
            PrivateFinalBytes = $leak.PrivateFinalBytes
            PrivateCeilingBytes = $leak.PrivateCeilingBytes
            HandleBaseline = $leak.HandleBaseline
            HandleFinal = $leak.HandleFinal
            HandleCeiling = $leak.HandleCeiling
            LeakReasons = @($leak.Reasons)
        })
    }
    if ($serverRows.Count -eq 0) {
        $skipped.Add('servers')
        $reasons.Add('skipped:servers')
    }
    $failureRate = $null
    if ($CycleTotal -le 0) {
        $skipped.Add('cycles')
        $reasons.Add('skipped:cycles')
    } else {
        $failureRate = [double]$CycleFailed / [double]$CycleTotal
        if (([int64]$CycleFailed * 1000) -gt [int64]$CycleTotal) {
            $reasons.Add('failure_rate=' + (Format-SoakNumber $failureRate))
        }
    }
    foreach ($pair in @(
        @('sample_gap', $GapStatus),
        @('watch', $WatchStatus),
        @('guard', $GuardStatus)
    )) {
        $label = [string]($pair[0])
        $status = [string]($pair[1])
        if ($status -eq 'pass') { continue }
        if ($status -eq 'fail') { $reasons.Add($label) }
        else {
            $skipped.Add($label)
            $reasons.Add('skipped:' + $label)
        }
    }
    foreach ($extra in @($ExtraReasons)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$extra)) { $reasons.Add([string]$extra) }
    }
    $privateParts = New-Object System.Collections.Generic.List[string]
    $handleParts = New-Object System.Collections.Generic.List[string]
    foreach ($row in $serverRows) {
        $privateParts.Add($row.Name + '=' + (Format-SoakNumber $row.PrivateSlopeMbPerHour))
        $handleParts.Add($row.Name + '=' + (Format-SoakNumber $row.HandleSlopePerHour))
    }
    $privateText = 'none'
    $handleText = 'none'
    if ($privateParts.Count -gt 0) { $privateText = ($privateParts.ToArray() -join ',') }
    if ($handleParts.Count -gt 0) { $handleText = ($handleParts.ToArray() -join ',') }
    $slopeText = 'private_slope_mb_h ' + $privateText + ' handle_slope_h ' + $handleText
    $pass = ($reasons.Count -eq 0)
    $line = $(if ($pass) { 'SOAK PASS ' + $slopeText } else { 'SOAK FAIL ' + ($reasons.ToArray() -join ' ') + ' ' + $slopeText })
    return [pscustomobject]@{
        Pass = $pass
        Reasons = @($reasons.ToArray())
        Skipped = @($skipped.ToArray())
        Line = $line
        Servers = @($serverRows.ToArray())
        FailureRate = $failureRate
        SlopeText = $slopeText
    }
}

function Get-SoakClientArguments {
    param(
        [string]$Account,
        [string]$Character,
        [int]$LoginPort,
        [int]$AgentPort,
        [int]$MapPort,
        [int]$Map12Port,
        [int]$MapNumber,
        [int]$TimeoutSeconds,
        [int]$HoldSeconds = 0,
        [string]$ExpectPeer = '',
        [int]$DropSocketAfterSeconds = 0,
        [string]$StateDumpPath = ''
    )
    # Existing mxh_client_e2e flags. They stay off unless the operator
    # opts in. --state-dump is that client's state-dump JSON.
    # Reconnect is a full re-login, not a resume.
    $args = @(
        '--no-spawn',
        '--login-host', '127.0.0.1',
        '--login-port', ([string]$LoginPort),
        '--agent-port', ([string]$AgentPort),
        '--map-port', ([string]$MapPort),
        '--map12-port', ([string]$Map12Port),
        '--map-number', ([string]$MapNumber),
        '--account', $Account,
        '--character-name', $Character,
        '--timeout', ([string]$TimeoutSeconds)
    )
    if ($HoldSeconds -gt 0) { $args += @('--hold-seconds', ([string]$HoldSeconds)) }
    if (-not [string]::IsNullOrWhiteSpace($ExpectPeer)) { $args += @('--expect-peer', $ExpectPeer) }
    if ($DropSocketAfterSeconds -gt 0) { $args += @('--drop-socket-after', ([string]$DropSocketAfterSeconds)) }
    if (-not [string]::IsNullOrWhiteSpace($StateDumpPath)) { $args += @('--state-dump', $StateDumpPath) }
    return $args
}

function Get-SoakServerPlan {
    param(
        [string]$LoginExe,
        [string]$AgentExe,
        [string]$MapExe,
        [int]$LoginPort,
        [int]$AgentPort,
        [int]$MapPort,
        [int]$MapNumber,
        [string]$ResourceRoot,
        [string]$ServerResourceRoot,
        [string]$Backend
    )
    $commonDb = @('--backend', $Backend, '--db-env', 'MXH_DATABASE_CONFIG')
    $legacy = @('--legacy')
    $mapArgs = @(
        '--port', ([string]$MapPort),
        '--map', ([string]$MapNumber),
        '--bind-address', '127.0.0.1',
        '--resource-root', $ResourceRoot,
        '--server-resource-root', $ServerResourceRoot,
        '--resource-profile', 'playdh-current'
    ) + $commonDb
    $agentArgs = @(
        '--port', ([string]$AgentPort),
        '--bind-address', '127.0.0.1',
        '--map-server', ('127.0.0.1:' + [string]$MapPort),
        '--default-map', ([string]$MapNumber)
    ) + $commonDb + $legacy
    $loginArgs = @(
        '--port', ([string]$LoginPort),
        '--bind-address', '127.0.0.1',
        '--agent-addr', '127.0.0.1',
        '--agent-port', ([string]$AgentPort)
    ) + $commonDb + $legacy
    return @(
        [pscustomobject]@{ Name = 'mxh_map_server_CHINA'; Role = 'map'; Executable = $MapExe; Arguments = $mapArgs; Port = $MapPort },
        [pscustomobject]@{ Name = 'mxh_agent_server_CHINA'; Role = 'agent'; Executable = $AgentExe; Arguments = $agentArgs; Port = $AgentPort },
        [pscustomobject]@{ Name = 'mxh_login_server'; Role = 'login'; Executable = $LoginExe; Arguments = $loginArgs; Port = $LoginPort }
    )
}

function Resolve-SoakExecutable {
    param(
        [string]$ServerBinDir,
        [string]$BuildRoot,
        [string]$FileName,
        [string[]]$ToolDirs
    )
    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($ServerBinDir)) {
        $candidates.Add((Join-Path $ServerBinDir $FileName))
        $candidates.Add((Join-Path (Join-Path $ServerBinDir 'Release') $FileName))
        $candidates.Add((Join-Path (Join-Path $ServerBinDir 'Debug') $FileName))
    }
    if (-not [string]::IsNullOrWhiteSpace($BuildRoot)) {
        foreach ($tool in @($ToolDirs)) {
            $base = Join-Path (Join-Path $BuildRoot 'tools') $tool
            $candidates.Add((Join-Path $base $FileName))
            $candidates.Add((Join-Path (Join-Path $base 'Release') $FileName))
            $candidates.Add((Join-Path (Join-Path $base 'Debug') $FileName))
        }
    }
    foreach ($candidate in $candidates) {
        if ([IO.File]::Exists($candidate)) { return $candidate }
    }
    return ''
}

function Get-SoakLaunchRefusal {
    param(
        [bool]$HostOk,
        [string]$RunDirRefusal,
        [string]$RunDir,
        [string]$WatchRoot,
        [int]$LoginPort,
        [int]$AgentPort,
        [int]$MapPort,
        [int]$Map12Port,
        [double]$DurationHours,
        [double]$WarmupHours,
        [int]$SampleSeconds,
        [int]$Clients,
        [int]$MaxSampleGapSeconds,
        [int]$CycleTimeoutSeconds,
        [int]$StartupTimeoutSeconds,
        [double]$MinFreeGB,
        [int]$MapNumber,
        [string]$MssqlDb,
        [bool]$IncludeSqlDataGuard,
        [object[]]$CharacterSlots,
        [string]$ClientExe,
        [string]$PlayDhRoot,
        [string]$DbTool,
        [string]$LoginExe,
        [string]$AgentExe,
        [string]$MapExe,
        [int]$HoldSeconds,
        [string]$ExpectPeer,
        [int]$DropSocketAfterSeconds,
        [string]$RunId
    )
    if (-not $HostOk) { return 'PowerShell 7.4 or newer is required' }
    if ($RunId -notmatch '^[A-Za-z0-9_]{1,32}$') { return 'RunId must match ^[A-Za-z0-9_]{1,32}$' }
    if ($DurationHours -le 0) { return 'DurationHours must be > 0' }
    if ($WarmupHours -lt 0 -or $WarmupHours -ge $DurationHours) { return 'WarmupHours must be >= 0 and < DurationHours' }
    if ($SampleSeconds -lt 1) { return 'SampleSeconds must be >= 1' }
    if ($Clients -lt 1) { return 'Clients must be >= 1' }
    if ($MaxSampleGapSeconds -lt 1) { return 'MaxSampleGapSeconds must be >= 1' }
    if ($CycleTimeoutSeconds -lt 1) { return 'CycleTimeoutSeconds must be >= 1' }
    if ($StartupTimeoutSeconds -lt 1) { return 'StartupTimeoutSeconds must be >= 1' }
    if ($MinFreeGB -lt 0) { return 'MinFreeGB must be >= 0' }
    if ($MapNumber -lt 0 -or $MapNumber -gt 255) { return 'MapNumber must be 0..255' }
    if ($HoldSeconds -lt 0) { return 'HoldSeconds must be >= 0' }
    if ($DropSocketAfterSeconds -lt 0) { return 'DropSocketAfterSeconds must be >= 0' }
    if (-not [string]::IsNullOrWhiteSpace($ExpectPeer) -and -not (Test-SoakNameToken $ExpectPeer)) {
        return 'ExpectPeer must match ^[A-Za-z0-9_]{1,16}$'
    }
    $portViolation = Get-SoakPortViolation -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort -Map12Port $Map12Port
    if (-not [string]::IsNullOrEmpty($portViolation)) { return $portViolation }
    if (-not [string]::IsNullOrEmpty($RunDirRefusal)) { return $RunDirRefusal }
    if ($RunDir.Contains(';')) { return 'RunDir must not contain a semicolon' }
    if (-not [string]::IsNullOrWhiteSpace($WatchRoot) -and (Test-SoakPathNested -Child $RunDir -Parent $WatchRoot)) {
        return 'RunDir must not be inside WatchRoot'
    }
    if ($IncludeSqlDataGuard -and [string]::IsNullOrWhiteSpace($MssqlDb)) {
        return 'IncludeSqlDataGuard requires -MssqlDb'
    }
    if (-not [string]::IsNullOrWhiteSpace($MssqlDb)) {
        $target = Resolve-SoakMssqlTarget -ConnectionString $MssqlDb
        if (-not $target.Ok) { return $target.Message }
    }
    if (@($CharacterSlots).Count -lt $Clients) { return 'CharacterPool must contain at least -Clients entries' }
    foreach ($slot in @($CharacterSlots)) {
        if (-not (Test-SoakNameToken $slot.Account) -or -not (Test-SoakNameToken $slot.Character)) {
            return 'character pool account and character must match ^[A-Za-z0-9_]{1,16}$'
        }
    }
    if ([string]::IsNullOrWhiteSpace($ClientExe) -or -not [IO.File]::Exists($ClientExe)) {
        return ('missing client executable: ' + $ClientExe)
    }
    if ([string]::IsNullOrWhiteSpace($LoginExe) -or -not [IO.File]::Exists($LoginExe)) {
        return 'missing server executable: mxh_login_server.exe'
    }
    if ([string]::IsNullOrWhiteSpace($AgentExe) -or -not [IO.File]::Exists($AgentExe)) {
        return 'missing server executable: mxh_agent_server_CHINA.exe'
    }
    if ([string]::IsNullOrWhiteSpace($MapExe) -or -not [IO.File]::Exists($MapExe)) {
        return 'missing server executable: mxh_map_server_CHINA.exe'
    }
    if ([string]::IsNullOrWhiteSpace($DbTool) -or -not [IO.File]::Exists($DbTool)) {
        return 'missing database tool: mxh_db_tool.exe'
    }
    if ([string]::IsNullOrWhiteSpace($PlayDhRoot) -or -not [IO.Directory]::Exists($PlayDhRoot)) {
        return ('missing PlayDhRoot: ' + $PlayDhRoot)
    }
    $serverResource = Join-Path $PlayDhRoot 'Resource'
    $serverResource = Join-Path $serverResource 'Server'
    if (-not [IO.Directory]::Exists($serverResource)) {
        return ('missing server resource root: ' + $serverResource)
    }
    return ''
}

function New-SoakEnvironment {
    param([hashtable]$Extra)
    $dict = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in [Environment]::GetEnvironmentVariables('Process').GetEnumerator()) {
        $key = [string]$entry.Key
        if ([string]::IsNullOrEmpty($key)) { continue }
        $dict[$key] = [string]$entry.Value
    }
    if ($null -ne $Extra) {
        foreach ($key in @($Extra.Keys)) {
            $dict[[string]$key] = [string]($Extra[$key])
        }
    }
    return $dict
}

function Start-SoakOwnedProcess {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [string[]]$ArgumentList,
        [string]$WorkingDirectory = '',
        [Parameter(Mandatory)][string]$StdoutPath,
        [Parameter(Mandatory)][string]$StderrPath,
        $Environment = $null
    )
    $work = $WorkingDirectory
    if ([string]::IsNullOrWhiteSpace($work)) { $work = [IO.Path]::GetDirectoryName($FilePath) }
    $stdoutDir = [IO.Path]::GetDirectoryName($StdoutPath)
    $stderrDir = [IO.Path]::GetDirectoryName($StderrPath)
    if (-not [string]::IsNullOrEmpty($stdoutDir)) { [void][IO.Directory]::CreateDirectory($stdoutDir) }
    if (-not [string]::IsNullOrEmpty($stderrDir)) { [void][IO.Directory]::CreateDirectory($stderrDir) }
    $windows = Test-CommercialSmokeWindowsHost
    $proc = $null
    if ($null -ne $Environment -and $windows) {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $work -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden -Environment $Environment
    } elseif ($null -ne $Environment) {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $work -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru -Environment $Environment
    } elseif ($windows) {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $work -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    } else {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $work -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru
    }
    try { $null = $proc.Handle } catch { }
    $identity = Get-CommercialSmokeOwnedIdentity -Process $proc
    if ($null -eq $identity) { throw ('started process has no identity: ' + $FilePath) }
    return [pscustomobject]@{
        Process = $proc
        Identity = $identity
        StdoutPath = $StdoutPath
        StderrPath = $StderrPath
    }
}

function Get-SoakIdentityState {
    param($Identity)
    if ($null -eq $Identity) { return 'dead' }
    $id = 0
    try { $id = [int]$Identity.Id } catch { return 'dead' }
    if ($id -le 0) { return 'dead' }
    $windows = Test-CommercialSmokeWindowsHost
    if ($windows) {
        $live = $null
        try { $live = Get-Process -Id $id -ErrorAction SilentlyContinue } catch { $live = $null }
        if ($null -eq $live) { return 'dead' }
        try { $live.Refresh() } catch { return 'dead' }
        try { if ([bool]$live.HasExited) { return 'dead' } } catch { return 'dead' }
    } elseif (-not (Test-Path -LiteralPath ('/proc/' + $id))) {
        return 'dead'
    }
    $ticks = [int64]0
    $kernel = [int64]0
    try { $ticks = [int64]$Identity.StartTicks } catch { $ticks = 0 }
    try { $kernel = [int64]$Identity.KernelStart } catch { $kernel = 0 }
    if (Test-CommercialSmokeLiveIdentity -ProcessId $id -StartTicks $ticks -KernelStart $kernel) { return 'match' }
    return 'reused'
}

function Stop-SoakOwnedTree {
    param($Identity)
    $state = Get-SoakIdentityState -Identity $Identity
    if ($state -ne 'match') {
        return [pscustomobject]@{ Action = $state; KilledIds = @() }
    }
    $rootId = [int]$Identity.Id
    if ($rootId -le 1 -or $rootId -eq $PID) {
        return [pscustomobject]@{ Action = 'refused'; KilledIds = @() }
    }
    $root = $null
    try { $root = Get-Process -Id $rootId -ErrorAction SilentlyContinue } catch { $root = $null }
    if ($null -eq $root) {
        return [pscustomobject]@{ Action = 'dead'; KilledIds = @() }
    }
    $owned = New-Object System.Collections.Generic.List[object]
    $seen = @{}
    Add-CommercialSmokeOwnedProcess -Owned $owned -Seen $seen -Process $root
    Add-CommercialSmokeOwnedDescendants -Owned $owned -Seen $seen -RootId $rootId
    if ((Get-SoakIdentityState -Identity $Identity) -ne 'match') {
        return [pscustomobject]@{ Action = 'reused'; KilledIds = @() }
    }
    $filtered = New-Object System.Collections.Generic.List[object]
    foreach ($entry in $owned) {
        $eid = 0
        try { $eid = [int]$entry.Id } catch { continue }
        if ($eid -le 1 -or $eid -eq $PID) { continue }
        $filtered.Add($entry)
    }
    Stop-CommercialSmokeOwnedProcesses -Owned $filtered
    $ids = New-Object System.Collections.Generic.List[int]
    foreach ($entry in $filtered) { $ids.Add([int]$entry.Id) }
    return [pscustomobject]@{ Action = 'stopped'; KilledIds = @($ids.ToArray()) }
}

function Get-SoakProcessSample {
    param(
        $Identity,
        [string]$Name,
        [datetime]$TimestampUtc
    )
    $state = Get-SoakIdentityState -Identity $Identity
    if ($state -ne 'match') {
        return [pscustomobject]@{ Alive = $false; State = $state }
    }
    $proc = $null
    try { $proc = Get-Process -Id ([int]$Identity.Id) -ErrorAction SilentlyContinue } catch { $proc = $null }
    if ($null -eq $proc) { return [pscustomobject]@{ Alive = $false; State = 'dead' } }
    try { $proc.Refresh() } catch { return [pscustomobject]@{ Alive = $false; State = 'dead' } }
    if ((Get-SoakIdentityState -Identity $Identity) -ne 'match') {
        return [pscustomobject]@{ Alive = $false; State = 'reused' }
    }
    $private = [int64]0
    $workingSet = [int64]0
    $handles = [int]0
    $threads = [int]0
    $cpu = 0.0
    try { $private = [int64]$proc.PrivateMemorySize64 } catch { $private = 0 }
    try { $workingSet = [int64]$proc.WorkingSet64 } catch { $workingSet = 0 }
    try { $handles = [int]$proc.HandleCount } catch { $handles = 0 }
    try { $threads = [int]@($proc.Threads).Count } catch { $threads = 0 }
    try { $cpu = [double]$proc.TotalProcessorTime.TotalSeconds } catch { $cpu = 0 }
    return [pscustomobject]@{
        Alive = $true
        State = 'match'
        TimestampUtc = $TimestampUtc
        Pid = [int]$Identity.Id
        Name = $Name
        PrivateBytes = $private
        WorkingSet = $workingSet
        HandleCount = $handles
        ThreadCount = $threads
        CpuSeconds = $cpu
    }
}

function Write-SoakSampleRow {
    param(
        [string]$Path,
        $Sample,
        [ref]$HeaderWritten
    )
    if (-not $HeaderWritten.Value) {
        Write-SoakText -Path $Path -Text "timestamp_utc,pid,name,private_bytes,working_set,handle_count,thread_count,cpu_seconds`n"
        $HeaderWritten.Value = $true
    }
    $line = @(
        (Format-SoakUtc $Sample.TimestampUtc),
        ([string]$Sample.Pid),
        ([string]$Sample.Name),
        ([string][int64]$Sample.PrivateBytes),
        ([string][int64]$Sample.WorkingSet),
        ([string][int]$Sample.HandleCount),
        ([string][int]$Sample.ThreadCount),
        ($Sample.CpuSeconds.ToString('0.000', [Globalization.CultureInfo]::InvariantCulture))
    ) -join ','
    Write-SoakText -Path $Path -Text ($line + "`n") -Append
}

function Write-SoakCycleRow {
    param(
        [string]$Path,
        [datetime]$TimestampUtc,
        [int]$Slot,
        [string]$Account,
        [string]$Character,
        [int]$ExitCode,
        [double]$DurationSeconds,
        [bool]$TimedOut,
        [string]$EndReason,
        [ref]$HeaderWritten
    )
    if (-not $HeaderWritten.Value) {
        Write-SoakText -Path $Path -Text "timestamp_utc,slot,account,character,exit_code,duration_seconds,timed_out,end_reason`n"
        $HeaderWritten.Value = $true
    }
    $line = @(
        (Format-SoakUtc $TimestampUtc),
        ([string]$Slot),
        $Account,
        $Character,
        ([string]$ExitCode),
        ($DurationSeconds.ToString('0.000', [Globalization.CultureInfo]::InvariantCulture)),
        $(if ($TimedOut) { '1' } else { '0' }),
        $EndReason
    ) -join ','
    Write-SoakText -Path $Path -Text ($line + "`n") -Append
}

function Write-SoakDbList {
    param([string]$Phase, [string[]]$Names)
    $rows = @($Names | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    if ($rows.Count -eq 0) {
        Write-SoakLine ('SOAK DBLIST ' + $Phase)
        return
    }
    foreach ($name in $rows) {
        Write-SoakLine ('SOAK DBLIST ' + $Phase + ' ' + ([string]$name).Trim())
    }
}

function Invoke-SoakSqlCmd {
    param(
        [string]$SqlCmd,
        [string]$Server,
        [string]$Query,
        [string]$StdoutPath,
        [string]$StderrPath
    )
    if (-not (Test-CommercialSmokeLocalNamedInstance -Value $Server)) {
        return [pscustomobject]@{ ExitCode = 1; Names = @() }
    }
    $proc = Start-Process -FilePath $SqlCmd -ArgumentList @('-E', '-S', $Server, '-d', 'master', '-b', '-h-1', '-W', '-w65535', '-Q', $Query) -Wait -PassThru -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath
    $stdoutText = ''
    $stderrText = ''
    if ([IO.File]::Exists($StdoutPath)) { $stdoutText = [IO.File]::ReadAllText($StdoutPath) }
    if ([IO.File]::Exists($StderrPath)) { $stderrText = [IO.File]::ReadAllText($StderrPath) }
    $code = 1
    try { $code = [int]$proc.ExitCode } catch { $code = 1 }
    return [pscustomobject]@{
        ExitCode = $code
        Names = @(Get-CommercialSmokeSqlCmdLines -Output @($stdoutText, $stderrText))
    }
}

function Test-SoakLoopbackOpen {
    param([int]$Port)
    $tcp = [Net.Sockets.TcpClient]::new()
    try {
        $task = $tcp.ConnectAsync('127.0.0.1', $Port)
        if (-not $task.Wait(500)) { return $false }
        return [bool]$tcp.Connected
    } catch {
        return $false
    } finally {
        $tcp.Dispose()
    }
}

function Test-SoakPortFree {
    param([int]$Port)
    $listener = $null
    try {
        $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, $Port)
        $listener.ExclusiveAddressUse = $true
        $listener.Start()
        return $true
    } catch {
        return $false
    } finally {
        if ($null -ne $listener) {
            try { $listener.Stop() } catch { }
        }
    }
}

function Measure-SoakGuard {
    param(
        [string[]]$Paths,
        [double]$MinFreeGB
    )
    $free = @{}
    foreach ($path in @($Paths)) {
        if ([string]::IsNullOrWhiteSpace($path)) { continue }
        try {
            $free[$path] = Get-SoakFreeBytes -Path $path
        } catch {
            # Leave the key absent so Test-SoakGuardSpace reports unreadable.
        }
    }
    return Test-SoakGuardSpace -Paths $Paths -MinFreeGB $MinFreeGB -FreeBytesByPath $free
}

function Invoke-SoakRun {
    [CmdletBinding()]
    param(
        [string]$ServerBinDir = '',
        [string]$ClientExe = '',
        [string]$BuildRoot = '',
        [string]$RunDir = '',
        [string]$RunId = '',
        [double]$DurationHours = 24,
        [double]$WarmupHours = 1,
        [int]$SampleSeconds = 60,
        [int]$Clients = 2,
        [string[]]$CharacterPool = @(),
        [string]$MssqlDb = '',
        [string]$WatchRoot = '',
        [long]$WatchExpectedFiles = -1,
        [long]$WatchExpectedBytes = -1,
        [double]$MinFreeGB = 10,
        [string]$GuardDrive = '',
        [string[]]$GuardPaths = @(),
        [switch]$IncludeSqlDataGuard,
        [string]$PlayDhRoot = '',
        [int]$LoginPort = 26001,
        [int]$AgentPort = 27001,
        [int]$MapPort = 28001,
        [int]$Map12Port = 28002,
        [int]$MapNumber = 10,
        [int]$MaxSampleGapSeconds = 120,
        [int]$CycleTimeoutSeconds = 180,
        [int]$StartupTimeoutSeconds = 30,
        [int]$HoldSeconds = 0,
        [string]$ExpectPeer = '',
        [int]$DropSocketAfterSeconds = 0,
        [string]$StateDumpPath = '',
        [switch]$AssumeWindowsPlatform
    )
    $ErrorActionPreference = 'Stop'
    if (-not [string]::IsNullOrEmpty($script:soakHelperImportError)) {
        Write-SoakLine ('SOAK FAIL ' + $script:soakHelperImportError)
        return 1
    }
    if ([string]::IsNullOrWhiteSpace($RunId)) {
        $RunId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
    }
    $windows = Test-SoakWindows -Assume ([bool]$AssumeWindowsPlatform)
    $directory = Resolve-SoakRunDirectory -RunDir $RunDir -BuildRoot $BuildRoot -ServerBinDir $ServerBinDir -RunId $RunId -ForWindows $windows
    $slots = @(Get-SoakCharacterSlots -CharacterPool $CharacterPool -Clients $Clients -RunId $RunId)
    $loginExe = Resolve-SoakExecutable -ServerBinDir $ServerBinDir -BuildRoot $BuildRoot -FileName 'mxh_login_server.exe' -ToolDirs @('MoxianLoginServer')
    $agentExe = Resolve-SoakExecutable -ServerBinDir $ServerBinDir -BuildRoot $BuildRoot -FileName 'mxh_agent_server_CHINA.exe' -ToolDirs @('MoxianAgentServer')
    $mapExe = Resolve-SoakExecutable -ServerBinDir $ServerBinDir -BuildRoot $BuildRoot -FileName 'mxh_map_server_CHINA.exe' -ToolDirs @('MoxianMapServer')
    $dbTool = Resolve-SoakExecutable -ServerBinDir $ServerBinDir -BuildRoot $BuildRoot -FileName 'mxh_db_tool.exe' -ToolDirs @('MoxianDbTool')
    $refusal = Get-SoakLaunchRefusal -HostOk (Test-SoakHostAtLeast74) -RunDirRefusal $directory.Refusal -RunDir $directory.Path -WatchRoot $WatchRoot -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort -Map12Port $Map12Port -DurationHours $DurationHours -WarmupHours $WarmupHours -SampleSeconds $SampleSeconds -Clients $Clients -MaxSampleGapSeconds $MaxSampleGapSeconds -CycleTimeoutSeconds $CycleTimeoutSeconds -StartupTimeoutSeconds $StartupTimeoutSeconds -MinFreeGB $MinFreeGB -MapNumber $MapNumber -MssqlDb $MssqlDb -IncludeSqlDataGuard ([bool]$IncludeSqlDataGuard) -CharacterSlots $slots -ClientExe $ClientExe -PlayDhRoot $PlayDhRoot -DbTool $dbTool -LoginExe $loginExe -AgentExe $agentExe -MapExe $mapExe -HoldSeconds $HoldSeconds -ExpectPeer $ExpectPeer -DropSocketAfterSeconds $DropSocketAfterSeconds -RunId $RunId
    if (-not [string]::IsNullOrEmpty($refusal)) {
        Write-SoakLine ('SOAK FAIL ' + $refusal)
        return 1
    }

    $runPath = $directory.Path
    [void][IO.Directory]::CreateDirectory($runPath)
    $serverLogDir = Join-Path $runPath 'servers'
    $clientLogDir = Join-Path $runPath 'clients'
    [void][IO.Directory]::CreateDirectory($serverLogDir)
    [void][IO.Directory]::CreateDirectory($clientLogDir)
    $samplesPath = Join-Path $runPath 'samples.csv'
    $cyclesPath = Join-Path $runPath 'cycles.csv'
    $summaryPath = Join-Path $runPath 'summary.json'
    $sampleHeader = $false
    $cycleHeader = $false
    $guardList = @(Get-SoakGuardPathList -GuardDrive $GuardDrive -GuardPaths $GuardPaths)
    $backend = $(if ([string]::IsNullOrWhiteSpace($MssqlDb)) { 'sqlite' } else { 'mssql_odbc' })
    $mssqlTarget = $null
    if ($backend -eq 'mssql_odbc') { $mssqlTarget = Resolve-SoakMssqlTarget -ConnectionString $MssqlDb }
    $databaseConfig = ''
    $sqliteFile = Join-Path (Join-Path $runPath 'data') 'moxian.db'
    if ($backend -eq 'sqlite') { $databaseConfig = 'backend=sqlite;path=' + $sqliteFile }
    else { $databaseConfig = $MssqlDb }

    $startedServers = New-Object System.Collections.Generic.List[object]
    $runningClients = New-Object System.Collections.Generic.List[object]
    $createdDb = $false
    $sqliteCreated = $false
    $dbListedBefore = $false
    $sqlCmdPath = ''
    $abort = ''
    $fatal = ''
    $watchStart = $null
    $watchEnd = $null
    $guardStatus = 'skip'
    $guardBreaches = @()
    $roundTimes = New-Object System.Collections.Generic.List[datetime]
    $maxGap = 0.0
    $gapFailed = $false
    $cycleTotal = 0
    $cycleFailed = 0
    $durationMet = $false
    $startedUtc = [datetime]::UtcNow
    $cleanupNotes = New-Object System.Collections.Generic.List[string]
    $clientSeq = 0
    $code = 2

    try {
        if (-not [string]::IsNullOrWhiteSpace($WatchRoot)) {
            $watchStart = Get-SoakTreeCensus -Root $WatchRoot
            if ($null -eq $watchStart) { $abort = 'watch' }
        }
        if ([string]::IsNullOrEmpty($abort) -and $guardList.Count -gt 0) {
            $initialGuard = Measure-SoakGuard -Paths $guardList -MinFreeGB $MinFreeGB
            $guardStatus = [string]$initialGuard.Status
            $guardBreaches = @($initialGuard.Breaches)
            if ($guardStatus -eq 'fail') { $abort = 'guard' }
        }
        foreach ($listen in @($MapPort, $AgentPort, $LoginPort)) {
            if (-not [string]::IsNullOrEmpty($abort)) { break }
            if (-not (Test-SoakPortFree -Port $listen)) { $abort = ('port_in_use:' + $listen) }
        }
        if ([string]::IsNullOrEmpty($abort) -and $backend -eq 'mssql_odbc') {
            $sqlCmdPath = Get-CommercialSmokeSqlCmdPath
            if ([string]::IsNullOrWhiteSpace($sqlCmdPath)) { $abort = 'sqlcmd_missing' }
        }
        if ([string]::IsNullOrEmpty($abort) -and $backend -eq 'mssql_odbc') {
            $before = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakDatabaseListQuery) -StdoutPath (Join-Path $runPath 'sql-before.out.txt') -StderrPath (Join-Path $runPath 'sql-before.err.txt')
            if ($before.ExitCode -ne 0) { $abort = 'sql_list_before' }
            else {
                $dbListedBefore = $true
                Write-SoakDbList -Phase 'BEFORE' -Names $before.Names
                if (Test-CommercialSmokeDatabaseListed -Name $mssqlTarget.Database -Names $before.Names) {
                    $abort = 'database_exists'
                } else {
                    $created = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakMssqlCreateBatch -DatabaseName $mssqlTarget.Database) -StdoutPath (Join-Path $runPath 'sql-create.out.txt') -StderrPath (Join-Path $runPath 'sql-create.err.txt')
                    $afterCreate = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakDatabaseListQuery) -StdoutPath (Join-Path $runPath 'sql-created.out.txt') -StderrPath (Join-Path $runPath 'sql-created.err.txt')
                    if ($afterCreate.ExitCode -eq 0 -and (Test-CommercialSmokeDatabaseListed -Name $mssqlTarget.Database -Names $afterCreate.Names)) {
                        $createdDb = $true
                    }
                    if ($created.ExitCode -ne 0 -or -not $createdDb) { $abort = 'database_create' }
                }
            }
            if ([string]::IsNullOrEmpty($abort) -and $IncludeSqlDataGuard) {
                $paths = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakSqlDataPathQuery) -StdoutPath (Join-Path $runPath 'sql-paths.out.txt') -StderrPath (Join-Path $runPath 'sql-paths.err.txt')
                if ($paths.ExitCode -ne 0 -or @($paths.Names).Count -lt 1) { $abort = 'sql_data_path' }
                else {
                    foreach ($sqlPath in @($paths.Names)) { $guardList += ([string]$sqlPath).Trim() }
                    $sqlGuard = Measure-SoakGuard -Paths $guardList -MinFreeGB $MinFreeGB
                    $guardStatus = [string]$sqlGuard.Status
                    $guardBreaches = @($sqlGuard.Breaches)
                    if ($guardStatus -eq 'fail') { $abort = 'guard' }
                }
            }
        }
        if ([string]::IsNullOrEmpty($abort) -and $backend -eq 'sqlite') {
            $dataDir = [IO.Path]::GetDirectoryName($sqliteFile)
            [void][IO.Directory]::CreateDirectory($dataDir)
            $sqliteCreated = -not [IO.File]::Exists($sqliteFile)
        }
        if ([string]::IsNullOrEmpty($abort)) {
            $migrateEnv = New-SoakEnvironment -Extra @{
                MXH_DATABASE_CONFIG = $databaseConfig
                MXH_PLAYDH_ROOT = $PlayDhRoot
                MXH_PROCESS = 'db-tool'
            }
            $migrate = Start-SoakOwnedProcess -FilePath $dbTool -ArgumentList @('migrate', '--db-env', 'MXH_DATABASE_CONFIG') -StdoutPath (Join-Path $runPath 'db-tool.out.txt') -StderrPath (Join-Path $runPath 'db-tool.err.txt') -Environment $migrateEnv
            $migrate.Process.WaitForExit()
            $migrateCode = 1
            try { $migrateCode = [int]$migrate.Process.ExitCode } catch { $migrateCode = 1 }
            if ($migrateCode -ne 0) { $abort = 'schema_migrate' }
        }
        $resourceServer = Join-Path (Join-Path $PlayDhRoot 'Resource') 'Server'
        if ([string]::IsNullOrEmpty($abort)) {
            $plan = @(Get-SoakServerPlan -LoginExe $loginExe -AgentExe $agentExe -MapExe $mapExe -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort -MapNumber $MapNumber -ResourceRoot $PlayDhRoot -ServerResourceRoot $resourceServer -Backend $backend)
            foreach ($item in $plan) {
                $envMap = New-SoakEnvironment -Extra @{
                    MXH_DATABASE_CONFIG = $databaseConfig
                    MXH_PLAYDH_ROOT = $PlayDhRoot
                    MXH_PROCESS = $item.Role
                }
                $started = Start-SoakOwnedProcess -FilePath $item.Executable -ArgumentList @($item.Arguments) -StdoutPath (Join-Path $serverLogDir ($item.Role + '.out.log')) -StderrPath (Join-Path $serverLogDir ($item.Role + '.err.log')) -Environment $envMap
                $record = [pscustomobject]@{
                    Name = $item.Name
                    Role = $item.Role
                    Port = [int]$item.Port
                    Identity = $started.Identity
                    Process = $started.Process
                    Samples = (New-Object System.Collections.Generic.List[object])
                    ExitedEarly = $false
                    Reused = $false
                }
                $startedServers.Add($record)
                Start-Sleep -Milliseconds 500
                if ((Get-SoakIdentityState -Identity $record.Identity) -ne 'match') {
                    $record.ExitedEarly = $true
                    $abort = ('startup:' + $record.Name)
                    break
                }
            }
        }
        if ([string]::IsNullOrEmpty($abort)) {
            $deadline = [datetime]::UtcNow.AddSeconds($StartupTimeoutSeconds)
            foreach ($server in @($startedServers.ToArray())) {
                $ready = $false
                while ([datetime]::UtcNow -lt $deadline) {
                    if ((Get-SoakIdentityState -Identity $server.Identity) -ne 'match') {
                        $server.ExitedEarly = $true
                        $abort = ('startup:' + $server.Name)
                        break
                    }
                    if (Test-SoakLoopbackOpen -Port ([int]$server.Port)) { $ready = $true; break }
                    Start-Sleep -Milliseconds 200
                }
                if (-not [string]::IsNullOrEmpty($abort)) { break }
                if (-not $ready) { $abort = ('startup_timeout:' + $server.Name); break }
            }
        }
        $nextSample = [datetime]::UtcNow
        while ([string]::IsNullOrEmpty($abort)) {
            $now = [datetime]::UtcNow
            if (($now - $startedUtc).TotalHours -ge $DurationHours) {
                $durationMet = $true
                break
            }
            $still = New-Object System.Collections.Generic.List[object]
            foreach ($client in @($runningClients.ToArray())) {
                $state = Get-SoakIdentityState -Identity $client.Identity
                $exited = $false
                try { $client.Process.Refresh() } catch { }
                try { $exited = [bool]$client.Process.HasExited } catch { $exited = $true }
                $durationSeconds = ($now - $client.StartedUtc).TotalSeconds
                if ($state -eq 'reused') {
                    $cycleTotal++
                    $cycleFailed++
                    Write-SoakCycleRow -Path $cyclesPath -TimestampUtc $now -Slot $client.Slot -Account $client.Account -Character $client.Character -ExitCode -1 -DurationSeconds $durationSeconds -TimedOut $false -EndReason 'reused' -HeaderWritten ([ref]$cycleHeader)
                    continue
                }
                if ($exited -or $state -eq 'dead') {
                    $exitCode = 1
                    try { $exitCode = [int]$client.Process.ExitCode } catch { $exitCode = 1 }
                    $cycleTotal++
                    if ($exitCode -ne 0) { $cycleFailed++ }
                    $reason = $(if ($exitCode -eq 0) { 'ok' } else { 'fail' })
                    Write-SoakCycleRow -Path $cyclesPath -TimestampUtc $now -Slot $client.Slot -Account $client.Account -Character $client.Character -ExitCode $exitCode -DurationSeconds $durationSeconds -TimedOut $false -EndReason $reason -HeaderWritten ([ref]$cycleHeader)
                    continue
                }
                if ($durationSeconds -gt $CycleTimeoutSeconds) {
                    $stop = Stop-SoakOwnedTree -Identity $client.Identity
                    if ($stop.Action -eq 'reused') { $cleanupNotes.Add('client_reused:' + $client.Account) }
                    $cycleTotal++
                    $cycleFailed++
                    Write-SoakCycleRow -Path $cyclesPath -TimestampUtc $now -Slot $client.Slot -Account $client.Account -Character $client.Character -ExitCode -1 -DurationSeconds $durationSeconds -TimedOut $true -EndReason 'timeout' -HeaderWritten ([ref]$cycleHeader)
                    continue
                }
                $still.Add($client)
            }
            $runningClients = $still
            $occupied = @{}
            foreach ($client in @($runningClients.ToArray())) { $occupied[[string]$client.Slot] = $true }
            for ($slotIndex = 0; $slotIndex -lt $Clients; $slotIndex++) {
                if ($occupied.ContainsKey([string]$slotIndex)) { continue }
                $slot = $slots[$slotIndex]
                $clientArgs = @(Get-SoakClientArguments -Account $slot.Account -Character $slot.Character -LoginPort $LoginPort -AgentPort $AgentPort -MapPort $MapPort -Map12Port $Map12Port -MapNumber $MapNumber -TimeoutSeconds $CycleTimeoutSeconds -HoldSeconds $HoldSeconds -ExpectPeer $ExpectPeer -DropSocketAfterSeconds $DropSocketAfterSeconds -StateDumpPath $StateDumpPath)
                $clientSeq++
                $stdout = Join-Path $clientLogDir ('{0:D4}.out.log' -f $clientSeq)
                $stderr = Join-Path $clientLogDir ('{0:D4}.err.log' -f $clientSeq)
                try {
                    $clientEnv = New-SoakEnvironment -Extra @{ MXH_PLAYDH_ROOT = $PlayDhRoot; MXH_PROCESS = 'client' }
                    $startedClient = Start-SoakOwnedProcess -FilePath $ClientExe -ArgumentList $clientArgs -StdoutPath $stdout -StderrPath $stderr -Environment $clientEnv
                    $runningClients.Add([pscustomobject]@{
                        Slot = $slotIndex
                        Account = $slot.Account
                        Character = $slot.Character
                        Identity = $startedClient.Identity
                        Process = $startedClient.Process
                        StartedUtc = [datetime]::UtcNow
                    })
                } catch {
                    $cycleTotal++
                    $cycleFailed++
                    Write-SoakCycleRow -Path $cyclesPath -TimestampUtc ([datetime]::UtcNow) -Slot $slotIndex -Account $slot.Account -Character $slot.Character -ExitCode -1 -DurationSeconds 0 -TimedOut $false -EndReason 'start_failed' -HeaderWritten ([ref]$cycleHeader)
                    Start-Sleep -Seconds 1
                }
            }
            $now = [datetime]::UtcNow
            if ($now -ge $nextSample) {
                if ($roundTimes.Count -gt 0) {
                    $gap = ($now - $roundTimes[$roundTimes.Count - 1]).TotalSeconds
                    if ($gap -gt $maxGap) { $maxGap = $gap }
                    if ($gap -gt $MaxSampleGapSeconds) { $gapFailed = $true; $abort = 'sample_gap' }
                }
                $roundTimes.Add($now)
                $elapsedHours = ($now - $startedUtc).TotalHours
                foreach ($server in @($startedServers.ToArray())) {
                    $sample = Get-SoakProcessSample -Identity $server.Identity -Name $server.Name -TimestampUtc $now
                    if (-not $sample.Alive) {
                        $server.ExitedEarly = $true
                        if ($sample.State -eq 'reused') { $server.Reused = $true; $abort = ('server_reused:' + $server.Name) }
                        else { $abort = ('server_exit:' + $server.Name) }
                        break
                    }
                    $server.Samples.Add([pscustomobject]@{
                        THours = $elapsedHours
                        PrivateBytes = [int64]$sample.PrivateBytes
                        Handles = [int]$sample.HandleCount
                    })
                    Write-SoakSampleRow -Path $samplesPath -Sample $sample -HeaderWritten ([ref]$sampleHeader)
                }
                if ([string]::IsNullOrEmpty($abort) -and $guardList.Count -gt 0) {
                    $liveGuard = Measure-SoakGuard -Paths $guardList -MinFreeGB $MinFreeGB
                    $guardStatus = [string]$liveGuard.Status
                    if (@($liveGuard.Breaches).Count -gt 0) { $guardBreaches = @($liveGuard.Breaches) }
                    if ($guardStatus -eq 'fail') { $abort = 'guard' }
                }
                $nextSample = $now.AddSeconds($SampleSeconds)
            }
            if (-not [string]::IsNullOrEmpty($abort)) { break }
            Start-Sleep -Milliseconds 200
        }
    } catch {
        $fatal = $_.Exception.Message
    } finally {
        foreach ($client in @($runningClients.ToArray())) {
            $stopClient = Stop-SoakOwnedTree -Identity $client.Identity
            if ($stopClient.Action -eq 'reused') { $cleanupNotes.Add('client_reused:' + $client.Account) }
            $durationSeconds = ([datetime]::UtcNow - $client.StartedUtc).TotalSeconds
            Write-SoakCycleRow -Path $cyclesPath -TimestampUtc ([datetime]::UtcNow) -Slot $client.Slot -Account $client.Account -Character $client.Character -ExitCode -2 -DurationSeconds $durationSeconds -TimedOut $false -EndReason 'harness_stop' -HeaderWritten ([ref]$cycleHeader)
        }
        for ($index = $startedServers.Count - 1; $index -ge 0; $index--) {
            $server = $startedServers[$index]
            $stopServer = Stop-SoakOwnedTree -Identity $server.Identity
            if ($stopServer.Action -eq 'reused') {
                $server.Reused = $true
                $cleanupNotes.Add('server_reused:' + $server.Name)
            }
        }
        $dropPlan = Get-SoakDatabaseCleanupPlan -CreatedByThisRun $createdDb
        if ($dropPlan.Drop -and -not [string]::IsNullOrWhiteSpace($sqlCmdPath) -and $null -ne $mssqlTarget) {
            $dropped = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakMssqlDropBatch -DatabaseName $mssqlTarget.Database) -StdoutPath (Join-Path $runPath 'sql-drop.out.txt') -StderrPath (Join-Path $runPath 'sql-drop.err.txt')
            if ($dropped.ExitCode -ne 0) { $cleanupNotes.Add('database_drop') }
        }
        if ($dbListedBefore -and -not [string]::IsNullOrWhiteSpace($sqlCmdPath) -and $null -ne $mssqlTarget) {
            $after = Invoke-SoakSqlCmd -SqlCmd $sqlCmdPath -Server $mssqlTarget.Server -Query (Get-SoakDatabaseListQuery) -StdoutPath (Join-Path $runPath 'sql-after.out.txt') -StderrPath (Join-Path $runPath 'sql-after.err.txt')
            if ($after.ExitCode -eq 0) { Write-SoakDbList -Phase 'AFTER' -Names $after.Names }
            else { $cleanupNotes.Add('sql_list_after'); Write-SoakLine 'SOAK DBLIST AFTER' }
        }
        if ($sqliteCreated -and [IO.File]::Exists($sqliteFile)) {
            try { [IO.File]::Delete($sqliteFile) } catch { $cleanupNotes.Add('sqlite_delete') }
        }
        if (-not [string]::IsNullOrWhiteSpace($WatchRoot)) {
            $watchEnd = Get-SoakTreeCensus -Root $WatchRoot
        }
        $endedUtc = [datetime]::UtcNow
        $elapsed = ($endedUtc - $startedUtc).TotalHours
        if ($durationMet -and $elapsed -lt $DurationHours) { $elapsed = $DurationHours }
        $watchStatus = 'skip'
        if (-not [string]::IsNullOrWhiteSpace($WatchRoot)) {
            $watchResult = Get-SoakWatchStatus -Start $watchStart -End $watchEnd -ExpectedFiles $WatchExpectedFiles -ExpectedBytes $WatchExpectedBytes
            $watchStatus = [string]$watchResult.Status
            if ($watchStatus -eq 'fail') { $cleanupNotes.Add([string]$watchResult.Reason) }
        }
        if ($guardList.Count -eq 0) { $guardStatus = 'skip' }
        $gapStatus = 'skip'
        if ($gapFailed) { $gapStatus = 'fail' }
        elseif ($roundTimes.Count -ge 2) { $gapStatus = 'pass' }
        $verdictServers = New-Object System.Collections.Generic.List[object]
        foreach ($server in @($startedServers.ToArray())) {
            $verdictServers.Add([pscustomobject]@{
                Name = $server.Name
                ExitedEarly = [bool]$server.ExitedEarly
                Reused = [bool]$server.Reused
                Samples = @($server.Samples.ToArray())
            })
        }
        $extra = New-Object System.Collections.Generic.List[string]
        if (-not [string]::IsNullOrWhiteSpace($fatal)) { $extra.Add('fatal') }
        if (-not [string]::IsNullOrWhiteSpace($abort) -and $abort -ne 'sample_gap' -and $abort -ne 'guard' -and $abort -notlike 'server_exit:*' -and $abort -notlike 'server_reused:*') {
            $extra.Add($abort)
        }
        foreach ($note in @($cleanupNotes.ToArray())) {
            if (-not [string]::IsNullOrWhiteSpace($note)) { $extra.Add($note) }
        }
        $verdict = Get-SoakVerdict -ElapsedHours $elapsed -DurationHours $DurationHours -WarmupHours $WarmupHours -Servers @($verdictServers.ToArray()) -CycleTotal $cycleTotal -CycleFailed $cycleFailed -WatchStatus $watchStatus -GuardStatus $guardStatus -GapStatus $gapStatus -ExtraReasons @($extra.ToArray())
        $summary = [ordered]@{
            verdict = $(if ($verdict.Pass) { 'PASS' } else { 'FAIL' })
            line = $verdict.Line
            reasons = @($verdict.Reasons)
            skipped = @($verdict.Skipped)
            exit_code = $(if ($verdict.Pass) { 0 } else { 2 })
            run_id = $RunId
            started_at = (Format-SoakUtc $startedUtc)
            ended_at = (Format-SoakUtc $endedUtc)
            duration_hours = $DurationHours
            elapsed_hours = $elapsed
            warmup_hours = $WarmupHours
            sample_seconds = $SampleSeconds
            max_sample_gap_seconds = $MaxSampleGapSeconds
            observed_max_sample_gap_seconds = $maxGap
            clients = $Clients
            cycle_total = $cycleTotal
            cycle_failed = $cycleFailed
            cycle_failure_rate = $verdict.FailureRate
            failure_rate_limit = 0.001
            private_slope_limit_mb_per_hour = 1
            private_ratio = 1.10
            private_allowance_bytes = 20971520
            handle_slope_limit_per_hour = 10
            handle_ratio = 1.10
            handle_allowance = 50
            position_tolerance = 32
            reconnect = 'full re-login after forced disconnect; no auto-resume'
            backend = $backend
            database = $(if ($null -ne $mssqlTarget) { $mssqlTarget.Database } else { 'sqlite' })
            database_created = [bool]$createdDb
            database_dropped = [bool]($dropPlan.Drop -and -not ($cleanupNotes -contains 'database_drop'))
            ports = [ordered]@{ login = $LoginPort; agent = $AgentPort; map = $MapPort; map12 = $Map12Port }
            map_number = $MapNumber
            playdh_root = $PlayDhRoot
            run_dir = $runPath
            watch_status = $watchStatus
            watch_start_files = $(if ($null -eq $watchStart) { $null } else { [int64]$watchStart.Files })
            watch_start_bytes = $(if ($null -eq $watchStart) { $null } else { [int64]$watchStart.Bytes })
            watch_end_files = $(if ($null -eq $watchEnd) { $null } else { [int64]$watchEnd.Files })
            watch_end_bytes = $(if ($null -eq $watchEnd) { $null } else { [int64]$watchEnd.Bytes })
            guard_status = $guardStatus
            guard_breaches = @($guardBreaches)
            min_free_gb = $MinFreeGB
            servers = @($verdict.Servers)
            hooks = [ordered]@{
                hold_seconds = $HoldSeconds
                expect_peer = $ExpectPeer
                drop_socket_after = $DropSocketAfterSeconds
                state_dump = $StateDumpPath
            }
        }
        Write-SoakText -Path $summaryPath -Text (($summary | ConvertTo-Json -Depth 8) + "`n")
        Write-SoakLine $verdict.Line
        if ($verdict.Pass) { $code = 0 } else { $code = 2 }
    }
    return $code
}

if ($soakHelperImportError -and $MyInvocation.InvocationName -ne '.') {
    Write-SoakLine ('SOAK FAIL ' + $soakHelperImportError)
    Write-SoakLine 'EXIT=1'
    exit 1
}

if ($MyInvocation.InvocationName -ne '.') {
    $soakExit = 1
    try {
        $soakExit = Invoke-SoakRun @PSBoundParameters
    } catch {
        Write-SoakLine ('SOAK FAIL ' + $_.Exception.Message)
        $soakExit = 1
    }
    Write-SoakLine ('EXIT=' + $soakExit)
    exit $soakExit
}
