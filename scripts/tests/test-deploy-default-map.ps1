# Static invariant for deploy/scripts/start_modern.ps1.
# Not a Pester suite. Run: pwsh -NoProfile -File scripts/tests/test-deploy-default-map.ps1
# The map process --map argument and --default-map must both be $MapNumber.
# This test does not check the parameter default, the VM112 wrappers, or main.cpp.
$ErrorActionPreference = 'Stop'

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
}

function Unwrap-PipelineExpression {
    param($Node)
    $guard = 0
    while ($null -ne $Node -and $guard -lt 12) {
        $guard++
        if ($Node -is [System.Management.Automation.Language.PipelineAst]) {
            if (@($Node.PipelineElements).Count -ne 1) { return $Node }
            $Node = $Node.PipelineElements[0]
            continue
        }
        if ($Node -is [System.Management.Automation.Language.CommandExpressionAst]) {
            $Node = $Node.Expression
            continue
        }
        break
    }
    return $Node
}

function Get-HashtableKeyText {
    param($Pair)
    $key = Unwrap-PipelineExpression $Pair.Item1
    if ($key -is [System.Management.Automation.Language.StringConstantExpressionAst]) {
        return [string]$key.Value
    }
    return [string]$key.Extent.Text
}

function Get-LeadingArrayElements {
    param($Expression)
    $node = Unwrap-PipelineExpression $Expression
    $guard = 0
    while ($node -is [System.Management.Automation.Language.BinaryExpressionAst] -and
        [string]$node.Operator -eq 'Plus' -and $guard -lt 8) {
        $node = Unwrap-PipelineExpression $node.Left
        $guard++
    }
    if ($node -isnot [System.Management.Automation.Language.ArrayExpressionAst]) {
        return @()
    }
    $statements = @($node.SubExpression.Statements)
    if ($statements.Count -ne 1) { return @() }
    $literal = Unwrap-PipelineExpression $statements[0]
    if ($literal -isnot [System.Management.Automation.Language.ArrayLiteralAst]) {
        return @()
    }
    return @($literal.Elements | ForEach-Object { Unwrap-PipelineExpression $_ })
}

function Get-FlagVariableName {
    param($Elements, [string]$Flag, [string]$Label)
    $names = New-Object System.Collections.Generic.List[string]
    for ($index = 0; $index -lt $Elements.Count; $index++) {
        $element = $Elements[$index]
        $isFlag = $element -is [System.Management.Automation.Language.StringConstantExpressionAst] -and
            [string]$element.Value -eq $Flag
        if (-not $isFlag) { continue }
        if ($index + 1 -ge $Elements.Count) {
            throw "FAIL: $Label $Flag is missing a value"
        }
        $value = $Elements[$index + 1]
        if ($value -isnot [System.Management.Automation.Language.VariableExpressionAst] -or
            [string]$value.Extent.Text -ne '$MapNumber') {
            throw "FAIL: $Label $Flag uses $($value.Extent.Text)"
        }
        $names.Add([string]$value.VariablePath.UserPath)
    }
    if ($names.Count -eq 0) {
        throw "FAIL: $Label is missing $Flag"
    }
    if ($names.Count -ne 1) {
        throw "FAIL: $Label has more than one $Flag"
    }
    if ($names[0] -ne 'MapNumber') {
        throw "FAIL: $Label $Flag uses `$$($names[0])"
    }
    return $names[0]
}

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$launcher = Join-Path $repoRoot 'deploy\scripts\start_modern.ps1'
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($launcher, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and @($parseErrors).Count -gt 0) {
    throw "FAIL: parse error in ${launcher}: $($parseErrors[0])"
}

$mapArgumentNames = New-Object System.Collections.Generic.List[string]
$defaultMapArgumentNames = New-Object System.Collections.Generic.List[string]
$tables = @($ast.FindAll({
    param($node)
    $node -is [System.Management.Automation.Language.HashtableAst]
}, $true))
foreach ($table in $tables) {
    $argsPair = $null
    $nameValue = $null
    foreach ($pair in $table.KeyValuePairs) {
        $key = Get-HashtableKeyText $pair
        if ($key -eq 'args') { $argsPair = $pair }
        if ($key -eq 'name') {
            $nameNode = Unwrap-PipelineExpression $pair.Item2
            if ($nameNode -is [System.Management.Automation.Language.StringConstantExpressionAst]) {
                $nameValue = [string]$nameNode.Value
            }
        }
    }
    if ($null -eq $argsPair) { continue }
    $elements = @(Get-LeadingArrayElements $argsPair.Item2)
    if ($nameValue -eq 'map') {
        $mapArgumentNames.Add((Get-FlagVariableName $elements '--map' 'map process'))
    }
    foreach ($element in $elements) {
        $isDefaultMap = $element -is [System.Management.Automation.Language.StringConstantExpressionAst] -and
            [string]$element.Value -eq '--default-map'
        if ($isDefaultMap) {
            $defaultMapArgumentNames.Add((Get-FlagVariableName $elements '--default-map' 'start_modern.ps1'))
            break
        }
    }
}

Assert-True ($mapArgumentNames.Count -eq 1) "expected one map process --map, found $($mapArgumentNames.Count)"
Assert-True ($defaultMapArgumentNames.Count -eq 1) "expected one --default-map, found $($defaultMapArgumentNames.Count)"
Assert-True ($mapArgumentNames[0] -eq 'MapNumber' -and $defaultMapArgumentNames[0] -eq 'MapNumber') (
    "map --map and --default-map must both be `$MapNumber"
)
Write-Output "DEPLOY_DEFAULT_MAP PASS (`$MapNumber)"
