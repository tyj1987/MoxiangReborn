# Synthetic ROG port checks for scripts/soak-24h.ps1.
# No listeners, no servers, no network. Run:
#   pwsh -NoProfile -File scripts/tests/test-forbidden-rog-ports.ps1
$ErrorActionPreference = 'Stop'

$version = $PSVersionTable.PSVersion
$versionOk = $false
if ($version.Major -gt 7) { $versionOk = $true }
if ($version.Major -eq 7 -and $version.Minor -ge 4) { $versionOk = $true }
if (-not $versionOk) { throw ('FAIL: requires pwsh 7.4+, found ' + $version) }

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$soak = Join-Path $repoRoot 'scripts\soak-24h.ps1'
if (-not [IO.File]::Exists($soak)) { throw ('FAIL: missing ' + $soak) }

$pwsh = (Get-Process -Id $PID).Path
$utf8 = New-Object System.Text.UTF8Encoding $false

function Invoke-PortChild {
    param([Parameter(Mandatory)][string]$Body)
    $path = Join-Path ([IO.Path]::GetTempPath()) ('mxh-rog-port-' + [guid]::NewGuid().ToString('N') + '.ps1')
    [IO.File]::WriteAllText($path, $Body, $utf8)
    try {
        $output = & $pwsh -NoProfile -File $path 2>&1
        return [pscustomobject]@{
            Exit = [int]$LASTEXITCODE
            Text = (@($output) | ForEach-Object { [string]$_ }) -join "`n"
        }
    } finally {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}

function New-DotSourceHeader {
    $literal = $soak.Replace("'", "''")
    return @"
`$ErrorActionPreference = 'Stop'
. '$literal'
"@
}

$failed = $false
$summaries = New-Object System.Collections.Generic.List[string]

function Add-Case {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Body,
        [Parameter(Mandatory)][int]$Expect,
        [string]$RequiredText = ''
    )
    $result = Invoke-PortChild -Body $Body
    $line = 'PWSH SUMMARY ' + $Name + ' exit=' + $result.Exit
    $summaries.Add($line)
    Write-Output $line
    $ok = ($result.Exit -eq $Expect)
    if ($ok -and -not [string]::IsNullOrEmpty($RequiredText)) {
        $ok = $result.Text.Contains($RequiredText)
    }
    if (-not $ok) {
        $script:failed = $true
        Write-Output ('FAIL: ' + $Name + ' exited ' + $result.Exit + ' want ' + $Expect)
        if (-not [string]::IsNullOrEmpty($RequiredText) -and -not $result.Text.Contains($RequiredText)) {
            Write-Output ('FAIL: ' + $Name + ' output missing [' + $RequiredText + ']')
        }
        if (-not [string]::IsNullOrEmpty($result.Text)) { Write-Output $result.Text }
    }
}

$header = New-DotSourceHeader

$forbiddenCases = @(
    @{ Name = 'port=16001'; Login = 16001; Agent = 27001; Map = 28001; Map12 = 28003; Hit = 16001 },
    @{ Name = 'port=17001'; Login = 26001; Agent = 17001; Map = 28001; Map12 = 28003; Hit = 17001 },
    @{ Name = 'port=18001'; Login = 26001; Agent = 27001; Map = 18001; Map12 = 28003; Hit = 18001 },
    @{ Name = 'port=36001'; Login = 36001; Agent = 27001; Map = 28001; Map12 = 28003; Hit = 36001 },
    @{ Name = 'port=37001'; Login = 26001; Agent = 37001; Map = 28001; Map12 = 28003; Hit = 37001 },
    @{ Name = 'port=38001'; Login = 26001; Agent = 27001; Map = 38001; Map12 = 28003; Hit = 38001 }
)
foreach ($case in $forbiddenCases) {
    $expect = 'port ' + $case.Hit + ' is forbidden'
    $body = $header + @"

`$msg = Get-SoakPortViolation -LoginPort $($case.Login) -AgentPort $($case.Agent) -MapPort $($case.Map) -Map12Port $($case.Map12)
Write-Output ('PORT_CHECK ' + `$msg)
if (`$msg -eq '$expect') { exit 1 }
exit 0
"@
    Add-Case -Name $case.Name -Body $body -Expect 1 -RequiredText $expect
}

$allowedBody = $header + @'

$blocked = @(Get-SoakForbiddenPorts)
foreach ($port in @(26001, 27001, 28001)) {
    if ($blocked -contains $port) {
        Write-Output ('PORT_CHECK forbidden ' + $port)
        exit 1
    }
}
$msg = Get-SoakPortViolation -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -Map12Port 28003
Write-Output ('PORT_CHECK ' + $msg)
if ([string]::IsNullOrEmpty($msg)) { exit 0 }
exit 1
'@
Add-Case -Name 'ports=26001,27001,28001' -Body $allowedBody -Expect 0 -RequiredText 'PORT_CHECK '

$missingBody = $header + @"

`$tokens = `$null
`$errors = `$null
`$ast = [System.Management.Automation.Language.Parser]::ParseFile('$($soak.Replace("'", "''"))', [ref]`$tokens, [ref]`$errors)
`$predicate = [scriptblock]::Create('param(`$node) `$node -is [System.Management.Automation.Language.ParameterAst] -and `$node.Name.VariablePath.UserPath -eq ''Map12Port''')
`$params = @(`$ast.FindAll(`$predicate, `$true))
foreach (`$param in `$params) {
    if (`$null -ne `$param.DefaultValue -and `$param.DefaultValue.Extent.Text -match '28002') {
        Write-Output 'PORT_CHECK Map12Port still defaults to 28002'
        exit 0
    }
}
if (-not (Get-Command Get-SoakMap12StepRefusal -ErrorAction SilentlyContinue)) {
    Write-Output 'PORT_CHECK map12 step does not refuse a missing port'
    exit 0
}
`$msg = Get-SoakMap12StepRefusal -Bound:`$false
Write-Output ('PORT_CHECK ' + `$msg)
if (`$msg -match 'Map12Port' -and `$msg -match 'explicit') { exit 1 }
exit 0
"@
Add-Case -Name 'map12=missing' -Body $missingBody -Expect 1 -RequiredText 'Map12Port'

$explicitBody = $header + @'

if (Get-Command Get-SoakMap12StepRefusal -ErrorAction SilentlyContinue) {
    $msg = Get-SoakMap12StepRefusal -Bound -Map12Port 28002
} else {
    $msg = Get-SoakPortViolation -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002
}
Write-Output ('PORT_CHECK ' + $msg)
if ([string]::IsNullOrEmpty($msg)) { exit 0 }
exit 1
'@
Add-Case -Name 'map12=28002' -Body $explicitBody -Expect 0 -RequiredText 'PORT_CHECK '

if ($failed) {
    Write-Output 'PWSH SUMMARY result=fail'
    exit 1
}
Write-Output 'PWSH SUMMARY result=pass'
