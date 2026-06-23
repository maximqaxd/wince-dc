/*
 * oemioctl.c - Dreamcast OAL IOCTL dispatch (was hal:oemioctl.obj).
 * The shipped OEMIoControl (0x8C03D2F4) dispatches HAL IOCTLs to WDM init,
 * interrupt status/include/exclude, platform version and SetRTC. For first boot
 * we answer nothing (return FALSE) to avoid pulling the WDM/interrupt-IOCTL
 * chain; wire the real HAL IOCTLs (and OEMInterruptStatus/Include/Exclude) later.
 * See OAL-NOTES.md.
 */
#include <windows.h>
#include "dc_hw.h"

BOOL OEMIoControl(DWORD dwIoControlCode, LPVOID pInBuf, DWORD nInBufSize,
                  LPVOID pOutBuf, DWORD nOutBufSize, LPDWORD pBytesReturned)
{
    /* TODO: handle IOCTL_HAL_REQUEST_SYSINTR / INTR status+include+exclude,
     * IOCTL_HAL_GET_DEVICEID, reboot, RTC. Minimal for bring-up. */
    if (pBytesReturned)
        *pBytesReturned = 0;
    return FALSE;
}
