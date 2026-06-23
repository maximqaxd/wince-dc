# Session log / handoff — wince-dc

A narrative of how this repo reached its current state, so work can resume cold on another
machine. (Exported in lieu of the raw chat transcript; the assistant's project memory is in
`handoff/memory/`.) Newest at the bottom.

## Goal
Run a fuller Windows CE on the Sega Dreamcast (SH-4 / SH7091, 16 MB RAM @ phys 0x0C000000 /
cached 0x8C000000) than the stripped CE 2.12 game runtime — toward a shell / multitasking.

## What we established (in order)
1. **Assets.** `C:\wcedreamcast` = the Sega "Dragon" WinCE **2.12** SDK (closed kernel
   `0winceos.bin`, but ALSO a complete `makeimg` build env: loose `nk.exe` + ~90 OS modules +
   `makeimg`/`romimage`/`fmerge`/`bingen` + full `.bib`/`.reg`). `Arquivotheca/WinCE-src_20201004`
   = the 2020 MS leak; branch **WINCE300** has the SuperH kernel (`NK/KERNEL/SHX/`, real
   `#if defined(SH4)` MMU/INTC/TMU) but is **WINCEOS-only** (no PUBLIC tree, no build tools, no shell).
2. **Path decision.** Path B = build CE 3.0 from the leak + the SDK's own makeimg + a real SH
   compiler. NT 4.0 on SH-4 rejected (no SH HAL/codegen ever; 16 MB below NT floor).
3. **Toolchain.** Platform Builder 3.0 needs a CD key — declined to pirate. Instead: the
   `gweslab/WindowsCE-Build-Tools` repo ships a working MS **SH compiler** (`cl.exe`, Renesas SH,
   defaults to SH-4 / `0x1A6`, verified with `dumpbin`). The authentic `wce212\SHCL.EXE` is also
   installed on the origin PC but defaults to SH-3; we standardized on gweslab `cl.exe`.
4. **Image pipeline — VALIDATED.** `makeimg` round-trips the shipped image (29 modules, start
   `8C010000`, ROM span `0x1CC43C`). `DUMPNK` parses our `NK.bin` (it rejects the wrapped shipped
   image). The Sega wrapper is decoded: `0winceos.bin = [0x800 header] + [DUMPNK raw image padded
   to 0x1CC800]`, header = magic `D61A`, base `0x0C010000` @0x14, payload off `0x800`, len
   `0x1CC800`. `wrap-image.ps1` reproduces that header **byte-for-byte** (verified vs shipped).
5. **Kernel compile — STARTED.** Two fixes landed: (a) use a **pure CE 3.0 include chain**
   (`vendor/sh-toolchain/ce3-ppc2k/include` base SDK headers), NOT the 2.12 DC SDK `inc` (it
   redefines `DWORD`/`BOOL`). (b) The leaked SHx headers are an **incomplete snapshot** — `MEM_SHX.H`
   lacks `VA_SECTION`; `SECTION_SHIFT`/`CURTLSPTR_OFFSET`/`KINFO_OFFSET` are absent entirely. We
   reconstructed them from the SH ABI (`KSSHX.H` asm offsets + `KERNEL.H` `ERRFALSE` contracts)
   in `bsp/inc/mem_shx_patch.h` (force-included). Errors dropped 50 → 35.

## Where it stands
- Toolchain + image pipeline: DONE and reproducible.
- Kernel compile frontier: `KWIN32.H(81,135,…)` — `CALLBACKINFO` and function-pointer typedefs
  undefined (next OAK-vs-leak header-skew layer). Same reconciliation method repeats.

## How to resume (do this)
1. Read `CLAUDE.md`, then `docs/04-kernel-build.md` (the method + exact frontier).
2. `git clone`, place the DC SDK at `C:\wcedreamcast`, run `toolchain\bootstrap.ps1`.
3. `toolchain\build-kernel.bat retail` → take the first error → locate the symbol
   (`grep` across `vendor/wince-src` + `vendor/sh-toolchain/ce3-oak/INC` + `ce3-ppc2k/include`)
   → add the header to the include chain, or reconstruct from the ABI into a new
   `bsp/inc/*_patch.h`. Repeat until `SHFLOAT.C` compiles, then the full `NK\KERNEL` SOURCES.
4. Build `nknodbg.exe`, drop it over `C:\wcedreamcast\release\retail\nknodbg.exe`,
   `build-image.bat retail`, `wrap-image.ps1`, test on Flycast/lxdream then HW.

## Key facts to keep handy
- Image base: RAMIMAGE @ `8C010000` (cached) = phys `0C010000`. Wrapper len pads to `0x800`.
- `ce.bib` pulls the kernel as `nk.exe` FROM `nknodbg.exe`. 28 other modules stay stock 2.12.
- No `explorer.exe` anywhere in the SDK; `gwes.exe` (windowing) IS present → a custom GUI
  shell/launcher is the path to a "desktop" (deferred; current focus is the kernel).
- Batch gotcha: `rem` lines with `>` or em-dash break cmd parsing. Keep rem ASCII.
