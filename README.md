<h1 align="center">Windows CE Dreamcast Community Edition</h1>

<p align="center">
  <img src="screens/dcshell/desktop.PNG" width="640" alt="DCWin desktop"><br>
  <em>A real, windowed Windows&nbsp;CE desktop — running on a Sega Dreamcast.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-Sega%20Dreamcast-orange">
  <img src="https://img.shields.io/badge/CPU-SH--4-blue">
  <img src="https://img.shields.io/badge/Windows%20CE-2.12-lightgrey">
  <img src="https://img.shields.io/badge/build-CMake-green">
</p>

---

The Dreamcast shipped a stripped-down Windows CE 2.12 that retail games booted into and never
exposed. **Windows CE Dreamcast Community Edition** takes that same on-disc CE runtime and turns
it into something you can actually *use*: a multitasking windowed desktop and an in-progress
TCP/IP stack over the Broadband Adapter — all baked into a standard bootable Dreamcast disc.

It is **fully self-contained**: the SH-4 PE compiler and the entire CE image toolchain are
vendored in this repo. One `cmake` invocation goes from source to a bootable `disc.gdi`. No
Platform Builder, no SDK install, no CD key.

## Screenshots

| The desktop | Multitasking |
|:---:|:---:|
| <img src="screens/dcshell/desktop.PNG" width="380"> | <img src="screens/dcshell/windows.PNG" width="380"> |
| Icons, Start menu, taskbar, mouse cursor | Task Manager, Explorer browsing the disc, a live Clock — each app its own process |

## Features

- **DCWin desktop shell** — a windowed, multitasking desktop composited on the PVR2 /
  Direct3D: move / resize / minimize / maximize windows, a Start button, taskbar, and a mouse
  cursor. Every app runs in its own CE process.
- **Built-in apps** — Explorer (browse `\`, `\Windows`, `\CD-ROM`; launch binaries), Task
  Manager (live process + RAM view), Clock, Calculator, a memory tester, and a winsock network
  tester.
- **Networking over the stock CE stack** *(work in progress)* — a universal link shim
  (`mppp.dll`) aims to let the stock `microstk.exe` + `winsock.dll` run over **Ethernet**
  instead of dial-up PPP, with DHCP/ARP/DNS handled in the shim (DHCP option-6 → the
  Dreamcast's own flash ISP config → a public resolver). Both link backends — the **Broadband
  Adapter** and a **W5500/MACRAW over SPI** transport — are still being brought up and not yet
  working end-to-end.
- **Self-contained CMake build** — vendored SH-4 `cl.exe` + the CE image tools (`makeimg`,
  `romimage`, …). Produces **retail** (silent) or **debug** (serial console) images, and a
  standard bootable GDI.

## Building

**Prerequisites:** CMake ≥ 3.20 and a generator (Ninja). The pair bundled with Visual Studio
works out of the box — nothing else from VS is used. PowerShell is used for the disc-imaging
steps (Windows host).

```sh
# configure (point -DCMAKE_MAKE_PROGRAM at ninja if it isn't on PATH)
cmake -G Ninja -S . -B build

# 1) just the SH-4 modules -> build/modules/  (dcspi.dll, mppp.dll, dcshell.exe, dcw* apps)
cmake --build build

# 2) the bootable OS image -> build/0winceos.bin
cmake --build build --target image

# 3) the full chain -> build/disc/disc.gdi
cmake --build build --target gdi
```

### Build options

| Option | Default | Effect |
|--------|---------|--------|
| `-DKERNEL=retail\|debug` | `retail` | `retail` = the silent kernel. `debug` = the SCIF serial-console kernel. Changes boot logging only. |
| `-DDLLS=retail\|debug` | `retail` | `retail` = the stock OS DLLs (what real games run on). `debug` = the checked (assert-heavy) DLLs; these break some titles (e.g. DirectDraw/DDHAL), so leave this `retail` unless you're chasing a system-DLL bug. |
| `-DAUTORUN=<exe>` | `dcshell.exe` | The program launched at boot (`HKLM\init` `Autorun`). Use forward slashes for a path, e.g. `-DAUTORUN=/CD-ROM/DC.EXE` to autostart a disc binary. |
| `-DEXTRADATA=<dir>` | *(none)* | Folder whose contents are placed in the disc's `\CD-ROM` (e.g. a game's files). Relative paths resolve against the repo root; our OS image always wins for `0WINCEOS.BIN`. |

```sh
# example: a SCIF serial-console image (over the working retail DLLs) with a game on the disc
cmake -S . -B build -DKERNEL=debug -DEXTRADATA=path/to/game
cmake --build build --target gdi
```

## Running

The build produces a standard multi-track **`build/disc/disc.gdi`** that boots on real Dreamcast
hardware:

- **GDEMU / MODE / USB-GD-ROM** — copy the `disc.gdi` + its track files onto the SD card / image
  and select it from the menu.
- **Burned disc** — for a GD-R / GDEMU-exact layout, use `toolchain/make-gdi-real.ps1` to
  rebuild against a real CE-game GDI (matches the original IP.BIN + track geometry).

The default image boots straight to the DCWin desktop. For diagnostics, build with
`-DKERNEL=debug` and watch the Dreamcast serial (SCIF) console.

## Repository layout

| Path | What |
|------|------|
| `CMakeLists.txt`, `cmake/` | the entire build (modules → image → GDI) |
| `shell/` | the DCWin desktop shell, PVR2/D3D compositor, and the client apps |
| `net/netif/` | the universal `mppp.dll` link shim (BBA + W5500 backends, DHCP/ARP/DNS) |
| `drivers/dcspi/` | reusable SPI transport (SCI hardware-SPI + SCIF bit-bang) for the W5500 |
| `toolchain/` | `wrap-image.ps1` / `make-gdi.ps1` / `make-gdi-real.ps1` + the build README |
| `vendor/sh-toolchain/` | the SH-4 PE compiler + CE headers |
| `vendor/wcesdk/` | the vendored CE 2.12 SDK: headers, libs, image tools, OS modules, kernels |

See [`toolchain/README.md`](toolchain/README.md) for build internals.

## Notes

This project builds on the Sega "Dragon" Windows CE for Dreamcast SDK (vendored). The shell,
networking shim, and SPI driver are original work; the CE kernel and system modules are the
stock SDK binaries. Don't redistribute the vendored SDK binaries outside the spirit of this
research project.
