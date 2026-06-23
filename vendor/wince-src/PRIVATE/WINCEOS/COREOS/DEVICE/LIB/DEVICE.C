/* File:    device.c
 *
 * Purpose: WinCE device manager
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 */

//
// This module contains these functions:
//  FS_RegisterDevice
//  RegisterDeviceEx
//  DoFreeFSD
//  DoFreeDevice
//  WaitForFSDs
//  NotifyFSDs
//  IsValidDevice
//  FS_DeregisterDevice
//  FS_ActivateDevice
//  FindActiveByHandle
//  FS_DeactivateDevice
//  GetFSD
//  FormatDeviceName
//  LoadFSDThread
//  FS_LoadFSD
//  FS_LoadFSDEx
//  FS_CeResyncFilesys
//  FS_PowerAllDevices
//  FS_GetDeviceByIndex
//  FS_CreateDeviceHandle
//  FS_DevCloseFileHandle
//  FS_DevReadFile
//  FS_DevWriteFile
//  FS_DevSetFilePointer
//  FS_DevDeviceIoControl
//  FS_DevGetFileSize
//  FS_DevGetFileInformation
//  FS_DevFlushFileBuffers
//  FS_DevGetFileTime
//  FS_DevSetFileTime
//  FS_DevSetEndOfFile
//  FS_DevProcNotify
//  FS_CloseAllDeviceHandles
//  WinMain
//

#include <device.h>
#include <devload.h>
#include <devloadp.h>
#include <cardserv.h>
#include <console.h>

#ifdef TARGET_NT
#include <devemul.h>
#include "proxy.h"
#endif

#ifdef INSTRUM_DEV
#include <instrumd.h>
#else
#define ENTER_INSTRUM
#define EXIT_INSTRUM_INIT
#define EXIT_INSTRUM_DEINIT
#define EXIT_INSTRUM_POWERDOWN
#define EXIT_INSTRUM_POWERUP
#define EXIT_INSTRUM_OPEN
#define EXIT_INSTRUM_CLOSE
#define EXIT_INSTRUM_READ
#define EXIT_INSTRUM_WRITE
#define EXIT_INSTRUM_SEEK
#define EXIT_INSTRUM_IOCONTROL
#endif // INSTRUM_DEV

/* Device driver implementation */

//      @doc    EXTERNAL OSDEVICE KERNEL

//      @module device.c - device driver support | Device driver support functions.

//      @topic WinCE Device Drivers | There are two types of device drivers.  The
//      first type are specialized drivers.  These are keyboard, mouse/touch screen,
//      and display drivers.  These drivers have specialized interfaces.  The other
//      type of driver is the stream based device driver model.  Drivers following the
//      stream based model will need to export several functions and be initialized
//      before they can be used.  They can be accessed through standard file APIs such
//      as OpenFile.  The driver dll must export a set of entrypoints of the form
//      ttt_api, where ttt is the device id passed in lpszType to RegisterDevice, and
//      api is a required or optional API.  The APIs required are: Init, Deinit, Open,
//      Close, Read, Write, Seek, and IOCtl.
//  Drivers which need interrupts can use kernel exported interrupt support 
//  routines. To understand the interrupt model and functions please look at
//  <l Interrupt Support.Kernel Interrupt Support>
//      @xref <f RegisterDevice> <f DeregisterDevice> 
//         <l Interrupt Support.Kernel Interrupt Support>

//	notes on handling of unloading of dlls while threads are in those dlls:
//	1> routines which take the device critical section before calling into a dll do not need to
//		adjust the reference count since the deregister call takes the critical section
//	2> routines which do not take the device critical section do the following:
//		a> check if the device is still valid [ if (fsodev->lpDev) ]
//		b> if so, increment the reference count, so dodec = 1, make call, decrement the reference count,
//			set dodec = 0
//		c> we assume we will not fault in the interlockedDecrement, since once we've succeeded in the
//			increment, we know we won't fault on the decrement since nobody will be LocalFree'ing the device
//		d> if we fault inside the actual function call, we'll do the decrement due to the dodec flag
// 	3> We signal an event when we put someone on the DyingDevs list.  We can just poll from that point onwards
//		since (1) it's a rare case and (2) it's cheaper than always doing a SetEvent if you decrement the
//		reference count to 0.

CRITICAL_SECTION g_devcs;
LIST_ENTRY g_DevChain;
LIST_ENTRY g_DyingDevs;
fsopendev_t *g_lpOpenDevs;
fsopendev_t *g_lpDyingOpens;
HANDLE g_hDevApiHandle, g_hDevFileApiHandle, g_hCleanEvt, g_hCleanDoneEvt;
fsd_t *g_lpFSDChain;

//
// Devload.c and pcmcia.c 
//
typedef DWORD (*SYSTEMPOWER)(DWORD);
extern SYSTEMPOWER pfnSystemPower;
extern void DevloadInit(void);
extern void DevloadPostInit(void);
extern BOOL I_DeactivateDevice(HANDLE hDevice, LPWSTR ActivePath);
extern HANDLE StartOneDriver(LPTSTR RegPath, LPTSTR PnpId,
                  DWORD LoadOrder, DWORD ClientInfo, CARD_SOCKET_HANDLE hSock);

#ifndef TARGET_NT
const PFNVOID FDevApiMethods[] = {
	(PFNVOID)FS_DevProcNotify,
	(PFNVOID)0,
	(PFNVOID)FS_RegisterDevice,
	(PFNVOID)FS_DeregisterDevice,
	(PFNVOID)FS_CloseAllDeviceHandles,
	(PFNVOID)FS_CreateDeviceHandle,
	(PFNVOID)FS_LoadFSD,
	(PFNVOID)FS_ActivateDevice,
	(PFNVOID)FS_DeactivateDevice,
	(PFNVOID)FS_LoadFSDEx,
	(PFNVOID)FS_GetDeviceByIndex,
	(PFNVOID)FS_CeResyncFilesys,
};

#define NUM_FDEV_APIS (sizeof(FDevApiMethods)/sizeof(FDevApiMethods[0]))

const DWORD FDevApiSigs[NUM_FDEV_APIS] = {
	FNSIG3(DW, DW, DW),                     // ProcNotify
	FNSIG0(),
	FNSIG4(PTR, DW, PTR, DW),       // RegisterDevice
	FNSIG1(DW),                                     // DeregisterDevice
	FNSIG0(),                                       // CloseAllDeviceHandles
	FNSIG4(PTR, DW, DW, DW),            // CreateDeviceHandle
	FNSIG2(PTR, PTR),            // LoadFSD
	FNSIG2(PTR, DW),       // ActivateDevice
	FNSIG1(DW),            // DeactivateDevice
	FNSIG3(PTR, PTR, DW),	//LoadFSDEx
	FNSIG2(DW, PTR),		// GetDeviceByIndex
	FNSIG1(DW),	// CeResyncFilesys
};

const PFNVOID DevFileApiMethods[] = {
    (PFNVOID)FS_DevCloseFileHandle,
    (PFNVOID)0,
    (PFNVOID)FS_DevReadFile,
    (PFNVOID)FS_DevWriteFile,
    (PFNVOID)FS_DevGetFileSize,
    (PFNVOID)FS_DevSetFilePointer,
    (PFNVOID)FS_DevGetFileInformationByHandle,
	(PFNVOID)FS_DevFlushFileBuffers,
	(PFNVOID)FS_DevGetFileTime,
	(PFNVOID)FS_DevSetFileTime,
	(PFNVOID)FS_DevSetEndOfFile,
	(PFNVOID)FS_DevDeviceIoControl,
};

#define NUM_FAPIS (sizeof(DevFileApiMethods)/sizeof(DevFileApiMethods[0]))

const DWORD DevFileApiSigs[NUM_FAPIS] = {
    FNSIG1(DW),                                 // CloseFileHandle
    FNSIG0(),
    FNSIG5(DW,PTR,DW,PTR,PTR),  // ReadFile
    FNSIG5(DW,PTR,DW,PTR,PTR),  // WriteFile
    FNSIG2(DW,PTR),                             // GetFileSize
    FNSIG4(DW,DW,PTR,DW),               // SetFilePointer
    FNSIG2(DW,PTR),                             // GetFileInformationByHandle
	FNSIG1(DW),                                     // FlushFileBuffers
	FNSIG4(DW,PTR,PTR,PTR),         // GetFileTime
	FNSIG4(DW,PTR,PTR,PTR),         // SetFileTime
	FNSIG1(DW),                                     // SetEndOfFile,
	FNSIG8(DW, DW, PTR, DW, PTR, DW, PTR, PTR), // DeviceIoControl
};
#endif // TARGET_NT

PFNVOID FS_GetProcAddr(LPCWSTR type, LPCWSTR lpszName, HINSTANCE hInst) {
	WCHAR fullname[32];
	memcpy(fullname,type,3*sizeof(WCHAR));
	fullname[3] = L'_';
	wcscpy(&fullname[4],lpszName);
	return (PFNVOID)GetProcAddress(hInst,fullname);
}

//      @func   HANDLE | RegisterDevice | Register a new device
//  @parm       LPCWSTR | lpszType | device id (SER, PAR, AUD, etc) - must be 3 characters long
//  @parm       DWORD | dwIndex | index between 0 and 9, ie: COM2 would be 2
//  @parm       LPCWSTR | lpszLib | dll containing driver code
//  @parm       DWORD | dwInfo | instance information
//      @rdesc  Returns a handle to a device, or 0 for failure.  This handle should
//                      only be passed to DeregisterDevice.
//      @comm   For stream based devices, the drivers will be .dll or .drv files.
//                      Each driver is initialized by
//                      a call to the RegisterDevice API (performed by the server process).
//                      The lpszLib parameter is what will be to open the device.  The
//                      lpszType is a three character string which is used to identify the 
//                      function entry points in the DLL, so that multiple devices can exist
//                      in one DLL.  The lpszLib is the name of the DLL which contains the
//                      entry points.  Finally, dwInfo is passed in to the init routine.  So,
//                      if there were two serial ports on a device, and comm.dll was the DLL
//                      implementing the serial driver, it would be likely that there would be
//                      the following init calls:<nl>
//                      <tab>h1 = RegisterDevice(L"COM", 1, L"comm.dll",0x3f8);<nl>
//                      <tab>h2 = RegisterDevice(L"COM", 2, L"comm.dll",0x378);<nl>
//                      Most likely, the server process will read this information out of a
//                      table or registry in order to initialize all devices at startup time.
//      @xref <f DeregisterDevice> <l Overview.WinCE Device Drivers>
//

HANDLE
FS_RegisterDevice(
    LPCWSTR lpszType,
    DWORD   dwIndex,
    LPCWSTR lpszLib,
    DWORD   dwInfo
    )
{
    return RegisterDeviceEx(
                lpszType,
                dwIndex,
                lpszLib,
                dwInfo,
                MAX_LOAD_ORDER+1
                );
}

HANDLE
RegisterDeviceEx(
    LPCWSTR lpszType,
    DWORD dwIndex,
    LPCWSTR lpszLib,
    DWORD dwInfo,
    DWORD dwLoadOrder
    )
{
	HANDLE hDev = 0;
	fsdev_t *lpdev = 0, *lpTrav;
	DWORD retval = 0;

	DEBUGMSG(ZONE_DYING, (TEXT("DEVICE: About to wait on CleanDoneEvt.\r\n")));

	// Need to wait for any filesystem devices to finish getting deregistered 
	// before we go ahead and register them again. This gets around problems
	// with storage card naming. 
	retval = WaitForSingleObject(g_hCleanDoneEvt, 5000);
	DEBUGMSG(ZONE_DYING, (TEXT("DEVICE: Got CleanDoneEvt.\r\n")));
	
	ASSERT(retval != WAIT_TIMEOUT);
	
	retval = ERROR_SUCCESS;	// Initialize for success

	if (dwIndex > 9) {
		retval = ERROR_INVALID_PARAMETER;
		goto errret;
	}
	if (!(lpdev = LocalAlloc(LPTR,sizeof(fsdev_t)))) {
		retval = ERROR_OUTOFMEMORY;
		goto errret;
	}
	
	__try {
        memset(lpdev, 0, sizeof(fsdev_t));
        memcpy(lpdev->type,lpszType,sizeof(lpdev->type));
		lpdev->index = dwIndex;
		if (!(lpdev->hLib = LoadDriver(lpszLib))) {
			retval = ERROR_FILE_NOT_FOUND;
		} else {
			if (!(lpdev->fnInit = (pInitFn)FS_GetProcAddr(lpdev->type,L"Init",lpdev->hLib)) ||
				!(lpdev->fnDeinit = (pDeinitFn)FS_GetProcAddr(lpdev->type,L"Deinit",lpdev->hLib)) ||
				!(lpdev->fnOpen = (pOpenFn)FS_GetProcAddr(lpdev->type,L"Open",lpdev->hLib)) ||
				!(lpdev->fnClose = (pCloseFn)FS_GetProcAddr(lpdev->type,L"Close",lpdev->hLib)) ||
				!(lpdev->fnRead = (pReadFn)FS_GetProcAddr(lpdev->type,L"Read",lpdev->hLib)) ||
				!(lpdev->fnWrite = (pWriteFn)FS_GetProcAddr(lpdev->type,L"Write",lpdev->hLib)) ||
				!(lpdev->fnSeek = (pSeekFn)FS_GetProcAddr(lpdev->type,L"Seek",lpdev->hLib)) ||
				!(lpdev->fnControl = (pControlFn)FS_GetProcAddr(lpdev->type,L"IOControl",lpdev->hLib))) {
				retval = ERROR_INVALID_FUNCTION;
			} else {
				// Got all the required functions.  Now let's get the optional ones
				lpdev->fnPowerup = (pPowerupFn)FS_GetProcAddr(lpdev->type,L"PowerUp",lpdev->hLib);
				lpdev->fnPowerdn = (pPowerupFn)FS_GetProcAddr(lpdev->type,L"PowerDown",lpdev->hLib);
			}
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		retval = ERROR_INVALID_PARAMETER;
	}

	if (retval) {
		goto errret;
	}

	// Now enter the critical section to look for it in the device list
	EnterCriticalSection (&g_devcs);
	__try {
		//
        // check for uniqueness
        //
        for (lpTrav = (fsdev_t *)g_DevChain.Flink;
             lpTrav != (fsdev_t *)&g_DevChain;
             lpTrav = (fsdev_t *)(lpTrav->list.Flink)) {
            if (!memcmp(lpTrav->type,lpdev->type,sizeof(lpdev->type)) &&
               (lpTrav->index == lpdev->index)) {
                retval = ERROR_DEVICE_IN_USE;
				break;
            }
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		retval = ERROR_INVALID_PARAMETER;
	}
	LeaveCriticalSection(&g_devcs);
	if (retval) {
		goto errret;
	}

	
	__try {
		ENTER_INSTRUM {
			lpdev->dwData = lpdev->fnInit(dwInfo);
		} EXIT_INSTRUM_INIT;

		if (!(lpdev->dwData)) {
			retval = ERROR_OPEN_FAILED;
		} else {
			// Sucess
			lpdev->PwrOn = TRUE;
			lpdev->dwLoadOrder = dwLoadOrder;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		retval = ERROR_INVALID_PARAMETER;
	}
	if (retval) {
		goto errret;
	}

	EnterCriticalSection(&g_devcs);
	__try {
        //
        // Insert according to LoadOrder
        //
        for (lpTrav = (fsdev_t *)g_DevChain.Flink;
             lpTrav != (fsdev_t *)&g_DevChain;
             lpTrav = (fsdev_t *)(lpTrav->list.Flink)) {
            if (lpTrav->dwLoadOrder >= dwLoadOrder) {
                InsertHeadList((PLIST_ENTRY)lpTrav, (PLIST_ENTRY)lpdev);
				break;
            }
        }
		if (lpTrav == (fsdev_t *)&g_DevChain) {
			// insert at the end
			InsertTailList(&g_DevChain, (PLIST_ENTRY)lpdev);
		}
        hDev = (HANDLE)lpdev;

	} __except (EXCEPTION_EXECUTE_HANDLER) {
		retval = ERROR_INVALID_PARAMETER;
	}
	LeaveCriticalSection(&g_devcs);

errret:
	
#ifdef DEBUG
    if (ZONE_ACTIVE) {
        if (hDev != NULL) {
            DEBUGMSG(ZONE_ACTIVE, (TEXT("DEVICE: Name   Load Order\r\n")));
            //
            // Display the list of devices in ascending load order.
            //
			EnterCriticalSection(&g_devcs);
            for (lpTrav = (fsdev_t *)g_DevChain.Flink;
                 lpTrav != (fsdev_t *)&g_DevChain;
                 lpTrav = (fsdev_t *)(lpTrav->list.Flink)) {
                DEBUGMSG(ZONE_ACTIVE, (TEXT("DEVICE: %c%c%c%d:  %d\r\n"),
                         lpTrav->type[0], lpTrav->type[1], lpTrav->type[2],
                         lpTrav->index, lpTrav->dwLoadOrder));
            }
			LeaveCriticalSection(&g_devcs);
        }
    }
#endif

	// If we failed then let's clean up the module and data
	if (retval) {
		SetLastError (retval);
		if (lpdev) {
			if (lpdev->hLib) {
				FreeLibrary(lpdev->hLib);
			}
			LocalFree (lpdev);
		}
	}

	return hDev;
}

void DoFreeFSD(pfsd_t pfsd)
{
    pfsd_t pfsd1;

    if (pfsd->cFSDDevices <= 0) {
        FreeLibrary(pfsd->hFSDLib);
        if (pfsd == g_lpFSDChain) {
            g_lpFSDChain = pfsd->next;
            LocalFree(pfsd);
            return;
        } else {
            pfsd1 = g_lpFSDChain;
            while (pfsd1) {
                if (pfsd1->next == pfsd) {
                    pfsd1->next = pfsd->next;
                    LocalFree(pfsd);
                    return;
                }
                pfsd1 = pfsd1->next;
            }
            // we should never get here
        }
    }
}

void DoFreeDevice(fsdev_t *lpdev)
{
    EnterCriticalSection(&g_devcs);
    if (lpdev->pfsd)
        DoFreeFSD(lpdev->pfsd);
    LeaveCriticalSection(&g_devcs);
    FreeLibrary(lpdev->hLib);
    LocalFree(lpdev);
}

void WaitForFSDs()
{
	fsdev_t *lpdev;

restart:
    EnterCriticalSection(&g_devcs);
    for (lpdev = (fsdev_t *)g_DevChain.Flink;
         lpdev != (fsdev_t *)&g_DevChain;
         lpdev = (fsdev_t *)lpdev->list.Flink) {

        // If the FSD hasn't finished init'ing yet, we must wait...
        if (lpdev->wFlags & DEVFLAGS_FSD_NEEDTOWAIT) {
            DEBUGMSG(1/*ZONE_FSD*/,(TEXT("DEVICE!WaitForFSDs: waiting for FSD to finish init'ing device 0x%x first...\r\n"), lpdev));
            LeaveCriticalSection(&g_devcs);
            Sleep(2000);
            goto restart;
        }
    }
    LeaveCriticalSection(&g_devcs);
}

void NotifyFSDs()
{
    int n;
    pfsd_t pfsd = g_lpFSDChain;
    pfsd_t pfsd1;

    EnterCriticalSection(&g_devcs);
    while (pfsd) {
        if (pfsd->pfnFSDDeinit && (n = pfsd->pfnFSDDeinit(0)) ||
            pfsd->pfnFSDDeinitEx && (n = pfsd->pfnFSDDeinitEx(0))) {
            DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!NotifyFSDs: FSD unmounted %d devices (out of %d)\n"), n, pfsd->cFSDDevices));
            pfsd->cFSDDevices -= n;
            DEBUGMSG(pfsd->cFSDDevices < 0,(TEXT("DEVICE!NotifyFSDs: device count underflow! (%d)\n"), pfsd->cFSDDevices));
            pfsd1 = pfsd;
            pfsd = pfsd->next;
            DoFreeFSD(pfsd1);
        } else {
            pfsd = pfsd->next;
        }
    }
    LeaveCriticalSection(&g_devcs);
}

//
// IsValidDevice - function to check if passed in device handle is actually a
// valid registered device.
//
// Return TRUE if it is a valid registered device, FALSE otherwise.
//
BOOL
IsValidDevice(
    fsdev_t * lpdev
    )
{
    fsdev_t * lpTrav;

    for (lpTrav = (fsdev_t *)g_DevChain.Flink;
         lpTrav != (fsdev_t *)&g_DevChain;
         lpTrav = (fsdev_t *)(lpTrav->list.Flink)) {
        if (lpTrav == lpdev) {
            return TRUE;
        }
    }
    return FALSE;
}

//
// Function to wait for the file system driver associated with the specified
// device to finish initializing before deregistering the device.
// (For instance when FATFS has a dialog up and the corresponding PC card is
// removed, the PCMCIA callback thread would get blocked until the dialog
// got dismissed. Now we spin up a thread so PCMCIA messages are not blocked.)
//
DWORD
WaitForFSDInitThread(
    HANDLE hDevice
    )
{
	fsdev_t *lpdev = (fsdev_t *)hDevice;
	
    while (1) {

        EnterCriticalSection(&g_devcs);
        if (!(lpdev->wFlags & DEVFLAGS_FSD_NEEDTOWAIT)) {
        	break;
        }
        LeaveCriticalSection(&g_devcs);
        DEBUGMSG(1/*ZONE_FSD*/,
            (TEXT("DEVICE!WaitForFSDInitThread: waiting for FSD to finish init'ing device 0x%x first...\n"), lpdev));
		Sleep(2000);
    }
    LeaveCriticalSection(&g_devcs);
    FS_DeregisterDevice(hDevice);
    return 0;
}

//      @func   BOOL | DeregisterDevice | Deregister a registered device
//  @parm       HANDLE | hDevice | handle to registered device, from RegisterDevice
//      @rdesc  Returns TRUE for success, FALSE for failure
//      @comm   DeregisterDevice can be used if a device is removed from the system or
//                      is being shut down.  An example would be:<nl>
//                      <tab>DeregisterDevice(h1);<nl>
//                      where h1 was returned from a call to RegisterDevice.
//      @xref <f RegisterDevice> <l Overview.WinCE Device Drivers>

BOOL FS_DeregisterDevice(HANDLE hDevice)
{
	fsdev_t *lpdev;
	fsopendev_t *lpopen;    
	BOOL retval = FALSE;
    HANDLE hThd;

restart:
    ENTER_DEVICE_FUNCTION {
		lpdev = (fsdev_t *)hDevice;
        if (!IsValidDevice(lpdev)) {
            goto errret;
        }

        // If an FSD hasn't finished init'ing yet, we should wait,
        // because if it does eventually successfully init, then it
        // will need to be de-init'ed as well.

        if (lpdev->wFlags & DEVFLAGS_FSD_NEEDTOWAIT) {
            DEBUGMSG(1/*ZONE_FSD*/,(TEXT("DEVICE!FS_DeregisterDevice: waiting for FSD to finish init'ing device 0x%x first...\n"), lpdev));
            hThd = CreateThread(NULL, 0,
                             (LPTHREAD_START_ROUTINE)&WaitForFSDInitThread,
                             (LPVOID) hDevice, 0, NULL);
            if (hThd != NULL) {
                CloseHandle(hThd);
                retval = TRUE;
                goto errret;
            } else {
                //
                // If we can't start a thread, then revert to polling.
                //
                LeaveCriticalSection(&g_devcs);
                Sleep(2000);
                goto restart;
            }       
        }

		lpopen = g_lpOpenDevs;
		while (lpopen) {
			if (lpopen->lpDev == lpdev)
				lpopen->lpDev = 0;
			lpopen = lpopen->nextptr;
		}
        RemoveEntryList((PLIST_ENTRY)lpdev);

	    if (lpdev->dwFSDData && lpdev->pfsd) {
            pfsd_t pfsd = lpdev->pfsd;
            if (pfsd->pfnFSDDeinit &&
                pfsd->pfnFSDDeinit(lpdev->dwFSDData) ||
                pfsd->pfnFSDDeinitEx &&
                pfsd->pfnFSDDeinitEx(lpdev->dwFSDData) ||
                pfsd->pfnFSDDeinit == NULL && pfsd->pfnFSDDeinitEx == NULL) {
	      		pfsd->cFSDDevices--;
            }
        }

        LeaveCriticalSection(&g_devcs);
		ENTER_INSTRUM {
		lpdev->fnDeinit(lpdev->dwData);
		} EXIT_INSTRUM_DEINIT;

        if ((!lpdev->dwRefCnt) && (!lpdev->pfsd)) {
       		DoFreeDevice(lpdev);
            EnterCriticalSection(&g_devcs);
        } else {
            EnterCriticalSection(&g_devcs);
            InsertTailList(&g_DyingDevs, (PLIST_ENTRY)lpdev);
            DEBUGMSG(ZONE_DYING, (TEXT("DEVICE: About to Reset CleanDoneEvt.\r\n")));

            ResetEvent(g_hCleanDoneEvt);
			SetEvent(g_hCleanEvt);
		}
		retval = TRUE;
errret:
        ;
	} EXIT_DEVICE_FUNCTION;
	return retval;
}


//
//  @func   HANDLE | ActivateDevice | Register a device and add it to the active list under HKEY_LOCAL_MACHINE\Drivers\Active.
//  @parm   LPCWSTR | lpszDevKey | The driver's device key under HKEY_LOCAL_MACHINE\Drivers (for example "Drivers\PCMCIA\Modem")
//  @parm   DWORD | dwClientInfo | Instance information to be stored in the device's active key.
//  @rdesc  Returns a handle to a device, or 0 for failure.  This handle should
//          only be passed to DeactivateDevice
//  @comm   ActivateDevice will RegisterDevice the specified device and
//  create an active key in HKEY_LOCAL_MACHINE\Drivers\Active for the new device.
//  Also the device's creation is announced to the system via a
//  WM_DEVICECHANGE message and through the application notification system.
//
//  An example would be:<nl>
//  <tab>hnd = ActivateDevice(lpszDevKey, dwClientInfo);<nl>
//  where hnd is a HANDLE that can be used for DeactivateDevice<nl>
//  lpszDevKey is the registry path in HKEY_LOCAL_MACHINE to the device
//    driver's device key which has values that specify the driver name and
//    device prefix.<nl>
//  dwClientInfo is a DWORD value that will be stored in the device's active
//    key.<nl>
//  @xref <f DeactivateDevice> <f RegisterDevice> <f DeregisterDevice> <l Overview.WinCE Device Drivers>
//

HANDLE FS_ActivateDevice(
    LPCWSTR lpszDevKey,
    DWORD dwClientInfo
    )
{
    CARD_SOCKET_HANDLE hSock;

    DEBUGMSG(1, (TEXT("DEVICE!ActivateDevice(%s, %d) entered\r\n"),
        lpszDevKey, dwClientInfo));
    hSock.uSocket = 0xff;  // Not a PCMCIA device.
    return StartOneDriver(
               (LPWSTR)lpszDevKey,
               NULL,       // PnpID
               MAX_LOAD_ORDER,
               dwClientInfo,
               hSock);
}


//
// Function to search through HLM\Drivers\Active for a matching device handle.
// Return TRUE if the device is found. The device's active registry path will be
// copied to the ActivePath parameter.
// 
BOOL
FindActiveByHandle(
    HANDLE hDevice,
    LPTSTR ActivePath
    )
{
    DWORD Handle;
    DWORD RegEnum;
    DWORD status;
    DWORD ValLen;
    DWORD ValType;
    LPTSTR RegPathPtr;
    HKEY DevKey;
    HKEY ActiveKey;
    TCHAR DevNum[DEVNAME_LEN];

    _tcscpy(ActivePath, s_ActiveKey);
    status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    ActivePath,
                    0,
                    0,
                    &ActiveKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!FindActiveByHandle: RegOpenKeyEx(%s) returned %d\r\n"),
            ActivePath, status));
        return FALSE;
    }

    RegEnum = 0;
    while (1) {
        ValLen = sizeof(DevNum)/sizeof(TCHAR);
        status = RegEnumKeyEx(
                    ActiveKey,
                    RegEnum,
                    DevNum,
                    &ValLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!FindActiveByHandle: RegEnumKeyEx() returned %d\r\n"),
                status));
            RegCloseKey(ActiveKey);
            return FALSE;
        }

        status = RegOpenKeyEx(
                    ActiveKey,
                    DevNum,
                    0,
                    0,
                    &DevKey);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!FindActiveByHandle: RegOpenKeyEx(%s) returned %d\r\n"),
                DevNum, status));
            goto fad_next;
        }

        //
        // See if the handle value matches
        //
        ValLen = sizeof(Handle);
        status = RegQueryValueEx(
                    DevKey,
                    DEVLOAD_HANDLE_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)&Handle,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!FindActiveByHandle: RegQueryValueEx(%s\\Handle) returned %d\r\n"),
                DevNum, status));
            //
            // We found an active device key with no handle value.
            //
            Handle = 0;
            goto fad_next;
        }

        if (Handle == (DWORD)hDevice) {
            //
            // Format the registry path
            //
            RegPathPtr = ActivePath + _tcslen(ActivePath);
            RegPathPtr[0] = (TCHAR)'\\';
            RegPathPtr++;
            _tcscpy(RegPathPtr, DevNum);
            RegCloseKey(ActiveKey);
            RegCloseKey(DevKey);
            return TRUE;
        }


fad_next:
        RegCloseKey(DevKey);
        RegEnum++;
    }   // while

}   // FindActiveByHandle


//
//  @func   BOOL | DeactivateDevice | Deregister a registered device and remove
//                                    it from the active list.
//  @parm   HANDLE | hDevice | handle to registered device, from ActivateDevice
//  @rdesc  Returns TRUE for success, FALSE for failure
//  @comm   DeactivateDevice will DeregisterDevice the specified device and
//  delete the device's active key from HKEY_LOCAL_MACHINE\Drivers\Active.
//  Also the device's removal is announced to the system via a
//  WM_DEVICECHANGE message and through the application notification system.
//          An example would be:<nl>
//                      <tab>DeactivateDevice(h1);<nl>
//                      where h1 was returned from a call to ActivateDevice.
//      @xref <f ActivateDevice> <f RegisterDevice> <f DeregisterDevice> <l Overview.WinCE Device Drivers>
//

BOOL FS_DeactivateDevice(HANDLE hDevice)
{
    TCHAR ActivePath[REG_PATH_LEN];

    if (FindActiveByHandle(hDevice, ActivePath)) {
        return I_DeactivateDevice(hDevice, ActivePath);
    }
    return FALSE;
}

//
// Find a currently loaded FSD or load it
//
pfsd_t GetFSD(LPCWSTR lpFSDName, BOOL fInitEx)
{
    pfsd_t pfsd;
    HMODULE hFSDLib;

    hFSDLib = LoadDriver(lpFSDName);
    if (hFSDLib == NULL) {
        DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!GetFSD: LoadDriver(%s) failed %d\n"), lpFSDName, GetLastError()));
        return NULL;
    }

    //
    // If this FSD sports the new "FSD_MountDisk" interface,
    // then we want to load FSDMGR.DLL instead, and give FSDMGR.DLL
    // our handle to the FSD.
    //
    if (!fInitEx && GetProcAddress(hFSDLib, L"FSD_MountDisk")) {
        pfsd = GetFSD(L"FSDMGR.DLL", TRUE); 
        if (pfsd) {
            pfsd->pfnFSDInitEx = 
              (pInitExFn)GetProcAddress(pfsd->hFSDLib, L"FSDMGR_InitEx");
            pfsd->pfnFSDDeinitEx = 
              (pDeinitFn)GetProcAddress(pfsd->hFSDLib, L"FSDMGR_DeinitEx");
            if (pfsd->pfnFSDInitEx && pfsd->pfnFSDDeinitEx)
                pfsd->hFSDLibEx = hFSDLib;
            else
                FreeLibrary(hFSDLib);
            return pfsd;
        }
        // If GetFSD(FSDMGR.DLL) failed for some reason, then the code below
        // will simply try to treat hFSDLib as an old-model FSD.  Probably won't
        // work, unless someone really did write an FSD that supports both models.
    }

    //
    // If this file system driver is already loaded, then return a pointer to
    // its fsd struct.
    //
    pfsd = g_lpFSDChain;
    while (pfsd) {
        if (pfsd->hFSDLib == hFSDLib) {
            FreeLibrary(hFSDLib);       // only keep one instance

            







            // Until this problem gets fixed in the loader, we can just "predict"
            // when this will happen and do an extra FreeLibrary on FSDMGR.DLL. -JTP

            //if (fInitEx && pfsd->hFSDLibEx)
            //    FreeLibrary(hFSDLib);   // perform an "extra" FreeLibrary

            DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!GetFSD found existing instance (%x)\n"), pfsd));
            return pfsd;
        }
        pfsd = pfsd->next;
    }

    //
    // Remember this newly loaded file system driver dll
    //
    pfsd = LocalAlloc(LPTR, sizeof(fsd_t));
    if (pfsd) {
        pfsd->hFSDLib = hFSDLib;
        if (!fInitEx) {
            pfsd->pfnFSDInit =
                (pInitFn)GetProcAddress(pfsd->hFSDLib, L"FSD_Init");
            pfsd->pfnFSDDeinit =
                (pDeinitFn)GetProcAddress(pfsd->hFSDLib, L"FSD_Deinit");
        }
        //
        // Link it into the global list
        //
        pfsd->next = g_lpFSDChain;
        g_lpFSDChain = pfsd;
    }
    else {
        FreeLibrary(hFSDLib);   // if we don't have enough memory, free the library
        DEBUGMSG(TRUE,(TEXT("DEVICE!GetFSD freed 0x%08x (ran out of memory)\n"), hFSDLib));
    }

    DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!GetFSD created new instance (%x)\n"), pfsd));
    return pfsd;
}


#define DEVICE_NAME_SIZE 6  // i.e. "COM1:" (includes space for 0 terminator)

void FormatDeviceName(LPWSTR lpszName, fsdev_t * lpdev)
{
    memcpy(lpszName, lpdev->type, sizeof(lpdev->type));
    lpszName[sizeof(lpdev->type)/sizeof(WCHAR)+0] = (WCHAR)(L'0' + lpdev->index);
    lpszName[sizeof(lpdev->type)/sizeof(WCHAR)+1] = L':';
    lpszName[sizeof(lpdev->type)/sizeof(WCHAR)+2] = 0;
}

//
// Context structure FS_LoadFSD passes to LoadFSDThread
//
typedef struct _FSD_LOAD_CONTEXT {
    HANDLE hDevice;
    LPWSTR lpFSDName;
} FSD_LOAD_CONTEXT, * PFSD_LOAD_CONTEXT;

//
// LoadFSDThread - Function to perform the actual work of FS_LoadFSD in case
// FSD displays a dialog requiring a user response.
//
DWORD
LoadFSDThread(
    PFSD_LOAD_CONTEXT pContext
    )
{
	fsdev_t *lpdev;
    pfsd_t pfsd;
    lpdev = (fsdev_t *)pContext->hDevice;

    DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!LoadFSDThread(0x%x, %s)\n"), lpdev, pContext->lpFSDName));

    EnterCriticalSection(&g_devcs);
    //
    // Try to load the FSD DLL for this device now.
    //
    lpdev->pfsd = pfsd = GetFSD(pContext->lpFSDName, FALSE);
    if (pfsd) {
        WCHAR wsDev[DEVICE_NAME_SIZE];
        FormatDeviceName(wsDev, lpdev);

        // We're ready to call the FSD now, and we want to release device.exe's
        // global critical section before doing so, but lest another driver using
        // the same FSD try to unload in the meantime, we must pre-increment
        // the FSD's ref count (cFSDDevices) beforehand, to prevent the FSD getting
        // unloaded out from under us.

        pfsd->cFSDDevices++;
        LeaveCriticalSection(&g_devcs);
        if (pfsd->pfnFSDInit)
            lpdev->dwFSDData = pfsd->pfnFSDInit((DWORD)wsDev);
        else if (pfsd->pfnFSDInitEx)
            lpdev->dwFSDData = pfsd->pfnFSDInitEx(pfsd->hFSDLibEx, wsDev, NULL);
        EnterCriticalSection(&g_devcs);

        // dwFSDData will be zero if the mount failed, and it will be
        // ODD if pfnFSDInit simply remounted an existing volume, so in both those
        // cases, we must back off the increment we applied to the FSD's ref count above.

        if (lpdev->dwFSDData == 0 || (lpdev->dwFSDData & 0x1))
            pfsd->cFSDDevices--;    // either the mount failed or it was actually a remount
        
        if (lpdev->dwFSDData == 0) {
            DoFreeFSD(pfsd);
            lpdev->pfsd = pfsd = NULL;
        }
    }

    // Clear DEVFLAGS_FSD_NEEDTOWAIT before releasing g_devcs to avoid unnecessary context switching
    lpdev->wFlags &= ~DEVFLAGS_FSD_NEEDTOWAIT;
    LeaveCriticalSection(&g_devcs);

    LocalFree(pContext->lpFSDName);
    LocalFree(pContext);
    DEBUGMSG(ZONE_FSD,(TEXT("DEVICE!LoadFSDThread: done\n")));
    return 0;
}

//
//  @func   BOOL | LoadFSD | Load a file system driver
//  @parm   HANDLE | hDevice | handle to registered device, from RegisterDevice
//  @parm   LPCWSTR | lpFSDName | Name of file system driver DLL to load
//  @rdesc  Returns TRUE for success, FALSE for failure
//  @comm   LoadFSD is used by a device driver to load its associated file
//                  system driver (e.g. ATADISK.DLL loads FATFS.DLL)
//                  An example would be:<nl>
//                      <tab>LoadFSD(h1, FSDName);<nl>
//                      where h1 is the handle returned by RegisterDevice
//                      and FSDName points to the name of the FSD DLL.
//
BOOL FS_LoadFSD(HANDLE hDevice, LPCWSTR lpFSDName)
{
    return FS_LoadFSDEx(hDevice, lpFSDName, LOADFSD_ASYNCH);
}


//
//  @func   BOOL | LoadFSDEx | Load a file system driver synch or asynch
//  @parm   HANDLE | hDevice | handle to registered device, from RegisterDevice
//  @parm   LPCWSTR | lpFSDName | Name of file system driver DLL to load
//  @parm	DWORD | dwFlag | indicates synchronous or asynchronous load of file system 
//  @rdesc  Returns TRUE for success, FALSE for failure
//  @comm   LoadFSDEx is used by a device driver to load its associated file
//                  system driver (e.g. ATADISK.DLL loads FATFS.DLL)
//                  An example would be:<nl>
//                      <tab>LoadFSDEx(h1, FSDName, Flag);<nl>
//                      where h1 is the handle returned by RegisterDevice
//                      and FSDName points to the name of the FSD DLL.
//						and Flag is one of the following:<nl>
//						<tab>LOADFSD_ASYNCH creates a thread to load the requested DLL<nl>
//						<tab>LOADFSD_SYNCH waits for the requested DLL to load before continuing<nl>
//
BOOL FS_LoadFSDEx(HANDLE hDevice, LPCWSTR lpFSDName, DWORD dwFlag)
{
    HANDLE hThd;
    PFSD_LOAD_CONTEXT pContext;
    fsdev_t *lpdev = (fsdev_t *)hDevice;

    pContext = LocalAlloc(LPTR, sizeof(FSD_LOAD_CONTEXT));
    if (pContext == NULL) {
        return FALSE;
    }

    pContext->lpFSDName = LocalAlloc(LPTR,
                                   (_tcslen(lpFSDName) + 1) * sizeof(TCHAR));
    if (pContext->lpFSDName == NULL) {
        LocalFree(pContext);
        return FALSE;
    }

    wcscpy(pContext->lpFSDName, lpFSDName);
    pContext->hDevice = hDevice;
    lpdev->wFlags |= DEVFLAGS_FSD_NEEDTOWAIT;

    switch(dwFlag)
    	{
    	case LOADFSD_SYNCH:
    		{
    		LoadFSDThread(pContext);
			hThd = NULL;
			return TRUE;
			break;
    		}
    	case LOADFSD_ASYNCH:
    	default:
    		{

		    hThd = CreateThread(NULL, 0,
                     (LPTHREAD_START_ROUTINE)&LoadFSDThread,
                     (LPVOID) pContext, 0, NULL);
    		if (hThd != NULL) 
    			{
        		CloseHandle(hThd);
        		return TRUE;
    			}
    		break;
    		}
    	}

    lpdev->wFlags &= ~DEVFLAGS_FSD_NEEDTOWAIT;
    LocalFree(pContext->lpFSDName);
    LocalFree(pContext);
    return FALSE;
}


//
//  @func   BOOL | CeResyncFilesys | Cause a file system driver to remount a device because of media change.
//  @parm   HANDLE | hDevice | handle to registered device from RegisterDevice
//  @rdesc  Returns TRUE for success, FALSE for failure
//  @comm   CeResyncFilesys is used by a device driver to indicate to its associated file
//                  system driver that a new volume/media has been inserted. The handle can be retrieved from
//                  the device's active key in the registry or from the p_hDevice field of the POST_INIT_BUF
//                  structure passed to the post-initialization IOCTL.
//                  
//
BOOL FS_CeResyncFilesys(HANDLE hDevice)
{
    fsdev_t *lpdev;
    pfsd_t pfsd;
    WCHAR wsDev[DEVICE_NAME_SIZE];
    BOOL bDeinit;

    lpdev = (fsdev_t *)hDevice;
    if (!IsValidDevice(lpdev)) {
        DEBUGMSG(ZONE_ERROR,(L"DEVICE: CeResyncFilesys - invalid handle\n"));
        return FALSE;   // Invalid device handle
    }

    pfsd = lpdev->pfsd;
    if (pfsd == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"DEVICE: CeResyncFilesys - No associated FSD\n"));
        return FALSE;   // No associated FSD
    }

    InterlockedIncrement(&lpdev->dwRefCnt);

    bDeinit = FALSE;
    if (pfsd->pfnFSDDeinit)
        bDeinit = pfsd->pfnFSDDeinit(lpdev->dwFSDData);
    else if (pfsd->pfnFSDDeinitEx)
        bDeinit = pfsd->pfnFSDDeinitEx(lpdev->dwFSDData);
    else{ 
        DEBUGMSG(ZONE_ERROR, (L"DEVICE: CeResyncFilesys - No FSDDeinit\n"));
        goto fcrf_fail;
    }

    FormatDeviceName(wsDev, lpdev);

    if (pfsd->pfnFSDInit)
        lpdev->dwFSDData = pfsd->pfnFSDInit((DWORD)wsDev);
    else if (pfsd->pfnFSDInitEx)
        lpdev->dwFSDData = pfsd->pfnFSDInitEx(pfsd->hFSDLibEx, wsDev, NULL);
    else {
        DEBUGMSG(ZONE_ERROR, (L"DEVICE: CeResyncFilesys - No FSDInit\n"));
        goto fcrf_fail;
    }

    InterlockedDecrement(&lpdev->dwRefCnt);
    return TRUE;

fcrf_fail:
    if (bDeinit)
        lpdev->pfsd->cFSDDevices--;
    InterlockedDecrement(&lpdev->dwRefCnt);
    return FALSE;

}


void FS_PowerAllDevices(BOOL bOff) {
	fsdev_t *lpdev;

    //
    // Power on the PCMCIA system before powering on devices
    //
    if (!bOff && pfnSystemPower)
	    pfnSystemPower(bOff);

    if (bOff) {
        //
        // Notify drivers of power off in reverse order from their LoadOrder
        //
        for (lpdev = (fsdev_t *)g_DevChain.Blink;
             lpdev != (fsdev_t *)&g_DevChain;
             lpdev = (fsdev_t *)lpdev->list.Blink) {
			if (lpdev->PwrOn) {
				if (lpdev->fnPowerdn) {
					ENTER_INSTRUM {
	                lpdev->fnPowerdn(lpdev->dwData);
					} EXIT_INSTRUM_POWERDOWN;
				}
				lpdev->PwrOn = FALSE;
			}
		}
	} else {
        //
        // Notify drivers of power on according to their LoadOrder
        //
        for (lpdev = (fsdev_t *)g_DevChain.Flink;
             lpdev != (fsdev_t *)&g_DevChain;
             lpdev = (fsdev_t *)lpdev->list.Flink) {
			if (!lpdev->PwrOn) {
				if (lpdev->fnPowerup) {
					ENTER_INSTRUM {
					lpdev->fnPowerup(lpdev->dwData);
					} EXIT_INSTRUM_POWERUP;
				}
				lpdev->PwrOn = TRUE;
			}
		}
	}

    //
    // Power off the PCMCIA system after powering off devices
    //
    if (bOff && pfnSystemPower)
	    pfnSystemPower(bOff);
}

BOOL FS_GetDeviceByIndex(DWORD dwIndex, LPWIN32_FIND_DATA lpFindFileData) {
	fsdev_t *lpdev;
	BOOL bRet = FALSE;
	ENTER_DEVICE_FUNCTION {
		lpdev = (fsdev_t *)g_DevChain.Flink;
		while (dwIndex && lpdev != (fsdev_t *)&g_DevChain) {
			dwIndex--;
			lpdev = (fsdev_t *)lpdev->list.Flink;
		}
		if (lpdev != (fsdev_t *)&g_DevChain) {
			lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		    *(__int64 *)&lpFindFileData->ftCreationTime = 0;
		    *(__int64 *)&lpFindFileData->ftLastAccessTime = 0;
		    *(__int64 *)&lpFindFileData->ftLastWriteTime = 0;
		    lpFindFileData->nFileSizeHigh = 0;
		    lpFindFileData->nFileSizeLow = 0;
		    lpFindFileData->dwOID = 0xffffffff;
            FormatDeviceName(lpFindFileData->cFileName, lpdev);
		    bRet = TRUE;
		}
	} EXIT_DEVICE_FUNCTION;
	return bRet;
}

// assumes len == 5 and lpnew[4] == ':'
HANDLE FS_CreateDeviceHandle(LPCWSTR lpNew, DWORD dwAccess, DWORD dwShareMode, HPROCESS hProc)
{
	HANDLE hDev = INVALID_HANDLE_VALUE;
    DWORD dwErrCode = ERROR_DEV_NOT_EXIST;
	fsopendev_t *lpopendev;
	fsdev_t *lpdev;
    
	ENTER_DEVICE_FUNCTION {
        for (lpdev = (fsdev_t *)g_DevChain.Flink;
             lpdev != (fsdev_t *)&g_DevChain;
             lpdev = (fsdev_t *)lpdev->list.Flink) {
	        if (!_wcsnicmp(lpNew,lpdev->type,
                     sizeof(lpdev->type)/sizeof(WCHAR))) {
				if ((DWORD)(lpNew[3]-L'0') == lpdev->index) {
					if (!(lpopendev = LocalAlloc(LPTR,sizeof(fsopendev_t)))) {
                        dwErrCode = ERROR_OUTOFMEMORY;
						goto errret;
					}
					lpopendev->lpDev = lpdev;
					lpopendev->lpdwDevRefCnt = &lpdev->dwRefCnt;
                    LeaveCriticalSection(&g_devcs);
					ENTER_INSTRUM {
					lpopendev->dwOpenData = lpdev->fnOpen(lpdev->dwData,dwAccess,dwShareMode);
					} EXIT_INSTRUM_OPEN;
                    EnterCriticalSection(&g_devcs);
					if ((!IsValidDevice(lpdev)) || (!lpopendev->dwOpenData)) {
						LocalFree(lpopendev);
                        // Don't set an error code, the driver should have done that.
						goto errret;
					}
					lpopendev->hProc = hProc;
					if (!(hDev = CreateAPIHandle(g_hDevFileApiHandle, lpopendev))) {
						hDev = INVALID_HANDLE_VALUE;
						dwErrCode = ERROR_OUTOFMEMORY;
						LocalFree(lpopendev);
						goto errret;
					}
                    // OK, we managed to create the handle
                    dwErrCode = 0;
					lpopendev->KHandle = hDev;
					lpopendev->nextptr = g_lpOpenDevs;
					g_lpOpenDevs = lpopendev;
					break;
				}
			}
		}
errret:
        if( dwErrCode ) {
            SetLastError( dwErrCode );
            // This debugmsg occurs too frequently on a system without a sound device (eg, CEPC)
            DEBUGMSG(0, (TEXT("ERROR (device.c:FS_CreateDeviceHandle): %s, %d\r\n"), lpNew, dwErrCode));
        }
        
	} EXIT_DEVICE_FUNCTION;
	return hDev;
}

BOOL FS_DevCloseFileHandle(fsopendev_t *fsodev)
{
	BOOL retval = FALSE;
	fsopendev_t *fsTrav;
    fsdev_t *lpdev;

	ENTER_DEVICE_FUNCTION {
		if (g_lpOpenDevs == fsodev)
			g_lpOpenDevs = fsodev->nextptr;
		else {
            fsTrav = g_lpOpenDevs;
            while (fsTrav != NULL && fsTrav->nextptr != fsodev) {
                fsTrav = fsTrav->nextptr;
            }
            if (fsTrav != NULL) {
                fsTrav->nextptr = fsodev->nextptr;
            } else {
                DEBUGMSG(1, (TEXT("WARNING (device.c): fsodev (0x%X) not in DEVICE.EXE list\r\n"), fsodev));
            }
		}
        if (lpdev = fsodev->lpDev) {
            LeaveCriticalSection(&g_devcs);
			ENTER_INSTRUM {
		    lpdev->fnClose(fsodev->dwOpenData);
			} EXIT_INSTRUM_CLOSE;
            EnterCriticalSection(&g_devcs);
    	}
        if (fsodev->dwOpenRefCnt) {
            fsodev->nextptr = g_lpDyingOpens;
            g_lpDyingOpens = fsodev;
            SetEvent(g_hCleanEvt);
        } else {
            LocalFree(fsodev);
        }
		retval = TRUE;
	} EXIT_DEVICE_FUNCTION;
	return retval;
}

void OpenAddRef(fsopendev_t *fsodev)
{
    InterlockedIncrement(&fsodev->dwOpenRefCnt);
    InterlockedIncrement(fsodev->lpdwDevRefCnt);
}

void OpenDelRef(fsopendev_t *fsodev)
{
    InterlockedDecrement(&fsodev->dwOpenRefCnt);
    InterlockedDecrement(fsodev->lpdwDevRefCnt);
}

BOOL FS_DevReadFile(fsopendev_t *fsodev, LPVOID buffer, DWORD nBytesToRead, LPDWORD lpNumBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	BOOL retval = FALSE;
	DWORD dodec = 0;
	__try {
		if (!fsodev->lpDev)
			SetLastError(ERROR_GEN_FAILURE);
		else {
			OpenAddRef(fsodev);
			dodec = 1;
			ENTER_INSTRUM {
			*lpNumBytesRead = fsodev->lpDev->fnRead(fsodev->dwOpenData,buffer,nBytesToRead);
			} EXIT_INSTRUM_READ;
			OpenDelRef(fsodev);
			dodec = 0;
			if (*lpNumBytesRead == 0xffffffff) {
				retval = FALSE;
				*lpNumBytesRead = 0;
			} else
				retval = TRUE;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		if (dodec)
			OpenDelRef(fsodev);
		SetLastError(ERROR_INVALID_PARAMETER);
	}
	return retval;
}

BOOL FS_DevWriteFile(fsopendev_t *fsodev, LPCVOID buffer, DWORD nBytesToWrite, LPDWORD lpNumBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	BOOL retval = FALSE;
	DWORD dodec = 0;
	__try {
		if (!fsodev->lpDev)
			SetLastError(ERROR_GEN_FAILURE);
		else {
			OpenAddRef(fsodev);
			dodec = 1;
			ENTER_INSTRUM {
			*lpNumBytesWritten = fsodev->lpDev->fnWrite(fsodev->dwOpenData,buffer,nBytesToWrite);
			} EXIT_INSTRUM_WRITE;
			OpenDelRef(fsodev);
			dodec = 0;
			if (*lpNumBytesWritten == 0xffffffff)
				*lpNumBytesWritten = 0;
			else
				retval = TRUE;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		if (dodec)
			OpenDelRef(fsodev);
		SetLastError(ERROR_INVALID_PARAMETER);
	}
	return retval;
}

DWORD FS_DevSetFilePointer(fsopendev_t *fsodev, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod) {
	DWORD retval = 0xffffffff;
	DWORD dodec = 0;
	__try {
		if (!fsodev->lpDev)
			SetLastError(ERROR_GEN_FAILURE);
		else {
			OpenAddRef(fsodev);
			dodec = 1;
			ENTER_INSTRUM {
			retval = fsodev->lpDev->fnSeek(fsodev->dwOpenData,lDistanceToMove,dwMoveMethod);
			} EXIT_INSTRUM_SEEK;
			OpenDelRef(fsodev);
			dodec = 0;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		if (dodec)
			OpenDelRef(fsodev);
		SetLastError(ERROR_INVALID_PARAMETER);
	}
	return retval;
}

BOOL FS_DevDeviceIoControl(fsopendev_t *fsodev, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    BOOL retval = FALSE;
    DWORD dodec = 0;
    __try {
        if (!fsodev->lpDev)
            SetLastError(ERROR_GEN_FAILURE);
        else {
            if(dwIoControlCode == IOCTL_DEVICE_AUTODEREGISTER) {
                // NOTE: This IOCTL is here to avoid a deadlock between (a) a DllMain calling
                // DeregisterDevice (and hence trying to acquire the device CS *after* the 
                // loader CS and (b) Various other paths in this file where we acquire the 
                // device CS *before* the Loader CS (by calling LoadLib, FreeLib etc). 
                // See WinCE#4165. Hence this code path must NOT acquire the device CS!!
                DEBUGMSG(ZONE_DYING, (L"FS_DevDeviceIoControl:IOCTL_DEVICE_AUTODEREGISTER. lpdev=%x\r\n", fsodev->lpDev));
                fsodev->lpDev->wFlags |= DEVFLAGS_AUTO_DEREGISTER;
                retval = TRUE;
                // signal the main device (dying-devs-handler) thread
                SetEvent(g_hCleanEvt);
            }
            else {
                OpenAddRef(fsodev);
                dodec = 1;
                ENTER_INSTRUM {
                    retval = fsodev->lpDev->fnControl(fsodev->dwOpenData,dwIoControlCode,lpInBuf,nInBufSize,lpOutBuf,nOutBufSize,lpBytesReturned);
                } EXIT_INSTRUM_IOCONTROL;
                OpenDelRef(fsodev);
                dodec = 0;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (dodec)
			OpenDelRef(fsodev);
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return retval;
}

DWORD FS_DevGetFileSize(fsopendev_t *fsodev, LPDWORD lpFileSizeHigh) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0xffffffff;
}

BOOL FS_DevGetFileInformationByHandle(fsopendev_t *fsodev, LPBY_HANDLE_FILE_INFORMATION lpFileInfo) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL FS_DevFlushFileBuffers(fsopendev_t *fsodev) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL FS_DevGetFileTime(fsopendev_t *fsodev, LPFILETIME lpCreation, LPFILETIME lpLastAccess, LPFILETIME lpLastWrite) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL FS_DevSetFileTime(fsopendev_t *fsodev, CONST FILETIME *lpCreation, CONST FILETIME *lpLastAccess, CONST FILETIME *lpLastWrite) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL FS_DevSetEndOfFile(fsopendev_t *fsodev) {
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

static void DevPSLNotify (DWORD flags, HPROCESS proc, HTHREAD thread) {
	fsopendev_t *fsodev;
	int			icalls, i;
	HANDLE		h[20];
	HANDLE		*ph;

	DEVICE_PSL_NOTIFY	pslPacket;

	EnterCriticalSection(&g_devcs);

	fsodev = g_lpOpenDevs;

	icalls = 0;

	while (fsodev) {
		if (fsodev->hProc == proc)
			++icalls;
		fsodev = fsodev->nextptr;
	}

	if (icalls < sizeof(h) / sizeof(h[0]))
		ph = h;
	else
		ph = (HANDLE *)LocalAlloc (LMEM_FIXED, icalls * sizeof(HANDLE));

	i = 0;
	fsodev = g_lpOpenDevs;

	while (fsodev && i < icalls) {
		if (fsodev->hProc == proc)
			ph[i++] = fsodev->KHandle;
		fsodev = fsodev->nextptr;
	}

	LeaveCriticalSection (&g_devcs);

	pslPacket.dwSize  = sizeof(pslPacket);
	pslPacket.dwFlags = flags;
	pslPacket.hProc   = proc;
	pslPacket.hThread = thread;

	for (i = 0 ; i < icalls ; ++i)
		DeviceIoControl (ph[i], IOCTL_PSL_NOTIFY, (LPVOID)&pslPacket, sizeof(pslPacket), NULL, 0, NULL, NULL);

	if (ph != h)
		LocalFree (ph);
}

void FS_DevProcNotify(DWORD flags, HPROCESS proc, HTHREAD thread) {
	switch (flags) {
#ifndef TARGET_NT
		case DLL_PROCESS_EXITING:
			DevPSLNotify (flags, proc, thread);
			break;
		case DLL_SYSTEM_STARTED:
			//
			// Eventually we'll want to signal all drivers that the system has
			// initialized.  For now do what's necessary.
			// 
			DevloadPostInit();
			break;
#endif // TARGET_NT
	}

	return;
}

void FS_CloseAllDeviceHandles(HPROCESS proc) {
#if TARGET_NT
	ENTER_DEVICE_FUNCTION {
#endif
	fsopendev_t *fsodev = g_lpOpenDevs;
	while (fsodev) {
		if (fsodev->hProc == proc) {
#if TARGET_NT
			FS_DevCloseFileHandle(fsodev);
#else
			CloseHandle(fsodev->KHandle);
#endif
			fsodev = g_lpOpenDevs;
		} else
			fsodev = fsodev->nextptr;
	}
#if TARGET_NT
	} EXIT_DEVICE_FUNCTION;
#endif
}

#ifndef TARGET_NT
void InitOOMSettings()
{
	SYSTEM_INFO	SysInfo;
	GetSystemInfo (&SysInfo);

	// Calling this will set up the initial critical memory handlers and
	// enable memory scavenging in the kernel
	SetOOMEvent (NULL, 30, 15, 0x4000 / SysInfo.dwPageSize,
				 (0x2000 / SysInfo.dwPageSize) > 2 ?
				 (0x2000 / SysInfo.dwPageSize) : 2);	
}


void ProcessDyingOpens(void)
{
	fsopendev_t *fsodev;
	fsopendev_t *fsodevprev;

    DEBUGMSG(ZONE_DYING, (L"Device: About to walk dying opens list\r\n"));
    EnterCriticalSection(&g_devcs);
    fsodevprev = fsodev = g_lpDyingOpens;
    while (fsodev) {
        if (fsodev->dwOpenRefCnt == 0) {
            // Unlink and free unreferenced open
            if (fsodev == g_lpDyingOpens) {
                g_lpDyingOpens = fsodev->nextptr;
                LocalFree(fsodev);
                fsodev = fsodevprev = g_lpDyingOpens;
            } else {
                fsodevprev->nextptr = fsodev->nextptr;
                LocalFree(fsodev);
                fsodev = fsodevprev->nextptr;
            }
        } else {
            fsodevprev = fsodev;
            fsodev = fsodev->nextptr;
        }
    }
    LeaveCriticalSection(&g_devcs);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    fsdev_t* lpdev;
    BOOL bFreedOne;
	if (IsAPIReady(SH_DEVMGR_APIS))
		return 0;

	InitOOMSettings();
	
    InitializeListHead(&g_DevChain);
    InitializeListHead(&g_DyingDevs);
    g_hCleanEvt = CreateEvent(0,0,0,0);
    g_hCleanDoneEvt = CreateEvent(0, 1, 1, 0); //manual reset, init state signalled
    g_hDevApiHandle = CreateAPISet("WFLD", NUM_FDEV_APIS, FDevApiMethods, FDevApiSigs);
    RegisterAPISet(g_hDevApiHandle, SH_DEVMGR_APIS);
    g_hDevFileApiHandle = CreateAPISet("W32D", NUM_FAPIS, DevFileApiMethods, DevFileApiSigs);
    RegisterAPISet(g_hDevFileApiHandle, HT_FILE | REGISTER_APISET_TYPE);
    InitializeCriticalSection(&g_devcs);
    SetPowerOffHandler((FARPROC)FS_PowerAllDevices);
    DevloadInit();
    SignalStarted(_wtol(lpCmdLine));
    while (1) {
        WaitForSingleObject(g_hCleanEvt, INFINITE);

        // check for auto-deregister devs first as they may end up queuing 
        // themselves on the dying devs list
        do {
            EnterCriticalSection(&g_devcs);
            bFreedOne = FALSE;
            // walking the chain of *active* devs, looking for ALL that want to deregister
            // don't care about refcount. That's handled by deregister & the dying-devs list
            DEBUGMSG(ZONE_DYING, (L"Device: About to walk main list\r\n"));
            for (lpdev = (fsdev_t *)g_DevChain.Flink;
                 lpdev != (fsdev_t *)&g_DevChain;
                 lpdev = (fsdev_t *)(lpdev->list.Flink)) {
                if (lpdev->wFlags & DEVFLAGS_AUTO_DEREGISTER) {
                    LeaveCriticalSection(&g_devcs);
                    // don't remove from list. DeregisterDevice will do that
                    DEBUGMSG(ZONE_DYING, (L"Device: FOUND auto-deregister lpdev=%x\r\n", lpdev));
                    FS_DeregisterDevice((HANDLE)lpdev);
                    bFreedOne = TRUE;
                    break;
                    // break out of inner loop, because after letting go of critsec &
                    // freeing lpdev, our loop traversal is not valid anymore. So start over
                }
            }
            if (lpdev == (fsdev_t *)&g_DevChain) {
                LeaveCriticalSection(&g_devcs);
                break;
            }
        } while (bFreedOne);
        // keep looping as long as we have something to do
        
        DEBUGMSG(ZONE_DYING, (L"Device: About to walk dying list\r\n"));
        while (!IsListEmpty(&g_DyingDevs)) 
        {
            EnterCriticalSection(&g_devcs);
            bFreedOne = FALSE;
            // walk the chain of dying devs (already deregistered. just waiting to be unloaded)
            for (lpdev = (fsdev_t *)g_DyingDevs.Flink;
                 lpdev != (fsdev_t *)&g_DyingDevs;
                 lpdev = (fsdev_t *)lpdev->list.Flink) {
                if (!lpdev->dwRefCnt) {
                    RemoveEntryList((PLIST_ENTRY)lpdev);
                    LeaveCriticalSection(&g_devcs);
                    DEBUGMSG(ZONE_DYING, (L"Device: FOUND dying dev lpdev=%x\r\n", lpdev));
                    DoFreeDevice(lpdev);
                    bFreedOne = TRUE;
                    break; 
                    // break out of inner loop, because after letting go of critsec &
                    // freeing lpdev, our loop traversal is not valid anymore. So start over
                }
            }
            if (bFreedOne == FALSE) {
                // Couldn't free anyone
                LeaveCriticalSection(&g_devcs);
                Sleep(5000);    // snooze, and try again...
            }
        }

        ProcessDyingOpens();

	    DEBUGMSG(ZONE_DYING, (TEXT("DEVICE: Setting CleanDoneEvt in WinMain.\r\n")));

        SetEvent(g_hCleanDoneEvt);
    }
    return 1;    // should not ever be reached
}
#else 

BOOL
WINAPI
SetPriorityClass(
    HANDLE hProcess,
    DWORD dwPriorityClass
    );

int wmain(int argc, wchar_t *argv)
{
	SetPriorityClass(GetCurrentProcess(), 0x80 /*HIGH_PRIORITY_CLASS*/);
	InitializeCriticalSection(&g_devcs);
	g_hDevFileApiHandle = (HANDLE) 0x80002000;
	if (!proxy_init(8, 4096))
		return 0;
	DevloadInit();
	DevloadPostInit();
	wait_for_exit_event(); 
	return 1;
}
#endif // TARGET_NT
