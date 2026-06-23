/***********************************************************************/
/**Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved**/
/***********************************************************************/
//
//	tapi.c		The Telephony Service Provider Interface
//
//	@doc	EX_TSPI
//
//	@topic	TSPI |
//
//		Tspi Stuff
//
//
#include "windows.h"
#include "types.h"
#include "memory.h"
#include "mcx.h"
#include "tspi.h"
#include "linklist.h"
#include "tspip.h"
#include "tapicomn.h"
#include "config.h"
#include "resource.h"

TSPIGLOBALS TspiGlobals;

const GETIDINFO aGetID[] ={{TEXT("tapi/line"),     STRINGFORMAT_BINARY},
                           {TEXT("comm"),          STRINGFORMAT_UNICODE},
                           {TEXT("comm/datamodem"),STRINGFORMAT_BINARY},
                           {TEXT("ndis"),          STRINGFORMAT_BINARY}};

const WCHAR g_szzClassList[] = {TEXT("tapi/line")TEXT("\0")
                                TEXT("comm")TEXT("\0")
                                TEXT("comm/datamodem")TEXT("\0")
                                TEXT("ndis")TEXT("\0\0")};

const WCHAR g_szDeviceClass[] = TEXT("com");
extern const WCHAR szSettings[];
extern const WCHAR szDialSuffix[];

HANDLE g_hCoreDLL;

// Debug Zones.
#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("Unimodem"), {
        TEXT("Init"),   TEXT("Temp"),       TEXT("Async"),      TEXT(""),
        TEXT(""),       TEXT(""),           TEXT(""),           TEXT("Dial"),
        TEXT("Thread"), TEXT("Lists"),      TEXT("Call State"), TEXT("Misc"),
        TEXT("Alloc"),  TEXT("Function"),   TEXT("Warning"),    TEXT("Error") },
    0xC000
}; 
#endif


extern BOOL IsAPIReady(DWORD dwAPISet);

// **********************************************************************
// First, we have the TSPI_provider functions
// **********************************************************************

LONG TSPIAPI
TSPI_providerInit(
    DWORD             dwTSPIVersion,  // @parm TSPI Version - in
    DWORD             dwPermanentProviderID, // @parm Permanent Provider ID - in
    DWORD             dwLineDeviceIDBase, // @parm Line Base ID - in
    DWORD             dwPhoneDeviceIDBase, // @parm phone Base ID - in
    DWORD             dwNumLines,  // @parm Number of lines - in
    DWORD             dwNumPhones,  // @parm Number of phones - in
    ASYNC_COMPLETION  lpfnCompletionProc, // @parm Pointer to callback - in
    LPDWORD           lpdwTSPIOptions // @parm Optional Behaviour Flags - out
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:+TSPI_providerInit, dwPPID 0x%X, dwDeviceIDBase 0x%X, dwNumLines 0x%X\n"),
              dwPermanentProviderID,
              dwLineDeviceIDBase,
              dwNumLines));
    
    TspiGlobals.fnCompletionCallback = lpfnCompletionProc;

    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:-TSPI_providerInit\n")));
    return SUCCESS;
}

LONG TSPIAPI
TSPI_providerInstall(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:+TSPI_providerInstall, dwPPID 0x%X\n"),
              dwPermanentProviderID ));
    
    // Unimodem doesn't really need an install.  Just say OK.

    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:-TSPI_providerInstall\n")));
    return SUCCESS;
}

LONG TSPIAPI
TSPI_providerShutdown(
    DWORD    dwTSPIVersion
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:+TSPI_providerShutdown\n")));
    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:-TSPI_providerShutdown\n")));
    return SUCCESS;
}


LONG TSPIAPI TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
    )
{
    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:+TSPI_providerEnumDevices\n")));

    *lpdwNumLines = 0;

    // This should be the same event proc that gets passed in to
    // lineOpen. but I need it here and now so that I can notify
    // TAPI about devices coming and going.  I'll probably go ahead
    // and store the per device copy just in case TAPI decides that
    // the two funcs should be different for some reason.
    TspiGlobals.fnLineEventProc  = lpfnLineCreateProc;

    TspiGlobals.dwProviderID     = dwPermanentProviderID;
    TspiGlobals.hProvider        = hProvider;
    
    // Note that the only devices that we actually enumerate are
    // the external modems.  All other entries are created when the
    // device loader loads the physical device and notifies us about
    // it.
    EnumExternModems();
    
    DEBUGMSG(ZONE_FUNC|ZONE_INIT, (TEXT("UNIMODEM:-TSPI_providerEnumDevices\n")));
    return SUCCESS;
}

// **********************************************************************
// Then, we have the TSPI_line functions
// **********************************************************************

//
// This function serves as a stub in the vtbl for any of the TSPI
// functions which we choose not to support
//
LONG
TSPIAPI
TSPI_Unsupported( void )
{
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_Unsupported\n")));
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_Unsupported\n")));
    return LINEERR_OPERATIONUNAVAIL;
}


LONG
TSPIAPI
TSPI_lineAccept(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    LPCSTR        lpsUserUserInfo,
    DWORD         dwSize
    )
{
    PTLINEDEV pLineDev;
    LONG rc;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineAccept\n")));
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineAccept ** Invalid Call Handle\n")));
        rc = LINEERR_INVALCALLHANDLE;
        goto exitPoint;
    }

    if (LINECALLSTATE_OFFERING == pLineDev->dwCallState) {
        SetAsyncOp(pLineDev, PENDING_LINEACCEPT);
        SetAsyncStatus(pLineDev, 0);
        NewCallState(pLineDev, LINECALLSTATE_ACCEPTED, 0);
        rc = SetAsyncID(pLineDev, dwRequestID);
    } else {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineAccept ** Invalid Call State\n")));
        rc = LINEERR_INVALCALLSTATE;
    }

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineAccept %x\n"), rc));
    return rc;
}


LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize
    )
{
    LONG rc;
    PTLINEDEV  pLineDev;


    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineAnswer\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineAnswer ** Invalid Call Handle\n")));
        rc = LINEERR_INVALCALLHANDLE;
        goto exitPoint;
    }

    if ((pLineDev->dwCallState != LINECALLSTATE_OFFERING) &&
        (pLineDev->dwCallState != LINECALLSTATE_ACCEPTED)) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineAnswer ** Invalid Call State\n")));
        rc = LINEERR_INVALCALLSTATE;
        goto exitPoint;
    }

    rc = ControlThreadCmd(pLineDev, PENDING_LINEANSWER, dwRequestID);
    if (!(rc & 0x80000000)) {
        rc = SetAsyncID(pLineDev, dwRequestID);
    }

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineAnswer %x\n"), rc));
    return rc;
}   // TSPI_lineAnswer


//
// Close a specified open line device
// 
LONG TSPIAPI
TSPI_lineClose(
    HDRVLINE hdLine
    )
{
    PTLINEDEV  pLineDev;
  
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineClose\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:-TSPI_lineClose ** Invalid Line Handle\n")));
        return LINEERR_OPERATIONFAILED;
    }

    EnterCriticalSection(&pLineDev->OpenCS);
    pLineDev->dwDetMediaModes = 0;
    pLineDev->htLine = NULL;
    LeaveCriticalSection(&pLineDev->OpenCS);

    if (ControlThreadCmd(pLineDev, PENDING_EXIT, INVALID_PENDINGID)) {
        // Make sure that we do not leave anything open
        DevlineClose(pLineDev, TRUE);            
    }

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineClose\n")));
    return SUCCESS;
}

LONG TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL hdCall
    )
{
    LONG rc;
    PTLINEDEV  pLineDev;
  
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineCloseCall\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineCloseCall ** Invalid Call Handle\n")));
        rc = LINEERR_OPERATIONFAILED;
        goto exitPoint;
    }

    // Mark call as unused
    pLineDev->dwCallFlags &= ~(CALL_ALLOCATED|CALL_ACTIVE);
    pLineDev->htCall = NULL;
    rc = 0;

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineCloseCall\n")));
    return rc;
}


LONG TSPIAPI
TSPI_lineConditionalMediaDetection(
    HDRVLINE hdLine,
    DWORD dwMediaModes,
    LPLINECALLPARAMS const lpCallParams
    )
{
    LONG rc;
    PTLINEDEV  pLineDev;
  
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineConditionalMediaDetection\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineConditionalMediaDetection ** Invalid Line Handle\n")));
        rc = LINEERR_OPERATIONFAILED;
        goto exitPoint;
    }

    // Check the requested modes. There must be only our media modes.
    //
    if (dwMediaModes & ~pLineDev->dwMediaModes) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineConditionalMediaDetection ** Invalid Media Mode\n")));
        rc = LINEERR_INVALMEDIAMODE;
        goto exitPoint;
    }
  
    // Check the call paramaters
    //
    if ((lpCallParams->dwBearerMode & (~pLineDev->dwBearerModes)) ||
        (lpCallParams->dwMediaMode  & (~pLineDev->dwMediaModes)) ||
        (lpCallParams->dwAddressMode & (~LINEADDRESSMODE_ADDRESSID))) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineConditionalMediaDetection ** Invalid Call Params\n")));
        rc = LINEERR_INVALCALLPARAMS;
        goto exitPoint;
    }

    if ((lpCallParams->dwAddressType) && (lpCallParams->dwAddressType != LINEADDRESSTYPE_PHONENUMBER)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineConditionalMediaDetection ** Invalid Call Params Address Type\n")));
        rc = LINEERR_INVALCALLPARAMS;
        goto exitPoint;
    }
  
    rc = 0;

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineConditionalMediaDetection\n")));
    return rc;
}   // TSPI_lineConditionalMediaDetection


LONG TSPIAPI
TSPI_lineConfigDialogEdit(
    DWORD dwDeviceID,
    HWND hwndOwner, 
    LPCWSTR lpszDeviceClass,
    LPVOID const lpDeviceConfigIn, 
    DWORD dwSize,
    LPVARSTRING lpDeviceConfigOut
    )
{
    PTLINEDEV pLineDev;
    BYTE cbSize;
    DWORD dwRet = SUCCESS;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineConfigDialogEdit\n")));
    
    // Validate the input/output buffer
    //
    if (lpDeviceConfigOut == NULL) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR, (TEXT("UNIMODEM:-TSPI_lineConfigDialogEdit Invalid lpDeviceConfigOut\n")));
        return LINEERR_INVALPOINTER;
    }

    if (lpDeviceConfigIn == NULL) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR, (TEXT("UNIMODEM:-TSPI_lineConfigDialogEdit Invalid lpDeviceConfigIn\n")));
        return LINEERR_INVALPOINTER;
    }

    if (lpDeviceConfigOut->dwTotalSize < sizeof(VARSTRING)) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR, (TEXT("UNIMODEM:-TSPI_lineConfigDialogEdit lpDeviceConfigOut too small\n")));
        return LINEERR_STRUCTURETOOSMALL;
    }

    // Validate the requested device class
    //
    if (lpszDeviceClass != NULL)
    {
        if (!ValidateDevCfgClass(lpszDeviceClass))
            return LINEERR_INVALDEVICECLASS;
    };

    // Validate the device ID
    //
    if ((pLineDev = GetLineDevfromID(dwDeviceID)) == NULL)
        return LINEERR_NODEVICE;

    // Validate the device configuration structure
    //
    if (pLineDev->DevMiniCfg.wVersion != ((PDEVMINICFG)lpDeviceConfigIn)->wVersion)
        return LINEERR_INVALPARAM;

    // Set the output buffer size
    //
    cbSize  = sizeof( DEVMINICFG );
    lpDeviceConfigOut->dwUsedSize = sizeof(VARSTRING);
    lpDeviceConfigOut->dwNeededSize = sizeof(VARSTRING) + cbSize;

    // Validate the output buffer size
    //
    if (lpDeviceConfigOut->dwTotalSize >= lpDeviceConfigOut->dwNeededSize)
    {
        PDEVMINICFG    pDevMiniConfig;

        // Initialize the buffer
        //
        lpDeviceConfigOut->dwStringFormat = STRINGFORMAT_BINARY;
        lpDeviceConfigOut->dwStringSize   = cbSize;
        lpDeviceConfigOut->dwStringOffset = sizeof(VARSTRING);
        lpDeviceConfigOut->dwUsedSize    += cbSize;

        pDevMiniConfig = (PDEVMINICFG)(lpDeviceConfigOut+1);

        // Bring up property sheets for modems and get the updated commconfig
        //
        if (!TSPI_EditMiniConfig(hwndOwner, pLineDev, (PDEVMINICFG)lpDeviceConfigIn, pDevMiniConfig))
        {
            DEBUGMSG(ZONE_MISC,(TEXT("UNIMODEM:TSPI_lineConfigDialogEdit User canceled in line config dialog\n")));
        }

        DEBUGMSG(ZONE_DIAL,
                 (TEXT("UNIMODEM:TSPI_lineConfigDialogEdit After config dialog edit, Dial Modifier %s\n"), pDevMiniConfig->szDialModifier));
        
    }
    else
    {
        DEBUGMSG(ZONE_FUNC,
                 (TEXT("UNIMODEM:TSPI_lineConfigDialogEdit Insufficient space in output buffer (passed %d, needed %d)\n"),
                  lpDeviceConfigOut->dwTotalSize, lpDeviceConfigOut->dwNeededSize));
    }


    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:-TSPI_lineConfigDialogEdit x%X (Used %d, Need %d)\n"),
              dwRet, lpDeviceConfigOut->dwUsedSize, lpDeviceConfigOut->dwNeededSize));
    return dwRet;
}


//
// Unimodem specific extensions - see common\oak\inc\unimodem.h for more details. Currently only setting properties of
// a DEVMINCONFIG is supported.
//
LONG TSPIAPI
TSPI_lineDevSpecific(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    LPVOID         lpParams,
    DWORD          dwSize
    )
{
    DWORD rc;
    LPDWORD lpdwCmd;
    PTLINEDEV  pLineDev;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineDevSpecific\n")));

    lpdwCmd = (LPDWORD)lpParams;    // The first 4 bytes is the command code

    switch (*lpdwCmd) {
    case UNIMDM_CMD_CHG_DEVCFG:
        if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineDevSpecific ** Invalid Line Handle\n")));
            rc = LINEERR_OPERATIONFAILED;
            goto exitPoint;
        }

        SetAsyncOp(pLineDev, PENDING_LINEDEVSPECIFIC);

        if (rc = DevSpecificLineConfigEdit(pLineDev,(PUNIMDM_CHG_DEVCFG)lpParams)) {
            goto exitPoint;
        }

        SetAsyncStatus(pLineDev, 0);
        break;

    default:
        rc = LINEERR_OPERATIONUNAVAIL;
        goto exitPoint;
    }

    rc = SetAsyncID(pLineDev, dwRequestID);

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineDevSpecific, rc x%X\n"), rc));    
    return rc;
}   // TSPI_lineDevSpecific


LONG TSPIAPI
TSPI_lineDial(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL    hdCall,
    LPCWSTR     lpszDestAddress,
    DWORD       dwCountryCode
    )
{
    PTLINEDEV pLineDev;
    LONG rc;

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:+TSPI_lineDial\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDial ** Invalid Call Handle\n")));
        rc = LINEERR_INVALLINEHANDLE;
        goto exitPoint;
    }

    if (pLineDev->DevState != DEVST_PORTCONNECTWAITFORLINEDIAL) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDial ** DevState != DEVST_PORTCONNECTWAITFORLINEDIAL\n")));
        rc = LINEERR_INVALCALLSTATE;
        goto exitPoint;
    }
    
     // Validate lpszDestAddress and get the processed form of it.
    DEBUGMSG(ZONE_MISC|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDial - validating destination address\n")));
    rc = ValidateAddress(pLineDev, lpszDestAddress, pLineDev->szAddress);
    if (SUCCESS != rc){
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDial ** Invalid Address\n")));
        goto exitPoint;
    }

    rc = ControlThreadCmd(pLineDev, PENDING_LINEDIAL, dwRequestID);
    if (!(rc & 0x80000000)) {
        rc = SetAsyncID(pLineDev, dwRequestID);
    }

exitPoint:
    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:-TSPI_lineDial, rc x%X\n"), rc));    
    return rc;
    
}   // TSPI_lineDial

//
// Terminate a call or abandon a call attempt in progress
// 
LONG TSPIAPI
TSPI_lineDrop(DRV_REQUESTID dwRequestID,
              HDRVCALL hdCall,
              LPCSTR lpsUserUserInfo,
              DWORD dwSize
    )
{
    PTLINEDEV pLineDev;
    LONG rc;
  
    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:+TSPI_lineDrop\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDrop ** Invalid Call Handle\n")));
        return LINEERR_INVALCALLHANDLE;
    }

    if (pLineDev->dwCallState == LINECALLSTATE_IDLE) {
        DEBUGMSG(ZONE_ERROR|ZONE_CALLS, (TEXT("UNIMODEM:TSPI_lineDrop ** Invalid Call State\n")));
        return LINEERR_INVALCALLSTATE;
    }

    EnterCriticalSection(&pLineDev->OpenCS);
    pLineDev->dwCallFlags |= CALL_DROPPING;  // Flag that this call is going away
    pLineDev->dwCallFlags &= ~CALL_ACTIVE;
    LeaveCriticalSection(&pLineDev->OpenCS);

    if (pLineDev->fTakeoverMode) {
        


        pLineDev->fTakeoverMode = FALSE;
        pLineDev->DevState = DEVST_DISCONNECTED;
        
        NewCallState(pLineDev, LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_NORMAL);
        NewCallState(pLineDev, LINECALLSTATE_IDLE, 0);
        SetAsyncOp(pLineDev, PENDING_LINEDROP);
        SetAsyncStatus(pLineDev, 0);
        rc = 0;
    } else {
        rc = ControlThreadCmd(pLineDev, PENDING_LINEDROP, dwRequestID);
    }

    if (!(rc & 0x80000000)) {
        rc = SetAsyncID(pLineDev, dwRequestID);
    }

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:-TSPI_lineDrop, rc x%X\n"), rc));    
    return rc;
}   // TSPI_lineDrop



LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD dwDeviceID,
    DWORD dwAddressID,
    DWORD dwTSPIVersion,
    DWORD dwExtVersion,
    LPLINEADDRESSCAPS lpAddressCaps
    )
{
    LONG rc;
    PTLINEDEV pLineDev;
    int cbDevClassLen;
    int cbAvailMem;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetAddressCaps\n")));
   
    rc = 0;
    InitVarData((LPVOID)lpAddressCaps, sizeof(LINEADDRESSCAPS));

    // Validate the device ID
    if ((pLineDev = GetLineDevfromID(dwDeviceID)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetAddressCaps ** Invalid DeviceID\n")));
        rc = LINEERR_NODEVICE;
        goto exitPoint;
    }

    // Validate the address ID
    if(dwAddressID != 0) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetAddressCaps ** Invalid AddressID\n")));
        rc = LINEERR_INVALADDRESSID;
        goto exitPoint;
    }

    lpAddressCaps->dwLineDeviceID      = dwDeviceID;
    lpAddressCaps->dwAddressSharing     = LINEADDRESSSHARING_PRIVATE;

    lpAddressCaps->dwCallInfoStates     = LINECALLINFOSTATE_APPSPECIFIC | LINECALLINFOSTATE_MEDIAMODE;
    lpAddressCaps->dwCallerIDFlags      = LINECALLPARTYID_UNAVAIL;
    lpAddressCaps->dwCalledIDFlags      = LINECALLPARTYID_UNAVAIL;
    lpAddressCaps->dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    lpAddressCaps->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    lpAddressCaps->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    
    lpAddressCaps->dwCallStates = LINECALLSTATE_IDLE |
                                    LINECALLSTATE_OFFERING |
                                    LINECALLSTATE_ACCEPTED |
                                    LINECALLSTATE_DIALTONE |
                                    LINECALLSTATE_DIALING |
                                    LINECALLSTATE_CONNECTED |
                                    LINECALLSTATE_PROCEEDING |
                                    LINECALLSTATE_DISCONNECTED |
                                    LINECALLSTATE_UNKNOWN;
    
    lpAddressCaps->dwDialToneModes   = LINEDIALTONEMODE_UNAVAIL;
    lpAddressCaps->dwBusyModes       = LINEBUSYMODE_UNAVAIL;
    
    lpAddressCaps->dwSpecialInfo     = LINESPECIALINFO_UNAVAIL;
    
    lpAddressCaps->dwDisconnectModes = LINEDISCONNECTMODE_UNAVAIL |
                                        LINEDISCONNECTMODE_NORMAL |
                                        LINEDISCONNECTMODE_BUSY |
                                        LINEDISCONNECTMODE_NODIALTONE |
                                        LINEDISCONNECTMODE_NOANSWER;
    
    lpAddressCaps->dwMaxNumActiveCalls          = 1;
    
    lpAddressCaps->dwAddrCapFlags = LINEADDRCAPFLAGS_PARTIALDIAL;
    if (!IS_NULL_MODEM(pLineDev)) {
        lpAddressCaps->dwAddrCapFlags |= LINEADDRCAPFLAGS_DIALED;
    }
    
    lpAddressCaps->dwCallFeatures = LINECALLFEATURE_ANSWER |
                                    LINECALLFEATURE_ACCEPT |
                                    LINECALLFEATURE_SETCALLPARAMS |
                                    LINECALLFEATURE_DIAL |
                                    LINECALLFEATURE_DROP;
    
    cbAvailMem = (int) (lpAddressCaps->dwTotalSize - lpAddressCaps->dwUsedSize);
    cbDevClassLen = sizeof(g_szzClassList);

    // Copy device classes if it fits
    if (cbAvailMem >= cbDevClassLen) {
        memcpy((LPBYTE)lpAddressCaps + lpAddressCaps->dwUsedSize, g_szzClassList, cbDevClassLen);
        lpAddressCaps->dwDeviceClassesSize  = cbDevClassLen;
        lpAddressCaps->dwDeviceClassesOffset= lpAddressCaps->dwUsedSize;
        lpAddressCaps->dwUsedSize += cbDevClassLen;
        cbAvailMem -= cbDevClassLen;
    } else {
        lpAddressCaps->dwDeviceClassesSize = 0;
        lpAddressCaps->dwDeviceClassesOffset = 0;
    }    

    lpAddressCaps->dwNeededSize += cbDevClassLen;

exitPoint:    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetAddressCaps 0x%x\n"), rc));
    return rc;
}   // TSPI_lineGetAddressCaps


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE hdLine,
    DWORD dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    LONG rc;
    PTLINEDEV pLineDev;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetAddressStatus\n")));
    rc = 0;
    InitVarData((LPVOID)lpAddressStatus, sizeof(LINEADDRESSSTATUS));
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetAddressStatus ** Invalid hdLine\n")));
        rc = LINEERR_INVALLINEHANDLE;
        goto exitPoint;
    }
    
    if(dwAddressID != 0) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetAddressStatus ** Invalid AddressID\n")));
        rc = LINEERR_INVALADDRESSID;
        goto exitPoint;
    }
    
    if (pLineDev->dwCallFlags & CALL_ACTIVE) {
        lpAddressStatus->dwNumInUse = 1;
        lpAddressStatus->dwNumActiveCalls = (pLineDev->dwCallState != LINECALLSTATE_IDLE) ? 1 : 0;
    } else {
        lpAddressStatus->dwNumInUse = 0;
        lpAddressStatus->dwNumActiveCalls = 0;
    };
    
    lpAddressStatus->dwAddressFeatures = (pLineDev->dwCallFlags & CALL_ALLOCATED) ? 0 : LINEADDRFEATURE_MAKECALL;
        
exitPoint:    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetAddressStatus 0x%x\n"), rc));
    return rc;
}   // TSPI_lineGetAddressStatus



LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL hdCall,
    LPLINECALLINFO lpCallInfo
    )
{
    PTLINEDEV pLineDev;
    LONG rc;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetCallInfo\n")));
    
    InitVarData((LPVOID)lpCallInfo, sizeof(LINECALLINFO));
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetCallInfo Invalid hdCall\n")));
        rc = LINEERR_INVALCALLHANDLE;
        goto exitPoint;
    }
    
    rc = 0;
    lpCallInfo->dwLineDeviceID = pLineDev->dwDeviceID;
    
    lpCallInfo->dwAddressID    = 0;
    lpCallInfo->dwBearerMode   = pLineDev->dwCurBearerModes;
    lpCallInfo->dwRate         = pLineDev->dwCurrentBaudRate;
    lpCallInfo->dwMediaMode    = pLineDev->dwCurMediaModes;
    
    lpCallInfo->dwAppSpecific  = 0;
    
    lpCallInfo->dwCallStates = pLineDev->dwCallFlags & CALL_INBOUND ?
    (LINECALLSTATE_IDLE        |
    LINECALLSTATE_OFFERING     |
    LINECALLSTATE_ACCEPTED     |
    LINECALLSTATE_CONNECTED    |
    LINECALLSTATE_DISCONNECTED |
    LINECALLSTATE_UNKNOWN)      :

    (LINECALLSTATE_IDLE        |
    LINECALLSTATE_DIALTONE     |
    LINECALLSTATE_DIALING      |
    LINECALLSTATE_PROCEEDING   |
    LINECALLSTATE_CONNECTED    |
    LINECALLSTATE_DISCONNECTED |
    LINECALLSTATE_UNKNOWN);
    
    
    lpCallInfo->dwOrigin = pLineDev->dwCallFlags & CALL_INBOUND ?
                                LINECALLORIGIN_INBOUND : LINECALLORIGIN_OUTBOUND;
    lpCallInfo->dwReason = pLineDev->dwCallFlags & CALL_INBOUND ?
                                LINECALLREASON_UNAVAIL : LINECALLREASON_DIRECT;
    lpCallInfo->dwCallerIDFlags      = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwCalledIDFlags      = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    lpCallInfo->dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
    
exitPoint:  
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetCallInfo %x\n"), rc));
    return rc;
}   // TSPI_lineGetCallInfo


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL hdCall,
    LPLINECALLSTATUS lpCallStatus
    )
{
    PTLINEDEV  pLineDev;
    LONG rc;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetCallStatus\n")));
    
    InitVarData((LPVOID)lpCallStatus, sizeof(LINECALLSTATUS));
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetCallStatus Invalid hdCall\n")));
        rc = LINEERR_INVALCALLHANDLE;
        goto exitPoint;
    }
    
    rc = 0;

    //
    // Current call information
    //
    lpCallStatus->dwCallState     = pLineDev->dwCallState;
    lpCallStatus->dwCallStateMode = pLineDev->dwCallStateMode;
    
    // if we are in takeover mode, disallow all dwCallFeatures
    if (!pLineDev->fTakeoverMode) {
        switch(lpCallStatus->dwCallState) {
        case LINECALLSTATE_OFFERING:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_ACCEPT |
            LINECALLFEATURE_SETCALLPARAMS |
            LINECALLFEATURE_DROP;
            // We can only answer if a possible media mode is DATAMODEM.
            if (pLineDev->dwCurMediaModes & LINEMEDIAMODE_DATAMODEM) {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_ANSWER;
            }
            break;
        
        case LINECALLSTATE_DIALTONE:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_DROP;
            break;
        
        case LINECALLSTATE_DIALING:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_DROP;
            if (DEVST_PORTCONNECTWAITFORLINEDIAL == pLineDev->DevState) {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_DIAL;
            }
            break;
        
        case LINECALLSTATE_ACCEPTED:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_SETCALLPARAMS|LINECALLFEATURE_DROP;
            // We can only answer if a possible media mode is DATAMODEM.
            if (pLineDev->dwCurMediaModes & LINEMEDIAMODE_DATAMODEM) {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_ANSWER;
            }
            break;
        
        case LINECALLSTATE_CONNECTED:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_SETCALLPARAMS |
            LINECALLFEATURE_DROP;
            break;
        
        case LINECALLSTATE_UNKNOWN:
        case LINECALLSTATE_PROCEEDING:
        case LINECALLSTATE_DISCONNECTED:
            lpCallStatus->dwCallFeatures  = LINECALLFEATURE_DROP;
            break;
        
        case LINECALLSTATE_IDLE:
        default:
            lpCallStatus->dwCallFeatures  = 0;
            break;
        }
    }

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetCallStatus %x\n"), rc));
    return rc;
}   // TSPI_lineGetCallStatus


static const WCHAR szProviderName[] = TEXT("UNIMODEM");

//
// Determine Telephony capabilites for the specified device.
//
// NOTES:
// WinCE UNIMODEM uses a DWORD of DevSpecific information (denoted by
// LINEDEVCAPS.dwDevSpecificSize and LINEDEVCAPS.dwDevSpecificOffset).
// The format of of these 4 bytes is that the first WORD is a device type
// and the second WORD is an "active" indicator (0 for PC card modems that
// have been removed).
//
// The provider name is stored in the ProviderInfo section (denoted by
// LINEDEVCAPS.dwProviderInfoSize and LINEDEVCAPS.dwProviderInfoOffset).
// 
LONG TSPIAPI
TSPI_lineGetDevCaps(
    DWORD dwDeviceID,
    DWORD dwTSPIVersion,
    DWORD dwExtVersion,
    LPLINEDEVCAPS lpLineDevCaps
    )
{
    PTLINEDEV pLineDev;
    int  cbLineNameLen = 0;
    int  cbProviderNameLen = 0;
    int  cbDevClassLen = 0;
    int cbAvailMem = 0;
    DWORD dwRet = SUCCESS;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetDevCaps\n")));

    InitVarData((LPVOID)lpLineDevCaps, sizeof(LINEDEVCAPS));

     // We need to fill in a device caps struct specific to
     // the device.
    if ((pLineDev = GetLineDevfromID ((DWORD)dwDeviceID)) == NULL)
        return LINEERR_NODEVICE;

    // Check to see how much memory we'll need.
    cbLineNameLen = (wcslen(pLineDev->szFriendlyName) + 1) * sizeof(WCHAR);
    cbProviderNameLen = (wcslen(szProviderName) + 1) * sizeof(WCHAR);
    cbDevClassLen = sizeof(g_szzClassList);
    
    cbAvailMem = (int) (lpLineDevCaps->dwTotalSize - lpLineDevCaps->dwUsedSize);
  
    // Enter the size we ideally need.
    lpLineDevCaps->dwNeededSize = lpLineDevCaps->dwUsedSize +
        cbLineNameLen +         // room for linename
        cbProviderNameLen +     // room for provider name
        cbDevClassLen +         // room for device class list
        (2*sizeof(WORD));       // and room for DevSpecific info

    





    if (cbAvailMem >= sizeof(DWORD) )
    {
        *(LPWORD)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwUsedSize) = pLineDev->wDeviceType;
        *((LPWORD)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwUsedSize) + 1) = pLineDev->wDeviceAvail;
        
        DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:TSPI_lineGetDevCaps Storing Device Type x%X, available x%X\n"),
                  pLineDev->wDeviceType,
                  pLineDev->wDeviceAvail));

        lpLineDevCaps->dwDevSpecificSize = sizeof( DWORD );
        lpLineDevCaps->dwDevSpecificOffset = lpLineDevCaps->dwUsedSize;        
        lpLineDevCaps->dwUsedSize += lpLineDevCaps->dwDevSpecificSize;
        cbAvailMem -= lpLineDevCaps->dwDevSpecificSize;
    }
    else
    {
        lpLineDevCaps->dwDevSpecificSize = 0;
        lpLineDevCaps->dwDevSpecificOffset = 0;
        lpLineDevCaps->dwNeededSize += sizeof(WORD);
    }
    
     // If the provider info fits, then also append the line name
    if (cbAvailMem >= cbLineNameLen)
    {
        wcscpy((LPWSTR)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwUsedSize),
                pLineDev->szFriendlyName);
        lpLineDevCaps->dwLineNameSize = cbLineNameLen;
        lpLineDevCaps->dwLineNameOffset = lpLineDevCaps->dwUsedSize;
        lpLineDevCaps->dwUsedSize += cbLineNameLen;
        cbAvailMem -= cbLineNameLen;
    }
    else
    {
        lpLineDevCaps->dwLineNameSize = 0;
        lpLineDevCaps->dwLineNameOffset = 0;
    }

    if (cbAvailMem >= cbProviderNameLen)
    {
        wcscpy((LPWSTR)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwUsedSize), szProviderName);
        lpLineDevCaps->dwProviderInfoSize = cbProviderNameLen;
        lpLineDevCaps->dwProviderInfoOffset = lpLineDevCaps->dwUsedSize;
        lpLineDevCaps->dwUsedSize += cbProviderNameLen;
        cbAvailMem -= cbProviderNameLen;
    }
    else
    {
        lpLineDevCaps->dwProviderInfoSize = 0;
        lpLineDevCaps->dwProviderInfoOffset = 0;
    }
    

     // TODO - We don't have permanent ID's yet.
    lpLineDevCaps->dwPermanentLineID = 0;
    lpLineDevCaps->dwStringFormat = STRINGFORMAT_UNICODE;
    lpLineDevCaps->dwAddressModes = LINEADDRESSMODE_ADDRESSID;
    lpLineDevCaps->dwNumAddresses = 1;

     // Bearer mode & information
    lpLineDevCaps->dwMaxRate      = pLineDev->dwMaxDCERate;
    lpLineDevCaps->dwBearerModes  = pLineDev->dwBearerModes;

     // Media mode
    lpLineDevCaps->dwMediaModes = pLineDev->dwMediaModes;

    // We can simulate wait-for-bong if the modem isn't capable of
    // supporting it.
    lpLineDevCaps->dwDevCapFlags  = pLineDev->dwDevCapFlags |
        LINEDEVCAPFLAGS_DIALBILLING |
        LINEDEVCAPFLAGS_CLOSEDROP;

    lpLineDevCaps->dwRingModes         = 1;
    lpLineDevCaps->dwMaxNumActiveCalls = 1;

     // Line device state to be notified
    lpLineDevCaps->dwLineStates = LINEDEVSTATE_CONNECTED |
        LINEDEVSTATE_DISCONNECTED |
        LINEDEVSTATE_OPEN |
        LINEDEVSTATE_CLOSE |
        LINEDEVSTATE_INSERVICE |
        LINEDEVSTATE_OUTOFSERVICE |
        LINEDEVSTATE_REMOVED |
        LINEDEVSTATE_RINGING |
        LINEDEVSTATE_REINIT;

    lpLineDevCaps->dwLineFeatures = LINEFEATURE_MAKECALL;

    //
    // Don't go beyond older version app's buffer
    //
    if (lpLineDevCaps->dwTotalSize >= sizeof(LINEDEVCAPS)) {
        lpLineDevCaps->dwSettableDevStatus = 0; 
        
        // Copy device classes if it fits
        if (cbAvailMem >= cbDevClassLen) {
            memcpy((LPBYTE)lpLineDevCaps + lpLineDevCaps->dwUsedSize, g_szzClassList, cbDevClassLen);
            lpLineDevCaps->dwDeviceClassesSize  = cbDevClassLen;
            lpLineDevCaps->dwDeviceClassesOffset= lpLineDevCaps->dwUsedSize;
            lpLineDevCaps->dwUsedSize += cbDevClassLen;
            cbAvailMem -= cbDevClassLen;
        } else {
            lpLineDevCaps->dwDeviceClassesSize = 0;
            lpLineDevCaps->dwDeviceClassesOffset = 0;
        }    
    
        lpLineDevCaps->dwAddressTypes = LINEADDRESSTYPE_PHONENUMBER;
        lpLineDevCaps->dwAvailableTracking = 0;
    }

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetDevCaps x%X\n"), dwRet));    
    return dwRet;
}

//
// WINCE NOTE:
// TSPI_lineGetDevConfig returns the default device configuration for the specified device.
// This is done so applications can avoid storing the devcfg when the user wants the default
// settings. If the settings are different than the default, then the application needs to store
// them and set them via lineSetDevConfig whether lineGetDevConfig returns defaults or not.
// 
LONG TSPIAPI
TSPI_lineGetDevConfig(
    DWORD dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR lpszDeviceClass
    )
{
    PTLINEDEV pLineDev;
    DWORD dwRet = SUCCESS;
    BYTE  cbSize;
    PDEVMINICFG pDevMiniCfg;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetDevConfig\n")));

    // Validate the requested device class
    //
    if (lpszDeviceClass != NULL)
    {
        if (!ValidateDevCfgClass(lpszDeviceClass))
            return LINEERR_INVALDEVICECLASS;
    }
    
    // Validate the buffer
    //
    if (lpDeviceConfig == NULL)
        return LINEERR_INVALPOINTER;

    if (lpDeviceConfig->dwTotalSize < sizeof(VARSTRING))
        return LINEERR_STRUCTURETOOSMALL;

    // Validate the device ID
    //
    if ((pLineDev = GetLineDevfromID (dwDeviceID)) == NULL)
        return LINEERR_NODEVICE;
    
    // Validate the buffer size
    //
    cbSize = sizeof(DEVMINICFG);
    lpDeviceConfig->dwUsedSize = sizeof(VARSTRING);
    lpDeviceConfig->dwNeededSize = sizeof(VARSTRING) + cbSize;

    if (lpDeviceConfig->dwTotalSize >= lpDeviceConfig->dwNeededSize)
    {
        pDevMiniCfg = (PDEVMINICFG)(((LPBYTE)lpDeviceConfig) + sizeof(VARSTRING));

        // If someone does a SetDevCfg for some provider who needs a post-dial-terminal
        // and they then want to create a new connection via GetDevCfg/CfgDialogEdit,
        // they will end up with the default connection lookin like the original.
        // To work around this, I will always return the default config here.  If the app
        // needs to remember a specific config for later edit, they must store it themselves.
        // LAM
        getDefaultDevConfig( pLineDev, pDevMiniCfg );
        
        lpDeviceConfig->dwStringFormat = STRINGFORMAT_BINARY;
        lpDeviceConfig->dwStringSize = cbSize;
        lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
        lpDeviceConfig->dwUsedSize += cbSize;

        DEBUGMSG(ZONE_FUNC|ZONE_CALLS,
                 (TEXT("UNIMODEM:TSPI_lineGetDevConfig (DevID %d): dwModemOptions x%X, fwOptions x%X, dwCallSetupFail x%X\n"),
                  dwDeviceID, pDevMiniCfg->dwModemOptions, pDevMiniCfg->fwOptions, pDevMiniCfg->dwCallSetupFailTimer));
    }
    else
    {
        // Not enough room
        DEBUGMSG(ZONE_FUNC,
                 (TEXT("UNIMODEM:TSPI_lineGetDevConfig needed %d bytes, had %d\n"),
                  lpDeviceConfig->dwNeededSize, lpDeviceConfig->dwTotalSize));
    };

    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:-TSPI_lineGetDevConfig x%X (Used %d, Need %d)\n"),
              dwRet, lpDeviceConfig->dwUsedSize, lpDeviceConfig->dwNeededSize));
    return dwRet;
}

typedef HICON (WINAPI * PFN_LOADICON)(HINSTANCE, LPCWSTR);

LONG
TSPIAPI
TSPI_lineGetIcon(
    DWORD    dwDeviceID,
    LPCWSTR   lpszDeviceClass,
    LPHICON  lphIcon
    )
{
    LONG rc;
    PFN_LOADICON pfnLoadIcon;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetIcon\n")));
    rc = LINEERR_RESOURCEUNAVAIL;

    //
    // TODO: First check that the device class corresponds to the icon
    //


    if (TspiGlobals.hIconLine == NULL) {
        if (IsAPIReady(SH_WMGR)) {
            if (pfnLoadIcon = (PFN_LOADICON)GetProcAddress(g_hCoreDLL, L"LoadIconW")) {
                TspiGlobals.hIconLine = pfnLoadIcon(
                                            TspiGlobals.hInstance,
                                            (LPCWSTR)MAKEINTRESOURCE(LINE_ICON)
                                            );
                rc = 0;
            }
        }
    } else {
        rc = 0;
    }
    
    *lphIcon = TspiGlobals.hIconLine;
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetIcon returning %x\n"), rc));
    return rc;
}   // TSPI_lineGetIcon


//
// Get the device ID for the specified device.  The caller
// can use the returned ID with the corresponding media (i.e. for
// serial lines the ID can be passed to ReadFile(), WriteFile(),
// etc).
// 
LONG TSPIAPI
TSPI_lineGetID(
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    DWORD          dwSelect,
    LPVARSTRING    lpDeviceID,
    LPCWSTR        lpszDeviceClass
    )
{
    PTLINEDEV   pLineDev;
    UINT       cbPort;
    UINT       idClass;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetID - ProcPerm=0x%X\n"),
			  GetCurrentPermissions()));

    switch (dwSelect)
    {
        case LINECALLSELECT_ADDRESS:
            if (dwAddressID != 0)
            {
                DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetID - INVALADDRESSID\n")));
                return LINEERR_INVALADDRESSID;
            }
             // FALLTHROUGH

        case LINECALLSELECT_LINE:
            if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL)
            {
                DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetID - INVALLINEHANDLE\n")));
                return LINEERR_INVALLINEHANDLE;
            }
            break;

        case LINECALLSELECT_CALL:
            if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL)
            {
                DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetID - INVALCALLHANDLE\n")));
                return LINEERR_INVALCALLHANDLE;
            }
            break;

        default:
            DEBUGMSG(ZONE_FUNC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID - Invalid dwSelect x%X\n"),
                         dwSelect));
            return LINEERR_OPERATIONFAILED;
    }


     // Determine the device class
     //
    for (idClass = 0; idClass < MAX_SUPPORT_CLASS; idClass++)
    {
        
        DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:TSPI_lineGetID Comparing strings %s to %s\n"),
                  lpszDeviceClass, aGetID[idClass].szClassName));
        if (wcsicmp(lpszDeviceClass, aGetID[idClass].szClassName) == 0)
            break;
    };
	DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:TSPI_lineGetID Class ID = %d (%s)\n"), idClass,
			  aGetID[idClass].szClassName));
    
     // Determine the required size
     //
    switch (idClass)
    {
        case TAPILINE:
            cbPort = sizeof(DWORD);
            break;

        case COMM:
            cbPort = (wcslen(pLineDev->szFriendlyName) + 1) * sizeof(WCHAR);
            break;

        case COMMMODEM:
            cbPort = (wcslen(pLineDev->szFriendlyName) + 1) * sizeof(WCHAR) + sizeof(DWORD);
            break;

        case NDIS:
            cbPort = sizeof(g_szDeviceClass) + sizeof(DWORD);
            break;
            
        default:
            DEBUGMSG(ZONE_FUNC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID - Invalid ID Class x%X\n"),
                      idClass ));
            return LINEERR_OPERATIONFAILED;
    };

     // Calculate the required size
     //
     //lpDeviceID->dwUsedSize = sizeof(VARSTRING); // TAPI fills it in.

    lpDeviceID->dwNeededSize = sizeof(VARSTRING) + cbPort;
    lpDeviceID->dwStringFormat = aGetID[idClass].dwFormat;
    ASSERT(lpDeviceID->dwUsedSize == sizeof(VARSTRING));
    if ((lpDeviceID->dwTotalSize - lpDeviceID->dwUsedSize) <
        cbPort)
    {
        DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetID - Inusufficient space\n")));
        return SUCCESS;
    }
    
     // We have enough space to return valid information
     //
    lpDeviceID->dwStringSize   = cbPort;
    lpDeviceID->dwStringOffset = sizeof(VARSTRING);
    lpDeviceID->dwUsedSize    += cbPort;


     // Return the useful information
    switch (idClass)
    {
        case TAPILINE:
        {
            LPDWORD lpdwDeviceID;

            lpdwDeviceID = (LPDWORD)(((LPBYTE)lpDeviceID) + sizeof(VARSTRING));
            *lpdwDeviceID = (DWORD) pLineDev->dwDeviceID;
            DEBUGMSG(ZONE_MISC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID TAPILINE - Device ID x%X\n"),
                      pLineDev->dwDeviceID));
            break;
        }
        case COMM:
        {
            wcsncpy( (LPWSTR)((LPBYTE)lpDeviceID + sizeof(VARSTRING)),
                      pLineDev->szFriendlyName, (cbPort/sizeof(WCHAR)) );
            DEBUGMSG(ZONE_MISC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID COMM - Device name \"%s\" (len %d)\n"),
                      pLineDev->szFriendlyName, (cbPort/sizeof(WCHAR))));
            break;
        }
        case COMMMODEM:
        {
            LPDWORD lpdwDeviceHandle;

            lpdwDeviceHandle = (LPDWORD)(((LPBYTE)lpDeviceID) + sizeof(VARSTRING));
            if (pLineDev->hDevice != (HANDLE)INVALID_DEVICE)
            {
                *lpdwDeviceHandle = (DWORD)pLineDev->hDevice;
				SetHandleOwner((HANDLE)*lpdwDeviceHandle, GetCallerProcess());
            }
            else
            {
                *lpdwDeviceHandle = (DWORD)NULL;
            };
            wcscpy((LPWSTR)(lpdwDeviceHandle+1), pLineDev->szFriendlyName );
            DEBUGMSG(ZONE_MISC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID COMMMODEM - Device Handle x%X, Device name %s\n"),
                      *lpdwDeviceHandle, pLineDev->szFriendlyName));
            break;
        }
        case NDIS:
        {
            LPDWORD lpdwDeviceID;

            lpdwDeviceID = (LPDWORD)(((LPBYTE)lpDeviceID) + sizeof(VARSTRING));
            *lpdwDeviceID = (pLineDev->hDevice != (HANDLE)INVALID_DEVICE ?
                             (DWORD)pLineDev->hDevice_r0 : (DWORD)NULL);
            wcscpy((LPWSTR)(lpdwDeviceID+1), g_szDeviceClass);
            DEBUGMSG(ZONE_MISC,
                     (TEXT("UNIMODEM:-TSPI_lineGetID NDIS - Device Handle x%X, Device name %s\n"),
                      *lpdwDeviceID, g_szDeviceClass));
            break;
        }
    };

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetID\n")));
    return SUCCESS;
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    LONG rc;
    PTLINEDEV  pLineDev;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetLineDevStatus\n")));
    
    InitVarData((LPVOID)lpLineDevStatus, sizeof(LINEDEVSTATUS));
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetLineDevStatus ** Invalid Handle\n")));
        rc = LINEERR_INVALLINEHANDLE;
        goto exitPoint;
    }
    
    rc = 0;

    if (pLineDev->dwCallFlags & CALL_ACTIVE) {
        lpLineDevStatus->dwNumActiveCalls = 1;
        lpLineDevStatus->dwLineFeatures = 0;
    } else {
        lpLineDevStatus->dwNumActiveCalls = 0;
        lpLineDevStatus->dwLineFeatures = (pLineDev->dwCallFlags & CALL_ALLOCATED) ?
                                            0 : LINEFEATURE_MAKECALL;
    }
    
    // Line hardware information
    //
    lpLineDevStatus->dwSignalLevel  = 0x0000FFFF;
    lpLineDevStatus->dwBatteryLevel = 0x0000FFFF;
    lpLineDevStatus->dwRoamMode     = LINEROAMMODE_UNAVAIL;
    
    // Always allow TAPI calls
    //
    lpLineDevStatus->dwDevStatusFlags = LINEDEVSTATUSFLAGS_CONNECTED;

    /* TODO
    if (!(pLineDev->fdwResources & LINEDEVFLAGS_OUTOFSERVICE)) {
        lpLineDevStatus->dwDevStatusFlags |= LINEDEVSTATUSFLAGS_INSERVICE;
    }      */
    
exitPoint:    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetLineDevStatus 0x%x\n"), rc));
    return rc;
}   // TSPI_lineGetLineDevStatus



LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE hdLine,
    LPDWORD lpdwNumAddressIDs
    )
{
    LONG rc;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetNumAddressIDs\n")));

    if (GetLineDevfromHandle ((DWORD)hdLine) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineGetNumAddressIDs ** Invalid Handle\n")));
        rc = LINEERR_INVALLINEHANDLE;
    } else {
        rc = 0;
        *lpdwNumAddressIDs = 1;
    }

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetNumAddressIDs 0x%x\n"), rc));
    return rc;
}   // TSPI_lineGetNumAddressIDs


LONG TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID          dwRequestID,
    HDRVLINE               hdLine,
    HTAPICALL              htCall,
    LPHDRVCALL             lphdCall,
    LPCWSTR                lpszDestAddress,
    DWORD                  dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    )
{
    PTLINEDEV pLineDev;
    DWORD dwRet;
    BOOL  fDoTakeover = FALSE;
    WCHAR szSuffix[16];
    DWORD dwSize;

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:+TSPI_lineMakeCall\n")));

    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdLine)) == NULL) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                 (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Handle\n")));
        return LINEERR_INVALLINEHANDLE;
    }
    
     // See if we have a free call struct.
    if (pLineDev->dwCallFlags & CALL_ALLOCATED) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                 (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Call already allocated\n")));
        return LINEERR_CALLUNAVAIL;
    }    

    // By default, don't do blind dialing.
    pLineDev->dwDialOptions &= ~MDM_BLIND_DIAL;

     // Examine LINECALLPARAMS, if present
    if (lpCallParams) {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                 (TEXT("UNIMODEM:TSPI_lineMakeCall - Check CallParams\n")));
        
         // verify media mode
        if (lpCallParams->dwMediaMode & ~pLineDev->dwMediaModes) {
            DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                     (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Media Mode\n")));
            return LINEERR_INVALMEDIAMODE;
        }
        DEBUGMSG(ZONE_MISC,
                 (TEXT("UNIMODEM:TSPI_lineMakeCall - verified media modes\n")));
        
         // verify bearer mode
        if ((~pLineDev->dwBearerModes) & lpCallParams->dwBearerMode) {
            DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                     (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Bearer Mode 0x%x vs 0x%x\n"),
                      pLineDev->dwBearerModes, lpCallParams->dwBearerMode));
            return LINEERR_INVALBEARERMODE;
        }

        if (lpCallParams->dwBearerMode & LINEBEARERMODE_PASSTHROUGH) {
            fDoTakeover = TRUE;
        } else {
             // We're not requested to do passthrough.  Can we actually
             // dial the media modes without passthrough?  This is to
             // prevent G3FAX from being used without passthrough...
             // (We can only dial with DATAMODEM)
            if ((lpCallParams->dwMediaMode &
                 (LINEMEDIAMODE_DATAMODEM)) == 0) {
                DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                         (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Media Mode\n")));
                return LINEERR_INVALMEDIAMODE;
            }
        }

        pLineDev->dwCurBearerModes = lpCallParams->dwBearerMode;
        pLineDev->dwCurMediaModes = lpCallParams->dwMediaMode;

        DEBUGMSG(ZONE_MISC,
                 (TEXT("UNIMODEM:TSPI_lineMakeCall - got media & bearer modes\n")));

        if (!(lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_IDLE)) {
            DEBUGMSG(ZONE_FUNC|ZONE_CALLS,
             (TEXT("UNIMODEM:TSPI_lineMakeCall LINECALLPARAMFLAGS_IDLE: not set, no dialtone detect\n") ));
             // Turn on blind dialing
            pLineDev->dwDialOptions |= MDM_BLIND_DIAL;
        }

        if ((lpCallParams->dwAddressType) && (lpCallParams->dwAddressType != LINEADDRESSTYPE_PHONENUMBER)) {
            DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                     (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Address Type\n")));
            return LINEERR_INVALADDRESS;
        }

        

            
    } else {
         // set the standard defaults - for peg tapi it is DATAMODEM
        ASSERT(pLineDev->dwMediaModes & LINEMEDIAMODE_DATAMODEM);
        pLineDev->dwCurMediaModes = LINEMEDIAMODE_DATAMODEM;

        pLineDev->dwCurBearerModes = pLineDev->dwBearerModes & ~LINEBEARERMODE_PASSTHROUGH;
    }

     // Do we have a phone number?
     //
    if (!fDoTakeover)
    {
        if (IS_NULL_MODEM(pLineDev) ||
            pLineDev->DevMiniCfg.fwOptions & MANUAL_DIAL)
        {
            *pLineDev->szAddress = '\0';

            DEBUGMSG(ZONE_MISC|ZONE_CALLS,
                     (TEXT("UNIMODEM:TSPI_lineMakeCall - Manual dial, zeroing dial string\n")));

             // Turn on blind dialing if this is MANUAL_DIAL.
            if (pLineDev->DevMiniCfg.fwOptions & MANUAL_DIAL)
            {
                pLineDev->dwDialOptions |= MDM_BLIND_DIAL;
            }
        }
        else
        {
             // Validate lpszDestAddress and get the processed form of it.
            DEBUGMSG(ZONE_MISC|ZONE_CALLS,
                     (TEXT("UNIMODEM:TSPI_lineMakeCall - validating destination address\n")));
            dwRet = ValidateAddress(pLineDev, lpszDestAddress, pLineDev->szAddress);
            if (SUCCESS != dwRet)
            {
                DEBUGMSG(ZONE_FUNC|ZONE_ERROR|ZONE_CALLS,
                         (TEXT("UNIMODEM:-TSPI_lineMakeCall ** Invalid Address\n")));
                return dwRet;
            }
    
             // if the lpszDestAddress was NULL or "", then we just want to do a
             // dialtone detection.  We expect that lineDial will be called.
             // Setting the szAddress to ";" will do this.
            if ('\0' == pLineDev->szAddress[0])
            {
                dwSize = sizeof(szSuffix);
                if (ERROR_SUCCESS != MdmRegGetValue( pLineDev,
                                         szSettings,
                                         szDialSuffix,
                                         REG_SZ,
                                         (PUCHAR)szSuffix,
                                         &dwSize) )
                {
                    wcscpy(szSuffix, TEXT(";"));
                }
                
                wcscpy(pLineDev->szAddress, szSuffix);
            }
        }
    }

     // Record the call attributes
    pLineDev->htCall = htCall;
    pLineDev->dwCallFlags = CALL_ALLOCATED;

    *lphdCall = (HDRVCALL)pLineDev;

     // We allow making a call to an already-opened line if the line is monitoring
     // a call. Therefore, if the line is in use, try making a call. The make-call
     // routine will return error if the state is not appropriate.
     //
    dwRet = DevlineOpen(pLineDev);
    if ((dwRet == SUCCESS) || (dwRet == LINEERR_ALLOCATED)) {
        if (fDoTakeover) {
            DEBUGMSG(ZONE_MISC|ZONE_CALLS ,(TEXT("UNIMODEM:TSPI_lineMakeCall - Takeover\n")));
            
            // For takeover, we don't actually do any calling.  Just open the
            // port so that the application can get the device handle from
            // lineGetID.
            if ((pLineDev->DevState == DEVST_DISCONNECTED) ||
                (pLineDev->DevState == DEVST_PORTLISTENING)) {
                // We can only go into passthrough if device is not in use

                // OK, the device was opened above, so now we just need to
                // let the apps know that the callstate has changed.
                pLineDev->dwCallState = LINECALLSTATE_UNKNOWN;
                pLineDev->DevState = DEVST_CONNECTED;
                pLineDev->fTakeoverMode = TRUE;
                SetAsyncOp(pLineDev, PENDING_LINEMAKECALL);
                SetAsyncStatus(pLineDev, 0);
                NewCallState(pLineDev, LINECALLSTATE_CONNECTED, LINECONNECTEDMODE_ACTIVE);
                dwRet = 0;
            } else {
                dwRet = LINEERR_OPERATIONFAILED;
            }
        } else {
            pLineDev->DevState = DEVST_PORTCONNECTDIAL;
            dwRet = ControlThreadCmd(pLineDev, PENDING_LINEMAKECALL, dwRequestID);
        }
    }

     // Check if an error occurs
     //
    if (IS_TAPI_ERROR(dwRet))
    {
        DEBUGMSG(ZONE_CALLS,
                 (TEXT("UNIMODEM:TSPI_lineMakeCall Ret Code x%X, invalidating pending ID.\n"),
                  dwRet ));
        
         // Deallocate the call from this line
        pLineDev->htCall = NULL;
        pLineDev->dwCallFlags = 0;
        *lphdCall = NULL;
    } else {
        dwRet = SetAsyncID(pLineDev, dwRequestID);
    }

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:-TSPI_lineMakeCall, dwRet x%X\n"), dwRet));    
    return dwRet;    
}   // TSPI_lineMakeCall


LONG TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD dwDeviceID,
    DWORD dwLowVersion,
    DWORD dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineNegotiateTSPIVersion\n")));
    

     // Check the range of the device ID
     //
    if ((dwDeviceID == INITIALIZE_NEGOTIATION) || (GetLineDevfromID(dwDeviceID) != NULL))
    {
         // Check the version range
         //
        if((dwLowVersion > SPI_VERSION) || (dwHighVersion < SPI_VERSION))
        {
            *lpdwTSPIVersion = 0;
            DEBUGMSG(ZONE_FUNC,
                     (TEXT("UNIMODEM:-TSPI_lineNegotiateTSPIVersion - SPI Ver x%X out of TAPI range x%X..x%X\n"),
                      SPI_VERSION, dwLowVersion, dwHighVersion));
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }
        else
        {
            *lpdwTSPIVersion = SPI_VERSION;
            DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:-TSPI_lineNegotiateTSPIVersion - Ver x%X\n"),
              SPI_VERSION));
            return SUCCESS;
        };
    };

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineNegotiateTSPIVersion - No Device\n")));

     // The requested device doesn't exist.
    return LINEERR_NODEVICE;
}

LONG TSPIAPI
TSPI_lineOpen(
    DWORD dwDeviceID,
    HTAPILINE htLine,
    LPHDRVLINE lphdLine,
    DWORD dwTSPIVersion,
    LINEEVENT lineEventProc)
{
    PTLINEDEV pLineDev;
    
    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:+TSPI_lineOpen DevID x%X, HTapiLine x%X, SPI Ver x%X\n"),
              dwDeviceID, htLine, dwTSPIVersion));

     // Validate the device ID
    if ((pLineDev = GetLineDevfromID(dwDeviceID)) == NULL) {
        DEBUGMSG(ZONE_ERROR,
                 (TEXT("TSPI_lineOpen, could not find device for dwId x%X\n"),
                  dwDeviceID));
        return LINEERR_NODEVICE;
    }

    EnterCriticalSection(&pLineDev->OpenCS);

     // Update the line device
    *lphdLine           = (HDRVLINE)pLineDev;
    pLineDev->lpfnEvent = lineEventProc;
    DEBUGMSG((lineEventProc != TspiGlobals.fnLineEventProc) & ZONE_WARN,
        (TEXT("UNIMODEM:TSPI_lineOpen lineEventProc != TspiGlobals.fnLineEventProc\n")));
    pLineDev->htLine    = htLine;

    LeaveCriticalSection(&pLineDev->OpenCS);

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineOpen\n")));
    return SUCCESS;
}

LONG TSPIAPI
TSPI_lineSetDevConfig(
    DWORD dwDeviceID,
    LPVOID const lpDeviceConfig,
    DWORD dwSize,
    LPCWSTR lpszDeviceClass
    )
{
    PTLINEDEV    pLineDev;
    PDEVMINICFG  pDevMiniCfg;
#ifdef DEBUG
    DWORD        dwTemp;
#endif
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineSetDevConfig\n")));

     // Validate the requested device class
     //
    if (!ValidateDevCfgClass(lpszDeviceClass))
    {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR,
                 (TEXT("UNIMODEM:-TSPI_lineSetDevConfig : LINEERR_INVALDEVICECLASS\n")));
        return LINEERR_INVALDEVICECLASS;
    }
    
     // Validate the buffer
     //
    if (lpDeviceConfig == NULL)
    {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR,
                 (TEXT("UNIMODEM:-TSPI_lineSetDevConfig : LINEERR_INVALPOINTER\n")));
        return LINEERR_INVALPOINTER;
    }
    
     // Validate the device ID
     //
    if ((pLineDev = GetLineDevfromID(dwDeviceID)) == NULL)
    {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR,
                 (TEXT("UNIMODEM:-TSPI_lineSetDevConfig : LINEERR_NODEVICE\n")));
        return LINEERR_NODEVICE;
    }
    
     // verify the structure size and version
     //
    pDevMiniCfg = (PDEVMINICFG)lpDeviceConfig;
    if ( (dwSize != sizeof(DEVMINICFG)) ||
         (pLineDev->DevMiniCfg.wVersion != (pDevMiniCfg->wVersion)) )
    {
        DEBUGMSG(ZONE_FUNC|ZONE_ERROR,
                 (TEXT("UNIMODEM:-TSPI_lineSetDevConfig : LINEERR_INVALPARAM\n")));
        return LINEERR_INVALPARAM;
    }
    
     // Get the new settings
    pLineDev->DevMiniCfg = *pDevMiniCfg;   

#ifdef DEBUG
    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:DevConfig set :\n")));
    if( MDM_FLOWCONTROL_HARD & pLineDev->DevMiniCfg.dwModemOptions )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tHardware Flow Control\n")));
    else if( MDM_FLOWCONTROL_SOFT & pLineDev->DevMiniCfg.dwModemOptions )    
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tSoftware Flow Control\n")));
    else
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tNo Flow Control\n")));

    if (MANUAL_DIAL & pLineDev->DevMiniCfg.fwOptions )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tManual Dialing\n")));
    if (TERMINAL_PRE & pLineDev->DevMiniCfg.fwOptions )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tPredial Terminal\n")));
    if (TERMINAL_POST & pLineDev->DevMiniCfg.fwOptions )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tPostdial Terminal\n")));
    if ( MDM_BLIND_DIAL & pLineDev->DevMiniCfg.dwModemOptions )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:\tBlind Dialing\n")));    
    dwTemp = wcslen(pLineDev->DevMiniCfg.szDialModifier);
    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:  Dial Modifier len %d\n"),
                            dwTemp));
    if( dwTemp )
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:  Dial Modifier string %s\n"),
                                pLineDev->DevMiniCfg.szDialModifier));
#endif
    

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineSetDevConfig\n")));
    return SUCCESS;
}


//
// Enable the opened line to detect an inbound call.
//
LONG TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE hdLine,
    DWORD dwMediaModes
    )
{
    PTLINEDEV  pLineDev;
    LONG rc = LINEERR_OPERATIONFAILED; // assume failure
    DWORD dwOrigMediaModes;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineSetDefaultMediaDetection\n")));
    
    pLineDev = GetLineDevfromHandle((DWORD)hdLine);
    if (!pLineDev) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineSetDefaultMediaDetection: invalid hdLine 0x%x\n"), hdLine));
        rc =  LINEERR_INVALLINEHANDLE;
        goto exitPoint;
    }
    
    // Check the requested modes. There must be only our media modes.
    // In addition, don't allow INTERACTIVEVOICE to be used for listening.
    //
    if (dwMediaModes & ~(pLineDev->dwMediaModes & ~LINEMEDIAMODE_INTERACTIVEVOICE)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineSetDefaultMediaDetection: invalid dwMediaModes 0x%x\n"), dwMediaModes));
        rc = LINEERR_INVALMEDIAMODE;
        goto exitPoint;
    }
    EnterCriticalSection(&pLineDev->OpenCS);
    LeaveCriticalSection(&pLineDev->OpenCS);
    
    //
    // If no detection and a detection is requested
    //
    dwOrigMediaModes = pLineDev->dwDetMediaModes;
    if ((dwOrigMediaModes == 0) && (dwMediaModes)) {
        pLineDev->dwDetMediaModes = dwMediaModes;
        rc = ControlThreadCmd(pLineDev, PENDING_LISTEN, INVALID_PENDINGID);
        if (rc) {
            pLineDev->dwDetMediaModes = dwOrigMediaModes;
        }
    } else {
        //
        // we are stopping detection OR adjusting the detection media modes
        //

        //
        // If we are detecting and requested not to, then stop the control thread
        //
        if (dwOrigMediaModes && (dwMediaModes == 0) &&
            (DEVST_PORTLISTENING == pLineDev->DevState ||
            DEVST_PORTLISTENINIT == pLineDev->DevState)) {
            pLineDev->dwDetMediaModes = 0;
            ControlThreadCmd(pLineDev, PENDING_EXIT, INVALID_PENDINGID);
        }
        pLineDev->dwDetMediaModes = dwMediaModes;
        
        rc = ERROR_SUCCESS;  
    }

exitPoint:    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineSetDefaultMediaDetection 0x%x\n"), rc));
    return rc;
}   //  TSPI_lineSetDefaultMediaDetection


LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL hdCall,
    DWORD dwMediaMode
    )
{
    LONG rc;
    PTLINEDEV pLineDev;
    
    
    if ((pLineDev = GetLineDevfromHandle ((DWORD)hdCall)) == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineSetMediaMode ** Invalid Handle\n")));
        rc = LINEERR_INVALCALLHANDLE;
        goto exitPoint;
    }
    
    // Check the requested modes. There must be only our media modes
    //
    if (dwMediaMode & ~pLineDev->dwMediaModes) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineSetMediaMode ** Invalid dwMediaMode\n")));
        rc = LINEERR_INVALMEDIAMODE;
        goto exitPoint;
    }
    
    
    rc = 0;
    if (pLineDev->dwCurMediaModes != dwMediaMode) {
        pLineDev->dwCurMediaModes = dwMediaMode;
        CallLineEventProc(
            pLineDev,
            pLineDev->htCall,
            LINE_CALLINFO,
            LINECALLINFOSTATE_MEDIAMODE,
            0,
            0
            );
    }
    
exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineSetMediaMode 0x%x\n"), rc));
    return rc;
}


LONG TSPIAPI
TSPI_lineSetStatusMessages(
    HDRVLINE hdLine,
    DWORD dwLineStates,
    DWORD dwAddressStates
    )
{
    PTLINEDEV  pLineDev;
    LONG rc;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineSetStatusMessages\n")));

    if (pLineDev = GetLineDevfromHandle((DWORD)hdLine)) {
        rc = 0;
        pLineDev->dwLineStatesMask = dwLineStates;
        // We don't send LINE_ADDRESSSTATE msgs so no need to filter them.
    } else {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:TSPI_lineSetStatusMessages ** Invalid Handle\n")));
        rc =  LINEERR_INVALLINEHANDLE;
    }

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineSetStatusMessages 0x%x\n"), rc));
    return rc;
}

//
// LONG TSPIAPI TSPI_providerCreateLineDevice()
//
// Dynamically creates a new device.  This entry point will be
// called by devloader whenver he adds a new device which lists
// unimodem.dll as the service provider.  
//
LONG TSPIAPI
TSPI_providerCreateLineDevice(
    HKEY    hActiveKey,    // @parm Registry key for this active device
    LPCWSTR lpszDevPath,   // @parm Registry path for this device
    LPCWSTR lpszDeviceName // @parm Device Name
    )
{
    PTLINEDEV ptLineDev;
    DWORD   dwRet;
    
    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:+TSPI_providerCreateLineDevice %s.\n"),
              lpszDeviceName));

    dwRet = (DWORD) -1;     // Assume failure

    // It is possible that this device already exists (for example
    // if it is a PCMCIA card that was removed and re-inserted.)
    // So scan the current device list looking for it, and use
    // the existing entry if it is found.  Otherwise, go ahead and
    // create a device.
    ptLineDev = createLineDev( hActiveKey, lpszDevPath, lpszDeviceName );
    
    if( NULL != ptLineDev )
    {
        dwRet = ptLineDev->dwDeviceID;
    }
    
    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:-TSPI_providerCreateLineDevice, return x%X.\n"),
              dwRet));
    return dwRet;
}

//
// LONG TSPIAPI TSPI_providerDeleteLineDevice()
//
// Removes a device from the system.
//
// NOTE : Devload doesn't actually know which devices we added,
// so it is possible that this device is not one for which we
// have created a lineDevice entry. Note that we never really remove
// a device.  We simply mark it as disconnected.  It may later get
// re-added to the system.
//
LONG TSPIAPI TSPI_providerDeleteLineDevice(
    DWORD Identifier    // @parm dwDeviceID associated with this device
    )
{
    DWORD   dwRet;
    PTLINEDEV ptLineDev;
    
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:+TSPI_providerDeleteLineDevice, x%X.\n"),
              Identifier));

     // Get the name of the device and check its validity
    if ( (DWORD)-1 == Identifier )
        return LINEERR_BADDEVICEID;
     
    dwRet = LINEERR_OPERATIONFAILED;     // Assume failure

     // See if we actually have such a device in our system
    if( ptLineDev = GetLineDevfromID(Identifier) )
    {
        if (ptLineDev->wDeviceAvail) {
            EnterCriticalSection(&ptLineDev->OpenCS);
            ptLineDev->DevState = DEVST_DISCONNECTED;
            ptLineDev->dwDetMediaModes = 0;
            LeaveCriticalSection(&ptLineDev->OpenCS);

            // OK, send a DEVSTATE_DISCONNECTED to TAPI.
            DEBUGMSG(ZONE_FUNC|ZONE_INIT,
                     (TEXT("UNIMODEM:TSPI_providerDeleteLineDevice Send disconnected for ptLine x%X, htLine x%X\n"),
                      ptLineDev, ptLineDev->htLine));
            
            CallLineEventProc(
                ptLineDev,
                0,
                LINE_LINEDEVSTATE,
                LINEDEVSTATE_DISCONNECTED|LINEDEVSTATE_OUTOFSERVICE|LINEDEVSTATE_REMOVED,
                0,
                0
                );
            
            // And this Setting Key is no longer valid.
            RegCloseKey( ptLineDev->hSettingsKey );        
    
            // And mark this as disconnected so that GetDevCaps can return this info.
            ptLineDev->wDeviceAvail = 0;
            dwRet = 0;
        }
        
    } else {
        
        DEBUGMSG(ZONE_FUNC|ZONE_INIT,
                 (TEXT("UNIMODEM:-TSPI_providerDeleteLineDevice, invalid device id x%X.\n"),
                  Identifier));
    }
    
    DEBUGMSG(ZONE_FUNC|ZONE_INIT,
             (TEXT("UNIMODEM:-TSPI_providerDeleteLineDevice.\n")));
    return dwRet;
}


LONG
TSPIAPI
TSPI_providerRemoveDevice(
    LPCWSTR lpszDeviceName
    )
{
    PTLINEDEV ptLineDev;
    LONG rc = LINEERR_OPERATIONFAILED;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_providerRemoveDevice\n")));
     // See if we actually have such a device in our system
    if (ptLineDev = GetLineDevfromName(lpszDeviceName, NULL)) {
        rc = TSPI_providerDeleteLineDevice(ptLineDev->dwDeviceID);
    }

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_providerRemoveDevice returning %x\n"), rc));
    return rc;
}   // TSPI_providerRemoveDevice


BOOL
DllEntry (HANDLE  hinstDLL,
		  DWORD   Op,
		  LPVOID  lpvReserved)
{
	switch (Op) {
        case DLL_PROCESS_ATTACH :
            DEBUGREGISTER(hinstDLL);
            DEBUGMSG (ZONE_FUNC, (TEXT("UNIMODEM:DllEntry(ProcessAttach)\n")));

             // Now lets init TSPIGlobals, etc.
            TSPIDLL_Load( );
            TspiGlobals.hInstance = hinstDLL;   // Instance handle needed to
                                                // load dialog resources
            break;
            
        case DLL_PROCESS_DETACH :
        case DLL_THREAD_DETACH :
        case DLL_THREAD_ATTACH :
        default :
            break;
	}
	return TRUE;
}


// **********************************************************************
// Now we need to provide a vtbl that can be used to access our functions
// **********************************************************************

TSPI_PROCS tspi_procs;

LONG TSPIAPI TSPI_lineGetProcTable(
    LPTSPI_PROCS *lplpTspiProcs
    )
{
    PDWORD pdw;
    LONG i;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+TSPI_lineGetProcTable, ptr = x%X\n"), &tspi_procs));




    if (tspi_procs.TSPI_lineClose != TSPI_lineClose) {

        for (pdw = (PDWORD)&tspi_procs, i = 0;
            i < sizeof(TSPI_PROCS)/sizeof(DWORD);
            pdw++,i++) {
            *pdw = (DWORD)TSPI_Unsupported;
        }
    
        tspi_procs.TSPI_lineAccept =                    TSPI_lineAccept;
        tspi_procs.TSPI_lineAnswer =                    TSPI_lineAnswer;
        tspi_procs.TSPI_lineClose =                     TSPI_lineClose;
        tspi_procs.TSPI_lineCloseCall =                 TSPI_lineCloseCall;
        tspi_procs.TSPI_lineConditionalMediaDetection = TSPI_lineConditionalMediaDetection;
        tspi_procs.TSPI_lineDevSpecific =               TSPI_lineDevSpecific;
        tspi_procs.TSPI_lineDial =                      TSPI_lineDial;
        tspi_procs.TSPI_lineDrop =                      TSPI_lineDrop;
        tspi_procs.TSPI_lineGetAddressCaps =            TSPI_lineGetAddressCaps;
        tspi_procs.TSPI_lineGetAddressStatus =          TSPI_lineGetAddressStatus;
        tspi_procs.TSPI_lineGetCallInfo =               TSPI_lineGetCallInfo;
        tspi_procs.TSPI_lineGetCallStatus =             TSPI_lineGetCallStatus;
        tspi_procs.TSPI_lineGetDevCaps =                TSPI_lineGetDevCaps;
        tspi_procs.TSPI_lineGetDevConfig =              TSPI_lineGetDevConfig;
        tspi_procs.TSPI_lineGetIcon =                   TSPI_lineGetIcon;
        tspi_procs.TSPI_lineGetID =                     TSPI_lineGetID;
        tspi_procs.TSPI_lineGetLineDevStatus =          TSPI_lineGetLineDevStatus;
        tspi_procs.TSPI_lineGetNumAddressIDs =          TSPI_lineGetNumAddressIDs;
        tspi_procs.TSPI_lineMakeCall =                  TSPI_lineMakeCall;
        tspi_procs.TSPI_lineNegotiateTSPIVersion =      TSPI_lineNegotiateTSPIVersion;
        tspi_procs.TSPI_lineOpen =                      TSPI_lineOpen;
        tspi_procs.TSPI_lineSetDevConfig =              TSPI_lineSetDevConfig;
        tspi_procs.TSPI_lineSetMediaMode =              TSPI_lineSetMediaMode;
        tspi_procs.TSPI_lineSetStatusMessages =         TSPI_lineSetStatusMessages;
        tspi_procs.TSPI_providerInit =                  TSPI_providerInit;
        tspi_procs.TSPI_providerInstall =               TSPI_providerInstall;
        tspi_procs.TSPI_providerShutdown =              TSPI_providerShutdown;
        tspi_procs.TSPI_providerEnumDevices =           TSPI_providerEnumDevices;
        tspi_procs.TSPI_providerCreateLineDevice =      TSPI_providerCreateLineDevice;
        tspi_procs.TSPI_providerDeleteLineDevice =      TSPI_providerDeleteLineDevice;
        tspi_procs.TSPI_lineConfigDialogEdit =          TSPI_lineConfigDialogEdit;
        tspi_procs.TSPI_providerRemoveDevice =          TSPI_providerRemoveDevice;
        tspi_procs.TSPI_lineSetDefaultMediaDetection =  TSPI_lineSetDefaultMediaDetection;
    }

    *lplpTspiProcs = &tspi_procs;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-TSPI_lineGetProcTable, ptr = x%X\n"), &tspi_procs));
    return SUCCESS;
}
