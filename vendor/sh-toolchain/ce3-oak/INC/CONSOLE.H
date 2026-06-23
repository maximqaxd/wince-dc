/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

console.h

Abstract:  

Notes: 


--*/
//	@doc	EXTERNAL SERIALDEVICE

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

// We'll need some defines
#include "windev.h"

#ifdef __cplusplus
extern "C" {
#endif

// This IOCTL causes DEVICE.EXE to automatically deregister a device when the ref-count goes to zero.
#define IOCTL_DEVICE_AUTODEREGISTER		CTL_CODE(FILE_DEVICE_CONSOLE, 255, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// DeviceIOControl dwIoControlCode values for Console virtual devices
//
#define IOCTL_CONSOLE_SETMODE				CTL_CODE(FILE_DEVICE_CONSOLE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_GETMODE				CTL_CODE(FILE_DEVICE_CONSOLE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_SETTITLE				CTL_CODE(FILE_DEVICE_CONSOLE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_GETTITLE				CTL_CODE(FILE_DEVICE_CONSOLE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_CLS					CTL_CODE(FILE_DEVICE_CONSOLE, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_FLUSHINPUT			CTL_CODE(FILE_DEVICE_CONSOLE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_GETSCREENROWS			CTL_CODE(FILE_DEVICE_CONSOLE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_SETCONTROLCHANDLER	CTL_CODE(FILE_DEVICE_CONSOLE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONSOLE_GETSCREENCOLS			CTL_CODE(FILE_DEVICE_CONSOLE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

// flags for Mode
#define CECONSOLE_MODE_ECHO_INPUT		0x0001	// echo kbd input or not
#define CECONSOLE_MODE_LINE_INPUT 		0x0002	// input returned line or char at a time
#define CECONSOLE_MODE_PROCESSED_OUTPUT 0x0004	// \b\t\n etc. processed or output raw


#define USE_CONIOCTL_CALLS	DWORD __dwConIoctlRetSizeDummy__;
#define CONIOCTL_CALL(h, c, pIn, nIn, pOut, nOut) DeviceIoControl(h, c, pIn, nIn, pOut, nOut, &__dwConIoctlRetSizeDummy__, NULL)

#define CeClearConsoleScreen(h)       CONIOCTL_CALL(h, IOCTL_CONSOLE_CLS, NULL, 0, NULL, 0)
#define CeFlushConsoleInput(h)        CONIOCTL_CALL(h, IOCTL_CONSOLE_FLUSHINPUT, NULL, 0, NULL, 0)

#define CeSetConsoleMode(h, mode)     CONIOCTL_CALL(h, IOCTL_CONSOLE_SETMODE, (LPVOID)&mode, sizeof(DWORD), NULL, 0)
#define CeSetConsoleTitleA(h, psz)    CONIOCTL_CALL(h, IOCTL_CONSOLE_SETTITLE, (LPVOID)psz, strlen(psz), NULL, 0)

#define CeGetConsoleRows(h, prows)     CONIOCTL_CALL(h, IOCTL_CONSOLE_GETSCREENROWS, NULL, 0, (LPVOID)prows, sizeof(DWORD))
#define CeGetConsoleCols(h, prows)     CONIOCTL_CALL(h, IOCTL_CONSOLE_GETSCREENCOLS, NULL, 0, (LPVOID)prows, sizeof(DWORD))
#define CeGetConsoleMode(h, pmode)    CONIOCTL_CALL(h, IOCTL_CONSOLE_GETMODE, NULL, 0, (LPVOID)pmode, sizeof(DWORD))
#define CeGetConsoleTitleA(h, psz, n) CONIOCTL_CALL(h, IOCTL_CONSOLE_GETMODE, NULL, 0, (LPVOID)psz, n)
#define CeSetControlCHandler(h, pfn) CONIOCTL_CALL(h, IOCTL_CONSOLE_SETCONTROLCHANDLER, (LPVOID)pfn, sizeof(DWORD), NULL, 0)
typedef int (*PFN_CONSOLE_CONTROLCHANDLER)(void);


#ifdef __cplusplus
}
#endif


#endif //!__CONSOLE_H__
