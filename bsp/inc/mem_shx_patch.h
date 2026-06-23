/*
 * mem_shx_patch.h
 *
 * Reconstructed SH-4 kernel constants that are MISSING from the leaked
 * WINCE300 SHx headers (MEM_SHX.H / NKSHX.H). The other architectures'
 * MEM_<cpu>.H define these; the SHx snapshot in the leak does not, and the
 * matching CE 3.0 PUBLIC headers were not part of the leak.
 *
 * Derivation (see docs/04-kernel-build.md):
 *   VA_SECTION       = 25   -- 2GB user VA / 64 sections => 32M each; identical
 *                             to MEM_X86.H (CE3 uses one VA layout: 4K page /
 *                             64K block / 32M section). KERNEL.H needs it @ L148.
 *   SECTION_SHIFT    = 25   -- KERNEL.H L217: ERRFALSE(SECTION_SHIFT==VA_SECTION).
 *   CURTLSPTR_OFFSET = 0x000-- KSSHX.H: "lpvTls: .equ h'000". KERNEL.H L199:
 *                             ERRFALSE(CURTLSPTR_OFFSET==offsetof(KDataStruct,lpvTls)).
 *   KINFO_OFFSET     = 0x300-- KSSHX.H: "PendEvents: .equ h'340" == aInfo[KINX_PENDEVENTS];
 *                             KINX_PENDEVENTS=16, 4-byte entries => aInfo[0] @ 0x340-0x40.
 *                             KERNEL.H L200: ERRFALSE(KINFO_OFFSET==offsetof(KDataStruct,aInfo)).
 *
 * Force-included ahead of KERNEL.H (build-kernel.bat: /FImem_shx_patch.h). The
 * ERRFALSE asserts in KERNEL.H independently re-validate these against the C
 * struct, so a wrong value here fails the build rather than passing silently.
 */
#ifndef MEM_SHX_PATCH_H
#define MEM_SHX_PATCH_H

#if defined(SH4) || defined(SH3) || defined(SHx)

#ifndef VA_SECTION
#define VA_SECTION        25
#endif
#ifndef SECTION_SHIFT
#define SECTION_SHIFT     VA_SECTION
#endif
#ifndef CURTLSPTR_OFFSET
#define CURTLSPTR_OFFSET  0x000
#endif
#ifndef KINFO_OFFSET
#define KINFO_OFFSET      0x300
#endif

#endif /* SHx */
#endif /* MEM_SHX_PATCH_H */
