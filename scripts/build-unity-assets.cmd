@echo off
setlocal
call C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x64
if errorlevel 1 exit /b %errorlevel%
cmake -S "%~dp0..\modern\tools\MoxianUnityAssetExport" -B "%~dp0..\modern\out\unity-remaster\asset-build" -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%
cmake --build "%~dp0..\modern\out\unity-remaster\asset-build" --config Debug
exit /b %errorlevel%
