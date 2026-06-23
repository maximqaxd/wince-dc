# Dreamcast OAL — reconstruction notes (from SDK `nknodbg.exe`)

Source of truth: the Sega "Katana" CE 3.0 kernel **`C:\wcedreamcast\release\debug\nknodbg.exe`**
(SH-4 PE, `SuperH4:LE:32`, image base `0x8C000000`, embedded COFF symbols). These notes are
the spec for rewriting the `hal:*` OAL from source to link against our from-source `nkmain.lib`.

> Addresses below are **debug-build** virtual addresses (`0x8C03xxxx`). The retail layout
> differs; the *logic and the hardware registers* are identical. Reverse with Ghidra (project
> `wce`); register-pointer globals and the key functions are already named/commented there.

## Link recipe recap
`nknodbg.exe = nk:* (our nkmain.lib) + fulllibc:* (SH runtime) + hal:* (THIS) + asedbg:* (KD)`.
The `hal:*` objects = the OAL to reconstruct: `fwinit`(StartUp), `cfwkatan`+`fwkatana`
(OEMInit/INTC/serial), `ktimer`(InitClock/TMU), `timer`, `rtc`, `oemioctl`, `isr`, `mdppfs`, `oemwdm`.

## Dreamcast / SH7091 hardware register map (confirmed from the image)
### SH-4 on-chip (P4 control region)
| reg | addr | width | use in OAL |
|-----|------|-------|-----------|
| INTC ICR    | `0xFFD00000` | 16 | interrupt control |
| INTC IPRA   | `0xFFD00004` | 16 | TMU0/TMU1/... priority; OEMInit zeroes, InitClock sets TMU nibbles |
| INTC IPRB   | `0xFFD00008` | 16 | zeroed at boot |
| INTC IPRC   | `0xFFD0000C` | 16 | zeroed at boot |
| TMU base    | `0xFFD80000` |    | TOCR/TSTR(+4)/TCOR0(+8)/TCNT0(+0xC)/TCR0(+0x10)/TMU1(+0x14..)/TMU2(+0x20..) |
| DMAC base   | `0xFFA00000` |    | DMAOR at `+0x40` |
| SCIF        | `0xFFE80000` | 16 | DC serial (debug); ints wired via SerialInit |

### Holly / system bus (area 0, `0xA05Fxxxx` uncached) — interrupt masks
9 mask registers, all zeroed by `OEMInit` at boot (mask everything):
```
SB_IML2NRM 0xA05F6910   SB_IML2EXT 0xA05F6914   SB_IML2ERR 0xA05F6918   (level 2)
SB_IML4NRM 0xA05F6920   SB_IML4EXT 0xA05F6924   SB_IML4ERR 0xA05F6928   (level 4)
SB_IML6NRM 0xA05F6930   SB_IML6EXT 0xA05F6934   SB_IML6ERR 0xA05F6938   (level 6)
```
Status regs (read in the Holly ISR demux): `SB_ISTNRM 0xA05F6900 / SB_ISTEXT 0xA05F6904 /
SB_ISTERR 0xA05F6908` (confirm against KatanaISR* — see TODO).

## OAL functions (debug addresses)
| function | addr | obj | summary |
|----------|------|-----|---------|
| `OEMInit` | `8C03CAFC` | cfwkatan | master boot init — see sequence below |
| `InitClock` | `8C03D0CC` | ktimer | TMU bring-up (system tick + free-running QPC counter) |
| `OEMInterruptEnable/Disable/Done` | `8C03CC0C/CC4C/CC80` | cfwkatan | dispatch via `GInterruptList[sysintr-0x10]` (12-byte {en,dis,done} thunks) |
| `OEMInterruptStatus` | `8C03CCB4` | cfwkatan | reads/caches pending via `g_KIntrList[(id-0x10)*0x18]` |
| `OEMInterruptInclude/Exclude` | `8C03CD38/CE1C` | cfwkatan | add/remove source from a SYSINTR |
| `OEMIoControl` | `8C03D2F4` | oemioctl | HAL IOCTL dispatch (WDM init, intr status/incl/excl, RTC, platver) |
| `SerialInit` | `8C03DF54` | cfwkatan | HookInterrupt(0x28..2B, SCIFISR) — SCIF ERI/RXI/BRI/TXI |
| `OEMGetRealTime/SetRealTime/SetAlarmTime` | `8C03CF6C/D024/D02C` | rtc | RTC |
| `OEMInitDebugSerial/WriteDebugString/Byte/ReadDebugByte` | `8C03DAE8/DBD4/DCC4/DDE8` | (asedbg/cfw) | debug serial |
| `OEMGetPlatformVersion` | `8C03CE7C` | cfwkatan | Katana board-rev detect (Set 4 / Set 5) |

### `OEMInit` boot sequence (the skeleton to reproduce)
1. Zero INTC `IPRA/IPRB/IPRC` (16-bit) + all 9 Holly `SB_IML*` masks → all interrupts masked.
2. `OEMGetPlatformVersion(NULL)` → detect board rev; logs "Set 4/5 is detected" (Set5 = DC dev HW).
3. `InitClock()` → TMU.
4. `OEMParallelPortInit()` → debug parallel port.
5. INTC vector → ISR table via `HookInterrupt(vec, isr)`:
   | vec | ISR | source |
   |-----|-----|--------|
   | 0x0D | KatanaISR2 | Holly IRL level 2 |
   | 0x0B | KatanaISR4 | Holly IRL level 4 |
   | 0x09 | KatanaISR6 | Holly IRL level 6 |
   | 0x20 | JTAGISR    | JTAG/debug |
   | 0x22..0x25 | DMAC0..3ISR | SH-4 DMAC channels 0-3 |
   | 0x10 | Timer0ISR  | TMU0 (system tick) — hooked by InitClock |
   | 0x11 | Timer1ISR  | TMU1 — hooked by InitClock |
   | 0x28..0x2B | SCIFISR | SCIF ERI/RXI/BRI/TXI — hooked by SerialInit |
6. `SerialInit()`.
7. `DMAC.DMAOR (0xFFA00040) = 0x8201` (enable DMA); pTOC fixups written through the P2 uncached
   alias (`addr | 0x20000000`).

### `InitClock` (TMU) specifics
- TMU0: `TCR0=0x20` (UNIE), `TCOR0=TCNT0=312500` (`TicksPerPeriod`) → periodic system tick.
- TMU1: `TCR1=0`, `TCOR1=TCNT1=125000`.
- TMU2: `TCR2=0x18`, `TCOR2=0xFFFFFFFF` → free-running down-counter for `QueryPerformanceCounter`.
- Sets TMU priorities in `IPRA` (TMU0 nibble<<12, TMU1 nibble<<8).
- `TSTR(+4) |= 0x05` → start TMU0 + TMU2.

### `OEMInterrupt*` model
CE SYSINTR ids start at `0x10`; range `[0x10, 0x10+0x13)`. `GInterruptList` is an array of
12-byte entries `{ PFN enable; PFN disable; PFN done; }` indexed by `(sysintr-0x10)`. Enable/
Disable/Done just call the matching per-source thunk. `OEMInterruptStatus` walks a parallel
`g_KIntrList` (0x18-byte entries) caching pending bits read from the source's status register.

## Boot entry: `StartUp` (hal:fwinit.obj)  [debug `0x8C00B2B0`, retail `0x8C0020C0`]
Minimal — the DC IPL / `1ST_READ.BIN` loader has already copied the image into RAM:
```asm
stc   SR, r0
mov.l =0x10000000, r2     ; BL (block all exceptions)
or    r2, r0
ldc   r0, SR              ; SR |= BL
jmp   @KernelStart        ; KernelStart @ 0x8C00B340 = nk:shexcept.obj (WE BUILD THIS)
```
So the OAL boot stub is ~6 instructions; the real SH-4 bring-up (VBR, cache CCR, MMU/TLB,
stacks) lives in `KernelStart`, which is part of our `nkmain.lib`. Reconstruction is trivial.

## Interrupt demux: `KatanaISR2/4/6` (hal:cfwkatan.obj) — RESOLVED
`IsrConstants` is just the Holly SB register file base `0xA05F6900` (not a struct). The three
SH-4 IRLs the Holly raises (OEMInit hooks vec 0x0D/0x0B/0x09) each map to one Holly interrupt
*class* — `pending = SB_IST<class> & SB_IML<level><class>` — using **mask-on-receipt**: on
dispatch the source bit(s) are cleared in the `SB_IML` mask (can't re-fire), and the kernel
re-enables via `OEMInterruptDone`. Each returns the CE `SYSINTR` for the top pending group.

| handler | class | reads | bit-group → SYSINTR (SB_IML clear-mask) |
|---------|-------|-------|------------------------------------------|
| KatanaISR4 (IRL4) | NRM (PVR/TA/DMA) | `SB_ISTNRM & SB_IML4NRM` | b0-11→0x10 (`&=FFC7F000`), b12-13→0x12 (`&=FFFFCFFF`), b14→0x15 (`&=FFFFBFFF`), b15→0x18 (`&=FFFF7FFF`) |
| KatanaISR6 (IRL6) | EXT (Maple/GD/AICA/BBA) | `SB_ISTEXT & SB_IML6EXT` | b0→0x14, b1→0x17, b2→0x1A, b3→0x1B (clear that bit) |
| KatanaISR2 (IRL2) | ERR | `SB_ISTERR & SB_IML2ERR` | b0-7→0x11, b8-11→0x13, b12-14→0x16, else→0x19 |

`Timer0ISR` (vec 0x10): clear TMU0 underflow (TCR0+0x10), advance the kernel tick (`CurMSec` +
reschedule accumulator @ KData `0x8C042888`) by 25 ms, return `SYSINTR_RESCHED` (1).

## Debug console: SCIF (ours — `dbgserial.c`)
The shipped kernel does NOT use the SCIF for debug — `OEMWriteDebug*` route through the Sega ASE
BIOS / hardware Debug Adapter (`g_DAPresent` + ASEBIOS_VECTOR; the `asedbg` objs). For bring-up
we instead write plain text out the SH-4 SCIF (`0xFFE80000`), polled, 8N1 @ 57600, NO KITL/WinDbg:
`OEMInitDebugSerial` (SCSCR2=0 → reset FIFOs → SCSMR2=8N1 → SCBRR2=divisor → SCSCR2=TE|RE),
`OEMWriteDebugByte` (poll SCFSR2.TDFE, write SCFTDR2, clear TDFE|TEND), `OEMWriteDebugString`
(LF→CRLF), `OEMReadDebugByte` (polled). Divisor `N = 50MHz/(32*baud)-1` (57600→26, 115200→13).

## TODO (remaining)
- Label each Holly `SB_IST*` bit → the DC peripheral (Maple, PVR/VBlank, GD-ROM, AICA, BBA, …)
  to give the 0x10-0x1B SYSINTRs real device names (KOS has the SB_ISTNRM/EXT bit table).
- `Timer1ISR` body (aux timer); `mdppfs`/`oemwdm` — bus access (`OEMGetBusDataByOffset`) + WDM glue.
- Decode the `OEMIoControl` IOCTL constants (`DAT_8C03D440..450`) → `IOCTL_HAL_*` numbers.
## Trial link — DONE (`build-nk.bat` → `nk-link.log`)
`link /machine:SH4 /subsystem:windowsce,3.00 /entry:StartUp /base:0x8C040000 nkmain.lib oal_dc.lib
corelibc.lib`. The graph expands cleanly from `_StartUp` through the whole kernel + OAL — every
structural cross-ref resolves. **60 unresolved externals remain**, in buckets:
- **SH C runtime — mem/str (5):** `memcpy memset memcmp strcmp strlen` — write ourselves (trivial).
- **SH integer runtime (5):** `__divlu __modlu __divi64 __modi64 __modls` — SH has no divide; need
  real impls (can't use C `/` — infinite recursion). Provide as asm or bit-shift routines.
- **SH soft-float (19):** `__addd __subd __muld __divd __itod __dtoi … _sqrt` — only on the FPU-trap
  path (`HandleHWFloatException`); stub for first boot.
  > The DC SDK ships NO static libc (only `coredll.lib`, an import lib the kernel can't use). The
  > real kernel linked `fulllibc.lib` we don't have → we write a minimal SH-4 CRT (e.g. `NK\OAL\CRT`).
- **OAL funcs to write (~25):** RTC (`OEMGetRealTime/SetRealTime/SetAlarmTime`), `OEMIoControl`,
  platform (`OEMGetPlatformVersion`/`OEMPlatformVersion`/`OEMGetExtensionDRAM`), parallel
  (`OEMParallelPortInit/GetByte/SendByte`,`NoPPFS`), `SerialInit`, power/idle (`OEMPowerOff`,
  `OEMIdle`,`OEMNMI`,`OEMClearDebugCommError`), `SH4CacheLines`, `GInterruptList` table, per-source
  ISR stubs (`DMAC0-3ISR`,`JTAGISR`,`Timer1ISR`), tick global `dwReschedTime` (resolve to KData).
- **kernel stubs / missing files (6):** `CECompress/CEDecompress` (compile `compress.c`/`nocompr`),
  `ModuleJit/InitializeJit/PKDInit` (JIT + kernel-dbg — stub for nknodbg), `SC_GetTickCount`.

### CRT — DONE (`NK\CRT\SHX` → `crt.lib`, `build-crt.bat`)
mem/str + integer-divide + 64-bit shift in C (shift-subtract; the helper ABI is standard SH, so
C `_xxx` → `__xxx` symbol with no recursion), soft-float STUBBED. Cleared all 29 CRT symbols
(+`__lshi64`/`__rshui64`). **Link is now 60 → 31 unresolved**, all OAL/kernel (zero CRT).

### Link CLOSED — `nk.exe` builds from source (zero unresolved)
All 31 remaining symbols provided; `build-nk.bat` produces `reference\kernel-obj\nk.exe`
(SH-4 `0x1A6`, entry `StartUp`, ~264 KB). New OAL/stub files:
- `platform.c` — `OEMGetPlatformVersion`/`OEMPlatformVersion`(=Set 4)/`OEMGetExtensionDRAM`(=0)/`SH4CacheLines`(=512).
- `rtc.c` — `OEMGetRealTime` (fixed 2000-01-01 for now; real DC AICA RTC @0xA0710000 TODO), `OEMSetRealTime`/`OEMSetAlarmTime` (=0, faithful).
- `oemioctl.c` — `OEMIoControl` minimal (returns FALSE; wire HAL IOCTLs later).
- `serial.c` — `SerialInit` (hooks SCIF), `SCIFISR` stub, `OEMParallelPort*`/`NoPPFS`/`OEMClearDebugCommError` stubs.
- `power.c` — `OEMIdle` (no-op), `OEMPowerOff` (halt). `OEMNMI` is asm in `startup.src` (bare symbol, no `_`).
- `intr.c` — `GInterruptList` table + `DMAC0-3ISR`/`JTAGISR`/`Timer1ISR` stubs. `timer.c` defines `dwReschedTime`.
- `kstubs.c` — `SC_GetTickCount`(=CurMSec), `CECompress/CEDecompress` (no-compress), `ModuleJit/InitializeJit/PKDInit` stubs.

Link note: linked at `/base:0x00010000 /fixed:no` (the linker sign-extends high bases and trips
LNK1249); fixups are kept so makeimg/romimage rebases to the RAMIMAGE address.

### Boot-readiness TODO (the stubs are link-satisfiers, not working HW yet)
Real DC AICA RTC; real `OEMIoControl` HAL IOCTLs (SYSINTR request, reboot, deviceid) +
`OEMInterruptStatus/Include/Exclude`; per-source ISRs + driver registration into `GInterruptList`;
soft-float (or enable FPU); verify `SH4CacheLines`/`dwStoreQueueBase` for `_FlushDCache`.
Next mechanical step: drop `nk.exe` into makeimg (rebase 0x8C040000) + `wrap-image.ps1` → Flycast.
