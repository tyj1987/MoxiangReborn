[CmdletBinding()]
param(
    [string]$Server = '(localdb)\MSSQLLocalDB'
)

$ErrorActionPreference = 'Continue'
$result = [ordered]@{
    server = $Server
    sqlcmd = 'missing'
    localdb = 'missing'
    connectivity = 'not-run'
}

$sqlcmd = Get-Command sqlcmd.exe -ErrorAction SilentlyContinue
if ($sqlcmd) {
    $result.sqlcmd = $sqlcmd.Source
    & $sqlcmd.Source -S $Server -E -Q 'select 1' -l 5 2>&1 | Out-Host
    if ($LASTEXITCODE -eq 0) { $result.connectivity = 'pass' } else { $result.connectivity = "fail:$LASTEXITCODE" }
}

$sqllocaldb = Get-Command SqlLocalDB.exe -ErrorAction SilentlyContinue
if ($sqllocaldb) {
    $result.localdb = $sqllocaldb.Source
    & $sqllocaldb.Source info MSSQLLocalDB 2>&1 | Out-Host
}

Write-Host ('MSSQL prerequisite summary: ' + (($result.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join '; '))
if ($result.connectivity -ne 'pass') { exit 1 }
exit 0
