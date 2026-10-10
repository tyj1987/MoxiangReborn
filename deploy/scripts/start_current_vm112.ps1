[CmdletBinding()]
param(
    [switch]$Start,
    [string]$DatabaseConfigEnv = 'MXH_DATABASE_CONFIG',
    [string]$DatabaseHost = '',
    [int]$LoginPort = 16001,
    [int]$AgentPort = 17001,
    [int]$MapPort = 18001,
    [int]$MapNumber = 10,
    [string]$AdvertisedAgentAddress = '192.168.2.244'
)
$ErrorActionPreference = 'Stop'
$stage = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')).TrimEnd('\')
$manifestPath = Join-Path $stage 'stage-manifest.json'
if (-not (Test-Path $manifestPath -PathType Leaf)) { throw 'stage-manifest.json is missing' }
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
if ([int]$manifest.schema -ne 2 -or [string]$manifest.stage -notlike '*stage-*') { throw 'current stage manifest identity mismatch' }
if ([string]$manifest.writerContract.databaseHost -ne '192.168.2.238' -or
    [string]$manifest.writerContract.database -ne 'MoxiangOriginalTest_20261009' -or
    [string]$manifest.writerContract.resourceProfile -ne 'playdh-current') { throw 'writer contract mismatch' }
if ($AdvertisedAgentAddress -ne '192.168.2.244' -or $DatabaseHost -ne '192.168.2.238') { throw 'VM112 binding mismatch' }
$resourceRoot = Join-Path $stage 'modern\data\PlayDH'
$serverResourceRoot = Join-Path $resourceRoot 'Resource\Server'
$resourceManifestPath = Join-Path $stage 'reference\manifests\playdh-current.sha256.json'
if (-not (Test-Path $resourceManifestPath -PathType Leaf)) { throw 'resource manifest missing' }
$resourceManifest = Get-Content $resourceManifestPath -Raw | ConvertFrom-Json
$actual = @(Get-ChildItem $resourceRoot -File -Recurse)
if ($actual.Count -ne [int]$resourceManifest.fileCount -or [int64](($actual | Measure-Object Length -Sum).Sum) -ne [int64]$resourceManifest.byteCount) { throw 'resource manifest count/size mismatch' }
foreach ($entry in @($manifest.files)) {
    $path = Join-Path $stage ([string]$entry.path -replace '/', '\')
    if (-not (Test-Path $path -PathType Leaf)) { throw "missing stage file: $($entry.path)" }
    if ((Get-Item $path).Length -ne [int64]$entry.bytes -or (Get-FileHash $path -Algorithm SHA256).Hash -ine [string]$entry.sha256) { throw "stage hash mismatch: $($entry.path)" }
}
$bin = Join-Path $stage 'modern\build\tools'
$runtime = Join-Path $stage 'runtime'
$mapExe = Join-Path $bin 'MoxianMapServer\mxh_map_server_CHINA.exe'
$agentExe = Join-Path $bin 'MoxianAgentServer\mxh_agent_server_CHINA.exe'
$loginExe = Join-Path $bin 'MoxianLoginServer\mxh_login_server.exe'
$state = Join-Path $stage 'run\pids-current.json'
if (-not $Start) { [ordered]@{mode='plan-only';stage=$stage;sourceHead=$manifest.sourceHead;ports=@($LoginPort,$AgentPort,$MapPort);startsProcesses=$false} | ConvertTo-Json; return }
if (Test-Path $state) { throw 'current stage PID file already exists' }
$cfg = [Environment]::GetEnvironmentVariable($DatabaseConfigEnv, 'Process')
if ([string]::IsNullOrWhiteSpace($cfg) -or $cfg -notmatch '(?i)(^|;)backend=mssql_odbc(;|$)' -or $cfg -notmatch '(?i)(^|;)host=192\.168\.2\.238(;|$)' -or $cfg -notmatch '(?i)(^|;)database=MoxiangOriginalTest_20261009(;|$)') { throw 'protected database config missing or mismatched' }
foreach ($port in @($LoginPort,$AgentPort,$MapPort)) { if (@(Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue).Count) { throw "port already listening: $port" } }
$log = Join-Path $stage 'run\logs-current'; New-Item $log -ItemType Directory -Force | Out-Null
$env:PATH = "$runtime;$env:PATH"
$specs = @(
    [pscustomobject]@{name='map10';exe=$mapExe;port=$MapPort;args=@('--port',$MapPort,'--map',$MapNumber,'--bind-address','192.168.2.244','--resource-root',$resourceRoot,'--server-resource-root',$serverResourceRoot,'--resource-profile','playdh-current','--backend','mssql_odbc','--db-env',$DatabaseConfigEnv)},
    [pscustomobject]@{name='agent';exe=$agentExe;port=$AgentPort;args=@('--port',$AgentPort,'--bind-address','192.168.2.244','--map-server',"192.168.2.244:$MapPort",'--map-server-map','12=192.168.2.244:18002','--map-server-map','2=192.168.2.244:18003','--default-map',$MapNumber,'--backend','mssql_odbc','--db-env',$DatabaseConfigEnv,'--legacy')},
    [pscustomobject]@{name='login';exe=$loginExe;port=$LoginPort;args=@('--port',$LoginPort,'--bind-address','192.168.2.244','--agent-addr','192.168.2.244','--agent-port',$AgentPort,'--backend','mssql_odbc','--db-env',$DatabaseConfigEnv,'--legacy')}
)
$started=@(); $rows=@()
try { foreach ($s in $specs) { $p=Start-Process -FilePath $s.exe -ArgumentList $s.args -WorkingDirectory (Split-Path $s.exe) -RedirectStandardOutput (Join-Path $log "$($s.name).out.log") -RedirectStandardError (Join-Path $log "$($s.name).err.log") -WindowStyle Hidden -PassThru; $started += $p; Start-Sleep -Milliseconds 500; if ($p.HasExited) { throw "$($s.name) exited: $($p.ExitCode)" }; $rows += [ordered]@{name=$s.name;pid=$p.Id;exe=$s.exe;port=$s.port;sha256=(Get-FileHash $s.exe -Algorithm SHA256).Hash.ToLowerInvariant();startUtc=(Get-Date).ToUniversalTime().ToString('o')}; $rows | ConvertTo-Json -Depth 4 | Set-Content $state -Encoding utf8 }; [ordered]@{stage=$stage;started=$rows;state=$state}|ConvertTo-Json -Depth 5 } catch { foreach($p in $started){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}; Remove-Item $state -Force -ErrorAction SilentlyContinue; throw }
