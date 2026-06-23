/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

psyscall.h

Abstract:

Private portion of syscall.h

Notes: 


--*/

#ifndef _PRIV_SYSCALL_H__
#define _PRIV_SYSCALL_H__


#ifdef __cplusplus
extern "C" {
#endif

/* Masks for encoding of implicit handle API calls */
#if defined(PPC)
#define FIRST_METHOD 	0xFFFFFC00
#define APICALL_SCALE	4
#elif defined(MIPS)
#define FIRST_METHOD 	0xFFFFFC02
#define APICALL_SCALE	4
#elif defined(x86)
#define FIRST_METHOD 	0xFFFFFE00
#define APICALL_SCALE	2
#elif defined(ARM)
#define FIRST_METHOD 	0xF0010000
#define APICALL_SCALE	4
#elif defined(SHx)
#define FIRST_METHOD 	0xFFFFFE01
#define APICALL_SCALE	2
#else
#error "Unknown CPU type"
#endif

#define SYSCALL_RETURN	(FIRST_METHOD-APICALL_SCALE)

#define HANDLE_SHIFT 	8
#define METHOD_MASK 0x00FF
#define HANDLE_MASK 0x003F

// Special handle indicies used for "typed" handle calls
#define HT_EVENT				4		// Event handle type
#define HT_MUTEX				5		// Mutex handle type
#define HT_APISET				6		// kernel API set handle type
#define HT_FILE					7		// open file handle type
#define HT_FIND					8		// FindFirst handle type
#define HT_DBFILE				9		// open database handle type
#define HT_DBFIND				10		// database find handle type
#define HT_SOCKET				11		// WinSock open socket handle type
#define HT_INTERFACE			12
#define HT_SEMAPHORE			13		// Semaphore handle type
#define HT_FSMAP				14		// mapped files
#define HT_WNETENUM             15      // Net Resource Enumeration

#define REGAPISET_KPSL		0x40000000
#define REGAPISET_TYPEONLY	0x80000000

/* These macros generate the special addresses to call for a given
 * method index for handle based APIs or a handle index and method
 * index pair for an implicit handle API call.
 */
#ifdef WINCEMACRO 
#define METHOD_CALL(mid)        (FIRST_METHOD + (mid)*APICALL_SCALE)
#define IMPLICIT_CALL(hid, mid) (FIRST_METHOD - ((hid)<<HANDLE_SHIFT | (mid))*APICALL_SCALE)
#endif
// Private version available even without WINCEMACRO being on
#define PRIV_IMPLICIT_CALL(hid, mid) (FIRST_METHOD - ((hid)<<HANDLE_SHIFT | (mid))*APICALL_SCALE)

/* These macros will generate a C declaration for psuedo function to
 * invoke a given method.  To define a function:
 *  #define Function METHOD_DECL(return_type, method index,
 *      (type1 arg1, type2 arg2, etc))
 * or
 *  #define Function IMPLICIT_DECL(return type, handle index, method index,
 *      (type1 arg1, type2 arg2, etc))
 */
#define METHOD_DECL(type, mid, args) (*(type (*)args)METHOD_CALL(mid))
#define IMPLICIT_DECL(type, hid, mid, args) (*(type (*)args)IMPLICIT_CALL(hid, mid))
// Private version available even without WINCEMACRO being on
#define PRIV_IMPLICIT_DECL(type, hid, mid, args) (*(type (*)args)PRIV_IMPLICIT_CALL(hid, mid))

/* Defines used to generate API calls to the Win32 system handle */
#ifndef WIN32_CALL
#ifdef BUILDING_DEBUGGER
#define WIN32_CALL(type, api, args)		IMPLICIT_DECL(type, SH_WIN32, W32_ ## api, args)
#endif
#endif
// Private version available even without WINCEMACRO being on
#define PRIV_WIN32_CALL(type, api, args) PRIV_IMPLICIT_DECL(type, SH_WIN32, W32_ ## api, args)

#define _THREAD_CALL(type, api, args)	IMPLICIT_DECL(type, SH_CURTHREAD, ID_THREAD_ ## api, args)
#define _PROCESS_CALL(type, api, args)	IMPLICIT_DECL(type, SH_CURPROC, ID_PROC_ ## api, args)
#define _EVENT_CALL(type, api, args)	IMPLICIT_DECL(type, HT_EVENT, ID_EVENT_ ## api, args)
#define _APISET_CALL(type, api, args)	IMPLICIT_DECL(type, HT_APISET, ID_APISET_ ## api, args)
#define _FSMAP_CALL(type, api, args)	IMPLICIT_DECL(type, HT_FSMAP, ID_FSMAP_ ## api, args)
#define _MUTEX_CALL(type, api, args)	IMPLICIT_DECL(type, HT_MUTEX, ID_MUTEX_ ## api, args)
#define _SEMAPHORE_CALL(type, api, args) IMPLICIT_DECL(type, HT_SEMAPHORE, ID_SEMAPHORE_ ## api, args)


#define W32_CreateAPISet		 2
#define W32_VirtualAlloc		 3
#define W32_VirtualFree			 4
#define W32_VirtualProtect		 5
#define W32_VirtualQuery		 6
#define W32_VirtualCopy			 7
#define W32_LoadLibraryW		 8
#define W32_FreeLibrary			 9
#define W32_GetProcAddressW		10
#define W32_ThreadAttachAllDLLs 11
#define W32_ThreadDetachAllDLLs 12
#define W32_GetTickCount		13
#define W32_OutputDebugStringW	14
#define W32_TlsCall				15
#define W32_GetSystemInfo		16
#define W32_ropen				17
#define W32_rread				18
#define W32_rwrite				19
#define W32_rlseek				20
#define W32_rclose				21
#define W32_RegisterDbgZones	22
#define W32_NKvDbgPrintfW		23
#define W32_ProfileSyscall		24
#define W32_FindResource		25
#define W32_LoadResource		26
#define W32_SizeofResource		27
#define W32_GetRealTime			28
#define W32_SetRealTime			29
#define W32_ProcessDetachAllDLLs 30
#define W32_ExtractResource		31
#define W32_GetRomFileInfo		32
#define W32_GetRomFileBytes		33
#define W32_CacheSync			34
#define W32_AddTrackedItem		35
#define W32_DeleteTrackedItem	36
#define W32_PrintTrackedItem	37
#define W32_GetKPhys			38
#define W32_GiveKPhys			39
#define W32_SetExceptionHandler 40
#define W32_RegisterTrackedItem 41
#define W32_FilterTrackedItem	42
#define W32_SetKernelAlarm		43
#define W32_RefreshKernelAlarm	44
#define W32_CeGetRandomSeed		45
#define W32_CloseProcOE			46
#define W32_SetGwesOOMEvent		47
#define W32_StringCompress		48
#define W32_StringDecompress	49
#define W32_BinaryCompress		50
#define W32_BinaryDecompress	51
#define W32_CreateEvent			52
#define W32_CreateProc			53
#define W32_CreateThread		54
#define W32_InputDebugCharW		55
#define W32_TakeCritSec			56
#define W32_LeaveCritSec		57
#define W32_WaitForMultiple		58
#define W32_MapPtrToProcess 	59
#define W32_MapPtrUnsecure		60
#define W32_GetProcFromPtr		61
#define W32_IsBadPtr			62
#define W32_GetProcAddrBits		63
#define W32_GetFSHeapInfo		64
#define W32_OtherThreadsRunning 65
#define W32_KillAllOtherThreads 66
#define W32_GetOwnerProcess		67
#define W32_GetCallerProcess 	68
#define W32_GetIdleTime			69
#define W32_SetLowestScheduledPriority 70
#define W32_IsPrimaryThread		71
#define W32_SetProcPermissions	72
#define W32_GetCurrentPermissions 73
#define W32_SetDaylightTime		75
#define W32_SetTimeZoneBias		76
#define W32_SetCleanRebootFlag	77
#define W32_CreateCrit			78
#define W32_PowerOffSystem		79
#define W32_CreateMutex			80
#define W32_SetDbgZone			81
#define W32_Sleep				82
#define W32_TurnOnProfiling		83
#define W32_TurnOffProfiling	84
#define W32_CeGetCurrentTrust 	85
#define W32_CeGetCallerTrust 	86
#define W32_NKTerminateThread	87
#define W32_SetLastError		88
#define W32_GetLastError		89
#define W32_GetProcName			90
#define W32_TerminateSelf		91
#define W32_CloseAllHandles		92
#define W32_SetHandleOwner		93
#define W32_LoadDriver			94
#define W32_CreateFileMapping	95
#define W32_UnmapViewOfFile		96
#define W32_FlushViewOfFile		97
#define W32_CreateFileForMappingW 98
#define W32_KernelIoControl		99
#define W32_MapUncompressedFileW 100
#define W32_PPSHRestart			101
#define W32_SignalStarted		102
#define W32_UpdateNLSInfo		103
#define W32_ConnectDebugger		104
#define W32_InterruptInitialize 105
#define W32_InterruptDone		106
#define W32_InterruptDisable	107
#define W32_SetKMode			108
#define W32_SetPowerOffHandler  109
#define W32_SetGwesPowerHandler 110
#define W32_SetHardwareWatch	111
#define W32_QueryAPISetID		112
#define W32_PerformCallBack 	113
#define W32_RaiseException		114
#define W32_GetCallerProcessIndex		115
#define W32_WaitForDebugEvent	116
#define W32_ContinueDebugEvent	117
#define W32_DebugNotify			118
#define W32_OpenProcess			119
#define W32_THCreateSnapshot	120
#define W32_THGrow				121
#define W32_NotifyForceCleanboot 122
#define W32_DumpKCallProfile	123
#define W32_GetProcessVersion	124
#define W32_GetModuleFileNameW	125
#define W32_QueryPerformanceCounter 126
#define W32_QueryPerformanceFrequency 127
#define W32_KernExtractIcons 128
#define W32_ForcePageout 129
#define W32_GetThreadTimes 130
#define W32_GetModuleHandleW 131
#define W32_SetWDevicePowerHandler 132
#define W32_SetStdioPathW 133
#define W32_GetStdioPathW 134
#define W32_ReadRegistryFromOEM 135
#define W32_WriteRegistryToOEM 136
#define W32_WriteDebugLED 137
#define W32_LockPages 138
#define W32_UnlockPages 139
#define W32_VirtualSetPageFlags 140
#define W32_SetRAMMode 141
#define W32_SetStoreQueueBase 142
#define W32_FlushViewOfFileMaybe 143
#define W32_GetProcAddressA 144
#define W32_GetCommandLineW 145
#define W32_DisableThreadLibraryCalls 146
#define W32_CreateSemaphoreW 147
#define W32_LoadLibraryExW 148
#define W32_PerformCallForward 149
#define W32_CeMapArgumentArray 150
#define W32_ProcGetIndex 152
#define W32_RegisterGwesHandler 153
#define W32_CeLogData 156
#define W32_CeLogSetZones 157
#define W32_CeModuleJit 158
#define W32_CeSetExtendedPdata 159
#define W32_VerQueryValueW 160
#define W32_GetFileVersionInfoSizeW 161
#define W32_GetFileVersionInfoW 162
#define W32_CreateLocaleView 163
#define W32_CeLogReSync 164

/* Common syscalls */

#define ID_ALLHANDLE_FREE        0
#define ID_ALLHANDLE_WAIT        1	// no longer used, but still reserved for future use

#define ID_HCALL0   2
#define ID_HCALL1   3
#define ID_HCALL2   4
#define ID_HCALL3   5
#define ID_HCALL4   6
#define ID_HCALL5   7
#define ID_HCALL6   8
#define ID_HCALL7   9
#define ID_HCALL8   10
#define ID_HCALL9   11
#define ID_HCALL10  12
#define ID_HCALL11  13

/* Process syscalls */

#define ID_PROC_TERMINATE           ID_HCALL0
#define ID_PROC_GETRETCODE			ID_HCALL1
#define ID_PROC_FLUSHICACHE			ID_HCALL3
#define ID_PROC_READMEMORY			ID_HCALL4
#define ID_PROC_WRITEMEMORY			ID_HCALL5
#define ID_PROC_DEBUGACTIVE			ID_HCALL6

/* Thread syscalls */

#define ID_THREAD_SUSPEND           ID_HCALL0
#define ID_THREAD_RESUME            ID_HCALL1
#define ID_THREAD_SETTHREADPRIO     ID_HCALL2
#define ID_THREAD_GETTHREADPRIO     ID_HCALL3
#define ID_THREAD_GETRETCODE        ID_HCALL4
#define ID_THREAD_GETCONTEXT		ID_HCALL5
#define ID_THREAD_SETCONTEXT		ID_HCALL6
#define ID_THREAD_TERMINATE			ID_HCALL7
#define ID_THREAD_CEGETPRIO			ID_HCALL8
#define ID_THREAD_CESETPRIO			ID_HCALL9
#define ID_THREAD_CEGETQUANT		ID_HCALL10
#define ID_THREAD_CESETQUANT		ID_HCALL11

/* Event syscalls */

#define ID_EVENT_MODIFY             ID_HCALL0
#define ID_EVENT_ADDACCESS			ID_HCALL1

/* Apiset syscalls */

#define ID_APISET_REGISTER			ID_HCALL0
#define ID_APISET_CREATEHANDLE		ID_HCALL1
#define ID_APISET_VERIFY			ID_HCALL2

/* mapped file syscalls */

#define ID_FSMAP_MAPVIEWOFFILE		ID_HCALL0

/* mutex syscalls */

#define ID_MUTEX_RELEASEMUTEX		ID_HCALL0

/* semaphore syscalls */

#define ID_SEMAPHORE_RELEASESEMAPHORE	ID_HCALL0

#ifdef __cplusplus
}
#endif

#endif // _PRIV_SYSCALL_H__

