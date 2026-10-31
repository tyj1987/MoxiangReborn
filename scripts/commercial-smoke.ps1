<#
.SYNOPSIS
    Commercial smoke: filtered ctest, MSSQL E2E, and GUI client smoke.

.DESCRIPTION
    Exit codes:
      0  ctest passed. MSSQL E2E passed, was skipped with -SkipMssql, or was
         skipped because mxh_client_e2e is not built while LocalDB resolved.
         GUI smoke passed or -SkipGui was set.
      1  a step failed, the owned executable timed out, GUI smoke refused
         a host below PowerShell 7.4, -MssqlDb was rejected before connect,
         the named database already existed, or DROP DATABASE failed after E2E.
      6  MSSQL_E2E E2E_UNAVAILABLE. sqllocaldb / SqlLocalDB did not resolve
         on PATH and -SkipMssql was not passed. The run prints
         MSSQL_E2E E2E_UNAVAILABLE (LocalDB not found; pass -SkipMssql to record as not-run)
         and stops. That result is neither SKIPPED nor PASS.
         -MssqlDb does not use this path: the connection string is the
         prerequisite. 2 through 5 are left unused here so a direct exit
         stays distinct from scripts/clean-deploy.ps1, which already uses
         those codes.

    -LocalDbProbe is an optional scriptblock. When it is set, that block
    is the LocalDB check for this run. Production omits it and calls
    Test-CommercialSmokeLocalDbAvailable. The probe does not read an
    environment variable. -MssqlDb does not call the probe.

    -MssqlDb is an ODBC or mxh_client_e2e kv connection string. It is
    passed verbatim to mxh_client_e2e --db. A connection string that
    contains whitespace or a quote character (" or ') is rejected
    before connect: Start-Process -ArgumentList does not quote array
    elements on Windows, so those characters would split --db.
    The database= value must
    match ^MxE2E_[A-Za-z0-9_]{1,100}$. sqlcmd always runs as
    sqlcmd -E -S <host> for the existence check, DROP, and the
    post-drop check. <host> is the host= value. There is no call
    without -S and no default-instance fallback. host= is required
    exactly once. server=, data source=, addr=, address=, and
    network address= are rejected. The host must be a local named
    instance: machine is ., (local), localhost, or 127.0.0.1, then
    \ and an instance name [A-Za-z0-9_]+. A bare machine, a remote
    host, and a port suffix are rejected before connect.
    -SkipMssql together with -MssqlDb is refused.
    The explicit -MssqlDb path and the LocalDB path start
    mxh_client_e2e through Invoke-CommercialSmokeOwnedExecutable.
    On Windows that helper starts the child with CreateProcess
    CREATE_SUSPENDED, assigns only that child to a new job whose
    LimitFlags include JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE, then
    ResumeThread. The PowerShell process is not placed in the job.
    Child stdout and stderr are files, printed after the child
    exits, so a descendant cannot hold this script's output pipe.
    TimeoutSeconds on that helper defaults to 600. If the child is
    still running, the helper prints COMMERCIAL_SMOKE TIMEOUT and
    returns exit code 1. Closing the job handle then reaps leftover
    descendants. On Linux the helper still uses Start-Process
    -PassThru and waits until that child exits. Both platforms
    remember the process plus descendants the scan records.
    Cleanup stops only those PIDs, and only while StartTime still
    matches (UTC ticks on Windows, boot-relative starttime on
    Linux). Any fallback after Kill() re-checks that identity
    immediately before signaling the PID. Cleanup does not stop
    other processes by image name.

.EXAMPLE
    pwsh -NoProfile -File scripts/commercial-smoke.ps1 -MssqlDb "backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_RogSmoke;encrypt=no;trust_server_certificate=yes;"
#>
param(
    [string]$BuildDir = '',
    [switch]$RepeatFlaky,
    [switch]$SkipMssql,
    [switch]$SkipGui,
    [string]$MssqlDb = '',
    [scriptblock]$LocalDbProbe
)

function Test-CommercialSmokeHostAtLeast74 {
    param($Version)
    if ($null -eq $Version) { return $false }
    $major = 0
    $minor = 0
    try {
        $major = [int]$Version.Major
        $minor = [int]$Version.Minor
    } catch {
        return $false
    }
    if ($major -gt 7) { return $true }
    if ($major -eq 7 -and $minor -ge 4) { return $true }
    return $false
}

function Invoke-CommercialSmokeGuiClient {
    param(
        [string]$BuildDir,
        $HostVersion,
        [scriptblock]$Runner,
        [string]$ScriptRoot
    )
    # HostVersion is the seam tests use to simulate Windows PowerShell 5.1.
    # Production passes $PSVersionTable.PSVersion. Below 7.4 this returns
    # before the GUI smoke script is started. There is no pwsh relaunch and
    # no fixed-port fallback.
    $message = '[ERROR] commercial-smoke requires PowerShell 7.4+ (pwsh) for -EphemeralLocalServer; run it with pwsh 7.4+'
    if (-not (Test-CommercialSmokeHostAtLeast74 -Version $HostVersion)) {
        Write-Host $message
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
            Message = $message
        }
    }
    if ([string]::IsNullOrWhiteSpace($ScriptRoot)) { $ScriptRoot = $PSScriptRoot }
    if ($null -ne $Runner) {
        & $Runner -File (Join-Path $ScriptRoot 'gui-client-smoke.ps1') -BuildDir $BuildDir -EphemeralLocalServer
        return [pscustomobject]@{
            ExitCode = 0
            Invoked = $true
            Message = ''
        }
    }
    & (Join-Path $PSScriptRoot 'gui-client-smoke.ps1') -BuildDir $BuildDir -EphemeralLocalServer
    $code = 0
    if ($null -ne $LASTEXITCODE) { $code = [int]$LASTEXITCODE }
    return [pscustomobject]@{
        ExitCode = $code
        Invoked = $true
        Message = ''
    }
}

function Split-CommercialSmokeConnectionSegments {
    param([string]$ConnectionString)
    # Brace depth keeps a semicolon inside {Driver=...} from splitting.
    # A quote is kept as part of the value so a quoted database name
    # fails the name check instead of being unwrapped.
    $segments = New-Object System.Collections.Generic.List[string]
    $current = ''
    $depth = 0
    $inQuote = $false
    foreach ($ch in $ConnectionString.ToCharArray()) {
        if ($inQuote) {
            $current = $current + $ch
            if ($ch -eq '"') { $inQuote = $false }
            continue
        }
        if ($ch -eq '"') {
            $current = $current + $ch
            $inQuote = $true
            continue
        }
        if ($ch -eq '{') {
            $depth = $depth + 1
            $current = $current + $ch
            continue
        }
        if ($ch -eq '}') {
            if ($depth -gt 0) { $depth = $depth - 1 }
            $current = $current + $ch
            continue
        }
        if ($ch -eq ';' -and $depth -eq 0) {
            $segments.Add($current)
            $current = ''
            continue
        }
        $current = $current + $ch
    }
    if ($inQuote -or $depth -ne 0) { return $null }
    $segments.Add($current)
    return $segments
}

function Test-CommercialSmokeDatabaseName {
    param([string]$Name)
    # Case-sensitive. MxE2E_ is required. Brackets, quotes, and spaces fail.
    return [regex]::IsMatch($Name, '^MxE2E_[A-Za-z0-9_]{1,100}$')
}

function Test-CommercialSmokeConnectionStringLiteral {
    param([string]$ConnectionString)
    # Start-Process -ArgumentList joins array elements with spaces and
    # does not quote them on Windows. Reject the string instead of
    # trying to quote it.
    if ([string]::IsNullOrEmpty($ConnectionString)) { return $false }
    foreach ($ch in $ConnectionString.ToCharArray()) {
        if ([char]::IsWhiteSpace($ch)) { return $false }
        if ($ch -eq '"' -or $ch -eq [char]39) { return $false }
    }
    return $true
}

function Test-CommercialSmokeLocalNamedInstance {
    param([string]$Value)
    # Machine is case-insensitive and must be exactly one of the local
    # tokens. A named instance is required. No port, no second slash,
    # and no default instance (bare "." / localhost / (local) / 127.0.0.1).
    if ([string]::IsNullOrWhiteSpace($Value)) { return $false }
    return [regex]::IsMatch($Value, '^(?i:\.|\(local\)|localhost|127\.0\.0\.1)\\[A-Za-z0-9_]+$')
}

function Resolve-CommercialSmokeMssqlConnection {
    param([string]$ConnectionString)
    $refused = 'MSSQL_E2E REFUSED (connection string rejected before connect; '
    if (-not (Test-CommercialSmokeConnectionStringLiteral -ConnectionString $ConnectionString)) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'whitespace or quote characters are not allowed)'
        }
    }
    $segments = Split-CommercialSmokeConnectionSegments -ConnectionString $ConnectionString
    if ($null -eq $segments) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'unbalanced quote or brace)'
        }
    }
    $databases = New-Object System.Collections.Generic.List[string]
    $hosts = New-Object System.Collections.Generic.List[string]
    $alternates = New-Object System.Collections.Generic.List[string]
    foreach ($segment in $segments) {
        if ([string]::IsNullOrWhiteSpace($segment)) { continue }
        $eq = $segment.IndexOf('=')
        if ($eq -lt 1) {
            return [pscustomobject]@{
                Ok = $false
                Server = ''
                Database = ''
                Message = $refused + 'malformed segment)'
            }
        }
        $key = $segment.Substring(0, $eq).Trim().ToLowerInvariant()
        $value = $segment.Substring($eq + 1).Trim()
        if ($key -eq 'database') {
            $databases.Add($value)
        } elseif ($key -eq 'host') {
            $hosts.Add($value)
        } elseif ($key -eq 'server' -or $key -eq 'data source' -or $key -eq 'addr' -or $key -eq 'address' -or $key -eq 'network address') {
            $alternates.Add($key)
        }
    }
    if ($alternates.Count -gt 0) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'alternate server key ' + $alternates[0] + '= is not allowed)'
        }
    }
    if ($hosts.Count -eq 0) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'no host= value)'
        }
    }
    if ($hosts.Count -ne 1) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'multiple host= values)'
        }
    }
    $server = [string]$hosts[0]
    if (-not (Test-CommercialSmokeLocalNamedInstance -Value $server)) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'host= must be a local named instance (.\Instance, (local)\Instance, localhost\Instance, or 127.0.0.1\Instance))'
        }
    }
    if ($databases.Count -eq 0) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'no database= value)'
        }
    }
    if ($databases.Count -ne 1) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'multiple database= values)'
        }
    }
    $database = [string]$databases[0]
    if (-not (Test-CommercialSmokeDatabaseName -Name $database)) {
        return [pscustomobject]@{
            Ok = $false
            Server = ''
            Database = ''
            Message = $refused + 'database= value must match ^MxE2E_[A-Za-z0-9_]{1,100}$)'
        }
    }
    return [pscustomobject]@{
        Ok = $true
        Server = $server
        Database = $database
        Message = ''
    }
}

function Get-CommercialSmokeSqlCmdPath {
    foreach ($name in @('sqlcmd', 'SQLCMD', 'sqlcmd.exe', 'SQLCMD.EXE')) {
        # Get-Command returns every PATH match. The host runs the first.
        $command = @(Get-Command -Name $name -CommandType Application -ErrorAction SilentlyContinue)
        if ($command.Count -gt 0) { return [string]$command[0].Source }
    }
    return ''
}

function Get-CommercialSmokeSqlCmdLines {
    param($Output)
    $names = New-Object System.Collections.Generic.List[string]
    if ($null -eq $Output) { return $names.ToArray() }
    foreach ($item in @($Output)) {
        if ($null -eq $item) { continue }
        $raw = [string]$item
        $normalized = $raw.Replace("`r`n", "`n").Replace("`r", "`n")
        foreach ($line in $normalized.Split([char]10)) {
            $trim = $line.Trim()
            if ($trim.Length -eq 0) { continue }
            if ($trim -match '^\([0-9]+ rows affected\)$') { continue }
            $names.Add($trim)
        }
    }
    return $names.ToArray()
}

function Format-CommercialSmokeNativeArgumentLine {
    param([string[]]$ArgumentList)
    # Windows PowerShell 5.1 has ProcessStartInfo.Arguments (one string)
    # and no ArgumentList. Quote only arguments that contain whitespace.
    # A double quote has no safe encoding here; callers must not pass one.
    $parts = New-Object System.Collections.Generic.List[string]
    foreach ($argument in @($ArgumentList)) {
        $text = [string]$argument
        if ($text.IndexOf('"') -ge 0) {
            throw 'native argument contains a double quote'
        }
        $quote = $false
        if ($text.Length -eq 0) { $quote = $true }
        if ($text.IndexOf(' ') -ge 0 -or $text.IndexOf("`t") -ge 0) { $quote = $true }
        if ($quote) {
            $parts.Add('"' + $text + '"')
        } else {
            $parts.Add($text)
        }
    }
    if ($parts.Count -eq 0) { return '' }
    return ($parts.ToArray() -join ' ')
}

function Invoke-CommercialSmokeNative {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList,
        [switch]$Capture
    )
    # Explicit argument array. No array splat and no hashtable splat:
    # each element is one argv entry, including values that contain spaces.
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    if ($Capture) {
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
    }
    $nativeArguments = @($ArgumentList)
    $useList = $false
    try {
        if ($null -ne $startInfo.ArgumentList) { $useList = $true }
    } catch {
        $useList = $false
    }
    if ($useList) {
        foreach ($argument in $nativeArguments) {
            [void]$startInfo.ArgumentList.Add([string]$argument)
        }
    } else {
        $startInfo.Arguments = Format-CommercialSmokeNativeArgumentLine -ArgumentList $nativeArguments
    }
    $native = New-Object System.Diagnostics.Process
    $native.StartInfo = $startInfo
    if (-not $native.Start()) { throw ('failed to start ' + $FilePath) }
    $stdoutText = ''
    $stderrText = ''
    if ($Capture) {
        $stdoutTask = $native.StandardOutput.ReadToEndAsync()
        $stderrTask = $native.StandardError.ReadToEndAsync()
        $null = $native.WaitForExit()
        $stdoutText = [string]$stdoutTask.GetAwaiter().GetResult()
        $stderrText = [string]$stderrTask.GetAwaiter().GetResult()
    } else {
        $null = $native.WaitForExit()
    }
    $nativeCode = 1
    try { $nativeCode = [int]$native.ExitCode } catch { $nativeCode = 1 }
    $native.Dispose()
    return [pscustomobject]@{
        ExitCode = $nativeCode
        Stdout = $stdoutText
        Stderr = $stderrText
    }
}

function Invoke-CommercialSmokeSqlCmd {
    param(
        [Parameter(Mandatory = $true)][string]$SqlCmd,
        [Parameter(Mandatory = $true)][string]$Server,
        [Parameter(Mandatory = $true)][string]$Query
    )
    # Every sqlcmd invocation is -E -S <host from host=>. Never omit -S:
    # a missing server argument would target the default instance.
    if (-not (Test-CommercialSmokeLocalNamedInstance -Value $Server)) {
        return [pscustomobject]@{
            ExitCode = 1
            Names = @()
        }
    }
    $sqlArgs = @('-E', '-S', $Server, '-d', 'master', '-b', '-h-1', '-W', '-w65535', '-Q', $Query)
    $code = 1
    $output = $null
    try {
        $native = Invoke-CommercialSmokeNative -FilePath $SqlCmd -ArgumentList $sqlArgs -Capture
        $code = [int]$native.ExitCode
        $output = @([string]$native.Stdout, [string]$native.Stderr)
    } catch {
        return [pscustomobject]@{
            ExitCode = 1
            Names = @()
        }
    }
    return [pscustomobject]@{
        ExitCode = $code
        Names = @(Get-CommercialSmokeSqlCmdLines -Output $output)
    }
}

function Write-CommercialSmokeDatabaseList {
    param(
        [Parameter(Mandatory = $true)][string]$Server,
        [Parameter(Mandatory = $true)][string]$When,
        [string[]]$Names
    )
    $shown = New-Object System.Collections.Generic.List[string]
    foreach ($name in @($Names)) {
        if ([string]::IsNullOrWhiteSpace([string]$name)) { continue }
        $shown.Add(([string]$name).Trim())
    }
    $joined = ''
    if ($shown.Count -gt 0) { $joined = ($shown.ToArray() -join ' ') }
    Write-Host ('[' + $Server + '] databases ' + $When + ': ' + $joined)
}

function Test-CommercialSmokeDatabaseListed {
    param(
        [string]$Name,
        [string[]]$Names
    )
    foreach ($one in @($Names)) {
        if ([string]::Equals([string]$one, $Name, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }
    return $false
}

function Resolve-CommercialSmokeE2EExe {
    param([string]$BuildDir)
    # Same order as resolve_server_exe: single-config (NMake/Ninja) first,
    # then the multi-config Debug directory. This script has no
    # -Configuration parameter, so the multi-config directory stays Debug,
    # which is also the ctest -C Debug config. Do not change that ctest flag.
    $single = Join-Path $BuildDir 'tools\MoxianClientE2E\mxh_client_e2e.exe'
    $multi = Join-Path $BuildDir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
    $candidates = @($single, $multi)
    foreach ($path in $candidates) {
        if (Test-Path -LiteralPath $path) {
            return [pscustomobject]@{
                Found = $true
                Path = $path
                Candidates = $candidates
            }
        }
    }
    return [pscustomobject]@{
        Found = $false
        Path = ''
        Candidates = $candidates
    }
}

function Test-CommercialSmokeWindowsHost {
    # $IsWindows exists in PowerShell 6+. Windows PowerShell 5.1 is Desktop.
    $variable = Get-Variable -Name 'IsWindows' -ErrorAction SilentlyContinue
    if ($null -ne $variable) { return [bool]$variable.Value }
    return $PSVersionTable.PSEdition -eq 'Desktop'
}

function Get-CommercialSmokeKernelStart {
    param([int]$ProcessId)
    # Linux stat field 22 is starttime in clock ticks since boot. It stays
    # stable for the life of that process and changes if the PID is reused.
    if ($ProcessId -le 0) { return [int64]0 }
    if (Test-CommercialSmokeWindowsHost) { return [int64]0 }
    $statPath = '/proc/' + $ProcessId + '/stat'
    if (-not (Test-Path -LiteralPath $statPath)) { return [int64]0 }
    $stat = ''
    try { $stat = [IO.File]::ReadAllText($statPath) } catch { return [int64]0 }
    $close = $stat.LastIndexOf(')')
    if ($close -lt 0 -or ($close + 2) -ge $stat.Length) { return [int64]0 }
    $fields = @($stat.Substring($close + 2).Trim() -split '\s+')
    if ($fields.Count -lt 20) { return [int64]0 }
    $start = [int64]0
    if (-not [int64]::TryParse([string]$fields[19], [ref]$start)) { return [int64]0 }
    if ($start -le 0) { return [int64]0 }
    return $start
}

function Get-CommercialSmokeOwnedIdentity {
    param($Process)
    if ($null -eq $Process) { return $null }
    $id = 0
    try { $id = [int]$Process.Id } catch { return $null }
    if ($id -le 0) { return $null }
    try { $Process.Refresh() } catch { }
    $alive = $true
    if (Test-CommercialSmokeWindowsHost) {
        try { $alive = -not [bool]$Process.HasExited } catch { $alive = $true }
    } elseif (-not (Test-Path -LiteralPath ('/proc/' + $id))) {
        $alive = $false
    }
    if (-not $alive) { return $null }
    $ticks = [int64]0
    try {
        $started = $Process.StartTime
        if ($null -ne $started) { $ticks = [int64]$started.ToUniversalTime().Ticks }
    } catch { $ticks = [int64]0 }
    $kernel = Get-CommercialSmokeKernelStart -ProcessId $id
    if ($ticks -le 0 -and $kernel -le 0) { return $null }
    return [pscustomobject]@{
        Id = $id
        StartTicks = $ticks
        KernelStart = $kernel
    }
}

function Add-CommercialSmokeOwnedProcess {
    param(
        $Owned,
        $Seen,
        $Process
    )
    if ($null -eq $Owned -or $null -eq $Seen) { return }
    $identity = Get-CommercialSmokeOwnedIdentity -Process $Process
    if ($null -eq $identity) { return }
    $key = [string]$identity.Id
    if ($Seen.ContainsKey($key)) { return }
    $Seen[$key] = $true
    $null = $Owned.Add($identity)
}

function Add-CommercialSmokeProcStatChildren {
    param(
        [int]$ParentId,
        $Ids
    )
    # /proc/<pid>/status PPid does not need ptrace. The children file can
    # exist and still be empty when ptrace is restricted.
    $procRoot = '/proc'
    if (-not (Test-Path -LiteralPath $procRoot)) { return }
    foreach ($entry in @([IO.Directory]::EnumerateDirectories($procRoot))) {
        $childId = 0
        $name = [IO.Path]::GetFileName($entry)
        if (-not [int]::TryParse($name, [ref]$childId)) { continue }
        if ($childId -le 0 -or $childId -eq $ParentId) { continue }
        $statusPath = $entry.TrimEnd([char]47) + '/status'
        $matched = $false
        try {
            foreach ($line in [IO.File]::ReadLines($statusPath)) {
                if (-not $line.StartsWith('PPid:')) { continue }
                $ppid = 0
                $value = $line.Substring(5).Trim()
                if ([int]::TryParse($value, [ref]$ppid) -and $ppid -eq $ParentId) {
                    $matched = $true
                }
                break
            }
        } catch { continue }
        if ($matched) { $null = $Ids.Add($childId) }
    }
}

function Get-CommercialSmokeChildProcessIds {
    param([int]$ParentId)
    $ids = New-Object System.Collections.Generic.List[int]
    if ($ParentId -le 0) { return @() }
    try {
        if (Test-CommercialSmokeWindowsHost) {
            $filter = 'ParentProcessId = {0}' -f $ParentId
            foreach ($row in @(Get-CimInstance -ClassName Win32_Process -Filter $filter -ErrorAction SilentlyContinue)) {
                if ($null -eq $row) { continue }
                $childId = 0
                try { $childId = [int]$row.ProcessId } catch { $childId = 0 }
                if ($childId -gt 0) { $null = $ids.Add($childId) }
            }
        } else {
            $childFile = '/proc/' + $ParentId + '/task/' + $ParentId + '/children'
            if (Test-Path -LiteralPath $childFile) {
                $text = ''
                try { $text = [IO.File]::ReadAllText($childFile) } catch { $text = '' }
                foreach ($part in @($text -split '\s+')) {
                    if ([string]::IsNullOrWhiteSpace([string]$part)) { continue }
                    $childId = 0
                    if ([int]::TryParse([string]$part, [ref]$childId) -and $childId -gt 0) {
                        $null = $ids.Add($childId)
                    }
                }
            }
            # Union with PPid. An empty or partial children file must not
            # hide a server this run actually started.
            Add-CommercialSmokeProcStatChildren -ParentId $ParentId -Ids $ids
        }
    } catch { }
    return @($ids.ToArray())
}

function Add-CommercialSmokeOwnedDescendants {
    param(
        $Owned,
        $Seen,
        [int]$RootId
    )
    if ($RootId -le 0 -or $null -eq $Owned -or $null -eq $Seen) { return }
    $pending = New-Object System.Collections.Generic.Queue[int]
    $pending.Enqueue($RootId)
    $guard = 0
    while ($pending.Count -gt 0 -and $guard -lt 1000) {
        $guard++
        $parentId = [int]$pending.Dequeue()
        foreach ($childId in @(Get-CommercialSmokeChildProcessIds -ParentId $parentId)) {
            if ($childId -le 0) { continue }
            $key = [string]$childId
            if ($Seen.ContainsKey($key)) { continue }
            $child = $null
            try { $child = Get-Process -Id $childId -ErrorAction SilentlyContinue } catch { $child = $null }
            if ($null -eq $child) { continue }
            Add-CommercialSmokeOwnedProcess -Owned $Owned -Seen $Seen -Process $child
            if ($Seen.ContainsKey($key)) { $pending.Enqueue([int]$childId) }
        }
    }
}

function Test-CommercialSmokeLiveIdentity {
    param(
        [int]$ProcessId,
        [int64]$StartTicks,
        [int64]$KernelStart = 0
    )
    # Platform clock used for a fallback stop: UTC StartTime ticks on
    # Windows, boot-relative starttime on Linux. A reused PID does not match.
    if ($ProcessId -le 0) { return $false }
    if (Test-CommercialSmokeWindowsHost) {
        if ($StartTicks -le 0) { return $false }
        $live = $null
        try { $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue } catch { return $false }
        if ($null -eq $live) { return $false }
        try { $live.Refresh() } catch { }
        $exited = $false
        try { $exited = [bool]$live.HasExited } catch { return $false }
        if ($exited) { return $false }
        $liveTicks = [int64]0
        try { $liveTicks = [int64]$live.StartTime.ToUniversalTime().Ticks } catch { return $false }
        if ($liveTicks -le 0) { return $false }
        return ($liveTicks -eq $StartTicks)
    }
    if ($KernelStart -le 0) { return $false }
    $liveKernel = Get-CommercialSmokeKernelStart -ProcessId $ProcessId
    if ($liveKernel -le 0) { return $false }
    return ($liveKernel -eq $KernelStart)
}

function Stop-CommercialSmokeOwnedProcessFallback {
    param(
        [int]$ProcessId,
        [int64]$StartTicks,
        [int64]$KernelStart = 0
    )
    # Kill() can exit the original process before these PID-based signals.
    # Re-read identity immediately before each one. Never signal a PID
    # whose StartTime no longer matches the process this run recorded.
    if (Test-CommercialSmokeLiveIdentity -ProcessId $ProcessId -StartTicks $StartTicks -KernelStart $KernelStart) {
        try { Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue } catch { }
    }
    if (Test-CommercialSmokeWindowsHost) { return }
    $statusPath = '/proc/' + $ProcessId + '/status'
    $needsKill = $false
    if (Test-Path -LiteralPath $statusPath) {
        $state = ''
        try {
            foreach ($line in [IO.File]::ReadLines($statusPath)) {
                if ($line.StartsWith('State:')) { $state = $line.Substring(6).Trim(); break }
            }
        } catch { $state = '' }
        if ($state.Length -gt 0 -and -not $state.StartsWith('Z') -and -not $state.StartsWith('X')) {
            $needsKill = $true
        }
    }
    if ($needsKill -and (Test-CommercialSmokeLiveIdentity -ProcessId $ProcessId -StartTicks $StartTicks -KernelStart $KernelStart)) {
        try { & /bin/kill '-9' "$ProcessId" 2>$null } catch { }
    }
}

function Stop-CommercialSmokeOwnedProcess {
    param(
        [int]$ProcessId,
        [int64]$StartTicks,
        [int64]$KernelStart = 0
    )
    # PID reuse: a dead PID can be recycled. Stop only when StartTime or
    # the Linux boot-relative starttime still matches. Never select by name.
    if ($ProcessId -le 0) { return }
    if ($StartTicks -le 0 -and $KernelStart -le 0) { return }
    try {
        $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if ($null -eq $live) { return }
        try { $live.Refresh() } catch { }
        $alive = $true
        if (Test-CommercialSmokeWindowsHost) {
            try { $alive = -not [bool]$live.HasExited } catch { $alive = $true }
        } elseif (-not (Test-Path -LiteralPath ('/proc/' + $ProcessId))) {
            $alive = $false
        }
        if (-not $alive) { return }
        $liveTicks = [int64]0
        try { $liveTicks = [int64]$live.StartTime.ToUniversalTime().Ticks } catch { $liveTicks = [int64]0 }
        $liveKernel = Get-CommercialSmokeKernelStart -ProcessId $ProcessId
        $dateMatch = ($StartTicks -gt 0 -and $liveTicks -eq $StartTicks)
        $kernelMatch = ($KernelStart -gt 0 -and $liveKernel -eq $KernelStart)
        if (-not $dateMatch -and -not $kernelMatch) { return }
        try { $live.Kill() } catch { }
        Stop-CommercialSmokeOwnedProcessFallback -ProcessId $ProcessId -StartTicks $StartTicks -KernelStart $KernelStart
        Wait-CommercialSmokeOwnedExit -ProcessId $ProcessId
    } catch {
        # Already exited, or the PID was reused between the checks above.
    }
}

function Wait-CommercialSmokeOwnedExit {
    param([int]$ProcessId)
    for ($i = 0; $i -lt 40; $i++) {
        if (Test-CommercialSmokeWindowsHost) {
            $live = $null
            try { $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue } catch { $live = $null }
            if ($null -eq $live) { return }
            try { if ([bool]$live.HasExited) { return } } catch { return }
        } else {
            $statusPath = '/proc/' + $ProcessId + '/status'
            if (-not (Test-Path -LiteralPath $statusPath)) { return }
            $state = ''
            try {
                foreach ($line in [IO.File]::ReadLines($statusPath)) {
                    if ($line.StartsWith('State:')) { $state = $line.Substring(6).Trim(); break }
                }
            } catch { return }
            if ($state.StartsWith('Z') -or $state.StartsWith('X') -or $state.Length -eq 0) { return }
        }
        Start-Sleep -Milliseconds 50
    }
}

function Stop-CommercialSmokeOwnedProcesses {
    param($Owned)
    if ($null -eq $Owned) { return }
    $count = 0
    try { $count = [int]$Owned.Count } catch { return }
    for ($index = $count - 1; $index -ge 0; $index--) {
        $entry = $Owned[$index]
        if ($null -eq $entry) { continue }
        $id = 0
        $ticks = [int64]0
        $kernel = [int64]0
        try { $id = [int]$entry.Id } catch { continue }
        try { $ticks = [int64]$entry.StartTicks } catch { $ticks = [int64]0 }
        try { $kernel = [int64]$entry.KernelStart } catch { $kernel = [int64]0 }
        Stop-CommercialSmokeOwnedProcess -ProcessId $id -StartTicks $ticks -KernelStart $kernel
    }
}

function Get-CommercialSmokeOwnedReportToken {
    param(
        $Entry,
        [bool]$WindowsHost = $(Test-CommercialSmokeWindowsHost)
    )
    # Windows identity is UTC StartTime ticks. KernelStart is always 0
    # there. Linux identity is the boot-relative starttime.
    if ($null -eq $Entry) { return '0' }
    if ($WindowsHost) {
        $ticks = [int64]0
        try { $ticks = [int64]$Entry.StartTicks } catch { $ticks = [int64]0 }
        return [string]$ticks
    }
    $kernel = [int64]0
    try { $kernel = [int64]$Entry.KernelStart } catch { $kernel = [int64]0 }
    return [string]$kernel
}

function Format-CommercialSmokeOwnedCommandLine {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList
    )
    # lpApplicationName is the path. The command line still begins with
    # a quoted argv0 because CreateProcess parses it that way.
    if ($FilePath.IndexOf('"') -ge 0) {
        throw 'owned executable path contains a double quote'
    }
    $argLine = ''
    if ($null -ne $ArgumentList) {
        $argLine = Format-CommercialSmokeNativeArgumentLine -ArgumentList @($ArgumentList)
    }
    $command = '"' + $FilePath + '"'
    if (-not [string]::IsNullOrEmpty($argLine)) {
        $command = $command + ' ' + $argLine
    }
    if ($command.Length -ge 32767) {
        throw 'owned executable command line is too long'
    }
    return $command
}

function Test-CommercialSmokePidAlive {
    param([int]$ProcessId)
    if ($ProcessId -le 0) { return $false }
    $live = $null
    try { $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue } catch { return $false }
    if ($null -eq $live) { return $false }
    try { $live.Refresh() } catch { }
    try { return -not [bool]$live.HasExited } catch { return $false }
}

function Wait-CommercialSmokeJobMembers {
    param([int[]]$ProcessIds)
    # Bounded confirm after the job handle is closed. Returns the PIDs
    # that are still alive. Does not signal them.
    $pending = New-Object System.Collections.Generic.List[int]
    foreach ($raw in @($ProcessIds)) {
        $id = 0
        try { $id = [int]$raw } catch { $id = 0 }
        if ($id -gt 0) { $pending.Add($id) }
    }
    $alive = New-Object System.Collections.Generic.List[int]
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($true) {
        $alive.Clear()
        foreach ($id in $pending) {
            if (Test-CommercialSmokePidAlive -ProcessId $id) { $alive.Add($id) }
        }
        if ($alive.Count -eq 0) { return ,@($alive.ToArray()) }
        if ($sw.ElapsedMilliseconds -ge 5000) { return ,@($alive.ToArray()) }
        $pending = $alive
        $alive = New-Object System.Collections.Generic.List[int]
        Start-Sleep -Milliseconds 50
    }
}

function Write-CommercialSmokeCapturedOutput {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    if (-not (Test-Path -LiteralPath $Path)) { return }
    $text = ''
    try {
        $stream = New-Object System.IO.FileStream(
            $Path,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::Read,
            [System.IO.FileShare]::ReadWrite)
        $reader = New-Object System.IO.StreamReader($stream)
        $text = $reader.ReadToEnd()
        $reader.Dispose()
    } catch {
        return
    }
    if ([string]::IsNullOrEmpty($text)) { return }
    $text = $text.Replace("`r`n", "`n").Replace("`r", "`n")
    $lines = @($text -split "`n", -1)
    $last = $lines.Count - 1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($i -eq $last -and $lines[$i].Length -eq 0) { continue }
        Write-Host $lines[$i]
    }
}

function Initialize-CommercialSmokeProcessJob {
    if ($null -ne ('Mxh.CommercialSmokeLaunch' -as [type])) { return }
    $code = @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace Mxh {
    public sealed class CommercialSmokeLaunch : IDisposable {
        const uint CREATE_SUSPENDED = 0x00000004;
        const uint EXTENDED_STARTUPINFO_PRESENT = 0x00080000;
        const uint JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000;
        const int STARTF_USESTDHANDLES = 0x00000100;
        const int PROC_THREAD_ATTRIBUTE_HANDLE_LIST = 0x00020002;
        const int JobObjectExtendedLimitInformation = 9;
        const int JobObjectBasicProcessIdList = 3;
        const uint GENERIC_READ = 0x80000000;
        const uint GENERIC_WRITE = 0x40000000;
        const uint OPEN_EXISTING = 3;
        const uint CREATE_ALWAYS = 2;

        IntPtr processHandle;
        IntPtr threadHandle;
        IntPtr jobHandle;
        public int ProcessId;

        [StructLayout(LayoutKind.Sequential)]
        struct SECURITY_ATTRIBUTES {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            [MarshalAs(UnmanagedType.Bool)]
            public bool bInheritHandle;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct STARTUPINFO {
            public int cb;
            public IntPtr lpReserved;
            public IntPtr lpDesktop;
            public IntPtr lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public int dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct STARTUPINFOEX {
            public STARTUPINFO StartupInfo;
            public IntPtr lpAttributeList;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct PROCESS_INFORMATION {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct IO_COUNTERS {
            public ulong ReadOperationCount;
            public ulong WriteOperationCount;
            public ulong OtherOperationCount;
            public ulong ReadTransferCount;
            public ulong WriteTransferCount;
            public ulong OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct JOBOBJECT_BASIC_LIMIT_INFORMATION {
            public long PerProcessUserTimeLimit;
            public long PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize;
            public UIntPtr MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass;
            public uint SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
            public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
            public IO_COUNTERS IoInfo;
            public UIntPtr ProcessMemoryLimit;
            public UIntPtr JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed;
            public UIntPtr PeakJobMemoryUsed;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool CreateProcessW(
            string lpApplicationName,
            StringBuilder lpCommandLine,
            IntPtr lpProcessAttributes,
            IntPtr lpThreadAttributes,
            bool bInheritHandles,
            uint dwCreationFlags,
            IntPtr lpEnvironment,
            string lpCurrentDirectory,
            ref STARTUPINFOEX lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr CreateJobObjectW(IntPtr lpJobAttributes, string lpName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool SetInformationJobObject(IntPtr hJob, int jobObjectInfoClass, IntPtr lpJobObjectInfo, uint cbJobObjectInfoLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryInformationJobObject(IntPtr hJob, int jobObjectInfoClass, IntPtr lpJobObjectInfo, uint cbJobObjectInfoLength, IntPtr lpReturnLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool AssignProcessToJobObject(IntPtr hJob, IntPtr hProcess);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint ResumeThread(IntPtr hThread);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GetExitCodeProcess(IntPtr hProcess, out uint lpExitCode);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr CreateFileW(string lpFileName, uint dwDesiredAccess, uint dwShareMode, IntPtr lpSecurityAttributes, uint dwCreationDisposition, uint dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool SetHandleInformation(IntPtr hObject, uint dwMask, uint dwFlags);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool InitializeProcThreadAttributeList(IntPtr lpAttributeList, int dwAttributeCount, int dwFlags, ref IntPtr lpSize);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool UpdateProcThreadAttribute(IntPtr lpAttributeList, uint dwFlags, IntPtr attribute, IntPtr lpValue, IntPtr cbSize, IntPtr lpPreviousValue, IntPtr lpReturnSize);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern void DeleteProcThreadAttributeList(IntPtr lpAttributeList);

        public static string ProbeLayouts() {
            int startup = Marshal.SizeOf(typeof(STARTUPINFO));
            int extended = Marshal.SizeOf(typeof(STARTUPINFOEX));
            int basic = Marshal.SizeOf(typeof(JOBOBJECT_BASIC_LIMIT_INFORMATION));
            int io = Marshal.SizeOf(typeof(IO_COUNTERS));
            int job = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
            int sa = Marshal.SizeOf(typeof(SECURITY_ATTRIBUTES));
            string text = "startup=" + startup.ToString()
                + " extended=" + extended.ToString()
                + " job=" + job.ToString()
                + " basic=" + basic.ToString()
                + " io=" + io.ToString()
                + " sa=" + sa.ToString();
            if (IntPtr.Size != 8 || startup != 104 || extended != 112 || job != 144 || basic != 64 || io != 48 || sa != 24) {
                throw new InvalidOperationException("unexpected Win32 struct size " + text);
            }
            return text;
        }

        static Exception Fail(string api) {
            int err = Marshal.GetLastWin32Error();
            return new Win32Exception(err, api + " failed with Win32 " + err.ToString());
        }

        static void Close(ref IntPtr handle) {
            if (handle == IntPtr.Zero || handle == new IntPtr(-1)) {
                handle = IntPtr.Zero;
                return;
            }
            CloseHandle(handle);
            handle = IntPtr.Zero;
        }

        static IntPtr CreateInheritSa() {
            var sa = new SECURITY_ATTRIBUTES();
            sa.nLength = Marshal.SizeOf(typeof(SECURITY_ATTRIBUTES));
            sa.lpSecurityDescriptor = IntPtr.Zero;
            sa.bInheritHandle = true;
            IntPtr ptr = Marshal.AllocHGlobal(sa.nLength);
            Marshal.StructureToPtr(sa, ptr, false);
            return ptr;
        }

        static IntPtr OpenInherited(IntPtr saPtr, string path, uint access, uint disposition) {
            IntPtr handle = CreateFileW(path, access, 7, saPtr, disposition, 0x80, IntPtr.Zero);
            if (handle == IntPtr.Zero || handle == new IntPtr(-1)) throw Fail("CreateFile");
            if (!SetHandleInformation(handle, 1, 1)) {
                int err = Marshal.GetLastWin32Error();
                CloseHandle(handle);
                throw new Win32Exception(err, "SetHandleInformation failed with Win32 " + err.ToString());
            }
            return handle;
        }

        static IntPtr CreateAttributeList(IntPtr handleMem, int count) {
            IntPtr bytes = IntPtr.Zero;
            InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref bytes);
            if (bytes == IntPtr.Zero) throw Fail("InitializeProcThreadAttributeList");
            IntPtr list = Marshal.AllocHGlobal(bytes);
            if (!InitializeProcThreadAttributeList(list, 1, 0, ref bytes)) {
                int err = Marshal.GetLastWin32Error();
                Marshal.FreeHGlobal(list);
                throw new Win32Exception(err, "InitializeProcThreadAttributeList failed with Win32 " + err.ToString());
            }
            bool updated = UpdateProcThreadAttribute(
                list,
                0,
                new IntPtr(PROC_THREAD_ATTRIBUTE_HANDLE_LIST),
                handleMem,
                new IntPtr(IntPtr.Size * count),
                IntPtr.Zero,
                IntPtr.Zero);
            if (!updated) {
                int err = Marshal.GetLastWin32Error();
                DeleteProcThreadAttributeList(list);
                Marshal.FreeHGlobal(list);
                throw new Win32Exception(err, "UpdateProcThreadAttribute failed with Win32 " + err.ToString());
            }
            return list;
        }

        static void SetKillOnClose(IntPtr job) {
            var info = new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
            var basic = info.BasicLimitInformation;
            basic.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            info.BasicLimitInformation = basic;
            int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
            IntPtr mem = Marshal.AllocHGlobal(size);
            try {
                Marshal.StructureToPtr(info, mem, false);
                if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, mem, (uint)size)) {
                    throw Fail("SetInformationJobObject");
                }
            } finally {
                Marshal.FreeHGlobal(mem);
            }
        }

        public static CommercialSmokeLaunch Start(string application, string commandLine, string workingDirectory, string stdoutPath, string stderrPath) {
            ProbeLayouts();
            if (string.IsNullOrEmpty(application)) throw new ArgumentException("application");
            if (string.IsNullOrEmpty(commandLine)) throw new ArgumentException("commandLine");
            if (string.IsNullOrEmpty(stdoutPath) || string.IsNullOrEmpty(stderrPath)) throw new ArgumentException("output path");
            if (string.IsNullOrEmpty(workingDirectory)) workingDirectory = null;

            IntPtr saPtr = IntPtr.Zero;
            IntPtr stdin = IntPtr.Zero;
            IntPtr stdout = IntPtr.Zero;
            IntPtr stderr = IntPtr.Zero;
            IntPtr attrList = IntPtr.Zero;
            IntPtr handleMem = IntPtr.Zero;
            IntPtr process = IntPtr.Zero;
            IntPtr thread = IntPtr.Zero;
            IntPtr job = IntPtr.Zero;
            bool haveList = false;
            try {
                saPtr = CreateInheritSa();
                stdin = OpenInherited(saPtr, "NUL", GENERIC_READ, OPEN_EXISTING);
                stdout = OpenInherited(saPtr, stdoutPath, GENERIC_WRITE, CREATE_ALWAYS);
                stderr = OpenInherited(saPtr, stderrPath, GENERIC_WRITE, CREATE_ALWAYS);
                handleMem = Marshal.AllocHGlobal(IntPtr.Size * 3);
                Marshal.WriteIntPtr(handleMem, 0, stdin);
                Marshal.WriteIntPtr(handleMem, IntPtr.Size, stdout);
                Marshal.WriteIntPtr(handleMem, IntPtr.Size * 2, stderr);
                attrList = CreateAttributeList(handleMem, 3);
                haveList = true;

                var startup = new STARTUPINFOEX();
                startup.StartupInfo.cb = Marshal.SizeOf(typeof(STARTUPINFOEX));
                startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
                startup.StartupInfo.hStdInput = stdin;
                startup.StartupInfo.hStdOutput = stdout;
                startup.StartupInfo.hStdError = stderr;
                startup.lpAttributeList = attrList;

                var command = new StringBuilder(commandLine, commandLine.Length + 16);
                PROCESS_INFORMATION created;
                uint flags = CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT;
                // Assign only this suspended child. Never the hosting PowerShell process.
                bool ok = CreateProcessW(
                    application,
                    command,
                    IntPtr.Zero,
                    IntPtr.Zero,
                    true,
                    flags,
                    IntPtr.Zero,
                    workingDirectory,
                    ref startup,
                    out created);
                if (!ok) throw Fail("CreateProcess");
                process = created.hProcess;
                thread = created.hThread;

                job = CreateJobObjectW(IntPtr.Zero, null);
                if (job == IntPtr.Zero) throw Fail("CreateJobObject");
                SetKillOnClose(job);
                if (!AssignProcessToJobObject(job, process)) throw Fail("AssignProcessToJobObject");

                var launch = new CommercialSmokeLaunch();
                launch.processHandle = process;
                launch.threadHandle = thread;
                launch.jobHandle = job;
                launch.ProcessId = created.dwProcessId;
                process = IntPtr.Zero;
                thread = IntPtr.Zero;
                job = IntPtr.Zero;
                return launch;
            } catch {
                if (process != IntPtr.Zero) {
                    TerminateProcess(process, 1);
                    CloseHandle(process);
                    process = IntPtr.Zero;
                }
                if (thread != IntPtr.Zero) {
                    CloseHandle(thread);
                    thread = IntPtr.Zero;
                }
                if (job != IntPtr.Zero) {
                    CloseHandle(job);
                    job = IntPtr.Zero;
                }
                throw;
            } finally {
                if (haveList && attrList != IntPtr.Zero) {
                    DeleteProcThreadAttributeList(attrList);
                }
                if (attrList != IntPtr.Zero) {
                    Marshal.FreeHGlobal(attrList);
                    attrList = IntPtr.Zero;
                }
                if (handleMem != IntPtr.Zero) {
                    Marshal.FreeHGlobal(handleMem);
                    handleMem = IntPtr.Zero;
                }
                Close(ref stdin);
                Close(ref stdout);
                Close(ref stderr);
                if (saPtr != IntPtr.Zero) {
                    Marshal.FreeHGlobal(saPtr);
                    saPtr = IntPtr.Zero;
                }
            }
        }

        public void Resume() {
            if (threadHandle == IntPtr.Zero) throw new InvalidOperationException("thread handle is closed");
            uint rc = ResumeThread(threadHandle);
            if (rc == uint.MaxValue) throw Fail("ResumeThread");
        }

        public bool Wait(int milliseconds) {
            if (processHandle == IntPtr.Zero) throw new InvalidOperationException("process handle is closed");
            uint rc = WaitForSingleObject(processHandle, (uint)milliseconds);
            if (rc == 0u) return true;
            if (rc == 258u) return false;
            throw Fail("WaitForSingleObject");
        }

        public int ReadExitCode() {
            uint code;
            if (!GetExitCodeProcess(processHandle, out code)) throw Fail("GetExitCodeProcess");
            return (int)code;
        }

        public int[] JobProcessIds() {
            if (jobHandle == IntPtr.Zero) return new int[0];
            int slots = 16;
            while (slots <= 4096) {
                int bytes = 8 + (8 * slots);
                IntPtr buf = Marshal.AllocHGlobal(bytes);
                try {
                    for (int i = 0; i < bytes; i++) Marshal.WriteByte(buf, i, 0);
                    bool ok = QueryInformationJobObject(jobHandle, JobObjectBasicProcessIdList, buf, (uint)bytes, IntPtr.Zero);
                    int assigned = Marshal.ReadInt32(buf, 0);
                    int listed = Marshal.ReadInt32(buf, 4);
                    if (!ok) {
                        int err = Marshal.GetLastWin32Error();
                        if (err == 234) {
                            int next = assigned + 8;
                            if (next <= slots) next = slots * 2;
                            slots = next;
                            continue;
                        }
                        throw new Win32Exception(err, "QueryInformationJobObject failed with Win32 " + err.ToString());
                    }
                    if (listed < 0) listed = 0;
                    if (listed > slots) listed = slots;
                    int[] ids = new int[listed];
                    for (int i = 0; i < listed; i++) {
                        long pid = Marshal.ReadInt64(buf, 8 + (i * 8));
                        ids[i] = (int)pid;
                    }
                    return ids;
                } finally {
                    Marshal.FreeHGlobal(buf);
                }
            }
            return new int[0];
        }

        public void CloseJob() {
            if (jobHandle == IntPtr.Zero) return;
            CloseHandle(jobHandle);
            jobHandle = IntPtr.Zero;
        }

        public void AbandonJob() {
            // Leak the handle on purpose when it contains this process.
            // Closing it would kill the hosting PowerShell process.
            jobHandle = IntPtr.Zero;
        }

        public void Dispose() {
            if (threadHandle != IntPtr.Zero) {
                CloseHandle(threadHandle);
                threadHandle = IntPtr.Zero;
            }
            if (processHandle != IntPtr.Zero) {
                CloseHandle(processHandle);
                processHandle = IntPtr.Zero;
            }
            if (jobHandle != IntPtr.Zero) {
                CloseHandle(jobHandle);
                jobHandle = IntPtr.Zero;
            }
        }
    }
}
'@
    Add-Type -TypeDefinition $code -Language CSharp | Out-Null
}

function Invoke-CommercialSmokeOwnedExecutableOnWindows {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutSeconds = 600
    )
    # Windows only. The PowerShell process stays outside the job.
    # stdout and stderr are files so a descendant cannot hold our pipe.
    Initialize-CommercialSmokeProcessJob
    $owned = New-Object System.Collections.Generic.List[object]
    $ownedSeen = @{}
    $exitCode = 1
    $launch = $null
    $captureDir = $null
    $timedOut = $false
    try {
        $captureDir = Join-Path ([IO.Path]::GetTempPath()) ('mxh-owned-' + [guid]::NewGuid().ToString('n'))
        New-Item -ItemType Directory -Force -Path $captureDir | Out-Null
        $stdoutPath = Join-Path $captureDir 'stdout.txt'
        $stderrPath = Join-Path $captureDir 'stderr.txt'
        $command = Format-CommercialSmokeOwnedCommandLine -FilePath $FilePath -ArgumentList $ArgumentList
        $workDir = [IO.Path]::GetDirectoryName($FilePath)
        $launch = [Mxh.CommercialSmokeLaunch]::Start($FilePath, $command, $workDir, $stdoutPath, $stderrPath)
        $launch.Resume()
        $rootId = 0
        try { $rootId = [int]$launch.ProcessId } catch { $rootId = 0 }
        $rootProc = $null
        if ($rootId -gt 0) {
            try { $rootProc = Get-Process -Id $rootId -ErrorAction SilentlyContinue } catch { $rootProc = $null }
        }
        if ($null -ne $rootProc) {
            try { $null = $rootProc.Handle } catch { }
            Add-CommercialSmokeOwnedProcess -Owned $owned -Seen $ownedSeen -Process $rootProc
        }
        $deadline = [Diagnostics.Stopwatch]::StartNew()
        while ($true) {
            Add-CommercialSmokeOwnedDescendants -Owned $owned -Seen $ownedSeen -RootId $rootId
            $signaled = $false
            try { $signaled = [bool]$launch.Wait(50) } catch { $signaled = $true }
            if ($signaled) { break }
            if ($deadline.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                $timedOut = $true
                break
            }
        }
        Add-CommercialSmokeOwnedDescendants -Owned $owned -Seen $ownedSeen -RootId $rootId
        if ($timedOut) {
            $exitCode = 1
        } else {
            try { $exitCode = [int]$launch.ReadExitCode() } catch { $exitCode = 1 }
        }
        Write-CommercialSmokeCapturedOutput -Path $stdoutPath
        Write-CommercialSmokeCapturedOutput -Path $stderrPath
        if ($timedOut) {
            Write-Host ('COMMERCIAL_SMOKE TIMEOUT (owned executable still running after ' + $TimeoutSeconds + 's)')
        }
    } finally {
        $queryFailed = $false
        $containsSelf = $false
        $jobIds = $null
        if ($null -ne $launch) {
            try { $jobIds = $launch.JobProcessIds() } catch { $queryFailed = $true }
            foreach ($rawId in @($jobIds)) {
                $jobPid = 0
                try { $jobPid = [int]$rawId } catch { $jobPid = 0 }
                if ($jobPid -gt 0 -and $jobPid -eq $PID) { $containsSelf = $true }
            }
            if ($containsSelf) {
                try { $launch.AbandonJob() } catch { }
                $exitCode = 1
                Write-Host 'COMMERCIAL_SMOKE JOB (refusing to close a job that contains this process)'
            } else {
                try { $launch.CloseJob() } catch { }
                if ($queryFailed) {
                    $exitCode = 1
                    Write-Host 'COMMERCIAL_SMOKE JOB (could not list processes before job close)'
                    Start-Sleep -Milliseconds 500
                } else {
                    $leftovers = $null
                    try { $leftovers = Wait-CommercialSmokeJobMembers -ProcessIds $jobIds } catch { $leftovers = $null }
                    if ($null -ne $leftovers -and $leftovers.Count -gt 0) {
                        $exitCode = 1
                        foreach ($left in $leftovers) {
                            Write-Host ('COMMERCIAL_SMOKE JOB (pid ' + $left + ' still running after job close)')
                        }
                    }
                }
            }
            try { $launch.Dispose() } catch { }
        }
        try { Stop-CommercialSmokeOwnedProcesses -Owned $owned } catch { }
        if (-not [string]::IsNullOrWhiteSpace($captureDir)) {
            try { Remove-Item -LiteralPath $captureDir -Recurse -Force -ErrorAction SilentlyContinue } catch { }
        }
    }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Owned = $owned
    }
}

function Invoke-CommercialSmokeOwnedExecutable {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutSeconds = 600
    )
    if ($TimeoutSeconds -lt 1) {
        throw 'owned executable TimeoutSeconds must be at least 1'
    }
    if (Test-CommercialSmokeWindowsHost) {
        return Invoke-CommercialSmokeOwnedExecutableOnWindows -FilePath $FilePath -ArgumentList $ArgumentList -TimeoutSeconds $TimeoutSeconds
    }
    # Record the process this run starts and the descendants it launches.
    # finally stops only those PIDs, and only when StartTime still matches.
    # Linux does not apply TimeoutSeconds; it waits until the child exits.
    $owned = New-Object System.Collections.Generic.List[object]
    $ownedSeen = @{}
    $exitCode = 1
    try {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -PassThru -NoNewWindow
        try { $null = $proc.Handle } catch { }
        Add-CommercialSmokeOwnedProcess -Owned $owned -Seen $ownedSeen -Process $proc
        $rootId = 0
        try { $rootId = [int]$proc.Id } catch { $rootId = 0 }
        # WaitForExit(ms) is the liveness check. HasExited can report a
        # script as exited before its children have been recorded. If the
        # wait says the process is gone but /proc/<pid> is still there,
        # keep scanning so a false exit does not skip its descendants.
        $falseExits = 0
        while ($true) {
            Add-CommercialSmokeOwnedDescendants -Owned $owned -Seen $ownedSeen -RootId $rootId
            $exited = $true
            try { $exited = [bool]$proc.WaitForExit(50) } catch { $exited = $true }
            $stillThere = $false
            if (-not (Test-CommercialSmokeWindowsHost) -and $rootId -gt 0) {
                $stillThere = Test-Path -LiteralPath ('/proc/' + $rootId)
            }
            if ($exited -and $stillThere -and $falseExits -lt 200) {
                $falseExits = $falseExits + 1
                Start-Sleep -Milliseconds 50
                continue
            }
            if ($exited) { break }
            $falseExits = 0
        }
        Add-CommercialSmokeOwnedDescendants -Owned $owned -Seen $ownedSeen -RootId $rootId
        try { $null = $proc.WaitForExit() } catch { }
        try { $exitCode = [int]$proc.ExitCode } catch { $exitCode = 1 }
    } finally {
        try { Stop-CommercialSmokeOwnedProcesses -Owned $owned } catch { }
    }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Owned = $owned
    }
}


function Invoke-CommercialSmokeExplicitMssqlE2E {
    param(
        [string]$E2eExe,
        [string[]]$E2eCandidates = @(),
        [string]$ConnectionString,
        [string]$Server,
        [string]$DatabaseName
    )
    if (-not (Test-CommercialSmokeConnectionStringLiteral -ConnectionString $ConnectionString)) {
        Write-Host 'MSSQL_E2E REFUSED (connection string rejected before connect; whitespace or quote characters are not allowed)' -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    $sqlcmd = Get-CommercialSmokeSqlCmdPath
    if ([string]::IsNullOrWhiteSpace($sqlcmd)) {
        Write-Host 'MSSQL_E2E REFUSED (sqlcmd not found; refusing before connect)' -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    $listQuery = 'SET NOCOUNT ON; SELECT name FROM sys.databases ORDER BY name'
    $before = Invoke-CommercialSmokeSqlCmd -SqlCmd $sqlcmd -Server $Server -Query $listQuery
    if ($before.ExitCode -ne 0) {
        Write-Host "MSSQL_E2E REFUSED (sys.databases query failed with exit $($before.ExitCode); not touching any database)" -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    Write-CommercialSmokeDatabaseList -Server $Server -When 'before' -Names $before.Names
    if (Test-CommercialSmokeDatabaseListed -Name $DatabaseName -Names $before.Names) {
        Write-Host "MSSQL_E2E REFUSED (database $DatabaseName already exists; refusing to touch it)" -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    $exeReady = -not [string]::IsNullOrWhiteSpace($E2eExe)
    if ($exeReady) { $exeReady = Test-Path -LiteralPath $E2eExe }
    if (-not $exeReady) {
        # Not-built stays a failure. Do not skip, and do not drop a database
        # this run has not created.
        Write-Host 'MSSQL_E2E FAILED (mxh_client_e2e not built; database was absent and was not dropped)' -ForegroundColor Red
        foreach ($candidate in @($E2eCandidates)) {
            if ([string]::IsNullOrWhiteSpace([string]$candidate)) { continue }
            Write-Host ('MSSQL_E2E EXE CANDIDATE ' + $candidate)
        }
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    Write-Host ('MSSQL_E2E EXE ' + $E2eExe)
    # Confirmed absent before E2E. The finally block is the only DROP,
    # and it runs for both a passing and a failing E2E process.
    $confirmedAbsent = $true
    $e2eExit = 1
    $dropFailed = $false
    # mxh_client_e2e is the process this run starts. On Windows it is
    # the only process assigned to the kill-on-close job. Descendants
    # the scan records are stopped only when StartTime still matches.
    # No name-based fallback.
    try {
        $e2eArgumentLine = '--login-host 127.0.0.1 --login-port 26001 --agent-port 27001 --map-port 28001 --backend mssql_odbc --init-schema --db'
        $e2eArguments = @($e2eArgumentLine.Split(' '))
        # Rejected above when the string contains whitespace or quotes.
        # Start-Process -ArgumentList does not quote array elements.
        $e2eArguments += $ConnectionString
        $ownedRun = Invoke-CommercialSmokeOwnedExecutable -FilePath $E2eExe -ArgumentList $e2eArguments
        try { $e2eExit = [int]$ownedRun.ExitCode } catch { $e2eExit = 1 }
        $ownedIds = New-Object System.Collections.Generic.List[string]
        $ownedCount = 0
        try { $ownedCount = [int]$ownedRun.Owned.Count } catch { $ownedCount = 0 }
        for ($ownedIndex = 0; $ownedIndex -lt $ownedCount; $ownedIndex++) {
            $entry = $ownedRun.Owned[$ownedIndex]
            if ($null -eq $entry) { continue }
            $ownedIds.Add(([string]$entry.Id) + ':' + (Get-CommercialSmokeOwnedReportToken -Entry $entry))
        }
        Write-Host ('MSSQL_E2E OWNED ' + ($ownedIds -join ','))
    } finally {
        if ($confirmedAbsent) {
            $dropQuery = 'DROP DATABASE [' + $DatabaseName + ']'
            $drop = Invoke-CommercialSmokeSqlCmd -SqlCmd $sqlcmd -Server $Server -Query $dropQuery
            $after = Invoke-CommercialSmokeSqlCmd -SqlCmd $sqlcmd -Server $Server -Query $listQuery
            if ($after.ExitCode -eq 0) {
                Write-CommercialSmokeDatabaseList -Server $Server -When 'after' -Names $after.Names
            } else {
                Write-Host ('[' + $Server + '] databases after: UNREADABLE')
            }
            $stillThere = $false
            if ($after.ExitCode -eq 0) {
                $stillThere = Test-CommercialSmokeDatabaseListed -Name $DatabaseName -Names $after.Names
            }
            if ($drop.ExitCode -ne 0) {
                $dropFailed = $true
                Write-Host "MSSQL_E2E DROP FAILED (database $DatabaseName; sqlcmd exit $($drop.ExitCode))" -ForegroundColor Red
            }
            if ($after.ExitCode -ne 0) {
                $dropFailed = $true
                Write-Host "MSSQL_E2E DROP FAILED (database $DatabaseName; could not verify sys.databases after DROP DATABASE)" -ForegroundColor Red
            } elseif ($stillThere) {
                $dropFailed = $true
                Write-Host "MSSQL_E2E DROP FAILED (database $DatabaseName still present after DROP DATABASE)" -ForegroundColor Red
            } elseif (-not $dropFailed) {
                Write-Host "MSSQL_E2E DROPPED (database $DatabaseName)" -ForegroundColor Green
            }
        }
    }
    if ($e2eExit -ne 0) {
        Write-Host "MSSQL_E2E FAILED (mxh_client_e2e exit $e2eExit)" -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $true
        }
    }
    if ($dropFailed) {
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $true
        }
    }
    Write-Host 'MSSQL_E2E PASS (explicit -MssqlDb, one command)' -ForegroundColor Green
    return [pscustomobject]@{
        ExitCode = 0
        Invoked = $true
    }
}

function Test-CommercialSmokeLocalDbAvailable {
    # LocalDB is available only when its utility resolves as an application
    # on PATH. Windows names the tool SqlLocalDB.exe. Get-Command is
    # case-insensitive there and case-sensitive on Linux, so both names
    # are checked. A missing command is not an error: with
    # $ErrorActionPreference = Stop that would become exit 1.
    foreach ($name in @('sqllocaldb', 'SqlLocalDB', 'SqlLocalDB.exe')) {
        $command = Get-Command -Name $name -CommandType Application -ErrorAction SilentlyContinue
        if ($null -ne $command) { return $true }
    }
    return $false
}

function Invoke-CommercialSmokeMssqlE2E {
    param(
        [string]$E2eExe,
        [string[]]$E2eCandidates = @(),
        [switch]$SkipMssql,
        [scriptblock]$LocalDbProbe,
        [string]$MssqlDb = '',
        [string]$MssqlServer = '',
        [string]$MssqlDatabase = ''
    )
    if (-not [string]::IsNullOrWhiteSpace($MssqlDb)) {
        if ($SkipMssql) {
            Write-Host 'MSSQL_E2E REFUSED (-SkipMssql and -MssqlDb conflict)' -ForegroundColor Red
            return [pscustomobject]@{
                ExitCode = 1
                Invoked = $false
            }
        }
        return Invoke-CommercialSmokeExplicitMssqlE2E -E2eExe $E2eExe -E2eCandidates $E2eCandidates -ConnectionString $MssqlDb -Server $MssqlServer -DatabaseName $MssqlDatabase
    }
    if ($SkipMssql) {
        Write-Host "MSSQL_E2E SKIPPED (explicit -SkipMssql; external prerequisite not satisfied)" -ForegroundColor Yellow
        return [pscustomobject]@{
            ExitCode = 0
            Invoked = $false
        }
    }
    $available = $false
    if ($null -ne $LocalDbProbe) {
        $available = [bool](& $LocalDbProbe)
    } else {
        $available = [bool](Test-CommercialSmokeLocalDbAvailable)
    }
    if (-not $available) {
        # Exit code 6 is documented in the script header. Do not report
        # SKIPPED or PASS, and do not continue into GUI smoke.
        Write-Host "MSSQL_E2E E2E_UNAVAILABLE (LocalDB not found; pass -SkipMssql to record as not-run)" -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 6
            Invoked = $false
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($E2eExe) -and (Test-Path -LiteralPath $E2eExe)) {
        Write-Host ('MSSQL_E2E EXE ' + $E2eExe)
        # Same owned-PID stop as -MssqlDb. On Windows the child is in a
        # kill-on-close job and its output goes to files. Never select
        # by image name.
        $localDbArgumentLine = '--login-host 127.0.0.1 --backend mssql_odbc --init-schema'
        $localDbArguments = @($localDbArgumentLine.Split(' '))
        $localRun = Invoke-CommercialSmokeOwnedExecutable -FilePath $E2eExe -ArgumentList $localDbArguments
        if ([int]$localRun.ExitCode -ne 0) {
            throw "MSSQL single-command E2E failed with exit code $($localRun.ExitCode)"
        }
        Write-Host "MSSQL_E2E PASS (LocalDB, one command)" -ForegroundColor Green
        return [pscustomobject]@{
            ExitCode = 0
            Invoked = $true
        }
    }
    Write-Host "MSSQL_E2E SKIPPED (mxh_client_e2e not built)" -ForegroundColor Yellow
    return [pscustomobject]@{
        ExitCode = 0
        Invoked = $false
    }
}

function Test-CommercialSmokeCtestExecutableExtension {
    param([string]$Path)
    # True only when the path ends in .exe. The comparison is
    # case-insensitive, so ctest.EXE matches and ctest.cmd / ctest.bat
    # do not. The Windows caller checks the first PATH hit only.
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    $extension = [IO.Path]::GetExtension($Path)
    if ([string]::IsNullOrWhiteSpace($extension)) { return $false }
    return $extension.Equals('.exe', [StringComparison]::OrdinalIgnoreCase)
}

$ErrorActionPreference = 'Stop'
if ($SkipMssql -and -not [string]::IsNullOrWhiteSpace($MssqlDb)) {
    Write-Host 'MSSQL_E2E REFUSED (-SkipMssql and -MssqlDb conflict)' -ForegroundColor Red
    exit 1
}
$explicitServer = ''
$explicitDatabase = ''
if (-not [string]::IsNullOrWhiteSpace($MssqlDb)) {
    $resolvedMssql = Resolve-CommercialSmokeMssqlConnection -ConnectionString $MssqlDb
    if (-not $resolvedMssql.Ok) {
        Write-Host $resolvedMssql.Message -ForegroundColor Red
        exit 1
    }
    $explicitServer = [string]$resolvedMssql.Server
    $explicitDatabase = [string]$resolvedMssql.Database
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $PSScriptRoot '..\modern\build' }
$ctest = Join-Path $BuildDir 'CTestTestfile.cmake'
if (-not (Test-Path -LiteralPath $ctest)) { throw "Build directory is not configured: $BuildDir" }
$filters = @(
    'LoginServerFixture.ScaffoldSmoke',
    'LoginServerFixture.ConnectReceivesDistConnectSuccess',
    'LoginServerFixture.LegacyLoginValidCredsReceivesAck',
    'LoginServerFixture.LegacyLoginInvalidCredsReceivesNack',
    'LoginServerFixtureGolden.GoldenCapturesDistConnectSuccess',
    'LoginServerFixtureGolden.GoldenCapturesLoginAck',
    'LoginServerFixtureGolden.GoldenCapturesLoginNack',
    'EncryptedLoginFixture.EncryptedLoginAckMatchesGolden',
    'WireFormatGolden.RoundTrip_login_ack',
    'WireFormatGolden.RoundTrip_login_nack',
    'WireFormatGolden.RoundTrip_login_request',
    'ResourcePayloadSha256.*',
    'ResourceByteLevel.*',
    'MoxianClientE2E.*',
    'LoginSbsE2E.*'
)
$pattern = ($filters -join '|')
$ctestArgs = @('-C','Debug','--test-dir',$BuildDir,'-R',$pattern,'--output-on-failure')
if ($RepeatFlaky) { $ctestArgs += @('--repeat','until-fail:3') }
$ctestMatches = @(Get-Command -Name 'ctest' -CommandType Application -ErrorAction SilentlyContinue)
if ($ctestMatches.Count -eq 0) { throw 'ctest was not found on PATH' }
$ctestPath = [string]$ctestMatches[0].Source
# First PATH hit only. On Windows that hit must be ctest.exe. Do not
# search later PATH entries, and do not start a .cmd or .bat: cmd.exe
# would split the unquoted -R pattern on '|'.
if ((Test-CommercialSmokeWindowsHost) -and -not (Test-CommercialSmokeCtestExecutableExtension -Path $ctestPath)) {
    Write-Host ('COMMERCIAL_SMOKE REFUSED (ctest must resolve to ctest.exe on Windows, got ' + $ctestPath + ')') -ForegroundColor Red
    exit 1
}
$ctestRun = Invoke-CommercialSmokeNative -FilePath $ctestPath -ArgumentList $ctestArgs
if ([int]$ctestRun.ExitCode -ne 0) { throw "Commercial smoke failed with exit code $($ctestRun.ExitCode)" }

# P0 gate: one-command MSSQL full-chain E2E on SQL Server LocalDB.
# mxh_client_e2e --backend mssql_odbc --init-schema bootstraps the modern
# schema (creates Moxiang DB + chr_log_info/character_info + test account)
# and drives the 3-process Login/Agent/Map chain over real SQL Server.
# -SkipMssql records an explicit skip. Otherwise a missing LocalDB utility
# is E2E_UNAVAILABLE (exit 6), including when mxh_client_e2e is not built.
# When LocalDB resolves, a missing executable is still the not-built skip.
# -MssqlDb skips that probe. The name is checked before sqlcmd runs, an
# existing database is refused, and a database this run confirmed was
# absent is dropped after E2E whether E2E passes or fails.
$resolvedE2e = Resolve-CommercialSmokeE2EExe -BuildDir $BuildDir
$e2eExe = [string]$resolvedE2e.Path
$e2eCandidates = @($resolvedE2e.Candidates)
$mssql = Invoke-CommercialSmokeMssqlE2E -E2eExe $e2eExe -E2eCandidates $e2eCandidates -SkipMssql:$SkipMssql -LocalDbProbe $LocalDbProbe -MssqlDb $MssqlDb -MssqlServer $explicitServer -MssqlDatabase $explicitDatabase
if ($mssql.ExitCode -ne 0) { exit $mssql.ExitCode }

if ($SkipGui) {
    Write-Host "GUI_CLIENT_SMOKE SKIPPED (explicit -SkipGui)" -ForegroundColor Yellow
} else {
    $guiResult = Invoke-CommercialSmokeGuiClient -BuildDir $BuildDir -HostVersion $PSVersionTable.PSVersion
    if (-not $guiResult.Invoked) { exit $guiResult.ExitCode }
    if ($guiResult.ExitCode -ne 0) {
        throw "GUI client smoke failed with exit code $($guiResult.ExitCode)"
    }
}

Write-Host "COMMERCIAL_SMOKE PASS" -ForegroundColor Green
