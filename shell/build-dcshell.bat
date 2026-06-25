@echo off
rem ----------------------------------------------------------------------------
rem  build-dcshell.bat  -  compile the DC WinCE shell (SH-4) against the 2.12 SDK.
rem  Uses the vendored gweslab SH compiler (defaults to SH-4) + the DC SDK
rem  inc/lib (apps use the 2.12 SDK API, NOT the CE3 leak headers).
rem ----------------------------------------------------------------------------
setlocal
if "%WCEDREAMCASTROOT%"=="" set WCEDREAMCASTROOT=C:\wcedreamcast
set DCSDK=%WCEDREAMCASTROOT%
for %%I in ("%~dp0..") do set REPO=%%~fI
set SHBIN=%REPO%\vendor\sh-toolchain\bin\I386\SH
set HOSTBIN=%REPO%\vendor\sh-toolchain\bin\I386
set PATH=%SHBIN%;%HOSTBIN%;%PATH%
set INCLUDE=%DCSDK%\inc
set LIB=%DCSDK%\lib\retail
set OUT=%~dp0..\reference\shell-obj
if not exist "%OUT%" mkdir "%OUT%"

echo [dcshell] compiling dcshell.c (SH-4) ...
"%SHBIN%\cl.exe" /nologo /c /W3 -DUNDER_CE=212 -D_WIN32_WCE=212 -DUNICODE -D_UNICODE -DSH4=1 -DSHx=1 /Fo"%OUT%\dcshell.obj" "%~dp0dcshell.c"
if errorlevel 1 (echo [dcshell] COMPILE FAILED & exit /b 1)

echo [dcshell] linking dcshell.exe ...
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:WinMainCRTStartup /out:"%OUT%\dcshell.exe" "%OUT%\dcshell.obj" "%DCSDK%\lib\retail\coredll.lib" "%DCSDK%\lib\retail\corelibc.lib" > "%OUT%\dcshell.link.log" 2>&1
type "%OUT%\dcshell.link.log"
echo [dcshell] errorlevel=%errorlevel%  (out: %OUT%\dcshell.exe)
endlocal
