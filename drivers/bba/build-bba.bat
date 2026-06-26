@echo off
rem ----------------------------------------------------------------------------
rem  build-bba.bat  -  build the Dreamcast Broadband Adapter CE stream driver
rem  (SH-4) against the Katana SDK. Produces reference\driver-obj\bba.dll.
rem  Flavor [retail|debug] must match the image coredll (ordinal resolution).
rem ----------------------------------------------------------------------------
setlocal
if "%WCEDREAMCASTROOT%"=="" set WCEDREAMCASTROOT=C:\wcedreamcast
set DCSDK=%WCEDREAMCASTROOT%
if "%~1"=="" (set DCBT=debug) else (set DCBT=%~1)
for %%I in ("%~dp0..\..") do set REPO=%%~fI
set SHBIN=%REPO%\vendor\sh-toolchain\bin\I386\SH
set HOSTBIN=%REPO%\vendor\sh-toolchain\bin\I386
set PATH=%SHBIN%;%HOSTBIN%;%PATH%
set INCLUDE=%DCSDK%\inc
set LIB=%DCSDK%\lib\%DCBT%
set OUT=%REPO%\reference\driver-obj
if not exist "%OUT%" mkdir "%OUT%"

set CF=/nologo /c /W3 -DUNDER_CE=212 -D_WIN32_WCE=212 -DUNICODE -D_UNICODE -DSH4=1 -DSHx=1
echo [bba] compiling bba.c (SH-4) ...
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\bba.obj" "%~dp0bba.c"
if errorlevel 1 (echo [bba] COMPILE FAILED & exit /b 1)

echo [bba] linking bba.dll ...
"%HOSTBIN%\link.exe" /nologo /machine:SH4 /subsystem:windowsce,2.12 /dll ^
  /entry:DllMain /def:"%~dp0bba.def" /out:"%OUT%\bba.dll" ^
  "%OUT%\bba.obj" "%DCSDK%\lib\%DCBT%\wdm.lib" "%DCSDK%\lib\%DCBT%\coredll.lib" "%DCSDK%\lib\%DCBT%\corelibc.lib" > "%OUT%\bba.link.log" 2>&1
type "%OUT%\bba.link.log"
echo [bba] errorlevel=%errorlevel%  (out: %OUT%\bba.dll)
endlocal
