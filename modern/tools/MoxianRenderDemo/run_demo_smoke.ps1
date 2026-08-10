param(
    [string]$DemoExe = "",
    [string]$Verifier = ""
)

$ErrorActionPreference = "Stop"
$effectivePath = [Environment]::GetEnvironmentVariable('Path', 'Process')
Remove-Item -LiteralPath Env:Path -ErrorAction SilentlyContinue
Remove-Item -LiteralPath Env:PATH -ErrorAction SilentlyContinue
[Environment]::SetEnvironmentVariable('Path', $effectivePath, 'Process')

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\.." )).Path
if (-not $DemoExe) {
    $DemoExe = Join-Path $repo "modern\build\tools\MoxianRenderDemo\Debug\mxh_render_demo.exe"
}
if (-not $Verifier) {
    $Verifier = Join-Path $repo "scripts\verify-render-frame.py"
}
if (-not (Test-Path -LiteralPath $DemoExe)) {
    Write-Host "FAIL: demo executable not found: $DemoExe"
    exit 2
}
if (-not (Test-Path -LiteralPath $Verifier)) {
    Write-Host "FAIL: frame verifier not found: $Verifier"
    exit 2
}

$demoDir = Split-Path -Parent $DemoExe
$outLog = Join-Path $demoDir "run_demo_stdout.txt"
$errLog = Join-Path $demoDir "run_demo_stderr.txt"
$frame = Join-Path $demoDir "run_demo_frame.tga"
Remove-Item -LiteralPath $outLog,$errLog,$frame -Force -ErrorAction SilentlyContinue

$arguments = @("--headless", "--save-frame", $frame, "--frame-count", "3")
$proc = Start-Process -FilePath $DemoExe -ArgumentList $arguments -WorkingDirectory $demoDir -RedirectStandardOutput $outLog -RedirectStandardError $errLog -PassThru -WindowStyle Hidden
$exited = $proc.WaitForExit(10000)
if (-not $exited) {
    $proc | Stop-Process -Force -ErrorAction SilentlyContinue
    Write-Host "FAIL: headless render did not exit within 10 seconds"
    exit 1
}
$proc.WaitForExit()
$stdoutText = if (Test-Path -LiteralPath $outLog) { Get-Content -LiteralPath $outLog -Raw -Encoding UTF8 } else { "" }
if ($stdoutText -notmatch "Demo ran for") {
    Write-Host "FAIL: demo did not report a clean shutdown"
    if (Test-Path -LiteralPath $errLog) { Get-Content -LiteralPath $errLog }
    exit 1
}

& python $Verifier $frame
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: saved frame did not meet the R-9 pixel contract"
    exit 1
}

Write-Host "PASS: headless render exited naturally and frame content is valid"
exit 0
