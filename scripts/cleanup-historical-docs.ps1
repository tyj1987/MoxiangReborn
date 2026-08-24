$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$targets = @(
    'docs/PHASE_6_3_STATUS.md',
    'docs/PHASE_6_PROGRESS_2026-07-30.md',
    'docs/PLAN_2026Q3.md',
    'docs/FAILURE-LOG.md',
    'docs/HONEST-1.0-RC-ASSESSMENT.md',
    'docs/KNOWN_BUGS_ARCHIVE.md',
    'docs/PLAN_PORTAL.md',
    'docs/COMMERCIAL_RC_VISUAL_VERIFICATION.md',
    'docs/REAL_GAME_VERIFICATION.md',
    'docs/GPU_SMOKE.md',
    'docs/CLEANUP_MANIFEST.md',
    'docs/RELEASE_READINESS.md',
    'docs/SOAK/soak-2026-08-18-findings.md',
    'docs/SOAK/soak-30m-CANARY-2026-08-18-PASS.md',
    'docs/SOAK/soak-4h-mssql-2026-08-18.md'
)
foreach ($relative in $targets) {
    $path = Join-Path $repoRoot $relative
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Remove-Item -LiteralPath $path -Force
        Write-Output "Removed $relative"
    }
}
$archive = Join-Path $repoRoot 'docs/archive/vm-gpu-pv'
if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Recurse -Force
    Write-Output 'Removed docs/archive/vm-gpu-pv'
}
$restoration = Join-Path $repoRoot 'modern/docs/restoration-plan'
if (Test-Path -LiteralPath $restoration) {
    Remove-Item -LiteralPath $restoration -Recurse -Force
    Write-Output 'Removed modern/docs/restoration-plan'
}
