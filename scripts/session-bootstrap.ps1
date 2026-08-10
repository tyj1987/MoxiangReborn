#!/usr/bin/env pwsh
# =============================================================================
#  Moxiang-Reborn | Session Bootstrap
#  AGENTS.md §2.5 - 每个新 Mavis session 第一件事
#
#  做什么:
#    0. Harness Probe (git --version 自检, 失败 exit 3 + 5 行替代命令模板)
#    1. 清根目录散落污染 (scratch_*.py / *.log / *.obj / *.db / Testing\Temporary 空目录)
#    2. 检查 working tree (git status --short)
#    3. 检查反 JSON 截断工具箱 scripts\no-truncation.ps1 是否存在
#    4. (隐含) 打印一行确认
#    5. 检查 scripts\safe-shell.ps1 是否存在 (F-1 followup)
#
#  失败模式:
#    - Step 0 失败 -> 退出码 3, 不要进 session
#    - Step 1 / Step 3 / Step 5 失败 -> 退出码 1, 不要进 session
#    - Step 2 有遗留 -> 退出码 2 (警告, 但不阻断, 让 agent 决定)
#
#  根因: 2026-07-30 session 019f9c8e + 019fb254 都死于
#        MiniMax-M3 在 shell_command arguments JSON 中段截断 (EOF at col 208).
#        此脚本确保每个 session 启动时干净状态 + 安全工具链就位.
# =============================================================================

[CmdletBinding()]
param(
    [switch]$CleanKnownArtifacts,
    [switch]$SkipClean,
    [switch]$SkipWorkingTree,
    [switch]$Quiet
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot '.git') -PathType Container)) {
    Write-Error "session-bootstrap.ps1 必须在仓库根目录的 scripts\ 子目录里. 当前: $RepoRoot"
    exit 99
}

# Step 0: Harness Probe (在初始化 $results 之前跑, 失败立即 exit 3)
# 根因: 2026-07-30 session 019fb254 死于 git --version 调用本身,
#       此处确保 harness 基础工具链能跑 -- 失败时给出 5 行替代命令模板.
$harnessProbeOk = $false
try {
    $gv = & git --version 2>&1
    if ($LASTEXITCODE -eq 0) {
        $harnessProbeOk = $true
    }
} catch { }
if (-not $harnessProbeOk) {
    Write-Host 'session-bootstrap Step0 HarnessProbe FAILED.' -ForegroundColor Red
    Write-Host 'git --version 跑不通. harness 截断 / git 不在 PATH / 权限问题都有可能.' -ForegroundColor Red
    Write-Host ''
    Write-Host '替代方案 1 (apply_patch 直写文件):'
    Write-Host '替代方案 2 (pwsh -File scripts\safe-shell.ps1 -Action Read-File -Path C:\moxiang\AGENTS.md):'
    Write-Host '替代方案 3 (pwsh -File scripts\safe-shell.ps1 -Action Write-File -Path C:\moxiang\tmp.txt -Content hello):'
    Write-Host '替代方案 4 (pwsh -File scripts\safe-shell.ps1 -Action Test-PathSafe -Path C:\moxiang):'
    Write-Host '替代方案 5 (pwsh -File scripts\safe-shell.ps1 -Action Get-GitStatus -RepoPath C:\moxiang):'
    exit 3
}

$results = [ordered]@{
    Step0_HarnessProbe    = 'OK'
    Step1_CleanRoot       = 'SKIP'
    Step2_WorkingTree     = 'SKIP'
    Step3_NoTruncation    = 'SKIP'
    Step5_SafeShell       = 'SKIP'
    Step4_Confirm         = 'PENDING'
}

function Write-Summary {
    if ($Quiet) { return }
    Write-Host '' -ForegroundColor Cyan
    Write-Host '----- session-bootstrap summary -----' -ForegroundColor Cyan
    foreach ($k in $results.Keys) {
        $color = if ($results[$k] -match '^(OK|CLEAN|READY)$') { 'Green' } elseif ($results[$k] -match '^WARN') { 'Yellow' } elseif ($results[$k] -match '^(FAIL|MISSING|ERROR)$') { 'Red' } else { 'Gray' }
        Write-Host ("  {0,-22} {1}" -f $k, $results[$k]) -ForegroundColor $color
    }
    Write-Host '' -ForegroundColor Cyan
}

# Step 1: 默认只审计根目录散落；显式 -CleanKnownArtifacts 才删除已知模式。
# 这避免 bootstrap 在用户未看到清单时静默删除根目录文件。
if ($CleanKnownArtifacts -and -not $SkipClean) {
    $cleaned = 0
    Get-ChildItem -LiteralPath $RepoRoot -Filter 'scratch_*.py' -File -ErrorAction SilentlyContinue | ForEach-Object {
        Remove-Item -LiteralPath $_.FullName -Force
        $cleaned++
        if (-not $Quiet) { Write-Host "  removed: $($_.Name)" }
    }
    foreach ($pattern in @('*.log', '*.obj', '*.db', 'test_*.txt')) {
        Get-ChildItem -LiteralPath $RepoRoot -Filter $pattern -File -ErrorAction SilentlyContinue | ForEach-Object {
            if ($_.Name -like '*.db' -and $_.Directory.Name -eq 'moxiang') { return }
            Remove-Item -LiteralPath $_.FullName -Force
            $cleaned++
            if (-not $Quiet) { Write-Host "  removed: $($_.Name)" }
        }
    }
    $testingTemp = Join-Path $RepoRoot 'Testing\Temporary'
    if (Test-Path -LiteralPath $testingTemp) {
        $items = Get-ChildItem -LiteralPath $testingTemp -Force -ErrorAction SilentlyContinue
        if (-not $items) {
            Remove-Item -LiteralPath $testingTemp -Recurse -Force -ErrorAction SilentlyContinue
            $cleaned++
            if (-not $Quiet) { Write-Host "  removed empty: Testing\Temporary\" }
        }
    }
    $results.Step1_CleanRoot = if ($cleaned -eq 0) { 'CLEAN' } else { "OK ($cleaned items)" }
} elseif (-not $SkipClean) {
    $candidates = @()
    foreach ($pattern in @('scratch_*.py', '*.log', '*.obj', '*.db', '*.db-shm', '*.db-wal', 'test_*.txt')) {
        $candidates += Get-ChildItem -LiteralPath $RepoRoot -Filter $pattern -File -ErrorAction SilentlyContinue
    }
    $candidateCount = @($candidates | Sort-Object FullName -Unique).Count
    $results.Step1_CleanRoot = if ($candidateCount -eq 0) { 'CLEAN' } else { "WARN ($candidateCount candidates; rerun with -CleanKnownArtifacts after review)" }
}

# Step 2: working tree
if (-not $SkipWorkingTree) {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        $results.Step2_WorkingTree = 'WARN (git not in PATH)'
    } else {
        $status = & git -C $RepoRoot status --short 2>&1
        if ($LASTEXITCODE -ne 0) {
            $results.Step2_WorkingTree = "WARN (git exit $LASTEXITCODE)"
        } elseif ([string]::IsNullOrWhiteSpace(($status | Out-String))) {
            $results.Step2_WorkingTree = 'CLEAN'
        } else {
            $count = ($status | Measure-Object).Count
            $results.Step2_WorkingTree = "WARN ($count dirty -- 上 session 留下未提交工作)"
            if (-not $Quiet) {
                Write-Host '  --- git status ---' -ForegroundColor Yellow
                $status | ForEach-Object { Write-Host "    $_" }
                Write-Host '  ------------------' -ForegroundColor Yellow
            }
        }
    }
}

# Step 3: 反 JSON 截断工具箱
$noTruncationPath = Join-Path $PSScriptRoot 'no-truncation.ps1'
if (Test-Path -LiteralPath $noTruncationPath) {
    $results.Step3_NoTruncation = 'OK'
    if (-not $Quiet) {
        Write-Host '' -ForegroundColor Cyan
        Write-Host '反 JSON 截断工具箱 (AGENTS.md §3 F-1) 已就位:' -ForegroundColor Cyan
        Write-Host "  $noTruncationPath"
        Write-Host '' -ForegroundColor Cyan
        Write-Host '用法示例 (单行 shell_command, 不会触发 JSON 截断):'
        Write-Host '  pwsh -File scripts\no-truncation.ps1 -Action Get-FileLines -Path C:\moxiang\modern\src\foo.cpp -StartLine 1 -EndLine 50'
        Write-Host '  pwsh -File scripts\no-truncation.ps1 -Action Test-PathSafe -Path C:\moxiang\modern\build'
        Write-Host '  pwsh -File scripts\no-truncation.ps1 -Action Format-TestOutput -ExePath modern\build\tests\unit\server\Debug\mxh_server_handler_tests.exe -Filter ServerHandler*'
        Write-Host '' -ForegroundColor Cyan
    }
} else {
    $results.Step3_NoTruncation = 'MISSING'
}

# Step 5: safe-shell.ps1 存在性检查 (F-1 followup -- bootstrap 自己挂了也能有 4 个 alias 兜底)
$safeShellPath = Join-Path $PSScriptRoot 'safe-shell.ps1'
if (Test-Path -LiteralPath $safeShellPath) {
    $results.Step5_SafeShell = 'OK'
} else {
    $results.Step5_SafeShell = 'MISSING'
}

$blocking = ($results.Step1_CleanRoot.StartsWith('FAIL') -or $results.Step3_NoTruncation -eq 'MISSING' -or $results.Step5_SafeShell -eq 'MISSING' -or $results.Step0_HarnessProbe.StartsWith('FAIL'))
if ($blocking) {
    $results.Step4_Confirm = 'FAIL'
    Write-Summary
    Write-Error 'session-bootstrap FAILED (Step0/1/3/5). 修完再进 session.'
    exit 1
} else {
    $results.Step4_Confirm = 'READY'
    Write-Summary
    Write-Host 'AGENTS.md OK | scratch AUDITED | bootstrap LOADED | ready.' -ForegroundColor Green
    exit 0
}
