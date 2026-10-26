#requires -Version 5.1
<#
.SYNOPSIS
    ROG trial-merge gate: clean stale build trees, then build and test.

.DESCRIPTION
    Verifies the trial-merge SHA, refuses unsafe build paths, and checks free
    space before deleting anything. An existing build directory is deleted and
    reconfigured when rog-gate.sha is missing or differs, or when CMakeCache.txt
    was generated with a different CMake generator.

    Before any git operation, build, or delete, a sparse checkout that also
    contains a junction or symlink is refused. Sparse alone, or a reparse point
    alone, is allowed. There is no override.

    The host toolchain is vcvarsall.bat x86 (Hostx86/x86). The default generator
    is "NMake Makefiles", matching existing D:\build-mxr gate directories.
    "Ninja" is optional. This script does not change build-modern.bat / .ps1.

.PARAMETER SourceDir
    Git worktree of the trial merge. Its drive is included in the free-space check.

.PARAMETER BuildDir
    CMake build directory. The resolved path must sit strictly under -AllowedBuildRoot.

.PARAMETER ExpectedSha
    Full 40-hex-digit trial-merge commit. Compared to git rev-parse HEAD.

.PARAMETER Config
    CMAKE_BUILD_TYPE and ctest -C. Default Debug.

.PARAMETER Generator
    CMake -G value. Default "NMake Makefiles". "Ninja" is the other accepted value.

.PARAMETER AllowedBuildRoot
    Parent that BuildDir must be strictly inside. Default D:\build-mxr.

.PARAMETER Target
    Optional cmake --build target names. Comma-separated values are split.

.PARAMETER CtestArgs
    Extra arguments appended to ctest after --output-on-failure.

.PARAMETER SkipTests
    Build and write the stamp, but do not run CTest.

.PARAMETER SkipE2E
    When MoxianClientE2E is registered without the mxh_client_e2e_ctest port
    launcher, warn and continue instead of failing. A registered launcher
    still runs. The main CTest pass still excludes MoxianClientE2E.

.PARAMETER DryRun
    Run the read-only checks and print the actions. Alias of -WhatIf for this script.

.PARAMETER MinFreeGB
    Minimum free gibibytes (1024^3) required on every checked drive. Default 10.
    10 GB is a hard floor; a lower value is rejected.

.PARAMETER DriveMinFreeGB
    Per-drive overrides, for example E=20 or E=20,D=15. Drives not listed use -MinFreeGB.
    Each override must be at least 10 GB. Overrides can only raise the floor.

.PARAMETER SelfTest
    Check path safety, generator decisions, drive selection, free-space
    thresholds, and the sparse-checkout reparse preflight without a compiler.
    On Windows, directory links in this self-test are NTFS junctions.

.PARAMETER SelfTestBuildExit
    Self-test only. When this parameter is present, skip the build and exit
    with the same code a failed build command would return.
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$SourceDir,
    [string]$BuildDir,
    [string]$ExpectedSha,
    [string]$Config = 'Debug',
    [ValidateSet('NMake Makefiles', 'Ninja')]
    [string]$Generator = 'NMake Makefiles',
    [string]$AllowedBuildRoot = 'D:\build-mxr',
    [string[]]$Target = @(),
    [string[]]$CtestArgs = @(),
    [switch]$SkipTests,
    [switch]$SkipE2E,
    [switch]$DryRun,
    [double]$MinFreeGB = 10,
    [string[]]$DriveMinFreeGB = @(),
    [switch]$SelfTest,
    [int]$SelfTestBuildExit = 0
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$script:RogGateVcVarsAll = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat'
$script:RogGateNinja = 'C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$script:RogGateForbiddenRoot = 'D:\MX'
$script:RogGateStampName = 'rog-gate.sha'
$script:RogGateSparsePreflightChecked = ''
$script:RogGateSawError = $false
# Win32 FILE_ATTRIBUTE_* bits. Do not convert raw values to
# [System.IO.FileAttributes]: Windows PowerShell 5.1 rejects unknown bits
# such as 0x80000, and -band against that enum re-parses the integer.
$script:RogGateAttributeReadOnly = [int64]0x1
$script:RogGateAttributeDirectory = [int64]0x10
$script:RogGateAttributeReparsePoint = [int64]0x400
# mxh_client_e2e_ctest reads this and execs mxh_client_e2e with
# --login-port, --agent-port, --map-port, and --map12-port. This gate sets
# the base and does not append those flags.
$script:RogGateE2EPortBaseText = '26001'
$script:RogGateE2ETestRegex = '^MoxianClientE2E$'
$script:RogGateE2ELauncherName = 'mxh_client_e2e_ctest'

function ConvertTo-RogGateWindowsPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw 'Path is empty.'
    }

    $normalized = $Path.Trim().Replace('/', '\')
    if ($normalized.StartsWith('\\?\')) {
        $normalized = $normalized.Substring(4)
    }
    if ($normalized.StartsWith('\\')) {
        throw "UNC paths are not allowed: $Path"
    }
    if ($normalized -notmatch '^[A-Za-z]:\\' -and $normalized -notmatch '^[A-Za-z]:$') {
        throw "Path must be a drive-absolute Windows path: $Path"
    }
    if ($normalized -match '^[A-Za-z]:$') {
        $normalized = $normalized + '\'
    }

    $drive = $normalized.Substring(0, 2).ToUpperInvariant()
    $rest = $normalized.Substring(2).TrimStart('\')
    $parts = New-Object 'System.Collections.Generic.List[string]'
    if (-not [string]::IsNullOrEmpty($rest)) {
        foreach ($segment in $rest.Split('\')) {
            if ($segment -eq '' -or $segment -eq '.') {
                continue
            }
            if ($segment -eq '..') {
                if ($parts.Count -eq 0) {
                    throw "Path escapes its drive root: $Path"
                }
                $parts.RemoveAt($parts.Count - 1)
                continue
            }
            if ($segment -match '[\u0000-\u001F<>:"|?*&!^%]') {
                throw "Unsupported path character in: $Path"
            }
            $parts.Add($segment)
        }
    }

    if ($parts.Count -eq 0) {
        return ($drive + '\')
    }
    return ($drive + '\' + ($parts -join '\'))
}

function Test-RogGateStrictlyUnder {
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Root
    )

    $rootCmp = $Root.TrimEnd('\')
    $candidateCmp = $Candidate.TrimEnd('\')
    if ($rootCmp.Length -eq 2 -and $rootCmp[1] -eq ':') {
        return $false
    }
    $prefix = $rootCmp + '\'
    return $candidateCmp.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Test-RogGateIsOrUnder {
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Root
    )

    $rootCmp = $Root.TrimEnd('\')
    $candidateCmp = $Candidate.TrimEnd('\')
    if ($candidateCmp.Equals($rootCmp, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    $prefix = $rootCmp + '\'
    return $candidateCmp.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function New-RogGateSafetyResult {
    param(
        [bool]$Safe,
        [string]$Reason,
        [string]$NormalizedBuildDir,
        [string]$NormalizedAllowedRoot
    )

    return [pscustomobject]@{
        Safe                  = $Safe
        Reason                = $Reason
        NormalizedBuildDir    = $NormalizedBuildDir
        NormalizedAllowedRoot = $NormalizedAllowedRoot
        ReparsePoints         = @()
    }
}

function Get-RogGateLexicalSafety {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$AllowedRoot,
        [string]$ForbiddenRoot = 'D:\MX'
    )

    $normBuild = ConvertTo-RogGateWindowsPath -Path $BuildDir
    $normRoot = ConvertTo-RogGateWindowsPath -Path $AllowedRoot
    $normForbid = ConvertTo-RogGateWindowsPath -Path $ForbiddenRoot

    if ($normRoot.TrimEnd('\').Length -eq 2) {
        return New-RogGateSafetyResult -Safe $false -Reason "Allowed build root must not be a drive root: $normRoot" -NormalizedBuildDir $normBuild -NormalizedAllowedRoot $normRoot
    }
    if (Test-RogGateIsOrUnder -Candidate $normRoot -Root $normForbid) {
        return New-RogGateSafetyResult -Safe $false -Reason "Allowed build root is inside forbidden path ${normForbid}: $normRoot" -NormalizedBuildDir $normBuild -NormalizedAllowedRoot $normRoot
    }
    if (Test-RogGateIsOrUnder -Candidate $normBuild -Root $normForbid) {
        return New-RogGateSafetyResult -Safe $false -Reason "BuildDir is inside forbidden path ${normForbid}: $normBuild" -NormalizedBuildDir $normBuild -NormalizedAllowedRoot $normRoot
    }
    if (-not (Test-RogGateStrictlyUnder -Candidate $normBuild -Root $normRoot)) {
        return New-RogGateSafetyResult -Safe $false -Reason "BuildDir must be strictly under allowed root ${normRoot} (not the root itself): $normBuild" -NormalizedBuildDir $normBuild -NormalizedAllowedRoot $normRoot
    }
    return New-RogGateSafetyResult -Safe $true -Reason '' -NormalizedBuildDir $normBuild -NormalizedAllowedRoot $normRoot
}

function Get-RogGateAttributeValue {
    param($Attributes)

    if ($null -eq $Attributes) {
        return [int64]0
    }
    # Enum instances expose the underlying integer as value__. Reading that
    # field does not re-parse the value as FileAttributes.
    $type = $Attributes.GetType()
    if ($type.IsEnum) {
        $valueProp = $Attributes.PSObject.Properties['value__']
        if ($null -ne $valueProp -and $null -ne $valueProp.Value) {
            return [int64]$valueProp.Value
        }
    }
    return [int64]$Attributes
}

function Test-RogGateAttributeBit {
    param(
        $Attributes,
        [Parameter(Mandatory = $true)][int64]$Bit
    )

    $value = Get-RogGateAttributeValue -Attributes $Attributes
    return (($value -band $Bit) -ne 0)
}

function Test-RogGateAttributeHasDirectory {
    param($Attributes)
    return Test-RogGateAttributeBit -Attributes $Attributes -Bit $script:RogGateAttributeDirectory
}

function Set-RogGateRawAttributes {
    param(
        [Parameter(Mandatory = $true)][string]$LiteralPath,
        [Parameter(Mandatory = $true)][int64]$Value
    )

    $bits = [uint32]($Value -band [uint32]::MaxValue)
    if (Test-RogGateHostIsWindows) {
        if (-not ('RogGateNativeAttributes' -as [type])) {
            Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class RogGateNativeAttributes {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool SetFileAttributes(string lpFileName, uint dwFileAttributes);
}
'@ | Out-Null
        }
        $wrote = [RogGateNativeAttributes]::SetFileAttributes($LiteralPath, $bits)
        if (-not $wrote) {
            throw "SetFileAttributes failed for ${LiteralPath}."
        }
        return
    }

    # Non-Windows hosts have no kernel32. Enum.ToObject boxes the integer
    # without PowerShell's validating FileAttributes cast.
    $boxed = [Enum]::ToObject([System.IO.FileAttributes], [int64]$Value)
    [System.IO.File]::SetAttributes($LiteralPath, $boxed)
}

function Test-RogGateItemReparsePoint {
    param($Item)

    if ($null -eq $Item) {
        return $false
    }
    $attrProp = $Item.PSObject.Properties['Attributes']
    if ($null -ne $attrProp -and $null -ne $attrProp.Value) {
        if (Test-RogGateAttributeBit -Attributes $attrProp.Value -Bit $script:RogGateAttributeReparsePoint) {
            return $true
        }
    }
    $linkProp = $Item.PSObject.Properties['LinkType']
    if ($null -ne $linkProp -and -not [string]::IsNullOrEmpty([string]$linkProp.Value)) {
        return $true
    }
    return $false
}

function Test-RogGateLiteralReparsePoint {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    try {
        $item = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    } catch {
        return $false
    }
    return (Test-RogGateItemReparsePoint -Item $item)
}

function Send-RogGateCollection {
    param([AllowEmptyCollection()][object[]]$Items)

    # Emit one pipeline object per item. Callers wrap with @() so 0, 1, and
    # many results all stay arrays. Returning the array object itself makes
    # @() wrap it again.
    if ($null -eq $Items) {
        return
    }
    foreach ($item in $Items) {
        Write-Output $item
    }
}

function Get-RogGateLexicalParentPath {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $trimmed = $LiteralPath.TrimEnd('\', '/')
    if ([string]::IsNullOrEmpty($trimmed)) {
        return $null
    }
    $index = $trimmed.LastIndexOfAny([char[]]@('\', '/'))
    if ($index -lt 0) {
        return $null
    }
    # Keep the drive root ("D:\") as its own path instead of collapsing to "D:".
    if ($index -eq 2 -and $trimmed[1] -eq ':') {
        return $trimmed.Substring(0, 3)
    }
    if ($index -eq 0) {
        return $trimmed.Substring(0, 1)
    }
    return $trimmed.Substring(0, $index)
}

function Get-RogGateReparseViolations {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $found = New-Object 'System.Collections.Generic.List[string]'
    $ancestorHit = $false
    $current = $LiteralPath
    $seen = @{}
    while (-not [string]::IsNullOrEmpty($current)) {
        $seenKey = $current.ToLowerInvariant()
        if ($seen.ContainsKey($seenKey)) {
            break
        }
        $seen[$seenKey] = $true
        if (Test-RogGateLiteralReparsePoint -LiteralPath $current) {
            $found.Add($current)
            $ancestorHit = $true
        }
        $parent = Get-RogGateLexicalParentPath -LiteralPath $current
        if ([string]::IsNullOrEmpty($parent) -or $parent -eq $current) {
            break
        }
        $current = $parent
    }

    if ($ancestorHit) {
        Send-RogGateCollection -Items $found.ToArray()
        return
    }

    $leaf = $null
    try {
        $leaf = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    } catch {
        $leaf = $null
    }
    if ($null -eq $leaf) {
        Send-RogGateCollection -Items $found.ToArray()
        return
    }
    $leafIsDir = (Test-RogGateAttributeHasDirectory -Attributes $leaf.Attributes)
    if (-not $leafIsDir) {
        Send-RogGateCollection -Items $found.ToArray()
        return
    }

    $stack = New-Object 'System.Collections.Generic.Stack[string]'
    $stack.Push($leaf.FullName)
    while ($stack.Count -gt 0) {
        $dirPath = $stack.Pop()
        $dirInfo = New-Object System.IO.DirectoryInfo $dirPath
        foreach ($entry in $dirInfo.EnumerateFileSystemInfos()) {
            if (Test-RogGateItemReparsePoint -Item $entry) {
                $found.Add($entry.FullName)
                continue
            }
            $entryIsDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
            if ($entryIsDir) {
                $stack.Push($entry.FullName)
            }
        }
    }
    Send-RogGateCollection -Items $found.ToArray()
        return
}

function Get-RogGateDeletionSafety {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$AllowedRoot,
        [string]$ForbiddenRoot = 'D:\MX'
    )

    $lexical = Get-RogGateLexicalSafety -BuildDir $BuildDir -AllowedRoot $AllowedRoot -ForbiddenRoot $ForbiddenRoot
    if (-not $lexical.Safe) {
        return $lexical
    }

    $reparses = @(Get-RogGateReparseViolations -LiteralPath $lexical.NormalizedBuildDir)
    if ($reparses.Count -gt 0) {
        $lexical.Safe = $false
        $lexical.Reason = 'Reparse point, junction, or symlink in BuildDir or an ancestor: ' + ($reparses -join '; ')
        $lexical.ReparsePoints = $reparses
    }
    return $lexical
}

function Assert-RogGateMayDelete {
    param($Safety)

    if ($null -eq $Safety -or -not $Safety.Safe) {
        $reason = 'path was not checked'
        if ($null -ne $Safety) {
            $reason = [string]$Safety.Reason
        }
        throw "Refusing to delete or build: $reason"
    }
    $forbidden = ConvertTo-RogGateWindowsPath -Path $script:RogGateForbiddenRoot
    if (-not (Test-RogGateStrictlyUnder -Candidate $Safety.NormalizedBuildDir -Root $Safety.NormalizedAllowedRoot)) {
        throw 'Refusing to delete: BuildDir is not strictly under the allowed root.'
    }
    if (Test-RogGateIsOrUnder -Candidate $Safety.NormalizedBuildDir -Root $forbidden) {
        throw 'Refusing to delete: BuildDir touches D:\MX.'
    }
    if (Test-RogGateIsOrUnder -Candidate $Safety.NormalizedAllowedRoot -Root $forbidden) {
        throw 'Refusing to delete: allowed build root touches D:\MX.'
    }
}

function Clear-RogGateReadOnlyAttribute {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $attrs = Get-RogGateAttributeValue -Attributes ([System.IO.File]::GetAttributes($LiteralPath))
    if (-not (Test-RogGateAttributeBit -Attributes $attrs -Bit $script:RogGateAttributeReadOnly)) {
        return
    }
    $cleared = $attrs -band (-bnot $script:RogGateAttributeReadOnly)
    $null = Set-RogGateRawAttributes -LiteralPath $LiteralPath -Value $cleared
}

# Remove-Item -Recurse and Directory.Delete(recursive) follow junctions.
# Walk with EnumerateFileSystemInfos and refuse a reparse point instead of entering it.
function Remove-RogGateDirectoryNoFollow {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    if (Test-RogGateLiteralReparsePoint -LiteralPath $LiteralPath) {
        throw "Refusing to delete reparse point: $LiteralPath"
    }
    if (-not (Test-Path -LiteralPath $LiteralPath)) {
        return
    }

    $info = Get-Item -LiteralPath $LiteralPath -Force
    $isDir = (Test-RogGateAttributeHasDirectory -Attributes $info.Attributes)
    if (-not $isDir) {
        throw "Refusing to delete because BuildDir is not a directory: $LiteralPath"
    }

    $dirInfo = New-Object System.IO.DirectoryInfo $info.FullName
    $children = @($dirInfo.EnumerateFileSystemInfos())
    foreach ($entry in $children) {
        if (Test-RogGateItemReparsePoint -Item $entry) {
            throw "Refusing to delete through reparse point: $($entry.FullName)"
        }
        $entryIsDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
        if ($entryIsDir) {
            $null = Remove-RogGateDirectoryNoFollow -LiteralPath $entry.FullName
        } else {
            $null = Clear-RogGateReadOnlyAttribute -LiteralPath $entry.FullName
            [System.IO.File]::Delete($entry.FullName)
        }
    }
    $null = Clear-RogGateReadOnlyAttribute -LiteralPath $info.FullName
    [System.IO.Directory]::Delete($info.FullName)
}

function Get-RogGateDriveLetter {
    param([Parameter(Mandatory = $true)][string]$NormalizedWindowsPath)

    if ($NormalizedWindowsPath -notmatch '^([A-Za-z]):\\') {
        throw "Cannot determine drive letter for path: $NormalizedWindowsPath"
    }
    return $Matches[1].ToUpperInvariant()
}

function Get-RogGateDrivesToCheck {
    param(
        [Parameter(Mandatory = $true)][string]$NormalizedBuildDir,
        [Parameter(Mandatory = $true)][string]$NormalizedSourceDir
    )

    $ordered = New-Object 'System.Collections.Generic.List[string]'
    $seen = @{}
    $letters = @(
        (Get-RogGateDriveLetter -NormalizedWindowsPath $NormalizedBuildDir),
        (Get-RogGateDriveLetter -NormalizedWindowsPath $NormalizedSourceDir),
        'E'
    )
    foreach ($letter in $letters) {
        if (-not $seen.ContainsKey($letter)) {
            $seen[$letter] = $true
            $ordered.Add($letter)
        }
    }
    Send-RogGateCollection -Items $ordered.ToArray()
    return
}

function ConvertTo-RogGateDriveOverrides {
    param([string[]]$Entries)

    $map = @{}
    if ($null -eq $Entries) {
        return $map
    }
    foreach ($entry in $Entries) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        foreach ($part in ($entry -split ',')) {
            $token = $part.Trim()
            if (-not $token) {
                continue
            }
            if ($token -notmatch '^([A-Za-z]):?\s*=\s*([0-9]+(\.[0-9]+)?)$') {
                throw "Invalid -DriveMinFreeGB entry '$token'. Expected E=15 or E:=15."
            }
            $letter = $Matches[1].ToUpperInvariant()
            $map[$letter] = [double]$Matches[2]
        }
    }
    return $map
}

function Assert-RogGateFreeSpaceFloor {
    param(
        [Parameter(Mandatory = $true)][double]$MinFreeGB,
        [string[]]$DriveMinFreeGB
    )

    # 10 GB is a hard floor. These parameters can only raise a drive's requirement.
    if ($MinFreeGB -lt 10) {
        throw "-MinFreeGB $MinFreeGB is below the 10 GB floor. Thresholds can only be raised."
    }
    $overrides = ConvertTo-RogGateDriveOverrides -Entries $DriveMinFreeGB
    foreach ($key in @($overrides.Keys)) {
        $value = [double]$overrides[$key]
        if ($value -lt 10) {
            throw "-DriveMinFreeGB ${key}=$value is below the 10 GB floor. Overrides can only raise the threshold."
        }
    }
}

function Get-RogGateGitDirectory {
    param([Parameter(Mandatory = $true)][string]$Worktree)

    # A linked worktree stores "gitdir: <path>" in a .git file. Do not follow a
    # reparse point that happens to be named .git.
    $dotGit = Join-Path -Path $Worktree -ChildPath '.git'
    if (-not (Test-Path -LiteralPath $dotGit)) {
        return $null
    }
    $item = Get-Item -LiteralPath $dotGit -Force -ErrorAction Stop
    if (Test-RogGateItemReparsePoint -Item $item) {
        return $null
    }
    $isDir = (Test-RogGateAttributeHasDirectory -Attributes $item.Attributes)
    if ($isDir) {
        return $item.FullName
    }

    $raw = [System.IO.File]::ReadAllText($item.FullName)
    $gitdirMatch = [regex]::Match($raw, '(?m)^\s*gitdir:\s*(.+?)\s*$')
    if (-not $gitdirMatch.Success) {
        return $null
    }
    $gitdir = $gitdirMatch.Groups[1].Value.Trim().Replace('/', [System.IO.Path]::DirectorySeparatorChar)
    if (-not [System.IO.Path]::IsPathRooted($gitdir)) {
        $gitdir = Join-Path -Path $Worktree -ChildPath $gitdir
    }
    return [System.IO.Path]::GetFullPath($gitdir)
}

function Get-RogGateSparsePatternLines {
    param([string]$LiteralPath)

    $patterns = New-Object 'System.Collections.Generic.List[string]'
    if ([string]::IsNullOrWhiteSpace($LiteralPath)) {
        Send-RogGateCollection -Items $patterns.ToArray()
        return
    }
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) {
        Send-RogGateCollection -Items $patterns.ToArray()
        return
    }
    if (Test-RogGateLiteralReparsePoint -LiteralPath $LiteralPath) {
        Send-RogGateCollection -Items $patterns.ToArray()
        return
    }
    foreach ($line in @(Get-Content -LiteralPath $LiteralPath -ErrorAction Stop)) {
        $text = ([string]$line).Trim()
        if ($text.Length -eq 0 -or $text.StartsWith('#')) {
            continue
        }
        $patterns.Add($text)
    }
    Send-RogGateCollection -Items $patterns.ToArray()
}

function Invoke-RogGateGitRead {
    param(
        [Parameter(Mandatory = $true)][string]$Worktree,
        [Parameter(Mandatory = $true)][string[]]$GitArguments
    )

    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & git -C $Worktree @GitArguments 2>&1
        $code = $LASTEXITCODE
        if ($null -eq $code) {
            $code = 1
        }
        $lines = New-Object 'System.Collections.Generic.List[string]'
        foreach ($item in @($output)) {
            if ($null -eq $item) {
                continue
            }
            if ($item -is [System.Management.Automation.ErrorRecord]) {
                continue
            }
            $text = $item.ToString().Trim()
            if ($text.Length -eq 0) {
                continue
            }
            if ($text -match '^(?i)(fatal|error|warning):') {
                continue
            }
            $lines.Add($text)
        }
        return [pscustomobject]@{
            ExitCode = [int]$code
            Text     = ($lines -join "`n")
        }
    } catch {
        return [pscustomobject]@{
            ExitCode = 1
            Text     = ''
        }
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Get-RogGateSparseCheckoutState {
    param([Parameter(Mandatory = $true)][string]$Worktree)

    # Sparse is enabled when any one of these is true: core.sparseCheckout is
    # true, `git sparse-checkout list` reports patterns, or the worktree's
    # info/sparse-checkout file exists and still has patterns. An explicit
    # core.sparseCheckout=false leaves a leftover file inactive.
    $evidence = New-Object 'System.Collections.Generic.List[string]'
    $config = Invoke-RogGateGitRead -Worktree $Worktree -GitArguments @('config', '--get', 'core.sparseCheckout')
    $configValue = ''
    if ($config.ExitCode -eq 0 -and -not [string]::IsNullOrWhiteSpace($config.Text)) {
        $configLines = @($config.Text -split "`n")
        $configValue = [string]$configLines[$configLines.Count - 1]
    }
    $configTrue = $configValue -match '^(?i)true$'
    $configFalse = $configValue -match '^(?i)false$'
    if ($configTrue) {
        $evidence.Add('core.sparseCheckout=true')
    }

    $listed = Invoke-RogGateGitRead -Worktree $Worktree -GitArguments @('sparse-checkout', 'list')
    $listedLines = @()
    if ($listed.ExitCode -eq 0 -and -not [string]::IsNullOrWhiteSpace($listed.Text)) {
        $listedLines = @($listed.Text -split "`n")
    }
    if ($listedLines.Count -gt 0) {
        $shown = @($listedLines | Select-Object -First 8)
        $evidence.Add('git sparse-checkout list reports: ' + ($shown -join ', '))
    }

    $sparseFile = ''
    $gitDir = $null
    try {
        $gitDir = Get-RogGateGitDirectory -Worktree $Worktree
    } catch {
        $gitDir = $null
    }
    if (-not [string]::IsNullOrWhiteSpace($gitDir)) {
        $sparseFile = Join-Path -Path (Join-Path -Path $gitDir -ChildPath 'info') -ChildPath 'sparse-checkout'
        $filePatterns = @(Get-RogGateSparsePatternLines -LiteralPath $sparseFile)
        if ($filePatterns.Count -gt 0 -and -not $configFalse) {
            $evidence.Add("active sparse-checkout file: $sparseFile")
        }
    }

    return [pscustomobject]@{
        Active      = ($evidence.Count -gt 0)
        SparseState = ($evidence -join '; ')
        GitDir      = $gitDir
        SparseFile  = $sparseFile
    }
}

function Get-RogGateWorkingTreeReparsePoints {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    # Record every ReparsePoint and do not descend into it. Skip the root .git
    # metadata directory; a .git junction or symlink is still reported.
    $found = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Test-Path -LiteralPath $LiteralPath)) {
        Send-RogGateCollection -Items $found.ToArray()
        return
    }
    $rootItem = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    if (Test-RogGateItemReparsePoint -Item $rootItem) {
        $found.Add($rootItem.FullName)
        Send-RogGateCollection -Items $found.ToArray()
        return
    }
    $rootIsDir = (Test-RogGateAttributeHasDirectory -Attributes $rootItem.Attributes)
    if (-not $rootIsDir) {
        Send-RogGateCollection -Items $found.ToArray()
        return
    }

    $rootFull = $rootItem.FullName.TrimEnd('\', '/')
    $stack = New-Object 'System.Collections.Generic.Stack[string]'
    $stack.Push($rootItem.FullName)
    while ($stack.Count -gt 0) {
        $dirPath = [string]$stack.Pop()
        $dirInfo = New-Object System.IO.DirectoryInfo $dirPath
        try {
            $entries = @($dirInfo.EnumerateFileSystemInfos())
        } catch {
            throw "Cannot scan SourceDir for reparse points without following them: $dirPath. $($_.Exception.Message)"
        }
        foreach ($entry in $entries) {
            if (Test-RogGateItemReparsePoint -Item $entry) {
                $found.Add($entry.FullName)
                continue
            }
            $entryIsDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
            if (-not $entryIsDir) {
                continue
            }
            $parentTrim = $dirPath.TrimEnd('\', '/')
            if (($entry.Name -eq '.git') -and ($parentTrim -eq $rootFull)) {
                continue
            }
            $stack.Push($entry.FullName)
        }
    }
    Send-RogGateCollection -Items $found.ToArray()
}

function Assert-RogGateSparseReparsePreflight {
    param([string]$SourceDir)

    if ([string]::IsNullOrWhiteSpace($SourceDir)) {
        return
    }
    $full = $null
    try {
        $full = [System.IO.Path]::GetFullPath($SourceDir)
    } catch {
        return
    }
    if ($script:RogGateSparsePreflightChecked -eq $full) {
        return
    }
    if (-not (Test-Path -LiteralPath $SourceDir -PathType Container)) {
        return
    }

    $state = Get-RogGateSparseCheckoutState -Worktree $SourceDir
    $reparses = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $SourceDir)
    if ($state.Active -and $reparses.Count -gt 0) {
        $paths = $reparses -join '; '
        throw "SourceDir sparse checkout is enabled ($($state.SparseState)) and the working tree contains reparse points: $paths. Refusing to run git, build, or delete."
    }
    $script:RogGateSparsePreflightChecked = $full
}

function Get-RogGateDriveThresholdGB {
    param(
        [Parameter(Mandatory = $true)][string]$Letter,
        [Parameter(Mandatory = $true)][double]$MinFreeGB,
        [hashtable]$Overrides
    )

    $key = $Letter.TrimEnd(':').ToUpperInvariant()
    if ($null -ne $Overrides -and $Overrides.ContainsKey($key)) {
        return [double]$Overrides[$key]
    }
    return $MinFreeGB
}

function Get-RogGateFreeSpaceReport {
    param(
        [Parameter(Mandatory = $true)][string[]]$Drives,
        [Parameter(Mandatory = $true)][hashtable]$FreeBytesByDrive,
        [Parameter(Mandatory = $true)][double]$MinFreeGB,
        [hashtable]$Overrides
    )

    if ($null -eq $Overrides) {
        $Overrides = @{}
    }
    $rows = New-Object 'System.Collections.Generic.List[object]'
    $ok = $true
    foreach ($drive in $Drives) {
        $letter = $drive.ToUpperInvariant()
        $requiredGb = Get-RogGateDriveThresholdGB -Letter $letter -MinFreeGB $MinFreeGB -Overrides $Overrides
        if (-not $FreeBytesByDrive.ContainsKey($letter)) {
            $rows.Add([pscustomobject]@{
                Drive      = $letter
                FreeBytes  = $null
                RequiredGB = $requiredGb
                Ok         = $false
                Detail     = 'free space unavailable'
            })
            $ok = $false
            continue
        }
        $free = [int64]$FreeBytesByDrive[$letter]
        $requiredBytes = [int64][math]::Floor([double]$requiredGb * 1GB)
        $driveOk = $free -ge $requiredBytes
        if (-not $driveOk) {
            $ok = $false
        }
        $rows.Add([pscustomobject]@{
            Drive      = $letter
            FreeBytes  = $free
            RequiredGB = $requiredGb
            Ok         = $driveOk
            Detail     = ''
        })
    }
    return [pscustomobject]@{
        Ok   = $ok
        Rows = @($rows.ToArray())
    }
}

function Get-RogGateDriveFreeBytes {
    param([Parameter(Mandatory = $true)][string]$Letter)

    $root = $Letter.ToUpperInvariant() + ':\'
    $info = New-Object System.IO.DriveInfo $root
    if (-not $info.IsReady) {
        throw "Drive $root is not ready."
    }
    return [int64]$info.AvailableFreeSpace
}

function Format-RogGateGibNumber {
    param([Parameter(Mandatory = $true)][double]$Value)
    return $Value.ToString('0.00', [System.Globalization.CultureInfo]::InvariantCulture)
}

function Format-RogGateBytesAsGib {
    param([Parameter(Mandatory = $true)][int64]$Bytes)
    return (Format-RogGateGibNumber -Value ([double]$Bytes / 1GB))
}

function Get-RogGateTreeSizeBytes {
    param(
        [Parameter(Mandatory = $true)][string]$LiteralPath,
        [ref]$SkippedReparse
    )

    if (Test-RogGateLiteralReparsePoint -LiteralPath $LiteralPath) {
        if ($null -ne $SkippedReparse) {
            $SkippedReparse.Value = $true
        }
        return $null
    }

    $total = [int64]0
    $stack = New-Object 'System.Collections.Generic.Stack[string]'
    $stack.Push($LiteralPath)
    while ($stack.Count -gt 0) {
        $dirPath = $stack.Pop()
        $dirInfo = New-Object System.IO.DirectoryInfo $dirPath
        foreach ($entry in $dirInfo.EnumerateFileSystemInfos()) {
            if (Test-RogGateItemReparsePoint -Item $entry) {
                if ($null -ne $SkippedReparse) {
                    $SkippedReparse.Value = $true
                }
                continue
            }
            $entryIsDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
            if ($entryIsDir) {
                $stack.Push($entry.FullName)
            } else {
                $total += [int64]$entry.Length
            }
        }
    }
    return $total
}

function Get-RogGateTopLevelDirectorySizes {
    param([Parameter(Mandatory = $true)][string]$RootPath)

    if (Test-RogGateLiteralReparsePoint -LiteralPath $RootPath) {
        throw "Refusing to measure build root because it is a reparse point: $RootPath"
    }
    if (-not (Test-Path -LiteralPath $RootPath)) {
        Send-RogGateCollection -Items @()
        return
    }

    $rootInfo = Get-Item -LiteralPath $RootPath -Force
    $rootIsDir = (Test-RogGateAttributeHasDirectory -Attributes $rootInfo.Attributes)
    if (-not $rootIsDir) {
        throw "Build root is not a directory: $RootPath"
    }

    $result = New-Object 'System.Collections.Generic.List[object]'
    $dirInfo = New-Object System.IO.DirectoryInfo $rootInfo.FullName
    foreach ($entry in $dirInfo.EnumerateFileSystemInfos()) {
        $isDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
        if (-not $isDir) {
            continue
        }
        if (Test-RogGateItemReparsePoint -Item $entry) {
            $result.Add([pscustomobject]@{
                Path    = $entry.FullName
                Bytes   = $null
                Reparse = $true
            })
            continue
        }
        $skipped = $false
        $bytes = Get-RogGateTreeSizeBytes -LiteralPath $entry.FullName -SkippedReparse ([ref]$skipped)
        $result.Add([pscustomobject]@{
            Path    = $entry.FullName
            Bytes   = $bytes
            Reparse = [bool]$skipped
        })
    }

    $sorted = @($result | Sort-Object -Property @{ Expression = { if ($null -eq $_.Bytes) { [int64]-1 } else { [int64]$_.Bytes } } } -Descending)
    Send-RogGateCollection -Items $sorted
    return
}

function Write-RogGateError {
    param([Parameter(Mandatory = $true)][string]$Message)
    $script:RogGateSawError = $true
    [Console]::Error.WriteLine("[ERROR] $Message")
}

function Resolve-RogGateFailureExit {
    param(
        [Parameter(Mandatory = $true)][int]$ExitCode,
        [Parameter(Mandatory = $true)][string]$Message
    )

    Write-RogGateError $Message
    if ($ExitCode -eq 0) {
        return 1
    }
    return $ExitCode
}

function Test-RogGateCollectedExitIsInteger {
    param($Value)

    if ($null -eq $Value) {
        return $false
    }
    # An array, even one that holds a single integer, is stray pipeline output.
    if ($Value -is [System.Array]) {
        return $false
    }
    if (($Value -is [System.Collections.IEnumerable]) -and ($Value -isnot [string])) {
        return $false
    }
    $type = $Value.GetType()
    if ($type.IsEnum) {
        return $false
    }
    switch ([System.Type]::GetTypeCode($type)) {
        'Byte' { return $true }
        'SByte' { return $true }
        'Int16' { return $true }
        'UInt16' { return $true }
        'Int32' { return $true }
        'UInt32' { return $true }
        'Int64' { return $true }
        'UInt64' { return $true }
        default { return $false }
    }
}

function Format-RogGateCollectedExitDetail {
    param($Code)

    if ($null -eq $Code) {
        return 'Collected exit code is not a single integer (type=null, count=0).'
    }
    if ($Code -is [System.Array]) {
        return ("Collected exit code is not a single integer (type={0}, count={1})." -f $Code.GetType().FullName, $Code.Count)
    }
    return ("Collected exit code is not a single integer (type={0}, count=1)." -f $Code.GetType().FullName)
}

function Resolve-RogGateCollectedExit {
    param($Code)

    # Fail closed. Never cast an array, a string, or null to [int]: PowerShell
    # would throw, or would turn a leaked pipeline object into a bogus code.
    if (Test-RogGateCollectedExitIsInteger -Value $Code) {
        return [pscustomobject]@{
            Ok       = $true
            ExitCode = $Code
            Detail   = ''
        }
    }
    return [pscustomobject]@{
        Ok       = $false
        ExitCode = 1
        Detail   = (Format-RogGateCollectedExitDetail -Code $Code)
    }
}

function Write-RogGateFreeSpaceReport {
    param($Report)

    foreach ($row in @($Report.Rows)) {
        $flag = 'OK'
        if (-not $row.Ok) {
            $flag = 'LOW'
        }
        $required = Format-RogGateGibNumber -Value ([double]$row.RequiredGB)
        if ($null -eq $row.FreeBytes) {
            Write-Host ("{0}: free space unavailable, required {1} GB [{2}] {3}" -f $row.Drive, $required, $flag, $row.Detail)
        } else {
            $free = Format-RogGateBytesAsGib -Bytes ([int64]$row.FreeBytes)
            Write-Host ("{0}: free {1} GB, required {2} GB [{3}]" -f $row.Drive, $free, $required, $flag)
        }
    }
}

function Write-RogGateLargestDirectories {
    param([Parameter(Mandatory = $true)][string]$Root)

    Write-Host "Largest top-level directories under ${Root}:"
    if (Test-RogGateLiteralReparsePoint -LiteralPath $Root) {
        Write-Host '  (refusing to list because the build root is a reparse point)'
        return
    }
    if (-not (Test-Path -LiteralPath $Root)) {
        Write-Host '  (build root does not exist)'
        return
    }

    $rows = @(Get-RogGateTopLevelDirectorySizes -RootPath $Root)
    if ($rows.Count -eq 0) {
        Write-Host '  (no top-level directories)'
        return
    }

    $limit = 30
    $shown = 0
    foreach ($row in $rows) {
        if ($shown -ge $limit) {
            break
        }
        $shown++
        if ($null -eq $row.Bytes) {
            Write-Host ("  {0,10}  {1}" -f '(reparse)', $row.Path)
        } else {
            $label = Format-RogGateBytesAsGib -Bytes ([int64]$row.Bytes)
            $note = ''
            if ($row.Reparse) {
                $note = ' (nested reparse not followed)'
            }
            Write-Host ("  {0,10} GB  {1}{2}" -f $label, $row.Path, $note)
        }
    }
    if ($rows.Count -gt $limit) {
        $more = $rows.Count - $limit
        Write-Host "  ... $more more"
    }
}

function Get-RogGateCMakeGenerator {
    param([Parameter(Mandatory = $true)][string]$BuildDirectory)

    $cache = Join-Path -Path $BuildDirectory -ChildPath 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cache)) {
        return [pscustomobject]@{ HasCache = $false; Generator = '' }
    }

    $generator = ''
    foreach ($line in @(Get-Content -LiteralPath $cache)) {
        if ($line -match '^CMAKE_GENERATOR:[^=]+=(.*)$') {
            $generator = $Matches[1].Trim()
            break
        }
    }
    return [pscustomobject]@{ HasCache = $true; Generator = $generator }
}

function Read-RogGateStampSha {
    param([Parameter(Mandatory = $true)][string]$BuildDirectory)

    $stamp = Join-Path -Path $BuildDirectory -ChildPath $script:RogGateStampName
    if (-not (Test-Path -LiteralPath $stamp)) {
        return [pscustomobject]@{ HasStamp = $false; Sha = '' }
    }
    $lines = @(Get-Content -LiteralPath $stamp)
    $sha = ''
    if ($lines.Count -gt 0) {
        $sha = ([string]$lines[0]).Trim().ToLowerInvariant()
    }
    return [pscustomobject]@{ HasStamp = $true; Sha = $sha }
}

function Write-RogGateStamp {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDirectory,
        [Parameter(Mandatory = $true)][string]$Sha,
        [Parameter(Mandatory = $true)][string]$GeneratorName,
        [Parameter(Mandatory = $true)][string]$ConfigName
    )

    $stamp = Join-Path -Path $BuildDirectory -ChildPath $script:RogGateStampName
    $utc = [DateTime]::UtcNow.ToString('o')
    $content = @(
        $Sha.ToLowerInvariant(),
        ("generator=" + $GeneratorName),
        ("config=" + $ConfigName),
        ("utc=" + $utc)
    )
    Set-Content -LiteralPath $stamp -Value $content -Encoding ASCII
}

function Get-RogGateCleanDecision {
    param(
        [bool]$BuildDirExists,
        [bool]$HasStamp,
        [string]$StampSha,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [bool]$HasCache,
        [string]$CacheGenerator,
        [Parameter(Mandatory = $true)][string]$RequestedGenerator
    )

    if (-not $BuildDirExists) {
        return [pscustomobject]@{
            Clean        = $true
            ShouldDelete = $false
            Reason       = 'build directory does not exist'
        }
    }

    $reasons = New-Object 'System.Collections.Generic.List[string]'
    $expectedNorm = $ExpectedSha.Trim().ToLowerInvariant()
    if (-not $HasStamp) {
        $reasons.Add('stamp missing')
    } else {
        $stampNorm = $StampSha.Trim().ToLowerInvariant()
        if ($stampNorm -ne $expectedNorm) {
            $reasons.Add("stamp SHA $stampNorm differs from ExpectedSha $expectedNorm")
        }
    }
    if ($HasCache) {
        if ([string]::IsNullOrWhiteSpace($CacheGenerator)) {
            $reasons.Add('CMakeCache.txt has no CMAKE_GENERATOR')
        } elseif ($CacheGenerator -cne $RequestedGenerator) {
            $reasons.Add("CMAKE_GENERATOR '$CacheGenerator' differs from requested '$RequestedGenerator'")
        }
    }
    if ($reasons.Count -eq 0) {
        return [pscustomobject]@{
            Clean        = $false
            ShouldDelete = $false
            Reason       = 'stamp SHA and CMAKE_GENERATOR match'
        }
    }
    return [pscustomobject]@{
        Clean        = $true
        ShouldDelete = $true
        Reason       = ($reasons -join '; ')
    }
}

function Get-RogGateFilesNoFollow {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][scriptblock]$Predicate
    )

    $found = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Test-Path -LiteralPath $Root)) {
        Send-RogGateCollection -Items @()
        return
    }
    if (Test-RogGateLiteralReparsePoint -LiteralPath $Root) {
        throw "Refusing to scan reparse point: $Root"
    }

    $rootItem = Get-Item -LiteralPath $Root -Force
    $stack = New-Object 'System.Collections.Generic.Stack[string]'
    $stack.Push($rootItem.FullName)
    while ($stack.Count -gt 0) {
        $dirPath = $stack.Pop()
        $dirInfo = New-Object System.IO.DirectoryInfo $dirPath
        foreach ($entry in $dirInfo.EnumerateFileSystemInfos()) {
            if (Test-RogGateItemReparsePoint -Item $entry) {
                throw "Refusing to scan through reparse point: $($entry.FullName)"
            }
            $entryIsDir = (Test-RogGateAttributeHasDirectory -Attributes $entry.Attributes)
            if ($entryIsDir) {
                $stack.Push($entry.FullName)
            } elseif (& $Predicate $entry) {
                $found.Add($entry.FullName)
            }
        }
    }
    Send-RogGateCollection -Items $found.ToArray()
        return
}

function Get-RogGateAddTestExecutables {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Line,
        [Parameter(Mandatory = $true)][string]$NormalizedBuildDir
    )

    $exes = New-Object 'System.Collections.Generic.List[string]'
    if ($Line -notmatch '^\s*add_test\s*\(') {
        Send-RogGateCollection -Items $exes.ToArray()
        return
    }
    $quoted = [regex]::Matches($Line, '(?i)"([^"]+\.exe)"')
    foreach ($match in $quoted) {
        try {
            $norm = ConvertTo-RogGateWindowsPath -Path $match.Groups[1].Value
        } catch {
            continue
        }
        if (Test-RogGateIsOrUnder -Candidate $norm -Root $NormalizedBuildDir) {
            $exes.Add($norm)
        }
    }
    Send-RogGateCollection -Items $exes.ToArray()
    return
}

function Get-RogGateRegisteredTestExecutables {
    param([Parameter(Mandatory = $true)][string]$NormalizedBuildDir)

    $files = @(Get-RogGateFilesNoFollow -Root $NormalizedBuildDir -Predicate {
        param($Entry)
        return ($Entry.Name -eq 'CTestTestfile.cmake')
    })
    $exes = New-Object 'System.Collections.Generic.List[string]'
    foreach ($file in $files) {
        foreach ($line in @(Get-Content -LiteralPath $file)) {
            foreach ($exe in @(Get-RogGateAddTestExecutables -Line $line -NormalizedBuildDir $NormalizedBuildDir)) {
                $exes.Add($exe)
            }
        }
    }
    Send-RogGateCollection -Items $exes.ToArray()
    return
}

function Get-RogGateTestExecutablePaths {
    param([Parameter(Mandatory = $true)][string]$NormalizedBuildDir)

    $names = @(Get-RogGateFilesNoFollow -Root $NormalizedBuildDir -Predicate {
        param($Entry)
        return ($Entry.Name -like '*_tests.exe')
    })
    $set = @{}
    foreach ($name in $names) {
        $set[$name.ToLowerInvariant()] = $name
    }
    foreach ($registered in @(Get-RogGateRegisteredTestExecutables -NormalizedBuildDir $NormalizedBuildDir)) {
        $key = $registered.ToLowerInvariant()
        if (-not $set.ContainsKey($key)) {
            $set[$key] = $registered
        }
    }
    Send-RogGateCollection -Items @($set.Values)
    return
}

function Test-RogGateExecutableFresh {
    param(
        [Parameter(Mandatory = $true)][string]$ExePath,
        [Parameter(Mandatory = $true)][datetime]$BuildStartedUtc,
        [bool]$AllowExistingBecauseStampMatches
    )

    if (-not (Test-Path -LiteralPath $ExePath)) {
        return [pscustomobject]@{ Ok = $false; Reason = "missing $ExePath" }
    }
    if (Test-RogGateLiteralReparsePoint -LiteralPath $ExePath) {
        return [pscustomobject]@{ Ok = $false; Reason = "reparse point $ExePath" }
    }
    if ($AllowExistingBecauseStampMatches) {
        return [pscustomobject]@{ Ok = $true; Reason = 'stamp matches ExpectedSha' }
    }

    $item = Get-Item -LiteralPath $ExePath -Force
    $threshold = $BuildStartedUtc.AddSeconds(-2)
    if ($item.LastWriteTimeUtc -lt $threshold) {
        $written = $item.LastWriteTime.ToString('o')
        return [pscustomobject]@{ Ok = $false; Reason = "stale $ExePath written $written before this rebuild" }
    }
    return [pscustomobject]@{ Ok = $true; Reason = 'written during this build' }
}

function Get-RogGateTargetNames {
    param([string[]]$TargetNames)

    $names = New-Object 'System.Collections.Generic.List[string]'
    if ($null -eq $TargetNames) {
        Send-RogGateCollection -Items @()
        return
    }
    foreach ($raw in $TargetNames) {
        if ([string]::IsNullOrWhiteSpace($raw)) {
            continue
        }
        foreach ($piece in ($raw -split ',')) {
            $name = $piece.Trim()
            if (-not $name) {
                continue
            }
            if ($name -notmatch '^[A-Za-z0-9_.+-]+$') {
                throw "Invalid -Target name: $name"
            }
            $names.Add($name)
        }
    }
    Send-RogGateCollection -Items $names.ToArray()
    return
}

function ConvertTo-RogGateCmdArgument {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Argument)

    $escaped = $Argument.Replace('%', '%%').Replace('"', '""')
    return '"' + $escaped + '"'
}

function Invoke-RogGateVcVarsCommand {
    param(
        [Parameter(Mandatory = $true)][string[]]$CommandArguments,
        [Parameter(Mandatory = $true)][string]$VcVarsAll,
        [switch]$Quiet,
        [switch]$NoConsole,
        [ref]$CapturedText
    )

    if (-not (Test-Path -LiteralPath $VcVarsAll)) {
        throw "vcvarsall.bat not found: $VcVarsAll"
    }
    $quoted = New-Object 'System.Collections.Generic.List[string]'
    foreach ($arg in $CommandArguments) {
        $quoted.Add((ConvertTo-RogGateCmdArgument -Argument $arg))
    }
    $command = ($quoted.ToArray() -join ' ')
    if ($Quiet) {
        $command = $command + ' >nul 2>&1'
    }
    $wrapper = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath ('rog-gate-' + [guid]::NewGuid().ToString('n') + '.cmd')
    $vcQuoted = ConvertTo-RogGateCmdArgument -Argument $VcVarsAll
    $lines = @(
        '@echo off',
        'setlocal DisableDelayedExpansion',
        "call $vcQuoted x86",
        'if errorlevel 1 exit /b 1',
        $command,
        'exit /b %ERRORLEVEL%'
    )
    Set-Content -LiteralPath $wrapper -Value $lines -Encoding ASCII
    try {
        # Windows PowerShell 5.1 Start-Process -ArgumentList quoting breaks on
        # a temp path that contains spaces. Read both pipes before waiting so
        # a long build log cannot deadlock, and return only the exit code.
        $startInfo = New-Object System.Diagnostics.ProcessStartInfo
        $startInfo.FileName = 'cmd.exe'
        $startInfo.Arguments = '/d /c "' + $wrapper + '"'
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
        $process = New-Object System.Diagnostics.Process
        $process.StartInfo = $startInfo
        if (-not $process.Start()) {
            if ($PSBoundParameters.ContainsKey('CapturedText')) {
                $CapturedText.Value = ''
            }
            return 1
        }
        $outTask = $process.StandardOutput.ReadToEndAsync()
        $errTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $outText = $outTask.GetAwaiter().GetResult()
        $errText = $errTask.GetAwaiter().GetResult()
        $combined = ''
        if (-not [string]::IsNullOrEmpty($outText)) {
            if (-not $NoConsole) {
                [Console]::Out.Write($outText)
            }
            $combined = $outText
        }
        if (-not [string]::IsNullOrEmpty($errText)) {
            if (-not $NoConsole) {
                [Console]::Error.Write($errText)
            }
            if ($combined.Length -gt 0) {
                $combined = $combined + "`n"
            }
            $combined = $combined + $errText
        }
        if ($PSBoundParameters.ContainsKey('CapturedText')) {
            $CapturedText.Value = $combined
        }
        return [int]$process.ExitCode
    } finally {
        Remove-Item -LiteralPath $wrapper -Force -ErrorAction SilentlyContinue
    }
}

function Get-RogGatePlayDhRootPath {
    param([Parameter(Mandatory = $true)][string]$SourceDir)

    # tests/support/playdh_env.hpp and the asset tests expect this directory:
    # Resource\ItemList.bin and Image\2D\login.dds live under it.
    $modern = Join-Path -Path $SourceDir -ChildPath 'modern'
    $data = Join-Path -Path $modern -ChildPath 'data'
    $playdh = Join-Path -Path $data -ChildPath 'PlayDH'
    return [System.IO.Path]::GetFullPath($playdh)
}

function Get-RogGatePlayDhAssignment {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDir,
        [AllowNull()][AllowEmptyString()][string]$PreviousValue
    )

    $path = Get-RogGatePlayDhRootPath -SourceDir $SourceDir
    $previous = ''
    if ($null -ne $PreviousValue) {
        $previous = [string]$PreviousValue
    }
    $info = "[INFO] MXH_PLAYDH_ROOT=$path (auto)"
    if (-not [string]::IsNullOrWhiteSpace($previous)) {
        $info = "[INFO] MXH_PLAYDH_ROOT=$path (overrode '$previous')"
    }
    return [pscustomobject]@{
        Path     = $path
        Previous = $previous
        Info     = $info
    }
}

function Set-RogGatePlayDhRootForChildren {
    param([Parameter(Mandatory = $true)][string]$SourceDir)

    # Always the worktree copy. A value inherited from the parent is replaced
    # so CTest children do not keep searching upward from the build directory.
    $assignment = Get-RogGatePlayDhAssignment -SourceDir $SourceDir -PreviousValue $env:MXH_PLAYDH_ROOT
    $env:MXH_PLAYDH_ROOT = $assignment.Path
    Write-Host $assignment.Info
    return $assignment
}

function Test-RogGateGitLfsPointer {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $marker = 'version https://git-lfs.github.com/spec/v1'
    $stream = $null
    $read = 0
    $buffer = New-Object byte[] ($marker.Length + 3)
    try {
        $stream = [System.IO.File]::Open($LiteralPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        $read = $stream.Read($buffer, 0, $buffer.Length)
    } finally {
        if ($null -ne $stream) {
            $stream.Dispose()
        }
    }
    $offset = 0
    if ($read -ge 3 -and $buffer[0] -eq 0xEF -and $buffer[1] -eq 0xBB -and $buffer[2] -eq 0xBF) {
        $offset = 3
    }
    if (($read - $offset) -lt $marker.Length) {
        return $false
    }
    $text = [System.Text.Encoding]::ASCII.GetString($buffer, $offset, $marker.Length)
    return ($text -eq $marker)
}

function Test-RogGatePlayDhPreflight {
    param([Parameter(Mandatory = $true)][string]$PlayDhRoot)

    if (-not (Test-Path -LiteralPath $PlayDhRoot -PathType Container)) {
        return [pscustomobject]@{
            Ok     = $false
            Reason = "MXH_PLAYDH_ROOT directory does not exist: $PlayDhRoot"
        }
    }
    $itemList = Join-Path -Path (Join-Path -Path $PlayDhRoot -ChildPath 'Resource') -ChildPath 'ItemList.bin'
    $login = Join-Path -Path (Join-Path -Path (Join-Path -Path $PlayDhRoot -ChildPath 'Image') -ChildPath '2D') -ChildPath 'login.dds'
    $required = @(
        $itemList,
        $login
    )
    foreach ($full in $required) {
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
            return [pscustomobject]@{
                Ok     = $false
                Reason = "PlayDH file missing: $full"
            }
        }
        if (Test-RogGateGitLfsPointer -LiteralPath $full) {
            return [pscustomobject]@{
                Ok     = $false
                Reason = "PlayDH file is a Git LFS pointer: $full"
            }
        }
    }
    return [pscustomobject]@{
        Ok     = $true
        Reason = ''
    }
}

function Get-RogGateCTestSkippedTests {
    param([AllowEmptyString()][string]$Text)

    $names = New-Object 'System.Collections.Generic.List[string]'
    $seen = @{}
    if (-not [string]::IsNullOrEmpty($Text)) {
        $patterns = @(
            '(?m)^\s*\d+\s+-\s+(.+?)\s+\(Skipped\)\s*$',
            '(?m)^\s*\d+/\d+\s+Test\s+#\d+:\s+(.+?)\s+\.+\*+Skipped\b'
        )
        foreach ($pattern in $patterns) {
            $found = [regex]::Matches($Text, $pattern)
            foreach ($match in $found) {
                $name = $match.Groups[1].Value.Trim()
                if ([string]::IsNullOrWhiteSpace($name)) {
                    continue
                }
                $key = $name.ToLowerInvariant()
                if (-not $seen.ContainsKey($key)) {
                    $seen[$key] = $true
                    $names.Add($name)
                }
            }
        }
    }
    $count = $names.Count
    if ($count -eq 0 -and -not [string]::IsNullOrEmpty($Text)) {
        $summary = [regex]::Match($Text, '(?m)(\d+)\s+tests?\s+skipped')
        if ($summary.Success) {
            $count = [int]$summary.Groups[1].Value
        }
    }
    return [pscustomobject]@{
        Count = $count
        Names = @($names.ToArray())
    }
}

function Get-RogGateCTestSkipReport {
    param([AllowEmptyString()][string]$Text)

    $parsed = Get-RogGateCTestSkippedTests -Text $Text
    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add(('[INFO] CTest skipped={0}' -f [int]$parsed.Count))
    foreach ($name in @($parsed.Names)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$name)) {
            $lines.Add(('[INFO] CTest skip: {0}' -f [string]$name))
        }
    }
    return [pscustomobject]@{
        Count = [int]$parsed.Count
        Names = @($parsed.Names)
        Lines = @($lines.ToArray())
    }
}

function New-RogGateE2EPortResult {
    param(
        [bool]$Ok,
        [string]$Reason,
        [int]$Login = 0,
        [int]$Agent = 0,
        [int]$Map = 0,
        [int]$Map12 = 0
    )

    return [pscustomobject]@{
        Ok     = $Ok
        Reason = $Reason
        Login  = $Login
        Agent  = $Agent
        Map    = $Map
        Map12  = $Map12
    }
}

function Get-RogGateE2EPortsFromBase {
    param([AllowNull()][AllowEmptyString()][string]$BaseText)

    # Match the #39 launcher: optional leading '+', then digits. The gate
    # range is 1024-65535 (the launcher allows 1-65535) and the forbidden
    # set also includes 36001, 37001, and 38001.
    $text = ''
    if ($null -ne $BaseText) {
        $text = ([string]$BaseText).Trim()
    }
    if ($text -notmatch '^\+?[0-9]+$') {
        return (New-RogGateE2EPortResult -Ok $false -Reason "MXH_E2E_PORT_BASE is not an integer: $text")
    }
    $digits = $text
    if ($digits.StartsWith('+')) {
        $digits = $digits.Substring(1)
    }
    $numeric = [int64]0
    $parsed = [int64]::TryParse($digits, [ref]$numeric)
    if (-not $parsed -or $numeric -gt 65535) {
        $shown = $text
        if ($parsed) {
            $shown = [string]$numeric
        }
        return (New-RogGateE2EPortResult -Ok $false -Reason "E2E port $shown is outside 1024-65535.")
    }
    $computed = @(
        $numeric,
        ($numeric + 1000),
        ($numeric + 2000),
        ($numeric + 2001)
    )
    foreach ($port in $computed) {
        if ($port -lt 1024 -or $port -gt 65535) {
            return (New-RogGateE2EPortResult -Ok $false -Reason "E2E port $port is outside 1024-65535.")
        }
    }
    $forbidden = @(16001, 17001, 18001, 36001, 37001, 38001)
    foreach ($port in $computed) {
        foreach ($blocked in $forbidden) {
            if ($port -eq [int64]$blocked) {
                return (New-RogGateE2EPortResult -Ok $false -Reason "E2E port $port is forbidden.")
            }
        }
    }
    return (New-RogGateE2EPortResult -Ok $true -Reason '' -Login ([int]$computed[0]) -Agent ([int]$computed[1]) -Map ([int]$computed[2]) -Map12 ([int]$computed[3]))
}

function Format-RogGateE2EPortsLine {
    param(
        [Parameter(Mandatory = $true)][int]$Login,
        [Parameter(Mandatory = $true)][int]$Agent,
        [Parameter(Mandatory = $true)][int]$Map,
        [Parameter(Mandatory = $true)][int]$Map12
    )

    return "[INFO] E2E ports login=$Login agent=$Agent map=$Map map12=$Map12"
}

function Set-RogGateE2EPortBase {
    $env:MXH_E2E_PORT_BASE = $script:RogGateE2EPortBaseText
    Write-Host ("[INFO] MXH_E2E_PORT_BASE=" + $script:RogGateE2EPortBaseText)
}

function Format-RogGatePortInUseMessage {
    param([Parameter(Mandatory = $true)][int]$Port)

    return "port $Port in use"
}

function Test-RogGateLoopbackPortFree {
    param([Parameter(Mandatory = $true)][int]$Port)

    $listener = $null
    $started = $false
    try {
        $listener = New-Object System.Net.Sockets.TcpListener -ArgumentList ([System.Net.IPAddress]::Loopback, $Port)
        $listener.Start()
        $started = $true
        return $true
    } catch {
        return $false
    } finally {
        if ($started -and $null -ne $listener) {
            $listener.Stop()
        }
    }
}

function Get-RogGateFirstBusyLoopbackPort {
    param([Parameter(Mandatory = $true)][int[]]$Ports)

    foreach ($port in $Ports) {
        $free = Test-RogGateLoopbackPortFree -Port $port
        if (-not $free) {
            return [int]$port
        }
    }
    return 0
}

function Get-RogGateMainCTestArguments {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$ConfigName,
        [AllowNull()][string[]]$ExtraArgs
    )

    $kept = New-Object 'System.Collections.Generic.List[string]'
    $excludes = New-Object 'System.Collections.Generic.List[string]'
    $extras = @()
    if ($null -ne $ExtraArgs) {
        $extras = @($ExtraArgs)
    }
    $index = 0
    while ($index -lt $extras.Count) {
        $token = ''
        if ($null -ne $extras[$index]) {
            $token = [string]$extras[$index]
        }
        if ([string]::IsNullOrEmpty($token)) {
            $index = $index + 1
            continue
        }
        $excludeValue = $null
        $isExclude = $false
        if ($token -eq '-E' -or $token -eq '--exclude-regex') {
            $isExclude = $true
            $index = $index + 1
            if ($index -ge $extras.Count -or [string]::IsNullOrEmpty([string]$extras[$index])) {
                throw 'ctest -E is missing its regex.'
            }
            $excludeValue = [string]$extras[$index]
        } elseif ($token.StartsWith('--exclude-regex=')) {
            $isExclude = $true
            $prefix = '--exclude-regex='
            $excludeValue = $token.Substring($prefix.Length)
            if ([string]::IsNullOrEmpty($excludeValue)) {
                throw 'ctest -E is missing its regex.'
            }
        }
        if ($isExclude) {
            $excludes.Add($excludeValue)
        } else {
            $kept.Add($token)
        }
        $index = $index + 1
    }

    $regex = $script:RogGateE2ETestRegex
    $onlyE2E = $true
    foreach ($item in $excludes) {
        if ($item -cne $script:RogGateE2ETestRegex) {
            $onlyE2E = $false
        }
    }
    if (-not $onlyE2E) {
        $parts = New-Object 'System.Collections.Generic.List[string]'
        foreach ($item in $excludes) {
            if ($item -cne $script:RogGateE2ETestRegex) {
                $parts.Add('(' + $item + ')')
            }
        }
        $parts.Add('(' + $script:RogGateE2ETestRegex + ')')
        $regex = ($parts.ToArray() -join '|')
    }

    $out = New-Object 'System.Collections.Generic.List[string]'
    $out.Add('ctest')
    $out.Add('--test-dir')
    $out.Add($BuildDir)
    $out.Add('-C')
    $out.Add($ConfigName)
    $out.Add('--output-on-failure')
    foreach ($item in $kept) {
        $out.Add($item)
    }
    $out.Add('-E')
    $out.Add($regex)
    return [pscustomobject]@{
        Arguments = @($out.ToArray())
    }
}

function Get-RogGateE2ECtestArguments {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$ConfigName,
        [bool]$ListOnly
    )

    $out = New-Object 'System.Collections.Generic.List[string]'
    $out.Add('ctest')
    $out.Add('--test-dir')
    $out.Add($BuildDir)
    $out.Add('-C')
    $out.Add($ConfigName)
    if ($ListOnly) {
        $out.Add('-N')
        $out.Add('-V')
    } else {
        $out.Add('--output-on-failure')
    }
    $out.Add('-R')
    $out.Add($script:RogGateE2ETestRegex)
    return [pscustomobject]@{
        Arguments = @($out.ToArray())
    }
}

function Get-RogGateCommandTokens {
    param([AllowEmptyString()][string]$CommandLine)

    $tokens = New-Object 'System.Collections.Generic.List[string]'
    if ([string]::IsNullOrEmpty($CommandLine)) {
        return @($tokens.ToArray())
    }
    $current = New-Object System.Text.StringBuilder
    $inQuote = $false
    $quote = [char]0
    foreach ($ch in $CommandLine.ToCharArray()) {
        if ($inQuote) {
            if ($ch -eq $quote) {
                $inQuote = $false
            } else {
                $null = $current.Append($ch)
            }
        } elseif ($ch -eq '"' -or $ch -eq "'") {
            $inQuote = $true
            $quote = $ch
        } elseif ($ch -eq ' ' -or $ch -eq "`t") {
            if ($current.Length -gt 0) {
                $tokens.Add($current.ToString())
                $current.Length = 0
            }
        } else {
            $null = $current.Append($ch)
        }
    }
    if ($current.Length -gt 0) {
        $tokens.Add($current.ToString())
    }
    return @($tokens.ToArray())
}

function Test-RogGateE2ELauncherName {
    param([AllowEmptyString()][string]$Executable)

    if ([string]::IsNullOrWhiteSpace($Executable)) {
        return $false
    }
    $leaf = [System.IO.Path]::GetFileName($Executable.Trim())
    $suffix = '.exe'
    if ($leaf.EndsWith($suffix, [System.StringComparison]::OrdinalIgnoreCase)) {
        $leaf = $leaf.Substring(0, $leaf.Length - $suffix.Length)
    }
    return $leaf.Equals($script:RogGateE2ELauncherName, [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-RogGateE2EListing {
    param(
        [AllowEmptyString()][string]$Text,
        [int]$ExitCode
    )

    $commands = New-Object 'System.Collections.Generic.List[string]'
    $directories = New-Object 'System.Collections.Generic.List[string]'
    $body = ''
    if ($null -ne $Text) {
        $body = [string]$Text
    }
    if (-not [string]::IsNullOrEmpty($body)) {
        $commandMatches = [regex]::Matches($body, '(?m)^\s*(?:\d+:\s*)?Test command:\s*(.*)$')
        foreach ($commandMatch in $commandMatches) {
            $commands.Add($commandMatch.Groups[1].Value.Trim())
        }
        $directoryMatches = [regex]::Matches($body, '(?m)^\s*(?:\d+:\s*)?Working Directory:\s*(.*)$')
        foreach ($directoryMatch in $directoryMatches) {
            $directories.Add($directoryMatch.Groups[1].Value.Trim())
        }
    }
    $notRegistered = $false
    if (-not [string]::IsNullOrEmpty($body)) {
        if ($body.Contains('No tests were found') -or [regex]::IsMatch($body, '(?m)Total Tests:\s*0\b')) {
            $notRegistered = $true
        }
    }
    $parseError = ''
    $command = ''
    $directory = ''
    $launcher = $false
    $registered = $false
    $absent = $false
    if ($commands.Count -gt 1) {
        $parseError = 'MoxianClientE2E listing matched more than one test command.'
    } elseif ($ExitCode -ne 0 -and -not $notRegistered) {
        $parseError = 'Could not read the MoxianClientE2E test command.'
    } elseif ($commands.Count -eq 1) {
        $registered = $true
        $command = [string]$commands[0]
        if ($directories.Count -ge 1) {
            $directory = [string]$directories[0]
        }
        $tokens = @(Get-RogGateCommandTokens -CommandLine $command)
        $executable = ''
        if ($tokens.Count -ge 1) {
            $executable = [string]$tokens[0]
        }
        $launcher = Test-RogGateE2ELauncherName -Executable $executable
    } elseif ($notRegistered) {
        $absent = $true
    } else {
        $parseError = 'Could not read the MoxianClientE2E test command.'
    }
    return [pscustomobject]@{
        Registered       = $registered
        Absent           = $absent
        ParseError       = $parseError
        Command          = $command
        WorkingDirectory = $directory
        Launcher         = $launcher
    }
}

function Resolve-RogGateE2ERegistration {
    param(
        [Parameter(Mandatory = $true)]$Listing,
        [switch]$SkipE2E
    )

    if (-not [string]::IsNullOrEmpty([string]$Listing.ParseError)) {
        return [pscustomobject]@{
            Action  = 'Error'
            Message = [string]$Listing.ParseError
        }
    }
    if ($Listing.Absent -or -not $Listing.Registered) {
        return [pscustomobject]@{
            Action  = 'Absent'
            Message = '[INFO] MoxianClientE2E not registered'
        }
    }
    if ($Listing.Launcher) {
        return [pscustomobject]@{
            Action  = 'Run'
            Message = ''
        }
    }
    if ($SkipE2E) {
        return [pscustomobject]@{
            Action  = 'Skip'
            Message = '[WARN] E2E skipped by -SkipE2E'
        }
    }
    return [pscustomobject]@{
        Action  = 'Error'
        Message = 'MoxianClientE2E is pre-#39 (no port launcher); refusing to run'
    }
}

function Get-RogGateHeadSha {
    param([Parameter(Mandatory = $true)][string]$Worktree)

    $output = & git -C $Worktree rev-parse HEAD
    $code = $LASTEXITCODE
    if ($code -ne 0) {
        throw "git -C SourceDir rev-parse HEAD failed (exit $code)."
    }
    $text = ''
    if ($null -ne $output) {
        $text = ((@($output) | Select-Object -First 1).ToString()).Trim().ToLowerInvariant()
    }
    if ($text -notmatch '^[0-9a-f]{40}$') {
        throw "git rev-parse HEAD returned '$text'."
    }
    return $text
}

function Write-RogGateHeader {
    param(
        [string]$Source,
        [string]$Build,
        [string]$Expected,
        [string]$ConfigName,
        [string]$GeneratorName,
        [string]$Allowed,
        [bool]$Clean,
        [string]$Reason,
        [string[]]$Drives
    )

    $cleanText = 'no'
    if ($Clean) {
        $cleanText = 'yes'
    }
    Write-Host '======== ROG gate build ========'
    Write-Host "SourceDir:     $Source"
    Write-Host "BuildDir:      $Build"
    Write-Host "ExpectedSha:   $Expected"
    Write-Host "Config:        $ConfigName"
    Write-Host "Generator:     $GeneratorName"
    Write-Host "AllowedRoot:   $Allowed"
    Write-Host "CleanRebuild:  $cleanText ($Reason)"
    Write-Host ("Drives:        " + ($Drives -join ', '))
    Write-Host 'Toolchain:     vcvarsall.bat x86 (Hostx86/x86)'
    Write-Host '==============================='
}

function Test-RogGateHostIsWindows {
    # $IsWindows exists on PowerShell 6+. Windows PowerShell 5.1 only has $env:OS.
    if (Test-Path -Path 'Variable:IsWindows') {
        return [bool](Get-Variable -Name 'IsWindows' -ValueOnly)
    }
    return ($env:OS -eq 'Windows_NT')
}

function Get-RogGateSelfTestLinkType {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $item = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    $prop = $item.PSObject.Properties['LinkType']
    if ($null -ne $prop -and -not [string]::IsNullOrWhiteSpace([string]$prop.Value)) {
        return [string]$prop.Value
    }
    # Windows PowerShell 5.1 does not surface LinkType. dir /al names the real
    # NTFS object: <JUNCTION> or <SYMLINKD>.
    if (-not (Test-RogGateHostIsWindows)) {
        return ''
    }
    $parent = Split-Path -Path $item.FullName -Parent
    $name = Split-Path -Path $item.FullName -Leaf
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $listing = & cmd.exe /d /c "dir /al `"$parent`"" 2>&1
    } finally {
        $ErrorActionPreference = $previous
    }
    foreach ($entry in @($listing)) {
        if ($entry -is [System.Management.Automation.ErrorRecord]) {
            continue
        }
        $text = [string]$entry
        if ($text -notmatch '<(JUNCTION|SYMLINKD|SYMLINK)>') {
            continue
        }
        $tag = $Matches[1]
        $shown = ''
        if ($text -match '<(?:JUNCTION|SYMLINKD|SYMLINK)>\s+(.*?)\s+\[') {
            $shown = $Matches[1].Trim()
        }
        if ($shown -ne $name) {
            continue
        }
        if ($tag -eq 'JUNCTION') {
            return 'Junction'
        }
        return 'SymbolicLink'
    }
    return ''
}

function New-RogGateWindowsJunction {
    param(
        [Parameter(Mandatory = $true)][string]$LinkPath,
        [Parameter(Mandatory = $true)][string]$TargetPath
    )

    # mklink /J does not need Developer Mode. Try it before any symlink API so
    # an elevated shell cannot silently substitute a symlink.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $null = & cmd.exe /d /c "mklink /J `"$LinkPath`" `"$TargetPath`"" 2>&1
        if ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $LinkPath)) {
            return $true
        }
    } finally {
        $ErrorActionPreference = $previous
    }
    try {
        New-Item -ItemType Junction -Path $LinkPath -Value $TargetPath -ErrorAction Stop | Out-Null
        return $true
    } catch {
        try {
            New-Item -ItemType Junction -Path $LinkPath -Target $TargetPath -ErrorAction Stop | Out-Null
            return $true
        } catch {
            return $false
        }
    }
}

function New-RogGateSelfTestDirectoryLink {
    param(
        [Parameter(Mandatory = $true)][string]$LinkPath,
        [Parameter(Mandatory = $true)][string]$TargetPath,
        [ValidateSet('Auto', 'Junction', 'SymbolicLink')]
        [string]$Kind = 'Auto'
    )

    if (-not (Test-Path -LiteralPath $TargetPath)) {
        New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
    }

    $wantJunction = $false
    if ($Kind -eq 'Junction') {
        $wantJunction = $true
    } elseif ($Kind -eq 'SymbolicLink') {
        $wantJunction = $false
    } else {
        $wantJunction = Test-RogGateHostIsWindows
    }

    $created = $false
    if ($wantJunction) {
        $created = New-RogGateWindowsJunction -LinkPath $LinkPath -TargetPath $TargetPath
    } else {
        try {
            New-Item -ItemType SymbolicLink -Path $LinkPath -Target $TargetPath -ErrorAction Stop | Out-Null
            $created = $true
        } catch {
            try {
                New-Item -ItemType SymbolicLink -Path $LinkPath -Value $TargetPath -ErrorAction Stop | Out-Null
                $created = $true
            } catch {
                $created = $false
            }
        }
    }

    $linkType = ''
    if ($created -and (Test-Path -LiteralPath $LinkPath)) {
        $linkType = Get-RogGateSelfTestLinkType -LiteralPath $LinkPath
    }
    $expected = 'SymbolicLink'
    if ($wantJunction) {
        $expected = 'Junction'
    }
    return [pscustomobject]@{
        Ok       = [bool]($created -and ($linkType -eq $expected))
        Created  = [bool]$created
        LinkType = [string]$linkType
        Expected = [string]$expected
    }
}

function Invoke-RogGateSelfTest {
    $script:RogGateSelfTestFailures = 0

    function Assert-RogGateSelfTest {
        param(
            [Parameter(Mandatory = $true)][string]$Name,
            [bool]$Condition
        )
        if ($Condition) {
            Write-Host "[PASS] $Name"
        } else {
            Write-Host "[FAIL] $Name"
            $script:RogGateSelfTestFailures = $script:RogGateSelfTestFailures + 1
        }
    }

    function Write-RogGateSelfTestSkip {
        param([Parameter(Mandatory = $true)][string]$Name)
        Write-Host "[SKIP] $Name"
    }

    $directoryLinkKind = 'SymbolicLink'
    if (Test-RogGateHostIsWindows) {
        $directoryLinkKind = 'Junction'
    }

    # 0x80000 is FILE_ATTRIBUTE_PINNED. Windows PowerShell 5.1's FileAttributes
    # enum does not define it, so 524320 (0x80020) must stay an integer.
    $attrThrew = $false
    $highReparse = $false
    $lowReparse = $true
    try {
        $enumHigh = [Enum]::ToObject([System.IO.FileAttributes], [int64]0x80420)
        $enumLow = [Enum]::ToObject([System.IO.FileAttributes], [int64]0x80020)
        $highRaw = Test-RogGateAttributeBit -Attributes ([int64]0x80420) -Bit $script:RogGateAttributeReparsePoint
        $highEnum = Test-RogGateAttributeBit -Attributes $enumHigh -Bit $script:RogGateAttributeReparsePoint
        $highItem = Test-RogGateItemReparsePoint -Item ([pscustomobject]@{ Attributes = $enumHigh })
        $lowRaw = Test-RogGateAttributeBit -Attributes ([int64]0x80020) -Bit $script:RogGateAttributeReparsePoint
        $lowEnum = Test-RogGateAttributeBit -Attributes $enumLow -Bit $script:RogGateAttributeReparsePoint
        $lowItem = Test-RogGateItemReparsePoint -Item ([pscustomobject]@{ Attributes = $enumLow })
        $highReparse = $highRaw -and $highEnum -and $highItem
        $lowReparse = $lowRaw -or $lowEnum -or $lowItem
    } catch {
        $attrThrew = $true
    }
    Assert-RogGateSelfTest -Name 'raw attributes 0x80420 are reparse and 0x80020 are not, without throwing' -Condition ((-not $attrThrew) -and $highReparse -and (-not $lowReparse))

    $normalized = ConvertTo-RogGateWindowsPath -Path 'D:/build-mxr/foo/./bar'
    Assert-RogGateSelfTest -Name 'normalize slashes and dots' -Condition ($normalized -eq 'D:\build-mxr\foo\bar')
    $normalized = ConvertTo-RogGateWindowsPath -Path 'D:\build-mxr\foo\..\bar\'
    Assert-RogGateSelfTest -Name 'normalize parent segment' -Condition ($normalized -eq 'D:\build-mxr\bar')
    $normalized = ConvertTo-RogGateWindowsPath -Path '\\?\D:\build-mxr\pr20'
    Assert-RogGateSelfTest -Name 'strip extended path prefix' -Condition ($normalized -eq 'D:\build-mxr\pr20')

    $threw = $false
    try { $null = ConvertTo-RogGateWindowsPath -Path 'D:\..\MX' } catch { $threw = $true }
    Assert-RogGateSelfTest -Name 'reject drive escape' -Condition $threw

    $threw = $false
    try { $null = ConvertTo-RogGateWindowsPath -Path '\\server\share\pr' } catch { $threw = $true }
    Assert-RogGateSelfTest -Name 'reject UNC' -Condition $threw

    $threw = $false
    try { $null = ConvertTo-RogGateWindowsPath -Path 'build-mxr\pr' } catch { $threw = $true }
    Assert-RogGateSelfTest -Name 'reject relative path' -Condition $threw

    $safe = Get-RogGateLexicalSafety -BuildDir 'D:\build-mxr\pr20' -AllowedRoot 'D:\build-mxr\'
    Assert-RogGateSelfTest -Name 'pr directory strictly under default root' -Condition $safe.Safe
    Assert-RogGateSelfTest -Name 'normalized build dir' -Condition ($safe.NormalizedBuildDir -eq 'D:\build-mxr\pr20')

    $rootItself = Get-RogGateLexicalSafety -BuildDir 'D:\build-mxr' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'root itself is not a build dir' -Condition ((-not $rootItself.Safe) -and ($rootItself.Reason -match 'strictly under'))

    $prefix = Get-RogGateLexicalSafety -BuildDir 'D:\build-mxr-evil\pr' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'prefix sibling is outside the root' -Condition ((-not $prefix.Safe) -and ($prefix.Reason -notmatch 'forbidden'))

    $escaped = Get-RogGateLexicalSafety -BuildDir 'D:\build-mxr\pr\..\..\MX\secret' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'dotdot into D:\MX is forbidden' -Condition ((-not $escaped.Safe) -and ($escaped.NormalizedBuildDir -eq 'D:\MX\secret') -and ($escaped.Reason -match 'forbidden'))

    $mx = Get-RogGateLexicalSafety -BuildDir 'D:\mx\foo' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'D:\MX is forbidden regardless of case' -Condition ((-not $mx.Safe) -and ($mx.Reason -match 'forbidden'))

    $mxr = Get-RogGateLexicalSafety -BuildDir 'D:\MXR\foo' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'D:\MXR is not the forbidden D:\MX tree' -Condition ((-not $mxr.Safe) -and ($mxr.Reason -match 'strictly under') -and ($mxr.Reason -notmatch 'forbidden'))

    $driveRoot = Get-RogGateLexicalSafety -BuildDir 'D:\build-mxr\pr' -AllowedRoot 'D:\'
    Assert-RogGateSelfTest -Name 'drive root cannot be the allowed root' -Condition ((-not $driveRoot.Safe) -and ($driveRoot.Reason -match 'drive root'))

    $forbidRoot = Get-RogGateLexicalSafety -BuildDir 'D:\MX\pr' -AllowedRoot 'D:\MX'
    Assert-RogGateSelfTest -Name 'allowed root inside D:\MX is refused' -Condition ((-not $forbidRoot.Safe) -and ($forbidRoot.Reason -match 'forbidden'))

    $combined = Get-RogGateDeletionSafety -BuildDir 'D:\MX\pr' -AllowedRoot 'D:\build-mxr'
    Assert-RogGateSelfTest -Name 'deletion safety refuses D:\MX before any delete' -Condition (-not $combined.Safe)

    $mayThrew = $false
    try { $null = Assert-RogGateMayDelete -Safety $rootItself } catch { $mayThrew = $true }
    Assert-RogGateSelfTest -Name 'may-delete refuses the allowed root itself' -Condition $mayThrew

    $drives = @(Get-RogGateDrivesToCheck -NormalizedBuildDir 'D:\build-mxr\pr20' -NormalizedSourceDir 'E:\work\trial')
    Assert-RogGateSelfTest -Name 'build D and source E dedup the required E drive' -Condition (($drives.Count -eq 2) -and ($drives[0] -eq 'D') -and ($drives[1] -eq 'E'))

    $drives = @(Get-RogGateDrivesToCheck -NormalizedBuildDir 'E:\build\pr' -NormalizedSourceDir 'E:\src\trial')
    Assert-RogGateSelfTest -Name 'source and build on E are checked once' -Condition (($drives.Count -eq 1) -and ($drives[0] -eq 'E'))

    $drives = @(Get-RogGateDrivesToCheck -NormalizedBuildDir 'D:\build-mxr\pr' -NormalizedSourceDir 'C:\src\repo')
    Assert-RogGateSelfTest -Name 'build, source, and E are all checked' -Condition (($drives.Count -eq 3) -and ($drives[0] -eq 'D') -and ($drives[1] -eq 'C') -and ($drives[2] -eq 'E'))

    $overrides = ConvertTo-RogGateDriveOverrides -Entries @('E=20,D=15')
    $eThreshold = Get-RogGateDriveThresholdGB -Letter 'E' -MinFreeGB 10 -Overrides $overrides
    $cThreshold = Get-RogGateDriveThresholdGB -Letter 'c' -MinFreeGB 10 -Overrides $overrides
    Assert-RogGateSelfTest -Name 'per-drive override replaces the default' -Condition (($eThreshold -eq 20) -and ($cThreshold -eq 10))

    $badOverride = $false
    try { $null = ConvertTo-RogGateDriveOverrides -Entries @('E') } catch { $badOverride = $true }
    Assert-RogGateSelfTest -Name 'drive override without a value is rejected' -Condition $badOverride

    $floorScript = $PSCommandPath
    $floorExe = [System.Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
    $floorSha = 'a' * 40
    function Invoke-RogGateFloorProbe {
        param([Parameter(Mandatory = $true)][string[]]$ProbeArguments)
        $ErrorActionPreference = 'Continue'
        $merged = & $floorExe -NoProfile -File $floorScript @ProbeArguments 2>&1
        $code = $LASTEXITCODE
        if ($null -eq $code) { $code = 1 }
        $text = ((@($merged) | ForEach-Object { $_.ToString() }) -join "`n")
        return [pscustomobject]@{ ExitCode = [int]$code; Text = $text }
    }
    $lowMin = Invoke-RogGateFloorProbe -ProbeArguments @('-MinFreeGB', '5', '-SourceDir', 'C:\src', '-BuildDir', 'D:\build-mxr\pr', '-ExpectedSha', $floorSha)
    $lowMinRejected = ($lowMin.ExitCode -eq 1) -and ($lowMin.Text -match '-MinFreeGB 5 is below the 10 GB floor') -and ($lowMin.Text -notmatch 'SourceDir not found')
    Assert-RogGateSelfTest -Name '-MinFreeGB 5 is rejected before any other work' -Condition $lowMinRejected
    $lowDrive = Invoke-RogGateFloorProbe -ProbeArguments @('-DriveMinFreeGB', 'E=1', '-SourceDir', 'C:\src', '-BuildDir', 'D:\build-mxr\pr', '-ExpectedSha', $floorSha)
    $lowDriveRejected = ($lowDrive.ExitCode -eq 1) -and ($lowDrive.Text -match '-DriveMinFreeGB E=1 is below the 10 GB floor') -and ($lowDrive.Text -notmatch 'SourceDir not found')
    Assert-RogGateSelfTest -Name '-DriveMinFreeGB E=1 is rejected before any other work' -Condition $lowDriveRejected
    $raisedDrive = Invoke-RogGateFloorProbe -ProbeArguments @('-DriveMinFreeGB', 'E=20', '-SourceDir', 'C:\src', '-BuildDir', 'D:\build-mxr\pr', '-ExpectedSha', $floorSha)
    $raisedDriveAccepted = ($raisedDrive.ExitCode -eq 1) -and ($raisedDrive.Text -match 'SourceDir not found') -and ($raisedDrive.Text -notmatch 'below the 10 GB floor')
    Assert-RogGateSelfTest -Name '-DriveMinFreeGB E=20 is accepted and later checks still run' -Condition $raisedDriveAccepted

    $buildProbe = Invoke-RogGateFloorProbe -ProbeArguments @('-SelfTestBuildExit', '2')
    $buildExitOk = ($buildProbe.ExitCode -eq 2) -and ($buildProbe.Text -match 'Build failed \(exit 2\)') -and ($buildProbe.Text -match '\[ERROR\]')
    Assert-RogGateSelfTest -Name 'stubbed build failure exits with the build code' -Condition $buildExitOk

    function Invoke-RogGateExitWithNoise {
        'noise'
        0
    }
    function Invoke-RogGateExitOnly {
        param([Parameter(Mandatory = $true)][int]$Code)
        return $Code
    }
    $noisy = Invoke-RogGateExitWithNoise
    $noisyResolved = Resolve-RogGateCollectedExit -Code $noisy
    $noisyRejected = (-not $noisyResolved.Ok) -and ($noisyResolved.ExitCode -eq 1) -and ($noisyResolved.Detail -match 'type=') -and ($noisyResolved.Detail -match 'count=2')
    Assert-RogGateSelfTest -Name 'exit code with extra pipeline output is rejected' -Condition $noisyRejected
    $onlyZero = Invoke-RogGateExitOnly -Code 0
    $zeroResolved = Resolve-RogGateCollectedExit -Code $onlyZero
    Assert-RogGateSelfTest -Name 'single exit code 0 passes through unchanged' -Condition ($zeroResolved.Ok -and ($zeroResolved.ExitCode -eq 0) -and ($zeroResolved.ExitCode -is [int]))
    $onlyTwo = Invoke-RogGateExitOnly -Code 2
    $twoResolved = Resolve-RogGateCollectedExit -Code $onlyTwo
    Assert-RogGateSelfTest -Name 'single exit code 2 passes through unchanged' -Condition ($twoResolved.Ok -and ($twoResolved.ExitCode -eq 2) -and ($twoResolved.ExitCode -is [int]))

    $savedPlayDh = $env:MXH_PLAYDH_ROOT
    $playdhTemp = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath ('rog-gate-playdh-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Path $playdhTemp -Force | Out-Null
    try {
        $playdhSource = Join-Path -Path $playdhTemp -ChildPath 'src'
        New-Item -ItemType Directory -Path $playdhSource -Force | Out-Null
        $expectedPlayDh = Get-RogGatePlayDhRootPath -SourceDir $playdhSource
        Remove-Item -Path 'Env:MXH_PLAYDH_ROOT' -ErrorAction SilentlyContinue
        $autoPlayDh = Set-RogGatePlayDhRootForChildren -SourceDir $playdhSource
        $autoInfo = "[INFO] MXH_PLAYDH_ROOT=$expectedPlayDh (auto)"
        $autoOk = ($autoPlayDh.Path -eq $expectedPlayDh) -and ($env:MXH_PLAYDH_ROOT -eq $expectedPlayDh) -and ($autoPlayDh.Info -eq $autoInfo)
        Assert-RogGateSelfTest -Name 'unset MXH_PLAYDH_ROOT is set to the worktree PlayDH path' -Condition $autoOk

        $presetPlayDh = 'D:\not-the-worktree\PlayDH'
        $env:MXH_PLAYDH_ROOT = $presetPlayDh
        $overPlayDh = Set-RogGatePlayDhRootForChildren -SourceDir $playdhSource
        $overInfo = "[INFO] MXH_PLAYDH_ROOT=$expectedPlayDh (overrode '$presetPlayDh')"
        $overOk = ($overPlayDh.Path -eq $expectedPlayDh) -and ($env:MXH_PLAYDH_ROOT -eq $expectedPlayDh) -and ($overPlayDh.Path -ne $presetPlayDh) -and ($overPlayDh.Info -eq $overInfo)
        Assert-RogGateSelfTest -Name 'preset MXH_PLAYDH_ROOT is overridden to the SourceDir path' -Condition $overOk

        $missingPlayDh = Join-Path -Path $playdhTemp -ChildPath 'missing'
        $missingCheck = Test-RogGatePlayDhPreflight -PlayDhRoot $missingPlayDh
        $missingOk = (-not $missingCheck.Ok) -and ($missingCheck.Reason -match 'does not exist') -and ($missingCheck.Reason -match [regex]::Escape($missingPlayDh))
        Assert-RogGateSelfTest -Name 'missing PlayDH directory fails preflight' -Condition $missingOk

        $lfsRoot = Join-Path -Path $playdhTemp -ChildPath 'lfs'
        $lfsItemDir = Join-Path -Path $lfsRoot -ChildPath 'Resource'
        $lfsImageDir = Join-Path -Path (Join-Path -Path $lfsRoot -ChildPath 'Image') -ChildPath '2D'
        New-Item -ItemType Directory -Path $lfsItemDir -Force | Out-Null
        New-Item -ItemType Directory -Path $lfsImageDir -Force | Out-Null
        $lfsItem = Join-Path -Path $lfsItemDir -ChildPath 'ItemList.bin'
        $lfsLogin = Join-Path -Path $lfsImageDir -ChildPath 'login.dds'
        [System.IO.File]::WriteAllBytes($lfsItem, [byte[]](0x4D, 0x48, 0x00, 0x01))
        [System.IO.File]::WriteAllBytes($lfsLogin, [byte[]](0x44, 0x44, 0x53, 0x20))
        $realCheck = Test-RogGatePlayDhPreflight -PlayDhRoot $lfsRoot
        $pointer = "version https://git-lfs.github.com/spec/v1`noid sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa`nsize 4`n"
        $utf8 = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($lfsLogin, $pointer, $utf8)
        $pointerCheck = Test-RogGatePlayDhPreflight -PlayDhRoot $lfsRoot
        $lfsOk = $realCheck.Ok -and (-not $pointerCheck.Ok) -and ($pointerCheck.Reason -match 'LFS pointer') -and ($pointerCheck.Reason -match 'login\.dds')
        Assert-RogGateSelfTest -Name 'LFS-pointer login.dds fails PlayDH preflight' -Condition $lfsOk

        $ctestSample = @(
            'Test project D:/build-mxr/pr',
            '    Start 1: Alpha.One',
            '1/4 Test #1: Alpha.One .....................***Skipped   0.01 sec',
            '    Start 2: Beta.Two',
            '2/4 Test #2: Beta.Two ......................   Passed    0.01 sec',
            '    Start 9: Other.Fail',
            '3/4 Test #9: Other.Fail ....................***Failed    0.02 sec',
            '    Start 3: Gamma.Three',
            '4/4 Test #3: Gamma.Three ...................***Skipped   0.01 sec',
            '',
            '50% tests passed, 2 tests skipped out of 4',
            '',
            'The following tests FAILED:',
            '          9 - Other.Fail (Failed)',
            '',
            'The following tests did not run:',
            '          1 - Alpha.One (Skipped)',
            '          3 - Gamma.Three (Skipped)',
            ''
        ) -join "`n"
        $skipReport = Get-RogGateCTestSkipReport -Text $ctestSample
        $skipNames = (@($skipReport.Names) -join ',')
        $skipLines = (@($skipReport.Lines) -join "`n")
        $skipOk = ($skipReport.Count -eq 2) -and ($skipNames -eq 'Alpha.One,Gamma.Three') -and ($skipLines -match '\[INFO\] CTest skipped=2') -and ($skipLines -match 'CTest skip: Alpha\.One') -and ($skipLines -match 'CTest skip: Gamma\.Three') -and ($skipNames -notmatch 'Other\.Fail') -and ($skipNames -notmatch 'Beta\.Two')
        Assert-RogGateSelfTest -Name 'CTest skipped tests are counted and named' -Condition $skipOk
    } finally {
        if ([string]::IsNullOrEmpty($savedPlayDh)) {
            Remove-Item -Path 'Env:MXH_PLAYDH_ROOT' -ErrorAction SilentlyContinue
        } else {
            $env:MXH_PLAYDH_ROOT = $savedPlayDh
        }
        $null = Remove-RogGateFixture -LiteralPath $playdhTemp
    }

    function Measure-RogGateSelfTestExclude {
        param([string[]]$Arguments)
        $count = 0
        $value = ''
        $index = 0
        $items = @($Arguments)
        while ($index -lt $items.Count) {
            if ([string]$items[$index] -eq '-E') {
                $count = $count + 1
                if (($index + 1) -lt $items.Count) {
                    $value = [string]$items[$index + 1]
                }
            }
            $index = $index + 1
        }
        return [pscustomobject]@{
            Count = $count
            Value = $value
        }
    }

    function Test-RogGateSelfTestHasPair {
        param(
            [string[]]$Arguments,
            [Parameter(Mandatory = $true)][string]$Flag,
            [Parameter(Mandatory = $true)][string]$Value
        )
        $index = 0
        $items = @($Arguments)
        while ($index -lt ($items.Count - 1)) {
            if (([string]$items[$index] -eq $Flag) -and ([string]$items[$index + 1] -eq $Value)) {
                return $true
            }
            $index = $index + 1
        }
        return $false
    }

    $savedE2EBase = $env:MXH_E2E_PORT_BASE
    try {
        $happyPorts = Get-RogGateE2EPortsFromBase -BaseText '26001'
        $happyLine = Format-RogGateE2EPortsLine -Login $happyPorts.Login -Agent $happyPorts.Agent -Map $happyPorts.Map -Map12 $happyPorts.Map12
        $plusPorts = Get-RogGateE2EPortsFromBase -BaseText '+26001'
        $happyOk = $happyPorts.Ok -and ($happyPorts.Login -eq 26001) -and ($happyPorts.Agent -eq 27001) -and ($happyPorts.Map -eq 28001) -and ($happyPorts.Map12 -eq 28002) -and ($happyLine -eq '[INFO] E2E ports login=26001 agent=27001 map=28001 map12=28002') -and $plusPorts.Ok -and ($plusPorts.Login -eq 26001) -and ($plusPorts.Map12 -eq 28002)
        Assert-RogGateSelfTest -Name 'E2E base 26001 yields login 26001 agent 27001 map 28001 map12 28002' -Condition $happyOk

        $abcPorts = Get-RogGateE2EPortsFromBase -BaseText 'abc'
        $decimalPorts = Get-RogGateE2EPortsFromBase -BaseText '12.5'
        $emptyPorts = Get-RogGateE2EPortsFromBase -BaseText ''
        $minusPorts = Get-RogGateE2EPortsFromBase -BaseText '-1'
        $nonIntegerOk = (-not $abcPorts.Ok) -and ($abcPorts.Reason -eq 'MXH_E2E_PORT_BASE is not an integer: abc') -and (-not $decimalPorts.Ok) -and ($decimalPorts.Reason -eq 'MXH_E2E_PORT_BASE is not an integer: 12.5') -and (-not $emptyPorts.Ok) -and ($emptyPorts.Reason -match 'not an integer') -and (-not $minusPorts.Ok) -and ($minusPorts.Reason -match 'not an integer') -and ($abcPorts.Reason -notmatch 'outside') -and ($decimalPorts.Reason -notmatch 'forbidden')
        Assert-RogGateSelfTest -Name 'non-integer MXH_E2E_PORT_BASE is rejected' -Condition $nonIntegerOk

        $lowPorts = Get-RogGateE2EPortsFromBase -BaseText '80'
        $belowPorts = Get-RogGateE2EPortsFromBase -BaseText '1023'
        $floorPorts = Get-RogGateE2EPortsFromBase -BaseText '1024'
        $ceilingPorts = Get-RogGateE2EPortsFromBase -BaseText '63534'
        $abovePorts = Get-RogGateE2EPortsFromBase -BaseText '63535'
        $rangeOk = (-not $lowPorts.Ok) -and ($lowPorts.Reason -eq 'E2E port 80 is outside 1024-65535.') -and (-not $belowPorts.Ok) -and ($belowPorts.Reason -eq 'E2E port 1023 is outside 1024-65535.') -and $floorPorts.Ok -and ($floorPorts.Login -eq 1024) -and ($floorPorts.Map12 -eq 3025) -and $ceilingPorts.Ok -and ($ceilingPorts.Map12 -eq 65535) -and (-not $abovePorts.Ok) -and ($abovePorts.Reason -eq 'E2E port 65536 is outside 1024-65535.') -and ($lowPorts.Reason -notmatch 'forbidden')
        Assert-RogGateSelfTest -Name 'E2E ports outside 1024-65535 are rejected' -Condition $rangeOk

        $forbiddenOk = $happyPorts.Ok
        foreach ($forbiddenPort in @(16001, 17001, 18001, 36001, 37001, 38001)) {
            $forbiddenPorts = Get-RogGateE2EPortsFromBase -BaseText ([string]$forbiddenPort)
            $forbiddenReason = 'E2E port ' + [string]$forbiddenPort + ' is forbidden.'
            if ($forbiddenPorts.Ok -or ($forbiddenPorts.Reason -ne $forbiddenReason)) {
                $forbiddenOk = $false
            }
        }
        $agentForbidden = Get-RogGateE2EPortsFromBase -BaseText '15001'
        if ($agentForbidden.Ok -or ($agentForbidden.Reason -ne 'E2E port 16001 is forbidden.')) {
            $forbiddenOk = $false
        }
        Assert-RogGateSelfTest -Name 'forbidden E2E ports 16001 17001 18001 36001 37001 38001 are rejected' -Condition $forbiddenOk

        $null = Set-RogGateE2EPortBase
        $fromEnv = Get-RogGateE2EPortsFromBase -BaseText $env:MXH_E2E_PORT_BASE
        $setOk = ($env:MXH_E2E_PORT_BASE -eq '26001') -and $fromEnv.Ok -and ($fromEnv.Login -eq 26001) -and ($fromEnv.Agent -eq 27001) -and ($fromEnv.Map -eq 28001) -and ($fromEnv.Map12 -eq 28002)
        Assert-RogGateSelfTest -Name 'MXH_E2E_PORT_BASE is set to 26001' -Condition $setOk

        $listener = $null
        $started = $false
        $busyPort = 0
        $busyOk = $false
        try {
            $listener = New-Object System.Net.Sockets.TcpListener -ArgumentList ([System.Net.IPAddress]::Loopback, 0)
            $listener.Start()
            $started = $true
            $busyPort = [int]$listener.LocalEndpoint.Port
            $foundBusy = Get-RogGateFirstBusyLoopbackPort -Ports @($busyPort)
            $busyMessage = Format-RogGatePortInUseMessage -Port $busyPort
            $busyOk = ($foundBusy -eq $busyPort) -and ($busyMessage -eq ('port ' + [string]$busyPort + ' in use'))
        } catch {
            $busyOk = $false
        } finally {
            if ($started -and $null -ne $listener) {
                $listener.Stop()
            }
        }
        Assert-RogGateSelfTest -Name 'an occupied loopback port is reported in use' -Condition $busyOk

        $launcherText = @(
            '1: Test command: "D:/build/mxh_client_e2e_ctest.EXE" "D:/build/mxh_client_e2e.exe" --login-exe "D:/build/login.exe"',
            '1: Working Directory: D:/build-mxr/pr'
        ) -join "`n"
        $launcherListing = Get-RogGateE2EListing -Text $launcherText -ExitCode 0
        $launcherPlan = Resolve-RogGateE2ERegistration -Listing $launcherListing
        $launcherOk = $launcherListing.Registered -and $launcherListing.Launcher -and ($launcherListing.WorkingDirectory -eq 'D:/build-mxr/pr') -and ($launcherListing.Command -match 'mxh_client_e2e_ctest\.EXE') -and ($launcherPlan.Action -eq 'Run')
        Assert-RogGateSelfTest -Name 'mxh_client_e2e_ctest listing is the port launcher' -Condition $launcherOk

        $legacyText = '1: Test command: "D:/build/mxh_client_e2e.exe" --login-host 127.0.0.1'
        $legacyListing = Get-RogGateE2EListing -Text $legacyText -ExitCode 0
        $legacyError = Resolve-RogGateE2ERegistration -Listing $legacyListing
        $legacySkip = Resolve-RogGateE2ERegistration -Listing $legacyListing -SkipE2E
        $legacyOk = (-not $legacyListing.Launcher) -and ($legacyError.Action -eq 'Error') -and ($legacyError.Message -eq 'MoxianClientE2E is pre-#39 (no port launcher); refusing to run') -and ($legacySkip.Action -eq 'Skip') -and ($legacySkip.Message -eq '[WARN] E2E skipped by -SkipE2E')
        Assert-RogGateSelfTest -Name 'pre-#39 MoxianClientE2E is refused unless SkipE2E' -Condition $legacyOk

        $absentListing = Get-RogGateE2EListing -Text 'No tests were found!!!' -ExitCode 0
        $absentPlan = Resolve-RogGateE2ERegistration -Listing $absentListing
        $totalListing = Get-RogGateE2EListing -Text 'Total Tests: 0' -ExitCode 8
        $absentOk = $absentListing.Absent -and (-not $absentListing.Registered) -and ($absentPlan.Action -eq 'Absent') -and ($absentPlan.Message -eq '[INFO] MoxianClientE2E not registered') -and $totalListing.Absent
        Assert-RogGateSelfTest -Name 'missing MoxianClientE2E is not registered' -Condition $absentOk

        $parseListing = Get-RogGateE2EListing -Text 'ctest failed with no listing' -ExitCode 1
        $parsePlan = Resolve-RogGateE2ERegistration -Listing $parseListing
        $multiText = "1: Test command: C:/build/mxh_client_e2e_ctest.exe`n2: Test command: C:/build/other.exe"
        $multiListing = Get-RogGateE2EListing -Text $multiText -ExitCode 0
        $multiPlan = Resolve-RogGateE2ERegistration -Listing $multiListing
        $parseOk = (-not [string]::IsNullOrEmpty($parseListing.ParseError)) -and ($parsePlan.Action -eq 'Error') -and (-not [string]::IsNullOrEmpty($multiListing.ParseError)) -and ($multiPlan.Action -eq 'Error')
        Assert-RogGateSelfTest -Name 'an unreadable E2E listing is an error' -Condition $parseOk

        $mainNone = Get-RogGateMainCTestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ExtraArgs @()
        $noneExclude = Measure-RogGateSelfTestExclude -Arguments @($mainNone.Arguments)
        $noneOk = ($noneExclude.Count -eq 1) -and ($noneExclude.Value -eq '^MoxianClientE2E$')
        Assert-RogGateSelfTest -Name 'main CTest excludes only MoxianClientE2E' -Condition $noneOk

        $mainExtra = Get-RogGateMainCTestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ExtraArgs @('-R', 'Foo', '-E', 'Bar')
        $extraExclude = Measure-RogGateSelfTestExclude -Arguments @($mainExtra.Arguments)
        $extraKept = Test-RogGateSelfTestHasPair -Arguments @($mainExtra.Arguments) -Flag '-R' -Value 'Foo'
        $equalsForm = Get-RogGateMainCTestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ExtraArgs @('--exclude-regex=Bar')
        $equalsExclude = Measure-RogGateSelfTestExclude -Arguments @($equalsForm.Arguments)
        $extraOk = $extraKept -and ($extraExclude.Count -eq 1) -and ($extraExclude.Value -eq '(Bar)|(^MoxianClientE2E$)') -and ($equalsExclude.Count -eq 1) -and ($equalsExclude.Value -eq '(Bar)|(^MoxianClientE2E$)')
        Assert-RogGateSelfTest -Name 'main CTest merges -R and -E with the E2E exclusion' -Condition $extraOk

        $mainDup = Get-RogGateMainCTestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ExtraArgs @('-E', '^MoxianClientE2E$')
        $dupExclude = Measure-RogGateSelfTestExclude -Arguments @($mainDup.Arguments)
        $dupOk = ($dupExclude.Count -eq 1) -and ($dupExclude.Value -eq '^MoxianClientE2E$')
        Assert-RogGateSelfTest -Name 'an extra E2E exclusion is not duplicated' -Condition $dupOk

        $runPlan = Get-RogGateE2ECtestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ListOnly $false
        $runArgs = @($runPlan.Arguments)
        $runHasR = Test-RogGateSelfTestHasPair -Arguments $runArgs -Flag '-R' -Value '^MoxianClientE2E$'
        $runExclude = Measure-RogGateSelfTestExclude -Arguments $runArgs
        $runHasPort = $false
        foreach ($runArg in $runArgs) {
            $runText = [string]$runArg
            if ($runText -eq '--login-port' -or $runText -eq '--agent-port' -or $runText -eq '--map-port' -or $runText -eq '--map12-port') {
                $runHasPort = $true
            }
        }
        $listPlan = Get-RogGateE2ECtestArguments -BuildDir 'D:\build-mxr\pr' -ConfigName 'Debug' -ListOnly $true
        $listArgs = @($listPlan.Arguments)
        $listHasN = $false
        $listHasV = $false
        foreach ($listArg in $listArgs) {
            if ([string]$listArg -eq '-N') { $listHasN = $true }
            if ([string]$listArg -eq '-V') { $listHasV = $true }
        }
        $listHasR = Test-RogGateSelfTestHasPair -Arguments $listArgs -Flag '-R' -Value '^MoxianClientE2E$'
        $runOk = $runHasR -and ($runExclude.Count -eq 0) -and (-not $runHasPort) -and $listHasN -and $listHasV -and $listHasR
        Assert-RogGateSelfTest -Name 'the separate E2E ctest uses -R and does not append port flags' -Condition $runOk
    } finally {
        if ([string]::IsNullOrEmpty($savedE2EBase)) {
            Remove-Item -Path 'Env:MXH_E2E_PORT_BASE' -ErrorAction SilentlyContinue
        } else {
            $env:MXH_E2E_PORT_BASE = $savedE2EBase
        }
    }

    function Initialize-RogGateSelfTestRepo {
        param(
            [Parameter(Mandatory = $true)][string]$Path,
            [bool]$Sparse
        )
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        $previous = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $null = & git -C $Path init -q 2>&1
            if ($LASTEXITCODE -ne 0) {
                return $false
            }
            if (-not $Sparse) {
                return $true
            }
            $null = & git -C $Path sparse-checkout set --no-cone '*' 2>&1
            return ($LASTEXITCODE -eq 0)
        } finally {
            $ErrorActionPreference = $previous
        }
    }
    function Invoke-RogGateSelfTestGit {
        param(
            [Parameter(Mandatory = $true)][string]$Worktree,
            [Parameter(Mandatory = $true)][string[]]$GitArguments
        )
        $previous = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $null = & git -C $Worktree @GitArguments 2>&1
            $code = $LASTEXITCODE
            if ($null -eq $code) {
                $code = 1
            }
            return ([int]$code -eq 0)
        } finally {
            $ErrorActionPreference = $previous
        }
    }
    function New-RogGateSelfTestReparseLink {
        param(
            [Parameter(Mandatory = $true)][string]$LinkPath,
            [Parameter(Mandatory = $true)][string]$TargetPath
        )
        New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
        $innerTarget = Join-Path -Path $TargetPath -ChildPath 'inner-target'
        $innerLink = Join-Path -Path $TargetPath -ChildPath 'inner-link'
        $inner = New-RogGateSelfTestDirectoryLink -LinkPath $innerLink -TargetPath $innerTarget
        Set-Content -LiteralPath (Join-Path -Path $TargetPath -ChildPath 'marker.txt') -Value 'keep' -Encoding ASCII
        $outer = New-RogGateSelfTestDirectoryLink -LinkPath $LinkPath -TargetPath $TargetPath
        $linkType = [string]$outer.LinkType
        return [pscustomobject]@{
            Ok       = [bool]($inner.Ok -and $outer.Ok)
            Created  = [bool]$outer.Created
            LinkType = $linkType
        }
    }
    function Format-RogGateSelfTestLinkKind {
        param([string]$LinkType)
        if ([string]::IsNullOrWhiteSpace($LinkType)) {
            return 'missing'
        }
        return $LinkType
    }
    function Test-RogGatePreflightAllows {
        param([Parameter(Mandatory = $true)][string]$Path)
        $threw = $false
        try {
            $null = Assert-RogGateSparseReparsePreflight -SourceDir $Path
        } catch {
            $threw = $true
        }
        return (-not $threw)
    }

    $preflightRoot = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath ('rog-gate-sparse-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Path $preflightRoot | Out-Null
    try {
        $neither = Join-Path -Path $preflightRoot -ChildPath 'neither'
        $neitherReady = Initialize-RogGateSelfTestRepo -Path $neither -Sparse $false
        $neitherState = Get-RogGateSparseCheckoutState -Worktree $neither
        $neitherReparse = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $neither)
        $neitherPass = $neitherReady -and (-not $neitherState.Active) -and ($neitherReparse.Count -eq 0) -and (Test-RogGatePreflightAllows -Path $neither)
        Assert-RogGateSelfTest -Name 'neither sparse checkout nor a junction passes (link=none)' -Condition $neitherPass

        $sparseOnly = Join-Path -Path $preflightRoot -ChildPath 'sparse-only'
        $sparseReady = Initialize-RogGateSelfTestRepo -Path $sparseOnly -Sparse $true
        $sparseState = Get-RogGateSparseCheckoutState -Worktree $sparseOnly
        $sparseReparse = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $sparseOnly)
        $sparsePass = $sparseReady -and $sparseState.Active -and ($sparseState.SparseState -match 'core.sparseCheckout=true') -and ($sparseReparse.Count -eq 0) -and (Test-RogGatePreflightAllows -Path $sparseOnly)
        Assert-RogGateSelfTest -Name 'sparse checkout alone passes (link=none)' -Condition $sparsePass

        $junctionOnly = Join-Path -Path $preflightRoot -ChildPath 'junction-only'
        $junctionTarget = Join-Path -Path $preflightRoot -ChildPath 'junction-target'
        $junctionReady = Initialize-RogGateSelfTestRepo -Path $junctionOnly -Sparse $false
        $junctionLink = Join-Path -Path $junctionOnly -ChildPath 'jump'
        $junctionResult = New-RogGateSelfTestReparseLink -LinkPath $junctionLink -TargetPath $junctionTarget
        $junctionLinked = [bool]$junctionResult.Ok
        $junctionKind = Format-RogGateSelfTestLinkKind -LinkType $junctionResult.LinkType
        $junctionState = Get-RogGateSparseCheckoutState -Worktree $junctionOnly
        $junctionReparse = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $junctionOnly)
        $junctionSawLink = $false
        $junctionFollowed = $false
        foreach ($reparsePath in $junctionReparse) {
            if ($reparsePath -eq (Get-Item -LiteralPath $junctionLink -Force).FullName) {
                $junctionSawLink = $true
            }
            if ([string]$reparsePath -match 'inner-link') {
                $junctionFollowed = $true
            }
        }
        $junctionPass = $junctionReady -and $junctionLinked -and ($junctionKind -eq $directoryLinkKind) -and (-not $junctionState.Active) -and $junctionSawLink -and (-not $junctionFollowed) -and (Test-RogGatePreflightAllows -Path $junctionOnly)
        Assert-RogGateSelfTest -Name ("junction alone passes (link={0})" -f $junctionKind) -Condition $junctionPass

        $both = Join-Path -Path $preflightRoot -ChildPath 'sparse-junction'
        $bothTarget = Join-Path -Path $preflightRoot -ChildPath 'sparse-junction-target'
        $bothReady = Initialize-RogGateSelfTestRepo -Path $both -Sparse $true
        $bothLink = Join-Path -Path $both -ChildPath 'jump'
        $bothResult = New-RogGateSelfTestReparseLink -LinkPath $bothLink -TargetPath $bothTarget
        $bothLinked = [bool]$bothResult.Ok
        $bothKind = Format-RogGateSelfTestLinkKind -LinkType $bothResult.LinkType
        $bothState = Get-RogGateSparseCheckoutState -Worktree $both
        $bothReparse = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $both)
        $bothMessage = ''
        $bothThrew = $false
        try {
            $null = Assert-RogGateSparseReparsePreflight -SourceDir $both
        } catch {
            $bothThrew = $true
            $bothMessage = $_.Exception.Message
        }
        $bothNamesPaths = ($bothReparse.Count -gt 0)
        $bothFollowed = $false
        foreach ($reparsePath in $bothReparse) {
            if ($bothMessage -notmatch [regex]::Escape([string]$reparsePath)) {
                $bothNamesPaths = $false
            }
            if ([string]$reparsePath -match 'inner-link') {
                $bothFollowed = $true
            }
        }
        $bothRejected = $bothReady -and $bothLinked -and ($bothKind -eq $directoryLinkKind) -and $bothState.Active -and ($bothState.SparseState -match 'core.sparseCheckout=true') -and $bothThrew -and ($bothMessage -match 'sparse checkout is enabled') -and $bothNamesPaths -and (-not $bothFollowed) -and ($bothMessage -notmatch 'inner-link')
        Assert-RogGateSelfTest -Name ("sparse checkout plus a junction is rejected (link={0})" -f $bothKind) -Condition $bothRejected

        $worktreeBase = Join-Path -Path $preflightRoot -ChildPath 'worktree'
        $worktreeMain = Join-Path -Path $worktreeBase -ChildPath 'main'
        $worktreeDir = Join-Path -Path $worktreeBase -ChildPath 'wt'
        $worktreeTarget = Join-Path -Path $worktreeBase -ChildPath 'wt-target'
        $worktreeReady = Initialize-RogGateSelfTestRepo -Path $worktreeMain -Sparse $false
        Set-Content -LiteralPath (Join-Path -Path $worktreeMain -ChildPath 'README') -Value 'readme' -Encoding ASCII
        $worktreeConfigured = (Invoke-RogGateSelfTestGit -Worktree $worktreeMain -GitArguments @('config', 'user.email', 'rog-gate@example.com')) -and (Invoke-RogGateSelfTestGit -Worktree $worktreeMain -GitArguments @('config', 'user.name', 'rog-gate'))
        $worktreeCommitted = $worktreeConfigured -and (Invoke-RogGateSelfTestGit -Worktree $worktreeMain -GitArguments @('add', 'README')) -and (Invoke-RogGateSelfTestGit -Worktree $worktreeMain -GitArguments @('-c', 'commit.gpgsign=false', 'commit', '-q', '-m', 'init'))
        $worktreeAdded = $worktreeCommitted -and (Invoke-RogGateSelfTestGit -Worktree $worktreeMain -GitArguments @('worktree', 'add', '--detach', $worktreeDir, 'HEAD'))
        $worktreeSparse = $worktreeAdded -and (Invoke-RogGateSelfTestGit -Worktree $worktreeDir -GitArguments @('sparse-checkout', 'set', '--no-cone', '*'))
        $worktreeDotGit = Join-Path -Path $worktreeDir -ChildPath '.git'
        $worktreeDotGitFile = $false
        $worktreeDotGitText = ''
        if (Test-Path -LiteralPath $worktreeDotGit) {
            $dotGitItem = Get-Item -LiteralPath $worktreeDotGit -Force
            $dotGitIsDir = (Test-RogGateAttributeHasDirectory -Attributes $dotGitItem.Attributes)
            $worktreeDotGitFile = (-not $dotGitIsDir) -and (-not (Test-RogGateItemReparsePoint -Item $dotGitItem))
            if ($worktreeDotGitFile) {
                $worktreeDotGitText = [System.IO.File]::ReadAllText($dotGitItem.FullName)
            }
        }
        $worktreeLink = Join-Path -Path $worktreeDir -ChildPath 'jump'
        $worktreeLinked = $false
        $worktreeKind = 'missing'
        if ($worktreeSparse) {
            $worktreeResult = New-RogGateSelfTestReparseLink -LinkPath $worktreeLink -TargetPath $worktreeTarget
            $worktreeLinked = [bool]$worktreeResult.Ok
            $worktreeKind = Format-RogGateSelfTestLinkKind -LinkType $worktreeResult.LinkType
        }
        $worktreeGitDir = ''
        if ($worktreeAdded) {
            $resolvedGitDir = Get-RogGateGitDirectory -Worktree $worktreeDir
            if ($null -ne $resolvedGitDir) {
                $worktreeGitDir = [string]$resolvedGitDir
            }
        }
        $worktreeLinkFull = ''
        if ($worktreeLinked) {
            $worktreeLinkFull = (Get-Item -LiteralPath $worktreeLink -Force).FullName
        }
        $worktreeMessage = ''
        $worktreeThrew = $false
        if ($worktreeLinked) {
            try {
                $null = Assert-RogGateSparseReparsePreflight -SourceDir $worktreeDir
            } catch {
                $worktreeThrew = $true
                $worktreeMessage = $_.Exception.Message
            }
        }
        $worktreeRejected = $worktreeReady -and $worktreeSparse -and $worktreeDotGitFile -and ($worktreeDotGitText -match '(?m)^\s*gitdir:') -and $worktreeLinked -and ($worktreeKind -eq $directoryLinkKind) -and $worktreeThrew -and ($worktreeMessage -match 'core.sparseCheckout=true') -and ($worktreeLinkFull.Length -gt 0) -and ($worktreeMessage -match [regex]::Escape($worktreeLinkFull)) -and ($worktreeMessage -match 'worktrees') -and ($worktreeMessage -notmatch 'inner-link') -and ($worktreeGitDir -match 'worktrees')
        Assert-RogGateSelfTest -Name ("sparse worktree whose .git file is a gitdir pointer is rejected when it contains a junction (link={0})" -f $worktreeKind) -Condition $worktreeRejected

        $bothProbe = Invoke-RogGateFloorProbe -ProbeArguments @('-SourceDir', $both, '-BuildDir', 'D:\build-mxr\pr', '-ExpectedSha', $floorSha)
        $bothLinkFull = ''
        if ($bothLinked) {
            $bothLinkFull = (Get-Item -LiteralPath $bothLink -Force).FullName
        }
        $markerPath = Join-Path -Path $bothTarget -ChildPath 'marker.txt'
        $markerText = ''
        if (Test-Path -LiteralPath $markerPath) {
            $markerText = (Get-Content -LiteralPath $markerPath -Raw).Trim()
        }
        $bothProbeRejected = $bothLinked -and ($bothKind -eq $directoryLinkKind) -and ($bothLinkFull.Length -gt 0) -and ($bothProbe.ExitCode -eq 1) -and ($bothProbe.Text -match 'sparse checkout is enabled') -and ($bothProbe.Text -match [regex]::Escape($bothLinkFull)) -and ($bothProbe.Text -match 'core.sparseCheckout=true') -and ($bothProbe.Text -notmatch 'rev-parse') -and ($bothProbe.Text -notmatch 'inner-link') -and ($markerText -eq 'keep')
        Assert-RogGateSelfTest -Name ("sparse plus junction exits 1 before git rev-parse (link={0})" -f $bothKind) -Condition $bothProbeRejected

        if (Test-RogGateHostIsWindows) {
            $mixed = Join-Path -Path $preflightRoot -ChildPath 'sparse-mixed'
            $mixedJunctionTarget = Join-Path -Path $preflightRoot -ChildPath 'sparse-mixed-junction-target'
            $mixedSymlinkTarget = Join-Path -Path $preflightRoot -ChildPath 'sparse-mixed-symlink-target'
            $mixedReady = Initialize-RogGateSelfTestRepo -Path $mixed -Sparse $true
            $mixedJunction = Join-Path -Path $mixed -ChildPath 'jump-junction'
            $mixedJunctionResult = New-RogGateSelfTestReparseLink -LinkPath $mixedJunction -TargetPath $mixedJunctionTarget
            $mixedJunctionKind = Format-RogGateSelfTestLinkKind -LinkType $mixedJunctionResult.LinkType
            $mixedName = 'windows sparse checkout with a junction and a directory symlink'
            if (-not ($mixedReady -and $mixedJunctionResult.Ok -and ($mixedJunctionKind -eq 'Junction'))) {
                Assert-RogGateSelfTest -Name ("{0} (link={1})" -f $mixedName, $mixedJunctionKind) -Condition $false
            } else {
                $mixedInnerTarget = Join-Path -Path $mixedSymlinkTarget -ChildPath 'inner-target'
                $mixedInnerLink = Join-Path -Path $mixedSymlinkTarget -ChildPath 'inner-link'
                $mixedInner = New-RogGateSelfTestDirectoryLink -LinkPath $mixedInnerLink -TargetPath $mixedInnerTarget -Kind Junction
                $mixedSymlink = Join-Path -Path $mixed -ChildPath 'jump-symlink'
                $mixedSymlinkResult = New-RogGateSelfTestDirectoryLink -LinkPath $mixedSymlink -TargetPath $mixedSymlinkTarget -Kind SymbolicLink
                $mixedSymlinkKind = Format-RogGateSelfTestLinkKind -LinkType $mixedSymlinkResult.LinkType
                if (-not $mixedInner.Ok) {
                    Assert-RogGateSelfTest -Name ("{0} (link={1}+{2})" -f $mixedName, $mixedJunctionKind, $mixedSymlinkKind) -Condition $false
                } elseif (-not $mixedSymlinkResult.Created) {
                    Write-RogGateSelfTestSkip -Name ("{0} (directory symlink not permitted)" -f $mixedName)
                } elseif ($mixedSymlinkResult.LinkType -ne 'SymbolicLink') {
                    Assert-RogGateSelfTest -Name ("{0} (link={1}+{2})" -f $mixedName, $mixedJunctionKind, $mixedSymlinkKind) -Condition $false
                } else {
                    $mixedState = Get-RogGateSparseCheckoutState -Worktree $mixed
                    $mixedReparse = @(Get-RogGateWorkingTreeReparsePoints -LiteralPath $mixed)
                    $mixedMessage = ''
                    $mixedThrew = $false
                    try {
                        $null = Assert-RogGateSparseReparsePreflight -SourceDir $mixed
                    } catch {
                        $mixedThrew = $true
                        $mixedMessage = $_.Exception.Message
                    }
                    $mixedJunctionFull = (Get-Item -LiteralPath $mixedJunction -Force).FullName
                    $mixedSymlinkFull = (Get-Item -LiteralPath $mixedSymlink -Force).FullName
                    $mixedNames = ($mixedMessage -match [regex]::Escape($mixedJunctionFull)) -and ($mixedMessage -match [regex]::Escape($mixedSymlinkFull))
                    $mixedFollowed = $false
                    foreach ($reparsePath in $mixedReparse) {
                        if ([string]$reparsePath -match 'inner-link') {
                            $mixedFollowed = $true
                        }
                    }
                    $mixedOk = $mixedState.Active -and $mixedThrew -and ($mixedMessage -match 'sparse checkout is enabled') -and ($mixedMessage -match 'core.sparseCheckout=true') -and $mixedNames -and (-not $mixedFollowed) -and ($mixedMessage -notmatch 'inner-link')
                    Assert-RogGateSelfTest -Name ("{0} (link=Junction+SymbolicLink)" -f $mixedName) -Condition $mixedOk
                }
            }
        }
    } finally {
        $null = Remove-RogGateFixture -LiteralPath $preflightRoot
    }

    $required = [int64]10GB
    $enough = @{ D = $required; E = $required }
    $passReport = Get-RogGateFreeSpaceReport -Drives @('D', 'E') -FreeBytesByDrive $enough -MinFreeGB 10 -Overrides @{}
    Assert-RogGateSelfTest -Name 'free space equal to the threshold passes' -Condition $passReport.Ok

    $low = @{ D = $required; E = ($required - 1) }
    $lowReport = Get-RogGateFreeSpaceReport -Drives @('D', 'E') -FreeBytesByDrive $low -MinFreeGB 10 -Overrides @{}
    $eLow = $false
    $dOk = $false
    foreach ($row in @($lowReport.Rows)) {
        if ($row.Drive -eq 'E') { $eLow = -not $row.Ok }
        if ($row.Drive -eq 'D') { $dOk = [bool]$row.Ok }
    }
    Assert-RogGateSelfTest -Name 'one byte under the threshold fails that drive only' -Condition ((-not $lowReport.Ok) -and $eLow -and $dOk)

    $missingFree = Get-RogGateFreeSpaceReport -Drives @('E') -FreeBytesByDrive @{} -MinFreeGB 10 -Overrides @{}
    Assert-RogGateSelfTest -Name 'missing free-space reading fails closed' -Condition (-not $missingFree.Ok)

    $fresh = Get-RogGateCleanDecision -BuildDirExists $false -HasStamp $false -StampSha '' -ExpectedSha ('a' * 40) -HasCache $false -CacheGenerator '' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'missing build dir is a clean configure without a delete' -Condition ($fresh.Clean -and -not $fresh.ShouldDelete)

    $sha = 'a' * 40
    $stampMiss = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $false -StampSha '' -ExpectedSha $sha -HasCache $false -CacheGenerator '' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'missing stamp deletes an existing directory' -Condition ($stampMiss.Clean -and $stampMiss.ShouldDelete -and ($stampMiss.Reason -match 'stamp missing'))

    $incremental = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $true -StampSha $sha.ToUpperInvariant() -ExpectedSha $sha -HasCache $true -CacheGenerator 'NMake Makefiles' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'matching stamp and NMake generator stay incremental' -Condition ((-not $incremental.Clean) -and -not $incremental.ShouldDelete)

    $genMismatch = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $true -StampSha $sha -ExpectedSha $sha -HasCache $true -CacheGenerator 'Ninja' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'generator mismatch cleans even when the SHA matches' -Condition ($genMismatch.Clean -and $genMismatch.ShouldDelete -and ($genMismatch.Reason -match 'CMAKE_GENERATOR'))

    $shaMismatch = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $true -StampSha ('b' * 40) -ExpectedSha $sha -HasCache $true -CacheGenerator 'NMake Makefiles' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'SHA mismatch cleans even when the generator matches' -Condition ($shaMismatch.Clean -and $shaMismatch.ShouldDelete)

    $blankGen = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $true -StampSha $sha -ExpectedSha $sha -HasCache $true -CacheGenerator '' -RequestedGenerator 'Ninja'
    Assert-RogGateSelfTest -Name 'cache without CMAKE_GENERATOR cleans' -Condition ($blankGen.Clean -and ($blankGen.Reason -match 'no CMAKE_GENERATOR'))

    $noCache = Get-RogGateCleanDecision -BuildDirExists $true -HasStamp $true -StampSha $sha -ExpectedSha $sha -HasCache $false -CacheGenerator '' -RequestedGenerator 'NMake Makefiles'
    Assert-RogGateSelfTest -Name 'absent cache does not by itself force a clean' -Condition (-not $noCache.Clean)

    $quoted = ConvertTo-RogGateCmdArgument -Argument 'NMake Makefiles'
    $quotedPct = ConvertTo-RogGateCmdArgument -Argument 'a%b'
    Assert-RogGateSelfTest -Name 'cmd quoting wraps generator and doubles percent' -Condition (($quoted -eq '"NMake Makefiles"') -and ($quotedPct -eq '"a%%b"'))

    $addTestLine = 'add_test([=[Foo.Bar]=] "D:/build-mxr/pr20/foo_tests.exe")'
    $registered = @(Get-RogGateAddTestExecutables -Line $addTestLine -NormalizedBuildDir 'D:\build-mxr\pr20')
    $outsideExe = @(Get-RogGateAddTestExecutables -Line 'add_test(Other "D:/MX/other_tests.exe")' -NormalizedBuildDir 'D:\build-mxr\pr20')
    $ignored = @(Get-RogGateAddTestExecutables -Line 'set_tests_properties([=[Foo]=] PROPERTIES WORKING_DIRECTORY "D:/build-mxr/pr20/foo_tests.exe")' -NormalizedBuildDir 'D:\build-mxr\pr20')
    $registeredOk = ($registered.Count -eq 1) -and ($registered[0] -eq 'D:\build-mxr\pr20\foo_tests.exe') -and ($outsideExe.Count -eq 0) -and ($ignored.Count -eq 0)
    Assert-RogGateSelfTest -Name 'CTest add_test executables stay inside BuildDir' -Condition $registeredOk

    $fixture = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath ('rog-gate-selftest-' + [guid]::NewGuid().ToString('n'))
    New-Item -ItemType Directory -Path $fixture | Out-Null
    try {
        $cacheDir = Join-Path -Path $fixture -ChildPath 'cache'
        New-Item -ItemType Directory -Path $cacheDir | Out-Null
        $cacheBody = @(
            '# cmake cache',
            'CMAKE_GENERATOR_INSTANCE:INTERNAL=',
            'CMAKE_GENERATOR:INTERNAL=NMake Makefiles',
            'CMAKE_GENERATOR_TOOLSET:INTERNAL='
        )
        Set-Content -LiteralPath (Join-Path -Path $cacheDir -ChildPath 'CMakeCache.txt') -Value $cacheBody -Encoding ASCII
        $parsed = Get-RogGateCMakeGenerator -BuildDirectory $cacheDir
        Assert-RogGateSelfTest -Name 'parse CMAKE_GENERATOR without matching the toolset keys' -Condition ($parsed.HasCache -and ($parsed.Generator -eq 'NMake Makefiles'))

        $ninjaDir = Join-Path -Path $fixture -ChildPath 'ninja-cache'
        New-Item -ItemType Directory -Path $ninjaDir | Out-Null
        Set-Content -LiteralPath (Join-Path -Path $ninjaDir -ChildPath 'CMakeCache.txt') -Value @('CMAKE_GENERATOR:INTERNAL=Ninja') -Encoding ASCII
        $ninjaParsed = Get-RogGateCMakeGenerator -BuildDirectory $ninjaDir
        Assert-RogGateSelfTest -Name 'parse Ninja generator' -Condition ($ninjaParsed.Generator -eq 'Ninja')

        $emptyDir = Join-Path -Path $fixture -ChildPath 'no-cache'
        New-Item -ItemType Directory -Path $emptyDir | Out-Null
        $noParsed = Get-RogGateCMakeGenerator -BuildDirectory $emptyDir
        Assert-RogGateSelfTest -Name 'missing CMakeCache is not a generator mismatch by itself' -Condition (-not $noParsed.HasCache)

        $stampDir = Join-Path -Path $fixture -ChildPath 'stamp'
        New-Item -ItemType Directory -Path $stampDir | Out-Null
        $null = Write-RogGateStamp -BuildDirectory $stampDir -Sha $sha -GeneratorName 'NMake Makefiles' -ConfigName 'Debug'
        $stampRead = Read-RogGateStampSha -BuildDirectory $stampDir
        Assert-RogGateSelfTest -Name 'stamp round-trip keeps the SHA on the first line' -Condition ($stampRead.HasStamp -and ($stampRead.Sha -eq $sha))

        $plain = Join-Path -Path $fixture -ChildPath 'plain'
        $nested = Join-Path -Path $plain -ChildPath 'sub'
        New-Item -ItemType Directory -Path $nested | Out-Null
        $plainFile = Join-Path -Path $nested -ChildPath 'a.txt'
        [System.IO.File]::WriteAllBytes($plainFile, (New-Object byte[] 32))
        $plainItem = Get-Item -LiteralPath $plainFile -Force
        $plainBits = Get-RogGateAttributeValue -Attributes $plainItem.Attributes
        $null = Set-RogGateRawAttributes -LiteralPath $plainFile -Value ($plainBits -bor $script:RogGateAttributeReadOnly)
        $sibling = Join-Path -Path $fixture -ChildPath 'keep.txt'
        [System.IO.File]::WriteAllText($sibling, 'keep')
        $null = Remove-RogGateDirectoryNoFollow -LiteralPath $plain
        Assert-RogGateSelfTest -Name 'no-follow delete removes a normal tree and keeps its sibling' -Condition ((-not (Test-Path -LiteralPath $plain)) -and (Test-Path -LiteralPath $sibling))

        $outside = Join-Path -Path $fixture -ChildPath 'outside'
        New-Item -ItemType Directory -Path $outside | Out-Null
        $marker = Join-Path -Path $outside -ChildPath 'marker.txt'
        [System.IO.File]::WriteAllText($marker, 'keep-me')
        $linkedBuild = Join-Path -Path $fixture -ChildPath 'linked-build'
        New-Item -ItemType Directory -Path $linkedBuild | Out-Null
        $link = Join-Path -Path $linkedBuild -ChildPath 'jump'
        $dirLink = New-RogGateSelfTestDirectoryLink -LinkPath $link -TargetPath $outside
        $dirLinkKind = Format-RogGateSelfTestLinkKind -LinkType $dirLink.LinkType
        if (-not $dirLink.Ok) {
            Assert-RogGateSelfTest -Name ("create a directory reparse point (link={0})" -f $dirLinkKind) -Condition $false
        } else {
            $violations = @(Get-RogGateReparseViolations -LiteralPath $linkedBuild)
            $sawLink = $false
            foreach ($violation in $violations) {
                if ($violation -eq $link) {
                    $sawLink = $true
                }
            }
            Assert-RogGateSelfTest -Name ("directory reparse point is reported and not descended into (link={0})" -f $dirLinkKind) -Condition ($sawLink -and (Test-Path -LiteralPath $marker) -and ($dirLinkKind -eq $directoryLinkKind))

            $fileLink = Join-Path -Path $linkedBuild -ChildPath 'file.link'
            $fileLinkOk = $true
            try {
                New-Item -ItemType SymbolicLink -Path $fileLink -Target $marker -ErrorAction Stop | Out-Null
            } catch {
                $fileLinkOk = $false
            }
            if ($fileLinkOk) {
                $fileViolations = @(Get-RogGateReparseViolations -LiteralPath $linkedBuild)
                $sawFile = $false
                foreach ($violation in $fileViolations) {
                    if ($violation -eq $fileLink) {
                        $sawFile = $true
                    }
                }
                Assert-RogGateSelfTest -Name 'file symlink inside the tree is reported' -Condition $sawFile
            }

            $deleteThrew = $false
            try {
                $null = Remove-RogGateDirectoryNoFollow -LiteralPath $linkedBuild
            } catch {
                $deleteThrew = $true
            }
            $markerText = ''
            if (Test-Path -LiteralPath $marker) {
                $markerText = [System.IO.File]::ReadAllText($marker)
            }
            Assert-RogGateSelfTest -Name 'delete refuses the symlink and leaves the target intact' -Condition ($deleteThrew -and ($markerText -eq 'keep-me') -and (Test-Path -LiteralPath $outside))

            $real = Join-Path -Path $fixture -ChildPath 'real'
            $realChild = Join-Path -Path $real -ChildPath 'child'
            New-Item -ItemType Directory -Path $realChild | Out-Null
            $ancestorLink = Join-Path -Path $fixture -ChildPath 'ancestor-link'
            $ancestorLinkResult = New-RogGateSelfTestDirectoryLink -LinkPath $ancestorLink -TargetPath $real
            $ancestorKind = Format-RogGateSelfTestLinkKind -LinkType $ancestorLinkResult.LinkType
            $sawAncestor = $false
            if ($ancestorLinkResult.Ok) {
                $via = Join-Path -Path $ancestorLink -ChildPath 'child'
                $ancestorHits = @(Get-RogGateReparseViolations -LiteralPath $via)
                foreach ($hit in $ancestorHits) {
                    if ($hit -eq $ancestorLink) {
                        $sawAncestor = $true
                    }
                }
            }
            Assert-RogGateSelfTest -Name ("ancestor reparse point is refused before descent (link={0})" -f $ancestorKind) -Condition ($ancestorLinkResult.Ok -and $sawAncestor -and ($ancestorKind -eq $directoryLinkKind))
        }

        $sizeRoot = Join-Path -Path $fixture -ChildPath 'sizes'
        $big = Join-Path -Path $sizeRoot -ChildPath 'big'
        $small = Join-Path -Path $sizeRoot -ChildPath 'small'
        $sizeOutside = Join-Path -Path $fixture -ChildPath 'size-outside'
        New-Item -ItemType Directory -Path $big | Out-Null
        New-Item -ItemType Directory -Path $small | Out-Null
        New-Item -ItemType Directory -Path $sizeOutside | Out-Null
        [System.IO.File]::WriteAllBytes((Join-Path -Path $big -ChildPath 'data.bin'), (New-Object byte[] 4000))
        [System.IO.File]::WriteAllBytes((Join-Path -Path $small -ChildPath 'data.bin'), (New-Object byte[] 50))
        [System.IO.File]::WriteAllBytes((Join-Path -Path $sizeOutside -ChildPath 'huge.bin'), (New-Object byte[] 20000))
        [System.IO.File]::WriteAllBytes((Join-Path -Path $sizeRoot -ChildPath 'readme.txt'), (New-Object byte[] 10000))
        $sizeJump = Join-Path -Path $sizeRoot -ChildPath 'jump'
        $sizeJumpLink = New-RogGateSelfTestDirectoryLink -LinkPath $sizeJump -TargetPath $sizeOutside
        $sizeSmallLink = New-RogGateSelfTestDirectoryLink -LinkPath (Join-Path -Path $small -ChildPath 'out') -TargetPath $sizeOutside
        $sizeKind = '{0}+{1}' -f (Format-RogGateSelfTestLinkKind -LinkType $sizeJumpLink.LinkType), (Format-RogGateSelfTestLinkKind -LinkType $sizeSmallLink.LinkType)
        $sizeLinkOk = $sizeJumpLink.Ok -and $sizeSmallLink.Ok -and ($sizeJumpLink.LinkType -eq $directoryLinkKind) -and ($sizeSmallLink.LinkType -eq $directoryLinkKind)
        if ($sizeLinkOk) {
            $sizes = @(Get-RogGateTopLevelDirectorySizes -RootPath $sizeRoot)
            $sawReadme = $false
            $bigBytes = [int64]-1
            $smallBytes = [int64]-1
            $jumpBytes = [int64]0
            $jumpReparse = $false
            $smallReparse = $false
            foreach ($row in $sizes) {
                if ($row.Path -like '*readme.txt') { $sawReadme = $true }
                if ($row.Path -like '*\big' -or $row.Path -like '*/big') {
                    $bigBytes = [int64]$row.Bytes
                }
                if ($row.Path -like '*\small' -or $row.Path -like '*/small') {
                    $smallBytes = [int64]$row.Bytes
                    $smallReparse = [bool]$row.Reparse
                }
                if ($row.Path -like '*\jump' -or $row.Path -like '*/jump') {
                    $jumpReparse = [bool]$row.Reparse
                    if ($null -eq $row.Bytes) {
                        $jumpBytes = [int64]-1
                    } else {
                        $jumpBytes = [int64]$row.Bytes
                    }
                }
            }
            $ordered = $false
            if ($sizes.Count -ge 1) {
                $ordered = ($sizes[0].Path -like '*big')
            }
            $sizeOk = (-not $sawReadme) -and $ordered -and ($bigBytes -eq 4000) -and ($smallBytes -eq 50) -and $smallReparse -and $jumpReparse -and ($jumpBytes -eq -1)
            Assert-RogGateSelfTest -Name ("top-level sizes skip files and do not follow directory links (link={0})" -f $sizeKind) -Condition $sizeOk
        } else {
            Assert-RogGateSelfTest -Name ("top-level sizes skip files and do not follow directory links (link={0})" -f $sizeKind) -Condition $false
        }

        $reparseRoot = Join-Path -Path $fixture -ChildPath 'reparse-root'
        $reparseTarget = Join-Path -Path $fixture -ChildPath 'reparse-target'
        $rootLink = New-RogGateSelfTestDirectoryLink -LinkPath $reparseRoot -TargetPath $reparseTarget
        $rootKind = Format-RogGateSelfTestLinkKind -LinkType $rootLink.LinkType
        $measureThrew = $false
        if ($rootLink.Ok) {
            try { $null = @(Get-RogGateTopLevelDirectorySizes -RootPath $reparseRoot) } catch { $measureThrew = $true }
        }
        Assert-RogGateSelfTest -Name ("measuring a reparse build root is refused (link={0})" -f $rootKind) -Condition ($rootLink.Ok -and ($rootKind -eq $directoryLinkKind) -and $measureThrew)
    } finally {
        $null = Remove-RogGateFixture -LiteralPath $fixture
    }

    Write-Host ("ROG gate self-test: {0} failed" -f $script:RogGateSelfTestFailures)
    if ($script:RogGateSelfTestFailures -gt 0) {
        return 1
    }
    return 0
}

function Remove-RogGateFixture {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)

    $item = $null
    try {
        $item = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    } catch {
        return
    }
    if (Test-RogGateItemReparsePoint -Item $item) {
        if ($env:OS -eq 'Windows_NT') {
            $null = & cmd.exe /d /c "rmdir `"$($item.FullName)`""
            if (Test-Path -LiteralPath $item.FullName) {
                Remove-Item -LiteralPath $item.FullName -Force -ErrorAction SilentlyContinue
            }
        } else {
            Remove-Item -LiteralPath $item.FullName -Force
        }
        return
    }
    $isDir = (Test-RogGateAttributeHasDirectory -Attributes $item.Attributes)
    if (-not $isDir) {
        $null = Clear-RogGateReadOnlyAttribute -LiteralPath $item.FullName
        Remove-Item -LiteralPath $item.FullName -Force
        return
    }
    $dirInfo = New-Object System.IO.DirectoryInfo $item.FullName
    foreach ($entry in @($dirInfo.EnumerateFileSystemInfos())) {
        $null = Remove-RogGateFixture -LiteralPath $entry.FullName
    }
    Remove-Item -LiteralPath $item.FullName -Force
}

function Invoke-RogGateMain {
    $null = Assert-RogGateFreeSpaceFloor -MinFreeGB $MinFreeGB -DriveMinFreeGB $DriveMinFreeGB
    if ([string]::IsNullOrWhiteSpace($SourceDir) -or [string]::IsNullOrWhiteSpace($BuildDir) -or [string]::IsNullOrWhiteSpace($ExpectedSha)) {
        throw '-SourceDir, -BuildDir, and -ExpectedSha are required.'
    }
    if ($Config -notmatch '^[A-Za-z][A-Za-z0-9_]*$') {
        throw "Invalid -Config: $Config"
    }
    $expected = $ExpectedSha.Trim().ToLowerInvariant()
    if ($expected -notmatch '^[0-9a-f]{40}$') {
        throw '-ExpectedSha must be the full 40-character git commit id.'
    }

    $overrides = ConvertTo-RogGateDriveOverrides -Entries $DriveMinFreeGB
    $targets = @(Get-RogGateTargetNames -TargetNames $Target)

    $lexical = Get-RogGateLexicalSafety -BuildDir $BuildDir -AllowedRoot $AllowedBuildRoot -ForbiddenRoot $script:RogGateForbiddenRoot
    if (-not $lexical.Safe) {
        throw $lexical.Reason
    }
    $normBuild = $lexical.NormalizedBuildDir
    $normRoot = $lexical.NormalizedAllowedRoot
    $normSource = ConvertTo-RogGateWindowsPath -Path $SourceDir

    if (-not (Test-Path -LiteralPath $normSource -PathType Container)) {
        throw "SourceDir not found: $normSource"
    }
    # Same gate as the script entry, before rev-parse, delete, or build.
    $null = Assert-RogGateSparseReparsePreflight -SourceDir $normSource
    $cmakeLists = Join-Path -Path (Join-Path -Path $normSource -ChildPath 'modern') -ChildPath 'CMakeLists.txt'
    if (-not (Test-Path -LiteralPath $cmakeLists)) {
        throw "modern/CMakeLists.txt not found under SourceDir: $cmakeLists"
    }

    $head = Get-RogGateHeadSha -Worktree $normSource
    if ($head -ne $expected) {
        throw "HEAD $head does not equal ExpectedSha $expected."
    }

    $buildExists = Test-Path -LiteralPath $normBuild
    $stamp = [pscustomobject]@{ HasStamp = $false; Sha = '' }
    $cache = [pscustomobject]@{ HasCache = $false; Generator = '' }
    if ($buildExists) {
        $stamp = Read-RogGateStampSha -BuildDirectory $normBuild
        $cache = Get-RogGateCMakeGenerator -BuildDirectory $normBuild
    }
    $decision = Get-RogGateCleanDecision -BuildDirExists $buildExists -HasStamp $stamp.HasStamp -StampSha $stamp.Sha -ExpectedSha $expected -HasCache $cache.HasCache -CacheGenerator $cache.Generator -RequestedGenerator $Generator
    $drives = @(Get-RogGateDrivesToCheck -NormalizedBuildDir $normBuild -NormalizedSourceDir $normSource)

    $null = Write-RogGateHeader -Source $normSource -Build $normBuild -Expected $expected -ConfigName $Config -GeneratorName $Generator -Allowed $normRoot -Clean $decision.Clean -Reason $decision.Reason -Drives $drives

    $free = @{}
    foreach ($letter in $drives) {
        try {
            $free[$letter] = Get-RogGateDriveFreeBytes -Letter $letter
        } catch {
            Write-RogGateError $_.Exception.Message
        }
    }
    $report = Get-RogGateFreeSpaceReport -Drives $drives -FreeBytesByDrive $free -MinFreeGB $MinFreeGB -Overrides $overrides
    Write-Host 'Free space:'
    $null = Write-RogGateFreeSpaceReport -Report $report
    if (-not $report.Ok) {
        $freeSpaceFailure = Resolve-RogGateFailureExit -ExitCode 1 -Message 'Free space pre-flight failed. Refusing to delete or build.'
        try {
            $null = Write-RogGateLargestDirectories -Root $normRoot
        } catch {
            Write-RogGateError $_.Exception.Message
        }
        return $freeSpaceFailure
    }

    $safety = Get-RogGateDeletionSafety -BuildDir $normBuild -AllowedRoot $normRoot -ForbiddenRoot $script:RogGateForbiddenRoot
    $null = Assert-RogGateMayDelete -Safety $safety

    $dry = [bool]$DryRun
    if ($WhatIfPreference) {
        $dry = $true
    }

    $configure = @('cmake', '-S', (Join-Path -Path $normSource -ChildPath 'modern'), '-B', $normBuild, '-G', $Generator, ("-DCMAKE_BUILD_TYPE=" + $Config))
    if ($Generator -eq 'Ninja') {
        $configure += ("-DCMAKE_MAKE_PROGRAM=" + $script:RogGateNinja)
    }
    $buildCmd = @('cmake', '--build', $normBuild, '--config', $Config)
    if ($Generator -eq 'Ninja') {
        $buildCmd += '--parallel'
    }
    foreach ($targetName in $targets) {
        $buildCmd += '--target'
        $buildCmd += $targetName
    }
    $ctestPlan = Get-RogGateMainCTestArguments -BuildDir $normBuild -ConfigName $Config -ExtraArgs $CtestArgs
    $ctest = @($ctestPlan.Arguments)

    if ($dry) {
        if ($decision.ShouldDelete) {
            Write-Host "[DRY-RUN] Would delete $normBuild"
        } else {
            Write-Host "[DRY-RUN] Would keep $normBuild"
        }
        Write-Host ("[DRY-RUN] Would configure: " + ($configure -join ' '))
        Write-Host ("[DRY-RUN] Would build: " + ($buildCmd -join ' '))
        if ($SkipTests) {
            Write-Host '[DRY-RUN] Would skip CTest'
        } else {
            Write-Host ("[DRY-RUN] Would test: " + ($ctest -join ' '))
        }
        return 0
    }

    if (-not (Test-Path -LiteralPath $script:RogGateVcVarsAll)) {
        throw "vcvarsall.bat not found: $script:RogGateVcVarsAll"
    }
    if ($Generator -eq 'Ninja' -and -not (Test-Path -LiteralPath $script:RogGateNinja)) {
        throw "Ninja not found: $script:RogGateNinja"
    }

    Write-Host '[GATE] Checking cmake inside vcvarsall x86...'
    $cmakeProbe = Invoke-RogGateVcVarsCommand -CommandArguments @('cmake', '--version') -VcVarsAll $script:RogGateVcVarsAll
    if ($cmakeProbe -ne 0) {
        return (Resolve-RogGateFailureExit -ExitCode $cmakeProbe -Message "cmake --version failed inside vcvarsall x86 (exit $cmakeProbe). Refusing to delete or build.")
    }
    if ($Generator -eq 'NMake Makefiles') {
        Write-Host '[GATE] Checking nmake inside vcvarsall x86...'
        $nmakeProbe = Invoke-RogGateVcVarsCommand -CommandArguments @('nmake', '/?') -VcVarsAll $script:RogGateVcVarsAll -Quiet
        if ($nmakeProbe -ne 0) {
            return (Resolve-RogGateFailureExit -ExitCode $nmakeProbe -Message "nmake /? failed inside vcvarsall x86 (exit $nmakeProbe). Refusing to delete or build.")
        }
    }

    if ($decision.ShouldDelete) {
        Write-Host "[GATE] Deleting $normBuild"
        $null = Remove-RogGateDirectoryNoFollow -LiteralPath $normBuild
    }

    $buildStarted = [DateTime]::UtcNow
    Write-Host "[GATE] Configure $($Generator) $Config"
    $configureCode = Invoke-RogGateVcVarsCommand -CommandArguments $configure -VcVarsAll $script:RogGateVcVarsAll
    if ($configureCode -ne 0) {
        return (Resolve-RogGateFailureExit -ExitCode $configureCode -Message "CMake configure failed (exit $configureCode). Tests will not run.")
    }

    Write-Host "[GATE] Build $Config"
    $buildCode = Invoke-RogGateVcVarsCommand -CommandArguments $buildCmd -VcVarsAll $script:RogGateVcVarsAll
    if ($buildCode -ne 0) {
        return (Resolve-RogGateFailureExit -ExitCode $buildCode -Message "Build failed (exit $buildCode). Tests will not run.")
    }

    if ($SkipTests) {
        $null = Write-RogGateStamp -BuildDirectory $normBuild -Sha $expected -GeneratorName $Generator -ConfigName $Config
        Write-Host '[GATE] Build succeeded. Tests skipped. Stamp written.'
        return 0
    }

    $testExes = @(Get-RogGateTestExecutablePaths -NormalizedBuildDir $normBuild)
    if ($testExes.Count -eq 0) {
        throw 'No test executables (*_tests.exe or CTest-registered .exe under BuildDir) were produced. Refusing to run CTest.'
    }
    $allowStaleTimestamp = -not $decision.Clean
    foreach ($exe in $testExes) {
        $freshness = Test-RogGateExecutableFresh -ExePath $exe -BuildStartedUtc $buildStarted -AllowExistingBecauseStampMatches $allowStaleTimestamp
        if (-not $freshness.Ok) {
            throw "Refusing to test: $($freshness.Reason)"
        }
    }
    if ($allowStaleTimestamp) {
        Write-Host ("[GATE] {0} test executable(s) accepted because the stamp matches {1}." -f $testExes.Count, $expected)
    } else {
        Write-Host ("[GATE] {0} test executable(s) are newer than this rebuild." -f $testExes.Count)
    }

    $null = Write-RogGateStamp -BuildDirectory $normBuild -Sha $expected -GeneratorName $Generator -ConfigName $Config
    # Before CTest. The variable stays in this process for any later test child.
    $playdh = Set-RogGatePlayDhRootForChildren -SourceDir $normSource
    $playdhCheck = Test-RogGatePlayDhPreflight -PlayDhRoot $playdh.Path
    if (-not $playdhCheck.Ok) {
        return (Resolve-RogGateFailureExit -ExitCode 1 -Message $playdhCheck.Reason)
    }
    # Ports come from MXH_E2E_PORT_BASE. The launcher adds --login-port,
    # --agent-port, --map-port, and --map12-port. Do not append them here.
    $null = Set-RogGateE2EPortBase
    $e2ePorts = Get-RogGateE2EPortsFromBase -BaseText $env:MXH_E2E_PORT_BASE
    if (-not $e2ePorts.Ok) {
        return (Resolve-RogGateFailureExit -ExitCode 1 -Message $e2ePorts.Reason)
    }
    Write-Host (Format-RogGateE2EPortsLine -Login $e2ePorts.Login -Agent $e2ePorts.Agent -Map $e2ePorts.Map -Map12 $e2ePorts.Map12)
    $e2eListPlan = Get-RogGateE2ECtestArguments -BuildDir $normBuild -ConfigName $Config -ListOnly $true
    $e2eListText = ''
    $e2eListCode = Invoke-RogGateVcVarsCommand -CommandArguments @($e2eListPlan.Arguments) -VcVarsAll $script:RogGateVcVarsAll -NoConsole -CapturedText ([ref]$e2eListText)
    $e2eListing = Get-RogGateE2EListing -Text $e2eListText -ExitCode $e2eListCode
    $e2ePlan = Resolve-RogGateE2ERegistration -Listing $e2eListing -SkipE2E:$SkipE2E
    if ($e2ePlan.Action -eq 'Error') {
        return (Resolve-RogGateFailureExit -ExitCode 1 -Message $e2ePlan.Message)
    }
    if ($e2ePlan.Action -eq 'Absent' -or $e2ePlan.Action -eq 'Skip') {
        Write-Host $e2ePlan.Message
    }
    if ($e2ePlan.Action -eq 'Run') {
        $e2ePortList = @(
            [int]$e2ePorts.Login,
            [int]$e2ePorts.Agent,
            [int]$e2ePorts.Map,
            [int]$e2ePorts.Map12
        )
        $busyPort = Get-RogGateFirstBusyLoopbackPort -Ports $e2ePortList
        if ($busyPort -ne 0) {
            return (Resolve-RogGateFailureExit -ExitCode 1 -Message (Format-RogGatePortInUseMessage -Port $busyPort))
        }
    }
    Write-Host '[GATE] CTest'
    $ctestText = ''
    $testCode = Invoke-RogGateVcVarsCommand -CommandArguments $ctest -VcVarsAll $script:RogGateVcVarsAll -CapturedText ([ref]$ctestText)
    $skipReport = Get-RogGateCTestSkipReport -Text $ctestText
    foreach ($skipLine in @($skipReport.Lines)) {
        Write-Host $skipLine
    }
    if ($testCode -ne 0) {
        return (Resolve-RogGateFailureExit -ExitCode $testCode -Message "CTest failed (exit $testCode).")
    }
    if ($e2ePlan.Action -ne 'Run') {
        return 0
    }
    $e2eCommandText = [string]$e2eListing.Command
    $e2eWork = [string]$e2eListing.WorkingDirectory
    if ([string]::IsNullOrWhiteSpace($e2eWork)) {
        $e2eWork = $normBuild
    }
    Write-Host "[INFO] E2E command: $e2eCommandText"
    Write-Host "[INFO] E2E working directory: $e2eWork"
    Write-Host '[GATE] MoxianClientE2E'
    $e2eRunPlan = Get-RogGateE2ECtestArguments -BuildDir $normBuild -ConfigName $Config -ListOnly $false
    $e2eText = ''
    $e2eCode = Invoke-RogGateVcVarsCommand -CommandArguments @($e2eRunPlan.Arguments) -VcVarsAll $script:RogGateVcVarsAll -CapturedText ([ref]$e2eText)
    $e2eSkipReport = Get-RogGateCTestSkipReport -Text $e2eText
    foreach ($e2eSkipLine in @($e2eSkipReport.Lines)) {
        Write-Host $e2eSkipLine
    }
    if ($e2eCode -ne 0) {
        return (Resolve-RogGateFailureExit -ExitCode $e2eCode -Message "MoxianClientE2E failed (exit $e2eCode).")
    }
    return 0
}

$script:RogGateSawError = $false
$collected = $null
try {
    # Reject a threshold under 10 GB before self-test, git, delete, or build.
    $null = Assert-RogGateFreeSpaceFloor -MinFreeGB $MinFreeGB -DriveMinFreeGB $DriveMinFreeGB
    if ($PSBoundParameters.ContainsKey('SelfTestBuildExit')) {
        # Same reporter the real build-failure path uses. This stub does not
        # call Invoke-RogGateMain. Exit once, below, so Windows PowerShell 5.1
        # does not swallow the code inside try/catch.
        $collected = Resolve-RogGateFailureExit -ExitCode $SelfTestBuildExit -Message "Build failed (exit $SelfTestBuildExit). Tests will not run."
    } else {
        # Sparse checkout plus a junction/symlink is refused before git, delete, or build.
        if (-not $SelfTest) {
            $null = Assert-RogGateSparseReparsePreflight -SourceDir $SourceDir
        }
        if ($SelfTest) {
            $collected = Invoke-RogGateSelfTest
        } else {
            $collected = Invoke-RogGateMain
        }
    }
} catch {
    Write-RogGateError $_.Exception.Message
    $collected = 1
}
$resolved = Resolve-RogGateCollectedExit -Code $collected
if (-not $resolved.Ok) {
    Write-RogGateError $resolved.Detail
    exit 1
}
$gateCode = $resolved.ExitCode
if ($script:RogGateSawError -and $gateCode -eq 0) {
    $gateCode = 1
}
exit $gateCode
