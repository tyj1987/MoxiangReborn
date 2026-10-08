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
    AND 4 = (
        SELECT COUNT(*)
        FROM (VALUES
            (N'base_gengol', N'ck_character_info_base_gengol'),
            (N'base_minchub', N'ck_character_info_base_minchub'),
            (N'base_cheryuk', N'ck_character_info_base_cheryuk'),
            (N'base_simmek', N'ck_character_info_base_simmek')
        ) AS required(column_name, constraint_name)
        JOIN sys.columns AS c
          ON c.object_id = OBJECT_ID(N'dbo.character_info')
         AND c.name = required.column_name
        JOIN sys.check_constraints AS ck
          ON ck.parent_object_id = c.object_id
         AND ck.name = required.constraint_name
        CROSS APPLY (SELECT LOWER(REPLACE(REPLACE(REPLACE(REPLACE(
            REPLACE(REPLACE(REPLACE(REPLACE(ck.definition,
                N'(', N''), N')', N''), N'[', N''), N']', N''),
                N' ', N''), NCHAR(9), N''), NCHAR(10), N''), NCHAR(13), N''))
            AS normalized) AS expr_norm
        WHERE TYPE_NAME(c.user_type_id) = N'int'
          AND c.is_nullable = 1
          AND c.default_object_id = 0
          AND c.is_identity = 0
          AND ck.is_disabled = 0
          AND ck.is_not_trusted = 0
          AND expr_norm.normalized IN (
              required.column_name + N'>=0and' + required.column_name + N'<=65535',
              required.column_name + N'<=65535and' + required.column_name + N'>=0'
          )
    )
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
