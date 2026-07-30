#!/usr/bin/env pwsh
# =============================================================================
#  Moxiang-Reborn | Anti JSON-Truncation Toolbox
#  AGENTS.md §3 F-1 - shell_command arguments JSON 截断的根本对策
#
#  根因 (2026-07-30 session 019f9c8e + 019fb254 死亡现场):
#    MiniMax-M3 生成 shell_command tool_call 时, arguments 字符串经常被截断
#    (不发闭合 " }), CLI 端报 "EOF while parsing a string at column N".
#    触发条件: 多语句 PowerShell / \" 转义 / $() 内插 / here-string / 长 for 循环.
#
#  对策:
#    把所有 "复杂 PowerShell" 抽到这个脚本里. agent 只用单行 shell_command 调用:
#      pwsh -File scripts\no-truncation.ps1 -Action <Name> -<Arg1> ... -<ArgN> ...
#    每个 Action 是单 cmdlet / 单文件 I/O, 不含内嵌脚本.
# =============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Get-FileLines','Read-JsonObject','Test-PathSafe','Format-TestOutput','Grep-Pattern','Count-TestCases','List-TestSuite','Help')]
    [string]$Action,

    [string]$Path,
    [int]$StartLine = 0,
    [int]$EndLine = -1,
    [string]$Pattern,
    [int]$MaxHits = 20,
    [string]$ExePath,
    [string]$Filter = '',
    [string]$Cwd = 'C:\moxiang'
)

# 每个 Action 是一段 < 30 行的 PowerShell, 无 here-string / 无 \" 转义 / 无 for+if 嵌套.

function Invoke-Get-FileLines {
    if (-not (Test-Path -LiteralPath $Path)) { Write-Host "MISSING: $Path"; return }
    $lines = Get-Content -LiteralPath $Path -ErrorAction SilentlyContinue
    if (-not $lines) { return }
    $last = if ($EndLine -lt 0) { $lines.Count } else { [Math]::Min($EndLine, $lines.Count) }
    $first = [Math]::Max(0, $StartLine)
    for ($i = $first; $i -lt $last; $i++) {
        '{0,5}: {1}' -f ($i + 1), $lines[$i]
    }
}

function Invoke-Read-JsonObject {
    if (-not (Test-Path -LiteralPath $Path)) { Write-Host "MISSING: $Path"; return }
    $raw = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
    if (-not $raw) { Write-Host 'EMPTY: $Path'; return }
    try { $raw | ConvertFrom-Json } catch { Write-Host "PARSE-ERROR: $($_.Exception.Message)" }
}

function Invoke-Test-PathSafe {
    if (Test-Path -LiteralPath $Path) {
        $item = Get-Item -LiteralPath $Path -ErrorAction SilentlyContinue
        if ($item.PSIsContainer) { 'DIR' } else { 'FILE' }
    } else { 'MISSING' }
}

function Invoke-Format-TestOutput {
    if (-not (Test-Path -LiteralPath $ExePath)) { Write-Host "MISSING-EXE: $ExePath"; return }
    $gtestArgs = @('--gtest_brief=1', '--gtest_color=no')
    if ($Filter) { $gtestArgs += @('--gtest_filter=' + $Filter) }
    & $ExePath @gtestArgs
    '---exit: $LASTEXITCODE---'
}

function Invoke-Grep-Pattern {
    if (-not (Test-Path -LiteralPath $Path)) { Write-Host "MISSING: $Path"; return }
    $hits = Select-String -LiteralPath $Path -Pattern $Pattern -ErrorAction SilentlyContinue | Select-Object -First $MaxHits
    foreach ($h in $hits) { '{0}: {1}' -f $h.LineNumber, $h.Line }
    '---' + $hits.Count + ' hits (max ' + $MaxHits + ')---'
}

function Invoke-Count-TestCases {
    if (-not (Test-Path -LiteralPath $ExePath)) { Write-Host "MISSING-EXE: $ExePath"; return }
    $out = & $ExePath --gtest_list_tests 2>&1
    $cases = ($out | Where-Object { $_ -match '^  ' }).Count
    $suites = ($out | Where-Object { $_ -notmatch '^  ' -and $_.Trim() -ne '' }).Count
    Write-Host "SUITES=$suites CASES=$cases"
}

function Invoke-List-TestSuite {
    if (-not (Test-Path -LiteralPath $ExePath)) { Write-Host "MISSING-EXE: $ExePath"; return }
    & $ExePath --gtest_list_tests 2>&1 | Select-Object -First 200
}

function Invoke-Help {
    @'

no-truncation.ps1 - 安全 wrapper 集, 避免 shell_command JSON 截断.

用法 (全部是单行调用, 不会触发 MiniMax-M3 JSON 截断):

  pwsh -File scripts\no-truncation.ps1 -Action Get-FileLines -Path C:\moxiang\ROADMAP.md -StartLine 0 -EndLine 50
  pwsh -File scripts\no-truncation.ps1 -Action Read-JsonObject -Path C:\moxiang\.vs\ProjectSettings.json
  pwsh -File scripts\no-truncation.ps1 -Action Test-PathSafe -Path C:\moxiang\modern\build
  pwsh -File scripts\no-truncation.ps1 -Action Grep-Pattern -Path C:\moxiang\modern\src\server\map_handler.cpp -Pattern MapHandler -MaxHits 10
  pwsh -File scripts\no-truncation.ps1 -Action Format-TestOutput -ExePath modern\build\tests\unit\server\Debug\mxh_server_handler_tests.exe -Filter ServerHandler*
  pwsh -File scripts\no-truncation.ps1 -Action Count-TestCases -ExePath modern\build\tests\unit\Debug\mxh_crypto_tests.exe
  pwsh -File scripts\no-truncation.ps1 -Action List-TestSuite -ExePath modern\build\tests\unit\Debug\mxh_crypto_tests.exe

设计原则:
  - 单 cmdlet / 单文件 I/O, 不含内嵌脚本
  - 无 here-string / 无 \" 转义 / 无 for+if 嵌套
  - 任何长输出截断到 -MaxHits (默认 20)

'@
}

switch ($Action) {
    'Get-FileLines'   { Invoke-Get-FileLines }
    'Read-JsonObject' { Invoke-Read-JsonObject }
    'Test-PathSafe'   { Invoke-Test-PathSafe }
    'Format-TestOutput' { Invoke-Format-TestOutput }
    'Grep-Pattern'    { Invoke-Grep-Pattern }
    'Count-TestCases' { Invoke-Count-TestCases }
    'List-TestSuite'  { Invoke-List-TestSuite }
    'Help'            { Invoke-Help }
}

