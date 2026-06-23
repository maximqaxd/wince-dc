@echo off
rem ----------------------------------------------------------------------------
rem  build-image.bat [retail|debug]
rem  Runs the DC SDK makeimg over release\<type>\*.bib -> NK.bin (B000FF CE ROM).
rem  Then run wrap-image.ps1 to produce the bootable 0winceos.bin.
rem ----------------------------------------------------------------------------
setlocal
call "%~dp0setenv.bat" %1
cd /d %WCEDREAMCASTROOT%
echo [build-image] running makeimg in %_FLATRELEASEDIR% ...
makeimg
set RC=%errorlevel%
echo [build-image] makeimg errorlevel=%RC%
if %RC%==0 (
  echo [build-image] OK : NK.bin written to %_FLATRELEASEDIR%
  echo [build-image] next: run wrap-image.ps1 (NkBin NK.bin, Out 0winceos.bin)
)
endlocal & exit /b %RC%
