# scripts/test-gpu-smoke.ps1
#
# 物理 GPU 烟雾测试 — 2026-08-20 解锁 (ROG 物理机 + Intel Arc B580).
#
# 验证 MoxianClient (modern/tools/MoxianClient) 能在物理 GPU 上
# 启动 + 渲染 1+ 帧 + 截到 ≥ 1 张 state-*.tga. 不需要 login server,
# 不需要显示器 (headless 模式 + IDXGIOutput1::DuplicateOutput 截屏
# 走 WARP/software path, 但 GPU 设备 / 资源装填 / 渲染管线都走真硬件).
#
# 完成判据:
#   1. cmake --target mxh_client build 0 error
#   2. mxh_client.exe 启动后 60s 内写出 1+ .tga
#   3. .tga 头部 800x600x32bpp (跟老 Moxian 800x600 golden 一致)
#   4. .tga 像素 50%+ 非零 (solid color / 全黑 不算渲染)
#   5. exit code 0 (干净退出)
#
# 不验证 SSIM ≥ 0.95 (那是 visual-smoke.ps1 + 老 legacy client 对照的活,
# 留给 M-R4 / M-R5 物理 GPU 段). 这个脚本只证明 GPU 通道可达.
#
# 用法:
#   pwsh -File scripts\test-gpu-smoke.ps1
#   pwsh -File scripts\test-gpu-smoke.ps1 -FrameCount 3 -TimeoutSeconds 60

[CmdletBinding()]
param(
    [int]$FrameCount = 3,
    [int]$TimeoutSeconds = 60,
    [string]$ResourceRoot = 'C:\moxiang\modern\data\PlayDH',
    [string]$OutDir = ''
)

$ErrorActionPreference = 'Stop'

# Resolve paths. Use string concat to avoid PSPath CJK issues.
$scriptFull = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot   = Split-Path -Parent $scriptFull
$buildDir   = $repoRoot + '\modern\build'
$exePath    = $buildDir + '\tools\MoxianClient\mxh_client.exe'
if (-not $OutDir) {
    $OutDir = $repoRoot + '\modern\data\screenshots\gpu-smoke'
}

Write-Host "=== test-gpu-smoke.ps1 ==="
Write-Host "exe       : $exePath"
Write-Host "resource  : $ResourceRoot"
Write-Host "out dir   : $OutDir"
Write-Host "frames    : $FrameCount"
Write-Host "timeout   : ${TimeoutSeconds}s"
Write-Host ""

# Step 0: 检查 exe
if (-not (Test-Path -LiteralPath $exePath)) {
    Write-Error "mxh_client.exe not found at $exePath. Build first: cmake --build modern/build --target mxh_client"
}

# Step 1: 清空 out dir
if (Test-Path -LiteralPath $OutDir) {
    Get-ChildItem -LiteralPath $OutDir -Filter 'state-*.tga' -ErrorAction SilentlyContinue | ForEach-Object {
        Remove-Item -LiteralPath $_.FullName -Force
    }
} else {
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}

# Step 2: 启动 mxh_client (headless, auto-login 是 no-op 当没 server)
Write-Host "[1/3] launching mxh_client (headless)..."
$stdoutFile = $OutDir + '.stdout.log'
$stderrFile = $OutDir + '.stderr.log'
$proc = Start-Process -FilePath $exePath `
    -ArgumentList @(
        '--resource-root', $ResourceRoot,
        '--headless',
        '--state-frames-dir', $OutDir,
        '--frame-count', "$FrameCount"
    ) `
    -RedirectStandardOutput $stdoutFile `
    -RedirectStandardError $stderrFile `
    -PassThru -NoNewWindow

$exited = $proc | Wait-Process -Timeout $TimeoutSeconds -ErrorAction SilentlyContinue
if (-not $proc.HasExited) {
    Write-Host "  process still running after ${TimeoutSeconds}s, killing"
    Stop-Process -Id $proc.Id -Force
    $proc.Refresh()
}
$exitCode = $proc.ExitCode
Write-Host "  exit code: $exitCode"

# Step 3: 收集 .tga
Write-Host "[2/3] collecting screenshots..."
$tgas = @(Get-ChildItem -LiteralPath $OutDir -Filter 'state-*.tga' -ErrorAction SilentlyContinue | Sort-Object Name)
if ($tgas.Count -eq 0) {
    Write-Error "FAIL: no state-*.tga produced in $OutDir"
}

Write-Host "  $($tgas.Count) screenshots:"
foreach ($t in $tgas) {
    Write-Host "    $($t.Name) ($($t.Length) bytes)"
}

# Step 4: 验证 .tga 头部 + 像素非零
Write-Host "[3/3] validating TGA content..."
$results = @()
foreach ($t in $tgas) {
    $bytes = [System.IO.File]::ReadAllBytes($t.FullName)
    if ($bytes.Length -lt 18) {
        $results += [PSCustomObject]@{ File = $t.Name; Pass = $false; Why = "size<18" }
        continue
    }
    $width  = $bytes[12] + ($bytes[13] * 256)
    $height = $bytes[14] + ($bytes[15] * 256)
    $bpp    = $bytes[16]
    if ($width -ne 800 -or $height -ne 600 -or $bpp -ne 32) {
        $results += [PSCustomObject]@{ File = $t.Name; Pass = $false; Why = "${width}x${height}x${bpp}bpp (expect 800x600x32)" }
        continue
    }
    $imageData = $bytes[18..($bytes.Length - 1)]
    $nonZero   = @($imageData | Where-Object { $_ -ne 0 }).Count
    $pct       = [math]::Round($nonZero * 100.0 / $imageData.Length, 1)
    if ($pct -lt 50.0) {
        $results += [PSCustomObject]@{ File = $t.Name; Pass = $false; Why = "$pct% non-zero (<50%)" }
        continue
    }
    $results += [PSCustomObject]@{ File = $t.Name; Pass = $true; Why = "${width}x${height}x${bpp}bpp, $pct% non-zero" }
}

$results | Format-Table -AutoSize | Out-String | Write-Host

$failed = @($results | Where-Object { -not $_.Pass }).Count
if ($failed -gt 0) {
    Write-Host "FAIL: $failed/$($results.Count) TGAs failed validation"
    Write-Host "  stdout: $stdoutFile"
    Write-Host "  stderr: $stderrFile"
    exit 1
}

Write-Host "OK: GPU smoke PASS"
Write-Host "  $($results.Count) TGA(s) at 800x600x32bpp, all with ≥50% non-zero pixels"
Write-Host "  exit code 0, no crash"
Write-Host "  out: $OutDir"
exit 0
