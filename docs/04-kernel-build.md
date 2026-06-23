# Kernel build — compiling the leaked CE 3.0 SH-4 NK

Status: **in progress** (header reconciliation). The SH-4 compiler builds CE kernel
sources; the work is reconciling the leaked WINCE300 `NK` headers (an incomplete
snapshot, no matching PUBLIC tree) into a clean compile. Driver: `toolchain\build-kernel.bat`.

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

## Current frontier
`KWIN32.H(81,135,…)`: `CALLBACKINFO` and several function-pointer typedefs undefined — the next
type-skew layer (a kernel/toolhelp header the leak places elsewhere, or another OAK-vs-leak skew).
Same reconciliation pattern repeats per layer.

## Method (repeatable)
1. `build-kernel.bat retail` → read first errors.
2. Locate the missing symbol's definition (`grep` leak + `ce3-oak` + `ce3-ppc2k`).
3. If present in a header → add that dir/header to the include chain.
4. If missing from the leak → reconstruct from the ABI (`KSSHX.H` asm offsets, the other CPUs'
   headers, `ERRFALSE` contracts) into a `bsp\inc\*_patch.h`, force-included.
5. Repeat until `SHFLOAT.C` (smoke) compiles, then expand to the full `NK\KERNEL` SOURCES.

## Honest scope
This is the multi-layer header grind the ROADMAP-style "P2" predicts. Each layer is mechanical
but real. End state: `nknodbg.exe` rebuilt from leaked source, dropped into the (validated)
makeimg pipeline. The toolchain and image halves are already proven; this is the remaining gap.
