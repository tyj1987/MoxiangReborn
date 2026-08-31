# scripts/register-test-account.ps1
#
# Phase 1 of human-acceptance: register a single test account against the
# modern server database (SQLite by default; pass -DbConfig for MSSQL).
#
# Usage:
#   pwsh -File scripts\register-test-account.ps1 `
#        -AccountName ha_user01 `
#        -Password 'Test1234' `
#        -DbConfig 'sqlite;path=C:\moxiang\deploy\runtime\modern\data\moxian.db'
#
# Or rely on MXH_DATABASE_CONFIG env var (set by start_modern.ps1):
#   pwsh -File scripts\register-test-account.ps1 -AccountName ha_user01 -Password 'Test1234'
#
# Returns:
#   exit 0 — account already existed OR was created successfully
#   exit 1 — register failed (tool non-zero / password rejected)
#
# Notes:
#   - Account name must be lowercase ASCII 4-16 chars (per legacy client limits).
#   - Password is read by mxh_db_tool.exe via stdin, encoded ASCII to avoid
#     PowerShell pipe encoding (UTF-16 NUL bytes) corrupting the read.
#   - "Account already exists" is treated as PASS because the typical human
#     acceptance rerun reuses the same account.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-z0-9_]{4,16}$')]
    [string]$AccountName,

    [Parameter(Mandatory = $true)]
    [ValidateLength(4, 32)]
    [string]$Password,

    [string]$DbConfig = '',

    [string]$DbEnv = 'MXH_DATABASE_CONFIG',

    [string]$DbTool = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ([string]::IsNullOrWhiteSpace($DbTool)) {
    $DbTool = Join-Path $repoRoot 'modern\build\tools\MoxianDbTool\mxh_db_tool.exe'
}
if (-not (Test-Path -LiteralPath $DbTool -PathType Leaf)) {
    throw "Missing mxh_db_tool.exe: $DbTool  (rebuild: scripts\build-modern.bat Debug mxh_db_tool)"
}

# Build the argument list in the exact order mxh_db_tool expects:
#   register [--db <cfg> | --db-env <env>] <account>
$argList = @('register')
if (-not [string]::IsNullOrWhiteSpace($DbConfig)) {
    $argList += @('--db', $DbConfig)
} else {
    $resolvedEnv = [Environment]::GetEnvironmentVariable($DbEnv, 'Process')
    if ([string]::IsNullOrWhiteSpace($resolvedEnv)) {
        throw "No -DbConfig provided and $DbEnv env var is not set. Run start_modern.ps1 first or pass -DbConfig."
    }
    $argList += @('--db-env', $DbEnv)
}
$argList += $AccountName

# Write the password to a temp file in pure ASCII to avoid PowerShell pipe
# re-encoding it as UTF-16 (mxh_db_tool reads stdin byte-exact).
$stdinFile = Join-Path ([System.IO.Path]::GetTempPath()) ("register-stdin-" + [Guid]::NewGuid().ToString('N') + '.txt')
try {
    [System.IO.File]::WriteAllText($stdinFile, $Password + "`n", [System.Text.Encoding]::ASCII)

    $stdoutFile = Join-Path ([System.IO.Path]::GetTempPath()) ("register-stdout-" + [Guid]::NewGuid().ToString('N') + '.log')
    $stderrFile = Join-Path ([System.IO.Path]::GetTempPath()) ("register-stderr-" + [Guid]::NewGuid().ToString('N') + '.log')
    try {
        $proc = Start-Process -FilePath $DbTool -ArgumentList $argList `
            -RedirectStandardInput $stdinFile `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError $stderrFile `
            -NoNewWindow -Wait -PassThru
        $stdout = if (Test-Path -LiteralPath $stdoutFile) { (Get-Content -LiteralPath $stdoutFile -Raw) } else { '' }
        $stderr = if (Test-Path -LiteralPath $stderrFile) { (Get-Content -LiteralPath $stderrFile -Raw) } else { '' }
        $combined = ($stdout + "`n" + $stderr).Trim()

        if ($proc.ExitCode -eq 0) {
            Write-Host "register OK: account='$AccountName'  (exit=0)" -ForegroundColor Green
            exit 0
        }
        # Treat "already exists" as success because human-acceptance reruns
        # often re-register the same account and we don't want to fail there.
        if ($combined -match 'already exists|already_exists|duplicate|exists') {
            Write-Host "register OK (already exists): account='$AccountName'" -ForegroundColor Yellow
            exit 0
        }
        Write-Host "register FAIL: account='$AccountName' exit=$($proc.ExitCode)" -ForegroundColor Red
        if ($combined.Length -gt 0) { Write-Host "--- tool output ---`n$combined`n--- end ---" }
        exit 1
    } finally {
        Remove-Item -LiteralPath $stdoutFile,$stderrFile -Force -ErrorAction SilentlyContinue
    }
} finally {
    Remove-Item -LiteralPath $stdinFile -Force -ErrorAction SilentlyContinue
}
