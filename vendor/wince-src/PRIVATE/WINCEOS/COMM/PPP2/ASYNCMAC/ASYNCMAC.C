/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include "windows.h"
#include "tapi.h"
#include "ndis.h"
#include "ndiswan.h"
#include "ndistapi.h"
#include "asyncmac.h"
#include "frame.h"

#include "cclib.h"

#define DEV_CLASS_COMM_DATAMODEM TEXT("comm/datamodem")

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("AsyncMac"), {
    TEXT("Init"),     TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Send"),		TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Tapi"),     TEXT("Recv"),     TEXT("Interface"),TEXT("Misc"),
    TEXT("Alloc"),    TEXT("Function"), TEXT("Warning"),  TEXT("Error") },
    0x0000C000
};
#endif // DEBUG

HINSTANCE			v_hInstance;

NDIS_HANDLE			v_hNdisWrapper;
PDRIVER_OBJECT		v_AsyncDriverObject;
DWORD				v_GlobalAdapterCount;

// Because this driver only supports a single adapter, it is unnecessary to have
// a list of active adapters.  Instead, a single pointer to the active adapter (NULL
// if none active) is sufficient.  

PASYNCMAC_ADAPTER	v_pAdapter;
CRITICAL_SECTION	v_AdapterCS;


NDIS_OID SupportedOids[] = {
    OID_GEN_MAC_OPTIONS,
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_LINK_SPEED,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,

    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
	
    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_GET_INFO,
    OID_WAN_GET_LINK_INFO,
    OID_WAN_SET_LINK_INFO,
    OID_WAN_MEDIUM_SUBTYPE,
    OID_WAN_HEADER_FORMAT,
	

/* NOT Supported yet	

    OID_TAPI_GET_ADDRESS_CAPS,
    OID_TAPI_GET_EXTENSION_ID,
    OID_TAPI_NEGOTIATE_EXT_VERSION,
	*/
	
    OID_TAPI_GET_DEV_CAPS,
    OID_TAPI_PROVIDER_INITIALIZE,
	OID_TAPI_TRANSLATE_ADDRESS,
    OID_TAPI_OPEN,
	OID_TAPI_MAKE_CALL,
	OID_TAPI_DROP,
	OID_TAPI_CLOSE_CALL,
    OID_TAPI_CLOSE,

	/*
    OID_WAN_GET_BRIDGE_INFO,
    OID_WAN_GET_COMP_INFO,
    OID_WAN_GET_STATS_INFO,
    OID_WAN_LINE_COUNT,
    OID_WAN_PROTOCOL_TYPE,
    OID_WAN_QUALITY_OF_SERVICE,
    OID_WAN_SET_BRIDGE_INFO,
    OID_WAN_SET_COMP_INFO,
*/
};

#if DEBUG
PUCHAR
GetOidString(
    NDIS_OID Oid
    )
{
    PUCHAR OidName = NULL;
    #define OID_CASE(oid) case (oid): OidName = #oid; break
    switch (Oid)
    {
        OID_CASE(OID_GEN_CURRENT_LOOKAHEAD);
        OID_CASE(OID_GEN_DRIVER_VERSION);
        OID_CASE(OID_GEN_HARDWARE_STATUS);
        OID_CASE(OID_GEN_LINK_SPEED);
        OID_CASE(OID_GEN_MAC_OPTIONS);
        OID_CASE(OID_GEN_MAXIMUM_LOOKAHEAD);
        OID_CASE(OID_GEN_MAXIMUM_FRAME_SIZE);
        OID_CASE(OID_GEN_MAXIMUM_TOTAL_SIZE);
        OID_CASE(OID_GEN_MEDIA_SUPPORTED);
        OID_CASE(OID_GEN_MEDIA_IN_USE);
        OID_CASE(OID_GEN_RECEIVE_BLOCK_SIZE);
        OID_CASE(OID_GEN_RECEIVE_BUFFER_SPACE);
        OID_CASE(OID_GEN_SUPPORTED_LIST);
        OID_CASE(OID_GEN_TRANSMIT_BLOCK_SIZE);
        OID_CASE(OID_GEN_TRANSMIT_BUFFER_SPACE);
        OID_CASE(OID_GEN_VENDOR_DESCRIPTION);
        OID_CASE(OID_GEN_VENDOR_ID);
		OID_CASE(OID_802_3_CURRENT_ADDRESS);
        OID_CASE(OID_TAPI_ACCEPT);
        OID_CASE(OID_TAPI_ANSWER);
        OID_CASE(OID_TAPI_CLOSE);
        OID_CASE(OID_TAPI_CLOSE_CALL);
        OID_CASE(OID_TAPI_CONDITIONAL_MEDIA_DETECTION);
        OID_CASE(OID_TAPI_CONFIG_DIALOG);
        OID_CASE(OID_TAPI_DEV_SPECIFIC);
        OID_CASE(OID_TAPI_DIAL);
        OID_CASE(OID_TAPI_DROP);
        OID_CASE(OID_TAPI_GET_ADDRESS_CAPS);
        OID_CASE(OID_TAPI_GET_ADDRESS_ID);
        OID_CASE(OID_TAPI_GET_ADDRESS_STATUS);
        OID_CASE(OID_TAPI_GET_CALL_ADDRESS_ID);
        OID_CASE(OID_TAPI_GET_CALL_INFO);
        OID_CASE(OID_TAPI_GET_CALL_STATUS);
        OID_CASE(OID_TAPI_GET_DEV_CAPS);
        OID_CASE(OID_TAPI_GET_DEV_CONFIG);
        OID_CASE(OID_TAPI_GET_EXTENSION_ID);
        OID_CASE(OID_TAPI_GET_ID);
        OID_CASE(OID_TAPI_GET_LINE_DEV_STATUS);
        OID_CASE(OID_TAPI_MAKE_CALL);
        OID_CASE(OID_TAPI_NEGOTIATE_EXT_VERSION);
        OID_CASE(OID_TAPI_OPEN);
        OID_CASE(OID_TAPI_PROVIDER_INITIALIZE);
        OID_CASE(OID_TAPI_PROVIDER_SHUTDOWN);
        OID_CASE(OID_TAPI_SECURE_CALL);
        OID_CASE(OID_TAPI_SELECT_EXT_VERSION);
        OID_CASE(OID_TAPI_SEND_USER_USER_INFO);
        OID_CASE(OID_TAPI_SET_APP_SPECIFIC);
        OID_CASE(OID_TAPI_SET_CALL_PARAMS);
        OID_CASE(OID_TAPI_SET_DEFAULT_MEDIA_DETECTION);
        OID_CASE(OID_TAPI_SET_DEV_CONFIG);
        OID_CASE(OID_TAPI_SET_MEDIA_MODE);
        OID_CASE(OID_TAPI_SET_STATUS_MESSAGES);
		OID_CASE(OID_TAPI_TRANSLATE_ADDRESS);
        OID_CASE(OID_WAN_CURRENT_ADDRESS);
        OID_CASE(OID_WAN_GET_BRIDGE_INFO);
        OID_CASE(OID_WAN_GET_COMP_INFO);
        OID_CASE(OID_WAN_GET_INFO);
        OID_CASE(OID_WAN_GET_LINK_INFO);
        OID_CASE(OID_WAN_GET_STATS_INFO);
        OID_CASE(OID_WAN_HEADER_FORMAT);
        OID_CASE(OID_WAN_LINE_COUNT);
        OID_CASE(OID_WAN_MEDIUM_SUBTYPE);
        OID_CASE(OID_WAN_PERMANENT_ADDRESS);
        OID_CASE(OID_WAN_PROTOCOL_TYPE);
        OID_CASE(OID_WAN_QUALITY_OF_SERVICE);
        OID_CASE(OID_WAN_SET_BRIDGE_INFO);
        OID_CASE(OID_WAN_SET_COMP_INFO);
        OID_CASE(OID_WAN_SET_LINK_INFO);

        default:
            OidName = "Unknown OID";
            break;
    }
    return OidName;
}
#endif

//
// This constant is used for places where NdisAllocateMemory
// needs to be called and the HighestAcceptableAddress does
// not matter.
//
NDIS_PHYSICAL_ADDRESS HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

// Number of bytes to allocate before and after each block to look for corruption
#define GUARD_REGION_SIZE	32
#define GUARD_BYTE_VALUE	(BYTE)'a'

#ifdef DEBUG

BOOL
memchk(
	  BYTE *pMem,
	  BYTE  bValue,
	  UINT	nBytes)
//
//	Return true if the bytes at pMem are all set to bValue.
//
{
	BYTE *pStartMem = pMem;
	UINT nOrigBytes = nBytes;

	//DEBUGMSG (1, (TEXT("Check %x bytes at %x\n"), nBytes, pMem));
	while (nBytes--)
		if (*pMem++ != bValue)
		{
			DumpMem(pStartMem, nOrigBytes);
			DebugBreak();
			return FALSE;
		}

	return TRUE;
}

BOOL
AsyncMacGuardRegionOk(
	IN	void	*pMem,
	IN	UINT	nBytes)
//
//	Validate the guard regions at the start and end of a block of allocated memory.
//
{
	BOOL bResult;

	pMem = (BYTE *)pMem - GUARD_REGION_SIZE;
	bResult =  memchk(pMem, GUARD_BYTE_VALUE, GUARD_REGION_SIZE)
			&& memchk((BYTE *)pMem + nBytes + GUARD_REGION_SIZE, GUARD_BYTE_VALUE, GUARD_REGION_SIZE);

	if (bResult == FALSE)
		DEBUGMSG(1, (TEXT("MemCorrupted: pMem=%x size=%x\n"), pMem, nBytes));

	return bResult;
}

#endif

void *
AsyncMacAllocateMemory(
	IN	UINT	nBytes)
//
//	Call NdisAllocateMemory to allocate a block of the requested size.
//	In DEBUG mode, allocate leading and trailing guard regions to look for memory corruption.
//
{
	void			*pMem;
	NDIS_STATUS		 Status;

#ifdef DEBUG
	nBytes += 2 * GUARD_REGION_SIZE;
#endif
    Status = NdisAllocateMemory( (PVOID *)&pMem, nBytes, 0, HighestAcceptableMax);
    if (Status != NDIS_STATUS_SUCCESS)
	{
		DEBUGMSG (ZONE_ERROR, (TEXT("+ASYNCMAC: AllocMem %d bytes failed\n")));
		pMem = NULL;
	}
#ifdef DEBUG
	else
	{
		memset(pMem, GUARD_BYTE_VALUE, GUARD_REGION_SIZE);
		memset((BYTE *)pMem + nBytes - GUARD_REGION_SIZE, GUARD_BYTE_VALUE, GUARD_REGION_SIZE);
		pMem = (BYTE *)pMem + GUARD_REGION_SIZE;

		ASSERT(AsyncMacGuardRegionOk(pMem, nBytes - 2 * GUARD_REGION_SIZE));
	}
#endif
	return pMem;
}

void
AsyncMacFreeMemory(
	IN	void	*pMem,
	IN	UINT	 nBytes)
{
#ifdef DEBUG
	ASSERT(AsyncMacGuardRegionOk(pMem, nBytes));
	pMem = (BYTE *)pMem - GUARD_REGION_SIZE;
	nBytes += 2 * GUARD_REGION_SIZE;
#endif
    NdisFreeMemory(pMem, nBytes, 0);
}



//
// ZZZ Portable interface.
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)


/*++

Routine Description:

    This is the primary initialization routine for the async driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{
	NDIS_STATUS	InitStatus;
    NDIS_MINIPORT_CHARACTERISTICS AsyncChar;

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+ASYNCMAC:DriverEntry(0x%X, 0x%X)\r\n"),
			   DriverObject, RegistryPath));

    v_AsyncDriverObject = DriverObject;

	v_pAdapter = NULL;

    //
    //  Initialize the wrapper.
    //
    NdisMInitializeWrapper(&v_hNdisWrapper, DriverObject,
						   RegistryPath, NULL);

    //
    //  Initialize the MAC characteristics for the call to NdisRegisterMac.
    //
	NdisZeroMemory(&AsyncChar, sizeof(AsyncChar));

    AsyncChar.MajorNdisVersion = ASYNC_NDIS_MAJOR_VERSION;
    AsyncChar.MinorNdisVersion = ASYNC_NDIS_MINOR_VERSION;
	AsyncChar.Reserved = NDIS_USE_WAN_WRAPPER;

	//
	// We do not need the following handlers:
	// CheckForHang
	// DisableInterrupt
	// EnableInterrupt
	// HandleInterrupt
	// ISR
	// Send
	// TransferData
	//
	AsyncChar.HaltHandler = MpHalt;
	AsyncChar.InitializeHandler = MpInit;
	AsyncChar.QueryInformationHandler = MpQueryInfo;
	AsyncChar.ReconfigureHandler = MpReconfigure;
	AsyncChar.ResetHandler = MpReset;
	AsyncChar.WanSendHandler = MpSend;
	AsyncChar.SetInformationHandler = MpSetInfo;

	InitStatus = NdisMRegisterMiniport(v_hNdisWrapper,
									   &AsyncChar,
									   sizeof(AsyncChar));

    if ( InitStatus == NDIS_STATUS_SUCCESS ) {

		DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
				  (TEXT("-ASYNCMAC:DriverEntry: Returning STATUS_SUCCESS\r\n"),
				   DriverObject, RegistryPath));

		return NDIS_STATUS_SUCCESS;
    }

    NdisTerminateWrapper(v_hNdisWrapper, DriverObject);

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE|ZONE_ERROR,
			  (TEXT("-ASYNCMAC:DriverEntry: Returning STATUS_FAILURE\r\n"),
			   DriverObject, RegistryPath));

    return NDIS_STATUS_FAILURE;
}

//
// Standard Windows DLL entrypoint.
// Since Windows CE NDIS miniports are implemented as DLLs, a DLL entrypoint is
// needed.
//
BOOL __stdcall
DllEntry(
  HANDLE hDLL,
  DWORD dwReason,
  LPVOID lpReserved
)
{
	
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
		v_hInstance = (HINSTANCE) hDLL;
        DEBUGREGISTER(hDLL);
		InitializeCriticalSection(&v_AdapterCS);
        DEBUGMSG(ZONE_INIT, (TEXT("ASYNCMAC: DLL_PROCESS_ATTACH\n")));
        break;
    case DLL_PROCESS_DETACH:
        DEBUGMSG(ZONE_INIT, (TEXT("ASYNCMAC: DLL_PROCESS_DETACH\n")));
		DeleteCriticalSection(&v_AdapterCS);
        break;
    }
    return TRUE;
}


VOID	
MpHalt(
	IN NDIS_HANDLE	MiniportAdapterContext
	)
{
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE, (TEXT("+ASYNCMAC:MpHalt(0x%X)\n"), MiniportAdapterContext));

	EnterCriticalSection(&v_AdapterCS);
    AsyncMacFreeMemory(MiniportAdapterContext, sizeof(ASYNCMAC_ADAPTER));
	v_pAdapter = NULL;
	v_GlobalAdapterCount--;
	LeaveCriticalSection(&v_AdapterCS);

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE, (TEXT("-ASYNCMAC:MpHalt\n")));
}


NDIS_STATUS
MpInit(
	OUT PNDIS_STATUS	OpenErrorStatus,
	OUT PUINT			SelectedMediumIndex,
	IN	PNDIS_MEDIUM	MediumArray,
	IN	UINT			MediumArraySize,
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	NDIS_HANDLE		WrapperConfigurationContext
	)
{
	NDIS_STATUS						Status;
    UINT							i;		// counter
	PASYNCMAC_ADAPTER				pAdapter;
    NDIS_HANDLE						ConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER	ReturnedValue;
    NDIS_STRING						MaxFrameSizeStr = NDIS_STRING_CONST("MaxFrameSize");
    NDIS_STRING						MaxSendFrameSizeStr = NDIS_STRING_CONST("MaxSendFrameSize");
    NDIS_STRING						MaxRecvFrameSizeStr = NDIS_STRING_CONST("MaxRecvFrameSize");
    NDIS_STRING						RecvBufSizeStr = NDIS_STRING_CONST("ReceiveBufferSize");
    NDIS_STRING						RecvThreadPrioStr = NDIS_STRING_CONST("ReceiveThreadPriority256");

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+ASYNCMAC:MpInit(0x%X, 0x%X, 0x%X, %d, 0x%X, 0x%X)\r\n"),
			   OpenErrorStatus, SelectedMediumIndex, MediumArray,
			   MediumArraySize, MiniportAdapterHandle,
			   WrapperConfigurationContext));
	
	//
	// We only support a single instance of AsyncMac
	//
	if (v_GlobalAdapterCount != 0) {
		return NDIS_STATUS_FAILURE;
	}

	for (i = 0; TRUE; i++)
	{
		if (i >= MediumArraySize)
			return NDIS_STATUS_UNSUPPORTED_MEDIA;

		if (MediumArray[i] == NdisMediumWan)
		{
			*SelectedMediumIndex = i;
			break;
		}
	}

	//
	// Let's allocate an AsyncMac adapter structure.
    //
	pAdapter = AsyncMacAllocateMemory(sizeof(ASYNCMAC_ADAPTER));
    if (pAdapter == NULL)
	{
        return NDIS_STATUS_RESOURCES;
    }

	ASSERT(AsyncMacGuardRegionOk(pAdapter, sizeof(ASYNCMAC_ADAPTER)));

#ifdef DEBUG
	pAdapter->dwDebugSigStart = AA_SIG_START;
	pAdapter->dwDebugSigEnd = AA_SIG_END;
	DEBUGMSG (ZONE_ALLOC, (TEXT(" ASYNCMAC:MpInit: Allocated pAdapter 0x%X(%d)\r\n"),
						   pAdapter, sizeof(ASYNCMAC_ADAPTER)));
	
#endif
	pAdapter->hMiniportAdapter = MiniportAdapterHandle;

    //
    // Open the configuration space.
    //
    NdisOpenConfiguration(&Status, &ConfigHandle, WrapperConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS)
	{
        AsyncMacFreeMemory(pAdapter, sizeof(ASYNCMAC_ADAPTER));

        DEBUGMSG(ZONE_INIT,
            (TEXT("NE2000:Initialize: NdisOpenConfiguration failed 0x%x\n"),
            Status));
        return Status;
    }

	pAdapter->Info.MaxFrameSize = MAX_FRAME_SIZE;

    NdisReadConfiguration(&Status, &ReturnedValue, ConfigHandle, &MaxFrameSizeStr, NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS)
	{
		pAdapter->Info.MaxFrameSize = ReturnedValue->ParameterData.IntegerData;
		DEBUGMSG(ZONE_INIT,(TEXT("ASYNCMAC: MaxFrameSize=%d.\n"), pAdapter->Info.MaxFrameSize));
    }
	

	pAdapter->MaxSendFrameSize = pAdapter->Info.MaxFrameSize;

    NdisReadConfiguration(&Status, &ReturnedValue, ConfigHandle, &MaxSendFrameSizeStr, NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS)
	{
		pAdapter->MaxSendFrameSize = ReturnedValue->ParameterData.IntegerData;
		DEBUGMSG(ZONE_INIT,(TEXT("ASYNCMAC: MaxSendFrameSize=%d.\n"), pAdapter->MaxSendFrameSize));
    }

	pAdapter->MaxRecvFrameSize = pAdapter->Info.MaxFrameSize;

    NdisReadConfiguration(&Status, &ReturnedValue, ConfigHandle, &MaxRecvFrameSizeStr, NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS)
	{
		pAdapter->MaxRecvFrameSize = ReturnedValue->ParameterData.IntegerData;
		DEBUGMSG(ZONE_INIT,(TEXT("ASYNCMAC: MaxRecvFrameSize=%d.\n"), pAdapter->MaxRecvFrameSize));
    }

	pAdapter->dwRecvBufSize = DEFAULT_RX_BUF_SIZE;

    NdisReadConfiguration(&Status, &ReturnedValue, ConfigHandle, &RecvBufSizeStr, NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS)
	{
		pAdapter->dwRecvBufSize = ReturnedValue->ParameterData.IntegerData;
		DEBUGMSG(ZONE_INIT,(TEXT("ASYNCMAC: RecvBufferSize=%d.\n"), pAdapter->dwRecvBufSize));
    }

	pAdapter->dwRecvThreadPrio = DEFAULT_RX_THREAD_PRIORITY;

	NdisReadConfiguration(&Status, &ReturnedValue, ConfigHandle, &RecvThreadPrioStr, NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS)
	{
		pAdapter->dwRecvThreadPrio = ReturnedValue->ParameterData.IntegerData;
    }
	DEBUGMSG(ZONE_INIT,(TEXT("ASYNCMAC: RecvThreadPrio=%d.\n"), pAdapter->dwRecvThreadPrio));

	// Just close the config handle
	NdisCloseConfiguration (ConfigHandle);

	// Initialize our structure.
	pAdapter->Info.MaxTransmit = 2;

#define	ROUND_UP_TO_MULTIPLE_OF_N(value, N)	((value + N - 1) & ~(N - 1))

	// For more info on HeaderPadding and TailPadding sizes see AssemblePPPFrame.
	// HeaderPadding = 1 byte for opening flag (7e) + MaxFrameSize bytes to allow for insertion of escape bytes
	// TailPadding   = 4 bytes for escaped 16 bit CRC + 1 byte for closing flag
	//
	// The header padding must be rounded up because the PPP code that builds a packet expects that
	// PacketStartAddress + HeaderPadding will be DWORD aligned.
	pAdapter->Info.HeaderPadding = ROUND_UP_TO_MULTIPLE_OF_N(1 + pAdapter->Info.MaxFrameSize, 4);
	pAdapter->Info.TailPadding = 4 + 1;

	pAdapter->Info.Endpoints = 1;
	pAdapter->Info.MemoryFlags = 0;
//	pAdapter->Info.HighestAcceptableAddress = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
	pAdapter->Info.FramingBits = PPP_FRAMING|SLIP_FRAMING;
	pAdapter->Info.DesiredACCM = DEFAULT_ACCM;
	
	// Tell NDIS about our handle
    NdisMSetAttributesEx(MiniportAdapterHandle, (NDIS_HANDLE)pAdapter, 1000,
						 0, NdisInterfaceInternal);
	
	
	// Initialize TAPI
#ifdef TODO
	// Once we change the load order so that miniports get loaded after the
	// system is initialized we can do the lineInitialize here.  In the meantime
	// we'll use a lame function to sleep for awhile until we think the system
	// might be happy.
	{
		long	lReturn;
		lReturn = lineInitialize (&(pAdapter->hLineApp),
								  v_hInstance, lineCallbackFunc, 
								  TEXT("ASYNCMAC"), &(pAdapter->dwNumDevs));
		DEBUGMSG (1, (TEXT("lineInitialize returned %d\r\n"),
					  lReturn));

		DEBUGMSG (1, (TEXT("lineInitialize say's there's %d devices\r\n"),
					  pAdapter->dwNumDevs));
	
	}
#else
	{
		NdisMInitializeTimer (&(pAdapter->ntLineInit),
							  pAdapter->hMiniportAdapter,
							  DoLineInitialize, pAdapter);
							  
							  
		NdisMSetTimer (&(pAdapter->ntLineInit), 1000);
					  
	}
#endif

	// Lot's more stuff.
	EnterCriticalSection(&v_AdapterCS);
	v_pAdapter = pAdapter;
	v_GlobalAdapterCount++;
	LeaveCriticalSection(&v_AdapterCS);

	ASSERT(AsyncMacGuardRegionOk(pAdapter, sizeof(ASYNCMAC_ADAPTER)));

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE, (TEXT("-ASYNCMAC:MpInit\r\n")));
	
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MapTapiErrorToNdisStatus(
	IN	DWORD	dwTapiErrorCode)
//
//	Translate a TAPI error code to an NDIS status.
//
{
	NDIS_STATUS	NdisStatus;

	switch(dwTapiErrorCode)
	{
	case 0:
		NdisStatus = NDIS_STATUS_SUCCESS;
		break;

	case LINEERR_BADDEVICEID:
	case LINEERR_INVALADDRESS:
	case LINEERR_INVALAPPHANDLE:
	case LINEERR_INVALPARAM:
	case LINEERR_INVALPOINTER:
	case LINEERR_INVALDEVICECLASS:
	case LINEERR_STRUCTURETOOSMALL:
		NdisStatus = NDIS_STATUS_TAPI_INVALPARAM;
		break;

	case LINEERR_CALLUNAVAIL:
		NdisStatus = NDIS_STATUS_TAPI_CALLUNAVAIL;
		break;

	case LINEERR_INCOMPATIBLEEXTVERSION:
		NdisStatus = NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION;
		break;

	case LINEERR_INVALLINESTATE:
		NdisStatus = NDIS_STATUS_TAPI_INVALLINESTATE;
		break;

	case LINEERR_INVALCARD:
	case LINEERR_NODEVICE:
	case LINEERR_UNINITIALIZED:
		NdisStatus = NDIS_STATUS_TAPI_NODEVICE;
		break;

	case LINEERR_NODRIVER:
		NdisStatus = NDIS_STATUS_TAPI_NODRIVER;
		break;

	case LINEERR_NOMEM:
		NdisStatus = NDIS_STATUS_RESOURCES;
		break;

	case LINEERR_OPERATIONUNAVAIL:
		NdisStatus = NDIS_STATUS_TAPI_OPERATIONUNAVAIL;
		break;

	case LINEERR_RESOURCEUNAVAIL:
		NdisStatus = NDIS_STATUS_TAPI_RESOURCEUNAVAIL;
		break;

	case LINEERR_INCOMPATIBLEAPIVERSION:
	case LINEERR_OPERATIONFAILED:
		NdisStatus = NDIS_STATUS_FAILURE;
		break;

	default:
		//
		//	All TAPI codes should be covered explicitly, if
		//	they are not we can catch them with the assert and
		//	add them.
		//
		ASSERT(FALSE);
		NdisStatus = NDIS_STATUS_FAILURE;
		break;

	}

	return NdisStatus;
}


DWORD WINAPI
LineConfigDialogEditThread(
	LPVOID pVArg)
{
	PNDIS_TAPI_LINE_CONFIG_DIALOG_EDIT pConfigDlgEdit = (PNDIS_TAPI_LINE_CONFIG_DIALOG_EDIT)pVArg;
	long lResult;
	LPTSTR	szDeviceClass;

	//
	//	In case any of the data is on the application stack or heap...
	//
	SetProcPermissions(-1);

	if (pConfigDlgEdit->ulDeviceClassLen) {
		szDeviceClass = (LPTSTR)pConfigDlgEdit->DataBuf;
	} else {
		szDeviceClass = NULL;
	}

	lResult = lineConfigDialogEdit (pConfigDlgEdit->ulDeviceID,
									pConfigDlgEdit->hwndOwner,
									szDeviceClass,
									pConfigDlgEdit->DataBuf + pConfigDlgEdit->ulConfigInOffset,
									pConfigDlgEdit->ulConfigInSize,
									(LPVARSTRING)(pConfigDlgEdit->DataBuf + pConfigDlgEdit->ulConfigOutOffset));

	return (DWORD)lResult;
}

NDIS_STATUS
MpQueryInfo(
	IN	NDIS_HANDLE	MiniportAdapterContext,
	IN	NDIS_OID	Oid,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT PULONG		BytesWritten,
	OUT PULONG		BytesNeeded
	)
/*++

Routine Description:

    The MpQueryProtocolInformation process a Query request for
    NDIS_OIDs that are specific to a binding about the MAC.  Note that
    some of the OIDs that are specific to bindings are also queryable
    on a global basis.  Rather than recreate this code to handle the
    global queries, I use a flag to indicate if this is a query for the
    global data or the binding specific data.

Arguments:

    Adapter - a pointer to the adapter.

    Oid - the NDIS_OID to process.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_MEDIUM             Medium          = NdisMediumWan;
	PASYNCMAC_ADAPTER		pAdapter = (PASYNCMAC_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS             StatusToReturn  = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS    HardwareStatus  = NdisHardwareStatusReady;
    PVOID                   MoveSource;
    ULONG                   MoveBytes;
    ULONG                   GenericULong    = 0;
    USHORT                  GenericUShort   = 0;
    INT                     fDoCommonMove = TRUE;
    UCHAR					WanAddress[6] = {' ','A','S','Y','N',0xFF};  // This is the address returned by OID_WAN_*_ADDRESS.
	
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+ASYNCMAC:MpQueryInfo(0x%X, 0x%X(%hs), 0x%X, %d, 0x%X, 0x%X)\r\n"),
			   MiniportAdapterContext, Oid, GetOidString(Oid),
			   InformationBuffer, InformationBufferLength,
			   BytesWritten, BytesNeeded));

	ASSERT(CHK_AA(pAdapter));
	
    MoveSource = &GenericULong;
    MoveBytes  = sizeof(GenericULong);
	
	switch ( Oid ) {
	case OID_GEN_SUPPORTED_LIST:
		MoveSource = (PVOID) SupportedOids;
		MoveBytes = sizeof(SupportedOids);
		break;
		
	case OID_GEN_MEDIA_SUPPORTED :
	case OID_GEN_MEDIA_IN_USE:
		MoveSource = (PVOID)&Medium;
		MoveBytes = sizeof(Medium);
		break;

	case OID_GEN_VENDOR_ID:
		GenericULong = 0xFFFFFFFF;
		MoveBytes = 3;
		break;

	case OID_GEN_VENDOR_DESCRIPTION:
		MoveSource = (PVOID)"AsyncMac Adapter";
		MoveBytes = 16;
		break;

	case OID_GEN_DRIVER_VERSION:
		GenericUShort = 0x0300;
		MoveSource = (PVOID)&GenericUShort;
		MoveBytes = sizeof(USHORT);
		break;

	case OID_GEN_HARDWARE_STATUS:
		MoveSource = (PVOID)&HardwareStatus;
		MoveBytes = sizeof(HardwareStatus);
		break;

	case OID_GEN_LINK_SPEED:
		//
		// Who knows what the initial link speed is?
		// This should not be called, right?
		//
		GenericULong = (ULONG)288;
		break;
		
	case OID_TAPI_PROVIDER_INITIALIZE:
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_PROVIDER_INITIALIZE)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_PROVIDER_INITIALIZE);	
		} else {
			PNDIS_TAPI_PROVIDER_INITIALIZE	pProvInit = (PNDIS_TAPI_PROVIDER_INITIALIZE)InformationBuffer;
			pProvInit->ulNumLineDevs = pAdapter->dwNumDevs;
			*BytesWritten = sizeof(NDIS_TAPI_PROVIDER_INITIALIZE);
		}
		break;

	case OID_TAPI_GET_DEV_CAPS :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_GET_DEV_CAPS))
		{
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_GET_DEV_CAPS);
		} else
		{
			LONG						lResult;

			PNDIS_TAPI_GET_DEV_CAPS	pGetDevCaps = (PNDIS_TAPI_GET_DEV_CAPS)InformationBuffer;

			lResult = lineGetDevCaps(pAdapter->hLineApp, pGetDevCaps->ulDeviceID, pAdapter->dwAPIVersion,
						   0, (LPLINEDEVCAPS)&(pGetDevCaps->LineDevCaps));
			if (lResult != 0)
			{
				DEBUGMSG (ZONE_TAPI|ZONE_ERROR, (TEXT("lineGetDevCaps failed (%d)\n"), lResult));
				StatusToReturn = MapTapiErrorToNdisStatus(lResult);
			}
		}
		break;

	case OID_TAPI_GET_DEV_CONFIG :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_GET_DEV_CONFIG))
		{
			DEBUGMSG (ZONE_TAPI, (TEXT("ASYNCMAC: Buffer too short (%d < %d)\n"), InformationBufferLength, sizeof(NDIS_TAPI_GET_DEV_CONFIG)));
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_GET_DEV_CONFIG);
			break;
		}
		else
		{
			PNDIS_TAPI_GET_DEV_CONFIG	pTapiDevConfig = (PNDIS_TAPI_GET_DEV_CONFIG)InformationBuffer;
			LPVARSTRING					pDevConfig = (LPVARSTRING)&pTapiDevConfig->DeviceConfig;
			LONG						lResult;

			DEBUGMSG (ZONE_TAPI, (TEXT("ASYNCMAC: Calling lineGetDevConfig\n")));
			lResult = lineGetDevConfig(
							pTapiDevConfig->ulDeviceID,
							pDevConfig,
							DEV_CLASS_COMM_DATAMODEM);
			if (lResult != 0)
			{
				DEBUGMSG (ZONE_TAPI|ZONE_ERROR, (TEXT("lineGetDevConfig failed (%d)\n"), lResult));
				StatusToReturn = MapTapiErrorToNdisStatus(lResult);
			}
		}
		break;

	case OID_TAPI_TRANSLATE_ADDRESS :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_LINE_TRANSLATE)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_LINE_TRANSLATE);
			DEBUGMSG (ZONE_ERROR, (TEXT(" ASYNCMAC:MpQueryInfo length too short for lineTranslateAddress, new length=%d\r\n"),
								   *BytesNeeded));
		} else {
			PNDIS_TAPI_LINE_TRANSLATE pLineTranslate = (PNDIS_TAPI_LINE_TRANSLATE)InformationBuffer;
			long lResult;

			// Lame check to see if the structures are the same size
			ASSERT (sizeof(LINETRANSLATEOUTPUT) == sizeof(LINE_TRANSLATE_OUTPUT));

			lResult = lineTranslateAddress (pAdapter->hLineApp, pLineTranslate->ulDeviceID,
											pAdapter->dwAPIVersion, (LPTSTR)pLineTranslate->DataBuf,
											0, pLineTranslate->ulTranslateOptions,
											(LPLINETRANSLATEOUTPUT)(pLineTranslate->DataBuf+pLineTranslate->ulLineTranslateOutputOffset));
			if (lResult != 0) {
				DEBUGMSG (ZONE_ERROR, (TEXT(" ASYNCMAC:MpQueryInfo Error %d from lineTranslateAddress\r\n"),
									   lResult));
				StatusToReturn = MapTapiErrorToNdisStatus(lResult);
			}
		}
		break;

	case OID_TAPI_OPEN :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_OPEN)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_OPEN);
			break;
		} else {
			PNDIS_TAPI_OPEN		pTapiOpen = (PNDIS_TAPI_OPEN)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;
			long				lResult;

			pOpenLine = AsyncMacAllocateMemory(sizeof(ASYNCMAC_OPEN_LINE));

			if (pOpenLine == NULL)
			{
				StatusToReturn = NDIS_STATUS_RESOURCES;
				break;
			}
			ASSERT(AsyncMacGuardRegionOk(pOpenLine, sizeof(ASYNCMAC_OPEN_LINE)));
#ifdef DEBUG
			pOpenLine->dwDebugSigStart = AOL_SIG_START;
			pOpenLine->dwDebugSigEnd = AOL_SIG_END;
			DEBUGMSG (ZONE_ALLOC, (TEXT(" ASYNCMAC:MpQueryInfo: Allocated pOpenLine 0x%X(%d)\r\n"),
								   pOpenLine, sizeof(ASYNCMAC_OPEN_LINE)));
#endif
			DEBUGMSG (ZONE_TAPI, (TEXT("***Allocated pOpenLine at 0x%X\r\n"),
						  pOpenLine));
			pOpenLine->dwRefCnt = 1;	// InitializeRefCnt
			pOpenLine->pAdapter = pAdapter;
			pOpenLine->dwDeviceID = pTapiOpen->ulDeviceID;
			pOpenLine->htLine = pTapiOpen->htLine;
			pOpenLine->dwBaudRate = AOL_DEF_BAUD_RATE;	// Default baud rate.

			// Initialize the GET_LINK_INFO struct
			pOpenLine->WanLinkInfo.MaxSendFrameSize = pAdapter->MaxSendFrameSize;
			pOpenLine->WanLinkInfo.MaxRecvFrameSize = pAdapter->MaxRecvFrameSize;
			pOpenLine->WanLinkInfo.HeaderPadding    = pAdapter->Info.HeaderPadding;
			pOpenLine->WanLinkInfo.TailPadding      = pAdapter->Info.TailPadding;
			pOpenLine->WanLinkInfo.SendFramingBits = PPP_FRAMING;	// For SLIP, we will have to be told to turn this off.
			pOpenLine->WanLinkInfo.RecvFramingBits = PPP_FRAMING;
			pOpenLine->WanLinkInfo.SendCompressionBits = 0;
			pOpenLine->WanLinkInfo.RecvCompressionBits = 0;
			pOpenLine->WanLinkInfo.SendACCM = (ULONG) -1;
			pOpenLine->WanLinkInfo.RecvACCM = (ULONG) -1;
			
			lResult = lineOpen(pAdapter->hLineApp, pTapiOpen->ulDeviceID,
							   &pOpenLine->hLine,
							   pAdapter->dwAPIVersion, 0,
							   (DWORD)pOpenLine, LINECALLPRIVILEGE_OWNER,
							   LINEMEDIAMODE_DATAMODEM, NULL);

			if (lResult != 0) {
				DEBUGMSG (ZONE_TAPI|ZONE_ERROR,
						  (TEXT("lineOpen failed (%x)\r\n"), lResult));
				AsyncMacFreeMemory (pOpenLine, sizeof(ASYNCMAC_OPEN_LINE));
				StatusToReturn = MapTapiErrorToNdisStatus(lResult);
			} else {
				DEBUGMSG (ZONE_TAPI, (TEXT("lineOpen success\r\n")));
				pTapiOpen->hdLine = (HDRV_LINE)pOpenLine;

				// Add ourselves to the list of open lines for this adapter
				pOpenLine->pNext = pAdapter->pHead;
				pAdapter->pHead = pOpenLine;
				DEBUGMSG(ZONE_TAPI, (TEXT(" Add pOpenLine=%x to pAdapter=%x (v_pAdapter=%x)\n"), pOpenLine, pAdapter, v_pAdapter));
			}
		}
		break;

	case OID_TAPI_MAKE_CALL :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_MAKE_CALL)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_MAKE_CALL);
			break;
		} else {
			PNDIS_TAPI_MAKE_CALL pTapiMakeCall = (PNDIS_TAPI_MAKE_CALL)InformationBuffer;
			long lResult;
			PASYNCMAC_OPEN_LINE	pOpenLine;
			LINECALLPARAMS CallParams;
			LPTSTR			szDialStr;

			pOpenLine = (PASYNCMAC_OPEN_LINE)pTapiMakeCall->hdLine;
			ASSERT(CHK_AOL(pOpenLine));
			pOpenLine->htCall = pTapiMakeCall->htCall;

#ifdef TODO
	// For now I'll assume that the bUserDefaultCallParams has been
	// specified
#endif
			szDialStr = (LPTSTR)((PBYTE)pTapiMakeCall+pTapiMakeCall->ulDestAddressOffset);
			RETAILMSG (1, (TEXT("ASYNCMAC: Dialing '%s'\r\n"), szDialStr));
			memset ((char *)&CallParams, 0, sizeof(LINECALLPARAMS));
			CallParams.dwTotalSize = sizeof(LINECALLPARAMS);
			CallParams.dwBearerMode = LINEBEARERMODE_DATA;
			CallParams.dwMinRate = 0;   // Any rate
			CallParams.dwMaxRate = 0;   // This should mean any max rate
			CallParams.dwMediaMode = LINEMEDIAMODE_DATAMODEM;
			CallParams.dwCallParamFlags = LINECALLPARAMFLAGS_IDLE;
			CallParams.dwAddressMode = LINEADDRESSMODE_ADDRESSID;
			CallParams.dwAddressID = 0;  // there's only one address...

			lResult = lineMakeCall (pOpenLine->hLine, &(pOpenLine->hCall),
									szDialStr, 0,
									&CallParams);
			if (lResult < 0) {
				DEBUGMSG (ZONE_TAPI|ZONE_ERROR,
						  (TEXT("Error %x from lineMakeCall\r\n"), lResult));
				StatusToReturn = MapTapiErrorToNdisStatus(lResult);
			} else {
				DEBUGMSG (ZONE_TAPI, (TEXT("lineMakeCall Success.  RequestID=%d, hCall=0x%X\r\n"),
									  lResult, pOpenLine->hCall));
				pTapiMakeCall->hdCall = (HDRV_CALL)pOpenLine;
				pOpenLine->TapiReqID = lResult;
			}
			ASSERT(CHK_AOL(pOpenLine));
		}
		break;

	case OID_TAPI_CONFIG_DIALOG_EDIT :
		fDoCommonMove = FALSE;
		if (InformationBufferLength < sizeof(NDIS_TAPI_LINE_CONFIG_DIALOG_EDIT)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_LINE_CONFIG_DIALOG_EDIT);
			DEBUGMSG (ZONE_ERROR, (TEXT(" ASYNCMAC:MpQueryInfo length too short for lineTranslateAddress, new length=%d\r\n"),
								   *BytesNeeded));
		} else {
			long lResult;
			HANDLE	hEditThread;
			DWORD	dwID;

			//
			//	Spin a thread to do the edit.
			//	That way, when the edit thread goes away GWE will release any "hidden" window
			//	from the system. Otherwise the hidden window would persist as long as the INdisDriverThread
			//	for asyncmac was around and cause problems as there is no message pump for it.
			//
			hEditThread = CreateThread(NULL, 0, LineConfigDialogEditThread, InformationBuffer, 0, &dwID);

			if (hEditThread == INVALID_HANDLE_VALUE)
			{
				StatusToReturn = NDIS_STATUS_RESOURCES;
			}
			else
			{
				WaitForSingleObject(hEditThread, INFINITE);
				GetExitCodeThread(hEditThread, (PDWORD)&lResult);
				CloseHandle(hEditThread);

				DEBUGMSG (lResult && ZONE_ERROR, (TEXT(" ASYNCMAC:MpQueryInfo Error 0x%X(%d) from lineConfigDialogEdit\r\n"), lResult, lResult));
			}
		}
		break;

	case OID_WAN_CURRENT_ADDRESS:
	case OID_WAN_PERMANENT_ADDRESS:
		MoveSource = WanAddress;
		MoveBytes = sizeof(WanAddress);
		break;
		
	case OID_WAN_GET_INFO:
		MoveSource = &pAdapter->Info;
		MoveBytes = sizeof(pAdapter->Info);
		break;
		
	case OID_WAN_MEDIUM_SUBTYPE:
	    GenericULong = NdisWanMediumSerial;
	    break;

	case OID_WAN_HEADER_FORMAT:
	    GenericULong = NdisWanHeaderEthernet;
	    break;


	case OID_GEN_MAC_OPTIONS:
		GenericULong = (ULONG)(NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
							   NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                               NDIS_MAC_OPTION_FULL_DUPLEX |
							   NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA);
		break;

	case OID_GEN_MAXIMUM_LOOKAHEAD :
	case OID_GEN_CURRENT_LOOKAHEAD:
	case OID_GEN_MAXIMUM_FRAME_SIZE:
	case OID_GEN_TRANSMIT_BLOCK_SIZE:
	case OID_GEN_RECEIVE_BLOCK_SIZE:
	case OID_GEN_MAXIMUM_TOTAL_SIZE:
		GenericULong = pAdapter->Info.MaxFrameSize;
		break;

	case OID_GEN_TRANSMIT_BUFFER_SPACE:
	case OID_GEN_RECEIVE_BUFFER_SPACE:
		// TODO: Get real buffer space numbers
		GenericULong = (ULONG)(pAdapter->Info.MaxFrameSize * 2);
		break;

	case OID_WAN_GET_LINK_INFO :
		if (InformationBufferLength < sizeof (NDIS_WAN_GET_LINK_INFO)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_WAN_GET_LINK_INFO);
			break;
		} else {
			PNDIS_WAN_GET_LINK_INFO	pLinkInfo = (PNDIS_WAN_GET_LINK_INFO)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;

			pOpenLine = (PASYNCMAC_OPEN_LINE)pLinkInfo->NdisLinkHandle;
			ASSERT(CHK_AOL(pOpenLine));

			// Let's move this just in case...
			pOpenLine->WanLinkInfo.NdisLinkHandle = pLinkInfo->NdisLinkHandle;
				
			MoveSource = &pOpenLine->WanLinkInfo;
			MoveBytes = sizeof(pOpenLine->WanLinkInfo);
		}
		
		break;

    default:
		DEBUGMSG (ZONE_ERROR,
				  (TEXT(" ASYNCMAC:MpQueryInfo: OID 0x%X/%hs not supported\r\n"),
				  Oid, GetOidString(Oid)));
        StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }
	
    if ( StatusToReturn == NDIS_STATUS_SUCCESS ) {

        if (fDoCommonMove) {
            //
            //  If there is enough room then we can copy the data and
            //  return the number of bytes copied, otherwise we must
            //  fail and return the number of bytes needed.
            //
            if ( MoveBytes <= InformationBufferLength ) {

                memcpy (InformationBuffer, MoveSource, MoveBytes);

                *BytesWritten = MoveBytes;

            } else {

                *BytesNeeded = MoveBytes;

	        	StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

            }
        }
    }

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("-ASYNCMAC:MpQueryInfo: Returning 0x%X\r\n"),
			   StatusToReturn));
	
	return StatusToReturn;
}

NDIS_STATUS
MpSetInfo(
	IN	NDIS_HANDLE	MiniportAdapterContext,
	IN	NDIS_OID	Oid,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT PULONG		BytesRead,
	OUT PULONG		BytesNeeded
	)
/*++

Routine Description:

    The AsyncSetInformation is used by AsyncRequest to set information
    about the MAC.

    Note: Assumes it is called with the lock held.  Any calls are made down
	to the serial driver from this routine may return pending.  If this happens
	the completion routine for the call needs to complete this request by
	calling NdisMSetInformationComplete.

Arguments:

    MiniportAdapterContext - A pointer to the adapter.


Return Value:

    The function value is the status of the operation.

--*/

{
	PASYNCMAC_ADAPTER pAdapter = (PASYNCMAC_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS     StatusToReturn = NDIS_STATUS_SUCCESS;

	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+ASYNCMAC:MpSetInfo(0x%X, 0x%X, 0x%X, %d, 0x%X, 0x%X)\r\n"),
			   MiniportAdapterContext, Oid, InformationBuffer,
			   InformationBufferLength, BytesRead, BytesNeeded));

	ASSERT(CHK_AA(pAdapter));
	
    switch ( Oid ) {

	case OID_WAN_SET_LINK_INFO :
		if (InformationBufferLength < sizeof (NDIS_WAN_GET_LINK_INFO)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_WAN_GET_LINK_INFO);
			break;
		} else {
			PNDIS_WAN_GET_LINK_INFO	pLinkInfo = (PNDIS_WAN_GET_LINK_INFO)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;

			pOpenLine = (PASYNCMAC_OPEN_LINE)pLinkInfo->NdisLinkHandle;
			ASSERT(CHK_AOL(pOpenLine));

			DEBUGMSG (1, (TEXT("MpSetInfo: Orig ACCM=0x%X\r\n"), pOpenLine->WanLinkInfo.SendACCM));
			
			// Let's just save this away.
			memcpy ((char *)&(pOpenLine->WanLinkInfo),
					(char *)pLinkInfo,
					sizeof (pOpenLine->WanLinkInfo));
			
			DEBUGMSG (1, (TEXT("MpSetInfo: New ACCM=0x%X\r\n"), pOpenLine->WanLinkInfo.SendACCM));
			
		}
		break;
		
	case OID_TAPI_SET_DEV_CONFIG :
		if (InformationBufferLength < sizeof(NDIS_TAPI_SET_DEV_CONFIG))
		{
			DEBUGMSG (ZONE_TAPI, (TEXT("ASYNCMAC: Buffer too short (%d < %d)\n"), InformationBufferLength, sizeof(NDIS_TAPI_SET_DEV_CONFIG)));
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_SET_DEV_CONFIG);
			break;
		}
		else
		{
			PNDIS_TAPI_SET_DEV_CONFIG	pTapiDevConfig = (PNDIS_TAPI_SET_DEV_CONFIG)InformationBuffer;
			LONG						lResult;

			if (pTapiDevConfig->ulDeviceConfigSize > 0)
			{
				DEBUGMSG (ZONE_TAPI, (TEXT("ASYNCMAC: Calling lineSetDevConfig\n")));
				lResult = lineSetDevConfig (
								pTapiDevConfig->ulDeviceID,
								&pTapiDevConfig->DeviceConfig[0],
								pTapiDevConfig->ulDeviceConfigSize,
								DEV_CLASS_COMM_DATAMODEM);
				if (lResult != 0)
				{
					DEBUGMSG (ZONE_TAPI|ZONE_ERROR,
							  (TEXT("lineSetDevConfig failed (%d)\n"), lResult));
					StatusToReturn = NDIS_STATUS_TAPI_INVALPARAM;
				} 
			}
		}
		break;

	case OID_TAPI_DROP :
		DEBUGMSG(ZONE_INTERFACE, (TEXT("ASYNCMAC: OID_TAPI_DROP\n")));
		if (InformationBufferLength < sizeof (NDIS_TAPI_DROP)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_DROP);
			break;
		} else {
			PNDIS_TAPI_DROP	pTapiDrop = (PNDIS_TAPI_DROP)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;
			long    lResult;
			
			// We return the pOpenLine for the hdCall.
			pOpenLine = (PASYNCMAC_OPEN_LINE)pTapiDrop->hdCall;
			ASSERT(CHK_AOL(pOpenLine));

			// Forward on the info.
            lResult = lineDrop(pOpenLine->hCall, pTapiDrop->UserUserInfo,
								pTapiDrop->ulUserUserInfoSize);

            DEBUGMSG( ZONE_TAPI, (TEXT("Return from lineDrop()=%d\r\n"), lResult));
			
		}
		break;

	case OID_TAPI_CLOSE_CALL :
		DEBUGMSG(ZONE_INTERFACE, (TEXT("ASYNCMAC: OID_TAPI_CLOSE_CALL\n")));
		if (InformationBufferLength < sizeof (NDIS_TAPI_CLOSE_CALL)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_CLOSE_CALL);
			break;
		} else {
			PNDIS_TAPI_CLOSE_CALL	pTapiCloseCall = (PNDIS_TAPI_CLOSE_CALL)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;
			long    lResult;

			// We return the pOpenLine for the hdCall.
			pOpenLine = (PASYNCMAC_OPEN_LINE)pTapiCloseCall->hdCall;
			ASSERT(CHK_AOL(pOpenLine));

			// TAPI docs state that CloseHandle must be done on the hPort before
			// calling lineDeallocateCall.
			if (pOpenLine->hPort != NULL)
			{
				CloseHandle(pOpenLine->hPort);
				pOpenLine->hPort = NULL;
			}

            lResult = lineDeallocateCall( pOpenLine->hCall );
			pOpenLine->hCall = NULL;

			// We don't actually free the pOpenLine until the OID_TAPI_CLOSE.
            DEBUGMSG( ZONE_TAPI, (TEXT("Return from lineDeallocateCall()=%d\r\n"),
								 lResult));
		}
		break;
		
	case OID_TAPI_CLOSE :
		DEBUGMSG(ZONE_INTERFACE, (TEXT("ASYNCMAC: OID_TAPI_CLOSE\n")));
		if (InformationBufferLength < sizeof (NDIS_TAPI_CLOSE)) {
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;
			*BytesNeeded = sizeof(NDIS_TAPI_CLOSE);
			break;
		} else {
			PNDIS_TAPI_CLOSE	pTapiClose = (PNDIS_TAPI_CLOSE)InformationBuffer;
			PASYNCMAC_OPEN_LINE	pOpenLine;
			long    lResult;

			// We return the pOpenLine for the hdLine.
			pOpenLine = (PASYNCMAC_OPEN_LINE)pTapiClose->hdLine;
			ASSERT(CHK_AOL(pOpenLine));

			// Ok, now it's removed from the list.
			lResult = lineClose (pOpenLine->hLine);

			ReleaseOpenLinePtr (pOpenLine);

            DEBUGMSG( ZONE_TAPI, (TEXT("Return from lineClose()=%d\r\n"), lResult));
		}
		break;

	default:

        StatusToReturn = NDIS_STATUS_INVALID_OID;

        *BytesRead   = 0;
        *BytesNeeded = 0;

        break;
    }
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("-ASYNCMAC:MpSetInfo: Returning 0x%X\r\n"),
			   StatusToReturn));
	
	return StatusToReturn;
}

NDIS_STATUS
MpReconfigure(
	OUT PNDIS_STATUS	OpenErrorStatus,
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_HANDLE		WrapperConfigurationContext
	)
{
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+/-ASYNCMAC:MpReconfigure(0x%X, 0x%X, 0x%X)\r\n"),
			   OpenErrorStatus, MiniportAdapterContext,
			  WrapperConfigurationContext));
	return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
MpReset(
	OUT PBOOLEAN		AddressingReset,
	IN	NDIS_HANDLE		MiniportAdapterContext
	)
{
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("+/-ASYNCMAC:MpReset(0x%X, 0x%X)\r\n"),
			   AddressingReset, MiniportAdapterContext));
    *AddressingReset = FALSE;

	return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
MpSend(
	IN NDIS_HANDLE		MacBindingHandle,
    IN NDIS_HANDLE      NdisLinkHandle,
    IN PNDIS_WAN_PACKET pPacket)
{
	PASYNCMAC_OPEN_LINE pOpenLine = (PASYNCMAC_OPEN_LINE) NdisLinkHandle;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	DWORD				bytesWritten;		// bytes written per WriteFile call
	DWORD				packetBytesSent;	// sum of prior WriteFile bytesWritten
	
	DEBUGMSG (ZONE_INIT|ZONE_INTERFACE,
			  (TEXT("ASYNCMAC:+MpSend(0x%X, 0x%X, 0x%X)\r\n"),
			   MacBindingHandle, NdisLinkHandle,
			   pPacket));

	if (pOpenLine == NULL)
	{
		//
		//	This can happen if the connection is closed by RAS/PPP just as it is
		//	sending a packet.  In this case, we don't want to send anything.
		//
		Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		ASSERT(CHK_AOL(pOpenLine));

		// Let's just send it for now.
		if (pOpenLine->hPort == NULL)
		{
			Status = NDIS_STATUS_FAILURE;
		}
		else
		{
#ifdef DEBUG
			if (ZONE_SEND)
			{
				DEBUGMSG (1, (TEXT("ASYNCMAC: MpSend: About to frame packet (%d):\n"), pPacket->CurrentLength));
				DumpMem (pPacket->CurrentBuffer, pPacket->CurrentLength);
			}
#endif
		
			if (pOpenLine->WanLinkInfo.SendFramingBits & PPP_FRAMING)
			{
				// Do CRC generation and escape byte insertion
				AssemblePPPFrame (pOpenLine, pPacket);
			}
			else
			{
				// Do SLIP escape byte insertion
				AssembleSLIPFrame(pOpenLine, pPacket);
			}

#ifdef DEBUG
			//		DEBUGMSG (1, (TEXT("MpSend: After frame packet (%d):\r\n"), pPacket->CurrentLength));
			//		DumpMem (pPacket->CurrentBuffer, pPacket->CurrentLength);
#endif

			for (packetBytesSent = 0;
				 packetBytesSent < pPacket->CurrentLength;
				 packetBytesSent += bytesWritten)
			{
				if (!WriteFile (pOpenLine->hPort, pPacket->CurrentBuffer + packetBytesSent,
								pPacket->CurrentLength - packetBytesSent, &bytesWritten, 0)
				||  bytesWritten == 0)
				{
					DEBUGMSG(ZONE_SEND | ZONE_ERROR, (
						TEXT( "AsyncMac:WriteFile Error %d Aborting Packet after sending %d of %d bytes\r\n" ),
						GetLastError(), packetBytesSent, pPacket->CurrentLength) );
						Status = NDIS_STATUS_FAILURE;
						break;
				}
				DEBUGMSG (ZONE_SEND, (TEXT("AsyncMac: MpSend wrote %d bytes\r\n"), bytesWritten));
			}
		}
	}

	DEBUGMSG ((ZONE_INIT|ZONE_INTERFACE) || (ZONE_ERROR && Status),
			  (TEXT("ASYNCMAC:-MpSend: Returning 0x%X\r\n"),
			   Status));
	
    return Status;
}

BOOL
IsValidOpenLinePtr(PASYNCMAC_OPEN_LINE pOpenLine)
//
//	Validate pOpenLine.
//	Return TRUE if it is valid, FALSE otherwise.
//
//	This function must be called with v_AdapterCS held.
{
	BOOL				bValid = FALSE;
	PASYNCMAC_OPEN_LINE pOLCurrent;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+IsValidOpenLinePtr %x\n"), pOpenLine));

	DEBUGMSG(ZONE_MISC, (TEXT(" IsValidOpenLinePtr v_pAdapter=%x\n"), v_pAdapter));
	if (v_pAdapter)
	{
		DEBUGMSG(ZONE_MISC, (TEXT(" IsValidOpenLinePtr pHead=%x\n"), v_pAdapter->pHead));
		for (pOLCurrent = v_pAdapter->pHead; pOLCurrent; pOLCurrent = pOLCurrent->pNext)
		{
			DEBUGMSG(ZONE_MISC, (TEXT(" IsValidOpenLinePtr pOLCurrent=%x\n"), pOLCurrent));
			if (pOLCurrent == pOpenLine)
			{
				bValid = TRUE;
				break;
			}
		}
	}

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-IsValidOpenLinePtr %x result=%d\n"), pOpenLine, bValid));

	return bValid;
}

void
ReleaseOpenLinePtr (PASYNCMAC_OPEN_LINE pOpenLine)
//
//	Decrement the refcnt for the OpenLine parameter.
//	If the refcnt reaches zero, then no threads are using the OpenLine and so it is freed.
//
{
	PASYNCMAC_OPEN_LINE	pOLTemp;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+ReleaseOpenLinePtr %x\n"), pOpenLine));

	EnterCriticalSection(&v_AdapterCS);

	if (IsValidOpenLinePtr(pOpenLine))
	{
		pOpenLine->dwRefCnt--;
		DEBUGMSG(ZONE_MISC, (TEXT(" ReleaseOpenLinePtr refcnt now %d\n"), pOpenLine->dwRefCnt));

		if (0 == pOpenLine->dwRefCnt) {

			DEBUGMSG(ZONE_MISC, (TEXT(" ReleaseOpenLinePtr removing pOpenLine=%x\n"), pOpenLine));

			// Remove this from the adapter list.
			if (pOpenLine == pOpenLine->pAdapter->pHead) {
				// Trivial case, remove from head.
				pOpenLine->pAdapter->pHead = pOpenLine->pNext;
				DEBUGMSG(ZONE_MISC, (TEXT(" ReleaseOpenLinePtr trivial remove from head\n")));
			} else {
				for (pOLTemp = pOpenLine->pAdapter->pHead; pOLTemp; pOLTemp = pOLTemp->pNext) {
					if (pOLTemp->pNext == pOpenLine) {
						pOLTemp->pNext = pOpenLine->pNext;
						break;
					}
				}
				ASSERT (pOLTemp);
			}

			DEBUGMSG(ZONE_MISC, (TEXT("***ReleaseOpenLinePtr FREEING pOpenLine=%x\n"), pOpenLine));
			AsyncMacFreeMemory (pOpenLine, sizeof(ASYNCMAC_OPEN_LINE));
		}
	}

	LeaveCriticalSection(&v_AdapterCS);

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-ReleaseOpenLinePtr %x\n"), pOpenLine));

}
PASYNCMAC_OPEN_LINE
GetOpenLinePtr (void *context)
//
//	Return an OPEN_LINE ptr if the context is valid, NULL if not.
//
//	Increments the refcnt to the OPEN_LINE.
//
//	The caller of this function must call ReleaseOpenLinePtr if this
//	function returns non-NULL, in order to decrement the refcnt.
//
{
	PASYNCMAC_OPEN_LINE	pOpenLine = (PASYNCMAC_OPEN_LINE) context;
	
	DEBUGMSG(ZONE_FUNCTION, (TEXT("+GetOpenLinePtr %x\n"), pOpenLine));

	EnterCriticalSection(&v_AdapterCS);

	if (IsValidOpenLinePtr(pOpenLine))
	{
		// Increment refcnt to make sure it doesn't go away while in use.
		pOpenLine->dwRefCnt++;
		DEBUGMSG(ZONE_FUNCTION, (TEXT(" GetOpenLinePtr refcnt now %d\n"), pOpenLine->dwRefCnt));
	}
	else
		pOpenLine = NULL;

	LeaveCriticalSection(&v_AdapterCS);

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-GetOpenLinePtr result=%x\n"), pOpenLine));

	return pOpenLine;
}
