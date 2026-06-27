@echo off
rem ----------------------------------------------------------------------------
rem  build-dcspi.bat  -  build the Dreamcast SPI transport (SH-4) as dcspi.dll.
rem  Exports SpiInit/SpiSetCS/SpiRwData/... for both the SCI (hardware sync) and
rem  SCIF (bit-bang) buses. Reusable by the W5500 ethernet backend now + SD/CF
rem  block drivers later. Produces reference\driver-obj\dcspi.dll (+ dcspi.lib).
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
echo [dcspi] compiling dcspi.c (SH-4) ...
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\dcspi.obj" "%~dp0dcspi.c"
if errorlevel 1 (echo [dcspi] COMPILE FAILED & exit /b 1)

echo [dcspi] linking dcspi.dll ...
"%HOSTBIN%\link.exe" /nologo /dll /machine:SH4 /subsystem:windowsce,2.12 /entry:DllMain ^
  /def:"%~dp0dcspi.def" /out:"%OUT%\dcspi.dll" ^
  "%OUT%\dcspi.obj" "%DCSDK%\lib\%DCBT%\coredll.lib" "%DCSDK%\lib\%DCBT%\corelibc.lib" > "%OUT%\dcspi.link.log" 2>&1
type "%OUT%\dcspi.link.log"
echo [dcspi] errorlevel=%errorlevel%  (out: %OUT%\dcspi.dll)
endlocal
