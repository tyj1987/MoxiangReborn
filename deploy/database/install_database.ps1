[CmdletBinding()]
param(
    [string]$SqlServer = '.',
    [string]$SqlCmdPath = '',
    [string]$ServiceIdentity = 'NT AUTHORITY\NETWORK SERVICE',
    [switch]$SkipRuntimeIdentity
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$schemaPath = Join-Path $scriptDir 'mx_modern_schema_mssql.sql'
$identityPath = Join-Path $scriptDir 'setup_db.sql'

function Resolve-SqlCmd {
    if (-not [string]::IsNullOrWhiteSpace($SqlCmdPath)) {
        if (-not (Test-Path -LiteralPath $SqlCmdPath -PathType Leaf)) {
            throw "sqlcmd was not found: $SqlCmdPath"
        }
        return (Resolve-Path -LiteralPath $SqlCmdPath).Path
    }
    $command = Get-Command sqlcmd.exe -ErrorAction SilentlyContinue
    if ($null -ne $command) { return $command.Source }
    $candidates = @(
        'C:\Program Files\Microsoft SQL Server\Client SDK\ODBC\180\Tools\Binn\SQLCMD.EXE',
        'C:\Program Files\Microsoft SQL Server\Client SDK\ODBC\170\Tools\Binn\SQLCMD.EXE'
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    throw 'sqlcmd is required. Install Microsoft SQL Server command-line utilities.'
}

if (-not (Test-Path -LiteralPath $schemaPath -PathType Leaf)) {
    throw "Schema file is missing: $schemaPath"
}

$odbc18 = Get-OdbcDriver -Name 'ODBC Driver 18 for SQL Server' -Platform '32-bit' -ErrorAction SilentlyContinue
if ($null -eq $odbc18) {
    throw 'The x86 ODBC Driver 18 for SQL Server is required by the x86 modern servers.'
}

$sqlcmd = Resolve-SqlCmd
Write-Host "SQL preflight: server=$SqlServer sqlcmd=$sqlcmd x86-odbc=18" -ForegroundColor Cyan

& $sqlcmd -S $SqlServer -E -C -b -Q 'SET NOCOUNT ON; SELECT @@VERSION AS version;'
if ($LASTEXITCODE -ne 0) { throw "Cannot connect to SQL Server: $SqlServer" }

& $sqlcmd -S $SqlServer -E -C -b -i $schemaPath
if ($LASTEXITCODE -ne 0) { throw "Modern schema migration failed with exit code $LASTEXITCODE" }

if (-not $SkipRuntimeIdentity) {
    if (-not (Test-Path -LiteralPath $identityPath -PathType Leaf)) {
        throw "Runtime identity script is missing: $identityPath"
    }
    & $sqlcmd -S $SqlServer -E -C -b -v "ServiceIdentity=$ServiceIdentity" -i $identityPath
    if ($LASTEXITCODE -ne 0) { throw "Runtime identity provisioning failed with exit code $LASTEXITCODE" }
}

& $sqlcmd -S $SqlServer -E -C -b -d Moxiang -Q 'SET NOCOUNT ON; SELECT MAX(version) AS schema_version FROM dbo.modern_schema_version;'
if ($LASTEXITCODE -ne 0) { throw 'Schema version verification failed' }

Write-Host 'Moxiang SQL Server staging database is ready; no default accounts were created.' -ForegroundColor Green
