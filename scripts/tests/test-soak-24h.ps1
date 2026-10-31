# Verdict, identity, and path checks for scripts/soak-24h.ps1.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-soak-24h.ps1
$ErrorActionPreference = 'Stop'
$script:Asserts = 0

function Assert-True {
    param($Condition, [string]$Message)
    $script:Asserts++
    if (-not $Condition) { throw ('FAIL: ' + $Message) }
}

$version = $PSVersionTable.PSVersion
$versionOk = $false
if ($version.Major -gt 7) { $versionOk = $true }
if ($version.Major -eq 7 -and $version.Minor -ge 4) { $versionOk = $true }
if (-not $versionOk) { throw ('FAIL: requires pwsh 7.4+, found ' + $version) }

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$soak = Join-Path $repoRoot 'scripts\soak-24h.ps1'
$cmd = Join-Path $repoRoot 'scripts\soak-24h.cmd'
. $soak

function Assert-Parses {
    param([string]$Path)
    $tokens = $null
    $errors = $null
    $null = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$errors)
    Assert-True ((-not $errors) -or (@($errors).Count -eq 0)) ('parse error in ' + $Path)
}

Assert-Parses $soak
Assert-Parses (Join-Path $repoRoot 'scripts\commercial-smoke.ps1')
Assert-True ([IO.File]::Exists($cmd)) 'soak-24h.cmd missing'
$cmdText = [IO.File]::ReadAllText($cmd)
Assert-True ($cmdText.Contains('pwsh -NoProfile -File')) 'cmd must launch pwsh -NoProfile -File'
Assert-True ($cmdText.Contains('soak-24h.ps1')) 'cmd must target soak-24h.ps1'
Assert-True ($cmdText.Contains('%*')) 'cmd must pass arguments through'

$soakText = [IO.File]::ReadAllText($soak)
Assert-True (-not $soakText.Contains('Get-Process -Name')) 'harness must not select processes by name'
Assert-True (-not $soakText.Contains('Stop-Process -Name')) 'harness must not stop processes by name'
Assert-True (-not $soakText.Contains('start_modern.ps1')) 'harness must not call the shared launcher script'
Assert-True (-not $soakText.Contains('Out-File')) 'harness must not use Out-File'
Assert-True (-not $soakText.Contains('Set-Content')) 'harness must not use Set-Content'
Assert-True (-not $soakText.Contains('Add-Content')) 'harness must not use Add-Content'
Assert-True ($soakText.Contains('Compare-SoakStateDump')) 'state dump hook missing'
Assert-True ($soakText.Contains('full re-login')) 'reconnect semantics missing'
Assert-True ($soakText.Contains('--default-map')) 'agent plan must pass --default-map'

$mb = 1048576.0
function New-Point {
    param([double]$THours, [double]$PrivateMb, [int]$Handles)
    return [pscustomobject]@{ THours = $THours; PrivateBytes = [int64]($PrivateMb * $mb); Handles = $Handles }
}

$flat = @(
    (New-Point 1 100 40),
    (New-Point 2 100 40),
    (New-Point 3 100 40),
    (New-Point 4 100 40)
)
$flatLeak = Test-SoakLeak -Samples $flat -WarmupHours 1
Assert-True ($flatLeak.Status -eq 'pass') ('flat series should pass, got ' + $flatLeak.Status + ' ' + ($flatLeak.Reasons -join ','))
Assert-True ([math]::Abs([double]$flatLeak.PrivateSlopeMbPerHour) -lt 0.0001) 'flat private slope is not zero'

$growth = @()
foreach ($hour in @(1, 2, 3, 4, 5)) {
    $growth += (New-Point $hour (100 + (2 * ($hour - 1))) 40)
}
$growthLeak = Test-SoakLeak -Samples $growth -WarmupHours 1
Assert-True ($growthLeak.Status -eq 'fail') 'linear +2 MB/h must fail'
Assert-True (@($growthLeak.Reasons) -contains 'private_slope') '+2 MB/h must fail the private slope'
Assert-True ([double]$growthLeak.PrivateSlopeMbPerHour -gt 1.5) 'slope should be about 2 MB/h'

$spike = @(
    (New-Point 0.2 800 9000),
    (New-Point 1 100 40),
    (New-Point 2 100 40),
    (New-Point 3 100 40)
)
$spikeLeak = Test-SoakLeak -Samples $spike -WarmupHours 1
Assert-True ($spikeLeak.Status -eq 'pass') ('early spike then flat should pass, got ' + ($spikeLeak.Reasons -join ','))

# Endpoints are equal, so a two-point slope is 0. Least squares over the
# middle climb is above 1 MB/h and must fail.
$climb = @(
    (New-Point 0 0 10),
    (New-Point 1 6 10),
    (New-Point 2 12 10),
    (New-Point 3 18 10),
    (New-Point 4 0 10)
)
$climbLeak = Test-SoakLeak -Samples $climb -WarmupHours 0
Assert-True ($climbLeak.Status -eq 'fail') 'least-squares climb must fail even though endpoints match'
Assert-True ([double]$climbLeak.PrivateSlopeMbPerHour -gt 1) 'least-squares slope must exceed 1 MB/h'

function New-FlatServer {
    param([string]$Name, [bool]$ExitedEarly = $false, [bool]$Reused = $false)
    return [pscustomobject]@{
        Name = $Name
        ExitedEarly = $ExitedEarly
        Reused = $Reused
        Samples = $flat
    }
}

function Get-BaseVerdict {
    param(
        [object[]]$Servers,
        [int]$CycleTotal = 1000,
        [int]$CycleFailed = 0,
        [double]$ElapsedHours = 24,
        [string]$WatchStatus = 'pass',
        [string]$GuardStatus = 'pass',
        [string]$GapStatus = 'pass',
        [string[]]$ExtraReasons = @()
    )
    return Get-SoakVerdict -ElapsedHours $ElapsedHours -DurationHours 24 -WarmupHours 1 -Servers $Servers -CycleTotal $CycleTotal -CycleFailed $CycleFailed -WatchStatus $WatchStatus -GuardStatus $GuardStatus -GapStatus $GapStatus -ExtraReasons $ExtraReasons
}

$passVerdict = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
)
Assert-True $passVerdict.Pass 'flat servers should pass'
Assert-True ($passVerdict.Line.StartsWith('SOAK PASS')) 'pass line must start with SOAK PASS'
Assert-True ($passVerdict.Line.Contains('private_slope_mb_h')) 'verdict line must include private slopes'
Assert-True ($passVerdict.Line.Contains('handle_slope_h')) 'verdict line must include handle slopes'

$exitVerdict = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login' -ExitedEarly $true),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
)
Assert-True (-not $exitVerdict.Pass) 'early server exit must fail'
Assert-True ($exitVerdict.Line.StartsWith('SOAK FAIL')) 'early exit line must be SOAK FAIL'
Assert-True ($exitVerdict.Line.Contains('server_exit:login')) 'early exit reason missing'

$rateFail = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
) -CycleFailed 2
Assert-True (-not $rateFail.Pass) '0.2% failure rate must fail'
Assert-True ($rateFail.Line.Contains('failure_rate=')) 'failure rate reason missing'

$ratePass = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
) -CycleFailed 1
Assert-True $ratePass.Pass '0.1% failure rate must pass'

$noCycles = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
) -CycleTotal 0 -CycleFailed 0
Assert-True (-not $noCycles.Pass) 'zero cycles must not pass'
Assert-True ($noCycles.Line.Contains('skipped:cycles')) 'zero cycles must be reported as skipped'

$short = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
) -ElapsedHours 23
Assert-True (-not $short.Pass) 'short elapsed must fail'
Assert-True ($short.Line.Contains('elapsed')) 'elapsed reason missing'

$skipWatch = Get-BaseVerdict -Servers @(
    (New-FlatServer 'login'),
    (New-FlatServer 'agent'),
    (New-FlatServer 'map')
) -WatchStatus 'skip'
Assert-True (-not $skipWatch.Pass) 'skipped watch must not pass'

$gapOk = Test-SoakSampleGaps -TimestampsUtc @(
    ([datetime]::new(2026, 1, 1, 0, 0, 0, [DateTimeKind]::Utc)),
    ([datetime]::new(2026, 1, 1, 0, 1, 0, [DateTimeKind]::Utc)),
    ([datetime]::new(2026, 1, 1, 0, 3, 0, [DateTimeKind]::Utc))
) -MaxGapSeconds 120
Assert-True ($gapOk.Status -eq 'pass') '120s gap must pass'
$gapBad = Test-SoakSampleGaps -TimestampsUtc @(
    ([datetime]::new(2026, 1, 1, 0, 0, 0, [DateTimeKind]::Utc)),
    ([datetime]::new(2026, 1, 1, 0, 2, 1, [DateTimeKind]::Utc))
) -MaxGapSeconds 120
Assert-True ($gapBad.Status -eq 'fail') '121s gap must fail'
$gapSkip = Test-SoakSampleGaps -TimestampsUtc @(([datetime]::new(2026, 1, 1, 0, 0, 0, [DateTimeKind]::Utc))) -MaxGapSeconds 120
Assert-True ($gapSkip.Status -eq 'skip') 'one sample must skip the gap check'

function New-Soak77Snapshot {
    param(
        [int]$Hp = 100,
        [int]$Mp = 40,
        [int]$Money = 50,
        [int]$Shield = 7,
        [int]$Map = 10,
        [int]$X = 25000,
        [int]$Y = 25001,
        [string]$Items = '[{"count":2,"item_index":50108,"slot":1}]',
        [switch]$NoPosition,
        [switch]$NoMoney,
        [switch]$NoItemIndex
    )
    if ($NoItemIndex) { $Items = '[{"count":2,"slot":1}]' }
    $moneyText = ''
    if (-not $NoMoney) { $moneyText = ',"money":' + $Money }
    $positionText = ''
    if (-not $NoPosition) {
        $positionText = ',"position":{"map":' + $Map + ',"x":' + $X + ',"y":' + $Y + '}'
    }
    return '{"hp":' + $Hp + ',"inventory":' + $Items + $moneyText + ',"mp":' + $Mp + $positionText + ',"shield":' + $Shield + '}'
}
function New-Soak77Dump {
    param([string]$Entry, [string]$Exit)
    return '{"entry":' + $Entry + ',"exit":' + $Exit + '}'
}
# Drop-time exit versus post-reconnect entry. x moves 32, the sample
# delta from the mxh_client_e2e state-dump writer, which is within 32.
$drop = New-Soak77Snapshot -X 25000 -Y 25001
$back = New-Soak77Snapshot -X 25032 -Y 25001
$before = New-Soak77Dump -Entry (New-Soak77Snapshot -Hp 1) -Exit $drop
$after = New-Soak77Dump -Entry $back -Exit (New-Soak77Snapshot -Hp 2)
$statePass = Compare-SoakStateDump -Before $before -After $after
Assert-True $statePass.Pass ('#77 state dump within 32 must pass: ' + ($statePass.Reasons -join ','))
$tooFar = Compare-SoakStateDump -Before $before -After (New-Soak77Dump -Entry (New-Soak77Snapshot -X 25033 -Y 25001) -Exit (New-Soak77Snapshot -Hp 2))
Assert-True (-not $tooFar.Pass) 'position over 32 must fail'
Assert-True (@($tooFar.Reasons) -contains 'position') 'position over 32 must report position'
$badItems = '[{"count":9,"item_index":50108,"slot":1}]'
$badInventory = Compare-SoakStateDump -Before $before -After (New-Soak77Dump -Entry (New-Soak77Snapshot -Items $badItems) -Exit (New-Soak77Snapshot -Hp 2))
Assert-True (-not $badInventory.Pass) 'inventory mismatch must fail'
Assert-True (@($badInventory.Reasons) -contains 'inventory') 'inventory mismatch must report inventory'
$missingPosition = Compare-SoakStateDump -Before $before -After (New-Soak77Dump -Entry (New-Soak77Snapshot -NoPosition) -Exit (New-Soak77Snapshot -Hp 2))
Assert-True (-not $missingPosition.Pass) 'missing position must fail'
Assert-True (@($missingPosition.Reasons) -contains 'missing:position') 'missing position must report missing:position'
$missingIndex = Compare-SoakStateDump -Before $before -After (New-Soak77Dump -Entry (New-Soak77Snapshot -NoItemIndex) -Exit (New-Soak77Snapshot -Hp 2))
Assert-True (-not $missingIndex.Pass) 'missing item_index must fail'
Assert-True (@($missingIndex.Reasons) -contains 'missing:item_index') 'missing item_index must report missing:item_index'
$missingMoney = Compare-SoakStateDump -Before $before -After (New-Soak77Dump -Entry (New-Soak77Snapshot -NoMoney) -Exit (New-Soak77Snapshot -Hp 2))
Assert-True (-not $missingMoney.Pass) 'missing money must fail'
Assert-True (@($missingMoney.Reasons) -contains 'missing:money') 'missing money must report missing:money'
$malformed = Compare-SoakStateDump -Before '{' -After $after
Assert-True (-not $malformed.Pass) 'malformed JSON must fail'
Assert-True (@($malformed.Reasons) -contains 'malformed') 'malformed JSON must report malformed'

$cRefused = Test-SoakRunDirPolicy -Path 'C:\soak\run' -ForWindows $true -IsDefault $false
Assert-True (-not $cRefused.Allowed) 'RunDir under C:\ must be refused on Windows'
Assert-True ($cRefused.Reason.Contains('C:\')) 'C:\ refusal must name the drive'
$eDefault = Resolve-SoakRunDirectory -RunDir '' -BuildRoot 'E:\build-mxr' -ServerBinDir '' -RunId 'abc' -ForWindows $true
Assert-True (-not [string]::IsNullOrEmpty($eDefault.Refusal)) 'default RunDir on E: must be refused'
$dDefault = Resolve-SoakRunDirectory -RunDir '' -BuildRoot 'D:\build-mxr' -ServerBinDir '' -RunId 'abc' -ForWindows $true
Assert-True ([string]::IsNullOrEmpty($dDefault.Refusal)) 'default RunDir on D: must be allowed'
Assert-True ($dDefault.Path.Contains('soak-abc')) 'default RunDir must live under the build root'
$unixOk = Test-SoakRunDirPolicy -Path '/tmp/soak-run' -ForWindows $false -IsDefault $false
Assert-True $unixOk.Allowed 'absolute Linux RunDir must be allowed'

Assert-True (-not [string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 16001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002))) '16001 must be forbidden'
Assert-True (-not [string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 26011 -AgentPort 27001 -MapPort 28001 -Map12Port 28002))) '26011 must be forbidden'
Assert-True (-not [string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 26001 -AgentPort 27011 -MapPort 28001 -Map12Port 28002))) '27011 must be forbidden'
Assert-True (-not [string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 26001 -AgentPort 27001 -MapPort 28011 -Map12Port 28002))) '28011 must be forbidden'
Assert-True (-not [string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 36001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002))) '36001 must be forbidden'
Assert-True ([string]::IsNullOrEmpty((Get-SoakPortViolation -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002))) 'default ephemeral ports must be allowed'

$goodDb = Resolve-SoakMssqlTarget -ConnectionString 'backend=mssql_odbc;host=.\SQLEXPRESS;database=MxE2E_run1;encrypt=no'
Assert-True $goodDb.Ok '.\SQLEXPRESS MxE2E database must be accepted'
Assert-True ($goodDb.Database -eq 'MxE2E_run1') 'database name must round-trip'
$badHost = Resolve-SoakMssqlTarget -ConnectionString 'backend=mssql_odbc;host=localhost\SQLEXPRESS;database=MxE2E_run1'
Assert-True (-not $badHost.Ok) 'localhost\SQLEXPRESS must be refused'
$badName = Resolve-SoakMssqlTarget -ConnectionString 'backend=mssql_odbc;host=.\SQLEXPRESS;database=Moxiang'
Assert-True (-not $badName.Ok) 'Moxiang database must be refused'
$badKey = Resolve-SoakMssqlTarget -ConnectionString 'Server=.\SQLEXPRESS;database=MxE2E_run1'
Assert-True (-not $badKey.Ok) 'Server= key must be refused'
$create = Get-SoakMssqlCreateBatch -DatabaseName 'MxE2E_run1'
Assert-True ($create.Contains('SET RECOVERY SIMPLE')) 'create batch must set SIMPLE recovery'
Assert-True ($create.Contains('[MxE2E_run1]')) 'create batch must name the database'
$refusedCreate = $false
try { Get-SoakMssqlCreateBatch -DatabaseName 'Moxiang' | Out-Null } catch { $refusedCreate = $true }
Assert-True $refusedCreate 'create batch must refuse a non MxE2E database'
Assert-True (Get-SoakDatabaseCleanupPlan -CreatedByThisRun $true).Drop 'a database this run created is dropped'
Assert-True (-not (Get-SoakDatabaseCleanupPlan -CreatedByThisRun $false).Drop) 'a pre-existing database is not dropped'
Assert-True ((Get-SoakSqlDataPathQuery).Contains("SERVERPROPERTY('InstanceDefaultDataPath')")) 'data path query missing'
Assert-True ((Get-SoakSqlDataPathQuery).Contains("SERVERPROPERTY('InstanceDefaultLogPath')")) 'log path query missing'

$guardLow = Test-SoakGuardSpace -Paths @('E:\') -MinFreeGB 10 -FreeBytesByPath @{ 'E:\' = [int64](9 * 1073741824) }
Assert-True ($guardLow.Status -eq 'fail') '9 GB free must fail a 10 GB guard'
$guardOk = Test-SoakGuardSpace -Paths @('E:\') -MinFreeGB 10 -FreeBytesByPath @{ 'E:\' = [int64](10 * 1073741824) }
Assert-True ($guardOk.Status -eq 'pass') '10 GB free must pass'
$guardSkip = Test-SoakGuardSpace -Paths @() -MinFreeGB 10 -FreeBytesByPath @{}
Assert-True ($guardSkip.Status -eq 'skip') 'no guard path is a skip'

$watchRoot = Join-Path ([IO.Path]::GetTempPath()) ('soak-watch-' + [Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($watchRoot)
$watchFile = Join-Path $watchRoot 'keep.txt'
[IO.File]::WriteAllText($watchFile, 'abc')
$censusA = Get-SoakTreeCensus -Root $watchRoot
$censusB = Get-SoakTreeCensus -Root $watchRoot
Assert-True ((Get-SoakWatchStatus -Start $censusA -End $censusB).Status -eq 'pass') 'unchanged watch root must pass'
[IO.File]::WriteAllText((Join-Path $watchRoot 'extra.txt'), 'z')
$censusC = Get-SoakTreeCensus -Root $watchRoot
Assert-True ((Get-SoakWatchStatus -Start $censusA -End $censusC).Status -eq 'fail') 'changed watch root must fail'
[IO.Directory]::Delete($watchRoot, $true)

$plan = @(Get-SoakServerPlan -LoginExe 'mxh_login_server.exe' -AgentExe 'mxh_agent_server_CHINA.exe' -MapExe 'mxh_map_server_CHINA.exe' -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -MapNumber 10 -ResourceRoot 'D:\MX' -ServerResourceRoot 'D:\MX\Resource\Server' -Backend 'sqlite')
Assert-True ($plan.Count -eq 3) 'plan must start three servers'
Assert-True (($plan[0].Role + ',' + $plan[1].Role + ',' + $plan[2].Role) -eq 'map,agent,login') 'start order must be map, agent, login'
Assert-True (@($plan[1].Arguments) -contains '--default-map') 'agent args must include --default-map'
Assert-True (@($plan[1].Arguments) -contains '10') 'default map must be 10'
Assert-True (@($plan[0].Arguments) -contains '127.0.0.1') 'map must bind loopback'
Assert-True (@($plan[2].Arguments) -contains '127.0.0.1') 'login must advertise loopback'
Assert-True (-not (@($plan[0].Arguments) -contains '--legacy')) 'map command stays without --legacy'
Assert-True (@($plan[1].Arguments) -contains '--legacy') 'agent command includes --legacy'

$pool = @(Get-SoakCharacterSlots -CharacterPool @('alpha:hero', 'beta:side') -Clients 2 -RunId 'abcd1234')
Assert-True ($pool[0].Account -eq 'alpha' -and $pool[0].Character -eq 'hero') 'pool slot 0 must be reused as given'
Assert-True ($pool[1].Account -eq 'beta' -and $pool[1].Character -eq 'side') 'pool slot 1 must be reused as given'
$generated = @(Get-SoakCharacterSlots -CharacterPool @() -Clients 2 -RunId 'abcd1234')
Assert-True ($generated[0].Account -eq $generated[0].Character) 'generated account and character match'
Assert-True ($generated[0].Account -ne $generated[1].Account) 'generated slots must differ'
$defaultArgs = @(Get-SoakClientArguments -Account $pool[0].Account -Character $pool[0].Character -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002 -MapNumber 10 -TimeoutSeconds 180)
Assert-True (@($defaultArgs) -contains '--no-spawn') 'client must use --no-spawn'
Assert-True (@($defaultArgs) -contains '--account') 'client must pass --account'
Assert-True (@($defaultArgs) -contains '--character-name') 'client must pass --character-name'
Assert-True (-not (@($defaultArgs) -contains '--hold-seconds')) 'hold hook must stay off'
Assert-True (-not (@($defaultArgs) -contains '--expect-peer')) 'expect-peer hook must stay off'
Assert-True (-not (@($defaultArgs) -contains '--drop-socket-after')) 'drop-socket hook must stay off'
Assert-True (-not (@($defaultArgs) -contains '--state-dump')) 'state-dump hook must stay off'
$hooked = @(Get-SoakClientArguments -Account 'alpha' -Character 'hero' -LoginPort 26001 -AgentPort 27001 -MapPort 28001 -Map12Port 28002 -MapNumber 10 -TimeoutSeconds 30 -HoldSeconds 5 -ExpectPeer 'side' -DropSocketAfterSeconds 3 -StateDumpPath 'D:\soak\state.json')
Assert-True (@($hooked) -contains '--hold-seconds') 'hold hook must be passable'
Assert-True (@($hooked) -contains '--expect-peer') 'expect-peer hook must be passable'
Assert-True (@($hooked) -contains '--drop-socket-after') 'drop-socket hook must be passable'
Assert-True (@($hooked) -contains '--state-dump') 'state-dump hook must be passable'

function Test-PidAlive {
    param([int]$ProcessId)
    if ($ProcessId -le 0) { return $false }
    $statusPath = '/proc/' + $ProcessId + '/status'
    if (Test-Path -LiteralPath $statusPath) {
        foreach ($line in [IO.File]::ReadLines($statusPath)) {
            if ($line.StartsWith('State:')) {
                $state = $line.Substring(6).Trim()
                if ($state.StartsWith('Z') -or $state.StartsWith('X')) { return $false }
                return $true
            }
        }
    }
    $proc = $null
    try { $proc = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue } catch { $proc = $null }
    if ($null -eq $proc) { return $false }
    try { return -not [bool]$proc.HasExited } catch { return $false }
}

$root = Join-Path ([IO.Path]::GetTempPath()) ('soak-owned-' + [Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($root)
$parentScript = Join-Path $root 'mxh_map_server_CHINA.exe'
$parentBody = "#!/bin/sh`nsleep 120 &`necho `$! > `"`$1`"`nwait`n"
[IO.File]::WriteAllText($parentScript, $parentBody.Replace('`n', "`n").Replace('`"', '"').Replace('`$', '$'))
& chmod +x $parentScript
$pidFile = Join-Path $root 'child.pid'
$owned = $null
$stranger = $null
$decoy = $null
$childId = 0
$childIdentity = $null
$strangerBorn = $null
$decoyBorn = $null
try {
    $owned = Start-SoakOwnedProcess -FilePath $parentScript -ArgumentList @($pidFile) -StdoutPath (Join-Path $root 'owned.out') -StderrPath (Join-Path $root 'owned.err')
    $deadline = [datetime]::UtcNow.AddSeconds(5)
    while ((-not [IO.File]::Exists($pidFile)) -and [datetime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 50 }
    Assert-True ([IO.File]::Exists($pidFile)) 'owned parent did not record its child'
    $childId = [int]([IO.File]::ReadAllText($pidFile).Trim())
    $childProc = $null
    try { $childProc = Get-Process -Id $childId -ErrorAction SilentlyContinue } catch { $childProc = $null }
    if ($null -ne $childProc) { $childIdentity = Get-CommercialSmokeOwnedIdentity -Process $childProc }
    $stranger = Start-Process -FilePath '/bin/sleep' -ArgumentList @('120') -PassThru -RedirectStandardOutput (Join-Path $root 'stranger.out') -RedirectStandardError (Join-Path $root 'stranger.err')
    if ($null -ne $stranger) { $strangerBorn = Get-CommercialSmokeOwnedIdentity -Process $stranger }
    $decoy = Start-Process -FilePath '/bin/sleep' -ArgumentList @('120') -PassThru -RedirectStandardOutput (Join-Path $root 'decoy.out') -RedirectStandardError (Join-Path $root 'decoy.err')
    if ($null -ne $decoy) { $decoyBorn = Get-CommercialSmokeOwnedIdentity -Process $decoy }
    Assert-True (Test-PidAlive $owned.Identity.Id) 'owned parent should be alive before stop'
    Assert-True (Test-PidAlive $childId) 'owned child should be alive before stop'
    Assert-True (Test-PidAlive $stranger.Id) 'same-name stranger should be alive before stop'
    Assert-True (Test-PidAlive $decoy.Id) 'decoy should be alive before stop'
    $fake = [pscustomobject]@{ Id = [int]$decoy.Id; StartTicks = [int64]1; KernelStart = [int64]1 }
    $fakeStop = Stop-SoakOwnedTree -Identity $fake
    Assert-True ($fakeStop.Action -eq 'reused' -or $fakeStop.Action -eq 'dead') 'mismatched start time must not be treated as a match'
    Assert-True (@($fakeStop.KilledIds).Count -eq 0) 'mismatched start time must not kill'
    Assert-True (Test-PidAlive $decoy.Id) 'process with a mismatched start time must survive'
    $stopped = Stop-SoakOwnedTree -Identity $owned.Identity
    Assert-True ($stopped.Action -eq 'stopped') ('owned tree should stop, got ' + $stopped.Action)
    $waitUntil = [datetime]::UtcNow.AddSeconds(3)
    while ([datetime]::UtcNow -lt $waitUntil -and ((Test-PidAlive $owned.Identity.Id) -or (Test-PidAlive $childId))) {
        Start-Sleep -Milliseconds 50
    }
    Assert-True (-not (Test-PidAlive $owned.Identity.Id)) 'owned parent must exit'
    Assert-True (-not (Test-PidAlive $childId)) 'owned child must exit with the tree'
    Assert-True (Test-PidAlive $stranger.Id) 'stub with the same sleep name that this run did not start must survive'
    Assert-True (Test-PidAlive $decoy.Id) 'decoy must still be alive after owned cleanup'

    $sample = Get-SoakProcessSample -Identity $owned.Identity -Name 'mxh_map_server_CHINA' -TimestampUtc ([datetime]::UtcNow)
    Assert-True (-not $sample.Alive) 'sample must not treat a stopped owned pid as alive'
    $strangerIdentity = Get-CommercialSmokeOwnedIdentity -Process $stranger
    $live = Get-SoakProcessSample -Identity $strangerIdentity -Name 'sleep' -TimestampUtc ([datetime]::UtcNow)
    Assert-True $live.Alive 'sample of a live owned identity must succeed'
    $header = $false
    $csv = Join-Path $root 'samples.csv'
    Write-SoakSampleRow -Path $csv -Sample $live -HeaderWritten ([ref]$header)
    $csvText = [IO.File]::ReadAllText($csv)
    Assert-True ($csvText.StartsWith('timestamp_utc,pid,name,private_bytes,working_set,handle_count,thread_count,cpu_seconds')) 'samples header mismatch'
    Assert-True ($csvText.Contains([string]$stranger.Id)) 'samples row must contain the pid'
    $bytes = [IO.File]::ReadAllBytes($csv)
    Assert-True ($bytes.Length -lt 3 -or -not ($bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191)) 'csv must be UTF-8 without BOM'
} finally {
    # Stop only the PID+start-time pairs captured when this test created or
    # learned them. Stop-SoakOwnedTree re-checks that pair and does not kill
    # a reused PID.
    $cleanupIdentities = New-Object System.Collections.Generic.List[object]
    if ($null -ne $owned -and $null -ne $owned.Identity) { $cleanupIdentities.Add($owned.Identity) }
    if ($null -ne $childIdentity) { $cleanupIdentities.Add($childIdentity) }
    if ($null -ne $strangerBorn) { $cleanupIdentities.Add($strangerBorn) }
    if ($null -ne $decoyBorn) { $cleanupIdentities.Add($decoyBorn) }
    foreach ($identity in @($cleanupIdentities.ToArray())) {
        Stop-SoakOwnedTree -Identity $identity | Out-Null
    }
    if ([IO.Directory]::Exists($root)) { [IO.Directory]::Delete($root, $true) }
}

$pwsh = (Get-Process -Id $PID).Path
$refuse = & $pwsh -NoProfile -File $soak -RunDir 'C:\soak-refused' -AssumeWindowsPlatform -DurationHours 1 -WarmupHours 0.1
$refuseText = @($refuse) -join "`n"
Assert-True ($LASTEXITCODE -ne 0) 'C:\ RunDir must exit non-zero'
Assert-True ($refuseText.Contains('SOAK FAIL')) 'C:\ RunDir must print SOAK FAIL'
Assert-True ($refuseText.Contains('C:\')) 'C:\ refusal text missing'
Assert-True ($refuseText.Contains('EXIT=')) 'EXIT line missing'
Assert-True ($refuseText.Contains('EXIT=1')) 'preflight refusal must be EXIT=1'
Assert-True (-not [IO.Directory]::Exists('C:\soak-refused')) 'refused RunDir must not be created'

Write-SoakLine ('SOAK_TEST_ASSERTIONS=' + $script:Asserts)
Write-SoakLine 'SOAK_TEST PASS'
