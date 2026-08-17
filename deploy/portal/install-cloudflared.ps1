# deploy/portal/install-cloudflared.ps1
# Idempotent install of cloudflared + tunnel registration for the Moxian Portal.
# Usage: .\install-cloudflared.ps1 [-TunnelToken <token>] [-TunnelName moxian-portal]

[CmdletBinding()]
param(
    [string] $TunnelToken = "",
    [string] $TunnelName  = "moxian-portal",
    [string] $CloudflaredDir = "$env:ProgramFiles\cloudflared"
)

$ErrorActionPreference = "Stop"

# 1. Install cloudflared if missing.
if (-not (Get-Command cloudflared.exe -ErrorAction SilentlyContinue)) {
    Write-Host "cloudflared not found; downloading installer..." -ForegroundColor Cyan
    $url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"
    $dst = Join-Path $CloudflaredDir "cloudflared.exe"
    New-Item -ItemType Directory -Force -Path $CloudflaredDir | Out-Null
    Invoke-WebRequest -Uri $url -OutFile $dst -UseBasicParsing
    Write-Host "Installed cloudflared to $dst" -ForegroundColor Green
} else {
    Write-Host "cloudflared already on PATH" -ForegroundColor DarkGray
}

# 2. Register a tunnel (if token provided).
if ($TunnelToken) {
    Write-Host "Registering tunnel $TunnelName..." -ForegroundColor Cyan
    cloudflared.exe tunnel login --token $TunnelToken
    cloudflared.exe tunnel create $TunnelName
    Write-Host "Tunnel $TunnelName registered" -ForegroundColor Green
} else {
    Write-Host "No -TunnelToken provided; skipping tunnel registration." -ForegroundColor Yellow
}

# 3. Print next-step instructions.
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Configure DNS: cloudflared tunnel route dns $TunnelName portal.your-domain.com"
Write-Host "  2. Run the portal:  .\start_portal.ps1" -ForegroundColor Gray
Write-Host "  3. Run the tunnel:  cloudflared tunnel run $TunnelName"
