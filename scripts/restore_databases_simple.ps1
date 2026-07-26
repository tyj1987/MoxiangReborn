[CmdletBinding()]
param(
    [string]$BackupDir = $env:MOXIAN_BACKUP_DIR,
    [string]$ServerInstance = $(if ($env:MOXIAN_SQL_INSTANCE) { $env:MOXIAN_SQL_INSTANCE } else { '(local)' }),
    [switch]$TrustServerCertificate
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($BackupDir)) { $BackupDir = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\database_backups' }

$sqlcmd = Get-Command sqlcmd -ErrorAction Stop
$databases = @('MHCMEMBER','MHGAME','MHLOG')
$missing = $databases | ForEach-Object { $path = Join-Path $BackupDir ($_.ToString() + '.bak'); if (-not (Test-Path -LiteralPath $path)) { $path } }
if ($missing) { throw "Backup files missing under '$BackupDir': $($missing -join ', ')" }

$common = @('-S', $ServerInstance, '-b', '-r', '1')
if ($TrustServerCertificate) { $common += @('-C') }
Write-Host "Restoring Moxian databases to $ServerInstance from $BackupDir"
foreach ($database in $databases) {
    $backup = (Join-Path $BackupDir ($database + '.bak')).Replace("'", "''")
    $sql = "RESTORE DATABASE [$database] FROM DISK = N'$backup' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 10"
    & $sqlcmd.Source @common '-Q' $sql
    if ($LASTEXITCODE -ne 0) { throw "Restore failed for $database (exit $LASTEXITCODE)" }
}
$query = "SELECT name, state_desc FROM sys.databases WHERE name IN ('MHCMEMBER','MHGAME','MHLOG') ORDER BY name"
& $sqlcmd.Source @common '-Q' $query
if ($LASTEXITCODE -ne 0) { throw "Database verification failed (exit $LASTEXITCODE)" }
Write-Host 'Restore and verification completed.'

