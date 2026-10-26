#requires -Version 5.1
<#
.SYNOPSIS
    ROG trial-merge gate: clean stale build trees, then build and test.

.DESCRIPTION
    Verifies the trial-merge SHA, refuses unsafe build paths, and checks free
    space before deleting anything. An existing build directory is deleted and
    reconfigured when rog-gate.sha is missing or differs, or when CMakeCache.txt
    was generated with a different CMake generator.

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

.PARAMETER DryRun
    Run the read-only checks and print the actions. Alias of -WhatIf for this script.

.PARAMETER MinFreeGB
    Minimum free gibibytes (1024^3) required on every checked drive. Default 10.
    10 GB is a hard floor; a lower value is rejected.

.PARAMETER DriveMinFreeGB
    Per-drive overrides, for example E=20 or E=20,D=15. Drives not listed use -MinFreeGB.
    Each override must be at least 10 GB. Overrides can only raise the floor.

.PARAMETER SelfTest
    Check path safety, generator decisions, drive selection, and free-space
    thresholds without a compiler.
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
    [switch]$DryRun,
    [double]$MinFreeGB = 10,
    [string[]]$DriveMinFreeGB = @(),
    [switch]$SelfTest
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$script:RogGateVcVarsAll = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat'
$script:RogGateNinja = 'C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$script:RogGateForbiddenRoot = 'D:\MX'
$script:RogGateStampName = 'rog-gate.sha'

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

function Test-RogGateItemReparsePoint {
    param($Item)

    if ($null -eq $Item) {
        return $false
    }
    $reparse = [System.IO.FileAttributes]::ReparsePoint
    if (($Item.Attributes -band $reparse) -ne 0) {
        return $true
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
    $leafIsDir = ($leaf.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
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
            $entryIsDir = ($entry.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
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

    $attrs = [System.IO.File]::GetAttributes($LiteralPath)
    $readOnly = [System.IO.FileAttributes]::ReadOnly
    if (($attrs -band $readOnly) -eq 0) {
        return
    }
    $cleared = [System.IO.FileAttributes]([int]$attrs -band (-bnot [int]$readOnly))
    [System.IO.File]::SetAttributes($LiteralPath, $cleared)
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
    $isDir = ($info.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
    if (-not $isDir) {
        throw "Refusing to delete because BuildDir is not a directory: $LiteralPath"
    }

    $dirInfo = New-Object System.IO.DirectoryInfo $info.FullName
    $children = @($dirInfo.EnumerateFileSystemInfos())
    foreach ($entry in $children) {
        if (Test-RogGateItemReparsePoint -Item $entry) {
            throw "Refusing to delete through reparse point: $($entry.FullName)"
        }
        $entryIsDir = ($entry.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
        if ($entryIsDir) {
            Remove-RogGateDirectoryNoFollow -LiteralPath $entry.FullName
        } else {
            Clear-RogGateReadOnlyAttribute -LiteralPath $entry.FullName
            [System.IO.File]::Delete($entry.FullName)
        }
    }
    Clear-RogGateReadOnlyAttribute -LiteralPath $info.FullName
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
            $entryIsDir = ($entry.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
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
    $rootIsDir = ($rootInfo.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
    if (-not $rootIsDir) {
        throw "Build root is not a directory: $RootPath"
    }

    $result = New-Object 'System.Collections.Generic.List[object]'
    $dirInfo = New-Object System.IO.DirectoryInfo $rootInfo.FullName
    foreach ($entry in $dirInfo.EnumerateFileSystemInfos()) {
        $isDir = ($entry.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
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
    [Console]::Error.WriteLine("[ERROR] $Message")
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
            $entryIsDir = ($entry.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
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
        [switch]$Quiet
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
            return 1
        }
        $outTask = $process.StandardOutput.ReadToEndAsync()
        $errTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $outText = $outTask.GetAwaiter().GetResult()
        $errText = $errTask.GetAwaiter().GetResult()
        if (-not [string]::IsNullOrEmpty($outText)) {
            [Console]::Out.Write($outText)
        }
        if (-not [string]::IsNullOrEmpty($errText)) {
            [Console]::Error.Write($errText)
        }
        return [int]$process.ExitCode
    } finally {
        Remove-Item -LiteralPath $wrapper -Force -ErrorAction SilentlyContinue
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

    $normalized = ConvertTo-RogGateWindowsPath -Path 'D:/build-mxr/foo/./bar'
    Assert-RogGateSelfTest -Name 'normalize slashes and dots' -Condition ($normalized -eq 'D:\build-mxr\foo\bar')
    $normalized = ConvertTo-RogGateWindowsPath -Path 'D:\build-mxr\foo\..\bar\'
    Assert-RogGateSelfTest -Name 'normalize parent segment' -Condition ($normalized -eq 'D:\build-mxr\bar')
    $normalized = ConvertTo-RogGateWindowsPath -Path '\\?\D:\build-mxr\pr20'
    Assert-RogGateSelfTest -Name 'strip extended path prefix' -Condition ($normalized -eq 'D:\build-mxr\pr20')

    $threw = $false
    try { ConvertTo-RogGateWindowsPath -Path 'D:\..\MX' } catch { $threw = $true }
    Assert-RogGateSelfTest -Name 'reject drive escape' -Condition $threw

    $threw = $false
    try { ConvertTo-RogGateWindowsPath -Path '\\server\share\pr' } catch { $threw = $true }
    Assert-RogGateSelfTest -Name 'reject UNC' -Condition $threw

    $threw = $false
    try { ConvertTo-RogGateWindowsPath -Path 'build-mxr\pr' } catch { $threw = $true }
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
    try { Assert-RogGateMayDelete -Safety $rootItself } catch { $mayThrew = $true }
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
    try { ConvertTo-RogGateDriveOverrides -Entries @('E') } catch { $badOverride = $true }
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
        Write-RogGateStamp -BuildDirectory $stampDir -Sha $sha -GeneratorName 'NMake Makefiles' -ConfigName 'Debug'
        $stampRead = Read-RogGateStampSha -BuildDirectory $stampDir
        Assert-RogGateSelfTest -Name 'stamp round-trip keeps the SHA on the first line' -Condition ($stampRead.HasStamp -and ($stampRead.Sha -eq $sha))

        $plain = Join-Path -Path $fixture -ChildPath 'plain'
        $nested = Join-Path -Path $plain -ChildPath 'sub'
        New-Item -ItemType Directory -Path $nested | Out-Null
        $plainFile = Join-Path -Path $nested -ChildPath 'a.txt'
        [System.IO.File]::WriteAllBytes($plainFile, (New-Object byte[] 32))
        $plainItem = Get-Item -LiteralPath $plainFile -Force
        $plainItem.Attributes = $plainItem.Attributes -bor [System.IO.FileAttributes]::ReadOnly
        $sibling = Join-Path -Path $fixture -ChildPath 'keep.txt'
        [System.IO.File]::WriteAllText($sibling, 'keep')
        Remove-RogGateDirectoryNoFollow -LiteralPath $plain
        Assert-RogGateSelfTest -Name 'no-follow delete removes a normal tree and keeps its sibling' -Condition ((-not (Test-Path -LiteralPath $plain)) -and (Test-Path -LiteralPath $sibling))

        $outside = Join-Path -Path $fixture -ChildPath 'outside'
        New-Item -ItemType Directory -Path $outside | Out-Null
        $marker = Join-Path -Path $outside -ChildPath 'marker.txt'
        [System.IO.File]::WriteAllText($marker, 'keep-me')
        $linkedBuild = Join-Path -Path $fixture -ChildPath 'linked-build'
        New-Item -ItemType Directory -Path $linkedBuild | Out-Null
        $link = Join-Path -Path $linkedBuild -ChildPath 'jump'
        $linkCreated = $false
        try {
            New-Item -ItemType SymbolicLink -Path $link -Target $outside -ErrorAction Stop | Out-Null
            $linkCreated = $true
        } catch {
            if ($env:OS -eq 'Windows_NT') {
                $null = & cmd.exe /d /c "mklink /J `"$link`" `"$outside`""
                if ($LASTEXITCODE -eq 0) {
                    $linkCreated = $true
                }
            }
        }
        if (-not $linkCreated) {
            Assert-RogGateSelfTest -Name 'create a directory symlink or junction' -Condition $false
        } else {
            $violations = @(Get-RogGateReparseViolations -LiteralPath $linkedBuild)
            $sawLink = $false
            foreach ($violation in $violations) {
                if ($violation -eq $link) {
                    $sawLink = $true
                }
            }
            Assert-RogGateSelfTest -Name 'directory symlink is reported and not descended into' -Condition ($sawLink -and (Test-Path -LiteralPath $marker))

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
                Remove-RogGateDirectoryNoFollow -LiteralPath $linkedBuild
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
            $ancestorOk = $false
            try {
                New-Item -ItemType SymbolicLink -Path $ancestorLink -Target $real -ErrorAction Stop | Out-Null
                $ancestorOk = $true
            } catch {
                if ($env:OS -eq 'Windows_NT') {
                    $null = & cmd.exe /d /c "mklink /J `"$ancestorLink`" `"$real`""
                    if ($LASTEXITCODE -eq 0) {
                        $ancestorOk = $true
                    }
                }
            }
            if ($ancestorOk) {
                $via = Join-Path -Path $ancestorLink -ChildPath 'child'
                $ancestorHits = @(Get-RogGateReparseViolations -LiteralPath $via)
                $sawAncestor = $false
                foreach ($hit in $ancestorHits) {
                    if ($hit -eq $ancestorLink) {
                        $sawAncestor = $true
                    }
                }
                Assert-RogGateSelfTest -Name 'ancestor symlink is refused before descent' -Condition $sawAncestor
            }
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
        $sizeLinkOk = $false
        try {
            New-Item -ItemType SymbolicLink -Path $sizeJump -Target $sizeOutside -ErrorAction Stop | Out-Null
            New-Item -ItemType SymbolicLink -Path (Join-Path -Path $small -ChildPath 'out') -Target $sizeOutside -ErrorAction Stop | Out-Null
            $sizeLinkOk = $true
        } catch {
            $sizeLinkOk = $false
        }
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
            Assert-RogGateSelfTest -Name 'top-level sizes skip files and do not follow symlinks' -Condition $sizeOk
        } else {
            Assert-RogGateSelfTest -Name 'top-level sizes skip files and do not follow symlinks' -Condition $false
        }

        $reparseRoot = Join-Path -Path $fixture -ChildPath 'reparse-root'
        $reparseTarget = Join-Path -Path $fixture -ChildPath 'reparse-target'
        New-Item -ItemType Directory -Path $reparseTarget | Out-Null
        $rootLinkOk = $false
        try {
            New-Item -ItemType SymbolicLink -Path $reparseRoot -Target $reparseTarget -ErrorAction Stop | Out-Null
            $rootLinkOk = $true
        } catch {
            $rootLinkOk = $false
        }
        if ($rootLinkOk) {
            $measureThrew = $false
            try { Get-RogGateTopLevelDirectorySizes -RootPath $reparseRoot } catch { $measureThrew = $true }
            Assert-RogGateSelfTest -Name 'measuring a reparse build root is refused' -Condition $measureThrew
        }
    } finally {
        Remove-RogGateFixture -LiteralPath $fixture
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
    $isDir = ($item.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0
    if (-not $isDir) {
        Clear-RogGateReadOnlyAttribute -LiteralPath $item.FullName
        Remove-Item -LiteralPath $item.FullName -Force
        return
    }
    $dirInfo = New-Object System.IO.DirectoryInfo $item.FullName
    foreach ($entry in @($dirInfo.EnumerateFileSystemInfos())) {
        Remove-RogGateFixture -LiteralPath $entry.FullName
    }
    Remove-Item -LiteralPath $item.FullName -Force
}

function Invoke-RogGateMain {
    Assert-RogGateFreeSpaceFloor -MinFreeGB $MinFreeGB -DriveMinFreeGB $DriveMinFreeGB
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

    Write-RogGateHeader -Source $normSource -Build $normBuild -Expected $expected -ConfigName $Config -GeneratorName $Generator -Allowed $normRoot -Clean $decision.Clean -Reason $decision.Reason -Drives $drives

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
    Write-RogGateFreeSpaceReport -Report $report
    if (-not $report.Ok) {
        Write-RogGateError 'Free space pre-flight failed. Refusing to delete or build.'
        try {
            Write-RogGateLargestDirectories -Root $normRoot
        } catch {
            Write-RogGateError $_.Exception.Message
        }
        exit 1
    }

    $safety = Get-RogGateDeletionSafety -BuildDir $normBuild -AllowedRoot $normRoot -ForbiddenRoot $script:RogGateForbiddenRoot
    Assert-RogGateMayDelete -Safety $safety

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
    $ctest = @('ctest', '--test-dir', $normBuild, '-C', $Config, '--output-on-failure')
    if ($null -ne $CtestArgs) {
        foreach ($extra in $CtestArgs) {
            if (-not [string]::IsNullOrEmpty($extra)) {
                $ctest += $extra
            }
        }
    }

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
        Write-RogGateError "cmake --version failed inside vcvarsall x86 (exit $cmakeProbe). Refusing to delete or build."
        exit $cmakeProbe
    }
    if ($Generator -eq 'NMake Makefiles') {
        Write-Host '[GATE] Checking nmake inside vcvarsall x86...'
        $nmakeProbe = Invoke-RogGateVcVarsCommand -CommandArguments @('nmake', '/?') -VcVarsAll $script:RogGateVcVarsAll -Quiet
        if ($nmakeProbe -ne 0) {
            Write-RogGateError "nmake /? failed inside vcvarsall x86 (exit $nmakeProbe). Refusing to delete or build."
            exit $nmakeProbe
        }
    }

    if ($decision.ShouldDelete) {
        Write-Host "[GATE] Deleting $normBuild"
        Remove-RogGateDirectoryNoFollow -LiteralPath $normBuild
    }

    $buildStarted = [DateTime]::UtcNow
    Write-Host "[GATE] Configure $($Generator) $Config"
    $configureCode = Invoke-RogGateVcVarsCommand -CommandArguments $configure -VcVarsAll $script:RogGateVcVarsAll
    if ($configureCode -ne 0) {
        Write-RogGateError "CMake configure failed (exit $configureCode). Tests will not run."
        exit $configureCode
    }

    Write-Host "[GATE] Build $Config"
    $buildCode = Invoke-RogGateVcVarsCommand -CommandArguments $buildCmd -VcVarsAll $script:RogGateVcVarsAll
    if ($buildCode -ne 0) {
        Write-RogGateError "Build failed (exit $buildCode). Tests will not run."
        exit $buildCode
    }

    if ($SkipTests) {
        Write-RogGateStamp -BuildDirectory $normBuild -Sha $expected -GeneratorName $Generator -ConfigName $Config
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

    Write-RogGateStamp -BuildDirectory $normBuild -Sha $expected -GeneratorName $Generator -ConfigName $Config
    Write-Host '[GATE] CTest'
    $testCode = Invoke-RogGateVcVarsCommand -CommandArguments $ctest -VcVarsAll $script:RogGateVcVarsAll
    if ($testCode -ne 0) {
        Write-RogGateError "CTest failed (exit $testCode)."
    }
    exit $testCode
}

try {
    # Reject a threshold under 10 GB before self-test, git, delete, or build.
    Assert-RogGateFreeSpaceFloor -MinFreeGB $MinFreeGB -DriveMinFreeGB $DriveMinFreeGB
    if ($SelfTest) {
        $selfTestCode = Invoke-RogGateSelfTest
        exit $selfTestCode
    }
    $gateCode = Invoke-RogGateMain
    exit $gateCode
} catch {
    Write-RogGateError $_.Exception.Message
    exit 1
}
