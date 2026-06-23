@echo off
rem ============================================================================
rem  build-oal.bat [retail|debug]
rem  Build the RECONSTRUCTED Dreamcast OAL (NK\OAL\DREAMCAST) into oal_dc.lib.
rem  Mirrors build-nklib.bat's include chain + kernel defines. These objects
rem  are our reconstruction (from the shipped SDK kernel), not from the leak.
rem  They compile/assemble standalone; the final nk.exe LINK resolves the
rem  cross-refs (KernelStart, HookInterrupt, GInterruptList, per-source ISRs)
rem  against nkmain.lib + the driver set. See NK\OAL\DREAMCAST\OAL-NOTES.md.
rem ============================================================================
setlocal enabledelayedexpansion
call "%~dp0setenv.bat" %1

set NK=%WINCESRC%\PRIVATE\WINCEOS\COREOS\NK
set OAL=%NK%\OAL\DREAMCAST
set OUTDIR=%~dp0..\reference\kernel-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set CE3SDK=%GWESLAB%\ce3-ppc2k\include
rem OAL source dir first so dc_hw.h ("" includes) resolves; then the kernel chain.
set INCLUDE=%OAL%;%NK%\INC;%NK%\KERNEL\SHX;%NK%\..\CORE\INC;%GWESLAB%\ce3-oak\INC;%CE3SDK%
set KDEFS=-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DKERNEL -DWINCEOEM=1 -DWINCEMACRO -DIN_KERNEL -DDBGSUPPORT

set CSRC=oeminit timer intr dbgserial platform rtc power serial oemioctl kstubs
set OBJS=
set FAILED=0

echo === assembling OAL startup ===
set INCLUDE=%NK%\INC;%NK%\KERNEL\SHX;%GWESLAB%\ce3-oak\INC;%CE3SDK%
shasm.exe -cpu=SH4 -DSH_CPU=64 -DSH4=1 -DSHx=1 -DCELOG=0 -nologo -object=%OUTDIR%\startup.obj "%OAL%\startup.src" >"%OUTDIR%\startup.err" 2>&1
if errorlevel 1 (echo   FAIL startup.src & type "%OUTDIR%\startup.err" & set /a FAILED+=1) else (echo   ok   startup.src & set OBJS=!OBJS! "%OUTDIR%\startup.obj")

echo === compiling OAL C ===
set INCLUDE=%OAL%;%NK%\INC;%NK%\KERNEL\SHX;%NK%\..\CORE\INC;%GWESLAB%\ce3-oak\INC;%CE3SDK%
for %%S in (%CSRC%) do (
  cl.exe /nologo /c /W3 %KDEFS% /Fo"%OUTDIR%\%%S.obj" "%OAL%\%%S.c" >nul 2>"%OUTDIR%\%%S.err"
  if errorlevel 1 (echo   FAIL %%S.c & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.c & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

if %FAILED% GTR 0 (echo. & echo [build-oal] %FAILED% source files failed -- not archiving. & endlocal & exit /b 1)

echo === archiving oal_dc.lib ===
lib.exe /nologo /machine:SH4 /out:"%OUTDIR%\oal_dc.lib" %OBJS%
echo [build-oal] errorlevel=%errorlevel%   -^>  %OUTDIR%\oal_dc.lib
endlocal
