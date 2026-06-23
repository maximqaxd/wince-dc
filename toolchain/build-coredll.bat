@echo off
rem ============================================================================
rem  build-coredll.bat [retail|debug]
rem  Build the CE 3.0 coredll CORE (coremain.lib) from the leaked WINCE300
rem  source. coremain = the PSL client + C runtime surface that every user
rem  process links: apis.c (DoPslFuncCall/cscode.c syscall table), coredll.c,
rem  time/random/profiler/strings + shx chandler/intrlock. USER-MODE flags
rem  (no -DKERNEL / -DWINCEOEM). The full coredll.dll additionally links the
rem  sibling CORE libs (lmem/thunks/locale/corelibc/...) -- see CORE\DIRS.
rem  This proves the userland core compiles against our header chain.
rem ============================================================================
setlocal enabledelayedexpansion
call "%~dp0setenv.bat" %1

set WOS=%WINCESRC%\PRIVATE\WINCEOS
set CORE=%WOS%\COREOS\CORE
set DLL=%CORE%\DLL
set NK=%WOS%\COREOS\NK
set OUTDIR=%~dp0..\reference\coredll-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set CE3SDK=%GWESLAB%\ce3-ppc2k\include
rem coredll include chain: shx (shx.h) ; CORE\INC ; NK\INC (kernel.h/osver.h) ;
rem ce3-oak (nkintr.h) ; ce3-ppc2k (windows.h/bldver.h).
set INCLUDE=%DLL%\SHX;%CORE%\INC;%NK%\INC;%GWESLAB%\ce3-oak\INC;%CE3SDK%
rem WINCEOEM=1 is set tree-wide by the master CE build (cesysgen): OS components
rem like coredll include kernel.h / pkfuncs.h (CALLBACKINFO etc.). Same unlock as
rem the kernel core (docs/04). Not -DKERNEL / -DIN_KERNEL -- this is user-mode.
set CDEFS=-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DWINCEOEM=1 -DWINCEMACRO -DCOREDLL
if /I "%BLDTYPE%"=="debug" set CDEFS=%CDEFS% -DDEBUG

rem apis.c #includes cscode.c, so cscode is not compiled separately.
set CSRC=apis coredll time random profiler strings
set OBJS=
set FAILED=0

echo === compiling coremain C ===
for %%S in (%CSRC%) do (
  cl.exe /nologo /c /W3 %CDEFS% /Fo"%OUTDIR%\%%S.obj" "%DLL%\%%S.c" >"%OUTDIR%\%%S.err" 2>&1
  if errorlevel 1 (echo   FAIL %%S.c & type "%OUTDIR%\%%S.err" & set /a FAILED+=1) else (echo   ok   %%S.c & set OBJS=!OBJS! "%OUTDIR%\%%S.obj")
)

echo === compiling shx chandler ===
cl.exe /nologo /c /W3 %CDEFS% /Fo"%OUTDIR%\chandler.obj" "%DLL%\SHX\CHANDLER.C" >"%OUTDIR%\chandler.err" 2>&1
if errorlevel 1 (echo   FAIL chandler.c & type "%OUTDIR%\chandler.err" & set /a FAILED+=1) else (echo   ok   chandler.c & set OBJS=!OBJS! "%OUTDIR%\chandler.obj")

echo === assembling shx intrlock ===
set INCLUDE=%NK%\INC;%DLL%\SHX;%GWESLAB%\ce3-oak\INC;%CE3SDK%
shasm.exe -cpu=SH4 -DSH_CPU=64 -DSH4=1 -DSHx=1 -DCELOG=0 -nologo -object=%OUTDIR%\intrlock.obj "%DLL%\SHX\INTRLOCK.SRC" >"%OUTDIR%\intrlock.err" 2>&1
if errorlevel 1 (echo   FAIL intrlock.src & type "%OUTDIR%\intrlock.err" & set /a FAILED+=1) else (echo   ok   intrlock.src & set OBJS=!OBJS! "%OUTDIR%\intrlock.obj")

if %FAILED% GTR 0 (echo. & echo [build-coredll] %FAILED% source files failed. & endlocal & exit /b 1)

echo === archiving coremain.lib ===
lib.exe /nologo /machine:SH4 /out:"%OUTDIR%\coremain.lib" %OBJS%
echo [build-coredll] errorlevel=%errorlevel%   -^>  %OUTDIR%\coremain.lib
endlocal
