@echo off
setlocal enabledelayedexpansion
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo vcvars failed & exit /b 1 )

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807
set WIN_KIT=C:\Program Files (x86)\Windows Kits\10
set MXH_ROOT=C:\moxiang\modern

set CL_EXE=%MSVC_ROOT%\bin\Hostx64\x64\cl.exe
set LINK_EXE=%MSVC_ROOT%\bin\Hostx64\x64\link.exe

set INCLUDE=%MXH_ROOT%\include;%MSVC_ROOT%\include;%MSVC_ROOT%\atlmfc\include;%MXH_ROOT%\src\client;%WIN_KIT%\Include\10.0.22621.0\ucrt;%WIN_KIT%\Include\10.0.22621.0\um;%WIN_KIT%\Include\10.0.22621.0\shared;%WIN_KIT%\Include\10.0.22621.0\winrt;%MXH_ROOT%\third_party\googletest-src\googletest\include

set OBJ_DIR=%MXH_ROOT%\build\tests\unit\client\Debug
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

set LIBOUT=%OBJ_DIR%\mxh_client_tests.exe

echo === compile client tests ===
set OBJS=
for %%F in (cmain_game_test.cpp game_state_stubs_test.cpp cmain_title_test.cpp) do (
    "%CL_EXE%" /c /nologo /EHsc /std:c++17 /W3 /utf-8 /MDd /Od /Z7 ^
       /I"%MXH_ROOT%\include" ^
       /I"%MXH_ROOT%\src\client" ^
       /I"%MSVC_ROOT%\include" ^
       /I"%MSVC_ROOT%\atlmfc\include" ^
       /I"%MXH_ROOT%\third_party\googletest-src\googletest\include" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\ucrt" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\um" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\shared" ^
       /I"%WIN_KIT%\Include\10.0.22621.0\winrt" ^
       /Fo"%OBJ_DIR%\%%~nF.obj" ^
       "%MXH_ROOT%\tests\unit\client\%%F"
    if errorlevel 1 ( echo %%F failed & exit /b 1 )
    set OBJS=!OBJS! "%OBJ_DIR%\%%~nF.obj"
)

REM Stub __std_search_1/2 for googletest's vectorized search symbols
"%CL_EXE%" /c /nologo /EHsc /std:c++17 /W3 /utf-8 /MDd /Od /Z7 ^
   /I"%MSVC_ROOT%\include" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\ucrt" ^
   /I"%WIN_KIT%\Include\10.0.22621.0\um" ^
   /Fo"%OBJ_DIR%\stub_search.obj" ^
   "%MXH_ROOT%\scratch\stub_search.cpp"
if errorlevel 1 ( echo stub_search.cpp failed & exit /b 1 )
set OBJS=!OBJS! "%OBJ_DIR%\stub_search.obj"

echo === link ===
"%LINK_EXE%" /nologo /SUBSYSTEM:CONSOLE /MACHINE:X64 /OUT:"%LIBOUT%" ^
   /LIBPATH:"%MXH_ROOT%\build\src\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\ui\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\render\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\src\client\Debug" ^
   /LIBPATH:"%MXH_ROOT%\build\lib\Debug" ^
   /LIBPATH:"%MSVC_ROOT%\lib\x64" ^
   /LIBPATH:"%MSVC_ROOT%\atlmfc\lib\x64" ^
   /LIBPATH:"%MSVC_ROOT%\lib\x64\stl\msvcprt" ^
   /LIBPATH:"%WIN_KIT%\Lib\10.0.22621.0\ucrt\x64" ^
   /LIBPATH:"%WIN_KIT%\Lib\10.0.22621.0\um\x64" ^
   %OBJS% ^
   "%MXH_ROOT%\build\_deps\googletest-build\googletest\gtest.dir\Debug\gtest-all.obj" ^
   "%MXH_ROOT%\build\_deps\googletest-build\googletest\gtest_main.dir\Debug\gtest_main.obj" ^
   msvcprtd.lib mxh_client_lib.lib mxh_ui.lib mxh_render.lib mxh_log.lib mxh_compat.lib
if errorlevel 1 ( echo link failed & exit /b 1 )

echo === run tests ===
"%LIBOUT%" 2>&1 | FindStr /R /C:"FAILED" /C:"Failure" /C:"expected" /C:"[  FAILED" /C:"[ RUN"
exit /b 0
