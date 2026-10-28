[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$DbToolPath,
    [ValidatePattern('^[A-Za-z_][A-Za-z0-9_]*$')]
    [string]$DatabaseName = 'MoxiangOriginalR18Test_20261009',
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
$contractSql = @"
SELECT CASE WHEN
    DB_NAME() = N'$DatabaseName'
    AND OBJECT_ID(N'dbo.modern_schema_version', N'U') IS NOT NULL
    AND (SELECT MAX(version) FROM dbo.modern_schema_version) = 4
    AND COL_LENGTH(N'dbo.chr_log_info', N'id') IS NOT NULL
    AND COL_LENGTH(N'dbo.chr_log_info', N'pw') IS NOT NULL
    AND COL_LENGTH(N'dbo.chr_log_info', N'userlevel') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'chrid') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'userid') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'charname') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'position_x') IS NOT NULL
    AND COL_LENGTH(N'dbo.character_info', N'position_z') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'player_id') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'money') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'exp') IS NOT NULL
    AND COL_LENGTH(N'dbo.modern_player_state', N'level') IS NOT NULL
    AND OBJECT_ID(N'dbo.modern_player_vitals', N'U') IS NOT NULL
    AND 8 = (
        SELECT COUNT(*) FROM (VALUES
            (N'player_id', N'bigint', 0),
            (N'hp', N'bigint', 1),
            (N'shield', N'bigint', 1),
            (N'mp', N'bigint', 1),
            (N'dead', N'bit', 0),
            (N'initialized', N'bit', 0),
            (N'revision', N'bigint', 0),
            (N'updated_at', N'nvarchar', 0)
        ) AS required(column_name, type_name, nullable)
        JOIN sys.columns AS c
          ON c.object_id = OBJECT_ID(N'dbo.modern_player_vitals')
         AND c.name = required.column_name
        WHERE TYPE_NAME(c.user_type_id) = required.type_name
          AND c.is_nullable = required.nullable
          AND c.is_identity = 0
    )
    AND EXISTS (
        SELECT 1 FROM sys.key_constraints AS pk
        JOIN sys.indexes AS ix
          ON ix.object_id = pk.parent_object_id
         AND ix.index_id = pk.unique_index_id
        JOIN sys.index_columns AS ic
          ON ic.object_id = ix.object_id
         AND ic.index_id = ix.index_id
         AND ic.key_ordinal = 1
        JOIN sys.columns AS pkc
          ON pkc.object_id = ic.object_id AND pkc.column_id = ic.column_id
        WHERE pk.parent_object_id = OBJECT_ID(N'dbo.modern_player_vitals')
          AND pk.type = N'PK'
          AND ix.is_disabled = 0
          AND pkc.name = N'player_id'
          AND 1 = (SELECT COUNT(*) FROM sys.index_columns AS keys
                   WHERE keys.object_id = ix.object_id
                     AND keys.index_id = ix.index_id
                     AND keys.key_ordinal > 0)
    )
    AND EXISTS (
        SELECT 1 FROM sys.check_constraints AS ck
        WHERE ck.parent_object_id = OBJECT_ID(N'dbo.modern_player_vitals')
          AND ck.name = N'ck_modern_player_vitals_state'
          AND ck.is_disabled = 0 AND ck.is_not_trusted = 0
    )
    AND 4 = (
        SELECT COUNT(*)
        FROM (VALUES
            (N'max_level', N'ck_modern_player_state_max_level', N'1', N'120'),
            (N'exp_flag', N'ck_modern_player_state_exp_flag', N'0', N'31'),
            (N'level_up_point', N'ck_modern_player_state_level_up_point', N'0', N'32767'),
            (N'hp_mp_flag', N'ck_modern_player_state_hp_mp_flag', N'0', N'15')
        ) AS required(column_name, constraint_name, low_bound, high_bound)
        JOIN sys.columns AS c
          ON c.object_id = OBJECT_ID(N'dbo.modern_player_state')
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
              required.column_name + N'>=' + required.low_bound + N'and' +
                  required.column_name + N'<=' + required.high_bound,
              required.column_name + N'<=' + required.high_bound + N'and' +
                  required.column_name + N'>=' + required.low_bound
          )
    )
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
"@

$output = @(& $DbToolPath query --db-env $DatabaseConfigEnv $contractSql 2>$null)
if ($LASTEXITCODE -ne 0 -or $output.Count -ne 3 -or
    [string]$output[0] -ne 'contract_ok' -or
    [string]$output[1] -ne '1' -or
    [string]$output[2] -ne '(1 rows)') {
    throw 'Isolated MSSQL writer contract check failed; no servers were started.'
}
Write-Output 'Isolated MSSQL writer contract verified.'
