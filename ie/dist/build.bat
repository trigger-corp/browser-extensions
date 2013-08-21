@ECHO OFF

:: Usage: build.bat [release|debug] <directory> <uuid> <password>
:: e.g. build.bat release user.apture 64309a88-4d2e-466e-a469-f681d9895758
::      build.bat release test-platform testplatform

PATH=%PATH%;c:\Windows\Microsoft.NET\Framework\v4.0.30319

cls
cd ..\..

rmdir /s "%3" /q

:: Build release
IF "%1" == "release" msbuild "ie\msvc\ForgeIE.sln" /t:BHO /p:Configuration=Release /p:Platform=Win32
::pause
IF "%1" == "release" msbuild "ie\msvc\ForgeIE.sln" /t:BHO /p:Configuration=Release /p:Platform=x64
::pause

:: Generate
forge-generate build --platforms ie chrome -c "%2\config.json" -u "%2" 
pause 
::GOTO END

:: Build variables
SET uuid="%3"

:: Build debug
:: IF "%1" == "debug" msbuild "ie\msvc\ForgeIE.sln" /t:BHO   /p:Configuration=Debug /p:Platform=x64
IF "%1" == "debug" msbuild "ie\msvc\ForgeIE.sln" /t:BHO   /p:Configuration=Debug /p:Platform=Win32

:: Sign release
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\Win32\Release\bho32.dll"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\Win32\Release\forge32.dll"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\Win32\Release\forge32.exe"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\Win32\Release\frame32.dll"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\x64\Release\bho64.dll"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\x64\Release\forge64.dll"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\x64\Release\forge64.exe"
IF "%1" == "release" signtool sign ^
    /f "ie\dist\certs.dev\windows.pfx" /p "%4"  ^
    /v /t http://timestamp.comodoca.com/authenticode ^
    "%3\development\ie\build\x64\Release\frame64.dll"
::pause

:: Build release installers
IF "%1" == "release" "C:\Program Files (x86)\NSIS\makensis.exe" "%3\development\ie\dist\setup-x86.nsi"
IF "%1" == "release" "C:\Program Files (x86)\NSIS\makensis.exe" "%3\development\ie\dist\setup-x64.nsi"

:: TODO sign release installers

:END

cd ie\dist 

