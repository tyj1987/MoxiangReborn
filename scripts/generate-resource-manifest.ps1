param(
    [Parameter(Mandatory = $true)][string]$Root,
    [Parameter(Mandatory = $true)][string]$Output,
    [Parameter(Mandatory = $true)][string]$ProfileId,
    [string]$Source = ''
)

$ErrorActionPreference = 'Stop'
$resolvedRoot = (Resolve-Path -LiteralPath $Root).Path
$parent = Split-Path -Parent $Output
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$rows = @(
foreach ($file in (Get-ChildItem -LiteralPath $resolvedRoot -File -Recurse -Force | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($resolvedRoot.Length).TrimStart('\', '/').Replace('\', '/')
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
    [pscustomobject]@{
        path = $relative
        bytes = [int64]$file.Length
        sha256 = $hash
    }
}
)
$doc = [ordered]@{
    schemaVersion = 1
    profileId = $ProfileId
    source = $Source
    root = $resolvedRoot
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
    fileCount = $rows.Count
    byteCount = ($rows | Measure-Object -Property bytes -Sum).Sum
    files = $rows
}
$doc | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Output -Encoding utf8
Write-Output "Wrote $($rows.Count) files to $Output"
