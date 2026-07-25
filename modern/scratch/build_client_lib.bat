@echo off
setlocal enabledelayedexpansion
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo vcvars failed & exit /b 1 )

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807
set WIN_KIT=C:\Program Files (x86)\Windows Kits\10
set MXH_ROOT=C:\moxiang\modern
set CL_EXE=%MSVC_ROOT%\bin\Hostx64\x64\cl.exe
set LIB_EXE=%MSVC_ROOT%\bin\Hostx64\x64\lib.exe

set INCLUDE=%MXH_ROOT%\include;%MSVC_ROOT%\include;%MSVC_ROOT%\atlmfc\include;%WIN_KIT%\Include\10.0.22621.0\ucrt;%WIN_KIT%\Include\10.0.22621.0\um;%WIN_KIT%\Include\10.0.22621.0\shared;%WIN_KIT%\Include\10.0.22621.0\winrt

set OBJ_DIR=%MXH_ROOT%\build\src\client\Debug
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

set LIBSRC=%MXH_ROOT%\src\client
set LIBOUT=%OBJ_DIR%\mxh_client_lib.lib

set SOURCES=CMainGame.cpp CMainTitle.cpp GameStateStubs.cpp

echo === compile src/client/*.cpp ===
set OBJS=
for %%F in (%SOURCES%) do (
    "%CL_EXE%" /c /nologo /EHsc /std:c++20 /W3 /utf-8 /MDd /Od /Z7 ^
       /I"%MXH_ROOT%\include" ^
       /I"%MSVC_ROOT%\include" ^
       /I"%MSVC_ROOT%\atlmfc\include" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\ucrt" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\um" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\shared" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\winrt" ^
       /Fo"%OBJ_DIR%\%%~nF.obj" ^
       "%LIBSRC%\%%F"
    if errorlevel 1 ( echo %%F failed & exit /b 1 )
    set OBJS=!OBJS! "%OBJ_DIR%\%%~nF.obj"
)

echo === pack mxh_client_lib.lib ===
"%LIB_EXE%" /nologo /OUT:"%LIBOUT%" %OBJS%
if errorlevel 1 ( echo lib failed & exit /b 1 )

echo === mxh_client_lib.lib built ===
dir /b "%LIBOUT%"
exit /b 0
