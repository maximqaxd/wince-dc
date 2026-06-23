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
- 🔄 **Kernel compile** — IN PROGRESS. Compiling the leaked CE 3.0 `NK` (SHX) source.
  Cleared header-set mismatch + reconstructed 4 missing SH-4 constants
  (`bsp/inc/mem_shx_patch.h`); errors 50→35. **Frontier:** `KWIN32.H` type-skew
  (`CALLBACKINFO` + fn-ptr typedefs undefined). Full method + state in `docs/04-kernel-build.md`.

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
build-kernel.bat retail                :: compile leaked NK/SHX (the in-progress grind)
```
`setenv.bat [retail|debug]` sets the whole environment; the others call it.

## Layout
- `toolchain/` — `setenv.bat`, `build-image.bat`, `build-kernel.bat`, `wrap-image.ps1`,
  `unwrap-image.ps1`, `bootstrap.ps1`, `README.md`.
- `docs/` — `01-findings` · `02-toolchain-setup` · `03-build-pipeline` · `04-kernel-build`. **Read these.**
- `bsp/` — Dreamcast BSP scaffold + `inc/mem_shx_patch.h` (reconstructed SH-4 constants).
- `vendor/wince-src/` — leaked CE 3.0 source (WINCE300). `vendor/sh-toolchain/` — SH compiler + CE3 headers.
- `reference/MANIFEST.md` — build artifacts + SHA-256 (binaries gitignored).
- `handoff/` — `SESSION-LOG.md` (full history of how we got here) + `memory/` (the assistant's
  project memory). **Read `SESSION-LOG.md` first when resuming.**

## Next action
Continue `docs/04-kernel-build.md` §"Method": run `build-kernel.bat retail`, take the first
error (`CALLBACKINFO` in `KWIN32.H`), locate or reconstruct the type, repeat until `SHFLOAT.C`
compiles — then expand to the full `NK\KERNEL` SOURCES, link `nknodbg.exe`, drop it into the
makeimg pipeline, wrap, and test on Flycast/lxdream then real HW (dc-load/BBA).

## Conventions
- Batch `rem` lines must be plain ASCII — no `>` or em-dash (cmd treats `>` as redirection).
- Kernel compiles use a **pure CE 3.0 include chain** — never mix the 2.12 DC SDK `inc`.
- Don't commit copyrighted binaries beyond what's already vendored; `reference/*.bin` stays ignored.
