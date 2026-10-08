[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$DbToolPath,
    [ValidatePattern('^[A-Za-z_][A-Za-z0-9_]*$')]
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $DbToolPath -PathType Leaf)) {
    throw 'Stage database tool is missing.'
}
if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($DatabaseConfigEnv, 'Process'))) {
    throw 'Protected database environment is missing.'
}

# This query is read-only. Missing tables, wrong database, older schema or
# missing columns all fail closed before any server process can be created.
$contractSql = @'
SELECT CASE WHEN
    DB_NAME() = N'MoxiangOriginalTest_20261009'
    AND OBJECT_ID(N'dbo.modern_schema_version', N'U') IS NOT NULL
    AND (SELECT MAX(version) FROM dbo.modern_schema_version) = 1
    AND COL_LENGTH(N'dbo.chr_log_info', N'id') IS NOT NULL
    AND COL_LENGTH(N'dbo.chr_log_info', N'pw') IS NOT NULL
    AND COL_LENGTH(N'dbo.chr_log_info', N'userlevel') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'chrid') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'userid') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'charname') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'player_id') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'money') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'exp') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'level') IS NOT NULL
THEN 1 ELSE 0 END AS contract_ok
'@

$output = @(& $DbToolPath query --db-env $DatabaseConfigEnv $contractSql 2>$null)
if ($LASTEXITCODE -ne 0 -or $output.Count -ne 3 -or
    [string]$output[0] -ne 'contract_ok' -or
    [string]$output[1] -ne '1' -or
    [string]$output[2] -ne '(1 rows)') {
    throw 'Isolated MSSQL writer contract check failed; no servers were started.'
}
Write-Output 'Isolated MSSQL writer contract verified.'
