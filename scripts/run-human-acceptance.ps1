[CmdletBinding()]
param(
    [ValidateSet('playdh-current')]
    [string]$ResourceProfileId = 'playdh-current',
    [ValidateRange(1, 65535)] [int]$LoginPort = 26101,
    [ValidateRange(1, 65535)] [int]$AgentPort = 27101,
    [ValidateRange(1, 65535)] [int]$MapPort = 28101,
    [ValidateRange(0, 255)] [int]$MapNumber = 10,
    [ValidateRange(0, 64)] [int]$MinimumEvidenceFrames = 8,
    [switch]$SkipServers
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'),
    ([guid]::NewGuid().ToString('N').Substring(0, 8))
$runRoot = Join-Path $repoRoot "modern\out\runs\human\$runId"
$logRoot = Join-Path $runRoot 'logs'
$evidenceRoot = Join-Path $runRoot 'evidence'
$null = New-Item -ItemType Directory -Force -Path $logRoot
$null = New-Item -ItemType Directory -Force -Path $evidenceRoot
$owned = [System.Collections.Generic.List[object]]::new()
$serverPids = [System.Collections.Generic.List[int]]::new()

function Resolve-RequiredFile {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Required file not found: $($Candidates -join ', ')"
}

function Test-ExactProcess {
    param([int]$Pid, [string]$ExpectedPath)
    $process = Get-Process -Id $Pid -ErrorAction SilentlyContinue
    if ($null -eq $process -or [string]::IsNullOrWhiteSpace([string]$process.Path)) { return $false }
    try {
        $actual = (Resolve-Path -LiteralPath $process.Path).Path
        $expected = (Resolve-Path -LiteralPath $ExpectedPath).Path
        return [StringComparer]::OrdinalIgnoreCase.Equals($actual, $expected)
    } catch { return $false }
}

function Stop-OwnedProcess {
    param($Entry)
    if ($null -eq $Entry) { return }
    $pidValue = [int]$Entry.pid
    if (Test-ExactProcess -Pid $pidValue -ExpectedPath ([string]$Entry.exe)) {
        Stop-Process -Id $pidValue -Force -ErrorAction SilentlyContinue
    } else {
        Write-Warning "Refusing to stop PID $pidValue because executable identity no longer matches."
    }
}

$resourceRoot = Join-Path $repoRoot 'modern\data\PlayDH'
if (-not (Test-Path -LiteralPath $resourceRoot -PathType Container)) {
    throw "Resource profile root not found: $resourceRoot"
}

$launcherExe = Resolve-RequiredFile @(
    (Join-Path $repoRoot 'modern\build\tools\MoxianLauncher\MoxianLauncher.exe'),
    (Join-Path $repoRoot 'modern\build\tools\MoxianLauncher\Debug\MoxianLauncher.exe'))
$clientExeCandidates = @(
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\mxh_client.exe'),
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\Debug\mxh_client.exe'))
$serverScript = Resolve-RequiredFile @((Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'))
$stateFile = Join-Path $repoRoot 'deploy\runtime\modern\pids.json'
$previousState = if (Test-Path -LiteralPath $stateFile -PathType Leaf) {
    Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json
} else { @() }
if (@($previousState).Count -gt 0 -and -not $SkipServers) {
    throw 'Existing managed server state detected. Stop it explicitly before a human acceptance run.'
}

$launcher = $null
$clientPids = [System.Collections.Generic.List[int]]::new()
$exitCode = 0
try {
    if (-not $SkipServers) {
        $serverArgs = @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $serverScript,
            '-Mode', 'start', '-ResourceProfileId', $ResourceProfileId,
            '-LoginPort', $LoginPort, '-AgentPort', $AgentPort, '-MapPort', $MapPort,
            '-MapNumber', $MapNumber, '-DataDir', $runRoot
        )
        $starter = Start-Process -FilePath 'powershell.exe' -ArgumentList $serverArgs -WorkingDirectory $repoRoot -Wait -PassThru -RedirectStandardOutput (Join-Path $logRoot 'server-start.stdout.log') -RedirectStandardError (Join-Path $logRoot 'server-start.stderr.log')
        if ($starter.ExitCode -ne 0) { throw "Server startup failed with exit code $($starter.ExitCode)" }
        if (-not (Test-Path -LiteralPath $stateFile -PathType Leaf)) { throw 'Server startup produced no PID manifest.' }
        $state = @(Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json)
        foreach ($entry in $state) {
            if (-not (Test-ExactProcess -Pid ([int]$entry.pid) -ExpectedPath ([string]$entry.exe))) {
                throw "Server PID identity check failed for $($entry.name)."
            }
            $serverPids.Add([int]$entry.pid)
        }
    }

    $launchStart = Get-Date
    $launcherArgs = @('--login-port', $LoginPort, '--agent-port', $AgentPort, '--map-port', $MapPort, '--resource-root', $resourceRoot)
    $launcher = Start-Process -FilePath $launcherExe -ArgumentList $launcherArgs -WorkingDirectory $repoRoot -PassThru
    $owned.Add([pscustomobject]@{ pid = $launcher.Id; exe = $launcherExe; name = 'launcher' })

    Write-Host "Human acceptance run: $runId" -ForegroundColor Cyan
    Write-Host "Artifacts: $runRoot"
    Write-Host 'Use the launcher to check/repair resources, configure display/audio, and start the client.'
    Write-Host 'Credentials are entered manually in the client. This script supplies no username, password, mouse or keyboard input.'
    Write-Host 'Complete the scenario: launcher/settings -> login -> display transition -> select/create -> Map10 -> movement -> NPC/UI -> combat/skill -> loot/pickup -> map change -> relog.'
    Write-Host 'Press F12 after each settled checkpoint to capture a TGA in the evidence folder.'
    Write-Host 'Checkpoints: login, display-transition, char-select-or-create, loading, map10, combat-and-loot, map-change, relog.'
    Write-Host 'Press Enter when finished, or Ctrl+C to abort.'
    [Console]::ReadLine() | Out-Null
    $captured = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue)
    if ($captured.Count -lt $MinimumEvidenceFrames) {
        throw "Human acceptance produced $($captured.Count) evidence frame(s); minimum is $MinimumEvidenceFrames. Capture settled checkpoints with F12 before finishing."
    }
} catch {
    $exitCode = 1
    Write-Error $_
} finally {
    # The launcher creates the client as a child after the operator clicks
    # Start. Discover only the two explicit client paths and only processes
    # created after this run began; never kill by a generic process name.
    foreach ($candidate in $clientExeCandidates) {
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        foreach ($process in @(Get-Process -ErrorAction SilentlyContinue)) {
            if ($process.Id -eq $PID -or $process.Id -eq [int]$launcher.Id) { continue }
            if ($process.StartTime -lt $launchStart) { continue }
            if (Test-ExactProcess -Pid $process.Id -ExpectedPath $resolved) {
                $clientPids.Add([int]$process.Id)
                $owned.Add([pscustomobject]@{ pid = $process.Id; exe = $resolved; name = 'client' })
            }
        }
    }
    foreach ($entry in $owned) { Stop-OwnedProcess $entry }
    if (-not $SkipServers -and (Test-Path -LiteralPath $stateFile -PathType Leaf)) {
        $state = @(Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json)
        foreach ($entry in $state) {
            if ($serverPids -contains [int]$entry.pid) { Stop-OwnedProcess $entry }
        }
        Remove-Item -LiteralPath $stateFile -Force -ErrorAction SilentlyContinue
    }
    [ordered]@{
        runId = $runId
        profile = $ResourceProfileId
        map = $MapNumber
        launcherPid = if ($null -ne $launcher) { $launcher.Id } else { $null }
        clientPids = @($clientPids)
        serverPids = @($serverPids)
        evidenceDir = $evidenceRoot
        evidenceFrames = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
        exitCode = $exitCode
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runRoot 'run.json') -Encoding utf8
}
exit $exitCode
