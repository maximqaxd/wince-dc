<#
  bootstrap.ps1 — verify a fresh clone is ready to build on this PC.

  The repo vendors everything except the Dreamcast WinCE SDK ("wcedreamcast"),
  which is copyrighted and fetched separately. This script checks the vendored
  toolchain + leak source, then checks for the SDK (optionally downloading it).

  usage:
    powershell -File bootstrap.ps1                 # verify only
    powershell -File bootstrap.ps1 -SdkUrl <url>   # download+extract the SDK to C:\wcedreamcast
    powershell -File bootstrap.ps1 -SdkPath D:\foo # use an SDK already at a custom path
#>
param(
  [string]$SdkUrl,
  [string]$SdkPath = $env:WCEDREAMCASTROOT
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
if (-not $SdkPath) { $SdkPath = "C:\wcedreamcast" }
$ok = $true
function Check($label, $path) {
  if (Test-Path $path) { Write-Host ("  OK   {0}" -f $label) -ForegroundColor Green }
  else { Write-Host ("  MISS {0}  ->  {1}" -f $label,$path) -ForegroundColor Red; $script:ok = $false }
}

Write-Host "== vendored toolchain (in repo) ==" -ForegroundColor Cyan
Check "SH compiler (cl.exe)"   "$repo\vendor\sh-toolchain\bin\I386\SH\cl.exe"
Check "host link.exe"          "$repo\vendor\sh-toolchain\bin\I386\link.exe"
Check "CE3 base SDK headers"   "$repo\vendor\sh-toolchain\ce3-ppc2k\include\windows.h"
Check "CE3 OAK headers"        "$repo\vendor\sh-toolchain\ce3-oak\INC\pkfuncs.h"
Check "leaked NK kernel src"   "$repo\vendor\wince-src\PRIVATE\WINCEOS\COREOS\NK\KERNEL\SHX\SHFLOAT.C"
Check "SH-4 constant patch"    "$repo\bsp\inc\mem_shx_patch.h"

Write-Host "`n== Dreamcast SDK (external) ==" -ForegroundColor Cyan
if ($SdkUrl -and -not (Test-Path "$SdkPath\tools\makeimg.exe")) {
  Write-Host "  downloading SDK from $SdkUrl ..." -ForegroundColor Yellow
  $zip = "$env:TEMP\wcedreamcast_sdk.zip"
  Invoke-WebRequest -Uri $SdkUrl -OutFile $zip
  New-Item -ItemType Directory -Force $SdkPath | Out-Null
  Expand-Archive -Path $zip -DestinationPath $SdkPath -Force
}
Check "makeimg.exe"  "$SdkPath\tools\makeimg.exe"
Check "DUMPNK.exe"   "$SdkPath\tools\DUMPNK.exe"
Check "retail ce.bib" "$SdkPath\release\retail\ce.bib"
Check "stock kernel"  "$SdkPath\release\retail\nknodbg.exe"

Write-Host ""
if ($ok) {
  Write-Host "READY. Set the SDK location if not default:  set WCEDREAMCASTROOT=$SdkPath" -ForegroundColor Green
  Write-Host "Then:  toolchain\build-image.bat retail   /   toolchain\build-kernel.bat retail"
} else {
  Write-Host "NOT READY - resolve MISS items above." -ForegroundColor Red
  Write-Host "The Dreamcast SDK is the only external dependency; place it at $SdkPath"
  Write-Host "(or pass -SdkUrl URL to download, or -SdkPath DIR, or set WCEDREAMCASTROOT)."
  exit 1
}
