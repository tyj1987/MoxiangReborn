# Static parse of the three deploy launchers' --default-map value.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-deploy-default-map.ps1
# Requires PowerShell 7.4 or newer. Runs on Linux and Windows.
#
# Get-DeployScriptDefaultMap is the script-side extractor. A later check can
# compare its result with modern/tools/MoxianAgentServer/main.cpp. This test
# does not read main.cpp.
$ErrorActionPreference = 'Stop'

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
}

function Get-DeployScriptDefaultMap {
    param([Parameter(Mandatory)][string]$Path)

    $tokens = $null
    $parseErrors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile(
        $Path, [ref]$tokens, [ref]$parseErrors)
    if ($parseErrors -and @($parseErrors).Count -gt 0) {
        throw "FAIL: parse error in ${Path}: $($parseErrors[0])"
    }

    $mapNumberDefault = $null
    if ($null -eq $ast.ParamBlock) {
        throw "FAIL: $Path has no param block for MapNumber"
    }
    $parameters = @($ast.ParamBlock.Parameters | Where-Object {
        $_.Name.VariablePath.UserPath -eq 'MapNumber'
    })
    if ($parameters.Count -ne 1) {
        throw "FAIL: $Path must declare one MapNumber parameter"
    }
    $defaultValue = $parameters[0].DefaultValue
    if ($null -eq $defaultValue -or
        $defaultValue -isnot [System.Management.Automation.Language.ConstantExpressionAst] -or
        [string]$defaultValue.Extent.Text -notmatch '^[0-9]+$') {
        throw "FAIL: $Path MapNumber default is not an integer constant"
    }
    $mapNumberDefault = [int]$defaultValue.Value

    $values = New-Object System.Collections.Generic.List[int]
    for ($index = 0; $index -lt $tokens.Count; $index++) {
        $token = $tokens[$index]
        $tokenKind = $token.Kind.ToString()
        $isDefaultMapFlag = $tokenKind -in @('StringLiteral', 'String', 'StringExpandable') -and
            [string]$token.Value -eq '--default-map'
        if (-not $isDefaultMapFlag) { continue }

        $nextIndex = $index + 1
        while ($nextIndex -lt $tokens.Count) {
            $kind = $tokens[$nextIndex].Kind.ToString()
            if ($kind -notin @('Comma', 'NewLine', 'LineContinuation', 'Comment')) { break }
            $nextIndex++
        }
        if ($nextIndex -ge $tokens.Count) {
            throw "FAIL: $Path has --default-map without a value"
        }
        $next = $tokens[$nextIndex]
        $nextKind = $next.Kind.ToString()
        if ($nextKind -eq 'Variable') {
            if ([string]$next.Name -ne 'MapNumber') {
                throw "FAIL: $Path --default-map uses `$$($next.Name)"
            }
            $values.Add($mapNumberDefault)
        } elseif ($nextKind -eq 'Number' -and [string]$next.Text -match '^[0-9]+$') {
            $values.Add([int]$next.Text)
        } elseif ($nextKind -in @('StringLiteral', 'String', 'StringExpandable') -and [string]$next.Value -match '^[0-9]+$') {
            $values.Add([int]$next.Value)
        } else {
            throw "FAIL: $Path --default-map value is $($next.Extent.Text)"
        }
    }

    if ($values.Count -lt 1) {
        throw "FAIL: $Path does not pass --default-map"
    }
    $distinct = @($values | Select-Object -Unique)
    if ($distinct.Count -ne 1) {
        throw "FAIL: $Path --default-map values disagree ($($values -join ', '))"
    }
    return [int]$distinct[0]
}

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$launchers = @(
    'deploy\scripts\start_modern.ps1',
    'deploy\scripts\start_original_vm112.ps1',
    'deploy\scripts\start_current_vm112.ps1'
)
$found = @()
foreach ($relative in $launchers) {
    $path = Join-Path $repoRoot $relative
    $value = Get-DeployScriptDefaultMap -Path $path
    $found += [pscustomobject]@{ Script = ($relative -replace '\\', '/'); DefaultMap = $value }
    Assert-True ($value -eq 17) "$relative passes --default-map $value"
}
$agreed = @($found.DefaultMap | Select-Object -Unique)
Assert-True ($agreed.Count -eq 1) (
    "deploy scripts disagree on --default-map: " +
    (($found | ForEach-Object { "$($_.Script)=$($_.DefaultMap)" }) -join ', ')
)
Write-Output "DEPLOY_DEFAULT_MAP PASS ($($agreed[0]))"
