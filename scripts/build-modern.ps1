#!/usr/bin/env powershell
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',
    [string[]]$Target = @()
)

$ErrorActionPreference = 'Stop'
$buildScript = Join-Path $PSScriptRoot 'build-modern.bat'
$targets = @($Target | ForEach-Object { $_ -split ',' } | Where-Object { $_ })

& $buildScript $Config @targets
exit $LASTEXITCODE
