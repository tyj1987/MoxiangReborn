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
Assert-True ($mainText.Contains('do not call start_modern.ps1')) 'header should say ephemeral mode does not call start_modern'
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
Assert-True ((@($spec.Processes | ForEach-Object { $_.Name }) -join ',') -eq 'map,agent,login') 'process order must be map, agent, login'
$mapArgs = @($spec.Processes[0].Arguments)
$agentArgs = @($spec.Processes[1].Arguments)
$loginArgs = @($spec.Processes[2].Arguments)
Assert-True ($mapArgs -notcontains '--legacy') 'map command must match start_modern (no --legacy)'
Assert-True ($agentArgs -contains '--legacy') 'agent command must include --legacy'
Assert-True ($loginArgs -contains '--legacy') 'login command must include --legacy'
Assert-True ($mapArgs -notcontains '0.0.0.0') 'map must not bind 0.0.0.0'
Assert-True ($agentArgs[[array]::IndexOf($agentArgs, '--map-server') + 1] -eq '127.0.0.1:18001') 'agent map endpoint'
Assert-True ($loginArgs[[array]::IndexOf($loginArgs, '--agent-addr') + 1] -eq '127.0.0.1') 'login advertised agent'
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

$outside = Join-Path $env:HOME ('mxh-not-temp-' + [Guid]::NewGuid().ToString('N'))
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

Write-Output 'PASS: gui-client-smoke ephemeral safety checks'
