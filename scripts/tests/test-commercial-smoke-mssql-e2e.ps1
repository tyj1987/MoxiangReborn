# LocalDB availability gate for scripts/commercial-smoke.ps1.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-commercial-smoke-mssql-e2e.ps1
# Requires PowerShell 7.4 or newer. Runs on Linux and Windows.
$ErrorActionPreference = 'Stop'

$version = $PSVersionTable.PSVersion
$versionOk = $false
if ($version.Major -gt 7) { $versionOk = $true }
if ($version.Major -eq 7 -and $version.Minor -ge 4) { $versionOk = $true }
if (-not $versionOk) { throw "FAIL: requires pwsh 7.4+, found $version" }

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$script:Smoke = Join-Path $repoRoot 'scripts\commercial-smoke.ps1'

$Unavailable = 'MSSQL_E2E E2E_UNAVAILABLE (LocalDB not found; pass -SkipMssql to record as not-run)'
$SkipExplicit = 'MSSQL_E2E SKIPPED (explicit -SkipMssql; external prerequisite not satisfied)'
$SkipNotBuilt = 'MSSQL_E2E SKIPPED (mxh_client_e2e not built)'
$PassLine = 'MSSQL_E2E PASS (LocalDB, one command)'
$DoneLine = 'COMMERCIAL_SMOKE PASS'
$ExpectedArgs = @('--login-host', '127.0.0.1', '--backend', 'mssql_odbc', '--init-schema')

# x64 Windows console stub. Writes each argument on its own line to
# <this-exe>.invoked and exits 0. Linux runs a shell script with the same log.
$script:WindowsE2eStubB64 = @'
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAgAAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAABQRQAAZIYFADFR4WoAAAAAAAAAAPAALgILAgIpAAQAAAAIAAAAAAAAIRIA
AAAQAAAAAABAAQAAAAAQAAAAAgAABAAAAAAAAAAFAAIAAAAAAABgAAAABAAADGwAAAMAYAEAACAA
AAAAAAAQAAAAAAAAAAAQAAAAAAAAEAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAUAAAIAEAAAAAAAAA
AAAAADAAABgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAYFAAADgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAudGV4dAAA
AIACAAAAEAAAAAQAAAAEAAAAAAAAAAAAAAAAAAAgAABgLnJkYXRhAAAgAAAAACAAAAACAAAACAAA
AAAAAAAAAAAAAAAAQAAAQC5wZGF0YQAAGAAAAAAwAAAAAgAAAAoAAAAAAAAAAAAAAAAAAEAAAEAu
eGRhdGEAABQAAAAAQAAAAAIAAAAMAAAAAAAAAAAAAAAAAABAAABALmlkYXRhAAAgAQAAAFAAAAAC
AAAADgAAAAAAAAAAAAAAAAAAQAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFNI
gexgBgAAMclBuAQBAABIjVwkSEiJ2v8VYkAAADHSjUj/gfkCAQAAdgq5AwAAAOmzAAAARIoEE0iN
jCRMAQAARIiEFEwBAABI/8I5wnLlicJFMclFMcDGhBRMAQAALo1QAcaEFEwBAABpjVACxoQUTAEA
AG6NUAPGhBRMAQAAdo1QBMaEFEwBAABvjVAFxoQUTAEAAGuNUAbGhBRMAQAAZY1QB4PACMaEFEwB
AABkMdLGhARMAQAAAEiJVCQwugAAAEDHRCQogAAAAMdEJCACAAAA/xWOPwAASInDSIP4/3ULuQQA
AAD/FYI/AAD/FYQ/AABIhcAPhBQBAACAOCJ1IIpQAUj/wITSdAeA+iJ18esdgPoidRHrFoD6CXQK
SP/AihD2wt918YoQgPogdQVI/8Dr9ID6CXT2QboBAAAARTHAMdJFMclEicmKDAiEyXRTgfr+AwAA
dE+A+SJ1BkGD8AHrO0WFwHUmgPkgdAWA+Ql1HEWF0nUnidFFMcD/wkG6AQAAAMaEDGACAAAK6xBB
idL/wkKIjBRgAgAARTHSQf/B66OF0nRojUL/gLwEYAIAAAp0HESNQgFBgfj/AwAAdxTGhBRgAgAA
CkWFwHRB6wtBidDrBkG4/v///zHASI2UJGACAABMjUwkREiJ2UiJRCQg/xWOPgAAhcB1E0iJ2f8V
WT4AALkFAAAA6df+//9Iidn/FUY+AAAxyenH/v//SIPsKEiD5PDo0v3//5CQ/yVSPgAAkJD/JUI+
AACQkP8lMj4AAJCQ/yUiPgAAkJD/JRI+AACQkP8lAj4AAJCQ//////////8AAAAAAAAAAP//////
////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABHQ0M6
IChHTlUpIDEzLXdpbjMyAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAh
EgAAAEAAACESAAAvEgAADEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQgDAAgB
zAABMAAAAQQBAARCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoUAAAAAAA
AAAAAAAQUQAAYFAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAmFAAAAAAAACmUAAAAAAAALRQAAAAAAAA
wlAAAAAAAADUUAAAAAAAAOpQAAAAAAAAAAAAAAAAAACYUAAAAAAAAKZQAAAAAAAAtFAAAAAAAADC
UAAAAAAAANRQAAAAAAAA6lAAAAAAAAAAAAAAAAAAAI0AQ2xvc2VIYW5kbGUAzABDcmVhdGVGaWxl
QQBsAUV4aXRQcm9jZXNzAOUBR2V0Q29tbWFuZExpbmVBAIcCR2V0TW9kdWxlRmlsZU5hbWVBAAAc
BldyaXRlRmlsZQAAAABQAAAAUAAAAFAAAABQAAAAUAAAAFAAAEtFUk5FTDMyLmRsbAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==
'@

function Assert-True($condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
}

function Get-SmokeText([string]$Text) {
    if ($null -eq $Text) { return '' }
    $clean = [regex]::Replace($Text, ([char]27).ToString() + '\[[0-9;]*m', '')
    return $clean.Replace("`r`n", "`n").Replace("`r", "`n").Trim()
}

function Count-SmokeLine([string]$Text, [string]$Line) {
    $n = 0
    foreach ($item in ($Text -split "`n")) {
        if ($item.Trim() -eq $Line) { $n = $n + 1 }
    }
    return $n
}

function New-ToolDir {
    param([switch]$WithLocalDb)
    $dir = Join-Path $script:Root ('bin-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    if ($IsWindows) {
        [IO.File]::WriteAllText((Join-Path $dir 'ctest.cmd'), "@echo off`r`nexit /b 0`r`n")
        if ($WithLocalDb) {
            [IO.File]::WriteAllText((Join-Path $dir 'sqllocaldb.cmd'), "@echo off`r`nexit /b 0`r`n")
        }
    } else {
        $sh = "#!/bin/sh`nexit 0`n"
        $ctest = Join-Path $dir 'ctest'
        [IO.File]::WriteAllText($ctest, $sh)
        & /bin/chmod +x -- $ctest
        if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod ctest exited $LASTEXITCODE" }
        if ($WithLocalDb) {
            $tool = Join-Path $dir 'sqllocaldb'
            [IO.File]::WriteAllText($tool, $sh)
            & /bin/chmod +x -- $tool
            if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod sqllocaldb exited $LASTEXITCODE" }
        }
    }
    return $dir
}

function New-E2eStub {
    param([Parameter(Mandatory = $true)][string]$Exe)
    $parent = Split-Path -Parent $Exe
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    if ($IsWindows) {
        [IO.File]::WriteAllBytes($Exe, $script:WindowsE2eStubBytes)
        return
    }
    $sh = @'
#!/bin/sh
log="$0.invoked"
printf '%s\n' "$@" > "$log"
exit 0
'@
    $sh = $sh.Replace("`r`n", "`n")
    if (-not $sh.EndsWith("`n")) { $sh += "`n" }
    [IO.File]::WriteAllText($Exe, $sh)
    & /bin/chmod +x -- $Exe
    if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod e2e stub exited $LASTEXITCODE" }
}

function New-BuildDir {
    param([switch]$WithExe)
    $dir = Join-Path $script:Root ('build-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    [IO.File]::WriteAllText((Join-Path $dir 'CTestTestfile.cmake'), "# fixture`n")
    $exe = Join-Path $dir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
    if ($WithExe) { New-E2eStub -Exe $exe }
    return [pscustomobject]@{
        Build = $dir
        Exe = $exe
    }
}

function Read-InvokedArgs {
    param([string]$Exe)
    $log = $Exe + '.invoked'
    if (-not (Test-Path -LiteralPath $log)) { return $null }
    $raw = [IO.File]::ReadAllText($log).Replace("`r`n", "`n").Replace("`r", "`n").Trim()
    if ($raw.Length -eq 0) { return @() }
    return @($raw.Split("`n"))
}

function Invoke-Smoke {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$PathValue,
        [ValidateSet('default', 'force-true', 'force-false')][string]$ProbeMode = 'default',
        [switch]$SkipMssql
    )
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = (Get-Process -Id $PID).Path
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.WorkingDirectory = $BuildDir
    $psi.Environment['PATH'] = $PathValue
    $psi.ArgumentList.Add('-NoProfile')
    $psi.ArgumentList.Add('-File')
    if ($ProbeMode -eq 'default') {
        $psi.ArgumentList.Add($script:Smoke)
        $psi.ArgumentList.Add('-BuildDir')
        $psi.ArgumentList.Add($BuildDir)
        $psi.ArgumentList.Add('-SkipGui')
        if ($SkipMssql) { $psi.ArgumentList.Add('-SkipMssql') }
    } else {
        $psi.ArgumentList.Add($script:Driver)
        $psi.ArgumentList.Add('-Smoke')
        $psi.ArgumentList.Add($script:Smoke)
        $psi.ArgumentList.Add('-BuildDir')
        $psi.ArgumentList.Add($BuildDir)
        $psi.ArgumentList.Add('-ProbeMode')
        $psi.ArgumentList.Add($ProbeMode)
        if ($SkipMssql) { $psi.ArgumentList.Add('-SkipMssql') }
    }
    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    if (-not $proc.Start()) { throw 'FAIL: smoke child did not start' }
    try {
        $outTask = $proc.StandardOutput.ReadToEndAsync()
        $errTask = $proc.StandardError.ReadToEndAsync()
        if (-not $proc.WaitForExit(30000)) {
            try { $proc.Kill() } catch { }
            throw 'FAIL: smoke child timed out'
        }
        $proc.WaitForExit()
        return [pscustomobject]@{
            ExitCode = $proc.ExitCode
            Stdout = $outTask.GetAwaiter().GetResult()
            Stderr = $errTask.GetAwaiter().GetResult()
        }
    } finally {
        $proc.Dispose()
    }
}

$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($script:Smoke, [ref]$tokens, [ref]$parseErrors)
if ($null -ne $parseErrors -and @($parseErrors).Count -gt 0) {
    throw "FAIL: commercial-smoke.ps1 did not parse: $($parseErrors[0])"
}
$smokeText = [IO.File]::ReadAllText($script:Smoke)
Assert-True (-not $smokeText.Contains('$env:')) 'product script must not read an environment variable'
Assert-True ($smokeText.Contains($Unavailable)) 'unavailable line missing from the script'
Assert-True ($smokeText.Contains('ExitCode = 6')) 'unavailable result must use exit code 6'
Assert-True ($smokeText.Contains('exit $mssql.ExitCode')) 'unavailable result must exit the script'
Assert-True ($smokeText.Contains('6  MSSQL_E2E E2E_UNAVAILABLE')) 'header must document exit code 6'
Assert-True ($smokeText.Contains('--login-host 127.0.0.1 --backend mssql_odbc --init-schema')) 'E2E arguments changed'

$script:WindowsE2eStubBytes = [Convert]::FromBase64String(($script:WindowsE2eStubB64 -replace '\s', ''))
Assert-True ($script:WindowsE2eStubBytes.Length -gt 64) 'Windows stub is empty'
Assert-True ($script:WindowsE2eStubBytes[0] -eq 77 -and $script:WindowsE2eStubBytes[1] -eq 90) 'Windows stub is not a PE'

$finder = { param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }
$functions = @($ast.FindAll($finder, $true))
foreach ($fn in $functions) {
    Invoke-Expression $fn.Extent.Text
}
Assert-True ($functions.Name -contains 'Test-CommercialSmokeLocalDbAvailable') 'LocalDB probe function missing'
Assert-True ($functions.Name -contains 'Invoke-CommercialSmokeMssqlE2E') 'MSSQL E2E function missing'

$temp = [IO.Path]::GetTempPath()
if ([string]::IsNullOrWhiteSpace($temp)) { $temp = [IO.Path]::GetFullPath('.') }
$script:Root = Join-Path $temp ('mxh-smoke-e2e-' + [guid]::NewGuid().ToString('n'))
New-Item -ItemType Directory -Force -Path $script:Root | Out-Null
$script:Driver = Join-Path $script:Root 'driver.ps1'
@'
param(
    [Parameter(Mandatory = $true)][string]$Smoke,
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][ValidateSet('force-true', 'force-false')][string]$ProbeMode,
    [switch]$SkipMssql
)
$ErrorActionPreference = 'Stop'
$invoke = @{
    BuildDir = $BuildDir
    SkipGui = $true
}
if ($SkipMssql) { $invoke.SkipMssql = $true }
if ($ProbeMode -eq 'force-true') {
    $invoke.LocalDbProbe = { $true }
} else {
    $invoke.LocalDbProbe = { $false }
}
& $Smoke @invoke
if ($null -ne $LASTEXITCODE -and [int]$LASTEXITCODE -ne 0) { exit ([int]$LASTEXITCODE) }
exit 0
'@ | Set-Content -LiteralPath $script:Driver -Encoding ascii

try {
    $savedPath = $env:PATH
    try {
        $emptyBin = New-ToolDir
        $env:PATH = $emptyBin
        Assert-True (-not (Test-CommercialSmokeLocalDbAvailable)) 'probe must be false when sqllocaldb is not on PATH'
        $presentBin = New-ToolDir -WithLocalDb
        $env:PATH = $presentBin
        Assert-True (Test-CommercialSmokeLocalDbAvailable) 'probe must be true when sqllocaldb resolves on PATH'
    } finally {
        if ($null -eq $savedPath) {
            Remove-Item -Path 'Env:PATH' -ErrorAction SilentlyContinue
        } else {
            $env:PATH = $savedPath
        }
    }

    $absent = New-ToolDir
    $present = New-ToolDir -WithLocalDb

    $missingExe = New-BuildDir -WithExe
    $missing = Invoke-Smoke -BuildDir $missingExe.Build -PathValue $absent
    $missingText = Get-SmokeText $missing.Stdout
    Assert-True ($missing.ExitCode -eq 6) "LocalDB missing exit was $($missing.ExitCode); stdout=[$missingText] stderr=[$($missing.Stderr)]"
    Assert-True ($missingText -eq $Unavailable) "LocalDB missing must print only the unavailable line, got [$missingText]"
    Assert-True ((Count-SmokeLine $missingText $Unavailable) -eq 1) 'unavailable line must appear once'
    Assert-True (-not $missingText.Contains('SKIPPED')) 'LocalDB missing must not skip'
    Assert-True (-not $missingText.Contains('PASS')) 'LocalDB missing must not pass'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $missingExe.Exe)) 'LocalDB missing must not invoke E2E'
    Assert-True ([string]::IsNullOrWhiteSpace((Get-SmokeText $missing.Stderr))) "LocalDB missing stderr: $($missing.Stderr)"

    $missingNoExeBuild = New-BuildDir
    $missingNoExe = Invoke-Smoke -BuildDir $missingNoExeBuild.Build -PathValue $absent
    $missingNoExeText = Get-SmokeText $missingNoExe.Stdout
    Assert-True ($missingNoExe.ExitCode -eq 6) "LocalDB missing without exe exit was $($missingNoExe.ExitCode); stdout=[$missingNoExeText]"
    Assert-True ($missingNoExeText -eq $Unavailable) "missing LocalDB without the exe must still be unavailable, got [$missingNoExeText]"

    $skipBuild = New-BuildDir -WithExe
    $skip = Invoke-Smoke -BuildDir $skipBuild.Build -PathValue $absent -SkipMssql
    $skipText = Get-SmokeText $skip.Stdout
    Assert-True ($skip.ExitCode -eq 0) "SkipMssql exit was $($skip.ExitCode); stdout=[$skipText] stderr=[$($skip.Stderr)]"
    Assert-True ((Count-SmokeLine $skipText $SkipExplicit) -eq 1) "SkipMssql line missing in [$skipText]"
    Assert-True ((Count-SmokeLine $skipText $DoneLine) -eq 1) "SkipMssql must still finish, got [$skipText]"
    Assert-True (-not $skipText.Contains($Unavailable)) 'SkipMssql must not report E2E_UNAVAILABLE'
    Assert-True (-not $skipText.Contains($PassLine)) 'SkipMssql must not report MSSQL pass'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $skipBuild.Exe)) 'SkipMssql must not invoke E2E'

    $presentBuild = New-BuildDir -WithExe
    $ran = Invoke-Smoke -BuildDir $presentBuild.Build -PathValue $present
    $ranText = Get-SmokeText $ran.Stdout
    Assert-True ($ran.ExitCode -eq 0) "LocalDB present exit was $($ran.ExitCode); stdout=[$ranText] stderr=[$($ran.Stderr)]"
    Assert-True ((Count-SmokeLine $ranText $PassLine) -eq 1) "LocalDB present must pass E2E, got [$ranText]"
    Assert-True ((Count-SmokeLine $ranText $DoneLine) -eq 1) "LocalDB present must finish smoke, got [$ranText]"
    Assert-True (-not $ranText.Contains($Unavailable)) 'LocalDB present must not report E2E_UNAVAILABLE'
    $invoked = @(Read-InvokedArgs -Exe $presentBuild.Exe)
    Assert-True ($null -ne $invoked -and $invoked.Count -eq $ExpectedArgs.Count) "E2E args were [$($invoked -join '|')]"
    for ($i = 0; $i -lt $ExpectedArgs.Count; $i++) {
        Assert-True ($invoked[$i] -eq $ExpectedArgs[$i]) "E2E arg $i was $($invoked[$i])"
    }

    $notBuilt = New-BuildDir
    $notBuiltRun = Invoke-Smoke -BuildDir $notBuilt.Build -PathValue $present
    $notBuiltText = Get-SmokeText $notBuiltRun.Stdout
    Assert-True ($notBuiltRun.ExitCode -eq 0) "not-built exit was $($notBuiltRun.ExitCode); stdout=[$notBuiltText]"
    Assert-True ((Count-SmokeLine $notBuiltText $SkipNotBuilt) -eq 1) "LocalDB present without the exe must skip not-built, got [$notBuiltText]"
    Assert-True (-not $notBuiltText.Contains($Unavailable)) 'not-built skip must not report E2E_UNAVAILABLE'
    Assert-True (-not $notBuiltText.Contains($PassLine)) 'not-built skip must not report MSSQL pass'

    $forcedOffBuild = New-BuildDir -WithExe
    $forcedOff = Invoke-Smoke -BuildDir $forcedOffBuild.Build -PathValue $present -ProbeMode 'force-false'
    $forcedOffText = Get-SmokeText $forcedOff.Stdout
    Assert-True ($forcedOff.ExitCode -eq 6) "probe override false exit was $($forcedOff.ExitCode); stdout=[$forcedOffText] stderr=[$($forcedOff.Stderr)]"
    Assert-True ($forcedOffText -eq $Unavailable) "probe override false must be unavailable even when sqllocaldb is on PATH, got [$forcedOffText]"
    Assert-True ($null -eq (Read-InvokedArgs -Exe $forcedOffBuild.Exe)) 'probe override false must not invoke E2E'

    $forcedOnBuild = New-BuildDir -WithExe
    $forcedOn = Invoke-Smoke -BuildDir $forcedOnBuild.Build -PathValue $absent -ProbeMode 'force-true'
    $forcedOnText = Get-SmokeText $forcedOn.Stdout
    Assert-True ($forcedOn.ExitCode -eq 0) "probe override true exit was $($forcedOn.ExitCode); stdout=[$forcedOnText] stderr=[$($forcedOn.Stderr)]"
    Assert-True ((Count-SmokeLine $forcedOnText $PassLine) -eq 1) "probe override true must invoke E2E, got [$forcedOnText]"
    $forcedArgs = @(Read-InvokedArgs -Exe $forcedOnBuild.Exe)
    Assert-True ($null -ne $forcedArgs -and ($forcedArgs -join "`n") -eq ($ExpectedArgs -join "`n")) "probe override true args were [$($forcedArgs -join '|')]"
} finally {
    Remove-Item -LiteralPath $script:Root -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Output 'PASS: commercial-smoke MSSQL E2E availability'
