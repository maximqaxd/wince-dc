/**************************************************************************/
/** Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. **/
/**************************************************************************/
//
//	misc.c		Support files for Telephony Service Provider Interface
//
//	@doc	EX_TSPI
//
//	@topic	TSPI |
//
//		Tspi utilities
//
//
#include "windows.h"
#include "types.h"
#include "memory.h"
#include "mcx.h"
#include "tspi.h"
#include "tspip.h"
#include "linklist.h"

extern const WCHAR szSettings[];
extern const WCHAR szDialSuffix[];

const WCHAR szDefaultsName[] = TEXT("Drivers\\Unimodem");

// Unimodem thread priority
#define DEFAULT_THREAD_PRIORITY 150
#define REGISTRY_PRIORITY_VALUE TEXT("Priority256")
DWORD g_dwUnimodemThreadPriority;

const DEVMINICFG DefaultDevCfg =
{
    DEVMINCFG_VERSION,          // Version
    0x00,                       // WaitBong
    90,                         // CallSetupFail
    0,                          // ModemOptions
    19200,                      // Baud Rate
    0,                          // Options
    8,                          // Byte Size
    0,                          // Stop Bits
    0                           // Parity
};


extern HANDLE g_hCoreDLL;

//
// Function to complete an outstanding async operation
// (unimodem can only handle one outstanding async operation per linedev)
//
// NOTE: Must be called holding the OpenCS
//
void
CompleteAsyncOp(
    PTLINEDEV pLineDev
    )
{
    DWORD dwID;
    DWORD dwStatus;

    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:+CompleteAsyncOp\n")));


    dwID = pLineDev->dwPendingID;
    dwStatus = pLineDev->dwPendingStatus;

    switch (pLineDev->dwPendingType) {
    case PENDING_LINEACCEPT:
    case PENDING_LINEANSWER:
    case PENDING_LINEDEVSPECIFIC:
    case PENDING_LINEDIAL:
    case PENDING_LINEDROP:
    case PENDING_LINEMAKECALL:
        LeaveCriticalSection(&pLineDev->OpenCS);
        DEBUGMSG(ZONE_ASYNC, (TEXT("UNIMODEM:CompleteAsyncOp completing Op %s(0x%x)\n"),
                             GetPendingName(pLineDev->dwPendingType), dwID));
        TspiGlobals.fnCompletionCallback(dwID, dwStatus);
        EnterCriticalSection(&pLineDev->OpenCS);
        break;
    }

    pLineDev->dwPendingID = INVALID_PENDINGID;
    pLineDev->dwPendingType = INVALID_PENDINGOP;
    pLineDev->dwPendingStatus = 0;
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:-CompleteAsyncOp\n")));
}   // CompleteAsyncOp


//
// NOTE on async operations:
// The function that invokes an async operation (TSPI_lineMakeCall, TSPI_lineAnswer, etc)
// first calls ControlThreadCmd which calls SetAsyncOp. Just before returning it calls
// SetAsyncID in order to avoid calling the TAPI completion callback before the user
// knows the async ID. The UnimodemControlThread will call SetAsyncStatus when the
// async operation has completed. If the ID has been set, then SetAsyncStatus will perform
// the callback.
//

//
// Set the async ID for a pending operation. This signals that the function which
// invoked the async operation has completed so any subsequent SetAsyncStatus call
// will cause a completion callback to TAPI. If the invoked async operation has
// already completed (SetAsyncStatus already called), then we need to kick off a
// thread to make the callback for this completed async operation.
//
// Return LINEERR_OPERATIONFAILED if this async operation has been aborted, else
// return async ID.
//
DWORD
SetAsyncID(
    PTLINEDEV pLineDev,
    DWORD dwID
    )
{
    DWORD dwRet = LINEERR_OPERATIONFAILED;

    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:+SetAsyncID\n")));
    EnterCriticalSection(&pLineDev->OpenCS);
    if (pLineDev->dwPendingID != INVALID_PENDINGID) {
        LeaveCriticalSection(&pLineDev->OpenCS);
    } else {
        pLineDev->dwPendingID = dwID;
        dwRet = dwID;
        //
        // If the operation has completed then we need to do the callback on another thread
        //
        if (pLineDev->dwPendingStatus != LINEERR_ALLOCATED) {
            LeaveCriticalSection(&pLineDev->OpenCS);
            if (!StartCompleteAsyncOp(pLineDev)) {
                dwRet = LINEERR_OPERATIONFAILED;
            }
        } else {
            LeaveCriticalSection(&pLineDev->OpenCS);
        }
    }
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:-SetAsyncID 0x%x\n"), dwRet));
    return dwRet;
}   // SetAsyncID


//
// Set the status for a pending async operation and complete it if possible.
//
void
SetAsyncStatus(
    PTLINEDEV pLineDev,
    DWORD dwNewStatus
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:+SetAsyncStatus\n")));
    EnterCriticalSection(&pLineDev->OpenCS);
    //
    // Is there a pending operation to complete?
    //
    if (pLineDev->dwPendingType != INVALID_PENDINGOP) {
        pLineDev->dwPendingStatus = dwNewStatus;
        if (pLineDev->dwPendingID != INVALID_PENDINGID) {
            CompleteAsyncOp(pLineDev);
        }
    }
    LeaveCriticalSection(&pLineDev->OpenCS);
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:-SetAsyncStatus\n")));
}   // SetAsyncStatus


//
// Abort any previously pending operation and record new pending operation
// (called by async TSPI_line* functions)
//
void
SetAsyncOp(
    PTLINEDEV pLineDev,
    DWORD dwNewOp
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:+SetAsyncOp\n")));
    EnterCriticalSection(&pLineDev->OpenCS);

    //
    // Is there a pending operation that we need to abort?
    //
    if (pLineDev->dwPendingType != INVALID_PENDINGOP) {
        LeaveCriticalSection(&pLineDev->OpenCS);
        SetAsyncStatus(pLineDev, LINEERR_OPERATIONFAILED);
        EnterCriticalSection(&pLineDev->OpenCS);
    }

    pLineDev->dwPendingType = dwNewOp;
    pLineDev->dwPendingStatus = LINEERR_ALLOCATED;
    pLineDev->dwPendingID = INVALID_PENDINGID;

    LeaveCriticalSection(&pLineDev->OpenCS);
    DEBUGMSG(ZONE_FUNC|ZONE_ASYNC, (TEXT("UNIMODEM:-SetAsyncOp\n")));
}   // SetAsyncOp


typedef struct _ASYNCCMD {
    DWORD dwRequestID;
    DWORD dwResult;
} ASYNCCMD, * PASYNCCMD;


DWORD
AsyncCompleteThread(
    PVOID lpvIn
    )
{
    PASYNCCMD pCmd = (PASYNCCMD)lpvIn;

    DEBUGMSG(ZONE_ASYNC, (TEXT("UNIMODEM:AsyncCompleteThread completing ID(0x%x) result 0x%x\n"),
                         pCmd->dwRequestID, pCmd->dwResult));
    TspiGlobals.fnCompletionCallback(pCmd->dwRequestID, pCmd->dwResult);
    TSPIFree(pCmd);
    return 0;
}   // AsyncCompleteThread


BOOL
StartCompleteAsyncOp(
    PTLINEDEV pLineDev
    )
{
    HANDLE hThd;
    PASYNCCMD pCmd;

    if (NULL == (pCmd = TSPIAlloc(sizeof(ASYNCCMD)))) {
        DEBUGMSG(1, (TEXT("UNIMODEM:StartCompleteAsyncOp Unable to allocate ASYNCCMD!!!\n")));
        return FALSE;
    }

    EnterCriticalSection(&pLineDev->OpenCS);
    pCmd->dwRequestID = pLineDev->dwPendingID;
    pCmd->dwResult = pLineDev->dwPendingStatus;
    pLineDev->dwPendingID = INVALID_PENDINGID;
    pLineDev->dwPendingType = INVALID_PENDINGOP;
    pLineDev->dwPendingStatus = 0;
    LeaveCriticalSection(&pLineDev->OpenCS);

    hThd = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)AsyncCompleteThread,
                         pCmd, 0, NULL );
    if (!hThd) {
        DEBUGMSG(1, (TEXT("UNIMODEM:StartCompleteAsyncOp Unable to Create AsyncCompleteThread!!!\n")));
        TSPIFree(pCmd);
        return FALSE;
    }

    CloseHandle(hThd);
    return TRUE;

}   // StartCompleteAsyncOp

//
// Read the default devconfig for this device from the registry
//
VOID
getDefaultDevConfig(
    PTLINEDEV ptLineDev,
    PDEVMINICFG pDevMiniCfg
    )
{
    static const WCHAR szDevConfig[]    = TEXT("DevConfig");
    DWORD dwSize, dwRetCode;
    
    DEBUGMSG(ZONE_FUNC | ZONE_INIT,
             (TEXT("UNIMODEM:+getDefaultDevConfig\r\n")));
    // Get the default device config from the registry
    dwSize = sizeof( DEVMINICFG );
    dwRetCode = MdmRegGetValue(ptLineDev, NULL, szDevConfig, REG_BINARY,
                               (LPBYTE)pDevMiniCfg, &dwSize);
    if( (dwRetCode != ERROR_SUCCESS) ||
        (pDevMiniCfg->wVersion != DEVMINCFG_VERSION) ||
        (dwSize < (sizeof(DEVMINICFG) - (DIAL_MODIFIER_LEN*sizeof(WCHAR)) ))) 
    {
        DEBUGMSG(ZONE_INIT | ZONE_ERROR,
                 (TEXT("UNIMODEM:Device Config error, retcode x%X, version x%X (x%X), Size x%X (x%X)\r\n"),
                  dwRetCode,
                  pDevMiniCfg->wVersion, DEVMINCFG_VERSION,
                  dwSize, (sizeof(DEVMINICFG) - (DIAL_MODIFIER_LEN*sizeof(WCHAR)) )) );
        // Either the registry value is missing, or it is invalid.
        // We need some default, use the one in ROM
        *pDevMiniCfg = DefaultDevCfg;
    }
    pDevMiniCfg->szDialModifier[DIAL_MODIFIER_LEN] = 0; // make sure it is null terminated

    DEBUGMSG(ZONE_INIT,
             (TEXT("UNIMODEM:Done reading Device Config, retcode x%X\r\n"),
              dwRetCode));
    DEBUGMSG(ZONE_INIT,
             (TEXT("UNIMODEM:Version x%X\tWait Bong x%X\tCall Setup Fail x%X\r\n"),
              ptLineDev->DevMiniCfg.wVersion,
              ptLineDev->DevMiniCfg.wWaitBong,
              ptLineDev->DevMiniCfg.dwCallSetupFailTimer));
    DEBUGMSG(ZONE_INIT,
             (TEXT("UNIMODEM:Baud %d\tByte Size %d\tStop Bits %d\tParity %d\r\n"),
              ptLineDev->DevMiniCfg.dwBaudRate,
              ptLineDev->DevMiniCfg.ByteSize,
              ptLineDev->DevMiniCfg.StopBits,
              ptLineDev->DevMiniCfg.Parity));

    DEBUGMSG(ZONE_FUNC | ZONE_INIT,
             (TEXT("UNIMODEM:-getDefaultDevConfig\r\n")));
}

//
// Create a LineDev
//
PTLINEDEV
createLineDev(
    HKEY    hActiveKey,
    LPCWSTR lpszDevPath,
    LPCWSTR lpszDeviceName
    )
{
    PTLINEDEV ptLineDev;
    PTLINEDEV ptExistingDev;
    static const WCHAR szDeviceType[]   = TEXT("DeviceType");
    static const WCHAR szPortName[]     = TEXT("Port");
    static const WCHAR szFriendlyName[] = TEXT("FriendlyName");
    static const WCHAR szPnpIdName[]    = TEXT("PnpId");
    static const WCHAR szMaxCmd[]       = TEXT("MaxCmd");
    static const WCHAR szDialBilling[]  = TEXT("DialBilling");
    DWORD dwSize, dwRetCode, dwTemp;
    WCHAR szContinuation[16];
    
    
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:+createLineDev\r\n")));

    // We are being asked to create a new linedev - do it
    if (!(ptLineDev = (PTLINEDEV) TSPIAlloc(sizeof(TLINEDEV)) )) {
        return NULL;
    }

     // Initialize all of the fields
    ptLineDev->dwPendingID = INVALID_PENDINGID;
    ptLineDev->dwDefaultMediaModes = LINEMEDIAMODE_DATAMODEM;  // Do this early, NullifyLine uses it
    ptLineDev->bControlThreadRunning = FALSE;
    NullifyLineDevice( ptLineDev, TRUE );  // also calls getDefaultDevConfig

    // Get the device registry handle for later use.
    dwRetCode = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        lpszDevPath,
        0,
        KEY_READ,
        &ptLineDev->hSettingsKey);
    if (ERROR_SUCCESS != dwRetCode) {
        // We can't very well continue if the registry values aren't there.
        DEBUGMSG(ZONE_ERROR|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - RegOpenKeyEx %s returned %d.\r\n"),
                  lpszDevPath, dwRetCode));
        goto abort_buffer_allocated;
    } else {
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - RegOpenKeyEx %s handle x%X.\r\n"),
                  lpszDevPath, ptLineDev->hSettingsKey));
    }
    
    // Record the port name, or read it ourselves if NULL
    if( NULL == lpszDeviceName ) {
        // Get the port name from registry
        dwSize = MAXDEVICENAME;
        dwRetCode = MdmRegGetValue(ptLineDev, NULL, szPortName, REG_SZ,
                                   (LPBYTE)ptLineDev->szDeviceName, &dwSize);
        if( dwRetCode != ERROR_SUCCESS ) {
            // We can't do much without a port, abort now.
            goto abort_registry_opened;
        }
    } else {
        wcscpy( ptLineDev->szDeviceName, lpszDeviceName );
    }
    
    // Get the device type from registry
    dwSize = sizeof( DWORD );
    dwRetCode = MdmRegGetValue(ptLineDev, NULL, szDeviceType, REG_DWORD,
                               (LPBYTE)&dwTemp, &dwSize);
    if( dwRetCode == ERROR_SUCCESS ) {
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - Device type x%X.\r\n"), dwTemp));
        
        ptLineDev->wDeviceType = (WORD)dwTemp;
    } else {    
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - Device type defaulting to NULL_MODEM.\r\n")));        
        ptLineDev->wDeviceType = DT_NULL_MODEM;  // default type is null
    }

    
    // Get the maximum command length from registry
    dwSize = sizeof( DWORD );
    dwRetCode = MdmRegGetValue(ptLineDev, szSettings, szMaxCmd, REG_DWORD,
                               (LPBYTE)&dwTemp, &dwSize);
    if( dwRetCode == ERROR_SUCCESS ) {
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - Max command length x%X.\r\n"), dwTemp));
        
        ptLineDev->dwMaxCmd = (WORD)dwTemp;
    } else {    
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev - Max command length defaulting to x%X.\r\n"), MAX_CMD_LENGTH));
        
        ptLineDev->dwMaxCmd = MAX_CMD_LENGTH;
    }

    // Get modem command continuation character - assume it is the first character of the dial suffix
    dwSize = sizeof( szContinuation );
    if (ERROR_SUCCESS != MdmRegGetValue(ptLineDev, szSettings, szDialSuffix,
                                    REG_SZ, (PUCHAR)szContinuation, &dwSize) ) {
        ptLineDev->chContinuation = ';';
    } else {
        ptLineDev->chContinuation = szContinuation[0];
    }
    
    //
    // Get modem dial capabilities
    //
    ptLineDev->dwDevCapFlags =  LINEDEVCAPFLAGS_DIALQUIET |
                                LINEDEVCAPFLAGS_DIALDIALTONE |
                                LINEDEVCAPFLAGS_DIALBILLING;
        
    //
    // Assume dial billing capability (wait for bong, '$' in a dial string)
    //
    dwSize = sizeof(DWORD);
    if (ERROR_SUCCESS == MdmRegGetValue(ptLineDev, szSettings, szDialBilling,
                                    REG_DWORD, (PUCHAR)&dwTemp, &dwSize) ) {
        if (dwTemp == 0) {
            ptLineDev->dwDevCapFlags &= ~LINEDEVCAPFLAGS_DIALBILLING;
        }
    }
    
    // Get the friendly name from registry
    dwSize = MAXDEVICENAME;
    dwRetCode = MdmRegGetValue(ptLineDev, NULL, szFriendlyName, REG_SZ,
                               (LPBYTE)ptLineDev->szFriendlyName, &dwSize);
    if( dwRetCode != ERROR_SUCCESS ) { // Friendly not found, use something else
        // For PCMCIA cards, use PNP-Id.  For other devices, use port name.
        if( DT_PCMCIA_MODEM == ptLineDev->wDeviceType ) {
            DWORD dwType;
            WCHAR szPnpId[MAXDEVICENAME];
            
            dwType = REG_SZ;
            dwSize = MAXDEVICENAME;
            dwRetCode = RegQueryValueEx(hActiveKey,
                                        szPnpIdName,
                                        NULL,
                                        &dwType,
                                        (PUCHAR)szPnpId,
                                        &dwSize);
    
            if ( dwRetCode  != ERROR_SUCCESS) {
                wcsncpy( ptLineDev->szFriendlyName, TEXT("Generic Hayes PCMCIA Modem"), MAXDEVICENAME );

                DEBUGMSG( ZONE_INIT|ZONE_ERROR,
                          (TEXT("UNIMODEM:Unable to read PnpId, defaulting to %s\r\n"),
                           ptLineDev->szFriendlyName));
            } else {
                // Remove the checksum from PnpId
                dwSize = wcslen(szPnpId);
                if( dwSize > 5 ) {
                    szPnpId[dwSize - 5] = (WCHAR)0;
                }
                
                // Now create a pseudo-friendly name
#ifdef PREPEND_GENERIC_TO_NAME
                wcsncpy( ptLineDev->szFriendlyName, TEXT("Generic "), MAXDEVICENAME );
                wcsncat( ptLineDev->szFriendlyName, szPnpId, MAXDEVICENAME );
#else
                wcsncpy( ptLineDev->szFriendlyName, szPnpId, MAXDEVICENAME );
#endif
                DEBUGMSG(ZONE_INIT,
                         (TEXT("UNIMODEM:Generic PCMCIA, friendly name  %s\r\n"),
                          ptLineDev->szFriendlyName));
            }
        } else {
            // Non PCMCIA & no friendly name, just use the port name
            wcsncpy( ptLineDev->szFriendlyName, ptLineDev->szDeviceName, MAXDEVICENAME );
        }
    }
    
    DEBUGMSG(ZONE_INIT,
             (TEXT("UNIMODEM:Done reading friendly name, retcode x%X, name %s\r\n"),
              dwRetCode, ptLineDev->szFriendlyName));

    //----------------------------------------------------------------------
    // OK, now that we have all these names, lets see if this same device
    // already exists (it may be a PCMCIA card that was removed and then
    // reinserted).  If it exists, then just reuse the existing entry and
    // let TAPI know that the device reconnected.
    //----------------------------------------------------------------------
    if( ptExistingDev = LineExists( ptLineDev ) ) {
        // We already know about this device
        ptExistingDev->hSettingsKey = ptLineDev->hSettingsKey;  // Get new handle
        
        TSPIFree( ptLineDev );  // Free the entry we started to create.

        // If card is reiniserted, mark it as available.
        ptExistingDev->wDeviceAvail = 1;

        CallLineEventProc(
            ptExistingDev,
            0,
            LINE_LINEDEVSTATE,
            LINEDEVSTATE_CONNECTED|LINEDEVSTATE_INSERVICE,
            0,
            0);
        
        return ptExistingDev;   // And return the original device
    } else {
        // This really is a new device.  Create whatever all we need.
        
        // If we are creating a device, it must be available.
        ptLineDev->wDeviceAvail = 1;

        // Create an event for timeouts, and one for call completion
        ptLineDev->hTimeoutEvent = CreateEvent(0,FALSE,FALSE,NULL);
        ptLineDev->hCallComplete = CreateEvent(0,FALSE,FALSE,NULL);

        // Close needs a critical section
        InitializeCriticalSection(&ptLineDev->OpenCS);
                                  
        if( IS_NULL_MODEM(ptLineDev)  )
            ptLineDev->dwBearerModes = LINEBEARERMODE_DATA | LINEBEARERMODE_PASSTHROUGH;
        else
            ptLineDev->dwBearerModes = LINEBEARERMODE_DATA | LINEBEARERMODE_VOICE | LINEBEARERMODE_PASSTHROUGH;
        
        // And add this new line device to list
        InsertHeadLockedList(&TspiGlobals.LineDevs,
                             &ptLineDev->llist,
                             &TspiGlobals.LineDevsCS);
        DEBUGMSG(ZONE_ALLOC, (TEXT("UNIMODEM:Insert Line Device in List\r\n")));

        // OK, lets notify TAPI about our new device
        DEBUGMSG(ZONE_FUNCTION|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev, calling TAPI Line Create Proc\r\n")));

        // Let TAPI know about our new device
        CallLineEventProc(NULL, NULL, LINE_CREATE,
                        (DWORD)TspiGlobals.hProvider,
                        (DWORD)&ptLineDev->dwDeviceID, 0L);

        DEBUGMSG(ZONE_FUNCTION|ZONE_INIT,
                 (TEXT("UNIMODEM:createLineDev, TAPI assigned device ID 0x%X\r\n"), ptLineDev->dwDeviceID));
    
        DEBUGMSG(ZONE_FUNC|ZONE_INIT,
                 (TEXT("UNIMODEM:-createLineDev\r\n")));
    
        return ptLineDev;
    }
    
abort_registry_opened:
    RegCloseKey( ptLineDev->hSettingsKey );
abort_buffer_allocated:
    TSPIFree( ptLineDev );

    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:-createLineDev, aborted\r\n")));
    return NULL;
}

/* NOT USED
//
// Destroy a LineDev
//
void
DestroyLineDev(
    PTLINEDEV ptLineDev
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:+destroyLineDev\r\n")));

    // First, we better make sure no one is still trying to use line
    DevlineClose ( ptLineDev, TRUE );
    
    // Delete the timeout event and CallComplete event.
    CloseHandle( ptLineDev->hTimeoutEvent );
    CloseHandle( ptLineDev->hCallComplete );

    // Close any critical sections
    DeleteCriticalSection( &ptLineDev->OpenCS );
    
    // Remove this line device from the list
    InsertHeadLockedList(&TspiGlobals.LineDevs,
                         &ptLineDev->llist,
                         &TspiGlobals.LineDevsCS);

    DEBUGMSG(ZONE_ALLOC, (TEXT("Removed Line Device from List\r\n")));

    // Let TAPI know the device went away
    CallLineEventProc(NULL, NULL, LINE_REMOVE,
                     (DWORD)TspiGlobals.hProvider,
                     (DWORD)ptLineDev->dwDeviceID, 0L);

    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:-destroyLineDev\r\n")));
    
    // Close any registry keys we held
    RegCloseKey( ptLineDev->hSettingsKey );

    // And free the structure
    TSPIFree( ptLineDev );

    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:-destroyLineDev\r\n")));

    return;
}
*/

// ****************************************************************************
// GetLineDevfromID()
//
// Function: This function looks for the LineDev associated with an ID
//
// Returns:  PLINEDEV pointer (or NULL if not found)
//       
// ****************************************************************************
PTLINEDEV
GetLineDevfromID(
    DWORD dwDeviceID
    )
{
    PLIST_ENTRY ptEntry;  
    PTLINEDEV ptLineDev;

    DEBUGMSG(ZONE_LIST,
             (TEXT("UNIMODEM:+GetLineDevfromID\r\n")));

     // Walk the Line List and find corresponding handle
    ptEntry = TspiGlobals.LineDevs.Flink;
    while( ptEntry != &TspiGlobals.LineDevs )
    {
        ptLineDev = CONTAINING_RECORD( ptEntry, TLINEDEV, llist);
        
        DEBUGMSG(ZONE_LIST,
                 (TEXT("UNIMODEM:GetLineDevfromID ptLineDev->dwDeviceID x%X, dwID x%X\r\n"),
                  ptLineDev->dwDeviceID, dwDeviceID));
         // See if this is the correct Line
        if( ptLineDev->dwDeviceID == dwDeviceID )
        {
            DEBUGMSG(ZONE_LIST,
                     (TEXT("UNIMODEM:-GetLineDevfromID - Success\r\n")));
            return ptLineDev;  // Cool, we found it
        }

         // No match yet, advance to next link
        ptEntry = ptEntry->Flink;
    }
    
    DEBUGMSG(ZONE_LIST,
             (TEXT("UNIMODEM:-GetLineDevfromID - Fail\r\n")));
    return NULL;
}


PTLINEDEV
GetLineDevfromName(
    LPCWSTR lpszDeviceName,
    LPCWSTR lpszFriendlyName
    )
{
    PLIST_ENTRY ptEntry;  
    PTLINEDEV ptLineDev;

    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:+GetLineDevfromName\r\n")));

     // Walk the Line List and find corresponding handle
    ptEntry = TspiGlobals.LineDevs.Flink;
    while( ptEntry != &TspiGlobals.LineDevs )
    {
        ptLineDev = CONTAINING_RECORD( ptEntry, TLINEDEV, llist);

         // See if this is the correct Line
        if (lpszDeviceName && lpszFriendlyName) {
            if (!wcscmp(lpszDeviceName, ptLineDev->szDeviceName)) {
                if (!wcscmp(lpszFriendlyName, ptLineDev->szFriendlyName)) {
                    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-GetLineDevfromName - Success both\r\n")));
                    return ptLineDev;
                }
            }
        } else if (lpszDeviceName) {
            //
            // When going by device name, only look at active devices since the removal code
            // is calling us and there may be multiple line devices on the same COMn: device.
            //
            if (ptLineDev->wDeviceAvail) {
                if (!wcscmp(lpszDeviceName, ptLineDev->szDeviceName)) {
                    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-GetLineDevfromName - Success device name\r\n")));
                    return ptLineDev;
                }
            }
        } else if (lpszFriendlyName) {
            if (!wcscmp(lpszFriendlyName, ptLineDev->szFriendlyName)) {
                DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-GetLineDevfromName - Success friendly name\r\n")));
                return ptLineDev;
            }
        }

        // Advance to next link.
        ptEntry = ptEntry->Flink;
    }
    
    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-GetLineDevfromName - Fail\r\n")));
    return NULL;
}   // GetLineDevfromName

// ****************************************************************************
//
// Function: This function looks for the LineDev which matches the names and
//           device types of the passed in LineDev.  This is so that we can
//           determine if a lineCreate is really just the reinsertion of a
//           PCMCIA card that we already know about.
//
// Returns:  PLINEDEV pointer (or NULL if not found)
//       
// ****************************************************************************
PTLINEDEV
LineExists(
    PTLINEDEV ptNewLine
    )
{
    PLIST_ENTRY ptEntry;  
    PTLINEDEV ptOldLine;

    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:+LineExists - Device type x%X\r\n"), ptNewLine->wDeviceType));
    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists\tDevice Name : %s\r\n"), ptNewLine->szDeviceName));
    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists\tFriendly Name : %s\r\n"), ptNewLine->szFriendlyName));

     // Walk the Line List and find corresponding handle
    ptEntry = TspiGlobals.LineDevs.Flink;
    while( ptEntry != &TspiGlobals.LineDevs )
    {
        ptOldLine = CONTAINING_RECORD( ptEntry, TLINEDEV, llist);
        
        DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists - checking old dwDeviceID x%X\r\n"), ptOldLine->dwDeviceID));
        DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists\tDevice Type : %x\r\n"), ptOldLine->wDeviceType));
        DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists\tDevice Name : %s\r\n"), ptOldLine->szDeviceName));
        DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:LineExists\tFriendly Name : %s\r\n"), ptOldLine->szFriendlyName));

        // See if this is the correct Line
        if( (ptOldLine->wDeviceType == ptNewLine->wDeviceType) &&
            (! wcscmp(ptOldLine->szDeviceName, ptNewLine->szDeviceName)) &&
            (! wcscmp(ptOldLine->szFriendlyName, ptNewLine->szFriendlyName)) )
        {
            DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-LineExists - previous was ID x%X\r\n"), ptOldLine->dwDeviceID ));
            return ptOldLine;  // this card has been reinserted
        }

         // No match yet, advance to next link
        ptEntry = ptEntry->Flink;
    }
    
    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-LineExists - Fail\r\n")));
    return NULL;
}


// ****************************************************************************
// GetLineDevfromHandle()
//
// Function: This function gets the Line Dev Pointer from a handle
//
// Returns:  a pointer to PLINEDEV structure if the handle is valid, or
//           NULL otherwise
//
// ****************************************************************************

PTLINEDEV
GetLineDevfromHandle (
    DWORD handle
    )
{

    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:+GetLineDevfromHandle\r\n")));
     // The handle IS the pointer.
     // However in the future we may need to provide some abstraction (as well
     // potential validity checks) so this function prepares for that, 
    
    DEBUGMSG(ZONE_LIST, (TEXT("UNIMODEM:-GetLineDevfromHandle\r\n")));
    return (PTLINEDEV)handle;
}

//
// 
void
InitVarData(
    LPVOID lpData,
    DWORD  dwSize
    )
{
    LPVARSTRING pVarData = (LPVARSTRING)lpData;     // Cast the pointer to a varstring

    memset(&(pVarData->dwNeededSize), 0, pVarData->dwTotalSize - sizeof(DWORD));
    pVarData->dwNeededSize = dwSize;
    pVarData->dwUsedSize = dwSize;
}



//
// This routine is called when the TSPI DLL is loaded.  It is primarily
// responsible for initializing TspiGlobals.
//
// Return : void 
//
void
TSPIDLL_Load(
    void
    )
{
    DWORD dwRet;
    
	DEBUGMSG(ZONE_FUNCTION|ZONE_INIT,
             (TEXT("+TSPIDLL_Load\r\n")));

    TspiGlobals.hInstance = 0;
    TspiGlobals.dwProviderID = 0;
    TspiGlobals.hProvider = 0;
    TspiGlobals.fnLineEventProc =0 ;
    TspiGlobals.fnCompletionCallback = 0;

     // Let's init our linked lists to empty
    InitializeListHead( &TspiGlobals.LineDevs );

     // And init the linked list critical sections
    InitializeCriticalSection( &TspiGlobals.LineDevsCS );

    // Go ahead and open the unimodem defaults registry key. 
    dwRet = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szDefaultsName,
        0,
        KEY_READ,
        &TspiGlobals.hDefaultsKey);
    if (ERROR_SUCCESS != dwRet)
    {
        DEBUGMSG(ZONE_ERROR|ZONE_INIT,
                 (TEXT("TSPIDLL_LOAD - RegOpenKeyEx %s returned %d.\r\n"),
                  szDefaultsName, dwRet));
    }
    else
    {
    	DWORD dwValType;
    	DWORD dwValLen;
    
        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("TSPIDLL_LOAD - RegOpenKeyEx %s handle x%X.\r\n"),
                  szDefaultsName, TspiGlobals.hDefaultsKey));

		// Get thread priority
    	g_dwUnimodemThreadPriority = DEFAULT_THREAD_PRIORITY;

        dwValLen = sizeof(DWORD);
        RegQueryValueEx(
        	TspiGlobals.hDefaultsKey,
            REGISTRY_PRIORITY_VALUE,
            NULL,
            &dwValType,
            (PUCHAR)&g_dwUnimodemThreadPriority,
            &dwValLen);

        DEBUGMSG(ZONE_MISC|ZONE_INIT,
                 (TEXT("TSPIDLL_LOAD - Priority %d\r\n"),
                  g_dwUnimodemThreadPriority));
    }

    g_hCoreDLL = LoadLibrary(L"coredll.dll");

	DEBUGMSG(ZONE_FUNCTION|ZONE_INIT,
             (TEXT("-TSPIDLL_Load\r\n")));
}

//****************************************************************************
//
//  NullifyLineDevice()
//
//****************************************************************************
DWORD
NullifyLineDevice (
    PTLINEDEV pLineDev,
    BOOL bDefaultConfig
    )
{
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+NullifyLineDevice\r\n")));

     // Turn the line device back to its initial state
    pLineDev->hDevice      = (HANDLE)INVALID_DEVICE;
    pLineDev->hDevice_r0   = (HANDLE)INVALID_DEVICE;
    pLineDev->hMdmLog      = (HANDLE)INVALID_DEVICE;
    pLineDev->pidDevice    = 0L;
    pLineDev->lpfnEvent    = NULL;
    pLineDev->DevState     = DEVST_DISCONNECTED;
    pLineDev->hwndLine     = NULL;
    pLineDev->szAddress[0] = '\0';
    pLineDev->hwTermCtrl   = NULL;
    pLineDev->dwCallFlags  = 0L;
    pLineDev->dwCallState  = LINECALLSTATE_IDLE;
    pLineDev->dwCurMediaModes = 0L;
    pLineDev->dwDetMediaModes = 0L;
    pLineDev->fTakeoverMode   = FALSE;
    pLineDev->dwMediaModes    = pLineDev->dwDefaultMediaModes;
    pLineDev->dwDialOptions = 0L;
    pLineDev->dwNumRings    = 0;
    if (bDefaultConfig) {
        getDefaultDevConfig( pLineDev, &pLineDev->DevMiniCfg ); // reset the devconfig
    }
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-NullifyLineDevice\r\n")));

    return SUCCESS;
}


//****************************************************************************
//
//  ValidateDevCfgClass()
//
//****************************************************************************

BOOL
ValidateDevCfgClass (
    LPCWSTR lpszDeviceClass
    )
{
    UINT        idClass;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+ValidateDevCfgClass\r\n")));

     // Need the device class
    if (lpszDeviceClass == NULL)
        return FALSE;

     // Determine the device class
     //
    for (idClass = 0; idClass < MAX_SUPPORT_CLASS; idClass++)
    {
        if (lstrcmpi(lpszDeviceClass, aGetID[idClass].szClassName) == 0)
            break;
    };

     // Do we support the requested class?
    switch (idClass)
    {
        case TAPILINE:
        case COMM:
        case COMMMODEM:
            DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-ValidateDevCfgClass (Valid)\r\n")));
            return TRUE;

        default:
            DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-ValidateDevCfgClass (Invalid)\r\n")));
            return FALSE;
    };
}

//****************************************************************************
// ValidateAddress()
//
// Function: This function validates a tapi address and creates a version of
//           it to pass to the VxD.
//
// Returns:  SUCCESS or LINEERR_xxx depending on the failure reason
//
//****************************************************************************

LONG
ValidateAddress(
    PTLINEDEV pLineDev,
    LPCWSTR   lpszInAddress,
    LPWSTR    lpszOutAddress)
{
    LPCWSTR lpszSrc;
    int cbOutLen = MAXADDRESSLEN;
    WCHAR chContinuation = pLineDev->chContinuation;

    ASSERT(lpszOutAddress);

     // is lpszInAddress NULL?
    if (lpszInAddress == NULL || *lpszInAddress == 0)
    {
        *lpszOutAddress = 0;
        return SUCCESS;
    }

     // Verify that the first char is a valid single-byte char.
    if (AnsiNext(lpszInAddress) - lpszInAddress != 1)
    {
        return LINEERR_INVALADDRESS;
    }

     // tone or pulse?  set dwDialOptions appropriately
     // also, set lpszSrc
    switch (*lpszInAddress) {
    case 'T':   // tone
    case 't':
        lpszSrc = lpszInAddress + 1;
        pLineDev->dwDialOptions |= MDM_TONE_DIAL;
        break;

    case 'P':   // pulse
    case 'p':
        lpszSrc = lpszInAddress + 1;
        pLineDev->dwDialOptions &= ~MDM_TONE_DIAL;
        break;

    default:
        lpszSrc = lpszInAddress;
        pLineDev->dwDialOptions |= MDM_TONE_DIAL;
        break;
    }

     // copy In to Out scanning for various dialoptions, returning error if we
     // don't support something.
    while (*lpszSrc && cbOutLen)
    {
        switch (*lpszSrc)
        {
            case '$':
                if (!(pLineDev->dwDevCapFlags & LINEDEVCAPFLAGS_DIALBILLING))
                {
                    UINT  cCommas;

                     // Get the wait-for-bong period
                    cCommas = pLineDev->DevMiniCfg.wWaitBong;

                     // Calculate the number of commas we need to insert
                    cCommas = (cCommas/INC_WAIT_BONG) +
                        (cCommas%INC_WAIT_BONG ? 1 : 0);

                     // Insert the strings of commas
                    while (cbOutLen && cCommas)
                    {
                        *lpszOutAddress++ = ',';
                        cbOutLen--;
                        cCommas--;
                    };
                    goto Skip_This_Character;
                }
                break;

            case '@':
                if (!(pLineDev->dwDevCapFlags & LINEDEVCAPFLAGS_DIALQUIET))
                {
                    return LINEERR_DIALQUIET;
                }
                break;

            case 'W':
            case 'w':
                if (!(pLineDev->dwDevCapFlags & LINEDEVCAPFLAGS_DIALDIALTONE))
                {
                    return LINEERR_DIALDIALTONE;
                }
                break;

            case '?':
                return LINEERR_DIALPROMPT;

            case '|':  // subaddress
            case '^':  // name field
                goto Skip_The_Rest;

            case ' ':
            case '-':
                 // skip these characters
                goto Skip_This_Character;
        }

        if (*lpszSrc == chContinuation) {
             // This signifies the end of a dialable address.
             // Use it and skip the rest.
            *lpszOutAddress++ = *lpszSrc;
            goto Skip_The_Rest;
        }

         // Copy this character
        *lpszOutAddress++ = *lpszSrc;
        cbOutLen--;

Skip_This_Character:
         // Verify that the next char is a valid single-byte char.
        if (AnsiNext(lpszSrc) - lpszSrc != 1)
        {
            return LINEERR_INVALADDRESS;
        }
        lpszSrc++;
    }

     // Did we run out of space in the outgoing buffer?
    if (*lpszSrc && cbOutLen == 0)
    {
         // yes
        DEBUGMSG(ZONE_ERROR,
                 (TEXT("\t*** Address too large.\r\n")));
        return LINEERR_INVALADDRESS;
    }

Skip_The_Rest:
    *lpszOutAddress = 0;
    return SUCCESS;
}


//
// Call TAPI's Line Event Proc with status message filtering
//
void
CallLineEventProc(
    PTLINEDEV   ptLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3
    )
{
    HTAPILINE htLine;

    htLine = (ptLine) ? ptLine->htLine : NULL;

    if (dwMsg == LINE_LINEDEVSTATE) {
        //
        // Honor the lineSetStatusMessages request
        //
        if ((dwParam1 & ptLine->dwLineStatesMask) == 0) {
            return;
        }
    }

    TspiGlobals.fnLineEventProc(
                    htLine,
                    htCall,
                    dwMsg,
                    dwParam1,
                    dwParam2,
                    dwParam3
                    );
}   // CallLineEventProc

#ifdef DEBUG
LPWSTR
CallStateName(
    DWORD dwCallState
    )
{
    LPWSTR lpszRet;

    switch (dwCallState) {
    case LINECALLSTATE_IDLE:                lpszRet = L"LINECALLSTATE_IDLE";                break;
    case LINECALLSTATE_OFFERING:            lpszRet = L"LINECALLSTATE_OFFERING";            break;
    case LINECALLSTATE_ACCEPTED:            lpszRet = L"LINECALLSTATE_ACCEPTED";            break;
    case LINECALLSTATE_DIALTONE:            lpszRet = L"LINECALLSTATE_DIALTONE";            break;
    case LINECALLSTATE_DIALING:             lpszRet = L"LINECALLSTATE_DIALING";             break;
    case LINECALLSTATE_RINGBACK:            lpszRet = L"LINECALLSTATE_RINGBACK";            break;
    case LINECALLSTATE_BUSY:                lpszRet = L"LINECALLSTATE_BUSY";                break;
    case LINECALLSTATE_SPECIALINFO:         lpszRet = L"LINECALLSTATE_SPECIALINFO";         break;
    case LINECALLSTATE_CONNECTED:           lpszRet = L"LINECALLSTATE_CONNECTED";           break;
    case LINECALLSTATE_PROCEEDING:          lpszRet = L"LINECALLSTATE_PROCEEDING";          break;
    case LINECALLSTATE_ONHOLD:              lpszRet = L"LINECALLSTATE_ONHOLD";              break;
    case LINECALLSTATE_CONFERENCED:         lpszRet = L"LINECALLSTATE_CONFERENCED";         break;
    case LINECALLSTATE_ONHOLDPENDCONF:      lpszRet = L"LINECALLSTATE_ONHOLDPENDCONF";      break;
    case LINECALLSTATE_ONHOLDPENDTRANSFER:  lpszRet = L"LINECALLSTATE_ONHOLDPENDTRANSFER";  break;
    case LINECALLSTATE_DISCONNECTED:        lpszRet = L"LINECALLSTATE_DISCONNECTED";        break;
    case LINECALLSTATE_UNKNOWN:             lpszRet = L"LINECALLSTATE_UNKNOWN";             break;
    default: lpszRet = L"UNDEFINED!!!"; break;
    }
    return lpszRet;
}
#endif

void
NewCallState(
    PTLINEDEV pLineDev,
    DWORD dwCallState,
    DWORD dwCallState2
    )
{
    DEBUGMSG(ZONE_CALLS, (L"UNIMODEM:NewCallState for Device %d = %s\n", pLineDev->dwDeviceID, CallStateName(dwCallState)));

    pLineDev->dwCallState = dwCallState;
    pLineDev->dwCallStateMode = dwCallState2;
    CallLineEventProc(
        pLineDev,
        pLineDev->htCall,
        LINE_CALLSTATE,
        dwCallState,
        dwCallState2,
        pLineDev->dwCurMediaModes
        );
}   // NewCallState


typedef int (WINAPI * PFN_LOADSTRING)(HINSTANCE,UINT,LPWSTR,int);

//
// Wrapper to load the termctrl DLL and then call TerminalWindow
//
BOOL
DisplayTerminalWindow(
    PTLINEDEV pLineDev,
    DWORD dwTitle
)
{
    PFNTERMINALWINDOW pfnTerminalWindow;
    HANDLE hTermCtrl;
	WCHAR  szWindowTitle[128];
    BOOL bRet;
    PFN_LOADSTRING pfnLoadString;

    bRet = FALSE;
    
    // Load the termctrl dll, and get the entrypoint
    if ((hTermCtrl = LoadLibrary(TEXT("termctrl.dll"))) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_INIT,
            (TEXT("UNIMODEM:DisplayTerminalWindow *** Unable to load termctrl dll\r\n")));
        goto dtw_exit;
    }

    // OK, lets get the function pointer.
    if ((pfnTerminalWindow = (PFNTERMINALWINDOW)GetProcAddress(hTermCtrl, TEXT("TerminalWindow"))) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_INIT,
            (TEXT("UNIMODEM:DisplayTerminalWindow *** GetProcAddress(TerminalWindow) failed\r\n")));
        goto dtw_exit;
    }

    if ((pfnLoadString = (PFN_LOADSTRING)GetProcAddress(g_hCoreDLL, L"LoadStringW")) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_INIT,
            (TEXT("UNIMODEM:DisplayTerminalWindow *** GetProcAddress(LoadStringW) failed\r\n")));
        goto dtw_exit;
    }

    pfnLoadString(TspiGlobals.hInstance, dwTitle, szWindowTitle, sizeof(szWindowTitle)/sizeof(WCHAR));

    //
    // Make UI run in non-realtime thread priority
    //
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    pLineDev->hwTermCtrl = NULL;  // make sure handle is NULL before calling
    if (TW_SUCCESS == pfnTerminalWindow(pLineDev->hDevice, szWindowTitle, &pLineDev->hwTermCtrl)) {
        bRet = TRUE;
    }
    pLineDev->hwTermCtrl = NULL;  // Zero Window handle now that we are done

    CeSetThreadPriority(GetCurrentThread(), g_dwUnimodemThreadPriority);
    
    
dtw_exit:
    if(hTermCtrl) {
        FreeLibrary(hTermCtrl);
    }
    return bRet;
    
}
