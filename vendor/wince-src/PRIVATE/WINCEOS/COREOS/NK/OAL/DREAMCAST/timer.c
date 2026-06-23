/*
 * timer.c - Dreamcast OAL SH-4 TMU bring-up + system-tick ISR (was hal:ktimer.obj).
 *
 * RECONSTRUCTED from release\debug\nknodbg.exe (InitClock @ 0x8C03D0CC,
 * Timer0ISR @ 0x8C03DAA0). See OAL-NOTES.md.
 *
 * TMU0 = periodic system tick (25 ms/underflow). TMU1 = aux (programmed, not
 * started here). TMU2 = free-running down-counter for QueryPerformanceCounter.
 */
#include <windows.h>
#include "dc_hw.h"

#ifndef SYSINTR_RESCHED
#define SYSINTR_RESCHED   1
#endif

extern BOOL HookInterrupt(int idInt, FARPROC pfn);
extern void Timer0ISR(void);
extern void Timer1ISR(void);

/* OAL clock globals (the kernel reads these for tick/QPC scaling). */
DWORD g_TicksPerPeriod;          /* TMU0 reload = 312500 -> 25 ms at PCLK/4 (12.5 MHz) */
DWORD g_TicksTo64kMSec;          /* = 5 (scaling constant from the shipped OAL) */

/* Pointer to the kernel's tick counter pair (CurMSec, reschedule accumulator).
 * The shipped OAL kept this as an indirect pointer; bind it to KData in init.
 * TODO: resolve the exact KData field offsets and set this up. */
volatile DWORD *g_pSysTick;

void InitClock(void)
{
    g_TicksTo64kMSec = 5;
    g_TicksPerPeriod = 312500;

    /* --- TMU0: periodic system tick --- */
    VUINT8(SH4_TMU_TSTR)  &= (BYTE)~SH4_TSTR_STR0;       /* stop TMU0 */
    VUINT16(SH4_TMU_TCR0)  = SH4_TMU_TCR_UNIE;           /* 0x20: underflow int enable */
    VUINT32(SH4_TMU_TCOR0) = g_TicksPerPeriod;
    VUINT32(SH4_TMU_TCNT0) = g_TicksPerPeriod;
    VUINT16(SH4_INTC_IPRA) = (VUINT16(SH4_INTC_IPRA) & 0x0FFF) | 0x1000;  /* TMU0 prio */
    HookInterrupt(INTRVEC_TMU0, (FARPROC)Timer0ISR);

    /* --- TMU1: aux timer --- */
    VUINT8(SH4_TMU_TSTR)  &= (BYTE)~SH4_TSTR_STR1;       /* stop TMU1 */
    VUINT16(SH4_TMU_TCR1)  = 0;
    VUINT32(SH4_TMU_TCOR1) = 125000;
    VUINT32(SH4_TMU_TCNT1) = 125000;
    VUINT16(SH4_INTC_IPRA) = (VUINT16(SH4_INTC_IPRA) & 0xF0FF) | 0x0100;  /* TMU1 prio */
    HookInterrupt(INTRVEC_TMU1, (FARPROC)Timer1ISR);

    /* --- TMU2: free-running counter for QueryPerformanceCounter --- */
    VUINT8(SH4_TMU_TSTR)  &= (BYTE)~SH4_TSTR_STR2;       /* stop TMU2 */
    VUINT16(SH4_TMU_TCR2)  = 0x18;
    VUINT32(SH4_TMU_TCOR2) = 0xFFFFFFFF;

    /* start TMU0 + TMU2 */
    VUINT8(SH4_TMU_TSTR)  |= (SH4_TSTR_STR0 | SH4_TSTR_STR2);
}

/* TMU0 underflow -> system tick. Returns the SYSINTR the kernel should schedule. */
ULONG OEMTimer0ISR(void)
{
    USHORT tcr;

    /* Ack underflow: clear UNF (bit 8) in TCR0, poll until it stays clear. */
    tcr = VUINT16(SH4_TMU_TCR0);
    do {
        VUINT16(SH4_TMU_TCR0) = (USHORT)(tcr & 0x00FF);
        tcr = VUINT16(SH4_TMU_TCR0);
    } while (tcr & 0x0100);

    if (g_pSysTick) {
        g_pSysTick[0] += 25;     /* CurMSec += 25 ms */
        g_pSysTick[1] += 25;     /* reschedule accumulator */
    }
    return SYSINTR_RESCHED;
}
