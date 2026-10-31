@echo off
setlocal
pwsh -NoProfile -File "%~dp0soak-24h.ps1" %*
exit /b %ERRORLEVEL%
