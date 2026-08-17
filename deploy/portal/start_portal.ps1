# deploy/portal/start_portal.ps1
# Starts the Moxian Portal HTTP server on the local machine.
# Usage: .\start_portal.ps1 [-Port <n>] [-StaticRoot <path>] [-WhatIf]
#
# Required env vars (set before running, or pass as args):
#   PORTAL_JWT_SECRET     -- JWT signing secret (REQUIRED in production)
#   PORTAL_DB_BACKEND     -- 'sqlite' (default) or 'mssql_odbc'
#   PORTAL_DB_PATH        -- DB file or MSSQL connection string
#
# Examples:
#   $env:PORTAL_JWT_SECRET = 'change-me-in-production'; .\start_portal.ps1
#   .\start_portal.ps1 -Port 8080 -StaticRoot 'C:\moxiang\deploy\portal\static'

[CmdletBinding()]
param(
    [int]    $Port        = 8080,
    [string] $StaticRoot  = "",
    [switch] $WhatIf
)

$ErrorActionPreference = "Stop"

$deployRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$exe = Join-Path $deployRoot "modern\build\tools\MoxianPortal\Debug\mxh_portal.exe"

if (-not (Test-Path -LiteralPath $exe)) {
    $alt = Join-Path $deployRoot "modern\build\tools\MoxianPortal\Release\mxh_portal.exe"
    if (Test-Path -LiteralPath $alt) { $exe = $alt }
}

if (-not (Test-Path -LiteralPath $exe)) {
    throw "mxh_portal.exe not found at $exe -- run cmake --build first"
}

# PID file for later stop
$stateDir = Join-Path $deployRoot "deploy\runtime\portal"
New-Item -ItemType Directory -Force -Path $stateDir | Out-Null
$pidFile = Join-Path $stateDir "pid.txt"

# Environment defaults
if (-not $env:PORTAL_JWT_SECRET) {
    # Auto-generate and persist a 64-byte secret if env var is unset.
    # Persisted at deploy/runtime/portal/jwt.secret (file mode 0600).
    $secretDir = $stateDir
    $secretFile = Join-Path $secretDir "jwt.secret"
    if (Test-Path -LiteralPath $secretFile) {
        $env:PORTAL_JWT_SECRET = (Get-Content -LiteralPath $secretFile -Raw).Trim()
        Write-Host "Loaded PORTAL_JWT_SECRET from $secretFile" -ForegroundColor DarkGray
    } else {
        $bytes = New-Object byte[] 64
        (New-Object Random).NextBytes($bytes)
        $secret = [Convert]::ToBase64String($bytes)
        Set-Content -LiteralPath $secretFile -Value $secret -Encoding utf8 -NoNewline
        # Restrict permissions on Windows where possible (icacls)
        $icacls = Get-Command icacls.exe -ErrorAction SilentlyContinue
        if ($icacls) {
            & icacls.exe $secretFile /inheritance:r /grant:r "$env:USERNAME:(R,W)" | Out-Null
        }
        $env:PORTAL_JWT_SECRET = $secret
        Write-Host "Generated new PORTAL_JWT_SECRET and persisted to $secretFile" -ForegroundColor DarkGray
    }
}
if (-not $env:PORTAL_DB_BACKEND) { $env:PORTAL_DB_BACKEND = "sqlite" }
if (-not $env:PORTAL_DB_PATH) {
    $env:PORTAL_DB_PATH = Join-Path $deployRoot "deploy\runtime\modern\data\moxian.db"
}
if (-not $StaticRoot) {
    $StaticRoot = Join-Path $deployRoot "deploy\portal\static"
}
if (-not $env:PORTAL_STATIC_ROOT) { $env:PORTAL_STATIC_ROOT = $StaticRoot }
if (-not $env:PORTAL_CONTENT_ROOT) {
    $env:PORTAL_CONTENT_ROOT = Join-Path $deployRoot "deploy\portal\content"
}
if (-not $env:PORTAL_SHOP_CATALOG) {
    $env:PORTAL_SHOP_CATALOG = Join-Path $deployRoot "deploy\portal\shop\catalog.json"
}
$env:PORTAL_PORT = $Port

if ($WhatIf) {
    Write-Host "WhatIf: would run: $exe"
    Write-Host "  PORTAL_PORT=$env:PORTAL_PORT"
    Write-Host "  PORTAL_STATIC_ROOT=$env:PORTAL_STATIC_ROOT"
    Write-Host "  PORTAL_DB_BACKEND=$env:PORTAL_DB_BACKEND"
    Write-Host "  PORTAL_DB_PATH=$env:PORTAL_DB_PATH"
    exit 0
}

# Stop existing portal on the same port
if (Test-Path -LiteralPath $pidFile) {
    $existing = Get-Content -LiteralPath $pidFile -Raw | ConvertFrom-Json
    if ($existing -and $existing.pid) {
        $p = Get-Process -Id ([int]$existing.pid) -ErrorAction SilentlyContinue
        if ($p) {
            Write-Host "Stopping existing portal (pid=$($existing.pid))..." -ForegroundColor Yellow
            Stop-Process -Id $existing.pid -Force
            Start-Sleep -Milliseconds 500
        }
    }
}

# Start the portal
$logDir = Join-Path $stateDir "logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

Write-Host "Starting Moxian Portal on :$Port..." -ForegroundColor Cyan
$proc = Start-Process -FilePath $exe `
    -RedirectStandardOutput (Join-Path $logDir "portal.out.log") `
    -RedirectStandardError  (Join-Path $logDir "portal.err.log") `
    -PassThru -WindowStyle Hidden

$stateJson = (@{name="portal"; pid=$proc.Id; port=$Port} | ConvertTo-Json)
$stateJson | Set-Content -LiteralPath $pidFile -Encoding utf8

Start-Sleep -Seconds 2
if ($proc.HasExited) {
    $err = Get-Content (Join-Path $logDir "portal.err.log") -ErrorAction SilentlyContinue | Select-Object -Last 10
    Write-Host "FATAL: portal exited immediately: $err" -ForegroundColor Red
    exit 1
}

Write-Host "Portal started: pid=$($proc.Id) port=$Port" -ForegroundColor Green
Write-Host "Logs: $logDir\portal.out.log / portal.err.log" -ForegroundColor Gray
Write-Host "Stop:  .\stop_portal.ps1"
