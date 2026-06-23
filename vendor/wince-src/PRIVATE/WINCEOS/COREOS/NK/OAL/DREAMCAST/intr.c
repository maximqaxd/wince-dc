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
/* Holly IRL demuxers. Each returns the SYSINTR for the top pending source */
/* on its level, or SYSINTR_NOP if none. (hooked in OEMInit.)              */
/*                                                                          */
/* TODO: the exact SB_IST* bit -> SYSINTR mapping comes from the per-level  */
/* IsrConstants table in the image (status latch + per-bit SYSINTR + ack    */
/* mask). The returns below match the shipped kernel's SYSINTR set; the     */
/* precise bit positions still need the IsrConstants struct decode          */
/* (see OAL-NOTES.md "TODO"). Each device's bit is acked by writing the      */
/* masked status back to SB_IST*.                                           */
/* ====================================================================== */

/* IRL level 6 (highest) -> SYSINTR 0x14/0x17/0x1A/0x1B (low 4 status bits). */
ULONG KatanaISR6(void)
{
    DWORD status  = VUINT32(SB_ISTEXT);
    DWORD pending = status & VUINT32(SB_IML6EXT);
    static const ULONG sysintr[4] = { 0x14, 0x17, 0x1A, 0x1B };
    int b;
    for (b = 0; b < 4; b++) {
        if (pending & (1u << b)) {
            VUINT32(SB_ISTEXT) = (1u << b);     /* W1C ack */
            return sysintr[b];
        }
    }
    return SYSINTR_NOP;
}

/* IRL level 4 -> SYSINTR 0x10/0x12/0x15/0x18. */
ULONG KatanaISR4(void)
{
    DWORD status  = VUINT32(SB_ISTNRM);
    DWORD pending = status & VUINT32(SB_IML4NRM);
    /* TODO: decode pending -> {0x10,0x12,0x15,0x18} per IsrConstants[level4]. */
    if (pending) {
        /* ack + return mapped SYSINTR; placeholder until bit map is decoded */
        return SYSINTR_FIRMWARE;
    }
    return SYSINTR_NOP;
}

/* IRL level 2 (lowest) -> SYSINTR 0x11/0x13/0x16/0x19. */
ULONG KatanaISR2(void)
{
    DWORD status  = VUINT32(SB_ISTNRM);
    DWORD pending = status & VUINT32(SB_IML2NRM);
    /* TODO: decode pending -> {0x11,0x13,0x16,0x19} per IsrConstants[level2]. */
    if (pending) {
        return 0x11;
    }
    return SYSINTR_NOP;
}
