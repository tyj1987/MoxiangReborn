# Proves deploy_client.ps1 does not read the repo-tree PlayDH copy unless
# MXH_PLAYDH_ROOT or -ResourceDir is given. Synthetic temp dirs only.
# A missing checkout tree is a failure, not a skip.

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$mxhMissing = 'MXH_PLAYDH_ROOT is not set and no --resource-root/-ResourceDir was given'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$repoPlay = (Join-Path (Join-Path (Join-Path $repoRoot 'modern') 'data') 'PlayDH')
$deploy = Join-Path (Join-Path $repoRoot 'deploy') (Join-Path 'scripts' 'deploy_client.ps1')
$deployServer = Join-Path (Join-Path $repoRoot 'deploy') (Join-Path 'scripts' 'deploy_server.ps1')
$pwsh = (Get-Process -Id $PID).Path
$savedPlay = $env:MXH_PLAYDH_ROOT
$passed = 0
$failed = 0

function Write-PlayDhRootCase {
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

function Restore-PlayDhRootEnv {
    if ($null -eq $savedPlay) {
        Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
    } else {
        $env:MXH_PLAYDH_ROOT = $savedPlay
    }
}

try {
    if (-not (Test-Path -LiteralPath $repoPlay -PathType Container)) {
        Write-Host "PLAYDH_ROOT_CASE checkout-missing FAIL full checkout required: $repoPlay"
        Write-Host 'PWSH SUMMARY passed=0 failed=1 out of 1'
        exit 1
    }
    if (-not (Test-Path -LiteralPath $deploy -PathType Leaf)) {
        Write-Host "PLAYDH_ROOT_CASE deploy-script-missing FAIL $deploy"
        Write-Host 'PWSH SUMMARY passed=0 failed=1 out of 1'
        exit 1
    }

    Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
    $unsetOut = & $pwsh -NoProfile -File $deploy -ResolveOnly 2>&1 | Out-String
    $unsetCode = $LASTEXITCODE
    $unsetOk = ($unsetCode -eq 1) -and ($unsetOut.Contains($mxhMissing)) -and (-not $unsetOut.Contains($repoPlay))
    if (-not $unsetOk) {
        Write-Host "unset exit=$unsetCode"
        Write-Host $unsetOut
    }
    Write-PlayDhRootCase -Name 'deploy-client-unset' -Ok $unsetOk

    $tempParent = if (-not [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
    $synthetic = Join-Path $tempParent ('mxh-deploy-playdh-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $synthetic | Out-Null
    try {
        [System.IO.File]::WriteAllText((Join-Path $synthetic 'synthetic.txt'), "synthetic`n")
        $env:MXH_PLAYDH_ROOT = $synthetic
        $synOut = & $pwsh -NoProfile -File $deploy -ResolveOnly 2>&1 | Out-String
        $synCode = $LASTEXITCODE
        $synFull = [System.IO.Path]::GetFullPath($synthetic)
        $repoFull = [System.IO.Path]::GetFullPath($repoPlay)
        $printed = ''
        foreach ($line in ($synOut -split "`r?`n")) {
            if ($line.StartsWith('RESOURCE_DIR=')) { $printed = $line.Substring('RESOURCE_DIR='.Length).Trim() }
        }
        $printedFull = ''
        if (-not [string]::IsNullOrWhiteSpace($printed)) {
            $printedFull = [System.IO.Path]::GetFullPath($printed)
        }
        $synOk = ($synCode -eq 0) -and ($printedFull -eq $synFull) -and ($printedFull -ne $repoFull)
        if (-not $synOk) {
            Write-Host "synthetic exit=$synCode printed=$printed"
            Write-Host $synOut
        }
        Write-PlayDhRootCase -Name 'deploy-client-synthetic' -Ok $synOk

        Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
        $serverUnset = & $pwsh -NoProfile -File $deployServer -ResolveOnly 2>&1 | Out-String
        $serverUnsetCode = $LASTEXITCODE
        $serverUnsetOk = ($serverUnsetCode -eq 1) -and ($serverUnset.Contains($mxhMissing)) -and (-not $serverUnset.Contains($repoPlay))
        if (-not $serverUnsetOk) {
            Write-Host "server unset exit=$serverUnsetCode"
            Write-Host $serverUnset
        }
        Write-PlayDhRootCase -Name 'deploy-server-unset' -Ok $serverUnsetOk

        $env:MXH_PLAYDH_ROOT = $synthetic
        $serverSyn = & $pwsh -NoProfile -File $deployServer -ResolveOnly 2>&1 | Out-String
        $serverSynCode = $LASTEXITCODE
        $serverPrinted = ''
        foreach ($line in ($serverSyn -split "`r?`n")) {
            if ($line.StartsWith('RESOURCE_DIR=')) { $serverPrinted = $line.Substring('RESOURCE_DIR='.Length).Trim() }
        }
        $serverPrintedFull = ''
        if (-not [string]::IsNullOrWhiteSpace($serverPrinted)) {
            $serverPrintedFull = [System.IO.Path]::GetFullPath($serverPrinted)
        }
        $serverSynOk = ($serverSynCode -eq 0) -and ($serverPrintedFull -eq $synFull) -and ($serverPrintedFull -ne $repoFull)
        if (-not $serverSynOk) {
            Write-Host "server synthetic exit=$serverSynCode printed=$serverPrinted"
            Write-Host $serverSyn
        }
        Write-PlayDhRootCase -Name 'deploy-server-synthetic' -Ok $serverSynOk
    } finally {
        if (Test-Path -LiteralPath $synthetic) {
            Remove-Item -LiteralPath $synthetic -Recurse -Force
        }
    }

    Remove-Item -Path Env:MXH_PLAYDH_ROOT -ErrorAction SilentlyContinue
    $pkgParent = Join-Path $tempParent ('mxh-pkg-' + [guid]::NewGuid().ToString('N'))
    $pkgSrc = Join-Path $pkgParent 'src'
    $pkgRes = Join-Path $pkgParent 'playdh'
    $pkgOut = Join-Path $pkgParent 'package'
    New-Item -ItemType Directory -Path $pkgSrc | Out-Null
    New-Item -ItemType Directory -Path $pkgRes | Out-Null
    New-Item -ItemType Directory -Path $pkgOut | Out-Null
    try {
        [System.IO.File]::WriteAllBytes((Join-Path $pkgSrc 'mxh_client.exe'), [Text.Encoding]::ASCII.GetBytes('synthetic-client'))
        [System.IO.File]::WriteAllText((Join-Path $pkgRes 'synthetic.txt'), "synthetic`n")
        $pkgOutText = & $pwsh -NoProfile -File $deploy -SyntheticPackage -SourceDir $pkgSrc -DeployDir $pkgOut -ResourceDir $pkgRes 2>&1 | Out-String
        $pkgCode = $LASTEXITCODE
        $batPath = Join-Path $pkgOut '启动游戏.bat'
        $staged = Join-Path (Join-Path (Join-Path $pkgOut 'data') 'PlayDH') 'synthetic.txt'
        $batAscii = ''
        if (Test-Path -LiteralPath $batPath) {
            $batAscii = [Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($batPath))
        }
        $needArg = '--resource-root "%~dp0data\PlayDH"'
        $startLine = ''
        foreach ($line in ($batAscii -split "`r?`n")) {
            if ($line.Contains('MoxianClient.exe')) { $startLine = $line.Trim() }
        }
        $pkgOk = ($pkgCode -eq 0) -and (Test-Path -LiteralPath $staged) -and $batAscii.Contains($needArg) -and (-not $batAscii.Contains($repoPlay))
        if (-not $pkgOk) {
            Write-Host "package exit=$pkgCode bat=$startLine"
            Write-Host $pkgOutText
        }
        Write-PlayDhRootCase -Name 'deploy-client-package-bat' -Ok $pkgOk
    } finally {
        if (Test-Path -LiteralPath $pkgParent) {
            Remove-Item -LiteralPath $pkgParent -Recurse -Force
        }
    }
} finally {
    Restore-PlayDhRootEnv
}

$total = $passed + $failed
Write-Host "PWSH SUMMARY passed=$passed failed=$failed out of $total"
if ($failed -ne 0) { exit 1 }
exit 0
