# deploy/portal/smoke-ecs.ps1
# Smoke test the deployed ECS portal from outside the LAN.
# Usage: .\smoke-ecs.ps1 -PublicUrl https://broker.52trz.com/portal

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $PublicUrl
)

$ErrorActionPreference = "Stop"

function Test-Endpoint {
    param([string]$Path, [string]$Expected)
    $url = "$PublicUrl$Path"
    try {
        $resp = Invoke-WebRequest -Uri $url -UseBasicParsing -TimeoutSec 10
        $status = $resp.StatusCode
        $match = ($status -eq $Expected) -or `
                  (($Expected -eq "200") -and ($status -ge 200 -and $status -lt 500))
        if ($match) {
            Write-Host "OK   $Path -> $status" -ForegroundColor Green
            return $true
        } else {
            Write-Host "FAIL $Path -> $status (expected $Expected)" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "ERR  $Path -> $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

$results = @(
    (Test-Endpoint "/api/healthz" "200"),
    (Test-Endpoint "/api/status"  "200"),
    (Test-Endpoint "/"            "200")
)

if ($results -contains $false) {
    Write-Host "FAIL: at least one endpoint returned non-200" -ForegroundColor Red
    exit 1
}
Write-Host "All ECS smoke endpoints OK" -ForegroundColor Green
exit 0
