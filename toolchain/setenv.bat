@echo off
rem ============================================================================
rem  Central build environment for WinCE-on-Dreamcast (Path B)
rem  usage:  call setenv.bat [retail|debug]      (default: retail)
rem  Sets: DC SDK makeimg env (replica of wce.bat + set_imginit) + SH compiler.
rem ============================================================================
if "%~1"=="" (set BLDTYPE=retail) else (set BLDTYPE=%~1)

rem ---- repo-relative paths (portable: clone the repo anywhere) -----------------
for %%I in ("%~dp0..") do set REPO=%%~fI
set WINCESRC=%REPO%\vendor\wince-src
set GWESLAB=%REPO%\vendor\sh-toolchain
rem ---- DC SDK is NOT in the repo (download separately; see RESUME.md). ---------
rem      Override the location by setting WCEDREAMCASTROOT before calling this.
if "%WCEDREAMCASTROOT%"=="" set WCEDREAMCASTROOT=C:\wcedreamcast
set DCSDK=%WCEDREAMCASTROOT%

rem ---- DC SDK image-build env (faithful replica of wce.bat retail|debug) ------
set WCEDREAMCASTROOT=%DCSDK%
set CEVersion=212
set CESubsystem=windowsce,2.12
set CEConfigName=Dreamcast
set RELEASEDIR_STDCORE=\CoreOS_Standard\
set RELEASEDIR_HTMLCORE=\CoreOS_HTML\
set RELEASEDIR_OS=\OS\
set RELEASEDIR_SAMPLES=\Samples\
set RELEASEDIR_APPS=\Applications\
set IMGNODEBUGGER=1
set IMGDIRECTDRAW=1
set IMGDIRECT3DIM=1
set IMGDDHAL=1
set IMGDIRECTSOUND=1
set IMGDIRECTINPUT=1
set IMGMICROSTK=1
set IMGNOCOMM=1
set IMGNOUSB=
set IMGNOPCMCIA=1
set IMGNOCEDDK=1
set INITNOCOMM=1
set INITMICROSTK=1
set INITSERIAL=1
set INITMODEM=1
set INITDIRECTSHOW=1
set COUNTRY=USA
set _HOSTCPUTYPE=i386
set _PUBLICROOT=%WCEDREAMCASTROOT%
set _DEPTREES=winceos gemini mcputech dshowdm6
set _TGTPROJ=DRAGON
set _FLATRELEASEDIR=%WCEDREAMCASTROOT%\release\%BLDTYPE%
if /I "%BLDTYPE%"=="debug" (set DCLIB=%DCSDK%\lib\debug) else (set DCLIB=%DCSDK%\lib\retail)

rem ---- SH compiler: vendored gweslab cl.exe (13.10 Renesas SH, defaults SH4/0x1A6) --
set SH_BIN=%GWESLAB%\bin\I386\SH
set HOST_BIN=%GWESLAB%\bin\I386
set PATH=%WCEDREAMCASTROOT%\tools;%WCEDREAMCASTROOT%\tools\GDWorkshop;%SH_BIN%;%HOST_BIN%;%PATH%

rem ---- headers/libs for compiling kernel + drivers ---------------------------
set INCLUDE=%GWESLAB%\ce3-oak\INC;%DCSDK%\inc
set LIB=%DCLIB%

echo [setenv] BLDTYPE=%BLDTYPE%   _FLATRELEASEDIR=%_FLATRELEASEDIR%
echo [setenv] SH cc = %SH_BIN%\cl.exe  (SH4)   image tools on PATH (makeimg/romimage/DUMPNK)
