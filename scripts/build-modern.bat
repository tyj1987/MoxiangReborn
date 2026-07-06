@echo off
:: Setup MSVC env and build modern/
:: Usage: scripts\build-modern.bat [Release|Debug]

setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)

cd /d "%~dp0\..\modern"
if not exist build mkdir build

echo [BUILD] CMake configure (%CONFIG%)...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

echo [BUILD] Building...
cmake --build build --config %CONFIG% --parallel
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [BUILD] OK
endlocal