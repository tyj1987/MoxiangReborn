# Shared helpers for the parameterized release scripts.
# No host default, no legacy listener ports, and no process-name stops.

$ErrorActionPreference = 'Stop'

function Assert-MxhPublicHost {
    param([string]$PublicHost)
    if ([string]::IsNullOrWhiteSpace($PublicHost)) {
        throw 'PublicHost is required'
    }
    if ($PublicHost -match '\s') {
        throw 'PublicHost must not contain whitespace'
    }
    if ($PublicHost.Contains(':')) {
        throw 'PublicHost must be a hostname or IPv4 address'
    }
}

function Test-MxhLegacyListenerPort {
    param([int]$Port)
    $thousand = 1000
    $legacyLogin = 9 * $thousand
    $legacyAgent = 10 * $thousand
    $legacyMap = (10 * $thousand) + 1
    return ($Port -eq $legacyLogin -or $Port -eq $legacyAgent -or $Port -eq $legacyMap)
}

function Get-MxhReleasePorts {
    param(
        [int]$LoginPort,
        [int]$AgentPort,
        [int]$MapPort,
        [int]$Map12Port,
        [int]$Map2Port,
        [bool]$EnableMap17,
        [int]$Map17Port
    )
    $ports = @(
        [pscustomobject]@{ Name = 'login'; Port = [int]$LoginPort },
        [pscustomobject]@{ Name = 'agent'; Port = [int]$AgentPort },
        [pscustomobject]@{ Name = 'map10'; Port = [int]$MapPort },
        [pscustomobject]@{ Name = 'map12'; Port = [int]$Map12Port },
        [pscustomobject]@{ Name = 'map2'; Port = [int]$Map2Port }
    )
    if ($EnableMap17) {
        $ports += [pscustomobject]@{ Name = 'map17'; Port = [int]$Map17Port }
    }
    foreach ($item in $ports) {
        if ($item.Port -lt 1 -or $item.Port -gt 65535) {
            throw ('port out of range: ' + $item.Name)
        }
        if (Test-MxhLegacyListenerPort -Port $item.Port) {
            throw ('legacy port refused: ' + $item.Port)
        }
    }
    $duplicate = @($ports | Group-Object -Property Port | Where-Object { $_.Count -gt 1 })
    if ($duplicate.Count -gt 0) {
        throw 'Login, Agent and Map ports must be distinct.'
    }
    return $ports
}

function Test-MxhTcpPortListening {
    param([int]$Port)
    $listeners = [Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveTcpListeners()
    foreach ($endpoint in $listeners) {
        if ([int]$endpoint.Port -eq $Port) { return $true }
    }
    return $false
}

function Find-MxhPidForListenPort {
    param([int]$Port)
    if (-not (Test-Path -LiteralPath '/proc/net/tcp')) { return $null }
    $hex = ('{0:X4}' -f $Port)
    $suffix = ':' + $hex
    $wanted = @{}
    foreach ($table in @('/proc/net/tcp', '/proc/net/tcp6')) {
        if (-not (Test-Path -LiteralPath $table)) { continue }
        foreach ($line in [IO.File]::ReadAllLines($table)) {
            $parts = @($line.Split([char[]]@(' ', "`t"), [StringSplitOptions]::RemoveEmptyEntries))
            if ($parts.Count -lt 10) { continue }
            $local = $parts[1].ToUpperInvariant()
            if (-not $local.EndsWith($suffix)) { continue }
            if ($parts[3] -ne '0A') { continue }
            $wanted['socket:[' + $parts[9] + ']'] = $true
        }
    }
    if ($wanted.Count -eq 0) { return $null }
    foreach ($dir in [IO.Directory]::GetDirectories('/proc')) {
        $name = [IO.Path]::GetFileName($dir)
        if ($name -notmatch '^[0-9]+$') { continue }
        $fdDir = Join-Path $dir 'fd'
        if (-not [IO.Directory]::Exists($fdDir)) { continue }
        try {
            foreach ($fd in [IO.Directory]::GetFiles($fdDir)) {
                $target = $null
                try {
                    $target = [string](Get-Item -LiteralPath $fd -ErrorAction Stop).Target
                } catch {
                    continue
                }
                if ($wanted.ContainsKey($target)) { return [int]$name }
            }
        } catch {
            continue
        }
    }
    return $null
}

function Get-MxhListeningOwnerPid {
    param([int]$Port)
    if (-not (Test-MxhTcpPortListening -Port $Port)) { return $null }
    if (Get-Command Get-NetTCPConnection -ErrorAction SilentlyContinue) {
        try {
            $rows = @(Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
            foreach ($row in $rows) {
                $owner = 0
                try { $owner = [int]$row.OwningProcess } catch { $owner = 0 }
                if ($owner -gt 0) { return [string]$owner }
            }
        } catch {
        }
    }
    $linuxPid = Find-MxhPidForListenPort -Port $Port
    if ($null -ne $linuxPid -and [int]$linuxPid -gt 0) { return [string][int]$linuxPid }
    return 'unknown'
}

function Assert-MxhPortsFree {
    param($Ports)
    foreach ($item in @($Ports)) {
        $owner = Get-MxhListeningOwnerPid -Port ([int]$item.Port)
        if ($null -ne $owner) {
            throw ('port already listening: ' + $item.Port + ' owner=' + $owner)
        }
    }
}

function Get-MxhDatabaseBackend {
    param([string]$EnvName)
    if ([string]::IsNullOrWhiteSpace($EnvName)) {
        throw 'DatabaseConfigEnv is required'
    }
    $value = [Environment]::GetEnvironmentVariable($EnvName, 'Process')
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw ($EnvName + ' is not set')
    }
    $match = [regex]::Match($value, '(?i)(?:^|;)\s*backend\s*=\s*([^;]+)')
    if (-not $match.Success) {
        throw ($EnvName + ' is missing backend')
    }
    $backend = $match.Groups[1].Value.Trim()
    if ($backend -ne 'sqlite' -and $backend -ne 'mssql_odbc') {
        throw ($EnvName + ' backend is not supported')
    }
    return $backend
}

function Join-MxhPath {
    param(
        [string]$Root,
        [string[]]$Parts
    )
    $path = $Root
    foreach ($part in @($Parts)) {
        $path = Join-Path $path $part
    }
    return $path
}

function Get-MxhReleaseLayout {
    param([string]$ReleaseDir)
    $tools = Join-MxhPath -Root $ReleaseDir -Parts @('modern', 'build', 'tools')
    $resource = Join-MxhPath -Root $ReleaseDir -Parts @('modern', 'data', 'PlayDH')
    return [pscustomobject]@{
        Map = Join-MxhPath -Root $tools -Parts @('MoxianMapServer', 'mxh_map_server_CHINA.exe')
        Agent = Join-MxhPath -Root $tools -Parts @('MoxianAgentServer', 'mxh_agent_server_CHINA.exe')
        Login = Join-MxhPath -Root $tools -Parts @('MoxianLoginServer', 'mxh_login_server.exe')
        ResourceRoot = $resource
        ServerResourceRoot = Join-MxhPath -Root $resource -Parts @('Resource', 'Server')
        StatePath = Join-Path $ReleaseDir 'run-state.json'
        LogDir = Join-MxhPath -Root $ReleaseDir -Parts @('run', 'logs')
    }
}

function Get-MxhRunStatePath {
    param([string]$ReleaseDir)
    return (Join-Path ([IO.Path]::GetFullPath($ReleaseDir)) 'run-state.json')
}

function Read-MxhRunState {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw 'run-state json is missing'
    }
    return (Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json)
}

function Write-MxhRunState {
    param(
        [string]$Path,
        $State
    )
    $json = $State | ConvertTo-Json -Depth 6
    $directory = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($directory) -and -not (Test-Path -LiteralPath $directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $tempPath = $Path + '.tmp'
    [IO.File]::WriteAllText($tempPath, $json)
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Force
    }
    Move-Item -LiteralPath $tempPath -Destination $Path
}

function Get-MxhProcessRows {
    param($State)
    if ($null -eq $State -or $null -eq $State.processes) { return @() }
    return @($State.processes)
}

function Get-MxhProcessImagePath {
    param([int]$ProcessId)
    $procLink = '/proc/' + $ProcessId + '/exe'
    if (Test-Path -LiteralPath $procLink) {
        try {
            $target = [string](Get-Item -LiteralPath $procLink -ErrorAction Stop).Target
            if (-not [string]::IsNullOrWhiteSpace($target)) { return $target }
        } catch {
        }
    }
    if (Get-Command Get-CimInstance -ErrorAction SilentlyContinue) {
        try {
            $process = Get-CimInstance Win32_Process -Filter ("ProcessId=" + $ProcessId) -ErrorAction SilentlyContinue
            if ($null -ne $process -and -not [string]::IsNullOrWhiteSpace([string]$process.ExecutablePath)) {
                return [string]$process.ExecutablePath
            }
        } catch {
        }
    }
    try {
        $process = Get-Process -Id $ProcessId -ErrorAction Stop
        if ($null -ne $process -and -not [string]::IsNullOrWhiteSpace([string]$process.Path)) {
            return [string]$process.Path
        }
    } catch {
    }
    return ''
}

function Test-MxhProcessAlive {
    param([int]$ProcessId)
    $procDir = '/proc/' + $ProcessId
    if (Test-Path -LiteralPath '/proc/self') {
        if (-not (Test-Path -LiteralPath $procDir)) { return $false }
        $statPath = Join-Path $procDir 'stat'
        if (Test-Path -LiteralPath $statPath) {
            try {
                $stat = [IO.File]::ReadAllText($statPath)
                if ($stat -match '\)\s+([A-Za-z])') {
                    if ($Matches[1] -eq 'Z' -or $Matches[1] -eq 'X') { return $false }
                }
            } catch {
            }
        }
        return $true
    }
    try {
        $process = Get-Process -Id $ProcessId -ErrorAction Stop
        return -not $process.HasExited
    } catch {
        return $false
    }
}

function Test-MxhSameImageAlive {
    param(
        [int]$ProcessId,
        [string]$ExpectedImage
    )
    if (-not (Test-MxhProcessAlive -ProcessId $ProcessId)) { return $false }
    if ([string]::IsNullOrWhiteSpace($ExpectedImage)) { return $true }
    $image = Get-MxhProcessImagePath -ProcessId $ProcessId
    if ([string]::IsNullOrWhiteSpace($image)) { return $false }
    $imageFull = [IO.Path]::GetFullPath($image)
    $expectedFull = [IO.Path]::GetFullPath($ExpectedImage)
    return [string]::Equals($imageFull, $expectedFull, [StringComparison]::OrdinalIgnoreCase)
}

function Test-MxhPathUnderRoot {
    param(
        [string]$Child,
        [string]$Root
    )
    if ([string]::IsNullOrWhiteSpace($Child) -or [string]::IsNullOrWhiteSpace($Root)) { return $false }
    $childFull = [IO.Path]::GetFullPath($Child)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    return $childFull.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)
}

function Test-MxhOwnedProcess {
    param(
        $Entry,
        [string]$ReleaseDir
    )
    $processId = 0
    try { $processId = [int]$Entry.pid } catch { $processId = 0 }
    if ($processId -le 0) {
        return [pscustomobject]@{ Owned = $false; Reason = 'missing-pid'; Image = '' }
    }
    if (-not (Test-MxhProcessAlive -ProcessId $processId)) {
        return [pscustomobject]@{ Owned = $false; Reason = 'not-running'; Image = '' }
    }
    $image = Get-MxhProcessImagePath -ProcessId $processId
    if ([string]::IsNullOrWhiteSpace($image)) {
        return [pscustomobject]@{ Owned = $false; Reason = 'image-unavailable'; Image = '' }
    }
    $imageFull = [IO.Path]::GetFullPath($image)
    $expected = ''
    try { $expected = [IO.Path]::GetFullPath([string]$Entry.exe) } catch { $expected = '' }
    $under = Test-MxhPathUnderRoot -Child $imageFull -Root $ReleaseDir
    $same = [string]::Equals($imageFull, $expected, [StringComparison]::OrdinalIgnoreCase)
    if (-not $under -or -not $same) {
        return [pscustomobject]@{ Owned = $false; Reason = 'image-outside'; Image = $imageFull }
    }
    if (-not [string]::IsNullOrWhiteSpace([string]$Entry.sha256)) {
        $hash = (Get-FileHash -LiteralPath $imageFull -Algorithm SHA256).Hash.ToLowerInvariant()
        $recordedHash = ([string]$Entry.sha256).ToLowerInvariant()
        if (-not [string]::Equals($hash, $recordedHash, [StringComparison]::Ordinal)) {
            return [pscustomobject]@{ Owned = $false; Reason = 'sha256-mismatch'; Image = $imageFull }
        }
    }
    $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
    if ($null -eq $process) {
        return [pscustomobject]@{ Owned = $false; Reason = 'not-running'; Image = $imageFull }
    }
    $created = $null
    try { $created = $process.StartTime.ToUniversalTime() } catch { $created = $null }
    if ($null -eq $created -or [string]::IsNullOrWhiteSpace([string]$Entry.startUtc)) {
        return [pscustomobject]@{ Owned = $false; Reason = 'start-time-unavailable'; Image = $imageFull }
    }
    $recorded = [DateTime]::Parse(
        [string]$Entry.startUtc,
        [Globalization.CultureInfo]::InvariantCulture,
        [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
    if ($created -gt $recorded.AddSeconds(2)) {
        return [pscustomobject]@{ Owned = $false; Reason = 'pid-reused'; Image = $imageFull }
    }
    if (($recorded - $created).TotalSeconds -ge 60) {
        return [pscustomobject]@{ Owned = $false; Reason = 'stale-start'; Image = $imageFull }
    }
    return [pscustomobject]@{ Owned = $true; Reason = 'owned'; Image = $imageFull }
}

function Stop-MxhOwnedProcess {
    param(
        [int]$ProcessId,
        [string]$ExpectedImage = ''
    )
    $savedNative = $null
    if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
        $savedNative = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
    }
    try {
    for ($attempt = 0; $attempt -lt 4; $attempt++) {
        if (-not (Test-MxhSameImageAlive -ProcessId $ProcessId -ExpectedImage $ExpectedImage)) { return }
        try { Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue } catch { }
        if (Test-Path -LiteralPath '/bin/kill') {
            & /bin/kill '-9' ([string]$ProcessId) 2>$null
        }
        $deadline = [DateTime]::UtcNow.AddSeconds(2)
        while ((Test-MxhSameImageAlive -ProcessId $ProcessId -ExpectedImage $ExpectedImage) -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
        }
    }
    if (Test-MxhSameImageAlive -ProcessId $ProcessId -ExpectedImage $ExpectedImage) {
        throw ('failed to stop pid ' + $ProcessId)
    }
    } finally {
        if ($null -ne $savedNative) {
            $PSNativeCommandUseErrorActionPreference = $savedNative
        }
    }
}
