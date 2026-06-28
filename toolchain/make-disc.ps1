<#
  make-disc.ps1 - stage a bootable Dreamcast WinCE disc tree and (if the tools
  are installed) master a bootable CDI/GDI.

  Boot model (from the SDK's own makecd.bat + ip_drago.bin):
    IP.BIN = ip_drago.bin  (its bootfile field @0x60 is "0WINCEOS.BIN")
    GD root: 0WINCEOS.BIN   (our wrapped image)  + \WinCE\ OS modules
  The DC BIOS runs IP.BIN, which loads 0WINCEOS.BIN from the ISO9660 root.

  Usage:
    powershell -File make-disc.ps1 -Image <0winceos.bin> [-OutDir <dir>] [-Cdi] [-WithModules]
#>
param(
  [string]$Image   = "C:\Dev\wince-dc\reference\0winceos.ours.bin",
  [string]$OutDir  = "C:\Dev\wince-dc\reference\disc",
  [string]$IpBin   = "C:\wcedreamcast\tools\GDWorkshop\ip_drago.bin",
  [switch]$Cdi,
  [switch]$WithModules
)
$ErrorActionPreference = "Stop"

if (-not (Test-Path $Image)) { throw "image not found: $Image (build it: build-image.bat + wrap-image.ps1)" }
if (-not (Test-Path $IpBin)) { throw "IP.BIN not found: $IpBin" }

$tree = Join-Path $OutDir "cd"
Remove-Item $OutDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $tree | Out-Null

# 0WINCEOS.BIN must be the first file (alphabetical) in the GD root per ipmaker docs.
Copy-Item $Image (Join-Path $tree "0WINCEOS.BIN") -Force
Copy-Item $IpBin (Join-Path $OutDir "IP.BIN") -Force      # ip_drago.bin -> IP.BIN

if ($WithModules) {
  $os = "C:\wcedreamcast\release\retail\OS"
  if (Test-Path $os) {
    New-Item -ItemType Directory -Force -Path (Join-Path $tree "WinCE") | Out-Null
    Get-ChildItem $os -Include *.dll,*.exe,*.ttf,*.drv,*.mpb -Recurse -EA SilentlyContinue |
      Copy-Item -Destination (Join-Path $tree "WinCE") -Force
  } else { Write-Warning "OS module dir not found ($os) - staging boot file only." }
}

Write-Host "Staged disc tree at: $tree"
Write-Host "  IP.BIN  : $(Join-Path $OutDir 'IP.BIN')"
Get-ChildItem $tree -Recurse | Select-Object FullName,Length | Format-Table -Auto

# --- master, if the homebrew tools are present ------------------------------
# mkisofs (cdrtools, cygwin build) + cdi4dc (img4dc). Pulled to c:\dev\* .
function Find-Tool($name, $fixed) { if (Test-Path $fixed) { return $fixed }; return (Get-Command $name -EA SilentlyContinue).Source }
$mkisofs = Find-Tool "mkisofs" "C:\dev\cdrtools\mkisofs.exe"
$cdi4dc  = Find-Tool "cdi4dc"  "C:\dev\cdi4dc\cdi4dc.exe"
$iso     = Join-Path $OutDir "wince.iso"
$ipbinStaged = Join-Path $OutDir "IP.BIN"

# mkisofs/cdi4dc write progress + harmless cygwin notes to stderr; don't let
# those trip $ErrorActionPreference=Stop. We gate on the output files instead.
$ErrorActionPreference = "Continue"
if ($mkisofs) {
  # Marcus/img4dc recipe: -G embeds the 0x8000-byte IP.BIN bootsector; -C gives
  # the high-density session offset cdi4dc expects (11702 = standard DC pregap).
  & $mkisofs -C 0,11702 -V WINCE -G $ipbinStaged -joliet -rock -l -o $iso $tree
  if (-not (Test-Path $iso)) { throw "mkisofs failed - no ISO produced." }
  Write-Host "ISO: $iso ($((Get-Item $iso).Length) bytes)"
  if ($Cdi) {
    # NB: must not be named $cdi - case-insensitively collides with [switch]$Cdi
    # and would coerce to a boolean, making cdi4dc write a file named "True".
    $cdiOut = Join-Path $OutDir "wince.cdi"
    Remove-Item $cdiOut -EA SilentlyContinue
    if ($cdi4dc) {
      & $cdi4dc $iso $cdiOut | Out-Null       # cdi4dc floods progress; discard it
      if (Test-Path $cdiOut) { Write-Host "CDI: $cdiOut ($((Get-Item $cdiOut).Length) bytes) -- bootable." }
      else { Write-Warning "cdi4dc produced no CDI." }
    } else { Write-Warning "cdi4dc not found (img4dc). ISO built; convert it with cdi4dc." }
  }
} else {
  Write-Host ""
  Write-Host "mkisofs/cdi4dc not on PATH. Tree is staged; finish with (see docs/05-disc-image.md):" -ForegroundColor Yellow
  Write-Host "  mkisofs -V WINCE -G `"$ipbinStaged`" -joliet -rock -l -C 0,11702 -o `"$iso`" `"$tree`""
  Write-Host "  cdi4dc `"$iso`" `"$(Join-Path $OutDir 'wince.cdi')`"     # -> bootable wince.cdi"
}
