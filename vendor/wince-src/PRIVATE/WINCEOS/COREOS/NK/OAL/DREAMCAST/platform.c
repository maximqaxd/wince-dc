/*
 * platform.c - Dreamcast OAL platform identity + memory sizing.
 * RECONSTRUCTED from nknodbg.exe (OEMGetPlatformVersion @ 0x8C03CE7C,
 * OEMGetExtensionDRAM @ 0x8C03CE70). See OAL-NOTES.md.
 */
#include <windows.h>
#include "dc_hw.h"

/* Katana board "Set" (4 = retail-ish, 5 = dev HW). OEMInit reads this for its
 * "Set N detected" log. The shipped detect probes the ASE BIOS; we don't use the
 * Debug Adapter (we log via SCIF), so default to Set 4. */
DWORD OEMPlatformVersion = 4;

/* SH-4 D-cache line count (16 KB / 32 B = 512). Used by _FlushDCache (shexcept). */
DWORD SH4CacheLines = 512;

int OEMGetPlatformVersion(void *pv)
{
    if (pv) {
        ((DWORD *)pv)[0] = OEMPlatformVersion;   /* Set */
        ((DWORD *)pv)[1] = 0;                     /* Revision */
    }
    return 1;
}

/* No extension DRAM on the DC (16 MB main only). Shipped kernel: return 0. */
int OEMGetExtensionDRAM(ULONG *pStart, ULONG *pLen)
{
    return 0;
}
