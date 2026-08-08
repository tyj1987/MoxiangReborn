param(
    [string]$BuildDir = "$PSScriptRoot\..\..\build",
    [switch]$RepeatFlaky
)
$ErrorActionPreference = 'Stop'
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
$e2eExe = Join-Path $BuildDir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
if (Test-Path -LiteralPath $e2eExe) {
    & $e2eExe --backend mssql_odbc --init-schema
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
} else {
    Write-Host "MSSQL_E2E SKIPPED (mxh_client_e2e not built)" -ForegroundColor Yellow
}

Write-Host "COMMERCIAL_SMOKE PASS" -ForegroundColor Green
