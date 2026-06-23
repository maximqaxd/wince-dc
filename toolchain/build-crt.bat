@echo off
rem ============================================================================
rem  build-crt.bat [retail|debug]
rem  Build the minimal SH-4 C runtime (NK\CRT\SHX) into crt.lib. OURS - the DC
rem  SDK ships no static libc; the real kernel used fulllibc.lib (absent). mem/str
rem  + integer-divide helpers (C shift-subtract) + soft-float STUBS. See
rem  NK\OAL\DREAMCAST\OAL-NOTES.md.
rem ============================================================================
setlocal enabledelayedexpansion
call "%~dp0setenv.bat" %1

set NK=%WINCESRC%\PRIVATE\WINCEOS\COREOS\NK
set CRT=%NK%\CRT\SHX
set OUTDIR=%~dp0..\reference\kernel-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set CE3SDK=%GWESLAB%\ce3-ppc2k\include
set INCLUDE=%NK%\INC;%NK%\KERNEL\SHX;%GWESLAB%\ce3-oak\INC;%CE3SDK%
set KDEFS=-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DKERNEL -DWINCEOEM=1 -DWINCEMACRO -DIN_KERNEL
if /I "%BLDTYPE%"=="debug" set KDEFS=%KDEFS% -DDEBUG

set CSRC=memmove memset memcmp strcmp strlen __divlu __modlu __modls i64div i64mod lshi64 rshui64 crtfp
set OBJS=
set FAILED=0

for %%S in (%CSRC%) do (
  cl.exe /nologo /c /W3 %KDEFS% /Fo"%OUTDIR%\%%S.obj" "%CRT%\%%S.c" >nul 2>"%OUTDIR%\%%S.err"
  if errorlevel 1 (echo   FAIL %%S.c & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.c & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

if %FAILED% GTR 0 (echo. & echo [build-crt] %FAILED% source files failed. & endlocal & exit /b 1)

echo === archiving crt.lib ===
lib.exe /nologo /machine:SH4 /out:"%OUTDIR%\crt.lib" %OBJS%
echo [build-crt] errorlevel=%errorlevel%   -^>  %OUTDIR%\crt.lib
endlocal
