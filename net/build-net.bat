@echo off
rem ----------------------------------------------------------------------------
rem  build-net.bat - compile lwIP 2.2.0 (vendored) + our port for SH-4 against the
rem  Katana SDK. Stage 1: just compile the core/ipv4/netif object set and report
rem  per-file pass/fail (feasibility + regression check). Flavor [retail|debug].
rem ----------------------------------------------------------------------------
setlocal
if "%WCEDREAMCASTROOT%"=="" set WCEDREAMCASTROOT=C:\wcedreamcast
set DCSDK=%WCEDREAMCASTROOT%
if "%~1"=="" (set DCBT=debug) else (set DCBT=%~1)
for %%I in ("%~dp0..") do set REPO=%%~fI
set SHBIN=%REPO%\vendor\sh-toolchain\bin\I386\SH
set HOSTBIN=%REPO%\vendor\sh-toolchain\bin\I386
set PATH=%SHBIN%;%HOSTBIN%;%PATH%
set LWIP=%REPO%\vendor\lwip\src
set PORT=%~dp0lwip-port
set INCLUDE=%PORT%;%LWIP%\include;%DCSDK%\inc;%REPO%\vendor\sh-toolchain\ce3-ppc2k\include
set OUT=%REPO%\reference\net-obj
if not exist "%OUT%" mkdir "%OUT%"
set CF=/nologo /c /W3 -DUNDER_CE=212 -D_WIN32_WCE=212 -DUNICODE -D_UNICODE -DSH4=1 -DSHx=1

set CORE=init def mem memp netif pbuf ip raw udp tcp tcp_in tcp_out timeouts inet_chksum sys dns
set IPV4=ip4 ip4_addr ip4_frag etharp icmp dhcp
set NETIF=ethernet

for %%F in (%CORE%)  do call :cc "%LWIP%\core\%%F.c" %%F
for %%F in (%IPV4%)  do call :cc "%LWIP%\core\ipv4\%%F.c" %%F
for %%F in (%NETIF%) do call :cc "%LWIP%\netif\%%F.c" %%F
echo [net] compile pass complete.
endlocal & exit /b 0

:cc
"%SHBIN%\cl.exe" %CF% /Fo"%OUT%\%2.obj" "%~1" >nul 2>"%OUT%\%2.err"
if errorlevel 1 (echo   FAIL %2 & type "%OUT%\%2.err") else (echo   ok   %2)
exit /b 0
