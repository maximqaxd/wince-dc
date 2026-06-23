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
- ✅ **From-source `nk.exe` LINKS (zero unresolved).** Reconstructed the Dreamcast OAL
  (`NK\OAL\DREAMCAST\`: StartUp, OEMInit, INTC/TMU bring-up, polled SCIF console, RTC/platform/
  ioctl/power, ISR + per-source stubs) from the shipped SDK kernel (reversed in Ghidra), plus a
  minimal SH-4 CRT (`NK\CRT\SHX\`: mem/str/div/shift in C, soft-float stubbed) since the DC SDK
  has no static libc. `build-nklib`+`build-oal`+`build-crt`+`build-nk` → `nk.exe` (SH-4 `0x1A6`,
  entry StartUp, ~264 KB). Detail in `NK\OAL\DREAMCAST\OAL-NOTES.md`.
- 🔄 **Boot it.** The OAL/CRT stubs are link-satisfiers, not working HW yet (fixed-time RTC,
  `OEMIoControl`→FALSE, ISR/soft-float stubs). Next: makeimg (rebase `0x8C040000`) +
  `wrap-image.ps1` → Flycast/lxdream, then make the stubs real per `OAL-NOTES.md` "Boot-readiness TODO".

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
build-nk.bat    retail                 :: TRIAL LINK nkmain.lib+oal_dc.lib -> nk.exe (lists unresolved)
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
- **OAL reconstruction** lives in the kernel tree (CE-native), `vendor/wince-src/PRIVATE/
  WINCEOS/COREOS/NK/OAL/DREAMCAST/` — `startup.src`, `oeminit.c`, `timer.c`, `intr.c`,
  `dc_hw.h`, `SOURCES`, `OAL-NOTES.md`. Built by `build-oal.bat` → `oal_dc.lib`. Reverse-
  engineered from the shipped SDK kernel; NOT from the leak.
- `vendor/wince-src/` — leaked CE 3.0 source (WINCE300). `vendor/sh-toolchain/` — SH compiler + CE3 headers.
- `reference/MANIFEST.md` — build artifacts + SHA-256 (binaries gitignored).
- `handoff/` — `SESSION-LOG.md` (full history of how we got here) + `memory/` (the assistant's
  project memory). **Read `SESSION-LOG.md` first when resuming.**

## Next action
Kernel core (`nkmain.lib`) and a first reconstructed DC OAL (`oal_dc.lib`: `startup.src`,
`oeminit.c`, `timer.c`, `intr.c`) both build SH-4 clean. Next: finish the OAL to a LINKABLE
`nk.exe` — (1) decode the `KatanaISR2/4` exact `SB_IST`→SYSINTR bit map + bind `Timer0ISR`'s
tick to KData (see `OAL-NOTES.md` TODO), (2) add SCIF `OEMWriteDebugByte` for first-boot output,
(3) link `nkmain.lib`+`oal_dc.lib` (EXEENTRY=StartUp, EXEBASE per `SHX\SOURCES`), resolving
remaining externs, (4) wrap via makeimg + `wrap-image.ps1`, test Flycast/lxdream → HW. DC memory
map: RAM phys `0x0C000000`/cached `0x8C000000`, RAMIMAGE `@8C010000`. Ghidra project `wce` has the
SDK kernel annotated as the spec.

## Conventions
- Batch `rem` lines must be plain ASCII — no `>` or em-dash (cmd treats `>` as redirection).
- Kernel compiles use a **pure CE 3.0 include chain** — never mix the 2.12 DC SDK `inc`.
- Don't commit copyrighted binaries beyond what's already vendored; `reference/*.bin` stays ignored.
