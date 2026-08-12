# deploy/portal/stop_portal.ps1
# Stops the Moxian Portal HTTP server.

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$deployRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$stateDir = Join-Path $deployRoot "deploy\runtime\portal"
$pidFile = Join-Path $stateDir "pid.txt"

if (Test-Path -LiteralPath $pidFile) {
    $json = Get-Content -LiteralPath $pidFile -Raw | ConvertFrom-Json
    if ($json -and $json.pid) {
        $p = Get-Process -Id ([int]$json.pid) -ErrorAction SilentlyContinue
        if ($p) {
            Stop-Process -Id $json.pid -Force
            Write-Host "Portal stopped (pid=$($json.pid))" -ForegroundColor Green
        } else {
            Write-Host "Portal process not running" -ForegroundColor Gray
        }
    }
    Remove-Item -LiteralPath $pidFile -Force
} else {
    Write-Host "No pid file found -- portal may not be running" -ForegroundColor Gray
}
