[CmdletBinding()]
param([switch]$BakePbr,[switch]$BakeHighLow,[switch]$BakeLighting)
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
        if (-not $editor.WaitForExit(180000)) { $editor.Kill(); throw "Unity timed out: $Name" }
        if ($editor.ExitCode -ne 0) { throw "Unity step failed: $Name exit=$($editor.ExitCode). See $unityLog" }
    } finally {
        if (-not $server.HasExited) { Stop-Process -Id $server.Id -Force }
    }
}
if ($BakePbr) {
    $python = (Get-Command python -ErrorAction Stop).Source
    & $python (Join-Path $repo 'scripts\bake-wuguan-pbr.py')
    if ($LASTEXITCODE -ne 0) { throw "Blender PBR bake failed with exit $LASTEXITCODE" }
}
if ($BakeHighLow -or $BakePbr) {
    $python = (Get-Command python -ErrorAction Stop).Source
    & $python (Join-Path $repo 'scripts\refine-wuguan-highlow.py')
    if ($LASTEXITCODE -ne 0) { throw 'High/low projection or AO bake failed.' }
}
if ($BakeLighting) { Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanLightingBake.Bake') 'lighting' }
$startedUtc = [DateTime]::UtcNow
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanTrainingHallSetup.Build') 'build'
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanTrainingHallSetup.Validate') 'validate'
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanMap44Setup.Bind') 'map44'
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanBakedPreview.Capture') 'baked-preview'
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanTrainingHallPreview.Capture') 'preview'
Invoke-MoxiangUnity @('-quit','-executeMethod','Moxiang.Editor.WuguanTrainingHallAudit.Run') 'audit'
$results = Join-Path $out 'editmode-results.xml'
if (Test-Path -LiteralPath $results) { Remove-Item -LiteralPath $results -Force }
Invoke-MoxiangUnity @('-runTests','-testPlatform','EditMode','-testFilter','Moxiang.Tests.WuguanTrainingHallTests;Moxiang.Tests.WuguanProductionTests;Moxiang.Tests.ServerEntityRegistryTests;Moxiang.Tests.MapVisualControllerTests','-testResults',$results) 'tests'
if (-not (Test-Path -LiteralPath $results -PathType Leaf)) { throw 'Unity test result XML was not produced.' }
[xml]$doc = Get-Content -LiteralPath $results
$run = $doc.'test-run'
if ($run.result -ne 'Passed' -or [int]$run.failed -ne 0 -or [int]$run.passed -lt 34 -or [int]$run.skipped -ne 0) { throw "Wuguan tests failed: result=$($run.result) failed=$($run.failed)" }
$prefab = Join-Path $project 'Assets\Moxiang\Art\WuguanTrainingHall\Prefabs\WuguanTrainingHall.prefab'
$preview = Join-Path $out 'blender-preview.png'
if (-not (Test-Path -LiteralPath $prefab -PathType Leaf)) { throw 'Wuguan prefab is missing after build.' }
if (-not (Test-Path -LiteralPath $preview -PathType Leaf)) { throw 'Blender preview is missing.' }
$audit = Join-Path $out 'audit.json'
if (-not (Test-Path -LiteralPath $audit -PathType Leaf)) { throw 'Wuguan audit report is missing.' }
$unityPreview = Join-Path $out 'unity-preview.png'
if (-not (Test-Path -LiteralPath $unityPreview -PathType Leaf)) { throw 'Unity preview is missing.' }
Add-Type -AssemblyName System.Drawing
$bitmap=[System.Drawing.Bitmap]::FromFile($unityPreview); try { if($bitmap.Width -ne 1280 -or $bitmap.Height -ne 720){throw "Unexpected Unity preview size: $($bitmap.Width)x$($bitmap.Height)"}; $sum=0.0; $n=0; $dark=0; $bright=0; for($y=0;$y -lt $bitmap.Height;$y+=12){for($x=0;$x -lt $bitmap.Width;$x+=12){$px=$bitmap.GetPixel($x,$y); $l=(0.2126*$px.R+0.7152*$px.G+0.0722*$px.B)/255.0; $sum+=$l; $n++; if($l -lt .03){$dark++}; if($l -gt .97){$bright++}}}; $mean=$sum/$n; $darkRatio=$dark/$n; $brightRatio=$bright/$n; if($mean -lt .045){throw "Unity preview too dark: mean=$mean"}; if($darkRatio -gt .35){throw "Unity preview black ratio too high: $darkRatio"}; if($brightRatio -gt .05){throw "Unity preview overexposed ratio too high: $brightRatio"}; } finally { $bitmap.Dispose() }
$cameraReport = Join-Path $out 'preview-camera.json'
# An unchanged serialized Prefab may retain its timestamp. Require a fresh successful
# Editor build receipt bound to its bytes AND the current texture/source manifest.
$buildReceipt = Join-Path $out 'pbr-build-receipt.json'
foreach ($artifact in @($buildReceipt,$results,$audit,$unityPreview,$cameraReport,(Join-Path $out 'map44-baked-entry.png'),(Join-Path $out 'map44-baked-side.png'),(Join-Path $out 'map44-baked-dummy-close.png'))) {
    if (-not (Test-Path -LiteralPath $artifact -PathType Leaf)) { throw "Missing artifact: $artifact" }
    if ((Get-Item -LiteralPath $artifact).LastWriteTimeUtc -lt $startedUtc) { throw "Stale artifact: $artifact" }
}
$receipt = Get-Content -LiteralPath $buildReceipt -Raw | ConvertFrom-Json
if ([DateTime]::Parse($receipt.completedUtc).ToUniversalTime() -lt $startedUtc) { throw 'Stale build receipt timestamp.' }
if ($receipt.prefabPath -ne 'unity/MoxiangClient/Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab' -or $receipt.textureCount -ne 3) { throw 'Invalid PBR build receipt.' }
$manifest = Join-Path $project 'Assets\Moxiang\Art\WuguanTrainingHall\pbr-manifest.json'
$material = Join-Path $project 'Assets\Moxiang\Art\WuguanTrainingHall\Materials\Wuguan_PBR_Atlas.mat'
if ($receipt.prefabSha256 -ne (Get-FileHash -LiteralPath $prefab).Hash -or $receipt.manifestSha256 -ne (Get-FileHash -LiteralPath $manifest).Hash -or $receipt.materialSha256 -ne (Get-FileHash -LiteralPath $material).Hash) { throw 'Build receipt no longer matches current PBR artifacts.' }
$cameraEvidence = Get-Content -LiteralPath $cameraReport -Raw | ConvertFrom-Json
if (-not $cameraEvidence.cameraAboveFloor -or -not $cameraEvidence.combatInFrame) { throw 'Camera geometry acceptance failed.' }
$hash = (Get-FileHash -LiteralPath $prefab -Algorithm SHA256).Hash
Write-Output "WUGUAN_VERIFY_OK tests=$($run.passed)/$($run.total) prefab_sha256=$hash unity_mean=$([math]::Round($mean,4)) dark_ratio=$([math]::Round($darkRatio,4))"


