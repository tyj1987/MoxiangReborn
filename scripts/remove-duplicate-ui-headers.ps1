$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$includeRoot = Join-Path $repoRoot 'modern\include\mxh\ui'
$sourceRoot = Join-Path $repoRoot 'modern\src\ui'
$backup = Join-Path $repoRoot 'reference\manifests\ui-header-duplicates-2026-08-25.txt'
$trackedFiles = @((git -C $repoRoot ls-files -- 'modern/include/mxh/ui/*'))
$duplicates = @()
foreach ($file in Get-ChildItem -LiteralPath $includeRoot -File) {
    $tracked = "modern/include/mxh/ui/" + $file.Name
    $source = Join-Path $sourceRoot $file.Name
    if (($trackedFiles -notcontains $tracked) -and (Test-Path -LiteralPath $source)) {
        $includeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash
        $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
        if ($includeHash -eq $sourceHash) {
            $duplicates += [pscustomobject]@{ Path = $file.FullName; Sha256 = $includeHash }
        }
    }
}
$duplicates | ConvertTo-Csv -NoTypeInformation | Set-Content -LiteralPath $backup -Encoding utf8
foreach ($item in $duplicates) {
    Remove-Item -LiteralPath $item.Path -Force
}
Write-Output "Removed $($duplicates.Count) exact untracked UI header duplicates"
