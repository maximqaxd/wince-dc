/*
 * power.c - Dreamcast OAL idle / power-off / NMI.
 * From nknodbg.exe: OEMIdle logs "OEM idle"; OEMPowerOff logs then halts with
 * interrupts off. We keep idle a no-op (don't flood the serial each idle tick).
 */
#include <windows.h>
#include "dc_hw.h"

extern void OEMWriteDebugString(LPCWSTR psz);

/* Kernel idle hook. A real OAL would sleep the CPU (SH-4 'sleep') until the next
 * interrupt; for bring-up just return so the scheduler spins. */
void OEMIdle(DWORD dwIdleParam)
{
    /* TODO: lower power via 'sleep' once the tick/wake path is trusted. */
}

void OEMPowerOff(void)
{
    OEMWriteDebugString(L"OEM Power off.\r\n");
    /* TODO: mask interrupts (raise SR.IMASK / set BL) before halting. */
    for (;;)                        /* halt */
        ;
}

/* OEMNMI is imported by shexcept.src as a bare (no-underscore) asm symbol, so it
 * is defined in startup.src, not here. */
