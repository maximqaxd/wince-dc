/* File:    devload.c
 *
 * Purpose: WinCE device manager initialization and built-in device management
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 */
#include <windows.h>
#include <types.h>
#include <winreg.h>
#include <tchar.h>
#include <device.h>
#include <devload.h>
#include <devloadp.h>
#include <cardserv.h>
#ifndef TARGET_NT
#include <dbt.h>
#include <tapi.h>
#include <tspi.h>
#include <tapicomn.h>
#include <notify.h>
#include <windev.h>
#else
#include <devemul.h>
#endif // TARGET_NT

//
// Functions:
//  DevicePostInit
//  CallTapiDeviceChange
//  AnnounceNewDevice
//  NotifyDevice
//  StartOneDriver
//  StartDriver
//  InitDevices
//  DevloadInit
//

extern VOID InitPCMCIA(VOID);       // pcmcia.c
extern void InitTAPIDevices(void);  // pcmcia.c
extern HANDLE v_hPcmciaDll;         // pcmcia.c
extern CRITICAL_SECTION g_devcs;    // device.c
extern LIST_ENTRY g_DevChain;       // device.c

int v_NextDeviceNum;            // Used to create active device list
HMODULE v_hTapiDLL;
HMODULE v_hCoreDLL;
BOOL g_bSystemInitd;

const TCHAR s_DllName_ValName[] = DEVLOAD_DLLNAME_VALNAME;
const TCHAR s_ActiveKey[] = DEVLOAD_ACTIVE_KEY;
const TCHAR s_BuiltInKey[] = DEVLOAD_BUILT_IN_KEY;
const TCHAR s_PcmciaKey[] = DEVLOAD_PCMCIA_KEY;

#ifdef DEBUG

//
// These defines must match the ZONE_* defines in devloadp.h
//
#define DBG_ERROR      1
#define DBG_WARNING    2
#define DBG_FUNCTION   4
#define DBG_INIT       8
#define DBG_BUILTIN    16
#define DBG_PCMCIA     32
#define DBG_ACTIVE     64
#define DBG_TAPI       128
#define DBG_FSD        256
#define DBG_DYING      512

DBGPARAM dpCurSettings = {
    TEXT("DEVLOAD"), {
    TEXT("Errors"),TEXT("Warnings"),TEXT("Functions"),TEXT("Initialization"),
    TEXT("Built-In Devices"),TEXT("PCMCIA Devices"),TEXT("Active Devices"),TEXT("TAPI stuff"),
    TEXT("File System Drvr"),TEXT("Dying Devs"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
#ifdef TARGET_NT // Enable some debug prints.
	DBG_PCMCIA
#else 
    0
#endif // TARGET_NT
};
#endif  // DEBUG

//
// Function to call a newly registered device's post initialization
// device I/O control function.
//
VOID
DevicePostInit(
    LPTSTR DevName,
    DWORD  dwIoControlCode,
    DWORD  Handle,          // Handle from RegisterDevice
    HKEY   hDevKey
    )
{
    HANDLE hDev;
    BOOL ret;
    POST_INIT_BUF PBuf;

    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!DevicePostInit calling CreateFile(%s)\r\n"), DevName));
    hDev = CreateFile(
                DevName,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (hDev == INVALID_HANDLE_VALUE) {
        DEBUGMSG(ZONE_ACTIVE,
            (TEXT("DEVICE!DevicePostInit CreateFile(%s) failed %d\r\n"),
            DevName, GetLastError()));
        return;
    }

    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!DevicePostInit calling DeviceIoControl(%s, %d)\r\n"),
         DevName, dwIoControlCode));
    PBuf.p_hDevice = (HANDLE)Handle;
    PBuf.p_hDeviceKey = hDevKey;
    ret = DeviceIoControl(
              hDev,
              dwIoControlCode,
              &PBuf,
              sizeof(PBuf),
              NULL,
              0,
              NULL,
              NULL);
    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!DevicePostInit DeviceIoControl(%s, %d) "),
         DevName, dwIoControlCode));
    if (ret == TRUE) {
        DEBUGMSG(ZONE_ACTIVE, (TEXT("succeeded\r\n")));
    } else {
        DEBUGMSG(ZONE_ACTIVE, (TEXT("failed\r\n")));
    }
    CloseHandle(hDev);
}   // DevicePostInit



//
// Function to notify TAPI of a device change. A TAPI device has a key which
// contains a Tsp value listing the TAPI Service Provider.
//
// Returns TRUE if the device has a TAPI association.
//
BOOL
CallTapiDeviceChange(
    HKEY hActiveKey,
    LPCTSTR DevName,
    BOOL bDelete
    )
{
#ifndef TARGET_NT
    DWORD ValType;
    DWORD ValLen;
    DWORD status;
    HKEY DevKey;
    TCHAR DevKeyPath[REG_PATH_LEN];
    BOOL bTapiDevice;

typedef BOOL (WINAPI *PFN_TapiDeviceChange) (HKEY DevKey, LPCWSTR lpszDevName, BOOL bDelete);
	PFN_TapiDeviceChange pfnTapiDeviceChange;

    //
    // TAPI.DLL must be loaded first.
    //
    if (v_hTapiDLL == NULL) {
        DEBUGMSG(ZONE_TAPI|ZONE_ERROR,
            (TEXT("DEVICE!CallTapiDeviceChange TAPI.DLL not loaded yet.\r\n")));
        return FALSE;
    }

    //
    // It's a TAPI device if a subkey of the device key contains a TSP value.
    //
    ValLen = sizeof(DevKeyPath);
    status = RegQueryValueEx(            // Read Device Key Path
                hActiveKey,
                DEVLOAD_DEVKEY_VALNAME,
                NULL,
                &ValType,
                (LPBYTE)DevKeyPath,
                &ValLen);
    if (status) {
        DEBUGMSG(ZONE_INIT,
            (TEXT("DEVICE!CallTapiDeviceChange RegQueryValueEx(%s) failed %d\r\n"),
            DEVLOAD_DEVKEY_VALNAME, status));
            return FALSE;
    }

    status = RegOpenKeyEx(             // Open the Device Key
                HKEY_LOCAL_MACHINE,
                DevKeyPath,
                0,
                0,
                &DevKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!CallTapiDeviceChange: RegOpenKeyEx(%s) returned %d\r\n"),
            DevKeyPath, status));
        return FALSE;
    }


	// Try to find the pointer to TapiDeviceChange
	pfnTapiDeviceChange = (PFN_TapiDeviceChange)GetProcAddress (v_hTapiDLL, TEXT("TapiDeviceChange"));
	if (pfnTapiDeviceChange == NULL) {
        DEBUGMSG(ZONE_TAPI,
            (TEXT("DEVICE!CallTapiDeviceChange can't get pointer to TapiDeviceChange.\r\n")));
        return FALSE;
	}
		
    bTapiDevice = pfnTapiDeviceChange(DevKey, DevName, bDelete);
    RegCloseKey(DevKey);
    return bTapiDevice;
#endif // TARGET_NT
}   // CallTapiDeviceChange


//
// Function to format and send a Windows broadcast message announcing the arrival
// or removal of a device in the system.
//
VOID
BroadcastDeviceChange(
    LPTSTR DevName,
    BOOL bNew
    )
{
#ifndef TARGET_NT
    PDEV_BROADCAST_PORT pBCast;
    DWORD len;
    LPTSTR str;
	typedef BOOL (WINAPI *PFN_SendNotifyMessageW)(HWND hWnd, UINT Msg,
											  WPARAM wParam,
											  LPARAM lParam);
	PFN_SendNotifyMessageW pfnSendNotifyMessageW;

	if (v_hCoreDLL == NULL) {
    	v_hCoreDLL = (HMODULE)LoadLibrary(TEXT("COREDLL.DLL"));
    	if (v_hCoreDLL == NULL) {
            DEBUGMSG(ZONE_TAPI|ZONE_ERROR,
                (TEXT("DEVICE!BroadcastDeviceChange unable to load CoreDLL.\r\n")));
    		return;
    	}
	}

	pfnSendNotifyMessageW = (PFN_SendNotifyMessageW)GetProcAddress(v_hCoreDLL, TEXT("SendNotifyMessageW"));
	if (pfnSendNotifyMessageW == NULL) {
		DEBUGMSG (ZONE_PCMCIA, (TEXT("Can't find SendNotifyMessage\r\n")));
		return;
	}
    //
    // Don't use GWE API functions if not there
    //
    if (IsAPIReady(SH_WMGR) == FALSE) {
        return;
    }

    len = sizeof(DEV_BROADCAST_HDR) + (_tcslen(DevName) + 1)*sizeof(TCHAR);

    pBCast = LocalAlloc(LPTR, len);
    if (pBCast == NULL) {
        return;
    }

    



    pBCast->dbcp_devicetype = DBT_DEVTYP_PORT;
    pBCast->dbcp_reserved = 0;
    str = (LPTSTR)&(pBCast->dbcp_name[0]);
    _tcscpy(str, DevName);
    pBCast->dbcp_size = len;

    DEBUGMSG(ZONE_PCMCIA,
        (TEXT("DEVICE!BroadcastDeviceChange Calling SendNotifyMessage for device %s\r\n"), DevName));

	// Call the function
    pfnSendNotifyMessageW(
        HWND_BROADCAST,
        WM_DEVICECHANGE,
        (bNew) ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE,
        (LPARAM)pBCast);

    LocalFree(pBCast);

#endif // TARGET_NT
}   // BroadcastDeviceChange


//
// Function to signal the app notification system of a device change
//
VOID
NotifyDevice(
    LPTSTR DevName,
    LPTSTR Op
    )
{
#ifndef TARGET_NT
    DWORD len;
    LPTSTR str;

    //
    // First check if shell functions can be called.
    //
    if (IsAPIReady(SH_WMGR) == FALSE) {
        DEBUGMSG(ZONE_PCMCIA,
            (TEXT("DEVICE!NotifyDevice IsAPIReady(SH_MGR i.e. GWES) returned FALSE, not calling CeEventHasOccurred(%s %s)\r\n"),
            Op, DevName));
        return;
    }

    len = (_tcslen(Op) + _tcslen(DevName) + 2)*sizeof(TCHAR);

    str = LocalAlloc(LPTR, len);
    if (str == NULL) {
        return;
    }

    //
    // Format the end of the command line
    //
    _tcscpy(str, Op);
    _tcscat(str, TEXT(" "));
    _tcscat(str, DevName);

    DEBUGMSG(ZONE_PCMCIA,
        (TEXT("DEVICE!NotifyDevice Calling CeEventHasOccurred(%s)\r\n"), str));
    CeEventHasOccurred(NOTIFICATION_EVENT_DEVICE_CHANGE, str);
    LocalFree(str);
#endif // TARGET_NT
}   // NotifyDevice


typedef struct _DEVICE_CHANGE_CONTEXT {
    TCHAR DevName[DEVNAME_LEN];
    BOOL bNew;
} DEVICE_CHANGE_CONTEXT, * PDEVICE_CHANGE_CONTEXT;


//
// Thread function to call CeEventHasOccurred and SendNotifyMessage for a device
// that has been recently installed or removed. Another thread is needed because
// in the context in which the device changes occur, the gwes and the filesystem
// critical sections are taken.
//
DWORD
DeviceNotifyThread(
   IN PVOID ThreadContext
   )
{
    PDEVICE_CHANGE_CONTEXT pdcc = (PDEVICE_CHANGE_CONTEXT)ThreadContext;

    BroadcastDeviceChange(pdcc->DevName, pdcc->bNew);
    NotifyDevice(pdcc->DevName, (pdcc->bNew) ? NOTIFY_DEVICE_ADD : NOTIFY_DEVICE_REMOVE);
    LocalFree(pdcc);
    return 0;
}   // DeviceNotifyThread


//
// Function to start a thread that signals a device change via the application
// notification system and via a broadcast windows message.
//
VOID
StartDeviceNotify(
    LPTSTR DevName,
    BOOL bNew
    )
{
    PDEVICE_CHANGE_CONTEXT pdcc;
    HANDLE hThd;

    pdcc = LocalAlloc(LPTR, sizeof(DEVICE_CHANGE_CONTEXT));
    if (pdcc == NULL) {
        return;
    }

    pdcc->bNew = bNew;
    memcpy(pdcc->DevName, DevName, DEVNAME_LEN*sizeof(TCHAR));

    hThd = CreateThread(NULL, 0,
                     (LPTHREAD_START_ROUTINE)&DeviceNotifyThread,
                     (LPVOID) pdcc, 0, NULL);
    if (hThd != NULL) {
        CloseHandle(hThd);
    } else {
        LocalFree(pdcc);
    }
}   // StartDeviceNotify

//
// Function to RegisterDevice a device driver and add it to the active device list
// in HLM\Drivers\Active and then signal the system that a new device is available.
//
// Return the HANDLE from RegisterDevice.
//
HANDLE
StartOneDriver(
    LPTSTR  RegPath,
    LPTSTR  PnpId,
    DWORD   LoadOrder,
    DWORD   ClientInfo,
    CARD_SOCKET_HANDLE hSock
    )
{
    BOOL bUseContext;
    BOOL bFoundIndex;
    HKEY ActiveKey;
    HKEY DevKey;
    DWORD Context;
    DWORD Disposition;
    DWORD Handle;
    DWORD Index;
    DWORD status;
    DWORD ValLen;
    DWORD ValType;
    LPTSTR str;
    TCHAR ActivePath[REG_PATH_LEN];
    TCHAR DevDll[DEVDLL_LEN];
    TCHAR DevName[DEVNAME_LEN];
    TCHAR DevNum[DEVNAME_LEN];
    TCHAR Prefix[DEVPREFIX_LEN];
    fsdev_t * lpdev;

    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!StartOneDriver starting HLM\\%s.\r\n"), RegPath));

    //
    // Get the required (dll and prefix) and optional (index and context) values.
    //
    status = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                RegPath,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                0,
                NULL,
                &DevKey,     // HKEY result
                &Disposition);
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegCreateKeyEx(%s) returned %d.\r\n"),
            RegPath, status));
        return NULL;
    }

    ValLen = sizeof(DevDll);
    status = RegQueryValueEx(
                DevKey,
                s_DllName_ValName,
                NULL,
                &ValType,
                (PUCHAR)DevDll,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegQueryValueEx(%s\\DllName) returned %d\r\n"),
            RegPath, status));
        RegCloseKey(DevKey);
        return NULL; // dll name is required
    }


    ValLen = sizeof(Prefix);
    status = RegQueryValueEx(
                DevKey,
                DEVLOAD_PREFIX_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)Prefix,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegQueryValueEx(%s\\Prefix) returned %d\r\n"),
            RegPath, status));
        RegCloseKey(DevKey);
        return NULL; // Prefix is required
    }

    //
    // Read the optional index and context values
    //
    ValLen = sizeof(Index);
    status = RegQueryValueEx(
                DevKey,
                DEVLOAD_INDEX_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)&Index,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegQueryValueEx(%s\\Index) returned %d\r\n"),
            RegPath, status));
        Index = (DWORD)-1;     // devload will find an index to use
    }

    bUseContext = TRUE;
    ValLen = sizeof(Context);
    status = RegQueryValueEx(
                DevKey,
                DEVLOAD_CONTEXT_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)&Context,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegQueryValueEx(%s\\Context) returned %d\r\n"),
            RegPath, status));
        bUseContext = FALSE;    // context is pointer to active reg path string
    }

    //
    // Format the key's registry path (HLM\Drivers\Active\nnnn)
    //
    _tcscpy(ActivePath, s_ActiveKey);
    _tcscat(ActivePath, TEXT("\\"));
    wsprintf(DevNum, TEXT("%02d"), v_NextDeviceNum);
    _tcscat(ActivePath, DevNum);
    v_NextDeviceNum++;

    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!StartOneDriver Adding HLM\\%s.\r\n"), ActivePath));

    //
    // Create a new key in the active list
    //
    status = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                ActivePath,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                0,
                NULL,
                &ActiveKey,     // HKEY result
                &Disposition);
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegCreateKeyEx(%s) returned %d.\r\n"),
            ActivePath, status));
        RegCloseKey(DevKey);
        return NULL;
    }

    //
    // Default context is registry path
    //
    if (bUseContext == FALSE) {
        Context = (DWORD)ActivePath;
    }


    //
    // Install pnp id, socket and key name values in the device's active registry key.
    // (handle and device name are added after RegisterDevice())
    //

    //
    // Registry path of the device driver (from HLM\Drivers\BuiltIn or HLM\Drivers\PCMCIA)
    //
    if (RegPath != NULL) {    
        status = RegSetValueEx(
                    ActiveKey,
                    DEVLOAD_DEVKEY_VALNAME,
                    0,
                    DEVLOAD_DEVKEY_VALTYPE,
                    (PBYTE)RegPath,
                    sizeof(TCHAR)*(_tcslen(RegPath) + sizeof(TCHAR)));
        if (status) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
                DEVLOAD_DEVKEY_VALNAME, status));
        }
    }

    //
    // Plug and Play Id string
    //
    if (PnpId != NULL) {
        status = RegSetValueEx(
                    ActiveKey,
                    DEVLOAD_PNPID_VALNAME,
                    0,
                    DEVLOAD_PNPID_VALTYPE,
                    (PBYTE)PnpId,
                    sizeof(TCHAR)*(_tcslen(PnpId) + sizeof(TCHAR)));
        if (status) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
                DEVLOAD_PNPID_VALNAME, status));
        }
    }

    //
    // Socket and function number of this device
    //
    if (hSock.uSocket != 0xff) {
        status = RegSetValueEx(
                    ActiveKey,
                    DEVLOAD_SOCKET_VALNAME,
                    0,
                    DEVLOAD_SOCKET_VALTYPE,
                    (PBYTE)&hSock,
                    sizeof(hSock));
        if (status) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
                DEVLOAD_SOCKET_VALNAME, status));
        }
    }

    //
    // Add ClientInfo from ActivateDevice
    //
    status = RegSetValueEx(
                ActiveKey,
                DEVLOAD_CLIENTINFO_VALNAME,
                0,
                DEVLOAD_CLIENTINFO_VALTYPE,
                (PBYTE)&ClientInfo,
                sizeof(DWORD));
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
            DEVLOAD_CLIENTINFO_VALNAME, status));
    }

    if (Index == -1) {
        //
        // Find the first available index for this device prefix.
        //
        bFoundIndex = FALSE;
        Index = 1;  // device index (run it through 1-9 and then 0)
        EnterCriticalSection(&g_devcs);
        while (!bFoundIndex) {
            bUseContext = FALSE; // reuse this flag for this loop.
            for (lpdev = (fsdev_t *)g_DevChain.Flink;
                lpdev != (fsdev_t *)&g_DevChain;
                lpdev = (fsdev_t *)lpdev->list.Flink) {
                if (!memcmp(Prefix, lpdev->type, sizeof(lpdev->type))) {
                    if (lpdev->index == Index) {
                        bUseContext = TRUE;
                        break;
                    }
                }
            }
            if (!bUseContext) {
                //
                // No other devices of this prefix are using this index.
                //
                bFoundIndex = TRUE;
                DEBUGMSG(ZONE_ACTIVE,
                    (TEXT("DEVICE:StartOneDriver using index %d for new %s device\r\n"),
                    Index, Prefix));
                break;
            }
            if (Index == 0) {   // There are no more indexes to try after 0
                break;
            }

            Index++;
            if (Index == 10) {
                Index = 0;       // Try 0 as the last index
            }
        }   // while (trying device indexes)
        LeaveCriticalSection(&g_devcs);
    } else {
        bFoundIndex = TRUE;
    }

    if (bFoundIndex) {
        //
        // Format device name from prefix and index and write it to registry
        //
        _tcscpy(DevName, Prefix);
        str = DevName + _tcslen(DevName);
        str[0] = (TCHAR)Index + (TCHAR)'0';
        str[1] = (TCHAR)':';
        str[2] = (TCHAR)0;
        status = RegSetValueEx(
                    ActiveKey,
                    DEVLOAD_DEVNAME_VALNAME,
                    0,
                    DEVLOAD_DEVNAME_VALTYPE,
                    (PBYTE)DevName,
                    sizeof(TCHAR)*(_tcslen(DevName) + sizeof(TCHAR)));
        if (status) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
                DEVLOAD_DEVNAME_VALNAME, status));
        }
    
#ifdef TARGET_NT
        Handle = (DWORD)RegisterDevice(
                            Prefix,
                            Index,
                            DevDll,
                            Context);
#else
        Handle = (DWORD)RegisterDeviceEx(
                            Prefix,
                            Index,
                            DevDll,
                            Context,
                            LoadOrder
                            );
#endif // TARGET_NT
    } else {
        Handle = 0;
    }

    if (Handle == 0) {
        //
        // RegisterDevice failed
        //
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegisterDevice(%s, %d, %s, 0x%x) failed\r\n"),
            Prefix, Index, DevDll, Context));
        RegCloseKey(DevKey);
        RegCloseKey(ActiveKey);
        RegDeleteKey(HKEY_LOCAL_MACHINE, ActivePath);
        return NULL;
    }

    //
    // Add handle from RegisterDevice()
    //
    status = RegSetValueEx(
                ActiveKey,
                DEVLOAD_HANDLE_VALNAME,
                0,
                DEVLOAD_HANDLE_VALTYPE,
                (PBYTE)&Handle,
                sizeof(Handle));
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegSetValueEx(%s) returned %d.\r\n"),
            DEVLOAD_HANDLE_VALNAME, status));
    }

    CallTapiDeviceChange(ActiveKey, DevName, FALSE); // bDelete == FALSE



    //
    // Determine whether this device wants a post init ioctl
    //
    ValLen = sizeof(Context);
    status = RegQueryValueEx(
                DevKey,
                DEVLOAD_INITCODE_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)&Context,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartOneDriver RegQueryValueEx(%s\\InitCode) returned %d\r\n"),
            RegPath, status));
    } else {
        //
        // Call the new device's IOCTL_DEVICE_INITIALIZED 
        //
        DevicePostInit(DevName, Context, Handle, DevKey);
    }

    RegCloseKey(DevKey);
    RegCloseKey(ActiveKey);

    //
    // Notify only for new PCMCIA devices
    //
#ifndef TARGET_NT
    if (hSock.uSocket != 0xff) {
        StartDeviceNotify(DevName, TRUE);
    }
#endif // TARGET_NT
    return (HANDLE)Handle;
}   // StartOneDriver


//
// Function to RegisterDevice a device driver and add it to the active device list
// in HLM\Drivers\Active and then signal the system that a new device is available.
//
// Return TRUE if the RegisterDevice call succeeded.
//
BOOL
StartDriver(
    LPTSTR  RegPath,
    LPTSTR  PnpId,
    DWORD   LoadOrder,
    CARD_SOCKET_HANDLE hSock
    )
{
    BOOL bRet;
    HKEY DevKey;
    HKEY MultiKey;
    DWORD status;
    DWORD RegEnum;
    DWORD ValLen;
    DWORD ValLen1;
    LPTSTR  RegPathPtr;
    
    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!StartDriver starting HLM\\%s.\r\n"), RegPath));

    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegPath,
                0,
                0,
                &DevKey);
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartDriver RegOpenKeyEx(%s) returned %d.\r\n"),
            RegPath, status));
        return FALSE;
    }

    //
    // If the "MULTI" key exists below this driver, then load the list of
    // drivers under this key.
    //
    status = RegOpenKeyEx(
                DevKey,
                TEXT("MULTI"),
                0,
                0,
                &MultiKey);
    RegCloseKey(DevKey);
    if (status) {
        //
        // No "MULTI" key, just load the driver like we used to.
        //
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartDriver Loading %s (key x%X, status %d).\r\n"), RegPath, DevKey, status));
        return StartOneDriver(RegPath, PnpId, LoadOrder, 0, hSock) == NULL ?
                    FALSE : TRUE;
    }

    //
    // Add "\\MULTI\\" to the registry path
    //
    DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
             (TEXT("DEVICE!StartDriver RegPath %s\r\n"),
              RegPath));
    RegPathPtr = RegPath + _tcslen(RegPath);
    _tcscat(RegPath, TEXT("\\MULTI\\"));
    RegPathPtr = RegPath + _tcslen(RegPath);

    DEBUGMSG(ZONE_ACTIVE,
        (TEXT("DEVICE!StartDriver walking HLM\\%s.\r\n"), RegPath));
    
    //
    // Walk the device keys under the "MULTI" key, calling StartOneDriver for
    // each one.
    //
    bRet = FALSE;
    RegEnum = 0;
    ValLen = REG_PATH_LEN - (RegPathPtr - RegPath);  // ValLen is in TCHAR units, not bytes
    while (1) {
        ValLen1 = ValLen;
        status = RegEnumKeyEx(
                    MultiKey,
                    RegEnum,
                    RegPathPtr,
                    &ValLen1,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                     (TEXT("DEVICE!StartDriver RegEnumKeyEx(%s) returned %d.\r\n"),
                      RegPath, status));
            RegCloseKey(MultiKey);
            return bRet;
        }

        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!StartDriver Loading %s.\r\n"), RegPath));
        if (StartOneDriver(RegPath, PnpId, LoadOrder, RegEnum, hSock)) {
            bRet = TRUE;
        }
        RegEnum++;
    }
    
    return bRet;  // should never get here.

}   // StartDriver

//
// Function to load device drivers for built-in devices.
//
VOID
InitDevices(VOID)
{
    HKEY BuiltInKey;
    HKEY DevKey;
    DWORD DevLoadOrder;
    DWORD LoadOrder;
    DWORD NumDevKeys;
    DWORD RegEnum;
    DWORD status;
    DWORD ValType;
    DWORD ValLen;
    BOOL  bKeepLib;
    TCHAR DevDll[DEVDLL_LEN];
    TCHAR DevEntry[DEVENTRY_LEN];
    TCHAR DevName[DEVNAME_LEN];
    TCHAR RegPath[REG_PATH_LEN];
    TCHAR * RegPathPtr;
    HMODULE hDevDll;
    PFN_DEV_ENTRY DevEntryFn;
    CARD_SOCKET_HANDLE hSock;

    //
    // Open the built-in device registry key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                s_BuiltInKey,
                0,
                0,
                &BuiltInKey);
    if (status) {
        DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
            (TEXT("DEVICE!InitDevices RegOpenKeyEx(BuiltIn) returned %d.\r\n"),
            status));
        return;
    }

    //
    // See how many built-in devices there are
    //
    RegEnum = sizeof(DevDll);
    status = RegQueryInfoKey(
                    BuiltInKey,
                    DevDll,         // class name buffer (lpszClass)
                    &RegEnum,       // ptr to length of class name buffer (lpcchClass)
                    NULL,           // reserved
                    &NumDevKeys,    // ptr to number of subkeys (lpcSubKeys)
                    &ValType,       // ptr to longest subkey name length (lpcchMaxSubKeyLen)
                    &ValLen,        // ptr to longest class string length (lpcchMaxClassLen)
                    &LoadOrder,     // ptr to number of value entries (lpcValues)
                    &DevLoadOrder,  // ptr to longest value name length (lpcchMaxValueNameLen)
                    &ValLen,        // ptr to longest value data length (lpcbMaxValueData)
                    NULL,           // ptr to security descriptor length
                    NULL);          // ptr to last write time
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
            (TEXT("DEVICE!InitDevices RegQueryInfoKey(Class) returned %d.\r\n"),
            status));
        goto id_end;
    }
    DEBUGMSG(ZONE_BUILTIN,
        (TEXT("DEVICE!InitDevices %d devices to load\r\n"), NumDevKeys));

    //
    // Setup registry path name string
    //
    _tcscpy(RegPath, s_BuiltInKey);
    RegPathPtr = RegPath + _tcslen(RegPath);
    *RegPathPtr = (TCHAR)'\\';
    RegPathPtr++;   // leave this pointing after the '\\'

    //
    // Read the built-in device list from the registry and start the drivers
    // according to their load order.
    //
    RegEnum = 0;
    LoadOrder = 0;
    DEBUGMSG(ZONE_BUILTIN,
        (TEXT("DEVICE!InitDevices LoadOrder = %d\r\n"), LoadOrder));
    while (NumDevKeys) {
        ValLen = sizeof(DevName) / sizeof(TCHAR);
        status = RegEnumKeyEx(
                    BuiltInKey,
                    RegEnum,
                    DevName,
                    &ValLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

        if (status != ERROR_SUCCESS) {
            //
            // Assume all keys have been enumerated and start over using the
            // next load order.
            //
            RegEnum = 0;
            LoadOrder++;
            DEBUGMSG(ZONE_BUILTIN,
                (TEXT("DEVICE!InitDevices LoadOrder = %d\r\n"), LoadOrder));
            if (LoadOrder == MAX_LOAD_ORDER) {
                goto id_end;
            }
            continue;
        }

        status = RegOpenKeyEx(
                    BuiltInKey,
                    DevName,
                    0,
                    0,
                    &DevKey);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
                (TEXT("DEVICE!InitDevices RegOpenKeyEx(%s) returned %d\r\n"),
                DevName, status));
            goto id_next_dev1;
        }

        //
        // Check the load order (required)
        //
        ValLen = sizeof(DevLoadOrder);
        status = RegQueryValueEx(
                    DevKey,
                    DEVLOAD_LOADORDER_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)&DevLoadOrder,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
                (TEXT("DEVICE!InitDevices RegQueryValueEx(%s\\LoadOrder) returned %d\r\n"),
                DevName, status));
            goto id_next_dev;
        }

        if (DevLoadOrder != LoadOrder) {
            goto id_next_dev;   // Don't load this device now.
        }

        //
        // If the entrypoint value exists, then devload will load the dll and call the
        // entrypoint function.  The dll device must then call RegisterDevice.
        //
        ValLen = sizeof(DevEntry);
        status = RegQueryValueEx(
                    DevKey,
                    DEVLOAD_ENTRYPOINT_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)DevEntry,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            goto id_register;
        }

        //
        // If the keeplib value is present, then do not call FreeLibrary after
        // calling the device's specified entrypoint.
        //
        ValLen = sizeof(DevDll);
        bKeepLib = (RegQueryValueEx(
                    DevKey,
                    DEVLOAD_KEEPLIB_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)DevDll,
                    &ValLen) == ERROR_SUCCESS) ? TRUE:FALSE;

        //
        // Read the DLL name (required)
        //
        ValLen = sizeof(DevDll);
        status = RegQueryValueEx(
                    DevKey,
                    s_DllName_ValName,
                    NULL,
                    &ValType,
                    (PUCHAR)DevDll,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
                (TEXT("DEVICE!InitDevices RegQueryValueEx(%s\\DllName) returned %d\r\n"),
                DevName, status));
            goto id_next_dev;
        }

        //
        // Load the device now.
        //
        hDevDll = LoadDriver(DevDll);
        if (hDevDll == NULL) {
            DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
                (TEXT("DEVICE!InitDevices LoadDriver(%s) failed %d\r\n"),
                DevDll, GetLastError()));
            goto id_next_dev;
        }

        DevEntryFn = (PFN_DEV_ENTRY)GetProcAddress(hDevDll, DevEntry);
        if (DevEntryFn == NULL) {
            DEBUGMSG(ZONE_BUILTIN|ZONE_ERROR,
                (TEXT("DEVICE!InitDevices GetProcAddr(%s, %s) failed %d\r\n"),
                DevDll, DevEntry, GetLastError()));
            FreeLibrary(hDevDll);
            goto id_next_dev;
        }

        //
        // Finally, call the device's initialization entrypoint
        //
        _tcscpy(RegPathPtr, DevName);
        (DevEntryFn)(RegPath);
        NumDevKeys--;
        if (bKeepLib == FALSE) {
            FreeLibrary(hDevDll);
        }
        goto id_next_dev;


id_register:
        //
        // The device key did not contain an entrypoint value.
        // This means the device wants devload to call RegisterDevice for it.
        //
        _tcscpy(RegPathPtr, DevName);
        hSock.uSocket = 0xff;   // This indicates to StartDriver to not
        hSock.uFunction = 0xff; // add the socket value in the active key

        StartDriver(
            RegPath,
            NULL,           // BuiltIn devices have no PNP Id
            LoadOrder,
            hSock
            );

        NumDevKeys--;

id_next_dev:
        RegCloseKey(DevKey);
id_next_dev1:
        RegEnum++;

    }   // while (more device keys to try)

id_end:
    RegCloseKey(BuiltInKey);
}   // InitDevices


//
// Called from device.c after it initializes.
//
void DevloadInit(void)
{
	DEBUGREGISTER(NULL);
    DEBUGMSG(ZONE_INIT, (TEXT("DEVICE!DevloadInit\r\n")));
    //
    // Delete the HLM\Drivers\Active key since there are no active devices at
    // init time.
    //
    RegDeleteKey(HKEY_LOCAL_MACHINE, s_ActiveKey);

    v_NextDeviceNum = 0;
    v_hTapiDLL = NULL;
    v_hCoreDLL = NULL;
    g_bSystemInitd = FALSE;
    InitPCMCIA();
    InitDevices();
    InitTAPIDevices();
}


//
// Called from device.c after the system has initialized.
//
void DevloadPostInit(void)
{
    SYSTEMINIT pfnCardSystemInit;

    if (g_bSystemInitd) {
        DEBUGMSG(ZONE_INIT, (TEXT("DEVICE!DevloadPostInit: multiple DLL_SYSTEM_STARTED msgs!!!\r\n")));
        return;
    }
    g_bSystemInitd = TRUE;

    DEBUGMSG(ZONE_INIT, (TEXT("DEVICE!DevloadPostInit\r\n")));

    //
    // Indicate to pcmcia.dll that the system has initialized.
    //
    pfnCardSystemInit = (SYSTEMINIT) GetProcAddress(v_hPcmciaDll, TEXT("CardSystemInit"));
    if (pfnCardSystemInit) {
        pfnCardSystemInit();
    }
}
