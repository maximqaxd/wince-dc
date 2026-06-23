/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mkfuncs.h

Abstract:  

Notes: 


--*/
#ifndef __MKFUNCS_H__
#define __MKFUNCS_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

#define CloseHandle METHOD_DECL(BOOL, ID_ALLHANDLE_FREE, (HANDLE hObject))
#ifdef WIN32_CALL
#define SetLastError WIN32_CALL(VOID, SetLastError, (DWORD))
#elif !defined(COREDLL)
#define SetLastError PRIV_WIN32_CALL(VOID, SetLastError, (DWORD))
#endif
#define CloseAllHandles PRIV_WIN32_CALL(void, CloseAllHandles, (void))
#define RaiseException PRIV_WIN32_CALL(VOID, RaiseException, (DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD cArgs, CONST DWORD *lpArgs))

#ifndef IN_KERNEL

#include <toolhelp.h>

#define IsBadCodePtr(lpfnProc) IsBadPtr(VERIFY_EXECUTE_FLAG, (LPBYTE)(lpfnProc), 1)
#define IsBadReadPtr(lpvPtr, cbBytes) IsBadPtr(VERIFY_READ_FLAG, (LPBYTE)(lpvPtr), (cbBytes))
#define IsBadWritePtr(lpvPtr, cbBytes) IsBadPtr(VERIFY_WRITE_FLAG, (LPBYTE)(lpvPtr), (cbBytes))

#ifdef WIN32_CALL

// WIN32 calls
#define CreateAPISet WIN32_CALL(HANDLE, CreateAPISet, \
	(char acName[4], USHORT cFunctions, const PFNVOID *ppfnMethods, const DWORD *pdwSig))
#define VirtualAlloc WIN32_CALL(LPVOID, VirtualAlloc, \
    (LPVOID lpvAddress, DWORD cbSize, DWORD fdwAllocationType, DWORD fdwProtect))
#define VirtualFree WIN32_CALL(BOOL, VirtualFree, \
    (LPVOID lpvAddress, DWORD cbSize, DWORD fdwFreeType))
#define VirtualProtect WIN32_CALL(BOOL, VirtualProtect, \
    (LPVOID lpvAddress, DWORD cbSize, DWORD fdwNewProtect, PDWORD pfdwOldProtect))
#define VirtualQuery WIN32_CALL(DWORD, VirtualQuery, \
    (LPCVOID lpvAddress, PMEMORY_BASIC_INFORMATION pmbiBuffer, DWORD cbLength))
#define VirtualCopy WIN32_CALL(BOOL, VirtualCopy, \
	(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect))
#ifdef SH4
#define VirtualSetPageFlags WIN32_CALL(BOOL, VirtualSetPageFlags, \
	(LPVOID lpvAddress, DWORD cbSize, DWORD dwFlags, LPDWORD lpdwOldFlags))
#define SetRAMMode WIN32_CALL(BOOL, SetRAMMode, \
	(BOOL bEnable, LPVOID *lplpvAddress, LPDWORD lpLength))
#define SetStoreQueueBase WIN32_CALL(LPVOID, SetStoreQueueBase, (DWORD dwPhysPage))
#endif
#define CeModuleJit WIN32_CALL(DWORD, CeModuleJit,(LPCWSTR, LPWSTR, HANDLE*))

#define LockPages WIN32_CALL(BOOL, LockPages, \
    (LPVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions))
#define UnlockPages WIN32_CALL(BOOL, UnlockPages, \
    (LPVOID lpvAddress, DWORD cbSize))

#define LoadLibraryW WIN32_CALL(HINSTANCE, LoadLibraryW, (LPCTSTR lpszFile))
#define FreeLibrary WIN32_CALL(BOOL, FreeLibrary, (HANDLE hInst))
#define GetProcAddressW WIN32_CALL(FARPROC, GetProcAddressW, (HANDLE hInst, LPCWSTR lpszProc))
#define ThreadAttachAllDLLs WIN32_CALL(void, ThreadAttachAllDLLs, (void))
#define ThreadDetachAllDLLs WIN32_CALL(void, ThreadDetachAllDLLs, (void))
#define GetTickCount WIN32_CALL(DWORD, GetTickCount, (VOID))
#define OutputDebugStringW WIN32_CALL(VOID, OutputDebugStringW, (LPCWSTR str))
#define TlsCall WIN32_CALL(DWORD, TlsCall, (DWORD,DWORD))
#define GetSystemInfo WIN32_CALL(VOID, GetSystemInfo, (LPSYSTEM_INFO lpSystemInfo))
#define U_ropen WIN32_CALL(int, ropen, (const WCHAR *, UINT))
#define U_rread WIN32_CALL(int, rread, (int, BYTE *, int))
#define U_rwrite WIN32_CALL(int, rwrite, (int, BYTE *, int))
#define U_rlseek WIN32_CALL(int, rlseek, (int, int, int))
#define U_rclose WIN32_CALL(int, rclose, (int))
#define RegisterDbgZones WIN32_CALL(BOOL, RegisterDbgZones, (HMODULE hMod, LPDBGPARAM lpdbgparam))
#define NKvDbgPrintfW WIN32_CALL(VOID, NKvDbgPrintfW, (LPCWSTR lpszFmt, ...))
#define ProfileSyscall WIN32_CALL(VOID, ProfileSyscall, (LPDWORD))
#define CeLogData WIN32_CALL(VOID, CeLogData, (BOOL fTimeStamp, WORD wID, PVOID pData, WORD wLen, DWORD dwZoneUser, DWORD dwZoneCE, WORD wFlag, BOOL fFlagged))
#define CeLogSetZones WIN32_CALL(VOID, CeLogSetZones, (DWORD dwZoneUser, DWORD dwZoneCE, DWORD dwZoneProcess))
#define CeLogReSync WIN32_CALL(VOID, CeLogReSync, (void))
#define FindResourceW WIN32_CALL(HRSRC, FindResource, (HANDLE hModule, LPCWSTR lpszName, LPCWSTR lpszType))
#define LoadResource WIN32_CALL(HGLOBAL, LoadResource, (HANDLE hModule, HANDLE hRsrc))
#define SizeofResource WIN32_CALL(DWORD, SizeofResource, (HANDLE hModule, HANDLE hRsrc))
#define GetRealTime WIN32_CALL(BOOL, GetRealTime,(LPSYSTEMTIME))
#define SetRealTime WIN32_CALL(BOOL, SetRealTime,(LPSYSTEMTIME))
#define ProcessDetachAllDLLs WIN32_CALL(void, ProcessDetachAllDLLs, (void))
#define ExtractResource WIN32_CALL(LPVOID, ExtractResource, \
	(LPCWSTR lpszFile, LPCWSTR lpszName, LPCWSTR lpszType))
#define KernExtractIcons WIN32_CALL(UINT, KernExtractIcons, \
	(LPCWSTR lpszFile, int nIconIndex, LPBYTE *pIconLarge, LPBYTE *pIconSmall, CALLBACKINFO *pcbi))
#define GetRomFileInfo WIN32_CALL(BOOL, GetRomFileInfo, \
	(DWORD type, LPWIN32_FIND_DATA  lpfd, DWORD count))
#define GetRomFileBytes WIN32_CALL(DWORD, GetRomFileBytes, \
	(DWORD type, DWORD count, DWORD pos, LPVOID buffer, DWORD nBytesToRead))
#define CacheSync WIN32_CALL(void, CacheSync, (int flags))
#define AddTrackedItem WIN32_CALL(BOOL, AddTrackedItem, \
	(DWORD dwType, HANDLE handle, TRACKER_CALLBACK cb, DWORD dwProc, \
	 DWORD dwSize, DWORD dw1, DWORD dw2))
#define DeleteTrackedItem WIN32_CALL(BOOL, DeleteTrackedItem, (DWORD dwType, HANDLE handle))
#define PrintTrackedItem WIN32_CALL(BOOL, PrintTrackedItem, \
	(DWORD dwFlag, DWORD dwType, DWORD dwProcID, HANDLE handle))
#define GetKPhys WIN32_CALL(BOOL, GetKPhys, (void *ptr, ULONG length))
#define GiveKPhys WIN32_CALL(BOOL, GiveKPhys, (void *ptr, ULONG length))
#define SetExceptionHandler WIN32_CALL(void, SetExceptionHandler, (PEXCEPTION_ROUTINE per))
#define RegisterTrackedItem WIN32_CALL(DWORD, RegisterTrackedItem, (LPWSTR szName))
#define FilterTrackedItem WIN32_CALL(VOID, FilterTrackedItem, (DWORD dwFlags, DWORD dwType, DWORD dwProcID))
#define SetKernelAlarm WIN32_CALL(void, SetKernelAlarm, (HANDLE hEvent, LPSYSTEMTIME lpst))
#define RefreshKernelAlarm WIN32_CALL(void, RefreshKernelAlarm, (void))
#define CloseProcOE WIN32_CALL(void, CloseProcOE, (DWORD))
#define SetGwesOOMEvent WIN32_CALL(VOID, SetGwesOOMEvent, \
	(HANDLE hEvt, DWORD cpLow, DWORD cpCritical, DWORD cpLowBlockSize, \
	 DWORD cpCriticalBlockSize))
#define SetOOMEvent WIN32_CALL(VOID, SetGwesOOMEvent, \
	(HANDLE hEvt, DWORD cpLow, DWORD cpCritical, DWORD cpLowBlockSize, \
	 DWORD cpCriticalBlockSize))
#define StringCompress WIN32_CALL(DWORD, StringCompress, \
	(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout))
#define StringDecompress WIN32_CALL(DWORD, StringDecompress, \
	(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout))
#define BinaryCompress WIN32_CALL(DWORD, BinaryCompress, \
	(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout))
#define BinaryDecompress WIN32_CALL(DWORD, BinaryDecompress, \
	(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout, DWORD skip))
#define CreateEventW WIN32_CALL(HANDLE, CreateEvent, \
	(LPSECURITY_ATTRIBUTES lpsa, BOOL fManual, BOOL fInit, LPCWSTR lpName))
#define CreateProcessW WIN32_CALL(BOOL, CreateProc, \
	(LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, \
	 BOOL, DWORD, LPVOID, LPWSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION))
#define CreateThread WIN32_CALL(HANDLE, CreateThread, \
	(LPSECURITY_ATTRIBUTES lpsa, DWORD cbStack, LPTHREAD_START_ROUTINE lpStartAddr, \
	 LPVOID lpvThreadParm, DWORD fdwCreate, LPDWORD lpIDThread))
#define InputDebugCharW WIN32_CALL(int, InputDebugCharW, (VOID))
#define TakeCritSec WIN32_CALL(void, TakeCritSec, (LPCRITICAL_SECTION lpcs))
#define LeaveCritSec WIN32_CALL(void, LeaveCritSec, (LPCRITICAL_SECTION lpcs))
#define MapPtrToProcess WIN32_CALL(LPVOID, MapPtrToProcess, (LPVOID lpv, HANDLE hProc))
#define MapPtrUnsecure WIN32_CALL(LPVOID, MapPtrUnsecure, (LPVOID lpv, HANDLE hProc))
#define GetProcFromPtr WIN32_CALL(HANDLE, GetProcFromPtr, (LPVOID lpv))
#define IsBadPtr WIN32_CALL(BOOL, IsBadPtr, (DWORD flags, LPBYTE ptr, UINT length))
#define GetProcAddrBits WIN32_CALL(DWORD, GetProcAddrBits, (HANDLE hProc))
#define GetFSHeapInfo WIN32_CALL(DWORD, GetFSHeapInfo, (void))
#define OtherThreadsRunning WIN32_CALL(BOOL, OtherThreadsRunning,(void))
#define KillAllOtherThreads WIN32_CALL(VOID, KillAllOtherThreads,(void))
#define GetOwnerProcess WIN32_CALL(HANDLE, GetOwnerProcess, (void))
#define GetCallerProcess WIN32_CALL(HANDLE, GetCallerProcess, (void))
#define CeGetCurrentTrust WIN32_CALL(DWORD, CeGetCurrentTrust, (void))
#define CeGetCallerTrust WIN32_CALL(DWORD, CeGetCallerTrust, (void))
#define GetIdleTime WIN32_CALL(DWORD, GetIdleTime, (void))
#define SetLowestScheduledPriority WIN32_CALL(DWORD, SetLowestScheduledPriority, (DWORD maxprio))
#define IsPrimaryThread WIN32_CALL(BOOL, IsPrimaryThread,(void))
#define SetProcPermissions WIN32_CALL(DWORD, SetProcPermissions, (DWORD))
#define GetCurrentPermissions WIN32_CALL(DWORD, GetCurrentPermissions, (void))
#define SetDaylightTime WIN32_CALL(void, SetDaylightTime, (DWORD dst))
#define SetTimeZoneBias WIN32_CALL(void, SetTimeZoneBias, (DWORD dwBias, DWORD dwDaylightBias))
#define SetCleanRebootFlag WIN32_CALL(void, SetCleanRebootFlag, (void))
#define CreateCrit WIN32_CALL(HANDLE, CreateCrit, (LPCRITICAL_SECTION lpcs))
#define PowerOffSystem WIN32_CALL(void, PowerOffSystem, (void))
#define CreateMutexW WIN32_CALL(HANDLE, CreateMutex, \
	(LPSECURITY_ATTRIBUTES lpsa, BOOL bInitialOwner, LPCTSTR lpName))
#define SetDbgZone WIN32_CALL(BOOL, SetDbgZone, \
	(DWORD dwProcid, LPVOID lpvMod, LPVOID baseptr, DWORD zone, LPDBGPARAM lpdbgTgt))
#define TurnOnProfiling WIN32_CALL(VOID, TurnOnProfiling, (HANDLE hThread))
#define TurnOffProfiling WIN32_CALL(VOID, TurnOffProfiling, (HANDLE hThread))
#define NKTerminateThread WIN32_CALL(void, NKTerminateThread, (DWORD dwExitCode))
#define GetLastError WIN32_CALL(DWORD, GetLastError, (void))
#define GetProcName WIN32_CALL(LPCWSTR, GetProcName, (void))
#define ExitProcess WIN32_CALL(void, TerminateSelf, (void))
#define SetHandleOwner WIN32_CALL(BOOL, SetHandleOwner, (HANDLE h, HANDLE hProc))
#define LoadDriver WIN32_CALL(HINSTANCE, LoadDriver, (LPCTSTR lpszFile))
#define CreateFileMappingW WIN32_CALL(HANDLE, CreateFileMapping, (HANDLE hFile, LPSECURITY_ATTRIBUTES lpsa, \
	DWORD flProtect, DWORD dwMaxSizeHigh, DWORD dwMaxSizeLow, LPCTSTR lpName))
#define UnmapViewOfFile WIN32_CALL(BOOL, UnmapViewOfFile, (LPVOID lpvAddr))
#define FlushViewOfFile WIN32_CALL(BOOL, FlushViewOfFile, (LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush))
#define FlushViewOfFileMaybe WIN32_CALL(BOOL, FlushViewOfFileMaybe, (LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush))
#define CreateFileForMappingW WIN32_CALL(HANDLE, CreateFileForMappingW, (LPCTSTR lpFileName, DWORD dwDesiredAccess, \
	DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, \
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))
#define KernelIoControl WIN32_CALL(BOOL, KernelIoControl, (DWORD dwIoControlCode, \
	LPVOID lpInBuf, DWORD nInBufSize, \
	LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned))
#define MapUncompressedFileW WIN32_CALL(LPVOID, MapUncompressedFileW, (LPCWSTR pFileName, LPDWORD pLen))
#define PPSHRestart WIN32_CALL(void, PPSHRestart, ())
#define SignalStarted WIN32_CALL(void, SignalStarted, (DWORD))
#define UpdateNLSInfo WIN32_CALL(void, UpdateNLSInfo, (DWORD ocp, DWORD acp, DWORD locale))
#define CeGetRandomSeed WIN32_CALL(__int64, CeGetRandomSeed, (void))
#define ConnectDebugger WIN32_CALL(BOOL, ConnectDebugger, (LPVOID pInit))
#define InterruptInitialize WIN32_CALL(BOOL, InterruptInitialize, \
	(DWORD idInt, HANDLE hEvent, LPVOID pvData, DWORD cbData))
#define InterruptDone WIN32_CALL(void, InterruptDone, (DWORD idInt))
#define InterruptDisable WIN32_CALL(void, InterruptDisable, (DWORD idInt))
#define SetKMode WIN32_CALL(BOOL, SetKMode, (BOOL))
#define SetPowerOffHandler WIN32_CALL(BOOL, SetPowerOffHandler, (FARPROC pfn))
#define SetGwesPowerHandler WIN32_CALL(BOOL, SetGwesPowerHandler, (FARPROC pfn))
#define SetWDevicePowerHandler WIN32_CALL(BOOL, SetWDevicePowerHandler, (FARPROC pfn))
#define SetHardwareWatch WIN32_CALL(BOOL, SetHardwareWatch, (LPVOID vAddr, DWORD flags))
#define QueryAPISetID WIN32_CALL(int, QueryAPISetID, (char *pName))
#define GetCallerProcessIndex WIN32_CALL(DWORD, GetCallerProcessIndex, (void))
#define WaitForDebugEvent WIN32_CALL(BOOL, WaitForDebugEvent, (LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds))
#define ContinueDebugEvent WIN32_CALL(BOOL, ContinueDebugEvent, (DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus))
#define DebugNotify WIN32_CALL(void, DebugNotify, (DWORD dwFlags, DWORD data))
#define OpenProcess WIN32_CALL(HANDLE, OpenProcess, (DWORD fdwAccess, BOOL fInherit, DWORD IDProcess))
#define THCreateSnapshot WIN32_CALL(THSNAP *, THCreateSnapshot, (DWORD dwFlags, DWORD dwProcID))
#define THGrow WIN32_CALL (LPBYTE, THGrow, (THSNAP *pSnap, DWORD dwSize))
#ifdef BUILDING_DEBUGGER
#define Sleep WIN32_CALL(VOID, Sleep, (DWORD cMilliseconds))
#define WaitForMultipleObjects WIN32_CALL(DWORD, WaitForMultiple, \
	(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout))
#else
#define KernelSleep WIN32_CALL(VOID, Sleep, (DWORD cMilliseconds))
#define KernelWaitForMultipleObjects WIN32_CALL(DWORD, WaitForMultiple, \
	(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout))
#endif
#define NotifyForceCleanboot WIN32_CALL(void, NotifyForceCleanboot, (void))
#define DumpKCallProfile WIN32_CALL(void, DumpKCallProfile, (DWORD bReset))
#define GetProcessVersion WIN32_CALL(DWORD, GetProcessVersion, (DWORD ProcessId))
#define GetModuleFileNameW WIN32_CALL(DWORD, GetModuleFileNameW, (HANDLE hModule, LPWSTR lpFilename, DWORD nSize))
#define QueryPerformanceCounter WIN32_CALL(BOOL, QueryPerformanceCounter, (LARGE_INTEGER *lpPerformanceCount))
#define QueryPerformanceFrequency WIN32_CALL(BOOL, QueryPerformanceFrequency, (LARGE_INTEGER *lpPerformanceCount))
#define WriteDebugLED WIN32_CALL(void, WriteDebugLED, (WORD wIndex, DWORD dwPattern))
#define ForcePageout WIN32_CALL(void, ForcePageout, (void))
#define GetThreadTimes WIN32_CALL(BOOL, GetThreadTimes, (HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime))
#define GetModuleHandleW WIN32_CALL(HMODULE, GetModuleHandleW, (LPCWSTR lpModuleName))
#define SetStdioPathW WIN32_CALL(BOOL,SetStdioPathW, (DWORD id, LPCWSTR pwszPath))
#define GetStdioPathW WIN32_CALL(BOOL, GetStdioPathW, (DWORD id, PWSTR pwszBuf, LPDWORD lpdwLen))
#define ReadRegistryFromOEM WIN32_CALL(DWORD, ReadRegistryFromOEM, (DWORD dwFlags, LPBYTE pBuf, DWORD len))
#define WriteRegistryToOEM WIN32_CALL (BOOL, WriteRegistryToOEM, (DWORD dwFlags, LPBYTE pBuf, DWORD len))
#define GetProcAddressA WIN32_CALL(FARPROC, GetProcAddressA, (HANDLE hInst, LPCSTR lpszProc))
#define GetCommandLineW WIN32_CALL(LPWSTR, GetCommandLineW, (void))
#define DisableThreadLibraryCalls WIN32_CALL(BOOL, DisableThreadLibraryCalls, (HMODULE hLibModule))
#define CreateSemaphoreW WIN32_CALL(HANDLE, CreateSemaphoreW, (LPSECURITY_ATTRIBUTES lpsa, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName))
#define LoadLibraryExW WIN32_CALL(HMODULE, LoadLibraryExW, (LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags))
#define CeMapArgumentArray WIN32_CALL(BOOL, CeMapArgumentArray, (HANDLE hProc, LPVOID *pArgList, DWORD dwSig))
#define GetProcessIndexFromID WIN32_CALL(DWORD, ProcGetIndex, (HANDLE hProcess))
#define CeSetExtendedPdata WIN32_CALL(BOOL, CeSetExtendedPdata, (LPVOID pData))
#define VerQueryValueW WIN32_CALL(BOOL, VerQueryValueW, (const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen))
#define GetFileVersionInfoSizeW WIN32_CALL(DWORD, GetFileVersionInfoSizeW, (LPWSTR lpFilename, LPDWORD lpdwHandle))
#define GetFileVersionInfoW WIN32_CALL(BOOL, GetFileVersionInfoW, (LPWSTR lpFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData))
#define CreateLocaleView WIN32_CALL(LPBYTE, CreateLocaleView, (BOOL bFirst))

#elif defined(COREDLL)

#ifdef SH4
BOOL xxx_VirtualSetPageFlags(LPVOID lpvAddress, DWORD cbSize, DWORD dwFlags, LPDWORD lpdwOldFlags);
#define VirtualSetPageFlags xxx_VirtualSetPageFlags
BOOL xxx_SetRAMMode(BOOL bEnable, LPVOID *lplpvAddress, LPDWORD lpLength);
#define SetRAMMode xxx_SetRAMMode
LPVOID xxx_SetStoreQueueBase(DWORD dwPhysPage);
#define SetStoreQueueBase xxx_SetStoreQueueBase
#endif

LPBYTE xxx_CreateLocaleView(BOOL bFirst);
#define CreateLocaleView xxx_CreateLocaleView
DWORD xxx_CeModuleJit(LPCWSTR, LPWSTR, HANDLE*);
#define CeModuleJit xxx_ModuleJit
int xxx_QueryAPISetID(char *pName);
#define QueryAPISetID xxx_QueryAPISetID
HANDLE WINAPI xxx_CreateEventW(LPSECURITY_ATTRIBUTES lpsa, BOOL bManualReset,BOOL bInitialState, LPCWSTR lpName);
#define CreateEventW xxx_CreateEventW
VOID WINAPI xxx_Sleep(DWORD dwMilliseconds);
#define Sleep xxx_Sleep
DWORD WINAPI xxx_WaitForMultipleObjects(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout);
#define WaitForMultipleObjects xxx_WaitForMultipleObjects
DWORD WINAPI xxx_WaitForSingleObject(HANDLE hHandle,  DWORD dwMilliseconds);
#define WaitForSingleObject xxx_WaitForSingleObject
DWORD WINAPI xxx_SuspendThread(HANDLE hThread);
#define SuspendThread xxx_SuspendThread
BOOL WINAPI xxx_CeMapArgumentArray(HANDLE hProc, LPVOID *pArgList, DWORD dwSig);
#define CeMapArgumentArray xxx_CeMapArgumentArray
BOOL WINAPI xxx_CeSetExtendedPdata(LPVOID pData);
#define CeSetExtendedPdata xxx_CeSetExtendedPdata
DWORD WINAPI xxx_GetLastError();
#define GetLastError xxx_GetLastError
VOID WINAPI xxx_SetLastError(DWORD dwError);
#define SetLastError xxx_SetLastError
DWORD xxx_TlsCall(DWORD p1, DWORD p2);
#define TlsCall xxx_TlsCall
LPVOID WINAPI xxx_VirtualAlloc(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect);
#define VirtualAlloc xxx_VirtualAlloc
BOOL WINAPI xxx_VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType);
#define VirtualFree xxx_VirtualFree
BOOL WINAPI xxx_VirtualProtect(LPVOID lpAddress, DWORD dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
#define VirtualProtect xxx_VirtualProtect
DWORD WINAPI xxx_VirtualQuery(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength);
#define VirtualQuery xxx_VirtualQuery
BOOL WINAPI xxx_LockPages(LPVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions);
#define LockPages xxx_LockPages
BOOL WINAPI xxx_UnlockPages(LPVOID lpvAddress, DWORD cbSize);
#define UnlockPages xxx_UnlockPages
HINSTANCE WINAPI xxx_LoadLibraryW(LPCWSTR lpLibFileName);
#define LoadLibraryW xxx_LoadLibraryW
HMODULE WINAPI xxx_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
#define LoadLibraryExW xxx_LoadLibraryExW
BOOL WINAPI xxx_FreeLibrary(HMODULE hLibModule);
#define FreeLibrary xxx_FreeLibrary
FARPROC WINAPI xxx_GetProcAddressA(HMODULE hModule, LPCSTR lpProcName);
#define GetProcAddressA xxx_GetProcAddressA
BOOL WINAPI xxx_DisableThreadLibraryCalls(HMODULE hLibModule);
#define DisableThreadLibraryCalls xxx_DisableThreadLibraryCalls
FARPROC WINAPI xxx_GetProcAddressW(HMODULE hModule, LPCWSTR lpProcName);
#define GetProcAddressW xxx_GetProcAddressW
LPWSTR WINAPI xxx_GetCommandLineW(VOID);
#define GetCommandLineW xxx_GetCommandLineW
HRSRC WINAPI xxx_FindResourceW(HMODULE hModule,LPCWSTR lpName, LPCWSTR lpType);
#define FindResourceW xxx_FindResourceW
HGLOBAL WINAPI xxx_LoadResource(HMODULE hModule, HRSRC hResInfo);
#define LoadResource xxx_LoadResource
DWORD WINAPI xxx_SizeofResource(HMODULE hModule, HRSRC hResInfo);
#define SizeofResource xxx_SizeofResource
DWORD WINAPI xxx_GetTickCount();
#define GetTickCount xxx_GetTickCount
DWORD WINAPI xxx_GetProcessVersion(DWORD dwProcessId);
#define GetProcessVersion xxx_GetProcessVersion
DWORD WINAPI xxx_GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
#define GetModuleFileNameW xxx_GetModuleFileNameW
HMODULE WINAPI xxx_GetModuleHandleW(LPCWSTR lpModuleName);
#define GetModuleHandleW xxx_GetModuleHandleW
BOOL WINAPI xxx_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
#define QueryPerformanceCounter xxx_QueryPerformanceCounter
BOOL WINAPI xxx_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency xxx_QueryPerformanceFrequency
VOID WINAPI xxx_WriteDebugLED(WORD wIndex, DWORD dwPattern);
#define WriteDebugLED xxx_WriteDebugLED
void WINAPI xxx_ForcePageout(void);
#define ForcePageout xxx_ForcePageout
BOOL WINAPI xxx_GetThreadTimes(HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
#define GetThreadTimes xxx_GetThreadTimes
VOID WINAPI xxx_OutputDebugStringW(LPCWSTR lpOutputString);
#define OutputDebugStringW xxx_OutputDebugStringW
VOID WINAPI xxx_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);
#define GetSystemInfo xxx_GetSystemInfo
BOOL xxx_RegisterDbgZones(HMODULE hMod, LPDBGPARAM lpdbgparam);
#define RegisterDbgZones xxx_RegisterDbgZones
void xxx_SetDaylightTime(DWORD dst);
#define SetDaylightTime xxx_SetDaylightTime
HANDLE xxx_CreateFileMappingW(HANDLE hFile, LPSECURITY_ATTRIBUTES lpsa, DWORD flProtect, DWORD dwMaxSizeHigh, DWORD dwMaxSizeLow, LPCWSTR lpName);
#define CreateFileMappingW xxx_CreateFileMappingW
HANDLE xxx_CreateFileForMappingW(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#define CreateFileForMappingW xxx_CreateFileForMappingW
BOOL xxx_UnmapViewOfFile(LPVOID lpvAddr);
#define UnmapViewOfFile xxx_UnmapViewOfFile
BOOL xxx_FlushViewOfFile(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush);
#define FlushViewOfFile xxx_FlushViewOfFile
BOOL xxx_FlushViewOfFileMaybe(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush);
#define FlushViewOfFileMaybe xxx_FlushViewOfFileMaybe
LPVOID xxx_MapUncompressedFileW(LPCWSTR pFileName, LPDWORD pLen);
#define MapUncompressedFileW xxx_MapUncompressedFileW
BOOL xxx_KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);
#define KernelIoControl xxx_KernelIoControl
HANDLE xxx_CreateMutexW(LPSECURITY_ATTRIBUTES lpsa, BOOL bInitialOwner, LPCTSTR lpName);
#define CreateMutexW xxx_CreateMutexW
HANDLE xxx_CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpsa, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
#define CreateSemaphoreW xxx_CreateSemaphoreW
HANDLE xxx_CreateAPISet(char acName[4], USHORT cFunctions, const PFNVOID *ppfnMethods, const DWORD *pdwSig);
#define CreateAPISet xxx_CreateAPISet
BOOL xxx_VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);
#define VirtualCopy xxx_VirtualCopy
void xxx_ThreadAttachAllDLLs(void);
#define ThreadAttachAllDLLs xxx_ThreadAttachAllDLLs
void xxx_ThreadDetachAllDLLs(void);
#define ThreadDetachAllDLLs xxx_ThreadDetachAllDLLs
int xxx_U_ropen(const WCHAR *fname, UINT mode);
#define U_ropen xxx_U_ropen
int xxx_U_rread(int fh, BYTE *buf, int len);
#define U_rread xxx_U_rread
int xxx_U_rwrite(int fh, BYTE *buf, int len);
#define U_rwrite xxx_U_rwrite
int xxx_U_rlseek(int fh, int pos, int type);
#define U_rlseek xxx_U_rlseek
int xxx_U_rclose(int fh);
#define U_rclose xxx_U_rclose
VOID xxx_NKvDbgPrintfW(LPWSTR lpszFmt, CONST VOID *lpParms);
#define NKvDbgPrintfW xxx_NKvDbgPrintfW
VOID xxx_ProfileSyscall(LPDWORD d1);
#define ProfileSyscall xxx_ProfileSyscall
VOID xxx_CeLogData(BOOL fTimeStamp, WORD wID, PVOID pData, WORD wLen, DWORD dwZoneUser, DWORD dwZoneCE, WORD wFlag, BOOL bFlagged);
#define CeLogData xxx_CeLogData
VOID xxx_CeLogSetZones(DWORD dwZoneUser, DWORD dwZoneCE, DWORD dwZoneProcess);
#define CeLogSetZones xxx_CeLogSetZones
BOOL xxx_GetRealTime(LPSYSTEMTIME lpst);
#define GetRealTime xxx_GetRealTime
BOOL xxx_SetRealTime(LPSYSTEMTIME lpst);
#define SetRealTime xxx_SetRealTime
void xxx_ProcessDetachAllDLLs(void);
#define ProcessDetachAllDLLs xxx_ProcessDetachAllDLLs
LPVOID xxx_ExtractResource(LPCWSTR lpszFile, LPCWSTR lpszName, LPCWSTR lpszType);
#define ExtractResource xxx_ExtractResource
UINT xxx_KernExtractIcons(LPCWSTR lpszFile, int nIconIndex, LPBYTE *pIconLarge, LPBYTE *pIconSmall, CALLBACKINFO *pcbi);
#define KernExtractIcons xxx_KernExtractIcons
BOOL xxx_GetRomFileInfo(DWORD type, LPWIN32_FIND_DATA  lpfd, DWORD count);
#define GetRomFileInfo xxx_GetRomFileInfo
DWORD xxx_GetRomFileBytes(DWORD type, DWORD count, DWORD pos, LPVOID buffer, DWORD nBytesToRead);
#define GetRomFileBytes xxx_GetRomFileBytes
void xxx_CacheSync(int flags);
#define CacheSync xxx_CacheSync
BOOL xxx_AddTrackedItem(DWORD dwType, HANDLE handle, TRACKER_CALLBACK cb, DWORD dwProc, DWORD dwSize, DWORD dw1, DWORD dw2);
#define AddTrackedItem xxx_AddTrackedItem
BOOL xxx_DeleteTrackedItem(DWORD dwType, HANDLE handle);
#define DeleteTrackedItem xxx_DeleteTrackedItem
BOOL xxx_PrintTrackedItem(DWORD dwFlag, DWORD dwType, DWORD dwProcID, HANDLE handle);
#define PrintTrackedItem xxx_PrintTrackedItem
BOOL xxx_GetKPhys(void *ptr, ULONG length);
#define GetKPhys xxx_GetKPhys
BOOL xxx_GiveKPhys(void *ptr, ULONG length);
#define GiveKPhys xxx_GiveKPhys
void xxx_SetExceptionHandler(PEXCEPTION_ROUTINE per);
#define SetExceptionHandler xxx_SetExceptionHandler
DWORD xxx_RegisterTrackedItem(LPWSTR szName);
#define RegisterTrackedItem xxx_RegisterTrackedItem
VOID xxx_FilterTrackedItem(DWORD dwFlags, DWORD dwType, DWORD dwProcID);
#define FilterTrackedItem xxx_FilterTrackedItem
void xxx_SetKernelAlarm(HANDLE hEvent, LPSYSTEMTIME lpst);
#define SetKernelAlarm xxx_SetKernelAlarm
void xxx_RefreshKernelAlarm(void);
#define RefreshKernelAlarm xxx_RefreshKernelAlarm
void xxx_CloseProcOE(DWORD bCloseProc);
#define CloseProcOE xxx_CloseProcOE
VOID xxx_SetGwesOOMEvent(HANDLE hEvt, DWORD cpLow, DWORD cpCritical, DWORD cpLowBlockSize, DWORD cpCriticalBlockSize);
#define SetGwesOOMEvent xxx_SetGwesOOMEvent
DWORD xxx_StringCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
#define StringCompress xxx_StringCompress
DWORD xxx_StringDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
#define StringDecompress xxx_StringDecompress
DWORD xxx_BinaryCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
#define BinaryCompress xxx_BinaryCompress
DWORD xxx_BinaryDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout, DWORD skip);
#define BinaryDecompress xxx_BinaryDecompress
int xxx_InputDebugCharW(VOID);
#define InputDebugCharW xxx_InputDebugCharW
void xxx_TakeCritSec(LPCRITICAL_SECTION lpcs);
#define TakeCritSec xxx_TakeCritSec
void xxx_LeaveCritSec(LPCRITICAL_SECTION lpcs);
#define LeaveCritSec xxx_LeaveCritSec
LPVOID xxx_MapPtrToProcess(LPVOID lpv, HANDLE hProc);
#define MapPtrToProcess xxx_MapPtrToProcess
LPVOID xxx_MapPtrUnsecure(LPVOID lpv, HANDLE hProc);
#define MapPtrUnsecure xxx_MapPtrUnsecure
HANDLE xxx_GetProcFromPtr(LPVOID lpv);
#define GetProcFromPtr xxx_GetProcFromPtr
BOOL xxx_IsBadPtr(DWORD flags, LPBYTE ptr, UINT length);
#define IsBadPtr xxx_IsBadPtr
DWORD xxx_GetProcAddrBits(HANDLE hProc);
#define GetProcAddrBits xxx_GetProcAddrBits
DWORD xxx_GetFSHeapInfo(void);
#define GetFSHeapInfo xxx_GetFSHeapInfo
BOOL xxx_OtherThreadsRunning(void);
#define OtherThreadsRunning xxx_OtherThreadsRunning
VOID xxx_KillAllOtherThreads(void);
#define KillAllOtherThreads xxx_KillAllOtherThreads
HANDLE xxx_GetOwnerProcess(void);
#define GetOwnerProcess xxx_GetOwnerProcess
HANDLE xxx_GetCallerProcess(void);
#define GetCallerProcess xxx_GetCallerProcess
DWORD xxx_GetIdleTime(void);
#define GetIdleTime xxx_GetIdleTime
DWORD xxx_SetLowestScheduledPriority(DWORD maxprio);
#define SetLowestScheduledPriority xxx_SetLowestScheduledPriority
BOOL xxx_IsPrimaryThread(void);
#define IsPrimaryThread xxx_IsPrimaryThread
DWORD xxx_SetProcPermissions(DWORD perms);
#define SetProcPermissions xxx_SetProcPermissions
DWORD xxx_GetCurrentPermissions(void);
#define GetCurrentPermissions xxx_GetCurrentPermissions
DWORD xxx_CeGetCurrentTrust(void);
#define CeGetCurrentTrust xxx_CeGetCurrentTrust
DWORD xxx_CeGetCallerTrust(void);
#define CeGetCallerTrust xxx_CeGetCallerTrust
void xxx_SetTimeZoneBias(DWORD dwBias, DWORD dwDaylightBias);
#define SetTimeZoneBias xxx_SetTimeZoneBias
void xxx_SetCleanRebootFlag(void);
#define SetCleanRebootFlag xxx_SetCleanRebootFlag
HANDLE xxx_CreateCrit(LPCRITICAL_SECTION lpcs);
#define CreateCrit xxx_CreateCrit
void xxx_PowerOffSystem(void);
#define PowerOffSystem xxx_PowerOffSystem
BOOL xxx_SetDbgZone(DWORD pid, LPVOID lpvMod, LPVOID baseptr, DWORD zone, LPDBGPARAM lpdbgTgt);
#define SetDbgZone xxx_SetDbgZone
VOID xxx_TurnOnProfiling(HANDLE hThread);
#define TurnOnProfiling xxx_TurnOnProfiling
VOID xxx_TurnOffProfiling(HANDLE hThread);
#define TurnOffProfiling xxx_TurnOffProfiling
void xxx_NKTerminateThread(DWORD dwExitCode);
#define NKTerminateThread xxx_NKTerminateThread
LPCWSTR xxx_GetProcName(void);
#define GetProcName xxx_GetProcName
BOOL xxx_SetHandleOwner(HANDLE h, HANDLE hProc);
#define SetHandleOwner xxx_SetHandleOwner
HINSTANCE xxx_LoadDriver(LPCTSTR lpszFile);
#define LoadDriver xxx_LoadDriver
BOOL xxx_InterruptInitialize(DWORD idInt, HANDLE hEvent, LPVOID pvData, DWORD cbData);
#define InterruptInitialize xxx_InterruptInitialize
void xxx_InterruptDone(DWORD idInt);
#define InterruptDone xxx_InterruptDone
void xxx_InterruptDisable(DWORD idInt);
#define InterruptDisable xxx_InterruptDisable
DWORD xxx_SetKMode(DWORD mode);
#define SetKMode xxx_SetKMode
BOOL xxx_SetPowerOffHandler(FARPROC pfn);
#define SetPowerOffHandler xxx_SetPowerOffHandler
BOOL xxx_SetGwesPowerHandler(FARPROC pfn);
#define SetGwesPowerHandler xxx_SetGwesPowerHandler
BOOL xxx_SetWDevicePowerHandler(FARPROC pfn);
#define SetWDevicePowerHandler xxx_SetWDevicePowerHandler
BOOL xxx_ConnectDebugger(LPVOID pInit);
#define ConnectDebugger xxx_ConnectDebugger
BOOL xxx_SetHardwareWatch(LPVOID vAddr, DWORD flags);
#define SetHardwareWatch xxx_SetHardwareWatch
void xxx_PPSHRestart(void);
#define PPSHRestart xxx_PPSHRestart
void xxx_SignalStarted(DWORD dw);
#define SignalStarted xxx_SignalStarted
void xxx_DebugNotify(DWORD dwFlags, DWORD data);
#define DebugNotify xxx_DebugNotify
DWORD xxx_GetCallerProcessIndex(void);
#define GetCallerProcessIndex xxx_GetCallerProcessIndex
BOOL xxx_WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);
#define WaitForDebugEvent xxx_WaitForDebugEvent
BOOL xxx_ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus);
#define ContinueDebugEvent xxx_ContinueDebugEvent
HANDLE xxx_OpenProcess(DWORD fdwAccess, BOOL fInherit, DWORD IDProcess);
#define OpenProcess xxx_OpenProcess
void xxx_DumpKCallProfile(DWORD bReset);
#define DumpKCallProfile xxx_DumpKCallProfile
THSNAP * xxx_THCreateSnapshot(DWORD dwFlags, DWORD dwProcID);
#define THCreateSnapshot xxx_THCreateSnapshot
LPBYTE xxx_THGrow(THSNAP *pSnap, DWORD dwSize);
#define THGrow xxx_THGrow
void xxx_NotifyForceCleanboot(void);
#define NotifyForceCleanboot xxx_NotifyForceCleanboot
BOOL WINAPI xxx_CreateProcessW(LPCWSTR lpszImageName, LPCWSTR lpszCommandLine, LPSECURITY_ATTRIBUTES lpsaProcess, LPSECURITY_ATTRIBUTES lpsaThread, BOOL fInheritHandles, DWORD fdwCreate, LPVOID lpvEnvironment, LPWSTR lpszCurDir, LPSTARTUPINFO lpsiStartInfo, LPPROCESS_INFORMATION lppiProcInfo);
#define CreateProcessW xxx_CreateProcessW
HANDLE WINAPI xxx_CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD cbStack, LPTHREAD_START_ROUTINE lpStartAddr, LPVOID lpvThreadParm, DWORD fdwCreate, LPDWORD lpIDThread);
#define CreateThread xxx_CreateThread
BOOL xxx_SetStdioPathW(DWORD id, LPCWSTR pwszPath);
#define SetStdioPathW xxx_SetStdioPathW
BOOL xxx_GetStdioPathW(DWORD id, PWSTR pwszBuf, LPDWORD lpdwLen);
#define GetStdioPathW xxx_GetStdioPathW
DWORD xxx_ReadRegistryFromOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len);
#define ReadRegistryFromOEM xxx_ReadRegistryFromOEM
BOOL xxx_WriteRegistryToOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len);
#define WriteRegistryToOEM xxx_WriteRegistryToOEM
__int64 xxx_CeGetRandomSeed(void);
#define CeGetRandomSeed xxx_CeGetRandomSeed
void xxx_UpdateNLSInfo(DWORD ocp, DWORD acp, DWORD locale);
#define UpdateNLSInfo xxx_UpdateNLSInfo
DWORD xxx_GetProcessIndexFromID(HANDLE hProcess);
#define GetProcessIndexFromID xxx_GetProcessIndexFromID
#endif

// THREAD calls
#ifdef BUILDING_DEBUGGER
#define SuspendThread _THREAD_CALL(DWORD, SUSPEND, (HANDLE hThread))
#else
#define KernelSuspendThread _THREAD_CALL(DWORD, SUSPEND, (HANDLE hThread))
#ifdef COREDLL
DWORD WINAPI xxx_SuspendThread(HANDLE hThread);
#define SuspendThread xxx_SuspendThread
#endif
#endif

#define ResumeThread _THREAD_CALL(DWORD, RESUME, (HANDLE hThread))
#define GetThreadPriority _THREAD_CALL(int, GETTHREADPRIO, (HANDLE hThread))
#define SetThreadPriority _THREAD_CALL(BOOL, SETTHREADPRIO, (HANDLE hThread, int nPriority))
#define GetExitCodeThread _THREAD_CALL(BOOL, GETRETCODE, (HANDLE hThread, LPDWORD lpretval))
#define GetThreadContext _THREAD_CALL(BOOL, GETCONTEXT, (HANDLE hThread, LPCONTEXT lpContext))
#define SetThreadContext _THREAD_CALL(BOOL, SETCONTEXT, (HANDLE hThread, CONST CONTEXT *lpContext))
#define TerminateThread _THREAD_CALL(BOOL, TERMINATE, (HANDLE hThread, DWORD dwExitCode))

#define CeGetThreadPriority _THREAD_CALL(int, CEGETPRIO, (HANDLE hThread))
#define CeSetThreadPriority _THREAD_CALL(BOOL, CESETPRIO, (HANDLE hThread, int nPriority))
#define CeGetThreadQuantum _THREAD_CALL(DWORD, CEGETQUANT, (HANDLE hThread))
#define CeSetThreadQuantum _THREAD_CALL(BOOL, CESETQUANT, (HANDLE hThread, DWORD dwTime))

// PROCESS calls
#define TerminateProcess _PROCESS_CALL(BOOL, TERMINATE, (HANDLE hProc, DWORD dwExitCode))
#define GetExitCodeProcess _PROCESS_CALL(BOOL, GETRETCODE, (HANDLE hProcess, LPDWORD lpretval))
#define FlushInstructionCache _PROCESS_CALL(BOOL, FLUSHICACHE, (HANDLE hProc, LPCVOID lpBaseAddress, DWORD dwSize))
#define ReadProcessMemory _PROCESS_CALL(BOOL, READMEMORY, (HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead))
#define WriteProcessMemory _PROCESS_CALL(BOOL, WRITEMEMORY, (HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesWritten))
#define DebugActiveProcess _PROCESS_CALL(BOOL, DEBUGACTIVE, (DWORD dwProcessId))

// EVENT calls
#ifdef COREDLL
#ifdef WIN32_CALL
#define EventModify _EVENT_CALL(BOOL, MODIFY, (HANDLE hEvent, DWORD func))
#else
BOOL xxx_EventModify(HANDLE hEvent, DWORD func);
#define EventModify xxx_EventModify
#endif
#endif
#define AddEventAccess _EVENT_CALL(BOOL, ADDACCESS, (HANDLE hEvent))

// APISET calls
#define RegisterAPISet _APISET_CALL(BOOL, REGISTER, (HANDLE hASet, DWORD dwSetID))
#define CreateAPIHandle _APISET_CALL(HANDLE, CREATEHANDLE, (HANDLE hASet, LPVOID pvData))
#define VerifyAPIHandle _APISET_CALL(LPVOID, VERIFY, (HANDLE hASet, HANDLE h))

// MAPPED FILE calls
#ifdef COREDLL
#ifdef WIN32_CALL
#define MapViewOfFile _FSMAP_CALL(LPVOID, MAPVIEWOFFILE, (HANDLE hMap, DWORD fdwAccess, DWORD dwOffsetHigh, \
	DWORD dwOffsetLow, DWORD cbMap))
#else
LPVOID xxx_MapViewOfFile(HANDLE hMap, DWORD fdwAccess, DWORD dwOffsetHigh, DWORD dwOffsetLow, DWORD cbMap);
#define MapViewOfFile xxx_MapViewOfFile
#endif
#endif

// MUTEX calls
#ifdef COREDLL
#ifdef WIN32_CALL
#define ReleaseMutex _MUTEX_CALL(BOOL, RELEASEMUTEX, (HANDLE hMutex))
#else
BOOL xxx_ReleaseMutex(HANDLE hMutex);
#define ReleaseMutex xxx_ReleaseMutex
#endif
#endif


// SEMPAHORE calls

#define ReleaseSemaphore _SEMAPHORE_CALL(BOOL, RELEASESEMAPHORE, (HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount))

#endif

#ifdef __cplusplus
}
#endif

#endif

