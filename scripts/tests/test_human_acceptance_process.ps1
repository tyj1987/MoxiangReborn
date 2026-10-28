$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\human-acceptance-process.ps1')

function Assert($condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
}

$launcher = [pscustomobject]@{
    pid = 4100; parentPid = 1000; startTicks = [int64]638000000000000000
    exe = 'C:\build\MoxianLauncher.exe'; sha256 = 'aa55'
}
$client = [pscustomobject]@{
    pid = 4200; parentPid = 4100; startTicks = [int64]638000000000000100
    exe = 'C:\build\mxh_client.exe'; sha256 = 'bb66'
}
Assert (Test-OwnedSnapshot $launcher $launcher) 'same launcher identity accepted'
Assert (Test-LauncherChildSnapshot $launcher $client $client.exe $client.sha256) `
    'direct child with exact path/hash and later ticks accepted'

foreach ($field in @('pid', 'parentPid', 'startTicks', 'exe', 'sha256')) {
    $changed = $client.psobject.Copy()
    switch ($field) {
        pid { $changed.pid = 4201 }
        parentPid { $changed.parentPid = 9999 }
        startTicks { $changed.startTicks = $launcher.startTicks }
        exe { $changed.exe = 'C:\other\mxh_client.exe' }
        sha256 { $changed.sha256 = 'wrong' }
    }
    Assert (-not (Test-OwnedSnapshot $client $changed)) "ownership rejects changed $field"
    if ($field -ne 'pid') {
        Assert (-not (Test-LauncherChildSnapshot $launcher $changed $client.exe $client.sha256)) `
            "child predicate rejects changed $field"
    }
}

# Rebind only in this test scope. These mocks prove the helper makes a
# graceful-close call and never reaches a real process or Stop-Process.
function Test-ExactProcess { param($Entry) return $true }
$script:fakeProcess = [pscustomobject]@{
    closed = $false; waited = $false
    StartTime = [datetime]::new($client.startTicks, [DateTimeKind]::Utc)
    Path = $client.exe
}
$script:fakeProcess | Add-Member -MemberType ScriptMethod -Name CloseMainWindow `
    -Value { $this.closed = $true; return $true }
$script:fakeProcess | Add-Member -MemberType ScriptMethod -Name WaitForExit `
    -Value { param($milliseconds) $this.waited = ($milliseconds -eq 5000); return $true }
function Get-Process { param([int]$Id) return $script:fakeProcess }
$result = Stop-OwnedProcess $client
Assert ($result.result -eq 'closed_normally') 'graceful close result'
Assert ($script:fakeProcess.closed -and $script:fakeProcess.waited) 'close then bounded wait'

$script:fakeProcess.closed = $false
$script:fakeProcess.StartTime = $script:fakeProcess.StartTime.AddTicks(1)
$result = Stop-OwnedProcess $client
Assert ($result.result -eq 'identity_changed_left_running') 'PID reuse is rejected at process handle'
Assert (-not $script:fakeProcess.closed) 'changed process was not closed'

function Test-ExactProcess { param($Entry) return $false }
function Get-Process { throw 'Get-Process must not run after identity mismatch' }
$result = Stop-OwnedProcess $client
Assert ($result.result -eq 'identity_mismatch_or_exited') 'identity mismatch leaves process untouched'
Write-Output 'PASS: human acceptance process ownership fixture'
