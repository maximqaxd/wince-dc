/**     TITLE("Kernel Win32 function declarations")
 *++
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *    KWin32.h
 *
 * Abstract:
 *
 *  This file contains the function prototypes and defines used by the kernel part
 * of the Win32 API implemented via the Win32 system API handle.
 *
 *--
 */

typedef struct VERBLOCK {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;
    WCHAR szKey[1];
} VERBLOCK ;

HGDIOBJ SC_SelectObject(HANDLE hDC, HANDLE hObj);
BOOL SC_PatBlt(HDC hdcDest,int nXLeft,int nYLeft,int nWidth,int nHeight,DWORD dwRop);
BOOL SC_GetTextExtentExPointW(HDC hdc,LPCWSTR lpszStr,int cchString,int nMaxExtent,LPINT lpnFit,LPINT alpDx,LPSIZE lpSize);
HBRUSH SC_GetSysColorBrush(int nIndex);
COLORREF SC_SetBkColor(HDC hDC, COLORREF dwColor);
HWND SC_GetParent(HWND hwnd);
BOOL SC_InvalidateRect(HWND hwnd, LPCRECT prc, BOOL fErase);
LONG SC_RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG SC_RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
HANDLE SC_CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL SC_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
CEOID SC_CeWriteRecordProps(HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL *rgPropVal);
BOOL SC_ReadFileWithSeek(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);
BOOL SC_WriteFileWithSeek(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);
HANDLE SC_OpenDatabaseEx(PCEGUID pguid, PCEOID poid, LPWSTR lpszName, CEPROPID propid, DWORD dwFlags, CENOTIFYREQUEST *pReq);
CEOID SC_SeekDatabase(HANDLE hDatabase, DWORD dwSeekType, DWORD dwValue, LPDWORD lpdwIndex);
CEOID SC_ReadRecordPropsEx (HANDLE hDbase, DWORD dwFlags, LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, LPDWORD lpcbBuffer, HANDLE hHeap);
LONG SC_RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
BOOL SC_DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
BOOL SC_CloseHandle(HANDLE hObj);
LONG SC_RegCloseKey(HKEY hKey);
BOOL SC_ClientToScreen(HWND hwnd, LPPOINT lpPoint);
LRESULT SC_DefWindowProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL SC_GetClipCursor(LPRECT lpRect);
HDC SC_GetDC (HWND hwnd);
HWND SC_GetFocus();
BOOL SC_GetMessageW(PMSG pMsgr, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
HWND SC_GetWindow(HWND hwnd, UINT uCmd);
BOOL SC_PeekMessageW(PMSG pMsg, HWND hWnd, UINT wMsgFilterMin,UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL SC_ReleaseDC(HWND hwnd, HDC hdc);
LRESULT SC_SendMessageW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int SC_SetScrollInfo(HWND hwnd, int fnBar, LPCSCROLLINFO lpsi, BOOL fRedraw);
LONG SC_SetWindowLongW(HWND hwnd, int nIndex, LONG lNewLong);
BOOL SC_SetWindowPos(HWND hwnd, HWND hwndInsertAfter, int x, int y, int dx,int dy, UINT fuFlags);
HBRUSH SC_CreateSolidBrush(COLORREF crColor);
BOOL SC_DeleteMenu(HMENU hmenu, UINT uPosition, UINT uFlags);
BOOL SC_DeleteObject(HGDIOBJ hObject);
int SC_DrawTextW(HDC hdc,LPCWSTR lpszStr,int cchStr,RECT *lprc,UINT wFormat);
BOOL SC_ExtTextOutW(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT *lprc, LPCWSTR lpszString, UINT cbCount, CONST INT *lpDx);
int SC_FillRect(HDC hdc,CONST RECT *lprc,HBRUSH hbr);
SHORT SC_GetAsyncKeyState(INT vKey);
int SC_GetDlgCtrlID(HWND hWnd);
HGDIOBJ SC_GetStockObject(int fnObject);
int SC_GetSystemMetrics(int nIndex);
ATOM SC_RegisterClassWStub(CONST WNDCLASSW *lpWndClass, LPCWSTR lpszClassName, HANDLE hprcWndProc);
UINT SC_RegisterClipboardFormatW(LPCWSTR lpszFormat);
int SC_SetBkMode(HDC hdc,int iBkMode);
COLORREF SC_SetTextColor(HDC hdc,COLORREF crColor);
BOOL SC_TransparentImage(HDC hdcDest, int nXDest, int nYDest, int nWidthDest, int nHeightDest,
		HANDLE hImgSrc, int nXSrc, int nYSrc, int nWidthSrc, int nHeightSrc, COLORREF crTransparentColor);
BOOL SC_IsDialogMessageW(HWND hDlg, LPMSG lpMsg);
BOOL SC_PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL SC_IsWindowVisible(HWND hWnd);
SHORT SC_GetKeyState(int nVirtKey);
HDC SC_BeginPaint(HWND hwnd, LPPAINTSTRUCT pps);
BOOL SC_EndPaint(HWND hwnd, LPPAINTSTRUCT pps);
DWORD SC_PerformCallBack4(CALLBACKINFO *pcbi, ...);
VOID SC_RegisterGwesHandler(LPVOID);
DWORD ModuleJit( LPCWSTR, LPWSTR, HANDLE*);
LPVOID SC_GetProfileBaseAddress(void);
VOID SC_SetProfilePortAddress(LPVOID lpv);

LPVOID SC_VirtualAlloc(
        LPVOID lpvAddress, DWORD cbSize, DWORD fdwAllocationType,
        DWORD fdwProtect);
BOOL SC_VirtualFree(LPVOID lpvAddress, DWORD cbSize, DWORD fdwFreeType);
BOOL SC_VirtualProtect(LPVOID lpvAddress, DWORD cbSize,
        DWORD fdwNewProtect, PDWORD pfdwOldProtect);
DWORD SC_VirtualQuery(LPVOID lpvAddress,
        PMEMORY_BASIC_INFORMATION pmbiBuffer, DWORD cbLength);
BOOL DoVirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);
BOOL SC_VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);
BOOL DoLockPages(LPCVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions);
BOOL SC_LockPages(LPVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions);
BOOL DoUnlockPages(LPCVOID lpvAddress, DWORD cbSize);
BOOL SC_UnlockPages(LPVOID lpvAddress, DWORD cbSize);
BOOL SC_VirtualSetPageFlags(LPVOID lpvAddress, DWORD cbSize, DWORD dwFlags, LPDWORD lpdwOldFlags);

HANDLE SC_CreateAPISet(char acName[4], USHORT cFunctions,
        const PFNVOID *ppfnMethods, const DWORD *pdwSig);
HANDLE SC_CreateAPIHandle(HANDLE hSet, LPVOID pvData);

HMODULE SC_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HANDLE SC_LoadLibraryW(LPCTSTR lpszFileName);
BOOL SC_FreeLibrary(HANDLE hInst);
VOID SC_ThreadAttachAllDLLs(void);
VOID SC_ThreadDetachAllDLLs(void);
VOID SC_ProcessDetachAllDLLs(void);
DWORD SC_GetTickCount(void);
DWORD SC_TlsCall(DWORD,DWORD);
VOID SC_GetSystemInfo(LPSYSTEM_INFO);
HANDLE SC_LoadDriver(LPCTSTR lpszFileName);
DWORD SC_GetProcessVersion(DWORD dwProcessId);
DWORD SC_GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
HMODULE SC_GetModuleHandleW(LPCWSTR lpModuleName);
BOOL SC_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
BOOL SC_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
VOID SC_WriteDebugLED(WORD wIndex, DWORD dwPattern);
void SC_ForcePageout(void);
BOOL SC_GetThreadTimes(HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);

HANDLE SC_CreateCallBack(PFNVOID pfn, LPVOID pvData);

BOOL SC_RegisterDbgZones(HANDLE hMod, LPDBGPARAM lpdbgparam);
void SC_ProfileSyscall(LPXT lpXt);

HANDLE SC_FindResource(HANDLE hModule, LPCWSTR lpszName, LPCWSTR lpszType);
HANDLE SC_LoadResource(HANDLE hModule, HANDLE hRsrc);
DWORD SC_SizeofResource(HANDLE hModule, HANDLE hRsrc);
LPVOID SC_ExtractResource(LPCWSTR lpszFile, LPCWSTR lpszName, LPCWSTR lpszType);
UINT SC_KernExtractIcons(LPCWSTR lpszFile, int nIconIndex, LPBYTE *pIconLarge, LPBYTE *pIconSmall, CALLBACKINFO *pcbi);
BOOL SC_GetRomFileInfo(DWORD type, LPWIN32_FIND_DATA lpfd, DWORD count);
DWORD SC_GetRomFileBytes(DWORD type, DWORD count, DWORD pos, LPVOID buffer, DWORD nBytesToRead);
BOOL SC_PrintTrackedItem(DWORD dwFlags, DWORD dwType, DWORD dwProcID, HANDLE handle);
BOOL SC_DeleteTrackedItem(DWORD dwType, HANDLE handle);
DWORD SC_RegisterTrackedItem(LPWSTR szName);
BOOL SC_AddTrackedItem(DWORD dwType, HANDLE handle, TRACKER_CALLBACK cb, DWORD dwProcID, 
    DWORD dwSize, DWORD dw1, DWORD dw2);
VOID SC_FilterTrackedItem(DWORD dwFlags, DWORD dwType, DWORD dwProcID);
HANDLE SC_CreateEvent(LPSECURITY_ATTRIBUTES lpsa, BOOL fManReset, BOOL fInitState, LPCWSTR lpEventName);
BOOL SC_CreateProc(LPCWSTR lpszImageName, LPCWSTR lpszCommandLine, LPSECURITY_ATTRIBUTES lpsaProcess,
	LPSECURITY_ATTRIBUTES lpsaThread, BOOL fInheritHandles, DWORD fdwCreate, LPVOID lpvEnvironment,
	LPWSTR lpszCurDir, LPSTARTUPINFO lpsiStartInfo, LPPROCESS_INFORMATION lppiProcInfo);
HANDLE SC_CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD cbStack, LPTHREAD_START_ROUTINE lpStartAddr,
	LPVOID lpvThreadParm, DWORD fdwCreate, LPDWORD lpIDThread);
HANDLE SC_CreateMutex(LPSECURITY_ATTRIBUTES lpsa, BOOL bInitialOwner, LPCTSTR lpName);
HANDLE SC_CreateSemaphore(LPSECURITY_ATTRIBUTES lpsa, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
int SC_CeGetThreadPriority(HANDLE hThread);
BOOL SC_CeSetThreadPriority(HANDLE hThread, DWORD nPriority);
DWORD SC_CeGetThreadQuantum(HANDLE hThread);
BOOL SC_CeSetThreadQuantum(HANDLE hThread, DWORD dwTime);
BOOL SC_CeMapArgumentArray(HANDLE hProc, LPVOID *pArgList, DWORD dwSig);
BOOL SC_CeSetExtendedPdata(LPVOID pData);
BOOL SC_VerQueryValueW(VERBLOCK *pBlock, LPWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);
DWORD SC_GetFileVersionInfoSizeW(LPWSTR lpFilename, LPDWORD lpdwHandle);
BOOL SC_GetFileVersionInfoW(LPWSTR lpFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
LPBYTE SC_CreateLocaleView(BOOL bFirst);
	
LPVOID SC_MapUncompressedFileW(LPCWSTR pFileName, LPDWORD pLen);
void SC_PPSHRestart(void);
void SC_SignalStarted(DWORD);
void SC_UpdateNLSInfo(DWORD ocp, DWORD acp, DWORD locale);
void SC_DebugNotify(DWORD dwFlags, DWORD data);
__int64 SC_CeGetRandomSeed();

BOOL SC_GetKPhys(void *ptr, ULONG length);
BOOL SC_GiveKPhys(void *ptr, ULONG length);
void SC_SetExceptionHandler(PEXCEPTION_ROUTINE per);
void SC_SetKernelAlarm(HANDLE hEvent, LPSYSTEMTIME lpst);
void SC_RefreshKernelAlarm(void);
void SC_CloseProcOE(DWORD);
void SC_SetGwesOOMEvent(HANDLE hEvent, DWORD cpLow, DWORD cpCritical,
						DWORD cpLowBlockSize, DWORD cpCriticalBlockSize);
void SC_CacheSync(int flags);
DWORD SC_FSStringCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
DWORD SC_FSStringDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
DWORD SC_FSBinaryCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout);
DWORD SC_FSBinaryDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout, DWORD skip);
void UB_TakeCritSec(LPCRITICAL_SECTION lpcs);
void SC_TakeCritSec(LPCRITICAL_SECTION lpcs);
void SC_LeaveCritSec(LPCRITICAL_SECTION lpcs);
DWORD UB_WaitForMultiple(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout);
DWORD SC_WaitForMultiple(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout);
LPVOID SC_MapPtrToProcess(LPVOID lpv, HANDLE hProc);
LPVOID SC_MapPtrUnsecure(LPVOID lpv, HANDLE hProc);
HANDLE SC_GetProcFromPtr(LPVOID lpv);
BOOL SC_IsBadPtr(DWORD flags, LPBYTE ptr, DWORD length);
DWORD SC_GetProcAddrBits(HANDLE hProc);
DWORD SC_GetFSHeapInfo(void);
BOOL SC_OtherThreadsRunning(void);
void SC_KillAllOtherThreads(void);
void SC_KillThreadIfNeeded(void);
HANDLE SC_GetOwnerProcess(void);
HANDLE SC_GetCallerProcess(void);
DWORD SC_CeGetCurrentTrust(void);
DWORD SC_CeGetCallerTrust(void);
DWORD SC_GetIdleTime(void);
DWORD SC_SetLowestScheduledPriority(DWORD);
BOOL SC_IsPrimaryThread(void);
DWORD SC_SetProcPermissions(DWORD);
DWORD SC_GetCurrentPermissions(void);
void SC_SetTimeZoneBias(DWORD dwBias, DWORD dwDaylightBias);
void SC_SetDaylightTime(DWORD dst);
void SC_SetCleanRebootFlag(void);
HANDLE SC_CreateCrit(LPCRITICAL_SECTION lpcs);
void SC_PowerOffSystem(void);
BOOL SC_SetDbgZone(DWORD pid, LPVOID lpvMod, LPVOID baseptr, DWORD zone, LPDBGPARAM lpdbgTgt);
void SC_Sleep(DWORD dwMilliseconds);
void UB_Sleep(DWORD dwMilliseconds);
void SC_TurnOnProfiling(void);
void SC_TurnOffProfiling(void);
void SC_SetLastError(DWORD dwErr);
DWORD SC_GetLastError(void);
LPCWSTR SC_GetProcName(void);
void SC_TerminateSelf(void);
void SC_CloseAllHandles(void);
BOOL SC_SetHandleOwner(HANDLE h, HANDLE hProc);
HANDLE SC_CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpsa, DWORD flProtect, DWORD dwMaxSizeHigh,
	DWORD dwMaxSizeLow, LPCTSTR lpName);
LPVOID SC_MapViewOfFile(HANDLE hMap, DWORD fdwAccess, DWORD dwOffsetLow, DWORD dwOffsetHigh, DWORD cbMap);
BOOL SC_UnmapViewOfFile(LPVOID lpvAddr);
BOOL SC_FlushViewOfFile(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush);
BOOL SC_FlushViewOfFileMaybe(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush);
HANDLE SC_CreateFileForMapping(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile);
BOOL OEMIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize,
	LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);
void SC_NotifyForceCleanboot(void);
DWORD SC_ReadRegistryFromOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len);
BOOL SC_WriteRegistryToOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len);

BOOL SC_ThreadCloseHandle(HANDLE hTh);
DWORD SC_ThreadSuspend(HANDLE hTh);
DWORD UB_ThreadSuspend(HANDLE hTh);
DWORD SC_ThreadResume(HANDLE hTh);
BOOL SC_ThreadSetPrio(HANDLE hTh, DWORD prio);
int SC_ThreadGetPrio(HANDLE hTh);
BOOL SC_ThreadGetCode(HANDLE hTh, LPDWORD dwExit);
BOOL SC_ThreadGetContext(HANDLE hTh, LPCONTEXT lpContext);
BOOL SC_ThreadSetContext(HANDLE hTh, const CONTEXT *lpContext);
BOOL SC_ThreadTerminate(HANDLE hTh, DWORD dwExitCode);

BOOL SC_ProcCloseHandle(HANDLE hProc);
BOOL SC_ProcTerminate(HANDLE hProc, DWORD dwExitCode);
LPCHAR SC_ProcGetName(HANDLE hProc);
BOOL SC_ProcGetCode(HANDLE hProc, LPDWORD dwExit);
DWORD SC_ProcGetIndex(HANDLE hProc);
BOOL SC_ProcFlushICache(HANDLE hProc, LPCVOID lpBaseAddress, DWORD dwSize);
BOOL SC_ProcReadMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead);
BOOL SC_ProcWriteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesWritten);

BOOL SC_WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);
BOOL SC_ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus);
BOOL SC_ProcDebug(DWORD dwProcessId);

BOOL SC_EventCloseHandle(HANDLE hEvent);
BOOL SC_EventModify(HANDLE hEvent, DWORD action);
BOOL SC_EventAddAccess(HANDLE hEvent);

BOOL SC_MutexCloseHandle(HANDLE hMutex);
BOOL SC_ReleaseMutex(HANDLE hMutex);
BOOL SC_SemCloseHandle(HANDLE hSem);
BOOL SC_ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);

BOOL SC_ConnectDebugger(LPVOID pInit);

BOOL SC_InterruptInitialize(DWORD idInt, HANDLE hEvent, LPVOID pvData, DWORD cbData);
void SC_InterruptDone(DWORD idInt);
void SC_InterruptDisable(DWORD idInt);
BOOL SC_SetKMode(BOOL bPriv);
BOOL SC_SetRAMMode(BOOL bEnable, LPVOID *lplpvAddress, LPDWORD lpLength);
LPVOID SC_SetStoreQueueBase(DWORD dwPhysPage);
BOOL SC_SetPowerOffHandler(FARPROC pfn);
BOOL SC_SetGwesPowerHandler(FARPROC pfn);
BOOL SC_SetWDevicePowerHandler(FARPROC pfn);
BOOL SC_SetHardwareWatch(LPVOID vAddr, DWORD flags);
int SC_QueryAPISetID(char *pName);
void CaptureContext(void);

DWORD SC_GetCallerIndex(void);
HANDLE SC_OpenProcess(DWORD fdwAccess, BOOL fInherit, DWORD IDProcess);
THSNAP *SC_THCreateSnapshot(DWORD dwFlags, DWORD dwProcID);
LPBYTE SC_THGrow(THSNAP *pSnap, DWORD dwSize);
void SC_DumpKCallProfile(DWORD bReset);
BOOL SC_SetStdioPathW(DWORD id, LPCWSTR pwszPath);
BOOL SC_GetStdioPathW(DWORD id, PWSTR pwszBuf, LPDWORD lpdwLen);

