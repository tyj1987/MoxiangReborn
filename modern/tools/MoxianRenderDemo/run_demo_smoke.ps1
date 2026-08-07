<#!
MoxianRenderDemo DX11 smoke harness.
PASS-A: natural exit with a demo summary.
PASS-B: headless timeout after all renderer init markers are present.
#>
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\.." )).Path
$demoExe = Join-Path $repo "modern\build\tools\MoxianRenderDemo\Debug\mxh_render_demo.exe"
$demoDir = Split-Path -Parent $demoExe
$outLog = Join-Path $demoDir "run_demo_stdout.txt"
$errLog = Join-Path $demoDir "run_demo_stderr.txt"
if (-not (Test-Path -LiteralPath $demoExe)) { Write-Host "FAIL-B: demo executable not found: $demoExe"; exit 2 }
Remove-Item -LiteralPath $outLog,$errLog -Force -ErrorAction SilentlyContinue
$proc = Start-Process -FilePath $demoExe -WorkingDirectory $demoDir -RedirectStandardOutput $outLog -RedirectStandardError $errLog -PassThru -WindowStyle Hidden
$waitSeconds = 10
$exited = $proc.WaitForExit($waitSeconds * 1000)
if (Test-Path -LiteralPath $errLog) { $stderrText = Get-Content -LiteralPath $errLog -Raw -Encoding UTF8 } else { $stderrText = "" }
if (Test-Path -LiteralPath $outLog) { $stdoutText = Get-Content -LiteralPath $outLog -Raw -Encoding UTF8 } else { $stdoutText = "" }
if (-not $exited) { $proc | Stop-Process -Force -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 300 }
if ($exited -and $stdoutText -match "Demo ran for") { Write-Host "PASS-A (natural exit)"; exit 0 }
$markers = @("Created feature level", "Device initialized", "CoD3DDeviceDX11 created", "Fog enabled")
$missing = @($markers | Where-Object { $stderrText -notmatch [regex]::Escape($_) })
if ($missing.Count -eq 0) { Write-Host "PASS-B (headless timeout after renderer init)"; exit 0 }
Write-Host "FAIL-A: renderer initialization incomplete; missing=$($missing -join ', ')"
if ($stderrText) { Write-Host $stderrText }
exit 1
