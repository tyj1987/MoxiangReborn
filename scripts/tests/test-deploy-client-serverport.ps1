# Builds a client package with deploy_client.ps1 -SyntheticPackage from a
# synthetic profile in the system temp dir. Every generated port setting
# must equal the modern login port, and legacy port 9000 must not appear
# in the package configs. Does not listen or connect. Does not loosen the
# profile hash gate (that path is not used by -SyntheticPackage).
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-deploy-client-serverport.ps1

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
}

function Get-ParamDefault {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name
    )
    $tokens = $null
    $parseErrors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$parseErrors)
    if ($parseErrors -and @($parseErrors).Count -gt 0) {
        throw "FAIL: parse error in ${Path}: $($parseErrors[0])"
    }
    if ($null -eq $ast.ParamBlock) { return $null }
    foreach ($parameter in @($ast.ParamBlock.Parameters)) {
        if ([string]$parameter.Name.VariablePath.UserPath -ne $Name) { continue }
        $default = $parameter.DefaultValue
        if ($null -eq $default) { return $null }
        if ($default -is [System.Management.Automation.Language.ConstantExpressionAst]) {
            return $default.Value
        }
        throw "FAIL: $Name default in $Path is not a constant ($($default.Extent.Text))"
    }
    return $null
}

function Invoke-SyntheticClientPackage {
    param(
        [Parameter(Mandatory)][string]$Deploy,
        [Parameter(Mandatory)][string]$SourceDir,
        [Parameter(Mandatory)][string]$DeployDir,
        [Parameter(Mandatory)][string]$ResourceDir,
        [Parameter(Mandatory)][string]$ServerIP,
        [int]$LoginPort = 0,
        [switch]$PassLoginPort
    )
    $pwsh = (Get-Process -Id $PID).Path
    $argumentList = @(
        '-NoProfile', '-File', $Deploy,
        '-SyntheticPackage',
        '-SourceDir', $SourceDir,
        '-DeployDir', $DeployDir,
        '-ResourceDir', $ResourceDir,
        '-ServerIP', $ServerIP
    )
    if ($PassLoginPort) {
        $argumentList += @('-LoginPort', ([string]$LoginPort))
    }
    $output = & $pwsh @argumentList 2>&1 | Out-String
    return [pscustomobject]@{ Code = $LASTEXITCODE; Output = $output }
}

function Assert-PackagePorts {
    param(
        [Parameter(Mandatory)][string]$PackageDir,
        [Parameter(Mandatory)][string]$ServerIP,
        [Parameter(Mandatory)][int]$ExpectedPort
    )
    $files = @(Get-ChildItem -LiteralPath $PackageDir -Recurse -File -Force)
    Assert-True ($files.Count -gt 0) "package is empty: $PackageDir"
    $all = New-Object System.Text.StringBuilder
    foreach ($file in $files) {
        $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
        $text = [System.Text.Encoding]::ASCII.GetString($bytes)
        [void]$all.AppendLine($text)
        $relative = $file.FullName.Substring($PackageDir.Length).TrimStart('\', '/')
        $legacy = [regex]::Matches($text, '(?<![0-9A-Fa-f])9000(?![0-9A-Fa-f])')
        if ($legacy.Count -ne 0) {
            throw "FAIL: legacy port 9000 in package config $relative (expected $ExpectedPort)"
        }
        if ($text.Contains('192.168.2.244')) {
            throw "FAIL: hardcoded address in package config $relative"
        }
    }
    $body = $all.ToString()
    Assert-True ($body.Contains("IP=$ServerIP")) "package host IP= was not $ServerIP"
    Assert-True ($body.Contains("Server1_IP=$ServerIP")) "package host Server1_IP= was not $ServerIP"
    Assert-True ($body.Contains("ServerIP=$ServerIP")) "package host ServerIP= was not $ServerIP"
    Assert-True ($body.Contains('synthetic-profile')) 'synthetic profile was not staged into the package'
    $ports = @([regex]::Matches($body, '(?m)^(?<key>[A-Za-z0-9_]*Port)=(?<value>\d+)\s*$'))
    Assert-True ($ports.Count -ge 3) "package has $($ports.Count) port settings, expected Port, Server1_Port, and ServerPort"
    $seen = @{}
    foreach ($match in $ports) {
        $key = [string]$match.Groups['key'].Value
        $value = [string]$match.Groups['value'].Value
        $seen[$key] = $value
        if ($value -ne ([string]$ExpectedPort)) {
            throw "FAIL: package $key=$value, expected modern login port $ExpectedPort"
        }
    }
    foreach ($key in @('Port', 'Server1_Port', 'ServerPort')) {
        Assert-True ($seen.ContainsKey($key)) "package is missing $key"
        Assert-True ($seen[$key] -eq ([string]$ExpectedPort)) "$key=$($seen[$key]), expected $ExpectedPort"
    }
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$deploy = Join-Path $repoRoot 'deploy\scripts\deploy_client.ps1'
$startModern = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
Assert-True (Test-Path -LiteralPath $deploy -PathType Leaf) "missing $deploy"
Assert-True (Test-Path -LiteralPath $startModern -PathType Leaf) "missing $startModern"

$deployText = [System.IO.File]::ReadAllText($deploy)
Assert-True ($deployText.Contains('Profile hash mismatch')) 'profile hash gate must stay in deploy_client.ps1'
Assert-True ($deployText.Contains('Get-FileHash -Algorithm SHA256')) 'profile hash verification must stay in deploy_client.ps1'
Assert-True ($deployText.Contains('SYNTHETIC_PACKAGE -ResourceDir must not contain a .pak file')) 'SyntheticPackage pak rejection must stay'

$loginDefault = Get-ParamDefault -Path $startModern -Name 'LoginPort'
$agentDefault = Get-ParamDefault -Path $startModern -Name 'AgentPort'
$mapDefault = Get-ParamDefault -Path $startModern -Name 'MapPort'
Assert-True ($loginDefault -eq 16001) "start_modern.ps1 LoginPort default is '$loginDefault'"
Assert-True ($agentDefault -eq 17001) "start_modern.ps1 AgentPort default is '$agentDefault'"
Assert-True ($mapDefault -eq 18001) "start_modern.ps1 MapPort default is '$mapDefault'"

$serverIP = 'login.example.test'
$explicitPort = 16011
$sysTemp = [System.IO.Path]::GetTempPath()
$root = Join-Path $sysTemp ('mxh-client-port-' + [guid]::NewGuid().ToString('N'))
$source = Join-Path $root 'src'
$profile = Join-Path $root 'profile'
$defaultPackage = Join-Path $root 'package-default'
$explicitPackage = Join-Path $root 'package-explicit'
New-Item -ItemType Directory -Force -Path $source | Out-Null
New-Item -ItemType Directory -Force -Path $profile | Out-Null
New-Item -ItemType Directory -Force -Path $defaultPackage | Out-Null
New-Item -ItemType Directory -Force -Path $explicitPackage | Out-Null
[System.IO.File]::WriteAllBytes((Join-Path $source 'mxh_client.exe'), [Text.Encoding]::ASCII.GetBytes('synthetic-client'))
[System.IO.File]::WriteAllText((Join-Path $profile 'synthetic.txt'), "synthetic-profile`n")

try {
    $defaultRun = Invoke-SyntheticClientPackage -Deploy $deploy -SourceDir $source -DeployDir $defaultPackage -ResourceDir $profile -ServerIP $serverIP
    if ($defaultRun.Code -ne 0) {
        Write-Host $defaultRun.Output
        throw "FAIL: default synthetic package exit=$($defaultRun.Code)"
    }
    Assert-PackagePorts -PackageDir $defaultPackage -ServerIP $serverIP -ExpectedPort ([int]$loginDefault)
    Write-Output "SERVERPORT_CASE default PASS port=$loginDefault"

    $explicitRun = Invoke-SyntheticClientPackage -Deploy $deploy -SourceDir $source -DeployDir $explicitPackage -ResourceDir $profile -ServerIP $serverIP -PassLoginPort -LoginPort $explicitPort
    if ($explicitRun.Code -ne 0) {
        Write-Host $explicitRun.Output
        throw "FAIL: explicit synthetic package exit=$($explicitRun.Code)"
    }
    Assert-PackagePorts -PackageDir $explicitPackage -ServerIP $serverIP -ExpectedPort $explicitPort
    Write-Output "SERVERPORT_CASE explicit PASS port=$explicitPort"

    $packagedDefault = Get-ParamDefault -Path $deploy -Name 'LoginPort'
    Assert-True ($packagedDefault -eq $loginDefault) "deploy_client.ps1 LoginPort default is '$packagedDefault', modern login port is $loginDefault"
    Write-Output "SERVERPORT_CASE parameter PASS port=$packagedDefault"
} finally {
    if (Test-Path -LiteralPath $root) {
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}

Write-Output "SERVERPORT PASS login=$loginDefault"
