<#
.SYNOPSIS
    现代化墨香项目的一次性初始化脚本。

.DESCRIPTION
    - 检测 VS 2022 / CMake / Git
    - 创建 modern/build 目录
    - 配置 + 构建 MoxianCompat + MoxianResourceExplorer
    - 运行单元测试

.EXAMPLE
    .\scripts\setup-modern.ps1
#>

[CmdletBinding()]
param(
    [switch]$SkipTests,
    [switch]$Verbose
)

$ErrorActionPreference = 'Stop'

function Write-Header([string]$text) {
    Write-Host ''
    Write-Host ('=' * 60) -ForegroundColor Cyan
    Write-Host " $text" -ForegroundColor Cyan
    Write-Host ('=' * 60) -ForegroundColor Cyan
}

# Detect project root.
$root = Split-Path -Parent $PSScriptRoot
Write-Header "Moxian-Reborn setup"
Write-Host "Root: $root" -ForegroundColor DarkGray

# Detect tools.
function Test-Tool([string]$name) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) {
        Write-Host "  [OK]   $name -> $($cmd.Source)" -ForegroundColor Green
        return $true
    }
    Write-Host "  [FAIL] $name not found" -ForegroundColor Red
    return $false
}

Write-Header "Checking required tools"
$cmake_ok = Test-Tool 'cmake'
$git_ok  = Test-Tool 'git'
$vs_ok   = $false

# VS2022 detection via vswhere.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vs) {
        Write-Host "  [OK]   Visual Studio -> $vs" -ForegroundColor Green
        $vs_ok = $true
    }
}
if (-not $vs_ok) {
    Write-Host "  [WARN] Visual Studio 2022 not detected. CMake will need a generator." -ForegroundColor Yellow
}

# Build.
Write-Header "Configuring with CMake"
$buildDir = Join-Path $root 'modern\build'
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

if (-not $cmake_ok) {
    Write-Host "CMake not found; skipping build. Install CMake and rerun." -ForegroundColor Yellow
    return
}

# Try VS2022 generator first; fall back to Ninja.
$gen = 'Visual Studio 17 2022'
$arch = 'Win32'

Push-Location $root
try {
    Write-Host "  Generator: $gen | Arch: $arch" -ForegroundColor DarkGray
    cmake -S modern -B $buildDir -G $gen -A $arch
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed (exit $LASTEXITCODE)"
    }

    Write-Header "Building"
    cmake --build $buildDir --config Release --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed (exit $LASTEXITCODE)"
    }

    if (-not $SkipTests) {
        Write-Header "Running tests"
        ctest --test-dir $buildDir -C Release --output-on-failure
    }

    Write-Header "Done"
    $exe = Join-Path $buildDir 'Release\mxh_explorer.exe'
    if (Test-Path $exe) {
        Write-Host "Resource explorer built: $exe" -ForegroundColor Green
        Write-Host ''
        Write-Host 'Try it:' -ForegroundColor Cyan
        Write-Host "  & '$exe' --help" -ForegroundColor Gray
    }
} finally {
    Pop-Location
}