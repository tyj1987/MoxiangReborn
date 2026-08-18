#!/usr/bin/env powershell
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',
    [string[]]$Target = @()
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

# Some desktop hosts inject both Path and PATH. MSBuild's environment copy is
# case-insensitive and CL.exe then fails with MSB6001 before compilation.
$effectivePath = [Environment]::GetEnvironmentVariable('Path', 'Process')
Remove-Item -LiteralPath Env:Path -ErrorAction SilentlyContinue
Remove-Item -LiteralPath Env:PATH -ErrorAction SilentlyContinue
[Environment]::SetEnvironmentVariable('Path', $effectivePath, 'Process')

# Auto-configure modern/build if it does not exist (clone after a clean
# pull lands here, before setup-modern.ps1 has been run).  This keeps
# `scripts/build-modern.ps1 -Config Debug` runnable as the first action
# without forcing the user to run setup-modern first.
$buildDir = Join-Path $repoRoot 'modern/build'
if (-not (Test-Path -LiteralPath $buildDir)) {
    Write-Host "[build-modern] modern/build missing; running cmake configure first..." -ForegroundColor Yellow
    Push-Location -LiteralPath $repoRoot
    try {
        & cmake -S modern -B $buildDir -G 'Visual Studio 17 2022' -A Win32
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed (exit $LASTEXITCODE)"
        }
    } finally {
        Pop-Location
    }
}

$arguments = @('--build', 'modern/build', '--config', $Config)
if ($Target.Count -gt 0) {
    $arguments += '--target'
    $arguments += @($Target | ForEach-Object { $_ -split ',' })
}

Push-Location -LiteralPath $repoRoot
try {
    & cmake @arguments
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
