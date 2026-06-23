/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include <windows.h>

HLOCAL WINAPI RemoteLocalAlloc(UINT uFlags, UINT uBytes) {
    CALLBACKINFO CallbackInfo;
    HANDLE hCallerProcess;
    if (!(CallbackInfo.hProc = GetCallerProcess()))
    	CallbackInfo.hProc = (HANDLE)GetCurrentProcessId();
    hCallerProcess = CallbackInfo.hProc;
    CallbackInfo.pfn = (FARPROC)LocalAlloc;
    CallbackInfo.pvArg0 = (PVOID)uFlags;
    return (HLOCAL)MapPtrToProcess((LPVOID)PerformCallBack4(&CallbackInfo, uBytes), hCallerProcess);
}

HLOCAL WINAPI RemoteLocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags) {
    CALLBACKINFO CallbackInfo;
    HANDLE hCallerProcess;
    if (!(CallbackInfo.hProc = GetCallerProcess()))
    	CallbackInfo.hProc = (HANDLE)GetCurrentProcessId();
    hCallerProcess = CallbackInfo.hProc;
    CallbackInfo.pfn = (FARPROC)LocalReAlloc;
    CallbackInfo.pvArg0 = (PVOID)UnMapPtr(hMem);
    return (HLOCAL)MapPtrToProcess((LPVOID)PerformCallBack4(&CallbackInfo, uBytes, uFlags),
		hCallerProcess);
}

UINT WINAPI RemoteLocalSize(HLOCAL hMem) {
    CALLBACKINFO CallbackInfo;
    if (!(CallbackInfo.hProc = GetCallerProcess()))
    	CallbackInfo.hProc = (HANDLE)GetCurrentProcessId();
    CallbackInfo.pfn = (FARPROC)LocalSize;
    CallbackInfo.pvArg0 = (PVOID)UnMapPtr(hMem);
    return (UINT)PerformCallBack4(&CallbackInfo);
}

HLOCAL WINAPI RemoteLocalFree(HLOCAL hMem) {
    CALLBACKINFO CallbackInfo;
    if (!(CallbackInfo.hProc = GetCallerProcess()))
    	CallbackInfo.hProc = (HANDLE)GetCurrentProcessId();
    CallbackInfo.pfn = (FARPROC)LocalFree;
    CallbackInfo.pvArg0 = (PVOID)UnMapPtr(hMem);
    return (HLOCAL)PerformCallBack4(&CallbackInfo);
}


HLOCAL WINAPI LocalAllocInProcess(
	UINT		uFlags,
	UINT		uBytes,
	HPROCESS	hProc
	)
{
    CALLBACKINFO CallbackInfo;
    CallbackInfo.hProc = hProc;
    CallbackInfo.pfn = (FARPROC)LocalAlloc;
    CallbackInfo.pvArg0 = (PVOID)uFlags;
    return (HLOCAL)MapPtrUnsecure((LPVOID)PerformCallBack4(&CallbackInfo, uBytes),
		CallbackInfo.hProc);
}


HLOCAL WINAPI LocalFreeInProcess(
	HLOCAL		hMem,
	HPROCESS	hProc
	)
{
    CALLBACKINFO CallbackInfo;
    CallbackInfo.hProc = hProc;
    CallbackInfo.pfn = (FARPROC)LocalFree;
    CallbackInfo.pvArg0 = (PVOID)UnMapPtr(hMem);
    return (HLOCAL)PerformCallBack4(&CallbackInfo);
}


UINT WINAPI LocalSizeInProcess(
	HLOCAL 		hMem,
	HPROCESS	hProc
	)
{
    CALLBACKINFO CallbackInfo;
    CallbackInfo.hProc = hProc;
    CallbackInfo.pfn = (FARPROC)LocalSize;
    CallbackInfo.pvArg0 = (PVOID)UnMapPtr(hMem);
    return (UINT)PerformCallBack4(&CallbackInfo);
}

