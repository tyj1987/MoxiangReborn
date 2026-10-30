[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Method,[string]$Stage="production")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$project = Join-Path $repo 'unity\MoxiangClient'
$out = Join-Path $repo 'modern\out\unity-remaster\wuguan-training-hall'
$unity = 'C:\Program Files\Unity\Hub\Editor\6000.6.0f1\Editor\Unity.exe'
$upm = 'C:\Program Files\Unity\Hub\Editor\6000.6.0f1\Editor\Data\Resources\PackageManager\Server\UnityPackageManager.exe'
if (-not (Test-Path -LiteralPath $unity -PathType Leaf)) { throw 'Pinned Unity 6000.6.0f1 is missing.' }
if (-not (Test-Path -LiteralPath $upm -PathType Leaf)) { throw 'Pinned Unity Package Manager is missing.' }
New-Item -ItemType Directory -Path $out -Force | Out-Null
$userRoot = [Environment]::GetFolderPath('UserProfile')
$env:USERPROFILE = $userRoot
$env:HOMEDRIVE = [IO.Path]::GetPathRoot($userRoot).TrimEnd('\')
$env:HOMEPATH = $userRoot.Substring($env:HOMEDRIVE.Length)
$env:HOME = $userRoot
$env:LOCALAPPDATA = Join-Path $userRoot 'AppData\Local'
$env:APPDATA = Join-Path $userRoot 'AppData\Roaming'
$env:PROGRAMDATA = 'C:\ProgramData'
$env:ALLUSERSPROFILE = 'C:\ProgramData'
$env:TEMP = Join-Path $env:LOCALAPPDATA 'Temp'
$env:TMP = $env:TEMP
$env:UPM_CACHE_ROOT = Join-Path $env:LOCALAPPDATA 'Unity\cache'
$env:BEE_CACHE_DIRECTORY = Join-Path $env:LOCALAPPDATA 'Unity\Caches\bee'
function Invoke-MoxiangUnity([string[]]$Arguments,[string]$Name) {
    $ipc = 'Unity-Upm-' + $PID
    $hint = 'Upm-' + $PID
    $upmLog = Join-Path $out ($Name + '-upm.log')
    $unityLog = Join-Path $out ($Name + '-unity.log')
    $server = Start-Process -FilePath $upm -ArgumentList @('server','-s',$PID,'--ipc-path',$ipc,'-l','2','--log-file',$upmLog) -PassThru -NoNewWindow
    try {
        Start-Sleep -Seconds 2
        if ($server.HasExited) { throw "UPM exited before Unity launch: $Name" }
        $args = @('-batchmode','-projectPath',$project,'-upmIpcPath',$hint) + $Arguments + @('-logFile',$unityLog)
        $editor = Start-Process -FilePath $unity -ArgumentList $args -PassThru -NoNewWindow
        $processHandle = $editor.Handle
        if (-not $editor.WaitForExit(540000)) { $editor.Kill(); throw "Unity timed out: $Name" }
        if ($editor.ExitCode -ne 0) { throw "Unity step failed: $Name exit=$($editor.ExitCode). See $unityLog" }
    } finally {
        if (-not $server.HasExited) { Stop-Process -Id $server.Id -Force }
    }
}

Invoke-MoxiangUnity @('-quit','-executeMethod',$Method) $Stage
Write-Output "WUGUAN_EDITOR_OK method=$Method"
