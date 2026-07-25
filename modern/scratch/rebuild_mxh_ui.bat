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

set OBJ_DIR=%MXH_ROOT%\build\src\ui\Debug
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

set LIBSRC=%MXH_ROOT%\src\ui
set LIBOUT=%OBJ_DIR%\mxh_ui.lib

REM List of all .cpp files in src/ui (kept in sync with modern/src/ui/CMakeLists.txt)
set SOURCES=cWindow.cpp cButton.cpp cEditBox.cpp cDialog.cpp cImage.cpp cListCtrl.cpp cWindowManager.cpp cMsgBox.cpp cDivideBox.cpp cIconDialog.cpp cIconGridDialog.cpp cStatic.cpp cPushupButton.cpp cListDialog.cpp cGuildDialog.cpp cExitDialog.cpp cMultiLineText.cpp cGuagen.cpp cListDialogEx.cpp cMacroDialog.cpp cmakdial.cpp guildjoindialog.cpp charstatedialog.cpp sosdialog.cpp wearedexdialog.cpp minifrienddialog.cpp revivedialog.cpp ctextarea.cpp mpnoticedialog.cpp eventnotifydialog.cpp skillpointnotify.cpp mallnoticedialog.cpp changejobdialog.cpp guildcreatedialog.cpp guildinvitedialog.cpp guildjoindialog.cpp guildlevelupdialog.cpp guildmarkdialog.cpp guildnicknamedialog.cpp guildnotedlg.cpp guildnoticedlg.cpp helpdialog.cpp petstateminidlg.cpp petwearedexdialog.cpp chaseinputdialog.cpp chasedialog.cpp

echo === compile all src/ui/*.cpp ===
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
echo === compile done ===

echo === pack mxh_ui.lib ===
"%LIB_EXE%" /nologo /OUT:"%LIBOUT%" %OBJS%
if errorlevel 1 ( echo lib failed & exit /b 1 )

echo === mxh_ui.lib rebuilt ===
dir /b "%LIBOUT%"
exit /b 0
