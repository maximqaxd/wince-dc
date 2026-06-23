/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include <windows.h>
#include <coredll.h>
#include "heap.h"
#include <celogcoredll.h>

#ifdef MEMTRACKING
DWORD dwHeapMemType;
#endif

DWORD pagemask;
heap ProcessHeap;
CRITICAL_SECTION hcs;

BOOL WINAPI LMemInit(void){
	pagemask = UserKInfo[KINX_PAGESIZE] - 1;
    DEBUGMSG(DBGLMEM, (L"LMemInit\r\n"));
    InitializeCriticalSection(&hcs);
    ProcessHeap.pNextHeap	  = NULL;
    ProcessHeap.pGrowthHeap	  = NULL;
	ProcessHeap.pVallocs	  = NULL;
	ProcessHeap.flOptions     = HEAP_IS_PROC_HEAP;
    ProcessHeap.dwMaximumSize = CE_FIXED_HEAP_MAXSIZE;
    ProcessHeap.pMem          = NULL;
    ProcessHeap.pHigh         = NULL;
    ProcessHeap.pCur          = NULL;
    ProcessHeap.dwCommitted   = 0;
    ProcessHeap.dwFree        = 0;
    ProcessHeap.dwLastCompact = 0;
    ProcessHeap.dwMaxLeft     = 0;
#ifdef MEMTRACKING
    ProcessHeap.dwMemType     = RegisterTrackedItem(L"Local Memory");
    dwHeapMemType = RegisterTrackedItem(L"Heaps");
#endif

    CELOG_HeapCreate(HEAP_IS_PROC_HEAP, 0, CE_FIXED_HEAP_MAXSIZE, (HANDLE)&ProcessHeap);

    DEBUGMSG(DBGLMEM, (L"   LMemInit ==> TRUE\r\n"));
    return TRUE;
}

#if defined(MEMTRACKING) && defined(MEM_ACCOUNT)
void TrackerCallback (DWORD dwFlags, DWORD dwType, HANDLE hItem, DWORD dwProc, BOOL fDeleted,
	DWORD dwSize, DWORD dw1, DWORD dw2) {
	PBYTE p = (PBYTE)hItem;
	if (!fDeleted && *(int*)p == 0x8482906) {
		NKDbgPrintfW (L"%a %i", p+4, *(int*)(p+20));
		if (!memcmp (p+4, "String", 7))
			NKDbgPrintfW (L"  :\"%s\"", p+24);
	}
	NKDbgPrintfW (L"\r\n");
}
#define TCB TrackerCallback
#else
#define TCB NULL
#endif

/*
	@doc BOTH EXTERNAL
	
	@func HLOCAL | LocalAlloc | Allocates a block of memory from the local heap
    @parm UINT | uFlags | flags for alloc operation
    @parm UINT | uBytes | desired size of block

	@comm Only fixed memory is supported, thus LMEM_MOVEABLE is invalid.
*/
HLOCAL WINAPI LocalAlloc(UINT uFlags, UINT uBytes) {
    LPVOID p;
    DEBUGMSG(DBGLMEM, (L"LocalAlloc %8.8lx %8.8lx\r\n",uBytes,uFlags));
    if(uFlags & ~LMEM_VALID_FLAGS) {
	    DEBUGMSG(DBGLMEM, (L"   LocalAlloc %8.8lx %8.8lx ==> %8.8lx\r\n",uBytes,uFlags,0));
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
    }
    p = HeapAlloc(&ProcessHeap,(uFlags & LMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0,uBytes);
	DEBUGMSG(DBGLMEM, (L"   LocalAlloc %8.8lx %8.8lx ==> %8.8lx [%8.8lx]\r\n",uBytes,uFlags,p,FixedHeapSize(&ProcessHeap,p)));
    return (HLOCAL)p;
}

/*
	@doc BOTH EXTERNAL
	
	@func HLOCAL | LocalRealloc | Realloc's a LocalAlloc'ed block of memory
    @parm HLOCAL | hMem | handle of local memory object to realloc
    @parm UINT | uBytes | desired new size of block
    @parm UINT | uFlags | flags for realloc operation

	@comm Follows the Win32 reference description without restrictions
    or modifications. 
*/
HLOCAL WINAPI LocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags) {
	LPVOID retval;
	UINT newflags;
    DEBUGMSG(DBGLMEM, (L"LocalRealloc %8.8lx %8.8lx %8.8lx\r\n",hMem,uBytes,uFlags));
    if((uFlags & ~(LMEM_VALID_FLAGS | LMEM_MODIFY )) ||
		((uFlags & LMEM_DISCARDABLE) && !(uFlags & LMEM_MODIFY))) {
	    DEBUGMSG(DBGLMEM, (L"   LocalRealloc %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n",hMem,uBytes,uFlags,0));
		SetLastError(ERROR_INVALID_PARAMETER);
		return(NULL);
    }
    newflags = ((uFlags & LMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0) | ((uFlags & LMEM_MOVEABLE) ? 0 : HEAP_REALLOC_IN_PLACE_ONLY);
	retval = HeapReAlloc(&ProcessHeap, newflags, hMem, uBytes);    
    DEBUGMSG(DBGLMEM, (L"   LocalRealloc %8.8lx %8.8lx %8.8lx ==> %8.8lx [%8.8lx]\r\n",hMem,uBytes,uFlags,retval,FixedHeapSize(&ProcessHeap,retval)));
    return (HLOCAL)retval;
}

/*
	@doc BOTH EXTERNAL
	
	@func UINT | LocalSize | Returns the size of a LocalAlloc'ed block of memory
    @parm HLOCAL | hMem | handle of local memory object to get size of

	@comm Follows the Win32 reference description without restrictions 
    or modifications. 
*/
UINT WINAPI LocalSize(HLOCAL hMem) {
	UINT retval;
    DEBUGMSG(DBGLMEM, (L"LocalSize %8.8lx\r\n",hMem));
    if ((retval = HeapSize(&ProcessHeap,0,hMem)) == (DWORD)-1)
    	retval = 0;
    DEBUGMSG(DBGLMEM, (L"   LocalSize %8.8lx ==> %8.8lx\r\n",hMem,retval));
	return retval;
}

/*
	@doc BOTH EXTERNAL
	
	@func HLOCAL | LocalFree | Frees a LocalAlloc'ed block of memory
    @parm HLOCAL | hMem | handle of local memory object to free

	@comm Follows the Win32 reference description without restrictions 
    or modifications. 
*/
HLOCAL WINAPI LocalFree(HLOCAL hMem) {
	HLOCAL retval;
    DEBUGMSG(DBGLMEM, (L"LocalFree %8.8lx\r\n",hMem));
	if (retval = (!hMem || HeapFree(&ProcessHeap,0,(LPVOID)hMem)) ? NULL : hMem)
		SetLastError(ERROR_INVALID_PARAMETER);
    DEBUGMSG(DBGLMEM, (L"   LocalFree %8.8lx ==> %8.8lx\r\n",hMem,retval));
    return retval;
}

/*
    @doc BOTH EXTERNAL
	
	@func HANDLE | HeapCreate    | Creates a new local heap
    @parm DWORD  | flOptions     | no flags supported
    @parm DWORD  | dwInitialSize | Initial committed heap size
    @parm DWORD  | dwMaximumSize | Maximum heap size
*/
HANDLE WINAPI HeapCreate(DWORD flOptions, DWORD dwInitialSize, DWORD dwMaximumSize) {
    pheap pHeap;
#ifdef MEMTRACKING
	WCHAR Name[14];
#endif
    DEBUGMSG(DBGLMEM, (L"HeapCreate %8.8lx %8.8lx %8.8lx\r\n", flOptions, dwInitialSize, dwMaximumSize));
    if (flOptions & ~HEAP_NO_SERIALIZE) {
	    DEBUGMSG(DBGLMEM, (L"   HeapCreate: Invalid parameter\r\n"));
	    SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
    }
    EnterCriticalSection(&hcs);
    pHeap = (pheap)FixedHeapAlloc(&ProcessHeap,sizeof(heap),0);
    LeaveCriticalSection(&hcs);
    if (!pHeap) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
    }
#ifdef MEMTRACKING
    AddTrackedItem(dwHeapMemType,pHeap,NULL,GetCurrentProcessId(),sizeof(heap),0,0);
#endif
    pHeap->flOptions     = 0;
	pHeap->pVallocs		 = 0;
    pHeap->dwMaximumSize = CE_FIXED_HEAP_MAXSIZE;
    pHeap->pMem          = NULL;
    pHeap->pHigh         = NULL;
    pHeap->pCur          = NULL;
    pHeap->dwCommitted   = 0;
    pHeap->dwFree        = 0;
    pHeap->dwLastCompact = 0;
	pHeap->dwMaxLeft     = 0;
#ifdef MEMTRACKING
	swprintf(Name, L"Heap %08X", pHeap);
	pHeap->dwMemType     = RegisterTrackedItem(Name);
#endif
    EnterCriticalSection(&hcs);
    pHeap->pGrowthHeap = NULL;
    pHeap->pNextHeap = ProcessHeap.pNextHeap;
    ProcessHeap.pNextHeap = pHeap;
    LeaveCriticalSection(&hcs);
    DEBUGMSG(DBGLMEM, (L"   HeapCreate %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", flOptions, dwInitialSize, dwMaximumSize, pHeap));
    CELOG_HeapCreate(flOptions, dwInitialSize, dwMaximumSize, (HANDLE)pHeap);
    return (HANDLE)pHeap;
}

/*
    @doc BOTH EXTERNAL
	
	@func LPVOID | HeapReAlloc | Realloc's a HeapAlloc'ed block of memory
	@parm HANDLE | hHeap     | Handle returned from CreateHeap()
    @parm DWORD  | dwFlags   | HEAP_NO_SERIALIZE, HEAP_REALLOC_IN_PLACE_ONLY and HEAP_ZERO_MEM supported
    @parm LPVOID | lpMem     | pointer to memory to realloc
    @parm DWORD  | dwBytes   | Desired size of block
*/
LPVOID WINAPI HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, DWORD dwBytes) {
    LPVOID p;
    DEBUGMSG(DBGLMEM, (L"HeapReAlloc %8.8lx %8.8lx %8.8lx %8.8lx\r\n", hHeap,dwFlags,lpMem,dwBytes));
    if(dwFlags & ~(HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_REALLOC_IN_PLACE_ONLY | HEAP_ZERO_MEMORY)) {
	    DEBUGMSG(DBGLMEM, (L"   HeapReAlloc %8.8lx %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,lpMem,dwBytes,0));
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
    }
    if(dwFlags & HEAP_GENERATE_EXCEPTIONS) {
	    DEBUGMSG(DBGLMEM, (L"   HeapReAlloc %8.8lx %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,lpMem,dwBytes,0));
		SetLastError(ERROR_NOT_SUPPORTED);
		return NULL;
    }
    if(!hHeap || !lpMem) {
		SetLastError(ERROR_INVALID_PARAMETER);
	    DEBUGMSG(DBGLMEM, (L"   HeapReAlloc %8.8lx %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,lpMem,dwBytes,0));
		return NULL;
    }
    EnterCriticalSection(&hcs);
    if (!(p=FixedHeapReAlloc((pheap)hHeap,lpMem,dwBytes,(dwFlags & HEAP_ZERO_MEMORY?1:0),
		(dwFlags & HEAP_REALLOC_IN_PLACE_ONLY ? 0 : 1))))
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
#ifdef MEMTRACKING
    if (p) {
		DeleteTrackedItem(((pheap)hHeap)->dwMemType, lpMem);
		AddTrackedItem(((pheap)hHeap)->dwMemType, p, TCB, GetCurrentProcessId(), 
	       FixedHeapSize((pheap)hHeap,p), 0, 0);
    }
#endif
    DEBUGMSG(DBGLMEM, (L"   HeapReAlloc %8.8lx %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,lpMem,dwBytes,p));
    DEBUGCHK(!p || ((FixedHeapSize((pheap)hHeap,p) >= dwBytes) && (FixedHeapSize((pheap)hHeap,p) != (DWORD)-1)));
    LeaveCriticalSection(&hcs);
    CELOG_HeapRealloc(hHeap, dwFlags, dwBytes, (DWORD)lpMem, (DWORD)p);
    return p;
}

/*
    @doc BOTH EXTERNAL
	
	@func LPVOID | HeapAlloc | Allocates a block of memory from specified heap
    @parm HANDLE | hHeap     | Handle returned from CreateHeap()
    @parm DWORD  | dwFlags   | HEAP_NO_SERIALIZE and HEAP_ZERO_MEM supported
    @parm DWORD  | dwBytes   | Desired size of block
*/
LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes) {
    LPVOID p;
    DEBUGMSG(DBGLMEM, (L"HeapAlloc %8.8lx %8.8lx %8.8lx\r\n", hHeap,dwFlags,dwBytes));
    if((dwFlags & ~(HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY)) || !hHeap) {
	    DEBUGMSG(DBGLMEM, (L"   HeapAlloc %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,dwBytes,0));
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
    }
    EnterCriticalSection(&hcs);
	if (!(p=FixedHeapAlloc((pheap)hHeap,dwBytes,(dwFlags & HEAP_ZERO_MEMORY?1:0))))
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
#ifdef MEMTRACKING
    if (p)
		AddTrackedItem(((pheap)hHeap)->dwMemType,p,TCB,GetCurrentProcessId(),FixedHeapSize((pheap)hHeap,p),0,0);
#endif
    CELOG_HeapAlloc(hHeap, dwFlags, dwBytes, (DWORD) p);
    DEBUGMSG(DBGLMEM, (L"   HeapAlloc %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,dwBytes,p));
    DEBUGCHK(!p || ((FixedHeapSize((pheap)hHeap,p) >= dwBytes) && (FixedHeapSize((pheap)hHeap,p) != (DWORD)-1)));
    LeaveCriticalSection(&hcs);
    return p;    
}

/*
    @doc BOTH EXTERNAL
	
	@func BOOL   | HeapFree | Frees a HeapAlloc'ed block of memory
    @parm HANDLE | hHeap    | Handle returned from CreateHeap()
    @parm DWORD  | dwFlags  | HEAP_NO_SERIALIZE only
    @parm LPVOID | lpMem    | Pointer to memory block
*/
BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) {
    BOOL retval;
    DEBUGMSG(DBGLMEM, (L"HeapFree %8.8lx %8.8lx %8.8lx\r\n",hHeap,dwFlags,lpMem));
    if ((dwFlags & ~HEAP_NO_SERIALIZE) || !hHeap || !lpMem) {
		DEBUGMSG(DBGLMEM, (L"   HeapFree %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n", hHeap,dwFlags,lpMem,0));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
    }
    EnterCriticalSection(&hcs);
    CELOG_HeapFree(hHeap, dwFlags, (DWORD)lpMem);
    retval = FixedHeapFree((pheap)hHeap,lpMem);
#ifdef MEMTRACKING
    if (retval)
		DeleteTrackedItem(((pheap)hHeap)->dwMemType, lpMem);
#endif
    DEBUGMSG(DBGLMEM, (L"HeapFree %8.8lx %8.8lx %8.8lx ==> %8.8lx\r\n",hHeap,dwFlags,lpMem,retval));
    LeaveCriticalSection(&hcs);
    return retval;
}

/*
	@doc BOTH EXTERNAL
	
	@func DWORD | HeapSize | Returns the size of a HeapAlloc'ed block of memory
    @parm HANDLE | hHeap | Heap from which memory was alloc'ed
    @parm DWORD | dwFlags | can be HEAP_NO_SERIALIZE
    @parm LPVOID | lpMem | handle of local memory object to get size of

	@comm Follows the Win32 reference description without restrictions 
    or modifications. 
*/

DWORD WINAPI HeapSize(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) {
	UINT retval;
    DEBUGMSG(DBGLMEM, (L"HeapSize %8.8lx\r\n",lpMem));
    if (dwFlags & ~HEAP_NO_SERIALIZE) {
	    DEBUGMSG(DBGLMEM, (L"   HeapSize %8.8lx: Invalid parameter\r\n"));
	    SetLastError(ERROR_INVALID_PARAMETER);
		return (DWORD)-1;
    }
    EnterCriticalSection(&hcs);
    retval = FixedHeapSize(hHeap,lpMem);
    DEBUGMSG(DBGLMEM, (L"   HeapSize %8.8lx ==> %8.8lx\r\n",lpMem,retval));
    LeaveCriticalSection(&hcs);
	return retval;
}

/*
    @doc BOTH EXTERNAL
	
	@func BOOL   | HeapDestroy | Destroys a heap and releases its memory
    @parm HANDLE | hHeap       | Handle returned from CreateHeap()
*/
BOOL WINAPI HeapDestroy(HANDLE hHeap) {
    pheap pHeap = (heap *)hHeap;
    pheap pHeapTrav, pHeapNext;
    vaheapalloc *pva, *pvanext;
    LPBYTE pMem;
#ifdef MEMTRACKING
    LPVOID ptr;
#endif
    DEBUGMSG(DBGLMEM, (L"HeapDestroy %8.8lx\r\n",hHeap));
    if(!hHeap) {
		SetLastError(ERROR_INVALID_PARAMETER);
		DEBUGMSG(DBGLMEM, (L"   HeapDestroy %8.8lx ==> %8.8lx\r\n", hHeap,0));
		return FALSE;
    }
    CELOG_HeapDestroy(hHeap);
    EnterCriticalSection(&hcs);
    pva = pHeap->pVallocs;
    while (pva) {
    	pvanext = pva->pnext;
#if MEMTRACKING
		DeleteTrackedItem(pHeap->dwMemType, pva->pBase);
#endif
    	VirtualFree(pva->pBase,0,MEM_RELEASE);
    	FixedHeapFree(hHeap,pva);
    	pva = pvanext;
    }
    if (ProcessHeap.pNextHeap == pHeap)
   		ProcessHeap.pNextHeap = pHeap->pNextHeap;
    else {
		pHeapTrav = ProcessHeap.pNextHeap;
		while (pHeapTrav->pNextHeap != pHeap)
			pHeapTrav = pHeapTrav->pNextHeap;
		pHeapTrav->pNextHeap = pHeap->pNextHeap;
    }
    pHeapTrav = pHeap;
    while (pHeapTrav) {
    	pHeapNext = pHeapTrav->pGrowthHeap;
	    if (pHeapTrav->pMem) {
#ifdef MEMTRACKING
		    ptr = pHeapTrav->pMem;
    		do {
    			if (HEAPTOPTR(ptr) != pHeapTrav) { // don't free heap itself
					if (!IsSigValid(pHeapTrav,ptr,1))
					    DEBUGCHK(0);
					if (ISINUSE(ptr))
					    DeleteTrackedItem(pHeapTrav->dwMemType, HEAPTOPTR(ptr));
				}
    		} while ((ptr = NEXTBLOCK(pHeapTrav, ptr)) != pHeapTrav->pMem);
#endif
			pMem = pHeapTrav->pMem;
		    VirtualFree(pMem,pHeap->dwMaximumSize,MEM_DECOMMIT);
		   	VirtualFree(pMem, 0, MEM_RELEASE);
		}
		pHeapTrav = pHeapNext;
    }
	LocalFree(pHeap);
#ifdef MEMTRACKING
    DeleteTrackedItem(dwHeapMemType, hHeap);
#endif
    LeaveCriticalSection(&hcs);
    return TRUE;
}

void CompactAllHeaps(void) {
	pheap pHeap, pHeap2;
	EnterCriticalSection(&hcs);
	for (pHeap = &ProcessHeap; pHeap; pHeap = pHeap->pNextHeap)
		for (pHeap2 = pHeap; pHeap2; pHeap2 = pHeap2->pGrowthHeap)
			FixedHeapCompact(pHeap2);
	LeaveCriticalSection(&hcs);
}

/*
    @doc BOTH EXTERNAL
	
	@func HANDLE   | GetProcessHeap | Obtains a handle to the heap of the 
	calling process
*/
HANDLE WINAPI GetProcessHeap(VOID) {
	return (HANDLE)&ProcessHeap;
}

BOOL FillInOneHeap(THSNAP *pSnap, heap *pBaseHeap, TH32HEAPENTRY **ppNext) {
	heap *pHeap;
	LPBYTE ptr;
	vaheapalloc *vaptr;
	for (pHeap = pBaseHeap; pHeap; pHeap = pHeap->pGrowthHeap) {
		if (ptr = pHeap->pMem) {
			do {
				if (!(*ppNext = (TH32HEAPENTRY *)THGrow(pSnap,sizeof(TH32HEAPENTRY))))
					return FALSE;
				(*ppNext)->heapentry.dwSize = sizeof(TH32HEAPENTRY);
				(*ppNext)->heapentry.hHandle = (HANDLE)ptr;
				(*ppNext)->heapentry.dwAddress = (DWORD)ptr;
				(*ppNext)->heapentry.dwBlockSize = HDRSIZE + ITEMSIZE(ptr);
				(*ppNext)->heapentry.dwFlags = ISFREE(ptr) ? LF32_FREE : ISDEAD(ptr) ? LF32_FREE | LF32_DECOMMIT : LF32_FIXED;
				(*ppNext)->heapentry.dwLockCount = ISINUSE(ptr) ? 1 : 0;
				(*ppNext)->heapentry.dwResvd = 0;
				(*ppNext)->heapentry.th32ProcessID = GetCurrentProcessId();
				(*ppNext)->heapentry.th32HeapID = (DWORD)pBaseHeap;
				ppNext = &(*ppNext)->pNext;
			} while ((ptr = NEXTBLOCK(pHeap, ptr)) != pHeap->pMem);
		}
	}
	for (vaptr = pBaseHeap->pVallocs; vaptr; vaptr = vaptr->pnext) {
		if (!(*ppNext = (TH32HEAPENTRY *)THGrow(pSnap,sizeof(TH32HEAPENTRY))))
			return FALSE;
		(*ppNext)->heapentry.dwSize = sizeof(TH32HEAPENTRY);
		(*ppNext)->heapentry.hHandle = (HANDLE)vaptr->pBase;
		(*ppNext)->heapentry.dwAddress = (DWORD)vaptr->pBase;
		(*ppNext)->heapentry.dwBlockSize = vaptr->dwSize;
		(*ppNext)->heapentry.dwFlags = LF32_FIXED | LF32_BIGBLOCK;
		(*ppNext)->heapentry.dwLockCount = 1;
		(*ppNext)->heapentry.dwResvd = 0;
		(*ppNext)->heapentry.th32ProcessID = GetCurrentProcessId();
		(*ppNext)->heapentry.th32HeapID = (DWORD)pBaseHeap;
		ppNext = &(*ppNext)->pNext;
	}
	*ppNext = 0;
	return TRUE;
}

BOOL GetHeapSnapshot(THSNAP *pSnap, BOOL bMainOnly, LPVOID *pDataPtr) {
	heap *pHeap;
	// Must set permissions since we're in a callback and it's possible pSnap is in an intermediate process
	// context we can no longer see.  We don't need to restore the old permissions, since when we return
	// from this function, they'll be restored by the system
	SetProcPermissions((DWORD)-1);
	EnterCriticalSection(&hcs);
	if (bMainOnly) {
		if (!FillInOneHeap(pSnap,&ProcessHeap,(TH32HEAPENTRY **)pDataPtr)) {
			LeaveCriticalSection(&hcs);
			return FALSE;
		}
	} else {
		TH32HEAPLIST **ppNext;
		ppNext = (TH32HEAPLIST **)pDataPtr;
		for (pHeap = &ProcessHeap; pHeap; pHeap = pHeap->pNextHeap) {
			if (!(*ppNext = (TH32HEAPLIST *)THGrow(pSnap,sizeof(TH32HEAPLIST)))) {
				LeaveCriticalSection(&hcs);
				return FALSE;
			}
			(*ppNext)->heaplist.dwSize = sizeof(HEAPLIST32);
			(*ppNext)->heaplist.th32ProcessID = GetCurrentProcessId();
			(*ppNext)->heaplist.th32HeapID = (DWORD)pHeap;
			(*ppNext)->heaplist.dwFlags = pHeap == &ProcessHeap ? HF32_DEFAULT : 0;
			if (!FillInOneHeap(pSnap,pHeap,&(*ppNext)->pHeapEntry)) {
				LeaveCriticalSection(&hcs);
				return FALSE;
			}
			ppNext = &(*ppNext)->pNext;
		}
		*ppNext = 0;
	}
	LeaveCriticalSection(&hcs);
	return TRUE;
}

