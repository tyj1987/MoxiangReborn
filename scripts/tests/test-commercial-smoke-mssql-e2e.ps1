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

$script:AssertPass = 0
$script:Failures = New-Object System.Collections.Generic.List[string]
$script:CasePassDetail = ''

function Assert-True($condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
    $script:AssertPass = $script:AssertPass + 1
}

function Invoke-Case {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Body
    )
    $script:CasePassDetail = ''
    try {
        . $Body
        Write-Output ('PASS ' + $Name + $script:CasePassDetail)
    } catch {
        $detail = [string]$_.Exception.Message
        if ([string]::IsNullOrWhiteSpace($detail)) { $detail = [string]$_ }
        $inner = $_.Exception.InnerException
        if ($null -ne $inner) { $detail = $detail + ' | inner ' + [string]$inner.Message }
        $position = ''
        try { $position = [string]$_.InvocationInfo.PositionMessage } catch { $position = '' }
        if (-not [string]::IsNullOrWhiteSpace($position)) {
            $detail = $detail + ' | ' + ($position -replace '\s+', ' ')
        }
        $stack = ''
        try { $stack = [string]$_.ScriptStackTrace } catch { $stack = '' }
        if (-not [string]::IsNullOrWhiteSpace($stack)) {
            $detail = $detail + ' | ' + ($stack -replace '\s+', ' ')
        }
        $script:Failures.Add($Name + ' :: ' + $detail)
        Write-Output ('FAIL ' + $Name + ' :: ' + $detail)
    } finally {
        $script:CasePassDetail = ''
    }
}

function Test-PidStillRunning {
    param([int]$ProcessId)
    if ($ProcessId -le 0) { return $false }
    if (-not $IsWindows) {
        $statusPath = '/proc/' + $ProcessId + '/status'
        if (-not (Test-Path -LiteralPath $statusPath)) { return $false }
        try {
            foreach ($line in [IO.File]::ReadLines($statusPath)) {
                if (-not $line.StartsWith('State:')) { continue }
                $state = $line.Substring(6).Trim()
                if ($state.StartsWith('Z') -or $state.StartsWith('X')) { return $false }
                return $true
            }
        } catch {
            return $false
        }
    }
    try {
        $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if ($null -eq $live) { return $false }
        try { $live.Refresh() } catch { }
        return -not [bool]$live.HasExited
    } catch {
        return $false
    }
}

function Get-ProcStatusField {
    param(
        [int]$ProcessId,
        [string]$Field
    )
    $statusPath = '/proc/' + $ProcessId + '/status'
    if (-not (Test-Path -LiteralPath $statusPath)) { return '' }
    try {
        foreach ($line in [IO.File]::ReadLines($statusPath)) {
            if ($line.StartsWith($Field)) { return $line.Substring($Field.Length).Trim() }
        }
    } catch { }
    return ''
}

function Stop-TestProcess {
    param($Process)
    if ($null -eq $Process) { return }
    try {
        $id = [int]$Process.Id
        if (Test-PidStillRunning -ProcessId $id) {
            Stop-Process -Id $id -Force -ErrorAction SilentlyContinue
        }
    } catch { }
}

function Get-TestProcessStartTicks {
    param([int]$ProcessId)
    # UTC StartTime ticks. A reused PID does not keep the same value.
    if ($ProcessId -le 0) { return [int64]0 }
    try {
        $live = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if ($null -eq $live) { return [int64]0 }
        try { $live.Refresh() } catch { }
        try { if ([bool]$live.HasExited) { return [int64]0 } } catch { return [int64]0 }
        $started = $live.StartTime
        if ($null -eq $started) { return [int64]0 }
        $ticks = [int64]$started.ToUniversalTime().Ticks
        if ($ticks -le 0) { return [int64]0 }
        return $ticks
    } catch {
        return [int64]0
    }
}

function Stop-TestPidIfSameStart {
    param(
        [int]$ProcessId,
        [int64]$StartTicks
    )
    # Stop only when the live process is still the one recorded earlier.
    # A missing or different start time means the PID is already gone.
    if ($ProcessId -le 0) { return }
    if ($StartTicks -le 0) { return }
    $liveTicks = Get-TestProcessStartTicks -ProcessId $ProcessId
    if ($liveTicks -le 0) { return }
    if ($liveTicks -ne $StartTicks) { return }
    try { Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue } catch { }
}

function Invoke-OwnedCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutSeconds = 600
    )
    $items = @(
        Invoke-CommercialSmokeOwnedExecutable -FilePath $FilePath -ArgumentList $ArgumentList -TimeoutSeconds $TimeoutSeconds 6>&1
    )
    $lines = New-Object System.Collections.Generic.List[string]
    $result = $null
    foreach ($item in $items) {
        if ($null -eq $item) { continue }
        $isResult = $false
        try {
            if ($null -ne $item.PSObject.Properties['ExitCode'] -and $null -ne $item.PSObject.Properties['Owned']) {
                $isResult = $true
            }
        } catch { $isResult = $false }
        if ($isResult) {
            $result = $item
            continue
        }
        $lines.Add([string]$item)
    }
    return [pscustomobject]@{
        Result = $result
        Text = ($lines.ToArray() -join "`n")
    }
}

function Invoke-WindowsCsc {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Output
    )
    if (-not $IsWindows) { throw 'FAIL: csc helper is Windows-only' }
    $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
    if (-not (Test-Path -LiteralPath $csc)) {
        $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
    }
    if (-not (Test-Path -LiteralPath $csc)) { throw 'FAIL: Windows stub needs csc.exe' }
    & $csc /nologo /platform:x64 /out:$Output $Source
    if ($LASTEXITCODE -ne 0) { throw "FAIL: csc exited $LASTEXITCODE for $Output" }
}

function New-MxhServerNamedExe {
    param([Parameter(Mandatory = $true)][string]$ProcessName)
    $dir = Join-Path $script:Root ('named-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    if ($IsWindows) {
        $exe = Join-Path $dir ($ProcessName + '.exe')
        $cs = Join-Path $dir 'sleeper.cs'
        @'
using System;
using System.Threading;
class Sleeper {
    static int Main(string[] args) {
        int seconds = 30;
        if (args.Length > 0) {
            int parsed;
            if (int.TryParse(args[0], out parsed)) seconds = parsed;
        }
        if (seconds < 0) seconds = 0;
        Thread.Sleep(seconds * 1000);
        return 0;
    }
}
'@ | Set-Content -LiteralPath $cs -Encoding ascii
        $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
        if (-not (Test-Path -LiteralPath $csc)) {
            $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
        }
        if (-not (Test-Path -LiteralPath $csc)) { throw 'FAIL: Windows named sleeper needs csc.exe' }
        & $csc /nologo /out:$exe $cs
        if ($LASTEXITCODE -ne 0) { throw "FAIL: csc named sleeper exited $LASTEXITCODE" }
        return $exe
    }
    $exe = Join-Path $dir $ProcessName
    $source = '/bin/sleep'
    if (-not (Test-Path -LiteralPath $source)) { throw 'FAIL: /bin/sleep is required for the process-stop fixture' }
    [IO.File]::Copy($source, $exe, $true)
    & /bin/chmod +x -- $exe
    if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod named sleeper exited $LASTEXITCODE" }
    return $exe
}

function Get-SmokeText([string]$Text) {
    if ($null -eq $Text) { return '' }
    $clean = [regex]::Replace($Text, ([char]27).ToString() + '\[[0-9;]*m', '')
    return $clean.Replace("`r`n", "`n").Replace("`r", "`n").Trim()
}

function Write-SmokeChildStdout {
    param([string]$Text)
    # Echo the child smoke stdout into this process so a redirected job log
    # contains the script's OWNED and DROPPED lines. Assertions use $Text
    # unchanged.
    if ([string]::IsNullOrEmpty($Text)) { return }
    # -split with a count does not break this string into lines.
    foreach ($line in [regex]::Split($Text, '\r\n|\n|\r')) {
        Write-Host ('CHILD> ' + $line)
    }
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
        # commercial-smoke invokes `& ctest @args`. A ctest.cmd file is
        # started through cmd.exe, and cmd treats '|' in the unquoted -R
        # regex as a pipe (exit 255). ctest.exe is a native image, same
        # path as real ctest.exe: CreateProcess, no cmd. This PE writes
        # each argument on its own line to ctest.exe.invoked and exits 0.
        $ctest = Join-Path $dir 'ctest.exe'
        if ($null -eq $script:WindowsE2eStubBytes -or $script:WindowsE2eStubBytes.Length -lt 64) {
            throw 'FAIL: Windows ctest stub needs the argument-logging PE'
        }
        [IO.File]::WriteAllBytes($ctest, $script:WindowsE2eStubBytes)
        if ($WithLocalDb) {
            [IO.File]::WriteAllText((Join-Path $dir 'sqllocaldb.cmd'), "@echo off`r`nexit /b 0`r`n")
        }
    } else {
        $ctestSh = @'
#!/bin/sh
printf '%s\n' "$@" > "$0.invoked"
exit 0
'@
        $ctestSh = $ctestSh.Replace("`r`n", "`n")
        if (-not $ctestSh.EndsWith("`n")) { $ctestSh += "`n" }
        $ctest = Join-Path $dir 'ctest'
        [IO.File]::WriteAllText($ctest, $ctestSh)
        & /bin/chmod +x -- $ctest
        if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod ctest exited $LASTEXITCODE" }
        if ($WithLocalDb) {
            $tool = Join-Path $dir 'sqllocaldb'
            [IO.File]::WriteAllText($tool, "#!/bin/sh`nexit 0`n")
            & /bin/chmod +x -- $tool
            if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod sqllocaldb exited $LASTEXITCODE" }
        }
    }
    return $dir
}

function Get-CtestStubPath {
    param([Parameter(Mandatory = $true)][string]$ToolDir)
    if ($IsWindows) { return (Join-Path $ToolDir 'ctest.exe') }
    return (Join-Path $ToolDir 'ctest')
}

function New-E2eStub {
    param(
        [Parameter(Mandatory = $true)][string]$Exe,
        [int]$ExitCode = 0,
        [switch]$SpawnChild
    )
    $parent = Split-Path -Parent $Exe
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    if ($ExitCode -ne 0) {
        [IO.File]::WriteAllText($Exe + '.exitcode', ([string]$ExitCode + "`n"))
    }
    if ($IsWindows) {
        # The fixed PE always exits 0 and does not read <exe>.exitcode.
        # A nonzero code, or -SpawnChild, must use the csc stub, which does.
        if ($ExitCode -eq 0 -and -not $SpawnChild) {
            [IO.File]::WriteAllBytes($Exe, $script:WindowsE2eStubBytes)
            return
        }
        $cs = Join-Path $parent 'e2e-stub.cs'
        @'
using System;
using System.IO;
class E2eStub {
    static int Main(string[] args) {
        string exe = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
        File.WriteAllText(exe + ".invoked", string.Join("\n", args));
        string childFlag = exe + ".spawnchild";
        if (File.Exists(childFlag)) {
            string shell = System.IO.Path.Combine(Environment.GetEnvironmentVariable("WINDIR"), "System32", "WindowsPowerShell", "v1.0", "powershell.exe");
            var child = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo {
                FileName = shell,
                Arguments = "-NoProfile -NonInteractive -Command Start-Sleep -Seconds 60",
                UseShellExecute = false,
                CreateNoWindow = true
            });
            File.WriteAllText(exe + ".children", child.Id.ToString());
            System.Threading.Thread.Sleep(1000);
        }
        string codeFile = exe + ".exitcode";
        if (File.Exists(codeFile)) {
            int code;
            if (int.TryParse(File.ReadAllText(codeFile).Trim(), out code)) return code;
        }
        return 0;
    }
}
'@ | Set-Content -LiteralPath $cs -Encoding ascii
        $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
        if (-not (Test-Path -LiteralPath $csc)) {
            $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
        }
        if (-not (Test-Path -LiteralPath $csc)) { throw 'FAIL: Windows E2E failure stub needs csc.exe' }
        & $csc /nologo /out:$Exe $cs
        if ($LASTEXITCODE -ne 0) { throw "FAIL: csc e2e stub exited $LASTEXITCODE" }
        if ($SpawnChild) {
            [IO.File]::WriteAllText($Exe + '.spawnchild', "spawn`n")
        }
        return
    }
    if ($SpawnChild) {
        $sh = @'
#!/bin/sh
log="$0.invoked"
printf '%s\n' "$@" > "$log"
/bin/sleep 60 &
echo $! > "$0.children"
sleep 2
code=0
codefile="$0.exitcode"
if [ -f "$codefile" ]; then
    IFS= read -r code < "$codefile" || code=0
fi
case "$code" in
    ''|*[!0-9]*) code=1 ;;
esac
exit "$code"
'@
    } else {
        $sh = @'
#!/bin/sh
log="$0.invoked"
printf '%s\n' "$@" > "$log"
code=0
codefile="$0.exitcode"
if [ -f "$codefile" ]; then
    IFS= read -r code < "$codefile" || code=0
fi
case "$code" in
    ''|*[!0-9]*) code=1 ;;
esac
exit "$code"
'@
    }
    $sh = $sh.Replace("`r`n", "`n")
    if (-not $sh.EndsWith("`n")) { $sh += "`n" }
    [IO.File]::WriteAllText($Exe, $sh)
    & /bin/chmod +x -- $Exe
    if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod e2e stub exited $LASTEXITCODE" }
}

function New-BuildDir {
    param(
        [switch]$WithExe,
        [ValidateSet('Debug', 'Single')]
        [string]$Layout = 'Debug',
        [switch]$SpawnChild,
        [int]$ExitCode = 0
    )
    $dir = Join-Path $script:Root ('build-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    [IO.File]::WriteAllText((Join-Path $dir 'CTestTestfile.cmake'), "# fixture`n")
    $single = Join-Path $dir 'tools\MoxianClientE2E\mxh_client_e2e.exe'
    $multi = Join-Path $dir 'tools\MoxianClientE2E\Debug\mxh_client_e2e.exe'
    $exe = $multi
    if ($Layout -eq 'Single') { $exe = $single }
    if ($WithExe) { New-E2eStub -Exe $exe -ExitCode $ExitCode -SpawnChild:$SpawnChild }
    return [pscustomobject]@{
        Build = $dir
        Exe = $exe
        Single = $single
        Multi = $multi
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

function New-SqlCmdStub {
    param([Parameter(Mandatory = $true)][string]$Dir)
    if ($IsWindows) {
        $exe = Join-Path $Dir 'sqlcmd.exe'
        $cs = Join-Path $Dir 'sqlcmd-stub.cs'
        @'
using System;
using System.Collections.Generic;
using System.IO;
class SqlCmdStub {
    static int Main(string[] args) {
        string log = Environment.GetEnvironmentVariable("MXH_SQLCMD_STUB_LOG");
        string dbs = Environment.GetEnvironmentVariable("MXH_SQLCMD_STUB_DBS");
        string dropExitPath = Environment.GetEnvironmentVariable("MXH_SQLCMD_STUB_DROP_EXIT");
        string queryExitPath = Environment.GetEnvironmentVariable("MXH_SQLCMD_STUB_QUERY_EXIT");
        string keepPath = Environment.GetEnvironmentVariable("MXH_SQLCMD_STUB_KEEP");
        if (!string.IsNullOrEmpty(log)) {
            File.AppendAllText(log, string.Join("\n", args) + "\n---\n");
        }
        string query = "";
        for (int i = 0; i < args.Length; i++) {
            if (args[i] == "-Q" && i + 1 < args.Length) query = args[i + 1];
        }
        if (query.IndexOf("DROP DATABASE", StringComparison.Ordinal) >= 0) {
            int code = ReadCode(dropExitPath);
            if (code == 0 && !string.IsNullOrEmpty(dbs)) {
                bool keep = !string.IsNullOrEmpty(keepPath) && File.Exists(keepPath);
                string name = "";
                int open = query.IndexOf('[');
                int close = query.IndexOf(']');
                if (open >= 0 && close > open) name = query.Substring(open + 1, close - open - 1);
                var lines = new List<string>();
                if (File.Exists(dbs)) {
                    foreach (string line in File.ReadAllLines(dbs)) {
                        string trim = line.Trim();
                        if (trim.Length == 0) continue;
                        if (!keep && trim == name) continue;
                        lines.Add(trim);
                    }
                }
                if (keep && name.Length > 0 && !lines.Contains(name)) lines.Add(name);
                File.WriteAllLines(dbs, lines.ToArray());
            }
            return code;
        }
        int qcode = ReadCode(queryExitPath);
        if (qcode != 0) {
            Console.Error.WriteLine("sqlcmd stub query failed");
            return qcode;
        }
        if (!string.IsNullOrEmpty(dbs) && File.Exists(dbs)) {
            Console.Write(File.ReadAllText(dbs));
        }
        return 0;
    }
    static int ReadCode(string path) {
        if (string.IsNullOrEmpty(path) || !File.Exists(path)) return 0;
        int code;
        if (!int.TryParse(File.ReadAllText(path).Trim(), out code)) return 0;
        return code;
    }
}
'@ | Set-Content -LiteralPath $cs -Encoding ascii
        $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
        if (-not (Test-Path -LiteralPath $csc)) {
            $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
        }
        if (-not (Test-Path -LiteralPath $csc)) { throw 'FAIL: Windows sqlcmd stub needs csc.exe' }
        & $csc /nologo /out:$exe $cs
        if ($LASTEXITCODE -ne 0) { throw "FAIL: csc sqlcmd stub exited $LASTEXITCODE" }
        return $exe
    }
    $tool = Join-Path $Dir 'sqlcmd'
    $sh = @'
#!/bin/sh
log="${MXH_SQLCMD_STUB_LOG}"
dbs="${MXH_SQLCMD_STUB_DBS}"
query=""
prev=""
for arg in "$@"; do
    if [ -n "$log" ]; then
        printf '%s\n' "$arg" >> "$log"
    fi
    if [ "$prev" = "-Q" ]; then
        query=$arg
    fi
    prev=$arg
done
if [ -n "$log" ]; then
    printf '%s\n' '---' >> "$log"
fi
read_code() {
    code=0
    if [ -n "$1" ] && [ -f "$1" ]; then
        IFS= read -r code < "$1" || code=0
    fi
    case "$code" in
        ''|*[!0-9]*) code=0 ;;
    esac
    printf '%s' "$code"
}
case "$query" in
    *DROP\ DATABASE*)
        code=$(read_code "${MXH_SQLCMD_STUB_DROP_EXIT}")
        if [ "$code" = "0" ] && [ -n "$dbs" ]; then
            prefix=${query#*[}
            name=${prefix%%]*}
            keep=0
            if [ -n "${MXH_SQLCMD_STUB_KEEP}" ] && [ -f "${MXH_SQLCMD_STUB_KEEP}" ]; then
                keep=1
            fi
            tmp="${dbs}.tmp"
            : > "$tmp"
            if [ -f "$dbs" ]; then
                while IFS= read -r line || [ -n "$line" ]; do
                    if [ -z "$line" ]; then
                        continue
                    fi
                    if [ "$keep" = "0" ] && [ "$line" = "$name" ]; then
                        continue
                    fi
                    printf '%s\n' "$line" >> "$tmp"
                done < "$dbs"
            fi
            if [ "$keep" = "1" ] && [ -n "$name" ]; then
                seen=0
                while IFS= read -r line || [ -n "$line" ]; do
                    if [ "$line" = "$name" ]; then
                        seen=1
                    fi
                done < "$tmp"
                if [ "$seen" = "0" ]; then
                    printf '%s\n' "$name" >> "$tmp"
                fi
            fi
            cat "$tmp" > "$dbs"
            rm -f "$tmp"
        fi
        exit "$code"
        ;;
esac
qcode=$(read_code "${MXH_SQLCMD_STUB_QUERY_EXIT}")
if [ "$qcode" != "0" ]; then
    echo "sqlcmd stub query failed" >&2
    exit "$qcode"
fi
if [ -n "$dbs" ] && [ -f "$dbs" ]; then
    cat "$dbs"
fi
exit 0
'@
    $sh = $sh.Replace("`r`n", "`n")
    if (-not $sh.EndsWith("`n")) { $sh += "`n" }
    [IO.File]::WriteAllText($tool, $sh)
    & /bin/chmod +x -- $tool
    if ($LASTEXITCODE -ne 0) { throw "FAIL: chmod sqlcmd stub exited $LASTEXITCODE" }
    return $tool
}

function Write-AsciiFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text
    )
    $normalized = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    if (-not $normalized.EndsWith("`n")) { $normalized += "`n" }
    [IO.File]::WriteAllText($Path, $normalized)
}

function New-SqlCmdState {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [int]$DropExit = 0,
        [int]$QueryExit = 0,
        [switch]$KeepAfterDrop
    )
    $dir = Join-Path $Root ('sqlstate-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $dbs = Join-Path $dir 'databases.txt'
    $log = Join-Path $dir 'sqlcmd.log'
    $drop = Join-Path $dir 'drop.exit'
    $query = Join-Path $dir 'query.exit'
    $keep = Join-Path $dir 'keep'
    Write-AsciiFile -Path $dbs -Text (($Names | ForEach-Object { $_.Trim() }) -join "`n")
    Write-AsciiFile -Path $drop -Text ([string]$DropExit)
    Write-AsciiFile -Path $query -Text ([string]$QueryExit)
    $envMap = @{
        MXH_SQLCMD_STUB_DBS = $dbs
        MXH_SQLCMD_STUB_LOG = $log
        MXH_SQLCMD_STUB_DROP_EXIT = $drop
        MXH_SQLCMD_STUB_QUERY_EXIT = $query
    }
    if ($KeepAfterDrop) {
        Write-AsciiFile -Path $keep -Text 'keep'
        $envMap['MXH_SQLCMD_STUB_KEEP'] = $keep
    }
    return [pscustomobject]@{
        Dir = $dir
        Log = $log
        Env = $envMap
    }
}

function Read-SqlCmdLog {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    return [IO.File]::ReadAllText($Path).Replace("`r`n", "`n").Replace("`r", "`n")
}

function Get-DatabaseBlock {
    param(
        [string]$Text,
        [string]$When,
        [string]$Server
    )
    $prefix = '[' + $Server + '] databases ' + $When + ': '
    foreach ($line in ($Text -split "`n")) {
        $trim = $line.Trim()
        if (-not $trim.StartsWith($prefix)) { continue }
        $rest = $trim.Substring($prefix.Length).Trim()
        $names = @()
        if ($rest.Length -gt 0 -and $rest -ne 'UNREADABLE') {
            $names = @($rest.Split(' ') | Where-Object { $_.Length -gt 0 })
        }
        return [pscustomobject]@{
            Complete = $true
            Names = @($names)
            Line = $trim
        }
    }
    return [pscustomobject]@{
        Complete = $false
        Names = @()
        Line = ''
    }
}

function Get-SqlCmdArgGroups {
    param([string]$Log)
    $groups = New-Object System.Collections.Generic.List[object]
    $current = New-Object System.Collections.Generic.List[string]
    foreach ($line in ($Log -split "`n")) {
        if ($line -eq '---') {
            if ($current.Count -gt 0) {
                $groups.Add($current.ToArray())
            }
            $current = New-Object System.Collections.Generic.List[string]
            continue
        }
        if ($line.Length -gt 0) { $current.Add($line) }
    }
    return ,$groups.ToArray()
}

function Assert-SqlCmdTargetsInstance {
    param(
        [string]$Log,
        [string]$Server,
        [int]$ExpectedCalls
    )
    $groups = Get-SqlCmdArgGroups -Log $Log
    Assert-True ($groups.Count -eq $ExpectedCalls) "sqlcmd call count was $($groups.Count), expected $ExpectedCalls, log [$Log]"
    foreach ($group in $groups) {
        $sqlArgGroup = @($group)
        $hasE = $false
        $sawS = $false
        for ($i = 0; $i -lt $sqlArgGroup.Count; $i++) {
            if ($sqlArgGroup[$i] -eq '-E') { $hasE = $true }
            if ($sqlArgGroup[$i] -eq '-S') {
                $sawS = $true
                $next = ''
                if (($i + 1) -lt $sqlArgGroup.Count) { $next = [string]$sqlArgGroup[$i + 1] }
                Assert-True ($next -eq $Server) "sqlcmd -S value was [$next], expected [$Server], args [$($sqlArgGroup -join ' ')]"
            }
        }
        Assert-True $hasE "sqlcmd call missing -E, args [$($sqlArgGroup -join ' ')]"
        Assert-True $sawS "sqlcmd call missing -S, args [$($sqlArgGroup -join ' ')]"
    }
}

function Invoke-Smoke {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$PathValue,
        [ValidateSet('default', 'force-true', 'force-false')][string]$ProbeMode = 'default',
        [switch]$SkipMssql,
        [string]$MssqlDb = '',
        [hashtable]$StubEnv
    )
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = (Get-Process -Id $PID).Path
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.WorkingDirectory = $BuildDir
    $psi.Environment['PATH'] = $PathValue
    if ($null -ne $StubEnv) {
        foreach ($key in @($StubEnv.Keys)) {
            $psi.Environment[[string]$key] = [string]$StubEnv[$key]
        }
    }
    $psi.ArgumentList.Add('-NoProfile')
    $psi.ArgumentList.Add('-File')
    if ($ProbeMode -eq 'default') {
        $psi.ArgumentList.Add($script:Smoke)
        $psi.ArgumentList.Add('-BuildDir')
        $psi.ArgumentList.Add($BuildDir)
        $psi.ArgumentList.Add('-SkipGui')
        if ($SkipMssql) { $psi.ArgumentList.Add('-SkipMssql') }
        if (-not [string]::IsNullOrWhiteSpace($MssqlDb)) {
            $psi.ArgumentList.Add('-MssqlDb')
            $psi.ArgumentList.Add($MssqlDb)
        }
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
Invoke-Case 'script text contract' {
    Assert-True (-not $smokeText.Contains('$env:')) 'product script must not read an environment variable'
    Assert-True ($smokeText.Contains($Unavailable)) 'unavailable line missing from the script'
    Assert-True ($smokeText.Contains('ExitCode = 6')) 'unavailable result must use exit code 6'
    Assert-True ($smokeText.Contains('exit $mssql.ExitCode')) 'unavailable result must exit the script'
    Assert-True ($smokeText.Contains('6  MSSQL_E2E E2E_UNAVAILABLE')) 'header must document exit code 6'
    Assert-True ($smokeText.Contains('--login-host 127.0.0.1 --backend mssql_odbc --init-schema')) 'E2E arguments changed'
}

$script:WindowsE2eStubBytes = [Convert]::FromBase64String(($script:WindowsE2eStubB64 -replace '\s', ''))
Invoke-Case 'Windows E2E PE stub' {
    Assert-True ($script:WindowsE2eStubBytes.Length -gt 64) 'Windows stub is empty'
    Assert-True ($script:WindowsE2eStubBytes[0] -eq 77 -and $script:WindowsE2eStubBytes[1] -eq 90) 'Windows stub is not a PE'
}

$finder = { param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }
$functions = @($ast.FindAll($finder, $true))
foreach ($fn in $functions) {
    Invoke-Expression $fn.Extent.Text
}
Invoke-Case 'script function contract' {
    Assert-True ($functions.Name -contains 'Test-CommercialSmokeLocalDbAvailable') 'LocalDB probe function missing'
    Assert-True ($functions.Name -contains 'Invoke-CommercialSmokeMssqlE2E') 'MSSQL E2E function missing'
    Assert-True ($functions.Name -contains 'Stop-CommercialSmokeOwnedProcesses') 'owned-process cleanup missing'
    Assert-True ($functions.Name -contains 'Get-CommercialSmokeOwnedIdentity') 'owned-process identity helper missing'
    $explicitFn = @($functions | Where-Object { $_.Name -eq 'Invoke-CommercialSmokeExplicitMssqlE2E' })
    Assert-True ($explicitFn.Count -eq 1) 'explicit MSSQL function missing'
    $explicitBody = $explicitFn[0].Extent.Text
    $ownedExeFn = @($functions | Where-Object { $_.Name -eq 'Invoke-CommercialSmokeOwnedExecutable' })
    Assert-True ($ownedExeFn.Count -eq 1) 'owned executable helper missing'
    $ownedExeBody = $ownedExeFn[0].Extent.Text
    Assert-True ($explicitBody.Contains('Invoke-CommercialSmokeOwnedExecutable')) 'explicit MSSQL path must start the owned executable helper'
    Assert-True ($ownedExeBody.Contains('Start-Process')) 'owned helper must start the E2E with Start-Process'
    Assert-True ($ownedExeBody.Contains('-PassThru')) 'owned helper must keep the Start-Process process object'
    Assert-True ($ownedExeBody.Contains('Stop-CommercialSmokeOwnedProcesses')) 'owned helper must stop owned PIDs'
    Assert-True (-not $explicitBody.Contains('Get-Process -Name')) 'explicit MSSQL path must not stop processes by name'
    Assert-True (-not $explicitBody.Contains("Stop-Process -Force -ErrorAction SilentlyContinue`n")) 'explicit MSSQL path must not use an unnamed Stop-Process'
    $mssqlFn = @($functions | Where-Object { $_.Name -eq 'Invoke-CommercialSmokeMssqlE2E' })
    Assert-True ($mssqlFn.Count -eq 1) 'MSSQL E2E function missing'
    $mssqlBody = $mssqlFn[0].Extent.Text
    Assert-True ($mssqlBody.Contains('Invoke-CommercialSmokeOwnedExecutable')) 'LocalDB path must stop PIDs through the owned helper'
    Assert-True (-not $mssqlBody.Contains('Get-Process -Name')) 'LocalDB path must not stop processes by name'
    Assert-True (-not $smokeText.Contains('Get-Process -Name')) 'commercial-smoke must not select processes by name'
    Assert-True (-not $smokeText.Contains('$args')) 'commercial-smoke must not assign or splat $args'
    Assert-True (-not $smokeText.Contains('@cmdArgs')) 'sqlcmd must not use array splat'
    Assert-True ($smokeText.Contains('Test-CommercialSmokeLiveIdentity')) 'fallback stop must re-check process identity'
    Assert-True ($functions.Name -contains 'Invoke-CommercialSmokeOwnedExecutableOnWindows') 'Windows job-object helper missing'
    Assert-True ($functions.Name -contains 'Format-CommercialSmokeOwnedCommandLine') 'owned command line formatter missing'
    Assert-True ($ownedExeBody.Contains('Invoke-CommercialSmokeOwnedExecutableOnWindows')) 'Windows host must use the job-object helper'
    Assert-True ($ownedExeBody.Contains('TimeoutSeconds')) 'owned helper must accept TimeoutSeconds'
}

Invoke-Case 'owned timeout parameter' {
    $ownedExeFn = @($functions | Where-Object { $_.Name -eq 'Invoke-CommercialSmokeOwnedExecutable' })
    Assert-True ($ownedExeFn.Count -eq 1) 'owned executable helper missing'
    $timeoutParam = @($ownedExeFn[0].Body.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -eq 'TimeoutSeconds' })
    Assert-True ($timeoutParam.Count -eq 1) 'TimeoutSeconds parameter missing'
    Assert-True ($timeoutParam[0].DefaultValue.Extent.Text -eq '600') "TimeoutSeconds default was $($timeoutParam[0].DefaultValue.Extent.Text)"
    foreach ($bad in @(0, -1)) {
        $rejected = $false
        $detail = ''
        try {
            Invoke-CommercialSmokeOwnedExecutable -FilePath 'not-used' -TimeoutSeconds $bad
        } catch {
            $rejected = $true
            $detail = [string]$_.Exception.Message
        }
        Assert-True $rejected "TimeoutSeconds $bad must be rejected before a process starts"
        Assert-True ($detail.Contains('TimeoutSeconds')) "validation message was [$detail]"
    }
    $line = Format-CommercialSmokeOwnedCommandLine -FilePath 'C:\Tools\mxh_client_e2e.exe' -ArgumentList @('--login-host', '127.0.0.1')
    Assert-True ($line -eq '"C:\Tools\mxh_client_e2e.exe" --login-host 127.0.0.1') "command line was [$line]"
    $spaced = Format-CommercialSmokeOwnedCommandLine -FilePath 'C:\Program Files\mxh_client_e2e.exe' -ArgumentList @('a b')
    Assert-True ($spaced -eq '"C:\Program Files\mxh_client_e2e.exe" "a b"') "spaced command line was [$spaced]"
    $quoteRejected = $false
    try {
        Format-CommercialSmokeOwnedCommandLine -FilePath 'C:\Tools\mxh_client_e2e.exe' -ArgumentList @('bad"quote')
    } catch { $quoteRejected = $true }
    Assert-True $quoteRejected 'a double quote in an argument must be rejected'
    $pathRejected = $false
    try {
        Format-CommercialSmokeOwnedCommandLine -FilePath 'C:\Tools\bad"name.exe' -ArgumentList @('a')
    } catch { $pathRejected = $true }
    Assert-True $pathRejected 'a double quote in the executable path must be rejected'
    foreach ($token in @(
        'CreateProcess',
        'CreateJobObject',
        'SetInformationJobObject',
        'AssignProcessToJobObject',
        'ResumeThread',
        'JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE',
        'CREATE_SUSPENDED',
        '0x00002000',
        'STARTF_USESTDHANDLES',
        '0x00020002',
        'COMMERCIAL_SMOKE TIMEOUT (owned executable still running after ',
        'refusing to close a job that contains this process'
    )) {
        Assert-True ($smokeText.Contains($token)) "commercial-smoke missing [$token]"
    }
    Assert-True (-not $smokeText.Contains('GetCurrentProcess')) 'script must not open its own process for the job'
}

Invoke-Case 'job object layout' {
    Initialize-CommercialSmokeProcessJob
    $layout = [Mxh.CommercialSmokeLaunch]::ProbeLayouts()
    Assert-True ($layout -eq 'startup=104 extended=112 job=144 basic=64 io=48 sa=24') "Win32 struct layout was [$layout]"
}

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
    Invoke-Case 'LocalDB probe' {
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
    }

    $absent = New-ToolDir
    $present = New-ToolDir -WithLocalDb

    Invoke-Case 'LocalDB missing' {
    $missingExe = New-BuildDir -WithExe
    $missing = Invoke-Smoke -BuildDir $missingExe.Build -PathValue $absent
    $missingText = Get-SmokeText $missing.Stdout
    Assert-True ($missing.ExitCode -eq 6) "LocalDB missing exit was $($missing.ExitCode); stdout=[$missingText] stderr=[$($missing.Stderr)]"
    Assert-True ($missingText -eq $Unavailable) "LocalDB missing must print only the unavailable line, got [$missingText]"
    $ctestArgs = @(Read-InvokedArgs -Exe (Get-CtestStubPath -ToolDir $absent))
    Assert-True ($null -ne $ctestArgs -and $ctestArgs.Count -gt 0) "LocalDB missing must still run ctest; args were [$($ctestArgs -join '][')]"
    $ctestFilters = @(
        'LoginServerFixture.ScaffoldSmoke',
        'LoginServerFixture.ConnectReceivesDistConnectSuccess',
        'LoginServerFixture.LegacyLoginValidCredsReceivesAck',
        'LoginServerFixture.LegacyLoginInvalidCredsReceivesNack',
        'LoginServerFixtureGolden.GoldenCapturesDistConnectSuccess',
        'LoginServerFixtureGolden.GoldenCapturesLoginAck',
        'LoginServerFixtureGolden.GoldenCapturesLoginNack',
        'EncryptedLoginFixture.EncryptedLoginAckMatchesGolden',
        'WireFormatGolden.RoundTrip_login_ack',
        'WireFormatGolden.RoundTrip_login_nack',
        'WireFormatGolden.RoundTrip_login_request',
        'ResourcePayloadSha256.*',
        'ResourceByteLevel.*',
        'MoxianClientE2E.*',
        'LoginSbsE2E.*'
    )
    $ctestPattern = ($ctestFilters -join '|')
    Assert-True ($ctestPattern.Contains('|')) 'ctest -R pattern must contain a pipe'
    $sawRegex = $false
    for ($i = 0; $i -lt $ctestArgs.Count; $i++) {
        if ($ctestArgs[$i] -ne '-R') { continue }
        $next = ''
        if (($i + 1) -lt $ctestArgs.Count) { $next = [string]$ctestArgs[$i + 1] }
        Assert-True ($next -eq $ctestPattern) "ctest -R must be one argument, was [$next]"
        $sawRegex = $true
    }
    Assert-True $sawRegex "ctest args missing -R: [$($ctestArgs -join '][')]"
    Assert-True (-not ($ctestArgs -contains 'LoginServerFixture.ConnectReceivesDistConnectSuccess')) 'ctest -R regex was split on the pipe'
    Assert-True ((Count-SmokeLine $missingText $Unavailable) -eq 1) 'unavailable line must appear once'
    Assert-True (-not $missingText.Contains('SKIPPED')) 'LocalDB missing must not skip'
    Assert-True (-not $missingText.Contains('PASS')) 'LocalDB missing must not pass'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $missingExe.Exe)) 'LocalDB missing must not invoke E2E'
    Assert-True ([string]::IsNullOrWhiteSpace((Get-SmokeText $missing.Stderr))) "LocalDB missing stderr: $($missing.Stderr)"
    $script:CasePassDetail = ' exit=' + [string]$missing.ExitCode + ' -R=1 argv=' + [string]$ctestArgs.Count
    }

    Invoke-Case 'LocalDB missing without exe' {
    $missingNoExeBuild = New-BuildDir
    $missingNoExe = Invoke-Smoke -BuildDir $missingNoExeBuild.Build -PathValue $absent
    $missingNoExeText = Get-SmokeText $missingNoExe.Stdout
    Assert-True ($missingNoExe.ExitCode -eq 6) "LocalDB missing without exe exit was $($missingNoExe.ExitCode); stdout=[$missingNoExeText]"
    Assert-True ($missingNoExeText -eq $Unavailable) "missing LocalDB without the exe must still be unavailable, got [$missingNoExeText]"
    }

    Invoke-Case 'SkipMssql' {
    $skipBuild = New-BuildDir -WithExe
    $skip = Invoke-Smoke -BuildDir $skipBuild.Build -PathValue $absent -SkipMssql
    $skipText = Get-SmokeText $skip.Stdout
    Assert-True ($skip.ExitCode -eq 0) "SkipMssql exit was $($skip.ExitCode); stdout=[$skipText] stderr=[$($skip.Stderr)]"
    Assert-True ((Count-SmokeLine $skipText $SkipExplicit) -eq 1) "SkipMssql line missing in [$skipText]"
    Assert-True ((Count-SmokeLine $skipText $DoneLine) -eq 1) "SkipMssql must still finish, got [$skipText]"
    Assert-True (-not $skipText.Contains($Unavailable)) 'SkipMssql must not report E2E_UNAVAILABLE'
    Assert-True (-not $skipText.Contains($PassLine)) 'SkipMssql must not report MSSQL pass'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $skipBuild.Exe)) 'SkipMssql must not invoke E2E'
    }

    Invoke-Case 'LocalDB present' {
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
    }

    Invoke-Case 'LocalDB present exe not built' {
    $notBuilt = New-BuildDir
    $notBuiltRun = Invoke-Smoke -BuildDir $notBuilt.Build -PathValue $present
    $notBuiltText = Get-SmokeText $notBuiltRun.Stdout
    Assert-True ($notBuiltRun.ExitCode -eq 0) "not-built exit was $($notBuiltRun.ExitCode); stdout=[$notBuiltText]"
    Assert-True ((Count-SmokeLine $notBuiltText $SkipNotBuilt) -eq 1) "LocalDB present without the exe must skip not-built, got [$notBuiltText]"
    Assert-True (-not $notBuiltText.Contains($Unavailable)) 'not-built skip must not report E2E_UNAVAILABLE'
    Assert-True (-not $notBuiltText.Contains($PassLine)) 'not-built skip must not report MSSQL pass'
    }

    Invoke-Case 'probe override false' {
    $forcedOffBuild = New-BuildDir -WithExe
    $forcedOff = Invoke-Smoke -BuildDir $forcedOffBuild.Build -PathValue $present -ProbeMode 'force-false'
    $forcedOffText = Get-SmokeText $forcedOff.Stdout
    Assert-True ($forcedOff.ExitCode -eq 6) "probe override false exit was $($forcedOff.ExitCode); stdout=[$forcedOffText] stderr=[$($forcedOff.Stderr)]"
    Assert-True ($forcedOffText -eq $Unavailable) "probe override false must be unavailable even when sqllocaldb is on PATH, got [$forcedOffText]"
    Assert-True ($null -eq (Read-InvokedArgs -Exe $forcedOffBuild.Exe)) 'probe override false must not invoke E2E'
    }

    Invoke-Case 'probe override true' {
    $forcedOnBuild = New-BuildDir -WithExe
    $forcedOn = Invoke-Smoke -BuildDir $forcedOnBuild.Build -PathValue $absent -ProbeMode 'force-true'
    $forcedOnText = Get-SmokeText $forcedOn.Stdout
    Assert-True ($forcedOn.ExitCode -eq 0) "probe override true exit was $($forcedOn.ExitCode); stdout=[$forcedOnText] stderr=[$($forcedOn.Stderr)]"
    Assert-True ((Count-SmokeLine $forcedOnText $PassLine) -eq 1) "probe override true must invoke E2E, got [$forcedOnText]"
    $forcedArgs = @(Read-InvokedArgs -Exe $forcedOnBuild.Exe)
    Assert-True ($null -ne $forcedArgs -and ($forcedArgs -join "`n") -eq ($ExpectedArgs -join "`n")) "probe override true args were [$($forcedArgs -join '|')]"
    }

    Invoke-Case 'LocalDB decoy survives cleanup' {
    $localDecoy = $null
    try {
        $localDecoyExe = New-MxhServerNamedExe -ProcessName 'mxh_login_server'
        $localDecoy = Start-Process -FilePath $localDecoyExe -ArgumentList '120' -PassThru -NoNewWindow
        try { $null = $localDecoy.Handle } catch { }
        $localDecoyName = ''
        $localDeadline = [Diagnostics.Stopwatch]::StartNew()
        while ($localDeadline.ElapsedMilliseconds -lt 5000 -and $localDecoyName -ne 'mxh_login_server') {
            try { $localDecoy.Refresh() } catch { }
            $localDecoyName = [string]$localDecoy.ProcessName
            if ($localDecoyName -ne 'mxh_login_server') { Start-Sleep -Milliseconds 50 }
        }
        Assert-True ($localDecoyName -eq 'mxh_login_server') "LocalDB decoy process name was [$localDecoyName]"
        Assert-True (Test-PidStillRunning -ProcessId $localDecoy.Id) 'LocalDB decoy exited before smoke'
        $localDecoyBuild = New-BuildDir -WithExe
        $localDecoyRun = Invoke-Smoke -BuildDir $localDecoyBuild.Build -PathValue $present
        $localDecoyText = Get-SmokeText $localDecoyRun.Stdout
        Assert-True ($localDecoyRun.ExitCode -eq 0) "LocalDB decoy smoke exit was $($localDecoyRun.ExitCode); stdout [$localDecoyText] stderr [$($localDecoyRun.Stderr)]"
        Assert-True ((Count-SmokeLine $localDecoyText $PassLine) -eq 1) "LocalDB decoy run must pass, got [$localDecoyText]"
        Assert-True (Test-PidStillRunning -ProcessId $localDecoy.Id) 'LocalDB cleanup stopped an unrelated mxh_login_server'
    } finally {
        Stop-TestProcess -Process $localDecoy
    }
    }

    Invoke-Case 'port and ctest contract' {
    Assert-True ($smokeText.Contains('--login-port 26001 --agent-port 27001 --map-port 28001 --backend mssql_odbc --init-schema')) 'explicit E2E must pass ports 26001/27001/28001'
    Assert-True (-not $smokeText.Contains('--map12-port')) 'commercial-smoke must not pass --map12-port without --exercise-mapchange'
    foreach ($forbiddenPort in @('16001', '17001', '18001', '36001', '37001', '38001')) {
        Assert-True (-not $smokeText.Contains($forbiddenPort)) "commercial-smoke must not mention port $forbiddenPort"
    }
    Assert-True ($smokeText.Contains('DROP DATABASE [')) 'drop statement must bracket the validated name'
    Assert-True ($smokeText.Contains("'-C','Debug'")) 'ctest -C Debug must stay unchanged'
    }

    $sqlTool = New-ToolDir
    [void](New-SqlCmdStub -Dir $sqlTool)
    $sqlPath = $sqlTool
    if (-not $IsWindows) { $sqlPath = $sqlTool + ':/bin:/usr/bin' }
    $baseNames = @('master', 'model', 'msdb', 'tempdb')
    $goodDb = 'MxE2E_RogSmoke'
    $goodConn = 'backend=mssql_odbc;host=.\SQLEXPRESS;database=' + $goodDb + ';encrypt=no;trust_server_certificate=yes;'
    $expectedExplicit = @(
        '--login-host', '127.0.0.1',
        '--login-port', '26001',
        '--agent-port', '27001',
        '--map-port', '28001',
        '--backend', 'mssql_odbc',
        '--init-schema',
        '--db', $goodConn
    )

    $rejected = @(
        'backend=mssql_odbc;host=.\SQLEXPRESS;encrypt=no;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=Moxiang;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=master;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=[MxE2E_Ok];',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database="MxE2E_Ok";',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok;database=master;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok];DROP DATABASE [master;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok;DROP DATABASE master;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=;',
        'backend=mssql_odbc;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok space;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;host=(local)\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=.\SQLEXPRESS;Server=.\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;Data Source=.\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;addr=.\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;address=.\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;network address=.\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=10.1.1.1\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=db.example\SQLEXPRESS;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=.;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=localhost;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=(local);database=MxE2E_Ok;',
        'backend=mssql_odbc;host=127.0.0.1;database=MxE2E_Ok;',
        'backend=mssql_odbc;host=.\SQLEXPRESS,1433;database=MxE2E_Ok;'
    )
    $rejectBuild = New-BuildDir -WithExe
    foreach ($badConn in $rejected) {
        Invoke-Case ('rejected connection ' + $badConn) {
        $state = New-SqlCmdState -Root $script:Root -Names $baseNames
        $rejectedRun = Invoke-Smoke -BuildDir $rejectBuild.Build -PathValue $sqlPath -MssqlDb $badConn -StubEnv $state.Env
        $rejectedText = Get-SmokeText $rejectedRun.Stdout
        Assert-True ($rejectedRun.ExitCode -ne 0) "rejected connection must be non-zero for [$badConn], exit $($rejectedRun.ExitCode), stdout [$rejectedText], stderr [$($rejectedRun.Stderr)]"
        Assert-True ($rejectedText.Contains('rejected before connect')) "rejected connection missing refuse line for [$badConn], got [$rejectedText]"
        Assert-True (-not $rejectedText.Contains('PASS')) "rejected connection must not pass for [$badConn], got [$rejectedText]"
        Assert-True (-not $rejectedText.Contains($Unavailable)) "rejected connection must not be E2E_UNAVAILABLE for [$badConn]"
        Assert-True ((Read-SqlCmdLog -Path $state.Log).Length -eq 0) "rejected connection must not call sqlcmd for [$badConn]"
        Assert-True ($null -eq (Read-InvokedArgs -Exe $rejectBuild.Exe)) "rejected connection must not invoke E2E for [$badConn]"
        if (Test-Path -LiteralPath ($rejectBuild.Exe + '.invoked')) {
            Remove-Item -LiteralPath ($rejectBuild.Exe + '.invoked') -Force
        }
        }
    }

    $hostCases = @(
        @{ Conn = 'backend=mssql_odbc;database=MxE2E_Ok;encrypt=no;'; Fragment = 'no host= value' },
        @{ Conn = 'backend=mssql_odbc;host=.\SQLEXPRESS;host=.\OTHER;database=MxE2E_Ok;'; Fragment = 'multiple host= values' },
        @{ Conn = 'backend=mssql_odbc;host=.\SQLEXPRESS;Server=localhost\SQLEXPRESS;database=MxE2E_Ok;'; Fragment = 'alternate server key server= is not allowed' },
        @{ Conn = 'backend=mssql_odbc;host=192.168.1.5\SQLEXPRESS;database=MxE2E_Ok;'; Fragment = 'host= must be a local named instance' },
        @{ Conn = 'backend=mssql_odbc;host=.;database=MxE2E_Ok;'; Fragment = 'host= must be a local named instance' },
        @{ Conn = 'backend=mssql_odbc;host=localhost;database=MxE2E_Ok;'; Fragment = 'host= must be a local named instance' },
        @{ Conn = 'backend=mssql_odbc;host=(local);database=MxE2E_Ok;'; Fragment = 'host= must be a local named instance' },
        @{ Conn = 'backend=mssql_odbc;host=127.0.0.1;database=MxE2E_Ok;'; Fragment = 'host= must be a local named instance' }
    )
    foreach ($hostCase in $hostCases) {
        Invoke-Case ('host guard ' + $hostCase.Fragment) {
        $state = New-SqlCmdState -Root $script:Root -Names $baseNames
        $hostRun = Invoke-Smoke -BuildDir $rejectBuild.Build -PathValue $sqlPath -MssqlDb $hostCase.Conn -StubEnv $state.Env
        $hostText = Get-SmokeText $hostRun.Stdout
        Assert-True ($hostRun.ExitCode -eq 1) "host guard exit was $($hostRun.ExitCode) for [$($hostCase.Conn)], stdout [$hostText] stderr [$($hostRun.Stderr)]"
        Assert-True ($hostText.Contains('rejected before connect')) "host guard missing refuse line for [$($hostCase.Conn)], got [$hostText]"
        Assert-True ($hostText.Contains($hostCase.Fragment)) "host guard fragment [$($hostCase.Fragment)] missing for [$($hostCase.Conn)], got [$hostText]"
        Assert-True ((Read-SqlCmdLog -Path $state.Log).Length -eq 0) "host guard must not call sqlcmd for [$($hostCase.Conn)]"
        Assert-True ($null -eq (Read-InvokedArgs -Exe $rejectBuild.Exe)) "host guard must not invoke E2E for [$($hostCase.Conn)]"
        if (Test-Path -LiteralPath ($rejectBuild.Exe + '.invoked')) {
            Remove-Item -LiteralPath ($rejectBuild.Exe + '.invoked') -Force
        }
        }
    }
    Invoke-Case 'local named instance' {
    Assert-True (Test-CommercialSmokeLocalNamedInstance -Value '.\SQLEXPRESS') '.\SQLEXPRESS must be a local named instance'
    Assert-True (Test-CommercialSmokeLocalNamedInstance -Value '(local)\SQLEXPRESS') '(local)\SQLEXPRESS must be a local named instance'
    Assert-True (Test-CommercialSmokeLocalNamedInstance -Value 'localhost\SQLEXPRESS') 'localhost\SQLEXPRESS must be a local named instance'
    Assert-True (Test-CommercialSmokeLocalNamedInstance -Value '127.0.0.1\SQLEXPRESS') '127.0.0.1\SQLEXPRESS must be a local named instance'
    Assert-True (Test-CommercialSmokeLocalNamedInstance -Value 'LOCALHOST\Inst_1') 'LOCALHOST\Inst_1 must be a local named instance'
    Assert-True (-not (Test-CommercialSmokeLocalNamedInstance -Value '.')) 'bare dot must be rejected'
    Assert-True (-not (Test-CommercialSmokeLocalNamedInstance -Value 'localhost')) 'bare localhost must be rejected'
    Assert-True (-not (Test-CommercialSmokeLocalNamedInstance -Value '(local)')) 'bare (local) must be rejected'
    Assert-True (-not (Test-CommercialSmokeLocalNamedInstance -Value '127.0.0.1')) 'bare 127.0.0.1 must be rejected'
    }

    Invoke-Case 'owned report token' {
    Assert-True (Test-CommercialSmokeConnectionStringLiteral -ConnectionString $goodConn) 'connection string without whitespace or quotes must be accepted'
    $windowsEntry = [pscustomobject]@{ Id = 9; StartTicks = [int64]638000000000000000; KernelStart = [int64]0 }
    $windowsToken = Get-CommercialSmokeOwnedReportToken -Entry $windowsEntry -WindowsHost $true
    Assert-True ($windowsToken -eq '638000000000000000') "Windows owned token was [$windowsToken], expected StartTicks"
    $linuxEntry = [pscustomobject]@{ Id = 9; StartTicks = [int64]11; KernelStart = [int64]22 }
    $linuxToken = Get-CommercialSmokeOwnedReportToken -Entry $linuxEntry -WindowsHost $false
    Assert-True ($linuxToken -eq '22') "non-Windows owned token was [$linuxToken], expected KernelStart"
    }

    $quoteFragment = 'whitespace or quote characters are not allowed'
    $unsafeConnections = @(
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok; encrypt=no',
        'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok;pwd="secret"',
        "backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok;pwd='secret'",
        "backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Ok;`tencrypt=no"
    )
    foreach ($unsafeConn in $unsafeConnections) {
        Invoke-Case ('connection string rejects whitespace or quote ' + $unsafeConn) {
        Assert-True (-not (Test-CommercialSmokeConnectionStringLiteral -ConnectionString $unsafeConn)) "predicate must reject [$unsafeConn]"
        $unsafeState = New-SqlCmdState -Root $script:Root -Names $baseNames
        $unsafeRun = Invoke-Smoke -BuildDir $rejectBuild.Build -PathValue $sqlPath -MssqlDb $unsafeConn -StubEnv $unsafeState.Env
        $unsafeText = Get-SmokeText $unsafeRun.Stdout
        Assert-True ($unsafeRun.ExitCode -ne 0) "unsafe connection must be non-zero for [$unsafeConn], exit $($unsafeRun.ExitCode), stdout [$unsafeText], stderr [$($unsafeRun.Stderr)]"
        Assert-True ($unsafeText.Contains('rejected before connect')) "unsafe connection missing refuse line for [$unsafeConn], got [$unsafeText]"
        Assert-True ($unsafeText.Contains($quoteFragment)) "unsafe connection missing quote/whitespace fragment for [$unsafeConn], got [$unsafeText]"
        Assert-True (-not $unsafeText.Contains('PASS')) "unsafe connection must not pass for [$unsafeConn], got [$unsafeText]"
        Assert-True ((Read-SqlCmdLog -Path $unsafeState.Log).Length -eq 0) "unsafe connection must not call sqlcmd for [$unsafeConn]"
        Assert-True ($null -eq (Read-InvokedArgs -Exe $rejectBuild.Exe)) "unsafe connection must not invoke E2E for [$unsafeConn]"
        if (Test-Path -LiteralPath ($rejectBuild.Exe + '.invoked')) {
            Remove-Item -LiteralPath ($rejectBuild.Exe + '.invoked') -Force
        }
        }
    }

    Invoke-Case 'pre-existing database' {
    $existsState = New-SqlCmdState -Root $script:Root -Names ($baseNames + @('MxE2E_Already'))
    $existsBuild = New-BuildDir -WithExe
    $existsConn = 'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_Already;encrypt=no;'
    $existsRun = Invoke-Smoke -BuildDir $existsBuild.Build -PathValue $sqlPath -MssqlDb $existsConn -StubEnv $existsState.Env
    $existsText = Get-SmokeText $existsRun.Stdout
    $existsLog = Read-SqlCmdLog -Path $existsState.Log
    $existsBefore = Get-DatabaseBlock -Text $existsText -When 'before' -Server '.\SQLEXPRESS'
    Assert-True ($existsRun.ExitCode -ne 0) "pre-existing database exit was $($existsRun.ExitCode); stdout [$existsText] stderr [$($existsRun.Stderr)]"
    Assert-True ($existsText.Contains('MSSQL_E2E REFUSED (database MxE2E_Already already exists; refusing to touch it)')) "pre-existing refuse line missing in [$existsText]"
    Assert-True ($existsBefore.Complete -and ($existsBefore.Names -contains 'MxE2E_Already')) "pre-existing BEFORE list missing the name in [$existsText]"
    Assert-True ($existsBefore.Line.StartsWith('[.\SQLEXPRESS] databases before:')) "pre-existing list must name the instance, got [$($existsBefore.Line)]"
    Assert-True (-not $existsText.Contains('databases after:')) 'pre-existing database must not print an AFTER list'
    Assert-True (-not $existsLog.Contains('DROP DATABASE')) "pre-existing database must not be dropped, log [$existsLog]"
    Assert-SqlCmdTargetsInstance -Log $existsLog -Server '.\SQLEXPRESS' -ExpectedCalls 1
    Assert-True ($null -eq (Read-InvokedArgs -Exe $existsBuild.Exe)) 'pre-existing database must not invoke E2E'
    Assert-True (-not $existsText.Contains('PASS')) 'pre-existing database must not pass'
    }

    Invoke-Case 'explicit MSSQL success' {
    $okState = New-SqlCmdState -Root $script:Root -Names $baseNames
    $okBuild = New-BuildDir -WithExe
    $okRun = Invoke-Smoke -BuildDir $okBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $okState.Env
    $okText = Get-SmokeText $okRun.Stdout
    Write-SmokeChildStdout -Text $okText
    $okLog = Read-SqlCmdLog -Path $okState.Log
    $okBefore = Get-DatabaseBlock -Text $okText -When 'before' -Server '.\SQLEXPRESS'
    $okAfter = Get-DatabaseBlock -Text $okText -When 'after' -Server '.\SQLEXPRESS'
    Assert-True ($okRun.ExitCode -eq 0) "explicit MSSQL success exit was $($okRun.ExitCode); stdout [$okText] stderr [$($okRun.Stderr)]"
    Assert-True ((Count-SmokeLine $okText 'MSSQL_E2E PASS (explicit -MssqlDb, one command)') -eq 1) "explicit pass line missing in [$okText]"
    Assert-True ((Count-SmokeLine $okText $DoneLine) -eq 1) "explicit success must finish smoke, got [$okText]"
    Assert-True (-not $okText.Contains($Unavailable)) 'explicit MSSQL must not report E2E_UNAVAILABLE'
    Assert-True (-not $okText.Contains($PassLine)) 'explicit MSSQL must not report the LocalDB pass line'
    Assert-True ($okBefore.Complete -and -not ($okBefore.Names -contains $goodDb)) "BEFORE list must not contain $goodDb, got [$($okBefore.Names -join ',')]"
    Assert-True ($okAfter.Complete -and -not ($okAfter.Names -contains $goodDb)) "AFTER list must not contain $goodDb, got [$($okAfter.Names -join ',')]"
    Assert-True ((Count-SmokeLine $okText '[.\SQLEXPRESS] databases before: master model msdb tempdb') -eq 1) "before list line missing in [$okText]"
    Assert-True ((Count-SmokeLine $okText '[.\SQLEXPRESS] databases after: master model msdb tempdb') -eq 1) "after list line missing in [$okText]"
    Assert-True ((Count-SmokeLine $okText "MSSQL_E2E DROPPED (database $goodDb)") -eq 1) "drop confirmation missing in [$okText]"
    Assert-True ($okLog.Contains('DROP DATABASE [' + $goodDb + ']')) "drop SQL missing in [$okLog]"
    Assert-True (-not $okLog.Contains("`n-U`n") -and -not $okLog.Contains("`n-P`n")) "sqlcmd log must not pass a SQL login, log [$okLog]"
    Assert-SqlCmdTargetsInstance -Log $okLog -Server '.\SQLEXPRESS' -ExpectedCalls 3
    $okArgs = @(Read-InvokedArgs -Exe $okBuild.Exe)
    Assert-True ($null -ne $okArgs -and ($okArgs -join "`n") -eq ($expectedExplicit -join "`n")) "explicit E2E args were [$($okArgs -join '|')]"
    }

    Invoke-Case 'E2E failure exit' {
    $failState = New-SqlCmdState -Root $script:Root -Names $baseNames
    # Pass 4 at build time. Writing .exitcode after the fixed PE is ignored
    # on Windows; that image never reads the file and always returns 0.
    $failBuild = New-BuildDir -WithExe -ExitCode 4
    $failRun = Invoke-Smoke -BuildDir $failBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $failState.Env
    $failText = Get-SmokeText $failRun.Stdout
    $failLog = Read-SqlCmdLog -Path $failState.Log
    Assert-True ($failRun.ExitCode -ne 0) "E2E failure must be non-zero, exit $($failRun.ExitCode), stdout [$failText] stderr [$($failRun.Stderr)]"
    Assert-True ($failText.Contains('MSSQL_E2E FAILED (mxh_client_e2e exit 4)')) "E2E failure line missing in [$failText]"
    Assert-True ($failLog.Contains('DROP DATABASE [' + $goodDb + ']')) "E2E failure must still drop, log [$failLog]"
    Assert-True ((Count-SmokeLine $failText "MSSQL_E2E DROPPED (database $goodDb)") -eq 1) "E2E failure drop confirmation missing in [$failText]"
    Assert-True (-not $failText.Contains('COMMERCIAL_SMOKE PASS')) 'E2E failure must not finish as smoke pass'
    Assert-True (-not $failText.Contains('MSSQL_E2E PASS')) 'E2E failure must not report MSSQL pass'
    $failAfter = Get-DatabaseBlock -Text $failText -When 'after' -Server '.\SQLEXPRESS'
    Assert-SqlCmdTargetsInstance -Log $failLog -Server '.\SQLEXPRESS' -ExpectedCalls 3
    Assert-True ($failAfter.Complete -and -not ($failAfter.Names -contains $goodDb)) 'E2E failure AFTER list must not contain the database'
    }

    Invoke-Case 'drop failure' {
    $dropState = New-SqlCmdState -Root $script:Root -Names $baseNames -DropExit 1
    $dropBuild = New-BuildDir -WithExe
    $dropRun = Invoke-Smoke -BuildDir $dropBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $dropState.Env
    $dropText = Get-SmokeText $dropRun.Stdout
    Write-SmokeChildStdout -Text $dropText
    $dropLog = Read-SqlCmdLog -Path $dropState.Log
    Assert-True ($dropRun.ExitCode -ne 0) "drop failure must be non-zero, exit $($dropRun.ExitCode), stdout [$dropText] stderr [$($dropRun.Stderr)]"
    Assert-True ($dropText.Contains("MSSQL_E2E DROP FAILED (database $goodDb; sqlcmd exit 1)")) "drop failure line missing in [$dropText]"
    Assert-True ($dropLog.Contains('DROP DATABASE [' + $goodDb + ']')) "drop failure must still issue DROP, log [$dropLog]"
    Assert-SqlCmdTargetsInstance -Log $dropLog -Server '.\SQLEXPRESS' -ExpectedCalls 3
    Assert-True ($dropText.Contains('[.\SQLEXPRESS] databases before:')) "drop failure before list must name the instance, got [$dropText]"
    Assert-True ($dropText.Contains('[.\SQLEXPRESS] databases after:')) "drop failure after list must name the instance, got [$dropText]"
    Assert-True (-not $dropText.Contains('MSSQL_E2E PASS')) 'drop failure must not report MSSQL pass'
    Assert-True (-not $dropText.Contains('COMMERCIAL_SMOKE PASS')) 'drop failure must not finish as smoke pass'
    }

    Invoke-Case 'database still present after drop' {
    $keepState = New-SqlCmdState -Root $script:Root -Names $baseNames -KeepAfterDrop
    $keepBuild = New-BuildDir -WithExe
    $keepRun = Invoke-Smoke -BuildDir $keepBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $keepState.Env
    $keepText = Get-SmokeText $keepRun.Stdout
    $keepAfter = Get-DatabaseBlock -Text $keepText -When 'after' -Server '.\SQLEXPRESS'
    Assert-True ($keepText.Contains('[.\SQLEXPRESS] databases after:')) "still-present after list must name the instance, got [$keepText]"
    Assert-True ($keepRun.ExitCode -ne 0) "database still present after drop must be non-zero, exit $($keepRun.ExitCode), stdout [$keepText]"
    Assert-True ($keepText.Contains("MSSQL_E2E DROP FAILED (database $goodDb still present after DROP DATABASE)")) "still-present line missing in [$keepText]"
    Assert-True ($keepAfter.Complete -and ($keepAfter.Names -contains $goodDb)) 'AFTER list must still show the database when drop did not remove it'
    Assert-True (-not $keepText.Contains('COMMERCIAL_SMOKE PASS')) 'still-present drop must not finish as smoke pass'
    }

    Invoke-Case 'sys.databases failure' {
    $queryState = New-SqlCmdState -Root $script:Root -Names $baseNames -QueryExit 1
    $queryBuild = New-BuildDir -WithExe
    $queryRun = Invoke-Smoke -BuildDir $queryBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $queryState.Env
    $queryText = Get-SmokeText $queryRun.Stdout
    $queryLog = Read-SqlCmdLog -Path $queryState.Log
    Assert-True ($queryRun.ExitCode -ne 0) "sys.databases failure must be non-zero, stdout [$queryText] stderr [$($queryRun.Stderr)]"
    Assert-True ($queryText.Contains('not touching any database')) "query failure must say it did not touch a database, got [$queryText]"
    Assert-True (-not $queryLog.Contains('DROP DATABASE')) "query failure must not drop, log [$queryLog]"
    Assert-True ($null -eq (Read-InvokedArgs -Exe $queryBuild.Exe)) 'query failure must not invoke E2E'
    }

    Invoke-Case 'SkipMssql conflicts with MssqlDb' {
    $conflictState = New-SqlCmdState -Root $script:Root -Names $baseNames
    $conflictBuild = New-BuildDir -WithExe
    $conflictRun = Invoke-Smoke -BuildDir $conflictBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -SkipMssql -StubEnv $conflictState.Env
    $conflictText = Get-SmokeText $conflictRun.Stdout
    Assert-True ($conflictRun.ExitCode -ne 0) "SkipMssql with MssqlDb exit was $($conflictRun.ExitCode); stdout [$conflictText] stderr [$($conflictRun.Stderr)]"
    Assert-True ($conflictText -eq 'MSSQL_E2E REFUSED (-SkipMssql and -MssqlDb conflict)') "conflict line was [$conflictText]"
    Assert-True ((Read-SqlCmdLog -Path $conflictState.Log).Length -eq 0) 'conflict must not call sqlcmd'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $conflictBuild.Exe)) 'conflict must not invoke E2E'
    }

    Invoke-Case 'single-config exe' {
    $singleBuild = New-BuildDir -WithExe -Layout Single
    $singleState = New-SqlCmdState -Root $script:Root -Names $baseNames
    $singleRun = Invoke-Smoke -BuildDir $singleBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $singleState.Env
    $singleText = Get-SmokeText $singleRun.Stdout
    Assert-True ($singleRun.ExitCode -eq 0) "single-config exit was $($singleRun.ExitCode); stdout [$singleText] stderr [$($singleRun.Stderr)]"
    Assert-True ($singleText.Contains('MSSQL_E2E EXE ' + $singleBuild.Single)) "single-config path was not logged in [$singleText]"
    Assert-True ($null -ne (Read-InvokedArgs -Exe $singleBuild.Single)) 'single-config stub was not invoked'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $singleBuild.Multi)) 'single-config run must not invoke the Debug stub'
    }

    Invoke-Case 'Debug-layout exe' {
    $debugBuild = New-BuildDir -WithExe -Layout Debug
    $debugState = New-SqlCmdState -Root $script:Root -Names $baseNames
    $debugRun = Invoke-Smoke -BuildDir $debugBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $debugState.Env
    $debugText = Get-SmokeText $debugRun.Stdout
    Assert-True ($debugRun.ExitCode -eq 0) "Debug-layout exit was $($debugRun.ExitCode); stdout [$debugText] stderr [$($debugRun.Stderr)]"
    Assert-True ($debugText.Contains('MSSQL_E2E EXE ' + $debugBuild.Multi)) "Debug path was not logged in [$debugText]"
    Assert-True ($null -ne (Read-InvokedArgs -Exe $debugBuild.Multi)) 'Debug stub was not invoked'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $debugBuild.Single)) 'Debug run must not invoke the single-config stub'
    }

    Invoke-Case 'missing exe does not drop' {
    $absentBuild = New-BuildDir
    $absentState = New-SqlCmdState -Root $script:Root -Names $baseNames
    $absentRun = Invoke-Smoke -BuildDir $absentBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $absentState.Env
    $absentText = Get-SmokeText $absentRun.Stdout
    $absentLog = Read-SqlCmdLog -Path $absentState.Log
    Assert-True ($absentRun.ExitCode -ne 0) "missing exe must be non-zero, exit $($absentRun.ExitCode); stdout [$absentText] stderr [$($absentRun.Stderr)]"
    Assert-True ($absentText.Contains('MSSQL_E2E FAILED (mxh_client_e2e not built; database was absent and was not dropped)')) "not-built failure missing in [$absentText]"
    Assert-True ($absentText.Contains('MSSQL_E2E EXE CANDIDATE ' + $absentBuild.Single)) "single-config candidate missing in [$absentText]"
    Assert-True ($absentText.Contains('MSSQL_E2E EXE CANDIDATE ' + $absentBuild.Multi)) "Debug candidate missing in [$absentText]"
    Assert-True (-not $absentText.Contains('MSSQL_E2E SKIPPED')) 'missing exe must not skip'
    Assert-True (-not $absentText.Contains('MSSQL_E2E PASS')) 'missing exe must not pass'
    Assert-True (-not $absentLog.Contains('DROP DATABASE')) "missing exe must not drop, log [$absentLog]"
    Assert-True ($null -eq (Read-InvokedArgs -Exe $absentBuild.Single)) 'missing single-config exe must not be invoked'
    Assert-True ($null -eq (Read-InvokedArgs -Exe $absentBuild.Multi)) 'missing Debug exe must not be invoked'
    }

    Invoke-Case 'owned process cleanup' {
    # Cleanup identity: real processes, no SQL Server. Both executables use
    # mxh_*_server names from the old name-based kill list. Only the PID
    # passed to cleanup may exit.
    $ownedExe = New-MxhServerNamedExe -ProcessName 'mxh_map_server_CHINA'
    $decoyExe = New-MxhServerNamedExe -ProcessName 'mxh_login_server'
    $ownedProc = $null
    $decoyProc = $null
    $quickProc = $null
    $fallbackProc = $null
    try {
        $ownedProc = Start-Process -FilePath $ownedExe -ArgumentList '60' -PassThru -NoNewWindow
        $decoyProc = Start-Process -FilePath $decoyExe -ArgumentList '60' -PassThru -NoNewWindow
        try { $null = $ownedProc.Handle } catch { }
        try { $null = $decoyProc.Handle } catch { }
        $ownedIdentity = $null
        $decoyIdentity = $null
        $deadline = [Diagnostics.Stopwatch]::StartNew()
        while ($deadline.ElapsedMilliseconds -lt 5000 -and ($null -eq $ownedIdentity -or $null -eq $decoyIdentity)) {
            if ($null -eq $ownedIdentity) { $ownedIdentity = Get-CommercialSmokeOwnedIdentity -Process $ownedProc }
            if ($null -eq $decoyIdentity) { $decoyIdentity = Get-CommercialSmokeOwnedIdentity -Process $decoyProc }
            if ($null -eq $ownedIdentity -or $null -eq $decoyIdentity) { Start-Sleep -Milliseconds 50 }
        }
        Assert-True ($null -ne $ownedIdentity) 'owned sleeper identity was not recorded'
        Assert-True ($null -ne $decoyIdentity) 'decoy sleeper identity was not recorded'
        Assert-True ($ownedProc.ProcessName -eq 'mxh_map_server_CHINA') "owned process name was [$($ownedProc.ProcessName)]"
        Assert-True ($decoyProc.ProcessName -eq 'mxh_login_server') "decoy process name was [$($decoyProc.ProcessName)]"
        $childIds = @(Get-CommercialSmokeChildProcessIds -ParentId $PID)
        Assert-True ($childIds -contains [int]$ownedIdentity.Id) "child discovery missed owned pid $($ownedIdentity.Id); found [$($childIds -join ',')]"
        Assert-True ($childIds -contains [int]$decoyIdentity.Id) "child discovery missed decoy pid $($decoyIdentity.Id); found [$($childIds -join ',')]"
        if (-not $IsWindows) {
            $statIds = New-Object System.Collections.Generic.List[int]
            Add-CommercialSmokeProcStatChildren -ParentId $PID -Ids $statIds
            $statFound = @($statIds)
            Assert-True ($statFound -contains [int]$ownedIdentity.Id) "status PPid scan missed owned pid $($ownedIdentity.Id); found [$($statFound -join ',')]"
            Assert-True ($statFound -contains [int]$decoyIdentity.Id) "status PPid scan missed decoy pid $($decoyIdentity.Id); found [$($statFound -join ',')]"
            $kernel = Get-CommercialSmokeKernelStart -ProcessId ([int]$ownedIdentity.Id)
            Assert-True ($kernel -gt 0) "kernel starttime missing for owned pid $($ownedIdentity.Id)"
            Assert-True ([int64]$ownedIdentity.KernelStart -eq $kernel) "identity kernel starttime was $($ownedIdentity.KernelStart), live is $kernel"
        }
        Assert-True (Test-CommercialSmokeLiveIdentity -ProcessId ([int]$ownedIdentity.Id) -StartTicks ([int64]$ownedIdentity.StartTicks) -KernelStart ([int64]$ownedIdentity.KernelStart)) 'recorded identity must match the live process'
        Assert-True (-not (Test-CommercialSmokeLiveIdentity -ProcessId ([int]$ownedIdentity.Id) -StartTicks ([int64]1) -KernelStart ([int64]1))) 'a mismatched clock must not match the live process'
        $fallbackProc = Start-Process -FilePath $ownedExe -ArgumentList '60' -PassThru -NoNewWindow
        try { $null = $fallbackProc.Handle } catch { }
        $fallbackIdentity = $null
        $fallbackDeadline = [Diagnostics.Stopwatch]::StartNew()
        while ($fallbackDeadline.ElapsedMilliseconds -lt 5000 -and $null -eq $fallbackIdentity) {
            $fallbackIdentity = Get-CommercialSmokeOwnedIdentity -Process $fallbackProc
            if ($null -eq $fallbackIdentity) { Start-Sleep -Milliseconds 50 }
        }
        Assert-True ($null -ne $fallbackIdentity) 'fallback sleeper identity was not recorded'
        Stop-CommercialSmokeOwnedProcessFallback -ProcessId ([int]$fallbackIdentity.Id) -StartTicks ([int64]1) -KernelStart ([int64]1)
        Assert-True (Test-PidStillRunning -ProcessId ([int]$fallbackIdentity.Id)) 'fallback stop must not signal a PID whose identity does not match'
        Stop-CommercialSmokeOwnedProcessFallback -ProcessId ([int]$fallbackIdentity.Id) -StartTicks ([int64]$fallbackIdentity.StartTicks) -KernelStart ([int64]$fallbackIdentity.KernelStart)
        $fallbackProc.WaitForExit(5000) | Out-Null
        Assert-True (-not (Test-PidStillRunning -ProcessId ([int]$fallbackIdentity.Id))) 'fallback stop must signal the PID when identity still matches'
        Stop-CommercialSmokeOwnedProcess -ProcessId $decoyIdentity.Id -StartTicks ([int64]1)
        Stop-CommercialSmokeOwnedProcesses -Owned @([pscustomobject]@{ Id = $ownedIdentity.Id; StartTicks = ([int64]1) })
        Assert-True (Test-PidStillRunning -ProcessId $ownedIdentity.Id) 'wrong StartTime must not stop the owned PID'
        Assert-True (Test-PidStillRunning -ProcessId $decoyIdentity.Id) 'wrong StartTime must not stop the decoy'
        Stop-CommercialSmokeOwnedProcesses -Owned @($ownedIdentity)
        $ownedProc.WaitForExit(5000) | Out-Null
        Assert-True (-not (Test-PidStillRunning -ProcessId $ownedIdentity.Id)) 'cleanup did not stop the PID it was given'
        Assert-True (Test-PidStillRunning -ProcessId $decoyIdentity.Id) 'cleanup stopped an unrelated mxh_login_server decoy'
        Stop-CommercialSmokeOwnedProcesses -Owned @($ownedIdentity)
        $quickProc = Start-Process -FilePath $ownedExe -ArgumentList '0' -PassThru -NoNewWindow
        if (-not $quickProc.WaitForExit(5000)) { throw 'FAIL: already-exited fixture did not exit' }
        Stop-CommercialSmokeOwnedProcess -ProcessId $quickProc.Id -StartTicks ([int64]1)
        Stop-CommercialSmokeOwnedProcesses -Owned @([pscustomobject]@{ Id = $quickProc.Id; StartTicks = ([int64]1) })
        Stop-CommercialSmokeOwnedProcesses -Owned @()
        Stop-CommercialSmokeOwnedProcesses -Owned $null
        Stop-CommercialSmokeOwnedProcess -ProcessId 0 -StartTicks ([int64]1)
    } finally {
        Stop-TestProcess -Process $ownedProc
        Stop-TestProcess -Process $decoyProc
        Stop-TestProcess -Process $quickProc
        Stop-TestProcess -Process $fallbackProc
    }
    }

    Invoke-Case 'MSSQL path scoped cleanup' {
    $pathDecoy = $null
    $pathChildPid = 0
    $spawnBuild = $null
    try {
        $pathDecoyExe = New-MxhServerNamedExe -ProcessName 'mxh_login_server'
        $pathDecoy = Start-Process -FilePath $pathDecoyExe -ArgumentList '120' -PassThru -NoNewWindow
        try { $null = $pathDecoy.Handle } catch { }
        $pathDecoyName = ''
        $pathDeadline = [Diagnostics.Stopwatch]::StartNew()
        while ($pathDeadline.ElapsedMilliseconds -lt 5000 -and $pathDecoyName -ne 'mxh_login_server') {
            try { $pathDecoy.Refresh() } catch { }
            $pathDecoyName = [string]$pathDecoy.ProcessName
            if ($pathDecoyName -ne 'mxh_login_server') { Start-Sleep -Milliseconds 50 }
        }
        Assert-True ($pathDecoyName -eq 'mxh_login_server') "path decoy process name was [$pathDecoyName]"
        Assert-True (Test-PidStillRunning -ProcessId $pathDecoy.Id) 'path decoy exited before smoke'
        $spawnBuild = New-BuildDir -WithExe -SpawnChild
        $spawnState = New-SqlCmdState -Root $script:Root -Names $baseNames
        $spawnRun = Invoke-Smoke -BuildDir $spawnBuild.Build -PathValue $sqlPath -MssqlDb $goodConn -StubEnv $spawnState.Env
        $spawnText = Get-SmokeText $spawnRun.Stdout
        Assert-True ($spawnRun.ExitCode -eq 0) "scoped cleanup smoke exit was $($spawnRun.ExitCode); stdout [$spawnText] stderr [$($spawnRun.Stderr)]"
        Assert-True ((Count-SmokeLine $spawnText 'MSSQL_E2E PASS (explicit -MssqlDb, one command)') -eq 1) "scoped cleanup pass line missing in [$spawnText]"
        Assert-True ((Count-SmokeLine $spawnText "MSSQL_E2E DROPPED (database $goodDb)") -eq 1) "scoped cleanup must still drop the database, got [$spawnText]"
        $spawnArgs = @(Read-InvokedArgs -Exe $spawnBuild.Exe)
        Assert-True ($null -ne $spawnArgs -and ($spawnArgs -join "`n") -eq ($expectedExplicit -join "`n")) "scoped cleanup E2E args were [$($spawnArgs -join '|')]"
        $childLog = $spawnBuild.Exe + '.children'
        Assert-True (Test-Path -LiteralPath $childLog) 'E2E stub did not record the child it started'
        $childRaw = [IO.File]::ReadAllText($childLog).Trim()
        Assert-True ([int]::TryParse($childRaw, [ref]$pathChildPid) -and $pathChildPid -gt 0) "child PID text was [$childRaw]"
        $childPpid = ''
        $childState = ''
        if (-not $IsWindows) {
            $childPpid = Get-ProcStatusField -ProcessId $pathChildPid -Field 'PPid:'
            $childState = Get-ProcStatusField -ProcessId $pathChildPid -Field 'State:'
        }
        $ownedLine = ''
        foreach ($line in ($spawnText -split "`n")) {
            if ($line.Trim().StartsWith('MSSQL_E2E OWNED ')) { $ownedLine = $line.Trim() }
        }
        $ownedIds = @()
        $ownedPrefix = 'MSSQL_E2E OWNED '
        if ($ownedLine.StartsWith($ownedPrefix)) {
            foreach ($token in @($ownedLine.Substring($ownedPrefix.Length).Split(','))) {
                $pidText = ($token -split ':')[0]
                if ($pidText -match '^\d+$') { $ownedIds += $pidText }
            }
        }
        Assert-True (-not (Test-PidStillRunning -ProcessId $pathChildPid)) "cleanup left the E2E child $pathChildPid running (ppid=$childPpid state=$childState) [$ownedLine]"
        Assert-True ($ownedIds -contains [string]$pathChildPid) "owned PID list did not include child $pathChildPid [$ownedLine]"
        $childToken = ''
        if ($ownedLine.StartsWith($ownedPrefix)) {
            foreach ($token in @($ownedLine.Substring($ownedPrefix.Length).Split(','))) {
                $pieces = @($token.Split(':'))
                if ($pieces.Count -ge 2 -and $pieces[0] -eq [string]$pathChildPid) {
                    $childToken = [string]$pieces[1]
                }
            }
        }
        Assert-True ($childToken -match '^\d+$' -and [int64]$childToken -gt 0) "owned identity token must be the non-zero clock for pid $pathChildPid [$ownedLine]"
        Assert-True (Test-PidStillRunning -ProcessId $pathDecoy.Id) 'MSSQL E2E cleanup stopped an unrelated mxh_login_server decoy'
    } finally {
        Stop-TestProcess -Process $pathDecoy
        if ($pathChildPid -gt 0) {
            try {
                if (Test-PidStillRunning -ProcessId $pathChildPid) {
                    Stop-Process -Id $pathChildPid -Force -ErrorAction SilentlyContinue
                }
            } catch { }
        }
    }
    }

    Invoke-Case 'ctest must resolve to ctest.exe' {
    Assert-True ($functions.Name -contains 'Test-CommercialSmokeCtestExecutableExtension') 'ctest extension predicate missing'
    Assert-True (Test-CommercialSmokeCtestExecutableExtension -Path 'C:\Tools\ctest.exe') '.exe must be accepted'
    Assert-True (Test-CommercialSmokeCtestExecutableExtension -Path 'C:\Tools\ctest.EXE') '.EXE must be accepted'
    Assert-True (-not (Test-CommercialSmokeCtestExecutableExtension -Path 'C:\Tools\ctest.cmd')) '.cmd must be refused'
    Assert-True (-not (Test-CommercialSmokeCtestExecutableExtension -Path 'C:\Tools\ctest.bat')) '.bat must be refused'
    if ($IsWindows) {
        # ctest.cmd is first on PATH. A later ctest.exe must not be used.
        # The cmd writes a marker only if it is started. Do not place the
        # fixed PE and then write .exitcode; this case does not use that PE.
        $cmdDir = Join-Path $script:Root ('ctest-cmd-' + [guid]::NewGuid().ToString('n'))
        New-Item -ItemType Directory -Force -Path $cmdDir | Out-Null
        $cmdPath = Join-Path $cmdDir 'ctest.cmd'
        $marker = Join-Path $cmdDir 'ctest-cmd.marker'
        $cmdBody = "@echo off`r`n> `"%~dp0ctest-cmd.marker`" echo invoked`r`nexit /b 0`r`n"
        [IO.File]::WriteAllBytes($cmdPath, [Text.Encoding]::ASCII.GetBytes($cmdBody))
        $laterExeDir = New-ToolDir
        $cmdFirstPath = $cmdDir + ';' + $laterExeDir
        $cmdBuild = New-BuildDir
        $cmdRun = Invoke-Smoke -BuildDir $cmdBuild.Build -PathValue $cmdFirstPath
        $cmdText = Get-SmokeText $cmdRun.Stdout
        $cmdExpected = 'COMMERCIAL_SMOKE REFUSED (ctest must resolve to ctest.exe on Windows, got ' + $cmdPath + ')'
        Assert-True ($cmdRun.ExitCode -ne 0) "ctest.cmd must be refused, exit $($cmdRun.ExitCode), stdout [$cmdText] stderr [$($cmdRun.Stderr)]"
        Assert-True ($cmdText -eq $cmdExpected) "refusal line was [$cmdText], expected [$cmdExpected], stderr [$($cmdRun.Stderr)]"
        Assert-True (-not (Test-Path -LiteralPath $marker)) 'ctest.cmd stub was invoked'
        Assert-True ($null -eq (Read-InvokedArgs -Exe (Join-Path $laterExeDir 'ctest.exe'))) 'a later ctest.exe must not be started'
    }
    }

    Invoke-Case 'stop pid checks start time' {
        $target = $null
        $targetPid = 0
        $targetStart = [int64]0
        try {
            $exe = New-MxhServerNamedExe -ProcessName 'mxh_agent_server'
            $target = Start-Process -FilePath $exe -ArgumentList '30' -PassThru -NoNewWindow
            try { $null = $target.Handle } catch { }
            $targetPid = [int]$target.Id
            $deadline = [Diagnostics.Stopwatch]::StartNew()
            while ($deadline.ElapsedMilliseconds -lt 5000 -and $targetStart -le 0) {
                $targetStart = Get-TestProcessStartTicks -ProcessId $targetPid
                if ($targetStart -le 0) { Start-Sleep -Milliseconds 50 }
            }
            Assert-True ($targetStart -gt 0) 'start time was not recorded for the sleeper'
            Assert-True (Test-PidStillRunning -ProcessId $targetPid) 'sleeper exited before the identity check'
            Stop-TestPidIfSameStart -ProcessId $targetPid -StartTicks ([int64]0)
            Assert-True (Test-PidStillRunning -ProcessId $targetPid) 'a missing start time must not stop the PID'
            Stop-TestPidIfSameStart -ProcessId $targetPid -StartTicks ([int64]1)
            Assert-True (Test-PidStillRunning -ProcessId $targetPid) 'a mismatched start time must not stop the PID'
            Stop-TestPidIfSameStart -ProcessId $targetPid -StartTicks $targetStart
            $gone = [Diagnostics.Stopwatch]::StartNew()
            while ($gone.ElapsedMilliseconds -lt 5000 -and (Test-PidStillRunning -ProcessId $targetPid)) {
                Start-Sleep -Milliseconds 50
            }
            Assert-True (-not (Test-PidStillRunning -ProcessId $targetPid)) 'the recorded start time must stop that PID'
        } finally {
            Stop-TestPidIfSameStart -ProcessId $targetPid -StartTicks $targetStart
        }
    }

    if ($IsWindows) {
        Invoke-Case 'owned executable timeout' {
            $outside = $null
            $outsideStart = [int64]0
            $ownedPid = 0
            $ownedStart = [int64]0
            try {
                $sleeper = New-MxhServerNamedExe -ProcessName 'mxh_map_server_CHINA'
                $warm = Invoke-OwnedCapture -FilePath $sleeper -ArgumentList @('0') -TimeoutSeconds 30
                Assert-True ($null -ne $warm.Result) 'warmup did not return a result'
                Assert-True ([int]$warm.Result.ExitCode -eq 0) "warmup exit $($warm.Result.ExitCode) text [$($warm.Text)]"
                $outside = Start-Process -FilePath $sleeper -ArgumentList '120' -PassThru -NoNewWindow
                try { $null = $outside.Handle } catch { }
                $outsideName = ''
                $nameDeadline = [Diagnostics.Stopwatch]::StartNew()
                while ($nameDeadline.ElapsedMilliseconds -lt 5000 -and $outsideName -ne 'mxh_map_server_CHINA') {
                    try { $outside.Refresh() } catch { }
                    $outsideName = [string]$outside.ProcessName
                    if ($outsideName -ne 'mxh_map_server_CHINA') { Start-Sleep -Milliseconds 50 }
                }
                Assert-True ($outsideName -eq 'mxh_map_server_CHINA') "outside process name was [$outsideName]"
                Assert-True (Test-PidStillRunning -ProcessId $outside.Id) 'outside process exited before the timeout run'
                $outsideStart = Get-TestProcessStartTicks -ProcessId ([int]$outside.Id)
                Assert-True ($outsideStart -gt 0) 'outside start time was not recorded'
                $sw = [Diagnostics.Stopwatch]::StartNew()
                $cap = Invoke-OwnedCapture -FilePath $sleeper -ArgumentList @('60') -TimeoutSeconds 3
                $sw.Stop()
                Assert-True ($null -ne $cap.Result) 'timeout run did not return a result'
                Assert-True ($sw.Elapsed.TotalSeconds -gt 1) 'timeout returned before the deadline'
                Assert-True ($sw.Elapsed.TotalSeconds -lt 40) "timeout run took $($sw.Elapsed.TotalSeconds)s"
                Assert-True ($cap.Text.Contains('COMMERCIAL_SMOKE TIMEOUT (owned executable still running after 3s)')) "timeout line missing [$($cap.Text)]"
                Assert-True ([int]$cap.Result.ExitCode -ne 0) "timeout exit was $($cap.Result.ExitCode)"
                # Index the generic list. @() enumeration throws "Argument types do not match".
                $ownedEntries = $cap.Result.Owned
                $ownedCount = 0
                if ($null -ne $ownedEntries) {
                    try { $ownedCount = [int]$ownedEntries.Count } catch { $ownedCount = 0 }
                }
                for ($ownedIndex = 0; $ownedIndex -lt $ownedCount; $ownedIndex++) {
                    $entry = $ownedEntries[$ownedIndex]
                    if ($null -eq $entry) { continue }
                    $ownedPid = [int]$entry.Id
                    try { $ownedStart = [int64]$entry.StartTicks } catch { $ownedStart = [int64]0 }
                    break
                }
                if ($ownedPid -gt 0 -and $ownedStart -le 0) {
                    $ownedStart = Get-TestProcessStartTicks -ProcessId $ownedPid
                }
                Assert-True ($ownedPid -gt 0 -and $ownedPid -ne [int]$outside.Id) "owned pid was $ownedPid outside $($outside.Id)"
                $gone = [Diagnostics.Stopwatch]::StartNew()
                while ($gone.ElapsedMilliseconds -lt 5000 -and (Test-PidStillRunning -ProcessId $ownedPid)) {
                    Start-Sleep -Milliseconds 50
                }
                Assert-True (-not (Test-PidStillRunning -ProcessId $ownedPid)) "timed out pid $ownedPid still running"
                Assert-True (Test-PidStillRunning -ProcessId $outside.Id) 'timeout cleanup stopped a same-named process outside the job'
                $elapsedText = '{0:0.0}' -f $sw.Elapsed.TotalSeconds
                $script:CasePassDetail = ' elapsed=' + $elapsedText + 's'
            } finally {
                $outsideId = 0
                if ($null -ne $outside) {
                    try { $outsideId = [int]$outside.Id } catch { $outsideId = 0 }
                }
                Stop-TestPidIfSameStart -ProcessId $outsideId -StartTicks $outsideStart
                Stop-TestPidIfSameStart -ProcessId $ownedPid -StartTicks $ownedStart
            }
        }

        Invoke-Case 'orphan job close' {
            # Synthetic parent exits after spawning a long-lived child that
            # inherits stdout. A same-named process started outside the job
            # must survive. No original game bytes.
            $outside = $null
            $outsideStart = [int64]0
            $orphanPid = 0
            $orphanStart = [int64]0
            try {
                $dir = Join-Path $script:Root ('orphan-' + [guid]::NewGuid().ToString('n'))
                New-Item -ItemType Directory -Force -Path $dir | Out-Null
                $holderCs = Join-Path $dir 'holder.cs'
                $parentCs = Join-Path $dir 'parent.cs'
                $holder = Join-Path $dir 'mxh_login_server.exe'
                $parentExe = Join-Path $dir 'mxh_client_e2e.exe'
                @'
using System;
using System.Threading;
class Holder {
    static int Main(string[] args) {
        int seconds = 180;
        if (args.Length > 0) {
            int parsed;
            if (int.TryParse(args[0], out parsed) && parsed >= 0) seconds = parsed;
        }
        Console.Out.WriteLine("MXH_ORPHAN_HOLDING_STDOUT");
        Console.Out.Flush();
        if (seconds > 0) Thread.Sleep(seconds * 1000);
        return 0;
    }
}
'@ | Set-Content -LiteralPath $holderCs -Encoding ascii
                @'
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
class OrphanParent {
    const int STARTF_USESTDHANDLES = 0x00000100;
    const uint CREATE_NO_WINDOW = 0x08000000;
    const int STD_INPUT_HANDLE = -10;
    const int STD_OUTPUT_HANDLE = -11;
    const int STD_ERROR_HANDLE = -12;
    [StructLayout(LayoutKind.Sequential)]
    struct STARTUPINFO {
        public int cb;
        public IntPtr lpReserved;
        public IntPtr lpDesktop;
        public IntPtr lpTitle;
        public int dwX;
        public int dwY;
        public int dwXSize;
        public int dwYSize;
        public int dwXCountChars;
        public int dwYCountChars;
        public int dwFillAttribute;
        public int dwFlags;
        public short wShowWindow;
        public short cbReserved2;
        public IntPtr lpReserved2;
        public IntPtr hStdInput;
        public IntPtr hStdOutput;
        public IntPtr hStdError;
    }
    [StructLayout(LayoutKind.Sequential)]
    struct PROCESS_INFORMATION {
        public IntPtr hProcess;
        public IntPtr hThread;
        public int dwProcessId;
        public int dwThreadId;
    }
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern bool CreateProcessW(string app, StringBuilder cmd, IntPtr pa, IntPtr ta, bool inherit, uint flags, IntPtr env, string dir, ref STARTUPINFO si, out PROCESS_INFORMATION pi);
    [DllImport("kernel32.dll")]
    static extern IntPtr GetStdHandle(int n);
    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(IntPtr h);
    static int Main(string[] args) {
        string exe = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
        File.WriteAllText(exe + ".invoked", string.Join("\n", args));
        Console.Out.WriteLine("MXH_OWNED_STDOUT_MARKER");
        Console.Out.Flush();
        string folder = Path.GetDirectoryName(exe);
        string holder = Path.Combine(folder, "mxh_login_server.exe");
        var si = new STARTUPINFO();
        si.cb = Marshal.SizeOf(typeof(STARTUPINFO));
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        var command = new StringBuilder("\"" + holder + "\" 180", 2048);
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(holder, command, IntPtr.Zero, IntPtr.Zero, true, CREATE_NO_WINDOW, IntPtr.Zero, folder, ref si, out pi)) {
            File.WriteAllText(exe + ".spawnerr", Marshal.GetLastWin32Error().ToString());
            return 22;
        }
        File.WriteAllText(exe + ".orphanpid", pi.dwProcessId.ToString());
        if (pi.hThread != IntPtr.Zero) CloseHandle(pi.hThread);
        if (pi.hProcess != IntPtr.Zero) CloseHandle(pi.hProcess);
        return 0;
    }
}
'@ | Set-Content -LiteralPath $parentCs -Encoding ascii
                Invoke-WindowsCsc -Source $holderCs -Output $holder
                Invoke-WindowsCsc -Source $parentCs -Output $parentExe
                $warm = Invoke-OwnedCapture -FilePath $holder -ArgumentList @('0') -TimeoutSeconds 30
                Assert-True ($null -ne $warm.Result -and [int]$warm.Result.ExitCode -eq 0) "holder warmup exit text [$($warm.Text)]"
                $outside = Start-Process -FilePath $holder -ArgumentList '120' -PassThru -NoNewWindow
                try { $null = $outside.Handle } catch { }
                $outsideName = ''
                $nameDeadline = [Diagnostics.Stopwatch]::StartNew()
                while ($nameDeadline.ElapsedMilliseconds -lt 5000 -and $outsideName -ne 'mxh_login_server') {
                    try { $outside.Refresh() } catch { }
                    $outsideName = [string]$outside.ProcessName
                    if ($outsideName -ne 'mxh_login_server') { Start-Sleep -Milliseconds 50 }
                }
                Assert-True ($outsideName -eq 'mxh_login_server') "outside process name was [$outsideName]"
                Assert-True (Test-PidStillRunning -ProcessId $outside.Id) 'outside process exited before the orphan run'
                $outsideStart = Get-TestProcessStartTicks -ProcessId ([int]$outside.Id)
                Assert-True ($outsideStart -gt 0) 'outside start time was not recorded'
                $sw = [Diagnostics.Stopwatch]::StartNew()
                $cap = Invoke-OwnedCapture -FilePath $parentExe -ArgumentList @('orphan-parent') -TimeoutSeconds 90
                $sw.Stop()
                $spawnErr = ''
                $spawnErrPath = $parentExe + '.spawnerr'
                if (Test-Path -LiteralPath $spawnErrPath) { $spawnErr = [IO.File]::ReadAllText($spawnErrPath).Trim() }
                Assert-True ($null -ne $cap.Result) 'orphan run did not return a result'
                Assert-True ($sw.Elapsed.TotalSeconds -lt 40) "orphan run took $($sw.Elapsed.TotalSeconds)s"
                Assert-True (-not $cap.Text.Contains('COMMERCIAL_SMOKE TIMEOUT')) "orphan run timed out [$($cap.Text)]"
                Assert-True ([int]$cap.Result.ExitCode -eq 0) "parent exit $($cap.Result.ExitCode) spawnerr [$spawnErr] text [$($cap.Text)]"
                Assert-True ($cap.Text.Contains('MXH_OWNED_STDOUT_MARKER')) "stdout marker missing [$($cap.Text)]"
                $parentArgs = @(Read-InvokedArgs -Exe $parentExe)
                Assert-True ($parentArgs -contains 'orphan-parent') "parent args were [$($parentArgs -join '|')]"
                $orphanRaw = [IO.File]::ReadAllText($parentExe + '.orphanpid').Trim()
                Assert-True ([int]::TryParse($orphanRaw, [ref]$orphanPid) -and $orphanPid -gt 0) "orphan pid text [$orphanRaw]"
                $orphanStart = Get-TestProcessStartTicks -ProcessId $orphanPid
                Assert-True ($orphanPid -ne [int]$outside.Id) 'orphan pid collided with the outside process'
                $gone = [Diagnostics.Stopwatch]::StartNew()
                while ($gone.ElapsedMilliseconds -lt 5000 -and (Test-PidStillRunning -ProcessId $orphanPid)) {
                    Start-Sleep -Milliseconds 50
                }
                Assert-True (-not (Test-PidStillRunning -ProcessId $orphanPid)) "orphan $orphanPid still running after job close"
                Assert-True (Test-PidStillRunning -ProcessId $outside.Id) 'same-named process outside the job was stopped'
                $inOwned = $false
                $ownedEntries = $cap.Result.Owned
                $ownedCount = 0
                if ($null -ne $ownedEntries) {
                    try { $ownedCount = [int]$ownedEntries.Count } catch { $ownedCount = 0 }
                }
                for ($ownedIndex = 0; $ownedIndex -lt $ownedCount; $ownedIndex++) {
                    $entry = $ownedEntries[$ownedIndex]
                    if ($null -eq $entry) { continue }
                    if ([int]$entry.Id -eq $orphanPid) { $inOwned = $true }
                }
                $elapsedText = '{0:0.0}' -f $sw.Elapsed.TotalSeconds
                $script:CasePassDetail = ' elapsed=' + $elapsedText + 's inOwned=' + [int]$inOwned
            } finally {
                $outsideId = 0
                if ($null -ne $outside) {
                    try { $outsideId = [int]$outside.Id } catch { $outsideId = 0 }
                }
                Stop-TestPidIfSameStart -ProcessId $outsideId -StartTicks $outsideStart
                Stop-TestPidIfSameStart -ProcessId $orphanPid -StartTicks $orphanStart
            }
        }
    }
} finally {
    Remove-Item -LiteralPath $script:Root -Recurse -Force -ErrorAction SilentlyContinue
}

if ($script:Failures.Count -gt 0) {
    Write-Output ('FAILED ' + $script:Failures.Count + ' case(s)')
    exit 1
}
Write-Output 'PASS: commercial-smoke MSSQL E2E availability'
Write-Output 'PASS: commercial-smoke -MssqlDb guard'
Write-Output ('PASS: assertions ' + $script:AssertPass)
