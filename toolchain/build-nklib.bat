@echo off
rem ============================================================================
rem  build-nklib.bat [retail|debug]
rem  Build the CE 3.0 SH-4 kernel core LIBRARY (nkmain.lib) entirely from the
rem  leaked WINCE300 source with the vendored SH toolchain.
rem
rem  Mirrors NK\KERNEL\NKNORMAL\SOURCES (machine-independent C) + NK\KERNEL\SHX
rem  \SOURCES (SH-4 machine-dependent C + asm). The final nk.exe LINK additionally
rem  needs the Dreamcast OAL/boot layer, which is NOT in the WINCEOS-only leak
rem  (it lives in the closed 2.12 0winceos.bin); that is the remaining gap. This
rem  proves the kernel core itself compiles + archives from source.
rem ============================================================================
setlocal enabledelayedexpansion
call "%~dp0setenv.bat" %1

set NK=%WINCESRC%\PRIVATE\WINCEOS\COREOS\NK
set KERNEL=%NK%\KERNEL
set SHX=%KERNEL%\SHX
set OUTDIR=%~dp0..\reference\kernel-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem ---- pure CE 3.0 include chain (see docs/04-kernel-build.md) ----------------
set CE3SDK=%GWESLAB%\ce3-ppc2k\include
set BSPINC=%~dp0..\bsp\inc
set INCLUDE=%BSPINC%;%NK%\INC;%SHX%;%NK%\..\CORE\INC;%GWESLAB%\ce3-oak\INC;%CE3SDK%

rem ---- kernel defines: WINCEOEM/WINCEMACRO gate the PRIVATE pkfuncs/mkfuncs,
rem which authoritatively define VA_SECTION/SECTION_SHIFT/CURTLSPTR_OFFSET/
rem KINFO_OFFSET (no reconstruction patch needed -- see docs/04-kernel-build.md).
set KDEFS=-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DKERNEL -DWINCEOEM=1 -DWINCEMACRO -DIN_KERNEL -DDBGSUPPORT
set FORCEINC=

rem ---- machine-independent C (NKNORMAL\SOURCES) ------------------------------
set CSRC=debug resource objdisp heap ppfs compr2 printf loader mapfile virtmem physmem schedule kwin32 kmisc intrapi stubs exdsptch memtrk profiler
rem ---- SHX machine-dependent C (SHX\SOURCES) --------------------------------
set SHXCSRC=mdsh3 shunwind strings shfloat
rem ---- SHX machine-dependent asm (SHX\SOURCES, via shasm) -------------------
set SHXASM=intrlock shexcept celogshx

set OBJS=
set FAILED=0

echo === compiling machine-independent kernel C ===
for %%S in (%CSRC%) do (
  cl.exe /nologo /c /W3 %KDEFS% %FORCEINC% /Fo"%OUTDIR%\%%S.obj" "%KERNEL%\%%S.c" >nul 2>"%OUTDIR%\%%S.err"
  if errorlevel 1 (echo   FAIL %%S.c & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.c & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

echo === compiling SHX machine-dependent C ===
for %%S in (%SHXCSRC%) do (
  cl.exe /nologo /c /W3 %KDEFS% %FORCEINC% /Fo"%OUTDIR%\%%S.obj" "%SHX%\%%S.c" >nul 2>"%OUTDIR%\%%S.err"
  if errorlevel 1 (echo   FAIL %%S.c & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.c & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

echo === assembling SHX machine-dependent asm ===
set ASMINC=%NK%\INC;%SHX%;%GWESLAB%\ce3-oak\INC;%CE3SDK%
for %%S in (%SHXASM%) do (
  set INCLUDE=%ASMINC%
  shasm.exe -cpu=SH4 -DSH_CPU=64 -DSH4=1 -DSHx=1 -DCELOG=0 -nologo -object=%OUTDIR%\%%S.obj "%SHX%\%%S.src" >"%OUTDIR%\%%S.err" 2>&1
  if errorlevel 1 (echo   FAIL %%S.src & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.src & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

if %FAILED% GTR 0 (echo. & echo [build-nklib] %FAILED% source files failed -- not archiving. & endlocal & exit /b 1)

echo === archiving nkmain.lib ===
lib.exe /nologo /machine:SH4 /out:"%OUTDIR%\nkmain.lib" %OBJS%
echo [build-nklib] errorlevel=%errorlevel%   -^>  %OUTDIR%\nkmain.lib
endlocal
