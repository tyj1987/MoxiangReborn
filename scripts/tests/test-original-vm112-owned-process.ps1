param([string]$RepoRoot = '')
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')) }
$wrapper = Join-Path $RepoRoot 'deploy\scripts\start_original_vm112.ps1'
$tokens = $null
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($wrapper, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw 'Original VM112 wrapper does not parse.' }
$native = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.CommandAst] -and
    $node.GetCommandName() -ceq 'Add-Type'
}, $true))
if ($native.Count -ne 1) { throw 'Expected one production process-image P/Invoke declaration.' }
. ([scriptblock]::Create($native[0].Extent.Text))
$names = @('Assert-OriginalOwnedProcessImage', 'Write-OriginalOwnedPidManifest',
    'Stop-OriginalOwnedProcess')
foreach ($name in $names) {
    $definitions = @($ast.FindAll({ param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name
    }, $true))
    if ($definitions.Count -ne 1) { throw "Expected exactly one owned-process helper: $name" }
    . ([scriptblock]::Create($definitions[0].Extent.Text))
}
$root = Join-Path ([IO.Path]::GetTempPath()) ('moxiang-original-owned-process-' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $root -ErrorAction Stop
$manifest = Join-Path $root 'pids.json'
$childA = $null
$childB = $null
try {
    $rows = @([ordered]@{name='map';pid=101;exe='C:\fixed\map.exe';port=18001;
        startUtcTicks=[int64]123456;exeSha256=('a' * 64)})
    Write-OriginalOwnedPidManifest -Path $manifest -Entries $rows -Initial $true
    $first = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
    if (@($first).Count -ne 1 -or [int64]$first[0].startUtcTicks -ne 123456 -or
        [string]$first[0].exeSha256 -cne ('a' * 64)) { throw 'First owned PID snapshot differs.' }
    $rows += [ordered]@{name='agent';pid=102;exe='C:\fixed\agent.exe';port=17001;
        startUtcTicks=[int64]123457;exeSha256=('b' * 64)}
    Write-OriginalOwnedPidManifest -Path $manifest -Entries $rows -Initial $false
    $second = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
    if (@($second).Count -ne 2 -or [int64]$second[1].startUtcTicks -ne 123457) {
        throw 'Replacement owned PID snapshot differs.'
    }
    $duplicateRejected = $false
    try { Write-OriginalOwnedPidManifest -Path $manifest -Entries $rows -Initial $true }
    catch [IO.IOException] { $duplicateRejected = $true }
    if (-not $duplicateRejected) { throw 'CreateNew allowed an existing PID manifest.' }

    $powershell = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    $childA = Start-Process -FilePath $powershell -ArgumentList '-NoProfile -NonInteractive -Command Start-Sleep -Seconds 30' -WindowStyle Hidden -PassThru
    $null = $childA.Handle
    Assert-OriginalOwnedProcessImage -Process $childA -ExpectedPath $powershell
    $childB = Start-Process -FilePath $powershell -ArgumentList '-NoProfile -NonInteractive -Command Start-Sleep -Seconds 30' -WindowStyle Hidden -PassThru
    $null = $childB.Handle
    Assert-OriginalOwnedProcessImage -Process $childB -ExpectedPath $powershell
    if ($childA.HasExited -or $childB.HasExited) { throw 'Harmless fixture process exited before ownership test.' }
    $wrongPathRejected = $false
    try { Assert-OriginalOwnedProcessImage -Process $childA -ExpectedPath 'C:\wrong\not-owned.exe' }
    catch {
        if ($_.Exception.Message -like '*image path differs*') { $wrongPathRejected = $true }
        else { throw }
    }
    if (-not $wrongPathRejected) { throw 'Wrong owned executable path was not rejected.' }
    Stop-OriginalOwnedProcess -Process $childA
    if (-not $childA.HasExited -or $childB.HasExited) { throw 'Owned handle cleanup affected another process.' }
    Stop-OriginalOwnedProcess -Process $childB
    Stop-OriginalOwnedProcess -Process $childA
    if (-not $childB.HasExited) { throw 'Second owned fixture process did not exit.' }
    [ordered]@{manifestCreateNew=$true;manifestAtomicReplace=$true;
        persistedStartTicksAndExeHash=$true;ownedHandleStoppedOnlyTarget=$true;
        idempotentAlreadyExited=$true;sameHandleX86ImagePath=$true;
        wrongImagePathRejected=$wrongPathRejected;gameServersStarted=$false;databaseTouched=$false} |
        ConvertTo-Json -Compress
} finally {
    foreach ($child in @($childA, $childB)) {
        if ($null -ne $child) {
            if (-not $child.HasExited) { $child.Kill(); $null = $child.WaitForExit(10000) }
            $child.Dispose()
        }
    }
    if (Test-Path -LiteralPath $root -PathType Container) {
        $tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
        $resolvedRoot = [IO.Path]::GetFullPath($root)
        if (-not $resolvedRoot.StartsWith($tempBase, [StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path -Leaf $resolvedRoot) -notmatch '^moxiang-original-owned-process-[0-9a-f]{32}$') {
            throw 'Owned temporary test directory escaped the intended temp root.'
        }
        Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction Stop
    }
}
