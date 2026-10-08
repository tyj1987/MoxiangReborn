[CmdletBinding()]
param(
    [ValidateSet('playdh-current')]
    [string]$ResourceProfileId = 'playdh-current',
    [ValidatePattern('^[A-Za-z0-9.-]+$')] [string]$LoginHost = '192.168.2.244',
    [ValidateRange(1, 65535)] [int]$LoginPort = 16001,
    [ValidateRange(1, 65535)] [int]$AgentPort = 17001,
    [ValidateRange(1, 65535)] [int]$MapPort = 18001,
    [ValidateRange(0, 64)] [int]$MinimumEvidenceFrames = 8,
    [switch]$PlanOnly
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$resourceRoot = Join-Path $repoRoot 'modern\data\PlayDH'
$launcherEndpointArgs = @('--login-host', $LoginHost, '--login-port', $LoginPort,
    '--agent-port', $AgentPort, '--map-port', $MapPort, '--resource-root', $resourceRoot)
if ($LoginPort -eq $AgentPort -or $LoginPort -eq $MapPort -or $AgentPort -eq $MapPort) {
    throw 'Login, Agent and Map ports must be different.'
}
if ($PlanOnly) {
    [ordered]@{
        mode = 'client-only'
        profile = $ResourceProfileId
        loginHost = $LoginHost
        loginPort = $LoginPort
        agentPort = $AgentPort
        mapPort = $MapPort
        launcherArguments = $launcherEndpointArgs
        startsLocalServers = $false
        needsLocalDatabaseConfig = $false
        acceptanceStatus = 'not_evaluated'
    } | ConvertTo-Json -Depth 4
    return
}
$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'),
    ([guid]::NewGuid().ToString('N').Substring(0, 8))
$runRoot = Join-Path $repoRoot "modern\out\runs\human\$runId"
$evidenceRoot = Join-Path $runRoot 'evidence'
$null = New-Item -ItemType Directory -Force -Path $evidenceRoot
$owned = [System.Collections.Generic.List[object]]::new()

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
    param([int]$ProcessId, [string]$ExpectedPath)
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
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
    if (Test-ExactProcess -ProcessId $pidValue -ExpectedPath ([string]$Entry.exe)) {
        Stop-Process -Id $pidValue -Force -ErrorAction SilentlyContinue
    } else {
        Write-Warning "Refusing to stop PID $pidValue because executable identity no longer matches."
    }
}

if (-not (Test-Path -LiteralPath $resourceRoot -PathType Container)) {
    throw "Resource profile root not found: $resourceRoot"
}

$launcherExe = Resolve-RequiredFile @(
    (Join-Path $repoRoot 'modern\build\tools\MoxianLauncher\MoxianLauncher.exe'),
    (Join-Path $repoRoot 'modern\build\tools\MoxianLauncher\Debug\MoxianLauncher.exe'))
$clientExeCandidates = @(
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\mxh_client.exe'),
    (Join-Path $repoRoot 'modern\build\tools\MoxianClient\Debug\mxh_client.exe'))
$launcher = $null
$clientPids = [System.Collections.Generic.List[int]]::new()
$exitCode = 0
try {
    $launchStart = Get-Date
    $launcherArgs = $launcherEndpointArgs + @('--evidence-dir', $evidenceRoot)
    $launcher = Start-Process -FilePath $launcherExe -ArgumentList $launcherArgs -WorkingDirectory $repoRoot -PassThru
    $owned.Add([pscustomobject]@{ pid = $launcher.Id; exe = $launcherExe; name = 'launcher' })

    Write-Host "Human acceptance run: $runId" -ForegroundColor Cyan
    Write-Host "Artifacts: $runRoot"
    Write-Host 'Use the launcher to check/repair resources, configure display/audio, and start the client.'
    Write-Host "Login endpoint: ${LoginHost}:$LoginPort (Agent $AgentPort, Map $MapPort)."
    Write-Host 'Account credentials are entered manually in the client.'
    Write-Host 'This script supplies no username, password, mouse or keyboard input.'
    Write-Host 'Complete the scenario: launcher/settings -> login -> display transition -> select/create -> Map10 -> movement -> NPC/UI -> combat/skill -> loot/pickup -> map change -> relog.'
    Write-Host 'Press F12 after each settled checkpoint to capture a TGA in the evidence folder.'
    Write-Host 'Checkpoints: login, display-transition, char-select-or-create, loading, map10, combat-and-loot, map-change, relog.'
    Write-Host 'Press Enter when finished, or Ctrl+C to abort.'
    [Console]::ReadLine() | Out-Null
    $captured = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue)
    if ($captured.Count -lt $MinimumEvidenceFrames) {
        throw "Human acceptance produced $($captured.Count) evidence frame(s); minimum is $MinimumEvidenceFrames. Capture settled checkpoints with F12 before finishing."
    }
    Write-Host 'Evidence collection finished; the GUI checkpoints still require human review.'
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
            if ($process.Id -eq $PID -or ($null -ne $launcher -and $process.Id -eq [int]$launcher.Id)) { continue }
            try {
                if ($process.StartTime -lt $launchStart) { continue }
            } catch { continue }
            if (Test-ExactProcess -ProcessId $process.Id -ExpectedPath $resolved) {
                $clientPids.Add([int]$process.Id)
                $owned.Add([pscustomobject]@{ pid = $process.Id; exe = $resolved; name = 'client' })
            }
        }
    }
    foreach ($entry in $owned) { Stop-OwnedProcess $entry }
    [ordered]@{
        runId = $runId
        profile = $ResourceProfileId
        mode = 'client-only'
        loginHost = $LoginHost
        loginPort = $LoginPort
        agentPort = $AgentPort
        mapPort = $MapPort
        launcherPid = if ($null -ne $launcher) { $launcher.Id } else { $null }
        clientPids = @($clientPids)
        evidenceDir = $evidenceRoot
        evidenceFrames = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
        acceptanceStatus = if ($exitCode -eq 0) { 'evidence_collected_awaiting_review' } else { 'collection_incomplete' }
        exitCode = $exitCode
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runRoot 'run.json') -Encoding utf8
}
exit $exitCode
