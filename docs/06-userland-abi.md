# Userland ABI wall — struct-size diff (leak-compiled vs shipped PDB)

To decide whether the `SC_GetOwnerProcess`/`GetKHeap` fault (loading the first
process) is a header/flag mismatch in **our** kernel or genuinely the
3.0-kernel ↔ stock-2.12-module boundary, we diffed the structs our kernel
compiles (leak `KERNEL.H`, exact `build-nklib` flags) against the shipped SDK
kernel's authoritative layout (`cvdump -t nknodbg.pdb`, PDB 2.00).

## Method (reproducible)
- `cvdump -t` → `reference/symbols/pdb/nknodbg-retail.structs.txt` (shipped sizes/offsets).
- `toolchain/probe-abi.bat` compiles `NK\KERNEL\SHX\abi_probe.c` (compile-only, NOT a
  build member) with the **identical** include chain + `-DWINCEOEM -DWINCEMACRO
  -DIN_KERNEL -DDBGSUPPORT …` as `build-nklib.bat`. Each `sizeof`/`offsetof` is forced
  into the C2440/C4047 diagnostic (`char (*)[N] = 1`) so we read layout without running
  an SH-4 binary. Grep the warnings for `char (*)[N]`.

## Result
Cross-boundary ABI surface — **identical** (asm/OAL fixed offsets, PSL dispatch, context save):

| surface | ours | PDB |
|---|---|---|
| `KDataStruct` (pCurThd@148, handleBase@156, aInfo@768) | 896 | 896 |
| `CPUCONTEXT` + `Thread.ctx`@92 | 228 / 92 | 228 / 92 |
| `cinfo` (PSL syscall dispatch) | 20 | 20 |
| `_HDATA` / `o32_lite` / `openexe_t` | 32 / 28 / 16 | 32 / 28 / 16 |
| `Thread`: pProc@12 pOwnerProc@16 aky@20 tlsPtr@36; `Process`: pTh@16 aky@20 | match | match |

Kernel-internal structs — **differ, benign** (3.0 source vs the older 2.12 kernel Sega shipped):

| | ours (3.0) | PDB (2.12) | Δ |
|---|---|---|---|
| `Process` | 176 | 156 | +20 |
| `Thread`  | 540 | 324 | +216 |
| `e32_lite` (→ `Process.oe`@80, `e32`@96) | 72 | 64 | +8 |

The 3.0 `Thread` adds a scheduler tail (`dwKernTime`/`dwUserTime`, quantum, owned-crit
hash, crab fields, `IntrStk`) — diverges at offset 0x2c — yet by design re-lands `ctx`
at **0x5c=92** in both. `KERNEL.H`'s `ERRFALSE(THREAD_CONTEXT_OFFSET == offsetof(THREAD,
ctx))` passes at compile, so the layout is self-consistent.

## Verdict
**No header/flag mismatch to chase.** Our kernel is internally correct (it boots, MMUs,
runs the first thread); the size deltas are purely internal and confirm we built a
genuinely newer kernel than the one Sega shipped. Because `KDataStruct`, `CPUCONTEXT`/
`ctx`, and **`cinfo` (the PSL) are byte-identical**, the syscall trap and the few `Thread`
fields user-mode touches by fixed offset (`tlsPtr`@36, `dwLastError`@56, `aky`@20) all line
up — so the **3.0 `coredll` syscall thunks will be ABI-compatible with our kernel's
dispatch**. The fault is the 3.0-kernel ↔ stock-2.12-module wall; building the 3.0 userland
(`coredll`/`FSDMGR`/`DEVICE`/`GWES`) is the correct, unblocked next step.

## Re-run
```
toolchain\probe-abi.bat retail     :: emits sizeof/offsetof via compiler diagnostics
```

## Addendum — the leak can't rebuild the userland (so don't)
Enumerating `vendor/wince-src/PRIVATE/WINCEOS` after building `coremain.lib`: the leak
is a **kernel-focused snapshot** (~121 `.c` + 8 `.cpp` total). `NK` is complete; the
userland is fragmentary:

| module | present | missing |
|---|---|---|
| coredll (`CORE`) | `dll` (→`coremain.lib`) + `lmem` | thunks, locale, corelibc/fulllibc, imm, sip, fpemul, lpc, mui, res, asyncio, fmtmsg |
| DEVICE | `LIB\` (device/devload/pcmcia) | the rest of the device manager |
| FSDMGR | 7 `.c` (alloc/apis/init/misc/serv/stubs/tables) | — (likely buildable) |
| GWES (`GWE`) | only `MGDI\GPE\*.cpp` (SW blt/line/fill) | **the entire window mgr / GDI / message queue** |

A full from-source `coredll.dll`/`GWES`/`DEVICE` is therefore impossible from this leak.
Since the cross-boundary ABI is byte-identical (above), the productive path is to **keep
the stock 2.12 modules** and debug the kernel-side `SC_GetOwnerProcess`/`GetKHeap` fault
(first-process load, slot-1 `0x03d50000` TLB miss) against the Ghidra SDK-kernel spec.
`coremain.lib` stands as proof the userland toolchain + header chain work.
