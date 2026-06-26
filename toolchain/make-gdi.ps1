<#
  make-gdi.ps1 - build a Flycast-loadable Dreamcast GDI of our WinCE image,
  using the *proven* Half-Life DC pipeline (HL-DC is itself a WinCE port):
    - utils\ip.bin       : HL-DC's IP.BIN (bootfile field = 0WINCEOS.BIN)
    - utils\buildgdi.exe : Sappharad's GDIBuilder (makes the high-density track 3)
    - utils\Half-Life.GDI: the 3-track layout (1@0 data, 2@756 audio, 3@45000 data)
  track03 (IP.BIN + ISO9660 with 0WINCEOS.BIN) is generated; the low-density
  track01/track02 are standard filler we synthesize to match the GDI's LBAs.

  Usage: powershell -File make-gdi.ps1 [-Image <0winceos.bin>] [-OutDir <dir>]
#>
param(
  [string]$Image     = "C:\dev\Dreamcast\wince-dc\reference\0winceos.ours.bin",
  [string]$OutDir    = "C:\dev\Dreamcast\wince-dc\reference\disc",
  [string]$Utils     = "C:\dev\Dreamcast\wince-dc\utils",
  [string]$ExtraData = ""    # optional folder whose contents go into \CD-ROM
)
$ErrorActionPreference = "Stop"
$sec = 2352

if (-not (Test-Path $Image))               { throw "image not found: $Image" }
if (-not (Test-Path "$Utils\buildgdi.exe")){ throw "buildgdi.exe not found in $Utils" }
if (-not (Test-Path "$Utils\ip.bin"))      { throw "ip.bin not found in $Utils" }

$data = Join-Path $OutDir "data"
Remove-Item $OutDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $data | Out-Null
Copy-Item $Image (Join-Path $data "0WINCEOS.BIN") -Force
if ($ExtraData -ne "" -and (Test-Path $ExtraData)) {
  Write-Host "Staging extra CD-ROM files from $ExtraData ..."
  robocopy $ExtraData $data /E /NFL /NDL /NJH /NJS /NP | Out-Null
}
$gdi = Join-Path $OutDir "disc.gdi"
Copy-Item "$Utils\Half-Life.GDI" $gdi -Force

# track03+ = IP.BIN bootsector + ISO9660 (with 0WINCEOS.BIN), high density @45000.
# -truncate: do NOT pad the data track to the full GD-ROM (~1.18 GB). buildgdi
# instead emits the ISO metadata (track03 @45000) + the file data at its real high
# LBA (a separate track04), skipping the empty gap between -> a ~5 MB disc (no
# extra data) instead of 1.1 GB of mostly-zero padding. Flycast loads the
# resulting multi-track GDI fine; the file data still ends at the disc's end LBA.
& "$Utils\buildgdi.exe" -data $data -ip "$Utils\ip.bin" -output $OutDir -gdi $gdi -V WINCE -truncate | Out-Null
if (-not (Test-Path (Join-Path $OutDir "track03.bin"))) { throw "buildgdi did not produce track03.bin" }

# Parse the GDI for the track1/2 LBAs, synthesize standard low-density filler.
$lines = Get-Content $gdi
$t1lba = [int]($lines[1].Split(' ')[1])      # track1 start LBA (0)
$t2lba = [int]($lines[2].Split(' ')[1])      # track2 start LBA (756)
$t1sectors = $t2lba - $t1lba                 # track1 length so it ends at track2
[IO.File]::WriteAllBytes((Join-Path $OutDir "track01.bin"), (New-Object byte[] ($t1sectors * $sec)))
[IO.File]::WriteAllBytes((Join-Path $OutDir "track02.raw"), (New-Object byte[] (300 * $sec)))   # ~4s silence

Write-Host "GDI ready: $gdi"
Get-ChildItem $OutDir -File | Select-Object Name,Length | Format-Table -Auto
Write-Host "Load $gdi in Flycast."
