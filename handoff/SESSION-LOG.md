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
- **Kernel core: BUILDS FROM SOURCE.** All of `NK` kernel core archives into a verified SH-4
  `nkmain.lib` (26 objs, `0x1A6`). See the 2026-06-23 entry below.
- Open gap: the **Dreamcast OAL / boot layer** (StartUp, INTC/TMU/MMU init, KITL) for a
  linkable from-source `nk.exe` — not in the WINCEOS-only leak; it's ours to write.

## 2026-06-23 — kernel core compiles + archives (the WINCEOEM unlock)
6. **The "header-skew frontier" was a missing build switch, not missing headers.** `kfuncs.h`
   gates its `#include <pkfuncs.h>` (PRIVATE: `CALLBACKINFO`, `TRACKER_CALLBACK`, the `xxx_`
   macros via `mkfuncs.h`) behind `#ifdef WINCEOEM` / `#ifdef WINCEMACRO`. The CE3 kernel
   `SOURCES` set `WINCEOEM=1` + `-DWINCEMACRO` (+ `-DIN_KERNEL -DDBGSUPPORT` from NKNORMAL).
   Adding those to `KDEFS` cleared all 35 errors at once. **Bonus:** pkfuncs.h defines
   `VA_SECTION=25 / SECTION_SHIFT=25 / CURTLSPTR_OFFSET=0x000 / KINFO_OFFSET=0x300` — identical
   to the 4 values we blind-reconstructed in `mem_shx_patch.h` — so the patch was redundant and
   was **removed** (build is clean without it; `/FI` dropped from both build scripts).
7. **One header-aggregation gap:** `resource.c` needs `VS_FIXEDFILEINFO` (winver.h); the CE3
   base `windows.h` didn't aggregate winver.h. Added `#include <winver.h>` to
   `ce3-ppc2k\include\windows.h` (a complete windows.h does this; can't force-include — needs
   windows.h types first).
8. **SHX asm via `shasm.exe`** (Renesas SH asm, in the toolchain): `.include` resolves via
   `INCLUDE` env (KSSHX.H in NK\INC, KXSHX.H in ce3-ppc2k\include). Flags `-cpu=SH4 -DSH_CPU=64
   -DCELOG=0` (CELOG=0 mirrors NKNORMAL SHx ADEFINES; without it shexcept.src fails "CELOG not
   defined"). intrlock/shexcept/celogshx all assemble (shexcept emits a benign A546).
9. **Result:** `build-nklib.bat retail` → `reference\kernel-obj\nkmain.lib`, 26 members,
   `dumpbin` confirms machine `0x1A6` (SH4) + symbols GeneralException, _InterlockedIncrement,
   _SC_PerformCallBack4, scheduler. New drivers: `build-asm.bat`, `build-nklib.bat`.
   `build-kernel.bat` now takes an optional single-file arg for smoke compiles.
10. **OAL gap is now precisely mapped.** The SDK SHIPS the kernel symbol set:
    `release\{retail,debug}\nknodbg.exe`/`nk.exe`/`nkscifkd.exe` (SH-4 `0x1A6`, entry `8C0020C0`
    =`_StartUp`, base `8C000000`) + `.map` (named addresses) + `.pdb` (full symbols). The
    `nknodbg.map` link recipe = 4 groups: **`nk:*`** (21 objs = our `nkmain.lib` ✅),
    **`fulllibc:*`** (28 = SH toolchain runtime ✅), **`hal:*`** (10 = the OAL gap ❌:
    `fwinit`/`cfwkatan`/`fwkatana`/`ktimer`/`timer`/`rtc`/`oemioctl`/`isr`/`mdppfs`/`oemwdm`),
    `asedbg:*` (3 = SCIF KD, optional ❌). Next: Ghidra the SDK `nknodbg.exe` (SH-4, base
    `0x8C000000`) with its `.map`/`.pdb` as the spec; reconstruct `hal:*` into
    `PLATFORM/DREAMCAST/KERNEL/HAL/`; link vs `nkmain.lib`. Detail in `docs/04` §"Next — the OAL gap".
11. **OAL reversing — first pass done.** Fixed the Ghidra MCP transport (Python 3.14 has no
    `socket.AF_UNIX`; pinned `GHIDRA_MCP_URL=http://127.0.0.1:8089` in `C:\Dev\.mcp.json` →
    bridge uses TCP). Imported debug `nknodbg.exe` into Ghidra project `wce` (SH-4, base
    `8C000000`); the debug PE's embedded COFF symbols auto-named all 629 funcs incl. the whole
    OAL — no `.map` apply needed. Decoded the DC hardware map from `OEMInit`/`InitClock`:
    INTC IPRA/B/C `0xFFD00004/8/C`, TMU `0xFFD80000`, DMAC `0xFFA00000` (DMAOR `+0x40`=0x8201),
    SCIF ints `0x28..2B`, Holly `SB_IML{2,4,6}{NRM,EXT,ERR}` `0xA05F6910..6938`; INTC vector→ISR
    map (KatanaISR2/4/6=Holly IRL6/4/2, DMAC0-3, TMU0/1, JTAG, SCIF). Named+typed 18 MMIO globals
    and plate-commented OEMInit/InitClock/OEMInterruptEnable/OEMIoControl/SerialInit in Ghidra
    (saved). Wrote the reconstruction spec to the OAL notes (see below).
12. **OAL reconstruction started — builds SH-4 clean.** Placed in the kernel tree (CE-native,
    not a separate bsp island): `PRIVATE\WINCEOS\COREOS\NK\OAL\DREAMCAST\`. Decoded `StartUp`
    (`_StartUp` @ debug `0x8C00B2B0`): just `SR |= 0x10000000 (BL)` then `jmp KernelStart`
    (`0x8C00B340` = nk:shexcept.obj, already ours) — so the boot stub is ~6 instrs; the real
    SH-4 bring-up is already in nkmain.lib. Decoded the 3 `KatanaISR2/4/6` Holly IRL demuxers
    (read SB_IST* & SB_IMLn, ack W1C, return SYSINTR 0x10-0x1B) + `Timer0ISR` (TMU0 underflow,
    +25ms tick, SYSINTR_RESCHED). Wrote `startup.src` (boot stub), `dc_hw.h` (DC register map),
    `oeminit.c` (OEMInit), `timer.c` (InitClock+tick ISR), `intr.c` (OEMInterrupt* dispatch +
    demuxers), `SOURCES`, `OAL-NOTES.md`. New driver `build-oal.bat` → `oal_dc.lib` (4 objs,
    SH-4, archives clean). TODO: exact SB_IST bit->SYSINTR map for KatanaISR2/4, bind tick to
    KData, SCIF debug-out, then link nkmain.lib+oal_dc.lib into nk.exe.
13. **OAL pass 2 — ISR demux resolved + SCIF console.** Disassembled KatanaISR4/6/2:
    `IsrConstants` is just the Holly SB base `0xA05F6900`; the 3 IRLs map to Holly classes
    (IRL4=NRM, IRL6=EXT, IRL2=ERR), `pending = SB_IST<cls> & SB_IML<lvl><cls>`, **mask-on-
    receipt** (clear bit in SB_IML, kernel re-enables via OEMInterruptDone). Exact bit->SYSINTR
    groups decoded (e.g. ISR4: b0-11->0x10, b12-13->0x12, b14->0x15, b15->0x18). Rewrote intr.c
    KatanaISR2/4/6 accurately; bound Timer0ISR tick to CurMSec/dwReschedTime (KData 0x8C042888).
    **SCIF:** confirmed the shipped kernel does NO SCIF debug (it uses the ASE BIOS / Debug
    Adapter via g_DAPresent+ASEBIOS_VECTOR). Wrote our own polled SCIF console `dbgserial.c`
    (OEMInitDebugSerial/WriteDebugByte/WriteDebugString/ReadDebugByte; 8N1 @57600, no KITL) +
    SCIF regs in dc_hw.h. build-oal.bat now builds 5 objs -> oal_dc.lib clean. Ghidra ISR
    comments updated + saved. Committed kernel-core+OAL milestone as 1ec742f.
14. **`nk.exe` LINKS FROM SOURCE — zero unresolved (the headline milestone).** Trial-linked
    nkmain.lib + oal_dc.lib + corelibc -> got the bounded unresolved report (60), then closed it:
    (a) wrote a minimal SH-4 CRT in `NK\CRT\SHX\` (crt.lib) — mem/str + integer-divide + 64-bit
    shift in C (ABI is standard SH r4,r5->r0, verified vs SDK __divlu; shift-subtract avoids
    recursion), soft-float stubbed; the DC SDK ships no static libc (only the coredll import lib).
    60->31. (b) wrote the remaining OAL + kernel stubs (platform/rtc/oemioctl/serial/power/
    intr-stubs/kstubs); OEMNMI had to be asm in startup.src (shexcept imports it WITHOUT the C
    underscore). 31->0. `build-nk.bat` -> nk.exe (SH-4 0x1A6, entry StartUp, ~264 KB). Linked at
    /base:0x10000 /fixed:no (linker sign-extends high bases -> LNK1249; fixups kept so makeimg
    rebases to 0x8C040000). NOTE: the stubs are link-satisfiers, not working HW (fixed-time RTC,
    OEMIoControl->FALSE, ISR/soft-float stubs) — see OAL-NOTES.md "Boot-readiness TODO". New
    drivers: build-crt.bat, build-nk.bat. Next: makeimg + wrap-image.ps1 -> Flycast.

## How to resume (do this)
1. Read `CLAUDE.md`, then `docs/04-kernel-build.md` (the method + exact frontier).
2. `git clone`, place the DC SDK at `C:\wcedreamcast`, run `toolchain\bootstrap.ps1`.
3. `toolchain\build-kernel.bat retail` → take the first error → locate the symbol
   (`grep` across `vendor/wince-src` + `vendor/sh-toolchain/ce3-oak/INC` + `ce3-ppc2k/include`)
   → add the header to the include chain, or reconstruct from the ABI into a new
   `bsp/inc/*_patch.h`. Repeat until `SHFLOAT.C` compiles, then the full `NK\KERNEL` SOURCES.
4. Build `nknodbg.exe`, drop it over `C:\wcedreamcast\release\retail\nknodbg.exe`,
   `build-image.bat retail`, `wrap-image.ps1`, test on Flycast/lxdream then HW.

## 2026-06-23 (later) — THE KERNEL BOOTS (from-source nk.exe runs on Flycast)
Built a bootable disc and iterated kernel bring-up on Flycast/lxdream:
- **Disc:** GDI via the Half-Life DC pipeline (`utils\buildgdi.exe` + HL-DC `ip.bin`, bootfile
  `0WINCEOS.BIN`) is the working path (`make-gdi.ps1`); CDI via mkisofs+cdi4dc also works
  (`make-disc.ps1`). HL-DC is itself a WinCE port so its pipeline is the right reference.
- **Flycast MMU gate:** Flycast (`core/hw/sh4/modules/mmu.cpp` mmu_set_state) only emulates the
  full SH-4 MMU when it finds UTF-16 "SH-4 Kernel" at VA 0x8C0110A8/0x8C011118. Our link order
  left those zero, so the kernel faulted the instant it set MMUCR.AT. Fixed: StartUp writes the
  magic to RAM (uncached) + wrap-image.ps1 plants it at 0x10A8.
- **pTOC bug (the big one):** romimage MIS-PATCHES the C `pTOC` global because we link at
  /base:0x10000 (the linker LNK1249's on EXEBASE 0x8C040000) - its GetMapSymbols resolves pTOC to
  the wrong address, leaving the real global (VA 0x8c0414a8) = 0xFFFFFFFF. Retail compiles out the
  pTOC==-1 guard, so KernelRelocate walked garbage copy entries (infinite loop). Fixed in
  mdsh3.c SH3Init: recover the real ROMHDR from the ROM signature romimage DOES write at
  RAMIMAGE+0x44 (0x8C010040="ECEC") and fix the global pre-MMU.
- **Result:** the from-source kernel now boots through KernelRelocate -> the real CE banner
  ("Windows CE Kernel for Hitachi SH... SH-4 Kernel") -> MMU+cache on -> OEMInit ("Set 4 is
  detected") -> "Booting Windows CE version 3.00" -> memory config -> the first thread. It then
  faults starting the first thread at PC=0xE3007FFC (CE PSL/API syscall-trap range) - the
  API-dispatch/trap mechanism is the next layer. Added `-DDEBUG` build support (build-*.bat
  [debug]) for more diagnostics; debug nk.exe links clean (0 unresolved).
- Diagnostic method: raw-SCIF markers in StartUp/SH3Init/KernelRelocate (now removed; kernel
  prints its own messages). build-nk.bat adds /MAP + /DEBUGTYPE:FIXUP for romimage.

## 2026-06-24 — USERLAND BOOTS (e32_rom + ProcMthds fixes) + emulator-debug research
Drove from-source `nk.exe` past the first-process fault into real userland. Full detail in
**`docs/07-userland-boot.md`**; emulator/WinDBG research in **`docs/08-emulator-debugging.md`**.
- Wired the **Ghidra MCP** properly: `C:\dev\Dreamcast\.mcp.json` runs
  `uv run --quiet C:\dev\ghidramcp\bridge_mcp_ghidra.py` with `GHIDRA_MCP_URL=http://127.0.0.1:8089`
  (203 tools; project `wce` = SDK debug `nknodbg.exe`). Used `get_struct_layout`, `read_memory`,
  `get_xrefs_to`, `decompile_function` as the authoritative 2.12 spec.
- **The old "SC_GetOwnerProcess/GetKHeap TLB miss" was really `DoImports+0x54`** reading coredll's
  import dir at the unmapped header page. Cause: **2.12 `e32_rom` ≠ 3.0** — 2.12 has `e32_subsys`
  @+24 and `e32_unit[]` @+28 (100 B, NO sect14); 3.0 added sect14 → shifted `e32_unit[]` +4 →
  `IMP` misread as `{0,0x31000}` instead of `{0,0}`. **FIXED** in `ROMLDR.H` + `loader.c:1701`
  (KEEP). → coredll + filesys load.
- Next fault: `APICall`→`ObjectCall` dispatch to PC=0. Probe found **`PROC` API method 4 = NULL**;
  2.12 `ProcMthds[4]=SC_ProcGetIndex` (3.0 zeroed the slot; fn still in `kmisc.obj`). **FIXED**
  `schedule.c ProcMthds[4]` (KEEP). → filesys fully inits, `RegisterAPISet`, `SignalStarted`,
  enters its idle service loop; `RunApps` launches the **2nd process**.
- **Frontier:** `p2` stuck on cross-process `PerformCallBack` (`Wn32:113`=`-1`); `SetCPUASID`
  migrates the thread into the target proc. `DCDBG CB p%d->p%d pfn` probe added — run + read.
- Timer confirmed firing (`[TMR]` heartbeat). Several `DCDBG` RETAILMSG probes remain in
  loader.c/virtmem.c/schedule.c/OBJDISP.C/timer.c — **strip when stable** (the two FIXES are not debug).
- **Emulator-debug research:** stock kernels can't log clean text in Flycast (`nk`/`nknodbg`→DA=silent;
  `nkscifkd`→KD-protocol garbage). Flycast Windows `SerialConsole` is **TX-only** (RX guarded to
  POSIX) so WinDBG can't attach as-is. No BBA driver in the SDK (modem/PPP only). Paths: (A) patch a
  kernel's `OEMWriteDebugByte`→SCIF for clean logging; (B) ~50-line Flycast patch to bridge SCIF↔TCP
  (bidirectional) → `nkscifkd` + WinDBG = real symbolic kernel debugging in the emulator.

## Key facts to keep handy
- Image base: RAMIMAGE @ `8C010000` (cached) = phys `0C010000`. Wrapper len pads to `0x800`.
- `ce.bib` pulls the kernel as `nk.exe` FROM `nknodbg.exe`. 28 other modules stay stock 2.12.
- No `explorer.exe` anywhere in the SDK; `gwes.exe` (windowing) IS present → a custom GUI
  shell/launcher is the path to a "desktop" (deferred; current focus is the kernel).
- Batch gotcha: `rem` lines with `>` or em-dash break cmd parsing. Keep rem ASCII.

## Networking session (2026-06-27) — full TCP/IP over the stock CE stack  (`docs/09-networking.md`)
Goal: networking without bringing our own stack. **Approach: replace the SDK `mppp.dll` (dial-up
PPP) with a universal link shim** (`net/netif/`) so the stock `microstk.exe` + `winsock.dll` run
over Ethernet. **Result: BBA path verified end-to-end in Flycast — DHCP → DNS → TCP → HTTP** (the
`dcwnet` app resolved www.sega.com and fetched it over the Broadband Adapter).
- **Link ABI** (decompiled from `microstk.exe`+`mppp.dll`, Ghidra project `wce`; captured in
  `net/netif/microstk_if.h`): microstk `LoadLibraryW("mppp.dll")`+`InterfaceInitialize` (called
  TWICE — distinct ifnet each); TX=`ifn_IPOutput`+must `FreePacket`; RX=alloc pkt+buf+memcpy+chain+
  `ifn_IPInput`@+0x54 (installed by `IPEnable` in `StackInitialize`, NOT the WinMain back-fill loop).
- **Three bugs, one debug round each:** (1) `IPInterfaceConfigure(ifn,ip,mask,MTU,peer)` — arg4 is
  **MTU(1500)**, NOT a gateway; this stack has **no route table** (flat subnet). Passing gw →
  garbage MTU → TX dead. (2) IPs are **network-order-as-LE everywhere** (IPInput compares
  `ifn_ipAddr` raw vs the wire dest, and writes it as the wire source) — do **not** byteswap.
  (3) DNS is **registry-only**: winsock reads `HKLM\Comm:"DnsServers"` (REG_BINARY `[hdr][ips]`,
  net order) per query — parse DHCP option 6 + `RegSetValueExW` it.
- **Shim pieces:** `netif.c` (LinkOps ladder W5500→BBA→modem; ARP/DHCP/DNS/routing; idempotent
  init), `bba_hw.c` (RTL8139 factored from `drivers/bba`, CS-locked G2), `ras.c` (13 `AfdRas*`
  stubs so dial titles "connect"), `netif.def` (16 exports at exact mppp ordinals), `w5500.c`.
- **W5500/MACRAW** backend reuses all of netif.c (MACRAW = raw ethernet). Transport =
  `drivers/dcspi/` (`dcspi.dll`: SCI hardware-SPI + SCIF bit-bang, ported from KOS
  `c:/dev/dreamcast/kallistios`; reusable later for SD/CF + FAT). **Hardware-only — untested** (no
  Flycast emu). Gated by `HKLM\Comm\Netif:"W5500Bus"` (1=SCI/PA7-CS, 2=SCIF/RTS-CS; absent=off, so
  the BBA path is untouched). Lazy-loaded → core networking has no link-time dep on dcspi.
- **SDK-side edits (NOT in repo — see doc):** deploy `mppp.dll`+`dcspi.dll` into `…\OS\`; `ce.bib`/
  `platform.bib` add `dcspi.dll` + remove standalone `bba.dll`; `reginit.ini`/`gemini.reg` remove
  `[HKLM\WDMDrivers\BuiltIn\BBA]` (was double-driving the RTL8139). `drivers/bba` standalone retired.
- **Stopped mid-** Flycast W5500 emulation (to make W5500 testable). Investigation complete: SCI is
  dumb RW cells in `serial.cpp` (easy byte-level hook), the host NAT bridge (`net::modbba`/
  `bba_recv_frame`, picotcp) is fully reusable for MACRAW frames. Plan in `docs/09-networking.md`.
- **Strip later:** diagnostic `OutputDebugString`s in `netif.c` (`netif TX[]/RX[]`, DNS write) +
  `w5500.c`.
