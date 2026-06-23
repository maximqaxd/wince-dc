/*
 * rtc.c - Dreamcast OAL real-time clock.
 * From nknodbg.exe: OEMSetRealTime/OEMSetAlarmTime are `return 0`. OEMGetRealTime
 * reads the AICA RTC (0xA0710000/04 = seconds since 1950) and converts. For first
 * boot we return a fixed time; wiring the real RTC is a TODO. See OAL-NOTES.md.
 */
#include <windows.h>
#include "dc_hw.h"

/* DC AICA RTC: 32-bit seconds since 1950-01-01, split across two regs. */
#define DC_RTC_HI   0xA0710000
#define DC_RTC_LO   0xA0710004

int OEMGetRealTime(LPSYSTEMTIME pst)
{
    /* TODO: read DC_RTC_HI/LO (seconds since 1950) and convert to SYSTEMTIME.
     * For bring-up, hand back a fixed sane wall-clock so the kernel has a time. */
    if (pst) {
        pst->wYear = 2000; pst->wMonth = 1; pst->wDayOfWeek = 6; pst->wDay = 1;
        pst->wHour = 0; pst->wMinute = 0; pst->wSecond = 0; pst->wMilliseconds = 0;
    }
    return 1;
}

int OEMSetRealTime(LPSYSTEMTIME pst)   { return 0; }
int OEMSetAlarmTime(LPSYSTEMTIME pst)  { return 0; }
