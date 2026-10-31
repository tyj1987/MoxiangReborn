# Proves the remaining PlayDH entry points do not read the repo tree unless
# MXH_PLAYDH_ROOT or an explicit root is given. Synthetic temp dirs only.
# A missing checkout tree is a failure, not a skip. No rog label: these
# cases do not need the ROG host.

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$mxhMissing = 'MXH_PLAYDH_ROOT is not set and no --resource-root was given'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$repoPlay = Join-Path (Join-Path (Join-Path $repoRoot 'modern') 'data') 'PlayDH'
$pwsh = (Get-Process -Id $PID).Path
$savedPlay = $env:MXH_PLAYDH_ROOT
$passed = 0
$failed = 0

function Write-PlayDhCase {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][bool]$Ok
    )
    if ($Ok) {
        $script:passed += 1
        Write-Host "PLAYDH_ROOT_CASE $Name PASS"
    } else {
        $script:failed += 1
        Write-Host "PLAYDH_ROOT_CASE $Name FAIL"
    }
}

function Restore-PlayDhEnv {
    if ($null -eq $script:savedPlay) {
        Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
    } else {
        $env:MXH_PLAYDH_ROOT = $script:savedPlay
    }
}

function Clear-PlayDhEnv {
    Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
}

function Test-MentionsRepoPlayDh {
    param([AllowEmptyString()][string]$Text)
    if ([string]::IsNullOrEmpty($Text)) { return $false }
    $full = [IO.Path]::GetFullPath($repoPlay)
    if ($Text.IndexOf($full, [StringComparison]::OrdinalIgnoreCase) -ge 0) { return $true }
    return $Text -match 'modern[/\\]data[/\\]PlayDH'
}

function Invoke-PlayDhScript {
    param(
        [Parameter(Mandatory)][string]$File,
        [string[]]$Arguments
    )
    $output = & $pwsh -NoProfile -File $File @Arguments 2>&1 | Out-String
    return [pscustomobject]@{
        Code = $LASTEXITCODE
        Text = $output
    }
}

function Get-PythonCommand {
    # Symlinks are not -PathType Leaf on every host. The Alpine CI image
    # publishes python3 as a symlink under /usr/local/bin.
    $known = @(
        '/usr/local/bin/python3',
        '/usr/bin/python3',
        '/usr/local/bin/python',
        '/usr/bin/python'
    )
    foreach ($candidate in $known) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    foreach ($dir in ($env:PATH -split [IO.Path]::PathSeparator)) {
        if ([string]::IsNullOrWhiteSpace($dir)) { continue }
        foreach ($name in @('python3', 'python')) {
            $candidate = Join-Path $dir $name
            if (Test-Path -LiteralPath $candidate) { return $candidate }
        }
    }
    return $null
}

$tempParent = if (-not [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) { $env:RUNNER_TEMP } else { [IO.Path]::GetTempPath() }
$tempRoot = Join-Path $tempParent ('mxh-playdh-remaining-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempRoot | Out-Null

try {
    if (-not (Test-Path -LiteralPath $repoPlay -PathType Container)) {
        Write-Host "PLAYDH_ROOT_CASE checkout-missing FAIL full checkout required: $repoPlay"
        Write-Host 'PWSH SUMMARY passed=0 failed=1 out of 1'
        exit 1
    }
    $synthetic = Join-Path $tempRoot 'synthetic-playdh'
    New-Item -ItemType Directory -Path $synthetic | Out-Null
    [IO.File]::WriteAllText((Join-Path $synthetic 'marker.txt'), "synthetic`n")
    $syntheticFull = [IO.Path]::GetFullPath($synthetic)

    $helper = Join-Path $repoRoot 'scripts\gui-client-smoke-ephemeral.ps1'
    . $helper
    Clear-PlayDhEnv
    $envMessage = $null
    try {
        $env:MXH_PLAYDH_ROOT = $syntheticFull
        $fromEnv = Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot ''
    } catch {
        $envMessage = $_.Exception.Message
        $fromEnv = $null
    }
    $envOk = ($null -eq $envMessage) -and ($fromEnv -eq $syntheticFull) -and ($fromEnv -ne ([IO.Path]::GetFullPath($repoPlay)))
    if (-not $envOk) {
        Write-Host "gui-env resolved='$fromEnv' error='$envMessage'"
    }
    Write-PlayDhCase -Name 'gui-env-synthetic' -Ok $envOk
    Clear-PlayDhEnv

    $smoke = Join-Path $repoRoot 'scripts\gui-client-smoke.ps1'
    $unsetSmoke = Invoke-PlayDhScript -File $smoke -Arguments @('-ResolveOnly')
    $unsetSmokeOk = ($unsetSmoke.Code -eq 1) -and ($unsetSmoke.Text.Contains($mxhMissing)) -and (-not (Test-MentionsRepoPlayDh $unsetSmoke.Text))
    if (-not $unsetSmokeOk) {
        Write-Host "gui-unset exit=$($unsetSmoke.Code)"
        Write-Host $unsetSmoke.Text
    }
    Write-PlayDhCase -Name 'gui-unset' -Ok $unsetSmokeOk

    $synSmoke = Invoke-PlayDhScript -File $smoke -Arguments @('-ResolveOnly', '-ResourceRoot', $syntheticFull)
    $printed = ''
    foreach ($line in ($synSmoke.Text -split "`r?`n")) {
        if ($line.StartsWith('resource_root=')) { $printed = $line.Substring('resource_root='.Length).Trim() }
    }
    $synSmokeOk = ($synSmoke.Code -eq 0) -and ($printed -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $synSmoke.Text))
    if (-not $synSmokeOk) {
        Write-Host "gui-explicit exit=$($synSmoke.Code) printed='$printed'"
        Write-Host $synSmoke.Text
    }
    Write-PlayDhCase -Name 'gui-explicit-synthetic' -Ok $synSmokeOk

    $frames = Join-Path $repoRoot 'scripts\capture-state-frames.ps1'
    $framesDir = Join-Path $tempRoot 'frames'
    New-Item -ItemType Directory -Path $framesDir | Out-Null
    $frameArgs = @('-AccountName', 'synthetic', '-Password', 'synthetic', '-FramesDir', $framesDir)
    $unsetFrames = Invoke-PlayDhScript -File $frames -Arguments ($frameArgs + @('-ResolveOnly'))
    $unsetFramesOk = ($unsetFrames.Code -eq 1) -and ($unsetFrames.Text.Contains($mxhMissing)) -and (-not (Test-MentionsRepoPlayDh $unsetFrames.Text))
    if (-not $unsetFramesOk) {
        Write-Host "capture-unset exit=$($unsetFrames.Code)"
        Write-Host $unsetFrames.Text
    }
    Write-PlayDhCase -Name 'capture-unset' -Ok $unsetFramesOk

    $synFrames = Invoke-PlayDhScript -File $frames -Arguments ($frameArgs + @('-ResolveOnly', '-ResourceRoot', $syntheticFull))
    $framePrinted = ''
    foreach ($line in ($synFrames.Text -split "`r?`n")) {
        if ($line.StartsWith('resource_root=')) { $framePrinted = $line.Substring('resource_root='.Length).Trim() }
    }
    $synFramesOk = ($synFrames.Code -eq 0) -and ($framePrinted -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $synFrames.Text))
    if (-not $synFramesOk) {
        Write-Host "capture-explicit exit=$($synFrames.Code) printed='$framePrinted'"
        Write-Host $synFrames.Text
    }
    Write-PlayDhCase -Name 'capture-explicit-synthetic' -Ok $synFramesOk

    $env:MXH_PLAYDH_ROOT = $syntheticFull
    $envFrames = Invoke-PlayDhScript -File $frames -Arguments ($frameArgs + @('-ResolveOnly'))
    $envPrinted = ''
    foreach ($line in ($envFrames.Text -split "`r?`n")) {
        if ($line.StartsWith('resource_root=')) { $envPrinted = $line.Substring('resource_root='.Length).Trim() }
    }
    $envFramesOk = ($envFrames.Code -eq 0) -and ($envPrinted -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $envFrames.Text))
    if (-not $envFramesOk) {
        Write-Host "capture-env exit=$($envFrames.Code) printed='$envPrinted'"
        Write-Host $envFrames.Text
    }
    Write-PlayDhCase -Name 'capture-env-synthetic' -Ok $envFramesOk
    Clear-PlayDhEnv

    $visual = Join-Path $repoRoot 'scripts\visual-smoke.ps1'
    $unsetVisual = Invoke-PlayDhScript -File $visual -Arguments @('-ResolveOnly')
    $unsetVisualOk = ($unsetVisual.Code -eq 1) -and ($unsetVisual.Text.Contains($mxhMissing)) -and (-not (Test-MentionsRepoPlayDh $unsetVisual.Text))
    if (-not $unsetVisualOk) {
        Write-Host "visual-unset exit=$($unsetVisual.Code)"
        Write-Host $unsetVisual.Text
    }
    Write-PlayDhCase -Name 'visual-unset' -Ok $unsetVisualOk

    $synVisual = Invoke-PlayDhScript -File $visual -Arguments @('-ResolveOnly', '-ResourceRoot', $syntheticFull)
    $visualPrinted = ''
    foreach ($line in ($synVisual.Text -split "`r?`n")) {
        if ($line.StartsWith('resource_root=')) { $visualPrinted = $line.Substring('resource_root='.Length).Trim() }
    }
    $synVisualOk = ($synVisual.Code -eq 0) -and ($visualPrinted -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $synVisual.Text))
    if (-not $synVisualOk) {
        Write-Host "visual-explicit exit=$($synVisual.Code) printed='$visualPrinted'"
        Write-Host $synVisual.Text
    }
    Write-PlayDhCase -Name 'visual-explicit-synthetic' -Ok $synVisualOk

    $human = Join-Path $repoRoot 'scripts\run-human-acceptance.ps1'
    $unsetHuman = Invoke-PlayDhScript -File $human -Arguments @('-PlanOnly')
    $unsetHumanOk = ($unsetHuman.Code -eq 1) -and ($unsetHuman.Text.Contains($mxhMissing)) -and (-not (Test-MentionsRepoPlayDh $unsetHuman.Text))
    if (-not $unsetHumanOk) {
        Write-Host "human-unset exit=$($unsetHuman.Code)"
        Write-Host $unsetHuman.Text
    }
    Write-PlayDhCase -Name 'human-unset' -Ok $unsetHumanOk

    $synHuman = Invoke-PlayDhScript -File $human -Arguments @('-PlanOnly', '-ResourceRoot', $syntheticFull)
    $synHumanOk = ($synHuman.Code -eq 0) -and ($synHuman.Text.Contains($syntheticFull)) -and (-not (Test-MentionsRepoPlayDh $synHuman.Text))
    if (-not $synHumanOk) {
        Write-Host "human-explicit exit=$($synHuman.Code)"
        Write-Host $synHuman.Text
    }
    Write-PlayDhCase -Name 'human-explicit-synthetic' -Ok $synHumanOk

    $gpu = Join-Path $repoRoot 'scripts\test-gpu-smoke.ps1'
    $unsetGpu = Invoke-PlayDhScript -File $gpu -Arguments @('-ResolveOnly')
    $unsetGpuOk = ($unsetGpu.Code -eq 1) -and ($unsetGpu.Text.Contains($mxhMissing)) -and (-not (Test-MentionsRepoPlayDh $unsetGpu.Text))
    if (-not $unsetGpuOk) {
        Write-Host "gpu-unset exit=$($unsetGpu.Code)"
        Write-Host $unsetGpu.Text
    }
    Write-PlayDhCase -Name 'gpu-unset' -Ok $unsetGpuOk

    $synGpu = Invoke-PlayDhScript -File $gpu -Arguments @('-ResolveOnly', '-ResourceRoot', $syntheticFull)
    $gpuPrinted = ''
    foreach ($line in ($synGpu.Text -split "`r?`n")) {
        if ($line.StartsWith('resource_root=')) { $gpuPrinted = $line.Substring('resource_root='.Length).Trim() }
    }
    $synGpuOk = ($synGpu.Code -eq 0) -and ($gpuPrinted -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $synGpu.Text))
    if (-not $synGpuOk) {
        Write-Host "gpu-explicit exit=$($synGpu.Code) printed='$gpuPrinted'"
        Write-Host $synGpu.Text
    }
    Write-PlayDhCase -Name 'gpu-explicit-synthetic' -Ok $synGpuOk

    $python = Get-PythonCommand
    $audit = Join-Path (Join-Path (Join-Path $repoRoot 'modern') 'tools') 'audit_minimap.py'
    if ([string]::IsNullOrWhiteSpace($python) -or -not (Test-Path -LiteralPath $audit -PathType Leaf)) {
        Write-PlayDhCase -Name 'audit-python-missing' -Ok $false
    } else {
        Push-Location -LiteralPath $tempRoot
        try {
            Clear-PlayDhEnv
            $auditUnset = & $python $audit --resolve-only 2>&1 | Out-String
            $auditUnsetCode = $LASTEXITCODE
            $auditUnsetOk = ($auditUnsetCode -eq 1) -and ($auditUnset.Contains('MXH_PLAYDH_ROOT')) -and ($auditUnset.Contains('--playdh')) -and ($auditUnset.Contains('--resource-root')) -and (-not (Test-MentionsRepoPlayDh $auditUnset))
            if (-not $auditUnsetOk) {
                Write-Host "audit-unset exit=$auditUnsetCode"
                Write-Host $auditUnset
            }
            Write-PlayDhCase -Name 'audit-unset' -Ok $auditUnsetOk

            $auditSyn = & $python $audit --resolve-only --playdh $syntheticFull 2>&1 | Out-String
            $auditSynCode = $LASTEXITCODE
            $auditPrinted = ''
            foreach ($line in ($auditSyn -split "`r?`n")) {
                if ($line.StartsWith('resource_root=')) { $auditPrinted = $line.Substring('resource_root='.Length).Trim() }
            }
            $auditSynOk = ($auditSynCode -eq 0) -and ($auditPrinted -eq $syntheticFull) -and (-not (Test-MentionsRepoPlayDh $auditSyn))
            if (-not $auditSynOk) {
                Write-Host "audit-explicit exit=$auditSynCode printed='$auditPrinted'"
                Write-Host $auditSyn
            }
            Write-PlayDhCase -Name 'audit-explicit-synthetic' -Ok $auditSynOk
        } finally {
            Pop-Location
        }
    }
} finally {
    Restore-PlayDhEnv
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

$total = $passed + $failed
Write-Host "PWSH SUMMARY passed=$passed failed=$failed out of $total"
if ($failed -ne 0) { exit 1 }
exit 0
