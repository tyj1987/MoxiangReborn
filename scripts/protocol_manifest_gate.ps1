# Compare protocol.manifest files beside a map server and mxh_client.
# PROTOCOL_MATCH item=<hash> or PROTOCOL_MISMATCH item=<map>/<client>.
# missing means that side has no manifest or the value is not 64 lowercase hex.
# Keep scripts/protocol_manifest_gate.py in step. ASCII only.

function Resolve-MxhBuiltExecutable {
    param(
        [string]$ToolsRoot,
        [string]$ToolDirectory,
        [string]$FileName,
        [string]$Config = 'Debug'
    )
    $toolRoot = Join-Path $ToolsRoot $ToolDirectory
    $candidates = @(
        (Join-Path $toolRoot $FileName),
        (Join-Path (Join-Path $toolRoot $Config) $FileName)
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }
    return ''
}

function Get-MxhItemProtocolHashFromManifest {
    param([string]$ManifestPath)
    if ([string]::IsNullOrWhiteSpace($ManifestPath)) { return 'missing' }
    if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) { return 'missing' }
    $text = ''
    try {
        $text = [System.IO.File]::ReadAllText($ManifestPath)
    } catch {
        return 'missing'
    }
    foreach ($line in ($text -split "`n")) {
        $trimmed = $line.Trim().Trim("`r")
        if (-not $trimmed.StartsWith('item_protocol_sha256=')) { continue }
        $value = $trimmed.Substring(21).Trim()
        if ([regex]::IsMatch($value, '^[0-9a-f]{64}$')) { return $value }
        return 'missing'
    }
    return 'missing'
}

function Get-MxhItemProtocolHashFromExecutable {
    param([string]$Executable)
    if ([string]::IsNullOrWhiteSpace($Executable)) { return 'missing' }
    if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) { return 'missing' }
    $manifest = Join-Path (Split-Path -Parent $Executable) 'protocol.manifest'
    return Get-MxhItemProtocolHashFromManifest -ManifestPath $manifest
}

function Assert-MxhItemProtocolMatch {
    param(
        [string]$MapExecutable,
        [string]$ClientExecutable
    )
    $mapHash = Get-MxhItemProtocolHashFromExecutable -Executable $MapExecutable
    $clientHash = Get-MxhItemProtocolHashFromExecutable -Executable $ClientExecutable
    $same = [StringComparer]::Ordinal.Equals($mapHash, $clientHash)
    if ($mapHash -eq 'missing' -or $clientHash -eq 'missing' -or -not $same) {
        Write-Host "PROTOCOL_MISMATCH item=$mapHash/$clientHash"
        exit 1
    }
    Write-Host "PROTOCOL_MATCH item=$mapHash"
}
