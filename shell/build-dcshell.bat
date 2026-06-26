@echo off
rem ----------------------------------------------------------------------------
rem  build-dcshell.bat  -  compile the DC WinCE shell (SH-4) against the 2.12 SDK.
rem  Uses the vendored gweslab SH compiler (defaults to SH-4) + the DC SDK
rem  inc/lib (apps use the 2.12 SDK API, NOT the CE3 leak headers).
rem ----------------------------------------------------------------------------
setlocal
if "%WCEDREAMCASTROOT%"=="" set WCEDREAMCASTROOT=C:\wcedreamcast
set DCSDK=%WCEDREAMCASTROOT%
rem  flavor MUST match the image's coredll.dll - CE resolves coredll by ordinal,
rem  so retail libs + debug dll (or vice-versa) mis-resolve GDI imports -> wild jump.
if "%~1"=="" (set DCBT=debug) else (set DCBT=%~1)
for %%I in ("%~dp0..") do set REPO=%%~fI
set SHBIN=%REPO%\vendor\sh-toolchain\bin\I386\SH
set HOSTBIN=%REPO%\vendor\sh-toolchain\bin\I386
set PATH=%SHBIN%;%HOSTBIN%;%PATH%
set INCLUDE=%DCSDK%\inc
set LIB=%DCSDK%\lib\%DCBT%
set OUT=%~dp0..\reference\shell-obj
if not exist "%OUT%" mkdir "%OUT%"

set CF=/nologo /c /W3 -DUNDER_CE=212 -D_WIN32_WCE=212 -DUNICODE -D_UNICODE -DSH4=1 -DSHx=1
echo [dcshell] compiling dcgfx.c + dcshell.c (SH-4) ...
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcgfx.obj" "%~dp0dcgfx.c"
if errorlevel 1 (echo [dcshell] COMPILE FAILED dcgfx & exit /b 1)
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcshell.obj" "%~dp0dcshell.c"
if errorlevel 1 (echo [dcshell] COMPILE FAILED dcshell & exit /b 1)

echo [dcshell] linking dcshell.exe ...
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:WinMainCRTStartup /out:"%OUT%\dcshell.exe" "%OUT%\dcshell.obj" "%OUT%\dcgfx.obj" "%DCSDK%\lib\%DCBT%\coredll.lib" "%DCSDK%\lib\%DCBT%\corelibc.lib" "%DCSDK%\lib\%DCBT%\ddraw.lib" > "%OUT%\dcshell.link.log" 2>&1
type "%OUT%\dcshell.link.log"

echo [dcshell] building DCWin clients (dcwlib + dcwcalc + dcwclock) ...
set CLIBS="%DCSDK%\lib\%DCBT%\coredll.lib" "%DCSDK%\lib\%DCBT%\corelibc.lib"
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcwlib.obj" "%~dp0dcwlib.c"
if errorlevel 1 (echo [dcwlib] COMPILE FAILED & exit /b 1)
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcwcalc.obj" "%~dp0dcwcalc.c"
if errorlevel 1 (echo [dcwcalc] COMPILE FAILED & exit /b 1)
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:WinMainCRTStartup /out:"%OUT%\dcwcalc.exe" "%OUT%\dcwcalc.obj" "%OUT%\dcwlib.obj" %CLIBS% >> "%OUT%\dcshell.link.log" 2>&1
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcwclock.obj" "%~dp0dcwclock.c"
if errorlevel 1 (echo [dcwclock] COMPILE FAILED & exit /b 1)
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:WinMainCRTStartup /out:"%OUT%\dcwclock.exe" "%OUT%\dcwclock.obj" "%OUT%\dcwlib.obj" %CLIBS% >> "%OUT%\dcshell.link.log" 2>&1
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcwexp.obj" "%~dp0dcwexp.c"
if errorlevel 1 (echo [dcwexp] COMPILE FAILED & exit /b 1)
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:WinMainCRTStartup /out:"%OUT%\dcwexp.exe" "%OUT%\dcwexp.obj" "%OUT%\dcwlib.obj" %CLIBS% >> "%OUT%\dcshell.link.log" 2>&1
echo [dcshell] errorlevel=%errorlevel%  (out: %OUT%\dcshell.exe)
endlocal
