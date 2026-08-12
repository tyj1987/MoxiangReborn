[CmdletBinding()]
param(
    [ValidateSet("start", "stop", "status", "restart")]
    [string]$Mode = "start",
    [ValidateSet("CHINA", "KOR", "HK", "JAPAN", "TL")]
    [string]$Locale = "CHINA",
    [int]$LoginPort = 16001,
    [int]$AgentPort = 17001,
    [int]$MapPort = 18001,
    [int]$MapNumber = 12,
    [ValidateSet("sqlite", "mssql_odbc")]
    [string]$Backend = "sqlite",
    [string]$DbRoot = "",
    [switch]$DryRun,
    [switch]$UseHsel,
    [string]$DataDir = "",
    [string]$ResourceRoot = ""
)
$ErrorActionPreference = "Stop"
$deployRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$buildRoot = Join-Path $deployRoot "modern\build\tools"
$stateDir = Join-Path $deployRoot "deploy\runtime\modern"
if ([string]::IsNullOrWhiteSpace($DataDir)) { $DataDir = Join-Path $stateDir "data" }
if ([string]::IsNullOrWhiteSpace($DbRoot)) { $DbRoot = $DataDir }
if ([string]::IsNullOrWhiteSpace($ResourceRoot)) { $ResourceRoot = (Resolve-Path (Join-Path $deployRoot "*\PlayDH")).Path }
if ($Backend -eq "mssql_odbc" -and [string]::IsNullOrWhiteSpace($DbRoot)) { throw "DbRoot must be an MSSQL connection string" }
$pidFile = Join-Path $stateDir "pids.json"
New-Item -ItemType Directory -Force -Path $stateDir,$DataDir | Out-Null
function Read-State { if (Test-Path -LiteralPath $pidFile) { $json = Get-Content -LiteralPath $pidFile -Raw | ConvertFrom-Json; return @($json) } return @() }
function Stop-Modern {
    $state = @(Read-State)
    foreach ($entry in $state) { $proc = Get-Process -Id ([int]$entry.pid) -ErrorAction SilentlyContinue; if ($proc) { Stop-Process -Id $proc.Id -Force } }
    Remove-Item -LiteralPath $pidFile -Force -ErrorAction SilentlyContinue
    Write-Host "Modern servers stopped" -ForegroundColor Green
}
function Test-Port([int]$port) { $client = [Net.Sockets.TcpClient]::new(); try { $task = $client.ConnectAsync("127.0.0.1", $port); if (-not $task.Wait(1000)) { return $false }; return $client.Connected } catch { return $false } finally { $client.Dispose() } }
if ($Mode -eq "stop") { Stop-Modern; exit 0 }
if ($Mode -eq "restart") { Stop-Modern; Start-Sleep -Seconds 1 }
if ($Mode -eq "status") { $statusState = @(Read-State); foreach ($entry in $statusState) { $p = Get-Process -Id ([int]$entry.pid) -ErrorAction SilentlyContinue; Write-Host "$($entry.name): $(if ($p) { 'running' } else { 'stopped' }) pid=$($entry.pid)" }; foreach ($port in @($LoginPort,$AgentPort,$MapPort)) { Write-Host "port ${port}: $(Test-Port $port)" }; exit 0 }
$loginInitArgs = if ($Backend -eq "sqlite") { @("--init-schema") } else { @() }
$hselArgs = if ($UseHsel) { @("--use-hsel") } else { @() }
$bins = @(
    @{ name = "login"; exe = Join-Path $buildRoot "MoxianLoginServer\Debug\mxh_login_server.exe"; args = @("--port",$LoginPort,"--backend",$Backend,"--db",$(if ($Backend -eq "sqlite") { Join-Path $DbRoot "login.db" } else { $DbRoot }),"--agent-addr","127.0.0.1","--agent-port",$AgentPort,"--legacy") + $loginInitArgs + $hselArgs },
    @{ name = "agent"; exe = Join-Path $buildRoot "MoxianAgentServer\Debug\mxh_agent_server_$Locale.exe"; args = @("--port",$AgentPort,"--backend",$Backend,"--db",$(if ($Backend -eq "sqlite") { Join-Path $DbRoot "agent.db" } else { $DbRoot }),"--legacy","--map-server","127.0.0.1:$MapPort","--default-map",$MapNumber) + $hselArgs },
    @{ name = "map"; exe = Join-Path $buildRoot "MoxianMapServer\Debug\mxh_map_server_$Locale.exe"; working_dir = $ResourceRoot; args = @("--port",$MapPort,"--map",$MapNumber,"--backend",$Backend,"--db",$(if ($Backend -eq "sqlite") { Join-Path $DbRoot "map.db" } else { $DbRoot }),"--legacy","--resource-root",".") + $hselArgs }
)
if ($DryRun) {
    Write-Host "Modern server dry-run (backend=$Backend locale=$Locale)" -ForegroundColor Cyan
    foreach ($item in $bins) {
        if (-not (Test-Path -LiteralPath $item.exe)) { throw "Missing modern server executable: $($item.exe)" }
        Write-Host "$($item.name): $($item.exe) $($item.args -join ' ')"
    }
    exit 0
}
$state = @()
foreach ($item in $bins) {
    if (-not (Test-Path -LiteralPath $item.exe)) { throw "Missing modern server executable: $($item.exe)" }
    $logDir = Join-Path $stateDir "logs"; New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    $workingDir = if ($item.working_dir) { $item.working_dir } else { Split-Path -Parent $item.exe }
    $proc = Start-Process -FilePath $item.exe -ArgumentList $item.args -WorkingDirectory $workingDir -RedirectStandardOutput (Join-Path $logDir "$($item.name).out.log") -RedirectStandardError (Join-Path $logDir "$($item.name).err.log") -PassThru -WindowStyle Hidden
    $state += [ordered]@{ name = $item.name; pid = $proc.Id; port = switch ($item.name) { "login" {$LoginPort}; "agent" {$AgentPort}; "map" {$MapPort} } }
    Start-Sleep -Milliseconds 500
    if ($proc.HasExited) { throw "$($item.name) exited during startup" }
}
$state | ConvertTo-Json | Set-Content -LiteralPath $pidFile -Encoding utf8
Start-Sleep -Seconds 1
foreach ($port in @($LoginPort,$AgentPort,$MapPort)) { if (-not (Test-Port $port)) { Stop-Modern; throw "Modern server port is not healthy: $port" } }
Write-Host "Modern servers started: Login=$LoginPort Agent=$AgentPort Map=$MapPort" -ForegroundColor Green
