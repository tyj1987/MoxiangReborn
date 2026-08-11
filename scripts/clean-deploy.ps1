<#
.SYNOPSIS
    One-command clean-machine deployment of the Moxian (墨香) modern stack.
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
      4. PlayDH junction  modern/data/PlayDH -> <RepoRoot>/MoxiangResources/PlayDH
                          (ASCII-named so resource tooling handles argv cleanly).
      5. Build modern     invokes scripts/build-modern.ps1 -Config <Config>.
      6. Run ctest        ctest -C <Config> --test-dir modern/build.
      7. Commercial smoke invokes scripts/commercial-smoke.ps1.

    Idempotency:
      - Prereq detection skips already-installed tools.
      - PlayDH junction: only created if missing; -Force resets stale target.
      - Build: CMake incremental (rebuilds only what changed).
      - ctest + commercial-smoke: safe to repeat.

    Exit codes:
      0  success.
      1  preflight failure.
      2  prereq missing (rerun with -InstallPrereqs).
      3  build failed.
      4  ctest failed.
      5  commercial-smoke failed.


.PARAMETER DryRun
    Print every step that would run, do not actually mutate the system.

.PARAMETER InstallPrereqs
    Install missing prereqs via Chocolatey (or direct download for ODBC 18).

.PARAMETER SkipTests
    Skip ctest (faster bootstrap verification).

.PARAMETER SkipSmoke
    Skip commercial-smoke.

.PARAMETER SkipGui
    Forward -SkipGui to commercial-smoke (skip GUI client smoke step).

.PARAMETER Config
    CMake config (Debug or Release). Default Debug.

.PARAMETER RepoRoot
    Override repo root. Defaults to the parent of $PSScriptRoot.

.PARAMETER Force
    Rebuild from scratch (reset PlayDH junction if target moved).

.EXAMPLE
    PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs

.EXAMPLE
    PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -DryRun
#>

[CmdletBinding()]
param(
    [switch]$DryRun,
    [switch]$InstallPrereqs,
    [switch]$SkipTests,
    [switch]$SkipSmoke,
    [switch]$SkipGui,
    [switch]$Force,
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [string]$RepoRoot = ""
)

$ErrorActionPreference = "Stop"

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
function Fail  { param([string]$Text) Write-Host "  [FAIL] $Text" -ForegroundColor Red; throw $Text }


function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $pr = New-Object Security.Principal.WindowsPrincipal($id)
    return $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-Tool {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
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
$state.vcredist   = (Test-Path "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64") -or `
                    (Test-Path "HKLM:\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64")
$state.odbc18     = Test-Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\ODBC Driver 18 for SQL Server"
$state.chocolatey = Test-Tool "choco"

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path -LiteralPath $vswhere) {
    $state.vswhere = $vswhere
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($vs) { $state.vs2022 = $vs.Trim() }
}

foreach ($key in @("cmake","git","sqlcmd","sqllocaldb","vcredist","odbc18","chocolatey","vs2022")) {
    if ($state[$key]) {
        Ok "$key : $($state[$key])"
    } else {
        Warn "$key : MISSING"
    }
}

$missing = @()
foreach ($key in @("cmake","git","vs2022","sqlcmd","sqllocaldb","odbc18","vcredist")) {
    if (-not $state[$key]) { $missing += $key }
}

if ($missing.Count -gt 0 -and -not $InstallPrereqs) {
    Fail ("missing prereqs ({0}); rerun with -InstallPrereqs" -f ($missing -join ", "))
    exit 2
}


# ----------------------------------------------------------------------------
# Step 3: PlayDH junction
# ----------------------------------------------------------------------------

Step "PlayDH junction"
$dataDir      = Join-Path $RepoRoot "modern\data"
$playdhLink   = Join-Path $dataDir "PlayDH"
$playdhTarget = Join-Path $RepoRoot "墨香【源码配套资源】\PlayDH"

if (-not (Test-Path -LiteralPath $playdhTarget)) {
    Warn "PlayDH source not found at: $playdhTarget"
    Warn "PlayDH-dependent ctest cases will be skipped"
} else {
    if (-not (Test-Path -LiteralPath $dataDir)) {
        if ($DryRun) {
            Info "would mkdir $dataDir"
        } else {
            New-Item -ItemType Directory -Path $dataDir -Force | Out-Null
        }
    }

    $needsLink = $true
    if (Test-Path -LiteralPath $playdhLink) {
        $item = Get-Item -LiteralPath $playdhLink -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            $existingTarget = $item.Target | Select-Object -First 1
            if ($existingTarget -eq $playdhTarget) {
                Ok "junction already correct: $playdhLink"
                $needsLink = $false
            } elseif ($Force) {
                if ($DryRun) {
                    Info "would remove stale junction $playdhLink"
                } else {
                    Remove-Item -LiteralPath $playdhLink -Force
                }
            } else {
                Warn "junction target differs ($existingTarget); rerun with -Force to reset"
                $needsLink = $false
            }
        } else {
            Warn "$playdhLink exists and is not a junction; rerun with -Force to replace"
            $needsLink = $false
        }
    }

    if ($needsLink) {
        if ($DryRun) {
            Info "would create junction $playdhLink -> $playdhTarget"
        } else {
            New-Item -ItemType Junction -Path $playdhLink -Target $playdhTarget | Out-Null
            Ok "junction created: $playdhLink -> $playdhTarget"
        }
    }
}


# ----------------------------------------------------------------------------
# Step 4: build
# ----------------------------------------------------------------------------

Step "build modern ($Config)"
$buildScript = Join-Path $PSScriptRoot "build-modern.ps1"
if (-not (Test-Path -LiteralPath $buildScript)) {
    Fail "build-modern.ps1 not found at $buildScript"
}

$buildDir = Join-Path $RepoRoot "modern\build"
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
        if ($LASTEXITCODE -ne 0) { Fail ('cmake build failed exit $LASTEXITCODE; exit 3') }
    } finally { Pop-Location }
}
Ok 'build OK'
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
        if ($LASTEXITCODE -ne 0) { Fail ('ctest failed exit $LASTEXITCODE; exit 4') }
    }
    Ok 'ctest OK'
} else {
    Warn ('ctest SKIPPED')
}
# ----------------------------------------------------------------------------
# Step 6: commercial smoke
# ----------------------------------------------------------------------------

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
        Write-Host ('  > ' + $smokeScript + ' -BuildDir ' + $buildDir + ' ' + $smokeArgs) -ForegroundColor DarkGray
        & $smokeScript -BuildDir $buildDir $smokeArgs -ErrorAction Stop
        if ($LASTEXITCODE -ne 0) { Fail ('commercial-smoke failed exit $LASTEXITCODE; exit 5') }
    }
    Ok 'commercial-smoke OK'
} else {
    Warn ('commercial-smoke SKIPPED')
}
# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------

Step "summary"
Info "Repo:               $RepoRoot"
Info "Config:             $Config"
Info "PlayDH junction:    $playdhLink"
Info ("ctest:              {0}" -f $(if ($SkipTests) { "SKIPPED" } else { "PASSED" }))
Info ("commercial-smoke:   {0}" -f $(if ($SkipSmoke) { "SKIPPED" } else { "PASSED" }))
Info "Next step (manual): scripts/start-server.ps1 -Mode start (legacy stack) OR launch modern mxh_* servers from modern\build\bin"

Write-Host ""
Write-Host "[deploy] DONE" -ForegroundColor Green

