<#
.SYNOPSIS
    Commercial smoke: filtered ctest, MSSQL LocalDB E2E, and GUI client smoke.

.DESCRIPTION
    Exit codes:
      0  ctest passed. MSSQL E2E passed, was skipped with -SkipMssql, or was
         skipped because mxh_client_e2e is not built while LocalDB resolved.
         GUI smoke passed or -SkipGui was set.
      1  a step failed, or GUI smoke refused a host below PowerShell 7.4.
      6  MSSQL_E2E E2E_UNAVAILABLE. sqllocaldb / SqlLocalDB did not resolve
         on PATH and -SkipMssql was not passed. The run prints
         MSSQL_E2E E2E_UNAVAILABLE (LocalDB not found; pass -SkipMssql to record as not-run)
         and stops. That result is neither SKIPPED nor PASS.
         2 through 5 are left unused here so a direct exit stays distinct
         from scripts/clean-deploy.ps1, which already uses those codes.

    -LocalDbProbe is an optional scriptblock. When it is set, that block
    is the LocalDB check for this run. Production omits it and calls
    Test-CommercialSmokeLocalDbAvailable. The probe does not read an
    environment variable.
#>
param(
    [string]$BuildDir = '',
    [switch]$RepeatFlaky,
    [switch]$SkipMssql,
    [switch]$SkipGui,
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
        [scriptblock]$LocalDbProbe
    )
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
$e2eExe = Join-Path $BuildDir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
$mssql = Invoke-CommercialSmokeMssqlE2E -E2eExe $e2eExe -SkipMssql:$SkipMssql -LocalDbProbe $LocalDbProbe
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
