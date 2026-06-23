/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*+
    objdisp.c - object call dispatch routines.

    This file contains the routines for dispatching method calls to objects
    implemented by protected system libraries, direct kernel functions,
    and kernel mode threads.
*/

#include "kernel.h"
#include "xtime.h"
#include "winsock.h"

#ifdef XTIME							//XTIME
void SyscallTime(DWORD);				//XTIME record exception elapsed time
#endif									//XTIME

void SC_CloseAPISet(HANDLE hSet);
BOOL SC_RegisterAPISet(HANDLE hSet, DWORD dwSetID);
LPVOID SC_VerifyAPIHandle(HANDLE hSet, HANDLE h);
void GoToUserTime(void);
void GoToKernTime(void);

extern CRITICAL_SECTION LLcs;

const PFNVOID APISetMethods[] = {
    (PFNVOID)SC_CloseAPISet,
    (PFNVOID)SC_NotSupported,
    (PFNVOID)SC_RegisterAPISet,
    (PFNVOID)SC_CreateAPIHandle,
    (PFNVOID)SC_VerifyAPIHandle,
};

const CINFO cinfAPISet = {
	"APIS",
	DISPATCH_KERNEL_PSL,
	HT_APISET,
	sizeof(APISetMethods)/sizeof(APISetMethods[0]),
	APISetMethods,
};

// Head of double linked list of active handles in the system.
DList HandleList = { &HandleList, &HandleList };

// Serial # for handle values.
unsigned int HandleSerial;

// Array of pointers to system api sets.
const CINFO *SystemAPISets[NUM_SYSTEM_SETS];

// Find a process index from an access lock.
int IndexFromLock(ulong lock)
{
    ulong mask;
    int inxRet;
    int inx;

    mask = 0x0000FFFFul;
    for (inxRet = 0, inx = 16 ; inx ; inx >>= 1, mask >>= inx) {
        if ((lock & mask) == 0) {
            inxRet += inx;
            lock >>= inx;
        }
    }
    return inxRet;
}


// Insert an item into a double linked list
void AddToDList(DList *head, DList *item)
{
    item->back = head;
    item->fwd = head->fwd;
    head->fwd->back = item;
    head->fwd = item;
}

// Remove an item from a double linked list
void RemoveDList(DList *item)
{
    item->fwd->back = item->back;
    item->back->fwd = item->fwd;
}

// Convert a HANDLE to an HDATA pointer.
//
// HandleToPointer() is a function which can be used by other kernel modules.
//
//  h2p() is an private macro used to speed up the internal processing of
// ObjectCall() and the various GetXXX/SetXXX functions.

#define h2p(h, phdRet) \
    do { \
        if ((ulong)h < NUM_SYS_HANDLES+SYS_HANDLE_BASE && (ulong)h >= SYS_HANDLE_BASE) \
             h = KData.ahSys[(uint)h-SYS_HANDLE_BASE]; \
        if (h) {            \
            phdRet = (PHDATA)(((ulong)h & HANDLE_ADDRESS_MASK) + KData.handleBase); \
            if (!IsValidKPtr(phdRet) || phdRet->hValue != h) \
                phdRet = 0; \
        } else              \
            phdRet = 0;     \
    } while (0)

PHDATA HandleToPointer(HANDLE h)
{
    PHDATA phdRet;

    h2p(h, phdRet);
    return phdRet;
}

// Return the next handle to close.
//
// The active handle list is scanned for a handle which is referenced by the process
// whose procnum is (id). (hLast) is the previous return value from this
// routine.  If a handle is found, its reference count for the process is forced
// to 1 so that it will be closed by a single call to CloseHandle(). To speed up the
// iterative process of scanning the entire table, a pointer to the next handle in
// the list is retained. The next call will use that value if the list is unchanged,
// except for the possible freeing of the handle previously returned. If the list
// has changed, the scan will restart at the beginning of the list.
//
//NOTE: This function is called via KCall().

PHDATA PhdPrevReturn, PhdNext;

PHDATA NextCloseHandle(PHDATA *phdLast, int ixProc) {
    PHDATA phd;
	KCALLPROFON(7);
	if (!PhdPrevReturn || !PhdNext || (*phdLast != PhdNext))
    	phd = (PHDATA)HandleList.fwd;
    else
    	phd = (PHDATA)PhdNext->linkage.fwd;
    DEBUGCHK(phd != 0);
    if (&phd->linkage == &HandleList) {
        PhdPrevReturn = 0;
		KCALLPROFOFF(7);
        return 0;
    }
    PhdPrevReturn = phd;
    if ((phd->lock & (1<<ixProc)) && (phd->hValue != hCurThread) && (phd->hValue != hCurProc)) {
        // Force handle reference count for this process to one.
        if (phd->ref.count < 0x10000)
            phd->ref.count = 1;
        else
            phd->ref.pFr->usRefs[ixProc] = 1;
    } else {
	    PhdNext = *phdLast = phd;
        phd = (PHDATA)((ulong)phd | 1);
	}
	KCALLPROFOFF(7);
    return phd;
}

void SC_CloseAllHandles(void) {
    PHDATA phd, next;
    int ixProc;
	next = 0;
	ixProc = pCurProc->procnum;
    while (phd = (PHDATA)KCall((PKFN)NextCloseHandle, &next, ixProc))
    	if (!((DWORD)phd & 1))
    		CloseHandle(phd->hValue);
}

// Non-preemtible handle setup for AllocHandle().
BOOL SetupHandle(PHDATA phdNew, PPROCESS pprc) {
	KCALLPROFON(8);
	phdNew->hValue = (HANDLE)(((ulong)phdNew & HANDLE_ADDRESS_MASK) | 2 | ((ulong)phdNew->hValue & 0xE0000000));
	LockFromKey(&phdNew->lock, &pprc->aky);
    // Zap PhdPrevReturn so to force a restart the next time
    // NextCloseHandle() is called.
    PhdPrevReturn = 0; 
    AddToDList(&HandleList, &phdNew->linkage);
	KCALLPROFOFF(8);
    return TRUE;
}

// Non-preemtible handle cleanup for FreeHandle().
PHDATA ZapHandle(HANDLE h) {
    PHDATA phd;
	KCALLPROFON(9);
    h2p(h, phd);
    if (phd) {
        phd->lock = 0;
        // make it so that the handle no longer matches existing handles given out
        DEBUGCHK(((DWORD)phd->hValue & 0x1fffffff) == (((ulong)phd & HANDLE_ADDRESS_MASK) | 2));
		phd->hValue = (HANDLE)((DWORD)phd->hValue+0x20000000);
        // If freeing a handle other than the last one returned by
        // NextCloseHandle(), then zap PhdPrevReturn so to force
        // a restart the next time NextCloseHandle() is called.
        if (phd != PhdPrevReturn)
            PhdPrevReturn = 0; 
        if (phd == PhdNext)
        	PhdNext = 0;
        RemoveDList(&phd->linkage);
    }
	KCALLPROFOFF(9);
    return phd;
}

// Create a new handle.
HANDLE AllocHandle(const CINFO *pci, PVOID pvObj, PPROCESS pprc)
{
    PHDATA phdNew;

    DEBUGMSG(ZONE_MEMORY, (TEXT("AllocHandle: pci=%8.8lx pvObj=%8.8lx pprc=%8.8lx\r\n"),
            pci, pvObj, pprc));
    if ((phdNew = AllocMem(HEAP_HDATA)) != 0) {
        phdNew->pci = pci;
        phdNew->pvObj = pvObj;
        phdNew->dwInfo = 0;
        phdNew->ref.count = 1;
        if (KCall((PKFN)SetupHandle, phdNew, pprc)) {
            DEBUGMSG(ZONE_MEMORY, (TEXT("AllocHandle: phd=%8.8lx hValue=%8.8lx\r\n"),
                    phdNew, phdNew->hValue));
            return phdNew->hValue;
        }
        DEBUGMSG(ZONE_MEMORY, (TEXT("AllocHandle: SetupHandle Failed!\r\n")));
        FreeMem(phdNew, HEAP_HDATA);
    } else
        DEBUGMSG(ZONE_MEMORY, (TEXT("AllocHandle: AllocMem Failed!\r\n")));
    return 0;
}


// Destroy a handle.
BOOL FreeHandle(HANDLE h)
{
    PHDATA phd;

    if ((phd = (PHDATA)KCall((PKFN)ZapHandle, h)) != 0) {
        if (phd->ref.count >= 0x10000)
            FreeMem(phd->ref.pFr, HEAP_FULLREF);
        FreeMem(phd, HEAP_HDATA);
        return TRUE;
    }
    return FALSE;
}

// Non-preemtible: copies single refcount to FULLREF.
BOOL CopyRefs(HANDLE h, FULLREF *pFr) {
    PHDATA phd;
    int inx;
    KCALLPROFON(12);
    h2p(h, phd);
    if (phd && phd->ref.count < 0x10000) {
        inx = IndexFromLock(phd->lock);
        pFr->usRefs[inx] = (ushort)phd->ref.count;
        phd->ref.pFr = pFr;
	    KCALLPROFOFF(12);
        return FALSE;
    }
    KCALLPROFOFF(12);
    return TRUE;
}

// Non-preemtible worker for IncRef().
int DoIncRef(HANDLE h, PPROCESS pprc) {
    PHDATA phd;
    ACCESSKEY aky;
    int inx;
    KCALLPROFON(11);
    aky = pprc->aky;
    inx = pprc->procnum;
    h2p(h, phd);
    if (phd && phd->lock != 0) {
        if (phd->ref.count < 0x10000) {               
            if (TestAccess(&phd->lock, &aky)) {
                ++phd->ref.count;
			    KCALLPROFOFF(11);
                return 1;
            }
		    KCALLPROFOFF(11);
            return 2;   // Tell IncRef to allocate a FULLREF.
        }
        // There is a FULLREF structure. Increment the entry for this
        // process and add it to the access key.
        AddAccess(&phd->lock, aky);
        ++phd->ref.pFr->usRefs[inx];
	    KCALLPROFOFF(11);
        return 1;
    }
    KCALLPROFOFF(11);
    return 0;
}

// Returns FALSE if handle not valid or refcnt==0.
BOOL IncRef(HANDLE h, PPROCESS pprc)
{
    FULLREF *pFr;
    int ret;

    while ((ret = KCall(DoIncRef, h, pprc)) == 2) {
        // Second process referencing handle. Must allocate
        // a FULLREF structure to track references from
        // multiple processes.
        if ((pFr = AllocMem(HEAP_FULLREF)) == 0)
            return FALSE;
        memset(pFr,0,sizeof(FULLREF));
        if (KCall(CopyRefs, h, pFr)) {
            // Another thread already allocated a FULLREF for this handle.
            FreeMem(pFr, HEAP_FULLREF);
        }
    }
    return ret;
}

// Non-preemtible worker for DecRef().
BOOL DoDecRef(HANDLE h, PPROCESS pprc, BOOL fAll) {
    PHDATA phd;
    ACCESSKEY aky;
    int inx;
    KCALLPROFON(30);
    aky = pprc->aky;
    inx = pprc->procnum;
    h2p(h, phd);
    if (phd && TestAccess(&phd->lock, &aky)) {
        if (phd->ref.count < 0x10000) {               
            if (fAll || phd->ref.count == 1) {
                phd->lock = 0;
                phd->ref.count = 0;
			    KCALLPROFOFF(30);
                return TRUE;
            }
            --phd->ref.count;
        } else {
            // There is a FULLREF structure. Decrement the entry for this
            // process. If the last reference for this process is removed,
            // remove its permission to access the handle.
            if (fAll || phd->ref.pFr->usRefs[inx] == 1) {
                phd->ref.pFr->usRefs[inx] = 0;
                if (RemoveAccess(&phd->lock, aky) == 0) {
				    KCALLPROFOFF(30);
                    return TRUE;
				}
            } else
                --phd->ref.pFr->usRefs[inx];
        }
    }
    KCALLPROFOFF(30);
    return FALSE;
}

// Returns TRUE if all references removed.
BOOL DecRef(HANDLE h, PPROCESS pprc, BOOL fAll)
{
    return KCall(DoDecRef, h, pprc, fAll);
}

BOOL CheckLastRef(HANDLE hTh) {
	PHDATA phd;
	BOOL bRet;
	KCALLPROFON(49);
	bRet = TRUE;
	h2p(hTh,phd);
	if (phd && phd->pvObj && (phd->pci->type==SH_CURTHREAD) && ((THREAD *)(phd->pvObj))->pOwnerProc == pCurProc) {
		if (phd->ref.count < 0x10000) {
			if (phd->ref.count == 1)
				bRet = FALSE;
		} else {
        	if (phd->ref.pFr->usRefs[pCurProc->procnum] == 1)
        		bRet = FALSE;
        }
    }
	KCALLPROFOFF(49);
	return bRet;
}

// Returns 0 if handle is not valid.
DWORD GetUserInfo(HANDLE h)
{
    PHDATA phd;

    h2p(h, phd);
    return phd ? phd->dwInfo : 0;
}

// Returns FALSE if handle is not valid.
BOOL SetUserInfo(HANDLE h, DWORD info)
{
    PHDATA phd;

    h2p(h, phd);
    if (phd) {
        phd->dwInfo = info;
        return TRUE;
    }
    return FALSE;
}

// Returns NULL if handle is not valid.
PVOID GetObjectPtr(HANDLE h)
{
    PHDATA phd;

    h2p(h, phd);
    return phd ? phd->pvObj : 0;
}

// Returns NULL if handle is not valid or not correct type.
PVOID GetObjectPtrByType(HANDLE h, int type)
{
    PHDATA phd;

    h2p(h, phd);
    return (phd && phd->pci && phd->pci->type==type) ? phd->pvObj : 0;
}

PVOID GetObjectPtrByTypePermissioned(HANDLE h, int type)
{
    PHDATA phd;

    h2p(h, phd);
    return (phd && phd->pci && phd->pci->type==type && TestAccess(&phd->lock, &pCurThread->aky)) ? phd->pvObj : 0;
}

// Returns FALSE if handle is not valid.
BOOL SetObjectPtr(HANDLE h, PVOID pvObj)
{
    PHDATA phd;

    h2p(h, phd);
    if (phd) {
        phd->pvObj = pvObj;
        return TRUE;
    }
    return FALSE;
}

// Returns 0 if handle is not valid.
int GetHandleType(HANDLE h)
{
    PHDATA phd;

    h2p(h, phd);
    return (phd && phd->pci != 0) ? phd->pci->type : 0;
}

// SetHandleOwner - switch ownership of a handle
//
//      SetHandleOwner will take a handle that the current process has exclusive
// permission to access and give exclusive access to another process. This is used
// by the file system to switch ownership of handles which are returned by forwarded
// api calls.
//
// NOTE: The handle reference count must be 1.

BOOL SC_SetHandleOwner(HANDLE h, HANDLE hProc)
{
    PPROCESS pprc;
    PHDATA phd;

    if ((phd = HandleToPointer(h)) != 0 && phd->ref.count == 1
            && TestAccess(&phd->lock, &CurAKey)
            && (pprc = HandleToProc(hProc)) != 0) {
        // Zap the old lock with the key from the new owner.
		LockFromKey(&phd->lock, &pprc->aky);
		return TRUE;
    }
    return FALSE;
}


// @func HANDLE | CreateAPISet | Creates an API set
// @rdesc Returns the handle to the API set
// @parm char[4] | acName | class name (for debugging)
// @parm USHORT | cFunctions | # of functions in set
// @parm PFNVOID* | ppfnMethods | array of API functions
// @parm LPDWORD | pdwSig | array of signatures for API functions
// @comm Creates an API set from the list of functions passed in

HANDLE
SC_CreateAPISet(
char acName[4],             /* 'class name' for debugging & QueryAPISetID() */
USHORT cFunctions,          /* # of functions in set */
const PFNVOID *ppfnMethods,       /* array of API functions */
const DWORD *pdwSig)             /* array of signatures for API functions */
{
    PAPISET pas;
    HANDLE hSet;
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_CreateAPISet failed due to insufficient trust\r\n"));
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
    if ((pas = AllocMem(HEAP_APISET)) != 0
            && (hSet = AllocHandle(&cinfAPISet, pas, pCurProc)) != 0) {
        *(PDWORD)pas->cinfo.acName = *(PDWORD)acName;
        pas->cinfo.disp = DISPATCH_PSL;
        pas->cinfo.type = NUM_SYS_HANDLES;
        pas->cinfo.cMethods = cFunctions;
        pas->cinfo.ppfnMethods = ppfnMethods;
        pas->cinfo.pdwSig = pdwSig;
        pas->cinfo.pServer = pCurProc;
        pas->iReg = -1;     /* not a registered handle */
        return hSet;
    } else if (pas != 0)
        FreeMem(pas, HEAP_APISET);
    KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
    return 0;
}


void
SC_CloseAPISet(HANDLE hSet)
{
    PAPISET pas;

    if (DecRef(hSet, pCurProc, FALSE) && (pas = HandleToAPISet(hSet)) != 0) {
        if (pas->iReg != -1 && pas->cinfo.type == 0) {
            SystemAPISets[pas->iReg] = 0;
            KInfoTable[KINX_API_MASK] &= ~(1ul << pas->iReg);
        }
        FreeMem(pas, HEAP_APISET);
        FreeHandle(hSet);
    }
    return;
}

//      @func BOOL | RegisterAPISet | Registers a global APIset
//      @rdesc Returns TRUE on success, else FALSE
//      @parm HANDLE | hASet | handle to API set
//      @parm DWORD | dwSetID | constant identifying which API set to register as
//      @comm Associates the APIset with a slot in the global APIset table

BOOL
SC_RegisterAPISet(HANDLE hSet, DWORD dwSetID)
{
    PAPISET pas;
    BOOL typeOnly, bKPSL;
    int err = ERROR_INVALID_PARAMETER;
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_RegisterAPISet failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return 0;
	}
    typeOnly = dwSetID >> 31;
    bKPSL = (dwSetID >> 30) & 1;
    dwSetID = dwSetID & 0x3fffffff;
    if (dwSetID == 0) {
        err = ERROR_NO_PROC_SLOTS;
        dwSetID = NUM_SYS_HANDLES;
        while ((dwSetID - 1) > SH_LASTRESERVED)
            if (!SystemAPISets[--dwSetID])
            	break;
    } else
        dwSetID &= 0x7fffffff;
    if ((pas = HandleToAPISet(hSet)) && (pas->iReg == -1) &&
    	(typeOnly ? (dwSetID < SH_LAST_NOTIFY && SystemAPISets[dwSetID] && 
    				 SystemAPISets[dwSetID]->type == dwSetID) :
        			((dwSetID >= SH_LAST_NOTIFY) && (dwSetID < NUM_SYS_HANDLES) && 
        			 !SystemAPISets[dwSetID]))) {
        if (typeOnly) {
            pas->iReg = dwSetID;
            pas->cinfo.type = (uchar)dwSetID;
        } else {
            pas->iReg = dwSetID;
            pas->cinfo.disp = bKPSL ? DISPATCH_I_KPSL : DISPATCH_I_PSL;
            pas->cinfo.type = 0;
            SystemAPISets[dwSetID] = &pas->cinfo;
            KInfoTable[KINX_API_MASK] |= 1ul << dwSetID;
        }
        return TRUE;
    } else {
        RETAILMSG(1, (TEXT("RegisterAPISet failed for api set id %d.\r\n"),dwSetID));
        KSetLastError(pCurThread, err);
    }
    return FALSE;
}

PPROCESS CalcHandleOwner(void) {
	if (pCurThread->pcstkTop) {
		if ((DWORD)pCurThread->pcstkTop->pprcLast >= 0x10000)
			return pCurThread->pcstkTop->pprcLast;
		if ((pCurThread->pcstkTop->pcstkNext) && ((DWORD)pCurThread->pcstkTop->pcstkNext->pprcLast >= 0x10000))
    		return pCurThread->pcstkTop->pcstkNext->pprcLast;
	}    		
	return pCurProc;
}

HANDLE
SC_CreateAPIHandle(HANDLE hSet, LPVOID pvData)
{
    PAPISET pas;
    HANDLE hRet;

    if ((pas = (bAllKMode ? HandleToAPISetPerm(hSet) : HandleToAPISet(hSet))) && (pas->cinfo.disp == DISPATCH_PSL)) {
        if ((hRet = AllocHandle(&pas->cinfo, pvData, CalcHandleOwner())) != 0)
            return hRet;
        KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
    } else
        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return 0;
}


LPVOID
SC_VerifyAPIHandle(HANDLE hSet, HANDLE h)
{
    PAPISET pas;
    PHDATA phd;

    if ((pas = HandleToAPISet(hSet)) != 0) {
        if ((phd = HandleToPointer(h)) != 0
                && TestAccess(&phd->lock, &pCurThread->aky)
                && phd->pci == &pas->cinfo)
            return phd->pvObj;
	    KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
    } else
	    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return 0;
}


// @func int | QueryAPISetID | Lookup an API Set index
// @rdesc Returns the index of the requested API set (-1 if not found)
// @parm PCHAR | pName | the name of the API Set (4 characters or less)
// @comm Looks in the table of registered API Sets to find an API set with
//       the requested name. If one is found, the index of that set is returned.
//       If no matching set is found, then -1 is returned.

int
SC_QueryAPISetID(char *pName)
{
    const CINFO *pci;
    char acName[4];
    int inx;

    for (inx = 0 ; inx < sizeof(acName) ; ++inx) {
        if ((acName[inx] = *pName) != 0)
            pName++;
    }
    if (*pName == 0) {
        for (inx = 0 ; inx < NUM_SYS_HANDLES ; ++inx) {
            if ((pci = SystemAPISets[inx]) != 0
                    && *(PDWORD)pci->acName == *(PDWORD)acName)
                return inx;
        }
    }
    return -1;
}


// @func DWORD | PerformCallback | Performs a call into another address space
// @rdesc Returns the return value from the called function
// @parm PCALLBACKINFO | pcbi | the callback to perform
// @parmvar Zero or more parameters to pass to the function
// @comm Switches into the address space of the process specified in the callback,
//       and then calls the specified function

//NOTE: PerformCallBack() is implemented directly by ObjectCall().

// SetBadHandleError - set error code based upon handle type
//
//  SetBadHandleError uses the handle type from the API call and the method
// index to determine what value to return from an API call on an invalid handle.
// If there is not default behavior, then an exception is raised.

uint SetBadHandleError(int type, long iMethod) {
    RETAILMSG(1, (TEXT("Invalid handle: Set=%d Method=%d\r\n"), type, iMethod));
    KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
    switch (type) {
    case HT_SOCKET:
	    return WSAENOTSOCK;

    case HT_WNETENUM:
        return ERROR_INVALID_HANDLE;
        
    case HT_FILE:
	    if (iMethod == 4 || iMethod == 5)
    	    return 0xFFFFFFFF;
    	return 0;

	case SH_CURTHREAD:
        switch (iMethod) {
	    case ID_THREAD_SUSPEND:
    	case ID_THREAD_RESUME:
            return 0xFFFFFFFF;

        case ID_THREAD_GETTHREADPRIO:
        case ID_THREAD_CEGETPRIO:
            return THREAD_PRIORITY_ERROR_RETURN;
        }
	    return 0;

    case SH_CURPROC:
		return FALSE;

    case HT_DBFILE:
    case HT_DBFIND:
    case HT_FIND:
	case HT_EVENT:
    case HT_APISET:
    case HT_MUTEX:
    case HT_FSMAP:
    case HT_SEMAPHORE:
    	return FALSE;	// just return FALSE
    }

    // Fault the thread.
    RaiseException(STATUS_INVALID_SYSTEM_SERVICE, EXCEPTION_NONCONTINUABLE, 1, &iMethod);
    DEBUGCHK(0);    /* shouldn't get here */
    return 0;
}

// ErrorReturn - return a error value from an invalid API call
//
//  This function is invoked via ObjectCall as a kernel PSL. It extracts a
// return value from the CALLSTACK's access key.

uint ErrorReturn(void) {
    uint ret;
    ret = pCurThread->pcstkTop->akyLast;
    pCurThread->pcstkTop->akyLast = 1;     // Make sure that ServerCallReturn frees the CALLSTACK
    return ret;
}

#ifdef NKPROF
extern BOOL bProfileObjCall;
extern void ProfilerHit(RETADDR ra);
#endif

BOOL TryCloseMappedHandle(HANDLE h);

RETADDR ObjectCall(int *pMode, RETADDR ra, void *args, long iMethod)
{
    const HDATA *phd;
    const CINFO *pci;
    int inx;
    HANDLE h;
    PPROCESS pprc;
    PCALLSTACK pcstk;
    PTHREAD pth;
    LPVOID lpvVal;
    register DWORD dwSig;
    DWORD dwVal, dwBase;
    ACCESSKEY akyCur;
    PCALLBACKINFO pcbi;
    PFNVOID pfn;

	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);

    pth = pCurThread;
    /** Setup a kernel PSL CALLSTACK structure. This is done here so that
     * any exceptions are handled correctly.
     */
    // If this fails, we'll fault on setting the retAddr - but we'd only raise an exception if we
    // were checking, anyway.
    pcstk = AllocMem(HEAP_CALLSTACK);
	/* Save return address, previous access key, and extra CPU dependent info */
    pcstk->retAddr = ra;
    akyCur = pcstk->akyLast = pth->aky;
    pcstk->extra = pMode[1];
    /* Save previous kernel/user mode info */
    pcstk->pprcLast = (HANDLE)*pMode;
    /* Link the CALLSTACK struct at the head of the thread's list */
    pcstk->pcstkNext = pth->pcstkTop;
    pth->pcstkTop = pcstk;

   	DEBUGMSG(ZONE_OBJDISP, (TEXT("ObjectCall: ra=%8.8lx h=%8.8lx iMethod=%lx mode=%x extra=%8.8lx\r\n"),
   	        ra, Arg(args, HANDLE), iMethod, pMode[0], pMode[1]));
   	inx = NUM_SYS_HANDLES;
    if (iMethod < 0) {
        /* API call to an implicit handle. In this case, bits 30-16 of the
         * method index indicate which entry in the system handle table.
         */
        iMethod = -iMethod;
        if ((inx = iMethod>>HANDLE_SHIFT & HANDLE_MASK) >= NUM_SYS_HANDLES
                || (pci = SystemAPISets[inx]) == 0) {
            DEBUGMSG(1, (TEXT("ObjectCall: Failed(1): %lx\r\n"), inx));
            RaiseException(STATUS_INVALID_SYSTEM_SERVICE, EXCEPTION_NONCONTINUABLE,
                    1, &iMethod);
            DEBUGCHK(0);    /* shouldn't get here */
        }
        iMethod &= METHOD_MASK;
        DEBUGCHK(pci->type == 0 || pci->type == inx);
        inx = pci->type;
    }

    /* For handle based api calls, validate the handle argument. */
    if (inx) {
        /* validate handle argument and turn into HDATA pointer. */
        h = Arg(args, HANDLE);
        h2p(h, phd);
        if (!phd || !TestAccess(&phd->lock, &pth->aky)) {
            if (iMethod <= 1) {
            	DEBUGMSG(1, (TEXT("CloseHandle/WaitForSingleObject: invalid handle h=%x\r\n"),
            	        Arg(args, HANDLE)));
	            KSetLastError(pth, ERROR_INVALID_HANDLE);
                pcstk->akyLast = (ACCESSKEY)(iMethod==0 ? 0 : WAIT_FAILED);
                /* Continue the thread in kernel mode */
                *pMode = KERNEL_MODE;
                return (PFNVOID)ErrorReturn;
	        }
    		DEBUGMSG(1, (TEXT("ObjectCall: Failed (2): h=%x\r\n"), Arg(args, HANDLE)));
            pcstk->akyLast = (ACCESSKEY)SetBadHandleError(inx, iMethod);
            /* Continue the thread in kernel mode */
            *pMode = KERNEL_MODE;
            return (PFNVOID)ErrorReturn;
        }
        pci = phd->pci;
        if (pci->type != inx && (inx != NUM_SYS_HANDLES || iMethod > 1)) {
            // The handle is valid but the type is wrong.
            DEBUGMSG(1, (TEXT("ObjectCall: Failed(2a): h=%x t=%d shdb %d\r\n"),
                    Arg(args, HANDLE), pci->type, inx));
            pcstk->akyLast = (ACCESSKEY)SetBadHandleError(inx, iMethod);
            /* Continue the thread in kernel mode */
            *pMode = KERNEL_MODE;
            return (PFNVOID)ErrorReturn;
        }
    }

    /* validate interface and method indicies. */
	DEBUGMSG(ZONE_OBJDISP,
	        (TEXT("ObjectCall: %8.8lx '%.4a' API call #%d. disp=%d.\r\n"),
	        pci, pci->acName, iMethod, pci->disp));
    if (iMethod >= pci->cMethods) {
		DEBUGMSG(1,(TEXT("ObjectCall: Failed (3): %d %d\r\n"),
			    iMethod, pci->cMethods));
        RaiseException(STATUS_INVALID_SYSTEM_SERVICE, EXCEPTION_NONCONTINUABLE, 1, &iMethod);
        DEBUGCHK(0);    /* shouldn't get here */
    }

    /* Dispatch api call based upon the dispatch type in the cinfo struct. */
	switch (pci->disp) {
	case DISPATCH_KERNEL_PSL:
    case DISPATCH_I_KPSL:
        pfn = pci->ppfnMethods[iMethod];
        if ((pfn == (PFNVOID)-1) || (pfn == (PFNVOID)-2)) {
            /* PerformCallBack() or PerformCallForward() kernel API call.
             * Save the previous process and scan the CALLSTACKs to find
             * the new access key to use.
             */
            pcbi = Arg(args, PCALLBACKINFO);
        	DEBUGMSG(ZONE_OBJDISP,(TEXT("CallBack: info=%8.8lx, pfn=%8.8lx\r\n"), pcbi, pcbi->pfn));
            Arg(args, LPVOID) = pcbi->pvArg0;
            pfn = (PFNVOID)pcbi->pfn;
            






            pfn = (PFNVOID)ZeroPtr(pfn);
            h = pcbi->hProc;
            h2p(h, phd);
            if (!phd || !phd->pci || (phd->pci->type != SH_CURPROC) || !(pprc = phd->pvObj) ||
                    ((pCurProc->bTrustLevel != KERN_TRUST_FULL) && !((DWORD)pcstk->retAddr & 0x80000000))) {
                /* Bogus process handle. Raise an "invalid parameter" exception. */
				ERRORMSG(phd && phd->pci && (phd->pci->type == SH_CURPROC) && phd->pvObj,(L"PerformCallback failed due to insufficient trust\r\n"));
                RaiseException(STATUS_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, 0);
                DEBUGCHK(0);    /* shouldn't get here */
            }
        	/* Save previous process */
            pcstk->pprcLast = pth->pProc;

            /* Scan the CALLSTACKs to find the new access key to use. */
            if ((pci->ppfnMethods[iMethod] == (PFNVOID)-1) && (pcbi->hProc != ProcArray[0].hProc)) {
	            while (((pcstk = (PCALLSTACK)((ulong)pcstk->pcstkNext&~1)) != 0) && (pcstk->pprcLast != pprc))
	            	;
	            if (pcstk)
    	        	pth->aky = pcstk->akyLast;
        	    else {
	            	/* The thread must always have the permissions of its owner process
    	        	 * so that it can access its stack, as well as the kernel and the
        	    	 * destination process. */
            		pth->aky = pth->pOwnerProc->aky;
                	AddAccess(&pth->aky, ProcArray[0].aky);
	                AddAccess(&pth->aky, pprc->aky);
	            }
            }
            /* Switch to the new process */
            pth->pProc = pprc;
            SetCPUASID(pth);        /* switch to callback process's address space */
        } else {
            /* Continue the thread in kernel mode */
            GoToKernTime();
            *pMode = KERNEL_MODE;
        }

    	DEBUGMSG(ZONE_OBJDISP, (TEXT("KPSLDispatch: pci %8.8lx, fnptr %8.8lx\r\n"),
    		    pci, pfn));
#ifdef XTIME
		if (iMethod >= 2 && iMethod <= XT_NUM_PSL+2)
			SyscallTime(iMethod+XT_FIRST_PSL-2);
#endif
#ifdef NKPROF
	if (bProfileObjCall)
		ProfilerHit(pfn);
#endif
		return pfn;

	case DISPATCH_PSL:
		lpvVal = phd->pvObj;
    	if (iMethod == 0) {
    	    /* Closing this handle, decrement the reference count */
        	if (!TestAccess(&phd->lock, &pCurProc->aky)) {
            	if ((phd->lock == 1) && (phd->ref.count == 1)) {
					pcstk->akyLast = TryCloseMappedHandle(h);
					*pMode = KERNEL_MODE;
					return (PFNVOID)ErrorReturn;
            	}
	            KSetLastError(pth, ERROR_INVALID_HANDLE);
        		pcstk->akyLast = 0;
	            *pMode = KERNEL_MODE;
    	        return (PFNVOID)ErrorReturn;
    	    }
        	DEBUGMSG(ZONE_OBJDISP, (TEXT("PSLDispatch: closing=%8.8lx\r\n"),
        	        Arg(args, HANDLE)));
    	    if (DecRef(Arg(args, HANDLE), pCurProc, FALSE))
    	    	FreeHandle(Arg(args,HANDLE));
    	}
        Arg(args, LPVOID) = lpvVal;
    	DEBUGMSG(ZONE_OBJDISP,(TEXT("PSLDispatch: info=%8.8lx\r\n"), phd->pvObj));
    case DISPATCH_I_PSL:
       	/* Save previous process */
        pcstk->pprcLast = pth->pProc;
	    dwBase = pth->pProc->dwVMBase;
        /* setup thread structure for method call */
        pth->pProc = pprc = pci->pServer;
        AddAccess(&pth->aky, pprc->aky);
        SetCPUASID(pth);        /* switch to server's address space */
	    for (dwSig = pci->pdwSig[iMethod] ; dwSig ; dwSig >>= ARG_TYPE_BITS) {
	    	if ((dwSig & ARG_TYPE_MASK) == ARG_PTR) {
	            dwVal = Arg(args, DWORD);
	            if (!(dwVal>>VA_SECTION) && (dwVal >= 0x10000))
	                Arg(args, DWORD) = dwVal + dwBase;
			}
            NextArg(args, DWORD);
	    }
		DEBUGMSG(ZONE_OBJDISP, (TEXT("PSLDispatch: pci %8.8lx, fnptr %8.8lx\r\n"),
			    pci, pci->ppfnMethods[iMethod]));
#ifdef NKPROF
	if (bProfileObjCall)
		ProfilerHit(pci->ppfnMethods[iMethod]);
#endif
	    return pci->ppfnMethods[iMethod];

	default:
	    DEBUGMSG(ZONE_OBJDISP, (TEXT("ObjectCall: invalid dispatch type\r\n")));
	    DEBUGCHK(0);
   		break;
	}
#ifdef NKPROF
	if (bProfileObjCall)
		ProfilerHit(ra);
#endif
	return ra;
}

BOOL SC_CeMapArgumentArray(HANDLE hProc, LPVOID *pArgList, DWORD dwSig) {
	DWORD dwVal;
	PPROCESS pproc;
	pproc = HandleToProc(hProc);
	if (!pproc) {
	    KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
	    return 0;
	}
    while (dwSig) {
    	if ((dwSig & ARG_TYPE_MASK) == ARG_PTR) {
            dwVal = (DWORD)*pArgList;
            if (!(dwVal>>VA_SECTION) && (dwVal >= 0x10000)) {
                *pArgList = (LPVOID)(dwVal + pproc->dwVMBase);
            }
        }
		pArgList++;    
		dwSig >>= ARG_TYPE_BITS;
	}
	return 1;
}

void SC_KillThreadIfNeeded() {
    if (GET_DYING(pCurThread) && !GET_DEAD(pCurThread) && (LLcs.OwnerThread != hCurThread) && (pCurProc == pCurThread->pOwnerProc)) {
		SET_DEAD(pCurThread);
		CLEAR_USERBLOCK(pCurThread);
		CLEAR_DEBUGWAIT(pCurThread);
		(*(void (*)(int))pExitThread)(0);
    }
}

RETADDR ServerCallReturn(int *pMode) {
    PPROCESS pprc;
    PCALLSTACK pcstk;
    RETADDR ra;
    ACCESSKEY aky;
    extern LPVOID pExitThread;

	pcstk = pCurThread->pcstkTop;
    pprc = pcstk->pprcLast;
    ra = pcstk->retAddr;
    pMode[1] = pcstk->extra;
	if (pcstk == &pCurThread->IntrStk) {
		pCurThread->pcstkTop = pcstk->pcstkNext;
        *pMode = (int)pprc;
        GoToUserTime();
	} else {
		aky = pcstk->akyLast;

		/* unlink CALLSTACK structure from list */
		pCurThread->pcstkTop = pcstk->pcstkNext;
        if (IsValidKPtr(pcstk))
		   	FreeMem(pcstk,HEAP_CALLSTACK);
	   	if (GET_DYING(pCurThread) && !GET_DEAD(pCurThread) && (LLcs.OwnerThread != hCurThread) &&
	   	    !((DWORD)ra & 0x80000000) && 
			(!pCurThread->pcstkTop || (pprc == pCurThread->pOwnerProc) ||
			 (((DWORD)pprc < 0x10000) && (pCurThread->pOwnerProc == pCurThread->pProc)))) {
			SET_DEAD(pCurThread);
			CLEAR_USERBLOCK(pCurThread);
			CLEAR_DEBUGWAIT(pCurThread);
			ra = (RETADDR)pExitThread;
	   	}

		DEBUGMSG(ZONE_OBJDISP, (TEXT("SCRet: return to %8.8lx Proc=%lx aky=%lx\r\n"),ra, pprc, pCurThread->aky));
        if ((ulong)pprc < 0x10000) {
            /* This call is returning from kernel mode server. Restore
             * the thread's previous operating mode and return.
             */
            *pMode = (int)pprc;
            GoToUserTime();
        } else {
            /* Returning from a user mode server, restore the previous
             * address space information and return.
             */
	        pCurThread->aky = aky;
            pCurThread->pProc = pprc;
            SetCPUASID(pCurThread);
        }
    }
    return ra;
}

PFNVOID DBG_CallCheck(PTHREAD pth, DWORD dwJumpAddress) {
    register const HDATA *phd;
    register const CINFO *pci;
    register int inx;
    register long iMethod;
    HANDLE h;

#if R3000 || R4000
    h = (HANDLE)pth->ARG0;
    if ((dwJumpAddress & 3) == 2 || dwJumpAddress
            < IMPLICIT_CALL(HANDLE_MASK, METHOD_MASK))
        return 0;
    iMethod = (long)(dwJumpAddress - FIRST_METHOD) >> 2;
#elif SHx
    h = (HANDLE)pth->ARG0;
    if (!(dwJumpAddress & 1) || dwJumpAddress
            < IMPLICIT_CALL(HANDLE_MASK, METHOD_MASK))
        return 0;
    iMethod = (long)(dwJumpAddress - FIRST_METHOD) >> 1;
#elif x86
    h = *(HANDLE*)(pth->ctx.TcxEsp+4);
    if ((dwJumpAddress & 1) || dwJumpAddress
            < IMPLICIT_CALL(HANDLE_MASK, METHOD_MASK))
        return 0;
    iMethod = (long)(dwJumpAddress - FIRST_METHOD) >> 1;
#elif PPC
    h = (HANDLE)pth->ARG0;
    if ((dwJumpAddress & 3) || dwJumpAddress
            < IMPLICIT_CALL(HANDLE_MASK, METHOD_MASK))
        return 0;
    iMethod = (long)(dwJumpAddress - FIRST_METHOD) >> 2;
#elif ARM
    h = (HANDLE)pth->ARG0;
    if ((dwJumpAddress & 3) || dwJumpAddress
            < IMPLICIT_CALL(HANDLE_MASK, METHOD_MASK))
        return 0;
    iMethod = (long)(dwJumpAddress - FIRST_METHOD) >> 2;
#else
    #error("Unknown CPU")
#endif

   	DEBUGMSG(ZONE_OBJDISP, (TEXT("CallCheck: pth %8.8lx, h %8.8lx, iMethod %lx\r\n"),
   	        pth, h, iMethod));
    if (iMethod < 0) {
        /* API call to an implicit handle. In this case, bits 30-16 of the
         * method index indicate which entry in the system handle table.
         */
        iMethod = -iMethod;
        if ((inx = iMethod>>HANDLE_SHIFT & HANDLE_MASK) >= NUM_SYS_HANDLES
                || (pci = SystemAPISets[inx]) == 0)
            return 0;
        iMethod &= METHOD_MASK;
    } else {
        /* validate handle argument and turn into HDATA pointer. */
        if ((phd = HandleToPointer(h)) == 0)
            return 0;
        pci = phd->pci;
    }

    /* validate interface and method indicies. */
    if (iMethod >= pci->cMethods)
        return 0;

    /* Dispatch api call based upon the dispatch type in the cinfo struct. */
	switch (pci->disp) {
	case DISPATCH_KERNEL:
    case DISPATCH_I_KERNEL:
	case DISPATCH_KERNEL_PSL:
    case DISPATCH_I_KPSL:
    	DEBUGMSG(ZONE_OBJDISP,
	            (TEXT("CallCheck: %8.8lx '%.4a' API call #%d. pfn=%8.8lx.\r\n"),
	            pci, pci->acName, iMethod, pci->ppfnMethods[iMethod]));
        return pci->ppfnMethods[iMethod];

	case DISPATCH_PSL:
    case DISPATCH_I_PSL:
    {
        DWORD vmBase;
        PFNVOID *ppfn;
        PFNVOID pfn;

        vmBase = pci->pServer->dwVMBase;
        ppfn = (PFNVOID*)((DWORD)pci->ppfnMethods | vmBase);
        pfn = (PFNVOID)((DWORD)ppfn[iMethod] | vmBase);
    	DEBUGMSG(ZONE_OBJDISP,
	            (TEXT("CallCheck: %8.8lx '%.4a' API call #%d. pfn=%8.8lx.\r\n"),
	            pci, pci->acName, iMethod, pfn));
        return pfn;
    }

	default:
   		break;
	}
	return 0;
}

