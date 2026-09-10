@echo off
setlocal
call C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x64
if errorlevel 1 exit /b %errorlevel%
cmake -S modern/unity-core -B modern/build-unity-x64 -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
if errorlevel 1 exit /b %errorlevel%
cmake --build modern/build-unity-x64 --config RelWithDebInfo
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir modern/build-unity-x64 -C RelWithDebInfo --output-on-failure
exit /b %errorlevel%
