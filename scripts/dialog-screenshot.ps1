<#
dialog-screenshot.ps1 — M-R0/M-R4 工具

截单个 dialog 截图，用于 M-R4 验证 165 dialog 视觉 1:1。

当前是框架版本。M-R3 完成 cDialog 树 + M-R4 完成 Init 真实 sprite 之后，
才能真正截出有 sprite 的 dialog 图。

usage:
  pwsh -File scripts/dialog-screenshot.ps1 -DialogName inventory
  pwsh -File scripts/dialog-screenshot.ps1 -DialogName shop -OpenKey B
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$DialogName,
    [string]$OpenKey = 'I',
    [int]$TimeoutSeconds = 60,
    [string]$BuildDir = '',
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $repoRoot 'modern\build' }
$buildRoot = (Resolve-Path $BuildDir).Path
$clientExe = Join-Path $buildRoot 'tools\MoxianClient\Debug\mxh_client.exe'
$serverScript = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'

if (-not (Test-Path -LiteralPath $clientExe)) {
    throw "MoxianClient.exe not found: $clientExe"
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $repoRoot "modern\docs\restoration-plan\dialogs\${DialogName}.tga"
}

# 当前阶段：M-R3/M-R4 还没完成，dialog 树还没接 cDialog
# 所以这个脚本只能截 5 状态基线图，dialog 自身还没法单独开
Write-Host "[dialog-screenshot] STUB MODE" -ForegroundColor Yellow
Write-Host "[dialog-screenshot] Dialog=$DialogName OpenKey=$OpenKey" -ForegroundColor Yellow
Write-Host "[dialog-screenshot] M-R3 + M-R4 还没完成，无法对单 dialog 单独截" -ForegroundColor Yellow
Write-Host "[dialog-screenshot] 等 M-R4 完成后这个脚本才生效" -ForegroundColor Yellow
Write-Host "[dialog-screenshot] 占位 output: $OutputPath" -ForegroundColor Yellow

# TODO M-R4: 实现
# 1. 启动 modern server
# 2. 启动 MoxianClient 进 GameIn
# 3. 模拟按键 $OpenKey 打开 dialog
# 4. CaptureScreen 截 $OutputPath
# 5. 关闭 client + server
