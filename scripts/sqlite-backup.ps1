[CmdletBinding(DefaultParameterSetName = 'Backup')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Backup')][switch]$Backup,
    [Parameter(Mandatory, ParameterSetName = 'Restore')][switch]$Restore,
    [Parameter(Mandatory, ParameterSetName = 'Verify')][switch]$Verify,
    [Parameter(Mandatory)][string]$Database,
    [Parameter(Mandatory)][string]$Archive,
    [Parameter(ParameterSetName = 'Restore')][switch]$Force
)

$ErrorActionPreference = 'Stop'
$databasePath = [IO.Path]::GetFullPath($Database)
$archivePath = [IO.Path]::GetFullPath($Archive)
$manifestPath = "$archivePath.sha256"

function Get-SqliteExe {
    $candidate = Join-Path $PSScriptRoot '..\modern\build\tools\MoxianDbTool\Debug\mxh_db_tool.exe'
    if (-not (Test-Path -LiteralPath $candidate)) { throw "Missing MoxianDbTool: $candidate" }
    return [IO.Path]::GetFullPath($candidate)
}

function Assert-Integrity([string]$path) {
    $tool = Get-SqliteExe
    $cfg = "sqlite;path=$path"
    $result = & $tool query --db $cfg 'PRAGMA integrity_check' 2>&1
    if ($LASTEXITCODE -ne 0 -or ($result -join "`n") -notmatch '(?m)^ok$') {
        throw "SQLite integrity_check failed: $path`n$($result -join "`n")"
    }
}

if ($Backup) {
    if (-not (Test-Path -LiteralPath $databasePath -PathType Leaf)) { throw "Database not found: $databasePath" }
    Assert-Integrity $databasePath
    $parent = Split-Path -Parent $archivePath
    if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
    $tool = Get-SqliteExe
    $escaped = $archivePath.Replace("'", "''")
    & $tool exec --db "sqlite;path=$databasePath" "VACUUM INTO '$escaped'"
    if ($LASTEXITCODE -ne 0) { throw 'VACUUM INTO backup failed' }
    Assert-Integrity $archivePath
    $hash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    Set-Content -LiteralPath $manifestPath -Value "$hash  $([IO.Path]::GetFileName($archivePath))" -Encoding Ascii
    Write-Host "SQLITE_BACKUP_OK archive=$archivePath sha256=$hash"
    exit 0
}

if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) { throw "Archive not found: $archivePath" }
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Checksum manifest not found: $manifestPath" }
$expected = ((Get-Content -LiteralPath $manifestPath -Raw).Trim() -split '\s+')[0].ToLowerInvariant()
$actual = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($expected -ne $actual) { throw "Backup checksum mismatch: expected=$expected actual=$actual" }
Assert-Integrity $archivePath

if ($Verify) {
    Write-Host "SQLITE_VERIFY_OK archive=$archivePath sha256=$actual"
    exit 0
}

if ((Test-Path -LiteralPath $databasePath) -and -not $Force) {
    throw "Restore target exists; pass -Force to replace: $databasePath"
}
$parent = Split-Path -Parent $databasePath
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$staged = "$databasePath.restore-new"
Copy-Item -LiteralPath $archivePath -Destination $staged -Force
Assert-Integrity $staged
Move-Item -LiteralPath $staged -Destination $databasePath -Force
Assert-Integrity $databasePath
Write-Host "SQLITE_RESTORE_OK database=$databasePath sha256=$actual"
