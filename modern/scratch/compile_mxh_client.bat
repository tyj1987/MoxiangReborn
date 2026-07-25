@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo vcvars failed & exit /b 1 )

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807
set WIN_KIT=C:\Program Files (x86)\Windows Kits\10
set MXH_ROOT=C:\moxiang\modern

set CL_EXE=%MSVC_ROOT%\bin\Hostx64\x64\cl.exe
set LINK_EXE=%MSVC_ROOT%\bin\Hostx64\x64\link.exe

set OBJ_DIR=%MXH_ROOT%\build\tools\MoxianClient\Debug
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

echo === cl ===
"%CL_EXE%" /c /nologo /EHsc /std:c++20 /W3 /utf-8 /MDd /Od /Z7 ^
   /I"%MXH_ROOT%\include" ^
   /I"%MXH_ROOT%\src\client" ^
   /I"%MSVC_ROOT%\include" ^
   /I"%MSVC_ROOT%\atlmfc\include" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\ucrt" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\um" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\shared" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\winrt" ^
   /Fo"%OBJ_DIR%\main.obj" ^
   "%MXH_ROOT%\tools\MoxianClient\main.cpp"
if errorlevel 1 ( echo cl failed & exit /b 1 )

echo === link ===
"%LINK_EXE%" /nologo /SUBSYSTEM:WINDOWS /MACHINE:X64 /OUT:"%OBJ_DIR%\mxh_client.exe" ^
   /LIBPATH:"%MXH_ROOT%\build\src\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\ui\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\render\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\client\Debug" ^
   /LIBPATH:"%MSVC_ROOT%\lib\x64" ^
   /LIBPATH:"%MSVC_ROOT%\atlmfc\lib\x64" ^
   /LIBPATH:"%WIN_KIT%\Lib\10.0.22621.0\ucrt\x64" ^
   /LIBPATH:"%WIN_KIT%\Lib\10.0.22621.0\um\x64" ^
   "%OBJ_DIR%\main.obj" ^
   mxh_client_lib.lib mxh_ui.lib mxh_render.lib mxh_log.lib mxh_compat.lib d3d11.lib dxgi.lib d3dcompiler.lib user32.lib gdi32.lib
if errorlevel 1 ( echo link failed & exit /b 1 )

echo.
echo === built: %OBJ_DIR%\mxh_client.exe ===
exit /b 0
