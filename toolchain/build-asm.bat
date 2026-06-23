@echo off
rem ----------------------------------------------------------------------------
rem  build-asm.bat [retail|debug] <file.src>
rem  Assemble a CE 3.0 SH-4 kernel .src (Renesas shasm) to an object.
rem  .src files resolve .include via the INCLUDE env var (same CE3 chain as the
rem  C compile). CPU is SH4. SH_CPU=64 mirrors NKNORMAL/SOURCES (SH4 ADEFINES).
rem ----------------------------------------------------------------------------
setlocal
call "%~dp0setenv.bat" %1

set NK=%WINCESRC%\PRIVATE\WINCEOS\COREOS\NK
set OUTDIR=%~dp0..\reference\kernel-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem Pure CE 3.0 asm-include chain: kernel-private INC has KSSHX.H; base SDK has KXSHX.H.
set INCLUDE=%NK%\INC;%NK%\KERNEL\SHX;%GWESLAB%\ce3-oak\INC;%GWESLAB%\ce3-ppc2k\include

set SRC=%~2
if "%SRC%"=="" set SRC=%NK%\KERNEL\SHX\INTRLOCK.SRC

for %%F in ("%SRC%") do set OBJ=%OUTDIR%\%%~nF.obj

echo [build-asm] shasm %SRC%  -^>  %OBJ%
rem -DCELOG=0 mirrors NKNORMAL/SOURCES (ADEFINES for _TGTCPUTYPE==SHx): the asm
rem conditionals gate CELOG hooks on this symbol; undefined => "not defined yet".
shasm.exe -cpu=SH4 -DSH_CPU=64 -DSH4=1 -DSHx=1 -DCELOG=0 -nologo -object=%OBJ% "%SRC%"
echo [build-asm] errorlevel=%errorlevel%
endlocal
