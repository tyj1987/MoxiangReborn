@echo off
chcp 65001 >nul
cd /d "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking"

echo ========================================
echo Starting Moxian Server Chain
echo ========================================

echo [1/3] Starting DistributeServer...
start /b "" DistributeServer.exe
timeout /t 5 /nobreak >nul

echo [2/3] Starting AgentServer...
start /b "" AgentServer.exe
timeout /t 5 /nobreak >nul

echo [3/3] Starting MapServer (map 17)...
start /b "" MapServer.exe 17
timeout /t 10 /nobreak >nul

echo.
echo Checking processes...
tasklist /FI "IMAGENAME eq DistributeServer.exe" 2>nul
tasklist /FI "IMAGENAME eq AgentServer.exe" 2>nul
tasklist /FI "IMAGENAME eq MapServer.exe" 2>nul

echo.
echo Checking ports...
netstat -ano | findstr "16001 14400 17001 14600 18017"

echo.
echo Done!
pause
