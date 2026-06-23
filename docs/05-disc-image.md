# Disc image — booting the from-source kernel in Flycast

Turns our wrapped `0winceos.bin` into a bootable Dreamcast disc. **GDI is the recommended
path** (`make-gdi.ps1`) because it reuses the *proven* Half-Life DC pipeline — and Half-Life
DC is itself a Windows CE port, so its bootstrap + layout are exactly what a WinCE image needs.
A CDI path (`make-disc.ps1`, mkisofs+cdi4dc) is also kept.

## GDI (recommended) — the Half-Life DC pipeline
HL-DC ships `utils\buildgdi.exe` (Sappharad's GDIBuilder), `utils\ip.bin` (its IP.BIN — bootfile
field `0WINCEOS.BIN`, same as `ip_drago.bin`), and `utils\Half-Life.GDI` (track layout:
`1 0 / 2 756 / 3 45000`). `buildgdi` makes the high-density **track03** = IP.BIN bootsector +
ISO9660 with our `0WINCEOS.BIN`; we synthesize the low-density `track01.bin`/`track02.raw` filler.
```
powershell -File toolchain\make-gdi.ps1 -Image reference\0winceos.ours.bin
```
→ `reference\disc-gdi\disc.gdi` (+ track01/02/03). Load `disc.gdi` in Flycast.
buildgdi call (from HL-DC's `postbuild_dc.bat`):
```
buildgdi.exe -data <dir-with-0WINCEOS.BIN> -ip ip.bin -output <out> -gdi Half-Life.GDI -V WINCE
```
(No `-truncate` — that emits a spurious end-of-disc track4; the standard shape is one full
~1.18 GB high-density track3, mostly zero-pad.)

> `utils\ip.bin` is Sega's copyrighted bootstrap — gitignored, not committed. `buildgdi.exe`
> + `Half-Life.GDI` (free) are committed. The HL-DC source lives at `c:\dev\halflife_dc`.

## CDI (alternative)
Turns our wrapped `0winceos.bin` into a bootable Dreamcast **CDI** (Padus DiscJuggler).
Driver: `toolchain\make-disc.ps1`.

## Boot model (from the SDK's own scripts)
- `ip_drago.bin` (GD Workshop) is the WinCE **IP.BIN** — its bootfile field @0x60 is
  literally `0WINCEOS.BIN`. The DC BIOS runs IP.BIN, which loads `0WINCEOS.BIN` from the
  ISO9660 root. (rename `ip_drago.bin` → `IP.BIN`.)
- `makecd.bat` shows the disc tree: `0winceos.bin` at the **GD root** (must be the first
  file alphabetically) + OS modules under `\WinCE\`. For a kernel smoke-boot, only the boot
  file is required.
- `gore.bat` = `dumpnk nk.bin nk.raw` → `gore2 nk.raw 0winceos.bin`; our `wrap-image.ps1`
  reimplements `gore2` (byte-identical header).

## Tools
- **mkisofs** (cdrtools 3.00, cygwin build) — builds the ISO and injects IP.BIN via `-G`.
  Not in the DC SDK; pulled to `c:\dev\cdrtools\` (mkisofs.exe + cygwin1/cygiconv-2/cygintl-8
  DLLs) from the `esc0rtd3w/dreamcast-tools` selfboot mirror.
- **cdi4dc** (img4dc 0.3b, by SiZiOUS) at `c:\dev\cdi4dc\cdi4dc.exe` — ISO → CDI.

## Recipe (the "Marcus tutorial" / img4dc flow)
```
mkisofs -C 0,11702 -V WINCE -G IP.BIN -joliet -rock -l -o wince.iso <srcdir>
cdi4dc  wince.iso  wince.cdi
```
- `-G IP.BIN` embeds the 0x8000-byte bootsector (the renamed `ip_drago.bin`).
- `-C 0,11702` sets the high-density session offset cdi4dc expects (11702 = DC pregap).
  (mkisofs warns "-C without -M" — harmless for selfboot.)
- `<srcdir>` contains `0WINCEOS.BIN` (our wrapped image) at its root.

## One-shot
```
powershell -File toolchain\make-disc.ps1 -Image reference\0winceos.ours.bin -Cdi
```
Stages `reference\disc\cd\0WINCEOS.BIN` + `IP.BIN`, runs mkisofs + cdi4dc →
`reference\disc\wince.cdi`. Add `-WithModules` to also stage `\WinCE\` OS modules.
Load `wince.cdi` in Flycast.

## Status / caveats
- The CDI boots **our from-source CE 3.0 kernel** (`nk.exe` → makeimg → `0winceos.bin`).
- The kernel's OAL/CRT still has bring-up **stubs** (fixed-time RTC, `OEMIoControl`→FALSE,
  ISR/soft-float stubs) and the user-mode modules are stock 2.12 (ABI-mismatched with a 3.0
  kernel), so expect the kernel to start (watch the SCIF console) but not bring up a full
  shell yet. Watch Flycast's serial output for our OEMInit/`OEMWriteDebugString` traffic.
- For CDI the boot LBA lives in IP.BIN; cdi4dc handles the layout. If you switch to a GDI
  flow, the bootfile LBA must match the high-density track base (45000).

## Flycast full-MMU gate ("SH-4 Kernel" magic)
Most DC games never use the SH-4 MMU, so Flycast only enables full MMU emulation
when it detects Windows CE: `core/hw/sh4/modules/mmu.cpp` `mmu_set_state()` checks
for the UTF-16 string **"SH-4 Kernel"** at virtual **0x8C0110A8** or **0x8C011118**
(= RAMIMAGE 0x8C010000 + 0x10A8 / 0x1118) when `MMUCR.AT` is set. The stock kernel's
mdsh3 banner lands at 0x10A8; our link order puts ours deep in the image, leaving
0x10A8 as zero padding — so Flycast left the MMU off and our kernel faulted the
instant `KernelStart` enabled address translation ("exception while SR.BL=1").
`wrap-image.ps1` now plants `"SH-4 Kernel"` at rawBytes[0x10A8] (only if that slot
is zero/already-magic, never over real code) so Flycast turns the MMU on.
