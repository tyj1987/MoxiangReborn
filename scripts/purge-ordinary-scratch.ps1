$scratchRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\modern\scratch')).Path
$keep = Join-Path $scratchRoot '2026-08-20-source-recovery'
$targets = Get-ChildItem -LiteralPath $scratchRoot -Force | Where-Object { $_.FullName -ne $keep }
foreach ($target in $targets) {
    try {
        Remove-Item -LiteralPath $target.FullName -Recurse -Force -ErrorAction Stop
    } catch {
        Write-Warning ("Preserved locked scratch entry: {0} ({1})" -f $target.FullName, $_.Exception.Message)
    }
}
$remaining = @(Get-ChildItem -LiteralPath $scratchRoot -Force | Where-Object { $_.FullName -ne $keep })
Write-Output ("Removed ordinary scratch entries; retained {0} non-recovery entries and recovery at {1}" -f $remaining.Count, $keep)
if ($remaining.Count -gt 0) { exit 1 }
