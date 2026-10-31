# Shared PlayDH package exclusion. Patterns come from
# modern/config/resource-profiles/playdh-current.json inventory.knownDerivedPaths
# plus MHClient*.exe. Callers copy through Copy-MxhPlayDhFiltered and then
# Invoke-MxhPlayDhPackageScan; a hit exits 1.

function Get-MxhPlayDhRepoRoot {
    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
}

function Get-MxhPlayDhExcludePatterns {
    param([Parameter(Mandatory)][string]$ProfilePath)
    $doc = Get-Content -LiteralPath $ProfilePath -Raw -Encoding UTF8 | ConvertFrom-Json
    $raw = $doc.inventory.knownDerivedPaths
    if ($null -eq $raw) { throw 'knownDerivedPaths missing' }
    $patterns = @($raw | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    if ($patterns.Count -lt 1) { throw 'knownDerivedPaths empty' }
    return @($patterns + 'MHClient*.exe')
}

function Test-MxhPlayDhExcludedRelativePath {
    param(
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][string[]]$Patterns
    )
    $rel = (($RelativePath -replace '\\','/').TrimStart('/'))
    if ([string]::IsNullOrWhiteSpace($rel)) { return $false }
    $leaf = $rel
    $slash = $rel.LastIndexOf('/')
    if ($slash -ge 0) { $leaf = $rel.Substring($slash + 1) }
    foreach ($pattern in $Patterns) {
        if ([string]::IsNullOrWhiteSpace($pattern)) { continue }
        $normalized = (($pattern -replace '\\','/').Trim())
        if ($normalized.EndsWith('/')) {
            $dir = $normalized.TrimEnd('/')
            if ($rel.StartsWith($normalized) -or $rel.Contains('/' + $normalized)) { return $true }
            if (($rel.Split('/')) -contains $dir) { return $true }
        } elseif ($leaf -like $normalized -or $rel -like $normalized) {
            return $true
        }
    }
    return $false
}

function Copy-MxhPlayDhFiltered {
    param(
        [Parameter(Mandatory)][string]$SourceRoot,
        [Parameter(Mandatory)][string]$SourcePath,
        [Parameter(Mandatory)][string]$DestinationRoot,
        [Parameter(Mandatory)][string[]]$Patterns
    )
    if (-not (Test-Path -LiteralPath $SourcePath)) { return }
    $sep = [System.IO.Path]::DirectorySeparatorChar
    $rootFull = [System.IO.Path]::GetFullPath($SourceRoot).TrimEnd('\','/')
    $sourceFull = [System.IO.Path]::GetFullPath($SourcePath).TrimEnd('\','/')
    $rootOk = ($sourceFull -eq $rootFull) -or $sourceFull.StartsWith($rootFull + $sep) -or $sourceFull.StartsWith($rootFull + '/')
    if (-not $rootOk) { throw 'source path escapes source root' }
    Get-ChildItem -LiteralPath $SourcePath -Recurse -File -Force | ForEach-Object {
        $full = [System.IO.Path]::GetFullPath($_.FullName)
        $fileOk = ($full -eq $rootFull) -or $full.StartsWith($rootFull + $sep) -or $full.StartsWith($rootFull + '/')
        if (-not $fileOk) { throw 'file escapes source root' }
        $relFromRoot = $full.Substring($rootFull.Length).TrimStart('\','/')
        if (Test-MxhPlayDhExcludedRelativePath -RelativePath $relFromRoot -Patterns $Patterns) { return }
        $relDest = $full.Substring($sourceFull.Length).TrimStart('\','/')
        $dest = Join-Path $DestinationRoot $relDest
        $parent = Split-Path -Parent $dest
        if ($parent -and -not (Test-Path -LiteralPath $parent)) {
            New-Item -ItemType Directory -Path $parent -Force | Out-Null
        }
        Copy-Item -LiteralPath $full -Destination $dest -Force
    }
}

function Get-MxhPlayDhPackageForbidden {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string[]]$Patterns
    )
    $hits = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $Root)) { return @() }
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    Get-ChildItem -LiteralPath $Root -Recurse -File -Force | ForEach-Object {
        $full = [System.IO.Path]::GetFullPath($_.FullName)
        $rel = ($full.Substring($rootFull.Length).TrimStart('\','/')) -replace '\\','/'
        if (Test-MxhPlayDhExcludedRelativePath -RelativePath $rel -Patterns $Patterns) {
            [void]$hits.Add($rel)
        }
    }
    return @($hits.ToArray())
}

function Invoke-MxhPlayDhPackageScan {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string[]]$Patterns
    )
    $hits = @(Get-MxhPlayDhPackageForbidden -Root $Root -Patterns $Patterns)
    if ($hits.Count -gt 0) {
        foreach ($hit in $hits) { Write-Host "PLAYDH_PACKAGE_FORBIDDEN $hit" }
        return 1
    }
    $count = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Force).Count
    Write-Host "PLAYDH_PACKAGE_SCAN OK files=$count"
    return 0
}
