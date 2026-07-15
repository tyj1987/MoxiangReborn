# Build 4DyuchiNET as 32-bit DLL
$ErrorActionPreference = 'Stop'

$msvcVer = "14.44.35207"
$hostX86 = "C:\BuildTools\VC\Tools\MSVC\$msvcVer\bin\Hostx86\x86"
$libX86 = "C:\BuildTools\VC\Tools\MSVC\$msvcVer\lib\x86"
$includeMSVC = "C:\BuildTools\VC\Tools\MSVC\$msvcVer\include"
$ucrtInclude = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Include\*\ucrt" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
$ucrtLib = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Lib\*\ucrt\x86" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
$umInclude = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Include\*\um" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
$umLib = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Lib\*\um\x86" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName

$env:PATH = "$hostX86;$env:PATH"
$env:INCLUDE = "$includeMSVC;$ucrtInclude;$umInclude"
$env:LIB = "$libX86;$ucrtLib;$umLib"
$env:LIBPATH = "$libX86;$ucrtLib;$umLib"

Write-Host "Compiler: $hostX86\cl.exe"

$repoRoot = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）"
$buildDir = "$repoRoot\modern\build_net32"
$srcDir = "$repoRoot\墨香【源码】\4DyuchiNET_Latest"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

Write-Host "=== CMake Configure ==="
cmake -S $srcDir -B $buildDir -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configure FAILED"
    exit 1
}

Write-Host "=== CMake Build ==="
cmake --build $buildDir --config Release --parallel 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build FAILED"
    exit 1
}

Write-Host "=== BUILD SUCCESS ==="
$dllPath = Get-ChildItem -Path $buildDir -Recurse -Filter "4DyuchiNET.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($dllPath) {
    Write-Host "Output: $($dllPath.FullName) ($($dllPath.Length) bytes)"
} else {
    Write-Host "WARNING: 4DyuchiNET.dll not found"
    Get-ChildItem $buildDir -Recurse -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $($_.FullName)" }
}
