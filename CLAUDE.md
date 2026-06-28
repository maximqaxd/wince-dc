# CLAUDE.md — wince-dc (Windows CE on the Sega Dreamcast)

Self-contained build that takes the Sega "Dragon" Windows CE 2.12 SDK and adds a real
networking stack, a windowed desktop shell, and an SPI transport — then bakes a bootable
Dreamcast disc with the vendored CE image tools + a real SH-4 PE compiler. Everything builds
from this repo via **CMake**; nothing external is needed.

> The user is a low-level Dreamcast/OS-porting expert — give expert-level, register-specific
> answers, no beginner framing. Sibling projects on their machine (ReactOS `sh4pe-toolchain`,
> `DreamShell`, `img4dc`) are SEPARATE — not part of this repo.

## Current state (resume here)
- ✅ **Self-contained CMake build.** The SH-4 compiler (`vendor/sh-toolchain`) and the whole
  CE 2.12 SDK (`vendor/wcesdk`: headers, libs, image tools, OS modules, patched kernels +
  `.map`/`.pdb`) are vendored. `CMakeLists.txt` (`project(NONE)`) drives the vendored
  `cl.exe`/`shasm.exe`/`link.exe` to build our modules, then `makeimg` → `wrap-image.ps1` →
  `make-gdi.ps1` for the bootable disc. Builds `retail` (silent) or `debug` (SCIF-console)
  images. See `toolchain/README.md`.
- ✅ **Networking — full TCP/IP over the STOCK CE stack.** The SDK's `mppp.dll` (dial-up PPP)
  is replaced by a universal link shim (`net/netif/`) so stock `microstk.exe` + `winsock.dll`
  run over Ethernet — no lwIP. **BBA path verified END-TO-END: DHCP → DNS → TCP → HTTP** (the
  `dcwnet` app resolved a hostname via the DHCP DNS server and fetched over the Broadband
  Adapter), and a real retail game (4x4 Evolution) dials + reaches its master server on real
  hardware. DNS resolution chains DHCP option-6 → DC system-flash ISP config (`flashrom.c`) →
  public resolver. The mppp/microstk link ABI + the MTU/byte-order/DNS-registry gotchas are in
  the `net/netif/` sources.
- ✅ **DCWin desktop shell + winsock apps.** `shell/` is a windowed desktop + PVR2/Direct3D
  compositor (move/resize/min/max, clipping, content-fill) with client apps in their own
  processes (`dcwcalc`/`dcwclock`/`dcwexp`/`dcwtask`/`dcwmem`/`dcwnet`). SDK-correct DirectInput
  (DC controller by Maple HID usage; mouse WIP). This confirms the stock kernel + userland boot
  to a usable multi-process desktop running winsock apps.
- 🔄 **W5500/MACRAW backend over SPI** (`drivers/dcspi/` SCI hardware-SPI + SCIF bit-bang;
  `net/netif/w5500.c`). The W5500 is detected over SPI on real hardware (VERSIONR reads), but
  off-link TCP doesn't complete yet — instrument the driver's TX/RX path. The modem (PPP)
  backend is not started.

## Setup on a fresh PC
1. `git clone <this repo>` — **fully self-contained.** Both the SH-4 compiler
   (`vendor/sh-toolchain`) and the CE 2.12 SDK (`vendor/wcesdk`: headers, libs, image tools,
   OS modules, patched kernels + their `.map`/`.pdb`) are vendored. No external SDK.
2. Install **CMake ≥ 3.20 + Ninja** (the VS-bundled pair works — see `toolchain/README.md`).
   Nothing else to configure; the build derives all paths from the repo.

## Build / commands (CMake — see `toolchain/README.md`)
```sh
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=<ninja> -S . -B build
cmake --build build                  # all SH-4 modules -> build/modules/ (dcspi/mppp/dcshell/dcw*)
cmake --build build --target image   # makeimg -> NK.bin -> wrap -> build/0winceos.bin
cmake --build build --target gdi     # full chain -> build/disc/disc.gdi
```
Configure-time options (kernel, DLL set, autorun, and disc extra-data are independent):
- `-DKERNEL=retail|debug` — `retail` (default) bakes the silent `nknodbg.exe`; `debug` bakes the
  patched (no-KD) SCIF-console `nkscifkd.exe`. This only changes boot logging.
- `-DDLLS=retail|debug` — `retail` (default) ships the stock OS DLLs (what real games run on);
  `debug` overlays the checked DLLs from `vendor/wcesdk/image-debug`. The checked DLLs assert and
  break some titles (e.g. DDHAL under DirectDraw), so a SCIF console over a *working* userland is
  `-DKERNEL=debug` alone (DLLs stay retail).
- `-DAUTORUN=<exe>` — program baked into `HKLM\init` `Autorun` (default `dcshell.exe`). Use
  forward slashes for a path, e.g. `-DAUTORUN=/CD-ROM/DC.EXE` to autostart a disc binary.
- `-DEXTRADATA=<dir>` — folder whose contents go into the disc's `\CD-ROM` (relative paths
  resolve against the repo root). Our `0winceos.bin` always overrides any `0WINCEOS.BIN` there.

The image step seeds the read-only `vendor/wcesdk/image` tree into `build/image`, applies the
optional checked-DLL overlay + our freshly-built modules + the Autorun edit
(`cmake/prep-image.cmake`), then runs `makeimg` (which re-merges the `.bib`s, so the `IMG*` env
flags pick the kernel). The makeimg env lives in `IMG_ENV` in `CMakeLists.txt`.

## Layout
- `CMakeLists.txt` + `cmake/prep-image.cmake` — the entire build (modules + image + gdi).
- `toolchain/` — `wrap-image.ps1`, `make-gdi.ps1`, `make-gdi-real.ps1`, `make-disc.ps1`,
  `unwrap-image.ps1`, `bootstrap.ps1`, `README.md` (the build doc).
- `net/` — networking. `netif/` = the universal microstk link shim (drop-in `mppp.dll`
  replacement; BBA verified + W5500 backend). `lwip-port/` + vendored lwIP = the alternative
  bring-your-own-stack route (built, unused). CMake `mppp` target.
- `drivers/` — `bba/` (RTL8139 driver — RETIRED into the shim's `bba_hw.c`, kept as HW ref);
  `dcspi/` (reusable SPI transport: SCI hardware-SPI + SCIF bit-bang, for W5500 now + SD/CF/FAT
  later; CMake `dcspi` target).
- `shell/` — the DCWin desktop shell + PVR2/Direct3D compositor + client apps. CMake `dcshell` +
  `dcw*` targets.
- `vendor/sh-toolchain/` — SH-4 compiler + CE headers. `vendor/wcesdk/` — the vendored CE 2.12
  SDK: `inc`, `lib/retail`, `tools`, `image` (retail OS modules + config + patched kernels
  `nknodbg.exe`/`nkscifkd.exe` + `.map`/`.pdb`), `image-debug` (checked DLL overlay).
- `reference/MANIFEST.md` — build artifacts + SHA-256 (binaries gitignored).

## Next actions
1. **W5500 off-link TCP.** Detected over SPI on real hardware but remote TCP doesn't complete on
   3 consoles/networks/W5500s. Instrument `net/netif/w5500.c` TX/RX (MACRAW frame in/out, ARP,
   socket state) on hardware. The `cf 1.1.1.1:80` probe in `dcwnet`, eager-ARP, SCI speed, and
   the torn-16-bit-read fix are in place but untested on hardware.
2. **Modem (PPP) backend** — serial + LCP/auth/IPCP, feeding the IPCP IP+DNS into
   `NetifOnLease`/`WriteDnsServers`. Doesn't fit the Ethernet `LinkOps`; a separate backend.

## Conventions
- Don't commit copyrighted binaries beyond what's already vendored; `reference/*.bin` stays
  ignored, `build/` is ignored.
- Our SH-4 modules are free (retail) builds; the debug image mixes the patched SCIF kernel +
  checked stock DLLs + our free modules (compatible at the syscall/PSL ABI).
