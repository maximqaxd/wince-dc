# bba — Dreamcast Broadband Adapter driver (WDM)

> **RETIRED (2026-06-27).** This standalone WDM driver is superseded by the universal
> networking shim: its RTL8139 hardware was factored into `net/netif/bba_hw.c`, which the
> `mppp.dll` link shim drives directly so the **stock** CE TCP/IP stack runs over the BBA
> (verified: DHCP→DNS→TCP→HTTP). Do NOT load this `bba.dll` alongside the shim — both
> would fight over the same RTL8139 (remove its `ce.bib` MODULES line + the
> `[HKLM\WDMDrivers\BuiltIn\BBA]` registry block). Kept in-tree as the RTL8139/GAPS
> hardware reference. See `docs/09-networking.md`.

A Dreamcast WinCE **WDM driver** for the HIT-0400 Broadband Adapter (RTL8139C on
the G2 bus), built against the Katana SDK (SH-4) + `wdm.lib`. This is the model the
DC actually uses (`maple.dll`, `sh4ser.dll`, `seg_rock.dll`), discovered the hard
way — see "Driver model" below. Hardware sequence ported from KallistiOS
(`broadband_adapter.c`, `g2bus.c`, `fifo.h`).

## Status (RESUME HERE)
**Loads, but DriverEntry not yet confirmed.** With the WDM registration the DC's
`wdevice.exe` now finds and loads `bba.dll` (the earlier silent boot was the wrong
model — see below). Current boot log:
```
LoadWDMDriver(bba.dll) failed 1114        (1114 = ERROR_DLL_INIT_FAILED)
```
No `BBA DriverEntry` line appears, so the `wdm.lib InitWDMDriver -> DriverEntry`
bridge is failing *before* our code logs. **Next step:** find why InitWDMDriver
returns failure / DriverEntry isn't reached. Ideas, in order:
1. Confirm DriverEntry is actually called — does `wdm.lib`'s `InitWDMDriver` look
   up our `DriverEntry` by exact name/decoration? Maybe `DriverEntry` must be
   **exported** (add it to `bba.def`) or named differently. Compare a working WDM
   driver (`maple.dll`) exports/imports.
2. wdmlib may require more than `DriverEntry` (a device-name reg value, an
   `AddDevice`, or a specific return). Check what `seg_rock` (also 1114) vs
   `maple`/`sh4ser` (load OK) differ on in reginit.ini / their DLLs.
3. `IoCreateDevice(L"\\Device\\BBA1", ...)` may be failing — try `NULL` name, or a
   different `DeviceType`. (But we'd expect the `BBA DriverEntry` log first.)
4. Possible DllMain/entry interaction with `wdm.lib`'s own startup — try linking
   without `/entry:DllMain` (let the lib provide the entry), or provide what it wants.

The hardware probe itself (GAPS init + RTL8139 reset + MAC read over G2, SEH-guarded)
is believed correct (KOS-derived) but **untested** until DriverEntry runs.

## Driver model (important — the DC does NOT use CE stream drivers)
The DC's `wdevice.exe` loads drivers via **`LoadWDMDriver`** from
**`HKLM\WDMDrivers\BuiltIn\<Name>`** (`"Dll"`, `"Order"`, `"FriendlyName"`) — NOT
the `HKLM\Drivers\BuiltIn` stream model (`XXX_Init`/`Prefix`/`Index`), which is
ignored here. A WDM driver:
- provides NT-style `DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING)`,
- links **`wdm.lib`** — it supplies the exported **`InitWDMDriver`** that
  `LoadWDMDriver` calls, which in turn calls your `DriverEntry`,
- uses `IoCreateDevice` + `MajorFunction[IRP_MJ_*]` dispatch + `IoCompleteRequest`,
- header `inc\wdm.h` (full NT DDK); runtime `wdmlib.dll` (already in the image),
- `.def` exports `InitWDMDriver`.

## Build
```
drivers\bba\build-bba.bat retail     :: -> reference\driver-obj\bba.dll  (SH-4, /dll)
```
Links `wdm.lib` + `coredll.lib` + `corelibc.lib`; entry `DllMain`; exports
`InitWDMDriver` (impl from `wdm.lib`). `SetKMode` is declared locally (coredll
export not in SDK headers). Build `debug` too for the debug image.

## Wire into the image
1. Copy `bba.dll` to `C:\wcedreamcast\release\<flavor>\OS\`.
2. Add to the source `platform.bib` MODULES block (after the dcw* lines):
   ```
   bba.dll         $(_FLATRELEASEDIR)$(RELEASEDIR_OS)bba.dll         NK  SH
   ```
3. Register under `HKLM\WDMDrivers\BuiltIn` — add to the source `gemini.reg`
   **after** the `[HKLM\init]` section (NEVER insert a `[HKEY...]` block mid-`init`
   — it reparents the following `LaunchXX` keys and kills sysstart):
   ```
   [HKEY_LOCAL_MACHINE\WDMDrivers\BuiltIn\BBA]
       "Dll"="bba.dll"
       "Order"=dword:00000004
       "FriendlyName"="WDM Broadband Adapter Driver"
   ```
4. `build-image <flavor>` -> `wrap-image` -> `make-gdi`. Use the **debug** flavor to
   see `DEBUGMSG`/`LoadWDMDriver` traces; retail strips them.

## Test
- Enable the Broadband Adapter in Flycast (Settings -> Network).
- Debug boot, SerialConsole on. Want: `BBA DriverEntry` then `BBA: up, MAC ...`.
  Currently: `LoadWDMDriver(bba.dll) failed 1114` (see Status).

## Hardware reference (KOS, verified addresses)
GAPS bridge `0xA1000000`; RTL8139 regs `GAPS+0x1700`; id string "GAPSPCI_BRIDGE_2"
@ `+0x1400`; glue magic `0x5a14a501` @ `+0x1418`; DMA window `+0x1428/+0x142c` =
`RTL_MEM 0xA1840000`; IRQ enable `+0x1414`. G2 FIFO wait: spin while
`*(u32*)0xA05F688C & 0x11`. MAC = `NIC(RT_IDR0)` two 32-bit reads.

## Next (after DriverEntry loads)
- Raw RX ring + TX (4 descriptors) via IOCTL, `CacheSync` on the DMA window.
- Interrupt-driven: map the BBA HOLLY EXT IRQ to a SYSINTR in our OAL
  (`isr.c`/`SB_ISTEXT`) + `InterruptInitialize` IST.
- lwIP (built with the SDK) on the frame IOCTLs.
