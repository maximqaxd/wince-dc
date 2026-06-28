<#
  make-gdi-real.ps1 - build a GDEMU/real-hardware-bootable disc by CLONING the real
  4x4 Evo GDI (its genuine IP.BIN + proper contiguous high-density layout) and replacing
  only 0WINCEOS.BIN with OUR kernel image. buildgdi -rebuild copies every track/file from
  the source GDI and overlays the files in -data.

  Why not make-gdi.ps1? That one uses -truncate to emit a sparse, gapped multi-track GDI
  (track03 = 25 sectors of ISO metadata, then a huge gap to track04). Lenient loaders tolerate
  that; GDEMU does not (it expects a real disc), so it drops to the BIOS with no banner.
  This produces the exact layout/IP.BIN the shipped game used -> GDEMU-valid.

  NOTE: output is large (~the size of the real disc, ~hundreds of MB). For fast iteration
  keep using make-gdi.ps1; use this only when burning/copying to GDEMU.

  Usage: powershell -File make-gdi-real.ps1 [-Image <0winceos.bin>]
#>
param(
  [string]$Image   = "C:\wcedreamcast\release\retail\0winceos.bin",
  [string]$RealGdi = "C:\dev\gdi2data\4X4 EVO v1.001 (2000)(GOD)(NTSC)(US)[!].gdi",
  [string]$OutDir  = "$PSScriptRoot\..\reference\disc-real",
  [string]$Utils   = "$PSScriptRoot\..\utils"
)
$ErrorActionPreference = "Stop"

# NOTE -LiteralPath: the real GDI filename contains [!] which PowerShell globbing would eat.
if (-not (Test-Path -LiteralPath $Image))                { throw "image not found: $Image" }
if (-not (Test-Path -LiteralPath $RealGdi))              { throw "real GDI not found: $RealGdi" }
if (-not (Test-Path -LiteralPath "$Utils\buildgdi.exe")) { throw "buildgdi.exe not found in $Utils" }

# Stage ONLY our kernel as 0WINCEOS.BIN; -rebuild overlays it onto the cloned game disc.
$stage = Join-Path $OutDir "_stage"
Remove-Item $OutDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item $Image (Join-Path $stage "0WINCEOS.BIN") -Force

Write-Host "Rebuilding real 4x4 Evo disc with OUR 0WINCEOS.BIN (this is large, please wait)..."
& "$Utils\buildgdi.exe" -rebuild -gdi $RealGdi -data $stage -output $OutDir
Write-Host "Real-layout GDI written under: $OutDir"
Get-ChildItem $OutDir -File | Select-Object Name,Length | Format-Table -Auto
