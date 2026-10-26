# 原版 pak 第二层（TYJ-120）。由 deploy_client.ps1 点源加载。
# 加载本文件不得产生副作用。复制只读打开 -OriginalPakSource，禁止写入该目录。
# 子目录名必须是 Original，与 storage 常量 kOriginalPakSubdir 同名同大小写。

function ConvertTo-JsonString {
    param([AllowNull()][string]$Value)
    if ($null -eq $Value) { $Value = '' }
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    foreach ($ch in $Value.ToCharArray()) {
        $code = [int]$ch
        if ($ch -eq '"') { [void]$sb.Append('\"') }
        elseif ($ch -eq '\') { [void]$sb.Append('\\') }
        elseif ($ch -eq "`b") { [void]$sb.Append('\b') }
        elseif ($ch -eq "`f") { [void]$sb.Append('\f') }
        elseif ($ch -eq "`n") { [void]$sb.Append('\n') }
        elseif ($ch -eq "`r") { [void]$sb.Append('\r') }
        elseif ($ch -eq "`t") { [void]$sb.Append('\t') }
        elseif ($code -lt 32) { [void]$sb.AppendFormat('\u{0:x4}', $code) }
        else { [void]$sb.Append($ch) }
    }
    [void]$sb.Append('"')
    return $sb.ToString()
}

function Join-DeployPath {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$Relative
    )
    $current = $Root
    foreach ($segment in @($Relative -split '[\\/]+')) {
        if ([string]::IsNullOrWhiteSpace($segment)) { continue }
        if ($segment -eq '.' -or $segment -eq '..') {
            throw "路径含有非法段: $segment"
        }
        $current = Join-Path -Path $current -ChildPath $segment
    }
    return $current
}

function Get-ComparablePath {
    param([Parameter(Mandatory)][string]$Path)
    $full = [System.IO.Path]::GetFullPath($Path)
    return $full.Replace('/', '\').TrimEnd('\').ToLowerInvariant()
}

function Test-SameOrNestedPath {
    param(
        [Parameter(Mandatory)][string]$Left,
        [Parameter(Mandatory)][string]$Right
    )
    $a = Get-ComparablePath $Left
    $b = Get-ComparablePath $Right
    if ($a -eq $b) { return $true }
    if ($b.StartsWith($a + '\')) { return $true }
    if ($a.StartsWith($b + '\')) { return $true }
    return $false
}

function Test-ReparsePoint {
    param([Parameter(Mandatory)][string]$LiteralPath)
    if (-not (Test-Path -LiteralPath $LiteralPath)) { return $false }
    $attrs = [System.IO.File]::GetAttributes($LiteralPath)
    return (($attrs -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)
}

function Get-Sha256Hex {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $LiteralPath).Hash
    if ([string]::IsNullOrWhiteSpace($hash)) { throw "无法计算 SHA-256: $LiteralPath" }
    return $hash.ToLowerInvariant()
}

function Get-TopLevelPakFiles {
    param([Parameter(Mandatory)][string]$Directory)
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        throw "目录不存在: $Directory"
    }
    $files = @(Get-ChildItem -LiteralPath $Directory -Force -File | Where-Object {
        [string]::Equals($_.Extension, '.pak', [System.StringComparison]::OrdinalIgnoreCase)
    })
    return $files
}

function Get-PakFingerprint {
    param(
        [Parameter(Mandatory)][string]$Directory,
        [Parameter(Mandatory)][string]$Role,
        [switch]$AllowEmpty
    )
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        throw "$Role 目录不存在: $Directory"
    }
    if (Test-ReparsePoint -LiteralPath $Directory) {
        throw "$Role 目录是重解析点，拒绝继续: $Directory"
    }
    $files = @(Get-TopLevelPakFiles -Directory $Directory)
    if ((-not $AllowEmpty) -and $files.Count -eq 0) {
        throw "$Role 目录下没有 pak: $Directory"
    }
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($file in $files) {
        if ($file.Name -match '[\\/]' -or $file.Name -eq '.' -or $file.Name -eq '..') {
            throw "$Role 含有非法 pak 文件名: $($file.Name)"
        }
        $sha = Get-Sha256Hex -LiteralPath $file.FullName
        $lines.Add(('{0}|{1}|{2}' -f $file.Name, [int64]$file.Length, $sha))
    }
    $arr = $lines.ToArray()
    [Array]::Sort($arr, [System.StringComparer]::Ordinal)
    return ($arr -join "`n")
}

function Assert-CopiedFile {
    param(
        [Parameter(Mandatory)][string]$SourcePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [Parameter(Mandatory)][string]$ExpectedSha256,
        [Parameter(Mandatory)][int64]$ExpectedBytes
    )
    if (-not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)) {
        throw "原版 pak 复制失败，目标不存在: $DestinationPath"
    }
    if (Test-ReparsePoint -LiteralPath $DestinationPath) {
        throw "原版 pak 目标是重解析点，拒绝承认复制结果: $DestinationPath"
    }
    $destLength = [int64](Get-Item -LiteralPath $DestinationPath -Force).Length
    if ($destLength -ne $ExpectedBytes) {
        throw "原版 pak 复制后大小不一致: $DestinationPath"
    }
    $destHash = Get-Sha256Hex -LiteralPath $DestinationPath
    if ($destHash -cne $ExpectedSha256) {
        throw "原版 pak 复制后 SHA-256 不一致: $DestinationPath"
    }
    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) {
        throw "原版 pak 源文件丢失: $SourcePath"
    }
    $sourceLength = [int64](Get-Item -LiteralPath $SourcePath -Force).Length
    if ($sourceLength -ne $ExpectedBytes) {
        throw "原版 pak 源大小在复制后变化: $SourcePath"
    }
    $sourceHash = Get-Sha256Hex -LiteralPath $SourcePath
    if ($sourceHash -cne $ExpectedSha256) {
        throw "原版 pak 源 SHA-256 在复制后变化: $SourcePath"
    }
}

function Copy-ReadOnlyFile {
    param(
        [Parameter(Mandatory)][string]$SourcePath,
        [Parameter(Mandatory)][string]$DestinationPath
    )
    $inStream = $null
    $outStream = $null
    try {
        $inStream = New-Object System.IO.FileStream(
            $SourcePath,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::Read,
            [System.IO.FileShare]::Read)
        $outStream = New-Object System.IO.FileStream(
            $DestinationPath,
            [System.IO.FileMode]::CreateNew,
            [System.IO.FileAccess]::Write,
            [System.IO.FileShare]::None)
        $inStream.CopyTo($outStream)
        $outStream.Flush()
    } catch {
        throw "原版 pak 复制失败: $SourcePath -> $DestinationPath ($($_.Exception.Message))"
    } finally {
        if ($null -ne $outStream) { $outStream.Dispose() }
        if ($null -ne $inStream) { $inStream.Dispose() }
    }
}

function Get-SourceDirectoryStamp {
    param([Parameter(Mandatory)][string]$Directory)
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($entry in @(Get-ChildItem -LiteralPath $Directory -Force)) {
        $kind = if ($entry.PSIsContainer) { 'dir' } else { 'file' }
        $length = if ($entry.PSIsContainer) { [int64]-1 } else { [int64]$entry.Length }
        $sha = '-'
        if (-not $entry.PSIsContainer) {
            $sha = Get-Sha256Hex -LiteralPath $entry.FullName
        }
        $lines.Add(('{0}|{1}|{2}|{3}' -f $entry.Name, $kind, $length, $sha))
    }
    $arr = $lines.ToArray()
    [Array]::Sort($arr, [System.StringComparer]::Ordinal)
    return ($arr -join "`n")
}

function Assert-LayerContract {
    param(
        [Parameter(Mandatory)][string]$ResourceRootRelative,
        [Parameter(Mandatory)][string]$SubdirName
    )
    if ($SubdirName -cne 'Original') {
        throw "子目录名必须与 kOriginalPakSubdir 一致（Original），实际: $SubdirName"
    }
    if ($ResourceRootRelative -cne 'data\PlayDH') {
        throw "客户端资源根必须是 data\PlayDH，实际: $ResourceRootRelative"
    }
}

function Resolve-SourceDirectory {
    param([Parameter(Mandatory)][string]$SourceDir)
    if (-not (Test-Path -LiteralPath $SourceDir)) {
        throw "原版 pak 源目录不存在: $SourceDir"
    }
    $item = Get-Item -LiteralPath $SourceDir -Force
    if (-not $item.PSIsContainer) {
        throw "原版 pak 源不是目录: $SourceDir"
    }
    $guard = 0
    while ((Test-ReparsePoint -LiteralPath $item.FullName) -and $guard -lt 8) {
        $guard++
        $target = $null
        if ($item.PSObject.Properties.Name -contains 'Target') {
            $raw = $item.Target
            if ($raw -is [System.Array]) { $raw = $raw | Select-Object -First 1 }
            $target = [string]$raw
        }
        if ([string]::IsNullOrWhiteSpace($target)) { break }
        if (-not [System.IO.Path]::IsPathRooted($target)) {
            $target = [System.IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $item.FullName) $target))
        }
        if (-not (Test-Path -LiteralPath $target -PathType Container)) { break }
        $item = Get-Item -LiteralPath $target -Force
    }
    return $item.FullName
}

function New-OriginalPakManifestJson {
    param(
        [Parameter(Mandatory)][string]$SubdirName,
        [Parameter(Mandatory)]$Entries
    )
    $ordered = @($Entries | Sort-Object { $_.FileName }, { $_.FileName })
    $names = New-Object System.Collections.Generic.List[string]
    foreach ($entry in $ordered) { $names.Add([string]$entry.FileName) }
    $nameArr = $names.ToArray()
    [Array]::Sort($nameArr, [System.StringComparer]::Ordinal)
    $byName = @{}
    foreach ($entry in $ordered) { $byName[[string]$entry.FileName] = $entry }
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine('{')
    [void]$sb.AppendLine(('  "subdirectory": {0},' -f (ConvertTo-JsonString $SubdirName)))
    [void]$sb.AppendLine('  "files": [')
    for ($i = 0; $i -lt $nameArr.Length; $i++) {
        $entry = $byName[$nameArr[$i]]
        if ($entry.Sha256 -notmatch '^[0-9a-f]{64}$') {
            throw "manifest SHA-256 无效: $($entry.FileName)"
        }
        $comma = ''
        if ($i -lt ($nameArr.Length - 1)) { $comma = ',' }
        [void]$sb.AppendLine('    {')
        [void]$sb.AppendLine(('      "fileName": {0},' -f (ConvertTo-JsonString ([string]$entry.FileName))))
        [void]$sb.AppendLine(('      "bytes": {0},' -f [int64]$entry.Bytes))
        [void]$sb.AppendLine(('      "sha256": {0},' -f (ConvertTo-JsonString ([string]$entry.Sha256))))
        [void]$sb.AppendLine(('      "sourcePath": {0}' -f (ConvertTo-JsonString ([string]$entry.SourcePath))))
        [void]$sb.AppendLine(('    }}{0}' -f $comma))
    }
    [void]$sb.AppendLine('  ]')
    [void]$sb.AppendLine('}')
    return $sb.ToString()
}

function Invoke-OriginalPakLayerInstall {
    param(
        [Parameter(Mandatory)][string]$DeployDir,
        [Parameter(Mandatory)][string]$SourceDir,
        [Parameter(Mandatory)][string]$ResourceRootRelative,
        [Parameter(Mandatory)][string]$SubdirName,
        [switch]$DryRun
    )
    $ErrorActionPreference = 'Stop'
    Assert-LayerContract -ResourceRootRelative $ResourceRootRelative -SubdirName $SubdirName
    if ([string]::IsNullOrWhiteSpace($DeployDir)) { throw 'DeployDir 为空' }

    $sourceFull = Resolve-SourceDirectory -SourceDir $SourceDir
    $resourceRoot = Join-DeployPath -Root $DeployDir -Relative $ResourceRootRelative
    $originalDir = Join-DeployPath -Root $resourceRoot -Relative $SubdirName
    if (Test-SameOrNestedPath -Left $sourceFull -Right $originalDir) {
        throw "源目录与 Original 重叠，拒绝写入源目录: $sourceFull"
    }
    if (Test-SameOrNestedPath -Left $sourceFull -Right $resourceRoot) {
        throw "源目录与 PlayDH 资源根重叠，拒绝写入源目录: $sourceFull"
    }

    $sourceFiles = @(Get-TopLevelPakFiles -Directory $sourceFull)
    if ($sourceFiles.Count -eq 0) {
        throw "原版 pak 源目录下没有 pak: $sourceFull"
    }
    $byName = New-Object 'System.Collections.Generic.Dictionary[string,object]' ([System.StringComparer]::OrdinalIgnoreCase)
    $entries = New-Object System.Collections.Generic.List[object]
    foreach ($file in $sourceFiles) {
        if ($file.Name -match '[\\/]' -or [System.IO.Path]::GetFileName($file.Name) -ne $file.Name) {
            throw "非法 pak 文件名: $($file.Name)"
        }
        if ($byName.ContainsKey($file.Name)) {
            throw "源目录存在重复或仅大小写不同的 pak 名: $($file.Name)"
        }
        $sha = Get-Sha256Hex -LiteralPath $file.FullName
        if ($sha -notmatch '^[0-9a-f]{64}$') { throw "SHA-256 无效: $($file.Name)" }
        $record = [pscustomobject]@{
            FileName = $file.Name
            Bytes = [int64]$file.Length
            Sha256 = $sha
            SourcePath = $file.FullName
            FullName = $file.FullName
        }
        $byName.Add($file.Name, $record) | Out-Null
        $entries.Add($record) | Out-Null
    }

    $sourceStamp = Get-SourceDirectoryStamp -Directory $sourceFull
    $stale = New-Object System.Collections.Generic.List[string]
    if (Test-Path -LiteralPath $originalDir) {
        if (Test-ReparsePoint -LiteralPath $originalDir) {
            throw "Original 目录是重解析点，拒绝写入: $originalDir"
        }
        if (-not (Get-Item -LiteralPath $originalDir -Force).PSIsContainer) {
            throw "Original 路径不是目录: $originalDir"
        }
        foreach ($existing in @(Get-TopLevelPakFiles -Directory $originalDir)) {
            if (Test-ReparsePoint -LiteralPath $existing.FullName) {
                throw "Original 内的 pak 是重解析点，拒绝继续: $($existing.FullName)"
            }
            if (-not $byName.ContainsKey($existing.Name)) {
                $stale.Add($existing.Name) | Out-Null
            }
        }
    }

    Write-Host ("原版 pak 第二层: {0} -> {1}" -f $sourceFull, $originalDir)
    foreach ($entry in $entries) {
        Write-Host ("  {0}  {1} bytes  sha256={2}" -f $entry.FileName, $entry.Bytes, $entry.Sha256)
    }
    if ($stale.Count -gt 0) {
        Write-Host ("  将移除源中已不存在的 Original pak: {0}" -f ($stale -join ', '))
    }

    $guardPlayDh = (Test-Path -LiteralPath $resourceRoot -PathType Container)
    if ($DryRun) {
        if ($guardPlayDh) {
            $preview = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
            $previewCount = @($preview -split "`n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
            Write-Host ("  已记录 PlayDH pak {0} 个的 SHA-256；正式复制后会再核对一次。" -f $previewCount)
        } else {
            Write-Host '  部署目录中的 PlayDH 资源根尚不存在。正式执行会在 profile 复制后记录 SHA-256，再复制原版 pak。'
        }
        Write-Host '  将写入 Original\manifest.json（文件名、字节数、SHA-256、源路径）。'
        Write-Host '未复制原版 pak，也未修改源目录。'
        $afterStamp = Get-SourceDirectoryStamp -Directory $sourceFull
        if ($afterStamp -cne $sourceStamp) { throw "源目录在计划阶段发生变化: $sourceFull" }
        return [pscustomobject]@{ Destination = $originalDir; FileCount = $entries.Count; DryRun = $true }
    }

    $playDhBefore = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
    if (-not (Test-Path -LiteralPath $originalDir)) {
        New-Item -ItemType Directory -Path $originalDir | Out-Null
    }
    if (Test-ReparsePoint -LiteralPath $originalDir) {
        throw "Original 目录是重解析点，拒绝写入: $originalDir"
    }

    try {
        foreach ($entry in $entries) {
            $finalPath = Join-Path $originalDir $entry.FileName
            if (Test-SameOrNestedPath -Left $sourceFull -Right $finalPath) {
                throw "拒绝把 pak 写回源目录: $finalPath"
            }
            if (Test-Path -LiteralPath $finalPath) {
                if (Test-ReparsePoint -LiteralPath $finalPath) {
                    throw "目标是重解析点，拒绝覆盖: $finalPath"
                }
            }
            $tempPath = Join-Path $originalDir ('.{0}.{1}.partial' -f $entry.FileName, [guid]::NewGuid().ToString('N'))
            try {
                Copy-ReadOnlyFile -SourcePath $entry.FullName -DestinationPath $tempPath
                Assert-CopiedFile -SourcePath $entry.FullName -DestinationPath $tempPath -ExpectedSha256 $entry.Sha256 -ExpectedBytes $entry.Bytes
                if (Test-Path -LiteralPath $finalPath) {
                    [System.IO.File]::Delete($finalPath)
                }
                [System.IO.File]::Move($tempPath, $finalPath)
                Assert-CopiedFile -SourcePath $entry.FullName -DestinationPath $finalPath -ExpectedSha256 $entry.Sha256 -ExpectedBytes $entry.Bytes
            } catch {
                if ((Test-Path -LiteralPath $tempPath) -and -not (Test-ReparsePoint -LiteralPath $tempPath)) {
                    [System.IO.File]::Delete($tempPath)
                }
                throw
            }
        }

        foreach ($name in @($stale)) {
            $stalePath = Join-Path $originalDir $name
            if (-not (Test-Path -LiteralPath $stalePath -PathType Leaf)) { continue }
            if (Test-ReparsePoint -LiteralPath $stalePath) {
                throw "拒绝删除重解析点: $stalePath"
            }
            if (Test-SameOrNestedPath -Left $sourceFull -Right $stalePath) {
                throw "拒绝删除源目录中的文件: $stalePath"
            }
            [System.IO.File]::Delete($stalePath)
        }

        $manifestEntries = @()
        foreach ($entry in $entries) {
            $finalPath = Join-Path $originalDir $entry.FileName
            Assert-CopiedFile -SourcePath $entry.FullName -DestinationPath $finalPath -ExpectedSha256 $entry.Sha256 -ExpectedBytes $entry.Bytes
            $manifestEntries += [pscustomobject]@{
                FileName = $entry.FileName
                Bytes = $entry.Bytes
                Sha256 = $entry.Sha256
                SourcePath = $entry.SourcePath
            }
        }
        $manifestPath = Join-Path $originalDir 'manifest.json'
        if (Test-ReparsePoint -LiteralPath $manifestPath) {
            throw "manifest.json 是重解析点，拒绝写入: $manifestPath"
        }
        $json = New-OriginalPakManifestJson -SubdirName $SubdirName -Entries $manifestEntries
        $utf8 = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($manifestPath, $json, $utf8)

        $playDhAfter = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
        if ($playDhAfter -cne $playDhBefore) {
            throw 'PlayDH pak 的 SHA-256 在复制后发生变化'
        }
        $sourceAfter = Get-SourceDirectoryStamp -Directory $sourceFull
        if ($sourceAfter -cne $sourceStamp) {
            throw "源目录被修改，已中止: $sourceFull"
        }
    } catch {
        $originalError = $_
        $playDhError = $null
        try {
            $playDhNow = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
            if ($playDhNow -cne $playDhBefore) {
                $playDhError = 'PlayDH pak 的 SHA-256 在复制后发生变化'
            }
        } catch {
            $playDhError = $_.Exception.Message
        }
        if ($null -ne $playDhError -and $originalError.Exception.Message -notlike '*PlayDH pak 的 SHA-256*') {
            throw ("{0}。原始错误: {1}" -f $playDhError, $originalError.Exception.Message)
        }
        throw $originalError
    }

    Write-Host ("已部署原版 pak 第二层: {0} 个文件 -> {1}" -f $entries.Count, $originalDir)
    return [pscustomobject]@{ Destination = $originalDir; FileCount = $entries.Count; DryRun = $false }
}

function Remove-OriginalLayerDirectory {
    param([Parameter(Mandatory)][string]$LiteralPath)
    if (-not (Test-Path -LiteralPath $LiteralPath)) {
        throw "要删除的 Original 目录不存在: $LiteralPath"
    }
    if (Test-ReparsePoint -LiteralPath $LiteralPath) {
        throw "Original 是重解析点，拒绝删除以免触及链接目标: $LiteralPath"
    }
    $root = Get-Item -LiteralPath $LiteralPath -Force
    if (-not $root.PSIsContainer) {
        throw "Original 路径不是目录: $LiteralPath"
    }
    $files = New-Object System.Collections.Generic.List[string]
    $dirs = New-Object System.Collections.Generic.List[string]
    $pending = New-Object System.Collections.Generic.Queue[string]
    $pending.Enqueue($root.FullName)
    while ($pending.Count -gt 0) {
        $dir = $pending.Dequeue()
        if (Test-ReparsePoint -LiteralPath $dir) {
            throw "拒绝删除重解析点: $dir"
        }
        $dirs.Add($dir)
        foreach ($child in @(Get-ChildItem -LiteralPath $dir -Force)) {
            if (Test-ReparsePoint -LiteralPath $child.FullName) {
                throw "拒绝删除，目录内含重解析点: $($child.FullName)"
            }
            if ($child.PSIsContainer) {
                $pending.Enqueue($child.FullName)
            } else {
                $files.Add($child.FullName)
            }
        }
    }
    foreach ($file in $files) {
        [System.IO.File]::Delete($file)
    }
    for ($i = $dirs.Count - 1; $i -ge 0; $i--) {
        [System.IO.Directory]::Delete($dirs[$i])
    }
}

function Invoke-OriginalPakLayerRollback {
    param(
        [Parameter(Mandatory)][string]$DeployDir,
        [Parameter(Mandatory)][string]$ResourceRootRelative,
        [Parameter(Mandatory)][string]$SubdirName,
        [switch]$DryRun
    )
    $ErrorActionPreference = 'Stop'
    Assert-LayerContract -ResourceRootRelative $ResourceRootRelative -SubdirName $SubdirName
    if ([string]::IsNullOrWhiteSpace($DeployDir)) { throw 'DeployDir 为空' }

    $deployFull = [System.IO.Path]::GetFullPath($DeployDir)
    $resourceRoot = Join-DeployPath -Root $deployFull -Relative $ResourceRootRelative
    $originalDir = Join-DeployPath -Root $resourceRoot -Relative $SubdirName
    $leaf = Split-Path -Leaf $originalDir
    if ($leaf -cne 'Original') { throw "拒绝删除非 Original 目录: $originalDir" }
    if (-not (Test-SameOrNestedPath -Left $deployFull -Right $originalDir)) {
        throw "拒绝删除部署目录之外的路径: $originalDir"
    }
    if ((Get-ComparablePath $originalDir) -eq (Get-ComparablePath $deployFull)) {
        throw "拒绝删除部署根目录: $originalDir"
    }
    if ((Get-ComparablePath $originalDir) -eq (Get-ComparablePath $resourceRoot)) {
        throw "拒绝删除 PlayDH 资源根: $originalDir"
    }
    if (-not (Test-Path -LiteralPath $originalDir -PathType Container)) {
        throw "要回滚的 Original 目录不存在: $originalDir"
    }
    if (Test-ReparsePoint -LiteralPath $originalDir) {
        throw "Original 是重解析点，拒绝删除以免触及链接目标: $originalDir"
    }

    $playDhBefore = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
    if ($DryRun) {
        Write-Host ("计划删除: {0}" -f $originalDir)
        $count = @($playDhBefore -split "`n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
        Write-Host ("已记录 PlayDH pak {0} 个的 SHA-256；删除后会再核对。未删除任何文件。" -f $count)
        return [pscustomobject]@{ Destination = $originalDir; DryRun = $true }
    }

    Remove-OriginalLayerDirectory -LiteralPath $originalDir
    if (Test-Path -LiteralPath $originalDir) {
        throw "Original 目录删除失败: $originalDir"
    }
    $playDhAfter = Get-PakFingerprint -Directory $resourceRoot -Role 'PlayDH 资源根'
    if ($playDhAfter -cne $playDhBefore) {
        throw '删除 Original 后 PlayDH pak 的 SHA-256 发生变化'
    }
    Write-Host ("已删除原版 pak 第二层: {0}。PlayDH pak SHA-256 未变化。" -f $originalDir)
    return [pscustomobject]@{ Destination = $originalDir; DryRun = $false }
}
