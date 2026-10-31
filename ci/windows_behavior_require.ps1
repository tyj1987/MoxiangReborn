# Shared reader and require_passed checker for ci/windows_behavior_tests.txt.
# Dot-source this file. It does not run tests on its own.
# ctest -R and require_passed both use Get-MxhBehaviorEntries, so one new
# list line joins both. A gtest "[       OK ]" line matches the entry when
# its name is the entry or <Prefix>/<entry>/<idx> (TEST_P).

function Parse-MxhBehaviorEntryLine {
    param([Parameter(Mandatory)][string]$Line)
    if ($Line -notmatch '^(\S+)(?:\s+ok=(\d+))?$') {
        throw "bad behavior test entry: $Line"
    }
    $expected = 1
    if ($Matches[2]) { $expected = [int]$Matches[2] }
    if ($expected -lt 1) { throw "ok count must be >= 1: $Line" }
    return [pscustomobject]@{
        Name       = $Matches[1]
        ExpectedOk = $expected
    }
}

function Get-MxhBehaviorEntries {
    param([Parameter(Mandatory)][string]$ListPath)
    $entries = New-Object System.Collections.Generic.List[object]
    foreach ($raw in [System.IO.File]::ReadLines($ListPath)) {
        $line = $raw.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith('#')) { continue }
        $hash = $line.IndexOf('#')
        if ($hash -ge 0) { $line = $line.Substring(0, $hash).Trim() }
        if ($line.Length -eq 0) { continue }
        $entries.Add((Parse-MxhBehaviorEntryLine -Line $line))
    }
    return $entries
}

function Test-MxhBehaviorOkName {
    param(
        [Parameter(Mandatory)][string]$OkName,
        [Parameter(Mandatory)][string]$EntryName
    )
    if ($OkName -eq $EntryName) { return $true }
    $pattern = '^[^/]+/' + [regex]::Escape($EntryName) + '/\d+$'
    return $OkName -match $pattern
}

function Split-MxhCTestSections {
    param([AllowEmptyString()][string]$LogText)
    $sections = @{}
    if ([string]::IsNullOrEmpty($LogText)) { return $sections }
    $currentName = $null
    $current = New-Object System.Collections.Generic.List[string]
    foreach ($raw in ($LogText -split "`r?`n")) {
        $line = $raw.TrimEnd("`r")
        if ($line -match '^\s*Start\s+\d+:\s+(\S+)\s*$') {
            $currentName = $Matches[1]
            $current = New-Object System.Collections.Generic.List[string]
            $current.Add($line)
            continue
        }
        if ($null -ne $currentName) { $current.Add($line) }
        if ($line -match '^\s*\d+/\d+\s+Test\s+#\d+:\s+(\S+)\s+\.+') {
            $resultName = $Matches[1]
            $status = 'unknown'
            if ($line -match '(?<![\w])(Failed|Skipped|Not Run|Passed)(\s|$)') {
                $status = $Matches[1]
            }
            $body = New-Object System.Collections.Generic.List[string]
            if ($currentName -eq $resultName) {
                foreach ($item in $current) { $body.Add($item) }
            } else {
                $body.Add($line)
            }
            $sections[$resultName] = @{
                Status = $status
                Lines  = $body
            }
            $currentName = $null
            $current = New-Object System.Collections.Generic.List[string]
        }
    }
    return $sections
}

function Invoke-MxhBehaviorRequire {
    param(
        [AllowEmptyString()][string]$LogText,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Entries
    )
    $sections = Split-MxhCTestSections -LogText $LogText
    $lines = New-Object System.Collections.Generic.List[string]
    $passedCount = 0
    $notPassedCount = 0
    if ($Entries.Count -lt 1) {
        $lines.Add('REQUIRED NOT PASSED (list empty) ok=0/0 reason=missing')
        $notPassedCount = 1
    }
    foreach ($entry in $Entries) {
        $name = $entry.Name
        $expected = [int]$entry.ExpectedOk
        $found = 0
        $reasons = New-Object System.Collections.Generic.List[string]
        $section = $null
        if ($sections.ContainsKey($name)) { $section = $sections[$name] }
        if ($null -eq $section) {
            $reasons.Add('missing')
        } else {
            $status = [string]$section.Status
            if ($status -ne 'Passed') {
                $reasons.Add('ctest-not-passed')
            }
            if ($status -eq 'Skipped' -or $status -eq 'Not Run') {
                $reasons.Add('skipped')
            }
            $bodyText = ($section.Lines -join "`n")
            if ($bodyText.Contains('[  PASSED  ] 0 tests')) {
                $reasons.Add('zero-tests')
            }
            if ($bodyText -match '(?<![0-9])0 tests from') {
                if (-not $reasons.Contains('zero-tests')) { $reasons.Add('zero-tests') }
            }
            if ($bodyText.Contains('[  SKIPPED ]')) {
                if (-not $reasons.Contains('skipped')) { $reasons.Add('skipped') }
            }
            if ($bodyText.Contains('DISABLED_')) {
                $reasons.Add('disabled')
            }
            foreach ($bodyLine in $section.Lines) {
                $stripped = $bodyLine
                if ($stripped -match '^\d+:\s?(.*)$') { $stripped = $Matches[1] }
                if ($stripped -match '^\[       OK \] (\S+)') {
                    $okName = $Matches[1]
                    if (Test-MxhBehaviorOkName -OkName $okName -EntryName $name) {
                        $found++
                        $lines.Add("REQUIRED OK $name $stripped")
                    }
                }
            }
            if ($found -lt $expected) { $reasons.Add('ok-count') }
        }
        if ($reasons.Count -eq 0) {
            $lines.Add("REQUIRED PASSED $name ok=$found/$expected")
            $passedCount++
        } else {
            $reason = ($reasons -join ',')
            $lines.Add("REQUIRED NOT PASSED $name ok=$found/$expected reason=$reason")
            $notPassedCount++
        }
    }
    $lines.Add("REQUIRED SUMMARY passed=$passedCount not_passed=$notPassedCount")
    return [pscustomobject]@{
        ExitCode       = $(if ($notPassedCount -eq 0) { 0 } else { 1 })
        Lines          = $lines
        PassedCount    = $passedCount
        NotPassedCount = $notPassedCount
    }
}

function Invoke-MxhBehaviorRequireSelfTest {
    function New-CaseLog {
        param([string[]]$Body)
        return ($Body -join "`n")
    }
    $cases = @(
        [pscustomobject]@{
            Name       = 'test-p-partial'
            ExpectPass = $false
            Entry      = 'LoginHandlerProtocolGate.Foo ok=2'
            Log        = New-CaseLog @(
                '    Start 1: LoginHandlerProtocolGate.Foo',
                '1: [       OK ] Gate/LoginHandlerProtocolGate.Foo/0 (1 ms)',
                '1: [  PASSED  ] 1 test.',
                '1/1 Test #1: LoginHandlerProtocolGate.Foo .....   Passed    0.01 sec'
            )
        },
        [pscustomobject]@{
            Name       = 'zero-tests'
            ExpectPass = $false
            Entry      = 'LoginHandlerProtocolGate.Foo ok=1'
            Log        = New-CaseLog @(
                '    Start 2: LoginHandlerProtocolGate.Foo',
                '2: [==========] Running 0 tests from 0 test suites.',
                '2: [==========] 0 tests from 0 test suites ran. (0 ms total)',
                '2: [  PASSED  ] 0 tests.',
                '1/1 Test #2: LoginHandlerProtocolGate.Foo .....   Passed    0.01 sec'
            )
        },
        [pscustomobject]@{
            Name       = 'skipped'
            ExpectPass = $false
            Entry      = 'LoginHandlerProtocolGate.Foo'
            Log        = New-CaseLog @(
                '    Start 3: LoginHandlerProtocolGate.Foo',
                '3: [  SKIPPED ] LoginHandlerProtocolGate.Foo (0 ms)',
                '1/1 Test #3: LoginHandlerProtocolGate.Foo .....   Passed    0.01 sec'
            )
        },
        [pscustomobject]@{
            Name       = 'full-match'
            ExpectPass = $true
            Entry      = 'LoginHandlerProtocolGate.Foo ok=2'
            Log        = New-CaseLog @(
                '    Start 4: LoginHandlerProtocolGate.Foo',
                '4: [       OK ] Gate/LoginHandlerProtocolGate.Foo/0 (1 ms)',
                '4: [       OK ] Gate/LoginHandlerProtocolGate.Foo/1 (2 ms)',
                '4: [  PASSED  ] 2 tests.',
                '1/1 Test #4: LoginHandlerProtocolGate.Foo .....   Passed    0.02 sec'
            )
        }
    )
    $failed = $false
    foreach ($case in $cases) {
        $entry = Parse-MxhBehaviorEntryLine -Line $case.Entry
        $result = Invoke-MxhBehaviorRequire -LogText $case.Log -Entries @($entry)
        foreach ($outLine in $result.Lines) { Write-Host $outLine }
        $actualPass = $result.ExitCode -eq 0
        $expectText = if ($case.ExpectPass) { 'pass' } else { 'fail' }
        if ($actualPass -eq $case.ExpectPass) {
            Write-Host "REQUIRE_CASE $($case.Name) expect=$expectText OK"
        } else {
            Write-Host "REQUIRE_CASE $($case.Name) expect=$expectText FAIL"
            $failed = $true
        }
    }
    if ($failed) { throw 'behavior require self-test failed' }
}
