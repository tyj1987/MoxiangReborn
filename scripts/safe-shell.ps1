#!/usr/bin/env pwsh
# =============================================================================
#  Moxiang-Reborn | Safe Shell Wrapper
#  AGENTS.md §2.5 / §3 F-1 followup - 4 action wrappers using raw .NET API
#
#  为什么不用 PowerShell 原生命令:
#    - Get-Content / Set-Content / Test-Path 在中文路径 / 含 [] 的目录名上
#      会触发解析器 bug 或 wildcard 展开
#    - & git ... 在 PowerShell pipeline 里 arguments 引号会被 reparse,
#      遇到 CJK 路径 + 转义时会再次触发 shell_command JSON 截断
#    - 改用 [System.IO.File] 静态方法 + ProcessStartInfo 裸 .NET API,
#      path 直接当 string 传, 没有任何 parser 介入
#
#  4 actions (单行 shell_command 调用, 不会触发 JSON 截断):
#    Read-File       pwsh -File scripts\safe-shell.ps1 -Action Read-File -Path X
#    Write-File      pwsh -File scripts\safe-shell.ps1 -Action Write-File -Path X -Content Y
#    Test-PathSafe   pwsh -File scripts\safe-shell.ps1 -Action Test-PathSafe -Path X
#    Get-GitStatus   pwsh -File scripts\safe-shell.ps1 -Action Get-GitStatus -RepoPath X
#
#  Exit codes:
#    0  success
#    1  bad arguments (missing required -Path / -RepoPath)
#    2  IO / process error
# =============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Read-File','Write-File','Test-PathSafe','Get-GitStatus')]
    [string]$Action,

    [string]$Path = '',
    [string]$Content = '',
    [string]$RepoPath = ''
)

$ErrorActionPreference = 'Stop'

function Invoke-ReadFile {
    param([string]$P)
    if ([string]::IsNullOrEmpty($P)) { Write-Error 'Path required'; exit 1 }
    try {
        $bytes = [System.IO.File]::ReadAllBytes($P)
        $text = [System.Text.Encoding]::UTF8.GetString($bytes)
        [Console]::Out.Write($text)
        exit 0
    } catch {
        Write-Error ('Read-File failed: ' + $_.Exception.Message)
        exit 2
    }
}

function Invoke-WriteFile {
    param([string]$P, [string]$C)
    if ([string]::IsNullOrEmpty($P)) { Write-Error 'Path required'; exit 1 }
    try {
        $dir = [System.IO.Path]::GetDirectoryName($P)
        if (-not [string]::IsNullOrEmpty($dir) -and -not [System.IO.Directory]::Exists($dir)) {
            [System.IO.Directory]::CreateDirectory($dir) | Out-Null
        }
        $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
        $bytes = $utf8NoBom.GetBytes($C)
        [System.IO.File]::WriteAllBytes($P, $bytes)
        Write-Output ('written: {0} ({1} bytes)' -f $P, $bytes.Length)
        exit 0
    } catch {
        Write-Error ('Write-File failed: ' + $_.Exception.Message)
        exit 2
    }
}

function Invoke-TestPathSafe {
    param([string]$P)
    if ([string]::IsNullOrEmpty($P)) { Write-Error 'Path required'; exit 1 }
    $exists = [System.IO.File]::Exists($P)
    if (-not $exists) { $exists = [System.IO.Directory]::Exists($P) }
    if ($exists) { Write-Output 'true' } else { Write-Output 'false' }
    exit 0
}

function Invoke-GitStatus {
    param([string]$R)
    $cwd = if (-not [string]::IsNullOrEmpty($R)) { $R } else { (Get-Location).Path }
    try {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = 'git'
        $psi.Arguments = 'status --short'
        $psi.WorkingDirectory = $cwd
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.UseShellExecute = $false
        $psi.CreateNoWindow = $true
        $proc = [System.Diagnostics.Process]::Start($psi)
        $out = $proc.StandardOutput.ReadToEnd()
        $err = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()
        if (-not [string]::IsNullOrEmpty($err)) { Write-Error $err }
        [Console]::Out.Write($out)
        exit $proc.ExitCode
    } catch {
        Write-Error ('Get-GitStatus failed: ' + $_.Exception.Message)
        exit 2
    }
}

switch ($Action) {
    'Read-File'     { Invoke-ReadFile -P $Path }
    'Write-File'    { Invoke-WriteFile -P $Path -C $Content }
    'Test-PathSafe' { Invoke-TestPathSafe -P $Path }
    'Get-GitStatus' { Invoke-GitStatus -R $RepoPath }
}
