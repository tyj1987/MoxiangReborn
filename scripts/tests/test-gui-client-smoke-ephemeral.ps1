# Parameter and cleanup checks for -EphemeralLocalServer.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-gui-client-smoke-ephemeral.ps1
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$helper = Join-Path $repoRoot 'scripts\gui-client-smoke-ephemeral.ps1'
$main = Join-Path $repoRoot 'scripts\gui-client-smoke.ps1'
. $helper

function Assert-True($condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
}

function Assert-Parses([string]$Path) {
    $tokens = $null
    $errors = $null
    $null = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$errors)
    if ($errors -and @($errors).Count -gt 0) {
        throw "FAIL: parse error in ${Path}: $($errors[0])"
    }
}

Assert-Parses $helper
Assert-Parses $main

$tokens = $null
$errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($main, [ref]$tokens, [ref]$errors)
$switch = @($ast.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -eq 'EphemeralLocalServer' })
Assert-True ($switch.Count -eq 1) 'EphemeralLocalServer parameter missing'
Assert-True ($switch[0].StaticType.Name -eq 'SwitchParameter') 'EphemeralLocalServer must be a switch'
$mainText = [IO.File]::ReadAllText($main)
Assert-True ($mainText.Contains('& $serverScript -Mode start -Backend sqlite -DataDir $dataRoot -MapNumber $MapNumber')) 'default start line changed'
Assert-True ($mainText.Contains('& $serverScript -Mode stop')) 'default stop line missing'
Assert-True ($mainText.Contains("'--resource-root'")) 'resource-root argument missing'
Assert-True ($mainText.Contains('MXH_GUI_SMOKE_RENDER_ENTITIES')) 'render entities env missing'
Assert-True ($mainText.Contains('if ($null -ne $exitCode -and $exitCode -ne 0)')) 'client exit-code check changed'
Assert-True ($mainText.Contains('$clientLoginPort = ''16001''')) 'switch-off login port must stay 16001'
Assert-True ($mainText.Contains('$clientMapPort = ''18001''')) 'switch-off map port must stay 18001'
Assert-True ($mainText.Contains('$clientLoginPort = [string]$ephemeralPortPlan.LoginPort')) 'switch-on login port must follow PortBase'
Assert-True ($mainText.Contains('Get-EphemeralPortInUseViolation -Ports $boundPorts')) 'port check must cover the listeners the ephemeral spec binds'
Assert-True ($mainText.Contains('Map17 listen port $($map17Port[0]) is occupied; refusing to fall back to another port')) 'Map17 occupied port must fail without choosing another port'
Assert-True ($mainText.Contains("INSERT INTO character_info")) 'MapNumber smoke must pre-save the character'
Assert-True ($mainText.Contains('must not live-create')) 'MapNumber smoke must reject live character creation'
Assert-True ($mainText.Contains('verify-map17-birth.py')) 'live birth must use the Map17 assertion script'
Assert-True ($mainText.Contains('if ($LiveCharacterBirth -and $configuredStartArea -eq 17)')) 'Map17 frame script runs only for configured StartArea 17'
Assert-True ($mainText.Contains('-not $LiveCharacterBirth -and $MapNumber -eq 10 -and $monsterCount -ne 228')) 'Map10 228 threshold must not apply to live birth'
Assert-True ($mainText.Contains('if ($serverProcess.HasExited)')) 'startup loop must check HasExited'
Assert-True ($mainText.Contains('EphemeralLocalServer requires PowerShell 7.4 or newer')) 'pwsh 7.4 gate missing'
Assert-True (-not $mainText.Contains('Get-EphemeralClientEndpointViolation')) 'client fallback intercept must be gone'
Assert-True ($mainText.Contains('do not call start_modern.ps1')) 'header should say ephemeral mode does not call start_modern'
$timeoutParam = @($ast.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -eq 'TimeoutSeconds' })
Assert-True ($timeoutParam.Count -eq 1) 'TimeoutSeconds parameter missing'
Assert-True ($timeoutParam[0].DefaultValue.Extent.Text -eq '90') "TimeoutSeconds default must be 90, got $($timeoutParam[0].DefaultValue.Extent.Text)"
Assert-True ($mainText.Contains('GUI client timeout: process still running after ${effectiveTimeoutSeconds}s')) 'timeout failure must say timeout and print the seconds used'
$helperText = [IO.File]::ReadAllText($helper)
Assert-True (-not $helperText.Contains('Start-Process')) 'helper must not start processes'
Assert-True (-not $helperText.Contains('start_modern.ps1"') ) 'helper must not invoke start_modern'

function New-ValidSafetyArgs {
    $dir = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
    $file = Join-Path $dir 'moxian.db'
    $config = 'backend=sqlite;path=' + $file
    return @{
        ListenAddresses = @('127.0.0.1', '127.0.0.1', '127.0.0.1')
        ClientConnectAddresses = @('127.0.0.1', '127.0.0.1', '127.0.0.1')
        DatabaseDirectory = $dir
        DatabaseFile = $file
        DatabaseConfig = $config
        RegisterDatabaseArgument = ('sqlite;path=' + $file)
        ServerArguments = @('--bind-address', '127.0.0.1', '--backend', 'sqlite', '--db-env', 'MXH_DATABASE_CONFIG', '--port', '16001')
        ServerEnvironment = @{ MXH_DATABASE_CONFIG = $config; PATH = 'C:\Program Files\ODBC\170\bin' }
        ClientArguments = @('--login-host', '127.0.0.1', '--resource-root', 'C:\playdh')
        ExistingDatabaseConfig = ''
        ExistingLoginHost = ''
    }
}

$valid = New-ValidSafetyArgs
$ok = Get-EphemeralSafetyViolation @valid
Assert-True ([string]::IsNullOrEmpty($ok)) "valid plan rejected: $ok"

function Assert-Rejects([string]$code, [hashtable]$override) {
    $case = New-ValidSafetyArgs
    foreach ($key in @($override.Keys)) { $case[$key] = $override[$key] }
    $violation = Get-EphemeralSafetyViolation @case
    Assert-True ($violation -like "ephemeral safety: $code*") "expected $code but got '$violation'"
}

Assert-Rejects 'non-loopback' @{ ListenAddresses = @('10.0.0.1') }
Assert-Rejects 'non-loopback' @{ ListenAddresses = @('0.0.0.0') }
Assert-Rejects 'non-loopback' @{ ClientConnectAddresses = @('localhost') }
Assert-Rejects 'remote-login-host' @{ ClientArguments = @('--login-host', '192.168.2.244') }
Assert-Rejects 'remote-login-host' @{ ExistingLoginHost = '10.1.1.1' }
Assert-Rejects 'mssql-marker' @{ ServerArguments = @('--bind-address', '127.0.0.1', 'Server=tcp:db.example') }
Assert-Rejects 'mssql-marker' @{ ServerArguments = @('--bind-address', '127.0.0.1', 'Data Source=prod') }
Assert-Rejects 'mssql-marker' @{ DatabaseConfig = 'backend=mssql_odbc;host=127.0.0.1' }
Assert-Rejects 'database-not-sqlite' @{ ExistingDatabaseConfig = 'backend=mssql_odbc;host=127.0.0.1;database=Moxiang' }
Assert-Rejects 'mssql-marker' @{ ServerEnvironment = @{ MXH_DATABASE_CONFIG = 'backend=sqlite;path=C:\t\moxian.db'; MXH_ODBC = 'odbc_driver=SQL Server' } }
Assert-Rejects 'non-loopback' @{ ServerEnvironment = @{ BIND = '10.9.8.7' } }
Assert-Rejects 'database-not-temp' @{ DatabaseDirectory = (Join-Path $repoRoot 'not-a-temp-db') }
Assert-True (-not (Test-EphemeralDeletableDatabaseDirectory (Get-EphemeralNormalizedTempRoot))) 'temp root itself must not be deletable'

$dir = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
$file = Join-Path $dir 'moxian.db'
$spec = New-EphemeralServerLaunchSpec `
    -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
    -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
    -LoginExecutable 'C:\build\mxh_login_server.exe' `
    -MapNumber 10 `
    -ResourceRoot 'C:\playdh' -ServerResourceRoot 'C:\playdh\Resource\Server' `
    -DatabaseDirectory $dir `
    -ProcessEnvironment @{ PATH = 'C:\Program Files\ODBC\170\bin'; MXH_FOG_DISABLE = '1'; MXH_STM_LIGHTING = '1' } `
    -ClientArguments @('--login-host', '127.0.0.1', '--resource-root', 'C:\playdh')
Assert-True ((@($spec.Processes | ForEach-Object { $_.Name }) -join ',') -eq 'map,agent,login') 'default process order must be map, agent, login'
$mapArgs = @($spec.Processes[0].Arguments)
$agentArgs = @($spec.Processes[1].Arguments)
$loginArgs = @($spec.Processes[2].Arguments)
Assert-True ($mapArgs -notcontains '--legacy') 'map command must match start_modern (no --legacy)'
Assert-True ($agentArgs -contains '--legacy') 'agent command must include --legacy'
Assert-True ($loginArgs -contains '--legacy') 'login command must include --legacy'
Assert-True ($mapArgs -notcontains '0.0.0.0') 'map must not bind 0.0.0.0'
Assert-True ($mapArgs[[array]::IndexOf($mapArgs, '--port') + 1] -eq '28001') 'default map port follows 26001+2000'
Assert-True ($agentArgs[[array]::IndexOf($agentArgs, '--port') + 1] -eq '27001') 'default agent port follows 26001+1000'
Assert-True ($loginArgs[[array]::IndexOf($loginArgs, '--port') + 1] -eq '26001') 'default login port is 26001'
Assert-True ($agentArgs[[array]::IndexOf($agentArgs, '--map-server') + 1] -eq '127.0.0.1:28001') 'agent map endpoint'
Assert-True ($agentArgs[[array]::IndexOf($agentArgs, '--default-map') + 1] -eq '10') 'default StartArea follows --default-map'
Assert-True ($agentArgs -notcontains '--start-area') 'default launch must not pass --start-area'
Assert-True ($loginArgs[[array]::IndexOf($loginArgs, '--agent-addr') + 1] -eq '127.0.0.1') 'login advertised agent'
Assert-True ($loginArgs[[array]::IndexOf($loginArgs, '--agent-port') + 1] -eq '27001') 'login must not keep the 7001 agent default'
Assert-True ($loginArgs[[array]::IndexOf($loginArgs, '--bind-address') + 1] -eq '127.0.0.1') 'login bind'
Assert-True ($spec.RegisterDatabaseArgument -eq ('sqlite;path=' + $file)) 'register db argument'
Assert-True ($spec.DatabaseConfig -eq ('backend=sqlite;path=' + $file)) 'server database config'
Assert-True ($spec.Environment['MXH_DATABASE_CONFIG'] -eq $spec.DatabaseConfig) 'server env database config'
Assert-True ($spec.Environment['MXH_FOG_DISABLE'] -eq '1') 'render env must stay in the server environment copy'
$rejected = $false
try {
    New-EphemeralServerLaunchSpec `
        -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
        -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
        -LoginExecutable 'C:\build\mxh_login_server.exe' `
        -MapNumber 10 -ResourceRoot 'C:\playdh' -ServerResourceRoot 'C:\playdh\Resource\Server' `
        -DatabaseDirectory $dir `
        -ProcessEnvironment @{ MXH_DATABASE_CONFIG = 'backend=mssql_odbc;host=10.0.0.5;database=Moxiang' } `
        -ClientArguments @('--login-host', '127.0.0.1') | Out-Null
} catch {
    $rejected = $_.Exception.Message -like '*database-not-sqlite*'
}
Assert-True $rejected 'pre-set MSSQL MXH_DATABASE_CONFIG must be rejected'

Assert-True ((Format-EphemeralCleanupLine -PidsStopped $true -DbRemoved $true) -eq '[ephemeral] cleanup pids_stopped=yes db_removed=yes') 'cleanup line yes'
Assert-True ((Format-EphemeralCleanupLine -PidsStopped $false -DbRemoved $true) -eq '[ephemeral] cleanup pids_stopped=no db_removed=yes') 'cleanup line pid failure'

$liveDir = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $liveDir | Out-Null
$shell = (Get-Process -Id $PID).Path
$child = Start-Process -FilePath $shell -ArgumentList @('-NoProfile', '-Command', 'Start-Sleep -Seconds 120') -PassThru
try {
    $cleanup = Invoke-EphemeralLocalCleanup -ProcessIds @([int]$child.Id) -DatabaseDirectory $liveDir
    Assert-True $cleanup.Ok "cleanup should succeed: $($cleanup.Line)"
    Assert-True ($cleanup.Line -eq '[ephemeral] cleanup pids_stopped=yes db_removed=yes') 'live cleanup line'
    Assert-True (-not (Test-Path -LiteralPath $liveDir)) 'temp database directory still exists'
    Assert-True ($null -eq (Get-Process -Id $child.Id -ErrorAction SilentlyContinue)) 'recorded pid still exists'
} finally {
    if (Get-Process -Id $child.Id -ErrorAction SilentlyContinue) {
        Stop-Process -Id $child.Id -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path -LiteralPath $liveDir) { Remove-Item -LiteralPath $liveDir -Recurse -Force -ErrorAction SilentlyContinue }
}

# Windows has no $env:HOME; use the profile directory (outside the temp root) on every OS.
$outsideBase = [Environment]::GetFolderPath('UserProfile')
if ([string]::IsNullOrEmpty($outsideBase)) { $outsideBase = $env:USERPROFILE }
if ([string]::IsNullOrEmpty($outsideBase)) { $outsideBase = $env:HOME }
Assert-True (-not [string]::IsNullOrEmpty($outsideBase)) 'no user profile directory for the outside-temp cleanup case'
$outside = Join-Path $outsideBase ('mxh-not-temp-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $outside | Out-Null
try {
    $cleanup = Invoke-EphemeralLocalCleanup -ProcessIds @() -DatabaseDirectory $outside
    Assert-True (Test-Path -LiteralPath $outside) 'cleanup deleted a directory outside the temp root'
    Assert-True (-not $cleanup.Ok) 'outside directory must fail verification'
    Assert-True ($cleanup.Line -eq '[ephemeral] cleanup pids_stopped=yes db_removed=no') "outside cleanup line: $($cleanup.Line)"
} finally {
    if (Test-Path -LiteralPath $outside) { Remove-Item -LiteralPath $outside -Recurse -Force -ErrorAction SilentlyContinue }
}

$missingBuild = Join-Path ([IO.Path]::GetTempPath()) ('mxh-missing-build-' + [Guid]::NewGuid().ToString('N'))
$stdoutPath = Join-Path ([IO.Path]::GetTempPath()) ('mxh-smoke-stdout-' + [Guid]::NewGuid().ToString('N'))
$stderrPath = Join-Path ([IO.Path]::GetTempPath()) ('mxh-smoke-stderr-' + [Guid]::NewGuid().ToString('N'))
$proc = Start-Process -FilePath $shell -ArgumentList @('-NoProfile', '-File', $main, '-EphemeralLocalServer', '-BuildDir', $missingBuild) `
    -Wait -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
$stdoutLines = @()
if (Test-Path -LiteralPath $stdoutPath) { $stdoutLines = @(Get-Content -LiteralPath $stdoutPath) }
Assert-True ($proc.ExitCode -ne 0) 'missing build with the switch must be non-zero before any server start'
Assert-True ($stdoutLines.Count -ge 1 -and $stdoutLines[0] -eq 'server=local-ephemeral') "first output line was '$($stdoutLines -join '|')'"
Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue

$stdoutPath2 = Join-Path ([IO.Path]::GetTempPath()) ('mxh-smoke-stdout-' + [Guid]::NewGuid().ToString('N'))
$stderrPath2 = Join-Path ([IO.Path]::GetTempPath()) ('mxh-smoke-stderr-' + [Guid]::NewGuid().ToString('N'))
$proc2 = Start-Process -FilePath $shell -ArgumentList @('-NoProfile', '-File', $main, '-BuildDir', $missingBuild) `
    -Wait -PassThru -RedirectStandardOutput $stdoutPath2 -RedirectStandardError $stderrPath2
$stdoutLines2 = @()
if (Test-Path -LiteralPath $stdoutPath2) { $stdoutLines2 = @(Get-Content -LiteralPath $stdoutPath2) }
Assert-True ($proc2.ExitCode -ne 0) 'missing build without the switch must still fail'
Assert-True (@($stdoutLines2 | Where-Object { $_ -eq 'server=local-ephemeral' }).Count -eq 0) 'default path printed server=local-ephemeral'
Remove-Item -LiteralPath $stdoutPath2, $stderrPath2 -Force -ErrorAction SilentlyContinue

$defaultPlan = Resolve-EphemeralPortPlan -Base 26001
Assert-True ($defaultPlan.LoginPort -eq 26001 -and $defaultPlan.AgentPort -eq 27001 -and $defaultPlan.MapPort -eq 28001 -and $defaultPlan.Map17Port -eq 28003) 'default port plan'
Assert-True ((@($defaultPlan.Ports) -join ',') -eq '26001,27001,28001') 'default plan lists login, agent, and map'

$customPlan = Resolve-EphemeralPortPlan -Base 26011
$customDir = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
$customSpec = New-EphemeralServerLaunchSpec `
    -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
    -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
    -LoginExecutable 'C:\build\mxh_login_server.exe' `
    -MapNumber 10 `
    -ResourceRoot 'C:\playdh' -ServerResourceRoot 'C:\playdh\Resource\Server' `
    -DatabaseDirectory $customDir `
    -LoginPort $customPlan.LoginPort -AgentPort $customPlan.AgentPort -MapPort $customPlan.MapPort `
    -ClientArguments @('--login-host', '127.0.0.1', '--login-port', ([string]$customPlan.LoginPort))
$customAgent = @($customSpec.Processes[1].Arguments)
$customLogin = @($customSpec.Processes[2].Arguments)
$customMap = @($customSpec.Processes[0].Arguments)
Assert-True ($customMap[[array]::IndexOf($customMap, '--port') + 1] -eq '28011') 'custom map listen'
Assert-True ($customAgent -notcontains '--start-area') 'custom default must not pass --start-area'
Assert-True ($customAgent[[array]::IndexOf($customAgent, '--map-server') + 1] -eq '127.0.0.1:28011') 'custom agent map-server'
$birthDir = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
$birthSpec = New-EphemeralServerLaunchSpec `
    -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
    -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
    -LoginExecutable 'C:\build\mxh_login_server.exe' `
    -MapNumber 12 -RegisterMap17 `
    -ResourceRoot 'C:\playdh' -ServerResourceRoot 'C:\playdh\Resource\Server' `
    -DatabaseDirectory $birthDir `
    -ClientArguments @('--login-host', '127.0.0.1', '--login-port', '26001')
Assert-True ((@($birthSpec.Processes | ForEach-Object { $_.Name }) -join ',') -eq 'map,map17,agent,login') 'explicit Map17 process order'
$birthMap17 = @($birthSpec.Processes[1].Arguments)
$birthAgent = @($birthSpec.Processes[2].Arguments)
Assert-True ($birthMap17[[array]::IndexOf($birthMap17, '--map') + 1] -eq '17') 'map17 listens as map 17'
Assert-True ($birthMap17[[array]::IndexOf($birthMap17, '--port') + 1] -eq '28003') 'map17 port is 28003'
Assert-True ($birthAgent[[array]::IndexOf($birthAgent, '--start-area') + 1] -eq '17') 'explicit StartArea is 17'
Assert-True ($birthAgent[[array]::IndexOf($birthAgent, '--map-server-map') + 1] -eq '17=127.0.0.1:28003') 'agent registers Map17'
Assert-True ($birthAgent[[array]::IndexOf($birthAgent, '--default-map') + 1] -eq '12') 'primary map stays the default-map route'
Assert-True ($customLogin[[array]::IndexOf($customLogin, '--agent-addr') + 1] -eq '127.0.0.1') 'custom login agent address'
Assert-True ($customLogin[[array]::IndexOf($customLogin, '--agent-port') + 1] -eq '27011') 'custom login agent port'
Assert-True ($customLogin[[array]::IndexOf($customLogin, '--port') + 1] -eq '26011') 'custom login listen'

# 15999 -> map17 18001, 35999 -> map17 38001. login/agent/map stay off the
# forbidden listeners, so a reserved-port failure is the map17 port alone.
$forbiddenListeners = @(16001, 17001, 18001, 36001, 37001, 38001)
foreach ($map17OnlyBase in @(15999, 35999)) {
    $derived = [ordered]@{
        login = $map17OnlyBase
        agent = $map17OnlyBase + 1000
        map = $map17OnlyBase + 2000
        map17 = $map17OnlyBase + 2002
    }
    foreach ($name in @('login', 'agent', 'map')) {
        $port = [int]$derived[$name]
        Assert-True ($forbiddenListeners -notcontains $port) "base $map17OnlyBase $name port $port is forbidden; this case no longer isolates map17"
    }
    Assert-True ($forbiddenListeners -contains [int]$derived.map17) "base $map17OnlyBase map17 port $($derived.map17) is not forbidden"
}
foreach ($reservedBase in @(16001, 15001, 14001, 36001, 35001, 34001, 36101, 35101, 34101, 36102, 36103, 15999, 35999)) {
    $reservedMessage = ''
    try {
        Resolve-EphemeralPortPlan -Base $reservedBase | Out-Null
    } catch {
        $reservedMessage = $_.Exception.Message
    }
    Assert-True ($reservedMessage -like 'ephemeral safety: reserved-port *') "base $reservedBase was '$reservedMessage'"
}
$rangeMessage = ''
try { Resolve-EphemeralPortPlan -Base 64000 | Out-Null } catch { $rangeMessage = $_.Exception.Message }
Assert-True ($rangeMessage -like 'ephemeral safety: port-out-of-range 64000*') "range message was '$rangeMessage'"

$reservedSpecMessage = ''
try {
    New-EphemeralServerLaunchSpec `
        -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
        -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
        -LoginExecutable 'C:\build\mxh_login_server.exe' `
        -MapNumber 10 -ResourceRoot 'C:\playdh' -ServerResourceRoot 'C:\playdh\Resource\Server' `
        -DatabaseDirectory $customDir `
        -LoginPort 16001 -AgentPort 27001 -MapPort 28001 `
        -ClientArguments @('--login-host', '127.0.0.1') | Out-Null
} catch {
    $reservedSpecMessage = $_.Exception.Message
}
Assert-True ($reservedSpecMessage -like '*reserved-port 16001*') "spec reserved message was '$reservedSpecMessage'"

function Assert-OccupiedPlanPort([int]$OccupiedPort) {
    $holder = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse('127.0.0.1'), $OccupiedPort)
    $holder.Start()
    try {
        $violation = Get-EphemeralPortInUseViolation -Ports @($customPlan.Ports)
        Assert-True ($violation -eq "ephemeral safety: port-in-use $OccupiedPort") "occupied $OccupiedPort reported '$violation'"
    } finally {
        $holder.Stop()
    }
}
Assert-OccupiedPlanPort ([int]$customPlan.LoginPort)
Assert-OccupiedPlanPort ([int]$customPlan.AgentPort)
Assert-OccupiedPlanPort ([int]$customPlan.MapPort)
$anyHolder = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, [int]$customPlan.MapPort)
$anyHolder.Start()
try {
    $anyViolation = Get-EphemeralPortInUseViolation -Ports @($customPlan.Ports)
    Assert-True ($anyViolation -eq "ephemeral safety: port-in-use $($customPlan.MapPort)") "0.0.0.0 listener reported '$anyViolation'"
} finally {
    $anyHolder.Stop()
}
# [::] listener on the map port: must be reported as port-in-use too.
$v6Holder = $null
$v6SkipReason = $null
if (-not [System.Net.Sockets.Socket]::OSSupportsIPv6) {
    $v6SkipReason = 'OSSupportsIPv6=false'
} else {
    try {
        $v6Holder = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::IPv6Any, [int]$customPlan.MapPort)
        $v6Holder.Start()
    } catch {
        $v6SkipReason = "cannot listen on [::]: $($_.Exception.Message)"
        $v6Holder = $null
    }
}
if ($null -ne $v6SkipReason) {
    Write-Output "[ephemeral] SKIP [::] listener case: $v6SkipReason"
} else {
    try {
        $v6Violation = Get-EphemeralPortInUseViolation -Ports @($customPlan.Ports)
        Assert-True ($v6Violation -eq "ephemeral safety: port-in-use $($customPlan.MapPort)") "[::] listener reported '$v6Violation'"
    } finally {
        $v6Holder.Stop()
    }
}
# Listener enumeration failure must fail closed (port treated as in use).
$realListeners = ${function:Get-EphemeralActiveTcpListeners}
Assert-True ($null -ne $realListeners) 'Get-EphemeralActiveTcpListeners must exist'
Set-Item -Path Function:\Get-EphemeralActiveTcpListeners -Value { throw 'simulated enumeration failure' }
try {
    $enumViolation = Get-EphemeralPortInUseViolation -Ports @($customPlan.Ports)
    Assert-True ($enumViolation -eq "ephemeral safety: port-in-use $($customPlan.LoginPort)") "enumeration failure must fail closed, got '$enumViolation'"
} finally {
    Set-Item -Path Function:\Get-EphemeralActiveTcpListeners -Value $realListeners
}
$freeViolation = Get-EphemeralPortInUseViolation -Ports @($customPlan.Ports)
Assert-True ([string]::IsNullOrEmpty($freeViolation)) "free plan rejected: $freeViolation"

$dead = Start-Process -FilePath $shell -ArgumentList @('-NoProfile', '-Command', 'exit 7') -PassThru
if (-not $dead.WaitForExit(10000)) { throw 'readiness fixture did not exit' }
$dead.Refresh()
$deadFailure = Get-EphemeralReadinessFailure -Servers @([pscustomobject]@{
    Name = 'login'; Process = $dead; Port = [int]$customPlan.LoginPort; Stderr = 'C:\login.err.log'
})
Assert-True ($deadFailure -eq 'login exited during startup (exit=7; see C:\login.err.log)') "HasExited readiness was '$deadFailure'"

$alive = Start-Process -FilePath $shell -ArgumentList @('-NoProfile', '-Command', 'Start-Sleep -Seconds 30') -PassThru
try {
    $aliveFailure = Get-EphemeralReadinessFailure -Servers @([pscustomobject]@{
        Name = 'agent'; Process = $alive; Port = [int]$customPlan.AgentPort
    })
    Assert-True ($aliveFailure -eq "agent did not accept connections on 127.0.0.1:$($customPlan.AgentPort)") "listening readiness was '$aliveFailure'"
} finally {
    if (Get-Process -Id $alive.Id -ErrorAction SilentlyContinue) {
        Stop-Process -Id $alive.Id -Force -ErrorAction SilentlyContinue
    }
}

$jdkHome = 'C:\Program Files\Eclipse Adoptium\jdk-17.0.10.7-hotspot'
$jdkPath = $jdkHome + '\bin;C:\Windows\system32'
Assert-True (-not (Test-EphemeralNonLoopbackText $jdkHome)) 'JDK directory must not look like an IP'
Assert-True (-not (Test-EphemeralNonLoopbackText $jdkPath)) 'JDK Path must not look like an IP'
Assert-True (@(Find-EphemeralIpv4Addresses $jdkHome).Count -eq 0) 'JDK directory must not match an address'
$jdkCase = New-ValidSafetyArgs
$jdkCase.ServerEnvironment = @{ JAVA_HOME = $jdkHome; Path = $jdkPath }
$jdkViolation = Get-EphemeralSafetyViolation @jdkCase
Assert-True ([string]::IsNullOrEmpty($jdkViolation)) "JDK JAVA_HOME/Path rejected: $jdkViolation"
$matchedAddresses = @(Find-EphemeralIpv4Addresses 'listen 127.0.0.1 and 10.0.0.5:16001')
Assert-True ($matchedAddresses -contains '127.0.0.1') '127.0.0.1 must still match'
Assert-True ($matchedAddresses -contains '10.0.0.5') '10.0.0.5:16001 must still match'
Assert-True (-not (Test-EphemeralNonLoopbackText '127.0.0.1')) '127.0.0.1 stays loopback'
Assert-True (Test-EphemeralNonLoopbackText '10.0.0.5:16001') '10.0.0.5 must stay non-loopback'
Assert-True (-not (Test-EphemeralNonLoopbackText '1.2.3.4.5')) 'five octets are not one address'
Assert-True (-not (Test-EphemeralNonLoopbackText '999.1.1.1')) 'octet above 255 is not an address'
Assert-True (-not (Test-EphemeralNonLoopbackText '256.1.1.1')) '256 is not an octet'
Assert-Rejects 'non-loopback' @{ ServerArguments = @('--bind-address', '10.0.0.5:16001') }

# -ResourceRoot: the passed PlayDH root reaches the client command line,
# the ephemeral map server and the audits; empty keeps the repo PlayDH.
$rrParam = @($ast.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -eq 'ResourceRoot' })
Assert-True ($rrParam.Count -eq 1) 'ResourceRoot parameter missing'
Assert-True ($rrParam[0].StaticType.Name -eq 'String') 'ResourceRoot must be a string'
Assert-True ($rrParam[0].DefaultValue.Extent.Text -eq "''") "ResourceRoot default must be empty, got $($rrParam[0].DefaultValue.Extent.Text)"
Assert-True ($mainText.Contains('$smokeResourceRoot = Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $ResourceRoot')) 'main must resolve -ResourceRoot once'
Assert-True ($mainText.Contains('Get-EphemeralResourceLayout -RepoRoot $repoRoot -ResourceRoot $smokeResourceRoot')) 'ephemeral layout must get the resolved root'
Assert-True ($mainText.Contains('& python $dependencyAudit $MapNumber $smokeResourceRoot')) 'map dependency audit must use the resolved root'
Assert-True ($mainText.Contains("(Join-Path `$smokeResourceRoot 'Resource\Client\NpcChxList.bin')")) 'NPC catalog audit must use the resolved root'
Assert-True ($mainText.Contains("(Join-Path `$smokeResourceRoot 'npc.pak')")) 'NPC pak audit must use the resolved root'
Assert-True (-not $mainText.Contains("Join-Path `$repoRoot 'modern\data\PlayDH")) 'main must not hardcode the repo PlayDH any more'

# Evaluate the real client $arguments assignment from gui-client-smoke.ps1.
$argsAssign = @($ast.FindAll({
    param($n)
    $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and
    $n.Left.Extent.Text -eq '$arguments' -and $n.Operator -eq 'Equals'
}, $true))
Assert-True ($argsAssign.Count -eq 1) "expected one client `$arguments assignment, got $($argsAssign.Count)"
function Get-SmokeClientArguments([string]$Root) {
    $smokeResourceRoot = $Root
    $clientLoginPort = '26001'; $clientMapPort = '28001'; $characterName = 'GUItest'
    $frame = 'frame.tga'; $runRoot = 'run'
    return @(& ([scriptblock]::Create($argsAssign[0].Right.Extent.Text)))
}
function Get-ArgValue([string[]]$List, [string]$Name) {
    $hits = @()
    for ($i = 0; $i -lt $List.Count - 1; $i++) { if ($List[$i] -eq $Name) { $hits += $List[$i + 1] } }
    return ,$hits
}

$rrTemp = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-rr-' + [Guid]::NewGuid().ToString('N'))
$deployRoot = Join-Path $rrTemp 'playdh-deploy'
New-Item -ItemType Directory -Force -Path $deployRoot | Out-Null
try {
    $repoPlayDh = Join-Path $repoRoot 'modern\data\PlayDH'
    Assert-True ((Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot '') -eq $repoPlayDh) 'empty -ResourceRoot must keep the repo PlayDH'
    Assert-True ((Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $null) -eq $repoPlayDh) 'null -ResourceRoot must keep the repo PlayDH'
    Assert-True ((Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $deployRoot) -eq $deployRoot) 'explicit -ResourceRoot must be returned unchanged'
    function Assert-ResourceRootRejected([string]$Value, [string]$Like, [string]$Name) {
        $message = $null
        try { $null = Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $Value } catch { $message = $_.Exception.Message }
        Assert-True ($message -like $Like) "${Name}: '$Value' expected '$Like', got '$message'"
    }
    # ResourceRoot-Relative
    foreach ($bad in @('relative\PlayDH', 'PlayDH', '.\PlayDH')) {
        Assert-ResourceRootRejected $bad '`[ERROR`] gui smoke: -ResourceRoot must be an absolute path*' 'ResourceRoot-Relative'
    }
    # ResourceRoot-Missing
    Assert-ResourceRootRejected (Join-Path $rrTemp 'missing') '`[ERROR`] gui smoke: -ResourceRoot does not exist*' 'ResourceRoot-Missing'
    # ResourceRoot-UnderDMX: after collapsing these are D:\MX or below it.
    foreach ($bad in @('D:\MX', 'D:\MX\', 'd:\mx\', 'D:\MX\.', 'd:\mx\PlayDH', 'D:/MX/data/PlayDH',
                       'D:\x\..\MX\PlayDH', 'D:\build-mxr\..\MX\PlayDH', 'D:\MX\.\PlayDH\..', '\\?\D:\MX\PlayDH', 'D:\\MX\\PlayDH')) {
        Assert-ResourceRootRejected $bad '`[ERROR`] gui smoke: -ResourceRoot resolves under D:\MX*' 'ResourceRoot-UnderDMX'
    }
    # ResourceRoot-CollapseAllowed: collapse to a path outside D:\MX, so the
    # D:\MX check passes them (they then fail only on existence/absolute rules).
    foreach ($ok in @('D:\MX\..\x', 'D:\MXR', 'D:\MX2\PlayDH', 'D:\build-mxr\playdh-99debcba\modern\data\PlayDH', 'E:\MX')) {
        $collapsedOk = Get-GuiSmokeCollapsedPathText -Path $ok
        Assert-True (-not (Test-GuiSmokePathUnderOriginalRoot -CollapsedPath $collapsedOk)) "ResourceRoot-CollapseAllowed: '$ok' -> '$collapsedOk' must not count as D:\MX"
        $message = $null
        try { $null = Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $ok } catch { $message = $_.Exception.Message }
        Assert-True ($message -notlike '*resolves under D:\MX*') "ResourceRoot-CollapseAllowed: '$ok' was refused as D:\MX: $message"
    }
    Assert-True ((Get-GuiSmokeCollapsedPathText -Path 'D:\MX\..\x') -eq 'D:\x') 'ResourceRoot-Collapse: D:\MX\..\x must collapse to D:\x'
    Assert-True ((Get-GuiSmokeCollapsedPathText -Path 'd:/x/../MX/./PlayDH/') -eq 'D:\MX\PlayDH') 'ResourceRoot-Collapse: d:/x/../MX/./PlayDH/ must collapse to D:\MX\PlayDH'
    # ResourceRoot-NotDMX: names that only start with MX are not D:\MX
    $mxLike = Join-Path $rrTemp 'MXR'
    New-Item -ItemType Directory -Force -Path $mxLike | Out-Null
    Assert-True ((Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot $mxLike) -eq $mxLike) 'ResourceRoot-NotDMX'

    $clientArgs = Get-SmokeClientArguments $deployRoot
    $rootValues = Get-ArgValue $clientArgs '--resource-root'
    Assert-True ($rootValues.Count -eq 1) "client must get exactly one --resource-root, got $($rootValues.Count)"
    Assert-True ($rootValues[0] -eq $deployRoot) "client --resource-root must be the passed root, got '$($rootValues[0])'"
    $defaultArgs = Get-SmokeClientArguments (Resolve-GuiSmokeResourceRoot -RepoRoot $repoRoot -ResourceRoot '')
    Assert-True ((Get-ArgValue $defaultArgs '--resource-root')[0] -eq $repoPlayDh) 'switch-off client --resource-root must stay the repo PlayDH'

    $savedServerRoot = [Environment]::GetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT', $null, 'Process')
        $layout = Get-EphemeralResourceLayout -RepoRoot $repoRoot -ResourceRoot $deployRoot
        Assert-True ($layout.ResourceRoot -eq $deployRoot) 'ephemeral layout must use the passed root'
        Assert-True ($layout.ServerResourceRoot -eq (Join-Path $deployRoot 'Resource\Server')) 'ephemeral server root must derive from the passed root'
        Assert-True ($layout.ResourceProfileId -eq 'playdh-current') 'resource profile id must stay playdh-current'
        $defaultLayout = Get-EphemeralResourceLayout -RepoRoot $repoRoot
        Assert-True ($defaultLayout.ResourceRoot -eq (Join-Path $repoRoot 'modern/data/PlayDH')) "default layout must stay the profile source, got '$($defaultLayout.ResourceRoot)'"
        [Environment]::SetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT', (Join-Path $rrTemp 'server-override'), 'Process')
        $overrideLayout = Get-EphemeralResourceLayout -RepoRoot $repoRoot -ResourceRoot $deployRoot
        Assert-True ($overrideLayout.ServerResourceRoot -eq (Join-Path $rrTemp 'server-override')) 'MXH_SERVER_RESOURCE_ROOT must still win for the server root'
    } finally {
        [Environment]::SetEnvironmentVariable('MXH_SERVER_RESOURCE_ROOT', $savedServerRoot, 'Process')
    }

    $rrDb = Join-Path (Get-EphemeralNormalizedTempRoot) ('mxh-gui-smoke-' + [Guid]::NewGuid().ToString('N'))
    $rrSpec = New-EphemeralServerLaunchSpec `
        -MapExecutable 'C:\build\mxh_map_server_CHINA.exe' `
        -AgentExecutable 'C:\build\mxh_agent_server_CHINA.exe' `
        -LoginExecutable 'C:\build\mxh_login_server.exe' `
        -MapNumber 10 `
        -ResourceRoot $layout.ResourceRoot -ServerResourceRoot $layout.ServerResourceRoot `
        -DatabaseDirectory $rrDb `
        -ProcessEnvironment @{} `
        -ClientArguments $clientArgs
    $rrMapArgs = [string[]]@($rrSpec.Processes[0].Arguments)
    Assert-True ((Get-ArgValue $rrMapArgs '--resource-root')[0] -eq $deployRoot) 'map server --resource-root must be the passed root'
    Assert-True ((Get-ArgValue $rrMapArgs '--server-resource-root')[0] -eq (Join-Path $deployRoot 'Resource\Server')) 'map server --server-resource-root must derive from the passed root'
} finally {
    Remove-Item -LiteralPath $rrTemp -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Output 'PASS: gui-client-smoke ephemeral safety checks'
