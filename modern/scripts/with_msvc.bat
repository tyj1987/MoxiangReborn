@echo off
REM Wrapper that sources vcvars64.bat and forwards to cmake.
REM Usage: with_msvc.bat <cmake-args...>
call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cmake %*
exit /b %errorlevel%