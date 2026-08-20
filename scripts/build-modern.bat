@echo off
:: Setup the supported x86 MSVC environment and build modern/.
:: Usage: scripts\build-modern.bat [Debug|Release] [target ...]

setlocal
set "REPO_ROOT=%~dp0.."
set "BUILD_DIR=%REPO_ROOT%\modern\build"
set "VCVARSALL=C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
set "NINJA=C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "CONFIG=%~1"
if "%CONFIG%"=="" (
    set "CONFIG=Debug"
) else (
    shift
)
set "TARGETS="
:collect_targets
if "%~1"=="" goto targets_collected
set "TARGETS=%TARGETS% %~1"
shift
goto collect_targets
:targets_collected

if not exist "%VCVARSALL%" (
    echo [ERROR] vcvarsall.bat not found: %VCVARSALL%
    exit /b 1
)
call "%VCVARSALL%" x86 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvarsall.bat x86 failed
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    if not exist "%NINJA%" (
        echo [ERROR] Ninja not found: %NINJA%
        exit /b 1
    )
    echo [BUILD] CMake configure x86 Ninja %CONFIG%...
    cmake -S "%REPO_ROOT%\modern" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_MAKE_PROGRAM="%NINJA%"
    if errorlevel 1 (
        echo [ERROR] CMake configure failed
        exit /b 1
    )
)

echo [BUILD] x86 %CONFIG%...
if "%TARGETS%"=="" (
    cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
) else (
    cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel --target %TARGETS%
)
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [BUILD] x86 %CONFIG% OK
endlocal
