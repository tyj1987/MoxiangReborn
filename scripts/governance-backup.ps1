param(
    [string]$RepoRoot = 'C:\moxiang',
    [string]$BackupRoot = 'C:\moxiang\reference\git-backups\2026-08-25'
)

$ErrorActionPreference = 'Stop'
$git = Join-Path $RepoRoot '.git'
$exclude = Join-Path $git 'info\exclude'
New-Item -ItemType Directory -Force -Path $BackupRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $git 'refs\backup\governance-2026-08-25') | Out-Null

$fsck = git -C $RepoRoot -c core.excludesFile=$exclude fsck --no-reflogs --unreachable 2>&1
$commits = @($fsck | ForEach-Object {
    if ($_ -match '^unreachable commit ([0-9a-f]{40})$') { $Matches[1] }
})

foreach ($commit in $commits) {
    git -C $RepoRoot update-ref "refs/backup/governance-2026-08-25/$commit" $commit
}

$bundle = Join-Path $BackupRoot 'moxiang-governance.bundle'
$bundleArgs = @('--all') + $commits
git -C $RepoRoot bundle create $bundle @bundleArgs
$unreachableBundle = Join-Path $BackupRoot 'moxiang-unreachable-commits.bundle'
if ($commits.Count -gt 0) {
    git -C $RepoRoot bundle create $unreachableBundle @commits
}
$commits | Set-Content -LiteralPath (Join-Path $BackupRoot 'unreachable-commits.txt') -Encoding utf8
$fsck | Set-Content -LiteralPath (Join-Path $BackupRoot 'fsck-unreachable.txt') -Encoding utf8
git -C $RepoRoot -c core.excludesFile=$exclude status --short --branch | Set-Content -LiteralPath (Join-Path $BackupRoot 'git-status.txt') -Encoding utf8
git -C $RepoRoot show -s --format=fuller HEAD | Set-Content -LiteralPath (Join-Path $BackupRoot 'head.txt') -Encoding utf8

$scriptSource = Join-Path $RepoRoot 'scripts\launch-human-playable.ps1'
if (Test-Path -LiteralPath $scriptSource) {
    Copy-Item -LiteralPath $scriptSource -Destination (Join-Path $BackupRoot 'launch-human-playable.ps1.original') -Force
    Get-FileHash -Algorithm SHA256 -LiteralPath $scriptSource | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $BackupRoot 'launch-human-playable.sha256.json') -Encoding utf8
}

Write-Output "Backed up $($commits.Count) unreachable commits to $BackupRoot"
Write-Output "Bundle: $bundle"
