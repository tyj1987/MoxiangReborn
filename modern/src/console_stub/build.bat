@echo off
call "C:\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
cl.exe /LD /EHsc /MT /O2 /Fe"C:\Windows\System32\ConsoleStub.dll" "C:\Temp\ConsoleStub\stub.cpp" /link ole32.lib oleaut32.lib user32.lib kernel32.lib advapi32.lib
echo EXITCODE=%ERRORLEVEL%
