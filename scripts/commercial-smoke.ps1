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
Write-Host "COMMERCIAL_SMOKE PASS" -ForegroundColor Green
