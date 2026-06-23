// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#define ASYNC_NDIS_MAJOR_VERSION	4
#define ASYNC_NDIS_MINOR_VERSION	0



typedef struct _ASYNCMAC_ADAPTER_ *PASYNCMAC_ADAPTER;

//
//	The bigger this value is, the fewer the number of calls to ReadFile
//	to receive bytes.
//
//	This value may be overridden with the registry setting:
//		HKLM\Comm\AsyncMac1\Parms\ReceiveBufferSize
//
#define DEFAULT_RX_BUF_SIZE	2000

//
//	Default receive thread priority.
//
//	This value may be overridden with the registry setting:
//		HKLM\Comm\AsyncMac1\Parms\ReceiveThreadPriority256
//
//
#define DEFAULT_RX_THREAD_PRIORITY	130

typedef struct _ASYNCMAC_OPEN_LINE
{
#ifdef DEBUG
	DWORD						dwDebugSigStart;
#endif
	struct _ASYNCMAC_OPEN_LINE	*pNext;
	DWORD						dwRefCnt;
	PASYNCMAC_ADAPTER			pAdapter;
	DWORD						dwDeviceID;
	HLINE						hLine;		// hLine from TAPI
	HTAPI_LINE					htLine;		// Handle from caller
	HCALL						hCall;
	DWORD						dwCallState;// The last callstate
	HTAPI_CALL					htCall;
	long						TapiReqID;
	NDIS_HANDLE					hNdisLinkContext;
	HANDLE						hPort;		// The serial port handle
	DWORD						dwBaudRate;	// The link baud rate (in 100's b/s)
#define AOL_DEF_BAUD_RATE		288			// The initial default
	DWORD						dwFlags;	// Some state flags
#define AOL_FLAGS_SENT_LINE_UP		0x00000001
#define AOL_FLAGS_ERROR_INDICATED	0x00000002
	HANDLE						hRxThrd;	// Recv Thread handle
	NDIS_WAN_GET_LINK_INFO		WanLinkInfo;
#ifdef DEBUG
	DWORD						dwDebugSigEnd;
#endif
} ASYNCMAC_OPEN_LINE, *PASYNCMAC_OPEN_LINE;
#define	AOL_SIG_START	'AOLS'
#define	AOL_SIG_END		'AOLE'

#ifdef DEBUG
#define CHK_AOL(ptr)	(ptr && (ptr->dwDebugSigStart == AOL_SIG_START) && (ptr->dwDebugSigEnd == AOL_SIG_END))
#else
#define CHK_AOL(ptr)	(1)
#endif

typedef struct _ASYNCMAC_ADAPTER_
{
#ifdef DEBUG
	DWORD				dwDebugSigStart;
#endif
	NDIS_HANDLE			hMiniportAdapter;
	HLINEAPP			hLineApp;
	DWORD				dwAPIVersion;
	DWORD				dwNumDevs;
	NDIS_MINIPORT_TIMER	ntLineInit;			// LineInitialize Timer
	NDIS_WAN_INFO		Info;
	DWORD				MaxSendFrameSize;
	DWORD				MaxRecvFrameSize;
	DWORD				dwRecvBufSize;		// Size of buffer passed to ReadFile
	DWORD				dwRecvThreadPrio;	// Priority of RxThread
	PASYNCMAC_OPEN_LINE	pHead;
	
#ifdef DEBUG
	DWORD				dwDebugSigEnd;
#endif
} ASYNCMAC_ADAPTER, *PASYNCMAC_ADAPTER;
#define	AA_SIG_START	'AA_S'
#define	AA_SIG_END		'AA_E'

#ifdef DEBUG
#define CHK_AA(ptr)	(ptr && (ptr->dwDebugSigStart == AA_SIG_START) && (ptr->dwDebugSigEnd == AA_SIG_END))
#else
#define CHK_AA(ptr)	(1)
#endif


#define	MAX_FRAME_SIZE	1500
#define FRAME_PADDING	256
#define DEFAULT_ACCM	0xFFFFFFFF


// ----------------------------------------------------------------
//
// Global Data
//
// ----------------------------------------------------------------
extern HINSTANCE		v_hInstance;
extern NDIS_HANDLE		v_hNdisMac;
extern NDIS_HANDLE		v_hNdisWrapper;
extern PDRIVER_OBJECT	v_AsyncDriverObject;
extern DWORD			v_GlobalAdapterCount;

// ----------------------------------------------------------------
//
// Debug Defines
//
// ----------------------------------------------------------------
#ifdef DEBUG
#define ZONE_INIT		DEBUGZONE(0)		// 0x0001
//#define ZONE_???		DEBUGZONE(1)		// 0x0002
//#define ZONE_???		DEBUGZONE(2)		// 0x0004
//#define ZONE_???		DEBUGZONE(3)		// 0x0008
//#define ZONE_???		DEBUGZONE(4)		// 0x0010
#define ZONE_SEND		DEBUGZONE(5)		// 0x0020
//#define ZONE_???		DEBUGZONE(6)		// 0x0040
//#define ZONE_???		DEBUGZONE(7)		// 0x0080
#define ZONE_TAPI		DEBUGZONE(8)		// 0x0100
#define ZONE_RECV       DEBUGZONE(9)		// 0x0200
#define ZONE_INTERFACE	DEBUGZONE(10)		// 0x0400
#define ZONE_MISC		DEBUGZONE(11)		// 0x0800
#define ZONE_ALLOC		DEBUGZONE(12)		// 0x1000
#define ZONE_FUNCTION	DEBUGZONE(13)		// 0x2000
#define ZONE_WARN		DEBUGZONE(14)		// 0x4000
#define ZONE_ERROR		DEBUGZONE(15)		// 0x8000
#endif


// ----------------------------------------------------------------
//
// Extern declarations
//
// ----------------------------------------------------------------
// ASYNCMAC.c

VOID MpHalt(NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS MpInit(PNDIS_STATUS	OpenErrorStatus, PUINT SelectedMediumIndex,
				   PNDIS_MEDIUM MediumArray,
				   UINT	MediumArraySize, NDIS_HANDLE MiniportAdapterHandle,
				   NDIS_HANDLE WrapperConfigurationContext);
NDIS_STATUS MpQueryInfo(NDIS_HANDLE MiniportAdapterContext, NDIS_OID Oid,
						PVOID InformationBuffer, ULONG InformationBufferLength,
						PULONG BytesWritten, PULONG BytesNeeded);
NDIS_STATUS MpSetInfo(NDIS_HANDLE MiniportAdapterContext,NDIS_OID Oid,
					  PVOID InformationBuffer, ULONG InformationBufferLength,
					  PULONG BytesRead, PULONG BytesNeeded);
NDIS_STATUS MpReconfigure(PNDIS_STATUS OpenErrorStatus,
						  NDIS_HANDLE MiniportAdapterContext,
						  NDIS_HANDLE WrapperConfigurationContext);
NDIS_STATUS MpReset(PBOOLEAN AddressingReset,
					NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS MpSend(NDIS_HANDLE MacBindingHandle, NDIS_HANDLE NdisLinkHandle,
				   PNDIS_WAN_PACKET Packet);
PASYNCMAC_OPEN_LINE GetOpenLinePtr (void *context);
void ReleaseOpenLinePtr (PASYNCMAC_OPEN_LINE pOpenLine);


PVOID	AsyncMacAllocateMemory(IN UINT	nBytes);
void	AsyncMacFreeMemory(IN void *pMem, IN	UINT	 nBytes);
BOOL	AsyncMacGuardRegionOk(IN void *pMem, IN	UINT	nBytes);



// tapifun.c
void CALLBACK	lineCallbackFunc(DWORD dwDevice, DWORD dwMsg,
								 DWORD dwCallbackInstance, DWORD dwParam1,
								 DWORD dwParam2, DWORD dwParam3);
VOID NDISAPI	DoLineInitialize(PVOID SystemSpecific1,
								 PVOID FunctionContext,
								 PVOID SystemSpecific2,
								 PVOID SystemSpecific3);
void			SendLineDown(PASYNCMAC_OPEN_LINE pOpenLine);

// Recv.c
DWORD WINAPI	MacRxThread (LPVOID pOpenLine);