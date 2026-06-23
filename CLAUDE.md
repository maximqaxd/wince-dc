# CLAUDE.md — wince-dc (Windows CE 3.0 on the Sega Dreamcast)

Research project: build a **fuller Windows CE** for the Dreamcast (SH-4) than the stripped
CE 2.12 game runtime — from the leaked CE 3.0 source + the Sega "Dragon" SDK's own image
tools + a real SH-4 PE compiler. This repo is **self-contained except the DC SDK** (see Setup).

> The user is a low-level Dreamcast/OS-porting expert — give expert-level, register-specific
> answers, no beginner framing. Sibling projects on their machine (ReactOS `sh4pe-toolchain`,
> `DreamShell`, `KallistiOS`, `img4dc`) are SEPARATE — not part of this repo.

## Current state (resume here)
- ✅ **SH-4 PE toolchain** — vendored gweslab `cl.exe` (Renesas SH, defaults to SH-4 / machine
  `0x1A6`). `vendor/sh-toolchain/bin/I386/SH/cl.exe`. Verified emitting SH-4 objects.
- ✅ **Image pipeline** — `makeimg` reproduces the shipped CE ROM (29 modules); `wrap-image.ps1`
  rebuilds the bootable `0winceos.bin` with a **byte-identical** header. No Platform Builder,
  no CD key, no extraction. (`docs/03-build-pipeline.md`)
- ✅ **Kernel core — BUILDS FROM SOURCE.** All of CE 3.0 `NK` kernel core (19 machine-indep
  C + 4 SHX C + 3 SHX `shasm`) compiles/assembles clean and archives into a verified SH-4
  `nkmain.lib` (26 objs, machine `0x1A6`). Unlock was the missing **build switch** `-DWINCEOEM=1
  -DWINCEMACRO` (gates `kfuncs.h`→PRIVATE `pkfuncs.h`/`mkfuncs.h`), not more headers — cleared
  all 35 errors at once and validated the 4 reconstructed constants (pkfuncs.h defines them
  identically). Driver: `build-nklib.bat`. Full detail in `docs/04-kernel-build.md`.
- ✅ **From-source `nk.exe` LINKS + the kernel BOOTS on Flycast.** Reconstructed the Dreamcast
  HAL/OAL from the shipped SDK kernel (reversed in Ghidra) + a minimal SH-4 CRT (`NK\CRT\SHX\`).
  The HAL now matches the PDB structure: `vendor\wince-src\PLATFORM\DREAMCAST\KERNEL\HAL\` =
  `fwinit.{src,c}` (StartUp/OEMNMI/OEMIdle), `cfwkatan.c` (OEMInit/OEMInterrupt*/GInterruptList/
  SerialInit/platform), `fwkatana.c`, `ktimer.c`/`timer.c`, `rtc.c`, `oemwdm.c`, `oemioctl.c`,
  `isr.c` (KatanaISR2/4/6 + per-source), `mdppfs.c`, `debug.c` (SCIF console), `compress.c`,
  `kdstub.c`. `build-nklib`+`build-oal`+`build-crt`+`build-nk` → `nk.exe`. See `…\HAL\OAL-NOTES.md`.
- ✅ **Bootable disc + it RUNS.** `nk.exe` → makeimg → `wrap-image.ps1` → `0winceos.bin` →
  **GDI** via `make-gdi.ps1` (Half-Life DC pipeline: `utils\buildgdi.exe` + HL-DC `ip.bin`) — the
  working path; CDI via `make-disc.ps1` (mkisofs+cdi4dc) also works. On Flycast the kernel boots
  through KernelRelocate → CE banner → MMU+cache → OEMInit ("Set 4 detected") → "Booting Windows
  CE 3.00" → first thread. Two key fixes: Flycast's "SH-4 Kernel" MMU magic (`wrap-image.ps1` +
  StartUp plant it at 0x10A8) and recovering `pTOC` from the ROM signature (romimage mis-patches
  it). See `docs/05-disc-image.md` + SESSION-LOG.
- 🔄 **Userland bring-up.** Now faults in `SC_GetOwnerProcess`/`GetKHeap` (TLB miss on slot-1
  process memory) loading the first process — the 3.0-kernel / stock-2.12-module ABI wall. Next:
  build the 3.0 user modules (`coredll`/`FSDMGR`/`DEVICE`/`GWES`, all in `vendor/wince-src`) so the
  userland matches the kernel. Build with `[debug]` for verbose kernel output (`-DDEBUG`).

## Setup on a fresh PC
1. `git clone <this repo>` — includes the leak source + SH toolchain under `vendor/`.
2. **Get the DC SDK** (the one external dep): place the Sega Dreamcast WinCE SDK at
   `C:\wcedreamcast` (or set `WCEDREAMCASTROOT` to its path). Run
   `powershell -File toolchain\bootstrap.ps1` to verify everything is present.
3. All scripts derive paths from the repo + `WCEDREAMCASTROOT`; nothing else to configure.

## Build / commands (run from `toolchain\`)
```bat
build-image.bat retail                 :: makeimg  -> C:\wcedreamcast\release\retail\NK.bin
powershell -File wrap-image.ps1 -NkBin C:\wcedreamcast\release\retail\NK.bin ^
                                -Out   C:\wcedreamcast\release\retail\0winceos.bin
build-nklib.bat retail                 :: build whole kernel core -> reference\kernel-obj\nkmain.lib
build-oal.bat   retail                 :: build reconstructed DC OAL -> reference\kernel-obj\oal_dc.lib
build-crt.bat   retail                 :: build minimal SH-4 C runtime -> reference\kernel-obj\crt.lib
build-nk.bat    retail                 :: link nkmain.lib+oal_dc.lib+crt.lib -> nk.exe
build-image.bat retail                 :: makeimg (swap our nk.exe in for nknodbg.exe first) -> NK.bin
powershell -File make-disc.ps1 -Image reference\0winceos.ours.bin -Cdi  :: -> wince.cdi (Flycast)
build-kernel.bat retail [file.c]       :: compile one NK/SHX C source (smoke / per-file)
build-asm.bat    retail [file.src]     :: assemble one SHX shasm source (-cpu=SH4 -DCELOG=0)
```
`setenv.bat [retail|debug]` sets the whole environment; the others call it.

## Layout
- `toolchain/` — `setenv.bat`, `build-image.bat`, `build-kernel.bat`, `build-asm.bat`,
  `build-nklib.bat`, `wrap-image.ps1`, `unwrap-image.ps1`, `bootstrap.ps1`, `README.md`.
- `docs/` — `01-findings` · `02-toolchain-setup` · `03-build-pipeline` · `04-kernel-build`. **Read these.**
- `bsp/` — Dreamcast BSP scaffold (`drivers/`, `inc/`, `files/`, …). The
  `inc/mem_shx_patch.h` reconstruction was removed once `-DWINCEOEM` made pkfuncs.h
  authoritative for the 4 SH-4 constants.
- **HAL/OAL reconstruction** (CE PLATFORM tree, PDB-matched file/func names):
  `vendor/wince-src/PLATFORM/DREAMCAST/KERNEL/HAL/` — `fwinit.{src,c}`, `cfwkatan.c`, `fwkatana.c`,
  `ktimer.c`, `timer.c`, `rtc.c`, `oemwdm.c`, `oemioctl.c`, `isr.c`, `mdppfs.c`, `debug.c`,
  `compress.c`, `kdstub.c`, `dc_hw.h`, `SOURCES`, `OAL-NOTES.md`. Built by `build-oal.bat` →
  `oal_dc.lib`. Reverse-engineered from the shipped SDK kernel; NOT from the leak.
- **SH-4 CRT** (`NK\CRT\SHX\`, fulllibc-PDB-matched names): `memmove.c`/`memset.c`/`memcmp.c`/
  `strcmp.c`/`strlen.c`, `__divlu.c`/`__modlu.c`/`__modls.c`/`i64div.c`/`i64mod.c`,
  `lshi64.c`/`rshui64.c`, `crtfp.c` (soft-float stubs — no fulllibc original). `build-crt.bat` → `crt.lib`.
- `vendor/wince-src/` — leaked CE 3.0 source (WINCE300). `vendor/sh-toolchain/` — SH compiler + CE3 headers.
- `reference/MANIFEST.md` — build artifacts + SHA-256 (binaries gitignored).
- `handoff/` — `SESSION-LOG.md` (full history of how we got here) + `memory/` (the assistant's
  project memory). **Read `SESSION-LOG.md` first when resuming.**

## Next action
The from-source kernel BOOTS on Flycast (banner → MMU → OEMInit → first thread) and now faults in
`SC_GetOwnerProcess`/`GetKHeap` loading the first process — the 3.0-kernel / stock-2.12-module ABI
wall. Build/test loop: `build-nklib`+`build-oal`+`build-crt`+`build-nk` `debug` → swap our `nk.exe`
in for `nknodbg.exe`, `build-image retail` → `wrap-image.ps1` → `make-gdi.ps1` → load
`reference\disc-gdi\disc.gdi` in Flycast (serial console on). Next phase: build the 3.0 user-mode
modules (`coredll`/`FSDMGR`/`DEVICE`/`GWES` — all in `vendor/wince-src`) so userland matches the
kernel, instead of the stock 2.12 modules. Ghidra project `wce` has the SDK kernel as the spec.

## Conventions
- Batch `rem` lines must be plain ASCII — no `>` or em-dash (cmd treats `>` as redirection).
- Kernel compiles use a **pure CE 3.0 include chain** — never mix the 2.12 DC SDK `inc`.
- Don't commit copyrighted binaries beyond what's already vendored; `reference/*.bin` stays ignored.
