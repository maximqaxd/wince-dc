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

## Interrupt demux: `KatanaISR2/4/6` (hal:cfwkatan.obj)  — the IRL→SYSINTR map
Three handlers, one per SH-4 external IRL the Holly raises (hooked in OEMInit: vec 0x0D/0x0B/0x09
= IRL 2/4/6). Each reads the Holly status latch (`SB_IST*`), ANDs the matching `SB_IML*` mask,
picks the highest pending bit, **ACKs by writing the masked status back**, and returns a CE
`SYSINTR` (firmware range 0x10-0x1B). `IsrConstants` is the per-level {status,mask} block.
| handler | reads | returns SYSINTR |
|---------|-------|-----------------|
| KatanaISR4 (IRL4) | SB_ISTNRM & SB_IML4NRM | 0x10, 0x12, 0x15, 0x18 |
| KatanaISR2 (IRL2) | SB_IST(+0x18) & SB_IML2(+8) | 0x11, 0x13, 0x16, 0x19 |
| KatanaISR6 (IRL6) | SB_ISTEXT & SB_IML6 (low 4 bits) | 0x14, 0x17, 0x1A, 0x1B |
`Timer0ISR` (vec 0x10): clear TMU0 underflow (TCR0+0x10), advance 64-bit kernel tick by 0x19
(=25 ms/tick), return `SYSINTR_RESCHED` (1).

## TODO (remaining)
- Map each Holly `SB_IST*` bit position -> the device (Maple, PVR/VBlank, GD-ROM, AICA, BBA, …)
  to label the 0x10-0x1B SYSINTRs; decode the `IsrConstants` struct field offsets (0/4/8/0x18/0x34).
- `mdppfs` / `oemwdm` — PCI-ish bus access (`OEMGetBusDataByOffset`) + WDM HAL glue.
- Decode the `OEMIoControl` IOCTL constants (`DAT_8C03D440..450`) → `IOCTL_HAL_*` numbers.
