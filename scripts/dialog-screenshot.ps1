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
    $OutputPath = Join-Path $repoRoot "modern\out\runs\dialog-screenshot\${DialogName}.tga"
}

# 单 dialog 截图仍要求真实 UI 树、输入分发和 settled-frame 采集；
# 在这些能力完成前必须硬失败，不能把日志或空路径当作视觉证据。
throw "dialog-screenshot is not available yet: real dialog capture is not implemented for '$DialogName' (OpenKey=$OpenKey). No screenshot was created."
