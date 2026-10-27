<#
.SYNOPSIS
    One-command clean-machine deployment of the Moxian modern stack.
    Targets ROADMAP M6-A clean machine deployment automation and
    KNOWN_BUGS DEPLOY-MSSQL.

.DESCRIPTION
    Idempotent bootstrap. From a blank Windows Server 2022 (or Windows 10/11)
    box with PowerShell + Git already installed, this script drives the machine
    to a fully built + smoke-verified modern server in one command.

    Steps:
      1. Preflight        admin check, OS, RAM, disk.
      2. Detect prereqs   VS2022 (vswhere), cmake, git, sqlcmd, SqlLocalDB,
                          ODBC 18, VC++ Redist, Chocolatey.
      3. Install prereqs  (only with -InstallPrereqs) via chocolatey + direct
                          download for ODBC 18 + VS Build Tools 2022.
      4. PlayDH root      Required -PlayDhRoot, an existing absolute directory.
                          Refused when the path is missing, relative, normalizes
                          under D:\MX, or normalizes inside RepoRoot. The script
                          does not create or modify modern/data/PlayDH and does
                          not create a junction. Supplemental pak copies go to
                          -PlayDhRoot. ctest and commercial-smoke inherit
                          MXH_PLAYDH_ROOT set to that path.
                          Then check reference/manifests/playdh-supplemental-fill.json.
                          The source path is the buildPath field only.
                          Empty pakSha256 and no zz_supplement.pak continues.
                          A filled pakSha256 must match; the outside-repo pak is
                          copied into -PlayDhRoot.
      5. Build modern     invokes scripts/build-modern.ps1 -Config <Config>.
      6. Run ctest        ctest -C <Config> --test-dir modern/build.
                          MXH_PLAYDH_ROOT is -PlayDhRoot.
      7. Commercial smoke invokes scripts/commercial-smoke.ps1.
                          When pakSha256 is set, this run's [pak] mount order
                          line must list exactly 8 paks, with zz_supplement.pak
                          once and last.

    Idempotency:
      - Prereq detection skips already-installed tools.
      - PlayDH root: the caller supplies -PlayDhRoot. Nothing under
        modern/data/PlayDH is created or replaced.
      - Build: CMake incremental (rebuilds only what changed).
      - ctest + commercial-smoke: safe to repeat.
      - Supplemental pak: copied only when pakSha256 is set and matches.

    Exit codes:
      0  success.
      1  preflight failure.
      2  prereq missing (rerun with -InstallPrereqs).
      3  build failed.
      4  ctest failed.
      5  commercial-smoke failed.


.PARAMETER PlayDhRoot
    Required for deployment. Absolute path of a directory that already
    exists. Refused, with [ERROR] and no write, when it is missing,
    relative, normalizes under D:\MX, or normalizes inside RepoRoot.
    Normalization collapses ".", "..", mixed slashes, and trailing
    separators, then compares without case. -DryRun prints the chosen
    root and does not copy or create anything. -SelfTest does not
    require this parameter.

.PARAMETER DryRun
    Print every step that would run, do not actually mutate the system.
    Prints the chosen -PlayDhRoot and does not copy the supplemental pak.

.PARAMETER InstallPrereqs
    Install missing prereqs via Chocolatey (or direct download for ODBC 18).

.PARAMETER SkipTests
    Skip ctest (faster bootstrap verification).

.PARAMETER SkipSmoke
    Skip commercial-smoke.

.PARAMETER SkipBuild
    Skip the cmake build. Prints [SKIP] step 4 build and does not run
    cmake. Requires -SkipTests because ctest needs a build. With
    -SkipBuild -SkipTests -SkipSmoke no toolchain prereq is required.

.PARAMETER SkipGui
    Forward -SkipGui to commercial-smoke (skip GUI client smoke step).

.PARAMETER Config
    CMake config (Debug or Release). Default Debug.

.PARAMETER RepoRoot
    Override repo root. Defaults to the parent of $PSScriptRoot.

.PARAMETER Force
    Rebuild from scratch (remove modern/build before the build step).

.PARAMETER SelfTest
    Run the supplemental-pak self-test and exit. Does not deploy, build,
    or copy a pak into the repo. Prints PASS, FAIL, and SKIP counts.
    Covers a missing, relative, D:\MX, and in-repo -PlayDhRoot, plus an
    external directory that receives the supplemental pak.

.EXAMPLE
    PS> powershell -ExecutionPolicy Bypass -File scripts\clean-deploy.ps1 -SelfTest

.EXAMPLE
    PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs

.EXAMPLE
    PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -DryRun

.NOTES
    Launch scripts\clean-deploy.cmd from a PowerShell 7 remote shell.
    That launcher clears PSModulePath before Windows PowerShell 5.1
    (powershell.exe). A leaked PSModulePath hides Get-FileHash.

.EXAMPLE
    PS> scripts\clean-deploy.cmd -SelfTest
#>

[CmdletBinding()]
param(
    [switch]$DryRun,
    [switch]$InstallPrereqs,
    [switch]$SkipTests,
    [switch]$SkipSmoke,
    [switch]$SkipBuild,
    [switch]$SkipGui,
    [switch]$Force,
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [string]$RepoRoot = "",
    [string]$PlayDhRoot = "",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"
$script:CleanDeployScriptPath = $PSCommandPath
if ([string]::IsNullOrWhiteSpace($script:CleanDeployScriptPath)) {
    $script:CleanDeployScriptPath = $MyInvocation.MyCommand.Path
}
$script:CleanDeployScriptRoot = $PSScriptRoot
$script:CleanDeploySupplementExpectedFilled = $false

# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------

function Step {
    param([string]$Text)
    Write-Host ""
    Write-Host ("=" * 64) -ForegroundColor Cyan
    Write-Host " $Text" -ForegroundColor Cyan
    Write-Host ("=" * 64) -ForegroundColor Cyan
}

function Info  { param([string]$Text) Write-Host "  $Text" -ForegroundColor Gray }
function Ok    { param([string]$Text) Write-Host "  [OK]   $Text" -ForegroundColor Green }
function Warn  { param([string]$Text) Write-Host "  [WARN] $Text" -ForegroundColor Yellow }
function Fail {
    param(
        [Parameter(Mandatory = $true, Position = 0)][string]$Text,
        [Parameter(Position = 1)][int]$Code = 1
    )
    Write-Host "  [FAIL] $Text" -ForegroundColor Red
    # clean-deploy.cmd launches powershell.exe -File. Windows PowerShell 5.1
    # leaves ERRORLEVEL 0 after an uncaught throw, so exit explicitly.
    try { $host.SetShouldExit($Code) } catch { }
    exit $Code
}


function Test-Admin {
    try {
        $id = [Security.Principal.WindowsIdentity]::GetCurrent()
        $pr = New-Object Security.Principal.WindowsPrincipal($id)
        return $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    } catch {
        return $false
    }
}

function Test-Tool {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

function Test-CleanDeployPrereqNameListed {
    param(
        [string]$Name,
        [string]$Raw
    )
    if ([string]::IsNullOrWhiteSpace($Raw)) { return $false }
    foreach ($part in $Raw.Split(",")) {
        if ($part.Trim() -eq $Name) { return $true }
    }
    return $false
}

function Test-CleanDeployPrereqForcedAbsent {
    param([string]$Name)
    # Self-test only. Production never sets MXH_CLEAN_DEPLOY_PREREQ_ABSENT.
    return Test-CleanDeployPrereqNameListed -Name $Name -Raw ([string]$env:MXH_CLEAN_DEPLOY_PREREQ_ABSENT)
}

function Test-CleanDeployPrereqForcedPresent {
    param([string]$Name)
    # Self-test only. Production never sets MXH_CLEAN_DEPLOY_PREREQ_PRESENT.
    return Test-CleanDeployPrereqNameListed -Name $Name -Raw ([string]$env:MXH_CLEAN_DEPLOY_PREREQ_PRESENT)
}

function Get-CleanDeployRequiredPrereqNames {
    param(
        [switch]$SkipBuild,
        [switch]$SkipTests,
        [switch]$SkipSmoke
    )
    # cmake and vs2022 are required only when the build step runs.
    # sqlcmd, sqllocaldb, odbc18, and vcredist are required when ctest
    # or commercial-smoke runs (the same set for either step).
    # git is not required: this script detects it and never invokes it.
    # chocolatey stays optional and is not part of the gate.
    $names = New-Object System.Collections.Generic.List[string]
    if (-not $SkipBuild) {
        [void]$names.Add("cmake")
        [void]$names.Add("vs2022")
    }
    $dataStep = $false
    if (-not $SkipTests) { $dataStep = $true }
    if (-not $SkipSmoke) { $dataStep = $true }
    if ($dataStep) {
        [void]$names.Add("sqlcmd")
        [void]$names.Add("sqllocaldb")
        [void]$names.Add("odbc18")
        [void]$names.Add("vcredist")
    }
    return [pscustomobject]@{ Names = $names.ToArray() }
}

function Get-CleanDeployLegacyPlayDhDirName {
    # UTF-8 bytes of the legacy resource directory. Kept as bytes so this
    # script stays pure ASCII for Windows PowerShell 5.1.
    $bytes = [byte[]](
        229, 162, 168, 233, 166, 153, 227, 128, 144, 230, 186, 144,
        231, 160, 129, 233, 133, 141, 229, 165, 151, 232, 181, 132,
        230, 186, 144, 227, 128, 145)
    return [System.Text.Encoding]::UTF8.GetString($bytes)
}

function Join-CleanDeployPath {
    param(
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Child
    )
    $sep = [string][System.IO.Path]::DirectorySeparatorChar
    $hasBack = $Parent.Contains('\')
    $hasForward = $Parent.Contains('/')
    if ($hasBack -and -not $hasForward) {
        $sep = '\'
    } elseif ($hasForward -and -not $hasBack) {
        $sep = '/'
    } elseif ($Parent -match '^[A-Za-z]:') {
        $sep = '\'
    }
    $trimmed = $Parent.TrimEnd('\', '/')
    return ($trimmed + $sep + $Child)
}

function Get-CleanDeployPathLeaf {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $norm = $Path.Replace('\', '/')
    $idx = $norm.LastIndexOf('/')
    if ($idx -ge 0 -and $idx -lt ($norm.Length - 1)) {
        return $norm.Substring($idx + 1)
    }
    return $norm
}

function Get-CleanDeployCollapsedPath {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $raw = $Path
    if ($raw.StartsWith('\\?\') -or $raw.StartsWith('//?/')) {
        $raw = $raw.Substring(4)
    }
    $slash = [string][char]47
    $backslash = [string][char]92
    $useBack = $false
    if ($raw -match '^[A-Za-z]:' -or $raw.StartsWith($backslash + $backslash) -or $raw.StartsWith($slash + $slash)) {
        $useBack = $true
    } elseif ($raw.Contains($backslash) -and -not $raw.StartsWith($slash)) {
        $useBack = $true
    }
    $parts = New-Object System.Collections.Generic.List[string]
    if ($useBack) {
        $text = $raw.Replace($slash, $backslash)
        $unc = $false
        if ($text.StartsWith($backslash + $backslash)) {
            $unc = $true
            $text = $text.TrimStart($backslash)
        }
        $drive = ""
        if (-not $unc -and $text -match '^[A-Za-z]:') {
            $drive = $text.Substring(0, 2)
            $text = $text.Substring(2).TrimStart($backslash)
        } else {
            $text = $text.Trim($backslash)
        }
        foreach ($seg in $text.Split($backslash)) {
            if ($seg -eq "" -or $seg -eq ".") { continue }
            if ($seg -eq "..") {
                if ($parts.Count -gt 0) { $parts.RemoveAt($parts.Count - 1) }
                continue
            }
            [void]$parts.Add($seg)
        }
        $joined = ""
        foreach ($part in $parts) {
            if ($joined.Length -gt 0) { $joined = $joined + $backslash }
            $joined = $joined + $part
        }
        if ($unc) {
            if ($joined.Length -eq 0) { return $backslash + $backslash }
            return $backslash + $backslash + $joined
        }
        if ($drive.Length -gt 0) {
            if ($joined.Length -eq 0) { return $drive + $backslash }
            return $drive + $backslash + $joined
        }
        return $joined
    }
    $text = $raw.Replace($backslash, $slash)
    $absolute = $text.StartsWith($slash)
    $text = $text.Trim($slash)
    foreach ($seg in $text.Split($slash)) {
        if ($seg -eq "" -or $seg -eq ".") { continue }
        if ($seg -eq "..") {
            if ($parts.Count -gt 0) { $parts.RemoveAt($parts.Count - 1) }
            continue
        }
        [void]$parts.Add($seg)
    }
    $joined = ""
    foreach ($part in $parts) {
        if ($joined.Length -gt 0) { $joined = $joined + $slash }
        $joined = $joined + $part
    }
    if ($absolute) {
        if ($joined.Length -eq 0) { return $slash }
        return $slash + $joined
    }
    return $joined
}

function Test-CleanDeployPathIsAbsolute {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    $slash = [string][char]47
    $backslash = [string][char]92
    if ($Path.StartsWith($backslash + $backslash + '?' + $backslash)) { return $true }
    if ($Path.StartsWith($slash + $slash + '?' + $slash)) { return $true }
    if ($Path -match '^[A-Za-z]:[\\/]') { return $true }
    if ($Path.StartsWith($backslash + $backslash)) { return $true }
    if ($Path.StartsWith($slash + $slash)) { return $true }
    $sep = [string][System.IO.Path]::DirectorySeparatorChar
    if (($sep -eq $slash) -and $Path.StartsWith($slash)) { return $true }
    return $false
}

function Test-CleanDeployPathUnderOriginalRoot {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    $norm = Get-CleanDeployCollapsedPath -Path $Path
    if ([string]::IsNullOrWhiteSpace($norm)) { return $false }
    $lower = $norm.TrimEnd('\').ToLowerInvariant()
    if ($lower -eq 'd:\mx') { return $true }
    if ($lower.StartsWith('d:\mx\')) { return $true }
    return $false
}

function Test-CleanDeployPathInsideRoot {
    param([string]$Path, [string]$Root)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    if ([string]::IsNullOrWhiteSpace($Root)) { return $false }
    $normPath = Get-CleanDeployCollapsedPath -Path $Path
    $normRoot = Get-CleanDeployCollapsedPath -Path $Root
    if ([string]::IsNullOrWhiteSpace($normPath)) { return $false }
    if ([string]::IsNullOrWhiteSpace($normRoot)) { return $false }
    $pathCmp = $normPath.TrimEnd('\', '/').ToLowerInvariant()
    $rootCmp = $normRoot.TrimEnd('\', '/').ToLowerInvariant()
    if ($pathCmp -eq $rootCmp) { return $true }
    $sep = '\'
    if ($rootCmp.StartsWith('/')) { $sep = '/' }
    if ($pathCmp.StartsWith($rootCmp + $sep)) { return $true }
    return $false
}

function Test-CleanDeployLinkUnderOriginalRoot {
    param([string]$LiteralPath)
    if ([string]::IsNullOrWhiteSpace($LiteralPath)) { return $false }
    if (-not (Test-Path -LiteralPath $LiteralPath)) { return $false }
    $item = $null
    try {
        $item = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    } catch {
        return $true
    }
    $targets = @()
    try {
        $targets = @($item.Target)
    } catch {
        return $true
    }
    foreach ($one in $targets) {
        if ([string]::IsNullOrWhiteSpace([string]$one)) { continue }
        if (Test-CleanDeployPathUnderOriginalRoot -Path ([string]$one)) { return $true }
    }
    return $false
}

function New-CleanDeployPlayDhDecision {
    param([bool]$Ok, [string]$Path, $Lines)
    $copy = @()
    if ($null -ne $Lines) { $copy = @($Lines.ToArray()) }
    return [pscustomobject]@{
        Ok = [bool]$Ok
        Path = [string]$Path
        Lines = $copy
    }
}

function Test-CleanDeployLinkInsideRoot {
    param([string]$LiteralPath, [string]$Root)
    if ([string]::IsNullOrWhiteSpace($LiteralPath)) { return $false }
    if ([string]::IsNullOrWhiteSpace($Root)) { return $false }
    if (-not (Test-Path -LiteralPath $LiteralPath)) { return $false }
    $item = $null
    try {
        $item = Get-Item -LiteralPath $LiteralPath -Force -ErrorAction Stop
    } catch {
        return $false
    }
    $targets = @()
    try {
        $targets = @($item.Target)
    } catch {
        return $false
    }
    foreach ($one in $targets) {
        if ([string]::IsNullOrWhiteSpace([string]$one)) { continue }
        if (Test-CleanDeployPathInsideRoot -Path ([string]$one) -Root $Root) { return $true }
    }
    return $false
}

function Resolve-CleanDeployPlayDhRoot {
    param(
        [AllowEmptyString()][string]$PlayDhRoot,
        [AllowEmptyString()][string]$RepoRoot
    )
    # Read-only. Callers must exit before creating or copying anything when Ok is false.
    $lines = New-Object System.Collections.Generic.List[string]
    if ([string]::IsNullOrWhiteSpace($PlayDhRoot)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] -PlayDhRoot is required"
        return (New-CleanDeployPlayDhDecision -Ok $false -Path "" -Lines $lines)
    }
    if (-not (Test-CleanDeployPathIsAbsolute -Path $PlayDhRoot)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot must be an absolute path: " + $PlayDhRoot)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path "" -Lines $lines)
    }
    $norm = Get-CleanDeployCollapsedPath -Path $PlayDhRoot
    if ([string]::IsNullOrWhiteSpace($norm)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot must be an absolute path: " + $PlayDhRoot)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path "" -Lines $lines)
    }
    if (Test-CleanDeployPathUnderOriginalRoot -Path $norm) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot resolves under D:\MX: " + $norm)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path $norm -Lines $lines)
    }
    if (Test-CleanDeployPathInsideRoot -Path $norm -Root $RepoRoot) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot resolves inside the repo: " + $norm)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path $norm -Lines $lines)
    }
    if (-not (Test-Path -LiteralPath $norm -PathType Container)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot must be an existing directory: " + $norm)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path $norm -Lines $lines)
    }
    if (Test-CleanDeployLinkUnderOriginalRoot -LiteralPath $norm) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot resolves under D:\MX: " + $norm)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path $norm -Lines $lines)
    }
    if (Test-CleanDeployLinkInsideRoot -LiteralPath $norm -Root $RepoRoot) {
        Write-CleanDeploySupplementLine -Lines $lines -Line ("[ERROR] -PlayDhRoot resolves inside the repo: " + $norm)
        return (New-CleanDeployPlayDhDecision -Ok $false -Path $norm -Lines $lines)
    }
    return (New-CleanDeployPlayDhDecision -Ok $true -Path $norm -Lines $lines)
}

function Publish-CleanDeployPlayDhRoot {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [switch]$DryRun
    )
    $lines = New-Object System.Collections.Generic.List[string]
    $rootLine = "[INFO] PlayDH root: " + $Path
    Write-CleanDeploySupplementLine -Lines $lines -Line $rootLine
    if ($DryRun) {
        $envLine = "[INFO] MXH_PLAYDH_ROOT=" + $Path + " (dry-run)"
        Write-CleanDeploySupplementLine -Lines $lines -Line $envLine
        return (New-CleanDeployPlayDhDecision -Ok $true -Path $Path -Lines $lines)
    }
    $env:MXH_PLAYDH_ROOT = $Path
    $envLine = "[INFO] MXH_PLAYDH_ROOT=" + $Path
    Write-CleanDeploySupplementLine -Lines $lines -Line $envLine
    return (New-CleanDeployPlayDhDecision -Ok $true -Path $Path -Lines $lines)
}

function Test-CleanDeployShaFilled {
    param($Value)
    if ($null -eq $Value) { return $false }
    return -not [string]::IsNullOrWhiteSpace([string]$Value)
}

function Test-CleanDeployShaEqual {
    param([string]$Actual, [string]$Expected)
    if ([string]::IsNullOrWhiteSpace($Actual)) { return $false }
    if ([string]::IsNullOrWhiteSpace($Expected)) { return $false }
    return $Actual.ToLowerInvariant() -eq $Expected.ToLowerInvariant()
}

function Get-CleanDeploySha256 {
    param([Parameter(Mandatory = $true)][string]$LiteralPath)
    $hash = Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256
    return $hash.Hash.ToLowerInvariant()
}

function Read-CleanDeployUtf8Text {
    param([Parameter(Mandatory = $true)][string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $offset = 0
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {
        $offset = 3
    }
    $utf8 = New-Object System.Text.UTF8Encoding $false, $true
    $text = $utf8.GetString($bytes, $offset, $bytes.Length - $offset)
    if ($text.Length -gt 0 -and [int][char]$text[0] -eq 0xFEFF) {
        $text = $text.Substring(1)
    }
    return $text
}

function Read-CleanDeployLogText {
    param([Parameter(Mandatory = $true)][string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 255 -and $bytes[1] -eq 254) {
        return [System.Text.Encoding]::Unicode.GetString($bytes, 2, $bytes.Length - 2)
    }
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 254 -and $bytes[1] -eq 255) {
        return [System.Text.Encoding]::BigEndianUnicode.GetString($bytes, 2, $bytes.Length - 2)
    }
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {
        return [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
    }
    $sample = $bytes.Length
    if ($sample -gt 64) { $sample = 64 }
    $oddZero = 0
    $pairs = 0
    for ($i = 1; $i -lt $sample; $i += 2) {
        $pairs = $pairs + 1
        if ($bytes[$i] -eq 0) { $oddZero = $oddZero + 1 }
    }
    if ($pairs -ge 8 -and $oddZero -eq $pairs) {
        return [System.Text.Encoding]::Unicode.GetString($bytes)
    }
    return [System.Text.Encoding]::UTF8.GetString($bytes)
}

function Get-CleanDeployScriptEncodingKind {
    param([Parameter(Mandatory = $true)][string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $ascii = $true
    foreach ($b in $bytes) {
        if ($b -gt 127) {
            $ascii = $false
            break
        }
    }
    if ($ascii) { return "ascii" }
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {
        try {
            $utf8 = New-Object System.Text.UTF8Encoding $false, $true
            [void]$utf8.GetString($bytes, 3, $bytes.Length - 3)
            return "utf8-bom"
        } catch {
            return "bad"
        }
    }
    return "bad"
}

function Get-CleanDeploySupplementBuildPath {
    param($Manifest)
    # buildPath is the only machine-readable source path. The pak description
    # is prose and is never parsed.
    $result = [pscustomobject]@{
        Status = "missing"
        Path = ""
    }
    if ($null -eq $Manifest) { return $result }
    $has = $false
    foreach ($name in @($Manifest.PSObject.Properties.Name)) {
        if ($name -eq "buildPath") { $has = $true }
    }
    if (-not $has) { return $result }
    $value = $Manifest.buildPath
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        $result.Status = "empty"
        return $result
    }
    $result.Status = "ok"
    $result.Path = ([string]$value).Trim()
    return $result
}

function Write-CleanDeploySupplementLine {
    param($Lines, [string]$Line)
    [void]$Lines.Add($Line)
    Write-Host $Line
}

function New-CleanDeploySupplementResult {
    param([bool]$Ok, [bool]$ExpectedFilled, $Lines)
    $copy = @()
    if ($null -ne $Lines) { $copy = @($Lines.ToArray()) }
    return [pscustomobject]@{
        Ok = [bool]$Ok
        ExpectedFilled = [bool]$ExpectedFilled
        Lines = $copy
    }
}

function Get-CleanDeployMountPakNames {
    param([string]$Line)
    $names = New-Object System.Collections.Generic.List[string]
    if ($null -eq $Line) { return $names }
    $marker = "[pak] mount order"
    $at = $Line.IndexOf($marker)
    if ($at -lt 0) { return $names }
    $tail = $Line.Substring($at + $marker.Length).Trim()
    if ($tail.StartsWith(":")) { $tail = $tail.Substring(1).Trim() }
    if ([string]::IsNullOrWhiteSpace($tail)) { return $names }
    foreach ($token in @($tail -split '\s+')) {
        if ([string]::IsNullOrWhiteSpace($token)) { continue }
        $name = [string]$token
        $eq = $name.LastIndexOf("=")
        if ($eq -gt 0) {
            $prefix = $name.Substring(0, $eq)
            $digits = $prefix.Length -gt 0
            foreach ($ch in $prefix.ToCharArray()) {
                if ($ch -lt '0' -or $ch -gt '9') { $digits = $false; break }
            }
            if ($digits) { $name = $name.Substring($eq + 1) }
        }
        if ($name.Length -gt 2 -and $name.EndsWith(")")) {
            $paren = $name.LastIndexOf("(")
            if ($paren -gt 0) {
                $tier = $name.Substring($paren + 1, $name.Length - $paren - 2)
                $tierDigits = $tier.Length -gt 0
                foreach ($ch in $tier.ToCharArray()) {
                    if ($ch -lt '0' -or $ch -gt '9') { $tierDigits = $false; break }
                }
                if ($tierDigits) { $name = $name.Substring(0, $paren) }
            }
        }
        [void]$names.Add($name)
    }
    return [pscustomobject]@{ Names = $names }
}

function Test-CleanDeploySupplementMountOrder {
    param([string]$LogText)
    $lines = New-Object System.Collections.Generic.List[string]
    if ($null -eq $LogText) { $LogText = "" }
    $found = 0
    $bad = $false
    foreach ($line in ($LogText -split '\r?\n')) {
        if ($line.IndexOf("[pak] mount order") -lt 0) { continue }
        $found = $found + 1
        $parsedNames = Get-CleanDeployMountPakNames -Line $line
        $names = $null
        if ($null -ne $parsedNames) { $names = $parsedNames.Names }
        $count = 0
        $zz = 0
        $last = ""
        if ($null -ne $names) {
            $count = $names.Count
            foreach ($name in $names) {
                if ($name -eq "zz_supplement.pak") { $zz = $zz + 1 }
                $last = [string]$name
            }
        }
        if ($count -ne 8) {
            $bad = $true
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] [pak] mount order must list exactly 8 paks"
        }
        if ($zz -ne 1) {
            $bad = $true
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] [pak] mount order must include zz_supplement.pak exactly once"
        }
        if ($last -ne "zz_supplement.pak") {
            $bad = $true
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] [pak] mount order must end with zz_supplement.pak"
        }
    }
    if ($found -eq 0) {
        $bad = $true
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] [pak] mount order line is missing"
    }
    if (-not $bad) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] [pak] mount order lists 8 paks ending with zz_supplement.pak"
    }
    return (New-CleanDeploySupplementResult -Ok (-not $bad) -ExpectedFilled $true -Lines $lines)
}

function Get-CleanDeployGuiSmokeRunNames {
    param([string]$BuildDir)
    $names = New-Object System.Collections.Generic.List[string]
    $result = [pscustomobject]@{ Names = $names }
    if ([string]::IsNullOrWhiteSpace($BuildDir)) { return $result }
    $smokeRoot = Join-Path $BuildDir "runtime"
    $smokeRoot = Join-Path $smokeRoot "gui-smoke"
    if (-not (Test-Path -LiteralPath $smokeRoot)) { return $result }
    $runs = @(Get-ChildItem -LiteralPath $smokeRoot -Directory -ErrorAction SilentlyContinue)
    foreach ($run in $runs) {
        if ($null -eq $run) { continue }
        if ([string]::IsNullOrWhiteSpace($run.Name)) { continue }
        [void]$names.Add($run.Name)
    }
    return $result
}

function Resolve-CleanDeployCurrentSmokeRunName {
    param($Before, $After)
    $fresh = New-Object System.Collections.Generic.List[string]
    $beforeNames = $null
    $afterNames = $null
    if ($null -ne $Before) { $beforeNames = $Before.Names }
    if ($null -ne $After) { $afterNames = $After.Names }
    if ($null -eq $afterNames) { return "" }
    foreach ($name in $afterNames) {
        if ([string]::IsNullOrWhiteSpace([string]$name)) { continue }
        $seen = $false
        if ($null -ne $beforeNames) {
            foreach ($old in $beforeNames) {
                if ([string]$old -eq [string]$name) { $seen = $true; break }
            }
        }
        if (-not $seen) { [void]$fresh.Add([string]$name) }
    }
    if ($fresh.Count -ne 1) { return "" }
    return [string]$fresh[0]
}

function Read-CleanDeploySmokeRunLog {
    param(
        [string]$BuildDir,
        [string]$RunName
    )
    $result = [pscustomobject]@{ Found = $false; Text = "" }
    if ([string]::IsNullOrWhiteSpace($BuildDir)) { return $result }
    if ([string]::IsNullOrWhiteSpace($RunName)) { return $result }
    $logDir = Join-Path $BuildDir "runtime"
    $logDir = Join-Path $logDir "gui-smoke"
    $logDir = Join-Path $logDir $RunName
    $logDir = Join-Path $logDir "logs"
    if (-not (Test-Path -LiteralPath $logDir -PathType Container)) { return $result }
    $chunks = New-Object System.Collections.Generic.List[string]
    $foundFile = $false
    foreach ($name in @("client.out.log", "client.err.log")) {
        $path = Join-Path $logDir $name
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $foundFile = $true
            [void]$chunks.Add((Read-CleanDeployLogText -Path $path))
        }
    }
    if (-not $foundFile) { return $result }
    $result.Found = $true
    $result.Text = ($chunks.ToArray() -join "`n")
    return $result
}

function Invoke-CleanDeployCurrentSmokeMountCheck {
    param(
        [string]$BuildDir,
        $Before,
        $After
    )
    $lines = New-Object System.Collections.Generic.List[string]
    $runName = Resolve-CleanDeployCurrentSmokeRunName -Before $Before -After $After
    if ([string]::IsNullOrWhiteSpace($runName)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] current smoke run log is missing"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
    }
    $log = Read-CleanDeploySmokeRunLog -BuildDir $BuildDir -RunName $runName
    if (-not $log.Found) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] current smoke run log is missing"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
    }
    return (Test-CleanDeploySupplementMountOrder -LogText $log.Text)
}

function Invoke-CleanDeploySupplementPak {
    param(
        [Parameter(Mandatory = $true)][string]$ManifestPath,
        [Parameter(Mandatory = $true)][string]$PlayDhRoot,
        [switch]$DryRun
    )
    # Rules from reference/manifests/playdh-supplemental-fill.json:
    #   pakSha256 empty + pak absent -> continue
    #   pakSha256 empty + pak present (source or PlayDH root) -> error
    #   pakSha256 filled + missing or hash mismatch -> error
    #   pakSha256 filled + match -> copy the outside-repo pak into the PlayDH root
    # The pak is never committed. Writes under D:\MX are refused. Reads are allowed.
    $lines = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental manifest not found: $ManifestPath"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $false -Lines $lines)
    }

    $manifest = $null
    try {
        $manifest = (Read-CleanDeployUtf8Text -Path $ManifestPath) | ConvertFrom-Json
    } catch {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental manifest could not be read: $ManifestPath"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $false -Lines $lines)
    }

    $filled = Test-CleanDeployShaFilled -Value $manifest.pakSha256
    $expectedLog = "empty"
    if ($filled) { $expectedLog = ([string]$manifest.pakSha256).Trim().ToLowerInvariant() }

    $buildPath = Get-CleanDeploySupplementBuildPath -Manifest $manifest
    if ($buildPath.Status -eq "missing") {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak sha256 actual=absent expected=$expectedLog"
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental manifest buildPath is missing"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $filled -Lines $lines)
    }
    if ($buildPath.Status -ne "ok") {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak sha256 actual=absent expected=$expectedLog"
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental manifest buildPath is empty"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $filled -Lines $lines)
    }
    $source = $buildPath.Path
    if ([string]::IsNullOrWhiteSpace($source)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak sha256 actual=absent expected=$expectedLog"
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental manifest buildPath is empty"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $filled -Lines $lines)
    }
    $leaf = Get-CleanDeployPathLeaf -Path $source
    if ($leaf -ne "zz_supplement.pak") {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak sha256 actual=absent expected=$expectedLog"
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] manifest pak source must be named zz_supplement.pak"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $filled -Lines $lines)
    }

    $target = Join-CleanDeployPath -Parent $PlayDhRoot -Child $leaf
    $sourceExists = Test-Path -LiteralPath $source -PathType Leaf
    $targetExists = Test-Path -LiteralPath $target -PathType Leaf
    $actualSource = ""
    $actualTarget = ""
    if ($sourceExists) { $actualSource = Get-CleanDeploySha256 -LiteralPath $source }
    if ($targetExists) { $actualTarget = Get-CleanDeploySha256 -LiteralPath $target }

    $actualLog = "absent"
    if ($sourceExists -and $targetExists -and ($actualSource -ne $actualTarget)) {
        $actualLog = "source=$actualSource;target=$actualTarget"
    } elseif ($sourceExists) {
        $actualLog = $actualSource
    } elseif ($targetExists) {
        $actualLog = $actualTarget
    }
    Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak sha256 actual=$actualLog expected=$expectedLog"

    if (-not $filled) {
        if ($sourceExists -or $targetExists) {
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak is present but pakSha256 is empty"
            return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $false -Lines $lines)
        }
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak not yet built"
        return (New-CleanDeploySupplementResult -Ok $true -ExpectedFilled $false -Lines $lines)
    }

    if ((-not $sourceExists) -and (-not $targetExists)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak is missing"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
    }
    if ($sourceExists -and -not (Test-CleanDeployShaEqual -Actual $actualSource -Expected $expectedLog)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak sha256 mismatch"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
    }
    if ((-not $sourceExists) -and $targetExists -and -not (Test-CleanDeployShaEqual -Actual $actualTarget -Expected $expectedLog)) {
        Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak sha256 mismatch"
        return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
    }

    $needsCopy = $false
    if ($sourceExists) {
        $targetMatches = $false
        if ($targetExists) {
            $targetMatches = Test-CleanDeployShaEqual -Actual $actualTarget -Expected $expectedLog
        }
        if (-not $targetMatches) { $needsCopy = $true }
    }

    if ($needsCopy) {
        $forbidden = Test-CleanDeployPathUnderOriginalRoot -Path $PlayDhRoot
        if (-not $forbidden) { $forbidden = Test-CleanDeployPathUnderOriginalRoot -Path $target }
        if (-not $forbidden) { $forbidden = Test-CleanDeployLinkUnderOriginalRoot -LiteralPath $PlayDhRoot }
        if ($forbidden) {
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] refusing to write supplemental pak under D:\MX"
            return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
        }
        if ($DryRun) {
            Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] would copy supplemental pak to $target"
            return (New-CleanDeploySupplementResult -Ok $true -ExpectedFilled $true -Lines $lines)
        }
        if (-not (Test-Path -LiteralPath $PlayDhRoot -PathType Container)) {
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] PlayDH root is not a directory: $PlayDhRoot"
            return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
        }
        try {
            Copy-Item -LiteralPath $source -Destination $target -Force
        } catch {
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak copy failed"
            return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
        }
        $copied = Get-CleanDeploySha256 -LiteralPath $target
        # Self-test only. Production never sets this. A non-empty value
        # replaces the post-copy rehash so the mismatch path can be proven.
        $forcedRehash = [string]$env:MXH_CLEAN_DEPLOY_REHASH_OVERRIDE
        if (-not [string]::IsNullOrWhiteSpace($forcedRehash)) {
            $copied = $forcedRehash.Trim().ToLowerInvariant()
        }
        if (-not (Test-CleanDeployShaEqual -Actual $copied -Expected $expectedLog)) {
            try { Remove-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue } catch { }
            Write-CleanDeploySupplementLine -Lines $lines -Line "[ERROR] supplemental pak sha256 mismatch"
            return (New-CleanDeploySupplementResult -Ok $false -ExpectedFilled $true -Lines $lines)
        }
        Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] copied supplemental pak to $target"
        return (New-CleanDeploySupplementResult -Ok $true -ExpectedFilled $true -Lines $lines)
    }

    Write-CleanDeploySupplementLine -Lines $lines -Line "[INFO] supplemental pak already matches pakSha256"
    return (New-CleanDeploySupplementResult -Ok $true -ExpectedFilled $true -Lines $lines)
}

function Write-CleanDeploySelfTestChildDetail {
    param(
        $Child,
        [Parameter(Mandatory = $true)][string]$Name
    )
    $code = -1
    $text = ""
    if ($null -ne $Child) {
        $code = [int]$Child.ExitCode
        $text = [string]$Child.Text
    }
    Write-Host ("[FAIL-DETAIL] " + $Name + " exit=" + $code)
    $shown = 0
    foreach ($line in ($text -split '\r?\n')) {
        if ($shown -ge 25) { break }
        Write-Host ("[FAIL-DETAIL] " + $line)
        $shown = $shown + 1
    }
}

function Assert-CleanDeploySelfTest {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]$Condition
    )
    $ok = $false
    if ($Condition) { $ok = $true }
    if ($ok) {
        $script:CleanDeploySelfTestPass = $script:CleanDeploySelfTestPass + 1
        Write-Host "[PASS] $Name"
    } else {
        $script:CleanDeploySelfTestFail = $script:CleanDeploySelfTestFail + 1
        Write-Host "[FAIL] $Name"
    }
}

function Write-CleanDeploySelfTestSkip {
    param([Parameter(Mandatory = $true)][string]$Name)
    $script:CleanDeploySelfTestSkip = $script:CleanDeploySelfTestSkip + 1
    Write-Host "[SKIP] $Name"
}

function Format-CleanDeploySelfTestMountLine {
    param([string[]]$Names)
    $joined = ""
    $index = 1
    foreach ($name in $Names) {
        $tier = "92"
        if ($name -eq "zz_supplement.pak") { $tier = "96" }
        $piece = [string]$index + "=" + $name + "(" + $tier + ")"
        if ($joined.Length -gt 0) { $joined = $joined + " " }
        $joined = $joined + $piece
        $index = $index + 1
    }
    return "[12:01:02.003] INFO filesystem_file_storage.cpp:338  [pak] mount order: " + $joined
}

function Write-CleanDeploySelfTestSmokeLog {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text,
        [switch]$Utf16
    )
    if ($Utf16) {
        $encoding = New-Object System.Text.UnicodeEncoding $false, $true
        $payload = $encoding.GetBytes($Text)
        $preamble = $encoding.GetPreamble()
        $encoded = New-Object byte[] ($preamble.Length + $payload.Length)
        [System.Array]::Copy($preamble, 0, $encoded, 0, $preamble.Length)
        [System.Array]::Copy($payload, 0, $encoded, $preamble.Length, $payload.Length)
        [System.IO.File]::WriteAllBytes($Path, $encoded)
        return
    }
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $Text, $utf8)
}

function New-CleanDeploySelfTestRun {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Name,
        [ValidateSet("null", "empty", "match-source", "match-target", "zeros")]
        [string]$ShaMode = "null",
        [ValidateSet("path", "omit", "empty", "prose")]
        [string]$BuildPathMode = "path",
        [string]$SourceBody = "",
        [string]$TargetBody = "",
        [switch]$WriteSource,
        [switch]$WriteTarget,
        [string]$PlayDhRoot = "",
        [switch]$DryRun
    )
    $caseRoot = Join-Path $Root $Name
    $sourceDir = Join-Path $caseRoot "source"
    $playdh = Join-Path $caseRoot "playdh"
    New-Item -ItemType Directory -Path $sourceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $playdh -Force | Out-Null
    $source = Join-CleanDeployPath -Parent $sourceDir -Child "zz_supplement.pak"
    $target = Join-CleanDeployPath -Parent $playdh -Child "zz_supplement.pak"
    if ($WriteSource) {
        [System.IO.File]::WriteAllBytes($source, [System.Text.Encoding]::ASCII.GetBytes($SourceBody))
    }
    if ($WriteTarget) {
        [System.IO.File]::WriteAllBytes($target, [System.Text.Encoding]::ASCII.GetBytes($TargetBody))
    }
    $shaJson = "null"
    if ($ShaMode -eq "empty") {
        $shaJson = '""'
    } elseif ($ShaMode -eq "zeros") {
        $shaJson = '"0000000000000000000000000000000000000000000000000000000000000000"'
    } elseif ($ShaMode -eq "match-source") {
        $shaJson = '"' + (Get-CleanDeploySha256 -LiteralPath $source) + '"'
    } elseif ($ShaMode -eq "match-target") {
        $shaJson = '"' + (Get-CleanDeploySha256 -LiteralPath $target) + '"'
    }
    $escaped = New-Object System.Text.StringBuilder
    foreach ($ch in $source.ToCharArray()) {
        if ($ch -eq '\') { [void]$escaped.Append('\\') }
        elseif ($ch -eq '"') { [void]$escaped.Append('\"') }
        else { [void]$escaped.Append($ch) }
    }
    $fields = New-Object System.Collections.Generic.List[string]
    if ($BuildPathMode -eq "path") {
        [void]$fields.Add('  "buildPath": "' + $escaped.ToString() + '"')
    } elseif ($BuildPathMode -eq "empty") {
        [void]$fields.Add('  "buildPath": ""')
    } elseif ($BuildPathMode -eq "prose") {
        [void]$fields.Add('  "pak": "' + $escaped.ToString() + ' (built outside the worktree)"')
    }
    [void]$fields.Add('  "pakSha256": ' + $shaJson)
    $body = ""
    foreach ($field in $fields) {
        if ($body.Length -gt 0) { $body = $body + ",`n" }
        $body = $body + $field
    }
    $json = "{`n" + $body + "`n}`n"
    $manifest = Join-Path $caseRoot "manifest.json"
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($manifest, $json, $utf8)
    $rootForDeploy = $playdh
    if (-not [string]::IsNullOrWhiteSpace($PlayDhRoot)) { $rootForDeploy = $PlayDhRoot }
    $result = Invoke-CleanDeploySupplementPak -ManifestPath $manifest -PlayDhRoot $rootForDeploy -DryRun:$DryRun
    return [pscustomobject]@{
        Result = $result
        Source = $source
        Target = $target
        PlayDh = $playdh
    }
}

function Invoke-CleanDeploySelfTestCases {
    param([Parameter(Mandatory = $true)][string]$Root)

    $hashOk = $false
    $hashFile = Join-Path ([System.IO.Path]::GetTempPath()) ("clean-deploy-hash-" + [Guid]::NewGuid().ToString("N") + ".txt")
    try {
        $hashCommand = Get-Command -Name Get-FileHash -ErrorAction SilentlyContinue
        if ($null -ne $hashCommand) {
            $utf8Hash = New-Object System.Text.UTF8Encoding $false
            [System.IO.File]::WriteAllText($hashFile, "clean-deploy", $utf8Hash)
            $hash = Get-FileHash -LiteralPath $hashFile -Algorithm SHA256
            $hashText = [string]$hash.Hash
            $hashOk = ($hashText -match "^[0-9A-Fa-f]{64}$")
        }
    } catch {
        $hashOk = $false
    } finally {
        if (Test-Path -LiteralPath $hashFile) {
            Remove-Item -LiteralPath $hashFile -Force -ErrorAction SilentlyContinue
        }
    }
    if (-not $hashOk) {
        Write-Host "Get-FileHash is not available. Clear PSModulePath or launch scripts\clean-deploy.cmd before Windows PowerShell 5.1."
    }
    Assert-CleanDeploySelfTest -Name "Get-FileHash is available (use clean-deploy.cmd if PSModulePath hides it)" -Condition $hashOk

    $launcherPath = Join-Path $script:CleanDeployScriptRoot "clean-deploy.cmd"
    $launcherOk = $false
    if (Test-Path -LiteralPath $launcherPath -PathType Leaf) {
        $launcherBytes = [System.IO.File]::ReadAllBytes($launcherPath)
        $launcherAscii = $true
        if ($launcherBytes.Length -ge 3) {
            if (($launcherBytes[0] -eq 0xEF) -and ($launcherBytes[1] -eq 0xBB) -and ($launcherBytes[2] -eq 0xBF)) {
                $launcherAscii = $false
            }
        }
        foreach ($launcherByte in $launcherBytes) {
            if (([int]$launcherByte) -gt 127) { $launcherAscii = $false }
        }
        $launcherText = [System.IO.File]::ReadAllText($launcherPath)
        $launcherOk = $launcherAscii -and ($launcherText.Contains('set "PSModulePath="')) -and ($launcherText.Contains("powershell.exe")) -and ($launcherText.Contains("clean-deploy.ps1"))
    }
    Assert-CleanDeploySelfTest -Name "clean-deploy.cmd clears PSModulePath before powershell.exe" -Condition $launcherOk

    $mismatch = New-CleanDeploySelfTestRun -Root $Root -Name "mismatch" -WriteSource -SourceBody "mismatch-bytes" -WriteTarget -TargetBody "keep-target" -ShaMode "zeros"
    $mismatchHash = Get-CleanDeploySha256 -LiteralPath $mismatch.Source
    $mismatchTargetHash = Get-CleanDeploySha256 -LiteralPath $mismatch.Target
    $zeros = "0000000000000000000000000000000000000000000000000000000000000000"
    $mismatchLine = "[INFO] supplemental pak sha256 actual=source=$mismatchHash;target=$mismatchTargetHash expected=$zeros"
    Assert-CleanDeploySelfTest -Name "sha256 mismatch is an error" -Condition (-not $mismatch.Result.Ok)
    Assert-CleanDeploySelfTest -Name "sha256 mismatch prints [ERROR]" -Condition ($mismatch.Result.Lines -contains "[ERROR] supplemental pak sha256 mismatch")
    Assert-CleanDeploySelfTest -Name "sha log line records actual and expected sha256" -Condition ($mismatch.Result.Lines -contains $mismatchLine)
    $kept = ""
    if (Test-Path -LiteralPath $mismatch.Target) {
        $kept = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($mismatch.Target))
    }
    Assert-CleanDeploySelfTest -Name "sha256 mismatch does not replace the target pak" -Condition ($kept -eq "keep-target")

    $missing = New-CleanDeploySelfTestRun -Root $Root -Name "missing" -ShaMode "zeros"
    $missingLine = "[INFO] supplemental pak sha256 actual=absent expected=$zeros"
    Assert-CleanDeploySelfTest -Name "missing pak is an error" -Condition (-not $missing.Result.Ok)
    Assert-CleanDeploySelfTest -Name "missing pak prints [ERROR]" -Condition ($missing.Result.Lines -contains "[ERROR] supplemental pak is missing")
    Assert-CleanDeploySelfTest -Name "missing pak sha log line records actual and expected" -Condition ($missing.Result.Lines -contains $missingLine)

    $present = New-CleanDeploySelfTestRun -Root $Root -Name "empty-source" -WriteSource -SourceBody "present-source" -ShaMode "null"
    $presentHash = Get-CleanDeploySha256 -LiteralPath $present.Source
    $presentLine = "[INFO] supplemental pak sha256 actual=$presentHash expected=empty"
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak present is an error" -Condition (-not $present.Result.Ok)
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak present prints [ERROR]" -Condition ($present.Result.Lines -contains "[ERROR] supplemental pak is present but pakSha256 is empty")
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak present logs actual and expected" -Condition ($present.Result.Lines -contains $presentLine)

    $presentTarget = New-CleanDeploySelfTestRun -Root $Root -Name "empty-target" -WriteTarget -TargetBody "present-target" -ShaMode "null"
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with only the deploy target present is an error" -Condition (-not $presentTarget.Result.Ok)
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with only the deploy target prints [ERROR]" -Condition ($presentTarget.Result.Lines -contains "[ERROR] supplemental pak is present but pakSha256 is empty")

    $absent = New-CleanDeploySelfTestRun -Root $Root -Name "empty-absent" -ShaMode "null"
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak absent continues" -Condition $absent.Result.Ok
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak absent prints [INFO] supplemental pak not yet built" -Condition ($absent.Result.Lines -contains "[INFO] supplemental pak not yet built")
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak absent logs actual and expected" -Condition ($absent.Result.Lines -contains "[INFO] supplemental pak sha256 actual=absent expected=empty")
    Assert-CleanDeploySelfTest -Name "empty pakSha256 with the pak absent does not create a target" -Condition (-not (Test-Path -LiteralPath $absent.Target))

    $absentEmpty = New-CleanDeploySelfTestRun -Root $Root -Name "empty-string" -ShaMode "empty"
    Assert-CleanDeploySelfTest -Name "blank pakSha256 with the pak absent continues" -Condition $absentEmpty.Result.Ok

    $noBuildPath = New-CleanDeploySelfTestRun -Root $Root -Name "buildpath-missing" -ShaMode "null" -BuildPathMode "omit"
    $noBuildPathText = $noBuildPath.Result.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "buildPath missing is an error" -Condition (-not $noBuildPath.Result.Ok)
    Assert-CleanDeploySelfTest -Name "buildPath missing prints [ERROR]" -Condition ($noBuildPath.Result.Lines -contains "[ERROR] supplemental manifest buildPath is missing")
    Assert-CleanDeploySelfTest -Name "buildPath missing does not continue as not yet built" -Condition (-not $noBuildPathText.Contains("[INFO] supplemental pak not yet built"))

    $proseOnly = New-CleanDeploySelfTestRun -Root $Root -Name "buildpath-prose" -ShaMode "null" -BuildPathMode "prose"
    $proseText = $proseOnly.Result.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "pak description is not used as buildPath" -Condition ((-not $proseOnly.Result.Ok) -and ($proseOnly.Result.Lines -contains "[ERROR] supplemental manifest buildPath is missing"))
    Assert-CleanDeploySelfTest -Name "pak description does not continue as not yet built" -Condition (-not $proseText.Contains("[INFO] supplemental pak not yet built"))

    $emptyBuildPath = New-CleanDeploySelfTestRun -Root $Root -Name "buildpath-empty" -ShaMode "null" -BuildPathMode "empty"
    $emptyBuildText = $emptyBuildPath.Result.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "buildPath empty is an error" -Condition (-not $emptyBuildPath.Result.Ok)
    Assert-CleanDeploySelfTest -Name "buildPath empty prints [ERROR]" -Condition ($emptyBuildPath.Result.Lines -contains "[ERROR] supplemental manifest buildPath is empty")
    Assert-CleanDeploySelfTest -Name "buildPath empty does not continue as not yet built" -Condition (-not $emptyBuildText.Contains("[INFO] supplemental pak not yet built"))

    $copied = New-CleanDeploySelfTestRun -Root $Root -Name "copy" -WriteSource -SourceBody "good-pak-bytes" -ShaMode "match-source"
    $copiedHash = ""
    $targetHash = ""
    if (Test-Path -LiteralPath $copied.Source) { $copiedHash = Get-CleanDeploySha256 -LiteralPath $copied.Source }
    if (Test-Path -LiteralPath $copied.Target) { $targetHash = Get-CleanDeploySha256 -LiteralPath $copied.Target }
    $copyLine = "[INFO] supplemental pak sha256 actual=$copiedHash expected=$copiedHash"
    Assert-CleanDeploySelfTest -Name "filled pakSha256 copies a matching source into the PlayDH root" -Condition ($copied.Result.Ok -and ($copiedHash -eq $targetHash) -and ($copiedHash -ne ""))
    Assert-CleanDeploySelfTest -Name "filled pakSha256 sha log line records actual and expected" -Condition ($copied.Result.Lines -contains $copyLine)

    $dry = New-CleanDeploySelfTestRun -Root $Root -Name "dry-run" -WriteSource -SourceBody "good-pak-bytes" -ShaMode "match-source" -DryRun
    $dryText = $dry.Result.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "dry-run does not copy the supplemental pak" -Condition ($dry.Result.Ok -and (-not (Test-Path -LiteralPath $dry.Target)) -and ($dryText.Contains("would copy")))

    $stale = New-CleanDeploySelfTestRun -Root $Root -Name "stale" -WriteSource -SourceBody "new-pak-bytes" -WriteTarget -TargetBody "old-pak-bytes" -ShaMode "match-source"
    $staleSource = Get-CleanDeploySha256 -LiteralPath $stale.Source
    $staleTarget = ""
    if (Test-Path -LiteralPath $stale.Target) { $staleTarget = Get-CleanDeploySha256 -LiteralPath $stale.Target }
    Assert-CleanDeploySelfTest -Name "matching source replaces a stale target pak" -Condition ($stale.Result.Ok -and ($staleSource -eq $staleTarget))

    $already = New-CleanDeploySelfTestRun -Root $Root -Name "already" -WriteTarget -TargetBody "already-there" -ShaMode "match-target"
    Assert-CleanDeploySelfTest -Name "filled pakSha256 accepts a matching target when the source is absent" -Condition $already.Result.Ok
    Assert-CleanDeploySelfTest -Name "matching target prints the sha log line" -Condition (($already.Result.Lines -join "`n").Contains("actual=") -and (($already.Result.Lines -join "`n").Contains("expected=")))

    $forbidden = New-CleanDeploySelfTestRun -Root $Root -Name "forbidden" -WriteSource -SourceBody "do-not-write" -ShaMode "match-source" -PlayDhRoot "D:\MX\PlayDH"
    $forbiddenText = $forbidden.Result.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "refusing to write the supplemental pak under D:\MX" -Condition ((-not $forbidden.Result.Ok) -and ($forbiddenText.Contains("[ERROR] refusing to write supplemental pak under D:\MX")))
    Assert-CleanDeploySelfTest -Name "D:\MX refusal does not create the pak" -Condition (-not (Test-Path -LiteralPath "D:\MX\PlayDH\zz_supplement.pak"))

    $validNames = @("Character.pak", "Effect.pak", "Map.pak", "monster.pak", "npc.pak", "Pet.pak", "Titan.pak", "zz_supplement.pak")
    $validLine = Format-CleanDeploySelfTestMountLine -Names $validNames
    $validResult = Test-CleanDeploySupplementMountOrder -LogText $validLine
    Assert-CleanDeploySelfTest -Name "valid 8-pak mount order passes" -Condition $validResult.Ok
    Assert-CleanDeploySelfTest -Name "valid 8-pak mount order prints the 8-pak info line" -Condition ($validResult.Lines -contains "[INFO] [pak] mount order lists 8 paks ending with zz_supplement.pak")

    $sevenNames = @("Character.pak", "Effect.pak", "Map.pak", "monster.pak", "npc.pak", "Pet.pak", "zz_supplement.pak")
    $sevenResult = Test-CleanDeploySupplementMountOrder -LogText (Format-CleanDeploySelfTestMountLine -Names $sevenNames)
    Assert-CleanDeploySelfTest -Name "7-pak mount order is an error" -Condition ((-not $sevenResult.Ok) -and ($sevenResult.Lines -contains "[ERROR] [pak] mount order must list exactly 8 paks"))

    $nineNames = @("Character.pak", "Effect.pak", "Map.pak", "monster.pak", "npc.pak", "Pet.pak", "Titan.pak", "extra.pak", "zz_supplement.pak")
    $nineResult = Test-CleanDeploySupplementMountOrder -LogText (Format-CleanDeploySelfTestMountLine -Names $nineNames)
    Assert-CleanDeploySelfTest -Name "9-pak mount order is an error" -Condition ((-not $nineResult.Ok) -and ($nineResult.Lines -contains "[ERROR] [pak] mount order must list exactly 8 paks"))

    $dupNames = @("Character.pak", "Effect.pak", "Map.pak", "monster.pak", "npc.pak", "Pet.pak", "zz_supplement.pak", "zz_supplement.pak")
    $dupResult = Test-CleanDeploySupplementMountOrder -LogText (Format-CleanDeploySelfTestMountLine -Names $dupNames)
    Assert-CleanDeploySelfTest -Name "duplicated zz_supplement.pak is an error" -Condition ((-not $dupResult.Ok) -and ($dupResult.Lines -contains "[ERROR] [pak] mount order must include zz_supplement.pak exactly once"))

    $notLastNames = @("Character.pak", "Effect.pak", "Map.pak", "monster.pak", "npc.pak", "Pet.pak", "zz_supplement.pak", "Titan.pak")
    $notLastResult = Test-CleanDeploySupplementMountOrder -LogText (Format-CleanDeploySelfTestMountLine -Names $notLastNames)
    Assert-CleanDeploySelfTest -Name "zz_supplement.pak not last is an error" -Condition ((-not $notLastResult.Ok) -and ($notLastResult.Lines -contains "[ERROR] [pak] mount order must end with zz_supplement.pak"))

    $trick = "[pak] mount order: 1=not_zz_supplement.pak"
    $trickResult = Test-CleanDeploySupplementMountOrder -LogText $trick
    Assert-CleanDeploySelfTest -Name "a pak name that only ends with the same suffix is not last" -Condition (-not $trickResult.Ok)

    $none = Test-CleanDeploySupplementMountOrder -LogText "client started"
    Assert-CleanDeploySelfTest -Name "missing mount order line is an error" -Condition ((-not $none.Ok) -and ($none.Lines -contains "[ERROR] [pak] mount order line is missing"))

    $mountInfo = "[INFO] [pak] mount order lists 8 paks ending with zz_supplement.pak"
    $missingLogError = "[ERROR] current smoke run log is missing"
    $build = Join-Path $Root "smoke-current"
    $oldRun = Join-Path (Join-Path (Join-Path $build "runtime") "gui-smoke") "previous"
    $oldLogs = Join-Path $oldRun "logs"
    New-Item -ItemType Directory -Path $oldLogs -Force | Out-Null
    $staleBad = Format-CleanDeploySelfTestMountLine -Names $sevenNames
    Write-CleanDeploySelfTestSmokeLog -Path (Join-Path $oldLogs "client.err.log") -Text ($staleBad + "`n")
    (Get-Item -LiteralPath $oldRun).LastWriteTime = (Get-Date)
    $before = Get-CleanDeployGuiSmokeRunNames -BuildDir $build
    $newRun = Join-Path (Join-Path (Join-Path $build "runtime") "gui-smoke") "current"
    $newLogs = Join-Path $newRun "logs"
    New-Item -ItemType Directory -Path $newLogs -Force | Out-Null
    Write-CleanDeploySelfTestSmokeLog -Path (Join-Path $newLogs "client.err.log") -Text ($validLine + "`n") -Utf16
    (Get-Item -LiteralPath $newRun).LastWriteTime = (Get-Date).AddHours(-3)
    $after = Get-CleanDeployGuiSmokeRunNames -BuildDir $build
    $currentMount = Invoke-CleanDeployCurrentSmokeMountCheck -BuildDir $build -Before $before -After $after
    Assert-CleanDeploySelfTest -Name "stale newer smoke log is ignored for this run" -Condition ($currentMount.Ok -and ($currentMount.Lines -contains $mountInfo))
    Assert-CleanDeploySelfTest -Name "stale newer smoke log is not the mount-order result" -Condition (-not (($currentMount.Lines -join "`n").Contains("[ERROR] [pak] mount order must list exactly 8 paks")))

    $staleOnly = Join-Path $Root "smoke-stale-only"
    $staleRun = Join-Path (Join-Path (Join-Path $staleOnly "runtime") "gui-smoke") "previous"
    $staleLogs = Join-Path $staleRun "logs"
    New-Item -ItemType Directory -Path $staleLogs -Force | Out-Null
    Write-CleanDeploySelfTestSmokeLog -Path (Join-Path $staleLogs "client.out.log") -Text ($validLine + "`n")
    (Get-Item -LiteralPath $staleRun).LastWriteTime = (Get-Date)
    $staleBefore = Get-CleanDeployGuiSmokeRunNames -BuildDir $staleOnly
    $staleAfter = Get-CleanDeployGuiSmokeRunNames -BuildDir $staleOnly
    $staleRejected = Invoke-CleanDeployCurrentSmokeMountCheck -BuildDir $staleOnly -Before $staleBefore -After $staleAfter
    $staleRejectedText = $staleRejected.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "no new smoke run rejects a stale valid log" -Condition ((-not $staleRejected.Ok) -and ($staleRejected.Lines -contains $missingLogError))
    Assert-CleanDeploySelfTest -Name "no new smoke run does not accept the stale log" -Condition (-not $staleRejectedText.Contains($mountInfo))

    $missingBuild = Join-Path $Root "smoke-missing-log"
    $missingOld = Join-Path (Join-Path (Join-Path $missingBuild "runtime") "gui-smoke") "previous"
    $missingOldLogs = Join-Path $missingOld "logs"
    New-Item -ItemType Directory -Path $missingOldLogs -Force | Out-Null
    Write-CleanDeploySelfTestSmokeLog -Path (Join-Path $missingOldLogs "client.err.log") -Text ($validLine + "`n")
    (Get-Item -LiteralPath $missingOld).LastWriteTime = (Get-Date)
    $missingBefore = Get-CleanDeployGuiSmokeRunNames -BuildDir $missingBuild
    $missingNew = Join-Path (Join-Path (Join-Path $missingBuild "runtime") "gui-smoke") "current"
    New-Item -ItemType Directory -Path (Join-Path $missingNew "logs") -Force | Out-Null
    (Get-Item -LiteralPath $missingNew).LastWriteTime = (Get-Date).AddHours(-3)
    $missingAfter = Get-CleanDeployGuiSmokeRunNames -BuildDir $missingBuild
    $missingLog = Invoke-CleanDeployCurrentSmokeMountCheck -BuildDir $missingBuild -Before $missingBefore -After $missingAfter
    $missingLogText = $missingLog.Lines -join "`n"
    Assert-CleanDeploySelfTest -Name "missing current smoke run log is an error" -Condition ((-not $missingLog.Ok) -and ($missingLog.Lines -contains $missingLogError))
    Assert-CleanDeploySelfTest -Name "missing current smoke run log does not fall back" -Condition (-not $missingLogText.Contains($mountInfo))

    $scriptPath = $script:CleanDeployScriptPath
    $kind = "bad"
    if (-not [string]::IsNullOrWhiteSpace($scriptPath) -and (Test-Path -LiteralPath $scriptPath)) {
        $kind = Get-CleanDeployScriptEncodingKind -Path $scriptPath
    }
    $encodingOk = ($kind -eq "ascii") -or ($kind -eq "utf8-bom")
    Assert-CleanDeploySelfTest -Name "script bytes are pure ASCII or UTF-8 with BOM" -Condition $encodingOk
    Assert-CleanDeploySelfTest -Name "script bytes are pure ASCII" -Condition ($kind -eq "ascii")

    $scriptText = ""
    if (-not [string]::IsNullOrWhiteSpace($scriptPath) -and (Test-Path -LiteralPath $scriptPath)) {
        $scriptText = Read-CleanDeployUtf8Text -Path $scriptPath
    }
    $banned = [string]::Concat([char]63, [char]63)
    $parallel = '-' + 'Parallel'
    $newestNeedle = "Sort-Object" + " LastWriteTime"
    $oldParser = "Get-CleanDeploySupplement" + "SourceFromManifest"
    $syntaxOk = (-not $scriptText.Contains($banned)) -and (-not $scriptText.Contains($parallel))
    Assert-CleanDeploySelfTest -Name "script stays on Windows PowerShell 5.1 syntax" -Condition $syntaxOk
    $notesStart = $scriptText.IndexOf(".NOTES")
    $notesEnd = $scriptText.IndexOf("#>")
    $notesRegion = ""
    if (($notesStart -ge 0) -and ($notesEnd -gt $notesStart)) {
        $notesRegion = $scriptText.Substring($notesStart, ($notesEnd - $notesStart))
    }
    $launcherMention = ($notesRegion.IndexOf("clean-deploy.cmd") -ge 0) -and ($notesRegion.IndexOf("PSModulePath") -ge 0)
    Assert-CleanDeploySelfTest -Name "help mentions clean-deploy.cmd and PSModulePath" -Condition $launcherMention
    Assert-CleanDeploySelfTest -Name "script does not select a smoke log by LastWriteTime" -Condition (-not $scriptText.Contains($newestNeedle))
    Assert-CleanDeploySelfTest -Name "script does not parse the pak description" -Condition (-not $scriptText.Contains($oldParser))

    $legacy = [System.Text.Encoding]::UTF8.GetBytes((Get-CleanDeployLegacyPlayDhDirName))
    $wantLegacy = [byte[]](
        229, 162, 168, 233, 166, 153, 227, 128, 144, 230, 186, 144,
        231, 160, 129, 233, 133, 141, 229, 165, 151, 232, 181, 132,
        230, 186, 144, 227, 128, 145)
    $legacyOk = $legacy.Length -eq $wantLegacy.Length
    if ($legacyOk) {
        for ($i = 0; $i -lt $legacy.Length; $i++) {
            if ($legacy[$i] -ne $wantLegacy[$i]) { $legacyOk = $false }
        }
    }
    Assert-CleanDeploySelfTest -Name "legacy PlayDH directory name bytes are unchanged" -Condition $legacyOk

    $repo = Split-Path -Parent $script:CleanDeployScriptRoot
    $manifestPath = Join-Path (Join-Path $repo "reference") "manifests"
    $manifestPath = Join-Path $manifestPath "playdh-supplemental-fill.json"
    $buildPathValue = ""
    $buildPathStatus = "missing"
    if (Test-Path -LiteralPath $manifestPath) {
        $repoManifest = (Read-CleanDeployUtf8Text -Path $manifestPath) | ConvertFrom-Json
        $parsedBuild = Get-CleanDeploySupplementBuildPath -Manifest $repoManifest
        $buildPathStatus = $parsedBuild.Status
        $buildPathValue = $parsedBuild.Path
    }
    Assert-CleanDeploySelfTest -Name "manifest buildPath is D:\build-mxr\supplement\zz_supplement.pak" -Condition (($buildPathStatus -eq "ok") -and ($buildPathValue -eq "D:\build-mxr\supplement\zz_supplement.pak"))

    $currentPath = Join-Path (Join-Path $repo "reference") "manifests"
    $currentPath = Join-Path $currentPath "playdh-current.sha256.json"
    $currentOk = $false
    if (Test-Path -LiteralPath $currentPath) {
        $currentText = Read-CleanDeployUtf8Text -Path $currentPath
        $currentOk = $currentText.IndexOf("zz_supplement.pak") -lt 0
    }
    Assert-CleanDeploySelfTest -Name "playdh-current.sha256.json does not include zz_supplement.pak" -Condition $currentOk

    $repoPlay = Join-Path (Join-Path (Join-Path $repo "modern") "data") "PlayDH"
    $repoPak = Join-Path $repoPlay "zz_supplement.pak"
    Assert-CleanDeploySelfTest -Name "self-test does not add zz_supplement.pak to the repo PlayDH" -Condition (-not (Test-Path -LiteralPath $repoPak))

    Invoke-CleanDeploySelfTestPlayDhRootCases -Root $Root -Repo $repo
    Invoke-CleanDeploySelfTestSkipCases -Root $Root
}

function Get-CleanDeploySelfTestTreeText {
    param([Parameter(Mandatory = $true)][string]$RootPath)
    $prefix = $RootPath.TrimEnd('\', '/')
    $items = @(Get-ChildItem -LiteralPath $RootPath -Recurse -Force -ErrorAction SilentlyContinue | ForEach-Object {
        $_.FullName.Substring($prefix.Length).ToLowerInvariant()
    })
    $sorted = @($items | Sort-Object)
    return ($sorted -join "`n")
}

function Get-CleanDeploySelfTestDataStamp {
    param([string]$Repo)
    $data = Join-Path (Join-Path $Repo "modern") "data"
    if (-not (Test-Path -LiteralPath $data)) { return "missing" }
    $names = @(Get-ChildItem -LiteralPath $data -Force | ForEach-Object { $_.Name + "=" + [int]$_.Attributes })
    return (($names | Sort-Object) -join "|")
}

function Test-CleanDeploySelfTestReparse {
    param([string]$LiteralPath)
    if (-not (Test-Path -LiteralPath $LiteralPath)) { return $false }
    $item = Get-Item -LiteralPath $LiteralPath -Force
    return [bool]($item.Attributes -band [IO.FileAttributes]::ReparsePoint)
}

function Invoke-CleanDeploySelfTestChild {
    param(
        [Parameter(Mandatory = $true)][string]$WorkRoot,
        [string[]]$ArgumentList,
        [hashtable]$Environment
    )
    try {
        $exe = ""
        try {
            $exe = [System.Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
        } catch {
            $exe = ""
        }
        if ([string]::IsNullOrWhiteSpace($exe)) {
            $exe = (Get-Process -Id $PID).Path
        }
        $quote = [string][char]34
        $argText = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File " + $quote + $script:CleanDeployScriptPath + $quote
        foreach ($one in $ArgumentList) {
            $escaped = ([string]$one).Replace($quote, ('\' + $quote))
            $argText = $argText + " " + $quote + $escaped + $quote
        }
        $proc = New-Object System.Diagnostics.Process
        $proc.StartInfo.FileName = $exe
        $proc.StartInfo.Arguments = $argText
        $proc.StartInfo.WorkingDirectory = $WorkRoot
        $proc.StartInfo.UseShellExecute = $false
        $proc.StartInfo.RedirectStandardOutput = $true
        $proc.StartInfo.RedirectStandardError = $true
        $proc.StartInfo.CreateNoWindow = $true
        if ($null -ne $Environment) {
            foreach ($envKey in @($Environment.Keys)) {
                $proc.StartInfo.EnvironmentVariables[[string]$envKey] = [string]$Environment[$envKey]
            }
        }
        [void]$proc.Start()
        $outTask = $proc.StandardOutput.ReadToEndAsync()
        $errTask = $proc.StandardError.ReadToEndAsync()
        $finished = $proc.WaitForExit(90000)
        if (-not $finished) {
            try { $proc.Kill() } catch { }
            [void]$proc.WaitForExit()
        }
        $text = [string]$outTask.Result
        $errText = [string]$errTask.Result
        if (-not [string]::IsNullOrEmpty($errText)) {
            if ($text.Length -gt 0) { $text = $text + "`n" }
            $text = $text + $errText
        }
        return [pscustomobject]@{
            ExitCode = [int]$proc.ExitCode
            Text = $text
        }
    } catch {
        return [pscustomobject]@{
            ExitCode = -1
            Text = [string]$_.Exception.Message
        }
    }
}

function Test-CleanDeployCommercialSmokeEphemeral {
    param([string]$Text)
    $normalized = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    $physical = $normalized.Split([char]10)
    $logical = New-Object System.Collections.Generic.List[string]
    $buffer = ""
    $tick = [string][char]96
    foreach ($physicalLine in $physical) {
        $piece = [string]$physicalLine
        $continued = $false
        if ($piece.EndsWith($tick)) {
            $continued = $true
            if ($piece.Length -gt 0) { $piece = $piece.Substring(0, $piece.Length - 1) }
        }
        if ($buffer.Length -eq 0) {
            $buffer = $piece
        } else {
            $buffer = $buffer + " " + $piece.Trim()
        }
        if (-not $continued) {
            [void]$logical.Add($buffer)
            $buffer = ""
        }
    }
    if ($buffer.Length -gt 0) { [void]$logical.Add($buffer) }
    $hits = 0
    $bad = 0
    foreach ($line in $logical) {
        $trim = $line.Trim()
        if ($trim.Length -eq 0) { continue }
        if ($trim.StartsWith("#")) { continue }
        if ($trim.Contains("gui-client-smoke.ps1")) {
            $hits = $hits + 1
            if (-not $trim.Contains("-EphemeralLocalServer")) { $bad = $bad + 1 }
        }
    }
    return [pscustomobject]@{
        Hits = $hits
        Bad = $bad
    }
}

function Invoke-CleanDeployCommercialSmokeHostCases {
    param([Parameter(Mandatory = $true)][string]$SmokePath)
    $belowOk = $false
    $aboveOk = $false
    $tokens = $null
    $parseErrors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($SmokePath, [ref]$tokens, [ref]$parseErrors)
    if ($null -ne $parseErrors -and $parseErrors.Count -gt 0) {
        return [pscustomobject]@{ BelowOk = $false; AboveOk = $false }
    }
    $finder = { param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }
    $functions = @($ast.FindAll($finder, $true))
    foreach ($fn in $functions) {
        Invoke-Expression $fn.Extent.Text
    }
    $required = '[ERROR] commercial-smoke requires PowerShell 7.4+ (pwsh) for -EphemeralLocalServer; run it with pwsh 7.4+'
    $script:CommercialSmokeHostInvoked = $false
    $lowRunner = {
        param(
            [string]$File,
            [string]$BuildDir,
            [switch]$EphemeralLocalServer
        )
        $script:CommercialSmokeHostInvoked = $true
    }
    $belowOk = $true
    foreach ($version in @([version]'5.1.26100.9711', [version]'7.3.9')) {
        $script:CommercialSmokeHostInvoked = $false
        $result = Invoke-CommercialSmokeGuiClient -BuildDir 'unused-build' -HostVersion $version -Runner $lowRunner -ScriptRoot $script:CleanDeployScriptRoot
        if ($null -eq $result) { $belowOk = $false; continue }
        if ($result.ExitCode -eq 0) { $belowOk = $false }
        if ($result.Invoked) { $belowOk = $false }
        if ($script:CommercialSmokeHostInvoked) { $belowOk = $false }
        if ([string]$result.Message -ne $required) { $belowOk = $false }
    }
    $script:CommercialSmokeHostFile = ''
    $script:CommercialSmokeHostSwitch = $false
    $highRunner = {
        param(
            [string]$File,
            [string]$BuildDir,
            [switch]$EphemeralLocalServer
        )
        $script:CommercialSmokeHostInvoked = $true
        $script:CommercialSmokeHostFile = $File
        $script:CommercialSmokeHostSwitch = [bool]$EphemeralLocalServer
    }
    $aboveOk = $true
    foreach ($version in @([version]'7.4', [version]'7.4.6', [version]'7.6.9')) {
        $script:CommercialSmokeHostInvoked = $false
        $script:CommercialSmokeHostFile = ''
        $script:CommercialSmokeHostSwitch = $false
        $result = Invoke-CommercialSmokeGuiClient -BuildDir 'unused-build' -HostVersion $version -Runner $highRunner -ScriptRoot $script:CleanDeployScriptRoot
        if ($null -eq $result) { $aboveOk = $false; continue }
        if ($result.ExitCode -ne 0) { $aboveOk = $false }
        if (-not $result.Invoked) { $aboveOk = $false }
        if (-not $script:CommercialSmokeHostInvoked) { $aboveOk = $false }
        if (-not $script:CommercialSmokeHostSwitch) { $aboveOk = $false }
        if (-not ([string]$script:CommercialSmokeHostFile).EndsWith('gui-client-smoke.ps1')) { $aboveOk = $false }
    }
    $smokeText = Read-CleanDeployUtf8Text -Path $SmokePath
    if ($smokeText.Contains('Get-Command pwsh') -or $smokeText.Contains('pwsh.exe')) {
        $belowOk = $false
        $aboveOk = $false
    }
    return [pscustomobject]@{
        BelowOk = [bool]$belowOk
        AboveOk = [bool]$aboveOk
    }
}

function Invoke-CleanDeploySelfTestPlayDhRootCases {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Repo
    )
    $fake = Join-Path $Root "fake-repo"
    $fakeModern = Join-Path $fake "modern"
    New-Item -ItemType Directory -Path $fakeModern -Force | Out-Null
    $inside = Join-Path $fake "inside-playdh"
    New-Item -ItemType Directory -Path $inside -Force | Out-Null
    $positive = Join-Path $Root "positive-root"
    New-Item -ItemType Directory -Path $positive -Force | Out-Null
    $dryDir = Join-Path $Root "dry-root"
    New-Item -ItemType Directory -Path $dryDir -Force | Out-Null
    $fakeFull = [System.IO.Path]::GetFullPath($fake)
    $insideFull = [System.IO.Path]::GetFullPath($inside)
    $positiveFull = [System.IO.Path]::GetFullPath($positive)
    $dryFull = [System.IO.Path]::GetFullPath($dryDir)
    # Placeholder manifest so a later dry-run child can pass step 3b.
    # The sha is filled in after the in-process copy, before that child.
    # Path list (not contents) is what the tree snapshot compares.
    $notBuiltPak = Join-CleanDeployPath -Parent (Join-Path $Root "not-built-source") -Child "zz_supplement.pak"
    $fakeManifest = Join-Path $fakeFull "reference"
    $fakeManifest = Join-Path $fakeManifest "manifests"
    $fakeManifest = Join-Path $fakeManifest "playdh-supplemental-fill.json"
    New-CleanDeploySelfTestManifestFile -Path $fakeManifest -BuildPath $notBuiltPak -ShaJson "null"
    $treeBefore = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $realBefore = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $repoPlay = Join-Path (Join-Path (Join-Path $Repo "modern") "data") "PlayDH"
    $repoPak = Join-Path $repoPlay "zz_supplement.pak"

    $missingFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot "" -RepoRoot $fakeFull
    $blankFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot "   " -RepoRoot $fakeFull
    $missingText = ($missingFn.Lines -join "`n")
    $blankText = ($blankFn.Lines -join "`n")
    $missingChild = Invoke-CleanDeploySelfTestChild -WorkRoot $fakeFull -ArgumentList @("-RepoRoot", $fakeFull)
    $missingTree = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $missingReal = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $missingOk = (-not $missingFn.Ok) -and (-not $blankFn.Ok) -and $missingText.Contains("[ERROR] -PlayDhRoot is required") -and $blankText.Contains("[ERROR] -PlayDhRoot is required") -and ($missingChild.ExitCode -ne 0) -and $missingChild.Text.Contains("[ERROR] -PlayDhRoot is required") -and ($missingTree -eq $treeBefore) -and ($missingReal -eq $realBefore) -and (-not (Test-Path -LiteralPath $repoPak))
    Assert-CleanDeploySelfTest -Name "missing -PlayDhRoot fails with [ERROR] and no write" -Condition $missingOk

    $relativeRaw = "relative\playdh"
    $relativeDot = ".\PlayDH"
    $relativeFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $relativeRaw -RepoRoot $fakeFull
    $relativeDotFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $relativeDot -RepoRoot $fakeFull
    $relativeText = ($relativeFn.Lines -join "`n")
    $relativeDotText = ($relativeDotFn.Lines -join "`n")
    $relativeChild = Invoke-CleanDeploySelfTestChild -WorkRoot $fakeFull -ArgumentList @("-RepoRoot", $fakeFull, "-PlayDhRoot", $relativeRaw)
    $relativeTree = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $relativeReal = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $relativeOk = (-not $relativeFn.Ok) -and (-not $relativeDotFn.Ok) -and $relativeText.Contains("[ERROR] -PlayDhRoot must be an absolute path:") -and $relativeDotText.Contains("[ERROR] -PlayDhRoot must be an absolute path:") -and ($relativeChild.ExitCode -ne 0) -and $relativeChild.Text.Contains("[ERROR] -PlayDhRoot must be an absolute path:") -and ($relativeTree -eq $treeBefore) -and ($relativeReal -eq $realBefore) -and (-not (Test-Path -LiteralPath $repoPak))
    Assert-CleanDeploySelfTest -Name "relative -PlayDhRoot fails with [ERROR] and no write" -Condition $relativeOk

    $mxCases = @(
        "D:\MX",
        "D:\MX\",
        "d:\mx\PlayDH",
        "D:/MX/PlayDH",
        "D:\foo\..\MX\PlayDH",
        "D:\MX\.\PlayDH\",
        "\\?\D:\MX\PlayDH"
    )
    $mxAll = $true
    foreach ($mxCase in $mxCases) {
        $mxFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $mxCase -RepoRoot $fakeFull
        $mxFnText = ($mxFn.Lines -join "`n")
        if ($mxFn.Ok -or -not $mxFnText.Contains("[ERROR] -PlayDhRoot resolves under D:\MX:")) { $mxAll = $false }
    }
    $escapedMx = Resolve-CleanDeployPlayDhRoot -PlayDhRoot "D:\MX\..\safe\playdh" -RepoRoot $fakeFull
    $escapedText = ($escapedMx.Lines -join "`n")
    $escapedOk = (-not $escapedMx.Ok) -and $escapedText.Contains("[ERROR] -PlayDhRoot must be an existing directory:") -and (-not $escapedText.Contains("under D:\MX"))
    $mxChildArg = "D:\foo\..\MX\PlayDH"
    $mxChild = Invoke-CleanDeploySelfTestChild -WorkRoot $fakeFull -ArgumentList @("-RepoRoot", $fakeFull, "-PlayDhRoot", $mxChildArg)
    $mxTree = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $mxReal = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $mxCreated = Test-Path -LiteralPath "D:\MX\PlayDH\zz_supplement.pak"
    $mxOk = $mxAll -and $escapedOk -and ($mxChild.ExitCode -ne 0) -and $mxChild.Text.Contains("[ERROR] -PlayDhRoot resolves under D:\MX:") -and ($mxTree -eq $treeBefore) -and ($mxReal -eq $realBefore) -and (-not $mxCreated) -and (-not (Test-Path -LiteralPath $repoPak))
    Assert-CleanDeploySelfTest -Name "PlayDhRoot under D:\MX fails with [ERROR] and no write" -Condition $mxOk

    $dottedInside = $insideFull.TrimEnd('\', '/') + "/../inside-playdh/"
    $insideFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $insideFull -RepoRoot $fakeFull
    $selfFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $fakeFull -RepoRoot $fakeFull
    $trailFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot ($insideFull.TrimEnd('\', '/') + "/") -RepoRoot $fakeFull
    $dottedFn = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $dottedInside -RepoRoot $fakeFull
    $insideText = ($insideFn.Lines -join "`n")
    $selfText = ($selfFn.Lines -join "`n")
    $trailText = ($trailFn.Lines -join "`n")
    $dottedText = ($dottedFn.Lines -join "`n")
    $insideChild = Invoke-CleanDeploySelfTestChild -WorkRoot $fakeFull -ArgumentList @("-RepoRoot", $fakeFull, "-PlayDhRoot", $insideFull)
    $insideTree = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $insideReal = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $insideErr = "[ERROR] -PlayDhRoot resolves inside the repo:"
    $fakeDataPlay = Join-Path (Join-Path $fakeModern "data") "PlayDH"
    Assert-CleanDeploySelfTest -Name "PlayDhRoot inside the repo is rejected" -Condition (-not $insideFn.Ok)
    Assert-CleanDeploySelfTest -Name "PlayDhRoot equal to RepoRoot is rejected" -Condition (-not $selfFn.Ok)
    Assert-CleanDeploySelfTest -Name "PlayDhRoot with a trailing slash inside the repo is rejected" -Condition (-not $trailFn.Ok)
    Assert-CleanDeploySelfTest -Name "PlayDhRoot with dot-dot inside the repo is rejected" -Condition (-not $dottedFn.Ok)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot error names the inside path" -Condition $insideText.Contains($insideErr)
    Assert-CleanDeploySelfTest -Name "RepoRoot PlayDhRoot error names the inside path" -Condition $selfText.Contains($insideErr)
    Assert-CleanDeploySelfTest -Name "trailing-slash PlayDhRoot error names the inside path" -Condition $trailText.Contains($insideErr)
    Assert-CleanDeploySelfTest -Name "dot-dot PlayDhRoot error names the inside path" -Condition $dottedText.Contains($insideErr)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot child exits non-zero" -Condition ($insideChild.ExitCode -ne 0)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot child prints the inside-repo error" -Condition $insideChild.Text.Contains($insideErr)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot leaves the fake repo tree unchanged" -Condition ($insideTree -eq $treeBefore)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot leaves the real data stamp unchanged" -Condition ($insideReal -eq $realBefore)
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot does not create modern/data/PlayDH" -Condition (-not (Test-Path -LiteralPath $fakeDataPlay))
    Assert-CleanDeploySelfTest -Name "inside PlayDhRoot does not add a repo pak" -Condition (-not (Test-Path -LiteralPath $repoPak))

    $positiveResolved = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $positiveFull -RepoRoot $fakeFull
    $copied = New-CleanDeploySelfTestRun -Root $Root -Name "positive-pak" -WriteSource -SourceBody "positive-pak-bytes" -ShaMode "match-source" -PlayDhRoot $positiveResolved.Path
    $landed = Join-CleanDeployPath -Parent $positiveResolved.Path -Child "zz_supplement.pak"
    $landedOk = $false
    $landedHash = ""
    $sourceHash = ""
    if (Test-Path -LiteralPath $landed) { $landedHash = Get-CleanDeploySha256 -LiteralPath $landed }
    if (Test-Path -LiteralPath $copied.Source) { $sourceHash = Get-CleanDeploySha256 -LiteralPath $copied.Source }
    if (($landedHash -ne "") -and ($landedHash -eq $sourceHash)) { $landedOk = $true }
    $dryCopy = New-CleanDeploySelfTestRun -Root $Root -Name "dry-external" -WriteSource -SourceBody "dry-pak-bytes" -ShaMode "match-source" -PlayDhRoot $dryFull -DryRun
    $dryPak = Join-CleanDeployPath -Parent $dryFull -Child "zz_supplement.pak"
    $dryCopyText = ($dryCopy.Result.Lines -join "`n")
    # Earlier children exit at PlayDhRoot validation and never reach step 4.
    # This child has a valid root, so it must not depend on host cmake/vs2022.
    # -SkipBuild keeps it out of the build. The manifest sha matches the pak
    # already copied above so step 3b does not treat that pak as an error.
    if (($landedHash -ne "") -and ($landedHash -eq $sourceHash)) {
        New-CleanDeploySelfTestManifestFile -Path $fakeManifest -BuildPath $notBuiltPak -ShaJson ('"' + $landedHash + '"')
    }
    $dryChild = Invoke-CleanDeploySelfTestChild -WorkRoot $fakeFull -ArgumentList @("-RepoRoot", $fakeFull, "-PlayDhRoot", $positiveResolved.Path, "-DryRun", "-SkipBuild", "-SkipTests", "-SkipSmoke")
    $rootLine = "[INFO] PlayDH root: " + $positiveResolved.Path
    $positiveTree = Get-CleanDeploySelfTestTreeText -RootPath $fakeFull
    $positiveReal = Get-CleanDeploySelfTestDataStamp -Repo $Repo
    $fakePlay = Join-Path (Join-Path $fakeModern "data") "PlayDH"
    $junctionNeedle = "New-Item -ItemType " + "Junction"
    $linkToolNeedle = "mk" + "link"
    $scriptText = ""
    if (-not [string]::IsNullOrWhiteSpace($script:CleanDeployScriptPath) -and (Test-Path -LiteralPath $script:CleanDeployScriptPath)) {
        $scriptText = Read-CleanDeployUtf8Text -Path $script:CleanDeployScriptPath
    }
    $noJunctionCmd = (-not $scriptText.Contains($junctionNeedle)) -and (-not $scriptText.ToLowerInvariant().Contains($linkToolNeedle))
    $positiveReparse = Test-CleanDeploySelfTestReparse -LiteralPath $positiveResolved.Path
    $repoReparse = Test-CleanDeploySelfTestReparse -LiteralPath $repoPlay
    $childReparse = $false
    foreach ($childItem in @(Get-ChildItem -LiteralPath $positiveResolved.Path -Recurse -Force -ErrorAction SilentlyContinue)) {
        if ($childItem.Attributes -band [IO.FileAttributes]::ReparsePoint) { $childReparse = $true }
    }
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot resolves" -Condition $positiveResolved.Ok
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot copy succeeds" -Condition $copied.Result.Ok
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot landed hash matches the source" -Condition $landedOk
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot does not add a repo pak" -Condition (-not (Test-Path -LiteralPath $repoPak))
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot does not create a fake repo PlayDH" -Condition (-not (Test-Path -LiteralPath $fakePlay))
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot target is not a reparse point" -Condition (-not $positiveReparse)
    Assert-CleanDeploySelfTest -Name "repo PlayDH is not a reparse point" -Condition (-not $repoReparse)
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot children are not reparse points" -Condition (-not $childReparse)
    Assert-CleanDeploySelfTest -Name "script text has no junction or link command" -Condition $noJunctionCmd
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot leaves the fake repo tree unchanged" -Condition ($positiveTree -eq $treeBefore)
    Assert-CleanDeploySelfTest -Name "absolute PlayDhRoot leaves the real data stamp unchanged" -Condition ($positiveReal -eq $realBefore)
    $dryCopyOk = $dryCopy.Result.Ok -and (-not (Test-Path -LiteralPath $dryPak)) -and $dryCopyText.Contains("would copy")
    Assert-CleanDeploySelfTest -Name "dry-run copy reports would copy and writes no pak" -Condition $dryCopyOk
    $dryChildRootOk = $dryChild.Text.Contains($rootLine)
    $dryChildClean = -not $dryChild.Text.Contains("[ERROR]")
    if ((-not $dryChildRootOk) -or (-not $dryChildClean)) {
        Write-CleanDeploySelfTestChildDetail -Child $dryChild -Name "dry child"
    }
    Assert-CleanDeploySelfTest -Name "dry child prints the PlayDH root line" -Condition $dryChildRootOk
    Assert-CleanDeploySelfTest -Name "dry child has no [ERROR]" -Condition $dryChildClean

    $savedPlay = $env:MXH_PLAYDH_ROOT
    $hadPlay = $false
    if ($null -ne $savedPlay) { $hadPlay = $true }
    $envOk = $false
    try {
        $env:MXH_PLAYDH_ROOT = "preset-playdh-value"
        $dryPub = Publish-CleanDeployPlayDhRoot -Path $positiveResolved.Path -DryRun
        $dryPubText = ($dryPub.Lines -join "`n")
        $stillPreset = ($env:MXH_PLAYDH_ROOT -eq "preset-playdh-value")
        $livePub = Publish-CleanDeployPlayDhRoot -Path $positiveResolved.Path
        $liveText = ($livePub.Lines -join "`n")
        $envAssign = '$env:MXH_PLAYDH_ROOT = $Path'
        $envOk = $stillPreset -and $dryPubText.Contains($rootLine) -and $dryPubText.Contains("(dry-run)") -and ($env:MXH_PLAYDH_ROOT -eq $positiveResolved.Path) -and $liveText.Contains("[INFO] MXH_PLAYDH_ROOT=" + $positiveResolved.Path) -and $scriptText.Contains($envAssign)
    } finally {
        if ($hadPlay) {
            $env:MXH_PLAYDH_ROOT = $savedPlay
        } else {
            Remove-Item -Path "Env:MXH_PLAYDH_ROOT" -ErrorAction SilentlyContinue
        }
    }
    Assert-CleanDeploySelfTest -Name "MXH_PLAYDH_ROOT is set from -PlayDhRoot and dry-run leaves it unchanged" -Condition $envOk

    $smokePath = Join-Path $script:CleanDeployScriptRoot "commercial-smoke.ps1"
    $smokeText = ""
    if (Test-Path -LiteralPath $smokePath) {
        $smokeText = Read-CleanDeployUtf8Text -Path $smokePath
    }
    $ephemeral = Test-CleanDeployCommercialSmokeEphemeral -Text $smokeText
    $ephemeralOk = ($ephemeral.Hits -ge 1) -and ($ephemeral.Bad -eq 0)
    Assert-CleanDeploySelfTest -Name "commercial-smoke passes -EphemeralLocalServer to gui-client-smoke" -Condition $ephemeralOk

    $hostCases = Invoke-CleanDeployCommercialSmokeHostCases -SmokePath $smokePath
    Assert-CleanDeploySelfTest -Name "PowerShell below 7.4 errors before gui-client-smoke and does not drop -EphemeralLocalServer" -Condition $hostCases.BelowOk
    Assert-CleanDeploySelfTest -Name "PowerShell 7.4+ invokes gui-client-smoke with -EphemeralLocalServer" -Condition $hostCases.AboveOk

    $cmdPath = Join-Path $script:CleanDeployScriptRoot "clean-deploy.cmd"
    if (Test-Path -LiteralPath $cmdPath -PathType Leaf) {
        $cmdText = [System.IO.File]::ReadAllText($cmdPath)
        $cmdOk = $cmdText.Contains("%*") -and $cmdText.Contains("clean-deploy.ps1") -and $cmdText.Contains('set "PSModulePath="')
        Assert-CleanDeploySelfTest -Name "clean-deploy.cmd forwards arguments including -PlayDhRoot" -Condition $cmdOk
    } else {
        Write-CleanDeploySelfTestSkip -Name "clean-deploy.cmd forwards arguments including -PlayDhRoot"
    }
}

function New-CleanDeploySelfTestManifestFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$BuildPath,
        [Parameter(Mandatory = $true)][string]$ShaJson
    )
    $escaped = New-Object System.Text.StringBuilder
    foreach ($ch in $BuildPath.ToCharArray()) {
        if ($ch -eq '\') { [void]$escaped.Append('\\') }
        elseif ($ch -eq '"') { [void]$escaped.Append('\"') }
        else { [void]$escaped.Append($ch) }
    }
    $json = "{`n  `"buildPath`": `"" + $escaped.ToString() + "`",`n  `"pakSha256`": " + $ShaJson + "`n}`n"
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $json, $utf8)
}

function New-CleanDeploySelfTestToolEnvironment {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string]$Absent = "",
        [string]$Present = "",
        [string]$RehashOverride = ""
    )
    $emptyPath = Join-Path $Root "empty-path"
    $emptyPf = Join-Path $Root "empty-pf86"
    New-Item -ItemType Directory -Path $emptyPath -Force | Out-Null
    New-Item -ItemType Directory -Path $emptyPf -Force | Out-Null
    $map = @{
        PATH = $emptyPath
        "ProgramFiles(x86)" = $emptyPf
    }
    if (-not [string]::IsNullOrWhiteSpace($Absent)) {
        $map["MXH_CLEAN_DEPLOY_PREREQ_ABSENT"] = $Absent
    }
    if (-not [string]::IsNullOrWhiteSpace($Present)) {
        $map["MXH_CLEAN_DEPLOY_PREREQ_PRESENT"] = $Present
    }
    if (-not [string]::IsNullOrWhiteSpace($RehashOverride)) {
        $map["MXH_CLEAN_DEPLOY_REHASH_OVERRIDE"] = $RehashOverride
    }
    return $map
}

function Invoke-CleanDeploySelfTestSkipCases {
    param([Parameter(Mandatory = $true)][string]$Root)
    $case = Join-Path $Root "skip-cases"
    $repo = Join-Path $case "repo"
    $modern = Join-Path $repo "modern"
    $play = Join-Path $case "playdh"
    $sourceDir = Join-Path $case "source"
    New-Item -ItemType Directory -Path $modern -Force | Out-Null
    New-Item -ItemType Directory -Path $play -Force | Out-Null
    New-Item -ItemType Directory -Path $sourceDir -Force | Out-Null
    $repoFull = [System.IO.Path]::GetFullPath($repo)
    $playFull = [System.IO.Path]::GetFullPath($play)
    $source = Join-CleanDeployPath -Parent $sourceDir -Child "zz_supplement.pak"
    $sourceBytes = [System.Text.Encoding]::ASCII.GetBytes("skip-build-pak-bytes")
    [System.IO.File]::WriteAllBytes($source, $sourceBytes)
    $sourceFull = [System.IO.Path]::GetFullPath($source)
    $sourceHash = Get-CleanDeploySha256 -LiteralPath $sourceFull
    $manifest = Join-Path $repoFull "reference"
    $manifest = Join-Path $manifest "manifests"
    $manifest = Join-Path $manifest "playdh-supplemental-fill.json"
    $absentTools = "cmake,sqllocaldb,vs2022,sqlcmd,odbc18,vcredist,git"
    $isolated = New-CleanDeploySelfTestToolEnvironment -Root $case -Absent $absentTools

    $emptyManifest = Join-Path $case "empty-repo"
    $emptyModern = Join-Path $emptyManifest "modern"
    New-Item -ItemType Directory -Path $emptyModern -Force | Out-Null
    $emptyRepoFull = [System.IO.Path]::GetFullPath($emptyManifest)
    $emptyBuild = Join-CleanDeployPath -Parent (Join-Path $case "missing-source") -Child "zz_supplement.pak"
    $emptyManifestPath = Join-Path $emptyRepoFull "reference"
    $emptyManifestPath = Join-Path $emptyManifestPath "manifests"
    $emptyManifestPath = Join-Path $emptyManifestPath "playdh-supplemental-fill.json"
    New-CleanDeploySelfTestManifestFile -Path $emptyManifestPath -BuildPath $emptyBuild -ShaJson "null"
    $dryChild = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $emptyRepoFull, "-PlayDhRoot", $playFull, "-DryRun", "-SkipBuild", "-SkipTests", "-SkipSmoke"
    ) -Environment $isolated
    $dryText = [string]$dryChild.Text
    $dryOk = ($dryChild.ExitCode -eq 0) -and ($dryText.Contains("[INFO] prereqs required for this run: none")) -and ($dryText.Contains("[WARN] cmake : MISSING")) -and ($dryText.Contains("[WARN] sqllocaldb : MISSING")) -and (-not $dryText.Contains("[FAIL] missing prereqs")) -and ($dryText.Contains("supplemental pak")) -and ($dryText.Contains("[SKIP] step 4 build")) -and ($dryText.Contains("[SKIP] step 5 ctest")) -and ($dryText.Contains("[SKIP] step 6 commercial-smoke")) -and ($dryText.Contains("build:") -and $dryText.Contains("SKIPPED"))
    Assert-CleanDeploySelfTest -Name "SkipBuild SkipTests SkipSmoke does not fail when cmake and sqllocaldb are missing" -Condition $dryOk

    $blocked = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $repoFull, "-PlayDhRoot", $playFull, "-DryRun"
    ) -Environment $isolated
    $blockedText = [string]$blocked.Text
    $blockedOk = ($blocked.ExitCode -eq 2) -and ($blockedText.Contains("[FAIL] missing prereqs (")) -and ($blockedText.Contains("sqllocaldb")) -and (-not $blockedText.Contains("supplemental pak")) -and ($blockedText.Contains("[INFO] prereqs required for this run: ")) -and ($blockedText.Contains("cmake")) -and ($blockedText.Contains("vs2022"))
    Assert-CleanDeploySelfTest -Name "sqllocaldb missing without SkipTests still fails at prereqs" -Condition $blockedOk

    $defaultText = $blockedText
    $defaultLine = "[INFO] prereqs required for this run: cmake, vs2022, sqlcmd, sqllocaldb, odbc18, vcredist"
    $defaultOk = ($blocked.ExitCode -eq 2) -and ($defaultText.Contains($defaultLine)) -and ($defaultText.Contains("[FAIL] missing prereqs (cmake, vs2022, sqlcmd, sqllocaldb, odbc18, vcredist)"))
    Assert-CleanDeploySelfTest -Name "default run still requires cmake and vs2022" -Condition $defaultOk

    $readyTree = Get-CleanDeploySelfTestTreeText -RootPath $case
    $noSkipTests = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $repoFull, "-PlayDhRoot", $playFull, "-SkipBuild"
    ) -Environment $isolated
    $noSkipText = [string]$noSkipTests.Text
    $readyAfter = Get-CleanDeploySelfTestTreeText -RootPath $case
    $targetPak = Join-CleanDeployPath -Parent $playFull -Child "zz_supplement.pak"
    $noSkipOk = ($noSkipTests.ExitCode -ne 0) -and ($noSkipText.Contains("[ERROR] -SkipBuild requires -SkipTests")) -and (-not $noSkipText.Contains("copied supplemental pak")) -and (-not $noSkipText.Contains("prereqs required for this run")) -and (-not (Test-Path -LiteralPath $targetPak)) -and ($readyAfter -eq $readyTree)
    Assert-CleanDeploySelfTest -Name "SkipBuild without SkipTests errors with no write" -Condition $noSkipOk

    New-CleanDeploySelfTestManifestFile -Path $manifest -BuildPath $sourceFull -ShaJson ('"' + $sourceHash + '"')
    $copiedChild = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $repoFull, "-PlayDhRoot", $playFull, "-SkipBuild", "-SkipTests", "-SkipSmoke"
    ) -Environment $isolated
    $copiedText = [string]$copiedChild.Text
    $shaLine = "[INFO] supplemental pak sha256 actual=" + $sourceHash + " expected=" + $sourceHash
    $copiedHash = ""
    if (Test-Path -LiteralPath $targetPak) { $copiedHash = Get-CleanDeploySha256 -LiteralPath $targetPak }
    $copiedOk = ($copiedChild.ExitCode -eq 0) -and ($copiedText.Contains($shaLine)) -and ($copiedText.Contains("[INFO] copied supplemental pak to ")) -and ($copiedHash -eq $sourceHash) -and ($copiedText.Contains("[SKIP] step 4 build")) -and ($copiedText.Contains("[SKIP] step 5 ctest")) -and ($copiedText.Contains("[SKIP] step 6 commercial-smoke"))
    Assert-CleanDeploySelfTest -Name "SkipBuild success exits 0 only after pak copy and matching rehash" -Condition $copiedOk

    $rehashPlay = Join-Path $case "playdh-rehash"
    New-Item -ItemType Directory -Path $rehashPlay -Force | Out-Null
    $rehashFull = [System.IO.Path]::GetFullPath($rehashPlay)
    $rehashEnv = New-CleanDeploySelfTestToolEnvironment -Root (Join-Path $case "rehash-env") -Absent $absentTools -RehashOverride "forced-rehash-mismatch"
    $rehashChild = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $repoFull, "-PlayDhRoot", $rehashFull, "-SkipBuild", "-SkipTests", "-SkipSmoke"
    ) -Environment $rehashEnv
    $rehashText = [string]$rehashChild.Text
    $rehashTarget = Join-CleanDeployPath -Parent $rehashFull -Child "zz_supplement.pak"
    $rehashOk = ($rehashChild.ExitCode -ne 0) -and ($rehashText.Contains("[ERROR] supplemental pak sha256 mismatch")) -and (-not (Test-Path -LiteralPath $rehashTarget))
    Assert-CleanDeploySelfTest -Name "post-copy rehash mismatch exits non-zero" -Condition $rehashOk

    $badRoot = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $repoFull, "-PlayDhRoot", "relative\playdh", "-SkipBuild", "-SkipTests", "-SkipSmoke"
    ) -Environment $isolated
    $badText = [string]$badRoot.Text
    $badOk = ($badRoot.ExitCode -ne 0) -and ($badText.Contains("[ERROR] -PlayDhRoot must be an absolute path:")) -and (-not $badText.Contains("[deploy] DONE"))
    Assert-CleanDeploySelfTest -Name "invalid PlayDhRoot exits non-zero when build and tests are skipped" -Condition $badOk

    $hostPlay = Join-Path $case "playdh-host-tools"
    New-Item -ItemType Directory -Path $hostPlay -Force | Out-Null
    $hostPlayFull = [System.IO.Path]::GetFullPath($hostPlay)
    $hostEnv = New-CleanDeploySelfTestToolEnvironment -Root (Join-Path $case "host-with-cmake") -Absent "sqllocaldb" -Present "cmake,vs2022"
    $hostChild = Invoke-CleanDeploySelfTestChild -WorkRoot $case -ArgumentList @(
        "-RepoRoot", $emptyRepoFull, "-PlayDhRoot", $hostPlayFull, "-DryRun", "-SkipBuild", "-SkipTests", "-SkipSmoke"
    ) -Environment $hostEnv
    $hostText = [string]$hostChild.Text
    $hostRoot = "[INFO] PlayDH root: " + $hostPlayFull
    $hostOk = ($hostChild.ExitCode -eq 0) -and $hostText.Contains($hostRoot) -and (-not $hostText.Contains("[ERROR]")) -and $hostText.Contains("[OK]   cmake : self-test-present") -and $hostText.Contains("[OK]   vs2022 : self-test-present") -and $hostText.Contains("[WARN] sqllocaldb : MISSING") -and $hostText.Contains("[INFO] prereqs required for this run: none")
    if (-not $hostOk) {
        Write-CleanDeploySelfTestChildDetail -Child $hostChild -Name "cmake present sqllocaldb absent"
    }
    Assert-CleanDeploySelfTest -Name "SkipBuild dry-run with cmake and vs2022 present and sqllocaldb absent prints the PlayDH root and exits 0" -Condition $hostOk
}

function Invoke-CleanDeploySelfTest {
    $script:CleanDeploySelfTestPass = 0
    $script:CleanDeploySelfTestFail = 0
    $script:CleanDeploySelfTestSkip = 0
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("clean-deploy-supplement-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    try {
        Invoke-CleanDeploySelfTestCases -Root $root
    } catch {
        Write-Host ("[FAIL] self-test aborted: " + $_.Exception.Message)
        $script:CleanDeploySelfTestFail = $script:CleanDeploySelfTestFail + 1
    } finally {
        if (Test-Path -LiteralPath $root) {
            Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-Host ("clean-deploy self-test: PASS={0} FAIL={1} SKIP={2}" -f $script:CleanDeploySelfTestPass, $script:CleanDeploySelfTestFail, $script:CleanDeploySelfTestSkip)
    if ($script:CleanDeploySelfTestFail -gt 0) { return 1 }
    return 0
}





if ($SelfTest) {
    $selfCode = Invoke-CleanDeploySelfTest
    if ($selfCode -ne 0) { exit 1 }
    exit 0
}

if ($SkipBuild -and -not $SkipTests) {
    Write-Host "[ERROR] -SkipBuild requires -SkipTests"
    exit 1
}

# Windows PowerShell 5.1 -File does not turn an uncaught throw into a
# non-zero ERRORLEVEL. clean-deploy.cmd forwards that ERRORLEVEL.
trap {
    if ($script:CleanDeployTrapExit) { exit 1 }
    $script:CleanDeployTrapExit = $true
    try { $host.SetShouldExit(1) } catch { }
    exit 1
}

# ----------------------------------------------------------------------------
# Resolve repo root
# ----------------------------------------------------------------------------

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent $PSScriptRoot
}
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot "modern"))) {
    Fail "RepoRoot does not look like a moxiang checkout: $RepoRoot"
}

# Refuse a bad -PlayDhRoot before preflight, prereq install, pak copy, or build.
$resolvedPlayDh = Resolve-CleanDeployPlayDhRoot -PlayDhRoot $PlayDhRoot -RepoRoot $RepoRoot
if (-not $resolvedPlayDh.Ok) { exit 1 }
$script:CleanDeployChosenPlayDhRoot = $resolvedPlayDh.Path
[void](Publish-CleanDeployPlayDhRoot -Path $script:CleanDeployChosenPlayDhRoot -DryRun:$DryRun)

# ----------------------------------------------------------------------------
# Step 1: preflight
# ----------------------------------------------------------------------------

Step "preflight"
if (Test-Admin) {
    Ok "running as Administrator"
} else {
    Warn "not running as Administrator; prereq install will require elevation"
}

try { $os = (Get-CimInstance Win32_OperatingSystem -ErrorAction Stop).Caption } catch { $os = [System.Environment]::OSVersion.VersionString }
$arch = $env:PROCESSOR_ARCHITECTURE
try { $ram = [math]::Round((Get-CimInstance Win32_ComputerSystem -ErrorAction Stop).TotalPhysicalMemory / 1GB, 1) } catch { $ram = -1 }
try { $free = [math]::Round(([System.IO.DriveInfo]::GetDrives() | Where-Object { $_.IsReady -and $_.DriveType -eq "Fixed" } | Measure-Object -Sum AvailableFreeSpace).Sum / 1GB, 1) } catch { $free = -1 }
if ($ram -lt 0) { Warn "RAM could not be detected (likely non-admin); preflight check skipped"; $ram = 999 }
if ($free -lt 0) { Warn "free disk could not be detected (likely non-admin); preflight check skipped"; $free = 999 }
Info  "OS: $os"
Info  "Arch: $arch"
Info  "RAM: $ram GB"
Info  "Free disk: $free GB"
if ($ram -lt 4)  { Fail "need >= 4 GB RAM (have $ram GB)" }
if ($free -lt 5) { Fail "need >= 5 GB free disk (have $free GB)" }
Ok "preflight OK"


# ----------------------------------------------------------------------------
# Step 2: prereq detection
# ----------------------------------------------------------------------------

Step "detecting prereqs"
$state = [ordered]@{
    cmake      = $null
    git        = $null
    vswhere    = $null
    vs2022     = $null
    sqlcmd     = $null
    sqllocaldb = $null
    odbc18     = $null
    vcredist   = $null
    chocolatey = $null
}

$state.cmake      = Test-Tool "cmake"
$state.git        = Test-Tool "git"
$state.sqlcmd     = Test-Tool "sqlcmd"
$state.sqllocaldb = Test-Tool "SqlLocalDB"
try {
    $state.vcredist = (Test-Path "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64") -or `
                      (Test-Path "HKLM:\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64")
} catch {
    $state.vcredist = $false
}
try {
    $state.odbc18 = Test-Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\ODBC Driver 18 for SQL Server"
} catch {
    $state.odbc18 = $false
}
$state.chocolatey = Test-Tool "choco"

$programFilesX86 = ${env:ProgramFiles(x86)}
if (-not [string]::IsNullOrWhiteSpace([string]$programFilesX86)) {
    $vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $state.vswhere = $vswhere
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($vs) { $state.vs2022 = $vs.Trim() }
    }
}

foreach ($key in @("cmake","git","sqlcmd","sqllocaldb","vcredist","odbc18","chocolatey","vs2022")) {
    if (Test-CleanDeployPrereqForcedAbsent -Name $key) {
        $state[$key] = $null
    } elseif ((-not $state[$key]) -and (Test-CleanDeployPrereqForcedPresent -Name $key)) {
        $state[$key] = "self-test-present"
    }
    if ($state[$key]) {
        Ok "$key : $($state[$key])"
    } else {
        Warn "$key : MISSING"
    }
}

$requiredResult = Get-CleanDeployRequiredPrereqNames -SkipBuild:$SkipBuild -SkipTests:$SkipTests -SkipSmoke:$SkipSmoke
$requiredNames = @()
if ($null -ne $requiredResult -and $null -ne $requiredResult.Names) {
    $requiredNames = @($requiredResult.Names)
}
$requiredLabel = "none"
if ($requiredNames.Count -gt 0) {
    $requiredLabel = ($requiredNames -join ", ")
}
Write-Host ("[INFO] prereqs required for this run: " + $requiredLabel)

$missing = @()
foreach ($key in $requiredNames) {
    if (-not $state[$key]) { $missing += $key }
}

if ($missing.Count -gt 0 -and -not $InstallPrereqs) {
    Fail ("missing prereqs ({0}); rerun with -InstallPrereqs" -f ($missing -join ", ")) 2
}


# ----------------------------------------------------------------------------
# Step 3: PlayDH root
# -PlayDhRoot was validated above. Do not create or modify modern/data/PlayDH.
# ----------------------------------------------------------------------------

Step "PlayDH root"
Info ("-PlayDhRoot " + $script:CleanDeployChosenPlayDhRoot)
Ok "PlayDH root accepted"


# ----------------------------------------------------------------------------
# Step 3b: supplemental pak
# The source path is the buildPath field. The pak description is not parsed.
# The pak itself stays outside the repo until this copy into -PlayDhRoot.
# ----------------------------------------------------------------------------

Step "supplemental pak"
$supplementManifest = Join-Path $RepoRoot "reference\manifests\playdh-supplemental-fill.json"
$supplement = Invoke-CleanDeploySupplementPak -ManifestPath $supplementManifest -PlayDhRoot $script:CleanDeployChosenPlayDhRoot -DryRun:$DryRun
if ($null -eq $supplement) { exit 1 }
$script:CleanDeploySupplementExpectedFilled = [bool]$supplement.ExpectedFilled
if (-not $supplement.Ok) { exit 1 }


# ----------------------------------------------------------------------------
# Step 4: build
# ----------------------------------------------------------------------------

$buildDir = Join-Path $RepoRoot "modern\build"
if ($SkipBuild) {
    Write-Host "[SKIP] step 4 build"
} else {
    Step "build modern ($Config)"
    $buildScript = Join-Path $PSScriptRoot "build-modern.ps1"
    if (-not (Test-Path -LiteralPath $buildScript)) {
        Fail "build-modern.ps1 not found at $buildScript" 3
    }

    if ($Force -and (Test-Path -LiteralPath $buildDir)) {
        if ($DryRun) {
            Info "would remove build dir $buildDir"
        } else {
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
    }

    if ($DryRun) {
        Info ("would run: cmake --build modern/build --config  + chr 36 + Config")
    } else {
        Write-Host ('  > ' + $buildScript + ' -Config ' + $Config) -ForegroundColor DarkGray
        Push-Location -LiteralPath $RepoRoot
        try {
            & cmake --build modern/build --config $Config
            if ($LASTEXITCODE -ne 0) { Fail ('cmake build failed exit $LASTEXITCODE; exit 3') 3 }
        } finally { Pop-Location }
    }
    Ok 'build OK'
}
# ----------------------------------------------------------------------------
# Step 5: ctest
# ----------------------------------------------------------------------------

if (-not $SkipTests) {
    Step "ctest -C $Config"
    if ($DryRun) {
#         Info ('would run: ctest -C ' + $Config + ' --test-dir ' + $buildDir + ' --output-on-failure')
    } else {
        Write-Host ('  > ctest -C ' + $Config + ' --test-dir ' + $buildDir + ' --output-on-failure') -ForegroundColor DarkGray
        ctest -C $Config --test-dir $buildDir --output-on-failure
        if ($LASTEXITCODE -ne 0) { Fail ('ctest failed exit $LASTEXITCODE; exit 4') 4 }
    }
    Ok 'ctest OK'
} else {
    Write-Host "[SKIP] step 5 ctest"
}
# ----------------------------------------------------------------------------
# Step 6: commercial smoke
# ----------------------------------------------------------------------------

$smokeBefore = $null
if (-not $SkipSmoke) {
    Step "commercial-smoke"
    $smokeScript = Join-Path $PSScriptRoot "commercial-smoke.ps1"
    if (-not (Test-Path -LiteralPath $smokeScript)) {
        Fail "commercial-smoke.ps1 not found at $smokeScript"
    }
    $smokeArgs = @()
    if ($SkipGui) { $smokeArgs += "-SkipGui" }
    if ($DryRun) {
        Info ('would run: ' + $smokeScript + ' -BuildDir ' + $buildDir + ' ' + $smokeArgs + ')')
    } else {
        if ($script:CleanDeploySupplementExpectedFilled) {
            $smokeBefore = Get-CleanDeployGuiSmokeRunNames -BuildDir $buildDir
        }
        Write-Host ('  > ' + $smokeScript + ' -BuildDir ' + $buildDir + ' ' + $smokeArgs) -ForegroundColor DarkGray
        & $smokeScript -BuildDir $buildDir $smokeArgs -ErrorAction Stop
        if ($LASTEXITCODE -ne 0) { Fail ('commercial-smoke failed exit $LASTEXITCODE; exit 5') 5 }
    }
    Ok 'commercial-smoke OK'
} else {
    Write-Host "[SKIP] step 6 commercial-smoke"
}
if ($script:CleanDeploySupplementExpectedFilled) {
    if ($DryRun) {
        Write-Host "[INFO] would check this run's [pak] mount order lists exactly 8 paks ending with zz_supplement.pak"
    } elseif ($SkipSmoke) {
        Write-Host "[INFO] supplemental pak mount order check skipped with -SkipSmoke"
    } else {
        $smokeAfter = Get-CleanDeployGuiSmokeRunNames -BuildDir $buildDir
        $mountOrder = Invoke-CleanDeployCurrentSmokeMountCheck -BuildDir $buildDir -Before $smokeBefore -After $smokeAfter
        if (-not $mountOrder.Ok) { exit 1 }
    }
}

# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------

Step "summary"
Info "Repo:               $RepoRoot"
Info "Config:             $Config"
Info ("PlayDH root:        " + $script:CleanDeployChosenPlayDhRoot)
Info ("build:              {0}" -f $(if ($SkipBuild) { "SKIPPED" } else { "PASSED" }))
Info ("ctest:              {0}" -f $(if ($SkipTests) { "SKIPPED" } else { "PASSED" }))
Info ("commercial-smoke:   {0}" -f $(if ($SkipSmoke) { "SKIPPED" } else { "PASSED" }))
Info "Next step (manual): scripts/start-server.ps1 -Mode start (legacy stack) OR launch modern mxh_* servers from modern\build\bin"

Write-Host ""
Write-Host "[deploy] DONE" -ForegroundColor Green
exit 0

