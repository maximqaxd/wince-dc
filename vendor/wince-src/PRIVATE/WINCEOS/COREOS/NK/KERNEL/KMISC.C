/*		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

#include <kernel.h>

extern CRITICAL_SECTION rtccs, LLcs, NameCS;
extern LPVOID pGwesHandler;
DWORD curridlelow, curridlehigh, idleconv;
Name *pDebugger, *pPath;
LPVOID pGwesHandler = SC_Nop;

BOOL bAllKMode;
int  InitializeJit(PFNOPEN, PFNCLOSE);

PFN_OEMKDIoControl pKDIoControl = NULL;

// DVCM Collector functions, assigned if DVCM is included in the image
HRESULT (*pInitialize_DataCollector)();
HRESULT (*pIoctl_Register_DataCollector)(LPVOID,DWORD,LPVOID,DWORD);
HRESULT (*pIoctl_Send_DataCollector)(LPVOID,DWORD,LPVOID,DWORD);
HRESULT (*pIoctl_Unregister_DataCollector)(LPVOID,DWORD,LPVOID,DWORD);


BOOL SC_CeSetExtendedPdata(LPVOID pData) {
#ifdef x86
	KSetLastError(pCurThread,ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
#else
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return FALSE;
	}
	pCurProc->pExtPdata = pData;
	return TRUE;
#endif	
}

VOID SC_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetSystemInfo entry: %8.8lx\r\n",lpSystemInfo));
    lpSystemInfo->wProcessorArchitecture = PROCESSOR_ARCHITECTURE;
	lpSystemInfo->wReserved = 0;
    lpSystemInfo->dwPageSize = PAGE_SIZE;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)0x10000;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)0x7fffffff;
    lpSystemInfo->dwActiveProcessorMask = 1;
    lpSystemInfo->dwNumberOfProcessors = 1;
    lpSystemInfo->dwProcessorType = CEProcessorType;
	lpSystemInfo->wProcessorLevel = ProcessorLevel;
    lpSystemInfo->wProcessorRevision = ProcessorRevision;
    lpSystemInfo->dwAllocationGranularity = 0x10000;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetSystemInfo exit\r\n"));
}

BOOL SC_IsBadPtr(DWORD flag, LPBYTE lpv, DWORD len) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_IsBadPtr entry: %8.8lx %8.8lx %8.8lx\r\n",flag,lpv,len));
	if (!len) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_IsBadPtr exit: FALSE (ptr OK - zero length)\r\n"));
		return FALSE;
	}
	if ((DWORD)lpv + len < (DWORD)lpv) {
		KSetLastError(pCurThread,ERROR_INVALID_ADDRESS);
		DEBUGMSG(ZONE_ENTRY,(L"SC_IsBadPtr exit: TRUE (ptr bad)\r\n"));
		return TRUE;
	}
	if (((ulong)lpv>>VA_SECTION) > SECTION_MASK) {
		if (bAllKMode || ((DWORD)pCurThread->pcstkTop->pprcLast == KERNEL_MODE))
			return FALSE;
		return TRUE;
	}
	if (!LockPages(lpv, len, 0, flag==VERIFY_WRITE_FLAG ?
	        (LOCKFLAG_QUERY_ONLY | LOCKFLAG_WRITE) : (LOCKFLAG_QUERY_ONLY | LOCKFLAG_READ))) {
		KSetLastError(pCurThread,ERROR_INVALID_ADDRESS);
		DEBUGMSG(ZONE_ENTRY,(L"SC_IsBadPtr exit: TRUE (ptr bad)\r\n"));
		return TRUE;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_IsBadPtr exit: FALSE (ptr OK)\r\n"));
    return FALSE;
}

// @func LPVOID | MapPtrToProcess | Maps an unmapped pointer to a mapped pointer in a process
// @parm LPVOID | lpv | pointer to map
// @parm HANDLE | hProc | process to map into
// @rdesc  Returns a mapped version of the pointer, or 0 for failure
// @comm If the pointer is already mapped, the original pointer is returned if the caller
//       has access to dereference that pointer, else 0 is returned.  If the pointer is
//       unmapped, it first maps it, then returns the mapped pointer if the caller can access
//       it, else 0.  This function should be called to map pointers which are passed to a PSL where the pointer is not
//       a parameter directly, but obtained from a structure, and needs to be adjusted for the address space.
// @xref <f MapPtrUnsecure>

LPVOID SC_MapPtrUnsecure(LPVOID lpv, HANDLE hProc) {
	LPVOID retval;
	PPROCESS pProc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_MapPtrUnsecure entry: %8.8lx %8.8lx\r\n",lpv,hProc));
	if (!hProc || (hProc == GetCurrentProcess()))
		hProc = hCurProc;
	if (!(pProc = HandleToProc(hProc))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		retval = 0;
	} else
		retval = MapPtrProc(lpv,pProc);
	DEBUGMSG(ZONE_ENTRY,(L"SC_MapPtrUnsecure exit: %8.8lx\r\n",retval));
	return retval;
}

// @func HANDLE | GetProcFromPtr | Returns the process id which owns the pointer passed in
// @parm LPVOID | ptr | pointer from which to find a process
// @rdesc  Returns the process id of the owning process
// @comm Returns the owner process of the pointer, or NULL if the pointer is not valid.

HANDLE SC_GetProcFromPtr(LPVOID lpv) {
	int loop;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcFromPtr entry: %8.8lx\r\n",lpv));
	if (ZeroPtr(lpv) == (DWORD)lpv) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcFromPtr exit: %8.8lx\r\n",hCurProc));
		return hCurProc;
	}
	loop = ((DWORD)lpv>>VA_SECTION)-1;
	if ((loop >= MAX_PROCESSES) || !ProcArray[loop].dwVMBase) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcFromPtr exit: %8.8lx\r\n",0));
		return 0;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcFromPtr exit :%8.8lx\r\n",ProcArray[loop].hProc));
	return ProcArray[loop].hProc;
}

// @func LPVOID | MapPtrUnsecure | Maps an unmapped pointer to a mapped pointer in a process
// @parm LPVOID | lpv | pointer to map
// @parm HANDLE | hProc | process to map into
// @rdesc  Returns a mapped version of the pointer, or 0 for failure
// @comm If the pointer is already mapped, the original pointer is returned.  If the pointer
//       is unmapped, it first maps it, then returns the mapped pointer.  No access validation is performed.
//       This function should be called to map pointers which are passed to a PSL where the pointer is not
//       a parameter directly, but obtained from a structure, and needs to be adjusted for the address space.
// @xref <f MapPtrToProcess>

LPVOID SC_MapPtrToProcess(LPVOID lpv, HANDLE hProc) {
	PPROCESS pProc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_MapPtrToProcess entry: %8.8lx %8.8lx\r\n",lpv,hProc));
	if (!(pProc = HandleToProc(hProc))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		lpv = 0;
	} else if ((DWORD)lpv>>VA_SECTION != 0) {
		if (!IsAccessOK(lpv,CurAKey)) {
			KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
			lpv = 0;
		}
	} else if ((DWORD)lpv > 0x10000)
		lpv = MapPtrProc(lpv,pProc);
	DEBUGMSG(ZONE_ENTRY,(L"SC_MapPtrToProcess exit: %8.8lx\r\n",lpv));
	return lpv;
}

DWORD SC_GetProcAddrBits(HANDLE hproc) {
	PPROCESS pproc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddrBits entry: %8.8lx\r\n",hproc));
	if (!(pproc = HandleToProc(hproc))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddrBits exit: %8.8lx\r\n",0));
		return 0;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddrBits exit: %8.8lx\r\n",pproc->dwVMBase));
	return pproc->dwVMBase;
}

// @func DWORD | GetFSHeapInfo | Gets info on the physical space reserved for the file system
// @comm Retrieves the start of the physical memory reserved for the file system

DWORD SC_GetFSHeapInfo(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFSHeapInfo entry\r\n"));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFSHeapInfo exit: %8.8lx\r\n",PAGEALIGN_UP(pTOC->ulRAMFree)));
	return PAGEALIGN_UP(pTOC->ulRAMFree);
}

void SC_UpdateNLSInfo(DWORD ocp, DWORD acp, DWORD locale) {
	KInfoTable[KINX_NLS_OCP] = ocp;
	KInfoTable[KINX_NLS_ACP] = acp;
	KInfoTable[KINX_NLS_LOC] = locale;
}

DWORD randdw1, randdw2;

__int64 SC_CeGetRandomSeed() {
	return (((__int64)randdw1)<<32) | (__int64)randdw2;
}

DWORD SC_GetIdleTime(void) {
	DWORD result;
	__int64 temp;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetIdleTime entry\r\n"));
	temp = curridlehigh;
	if (idleconv) {
		temp = (temp * 0x100000000) + curridlelow;
		result = (DWORD)(temp/idleconv);
	} else
		result = 0xffffffff;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetIdleTime exit: %8.8lx\r\n",result));
	return result;
}

LPCWSTR SC_GetProcName(void) {
	LPWSTR retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetName entry\r\n"));
	retval = MapPtr(pCurProc->lpszProcName);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetName exit: %8.8lx\r\n",retval));
	return retval;
}

// @func HANDLE | GetOwnerProcess | Returns the process id which owns the current thread
// @rdesc Returns the process id of the process which spawned the current thread
// @comm Returns the process id of the process which spawned the current thread

HANDLE SC_GetOwnerProcess(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetOwnerProcess entry\r\n"));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetOwnerProcess exit: %8.8lx\r\n",pCurThread->pOwnerProc->hProc));
	return pCurThread->pOwnerProc->hProc;
}

LPWSTR SC_GetCommandLineW(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCommandLineW entry\r\n"));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCommandLineW exit: %8.8lx\r\n",pCurThread->pOwnerProc->pcmdline));
	return (LPWSTR)pCurThread->pOwnerProc->pcmdline;
}

// @func HANDLE | GetCallerProcess | Returns the process id which called the currently running PSL
// @rdesc Returns the process id of the process which called the currently running PSL
// @comm Returns the process id of the process which called the currently running PSL

HANDLE SC_GetCallerProcess(void) {
	HANDLE retval = 0;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCallerProcess entry\r\n"));
	if (pCurThread->pcstkTop)
		if ((DWORD)pCurThread->pcstkTop->pprcLast >= 0x10000)
			retval = pCurThread->pcstkTop->pprcLast->hProc;
		else if ((pCurThread->pcstkTop->pcstkNext) &&
				 ((DWORD)pCurThread->pcstkTop->pcstkNext->pprcLast >= 0x10000))
    		retval = pCurThread->pcstkTop->pcstkNext->pprcLast->hProc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCallerProcess exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD SC_CeGetCurrentTrust(void) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetCurrentTrust entry\r\n"));
    retval = pCurProc->bTrustLevel;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetCurrentTrust exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD SC_CeGetCallerTrust(void) {
	HANDLE hval = SC_GetCallerProcess();
	DWORD retval;
	PPROCESS pproc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetCallerTrust entry\r\n"));
	if (hval) {
		pproc = HandleToProc(hval);
		DEBUGCHK(pproc);
		retval = pproc->bTrustLevel;
	} else
		retval = pCurProc->bTrustLevel;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetCallerTrust exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD SC_GetCallerIndex(void) {
	HANDLE hval = SC_GetCallerProcess();
	DWORD retval;
	PPROCESS pproc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCallerIndex entry\r\n"));
	if (hval) {
		pproc = HandleToProc(hval);
		DEBUGCHK(pproc);
		retval = pproc->procnum;
	} else
		retval = (DWORD)-1;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCallerIndex exit: %8.8lx\r\n",retval));
	return retval;
}

#define MAX_APPSTART_INDEX 32
#define MAX_APPSTART_KEYNAME 128

typedef struct appinfo_t {
	DWORD index;
	DWORD done;
	WORD depchain[MAX_APPSTART_INDEX];
	WCHAR appname[MAX_PATH];
} appinfo_t;

HANDLE hSignalApp;
DWORD nextapp;
appinfo_t *pappinfo;
DWORD atoiW(LPWSTR str);

void SC_SignalStarted(DWORD dw) {
	DWORD loop;
	if (!pappinfo)
		return;
	if (!dw)
		SetEvent(hSignalApp);
	else
		for (loop = 0; loop < nextapp; loop++)
			if (pappinfo[loop].index == dw) {
				pappinfo[loop].done = 1;
				SetEvent(hSignalApp);
				break;
			}
}

BOOL Kbstrcmpn(LPWSTR p1, LPWSTR p2, int len) {
	while (len && *p1 && (*p1 == *p2)) {
		len--;
		p1++;
		p2++;
	}
	if (!len || (!*p1 && !*p2))
		return 0;
	return 1;
}

void kItoW(LPWSTR pStr, DWORD dw) {
	if (dw >= 10000)
		*pStr++ = (WCHAR)(((dw/10000)%10) + '0');
	if (dw >= 1000)
		*pStr++ = (WCHAR)(((dw/1000)%10) + '0');
	if (dw >= 100)
		*pStr++ = (WCHAR)(((dw/100)%10) + '0');
	if (dw >= 10)
		*pStr++ = (WCHAR)(((dw/10)%10) + '0');
	*pStr++ = (WCHAR)((dw%10) + '0');
	*pStr = 0;
}

BOOL CanBeLaunched(DWORD index) {
	LPWORD pW;
	DWORD loop;
	pW = pappinfo[index].depchain;
	while (*pW) {
		for (loop = 0; loop < nextapp; loop++)
			if (pappinfo[loop].index == *pW)
				break;
		if ((loop != nextapp) && !pappinfo[loop].done)
			return 0;
		pW++;
	}
	return 1;
}

BOOL fJitIsPresent;

void RunApps(ulong param) {
	
	HKEY key;
	DWORD index, enumindex;
	WCHAR pName[MAX_APPSTART_KEYNAME];
	BYTE pProp[MAX_PATH*sizeof(WCHAR)];
	DWORD size,size2,type, loop;
	appinfo_t AppInfo[MAX_APPSTART_INDEX];
	appinfo_t SwapInfo;
	hSignalApp = CreateEvent(0,0,0,0);
	pappinfo = &AppInfo[0];
	CreateProcess(L"filesys.exe",0,0,0,0,0x80000000,0,0,0,0);
	WaitForMultipleObjects(1,&hSignalApp,0,INFINITE);
    fJitIsPresent = InitializeJit(JitOpenFile,JitCloseFile);
	// Initialize MUI-Resource loader (requires registry)
	InitMUILanguages();
	// read registry in
	size = sizeof(pName);
	if (!RegQueryValueExW(HKEY_LOCAL_MACHINE,L"JITDebugger",(LPDWORD)L"Debug",&type,(LPBYTE)pName,&size) &&
		(type == REG_SZ) && (size < MAX_PATH*sizeof(WCHAR)) && (pDebugger = AllocName((strlenW(pName)+1)*2)))
		kstrcpyW(pDebugger->name,pName);
	size = sizeof(pProp);
	if (!RegQueryValueExW(HKEY_LOCAL_MACHINE,L"SystemPath",(LPDWORD)L"Loader",&type,(LPBYTE)pProp,&size) &&
		(type == REG_MULTI_SZ) && (size < MAX_PATH*sizeof(WCHAR)) && (pPath = AllocName(size)))
		memcpy(pPath->name,pProp,size);
	if (!(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"init",0,KEY_ALL_ACCESS,&key))) {
		size = MAX_APPSTART_KEYNAME;
		size2 = MAX_PATH*sizeof(WCHAR);
		enumindex = 0;
		while (!RegEnumValue(key,enumindex++,pName,&size,0,&type,pProp,&size2)) {
			if (!Kbstrcmpn(pName,L"Launch",6)) {
				if (size2 > MAX_PATH*sizeof(WCHAR))
					goto nextkey;
				index = atoiW(pName+6);
				for (loop = 0; loop < nextapp; loop++)
					if (index == AppInfo[loop].index)
						break;
				if (loop == nextapp) {
					if (nextapp == MAX_APPSTART_INDEX)
						goto nextkey;
					AppInfo[nextapp++].index = index;
				}
				kstrcpyW(AppInfo[loop].appname,(LPWSTR)pProp);
			} else if (!Kbstrcmpn(pName,L"Depend",6)) {
				if ((size2 > MAX_APPSTART_INDEX*sizeof(WORD)) || (size2 & 1))
					goto nextkey;
				index = atoiW(pName+6);
				for (loop = 0; loop < nextapp; loop++)
					if (index == AppInfo[loop].index)
						break;
				if (loop == nextapp) {
					if (nextapp == MAX_APPSTART_INDEX)
						goto nextkey;
					AppInfo[nextapp++].index = index;
				}
				memcpy(AppInfo[loop].depchain,pProp,size2);
				if (size2 != MAX_APPSTART_INDEX*sizeof(WORD))
					AppInfo[loop].depchain[size2/2] = 0;
			}
nextkey:
			size = MAX_APPSTART_KEYNAME;
			size2 = MAX_PATH*sizeof(WCHAR);
		}
		RegCloseKey(key);
		for (size = 1; size < nextapp; size++) {
			for (size2 = 0; size2 < size; size2++)
				if (AppInfo[size].index < AppInfo[size2].index)
					break;
			memcpy(&SwapInfo,&AppInfo[size],sizeof(appinfo_t));
			for (loop = size; loop > size2; loop--)
				memcpy(&AppInfo[loop],&AppInfo[loop-1],sizeof(appinfo_t));
			memcpy(&AppInfo[size2],&SwapInfo,sizeof(appinfo_t));
		}
		for (loop = 0; loop < nextapp; loop++)
			AppInfo[loop].done = 0;
		loop = 0;	
		while (loop < nextapp) {
			if (CanBeLaunched(loop)) {
				kItoW(pName,AppInfo[loop].index);
				if (!CreateProcess(AppInfo[loop].appname,pName,0,0,0,0,0,0,0,0))
					AppInfo[loop].done = 1;
				loop++;
			} else
				WaitForMultipleObjects(1,&hSignalApp,0,INFINITE);
		}
		// launch apps!
	}
	pappinfo = 0;
	CloseHandle(hSignalApp);
	pPSLNotify(DLL_SYSTEM_STARTED,0,0);
}

BOOL SC_SetDbgZone(DWORD dwProcid, LPVOID lpvMod, LPVOID BasePtr, DWORD zone, LPDBGPARAM lpdbgTgt) {
	DWORD retval = FALSE;
	PMODULE pMod;
	PPROCESS pProc;
	LPDBGPARAM lpdbgSrc = 0;
    ACCESSKEY ulOldKey;
    SWITCHKEY(ulOldKey,0xffffffff);
	if (dwProcid) {
		if (!lpvMod && (pProc = HandleToProc((HANDLE)dwProcid)) && pProc->ZonePtr) {
			if (zone != 0xffffffff)
				pProc->ZonePtr->ulZoneMask=zone;
			lpdbgSrc = pProc->ZonePtr;
			retval = TRUE;
		}
	} else if (lpvMod) {
		pMod = (PMODULE)lpvMod;
	    if (IsValidModule(pMod) && pMod->ZonePtr) {
			DWORD dwUseMask = pMod->inuse | 0x1;    // Add the kernel's bit
			DWORD dwZeroAddr = (DWORD)(ZeroPtr((LPVOID)(pMod->ZonePtr)));
			INT i=0;
			// set source for copying names: use kernel's version since it's guaranteed to be there !
			lpdbgSrc = (LPDBGPARAM)(MapPtrProc(dwZeroAddr,&ProcArray[0]));
			if (zone != 0xffffffff) {
				for (i = 0; dwUseMask; i++, dwUseMask >>= 1) {
				    if (dwUseMask & 0x1)
					    ((LPDBGPARAM)MapPtrProc(dwZeroAddr,&ProcArray[i]))->ulZoneMask = zone;
				}
			}
			retval = TRUE;
	    }
	}
    // Copy information if required
    if (lpdbgSrc && lpdbgTgt)
		memcpy(lpdbgTgt, lpdbgSrc, sizeof(DBGPARAM));
	// restore access rights
	SETCURKEY(ulOldKey);
	return retval;
}

BOOL SC_RegisterDbgZones(HANDLE hMod, LPDBGPARAM lpdbgparam) {
	DWORD hKeyPeg=0, hKeyZones=0;
    DWORD dwType, dwData, dwSize;
    CHAR szName[16];
	PMODULE pMod = (PMODULE)hMod;
	HKEY hZones;
	BOOL bTryLocal;
	lpdbgparam = MapPtr(lpdbgparam);
	if (pMod) {
		if (!IsValidModule(pMod))
			return FALSE;
		if (pMod->ZonePtr)
			return TRUE;
		pMod->ZonePtr = (LPDBGPARAM)MapPtrProc(ZeroPtr(lpdbgparam),&ProcArray[0]);
	} else
	    pCurThread->pOwnerProc->ZonePtr = lpdbgparam;

	bTryLocal = TRUE;
	//
	// See if the debug zone mask is specified in the desktop PC's registry
	//
	rRegOpen((DWORD)HKEY_CURRENT_USER, "Pegasus", &hKeyPeg);
	if (hKeyPeg) {
		rRegOpen(hKeyPeg, "Zones", &hKeyZones);
		if (hKeyZones) {
			KUnicodeToAscii(szName, lpdbgparam->lpszName, sizeof(szName));
			rRegGet(hKeyZones, szName, &dwType, (LPBYTE)&dwData, &dwSize);
			if (dwType == REG_DWORD) {
			    if (pMod)
			    	SC_SetDbgZone(0,pMod,0,dwData,0);
				else
					SC_SetDbgZone((DWORD)hCurProc,0,0,dwData,0);
				bTryLocal = FALSE;
			}
			rRegClose(hKeyZones);
		}
		rRegClose(hKeyPeg);
	}

	//
	// Can only check the local registry if the filesys API is registered.
	//
	if (bTryLocal && (UserKInfo[KINX_API_MASK] & (1 << SH_FILESYS_APIS))) {
		//
		// See if the debug zone mask is specified in the Windows CE registry
		// (in HLM\DebugZones:<module name>)
		//
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("DebugZones"), 0, 0,
						&hZones) == ERROR_SUCCESS) {
			dwSize = sizeof(DWORD);
			if (RegQueryValueEx(hZones, lpdbgparam->lpszName, 0, &dwType,
						(PUCHAR)&dwData, &dwSize) == ERROR_SUCCESS) {
				if (dwType == REG_DWORD) {
					if (pMod)
						SC_SetDbgZone(0,pMod,0,dwData,0);
					else
						SC_SetDbgZone((DWORD)hCurProc,0,0,dwData,0);
				}
			}
			RegCloseKey(hZones);
		}
	}
	return TRUE;
}

PTHREAD ZeroAllThreadsTLS(PTHREAD pCur, DWORD slot) {
    KCALLPROFON(65);
    if (pCur == (PTHREAD)1)
        pCur = pCurProc->pTh;
    else if (HandleToThread(pCur->hTh) != pCur)
        return (PTHREAD)1;
    pCur->tlsPtr[slot] = 0;
    KCALLPROFOFF(65);
    return pCur->pNextInProc;
}

/* Thread local storage APIs */

DWORD SC_TlsCall(DWORD type, DWORD slot) {
	int loop, mask;
	DWORD used;
	PTHREAD pth;
	DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall entry: %8.8lx %8.8lx\r\n",type,slot));
	switch (type) {
		case TLS_FUNCALLOC:
			do {
				used = pCurProc->tlsLowUsed;
				for (loop = 0, mask = 1; loop < 32; loop++, mask <<= 1)
					if (!(used & mask))
						break;
			} while ((loop != 32) && ((DWORD)InterlockedTestExchange(&pCurProc->tlsLowUsed,used,used | mask) != used));
			if (loop == 32) {
				do {
					used = pCurProc->tlsHighUsed;
					for (mask = 1; loop < 64; loop++, mask <<= 1)
						if (!(used & mask))
							break;
				} while ((loop != 64) && ((DWORD)InterlockedTestExchange(&pCurProc->tlsHighUsed,used,used|mask) != used));
				if (loop == 64) {
					DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall exit: %8.8lx\r\n",0xffffffff));
					return 0xffffffff;
				}
			}
			DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall exit: %8.8lx\r\n",loop));
			return loop;
		case TLS_FUNCFREE:
			if (slot >= 64) {
				DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall exit: %8.8lx\r\n",0));
				return 0;
			}
			pth = (PTHREAD)1;
			while (pth = (PTHREAD)KCall((PKFN)ZeroAllThreadsTLS,pth,slot))
				;
			if (slot < 32) {
				do {
					used = pCurProc->tlsLowUsed;
				} while ((DWORD)InterlockedTestExchange(&pCurProc->tlsLowUsed,used,used & ~(1<<slot)) != used);
			} else {
				do {
					used = pCurProc->tlsHighUsed;
				} while ((DWORD)InterlockedTestExchange(&pCurProc->tlsHighUsed,used,used & ~(1<<(slot-32))) != used);
			}
			DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall exit: %8.8lx\r\n",1));
			return 1;
		default:
			DEBUGCHK(0);
			DEBUGMSG(ZONE_ENTRY,(L"SC_TlsCall exit: %8.8lx\r\n",0));
			return 0;
	}
}

//  @func DWORD | SetProcPermissions | Sets the kernel's internal thread permissions
//  @rdesc Returns the new permission dword
//  @parm DWORD | newpermissions | the new permissions bitmask
//  @comm Sets the internal permissions bitmask for the current thread

DWORD SC_SetProcPermissions(DWORD newperms) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetProcPermissions entry: %8.8lx\r\n",newperms));
	if (!newperms)
		newperms = 0xffffffff;
	else {
		// prevent user from removing kernel access
		AddAccess(&newperms, ProcArray[0].aky);
		// prevent user from removing access to his stack
		AddAccess(&newperms, pCurThread->pOwnerProc->aky);
	}
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetProcPermissions failed due to insufficient trust\r\n"));
	if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
		SWITCHKEY(retval, newperms);
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetProcPermissions exit: %8.8lx\r\n",retval));
	return retval;
}

//  @func DWORD | GetCurrentPermissions | Obtains the kernel's internal thread permissions dword
//  @rdesc Returns the thread permissions dword
//  @comm Obtains the internal permissions bitmask for the current thread

DWORD SC_GetCurrentPermissions(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCurrentPermissions entry\r\n"));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetCurrentPermissions exit: %8.8lx\r\n",pCurThread->aky));
	return pCurThread->aky;
}

DWORD NormalBias, DaylightBias, InDaylight;

void SC_SetDaylightTime(DWORD dst) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetDaylightTime entry: %8.8lx\r\n",dst));
	InDaylight = dst;
	KInfoTable[KINX_TIMEZONEBIAS] = (InDaylight ? DaylightBias : NormalBias);
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetDaylightTime exit\r\n"));
}

void SC_SetTimeZoneBias(DWORD dwBias, DWORD dwDaylightBias) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetTimeZoneBias entry: %8.8lx %8.8lx\r\n",dwBias,dwDaylightBias));
	NormalBias = dwBias;
	DaylightBias = dwDaylightBias;
	KInfoTable[KINX_TIMEZONEBIAS] = (InDaylight ? DaylightBias : NormalBias);
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetTimeZoneBias exit\r\n"));
}

/* Set thread last error */

void SC_SetLastError(DWORD dwError) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetLastError entry: %8.8lx\r\n",dwError));
	pCurThread->dwLastError = dwError;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetLastError exit\r\n"));
}

/* Get thread last error */

DWORD SC_GetLastError(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetLastError entry\r\n"));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetLastError exit: %8.8lx\r\n",pCurThread->dwLastError));
	return pCurThread->dwLastError;
}

/* Get thread return code */

BOOL SC_ThreadGetCode(HANDLE hTh, LPDWORD lpExit) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadGetCode entry: %8.8lx %8.8lx\r\n",hTh,lpExit));
	*lpExit = GetUserInfo(hTh);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadGetCode exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

/* Get process return code */

BOOL SC_ProcGetCode(HANDLE hProc, LPDWORD lpExit) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetCode entry: %8.8lx %8.8lx\r\n",hProc,lpExit));
	*lpExit = GetUserInfo(hProc);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetCode exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

DWORD SC_ProcGetIndex(HANDLE hProc) {
	PPROCESS pproc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetIndex entry: %8.8lx\r\n",hProc));
	if (!(pproc = HandleToProc(hProc))) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetIndex exit: %8.8lx\r\n",0xffffffff));
		return 0xffffffff;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcGetIndex exit: %8.8lx\r\n",pproc->procnum));
	return pproc->procnum;
}

BOOL SC_ProcFlushICache(HANDLE hProc, LPCVOID lpBaseAddress, DWORD dwSize) {
	SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
	return TRUE;
}

BOOL SC_ProcReadMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead) {
	BOOL retval = FALSE;
	LPBYTE ptr;
	PPROCESS pproc;
   	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_ProcReadMemory failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return FALSE;
	}
    if (lpNumberOfBytesRead)
    	*lpNumberOfBytesRead = 0;
	if (pproc = HandleToProc(hProcess)) {
	    ACCESSKEY ulOldKey;
    	SWITCHKEY(ulOldKey,0xffffffff);
		ptr = MapPtrProc(lpBaseAddress,pproc);
		__try {
			memcpy(lpBuffer,ptr,nSize);
			if (lpNumberOfBytesRead)
				*lpNumberOfBytesRead = nSize;
			retval = TRUE;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			SetLastError(ERROR_NOACCESS);
		}
		SETCURKEY(ulOldKey);
	}
	return retval;
}

BOOL SC_ProcWriteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesWritten) {
	BOOL retval = FALSE;
	LPBYTE ptr, ptr2, ptrend;
	PPROCESS pproc;
	DWORD bytesleft, bytestocopy;
   	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_ProcWriteMemory failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (lpNumberOfBytesWritten)
		*lpNumberOfBytesWritten = 0;
	if (pproc = HandleToProc(hProcess)) {
	    ACCESSKEY ulOldKey;
	    SWITCHKEY(ulOldKey,0xffffffff);
		ptr = MapPtrProc(lpBaseAddress,pproc);
		__try {
			bytesleft = nSize;
			while (bytesleft) {
				BOOL bUnlock;
				bytestocopy = 0x10000 - ((DWORD)ptr & 0xffff);
				if (bytestocopy > bytesleft)
					bytestocopy = bytesleft;
				bUnlock = LockPages(ptr,bytestocopy,0,LOCKFLAG_READ);
				ptr2 = ptr;
				ptrend = ptr2 + bytestocopy;
				while (ptr2 < ptrend) {
					MEMORY_BASIC_INFORMATION mbi;
					if (!VirtualQuery(ptr2,&mbi,sizeof(mbi)))
						bUnlock = FALSE;
					else if ((mbi.Protect != PAGE_READWRITE) && (mbi.Protect != PAGE_EXECUTE_READWRITE)) {
						DWORD dwProt;
						dwProt = ((mbi.Protect == PAGE_EXECUTE) || (mbi.Protect == PAGE_EXECUTE_READ)) ?
							PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
						VirtualProtect(ptr2,PAGE_SIZE - ((DWORD)ptr2 & (PAGE_SIZE-1)), dwProt, &dwProt);
						bUnlock = FALSE;
					}
					ptr2 += PAGE_SIZE - ((DWORD)ptr2 & (PAGE_SIZE-1));
				}
				if (bUnlock)
					UnlockPages(ptr,bytestocopy);
				memcpy(ptr,lpBuffer,bytestocopy);
				ptr += bytestocopy;
				lpBuffer = (LPVOID)((LPBYTE)lpBuffer + bytestocopy);
				bytesleft -= bytestocopy;
			}
			if (lpNumberOfBytesWritten)
				*lpNumberOfBytesWritten = nSize;
			SC_CacheSync(CACHE_SYNC_DISCARD | CACHE_SYNC_INSTRUCTIONS);
			retval = TRUE;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			SetLastError(ERROR_NOACCESS);
		}
		SETCURKEY(ulOldKey);
	}
	return retval;
}

HANDLE SC_OpenProcess(DWORD fdwAccess, BOOL fInherit, DWORD IDProcess) {
	if (!fInherit && (GetHandleType((HANDLE)IDProcess) == SH_CURPROC) && IncRef((HANDLE)IDProcess, pCurProc))
		return (HANDLE)IDProcess;
	SetLastError(ERROR_INVALID_PARAMETER);
	return NULL;
}

LPBYTE SC_THGrow(THSNAP *pSnap, DWORD dwSize) {
	LPBYTE lpRet = pSnap->pNextFree;
	pSnap->pNextFree += dwSize;
	if (pSnap->pNextFree > pSnap->pHighReserve) {
		ERRORMSG(1,(L"THGrow: Not enough reservation for toolhelp!\r\n"));
		return 0;
	}
	if (pSnap->pNextFree > pSnap->pHighCommit) {
		if (!VirtualAlloc(pSnap->pHighCommit, (pSnap->pNextFree - pSnap->pHighCommit + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
			MEM_COMMIT, PAGE_READWRITE))
			return 0;
		pSnap->pHighCommit = (LPBYTE)((DWORD)(pSnap->pNextFree + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
	}
	return lpRet;
}

BOOL THGetProcs(THSNAP *pSnap, BOOL bGetHeap) {
	TH32PROC **ppNext;
	PTHREAD pTh;
	DWORD loop;
	CALLBACKINFO cbi;
	LPBYTE pNextFree;
	ppNext = &pSnap->pProc;
	for (loop = 0; loop < MAX_PROCESSES; loop++) {
		if (ProcArray[loop].dwVMBase && (!loop || ProcStarted(&ProcArray[loop]))) {
			if (!(*ppNext = (TH32PROC *)THGrow(pSnap,sizeof(TH32PROC))))
				return FALSE;
			(*ppNext)->procentry.dwSize = sizeof(PROCESSENTRY32);
			(*ppNext)->procentry.cntUsage = 1;
			(*ppNext)->procentry.th32ProcessID = (DWORD)ProcArray[loop].hProc;
			(*ppNext)->procentry.th32ModuleID = 0;
			(*ppNext)->procentry.cntThreads = 0;
			for (pTh = ProcArray[loop].pTh; pTh; pTh = pTh->pNextInProc)
				(*ppNext)->procentry.cntThreads++;
			(*ppNext)->procentry.th32ParentProcessID = 0;
			(*ppNext)->procentry.pcPriClassBase = THREAD_PRIORITY_NORMAL;
			(*ppNext)->procentry.dwFlags = ProcArray[loop].ZonePtr ? ProcArray[loop].ZonePtr->ulZoneMask : 0;
			(*ppNext)->procentry.th32MemoryBase = ProcArray[loop].dwVMBase;
			(*ppNext)->procentry.th32AccessKey = ProcArray[loop].aky;
			kstrcpyW((*ppNext)->procentry.szExeFile,ProcArray[loop].lpszProcName);
			if (loop && bGetHeap) {
				pNextFree = pSnap->pNextFree;
				__try {
					cbi.hProc = ProcArray[loop].hProc;
					cbi.pfn = (FARPROC)pGetProcessHeap;
					cbi.pvArg0 = 0;
					(*ppNext)->procentry.th32DefaultHeapID = PerformCallBack4(&cbi);
					cbi.pfn = (FARPROC)pGetHeapSnapshot;
					cbi.pvArg0 = pSnap;
					if (!PerformCallBack4(&cbi,1,&(*ppNext)->pMainHeapEntry)) {
						(*ppNext)->procentry.th32DefaultHeapID = 0;
						(*ppNext)->pMainHeapEntry = 0;
						pSnap->pNextFree = pNextFree;
					}
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					(*ppNext)->procentry.th32DefaultHeapID = 0;
					(*ppNext)->pMainHeapEntry = 0;
					pSnap->pNextFree = pNextFree;
				}
			} else {
				(*ppNext)->procentry.th32DefaultHeapID = 0;
				(*ppNext)->pMainHeapEntry = 0;
			}
			ppNext = &(*ppNext)->pNext;
		}
	}
	*ppNext = 0;
	return TRUE;
}

BOOL THGetMods(THSNAP *pSnap, DWORD dwProcID) {
	TH32MOD **ppNext;
	PMODULE pMod;
	PPROCESS pProc;
	DWORD loop;
	ppNext = &pSnap->pMod;
	if (dwProcID && !(pProc = HandleToProc((HANDLE)dwProcID)))
		return FALSE;
	for (pMod = pModList; pMod; pMod = pMod->pMod) {
		if (dwProcID && !HasModRefProcPtr(pMod,pProc))
			continue;
		if (!(*ppNext = (TH32MOD *)THGrow(pSnap,sizeof(TH32MOD))))
			return FALSE;
		(*ppNext)->modentry.dwSize = sizeof(MODULEENTRY32);
		(*ppNext)->modentry.th32ModuleID = (DWORD)pMod;
		(*ppNext)->modentry.th32ProcessID = dwProcID;
		(*ppNext)->modentry.GlblcntUsage = 0;
		for (loop = 0; loop < MAX_PROCESSES; loop++)
			(*ppNext)->modentry.GlblcntUsage += pMod->refcnt[loop];
		(*ppNext)->modentry.ProccntUsage = dwProcID ? pMod->refcnt[pProc->procnum] : pMod->inuse;
		(*ppNext)->modentry.modBaseAddr = (LPBYTE)ZeroPtr(pMod->BasePtr);
		(*ppNext)->modentry.modBaseSize = pMod->e32.e32_vsize;
		(*ppNext)->modentry.hModule = (HMODULE)pMod;
		(*ppNext)->modentry.dwFlags = pMod->ZonePtr ? pMod->ZonePtr->ulZoneMask : 0;
		kstrcpyW((*ppNext)->modentry.szModule,pMod->lpszModName);
		(*ppNext)->modentry.szExePath[0] = 0;
		ppNext = &(*ppNext)->pNext;
	}
	*ppNext = 0;
	return TRUE;
}

BOOL THGetThreads(THSNAP *pSnap) {
	TH32THREAD **ppNext;
	PTHREAD pTh;
	DWORD loop;
	ppNext = &pSnap->pThread;
	for (loop = 0; loop < MAX_PROCESSES; loop++) {
		if (ProcArray[loop].dwVMBase && (!loop || ProcStarted(&ProcArray[loop]))) {
			for (pTh = ProcArray[loop].pTh; pTh; pTh = pTh->pNextInProc) {
				if (!(*ppNext = (TH32THREAD *)THGrow(pSnap,sizeof(TH32THREAD))))
					return FALSE;
				(*ppNext)->threadentry.dwSize = sizeof(THREADENTRY32);
				(*ppNext)->threadentry.cntUsage = 1;
				(*ppNext)->threadentry.th32ThreadID = (DWORD)pTh->hTh;
				(*ppNext)->threadentry.th32OwnerProcessID = (DWORD)ProcArray[loop].hProc;
				(*ppNext)->threadentry.tpBasePri = GET_BPRIO(pTh);
				(*ppNext)->threadentry.tpDeltaPri = GET_BPRIO(pTh) - GET_CPRIO(pTh);
				(*ppNext)->threadentry.dwFlags = GET_SLEEPING(pTh) ? 4 + (pTh->lpProxy != 0) + (pTh->bSuspendCnt != 0) * 2 : GET_RUNSTATE(pTh);
				(*ppNext)->threadentry.th32CurrentProcessID = (DWORD)pTh->pProc->hProc;
				(*ppNext)->threadentry.th32AccessKey = pTh->aky;
				ppNext = &(*ppNext)->pNext;
			}
		}
	}
	*ppNext = 0;
	return TRUE;
}

BOOL THGetHeaps(THSNAP *pSnap, DWORD dwProcID) {
	CALLBACKINFO cbi;
	LPBYTE pNextFree = pSnap->pNextFree;
	if ((HANDLE)dwProcID == ProcArray[0].hProc)
		pSnap->pHeap = 0;
	else {
		__try {
			cbi.hProc = (HANDLE)dwProcID;
			cbi.pfn = (FARPROC)pGetHeapSnapshot;
			cbi.pvArg0 = pSnap;
			if (!PerformCallBack4(&cbi,0,&pSnap->pHeap))
				return FALSE;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			pSnap->pNextFree = pNextFree;
			pSnap->pHeap = 0;
		}
	}
	return TRUE;
}

THSNAP *SC_THCreateSnapshot(DWORD dwFlags, DWORD dwProcID) {
    ACCESSKEY ulOldKey;
	THSNAP *pSnap;
	if (!dwProcID || (dwProcID == (DWORD)GetCurrentProcess()))
		dwProcID = (DWORD)hCurProc;
	else if (GetHandleType((HANDLE)dwProcID) != SH_CURPROC) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	if (!(pSnap = VirtualAlloc(0, THSNAP_RESERVE, MEM_RESERVE, PAGE_NOACCESS))) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}
	pSnap = MapPtr(pSnap);
	if (!VirtualAlloc(pSnap,PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
		VirtualFree(pSnap,0,MEM_RELEASE);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}
	pSnap->pNextFree = (LPBYTE)pSnap + sizeof(THSNAP);
	pSnap->pHighCommit = (LPBYTE)pSnap + PAGE_SIZE;
	pSnap->pHighReserve = (LPBYTE)pSnap + THSNAP_RESERVE;
    SWITCHKEY(ulOldKey,0xffffffff);
	EnterCriticalSection(&LLcs);
	if (((dwFlags & TH32CS_SNAPPROCESS) && !THGetProcs(pSnap,(dwFlags & TH32CS_SNAPNOHEAPS ? 0 : 1))) ||
		((dwFlags & TH32CS_SNAPMODULE) && !THGetMods(pSnap,(dwFlags & TH32CS_GETALLMODS) ? 0 : dwProcID)) ||
		((dwFlags & TH32CS_SNAPTHREAD) && !THGetThreads(pSnap)) ||
		((dwFlags & TH32CS_SNAPHEAPLIST) && !THGetHeaps(pSnap,dwProcID))) {
		VirtualFree(pSnap,THSNAP_RESERVE,MEM_DECOMMIT);
		VirtualFree(pSnap,0,MEM_RELEASE);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		pSnap = INVALID_HANDLE_VALUE;
	}
	LeaveCriticalSection(&LLcs);
	SETCURKEY(ulOldKey);
	return pSnap;
}

DWORD SC_GetProcessVersion(DWORD dwProcessId) {
	PPROCESS pProc;
	if (!dwProcessId)
		pProc = pCurProc;
	else if (!(pProc = HandleToProc((HANDLE)dwProcessId))) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	return MAKELONG(pProc->e32.e32_ceverminor,pProc->e32.e32_cevermajor);
}

BOOL SC_SetStdioPathW(DWORD id, LPCWSTR pwszPath) {
	LPName pName, pName2;
	DWORD len;
	if ((id >= 3) || (pwszPath && ((len = strlenW(pwszPath)) > MAX_PATH-1))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pwszPath) {
		if (!(pName = AllocName(sizeof(WCHAR)*(len+1)))) {
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			return FALSE;
		}
		memcpy(pName->name,pwszPath,sizeof(WCHAR)*(len+1));
	} else
		pName = 0;
	EnterCriticalSection(&NameCS);
	pName2 = pCurProc->pStdNames[id];
	pCurProc->pStdNames[id] = pName;
	LeaveCriticalSection(&NameCS);
	if (pName2)
		FreeName(pName2);
	return TRUE;
}

BOOL SC_GetStdioPathW(DWORD id, PWSTR pwszBuf, LPDWORD lpdwLen) {
	DWORD len;
	Name name;
	if (id >= 3) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	LockPages(&name,sizeof(name),0,LOCKFLAG_WRITE);
	EnterCriticalSection(&NameCS);
	if (!(pCurProc->pStdNames[id]))
		len = 0;
	else {
		len = strlenW(pCurProc->pStdNames[id]->name)+1;
		memcpy(name.name,pCurProc->pStdNames[id]->name,sizeof(WCHAR)*len);
	}
	LeaveCriticalSection(&NameCS);
	UnlockPages(&name,sizeof(name));
	if (*lpdwLen < len) {
		*lpdwLen = len;
		KSetLastError(pCurThread,ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}
	*lpdwLen = len;
	memcpy(pwszBuf,name.name,sizeof(WCHAR)*len);
	return TRUE;
}

BOOL (*pQueryPerformanceCounter)(LARGE_INTEGER *lpPerformanceCount);
BOOL (*pQueryPerformanceFrequency)(LARGE_INTEGER *lpPerformanceFrequency);

BOOL SC_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	if (pQueryPerformanceCounter)
		return pQueryPerformanceCounter(lpPerformanceCount);
	lpPerformanceCount->HighPart = 0;
	lpPerformanceCount->LowPart = CurMSec;
	return TRUE;
}

BOOL SC_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
	if (pQueryPerformanceFrequency)
		return pQueryPerformanceFrequency(lpFrequency);
	lpFrequency->HighPart = 0;
	lpFrequency->LowPart = 1000;
	return TRUE;
}

DWORD (*pReadRegistryFromOEM)(DWORD dwFlags, LPBYTE pBuf, DWORD len);
BOOL (*pWriteRegistryToOEM)(DWORD dwFlags, LPBYTE pBuf, DWORD len);

DWORD SC_ReadRegistryFromOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len) {
	if (pReadRegistryFromOEM)
		return pReadRegistryFromOEM(dwFlags, pBuf, len);
	return 0;
}

BOOL SC_WriteRegistryToOEM(DWORD dwFlags, LPBYTE pBuf, DWORD len) {
	if (dwFlags == REG_WRITE_BYTES_PROBE)
		return (pWriteRegistryToOEM ? TRUE : FALSE);
	if (pWriteRegistryToOEM)
		return pWriteRegistryToOEM(dwFlags, pBuf, len);
	return 0;
}

DWORD SC_GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize) {
	PPROCESS pProc;
	openexe_t *oe;
	CEOIDINFO ceoi;
	LPWSTR pstr;
	int loop;
	if (!nSize) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	if (!hModule)
		oe = &pCurProc->oe;
	else if (pProc = HandleToProc((HANDLE)hModule))
		oe = &pProc->oe;
	else if (IsValidModule((PMODULE)hModule))
		oe = &(((LPMODULE)hModule)->oe);
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	if (!GetNameFromOE(&ceoi,oe)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	for (pstr = ceoi.infFile.szFileName, loop = 1; (loop < (int)nSize) && *pstr; loop++)
		*lpFilename++ = *pstr++;
	*lpFilename = 0;
	return loop-1;
}

void SC_CacheSync(int flags) {
#if defined(SH3)
	if ((flags & CACHE_SYNC_WRITEBACK) && !(flags & (CACHE_SYNC_DISCARD)))
		FlushCacheNoDiscard();
	else
		FlushCache();
#elif defined(x86)
	FlushCache();
#else
	FlushDCache();
	if (flags & (CACHE_SYNC_INSTRUCTIONS | CACHE_SYNC_DISCARD))
		FlushICache();
#endif
}

void SC_RegisterGwesHandler(LPVOID pfn) {
	pGwesHandler = pfn;
}

VOID xxx_RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD cArgs, CONST DWORD *lpArgs) {
	RaiseException(dwExceptionCode, dwExceptionFlags, cArgs, lpArgs);
}

void DoMapPtr(LPVOID *P) {
	if (((DWORD)*P >= 0x10000) && !((DWORD)*P >> VA_SECTION))
		*P = (LPVOID)((DWORD)*P | (DWORD)pCurProc->dwVMBase);
}
#define MapArgPtr(P) DoMapPtr((LPVOID *)&(P))

const CINFO *SwitchToProc(PCALLSTACK pcstk, DWORD dwAPISet) {
	const CINFO *pci;
	if (!(pci = SystemAPISets[dwAPISet]))
        RaiseException(STATUS_INVALID_SYSTEM_SERVICE, EXCEPTION_NONCONTINUABLE, 0, 0);
    pcstk->pprcLast = pCurProc;
    pcstk->akyLast = CurAKey;
    pcstk->retAddr = 0;
    pcstk->pcstkNext = pCurThread->pcstkTop;
    pCurThread->pcstkTop = pcstk;
	pCurThread->pProc = pci->pServer;
	AddAccess(&pCurThread->aky, pci->pServer->aky);
	SetCPUASID(pCurThread);
	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);
	return pci;
}

const CINFO *SwitchToProcHandle(PCALLSTACK pcstk, HANDLE *ph, DWORD dwType) {
	const CINFO *pci;
    const HDATA *phd;
    phd = HandleToPointer(*ph);
    if (!phd || !TestAccess(&phd->lock, &pCurThread->aky))
    	return 0;
    pci = phd->pci;
    if (pci->type != dwType)
		return 0;
	*ph = (HANDLE)phd->pvObj;
    pcstk->pprcLast = pCurProc;
    pcstk->akyLast = CurAKey;
    pcstk->retAddr = 0;
    pcstk->pcstkNext = pCurThread->pcstkTop;
    pCurThread->pcstkTop = pcstk;
	pCurThread->pProc = pci->pServer;
	AddAccess(&pCurThread->aky, pci->pServer->aky);
	SetCPUASID(pCurThread);
	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);
	return pci;
}

extern BOOL TryCloseMappedHandle(HANDLE h);

const CINFO *SwitchToProcHandleClosing(PCALLSTACK pcstk, HANDLE *ph, LPBOOL bRet) {
	const CINFO *pci;
    const HDATA *phd;
    HANDLE hVal;
    phd = HandleToPointer(*ph);
    if (!phd)
    	return 0;
    pci = phd->pci;
    if (!TestAccess(&phd->lock, &pCurProc->aky)) {
	    if ((phd->lock == 1) && (phd->ref.count == 1)) {
	    	*bRet = TryCloseMappedHandle(*ph);
			return 0;
		}
		return 0;
	}
	if (pci->disp == DISPATCH_KERNEL_PSL) {
		*bRet = (*(BOOL (*)(HANDLE))(pci->ppfnMethods[0]))(*ph);
		return 0;
	}
	hVal = (HANDLE)phd->pvObj;
	if (DecRef(*ph, pCurProc, FALSE))
		FreeHandle(*ph);
	*ph = hVal;
    pcstk->pprcLast = pCurProc;
    pcstk->akyLast = CurAKey;
    pcstk->retAddr = 0;
    pcstk->pcstkNext = pCurThread->pcstkTop;
    pCurThread->pcstkTop = pcstk;
	pCurThread->pProc = pci->pServer;
	AddAccess(&pCurThread->aky, pci->pServer->aky);
	SetCPUASID(pCurThread);
	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);
	return pci;
}

void SwitchBack() {
	PCALLSTACK pcstk;
	pcstk = pCurThread->pcstkTop;
	pCurThread->pcstkTop = pcstk->pcstkNext;
	SETCURKEY(pcstk->akyLast);
	pCurThread->pProc = pcstk->pprcLast;
    SetCPUASID(pCurThread);
}

DWORD PerformCallBack4Int(CALLBACKINFO *pcbi, LPVOID p2, LPVOID p3, LPVOID p4) {
	CALLSTACK cstk, *pcstk;
	DWORD dwRet, dwFault = 0;
	PPROCESS pProc;
	if (!(pProc = HandleToProc(pcbi->hProc))) {
        RaiseException(STATUS_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, 0);
        return 0;
	}
    cstk.pprcLast = pCurProc;
    cstk.akyLast = CurAKey;
    cstk.retAddr = 0;
    cstk.pcstkNext = pCurThread->pcstkTop;
    pCurThread->pProc = pProc;
    pCurThread->pcstkTop = &cstk;
    if (pProc != &ProcArray[0]) {
	    pcstk = &cstk;
	    while ((pcstk = (PCALLSTACK)((ulong)pcstk->pcstkNext&~1)) && (pcstk->pprcLast != pProc))
	       	;
	    if (pcstk)
	       	pCurThread->aky = pcstk->akyLast;
	    else {
			pCurThread->aky = pCurThread->pOwnerProc->aky;
	       	AddAccess(&pCurThread->aky, ProcArray[0].aky);
	        AddAccess(&pCurThread->aky, pProc->aky);
	    }
	}
	SetCPUASID(pCurThread);
	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);
	try {
		dwRet = (*(DWORD (*)(LPVOID, LPVOID, LPVOID, LPVOID))(ZeroPtr(pcbi->pfn)))(pcbi->pvArg0,p2,p3,p4);
	} except (EXCEPTION_EXECUTE_HANDLER) {
		dwFault = 1;
		dwRet = 0; // set return value
	}
	SwitchBack();
	if (dwFault)
        RaiseException(STATUS_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, 0);
	return dwRet;
}

DWORD SC_PerformCallBack4(CALLBACKINFO *pcbi, LPVOID p2, LPVOID p3, LPVOID p4) {
	if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
		return PerformCallBack4Int(pcbi,p2,p3,p4);
  	RaiseException(STATUS_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, 0);
	return 0;
}

HGDIOBJ SC_SelectObject(HANDLE hDC, HANDLE hObj) {
	CALLSTACK cstk;
	HGDIOBJ hRet;
    const CINFO *pci;
	pci = SwitchToProc(&cstk,SH_GDI);
    try {
		hRet = (*(HGDIOBJ (*)(HANDLE,HANDLE))(pci->ppfnMethods[SELECTOBJECT]))(hDC,hObj);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_PatBlt(HDC hdcDest,int nXLeft,int nYLeft,int nWidth,int nHeight,DWORD dwRop) {
	CALLSTACK cstk;
	BOOL bRet = 0;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		bRet = (*(BOOL (*)(HDC,int,int,int,int,DWORD))(pci->ppfnMethods[PATBLT]))(hdcDest,nXLeft,nYLeft,nWidth,nHeight,dwRop);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_GetTextExtentExPointW(HDC hdc,LPCWSTR lpszStr,int cchString,int nMaxExtent,LPINT lpnFit,LPINT alpDx,LPSIZE lpSize) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(lpszStr);
	MapArgPtr(lpnFit);
	MapArgPtr(alpDx);
	MapArgPtr(lpSize);
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		bRet = (*(BOOL (*)(HDC,LPCWSTR,int,int,LPINT,LPINT,LPSIZE))(pci->ppfnMethods[GETTEXTEXTENTEXPOINTW]))(hdc,lpszStr,cchString,nMaxExtent,lpnFit,alpDx,lpSize);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

HBRUSH SC_GetSysColorBrush(int nIndex) {
	CALLSTACK cstk;
	HBRUSH hRet ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		hRet = (*(HBRUSH (*)(int))(pci->ppfnMethods[GETSYSCOLORBRUSH]))(nIndex);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

COLORREF SC_SetBkColor(HDC hDC,COLORREF dwColor) {
	CALLSTACK cstk;
	COLORREF cr ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		cr = (*(COLORREF (*)(HDC,COLORREF))(pci->ppfnMethods[SETBKCOLOR]))(hDC,dwColor);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		cr = 0;
	}
	SwitchBack();
	return cr;
}

HWND SC_GetParent(HWND hwnd) {
	CALLSTACK cstk;
	HWND hRet ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		hRet = (*(HWND (*)(HWND))(pci->ppfnMethods[MID_GetParent]))(hwnd);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_ClientToScreen(HWND hwnd, LPPOINT lpPoint) {
	CALLSTACK cstk;
	BOOL bRet ;
    const CINFO *pci;
	MapArgPtr(lpPoint);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND, LPPOINT))(pci->ppfnMethods[MID_ClientToScreen]))(hwnd,lpPoint);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

LRESULT SC_DefWindowProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CALLSTACK cstk;
	LRESULT lRet ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		lRet = (*(LRESULT (*)(HWND, UINT, WPARAM, LPARAM))(pci->ppfnMethods[MID_DefWindowProcW]))(hwnd,msg,wParam,lParam);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		lRet = 0;
	}
	SwitchBack();
	return lRet;
}

BOOL SC_GetClipCursor(LPRECT lpRect) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(lpRect);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(LPRECT))(pci->ppfnMethods[MID_GetClipCursor]))(lpRect);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

HDC SC_GetDC (HWND hwnd) {
	CALLSTACK cstk;
	HDC hRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		hRet = (*(HDC (*)(HWND))(pci->ppfnMethods[MID_GetDC]))(hwnd);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

HWND SC_GetFocus() {
	CALLSTACK cstk;
	HWND hRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		hRet = (*(HWND (*)())(pci->ppfnMethods[MID_GetFocus]))();
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_GetMessageW(PMSG pMsgr, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(pMsgr);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(PMSG,HWND,UINT,UINT))(pci->ppfnMethods[MID_GetMessageW]))(pMsgr,hwnd,wMsgFilterMin,wMsgFilterMax);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

HWND SC_GetWindow(HWND hwnd, UINT uCmd) {
	CALLSTACK cstk;
	HWND hRet;
    const CINFO *pci;
	pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		hRet = (*(HWND (*)(HWND,UINT))(pci->ppfnMethods[MID_GetWindow]))(hwnd,uCmd);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_PeekMessageW(PMSG pMsg, HWND hWnd, UINT wMsgFilterMin,UINT wMsgFilterMax, UINT wRemoveMsg) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(pMsg);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(PMSG,HWND,UINT,UINT,UINT))(pci->ppfnMethods[MID_PeekMessageW]))(pMsg,hWnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_ReleaseDC(HWND hwnd, HDC hdc) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND,HDC))(pci->ppfnMethods[MID_ReleaseDC]))(hwnd,hdc);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

LRESULT SC_SendMessageW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	CALLSTACK cstk;
	LRESULT lRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		lRet = (*(LRESULT (*)(HWND,UINT,WPARAM,LPARAM))(pci->ppfnMethods[MID_SendMessageW]))(hwnd,Msg,wParam,lParam);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		lRet = 0;
	}
	SwitchBack();
	return lRet;
}

int SC_SetScrollInfo(HWND hwnd, int fnBar, LPCSCROLLINFO lpsi, BOOL fRedraw) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
	MapArgPtr(lpsi);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		iRet = (*(int (*)(HWND,int,LPCSCROLLINFO,BOOL))(pci->ppfnMethods[MID_SetScrollInfo]))(hwnd,fnBar,lpsi,fRedraw);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

LONG SC_SetWindowLongW(HWND hwnd, int nIndex, LONG lNewLong) {
	CALLSTACK cstk;
	LONG lRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		lRet = (*(LONG (*)(HWND,int,LONG))(pci->ppfnMethods[MID_SetWindowLongW]))(hwnd,nIndex,lNewLong);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		lRet = 0;
	}
	SwitchBack();
	return lRet;
}

BOOL SC_SetWindowPos(HWND hwnd, HWND hwndInsertAfter, int x, int y, int dx,int dy, UINT fuFlags) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND,HWND,int,int,int,int,UINT))(pci->ppfnMethods[MID_SetWindowPos]))(hwnd,hwndInsertAfter,x,y,dx,dy,fuFlags);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

HBRUSH SC_CreateSolidBrush(COLORREF crColor) {
	CALLSTACK cstk;
	HBRUSH hRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		hRet = (*(HBRUSH (*)(COLORREF))(pci->ppfnMethods[CREATESOLIDBRUSH]))(crColor);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_DeleteMenu(HMENU hmenu, UINT uPosition, UINT uFlags) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HMENU, UINT, UINT))(pci->ppfnMethods[MID_DeleteMenu]))(hmenu,uPosition,uFlags);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_DeleteObject(HGDIOBJ hObject) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		bRet = (*(BOOL (*)(HGDIOBJ))(pci->ppfnMethods[DELETEOBJECT]))(hObject);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

int SC_DrawTextW(HDC hdc,LPCWSTR lpszStr,int cchStr,RECT *lprc,UINT wFormat) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
	MapArgPtr(lpszStr);
	MapArgPtr(lprc);
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		iRet = (*(int (*)(HDC,LPCWSTR,int,RECT *,UINT))(pci->ppfnMethods[DRAWTEXTW]))(hdc,lpszStr,cchStr,lprc,wFormat);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

BOOL SC_ExtTextOutW(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT *lprc, LPCWSTR lpszString, UINT cbCount, CONST INT *lpDx) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(lprc);
	MapArgPtr(lpszString);
	MapArgPtr(lpDx);
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		bRet = (*(BOOL (*)(HDC,int,int,UINT,CONST RECT *, LPCWSTR, UINT, CONST INT *))(pci->ppfnMethods[EXTTEXTOUTW]))(hdc,X,Y,fuOptions,lprc,lpszString,cbCount,lpDx);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

int SC_FillRect(HDC hdc,CONST RECT *lprc,HBRUSH hbr) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
	MapArgPtr(lprc);
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		iRet = (*(int (*)(HDC,CONST RECT *, HBRUSH))(pci->ppfnMethods[FILLRECT]))(hdc,lprc,hbr);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

SHORT SC_GetAsyncKeyState(INT vKey) {
	CALLSTACK cstk;
	SHORT sRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		sRet = (*(SHORT (*)(INT))(pci->ppfnMethods[MID_GetAsyncKeyState]))(vKey);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		sRet = 0;
	}
	SwitchBack();
	return sRet;
}

int SC_GetDlgCtrlID(HWND hWnd) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		iRet = (*(int (*)(HWND))(pci->ppfnMethods[MID_GetDlgCtrlID]))(hWnd);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

HGDIOBJ SC_GetStockObject(int fnObject) {
	CALLSTACK cstk;
	HGDIOBJ hRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		hRet = (*(HGDIOBJ (*)(int))(pci->ppfnMethods[GETSTOCKOBJECT]))(fnObject);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

int SC_GetSystemMetrics(int nIndex) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		iRet = (*(int (*)(int))(pci->ppfnMethods[MID_GetSystemMetrics]))(nIndex);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

ATOM SC_RegisterClassWStub(CONST WNDCLASSW *lpWndClass, LPCWSTR lpszClassName, HANDLE hprcWndProc) {
	CALLSTACK cstk;
	ATOM aRet;
    const CINFO *pci;
	MapArgPtr(lpWndClass);
	MapArgPtr(lpszClassName);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		aRet = (*(ATOM (*)(CONST WNDCLASSW *, LPCWSTR, HANDLE))(pci->ppfnMethods[MID_RegisterClassWStub]))(lpWndClass,lpszClassName,hprcWndProc);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		aRet = 0;
	}
	SwitchBack();
	return aRet;
}

UINT SC_RegisterClipboardFormatW(LPCWSTR lpszFormat) {
	CALLSTACK cstk;
	UINT uRet;
    const CINFO *pci;
	MapArgPtr(lpszFormat);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		uRet = (*(UINT (*)(LPCWSTR))(pci->ppfnMethods[MID_RegisterClipboardFormatW]))(lpszFormat);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		uRet = 0;
	}
	SwitchBack();
	return uRet;
}

int SC_SetBkMode(HDC hdc,int iBkMode) {
	CALLSTACK cstk;
	int iRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		iRet = (*(int (*)(HDC,int))(pci->ppfnMethods[SETBKMODE]))(hdc,iBkMode);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		iRet = 0;
	}
	SwitchBack();
	return iRet;
}

COLORREF SC_SetTextColor(HDC hdc,COLORREF crColor) {
	CALLSTACK cstk;
	COLORREF cRet ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		cRet = (*(COLORREF (*)(HDC,COLORREF))(pci->ppfnMethods[SETTEXTCOLOR]))(hdc,crColor);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		cRet = 0;
	}
	SwitchBack();
	return cRet;
}

BOOL SC_InvalidateRect(HWND hwnd, LPCRECT prc, BOOL fErase) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(prc);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND, LPCRECT, BOOL))(pci->ppfnMethods[MID_InvalidateRect]))(hwnd,prc,fErase);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_TransparentImage(HDC hdcDest, int nXDest, int nYDest, int nWidthDest, int nHeightDest,
		HANDLE hImgSrc, int nXSrc, int nYSrc, int nWidthSrc, int nHeightSrc, COLORREF crTransparentColor) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_GDI);
    try {
		bRet = (*(BOOL (*)(HDC,int,int,int,int,HANDLE,int,int,int,int,COLORREF))(pci->ppfnMethods[TRANSPARENTIMAGE]))
			(hdcDest, nXDest, nYDest, nWidthDest, nHeightDest, hImgSrc, nXSrc, nYSrc, nWidthSrc, nHeightSrc, crTransparentColor);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_IsDialogMessageW(HWND hDlg, LPMSG lpMsg) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(lpMsg);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND,LPMSG))(pci->ppfnMethods[MID_IsDialogMessageW]))(hDlg,lpMsg);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND,UINT,WPARAM,LPARAM))(pci->ppfnMethods[MID_PostMessageW]))(hWnd,Msg,wParam,lParam);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

BOOL SC_IsWindowVisible(HWND hWnd) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND))(pci->ppfnMethods[MID_IsWindowVisible]))(hWnd);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

SHORT SC_GetKeyState(int nVirtKey){
	CALLSTACK cstk;
	SHORT sRet;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		sRet = (*(SHORT (*)(int))(pci->ppfnMethods[MID_GetKeyState]))(nVirtKey);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		sRet = 0;
	}
	SwitchBack();
	return sRet;
}

HDC SC_BeginPaint(HWND hwnd, LPPAINTSTRUCT pps) {
	CALLSTACK cstk;
	HDC hRet;
    const CINFO *pci;
	MapArgPtr(pps);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		hRet = (*(HDC (*)(HWND,LPPAINTSTRUCT))(pci->ppfnMethods[MID_BeginPaint]))(hwnd,pps);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		hRet = 0;
	}
	SwitchBack();
	return hRet;
}

BOOL SC_EndPaint(HWND hwnd, LPPAINTSTRUCT pps) {
	CALLSTACK cstk;
	BOOL bRet;
    const CINFO *pci;
	MapArgPtr(pps);
    pci = SwitchToProc(&cstk,SH_WMGR);
    try {
		bRet = (*(BOOL (*)(HWND,LPPAINTSTRUCT))(pci->ppfnMethods[MID_EndPaint]))(hwnd,pps);
	} except(EXCEPTION_EXECUTE_HANDLER) {
		(*(void (*)(void))pGwesHandler)();
		bRet = 0;
	}
	SwitchBack();
	return bRet;
}

LONG SC_RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) {
	CALLSTACK cstk;
	LONG lRet ;
    const CINFO *pci;
    MapArgPtr(lpSubKey);
    MapArgPtr(phkResult);
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	lRet = (*(LONG (*)(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY))(pci->ppfnMethods[23]))(hKey,lpSubKey,ulOptions,samDesired,phkResult);
	SwitchBack();
	return lRet;
}

LONG SC_RegCloseKey(HKEY hKey) {
	CALLSTACK cstk;
	LONG lRet ;
    const CINFO *pci;
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	lRet = (*(LONG (*)(HKEY))(pci->ppfnMethods[17]))(hKey);
	SwitchBack();
	return lRet;
}

LONG SC_RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	CALLSTACK cstk;
	LONG lRet ;
    const CINFO *pci;
    MapArgPtr(lpValueName);
    MapArgPtr(lpReserved);
    MapArgPtr(lpType);
    MapArgPtr(lpData);
    MapArgPtr(lpcbData);
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	lRet = (*(LONG (*)(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD))(pci->ppfnMethods[25]))(hKey,lpValueName,lpReserved,lpType,lpData,lpcbData);
	SwitchBack();
	return lRet;
}

LONG SC_RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition) {
	CALLSTACK cstk;
	LONG lRet ;
    const CINFO *pci;
    MapArgPtr(lpSubKey);
    MapArgPtr(lpClass);
    MapArgPtr(lpSecurityAttributes);
    MapArgPtr(phkResult);
    MapArgPtr(lpdwDisposition);
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	lRet = (*(LONG (*)(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,PHKEY,LPDWORD))(pci->ppfnMethods[18]))(hKey,lpSubKey,Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition);
	SwitchBack();
	return lRet;
}

HANDLE SC_CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile){
	CALLSTACK cstk;
	HANDLE h;
    const CINFO *pci;
    MapArgPtr(lpFileName);
    MapArgPtr(lpSecurityAttributes);
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	h = (*(HANDLE (*)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE))(pci->ppfnMethods[9]))(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
 	SwitchBack();
	return h;
}

BOOL SC_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
	CALLSTACK cstk;
	BOOL b = FALSE;
    const CINFO *pci;
    MapArgPtr(lpBuffer);
    MapArgPtr(lpNumberOfBytesRead);
    MapArgPtr(lpOverlapped);
    if (!(pci = SwitchToProcHandle(&cstk,&hFile,HT_FILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			b = (*(BOOL (*)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED))(pci->ppfnMethods[2]))(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return b;
}

CEOID SC_CeWriteRecordProps(HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL *rgPropVal) {
	CALLSTACK cstk;
	CEOID oid = 0;
    const CINFO *pci;
    MapArgPtr(rgPropVal);
    if (!(pci = SwitchToProcHandle(&cstk,&hDbase,HT_DBFILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			oid = (*(CEOID (*)(HANDLE,CEOID,WORD,CEPROPVAL *))(pci->ppfnMethods[5]))(hDbase,oidRecord,cPropID,rgPropVal);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return oid;
}

BOOL SC_ReadFileWithSeek(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset) {
	CALLSTACK cstk;
	BOOL b = 0;
    const CINFO *pci;
    MapArgPtr(lpBuffer);
    MapArgPtr(lpNumberOfBytesRead);
    MapArgPtr(lpOverlapped);
    if (!(pci = SwitchToProcHandle(&cstk,&hFile,HT_FILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			b = (*(BOOL (*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED, DWORD, DWORD))(pci->ppfnMethods[12]))
				(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped, dwLowOffset, dwHighOffset);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return b;
}

BOOL SC_WriteFileWithSeek(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset) {
	CALLSTACK cstk;
	BOOL b = 0;
    const CINFO *pci;
    MapArgPtr(lpBuffer);
    MapArgPtr(lpNumberOfBytesWritten);
    MapArgPtr(lpOverlapped);
    if (!(pci = SwitchToProcHandle(&cstk,&hFile,HT_FILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			b = (*(BOOL (*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED, DWORD, DWORD))(pci->ppfnMethods[13]))
				(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped, dwLowOffset, dwHighOffset);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return b;
}

BOOL SC_CloseHandle(HANDLE hObj) {
	CALLSTACK cstk;
	BOOL b = FALSE;
    const CINFO *pci;
    if (!(pci = SwitchToProcHandleClosing(&cstk,&hObj,&b))) {
    	if (!b)
    		SetLastError(ERROR_INVALID_HANDLE);
	} else {
		__try {
			b = (*(BOOL (*)(HANDLE))(pci->ppfnMethods[0]))(hObj);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return b;
}

BOOL SC_DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
	CALLSTACK cstk;
	BOOL b = FALSE;
    const CINFO *pci;
    MapArgPtr(lpInBuf);
    MapArgPtr(lpOutBuf);
    MapArgPtr(lpBytesReturned);
    MapArgPtr(lpOverlapped);
    if (!(pci = SwitchToProcHandle(&cstk,&hDevice,HT_FILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			b = (*(BOOL (*)(HANDLE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPOVERLAPPED))(pci->ppfnMethods[11]))(hDevice,dwIoControlCode,lpInBuf,nInBufSize,lpOutBuf,nOutBufSize,lpBytesReturned,lpOverlapped);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return b;
}

HANDLE SC_OpenDatabaseEx(PCEGUID pguid, PCEOID poid, LPWSTR lpszName, CEPROPID propid, DWORD dwFlags, CENOTIFYREQUEST *pReq) {
	CALLSTACK cstk;
	HANDLE h;
    const CINFO *pci;
    MapArgPtr(pguid);
    MapArgPtr(poid);
    MapArgPtr(lpszName);
    MapArgPtr(pReq);
    pci = SwitchToProc(&cstk,SH_FILESYS_APIS);
	h = (*(HANDLE (*)(PCEGUID,PCEOID,LPWSTR,CEPROPID,DWORD,CENOTIFYREQUEST *))(pci->ppfnMethods[16]))(pguid,poid,lpszName,propid,dwFlags,pReq);
 	SwitchBack();
	return h;
}

CEOID SC_SeekDatabase(HANDLE hDatabase, DWORD dwSeekType, DWORD dwValue, LPDWORD lpdwIndex) {
	CALLSTACK cstk;
	CEOID oid = 0;
    const CINFO *pci;
    MapArgPtr(lpdwIndex);
    if (!(pci = SwitchToProcHandle(&cstk,&hDatabase,HT_DBFILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			oid = (*(CEOID (*)(HANDLE,DWORD,DWORD,LPDWORD))(pci->ppfnMethods[2]))(hDatabase,dwSeekType,dwValue,lpdwIndex);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return oid;
}

CEOID SC_ReadRecordPropsEx (HANDLE hDbase, DWORD dwFlags, LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, LPDWORD lpcbBuffer, HANDLE hHeap) {
	CALLSTACK cstk;
	CEOID oid = 0;
    const CINFO *pci;
    MapArgPtr(lpcPropID);
    MapArgPtr(rgPropID);
    MapArgPtr(lplpBuffer);
    MapArgPtr(lpcbBuffer);
    if (!(pci = SwitchToProcHandle(&cstk,&hDbase,HT_DBFILE)))
    	SetLastError(ERROR_INVALID_HANDLE);
	else {
		__try {
			oid = (*(CEOID (*)(HANDLE,DWORD,LPWORD,CEPROPID *,LPBYTE *,LPDWORD,HANDLE))(pci->ppfnMethods[4]))(hDbase,dwFlags,lpcPropID,rgPropID,lplpBuffer,lpcbBuffer,hHeap);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		    SetLastError(ERROR_INVALID_PARAMETER);
		}
		SwitchBack();
	}
	return oid;
}

LPBYTE SC_CreateLocaleView(BOOL bFirst) {
	LPVOID pvPhysical;
	DWORD cbOffset, cbSize;
	HANDLE hNLS, hNLSMap;
	LPBYTE lpNLSView;
	if (pvPhysical = MapUncompressedFile(L"wince.nls", &cbSize)) {
    	cbOffset = (DWORD)pvPhysical & (UserKInfo[KINX_PAGESIZE] - 1);
	    pvPhysical = (LPVOID)((DWORD)pvPhysical - cbOffset);
    	cbSize += cbOffset;
    	lpNLSView = VirtualAlloc(0, cbSize, MEM_RESERVE, PAGE_NOACCESS);
    	VirtualCopy(lpNLSView, pvPhysical, cbSize, PAGE_READONLY);
    	lpNLSView += cbOffset;
    } else {
    	if (bFirst) {
    		RETAILMSG(1,(L"Warning - using inefficient file mapping for wince.nls - check to make sure file is uncompressed\r\n"));
	    	hNLS = CreateFileForMapping(L"\\windows\\wince.nls",GENERIC_READ,0,0,OPEN_EXISTING,0,0);
	    	DEBUGCHK(hNLS != INVALID_HANDLE_VALUE);
		   	hNLSMap = CreateFileMapping(hNLS,0,PAGE_READONLY,0,0,L"NLSFILE");
	    } else {
		    hNLSMap = CreateFileMapping(INVALID_HANDLE_VALUE,0,PAGE_READONLY,0,0,L"NLSFILE");
	    }
		DEBUGCHK(hNLSMap);
	    lpNLSView = MapViewOfFile(hNLSMap,FILE_MAP_READ,0,0,0);
    	DEBUGCHK(lpNLSView);
    }
    return lpNLSView;
}

