[CmdletBinding()]
param(
    [ValidateSet('playdh-current')]
    [string]$ResourceProfileId = 'playdh-current',
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9.-]+$')] [string]$LoginHost = '192.168.2.244',
    [ValidateRange(1, 65535)] [int]$LoginPort = 16001,
    [ValidateRange(1, 65535)] [int]$AgentPort = 17001,
    [ValidateRange(1, 65535)] [int]$MapPort = 18001,
    [ValidateRange(0, 64)] [int]$MinimumEvidenceFrames = 0,
    [switch]$PlanOnly,
    # PlayDH root passed as --resource-root. Empty reads MXH_PLAYDH_ROOT.
    [string]$ResourceRoot = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$humanChosen = $ResourceRoot
if ([string]::IsNullOrWhiteSpace($humanChosen)) { $humanChosen = $env:MXH_PLAYDH_ROOT }
if ([string]::IsNullOrWhiteSpace($humanChosen)) {
    Write-Output 'MXH_PLAYDH_ROOT is not set and no --resource-root was given'
    exit 1
}
if (-not (Test-Path -LiteralPath $humanChosen -PathType Container)) {
    Write-Output ('MXH_PLAYDH_ROOT is set to "' + $humanChosen + '" but that directory does not exist')
    exit 1
}
$resourceRoot = [IO.Path]::GetFullPath($humanChosen)
. (Join-Path $PSScriptRoot 'human-acceptance-process.ps1')

function Resolve-RequiredFile {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Required file not found: $($Candidates -join ', ')"
}

function Assert-RemoteLoginHost {
    param([string]$HostName, [switch]$ResolveDns)
    if ($HostName -ieq 'localhost' -or $HostName -ieq 'localhost.localdomain') {
        throw 'Loopback login host is not allowed for original PVE acceptance.'
    }
    $address = $null
    if ([System.Net.IPAddress]::TryParse($HostName, [ref]$address)) {
        if ([System.Net.IPAddress]::IsLoopback($address) -or
            $address.Equals([System.Net.IPAddress]::Any)) {
            throw 'Loopback or wildcard login host is not allowed.'
        }
    } elseif ($ResolveDns) {
        $resolved = [System.Net.Dns]::GetHostAddresses($HostName)
        if ($resolved.Count -eq 0 -or @($resolved | Where-Object {
            [System.Net.IPAddress]::IsLoopback($_) -or
            $_.Equals([System.Net.IPAddress]::Any)
        }).Count -ne 0) {
            throw 'Login host resolves to a loopback or wildcard address.'
        }
    }
}

if ($LoginPort -eq $AgentPort -or $LoginPort -eq $MapPort -or $AgentPort -eq $MapPort) {
    throw 'Login, Agent and Map ports must be different.'
}
Assert-RemoteLoginHost -HostName $LoginHost -ResolveDns:(-not $PlanOnly)
$launcherEndpointArgs = @('--login-host', $LoginHost, '--login-port', $LoginPort,
    '--agent-port', $AgentPort, '--map-port', $MapPort, '--resource-root', $resourceRoot)
if ($PlanOnly) {
    [ordered]@{
        mode = 'original-client-to-pve'
        profile = $ResourceProfileId
        buildDirectory = $BuildDirectory
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

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    throw 'Specify -BuildDirectory with source-bound Launcher and Client binaries.'
}
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory -ErrorAction Stop).Path
$modernRoot = (Resolve-Path -LiteralPath (Join-Path $repoRoot 'modern')).Path
if (-not $buildRoot.StartsWith($modernRoot + [IO.Path]::DirectorySeparatorChar,
                                [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildDirectory must be inside this checkout modern directory.'
}
if (-not (Test-Path -LiteralPath $resourceRoot -PathType Container)) {
    throw "Resource profile root not found: $resourceRoot"
}
$launcherExe = Resolve-RequiredFile @(
    (Join-Path $buildRoot 'tools\MoxianLauncher\MoxianLauncher.exe'),
    (Join-Path $buildRoot 'tools\MoxianLauncher\Debug\MoxianLauncher.exe'))
$clientExe = Resolve-RequiredFile @(
    (Join-Path $buildRoot 'tools\MoxianClient\mxh_client.exe'),
    (Join-Path $buildRoot 'tools\MoxianClient\Debug\mxh_client.exe'))
$launcherSha = (Get-FileHash -LiteralPath $launcherExe -Algorithm SHA256).Hash.ToLowerInvariant()
$clientSha = (Get-FileHash -LiteralPath $clientExe -Algorithm SHA256).Hash.ToLowerInvariant()

# Installed x86 Debug CRT/UCRT for the current BuildTools and SDK. No audit
# directory or local server/database runtime is involved.
$originalPath = $env:Path
$cache = Join-Path $buildRoot 'CMakeCache.txt'
if ((Test-Path -LiteralPath $cache -PathType Leaf) -and
    (Select-String -LiteralPath $cache -Pattern '^CMAKE_BUILD_TYPE:STRING=Debug$' -Quiet)) {
    $debugCrt = 'C:\BuildTools\VC\Redist\MSVC\14.44.35112\debug_nonredist\x86\Microsoft.VC143.DebugCRT'
    $debugUcrt = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\ucrt'
    if (-not (Test-Path -LiteralPath $debugCrt -PathType Container) -or
        -not (Test-Path -LiteralPath $debugUcrt -PathType Container)) {
        throw 'Current x86 Debug CRT/UCRT paths are missing; run from an initialized vcvarsall x86 environment.'
    }
    $env:Path = "$debugCrt;$debugUcrt;$env:Path"
}

$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'),
    ([guid]::NewGuid().ToString('N').Substring(0, 8))
$runRoot = Join-Path $repoRoot "modern\out\runs\human\$runId"
$evidenceRoot = Join-Path $runRoot 'evidence'
$null = New-Item -ItemType Directory -Force -Path $evidenceRoot
$launcherEntry = $null
$clientEntries = [System.Collections.Generic.List[object]]::new()
$stopResults = [System.Collections.Generic.List[object]]::new()
$exitCode = 0
try {
    $launcherArgs = $launcherEndpointArgs + @('--evidence-dir', $evidenceRoot)
    $launcher = Start-Process -FilePath $launcherExe -ArgumentList $launcherArgs `
        -WorkingDirectory $repoRoot -PassThru
    $snapshot = Get-ProcessSnapshot -ProcessId $launcher.Id
    if ($null -eq $snapshot -or
        -not [StringComparer]::OrdinalIgnoreCase.Equals($snapshot.exe, $launcherExe) -or
        -not [StringComparer]::OrdinalIgnoreCase.Equals($snapshot.sha256, $launcherSha)) {
        throw 'Started launcher identity could not be recorded; refusing unowned cleanup.'
    }
    $launcherEntry = $snapshot
    Write-Host "Human evidence collection: $runId" -ForegroundColor Cyan
    Write-Host "Artifacts: $runRoot"
    Write-Host "Original PVE endpoint: ${LoginHost}:$LoginPort (Agent $AgentPort, Map $MapPort)."
    Write-Host 'Use Launcher, log in manually, and record screenshots plus server/DB event evidence.'
    Write-Host 'Press Enter when finished. Screenshot count is a collection check, not acceptance.'
    [Console]::ReadLine() | Out-Null
    $captured = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File)
    if ($MinimumEvidenceFrames -gt 0 -and $captured.Count -lt $MinimumEvidenceFrames) {
        throw "Captured $($captured.Count) frame(s); requested collection minimum is $MinimumEvidenceFrames."
    }
} catch {
    $exitCode = 1
    Write-Warning "Collection failed: $($_.Exception.Message)"
} finally {
    # Launcher uses CreateProcessW: only its exact live PID/path/hash/ticks
    # can own direct client children. Never scan by name or recent start time.
    if ($null -ne $launcherEntry -and (Test-ExactProcess -Entry $launcherEntry)) {
        foreach ($child in @(Get-CimInstance -ClassName Win32_Process `
            -Filter "ParentProcessId = $($launcherEntry.pid)" -ErrorAction SilentlyContinue)) {
            $candidate = Get-ProcessSnapshot -ProcessId ([int]$child.ProcessId)
            if ($null -eq $candidate) { continue }
            if (-not (Test-LauncherChildSnapshot -Launcher $launcherEntry `
                -Candidate $candidate -ClientExe $clientExe -ClientSha256 $clientSha)) {
                continue
            }
            $clientEntries.Add($candidate)
        }
    }
    foreach ($entry in $clientEntries) { $stopResults.Add((Stop-OwnedProcess -Entry $entry)) }
    if ($null -ne $launcherEntry) {
        $stopResults.Add((Stop-OwnedProcess -Entry $launcherEntry))
    }
    $env:Path = $originalPath
    [ordered]@{
        runId = $runId
        profile = $ResourceProfileId
        mode = 'original-client-to-pve'
        buildDirectory = $buildRoot
        loginHost = $LoginHost
        loginPort = $LoginPort
        agentPort = $AgentPort
        mapPort = $MapPort
        launcher = $launcherEntry
        clients = @($clientEntries.ToArray())
        shutdown = @($stopResults.ToArray())
        evidenceDir = $evidenceRoot
        evidenceFrames = @(Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.tga' -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
        acceptanceStatus = 'not_evaluated'
        collectionStatus = if ($exitCode -eq 0) { 'finished' } else { 'incomplete' }
        exitCode = $exitCode
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runRoot 'run.json') -Encoding utf8
}
exit $exitCode
