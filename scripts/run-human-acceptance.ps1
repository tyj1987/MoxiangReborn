[CmdletBinding()]
param(
    [ValidateSet('playdh-current', 'sworking-2008-reference')]
    [string]$ResourceProfileId = 'playdh-current',
    [ValidateRange(1, 65535)] [int]$LoginPort = 26101,
    [ValidateRange(1, 65535)] [int]$AgentPort = 27101,
    [ValidateRange(1, 65535)] [int]$MapPort = 28101,
    [ValidateRange(0, 255)] [int]$MapNumber = 10,
    [switch]$SkipServers
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$runId = Get-Date -Format 'yyyyMMdd-HHmmss'
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

$resourceRoot = if ($ResourceProfileId -eq 'playdh-current') {
    Join-Path $repoRoot 'modern\data\PlayDH'
} else {
    Join-Path $repoRoot 'reference\legacy-source\4dddd9a6\SWorking'
}
if (-not (Test-Path -LiteralPath $resourceRoot -PathType Container)) {
    throw "Resource profile root not found: $resourceRoot"
}

$clientExe = Resolve-RequiredFile @(
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

$client = $null
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

    $clientArgs = @('--login-port', $LoginPort, '--map-port', $MapPort,
        '--resource-profile', $ResourceProfileId, '--resource-root', $resourceRoot,
        '--width', 800, '--height', 600, '--evidence-dir', $evidenceRoot)
    $client = Start-Process -FilePath $clientExe -ArgumentList $clientArgs -WorkingDirectory $repoRoot -PassThru -RedirectStandardOutput (Join-Path $logRoot 'client.stdout.log') -RedirectStandardError (Join-Path $logRoot 'client.stderr.log')
    $owned.Add([pscustomobject]@{ pid = $client.Id; exe = $clientExe; name = 'client' })

    Write-Host "Human acceptance run: $runId" -ForegroundColor Cyan
    Write-Host "Artifacts: $runRoot"
    Write-Host 'Credentials are entered manually in the client. This script supplies no username, password, mouse or keyboard input.'
    Write-Host 'Complete the scenario: launcher/settings -> login -> display transition -> select/create -> Map10 -> movement -> NPC/UI -> combat/skill -> loot/pickup -> map change -> relog.'
    Write-Host 'Press F12 after each settled checkpoint to capture a TGA in the evidence folder.'
    Write-Host 'Checkpoints: login, display-transition, char-select-or-create, loading, map10, combat-and-loot, map-change, relog.'
    Write-Host 'Press Enter when finished, or Ctrl+C to abort.'
    [Console]::ReadLine() | Out-Null
} catch {
    $exitCode = 1
    Write-Error $_
} finally {
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
        clientPid = if ($null -ne $client) { $client.Id } else { $null }
        serverPids = @($serverPids)
        evidenceDir = $evidenceRoot
        evidenceFrames = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
        exitCode = $exitCode
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runRoot 'run.json') -Encoding utf8
}
exit $exitCode
