@echo off
rem ----------------------------------------------------------------------------
rem  build-kernel.bat [retail|debug] [sourcefile.c]
rem  EXPERIMENTAL: compile the leaked CE 3.0 SH-4 kernel (NK) with the SH cc.
rem  Default = "kernel compile smoke": compile NK\KERNEL\SHX\SHFLOAT.C to an obj.
rem  Full NK link is the next milestone (wire NK private headers iteratively).
rem ----------------------------------------------------------------------------
setlocal
call "%~dp0setenv.bat" %1

set NK=%WINCESRC%\PRIVATE\WINCEOS\COREOS\NK
set OUTDIR=%~dp0..\reference\kernel-obj
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem PURE CE 3.0 include chain. Do NOT inherit the 2.12 DC SDK inc: it redefines
rem DWORD/BOOL and lacks CE3 constants. Order: kernel-private, CORE, OAK, base SDK.
set CE3SDK=%GWESLAB%\ce3-ppc2k\include
set BSPINC=%~dp0..\bsp\inc
set INCLUDE=%BSPINC%;%NK%\INC;%NK%\KERNEL\SHX;%WINCESRC%\PRIVATE\WINCEOS\COREOS\CORE\INC;%GWESLAB%\ce3-oak\INC;%CE3SDK%

rem CE SH-4 kernel defines (retail). Adjust as the compile dictates.
rem WINCEOEM/WINCEMACRO are required: the CE3 SOURCES set WINCEOEM=1 + -DWINCEMACRO,
rem which gate kfuncs.h's pull-in of the PRIVATE pkfuncs.h/mkfuncs.h (CALLBACKINFO,
rem TRACKER_CALLBACK, the xxx_ macros, etc.). Without them only the public kfuncs
rem surface is visible and the kernel-private prototypes fail to parse.
set KDEFS=-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DKERNEL -DWINCEOEM=1 -DWINCEMACRO -DIN_KERNEL -DDBGSUPPORT

rem No force-include needed: -DWINCEOEM pulls in pkfuncs.h which defines the SH-4
rem kernel constants (VA_SECTION/SECTION_SHIFT/CURTLSPTR_OFFSET/KINFO_OFFSET).
set FORCEINC=

set SRC=%~2
if "%SRC%"=="" set SRC=%NK%\KERNEL\SHX\SHFLOAT.C

echo [build-kernel] cc %SRC%
cl.exe /nologo /c /W3 %KDEFS% %FORCEINC% /Fo"%OUTDIR%\\" "%SRC%"
echo [build-kernel] errorlevel=%errorlevel%
echo [build-kernel] NOTE: first runs WILL hit missing NK headers; that is the
echo [build-kernel] iteration loop (add the right PRIVATE include dirs above).
endlocal
