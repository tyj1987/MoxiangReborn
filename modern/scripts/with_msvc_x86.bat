@echo off
REM Wrapper that sources vcvars32.bat for x86 (Win32) build of modern project.
call "C:\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cmake %*
exit /b %errorlevel%