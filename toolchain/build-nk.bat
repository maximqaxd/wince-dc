@echo off
rem ============================================================================
rem  build-nk.bat [retail|debug]
rem  TRIAL LINK: nkmain.lib (from-source kernel core) + oal_dc.lib (reconstructed
rem  DC OAL) + corelibc.lib (SH-4 C runtime/intrinsics) -> nk.exe.
rem  Link params mirror NK\KERNEL\SHX\SOURCES: EXEENTRY=StartUp, EXEBASE=0x8C040000,
rem  subsystem:native, align:1024. Expect unresolved externals on the first runs --
rem  that list is the remaining OAL/driver glue to write. See OAL-NOTES.md.
rem ============================================================================
setlocal
call "%~dp0setenv.bat" %1

set OBJDIR=%~dp0..\reference\kernel-obj
set OUT=%OBJDIR%\nk.exe
set LOG=%OBJDIR%\nk-link.log

if not exist "%OBJDIR%\nkmain.lib" (echo [build-nk] missing nkmain.lib -- run build-nklib.bat & endlocal & exit /b 1)
if not exist "%OBJDIR%\oal_dc.lib" (echo [build-nk] missing oal_dc.lib -- run build-oal.bat & endlocal & exit /b 1)
if not exist "%OBJDIR%\crt.lib"    (echo [build-nk] missing crt.lib -- run build-crt.bat & endlocal & exit /b 1)

rem NOTE: link at a low base with relocations kept (/fixed:no). The real EXEBASE
rem is 0x8C040000, but this linker sign-extends high bases and trips LNK1249; the
rem image carries fixups so makeimg/romimage rebases it to the RAMIMAGE address.
rem /DEBUG /DEBUGTYPE:BOTH,FIXUP + /STACK mirror NK\KERNEL\SHX\SOURCES LDEFINES.
rem The FIXUP debug section is what romimage reads to relocate NK to the RAMIMAGE
rem base; without it romimage fails "No debug section for NK - unable to fixup".
echo [build-nk] linking %OUT%
rem /MAP: romimage's GetMapSymbols reads nk.map to find+patch kernel symbols
rem (pTOC, the ROM structures). The real CE build generates it; without it
rem romimage can mis-resolve symbols.
link.exe /nologo /machine:SH4 /subsystem:windowsce,3.00 /entry:StartUp /base:0x00010000 ^
    /fixed:no /align:1024 /DEBUG /DEBUGTYPE:BOTH,FIXUP /STACK:64000,64000 ^
    /MAP:"%OBJDIR%\nk.map" /nodefaultlib /out:"%OUT%" ^
    "%OBJDIR%\nkmain.lib" "%OBJDIR%\oal_dc.lib" "%OBJDIR%\crt.lib" > "%LOG%" 2>&1

echo [build-nk] errorlevel=%errorlevel%   (log: %LOG%)
echo [build-nk] --- unresolved externals ---
findstr /C:"unresolved" /C:"error LNK" "%LOG%"
endlocal
