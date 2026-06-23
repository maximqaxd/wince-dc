# Kernel build — compiling the leaked CE 3.0 SH-4 NK

Status: **kernel core builds + archives from source.** The complete CE 3.0 `NK`
kernel core — all 19 machine-independent C files, the 4 SHX machine-dependent C
files, and the 3 SHX `shasm` sources — compiles/assembles clean and archives into a
verified SH-4 `nkmain.lib` (26 objects, machine `0x1A6`). Driver:
`toolchain\build-nklib.bat`. The smoke driver `build-kernel.bat [file]` still
compiles any single C source; `build-asm.bat [file.src]` assembles a single asm source.

The remaining gap to a bootable from-source `nk.exe` is the **Dreamcast OAL / boot
layer** (StartUp entry, INTC/TMU/MMU init, KITL) — NOT present in the WINCEOS-only
leak; it lives in the closed 2.12 `0winceos.bin`. See "Next" below.

## The breakthrough: `WINCEOEM` / `WINCEMACRO`
The header-skew "frontier" (`CALLBACKINFO`, `TRACKER_CALLBACK`, fn-ptr typedefs) was
NOT a missing-header problem — it was a missing **build switch**. The CE3 kernel
`SOURCES` set `WINCEOEM=1` and `-DWINCEMACRO`; `kfuncs.h` gates its pull-in of the
PRIVATE `pkfuncs.h` (and `mkfuncs.h`) behind exactly those:
```c
#ifdef WINCEOEM
#include <pkfuncs.h>     // CALLBACKINFO, TRACKER_CALLBACK, SECTION_SHIFT, ...
#ifdef WINCEMACRO
#include <mkfuncs.h>     // xxx_ macros
#endif
#endif
```
Without them only the *public* kfuncs surface is visible, so every kernel-private
prototype fails to parse. Adding `-DWINCEOEM=1 -DWINCEMACRO` (plus `-DIN_KERNEL
-DDBGSUPPORT` from `NKNORMAL\SOURCES`) cleared all 35 errors at once.

**Bonus validation + cleanup:** `pkfuncs.h` defines `VA_SECTION=25`, `SECTION_SHIFT=25`,
`CURTLSPTR_OFFSET=0x000`, `KINFO_OFFSET=0x300` — *identical* to the four constants we
blind-reconstructed from the SH ABI in the old `bsp\inc\mem_shx_patch.h`. The
reconstruction was correct to the value, which made it redundant: with `-DWINCEOEM`,
pkfuncs.h is the authoritative source. **The patch file and its `/FI` force-include were
removed** — kernel files (incl. those hitting `KERNEL.H`'s `ERRFALSE` asserts) compile
clean without it. (The ABI derivation table below is kept as documentation.)

## Header-aggregation gap (resource.c)
`NK\KERNEL\resource.c` (version-resource loader) uses `VS_FIXEDFILEINFO`, which lives
in `winver.h`. The vendored CE3 base `windows.h` did not aggregate `winver.h` (a
complete `windows.h` does). Fixed by adding `#include <winver.h>` to
`vendor\sh-toolchain\ce3-ppc2k\include\windows.h` after `winreg.h`. Cannot be
force-included (depends on windows.h types).

## Assembling the SHX `.src` (Renesas shasm)
`shasm.exe` resolves `.include` via the `INCLUDE` env var (KSSHX.H in `NK\INC`,
KXSHX.H in `ce3-ppc2k\include`). Flags: `-cpu=SH4 -DSH_CPU=64 -DCELOG=0` — the last
mirrors `NKNORMAL\SOURCES` (`ADEFINES` for `_TGTCPUTYPE==SHx`); without it
`shexcept.src` fails "Symbol CELOG has not been defined yet". (`shexcept.src` emits a
benign `A546 Missing .ENDF` warning; object still produced.)

## Include chain (pure CE 3.0 — do NOT mix the 2.12 DC SDK inc)
```
bsp\inc                                  (our reconstruction patches)
wince-src\...\NK\INC                      (leaked kernel-private headers)
wince-src\...\NK\KERNEL\SHX               (SH asm/headers)
wince-src\...\CORE\INC
vendor\WindowsCE-Build-Tools\ce3-oak\INC  (OAK public headers)
vendor\WindowsCE-Build-Tools\ce3-ppc2k\include   (CE3 base SDK: windows/winnt/winbase/windef)
```
Defines: `-DSH4=1 -DSHx=1 -DUNDER_CE=300 -D_WIN32_WCE=300 -DUNICODE -D_UNICODE -DKERNEL`
Forced include: `/FImem_shx_patch.h`.

## Findings / decisions
1. **The 2.12 DC SDK headers are incompatible** with the CE 3.0 source — `windows.h` there
   redefines `DWORD`/`BOOL` differently and lacks CE3 constants (`LOAD_LIBRARY_AS_DATAFILE`,
   etc.). Fixed by using `ce3-ppc2k\include` as the base SDK header set.
2. **The leaked SHx headers are an incomplete snapshot.** `MEM_SHX.H` is missing `VA_SECTION`
   (present in `MEM_{ARM,MIPS,PPC,X86}.H`), and `SECTION_SHIFT`/`CURTLSPTR_OFFSET`/`KINFO_OFFSET`
   are defined nowhere in the leak — they'd have come from the matching (absent) PUBLIC headers.
   **Reconstructed** in `bsp\inc\mem_shx_patch.h` from the SH ABI:

   | const | value | source |
   |-------|-------|--------|
   | `VA_SECTION` | `25` | CE3 VA layout (4K page / 64K block / 32M section); == `MEM_X86.H` |
   | `SECTION_SHIFT` | `25` | `KERNEL.H` `ERRFALSE(SECTION_SHIFT == VA_SECTION)` |
   | `CURTLSPTR_OFFSET` | `0x000` | `KSSHX.H` `lpvTls .equ h'000` |
   | `KINFO_OFFSET` | `0x300` | `KSSHX.H` `PendEvents=0x340`=`aInfo[KINX_PENDEVENTS=16]`, 4B ⇒ `aInfo[0]@0x300` |

   `KERNEL.H`'s `ERRFALSE` asserts re-validate each against the C `struct KDataStruct`, so a wrong
   value fails the build rather than passing silently. Effect: cleared all 4; errors 50 → 35.

   The 4 reconstructed values were later confirmed identical to the canonical `pkfuncs.h`
   definitions once `-DWINCEOEM` pulled that header in (see top). Reconstruction → validated →
   `mem_shx_patch.h` removed as redundant.

## Build result (verified)
`build-nklib.bat retail` produces `reference\kernel-obj\nkmain.lib`:
- 26 members, machine `0x1A6` (SH4) — `dumpbin /headers schedule.obj` ⇒ "1A6 machine (SH4)".
- Symbols present incl. `GeneralException` (shexcept.src), `_InterlockedIncrement`
  (intrlock.src), `_SC_PerformCallBack4` + the scheduler (kmisc.c / schedule.c).
- C: debug resource objdisp heap ppfs compr2 printf loader mapfile virtmem physmem
  schedule kwin32 kmisc intrapi stubs exdsptch memtrk profiler · mdsh3 shunwind strings shfloat.
- asm: intrlock shexcept celogshx.

## Next — the OAL gap (to a bootable from-source nk.exe)
The kernel *core* is done. The exact remaining gap is now **known precisely** from the SDK's
own kernel link maps — `C:\wcedreamcast\release\{retail,debug}\nknodbg.map` (+ `nk.map`,
`nkscifkd.map`) and `.pdb`. `nknodbg.exe` is SH-4 (`0x1A6`), entry `8C0020C0` (`_StartUp`),
image base `8C000000`. Its link recipe is four object groups:

| group | what | our status |
|-------|------|-----------|
| `nk:*` (21 objs) | schedule, kwin32, loader, mdsh3, shexcept, strings, … | ✅ **= our `nkmain.lib`** |
| `fulllibc:*` (28) | SH runtime/intrinsics (i64 math, memmove, strcmp, …) | ✅ in the SH toolchain libs |
| `hal:*` (10) | `fwinit`(_StartUp), `cfwkatan`+`fwkatana`(OEMInit/INTC — "Katana"=DC codename), `ktimer`+`timer`+`rtc`(TMU/RTC), `oemioctl`, `isr`, `mdppfs`, `oemwdm` | ❌ **the OAL — reconstruct** |
| `asedbg:*` (3) | `debug`/`init`/`isr` — SCIF kernel-debugger serial | ❌ optional (KD) |

So the reconstruction target is just the **`hal:*` Dreamcast OAL** (~10 objs) + optionally
`asedbg:*`. Named entry points to implement (addresses from the retail map):
`_StartUp` `8c0020c0`, `_OEMInit` `8c01c780`, `_OEMInterrupt{Enable,Disable,Done,Status,
Include,Exclude}` `8c01c8a8…`, `_InitClock` `8c01cb50`, `_OEMIoControl` `8c01ccd4`,
`_OEMInitDebugSerial`/`_OEMWriteDebug*` `8c01d334…`.

**Reconstruction underway** in the kernel tree: `PRIVATE\WINCEOS\COREOS\NK\OAL\DREAMCAST\`.
`OAL-NOTES.md` captures the DC register map (INTC/TMU/DMAC/SCIF + Holly `SB_IML*`), the `OEMInit`
boot sequence, the INTC vector→ISR table, the `KatanaISR*` demux + `StartUp` — reverse-engineered
from the SDK `nknodbg.exe` (Ghidra project `wce`, OAL functions commented + MMIO globals
named/typed). First sources written + building SH-4 clean (`build-oal.bat` → `oal_dc.lib`):
`startup.src` (boot stub: `SR|=BL; jmp KernelStart`), `oeminit.c` (`OEMInit`), `timer.c`
(`InitClock`+tick ISR), `intr.c` (`OEMInterrupt*` dispatch + Holly IRL demuxers), `dc_hw.h`.

Two paths: (a) **reconstruct** the DC OAL from the SH-4 manual + DC memory map (Ghidra the
SDK `nknodbg.exe` using its `.map`/`.pdb` as the spec), link against `nkmain.lib` →
fully-from-source `nk.exe`; (b) binary-graft the from-source core over the closed image
keeping its OAL (risky: layout/ABI coupling, `RAMIMAGE @ 8C010000`). Recommend (a) — the
named maps make it tractable and it's testable on Flycast/lxdream. Either path wraps via the
proven makeimg + `wrap-image.ps1` pipeline.

## Method (repeatable, for the OAL grind)
1. `build-nklib.bat retail` → kernel core (green).
2. For each new link/compile gap: `grep` leak + `ce3-oak` + `ce3-ppc2k` for the symbol.
3. Present in a header → add to the include chain. Missing from the leak (OAL/boot) →
   reconstruct from the SH-4 HW manual + DC memory map into `bsp\` (the OAL is ours to write).
4. Wrap with makeimg + `wrap-image.ps1`; test Flycast/lxdream → dc-load/BBA on HW.
