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
