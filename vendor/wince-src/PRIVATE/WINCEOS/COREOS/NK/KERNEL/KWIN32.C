/**     TITLE("Kernel Win32 Handle")
 *++
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *    KWin32.c
 *
 * Abstract:
 *
 *  This file contains the definition of the Win32 system API handle.
 *
 *--
 */
#include "kernel.h"
#include "halether.h"
#include "ethdbg.h"

const PFNVOID Win32Methods[] = {
    (PFNVOID)SC_Nop,
    (PFNVOID)SC_NotSupported,
    (PFNVOID)SC_CreateAPISet,				//  2
    (PFNVOID)SC_VirtualAlloc,				//  3
    (PFNVOID)SC_VirtualFree,				//  4
    (PFNVOID)SC_VirtualProtect,				//  5
    (PFNVOID)SC_VirtualQuery,				//  6
    (PFNVOID)SC_VirtualCopy,				//  7
	(PFNVOID)SC_LoadLibraryW,				//  8
	(PFNVOID)SC_FreeLibrary,				//  9
	(PFNVOID)SC_GetProcAddressW,			// 10
	(PFNVOID)SC_ThreadAttachAllDLLs,		// 11
	(PFNVOID)SC_ThreadDetachAllDLLs,		// 12
	(PFNVOID)SC_GetTickCount,				// 13
	(PFNVOID)OutputDebugStringW,			// 14
	(PFNVOID)SC_TlsCall,					// 15
	(PFNVOID)SC_GetSystemInfo,				// 16
	(PFNVOID)ropen,							// 17
	(PFNVOID)rread,							// 18
	(PFNVOID)rwrite,						// 19
	(PFNVOID)rlseek,						// 20
	(PFNVOID)rclose,						// 21
	(PFNVOID)SC_RegisterDbgZones,			// 22
	(PFNVOID)NKvDbgPrintfW,					// 23
	(PFNVOID)SC_ProfileSyscall,				// 24
	(PFNVOID)SC_FindResource,				// 25
	(PFNVOID)SC_LoadResource,				// 26
	(PFNVOID)SC_SizeofResource,				// 27
	(PFNVOID)OEMGetRealTime,				// 28
	(PFNVOID)OEMSetRealTime,				// 29
	(PFNVOID)SC_ProcessDetachAllDLLs,		// 30
	(PFNVOID)SC_ExtractResource,			// 31
	(PFNVOID)SC_GetRomFileInfo,				// 32
	(PFNVOID)SC_GetRomFileBytes,			// 33
	(PFNVOID)SC_CacheSync,				    // 34
	(PFNVOID)SC_AddTrackedItem,				// 35
	(PFNVOID)SC_DeleteTrackedItem,			// 36
	(PFNVOID)SC_PrintTrackedItem,			// 37
	(PFNVOID)SC_GetKPhys,					// 38
	(PFNVOID)SC_GiveKPhys,					// 39
	(PFNVOID)SC_SetExceptionHandler,		// 40
	(PFNVOID)SC_RegisterTrackedItem,		// 41
	(PFNVOID)SC_FilterTrackedItem,			// 42
	(PFNVOID)SC_SetKernelAlarm,				// 43
	(PFNVOID)SC_RefreshKernelAlarm,			// 44
	(PFNVOID)SC_CeGetRandomSeed,			// 45
	(PFNVOID)SC_CloseProcOE,				// 46
	(PFNVOID)SC_SetGwesOOMEvent,			// 47
	(PFNVOID)SC_FSStringCompress,			// 48
	(PFNVOID)SC_FSStringDecompress,			// 49
	(PFNVOID)SC_FSBinaryCompress,			// 50
	(PFNVOID)SC_FSBinaryDecompress,			// 51
	(PFNVOID)SC_CreateEvent,				// 52
	(PFNVOID)SC_CreateProc,					// 53
	(PFNVOID)SC_CreateThread,				// 54
	(PFNVOID)InputDebugCharW,				// 55
	(PFNVOID)UB_TakeCritSec,				// 56
	(PFNVOID)SC_LeaveCritSec,				// 57
	(PFNVOID)UB_WaitForMultiple,			// 58
	(PFNVOID)SC_MapPtrToProcess,			// 59
	(PFNVOID)SC_MapPtrUnsecure,				// 60
	(PFNVOID)SC_GetProcFromPtr,				// 61
	(PFNVOID)SC_IsBadPtr,					// 62
	(PFNVOID)SC_GetProcAddrBits,			// 63
	(PFNVOID)SC_GetFSHeapInfo,				// 64
	(PFNVOID)SC_OtherThreadsRunning,		// 65
	(PFNVOID)SC_KillAllOtherThreads,		// 66
    (PFNVOID)SC_GetOwnerProcess,			// 67
    (PFNVOID)SC_GetCallerProcess,			// 68
	(PFNVOID)SC_GetIdleTime,				// 69
	(PFNVOID)SC_SetLowestScheduledPriority,	// 70
	(PFNVOID)SC_IsPrimaryThread,			// 71
	(PFNVOID)SC_SetProcPermissions,			// 72
	(PFNVOID)SC_GetCurrentPermissions,		// 73
	(PFNVOID)0,								// 74
	(PFNVOID)SC_SetDaylightTime,			// 75
	(PFNVOID)SC_SetTimeZoneBias,			// 76
	(PFNVOID)SC_SetCleanRebootFlag,			// 77
	(PFNVOID)SC_CreateCrit,					// 78
	(PFNVOID)SC_PowerOffSystem,				// 79
	(PFNVOID)SC_CreateMutex,				// 80
	(PFNVOID)SC_SetDbgZone,					// 81
	(PFNVOID)UB_Sleep,						// 82
	(PFNVOID)SC_TurnOnProfiling,			// 83
	(PFNVOID)SC_TurnOffProfiling,			// 84
	(PFNVOID)SC_CeGetCurrentTrust,			// 85
	(PFNVOID)SC_CeGetCallerTrust,			// 86
	(PFNVOID)SC_NKTerminateThread,			// 87
	(PFNVOID)SC_SetLastError,				// 88
	(PFNVOID)SC_GetLastError,				// 89
	(PFNVOID)SC_GetProcName,				// 90
	(PFNVOID)SC_TerminateSelf,				// 91
	(PFNVOID)SC_CloseAllHandles,			// 92
	(PFNVOID)SC_SetHandleOwner,             // 93
	(PFNVOID)SC_LoadDriver,					// 94
	(PFNVOID)SC_CreateFileMapping,			// 95
	(PFNVOID)SC_UnmapViewOfFile,			// 96
	(PFNVOID)SC_FlushViewOfFile,			// 97
	(PFNVOID)SC_CreateFileForMapping,		// 98
	(PFNVOID)OEMIoControl,                  // 99
	(PFNVOID)SC_MapUncompressedFileW,		// 100
	(PFNVOID)SC_PPSHRestart,				// 101
	(PFNVOID)SC_SignalStarted,				// 102
	(PFNVOID)SC_UpdateNLSInfo,				// 103
	(PFNVOID)SC_ConnectDebugger,			// 104
	(PFNVOID)SC_InterruptInitialize,		// 105
	(PFNVOID)SC_InterruptDone,				// 106
	(PFNVOID)SC_InterruptDisable,			// 107
	(PFNVOID)SC_SetKMode,					// 108
	(PFNVOID)SC_SetPowerOffHandler,			// 109
	(PFNVOID)SC_SetGwesPowerHandler,		// 110
	(PFNVOID)SC_SetHardwareWatch,			// 111
    (PFNVOID)SC_QueryAPISetID,              // 112
	(PFNVOID)-1,                            // 113 (PerformCallBack)
	(PFNVOID)CaptureContext,                // 114 (RaiseException)
	(PFNVOID)SC_GetCallerIndex,				// 115
	(PFNVOID)SC_WaitForDebugEvent,			// 116
	(PFNVOID)SC_ContinueDebugEvent,			// 117
	(PFNVOID)SC_DebugNotify,				// 118
	(PFNVOID)SC_OpenProcess,				// 119
	(PFNVOID)SC_THCreateSnapshot,			// 120
	(PFNVOID)SC_THGrow,						// 121
	(PFNVOID)SC_NotifyForceCleanboot,		// 122
	(PFNVOID)SC_DumpKCallProfile,			// 123
	(PFNVOID)SC_GetProcessVersion,			// 124
	(PFNVOID)SC_GetModuleFileNameW,			// 125
	(PFNVOID)SC_QueryPerformanceCounter,	// 126
	(PFNVOID)SC_QueryPerformanceFrequency,	// 127
	(PFNVOID)SC_KernExtractIcons,			// 128
	(PFNVOID)SC_ForcePageout,				// 129
	(PFNVOID)SC_GetThreadTimes,				// 130
	(PFNVOID)SC_GetModuleHandleW,			// 131
	(PFNVOID)SC_SetWDevicePowerHandler,		// 132
	(PFNVOID)SC_SetStdioPathW,				// 133
	(PFNVOID)SC_GetStdioPathW,				// 134
	(PFNVOID)SC_ReadRegistryFromOEM,		// 135
	(PFNVOID)SC_WriteRegistryToOEM,			// 136
	(PFNVOID)SC_WriteDebugLED,		        // 137
	(PFNVOID)SC_LockPages,	                // 138
	(PFNVOID)SC_UnlockPages,	            // 139
#ifdef SH4
	(PFNVOID)SC_VirtualSetPageFlags,		// 140
	(PFNVOID)SC_SetRAMMode,					// 141
	(PFNVOID)SC_SetStoreQueueBase,			// 142
#else
	(PFNVOID)0,								// 140
	(PFNVOID)0,								// 141
	(PFNVOID)0,								// 142
#endif
	(PFNVOID)SC_FlushViewOfFileMaybe,		// 143
	(PFNVOID)SC_GetProcAddressA,			// 144
	(PFNVOID)SC_GetCommandLineW,			// 145
	(PFNVOID)SC_DisableThreadLibraryCalls,	// 146
	(PFNVOID)SC_CreateSemaphore,			// 147
	(PFNVOID)SC_LoadLibraryExW,				// 148
	(PFNVOID)-2,							// 149 (PerformCallForward)
	(PFNVOID)SC_CeMapArgumentArray,			// 150
	(PFNVOID)SC_KillThreadIfNeeded,			// 151
	(PFNVOID)SC_ProcGetIndex,				// 152
	(PFNVOID)SC_RegisterGwesHandler,		// 153
	(PFNVOID)SC_GetProfileBaseAddress,		// 154
	(PFNVOID)SC_SetProfilePortAddress,		// 155
#ifdef CELOG
	(PFNVOID)CeLogData,						// 156
	(PFNVOID)CeLogSetZones,					// 157
#else
	(PFNVOID)0,								// 156
	(PFNVOID)0,								// 157
#endif
	(PFNVOID)ModuleJit,						// 158
	(PFNVOID)SC_CeSetExtendedPdata,			// 159
	(PFNVOID)SC_VerQueryValueW,				// 160
	(PFNVOID)SC_GetFileVersionInfoSizeW,	// 161
	(PFNVOID)SC_GetFileVersionInfoW,		// 162
	(PFNVOID)SC_CreateLocaleView,			// 163
#ifdef CELOG
    (PFNVOID)CeLogReSync,                   // 164
#else
    (PFNVOID)0,                             // 164
#endif
};

const CINFO cinfWin32 = {
	"Wn32",
	DISPATCH_I_KPSL,
	0,
	sizeof(Win32Methods)/sizeof(Win32Methods[0]),
	Win32Methods,
};

extern const CINFO cinfThread;
extern const CINFO cinfProc;
extern const CINFO cinfMutex;
extern const CINFO cinfSem;
extern const CINFO cinfEvent;
extern const CINFO cinfAPISet;
extern const CINFO cinfMap;
const CINFO CinfFile = { "FILE", DISPATCH_KERNEL, HT_FILE, 0, 0 };
const CINFO CinfFind = { "FIND", DISPATCH_KERNEL, HT_FIND, 0, 0 };
const CINFO CinfDBFile = { "DFIL", DISPATCH_KERNEL, HT_DBFILE, 0, 0 };
const CINFO CinfDBFind = { "DFND", DISPATCH_KERNEL, HT_DBFIND, 0, 0 };
const CINFO CinfSocket = { "SKT", DISPATCH_KERNEL, HT_SOCKET, 0, 0 };
const CINFO CinfWnetEnum = { "ENUM", DISPATCH_KERNEL, HT_WNETENUM, 0, 0 };

extern void InitMemoryPool(void);
extern void ProfInit(void);
extern void HeapInit(void);
extern void SchedInit(void);
extern void ProcInit(void);
extern CRITICAL_SECTION VAcs, RFBcs, ppfscs, PhysCS, LLcs, ModListcs, ODScs, CompCS, MapCS, NameCS, DbgApiCS, PagerCS, WriterCS, MapNameCS, ppfcs, PageOutCS;
BOOL ReadyForStrings;
PPROCESS PowerProc;
FARPROC PowerFunc;
ACCESSKEY PowerKey;
PPROCESS GwesPowerProc;
FARPROC GwesPowerFunc;
ACCESSKEY GwesPowerKey;
PPROCESS WDevicePowerProc;
FARPROC WDevicePowerFunc;
ACCESSKEY WDevicePowerKey;

/* Kernel Debugger interface pointers */
// rameshk
// added more arguments
extern BOOLEAN (*PKDInit)(LPVOID * p1, LPVOID * p2, LPVOID *p3,LPVOID,LPVOID *,LPVOID *);
ULONG FakeKDTrap(PEXCEPTION_RECORD ExceptionRecord, CONTEXT *ContextRecord, BOOLEAN SecondChance);
ULONG (*KDTrap)(PEXCEPTION_RECORD ExceptionRecord, CONTEXT *ContextRecord, BOOLEAN SecondChance) = FakeKDTrap;

//rameshk
// Added FakePrintString().
// When windbg sends TerminateApi, KDPrintString should also made to point to
// a fake routine like the other 2 pointers KDTrap and KDUpdateSymbols.

BOOLEAN FakePrintString(IN LPCWSTR);
BOOLEAN (*KDPrintString)(IN LPCWSTR Output)=FakePrintString;

//end

//rameshk
BOOLEAN FakeSetLoadSymbolsFlag();
BOOLEAN FakeResetLoadSymbolsFlag();
BOOLEAN (*KDSetLoadSymbolsFlag)()=FakeSetLoadSymbolsFlag;
BOOLEAN (*KDResetLoadSymbolsFlag)()=FakeResetLoadSymbolsFlag;
//end

void FakeUpdateSymbols(DWORD dwAddr, BOOL bUnload);
void (*KDUpdateSymbols)(DWORD dwAddr, BOOL bUnload) = FakeUpdateSymbols;

// rameshk
BOOLEAN KDCleanup(void);
// end

extern void MemTrackInit(void);

// For mapping which communications devices to use for kernel debug services
UCHAR CommDev[3] = {KERNEL_COMM_SERIAL,KERNEL_COMM_PARALLEL,KERNEL_COMM_SERIAL};

void  (* lpWriteDebugStringFunc)(unsigned short *str) = OEMWriteDebugString;
int   (* lpReadDebugByteFunc)(void)                   = OEMReadDebugByte;
void  (* lpWriteDebugByteFunc)(BYTE ch)               = OEMWriteDebugByte;
void  (* lpParallelPortSendByteFunc)(BYTE ch)         = OEMParallelPortSendByte;
int   (* lpParallelPortGetByteFunc)(void)             = OEMParallelPortGetByte;

// Ethernet debug functions, pointers set only if platform supports ether debug.
PFN_EdbgRegisterDfltClient pEdbgRegisterDfltClient;
PFN_EdbgInitializeInterrupt pEdbgInitializeInterrupt;
PFN_EdbgSend pEdbgSend;
PFN_EdbgRecv pEdbgRecv;

// For PPFS over ether
extern void PpfsWriteDebugString(unsigned short *name);
extern void ppfs_send_byte_ether(BYTE ch);
extern int  ppfs_get_byte_ether(void);
BOOL BufferedPPFS;
UCHAR *PpfsEthBuf;


/* dummy Kernel Debugger routines */
ULONG FakeKDTrap(PEXCEPTION_RECORD ExceptionRecord, CONTEXT *ContextRecord, BOOLEAN SecondChance) {
	return FALSE;
}

void FakeUpdateSymbols(DWORD dwAddr, BOOL bUnload) {
	return;
}

void DoLoadAllSymbols(void) {
	PMODULE pMod;
	HANDLE hRealProc;
	int loop;
    KDSetLoadSymbolsFlag();
	KDUpdateSymbols((DWORD)ProcArray[0].BasePtr+1, FALSE);
	if ((CommDev[KERNEL_SVC_DBGMSG] == KERNEL_COMM_SERIAL) &&
		(CommDev[KERNEL_SVC_KDBG] == KERNEL_COMM_SERIAL))
		lpWriteDebugStringFunc = KDPrintString;
	ReadyForStrings = TRUE;
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		KDUpdateSymbols(((DWORD)pMod->BasePtr)+1, FALSE);
	hRealProc = hCurProc;
	for (loop = 1; loop < MAX_PROCESSES; loop++)
		if (ProcArray[loop].dwVMBase) {
			hCurProc = ProcArray[loop].hProc;
			KDUpdateSymbols(((DWORD)ProcArray[loop].BasePtr)+1, FALSE);
		}
	hCurProc = hRealProc;
    KDResetLoadSymbolsFlag();
}

BOOL SC_ConnectDebugger(LPVOID pInit) {
	LPVOID p1,p2,p3,p4,p5;
	BOOLEAN (*DbgInit)(kerndata_t *kdp, LPVOID kpTOC, LPVOID kpProcArray,
		LPVOID *p1, LPVOID *p2, LPVOID *p3,LPVOID,LPVOID *p4,LPVOID *p5) =
			(BOOLEAN (*)(kerndata_t *, LPVOID, LPVOID, LPVOID, LPVOID, LPVOID,LPVOID,LPVOID,LPVOID))pInit;
#ifdef SHx
	extern void FlushCache(void);
#if defined(SH4)
    extern void FPUFlushContext(void);
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushCache, FPUFlushContext};
#else
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushCache};
#endif
#elif MIPS
	extern void FlushDCache(void);
	extern void FlushICache(void);
#if defined(MIPS_HAS_FPU)
    extern void FPUFlushContext(void);
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushDCache, FlushICache, FPUFlushContext};
#else
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushDCache, FlushICache};
#endif
#elif PPC
	extern void FlushDCache(void);
	extern void FlushICache(void);
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushDCache, FlushICache};
#elif ARM
	extern void __declspec(iw32) FlushDCache(void);
	extern void __declspec(iw32) FlushICache(void);
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, FlushDCache, FlushICache};
#elif x86
	kerndata_t kdp = {OEMClearDebugCommError, OEMWriteDebugByte, OEMReadDebugByte,
		DbgVerify, DBG_CallCheck, NULL, &KData, NULL};
#else
	kerndata_t kdp;
    #pragma message("ERROR: ConnectDebugger not supported on this CPU!")
	lpWriteDebugStringFunc(TEXT("ConnectDebugger Failed - not supported on this CPU!\r\n"));
	return FALSE;
#endif
#ifdef DEBUG
	lpWriteDebugStringFunc(TEXT("Entering ConnectDebugger\r\n"));
#endif
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_ConnectDebugger failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return FALSE;
	}
    if (DbgInit(&kdp,pTOC,ProcArray,&p1, &p2, &p3,(LPVOID)KDCleanup,&p4,&p5)) {
		KDPrintString = (BOOLEAN (*)(IN LPCWSTR))p1;
		KDUpdateSymbols = (void (*)(DWORD, BOOL))p2;
	  	KDTrap = (ULONG (*)(PEXCEPTION_RECORD, CONTEXT *, BOOLEAN))p3;
	    KDSetLoadSymbolsFlag=(BOOLEAN (*)())p4;
	    KDResetLoadSymbolsFlag=(BOOLEAN (*)())p5;
    	EnterCriticalSection(&LLcs);
		DoLoadAllSymbols();
		LeaveCriticalSection(&LLcs);
		return TRUE;
   	}
#ifdef DEBUG
	lpWriteDebugStringFunc(TEXT("ConnectDebugger Failed\r\n"));
#endif
	return FALSE;
}

void KernelInit(void) {
#ifdef DEBUG
	lpWriteDebugStringFunc(TEXT("Windows CE KernelInit\r\n"));
#endif
    /* setup the well known system API sets & API types */
    SystemAPISets[SH_WIN32] = &cinfWin32;
    SystemAPISets[SH_CURTHREAD] = &cinfThread;
    SystemAPISets[SH_CURPROC] = &cinfProc;
    SystemAPISets[HT_EVENT] = &cinfEvent;
    SystemAPISets[HT_MUTEX] = &cinfMutex;
    SystemAPISets[HT_SEMAPHORE] = &cinfSem;
    SystemAPISets[HT_APISET] = &cinfAPISet;
    SystemAPISets[HT_FILE] = &CinfFile;
    SystemAPISets[HT_FIND] = &CinfFind;
    SystemAPISets[HT_DBFILE] = &CinfDBFile;
    SystemAPISets[HT_DBFIND] = &CinfDBFind;
    SystemAPISets[HT_SOCKET] = &CinfSocket;
    SystemAPISets[HT_FSMAP] = &cinfMap;
    SystemAPISets[HT_WNETENUM] = &CinfWnetEnum;
	KInfoTable[KINX_APISETS] = (DWORD)SystemAPISets;
    HeapInit();
    InitMemoryPool();
    ProcInit();
    SchedInit();
    ProfInit();
#ifdef DEBUG
	lpWriteDebugStringFunc(TEXT("Scheduling the first thread.\r\n"));
#endif
}

void KernelInit2(void) {
	LPVOID p1, p2, p3,p4,p5;

	DEBUGMSG(1, (TEXT("KernelInit2: pCurThread=%8.8lx hCurThread=%8.8lx hCurProc=%8.8lx\r\n"),
	        pCurThread, hCurThread, hCurProc));
	/* initialize kernel debugger subsystem if present. */
	if (PKDInit) {
#ifdef DEBUG
		lpWriteDebugStringFunc(TEXT("Entering KdInit\r\n"));
#endif
	// rameshk
	// added an additional arg KDCleanup and p4,p5
    	if (PKDInit(&p1, &p2, &p3,(LPVOID)KDCleanup,&p4,&p5)) {
    		KDPrintString = (BOOLEAN (*)(IN LPCWSTR))p1;
    		KDUpdateSymbols = (void (*)(DWORD, BOOL))p2;
    		KDTrap = (ULONG (*)(PEXCEPTION_RECORD, CONTEXT *, BOOLEAN))p3;
	   		KDUpdateSymbols((DWORD)ProcArray[0].BasePtr+1, FALSE); // the kernel

			if ((CommDev[KERNEL_SVC_DBGMSG] == KERNEL_COMM_SERIAL) &&
				(CommDev[KERNEL_SVC_KDBG] == KERNEL_COMM_SERIAL))
				lpWriteDebugStringFunc = KDPrintString;
			ReadyForStrings = TRUE;
    	}
#ifdef DEBUG
		lpWriteDebugStringFunc(TEXT("KdInit Done\r\n"));
#endif
	}
    InitializeCriticalSection(&ODScs);
    InitializeCriticalSection(&CompCS);
	InitializeCriticalSection(&PhysCS);
    InitializeCriticalSection(&VAcs);
    InitializeCriticalSection(&LLcs);
    InitializeCriticalSection(&ModListcs);
    InitializeCriticalSection(&ppfscs);
    InitializeCriticalSection(&RFBcs);
    InitializeCriticalSection(&MapCS);
    InitializeCriticalSection(&NameCS);
    InitializeCriticalSection(&MapNameCS);
    InitializeCriticalSection(&DbgApiCS);
    InitializeCriticalSection(&PagerCS);
    InitializeCriticalSection(&PageOutCS);
    InitializeCriticalSection(&WriterCS);

    // If logging enabled (NKPROF), initialize.
    CELOGINIT();

#ifdef MEMTRACKING
	MemTrackInit();
#endif
}

void SC_SetExceptionHandler(PEXCEPTION_ROUTINE per) {
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetExceptionHandler failed due to insufficient trust\r\n"));
	if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
		pCurProc->pfnEH = per;
}

BOOL SC_Nop() {
    return TRUE;
}

BOOL SC_NotSupported() {
    KSetLastError(pCurThread,ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL SC_SetHardwareWatch(LPVOID vAddr, DWORD flags)
{
	extern SetCPUHardwareWatch(LPVOID,DWORD);
#ifdef MIPS
	LPVOID pAddr;
	vAddr = MapPtrProc(vAddr, pCurProc);
	pAddr = VerifyAccess(vAddr, VERIFY_KERNEL_OK|VERIFY_READ_FLAG, 0xffffffff);
	SetCPUHardwareWatch(pAddr, flags);
	return TRUE;
#elif x86
    // Make sure this runs without preemption
	vAddr = MapPtrProc(vAddr, pCurProc);
	SetCPUHardwareWatch(vAddr, flags);
	return TRUE;
#else
	return FALSE;
#endif
}

BOOL SC_SetKMode(BOOL newMode)
{
	PCALLSTACK pcstk;
	BOOL bRet;
	// Retrive the previous mode from the callstack structure because the thread is now
	// in kernel mode because it is running here. Then update the callstack to switch to
	// the desired mode when we return.
	if (bAllKMode)
		bRet = 1;
	else {
		pcstk = pCurThread->pcstkTop;
		bRet = (DWORD)pcstk->pprcLast == KERNEL_MODE;
		ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetKMode failed due to insufficient trust\r\n"));
		if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
			pcstk->pprcLast = (HANDLE)(newMode ? KERNEL_MODE : USER_MODE);
	}
	return bRet;
}

void SC_PowerOffSystem(void) {
	if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
		PowerOffFlag = TRUE;
}

BOOL SC_SetPowerOffHandler(FARPROC pfn)
{
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetPowerOffHandler failed due to insufficient trust\r\n"));
	if ((pCurProc->bTrustLevel == KERN_TRUST_FULL) && !PowerProc) {
		PowerKey = GETCURKEY();
		PowerFunc = pfn;
		PowerProc = pCurProc;
		return TRUE;
	}
	return FALSE;
}

BOOL SC_SetGwesPowerHandler(FARPROC pfn)
{
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetGwesPowerHandler failed due to insufficient trust\r\n"));
	if ((pCurProc->bTrustLevel == KERN_TRUST_FULL) && !GwesPowerProc) {
		GwesPowerKey = GETCURKEY();
		GwesPowerFunc = pfn;
		GwesPowerProc = pCurProc;
		return TRUE;
	}
	return FALSE;
}

BOOL SC_SetWDevicePowerHandler(FARPROC pfn)
{
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetWDevicePowerHandler failed due to insufficient trust\r\n"));
	if ((pCurProc->bTrustLevel == KERN_TRUST_FULL) && !WDevicePowerProc) {
		WDevicePowerKey = GETCURKEY();
		WDevicePowerFunc = pfn;
		WDevicePowerProc = pCurProc;
		return TRUE;
	}
	return FALSE;
}

/* Invoke power on/off handler in the registered process space. Note that the
 * current thread is used to set the access key and process address space information
 * but that the thread is not actually scheduled in this state. Since a reschedule
 * will always occur after a power off, it is not necessary to call SetCPUASID to
 * restore the address space information when we are done.
 */
void CallPowerProc(PTHREAD pth, ACCESSKEY aky, PPROCESS pprc, FARPROC pfn, BOOL bOff)
{
	ACCESSKEY oldAky;
	PPROCESS pOldProc;

	if (pfn) {
		oldAky = pth->aky;
		pOldProc = pth->pProc;
		pth->aky = aky;
		pth->pProc = pprc;
		SetCPUASID(pCurThread);
		(*pfn)(bOff);		/* set power off state */
		pth->aky = oldAky;
		pth->pProc = pOldProc;
	}
}

extern void InitClock(void);

void DoPowerOff(void)
{
	extern DWORD bLastIdle;
	/* Tell GWES and FileSys that we are turning power off */
	bLastIdle = 0;
	RETAILMSG(1, (TEXT("Powering Off system:\r\n")));
	RETAILMSG(1, (TEXT("  Calling GWES power proc.\r\n")));
	CallPowerProc(pCurThread, GwesPowerKey, GwesPowerProc, GwesPowerFunc, TRUE);
	RETAILMSG(1, (TEXT("  Calling WDevice power proc.\r\n")));
	CallPowerProc(pCurThread, WDevicePowerKey, WDevicePowerProc, WDevicePowerFunc, TRUE);
	RETAILMSG(1, (TEXT("  Calling Filesys power proc.\r\n")));
	CallPowerProc(pCurThread, PowerKey, PowerProc, PowerFunc, TRUE);
	RETAILMSG(1, (TEXT("  Calling OEMPowerOff...\r\n")));

#if SHx
	FlushCache();
#endif
	OEMPowerOff();
	RETAILMSG(1, (TEXT("Back from OEMPowerOff\r\n")));
#if SHx
	FlushCache();
#endif
	InitClock();				/* reinitialize timer h/w */
	/* Tell GWES and FileSys that we are turning power back on */
	RETAILMSG(1, (TEXT("  Calling Filesys power proc.\r\n")));
	CallPowerProc(pCurThread, PowerKey, PowerProc, PowerFunc, FALSE);
	RETAILMSG(1, (TEXT("  Calling WDevice power proc.\r\n")));
	CallPowerProc(pCurThread, WDevicePowerKey, WDevicePowerProc, WDevicePowerFunc, FALSE);
	RETAILMSG(1, (TEXT("  Calling GWES power proc.\r\n")));
	CallPowerProc(pCurThread, GwesPowerKey, GwesPowerProc, GwesPowerFunc, FALSE);
	RETAILMSG(1, (TEXT("  Returning to normally scheduled programming.\r\n")));

    // If platform supports debug Ethernet, turn interrupts back on
    if (pEdbgInitializeInterrupt)
        pEdbgInitializeInterrupt();
}

// rameshk
// This function is called when kdstub gets Terminate Message from windbg
// It simply reverts back KDTrap,KDUpdateSymbols and KDPrintString function pointers
// to FAKE functions.
BOOLEAN KDCleanup(void)
{

        KDTrap=FakeKDTrap;
        KDUpdateSymbols=FakeUpdateSymbols;
        KDPrintString=FakePrintString;
        return TRUE;
}

BOOLEAN FakePrintString(IN LPCWSTR Output)
{
        return TRUE;
}

BOOLEAN FakeSetLoadSymbolsFlag()
{
return TRUE;
}

BOOLEAN FakeResetLoadSymbolsFlag()
{
return TRUE;
}

VOID (*pWriteDebugLED)(WORD wIndex, DWORD dwPattern);

VOID SC_WriteDebugLED(WORD wIndex, DWORD dwPattern) {
   if( pWriteDebugLED )
       pWriteDebugLED( wIndex, dwPattern );
}

// Print debug message using debug Ethernet card
void
EdbgWriteDebugString(unsigned short *str) {
    UCHAR FmtBuf[256+sizeof(EDBG_DBGMSG_INFO)];
    EDBG_DBGMSG_INFO *pDbgInfo = (EDBG_DBGMSG_INFO *)FmtBuf;
    // Prepend a header so that application on the other side can get timing
    // and thread info.
    pDbgInfo->dwLen      = sizeof(EDBG_DBGMSG_INFO);
    pDbgInfo->dwThreadId = (DWORD)pCurThread;
    pDbgInfo->dwProcId   = (DWORD)pCurProc;
    pDbgInfo->dwTimeStamp = CurMSec;
    KUnicodeToAscii(FmtBuf+sizeof(EDBG_DBGMSG_INFO),str,256);
    pEdbgSend(EDBG_SVC_DBGMSG, FmtBuf, sizeof(EDBG_DBGMSG_INFO)+strlenW(str)+1);
}

/* Routine to change communications device used for debug kernel services (debug msgs,
 * PPFS, kernel debugger).  Be careful with PPSH and windbg - if they are configured
 * to run over a serial or parallel link with debug messages, an alternate debug message
 * function must be called to format the data.  Note that windbg and ppsh cannot
 * be run together over serial or parallel.
 */
BOOL
SetKernelCommDev (UCHAR Service, UCHAR CommDevice)
{
    switch (Service) {
        case KERNEL_SVC_DBGMSG:
            switch(CommDevice) {
                case KERNEL_COMM_SERIAL:
                    if (CommDev[KERNEL_SVC_PPSH] == KERNEL_COMM_SERIAL)
                        lpWriteDebugStringFunc = PpfsWriteDebugString;
                    else if (ReadyForStrings && (CommDev[KERNEL_SVC_KDBG] == KERNEL_COMM_SERIAL))
                        lpWriteDebugStringFunc = KDPrintString;
                    else
                        lpWriteDebugStringFunc = OEMWriteDebugString;
                    break;
                case KERNEL_COMM_PARALLEL:
                    lpWriteDebugStringFunc = PpfsWriteDebugString;
                    break;
                case KERNEL_COMM_ETHER:
                    if (pEdbgRegisterDfltClient &&
                        pEdbgRegisterDfltClient(EDBG_SVC_DBGMSG,0,NULL,NULL))
                        lpWriteDebugStringFunc = EdbgWriteDebugString;
                    else
                        return FALSE;
                    break;
            }
            break;

        case KERNEL_SVC_PPSH:
            switch(CommDevice) {
                case KERNEL_COMM_SERIAL:
                    lpParallelPortSendByteFunc   = OEMWriteDebugByte;
                    lpParallelPortGetByteFunc    = OEMReadDebugByte;
                    if (CommDev[KERNEL_SVC_DBGMSG] == KERNEL_COMM_SERIAL)
                        lpWriteDebugStringFunc = PpfsWriteDebugString;
                    break;
                case KERNEL_COMM_PARALLEL:
                    lpParallelPortSendByteFunc   = OEMParallelPortSendByte;
                    lpParallelPortGetByteFunc    = OEMParallelPortGetByte;
                    break;
                case KERNEL_COMM_ETHER:
                    if (pEdbgRegisterDfltClient &&
                        pEdbgRegisterDfltClient(EDBG_SVC_PPSH,0,&PpfsEthBuf,&PpfsEthBuf)) {
                        lpParallelPortSendByteFunc   = ppfs_send_byte_ether;
                        lpParallelPortGetByteFunc    = ppfs_get_byte_ether;
                        BufferedPPFS = TRUE;
                    }
                    else
                        return FALSE;
                    break;
            }
            break;

        case KERNEL_SVC_KDBG:
            // For debugger, just set CommDev, KD will look at this in
            // KDInit and set up communications appropriately. So, this will
            // only work at init (e.g. in OEMInit) time...
            break;
    }
    CommDev[Service] = CommDevice;
    return TRUE;
}
