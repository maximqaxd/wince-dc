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
- 🔄 **Userland is BOOTING (2026-06-24).** The old "SC_GetOwnerProcess/GetKHeap" fault was really
  a `DoImports` page-in fault from a **2.12-vs-3.0 `e32_rom` struct mismatch** — FIXED. Plus an API
  method-table hole (`ProcMthds[4]`) — FIXED. Now **coredll + filesys load, filesys initialises +
  `SignalStarted`s, and `RunApps` launches the 2nd process**, which is stuck on a cross-process
  `PerformCallBack` (current frontier). Strategy DECIDED: **keep the stock 2.12 modules** (GWES has
  no source, so the 3.0 userland can't be rebuilt — see `docs/06`); fix kernel-side 2.12/3.0 deltas
  as they surface (on-disk module structs + API method numbering must match 2.12, verified via the
  Ghidra SDK kernel). **Full detail + repeatable method + active DCDBG probes: `docs/07-userland-boot.md`.**
  Emulator debug/WinDBG feasibility: `docs/08-emulator-debugging.md`. (`coremain.lib` from 3.0
  source also builds via `build-coredll.bat`, but is NOT used — we keep stock coredll.)
- ✅ **Networking — full TCP/IP over the STOCK CE stack (2026-06-27).** Replaced the SDK's
  `mppp.dll` (dial-up PPP) with a universal link shim (`net/netif/`) so stock `microstk.exe` +
  `winsock.dll` run over Ethernet — no lwIP needed. **BBA path verified END-TO-END in Flycast:
  DHCP → DNS → TCP → HTTP** (the `dcwnet` app resolved www.sega.com via the DHCP DNS server and
  fetched over the Broadband Adapter). The mppp/microstk link ABI, the MTU/byte-order/DNS-registry
  gotchas (each cost a debug round), and the SDK-side `ce.bib`/`reginit.ini` edits are documented in
  **`docs/09-networking.md`**. A W5500/MACRAW backend over an SPI transport (`drivers/dcspi/` — SCI
  hardware-SPI + SCIF bit-bang, ported from KOS) is written but **hardware-only/untested**; the
  modem (PPP) backend is not started. (This also confirms userland now boots far enough to run a
  windowed desktop shell + winsock apps — see the `shell/` commits.)

## Setup on a fresh PC
1. `git clone <this repo>` — **fully self-contained.** Both the SH-4 compiler
   (`vendor/sh-toolchain`) and the Sega "Dragon" CE 2.12 SDK (`vendor/wcesdk`: headers, libs,
   image tools, OS modules, patched kernels + their `.map`s) are vendored. No external SDK,
   no `C:\wcedreamcast`.
2. Install **CMake ≥ 3.20 + Ninja** (the VS-bundled pair works — see `toolchain/README.md`).
   Nothing else to configure; the build derives all paths from the repo.

## Build / commands (CMake — see `toolchain/README.md`)
```sh
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=<ninja> -S . -B build
cmake --build build                  # all SH-4 modules -> build/modules/ (dcspi/mppp/dcshell/dcw*)
cmake --build build --target image   # makeimg -> NK.bin -> wrap -> build/0winceos.bin
cmake --build build --target gdi     # full chain -> build/disc/disc.gdi (load in Flycast)
```
The image step seeds the read-only `vendor/wcesdk/image` tree into `build/image`, overlays our
freshly-built modules into `build/image/OS/`, runs `makeimg`, then `wrap-image.ps1` +
`make-gdi.ps1`. CMake mirrors the old `setenv.bat` makeimg env verbatim (`IMG_ENV` in
`CMakeLists.txt`). The from-source kernel build (`build-nklib`/`build-oal`/`build-crt`/`build-nk`)
was retired with the leak source — the image uses the vendored stock kernels (`nknodbg.exe`).

## Layout
- `CMakeLists.txt` — the entire build (modules + image + gdi). `project(NONE)` driving the
  vendored `cl.exe`/`shasm.exe`/`link.exe`/`makeimg`.
- `toolchain/` — `wrap-image.ps1`, `make-gdi.ps1`, `make-gdi-real.ps1`, `make-disc.ps1`,
  `unwrap-image.ps1`, `bootstrap.ps1`, `README.md` (CMake build doc).
- `docs/` — `01-findings` · `02-toolchain-setup` · `03-build-pipeline` · `04-kernel-build` ·
  `05-disc-image` · `06-userland-abi` · `07-userland-boot` · `08-emulator-debugging` ·
  `09-networking`. **Read these.**
- `net/` — networking. `netif/` = the universal microstk link shim (drop-in `mppp.dll` replacement;
  BBA verified + W5500 backend) — see `docs/09-networking.md`. `lwip-port/` + vendored lwIP = the
  alternative bring-your-own-stack route (built, unused). Built by CMake (`mppp` target).
- `drivers/` — `bba/` (RTL8139 WDM driver — RETIRED into the shim's `bba_hw.c`, kept as HW ref);
  `dcspi/` (reusable SPI transport: SCI hardware-SPI + SCIF bit-bang, for W5500 now + SD/CF/FAT later;
  CMake `dcspi` target).
- `shell/` — the DCWin desktop shell + PVR2/Direct3D compositor + client apps (`dcwcalc`/`dcwclock`/
  `dcwexp`/`dcwtask`/`dcwmem` memtest/`dcwnet` winsock test). CMake `dcshell` + `dcw*` targets.
- `vendor/sh-toolchain/` — SH-4 compiler + CE3 headers. `vendor/wcesdk/` — the vendored Sega
  "Dragon" CE 2.12 SDK (`inc`, `lib`, `tools`, `image`: OS modules + config + stock kernels
  `nknodbg.exe`/`nkscifkd.exe` and their `.map`s). The leaked CE 3.0 source (`vendor/wince-src`)
  and the from-source kernel/HAL/OAL/CRT build were retired — we ship the stock SDK kernel.
- `reference/MANIFEST.md` — build artifacts + SHA-256 (binaries gitignored).
- `handoff/` — `SESSION-LOG.md` (full history of how we got here) + `memory/` (the assistant's
  project memory). **Read `SESSION-LOG.md` first when resuming.**

## Next action  (NETWORKING track, updated 2026-06-27 — read `docs/09-networking.md`)
Userland now boots to a windowed desktop shell running winsock apps, and the **BBA networking path
is complete + verified in Flycast** (DHCP→DNS→TCP→HTTP on the stock stack via our `mppp.dll` shim).
Next on this track:
1. **Debug W5500 networking.** The Flycast W5500 emulation is BUILT + pushed (flycast master
   `d51495b79`, `core/hw/w5500/`, env `FLYCAST_W5500=1`; `run_w5500.bat`); the disc bakes
   `HKLM\Comm\Netif\W5500Bus=1`. Test: the W5500 is **detected over SPI** (`w5500: up on bus 1`,
   `netif: link=W5500`) — SCI/CS/VERSIONR works — but networking over it doesn't complete (dcwnet
   silent). WIP: instrument `w5500.cpp` TX/RX. See `docs/09-networking.md` §"Flycast W5500 emulation".
2. **Modem (PPP) backend** — backport from `mppp.dll` (in Ghidra): serial + LCP/auth/IPCP → feed
   IPCP IP+DNS into `NetifOnLease`/`WriteDnsServers`. Doesn't fit the Ethernet `LinkOps`.
Reminder: the SDK-side image edits (deploy mppp/dcspi, ce.bib/reginit.ini) are NOT in the repo —
reproduce per `docs/09-networking.md` §SDK-side edits.

## Next action — KERNEL/USERLAND track  (older, 2026-06-24 — read `docs/07-userland-boot.md`)
Big progress past the old "SC_GetOwnerProcess/GetKHeap" fault: that turned out to be a
`DoImports` page-in fault from a **2.12-vs-3.0 `e32_rom` struct mismatch** (FIXED — `ROMLDR.H`
+ `loader.c`). With that + an API-table fix (`ProcMthds[4]=SC_ProcGetIndex`, FIXED), **coredll +
filesys load, filesys fully initialises and `SignalStarted`s, and `RunApps` launches a 2nd
process.** Decision REVERSED: do NOT try to rebuild the 3.0 userland (GWES has no source) — keep
the stock 2.12 modules and fix kernel-side 2.12/3.0 deltas as they surface (the modules' on-disk
structs + API method-table numbering must match 2.12; use Ghidra `get_struct_layout`/`read_memory`
on the SDK kernel as the spec).

**Current frontier:** the 2nd process (`p2`) is stuck on a cross-process `PerformCallBack`
(`Wn32:113`); a `DCDBG CB p%d->p%d pfn` probe was just added — run `disc.gdi` in Flycast and read
that line to localise. Full method, the two fixes, the active `DCDBG` probes (to strip later), and
the Win32 method-number table are in **`docs/07-userland-boot.md`**. Emulator debug + WinDBG
research (why `nkscifkd` garbles, Flycast SCIF is TX-only on Windows, no BBA) in
**`docs/08-emulator-debugging.md`**.

Build/test loop (retail): `build-nklib` (or `build-oal` if HAL changed) → `build-nk` → copy
`reference\kernel-obj\nk.exe` over `C:\wcedreamcast\release\retail\nknodbg.exe` → `build-image
retail` → `wrap-image.ps1 -Out reference\0winceos.ours.bin` → `make-gdi.ps1` → load
`reference\disc-gdi\disc.gdi` in Flycast (SerialConsole on). NOTE: `nknodbg.exe` is currently OUR
kernel (stock at `.stock`); `utils\ip.bin` = SDK `ip_drago.bin` (gitignored, recopy on clone).

## Conventions
- Batch `rem` lines must be plain ASCII — no `>` or em-dash (cmd treats `>` as redirection).
- Kernel compiles use a **pure CE 3.0 include chain** — never mix the 2.12 DC SDK `inc`.
- Don't commit copyrighted binaries beyond what's already vendored; `reference/*.bin` stays ignored.
