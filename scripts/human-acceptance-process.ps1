# Pure ownership predicate and guarded process operations for human acceptance.
function Test-OwnedSnapshot {
    param($Expected, $Actual)
    if ($null -eq $Expected -or $null -eq $Actual) { return $false }
    if ([int]$Expected.pid -ne [int]$Actual.pid -or
        [int64]$Expected.startTicks -ne [int64]$Actual.startTicks -or
        [int]$Expected.parentPid -ne [int]$Actual.parentPid) { return $false }
    if (-not [StringComparer]::OrdinalIgnoreCase.Equals(
            [string]$Expected.exe, [string]$Actual.exe)) { return $false }
    return [StringComparer]::OrdinalIgnoreCase.Equals(
        [string]$Expected.sha256, [string]$Actual.sha256)
}

function Test-LauncherChildSnapshot {
    param($Launcher, $Candidate, [string]$ClientExe, [string]$ClientSha256)
    if ($null -eq $Launcher -or $null -eq $Candidate) { return $false }
    if ([int]$Candidate.parentPid -ne [int]$Launcher.pid -or
        [int64]$Candidate.startTicks -le [int64]$Launcher.startTicks) { return $false }
    if (-not [StringComparer]::OrdinalIgnoreCase.Equals(
            [string]$Candidate.exe, $ClientExe)) { return $false }
    return [StringComparer]::OrdinalIgnoreCase.Equals(
        [string]$Candidate.sha256, $ClientSha256)
}

function Get-ProcessSnapshot {
    param([int]$ProcessId)
    try {
        $process = Get-Process -Id $ProcessId -ErrorAction Stop
        $details = Get-CimInstance -ClassName Win32_Process `
            -Filter "ProcessId = $ProcessId" -ErrorAction Stop
        if ($null -eq $details -or
            [string]::IsNullOrWhiteSpace([string]$process.Path) -or
            [string]::IsNullOrWhiteSpace([string]$details.ExecutablePath)) { return $null }
        $path = (Resolve-Path -LiteralPath $process.Path -ErrorAction Stop).Path
        $cimPath = (Resolve-Path -LiteralPath $details.ExecutablePath -ErrorAction Stop).Path
        if (-not [StringComparer]::OrdinalIgnoreCase.Equals($path, $cimPath)) {
            return $null
        }
        [pscustomobject]@{
            pid = $ProcessId
            parentPid = [int]$details.ParentProcessId
            startTicks = [int64]$process.StartTime.ToUniversalTime().Ticks
            exe = $path
            sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    } catch { return $null }
}

function Test-ExactProcess {
    param($Entry)
    return Test-OwnedSnapshot -Expected $Entry `
        -Actual (Get-ProcessSnapshot -ProcessId ([int]$Entry.pid))
}

function Stop-OwnedProcess {
    param($Entry, [int]$WaitMilliseconds = 5000)
    if (-not (Test-ExactProcess -Entry $Entry)) {
        return [pscustomobject]@{ pid = $Entry.pid; result = 'identity_mismatch_or_exited' }
    }
    try {
        $process = Get-Process -Id ([int]$Entry.pid) -ErrorAction Stop
        # The PID may have been reused after Test-ExactProcess. This process
        # handle must still refer to the same recorded start and executable.
        if ([int64]$process.StartTime.ToUniversalTime().Ticks -ne
                [int64]$Entry.startTicks -or
            -not [StringComparer]::OrdinalIgnoreCase.Equals(
                [string]$process.Path, [string]$Entry.exe)) {
            return [pscustomobject]@{ pid = $Entry.pid; result = 'identity_changed_left_running' }
        }
        if (-not $process.CloseMainWindow()) {
            return [pscustomobject]@{ pid = $Entry.pid; result = 'no_main_window_left_running' }
        }
        if ($process.WaitForExit($WaitMilliseconds)) {
            return [pscustomobject]@{ pid = $Entry.pid; result = 'closed_normally' }
        }
        return [pscustomobject]@{ pid = $Entry.pid; result = 'close_timeout_left_running' }
    } catch {
        return [pscustomobject]@{ pid = $Entry.pid; result = 'close_error_left_running' }
    }
}
