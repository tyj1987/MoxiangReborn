# 用临时目录里的假 pak 锁住原版第二层：只复制直接子级 *.pak、不写源目录、
# PlayDH pak 的 SHA-256 保持不变、回滚只删除 Original。
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$deployClient = [System.IO.Path]::GetFullPath((Join-Path $here '..\deploy_client.ps1'))
$layer = [System.IO.Path]::GetFullPath((Join-Path $here '..\original_pak_layer.ps1'))
. $layer

$failures = New-Object System.Collections.Generic.List[string]
function Add-Failure {
    param([string]$Message)
    $script:failures.Add($Message) | Out-Null
    Write-Host "FAIL: $Message" -ForegroundColor Red
}

function New-TempRoot {
    $path = Join-Path ([System.IO.Path]::GetTempPath()) ('mxh-original-pak-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $path | Out-Null
    return $path
}

function New-FileBytes {
    param([string]$Path, [byte[]]$Bytes)
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent | Out-Null
    }
    [System.IO.File]::WriteAllBytes($Path, $Bytes)
}

function Get-HexSha {
    param([string]$Path)
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Assert-SameFile {
    param([string]$Left, [string]$Right, [string]$Label)
    if ((Get-HexSha $Left) -ne (Get-HexSha $Right)) {
        Add-Failure "$Label SHA 不一致"
    }
    $a = [int64](Get-Item -LiteralPath $Left).Length
    $b = [int64](Get-Item -LiteralPath $Right).Length
    if ($a -ne $b) { Add-Failure "$Label 大小不一致" }
}

$clientText = [System.IO.File]::ReadAllText($deployClient)
$layerText = [System.IO.File]::ReadAllText($layer)
if ($clientText -notmatch '\$kOriginalPakSubdir = ''Original''') {
    Add-Failure 'deploy_client.ps1 缺少 $kOriginalPakSubdir = ''Original'''
}
if ($clientText -notmatch '\$kClientResourceRootRelative = ''data\\PlayDH''') {
    Add-Failure 'deploy_client.ps1 缺少 data\PlayDH 资源根常量'
}
$silent = [regex]::Matches($clientText, 'SilentlyContinue')
if ($silent.Count -ne 1) { Add-Failure "deploy_client.ps1 的 SilentlyContinue 应只保留原来 1 处，实际 $($silent.Count)" }
if ($layerText -match 'SilentlyContinue') { Add-Failure 'original_pak_layer.ps1 不应使用 SilentlyContinue' }
if ($clientText -notmatch [regex]::Escape('Get-ChildItem -LiteralPath $ResourceDir -Force | Copy-Item -Destination $profileDataRoot -Recurse -Force')) {
    Add-Failure 'profile 整树复制行被改动'
}
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($deployClient, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count -ne 0) { Add-Failure 'deploy_client.ps1 无法解析' }
$layerErrors = $null
$layerTokens = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($layer, [ref]$layerTokens, [ref]$layerErrors)
if ($layerErrors.Count -ne 0) { Add-Failure 'original_pak_layer.ps1 无法解析' }
$paramNames = @($ast.ParamBlock.Parameters | ForEach-Object { $_.Name.VariablePath.UserPath })
foreach ($name in @('OriginalPakSource', 'RemoveOriginalPaks', 'DryRun')) {
    if ($paramNames -notcontains $name) { Add-Failure "缺少参数 $name" }
}

$root = New-TempRoot
try {
    $source = Join-Path $root 'source'
    $nested = Join-Path $source 'nested'
    $deploy = Join-Path $root 'deploy'
    $playDh = Join-Path (Join-Path $deploy 'data') 'PlayDH'
    New-Item -ItemType Directory -Path $nested | Out-Null
    New-Item -ItemType Directory -Path $playDh | Out-Null
    New-FileBytes -Path (Join-Path $source 'Map.pak') -Bytes ([byte[]](1, 2, 3, 4, 5))
    New-FileBytes -Path (Join-Path $source 'Effect.pak') -Bytes ([byte[]](9, 8, 7))
    New-FileBytes -Path (Join-Path $source 'readme.txt') -Bytes ([byte[]](1))
    New-FileBytes -Path (Join-Path $source 'only-original.map') -Bytes ([byte[]](2, 2))
    New-FileBytes -Path (Join-Path $nested 'npc.pak') -Bytes ([byte[]](3, 3, 3))
    New-FileBytes -Path (Join-Path $playDh 'Map.pak') -Bytes ([byte[]](80, 81, 82, 83))
    New-FileBytes -Path (Join-Path $playDh 'monster.pak') -Bytes ([byte[]](7, 7, 7, 7, 7))
    New-FileBytes -Path (Join-Path $playDh 'keep.txt') -Bytes ([byte[]](4))
    $original = Join-Path $playDh 'Original'
    New-Item -ItemType Directory -Path $original | Out-Null
    New-FileBytes -Path (Join-Path $original 'stale.pak') -Bytes ([byte[]](6, 6))

    $playMap = Get-HexSha (Join-Path $playDh 'Map.pak')
    $playMonster = Get-HexSha (Join-Path $playDh 'monster.pak')
    $sourceMap = Get-HexSha (Join-Path $source 'Map.pak')
    $sourceStamp = Get-SourceDirectoryStamp -Directory $source

    Invoke-OriginalPakLayerInstall -DeployDir $deploy -SourceDir $source -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' -DryRun | Out-Null
    if (-not (Test-Path -LiteralPath (Join-Path $original 'stale.pak'))) { Add-Failure 'DryRun 不应删除已有 pak' }
    if (Test-Path -LiteralPath (Join-Path $original 'Map.pak')) { Add-Failure 'DryRun 不应复制 pak' }
    if (Test-Path -LiteralPath (Join-Path $original 'manifest.json')) { Add-Failure 'DryRun 不应写 manifest' }
    if ((Get-SourceDirectoryStamp -Directory $source) -cne $sourceStamp) { Add-Failure 'DryRun 改变了源目录' }

    Invoke-OriginalPakLayerInstall -DeployDir $deploy -SourceDir $source -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    $copiedMap = Join-Path $original 'Map.pak'
    $copiedEffect = Join-Path $original 'Effect.pak'
    if (-not (Test-Path -LiteralPath $copiedMap)) { Add-Failure '未复制 Map.pak' }
    if (-not (Test-Path -LiteralPath $copiedEffect)) { Add-Failure '未复制 Effect.pak' }
    if (Test-Path -LiteralPath (Join-Path $original 'stale.pak')) { Add-Failure '源中已不存在的 Original pak 应移除' }
    if (Test-Path -LiteralPath (Join-Path $original 'readme.txt')) { Add-Failure '不应复制 txt' }
    if (Test-Path -LiteralPath (Join-Path $original 'only-original.map')) { Add-Failure '不应复制 .map' }
    if (Test-Path -LiteralPath (Join-Path $original 'npc.pak')) { Add-Failure '不应递归复制子目录 pak' }
    Assert-SameFile -Left (Join-Path $source 'Map.pak') -Right $copiedMap -Label 'Map.pak'
    Assert-SameFile -Left (Join-Path $source 'Effect.pak') -Right $copiedEffect -Label 'Effect.pak'
    if ((Get-HexSha (Join-Path $playDh 'Map.pak')) -cne $playMap) { Add-Failure 'PlayDH Map.pak SHA 变化' }
    if ((Get-HexSha (Join-Path $playDh 'monster.pak')) -cne $playMonster) { Add-Failure 'PlayDH monster.pak SHA 变化' }
    $keep = [System.IO.File]::ReadAllBytes((Join-Path $playDh 'keep.txt'))
    if ($keep.Length -ne 1 -or $keep[0] -ne 4) { Add-Failure 'PlayDH keep.txt 被修改' }
    if ((Get-SourceDirectoryStamp -Directory $source) -cne $sourceStamp) { Add-Failure '复制后源目录变化' }
    if ((Get-HexSha $copiedMap) -eq $playMap) { Add-Failure '第二层 Map.pak 不应等于 PlayDH Map.pak' }
    $partial = @(Get-ChildItem -LiteralPath $original -Force -Filter '*.partial')
    if ($partial.Count -ne 0) { Add-Failure '残留 partial 文件' }

    $manifestPath = Join-Path $original 'manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $top = @($manifest.PSObject.Properties.Name)
    foreach ($extra in @($top | Where-Object { $_ -notin @('subdirectory', 'files') })) {
        Add-Failure "manifest 含多余字段 $extra"
    }
    if ($manifest.subdirectory -cne 'Original') { Add-Failure 'manifest subdirectory 不是 Original' }
    if (@($manifest.files).Count -ne 2) { Add-Failure "manifest 文件数 $($manifest.files.Count)" }
    foreach ($entry in @($manifest.files)) {
        $fields = @($entry.PSObject.Properties.Name)
        foreach ($extra in @($fields | Where-Object { $_ -notin @('fileName', 'bytes', 'sha256', 'sourcePath') })) {
            Add-Failure "manifest 条目含多余字段 $extra"
        }
        if ($entry.sha256 -notmatch '^[0-9a-f]{64}$') { Add-Failure "$($entry.fileName) sha256 格式不对" }
        $dest = Join-Path $original $entry.fileName
        if ([int64]$entry.bytes -ne [int64](Get-Item -LiteralPath $dest).Length) { Add-Failure "$($entry.fileName) 字节数不对" }
        if ($entry.sha256 -cne (Get-HexSha $dest)) { Add-Failure "$($entry.fileName) sha 与目标不一致" }
        if ($entry.sourcePath -ne (Join-Path $source $entry.fileName)) { Add-Failure "$($entry.fileName) 源路径不对" }
        if ($entry.sourcePath -like '*PlayDH*') { Add-Failure "$($entry.fileName) 源路径指向了部署树" }
    }
    $rawManifest = [System.IO.File]::ReadAllText($manifestPath)
    foreach ($secret in @('password', 'Server=', 'Initial Catalog', 'User ID', 'connectionString')) {
        if ($rawManifest -like ('*' + $secret + '*')) { Add-Failure "manifest 含敏感片段 $secret" }
    }

    $badDest = Join-Path $original 'Map.pak'
    $badCopy = Join-Path $root 'bad-map.pak'
    [System.IO.File]::Copy($badDest, $badCopy, $true)
    $badBytes = [System.IO.File]::ReadAllBytes($badCopy)
    $badBytes[0] = 255
    [System.IO.File]::WriteAllBytes($badCopy, $badBytes)
    $mismatch = $false
    try {
        Assert-CopiedFile -SourcePath (Join-Path $source 'Map.pak') -DestinationPath $badCopy -ExpectedSha256 $sourceMap -ExpectedBytes 5
    } catch {
        $mismatch = $_.Exception.Message -like '*SHA-256*'
    }
    if (-not $mismatch) { Add-Failure '大小或 SHA-256 不一致时应失败' }

    $empty = Join-Path $root 'empty-source'
    New-Item -ItemType Directory -Path $empty | Out-Null
    $emptyFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $deploy -SourceDir $empty -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $emptyFailed = $_.Exception.Message -like '*没有 pak*'
    }
    if (-not $emptyFailed) { Add-Failure '没有 pak 的源目录应失败' }

    $missingFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $deploy -SourceDir (Join-Path $root 'missing') -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $missingFailed = $_.Exception.Message -like '*不存在*'
    }
    if (-not $missingFailed) { Add-Failure '源目录不存在应失败' }

    $caseFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $deploy -SourceDir $source -ResourceRootRelative 'data\PlayDH' -SubdirName 'original' | Out-Null
    } catch {
        $caseFailed = $_.Exception.Message -like '*Original*'
    }
    if (-not $caseFailed) { Add-Failure '子目录名大小写不一致应失败' }

    $overlap = Join-Path $root 'overlap-source'
    $overlapDeploy = Join-Path $overlap 'deploy'
    New-Item -ItemType Directory -Path (Join-Path (Join-Path $overlapDeploy 'data') 'PlayDH') | Out-Null
    New-FileBytes -Path (Join-Path $overlap 'Map.pak') -Bytes ([byte[]](1, 1))
    $beforeOverlap = @(Get-ChildItem -LiteralPath $overlap -Recurse -Force | ForEach-Object { $_.FullName } | Sort-Object)
    $overlapFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $overlapDeploy -SourceDir $overlap -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $overlapFailed = $_.Exception.Message -like '*拒绝写入源目录*'
    }
    $afterOverlap = @(Get-ChildItem -LiteralPath $overlap -Recurse -Force | ForEach-Object { $_.FullName } | Sort-Object)
    if (-not $overlapFailed) { Add-Failure '源目录与部署树重叠应失败' }
    if (($beforeOverlap -join '|') -ne ($afterOverlap -join '|')) { Add-Failure '重叠失败时仍写入了源目录' }

    $dup = Join-Path $root 'dup-source'
    New-Item -ItemType Directory -Path $dup | Out-Null
    New-FileBytes -Path (Join-Path $dup 'Map.pak') -Bytes ([byte[]](1))
    New-FileBytes -Path (Join-Path $dup 'map.pak') -Bytes ([byte[]](2))
    $dupDeploy = Join-Path $root 'dup-deploy'
    $dupPlay = Join-Path (Join-Path $dupDeploy 'data') 'PlayDH'
    New-Item -ItemType Directory -Path $dupPlay | Out-Null
    New-FileBytes -Path (Join-Path $dupPlay 'monster.pak') -Bytes ([byte[]](3))
    $dupFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $dupDeploy -SourceDir $dup -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $dupFailed = $_.Exception.Message -like '*重复*'
    }
    if (-not $dupFailed) { Add-Failure '仅大小写不同的 pak 名应失败' }
    if (Test-Path -LiteralPath (Join-Path $dupPlay 'Original')) { Add-Failure '重名失败后不应创建 Original' }

    $ro = Join-Path $root 'ro-source'
    $roDeploy = Join-Path $root 'ro-deploy'
    $roPlay = Join-Path (Join-Path $roDeploy 'data') 'PlayDH'
    New-Item -ItemType Directory -Path $ro | Out-Null
    New-Item -ItemType Directory -Path $roPlay | Out-Null
    New-FileBytes -Path (Join-Path $ro 'Map.pak') -Bytes ([byte[]](4, 5, 6))
    New-FileBytes -Path (Join-Path $roPlay 'Map.pak') -Bytes ([byte[]](1))
    $roStamp = Get-SourceDirectoryStamp -Directory $ro
    & chmod a-w $ro
    if ($LASTEXITCODE -ne 0) { Add-Failure 'chmod 源目录失败' }
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $roDeploy -SourceDir $ro -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
        if ((Get-SourceDirectoryStamp -Directory $ro) -cne $roStamp) { Add-Failure '只读源目录被修改' }
        if (-not (Test-Path -LiteralPath (Join-Path (Join-Path $roPlay 'Original') 'Map.pak'))) {
            Add-Failure '只读源目录未能复制 pak'
        }
    } finally {
        & chmod u+w $ro
    }

    $linkSource = Join-Path $root 'link-target'
    $linkDeploy = Join-Path $root 'link-deploy'
    $linkPlay = Join-Path (Join-Path $linkDeploy 'data') 'PlayDH'
    New-Item -ItemType Directory -Path $linkSource | Out-Null
    New-Item -ItemType Directory -Path $linkPlay | Out-Null
    New-FileBytes -Path (Join-Path $linkSource 'Map.pak') -Bytes ([byte[]](9))
    New-FileBytes -Path (Join-Path $linkPlay 'monster.pak') -Bytes ([byte[]](8))
    $link = Join-Path $linkPlay 'Original'
    & ln -s $linkSource $link
    if ($LASTEXITCODE -ne 0) { Add-Failure '创建 Original 符号链接失败' }
    $linkFailed = $false
    try {
        Invoke-OriginalPakLayerInstall -DeployDir $linkDeploy -SourceDir $source -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $linkFailed = $_.Exception.Message -like '*重解析点*'
    }
    if (-not $linkFailed) { Add-Failure 'Original 为符号链接时应拒绝复制' }
    $targetBytes = [System.IO.File]::ReadAllBytes((Join-Path $linkSource 'Map.pak'))
    if ($targetBytes.Length -ne 1 -or $targetBytes[0] -ne 9) { Add-Failure '符号链接目标被写入' }

    $rollbackSourceSentinel = Join-Path $root 'do-not-touch-source'
    New-Item -ItemType Directory -Path $rollbackSourceSentinel | Out-Null
    New-FileBytes -Path (Join-Path $rollbackSourceSentinel 'Map.pak') -Bytes ([byte[]](11, 12))
    $sentinelSha = Get-HexSha (Join-Path $rollbackSourceSentinel 'Map.pak')
    Invoke-OriginalPakLayerRollback -DeployDir $deploy -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' -DryRun | Out-Null
    if (-not (Test-Path -LiteralPath $original)) { Add-Failure '回滚 DryRun 不应删除 Original' }
    Invoke-OriginalPakLayerRollback -DeployDir $deploy -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    if (Test-Path -LiteralPath $original) { Add-Failure '回滚后 Original 仍在' }
    if ((Get-HexSha (Join-Path $playDh 'Map.pak')) -cne $playMap) { Add-Failure '回滚后 PlayDH Map.pak SHA 变化' }
    if ((Get-HexSha (Join-Path $playDh 'monster.pak')) -cne $playMonster) { Add-Failure '回滚后 PlayDH monster.pak SHA 变化' }
    if ((Get-HexSha (Join-Path $rollbackSourceSentinel 'Map.pak')) -cne $sentinelSha) { Add-Failure '回滚触及了源目录' }
    $missingRollback = $false
    try {
        Invoke-OriginalPakLayerRollback -DeployDir $deploy -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $missingRollback = $_.Exception.Message -like '*不存在*'
    }
    if (-not $missingRollback) { Add-Failure 'Original 不存在时回滚应失败' }
    if ((Get-HexSha (Join-Path $playDh 'Map.pak')) -cne $playMap) { Add-Failure '失败的回滚改了 PlayDH' }

    $unsafe = Join-Path $root 'unsafe'
    $unsafePlay = Join-Path (Join-Path $unsafe 'data') 'PlayDH'
    $unsafeOriginal = Join-Path $unsafePlay 'Original'
    New-Item -ItemType Directory -Path $unsafeOriginal | Out-Null
    New-FileBytes -Path (Join-Path $unsafeOriginal 'Map.pak') -Bytes ([byte[]](1))
    $noPlayDhPak = $false
    try {
        Invoke-OriginalPakLayerRollback -DeployDir $unsafe -ResourceRootRelative 'data\PlayDH' -SubdirName 'Original' | Out-Null
    } catch {
        $noPlayDhPak = $_.Exception.Message -like '*没有 pak*'
    }
    if (-not $noPlayDhPak) { Add-Failure '没有 PlayDH pak 时不应删除 Original' }
    if (-not (Test-Path -LiteralPath $unsafeOriginal)) { Add-Failure '没有 PlayDH pak 时 Original 被删了' }
} finally {
    if (Test-Path -LiteralPath $root) {
        Get-ChildItem -LiteralPath $root -Recurse -Force -Directory -ErrorAction SilentlyContinue | ForEach-Object { & chmod u+w $_.FullName }
        & chmod -R u+w $root
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}

$exe = [System.Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
function Invoke-DeployClient {
    param([string[]]$Arguments)
    $output = & $exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $deployClient @Arguments 2>&1 | Out-String
    return [pscustomobject]@{ Code = $LASTEXITCODE; Output = $output }
}

$help = Invoke-DeployClient -Arguments @('-Help')
if ($help.Code -ne 0) { Add-Failure "-Help 退出码 $($help.Code)" }
if ($help.Output -notlike '*-OriginalPakSource*') { Add-Failure '-Help 未说明 -OriginalPakSource' }
if ($help.Output -notlike '*-RemoveOriginalPaks*') { Add-Failure '-Help 未说明 -RemoveOriginalPaks' }

$wire = New-TempRoot
try {
    $wireDeploy = Join-Path $wire 'client'
    $wirePlay = Join-Path (Join-Path $wireDeploy 'data') 'PlayDH'
    $wireOriginal = Join-Path $wirePlay 'Original'
    $wireSource = Join-Path $wire 'source'
    New-Item -ItemType Directory -Path $wireOriginal | Out-Null
    New-Item -ItemType Directory -Path $wireSource | Out-Null
    New-FileBytes -Path (Join-Path $wirePlay 'monster.pak') -Bytes ([byte[]](21, 22, 23))
    New-FileBytes -Path (Join-Path $wireOriginal 'Map.pak') -Bytes ([byte[]](31))
    New-FileBytes -Path (Join-Path $wireSource 'Map.pak') -Bytes ([byte[]](41, 42))
    $wireMonster = Get-HexSha (Join-Path $wirePlay 'monster.pak')
    $wireSourceSha = Get-HexSha (Join-Path $wireSource 'Map.pak')
    $both = Invoke-DeployClient -Arguments @('-OriginalPakSource', $wireSource, '-RemoveOriginalPaks', '-DeployDir', $wireDeploy)
    if ($both.Code -eq 0) { Add-Failure '同时指定两个新参数应失败' }
    if (-not (Test-Path -LiteralPath $wireOriginal)) { Add-Failure '参数冲突时不应删除 Original' }
    if ((Get-HexSha (Join-Path $wireSource 'Map.pak')) -cne $wireSourceSha) { Add-Failure '参数冲突时改了源目录' }

    $plan = Invoke-DeployClient -Arguments @('-RemoveOriginalPaks', '-DryRun', '-DeployDir', $wireDeploy)
    if ($plan.Code -ne 0) { Add-Failure "回滚 DryRun 退出码 $($plan.Code) $($plan.Output)" }
    if (-not (Test-Path -LiteralPath (Join-Path $wireOriginal 'Map.pak'))) { Add-Failure '脚本 DryRun 删除了 Original' }
    $gone = Invoke-DeployClient -Arguments @('-RemoveOriginalPaks', '-DeployDir', $wireDeploy)
    if ($gone.Code -ne 0) { Add-Failure "回滚退出码 $($gone.Code) $($gone.Output)" }
    if (Test-Path -LiteralPath $wireOriginal) { Add-Failure '脚本回滚后 Original 仍在' }
    if ((Get-HexSha (Join-Path $wirePlay 'monster.pak')) -cne $wireMonster) { Add-Failure '脚本回滚后 PlayDH pak SHA 变化' }
    if ((Get-HexSha (Join-Path $wireSource 'Map.pak')) -cne $wireSourceSha) { Add-Failure '脚本回滚后源目录变化' }
} finally {
    if (Test-Path -LiteralPath $wire) {
        Remove-Item -LiteralPath $wire -Recurse -Force
    }
}

if ($failures.Count -gt 0) {
    Write-Host ("{0} failed" -f $failures.Count) -ForegroundColor Red
    exit 1
}
Write-Host 'original pak layer tests passed'
exit 0
