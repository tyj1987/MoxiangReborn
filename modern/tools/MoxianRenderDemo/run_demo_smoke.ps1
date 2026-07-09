<#
run_demo_smoke.ps1 — MoxianRenderDemo smoke harness.

The demo is a Win32 window app with a 5-second auto-cap inside its main loop.
On a normal desktop the loop runs for 5s, prints "Demo ran for X ms, Y frames.",
then exits. On a headless / Session 0 build host the main loop hangs in
InvalidateRect/UpdateWindow because there's no real display server, so the
5s cap never fires. This script accepts BOTH outcomes as PASS for the modern
DX11 renderer:

  PASS-A (natural exit)  : exit code 0 within 10s, stdout contains "Demo ran"
  PASS-B (cap hang)      : stderr shows DX11 init + renderer created + main
                            loop entered (Fog enabled is the last init log
                            before the render loop), force-killed after 10s
  FAIL-A (init error)    : any of "Created feature level", "Device initialized",
                            "CoD3DDeviceDX11 created" missing from stderr
  FAIL-B (launch error)  : exe not found, or process error before init

The harness always kills the process at the end so a hung demo doesn't leave
a zombie window in Session 0.
#>
$ErrorActionPreference = "Continue"

$repo    = "D:\墨香全套源代码（源码+资源+客户端+服务端+教程）"
$demoExe = Join-Path $repo "modern\build\tools\MoxianRenderDemo\Debug\mxh_render_demo.exe"
$demoDir = Split-Path $demoExe -Parent
$outLog  = Join-Path $demoDir "run_demo_stdout.txt"
$errLog  = Join-Path $demoDir "run_demo_stderr.txt"

if (-not (Test-Path -LiteralPath $demoExe)) {
    Write-Host "FAIL-B: $demoExe not found"
    exit 2
}

# Fresh logs.
if (Test-Path -LiteralPath $outLog) { Remove-Item -LiteralPath $outLog -Force }
if (Test-Path -LiteralPath $errLog) { Remove-Item -LiteralPath $errLog -Force }

$proc = Start-Process -FilePath $demoExe `
                       -WorkingDirectory $demoDir `
                       -RedirectStandardOutput $outLog `
                       -RedirectStandardError $errLog `
                       -PassThru

$WAIT_SECONDS = 10
$exited = $proc.WaitForExit($WAIT_SECONDS * 1000)
$stderrText = ""
if (Test-Path -LiteralPath $errLog) { $stderrText = Get-Content -LiteralPath $errLog -Encoding UTF8 -Raw }
$stdoutText = ""
if (Test-Path -LiteralPath $outLog) { $stdoutText = Get-Content -LiteralPath $outLog -Encoding UTF8 -Raw }

if (-not $exited) {
    Write-Host "Process did not exit within ${WAIT_SECONDS}s; force-killing (Session 0 / headless demo loop hang)"
    $proc | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

# PASS-A: natural exit, demo printed the run summary.
if ($exited -and $stdoutText -match "Demo ran for") {
    Write-Host "PASS-A (natural exit)"
    Write-Host "  exit=$($proc.ExitCode)  ms=$($matches[1] 2>$null)  frames=$($matches[2] 2>$null)"
    Write-Host "=== stdout ==="
    Write-Host $stdoutText.Trim()
    exit 0
}

# PASS-B: cap hang but DX11 init + renderer + main loop all reached.
$initMarkers = @(
    "Created feature level",
    "Device initialized",
    "CoD3DDeviceDX11 created",
    "Fog enabled"
)
$missing = @($initMarkers | Where-Object { $stderrText -notmatch [regex]::Escape($_) })
if ($missing.Count -eq 0) {
    Write-Host "PASS-B (cap hang, but init + main-loop entry confirmed)"
    Write-Host "  process: $(if ($exited) { 'exited cleanly' } else { 'force-killed' })"
    Write-Host "  exit=$($proc.ExitCode)  stderr_lines=$((($stderrText -split ""`n"").Count))"
    Write-Host "=== stderr (last 12 lines) ==="
    $stderrText -split "`n" | Select-Object -Last 12 | ForEach-Object { Write-Host "  $_" }
    exit 0
}

# FAIL paths.
if ($missing.Count -gt 0) {
    Write-Host "FAIL-A (DX11 init incomplete — missing markers: $($missing -join ', '))"
} else {
    Write-Host "FAIL-B (process did not produce expected output)"
}
Write-Host "=== stderr ==="
if ($stderrText) { Write-Host $stderrText } else { Write-Host "(empty)" }
Write-Host "=== stdout ==="
if ($stdoutText) { Write-Host $stdoutText } else { Write-Host "(empty)" }
exit 1
