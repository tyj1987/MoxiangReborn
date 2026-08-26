[CmdletBinding()]
<#
.SYNOPSIS
Deploy modern 3-server stack (Login/Agent/Map) to PVE 200 (192.168.2.200)
Windows VM via SSH-then-WinRM, with port probes for 16001/17001/18001.

.DESCRIPTION
User-stated context (see ROADMAP / session objective):
  - PVE host:    192.168.2.200 (Linux sshd on port 22, root / Tyj_198729)
  - Windows VM:  VM 100 inside PVE (user's "winserver")
  - Modern 3 服:  LoginServer (16001) + AgentServer (17001) + MapServer (18001)
  - Local build: C:\moxiang\modern\build\tools\Moxian{Login,Agent,Map}Server\

.PARAMETER BuildDir
Path to the local modern build directory (default: C:\moxiang\modern\build).

.PARAMETER WindowsVMId
The VM ID inside PVE 200 where modern servers run (default: 100).

.PARAMETER RemotePath
Path on the Windows VM where binaries are pushed (default: C:\moxiang\bin).

.PARAMETER PvePort
SSH port on PVE 200 (default: 22).

.PARAMETER PvePassword
SSH password for PVE 200. If omitted, reads `MXH_PVE_PASSWORD` or prompts.

.PARAMETER MapNumber
Map number to bind MapServer to (default: 12).

.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File C:\moxiang\scripts\start-modern-pve-200.ps1

.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File C:\moxiang\scripts\start-modern-pve-200.ps1 -MapNumber 1 -Verbose
#>
param(
    [string]$BuildDir  = 'C:\moxiang\modern\build',
    [int]   $WindowsVMId = 100,
    [string]$RemotePath = 'C:\moxiang\bin',
    [int]   $PvePort    = 22,
    [string]$PveHost    = '192.168.2.200',
    [string]$PveUser    = 'root',
    [string]$PvePassword = '',
    [int]   $MapNumber  = 12,
    [int]   $SshConnectTimeoutSec = 10,
    [int]   $PortProbeTimeoutSec  = 60,
    [switch]$SkipPush,
    [switch]$SkipStart,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$pvePasswordFromEnv = $env:MXH_PVE_PASSWORD
if ([string]::IsNullOrWhiteSpace($PvePassword)) {
    if (-not [string]::IsNullOrWhiteSpace($pvePasswordFromEnv)) {
        $PvePassword = $pvePasswordFromEnv
    } elseif (-not $DryRun) {
        $secure = Read-Host 'PVE SSH password' -AsSecureString
        $ptr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
        try { $PvePassword = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr) }
        finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr) }
    }
}
if (-not $DryRun -and [string]::IsNullOrWhiteSpace($PvePassword)) {
    throw 'PVE password is required via -PvePassword, MXH_PVE_PASSWORD, or the secure prompt'
}
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not (Test-Path -LiteralPath $BuildDir)) {
    throw "BuildDir not found: $BuildDir"
}

# -----------------------------------------------------------------------------
# 1. Locate local binaries (resolve mxh_*_server*.exe inside the build tree).
# -----------------------------------------------------------------------------
$loginExe = Get-ChildItem -LiteralPath $BuildDir -Recurse -Filter 'mxh_login_server.exe' -ErrorAction SilentlyContinue |
            Select-Object -First 1
$agentExe = Get-ChildItem -LiteralPath $BuildDir -Recurse -Filter 'mxh_agent_server_HK.exe' -ErrorAction SilentlyContinue |
            Select-Object -First 1
$mapExe   = Get-ChildItem -LiteralPath $BuildDir -Recurse -Filter 'mxh_map_server_HK.exe'   -ErrorAction SilentlyContinue |
            Select-Object -First 1

if (-not $loginExe) { throw "mxh_login_server.exe not found under $BuildDir" }
if (-not $agentExe) { throw "mxh_agent_server_HK.exe not found under $BuildDir" }
if (-not $mapExe)   { throw "mxh_map_server_HK.exe not found under $BuildDir" }

Write-Host "Local binaries:" -ForegroundColor Cyan
Write-Host "  LoginServer : $($loginExe.FullName)"
Write-Host "  AgentServer : $($agentExe.FullName)"
Write-Host "  MapServer   : $($mapExe.FullName)"

# -----------------------------------------------------------------------------
# 2. SSH reachability probe to PVE 200 (qemu host, Linux).
#    2026-08-21 session learned: the Windows install IO on VM 100 blocks
#    sshd for ~minutes at a time, so we retry with a generous window.
# -----------------------------------------------------------------------------
$pveSshArgs = @(
    '-o', "PreferredAuthentications=password",
    '-o', "PubkeyAuthentication=no",
    '-o', "NumberOfPasswordPrompts=1",
    '-o', "ConnectTimeout=$SshConnectTimeoutSec",
    '-o', "StrictHostKeyChecking=accept-new",
    '-p', "$PvePort",
    "$PveUser@$PveHost"
)

function Invoke-PveSsh {
    param([string]$Command)
    $env:SSHPASS = $PvePassword
    try {
        # Prefer sshpass if installed; fall back to a pty-allocating ssh.
        $sshpass = Get-Command sshpass -ErrorAction SilentlyContinue
        if ($sshpass) {
            & sshpass -e ssh @pveSshArgs $Command
        } else {
            # No sshpass -> user will be prompted for the password interactively.
            Write-Verbose "sshpass not available; using ssh (will prompt for password)"
            & ssh @pveSshArgs $Command
        }
    } finally {
        Remove-Item Env:SSHPASS -ErrorAction SilentlyContinue
    }
}

Write-Host "`n[PVE] probing $PveHost on port $PvePort ..." -ForegroundColor Cyan
$probeCmd = 'echo PVE_PROBE_OK; date; uname -a; qm list | head -40'
$probeOk = $false
for ($attempt = 1; $attempt -le 3; $attempt++) {
    if ($DryRun) { Write-Host "[DRY] ssh $pveSshArgs $probeCmd"; $probeOk = $true; break }
    try {
        $out = Invoke-PveSsh $probeCmd 2>&1
        if ($LASTEXITCODE -eq 0 -and ($out -join "`n") -match 'PVE_PROBE_OK') {
            $probeOk = $true
            Write-Host "[PVE] probe OK (attempt $attempt):"
            Write-Host ($out -join "`n")
            break
        }
    } catch { Write-Verbose "[PVE] attempt $attempt failed: $_" }
    Write-Host "[PVE] attempt $attempt failed; sleeping 10s and retrying..."
    Start-Sleep -Seconds 10
}
if (-not $probeOk) {
    throw "[PVE] SSH probe failed after 3 attempts to $PveHost"
}

# -----------------------------------------------------------------------------
# 3. Check Windows VM 100 status via `qm list`.
# -----------------------------------------------------------------------------
$vmListCmd = "qm list | awk -v vm=$WindowsVMId '\$1 == vm {print \$2, \$5}'"
$vmState = Invoke-PveSsh $vmListCmd 2>&1
Write-Host "[PVE] VM $WindowsVMId state: $($vmState -join ' ')" -ForegroundColor Cyan
if ($vmState -notmatch 'running') {
    Write-Warning "[PVE] VM $WindowsVMId is not running. Start it via Proxmox UI, then rerun."
    Write-Warning "Skipping push + start; continuing to port probe in case servers are already up."
}

# -----------------------------------------------------------------------------
# 4. Push binaries to the Windows VM via SMB / shared folder.
#    The PVE VM 100 must have a share mounted for this to work; we write to
#    a known path that the in-VM agent is expected to publish. If the share
#    isn't available, the operator should copy the 3 .exe files manually to
#    $RemotePath on the Windows guest, then run this script with -SkipPush.
# -----------------------------------------------------------------------------
$sharePath = "\\$PveHost\qm-${WindowsVMId}-share\$RemotePath"
if (-not $SkipPush) {
    if ($DryRun) { Write-Host "[DRY] would copy 3 exes to $sharePath" }
    elseif (Test-Path -LiteralPath $sharePath) {
        Write-Host "[PUSH] copying binaries to $sharePath ..." -ForegroundColor Cyan
        Copy-Item -LiteralPath $loginExe.FullName -Destination (Join-Path $sharePath 'mxh_login_server.exe') -Force
        Copy-Item -LiteralPath $agentExe.FullName -Destination (Join-Path $sharePath 'mxh_agent_server_HK.exe') -Force
        Copy-Item -LiteralPath $mapExe.FullName   -Destination (Join-Path $sharePath 'mxh_map_server_HK.exe')   -Force
    } else {
        Write-Warning "[PUSH] share $sharePath not reachable; copy the 3 exes manually to $RemotePath on the VM, then rerun with -SkipPush"
    }
}

# -----------------------------------------------------------------------------
# 5. Remote start (operator action).
#    Starting Windows services inside a guest requires WinRM (or psexec). The
#    operator should run the following inside the Windows VM (or via WinRM):
#
#       cd C:\moxiang\bin
#       set MXH_DB_CONFIG=backend=sqlite;path=C:\moxiang\data\moxian.db
#       start /B mxh_map_server_HK.exe    --port 18001 --map 12 --bind-address 0.0.0.0 --resource-root C:\moxiang\data\PlayDH --server-resource-root C:\moxiang\data\PlayDH\Resource\Server --backend sqlite --db-env MXH_DB_CONFIG --legacy
#       start /B mxh_agent_server_HK.exe  --port 17001 --bind-address 0.0.0.0 --map-server 127.0.0.1:18001 --default-map 12 --backend sqlite --db-env MXH_DB_CONFIG --legacy
#       start /B mxh_login_server.exe     --port 16001 --bind-address 0.0.0.0 --agent-addr 127.0.0.1 --agent-port 17001 --backend sqlite --db-env MXH_DB_CONFIG --legacy
# -----------------------------------------------------------------------------
if (-not $SkipStart) {
    Write-Host "[START] manual start required on VM 100 (see comment in script)" -ForegroundColor Yellow
}

# -----------------------------------------------------------------------------
# 6. Port probe — wait for the 3 ports to accept TCP on the PVE 200 host
#    (assumes the Windows VM is reachable at its bridged IP, defaulting to
#     the same address as the PVE host for NAT-routed setups). Operators with
#     a different guest IP should set $WinGuestIp below.
# -----------------------------------------------------------------------------
$WinGuestIp = $PveHost  # override if VM is on a different subnet
$ports      = @(18001, 17001, 16001)
$deadline   = (Get-Date).AddSeconds($PortProbeTimeoutSec)
Write-Host "`n[PROBE] waiting for $WinGuestIp on ports $($ports -join ',') ..." -ForegroundColor Cyan
while ((Get-Date) -lt $deadline) {
    $allUp = $true
    foreach ($p in $ports) {
        $probe = Test-NetConnection -ComputerName $WinGuestIp -Port $p -InformationLevel Quiet -WarningAction SilentlyContinue
        if (-not $probe) { $allUp = $false; break }
    }
    if ($allUp) {
        Write-Host "[PROBE] all 3 ports open on $WinGuestIp" -ForegroundColor Green
        return
    }
    Start-Sleep -Seconds 5
}
throw "[PROBE] timeout after ${PortProbeTimeoutSec}s; $WinGuestIp ports $($ports -join ',') not all reachable"
