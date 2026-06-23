/*
 * intr.c - Dreamcast OAL interrupt layer (was hal:cfwkatan.obj).
 *
 * RECONSTRUCTED from release\debug\nknodbg.exe:
 *   OEMInterruptEnable/Disable/Done @ 0x8C03CC0C/CC4C/CC80
 *   OEMInterruptStatus             @ 0x8C03CCB4
 *   KatanaISR2/4/6                  @ 0x8C03D880/D914/D9AC
 * See OAL-NOTES.md.
 *
 * Model: CE logical interrupts (SYSINTR) start at 0x10. GInterruptList[] is one
 * 3-PFN entry {enable,disable,done} per SYSINTR (idx = sysintr-0x10). The Holly
 * raises SH-4 IRL 2/4/6; KatanaISRn reads the matching SB_IST* status, ANDs the
 * SB_IMLn mask, ACKs the top pending bit (write-back), and returns the SYSINTR.
 */
#include <windows.h>
#include "dc_hw.h"

#define SYSINTR_NOP         0
#define SYSINTR_FIRMWARE    0x10
#define NUM_SYSINTR         0x13        /* 19 dispatch slots (OEMInterruptEnable bound) */

typedef void (*PFN_INTR)(int sysintr);

typedef struct _INTR_VECTOR {
    PFN_INTR pfnEnable;
    PFN_INTR pfnDisable;
    PFN_INTR pfnDone;
} INTR_VECTOR;

/* Populated by the per-source registration (Maple/PVR/GD/AICA/BBA drivers). */
extern INTR_VECTOR GInterruptList[NUM_SYSINTR];

/* ====================================================================== */
/* SYSINTR enable/disable/done - thin dispatch to the per-source thunks.   */
/* ====================================================================== */
BOOL OEMInterruptEnable(DWORD idInt, LPVOID pvData, DWORD cbData)
{
    DWORD idx = idInt - SYSINTR_FIRMWARE;
    if (idx < NUM_SYSINTR) {
        if (GInterruptList[idx].pfnEnable)
            GInterruptList[idx].pfnEnable(idx);
        return TRUE;
    }
    return FALSE;
}

void OEMInterruptDisable(DWORD idInt)
{
    DWORD idx = idInt - SYSINTR_FIRMWARE;
    if (idx < NUM_SYSINTR && GInterruptList[idx].pfnDisable)
        GInterruptList[idx].pfnDisable(idx);
}

void OEMInterruptDone(DWORD idInt)
{
    DWORD idx = idInt - SYSINTR_FIRMWARE;
    if (idx < NUM_SYSINTR && GInterruptList[idx].pfnDone)
        GInterruptList[idx].pfnDone(idx);
}

/* ====================================================================== */
/* Holly IRL demuxers (decoded from KatanaISR4/6/2 @ 0x8C03D914/D9AC/D880). */
/*                                                                          */
/* The SH-4 takes three external IRLs from the Holly; each maps to one      */
/* Holly interrupt CLASS, read as (SB_IST<class> & SB_IML<level><class>):   */
/*    IRL6 -> EXT  (external: Maple, GD-ROM, AICA, BBA, ...)                 */
/*    IRL4 -> NRM  (normal: PVR render/vblank/TA, DMA done, ...)            */
/*    IRL2 -> ERR  (error conditions)                                       */
/* On dispatch the source's bit(s) are MASKED in SB_IML<...> (mask-on-      */
/* receipt); OEMInterruptDone re-enables them. Each handler returns the     */
/* CE SYSINTR for the top pending group, or SYSINTR_NOP if none.            */
/*                                                                          */
/* TODO: label which SB_IST bit = which DC peripheral (Maple/PVR-VBlank/    */
/* GD/AICA/BBA) to give the 0x10-0x1B SYSINTRs real device names.           */
/* ====================================================================== */

/* IRL6 / external sources -> SYSINTR 0x14/0x17/0x1A/0x1B (status bits 0..3). */
ULONG KatanaISR6(void)
{
    DWORD pending = VUINT32(SB_ISTEXT) & VUINT32(SB_IML6EXT);
    if (pending & 0x1) { VUINT32(SB_IML6EXT) &= 0xFFFFFFFE; return 0x14; }
    if (pending & 0x2) { VUINT32(SB_IML6EXT) &= 0xFFFFFFFD; return 0x17; }
    if (pending & 0x4) { VUINT32(SB_IML6EXT) &= 0xFFFFFFFB; return 0x1A; }
    if (pending & 0x8) { VUINT32(SB_IML6EXT) &= 0xFFFFFFF7; return 0x1B; }
    return SYSINTR_NOP;
}

/* IRL4 / normal sources -> SYSINTR 0x10/0x12/0x15/0x18. */
ULONG KatanaISR4(void)
{
    DWORD pending = VUINT32(SB_ISTNRM) & VUINT32(SB_IML4NRM);
    if (pending & 0x00000FFF) { VUINT32(SB_IML4NRM) &= 0xFFC7F000; return 0x10; } /* bits 0-11 */
    if (pending & 0x00003000) { VUINT32(SB_IML4NRM) &= 0xFFFFCFFF; return 0x12; } /* bits 12-13 */
    if (pending & 0x00004000) { VUINT32(SB_IML4NRM) &= 0xFFFFBFFF; return 0x15; } /* bit 14 */
    if (pending & 0x00008000) { VUINT32(SB_IML4NRM) &= 0xFFFF7FFF; return 0x18; } /* bit 15 */
    return SYSINTR_NOP;
}

/* IRL2 / error sources -> SYSINTR 0x11/0x13/0x16/0x19. */
ULONG KatanaISR2(void)
{
    DWORD pending = VUINT32(SB_ISTERR) & VUINT32(SB_IML2ERR);
    if (pending & 0x000000FF) { VUINT32(SB_IML2ERR) &= 0xEFFFFF00; return 0x11; } /* bits 0-7 */
    if (pending & 0x00000F00) { VUINT32(SB_IML2ERR) &= 0xFFFFF0FF; return 0x13; } /* bits 8-11 */
    if (pending & 0x00007000) { VUINT32(SB_IML2ERR) &= 0xFFFF8FFF; return 0x16; } /* bits 12-14 */
    if (pending)              { VUINT32(SB_IML2ERR) &= 0xFFFF7FFF; return 0x19; }
    return SYSINTR_NOP;
}
