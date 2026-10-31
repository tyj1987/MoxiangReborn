# Synthetic checks for the parameterized release scripts.
# No network, no real server binaries. Ports are chosen at runtime.
#   pwsh -NoProfile -File scripts/tests/test-release-scripts.ps1
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$scriptDir = Join-Path (Join-Path $repoRoot 'deploy') 'scripts'
$startScript = Join-Path $scriptDir 'start_release.ps1'
$rollbackScript = Join-Path $scriptDir 'rollback_release.ps1'
$healthScript = Join-Path $scriptDir 'healthcheck_release.ps1'
$commonScript = Join-Path $scriptDir 'release_common.ps1'
$pwsh = (Get-Process -Id $PID).Path
$publicHost = '192.0.2.10'
$passed = 0
$failed = 0

# Pause-loop ELF (x86_64). Image path stays on this file; it does not exec another binary.
$script:SleepElf = 'f0VMRgIBAQAAAAAAAAAAAAIAPgABAAAAeABAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAEAAOAABAAAAAAAAAAEAAAAFAAAAAAAAAAAAAAAAAEAAAAAAAAAAQAAAAAAAgQAAAAAAAACBAAAAAAAAAAAQAAAAAAAAuCIAAAAPBev3'

function Test-ForbiddenSyntheticPort {
    param([int]$Port)
    if ($Port -eq 16001 -or $Port -eq 17001 -or $Port -eq 18001) { return $true }
    if ($Port -eq 26001 -or $Port -eq 27001 -or $Port -eq 28001) { return $true }
    if ($Port -ge 36001 -and $Port -le 38001) { return $true }
    if ($Port -eq 9000 -or $Port -eq 10000 -or $Port -eq 10001) { return $true }
    if ($Port -eq 18002 -or $Port -eq 18003 -or $Port -eq 18004) { return $true }
    return $false
}

function Assert-ReleaseScripts {
    foreach ($path in @($startScript, $rollbackScript, $healthScript, $commonScript)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw ('missing ' + $path)
        }
    }
}

function Invoke-ReleaseScript {
    param(
        [Parameter(Mandatory = $true)][string]$File,
        [string[]]$ArgumentList
    )
    $output = & $pwsh -NoProfile -NonInteractive -File $File @ArgumentList 2>&1 | Out-String
    return [pscustomobject]@{
        Code = [int]$LASTEXITCODE
        Text = [string]$output
    }
}

function New-FreePortList {
    param([int]$Count)
    $chosen = @()
    $held = @()
    try {
        $guard = 0
        while (@($chosen).Count -lt $Count) {
            $guard++
            if ($guard -gt 80) { throw 'no safe ephemeral port' }
            $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
            $listener.Start()
            $port = [int]([Net.IPEndPoint]$listener.LocalEndpoint).Port
            if ((Test-ForbiddenSyntheticPort -Port $port) -or (@($chosen) -contains $port)) {
                $listener.Stop()
                continue
            }
            $chosen += $port
            $held += $listener
        }
        return @($chosen)
    } finally {
        foreach ($listener in $held) {
            try { $listener.Stop() } catch { }
        }
    }
}

function New-HeldPort {
    $guard = 0
    while ($guard -lt 40) {
        $guard++
        $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
        $listener.Start()
        $port = [int]([Net.IPEndPoint]$listener.LocalEndpoint).Port
        if (Test-ForbiddenSyntheticPort -Port $port) {
            $listener.Stop()
            continue
        }
        return [pscustomobject]@{ Port = $port; Listener = $listener }
    }
    throw 'no safe ephemeral port to hold'
}

function Join-Rel {
    param(
        [string]$Root,
        [string[]]$Parts
    )
    $path = $Root
    foreach ($part in @($Parts)) { $path = Join-Path $path $part }
    return $path
}

function Set-ExecutableBit {
    param([string]$Path)
    if ($env:OS -eq 'Windows_NT') { return }
    & /bin/chmod '755' $Path
    if ($LASTEXITCODE -ne 0) { throw ('chmod failed for ' + $Path) }
}

function New-SleepExecutable {
    param([string]$Path)
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    if ($env:OS -eq 'Windows_NT') {
        $cs = Join-Path ([IO.Path]::GetTempPath()) ('mxh-sleep-' + [guid]::NewGuid().ToString('N') + '.cs')
        $source = "using System.Threading;`r`nclass Program { static void Main(string[] args) { Thread.Sleep(System.Threading.Timeout.Infinite); } }`r`n"
        [IO.File]::WriteAllText($cs, $source)
        $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
        if (-not (Test-Path -LiteralPath $csc -PathType Leaf)) { throw ('missing ' + $csc) }
        & $csc /nologo /t:exe ('/out:' + $Path) $cs
        $code = $LASTEXITCODE
        Remove-Item -LiteralPath $cs -Force -ErrorAction SilentlyContinue
        if ($code -ne 0 -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
            throw 'dummy executable was not created'
        }
        return
    }
    $bytes = [Convert]::FromBase64String($script:SleepElf)
    [IO.File]::WriteAllBytes($Path, $bytes)
    Set-ExecutableBit -Path $Path
}

function Copy-SleepExecutable {
    param(
        [string]$Source,
        [string]$Destination
    )
    $parent = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    Set-ExecutableBit -Path $Destination
}

function New-ReleaseTree {
    param([string]$Root)
    $map = Join-Rel -Root $Root -Parts @('modern', 'build', 'tools', 'MoxianMapServer', 'mxh_map_server_CHINA.exe')
    $agent = Join-Rel -Root $Root -Parts @('modern', 'build', 'tools', 'MoxianAgentServer', 'mxh_agent_server_CHINA.exe')
    $login = Join-Rel -Root $Root -Parts @('modern', 'build', 'tools', 'MoxianLoginServer', 'mxh_login_server.exe')
    New-SleepExecutable -Path $map
    Copy-SleepExecutable -Source $map -Destination $agent
    Copy-SleepExecutable -Source $map -Destination $login
}

function Get-LiveImagePath {
    param([int]$ProcessId)
    $procLink = '/proc/' + $ProcessId + '/exe'
    if (Test-Path -LiteralPath $procLink) {
        return [string](Get-Item -LiteralPath $procLink).Target
    }
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if ($null -ne $process) { return [string]$process.Path }
    return ''
}

function Test-ProcessAlive {
    param([int]$ProcessId)
    try {
        $process = Get-Process -Id $ProcessId -ErrorAction Stop
        return -not $process.HasExited
    } catch {
        return $false
    }
}

function Stop-TrackedPid {
    param([int]$ProcessId)
    if ($ProcessId -le 0) { return }
    try { Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue } catch { }
}

function Get-PortArgs {
    param([int[]]$Ports)
    if (@($Ports).Count -ne 5) { throw 'expected five ephemeral ports' }
    foreach ($port in @($Ports)) {
        if (Test-ForbiddenSyntheticPort -Port $port) { throw ('refusing synthetic port ' + $port) }
    }
    return @(
        '-PublicHost', $publicHost,
        '-LoginPort', [string]$Ports[0],
        '-AgentPort', [string]$Ports[1],
        '-MapPort', [string]$Ports[2],
        '-Map12Port', [string]$Ports[3],
        '-Map2Port', [string]$Ports[4]
    )
}

function Add-ReleaseCase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Body
    )
    try {
        & $Body
        $script:passed++
        Write-Host ('RELEASE_CASE ' + $Name + ' PASS')
    } catch {
        $script:failed++
        Write-Host ('RELEASE_CASE ' + $Name + ' FAIL')
        Write-Host ([string]$_.Exception.Message)
        if ($_.InvocationInfo -and $_.InvocationInfo.PositionMessage) {
            Write-Host $_.InvocationInfo.PositionMessage
        }
    }
}

Add-ReleaseCase -Name 'missing-public-host' -Body {
    Assert-ReleaseScripts
    $dir = Join-Path ([IO.Path]::GetTempPath()) ('mxh-rel-host-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $dir | Out-Null
    try {
        $result = Invoke-ReleaseScript -File $startScript -ArgumentList @('-ReleaseDir', $dir)
        if ($result.Code -eq 0) { throw 'missing PublicHost exited 0' }
        if ($result.Text -notmatch 'PublicHost') {
            throw ('missing PublicHost did not name the parameter: ' + $result.Text)
        }
    } finally {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Add-ReleaseCase -Name 'legacy-port' -Body {
    Assert-ReleaseScripts
    $dir = Join-Path ([IO.Path]::GetTempPath()) ('mxh-rel-legacy-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $dir | Out-Null
    try {
        $free = @(New-FreePortList -Count 4)
        $legacyPorts = @(9000, 10000, 10001)
        $slot = 0
        foreach ($legacy in $legacyPorts) {
            $ports = New-Object System.Collections.Generic.List[int]
            for ($index = 0; $index -lt 5; $index++) {
                if ($index -eq $slot) { $ports.Add([int]$legacy) }
                else {
                    $pick = $free[0]
                    if ($index -gt $slot) { $pick = $free[$index - 1] }
                    if ($index -lt $slot) { $pick = $free[$index] }
                    $ports.Add([int]$pick)
                }
            }
            $argList = @(
                '-PublicHost', $publicHost,
                '-ReleaseDir', $dir,
                '-LoginPort', [string]$ports[0],
                '-AgentPort', [string]$ports[1],
                '-MapPort', [string]$ports[2],
                '-Map12Port', [string]$ports[3],
                '-Map2Port', [string]$ports[4]
            )
            $result = Invoke-ReleaseScript -File $startScript -ArgumentList $argList
            if ($result.Code -eq 0) { throw ('legacy port ' + $legacy + ' exited 0') }
            if ($result.Text -notmatch 'legacy port refused') {
                throw ('legacy port ' + $legacy + ' was not refused: ' + $result.Text)
            }
            $slot++
        }
    } finally {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Add-ReleaseCase -Name 'port-in-use' -Body {
    Assert-ReleaseScripts
    $dir = Join-Path ([IO.Path]::GetTempPath()) ('mxh-rel-busy-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $dir | Out-Null
    $held = $null
    try {
        $held = New-HeldPort
        $others = @(New-FreePortList -Count 4)
        $ports = @([int]$held.Port) + @($others)
        $argList = @('-ReleaseDir', $dir) + @(Get-PortArgs -Ports $ports)
        $result = Invoke-ReleaseScript -File $startScript -ArgumentList $argList
        if ($result.Code -eq 0) { throw 'busy port exited 0' }
        $expected = 'port already listening: ' + $held.Port + ' owner=' + $PID
        if ($result.Text -notmatch [regex]::Escape($expected)) {
            throw ('busy port did not report owner: ' + $result.Text)
        }
        if (Test-Path -LiteralPath (Join-Path $dir 'run-state.json')) {
            throw 'busy port wrote a run-state json'
        }
    } finally {
        if ($null -ne $held) {
            try { $held.Listener.Stop() } catch { }
        }
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Add-ReleaseCase -Name 'rollback-recorded-pids-only' -Body {
    Assert-ReleaseScripts
    $parent = if (-not [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) { $env:RUNNER_TEMP } else { [IO.Path]::GetTempPath() }
    $root = Join-Path $parent ('mxh-rel-rollback-' + [guid]::NewGuid().ToString('N'))
    $release = Join-Path $root 'release'
    $previous = Join-Path $root 'previous'
    $decoyDir = Join-Path $root 'decoy'
    New-Item -ItemType Directory -Path $release, $previous, $decoyDir | Out-Null
    $tracked = New-Object System.Collections.Generic.List[int]
    $savedDb = [Environment]::GetEnvironmentVariable('MXH_DATABASE_CONFIG', 'Process')
    try {
        New-ReleaseTree -Root $release
        New-ReleaseTree -Root $previous
        $decoy = Join-Path $decoyDir 'mxh_map_server_CHINA.exe'
        $map = Join-Rel -Root $release -Parts @('modern', 'build', 'tools', 'MoxianMapServer', 'mxh_map_server_CHINA.exe')
        Copy-SleepExecutable -Source $map -Destination $decoy
        if ((Split-Path -Leaf $decoy) -ne (Split-Path -Leaf $map)) { throw 'decoy name differs' }
        $ports = @(New-FreePortList -Count 5)
        $portArgs = @(Get-PortArgs -Ports $ports)
        $dbPath = Join-Path $root 'unused.db'
        [Environment]::SetEnvironmentVariable('MXH_DATABASE_CONFIG', ('backend=sqlite;path=' + $dbPath), 'Process')
        $started = Invoke-ReleaseScript -File $startScript -ArgumentList (@('-ReleaseDir', $release) + $portArgs)
        if ($started.Code -ne 0) { throw ('start failed: ' + $started.Text) }
        $statePath = Join-Path $release 'run-state.json'
        if (-not (Test-Path -LiteralPath $statePath -PathType Leaf)) { throw 'run-state json was not written' }
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        $rows = @($state.processes)
        if ($rows.Count -ne 5) { throw ('expected 5 recorded processes, found ' + $rows.Count) }
        $releaseFull = [IO.Path]::GetFullPath($release)
        foreach ($row in $rows) {
            $processId = [int]$row.pid
            $tracked.Add($processId)
            if (-not (Test-ProcessAlive -ProcessId $processId)) { throw ('recorded pid is dead: ' + $processId) }
            $image = Get-LiveImagePath -ProcessId $processId
            $expected = [IO.Path]::GetFullPath([string]$row.exe)
            if ($image -ne $expected) { throw ('image path differs for pid ' + $processId + ': ' + $image) }
            $prefix = $releaseFull.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
            if (-not $image.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw ('recorded image is outside the release dir: ' + $image)
            }
            $hash = (Get-FileHash -LiteralPath $expected -Algorithm SHA256).Hash.ToLowerInvariant()
            if ([string]$row.sha256 -ne $hash) { throw ('sha256 mismatch for ' + $row.name) }
        }
        $decoyProc = Start-Process -FilePath $decoy -PassThru
        $tracked.Add([int]$decoyProc.Id)
        Start-Sleep -Milliseconds 200
        if ($decoyProc.HasExited) { throw 'decoy exited before rollback' }
        $decoyImage = Get-LiveImagePath -ProcessId ([int]$decoyProc.Id)
        $decoyFull = [IO.Path]::GetFullPath($decoy)
        if ($decoyImage -ne $decoyFull) { throw ('decoy image differs: ' + $decoyImage) }

        foreach ($switch in @('-WhatIf', '-DryRun')) {
            $plan = Invoke-ReleaseScript -File $rollbackScript -ArgumentList (@(
                '-ReleaseDir', $release,
                '-PreviousReleaseDir', $previous,
                $switch
            ) + $portArgs)
            if ($plan.Code -ne 0) { throw ($switch + ' failed: ' + $plan.Text) }
            if ($plan.Text -notmatch 'WhatIf:') { throw ($switch + ' did not print WhatIf') }
            foreach ($row in $rows) {
                if (-not (Test-ProcessAlive -ProcessId ([int]$row.pid))) {
                    throw ($switch + ' stopped recorded pid ' + $row.pid)
                }
            }
            if (-not (Test-ProcessAlive -ProcessId ([int]$decoyProc.Id))) {
                throw ($switch + ' stopped the decoy')
            }
            if (Test-Path -LiteralPath (Join-Path $previous 'run-state.json')) {
                throw ($switch + ' started the previous release')
            }
        }

        $rolled = Invoke-ReleaseScript -File $rollbackScript -ArgumentList (@(
            '-ReleaseDir', $release,
            '-PreviousReleaseDir', $previous
        ) + $portArgs)
        if ($rolled.Code -ne 0) { throw ('rollback failed: ' + $rolled.Text) }
        foreach ($row in $rows) {
            $processId = [int]$row.pid
            if (-not (Test-ProcessAlive -ProcessId $processId)) { continue }
            $image = Get-LiveImagePath -ProcessId $processId
            $expected = [IO.Path]::GetFullPath([string]$row.exe)
            if ([string]::Equals($image, $expected, [StringComparison]::OrdinalIgnoreCase)) {
                throw ('recorded pid survived rollback: ' + $processId)
            }
        }
        if (-not (Test-ProcessAlive -ProcessId ([int]$decoyProc.Id))) {
            throw 'decoy outside ReleaseDir did not survive rollback'
        }
        $previousStatePath = Join-Path $previous 'run-state.json'
        if (-not (Test-Path -LiteralPath $previousStatePath -PathType Leaf)) {
            throw 'previous release run-state json is missing'
        }
        $previousState = Get-Content -LiteralPath $previousStatePath -Raw | ConvertFrom-Json
        $previousRows = @($previousState.processes)
        if ($previousRows.Count -ne 5) { throw ('previous release started ' + $previousRows.Count) }
        $previousFull = [IO.Path]::GetFullPath($previous)
        $previousPrefix = $previousFull.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
        foreach ($row in $previousRows) {
            $processId = [int]$row.pid
            $tracked.Add($processId)
            if (-not (Test-ProcessAlive -ProcessId $processId)) { throw ('previous pid is dead: ' + $processId) }
            $image = Get-LiveImagePath -ProcessId $processId
            if (-not $image.StartsWith($previousPrefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw ('previous image is outside PreviousReleaseDir: ' + $image)
            }
        }
        if (Test-Path -LiteralPath $statePath) { throw 'current run-state json was left in place' }
    } finally {
        if ($null -eq $savedDb) {
            Remove-Item -Path Env:MXH_DATABASE_CONFIG -ErrorAction SilentlyContinue
        } else {
            [Environment]::SetEnvironmentVariable('MXH_DATABASE_CONFIG', $savedDb, 'Process')
        }
        foreach ($processId in @($tracked)) { Stop-TrackedPid -ProcessId $processId }
        if (Test-Path -LiteralPath $root) {
            Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

Add-ReleaseCase -Name 'healthcheck-port-down' -Body {
    Assert-ReleaseScripts
    $dir = Join-Path ([IO.Path]::GetTempPath()) ('mxh-rel-health-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $dir | Out-Null
    try {
        $ports = @(New-FreePortList -Count 5)
        $argList = @('-ReleaseDir', $dir) + @(Get-PortArgs -Ports $ports | Where-Object { $_ -ne '-PublicHost' -and $_ -ne $publicHost })
        $result = Invoke-ReleaseScript -File $healthScript -ArgumentList $argList
        if ($result.Code -eq 0) { throw 'healthcheck exited 0 while a port was down' }
        if ($result.Text -notmatch 'HEALTH FAIL port') { throw ('missing HEALTH FAIL port: ' + $result.Text) }
        foreach ($port in @($ports)) {
            $needle = 'HEALTH FAIL port ' + $port + ' not-listening'
            if ($result.Text -notmatch [regex]::Escape($needle)) {
                throw ('missing ' + $needle)
            }
        }
    } finally {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Add-ReleaseCase -Name 'no-legacy-literals' -Body {
    Assert-ReleaseScripts
    $banned = @('192.168.2.244', '9000', '10000', '10001')
    foreach ($path in @($startScript, $rollbackScript, $healthScript, $commonScript)) {
        $text = [IO.File]::ReadAllText($path)
        foreach ($token in $banned) {
            if ($text.Contains($token)) { throw ($path + ' contains ' + $token) }
        }
        if ($text -match 'Stop-Process\s+-Name' -or $text -match 'Get-Process\s+-Name') {
            throw ($path + ' stops by process name')
        }
    }
}

$total = $passed + $failed
Write-Host ('PWSH SUMMARY passed=' + $passed + ' failed=' + $failed + ' out of ' + $total)
if ($failed -ne 0) { exit 1 }
exit 0
