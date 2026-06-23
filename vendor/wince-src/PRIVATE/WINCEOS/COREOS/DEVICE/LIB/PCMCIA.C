/* File:    pcmcia.c
 *
 * Purpose: WinCE device loader for PCMCIA devices
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 */
#include <windows.h>
#include <types.h>
#include <tchar.h>
#include <winreg.h>
#include <cardserv.h>
#include <tuple.h>
#include <devload.h>
#include <devloadp.h>
#include <dbt.h>
#include <notify.h>
#include <extfile.h>
#include <netui.h>

#ifdef TARGET_NT
#include <devemul.h>
#include <device.h>
#include <proxy.h>
#endif // TARGET_NT

#ifdef INSTRUM_DEV
#include <instrumd.h>
#endif // INSTRUM_DEV

//
// This module contains these functions:
//  QueryDriverNameThread
//  QueryDriverName
//  InitTapiDeviceChange
//  RunDetectors
//  LoadPCCardDriver
//  FindPCCardDriver
//  FindActiveBySocket
//  I_DeactivateDevice
//  IsCardInserted
//  PcmciaCallBack
//  InitPCMCIA
//  InitTAPIDevices

//
// Functions from device.c
//
void NotifyFSDs(void);
void WaitForFSDs(void);

//
// Functions from devload.c
//
extern BOOL StartDriver(LPTSTR RegPath, LPTSTR PnpId, DWORD LoadOrder, CARD_SOCKET_HANDLE hSock);
extern BOOL CallTapiDeviceChange(HKEY hActiveKey, LPCTSTR DevName, BOOL bDelete);
extern VOID StartDeviceNotify(LPTSTR DevName, BOOL bNew);
extern HMODULE v_hTapiDLL;

CARD_CLIENT_HANDLE v_hPcmcia;
HANDLE          v_hPcmciaDll;
REGISTERCLIENT  pfnRegisterClient;
GETFIRSTTUPLE   pfnGetFirstTuple;
GETNEXTTUPLE    pfnGetNextTuple;
GETTUPLEDATA    pfnGetTupleData;
GETSTATUS       pfnGetStatus;
SYSTEMPOWER     pfnSystemPower;

BOOL IsCardInserted(CARD_SOCKET_HANDLE hSock);

typedef struct _QUERY_NAME_CONTEXT {
    LPTSTR PnpId;
    UCHAR DevType;
    CARD_SOCKET_HANDLE  hSock;
} QUERY_NAME_CONTEXT, * PQUERY_NAME_CONTEXT;

typedef LPTSTR (*PFN_INSTALL_DRIVER)(LPTSTR, LPTSTR, DWORD);

//
// Thread to query the user for the name of the driver for
// an unrecognized pccard, call its Install_Driver function and
// start it.
// 
DWORD
QueryDriverNameThread(
    IN PVOID ThreadContext
    )
{
    PQUERY_NAME_CONTEXT pContext = (PQUERY_NAME_CONTEXT)ThreadContext;
    HMODULE hInstallDll;
    PFN_INSTALL_DRIVER pfnInstall;
    LPTSTR RegPath = NULL;
    GETDRIVERNAMEPARMS GDNP;

    if (!IsCardInserted(pContext->hSock)) {
        goto qdnt_exit;
    }

    GDNP.PCCardType = (DWORD)pContext->DevType;
    GDNP.Socket = (DWORD)pContext->hSock.uSocket;

    //
    // Get this unrecognized card's device driver name.
    //
    if (!CallGetDriverName(NULL, &GDNP)) {
        DEBUGMSG(ZONE_PCMCIA, (TEXT("DEVICE: CallGetDriverName failed %d\r\n"),
            GetLastError()));
        goto qdnt_exit;
    }

    DEBUGMSG(ZONE_PCMCIA,
        (TEXT("DEVICE: CallGetDriverName returned %s\r\n"), GDNP.DriverName));

    hInstallDll = LoadDriver(GDNP.DriverName);
    if (hInstallDll == NULL) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE: LoadDriver(%s) failed %d\r\n"),
            GDNP.DriverName, GetLastError()));
        goto qdnt_exit;
    }

    pfnInstall = (PFN_INSTALL_DRIVER)GetProcAddress(hInstallDll,
                     TEXT("Install_Driver"));
    //
    // Tell it to install itself
    //
    if (pfnInstall) {
        RegPath = pfnInstall(
                     pContext->PnpId,
                     GDNP.DriverName,
                     sizeof(GDNP.DriverName));
    }

    //
    // Start it
    //
    if (RegPath) {
        StartDriver(
            RegPath,
            pContext->PnpId,
            MAX_LOAD_ORDER+1,
            pContext->hSock);
    }

    FreeLibrary(hInstallDll);

qdnt_exit:
    LocalFree(pContext->PnpId);
    LocalFree(pContext);
    return 0;
}   // QueryDriverNameThread

//
// Function to start a thread to query the user for the name of the driver for
// an unrecognized pccard.
//
VOID
QueryDriverName(
    LPTSTR PnpId,
    UCHAR DevType,
    CARD_SOCKET_HANDLE hSock
    )
{
    HANDLE hThd;
    LPTSTR pPnpId;
    PQUERY_NAME_CONTEXT pContext;
    DWORD cExtra;

    pContext = LocalAlloc(LPTR, sizeof(QUERY_NAME_CONTEXT));
    if (pContext == NULL) {
        return;
    }

    cExtra = (hSock.uFunction > 0) ? 3 : 1;
    pPnpId = LocalAlloc(LPTR, (_tcslen(PnpId) + cExtra) * sizeof(TCHAR));
    if (pPnpId == NULL) {
        LocalFree(pContext);
        return;
    }

    _tcscpy(pPnpId, PnpId);
    //
    // Additional functions will have a "-n" appended to their PnpId where
    // 'n' is the function number. This is so we can have a PnpId registry
    // entry for each function.
    //
    if (hSock.uFunction > 0) {
        cExtra = _tcslen(pPnpId);
        pPnpId[cExtra]   = (TCHAR) '-';
        pPnpId[cExtra+1] = (TCHAR) '0' + (TCHAR) hSock.uFunction;
        pPnpId[cExtra+2] = (TCHAR) 0;
    }

    pContext->PnpId = pPnpId;
    pContext->DevType = DevType;
    pContext->hSock = hSock;
    hThd = CreateThread(NULL, 0,
                     (LPTHREAD_START_ROUTINE)&QueryDriverNameThread,
                     (LPVOID) pContext, 0, NULL);
    if (hThd != NULL) {
        CloseHandle(hThd);
    } else {
        LocalFree(pPnpId);
        LocalFree(pContext);
    }
}   // QueryDriverName


//
// RunDetectors - Function to call the detection modules under the key 
// HLM\Drivers\PCMCIA\Detect and return the key name under HLM\Drivers\PCMCIA
// for the device driver to load (will be passed to LoadPCCardDriver), or return
// NULL if none of the detection modules recognizes the card.
// The names of the keys under HLM\Drivers\PCMCIA\Detect are actually numbers to
// allow a definite ordering of the detection modules.
//
LPTSTR
RunDetectors(
    CARD_SOCKET_HANDLE hSock,
    UCHAR DevType,
    LPTSTR StrBuf,
    DWORD StrLen
    )
{
    HKEY hDetectKey;
    HKEY hDetectMod;
    PFN_DETECT_ENTRY pfnDetectEntry;
    TCHAR DetectDll[DEVDLL_LEN];
    TCHAR DetectEntry[DEVENTRY_LEN];
    LPTSTR DeviceKey;
    DWORD status;
    DWORD ValLen;
    DWORD ValType;
    DWORD NumDetectKeys;
    DWORD RegEnum;
    DWORD DetectOrder;
    HMODULE hDetectDll;

    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                DEVLOAD_DETECT_KEY,
                0,
                0,
                &hDetectKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!RunDetectors: RegOpenKeyEx(%s) returned %d\r\n"),
            DEVLOAD_DETECT_KEY, status));
        return FALSE;
    }

    //
    // See how many detection modules there are
    //
    RegEnum = sizeof(DetectDll);
    status = RegQueryInfoKey(
                    hDetectKey,
                    DetectDll,      // class name buffer (lpszClass)
                    &RegEnum,       // ptr to length of class name buffer (lpcchClass)
                    NULL,           // reserved
                    &NumDetectKeys, // ptr to number of subkeys (lpcSubKeys)
                    &ValType,       // ptr to longest subkey name length (lpcchMaxSubKeyLen)
                    &ValLen,        // ptr to longest class string length (lpcchMaxClassLen)
                    &DetectOrder,   // ptr to number of value entries (lpcValues)
                    &DetectOrder,   // ptr to longest value name length (lpcchMaxValueNameLen)
                    &ValLen,        // ptr to longest value data length (lpcbMaxValueData)
                    NULL,           // ptr to security descriptor length
                    NULL);          // ptr to last write time
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!RunDetectors: RegQueryInfoKey() returned %d.\r\n"),
            status));
        goto rd_end;
    }
    DEBUGMSG(ZONE_PCMCIA,
        (TEXT("DEVICE!RunDetectors: %d detection modules\r\n"), NumDetectKeys));


    //
    // Call the detection modules in numeric order.  (The key names are numbers).
    //
    DetectOrder = 0;
    while (NumDetectKeys) {
        //
        // First check if the user yanked the card.
        //
        if (IsCardInserted(hSock) == FALSE) {
            goto rd_end;
        }

        //
        // Find the next detection module
        //
        wsprintf(DetectDll, TEXT("%02d"), DetectOrder); // format key name
        status = RegOpenKeyEx(hDetectKey, DetectDll, 0, 0, &hDetectMod);
        if (status) {
            goto rd_next_detector1;
        }
        NumDetectKeys--;

        //
        // Get the detection module's DLL name
        //
        ValLen = sizeof(DetectDll);
        status = RegQueryValueEx(
                    hDetectMod,
                    DEVLOAD_DLLNAME_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)&DetectDll,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
                (TEXT("DEVICE!RunDetectors: RegQueryValueEx(%d\\DetectDll) returned %d\r\n"),
                DetectOrder, status));
            goto rd_next_detector;
        }

        //
        // Get the detection module's entrypoint
        //
        ValLen = sizeof(DetectEntry);
        status = RegQueryValueEx(
                    hDetectMod,
                    DEVLOAD_ENTRYPOINT_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)DetectEntry,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
                (TEXT("DEVICE!RunDetectors: RegQueryValueEx(%d\\DetectEntry) returned %d\r\n"),
                DetectOrder, status));
            goto rd_next_detector;
        }

        //
        // Load the detection module and get the address of its entrypoint
        //
        hDetectDll = LoadDriver(DetectDll);
        if (hDetectDll == NULL) {
            DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
                (TEXT("DEVICE!RunDetectors: LoadLibrary(%s) failed %d\r\n"),
                DetectDll, GetLastError()));
            goto rd_next_detector;
        }

        pfnDetectEntry = (PFN_DETECT_ENTRY) GetProcAddress(hDetectDll, DetectEntry);
        if (pfnDetectEntry == NULL) {
            DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
                (TEXT("DEVICE!RunDetectors: GetProcAddr(%s, %s) failed %d\r\n"),
                DetectDll, DetectEntry, GetLastError()));
            FreeLibrary(hDetectDll);
            goto rd_next_detector;
        }

        //
        // Finally, call the detection module's entrypoint
        //
        DEBUGMSG(ZONE_PCMCIA,
            (TEXT("DEVICE!RunDetectors: calling %d:%s:%s\r\n"),
            DetectOrder, DetectDll, DetectEntry));
        __try {
            DeviceKey = (pfnDetectEntry)(hSock, DevType, StrBuf, StrLen);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DEBUGMSG(ZONE_PCMCIA,
                (TEXT("DEVICE!RunDetectors: faulted in %s:%s, continuing\r\n"),
                DetectDll, DetectEntry));
            DeviceKey = NULL;
        }
        FreeLibrary(hDetectDll);
        if (DeviceKey) {
            DEBUGMSG(ZONE_PCMCIA,
                (TEXT("DEVICE!RunDetectors: %d:%s:%s returned %s\r\n"),
                DetectOrder, DetectDll, DetectEntry, DeviceKey));
            RegCloseKey(hDetectKey);
            RegCloseKey(hDetectMod);
            return DeviceKey;
        }

rd_next_detector:
        RegCloseKey(hDetectMod);
rd_next_detector1:
        DetectOrder++;
    }

rd_end:
    RegCloseKey(hDetectKey);
    return NULL;
}   // RunDetectors


//
// Function to start a driver in the devload manner or to LoadDriver(devdll) 
// and GetProcAddress(entry) and call an entrypoint.
//
// Return FALSE only if the driver key does not exist in the registry.
//
BOOL
LoadPCCardDriver(
    CARD_SOCKET_HANDLE hSock,
    LPTSTR PnpId,
    LPTSTR DevName
    )
{
    HKEY hDevKey;
    PFN_DEV_ENTRY pfnEntry;
    TCHAR DevDll[DEVDLL_LEN];
    TCHAR DevEntry[DEVENTRY_LEN];
    TCHAR RegPath[REG_PATH_LEN];
    TCHAR PnpMFCId[REG_PATH_LEN];
    DWORD status;
    DWORD ValLen;
    DWORD ValType;
    HMODULE hDevDll;

    //
    // Format the registry path for this device.
    //
    _tcscpy(RegPath, s_PcmciaKey);
    _tcscat(RegPath, TEXT("\\"));
    if (DevName == NULL) {
        _tcscat(RegPath, PnpId);
        //
        // Additional functions will have a "-n" appended to their PnpId where
        // 'n' is the function number. This is so we can have a PnpId registry
        // entry for each function.
        //
        if (hSock.uFunction > 0) {
            ValLen = _tcslen(RegPath);
            RegPath[ValLen]   = (TCHAR) '-';
            RegPath[ValLen+1] = (TCHAR) '0' + (TCHAR) hSock.uFunction;
            RegPath[ValLen+2] = (TCHAR) 0;
        }
    } else {
        _tcscat(RegPath, DevName);
    }

    //
    // If a driver exists for this PNP id, then use it.
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegPath,
                0,
                0,
                &hDevKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!LoadPCCardDriver: RegOpenKeyEx(%s) returned %d\r\n"),
            RegPath, status));
        return FALSE;
    }

    //
    // Get the entrypoint and dll names.  If the entrypoint name does not exist
    // then call StartDriver which will call RegisterDevice on the driver's 
    // behalf.
    //
    ValLen = sizeof(DevEntry);
    status = RegQueryValueEx(
                hDevKey,
                DEVLOAD_ENTRYPOINT_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)DevEntry,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        _tcscpy(PnpMFCId, PnpId);
        if (hSock.uFunction > 0) {
            ValLen = _tcslen(PnpMFCId);
            if (ValLen > 5 && PnpMFCId[ValLen-5] == '-') {
                _tcsncpy(PnpMFCId + ValLen, PnpMFCId + ValLen - 5, 5);
                _tcsncpy(PnpMFCId + ValLen - 5, L"-DEV", 4);
                PnpMFCId[ValLen-1] = (TCHAR) '0' + (TCHAR) hSock.uFunction;
                PnpMFCId[ValLen+5] = (TCHAR) 0;
            } else {
                _tcscat(PnpMFCId, L"-DEV");
                PnpMFCId[ValLen+4] = (TCHAR) '0' + (TCHAR) hSock.uFunction;
                PnpMFCId[ValLen+5] = (TCHAR) 0;
            }
        }
    
        RegCloseKey(hDevKey);
        StartDriver(RegPath, PnpMFCId, MAX_LOAD_ORDER+1, hSock);
        return TRUE;
    }

    ValLen = sizeof(DevDll);
    status = RegQueryValueEx(
                hDevKey,
                s_DllName_ValName,
                NULL,
                &ValType,
                (PUCHAR)DevDll,
                &ValLen);
    RegCloseKey(hDevKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!LoadPCCardDriver: RegQueryValueEx(DllName) returned %d\r\n"),
            status));
        return TRUE;
    }

    //
    // Load the dll, get the entrypoint's procaddr and call it.
    //
    hDevDll = LoadDriver(DevDll);
    if (hDevDll == NULL) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!LoadPCCardDriver: LoadDriver(%s) failed %d\r\n"),
            DevDll, GetLastError()));
        return TRUE;
    }

    pfnEntry = (PFN_DEV_ENTRY) GetProcAddress(hDevDll, DevEntry);
    if (pfnEntry == NULL) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!LoadPCCardDriver: GetProcAddr(%s, %s) failed %d\r\n"),
            DevDll, DevEntry, GetLastError()));
        FreeLibrary(hDevDll);
        return TRUE;
    }

    //
    // It is the device driver's responsibility to call RegisterDevice.
    // The following FreeLibrary will not unload the DLL if it has registered.
    //
    DEBUGMSG(ZONE_PCMCIA,
        (TEXT("DEVICE!LoadPCCardDriver: Calling %s:%s. socket handle = %d %d\r\n"),
        DevDll, DevEntry, hSock.uSocket, hSock.uFunction));
    (pfnEntry)(RegPath);
    FreeLibrary(hDevDll);
    return TRUE;
}   // LoadPCCardDriver


//
// Function to search the registry under HLM\Drivers\PCMCIA for a device driver
// for a newly inserted card.  If there is no driver associated with the card's
// PNP id, then look at the card's CIS to figure out what generic device driver
// to use.  If the generic driver doesn't recognize it, then see if one of the
// installable detection modules recognizes it.
//
VOID
FindPCCardDriver(
    CARD_SOCKET_HANDLE hSock,
    LPTSTR PnpId
    )
{
    DWORD status;
    UCHAR buf[sizeof(CARD_DATA_PARMS) + 256];
    PCARD_TUPLE_PARMS pParms;
    PCARD_DATA_PARMS  pTuple;
    PUCHAR pData;
    LPTSTR DevKeyName;
    UCHAR DevType;

    //
    // If there is a driver for this particular card, then try it first
    //
    if (LoadPCCardDriver(hSock, PnpId, NULL)) {
        return;
    }

    DevKeyName = NULL;
    DevType = PCCARD_TYPE_UNKNOWN;  // 0xff

    //
    // Find this card's device type to provide a hint to the detection modules
    // (Find the second one if possible to skip a potential CISTPL_FUNCID in a
    // MFCs global tuple chain)
    //
    pParms = (PCARD_TUPLE_PARMS)buf;
    pParms->hSocket = hSock;
    pParms->uDesiredTuple = CISTPL_FUNCID; // contains the device type.
    pParms->fAttributes = 0;
    status = pfnGetFirstTuple(pParms);
    if (status == CERR_SUCCESS) {
        if (hSock.uFunction > 0) {
            status = pfnGetNextTuple(pParms);
            if (status != CERR_SUCCESS) {
                status = pfnGetFirstTuple(pParms);
            }
        }
        if (status == CERR_SUCCESS) {
            pTuple = (PCARD_DATA_PARMS)buf;
            pTuple->uBufLen = sizeof(buf) - sizeof(CARD_DATA_PARMS);
            pTuple->uTupleOffset = 0;
            status = pfnGetTupleData(pTuple);
            if (status == CERR_SUCCESS) {
                pData = buf + sizeof(CARD_DATA_PARMS);
                DevType = *pData;
            } else {
                DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
                    (TEXT("DEVICE!FindPCCardDriver: CardGetTupleData returned %d\r\n"), status));
            }
        }
    } else {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!FindPCCardDriver: CardGetFirstTuple returned %d\r\n"), status));
    }

    DevKeyName = RunDetectors(hSock, DevType, (LPTSTR)buf, sizeof(buf)/sizeof(TCHAR));

    if (DevKeyName) {    
        if (LoadPCCardDriver(hSock, PnpId, DevKeyName)) {
            return;
        }
    } else {
        QueryDriverName(PnpId, DevType, hSock); // unrecognized card inserted,
                                                // ask user for driver name.
    }
    return;
}   // FindPCCardDriver


//
// Function to search through HLM\Drivers\Active for a matching CARD_SOCKET_HANDLE
// Return code is the device's handle.  A handle of 0 indicates that no match was
// found.  The active device number is copied to ActiveNum on success.
// 
DWORD
FindActiveBySocket(
    CARD_SOCKET_HANDLE hSock,
    LPTSTR RegPath
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
    CARD_SOCKET_HANDLE hRegSock;

    _tcscpy(RegPath, s_ActiveKey);
    status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    RegPath,
                    0,
                    0,
                    &ActiveKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!FindActiveBySocket: RegOpenKeyEx(%s) returned %d\r\n"),
            RegPath, status));
        return 0;
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
                (TEXT("DEVICE!FindActiveBySocket: RegEnumKeyEx() returned %d\r\n"),
                status));
            RegCloseKey(ActiveKey);
            return 0;
        }

        status = RegOpenKeyEx(
                    ActiveKey,
                    DevNum,
                    0,
                    0,
                    &DevKey);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!FindActiveBySocket: RegOpenKeyEx(%s) returned %d\r\n"),
                DevNum, status));
            goto fabs_next;
        }

        //
        // See if the socket value matches
        //
        ValLen = sizeof(hRegSock);
        status = RegQueryValueEx(
                    DevKey,
                    DEVLOAD_SOCKET_VALNAME,
                    NULL,
                    &ValType,
                    (PUCHAR)&hRegSock,
                    &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!FindActiveBySocket: RegQueryValueEx(%s\\Socket) returned %d\r\n"),
                DevNum, status));
            goto fabs_next;
        }

        //
        // Is this the one the caller wants?
        //
        if (!((hRegSock.uSocket == hSock.uSocket) &&
            (hRegSock.uFunction == hSock.uFunction))) {
            goto fabs_next;         // No.
        }

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
                (TEXT("DEVICE!FindActiveBySocket: RegQueryValueEx(%s\\Handle) returned %d\r\n"),
                DevNum, status));
            //
            // We found the device, but can't get its handle.
            //
            Handle = 0;
        } else {
            //
            // Format the registry path
            //
            RegPathPtr = RegPath + _tcslen(RegPath);
            RegPathPtr[0] = (TCHAR)'\\';
            RegPathPtr++;
            _tcscpy(RegPathPtr, DevNum);
        }

        RegCloseKey(ActiveKey);
        RegCloseKey(DevKey);
        return Handle;

fabs_next:
        RegCloseKey(DevKey);
        RegEnum++;
    }   // while

}   // FindActiveBySocket


//
// Function to delete all the values under an active key that we were unable
// to delete (most likely because someone else has it opened).
//
VOID
DeleteActiveValues(
    LPTSTR ActivePath
    )
{
    HKEY hActiveKey;
    DWORD status;
    
    //
    // Open active key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                ActivePath,
                0,
                0,
                &hActiveKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
           (TEXT("DEVICE!DeleteActiveValues: RegOpenKeyEx(%s) returned %d\r\n"),
           ActivePath, status));
        return;
    }

    //
    // Delete some values so we no longer mistake it for a valid active key.
    //
    RegDeleteValue(hActiveKey, DEVLOAD_CLIENTINFO_VALNAME);
    RegDeleteValue(hActiveKey, DEVLOAD_HANDLE_VALNAME);
    RegDeleteValue(hActiveKey, DEVLOAD_DEVNAME_VALNAME);
    RegDeleteValue(hActiveKey, DEVLOAD_DEVKEY_VALNAME);
    RegDeleteValue(hActiveKey, DEVLOAD_PNPID_VALNAME);
    RegDeleteValue(hActiveKey, DEVLOAD_SOCKET_VALNAME);
    RegCloseKey(hActiveKey);
}   // DeleteActiveValues


//
// Function to DeregisterDevice a device and remove its active key and announce
// the device's removal to the system.
//
BOOL
I_DeactivateDevice(
    HANDLE hDevice,
    LPTSTR ActivePath
    )
{
    HKEY hActiveKey;
    DWORD status;
    DWORD ValLen;
    DWORD ValType;
    TCHAR DevName[DEVNAME_LEN];
    BOOL bAnnounce;

    bAnnounce = FALSE;

    //
    // Open its active key in the registry
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                ActivePath,
                0,
                0,
                &hActiveKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
           (TEXT("DEVICE!I_DeactivateDevice: RegOpenKeyEx(%s) returned %d\r\n"),
           ActivePath, status));
    } else {
        // We're going to need the name
        ValLen = sizeof(DevName);
        status = RegQueryValueEx(
                     hActiveKey,
                     DEVLOAD_DEVNAME_VALNAME,
                     NULL,
                     &ValType,
                     (PUCHAR)DevName,
                     &ValLen);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
               (TEXT("DEVICE!I_DeactivateDevice: RegQueryValueEx(DevName) returned %d\r\n"),
               status));
            RegCloseKey(hActiveKey);
        } else {
            bAnnounce = TRUE;
        }

        CallTapiDeviceChange(hActiveKey, DevName, TRUE); // bDelete == TRUE
        RegCloseKey(hActiveKey);
    }
#ifndef TARGET_NT
    DeregisterDevice((HANDLE)hDevice);
#else
    {    extern BOOL FS_DeregisterDevice(HANDLE);
        FS_DeregisterDevice((HANDLE)hDevice);
    }
#endif // TARGET_NT
    status = RegDeleteKey(HKEY_LOCAL_MACHINE, ActivePath);
    if (status) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,(TEXT("DEVICE!I_DeactivateDevice: RegDeleteKey failed %d\n"), status));
        DeleteActiveValues(ActivePath);
    }

    //
    // Announce to the world after the key has been deleted
    //
    if (bAnnounce) {
        StartDeviceNotify(DevName, FALSE);
    }
    return TRUE;
}


//
// Returns TRUE if there is a card inserted in the specified socket
//
BOOL
IsCardInserted(
    CARD_SOCKET_HANDLE hSock
    )
{
    STATUS status;
    CARD_STATUS CardStatus;

    CardStatus.hSocket = hSock;
    status = pfnGetStatus(&CardStatus);
    if (status == CERR_SUCCESS) {
        if (CardStatus.fCardState & EVENT_MASK_CARD_DETECT) {
            return TRUE;
        }
    } else {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!IsCardInserted: CardGetStatus returned %d\r\n"), status));
    }
    return FALSE;
}   // IsCardInserted


//
// This is the PCMCIA callback function specified in CardRegisterClient.
// PCMCIA indicates card insertions and removals by calling this function.
//
STATUS PcmciaCallBack(
    CARD_EVENT EventCode,
    CARD_SOCKET_HANDLE hSock,
    PCARD_EVENT_PARMS pParms
    )
{
#define MAX_UNLOAD_DEVICES 32

    DWORD DevHandle;
    DWORD n;
    TCHAR ActivePath[REG_PATH_LEN];

    switch (EventCode) {
    case CE_CARD_INSERTION:
        //
        // If no key exists in the active list for this socket, then this
        // insertion notice is for a newly inserted card (i.e. it is not an
        // artificial insertion notice)
        //
        if (IsCardInserted(hSock) == TRUE) {
            DevHandle = FindActiveBySocket(hSock, ActivePath);
            if (DevHandle == 0) {
                //
                // Look in the registry for a matching PNP id or device type
                // and load the corresponding DLL.
                //
                FindPCCardDriver(
                    hSock,
                    (LPTSTR)pParms->Parm1 /* PNP ID string */);
            }
#ifdef INSTRUM_DEV
			instrum_cardInserted = TRUE;
			SetEvent(instrum_insertEvent);
#endif
        }
        break;

    case CE_CARD_REMOVAL:
        //
        // If this is not an artificial removal notice, then DeregisterDevice the
        // associated device and remove its instance from the active list.
        //
        if (IsCardInserted(hSock) == FALSE) {
#ifdef INSTRUM_DEV
			instrum_cardInserted = FALSE;
#endif
        for (n = 0; n < MAX_UNLOAD_DEVICES; n++) {
            DevHandle = FindActiveBySocket(hSock, ActivePath);
            if (DevHandle) {
                I_DeactivateDevice((HANDLE)DevHandle, ActivePath);
            } else {
                break;  // no more devices
            }
          }
        }
        break;

    case CE_PM_RESUME:
        //
        // This event occurs after the forced removals due to power off and
        // after the subsequent inserts have occured.  The filesystem needs an
        // indication at this time to know that it's safe to do pc card I/O
        //
#ifndef TARGET_NT
        DEBUGMSG(ZONE_PCMCIA,
            (TEXT("DEVICE!PcmciaCallBack: Calling FileSystemPowerFunction(FSNOTIFY_DEVICES_ON\r\n")));
        WaitForFSDs();
        FileSystemPowerFunction(FSNOTIFY_DEVICES_ON);
        NotifyFSDs();
#endif // TARGET_NT
        break;
    }

    return CERR_SUCCESS;
}   // PcmciaCallBack



//
// Called at end of device.exe initialization.
//
VOID
InitPCMCIA(VOID)
{
    CARD_REGISTER_PARMS Parms;

#ifdef TARGET_NT
	// Check the wceemuld NT device driver.
	if (!check_wceemuld()) {
		v_hPcmciaDll = 0;
		return;
	}
#endif

    if (!(v_hPcmciaDll = LoadDriver(TEXT("PCMCIA.DLL")))) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_INIT,
            (TEXT("DEVICE!InitPCMCIA: Failed to load PCMCIA.DLL\r\n")));
    	return;
   	}

    pfnRegisterClient = (REGISTERCLIENT) GetProcAddress(v_hPcmciaDll, TEXT("CardRegisterClient"));
    pfnGetFirstTuple = (GETFIRSTTUPLE) GetProcAddress(v_hPcmciaDll, TEXT("CardGetFirstTuple"));
    pfnGetNextTuple = (GETNEXTTUPLE) GetProcAddress(v_hPcmciaDll, TEXT("CardGetNextTuple"));
    pfnGetTupleData = (GETTUPLEDATA) GetProcAddress(v_hPcmciaDll, TEXT("CardGetTupleData"));
    pfnGetStatus = (GETSTATUS) GetProcAddress(v_hPcmciaDll, TEXT("CardGetStatus"));
    pfnSystemPower = (SYSTEMPOWER) GetProcAddress(v_hPcmciaDll, TEXT("CardSystemPower"));

    if ((v_hPcmciaDll == NULL) || 
        (pfnRegisterClient == NULL) || 
        (pfnGetFirstTuple == NULL) ||
        (pfnGetNextTuple == NULL) ||
        (pfnGetTupleData == NULL) ||
        (pfnGetStatus == NULL)) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_INIT,
            (TEXT("DEVICE!InitPCMCIA: Failed to load PCMCIA.DLL\r\n")));
        FreeLibrary(v_hPcmciaDll);
        v_hPcmciaDll = NULL;
        return;
    }

#ifdef INSTRUM_DEV
	start_instrum_thread(v_hPcmciaDll);
#endif // INSTRUM_DEV

    //
    // Register callback function with PCMCIA driver
    //
    Parms.fEventMask = EVENT_MASK_CARD_DETECT|EVENT_MASK_POWER_MGMT;
    Parms.fAttributes = CLIENT_ATTR_IO_DRIVER | CLIENT_ATTR_NOTIFY_SHARED |
                        CLIENT_ATTR_NOTIFY_EXCLUSIVE;
    v_hPcmcia = (CARD_CLIENT_HANDLE)pfnRegisterClient(PcmciaCallBack, &Parms);
    if (v_hPcmcia == 0) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_INIT|ZONE_ERROR,
            (TEXT("DEVICE!InitPCMCIA: CardRegisterClient failed %d\r\n"), GetLastError()));
        return;
    }
}   // InitPCMCIA


void
InitTapiDeviceChange(
    HKEY hActiveKey
    )
{
#ifndef TARGET_NT
    TCHAR DevName[DEVNAME_LEN];
    DWORD ValType;
    DWORD ValLen;
    DWORD status;

    //
    // Get the device name out of its active key in the registry
    //
    ValLen = sizeof(DevName);
    status = RegQueryValueEx(
                hActiveKey,
                DEVLOAD_DEVNAME_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)DevName,
                &ValLen);

    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PCMCIA|ZONE_ERROR,
            (TEXT("DEVICE!InitTapiDeviceChange: RegQueryValueEx(DevName) returned %d\r\n"),
            status));
        return;
    }

    if (CallTapiDeviceChange(hActiveKey, DevName, FALSE /* bDelete == FALSE */)) {    
        //
        // Cause a WM_DEVICECHANGE message to be posted and CeEventHasOccurred to
        // be called while we have all the information available.
        //
        StartDeviceNotify(DevName, TRUE);
    }
#endif // TARGET_NT
}   // InitTapiDeviceChange


//
// Let TAPI know about the active devices.
// Called after the whole system has initialized.
//
void InitTAPIDevices(void)
{
#ifndef TARGET_NT
    DWORD RegEnum;
    DWORD status;
    DWORD ValLen;
    HKEY ActiveKey;
    HKEY ActiveDevKey;
    TCHAR DevNum[DEVNAME_LEN];
    TCHAR RegPath[REG_PATH_LEN];

    if (v_hTapiDLL) {
        return;
    }

	v_hTapiDLL = (HMODULE)LoadDriver(TEXT("TAPI.DLL"));
    if (v_hTapiDLL == NULL) {
        return;
    }

    //
    // Walk the active device list and call InitTapiDeviceChange(ADD) for
    // each one since there will be tapi devices loaded before TAPI.DLL gets
    // loaded.  Tapi will only get notified if there is a Tsp value in the
    // device key for the active device.
    //
    _tcscpy(RegPath, s_ActiveKey);
    status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    RegPath,
                    0,
                    0,
                    &ActiveKey);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
            (TEXT("DEVICE!InitTAPIDevices: RegOpenKeyEx(%s) returned %d\r\n"),
            RegPath, status));
        return;
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
                (TEXT("DEVICE!InitTAPIDevices: RegEnumKeyEx() returned %d\r\n"),
                status));
            RegCloseKey(ActiveKey);
            return;
        }

        status = RegOpenKeyEx(
                    ActiveKey,
                    DevNum,
                    0,
                    0,
                    &ActiveDevKey);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ACTIVE|ZONE_ERROR,
                (TEXT("DEVICE!InitTAPIDevices: RegOpenKeyEx(%s) returned %d\r\n"),
                DevNum, status));
        } else {
            InitTapiDeviceChange(ActiveDevKey);
            RegCloseKey(ActiveDevKey);
        }

        RegEnum++;
    }   // while
#endif // TARGET_NT
}   // InitTAPIDevices
