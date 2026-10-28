<#
.SYNOPSIS
    Commercial smoke: filtered ctest, MSSQL E2E, and GUI client smoke.

.DESCRIPTION
    Exit codes:
      0  ctest passed. MSSQL E2E passed, was skipped with -SkipMssql, or was
         skipped because mxh_client_e2e is not built while LocalDB resolved.
         GUI smoke passed or -SkipGui was set.
      1  a step failed, GUI smoke refused a host below PowerShell 7.4,
         -MssqlDb was rejected before connect, the named database already
         existed, or DROP DATABASE failed after E2E.
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
    passed verbatim to mxh_client_e2e --db. The database= value must
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
        $command = Get-Command -Name $name -CommandType Application -ErrorAction SilentlyContinue
        if ($null -ne $command) { return [string]$command.Source }
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
    $cmdArgs = @('-E', '-S', $Server, '-d', 'master', '-b', '-h-1', '-W', '-w65535', '-Q', $Query)
    $code = 1
    $output = $null
    try {
        $output = & $SqlCmd @cmdArgs 2>&1
        if ($null -ne $LASTEXITCODE) { $code = [int]$LASTEXITCODE }
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

function Invoke-CommercialSmokeExplicitMssqlE2E {
    param(
        [string]$E2eExe,
        [string]$ConnectionString,
        [string]$Server,
        [string]$DatabaseName
    )
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
    if (-not (Test-Path -LiteralPath $E2eExe)) {
        Write-Host 'MSSQL_E2E FAILED (mxh_client_e2e not built; database was absent and was not dropped)' -ForegroundColor Red
        return [pscustomobject]@{
            ExitCode = 1
            Invoked = $false
        }
    }
    # Confirmed absent before E2E. The finally block is the only DROP,
    # and it runs for both a passing and a failing E2E process.
    $confirmedAbsent = $true
    $e2eExit = 1
    $dropFailed = $false
    try {
        & $E2eExe --login-host 127.0.0.1 --login-port 26001 --agent-port 27001 --map-port 28001 --backend mssql_odbc --init-schema --db $ConnectionString
        if ($null -ne $LASTEXITCODE) { $e2eExit = [int]$LASTEXITCODE } else { $e2eExit = 0 }
    } finally {
        try {
            Get-Process -Name 'mxh_login_server','mxh_agent_server_CHINA','mxh_agent_server_KOR','mxh_map_server_CHINA','mxh_map_server_KOR' -ErrorAction SilentlyContinue |
                Stop-Process -Force -ErrorAction SilentlyContinue
        } catch { }
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
        return Invoke-CommercialSmokeExplicitMssqlE2E -E2eExe $E2eExe -ConnectionString $MssqlDb -Server $MssqlServer -DatabaseName $MssqlDatabase
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
    if (Test-Path -LiteralPath $E2eExe) {
        & $E2eExe --login-host 127.0.0.1 --backend mssql_odbc --init-schema
        if ($LASTEXITCODE -ne 0) {
            throw "MSSQL single-command E2E failed with exit code $LASTEXITCODE"
        }
        Write-Host "MSSQL_E2E PASS (LocalDB, one command)" -ForegroundColor Green
        # The E2E tool terminates its own server children, but a flake can
        # leave orphaned mxh_* processes holding the redirected output pipes
        # open, which keeps this host from exiting.  Clean them up so the
        # gate always terminates.
        Get-Process -Name 'mxh_login_server','mxh_agent_server_CHINA','mxh_agent_server_KOR','mxh_map_server_CHINA','mxh_map_server_KOR' -ErrorAction SilentlyContinue |
            Stop-Process -Force -ErrorAction SilentlyContinue
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
$args = @('-C','Debug','--test-dir',$BuildDir,'-R',$pattern,'--output-on-failure')
if ($RepeatFlaky) { $args += @('--repeat','until-fail:3') }
& ctest @args
if ($LASTEXITCODE -ne 0) { throw "Commercial smoke failed with exit code $LASTEXITCODE" }

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
$e2eExe = Join-Path $BuildDir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
$mssql = Invoke-CommercialSmokeMssqlE2E -E2eExe $e2eExe -SkipMssql:$SkipMssql -LocalDbProbe $LocalDbProbe -MssqlDb $MssqlDb -MssqlServer $explicitServer -MssqlDatabase $explicitDatabase
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
