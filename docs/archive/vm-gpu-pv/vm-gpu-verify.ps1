<#
.SYNOPSIS
  VM-side GPU self-check -- after Hyper-V GPU-PV (DDA / GPU-PV) is configured on the host.
.DESCRIPTION
  Inspects VM internal display devices, DirectX feature level, vendor SMI tools.
  Outputs JSON report to modern/docs/restoration-plan/gpu-pv-report.json.
  visual-smoke.ps1 reads this report to decide whether to use WARP or physical GPU.

  Console output is pure ASCII (PowerShell 5.1 ISE cannot decode UTF-8 without BOM).
  All Chinese context lives in the JSON report (UTF-8 with BOM signature).
.PARAMETER Output
  Report output path (default: modern\docs\restoration-plan\gpu-pv-report.json)
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\vm-gpu-verify.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = ''
)
$ErrorActionPreference = 'Continue'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = Join-Path $repoRoot 'modern\docs\restoration-plan\gpu-pv-report.json'
}

function Get-GpuSummary {
    $gpus = @()
    try { $gpus = @(Get-CimInstance -ClassName 'Win32_VideoController' -ErrorAction SilentlyContinue) } catch {}
    if ($gpus.Count -eq 0) {
        try { $gpus = @(Get-WmiObject -Class 'Win32_VideoController' -ErrorAction SilentlyContinue) } catch {}
    }
    $out = @()
    foreach ($g in $gpus) {
        $out += [ordered]@{
            name              = [string]$g.Name
            compatibility     = [string]$g.AdapterCompatibility
            video_mode        = [string]$g.VideoModeDescription
            driver_version    = [string]$g.DriverVersion
            driver_date       = [string]$g.DriverDate
            processor         = [string]$g.VideoProcessor
            pnp_class         = 'Display'
        }
    }
    return $out
}

function Get-PnpGpu {
    $pnps = @()
    try { $pnps = @(Get-PnpDevice -Class Display -ErrorAction SilentlyContinue) } catch {}
    $out = @()
    foreach ($p in $pnps) {
        $isPhysical = $false
        $iid = [string]$p.InstanceId
        if ($iid -match '^PCI\\') { $isPhysical = $true }
        elseif ($iid -match 'VMBUS|HYPER|RDP|REMOTEDISPLAY') { $isPhysical = $false }
        $out += [ordered]@{
            friendly_name    = [string]$p.FriendlyName
            instance_id      = $iid
            status           = [string]$p.Status
            problem          = [int]$p.Problem
            is_physical_pci  = $isPhysical
        }
    }
    return $out
}

function Get-DirectX {
    $dx = $null
    try { $dx = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\DirectX' -ErrorAction SilentlyContinue } catch {}
    return [ordered]@{
        version                = [string]$dx.Version
        max_feature_level      = [int]$dx.MaxFeatureLevel
        d3d12_max_feature_level = [int]$dx.D3D12MaxFeatureLevel
        dx_db_version          = [string]$dx.DxDbVersion
    }
}

function Get-VendorSmi {
    $nvidiaPath = 'C:\Windows\System32\nvidia-smi.exe'
    $amdPath    = 'C:\Windows\System32\AMD Adrenalin\amdxc64.dll'
    $out = [ordered]@{
        nvidia_smi        = (Test-Path -LiteralPath $nvidiaPath)
        amd_adrenalin     = (Test-Path -LiteralPath $amdPath -ErrorAction SilentlyContinue)
        nvidia_smi_output = ''
    }
    if ($out.nvidia_smi) {
        try {
            $out.nvidia_smi_output = (& nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv 2>&1 | Out-String).Trim()
        } catch {
            $out.nvidia_smi_output = 'ERR: ' + $_.Exception.Message
        }
    }
    return $out
}

function Judge-Pass($gpus, $pnpGpus) {
    $hasPhysical = $false
    foreach ($p in $pnpGpus) {
        if ($p.is_physical_pci -and $p.status -eq 'OK') { $hasPhysical = $true; break }
    }
    $hasWarp = $false
    foreach ($p in $pnpGpus) {
        if ($p.instance_id -match 'VMBUS|HYPER' -and $p.friendly_name -match 'Hyper-V') { $hasWarp = $true; break }
    }
    if ($hasPhysical -and -not $hasWarp) {
        return [ordered]@{
            verdict = 'PHYSICAL_GPU'
            message_zh = 'VM has PCI physical GPU and no Hyper-V soft display overlay -- GPU-PV configured'
            message_en = 'VM has PCI physical GPU and no Hyper-V soft display overlay -- GPU-PV configured'
        }
    } elseif ($hasPhysical -and $hasWarp) {
        return [ordered]@{
            verdict = 'MIXED'
            message_zh = 'VM has both physical GPU and Hyper-V soft display; main display may still be soft -- verify RDP path'
            message_en = 'VM has both physical GPU and Hyper-V soft display; main display may still be soft -- verify RDP path'
        }
    } elseif (-not $hasPhysical -and $hasWarp) {
        return [ordered]@{
            verdict = 'WARP_ONLY'
            message_zh = 'VM has only Hyper-V soft display WARP -- GPU-PV NOT configured. M-R5/M-R7/M-R4 physical GPU screenshot still blocked'
            message_en = 'VM has only Hyper-V soft display WARP -- GPU-PV NOT configured. M-R5/M-R7/M-R4 physical GPU screenshot still blocked'
        }
    } else {
        return [ordered]@{
            verdict = 'UNKNOWN'
            message_zh = 'VM has no display devices (rare)'
            message_en = 'VM has no display devices (rare)'
        }
    }
}

$report = [ordered]@{
    generated_at   = (Get-Date).ToString('o')
    hostname       = [string]$env:COMPUTERNAME
    username       = [string]$env:USERNAME
    gpus           = (Get-GpuSummary)
    pnp_displays   = (Get-PnpGpu)
    directx        = (Get-DirectX)
    vendor_smi     = (Get-VendorSmi)
}

$verdict = Judge-Pass $report.gpus $report.pnp_displays
$report.verdict = $verdict.verdict
$report.message_zh = $verdict.message_zh
$report.message_en = $verdict.message_en

# JSON output (UTF-8 with BOM, readable in any text editor)
$json = $report | ConvertTo-Json -Depth 6
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output) | Out-Null
$utf8Bom = New-Object System.Text.UTF8Encoding $true
[System.IO.File]::WriteAllText($Output, $json, $utf8Bom)

# Console: pure ASCII (PowerShell 5.1 ISE cannot decode UTF-8 without BOM)
Write-Host -Object '===========================================' -ForegroundColor Cyan
Write-Host -Object ' VM GPU Self-Check Report' -ForegroundColor Cyan
Write-Host -Object '===========================================' -ForegroundColor Cyan
Write-Host -Object ("Hostname:    " + $report.hostname)
Write-Host -Object ("Username:    " + $report.username)
Write-Host -Object ("Generated:   " + $report.generated_at)
Write-Host -Object ''
Write-Host -Object '-- DirectX --' -ForegroundColor Yellow
Write-Host -Object ("  Version:           " + $report.directx.version)
Write-Host -Object ("  Max FL:            " + $report.directx.max_feature_level)
Write-Host -Object ("  D3D12 Max FL:      " + $report.directx.d3d12_max_feature_level)
Write-Host -Object ''
Write-Host -Object '-- Display Adapters Win32 VideoController --' -ForegroundColor Yellow
if ($report.gpus.Count -eq 0) {
    Write-Host -Object '  [none]' -ForegroundColor Red
} else {
    foreach ($g in $report.gpus) {
        Write-Host -Object ("  - " + $g.name)
        Write-Host -Object ("    compat:    " + $g.compatibility)
        Write-Host -Object ("    mode:      " + $g.video_mode)
        Write-Host -Object ("    driver:    " + $g.driver_version)
    }
}
Write-Host -Object ''
Write-Host -Object '-- PnP Display Devices --' -ForegroundColor Yellow
foreach ($p in $report.pnp_displays) {
    $tag = if ($p.is_physical_pci) { '[PCI phys]' } else { '[soft]   ' }
    $clr = if ($p.is_physical_pci) { 'Green' } else { 'DarkGray' }
    Write-Host -Object ("  " + $tag + " " + $p.friendly_name) -ForegroundColor $clr
    Write-Host -Object ("    instance: " + $p.instance_id) -ForegroundColor DarkGray
    Write-Host -Object ("    status:   " + $p.status)
}
Write-Host -Object ''
Write-Host -Object '-- Vendor SMI --' -ForegroundColor Yellow
if ($report.vendor_smi.nvidia_smi) {
    Write-Host -Object '  nvidia-smi: present OK' -ForegroundColor Green
    Write-Host -Object ("  " + $report.vendor_smi.nvidia_smi_output)
} else {
    Write-Host -Object '  nvidia-smi: not present' -ForegroundColor DarkGray
}
if ($report.vendor_smi.amd_adrenalin) {
    Write-Host -Object '  AMD Adrenalin: present OK' -ForegroundColor Green
} else {
    Write-Host -Object '  AMD Adrenalin: not present' -ForegroundColor DarkGray
}
Write-Host -Object ''
Write-Host -Object '-- Verdict --' -ForegroundColor Yellow
$verdictColor = switch ($report.verdict) {
    'PHYSICAL_GPU' { 'Green' }
    'MIXED'        { 'Yellow' }
    'WARP_ONLY'    { 'Red' }
    default        { 'Red' }
}
Write-Host -Object ("  Verdict:    " + $report.verdict) -ForegroundColor $verdictColor
Write-Host -Object ("  Message:    " + $report.message_en)
Write-Host -Object ''
Write-Host -Object ("  JSON report: " + $Output) -ForegroundColor Cyan
Write-Host -Object "  See 'message_zh' in JSON for Chinese context." -ForegroundColor DarkGray
