/*		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
// @doc    EXTERNAL KERNEL
#include "kernel.h"
#include "excpt.h"
#include "memtrack.h"

#if !defined(MIPS) && !defined(SHx)
#undef MEMTRACKING
#endif

/*
@topic  Resource tracking Support for Applications | 
        The kernel provides support for registering and tracking any kind
        of resource. If you are trying to register and track resources in 
        your program you will need to look at the <f RegisterTrackedItem>,
        <f AddTrackedItem> and <f DeleteTrackedItem> calls. If you are a 
        user trying to use the tracking to find leaks, you will need to 
        study the <f FilterTrackedItem> and <f PrintTrackedItem> calls. The
        WinCE shell has been enhanced to give you a command line interface
        to these calls.  You can make a function to print the tracked item
		using <f TrackerCallBack>.
*/ 

#ifdef MEMTRACKING
CALLBACKINFO cbi;
pTrack_Node pnFirst;          // index of first add in sequence
pTrack_Node pnLast;           // index of last add in sequence
LPMEMTR_ITEMTYPE rgRegTypes;  // registered types
LPMEMTR_FILTERINFO lpFilterInfo; // filter information
pTrack_Node table;            // hash table
UINT uiTablesize, Count;	  // size of hash table
#pragma message("MemTracking")

typedef struct _NameandBase {
    LPCHAR   szName;
    UINT    ImageBase;
    UINT    ImageSize;
    } NAMEANDBASE, *PNAMEANDBASE;

#if MIPS
#define STK_ALIGN	0x7
#define INST_SIZE	4
#define STACKPTR 	IntSp
#define RETADDR		IntRa
PRUNTIME_FUNCTION NKLookupFunctionEntry(PPROCESS pProc, ULONG ControlPc);
#define LookupFunctionEntry(a,b) NKLookupFunctionEntry((a),(b))
#elif SHx
#define STK_ALIGN	0x3
#define INST_SIZE	2
#define STACKPTR 	R15
#define RETADDR		PR
PRUNTIME_FUNCTION NKLookupFunctionEntry(PPROCESS pProc, ULONG ControlPc, PRUNTIME_FUNCTION prf);
RUNTIME_FUNCTION TempFunctionEntry;
#define LookupFunctionEntry(a,b) NKLookupFunctionEntry((a),(b),(&TempFunctionEntry))
#endif

#define NKGetStackLimits(pth, LL, HL) ((*(LL) = (pth)->dwStackBase), (*(HL) = (ULONG)(pth)->tlsPtr))

void UpdateASID(IN PTHREAD pth, IN PPROCESS pProc, IN ACCESSKEY aky);

LPMODULE MEMTR_ModuleFromAddress(DWORD dwAdd);
LPWSTR MEMTR_GetWin32ExeName(PPROCESS pProc);

CONTEXT CurrentContext;

CRITICAL_SECTION Trackcs;

BOOL MemTrackInit2(void) {
    LPBYTE lpbMem;
    DWORD  dwUsed;

	RETAILMSG(1, (L"InInit\r\n"));
    
    DEBUGMSG(ZONE_MEMTRACKER, (L"Len : %8.8lx Start: %8.8lx\r\n", 
                pTOC->ulTrackingLen, pTOC->ulTrackingStart));

	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
		return FALSE;
    }
	RETAILMSG(1, (L"StillInInit!!\r\n"));
	if (!(lpbMem = (LPBYTE)VirtualAlloc(0, pTOC->ulTrackingLen, MEM_RESERVE, PAGE_NOACCESS)))
    {
        ERRORMSG(1, (L"VirtualAlloc failed (%8.8lx) for memtracking\r\n", GetLastError()));
		return FALSE;
    }
	if (!VirtualCopy(lpbMem, (LPVOID)pTOC->ulTrackingStart, pTOC->ulTrackingLen, PAGE_READWRITE)) {
		VirtualFree(lpbMem, 0, MEM_RELEASE);
        ERRORMSG(1, (L"VirtualCopy failed (%8.8lx) for memtracking\r\n", GetLastError()));
		return FALSE;
	}
	lpbMem = MapPtr(lpbMem);
    memset(lpbMem, 0, pTOC->ulTrackingLen);
    dwUsed = 0;

	// now divy up the memory we got - the table soaks up all available left in the end
	lpFilterInfo = (LPMEMTR_FILTERINFO)(lpbMem+dwUsed);
	// initialize to start off with no tracking
    lpFilterInfo->dwFlags |= FILTER_TYPEDEFAULTOFF;
	dwUsed += sizeof(MEMTR_FILTERINFO);
	
	rgRegTypes = (LPMEMTR_ITEMTYPE)(lpbMem+dwUsed);
	dwUsed += MAX_REGTYPES*sizeof(MEMTR_ITEMTYPE);
	
	table = (pTrack_Node)(lpbMem+dwUsed);
	uiTablesize = (pTOC->ulTrackingLen-dwUsed) / sizeof(Track_Node);

	    
    DEBUGMSG(ZONE_MEMTRACKER, (L"Size %8.8lx Sizeof %8.8lx\r\n", 
        uiTablesize, (DWORD)sizeof(Track_Node)));

    return TRUE;
}

// init: initialize hash table, sequence pointers
void MemTrackInit(void)
{								 
	InitializeCriticalSection(&Trackcs);
	cbi.hProc = ProcArray[0].hProc;
	cbi.pfn = (FARPROC)MemTrackInit2;
	cbi.pvArg0 = 0;
}

#endif

/*
@func   DWORD | RegisterTrackedItem | Registers a type of resource to be tracked
@parm   LPWSTR | szName | A friendly name for the resource to be tracked. Must be less than 16
        characters. If this is NULL a list of all currently registered types is printed on the
        debug terminal.
@rdesc  The typeid which this resource has been alloted. You should use this in all other calls.
@comm   Before you can track any resource, you must register it's type and get back a dword
        TypeID for it. This typeid is used in all other resource tracker calls. Each resource
        is guaranteed a unique resource type. If a resource with the exact same name exists
        it's type id is returned. This allows DLL's to register the same type multiple times
        in their PROCESS_ATTACH while keeping the same TypeID.
@xref   <f FilterTrackedItem> <f AddTrackedItem> <f PrintTrackedItem> <f DeleteTrackedItem> <f TrackerCallBack>
*/
DWORD SC_RegisterTrackedItem(LPWSTR szName)
{
    DWORD dwRet = (DWORD)-1;
#ifdef MEMTRACKING
    int i,j;
    LPWSTR  lpwTgt, lpwSrc;
    
	EnterCriticalSection(&Trackcs);
	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
        goto errret;
    }
    if (!uiTablesize && !PerformCallBack4(&cbi,0))
        goto errret;

    if (!szName) {
        RETAILMSG(1, (L"Registered types: Default tracking state is %s\r\n", 
            (lpFilterInfo->dwFlags&FILTER_TYPEDEFAULTOFF)? L"Off": L"On"));
        RETAILMSG(1, (L"  Resource Name                   :TypeID:Tracking\r\n"));
        for (i=0; i<MAX_REGTYPES && rgRegTypes[i].dwFlags; i++) {
            RETAILMSG(1, (L"  %-32.32s:%6.6u:%s\r\n", rgRegTypes[i].szName, i, 
                (rgRegTypes[i].dwFlags&REGTYPE_MASKED)? L"  Off ":L"  On "));
        }    
        goto errret;
    }
    for (i=0; i<MAX_REGTYPES && rgRegTypes[i].dwFlags; i++) {
        lpwTgt=rgRegTypes[i].szName; 
        lpwSrc=szName;
        while(*lpwTgt && (*lpwTgt++==*lpwSrc++));
        if (!*lpwTgt && !*lpwSrc) {
            DEBUGMSG(ZONE_MEMTRACKER, (TEXT("Tracking: Name %s found at %u\r\n"), rgRegTypes[i].szName, i));
            dwRet = i;
            goto errret;
        }
    }
    if (i<MAX_REGTYPES) {
        // copy the name (cant use coredll string functions here)
        j=MAX_REGTYPENAMESIZE;
        lpwTgt=rgRegTypes[i].szName; 
        while(--j && (*lpwTgt++=*szName++));
        *lpwTgt = 0;
        rgRegTypes[i].dwFlags = (lpFilterInfo->dwFlags&FILTER_TYPEDEFAULTOFF)? 
                        REGTYPE_MASKED|REGTYPE_INUSE : REGTYPE_INUSE;
        DEBUGMSG(ZONE_MEMTRACKER, (TEXT("Tracking: Name %s type %u\r\n"), rgRegTypes[i].szName, i));
        dwRet = i;
    } else {
        ERRORMSG(1, (TEXT("MEMTRACK: Couldnt register type. Name ptr 0x%08X"), szName));
    }
errret:
	LeaveCriticalSection(&Trackcs);
#endif
	return dwRet;
}

/*
@func   VOID | FilterTrackedItem | Installs filters on AddTrackedItem
@parm   DWORD | dwFlags | A combination of the following
        @flag FILTER_TYPEDEFAULTON | Unmasks all types of resource regardless of their 
               previous state. Any new resource will also start off unmasked.
        @flag FILTER_TYPEDEFAULTOFF | All resource types are masked out. Any new resources
               will also start off masked off.
        @flag FILTER_TYPEON | Unmasks the resource type specified in <p dwType>. This flag
                is processed after the default flag
        @flag FILTER_TYPEOFF | Masks the resource type specified in <p dwType>. This flag
                is processed after the default flag
        @flag FILTER_PROCIDON | Installs a filter so that only resources for <p dwProcID>
                are tracked.
        @flag FILTER_PROCIDOFF | Removes any installed process filters.
@parm   DWORD | dwType | TypeID if the FILTER_TYPEON or FILTER_TYPEOFF flag is specified
@parm   DWORD | dwProcID | ProcID if the FILTER_PROCIDON flag is specified
@comm   You can use this function to restrict the number of resources being tracked in the 
        system. These filters will cause any resource items not matching the criteria to be
        removed from the resource list, and calls to <f AddTrackedItem> for them to fail. This
        can be used to improve the efficiency and memory usage of the resource tracker by keeping
        the number of resources being tracked to a minimum. For example of you want to track
        all resource types except for type 3, you can specify the FILTER_TYPEDEFAULTON and 
        the FILTER_TYPEOFF flags with 3 as dwTypeID. Similarly if you want to track only resource 
        type 7, you can specify the FILTER_TYPEDEFAULTOFF and FILTER_TYPEON flags with 7 as the
        dwTypeID. Currently process filters are only installable for a single process id.
@xref   <f RegisterTrackedItem> <f AddTrackedItem> <f PrintTrackedItem> <f DeleteTrackedItem> <f TrackerCallBack>
*/
VOID SC_FilterTrackedItem(DWORD dwFlags, DWORD dwType, DWORD dwProcID)
{
#ifdef MEMTRACKING
    int i;
    pTrack_Node pnCur, pnFree;
    
	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
    	ERRORMSG(1, (L"No resource tracking memory!!\r\n"));
        return;
    }
    
	EnterCriticalSection(&Trackcs);
	if (dwFlags&FILTER_TYPEDEFAULTOFF) {
	    lpFilterInfo->dwFlags |= FILTER_TYPEDEFAULTOFF;
	    for (i=0; i<MAX_REGTYPES && rgRegTypes[i].dwFlags&REGTYPE_INUSE; i++) {
	        rgRegTypes[i].dwFlags |= REGTYPE_MASKED;
	    }
	} else if (dwFlags&FILTER_TYPEDEFAULTON) {
	    lpFilterInfo->dwFlags &= ~FILTER_TYPEDEFAULTOFF;
	    for (i=0; i<MAX_REGTYPES && rgRegTypes[i].dwFlags&REGTYPE_INUSE; i++) {
	        rgRegTypes[i].dwFlags &= ~REGTYPE_MASKED;
	    }
	} 
	if (dwFlags&FILTER_TYPEON) {
	    if (dwType<MAX_REGTYPES && rgRegTypes[dwType].dwFlags&REGTYPE_INUSE) {
	        rgRegTypes[dwType].dwFlags &= ~REGTYPE_MASKED;
	    }    
	} else if (dwFlags&FILTER_TYPEOFF) {
	    if (dwType<MAX_REGTYPES && rgRegTypes[dwType].dwFlags&REGTYPE_INUSE) {
	        rgRegTypes[dwType].dwFlags |= REGTYPE_MASKED;
	    }    
	} 
	if (dwFlags&FILTER_PROCIDON) {
	    lpFilterInfo->dwFlags |= FILTER_PROCIDON;
	    lpFilterInfo->dwProcID = dwProcID;
	} else if (dwFlags&FILTER_PROCIDOFF) {
	    lpFilterInfo->dwFlags &= ~FILTER_PROCIDON;
	}

    // now put all existing entries through the filter
	for (pnCur = pnFirst; pnCur;) {
	    // do all the filtering
	    if ( (rgRegTypes[pnCur->dwType].dwFlags&REGTYPE_MASKED) ||
	         ((lpFilterInfo->dwFlags&FILTER_PROCIDON)&&(lpFilterInfo->dwProcID!=pnCur->dwProcID))) {
    	    // free the node
    	    pnFree=pnCur;
    	    pnCur=pnCur->pnNext;
	        MEMTR_deletenode(pnFree);
        } else {
            pnCur=pnCur->pnNext;
        }	         
	}
	
	LeaveCriticalSection(&Trackcs);
#endif
}

/*
@func   BOOL | AddTrackedItem | Registers a resource item
@parm   DWORD | dwType | TypeID for the resource. Must have been got from a call
        to <f RegisterTrackedItem>
@parm   HANDLE | handle | Handle for the resource. Must be unique within this type.
@parm   TRACKER_CALLBACK | cb | Callback function to pretty print the item. Can be NULL
        if no special printing of the custom DWORDs is required. See <f TrackerCallBack>.
@parm   DWORD | dwProcID | The ProcID of the process to which this resource should be
        registered to. This must have been got back from a call to <f GetCurrentProcessId>
        or <f GetCallerProcessId>
@parm   DWORD | dwSize | Amount of RAM this resource uses
@parm   DWORD | dwCustom1 | Custom dword - this is not interpreted at all. It can be used
        to point at other information which is dereferenced in the callback function. If the
        callback function is NULL these are simply printed out as DWORD's
@parm   DWORD | dwCustom2 | Second custom dword        
@rdesc  TRUE if the item was added, FALSE otherwise.  Function can fail if some parameter
        is invalid, if the system is out of memory for resource tracking, or if the user
        has filtered this resouce out.
@xref   <f RegisterTrackedItem> <f DeleteTrackedItem> <f PrintTrackedItem> <f FilterTrackedItem> <f TrackerCallBack>
*/
BOOL SC_AddTrackedItem(DWORD dwType, HANDLE handle, TRACKER_CALLBACK cb, DWORD dwProcID, 
    DWORD dwSize, DWORD dw1, DWORD dw2)
{
#ifdef MEMTRACKING
	UINT uiHashVal, uiRealHash;

	EnterCriticalSection(&Trackcs);
	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
        goto errret;
    }
    if (!uiTablesize && !PerformCallBack4(&cbi,0)){
        ERRORMSG(1, (L"MEMTR: Out of resource tracking memory!\r\n"));
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        goto errret;
    }        
	DEBUGCHK(table);

    // Check for any installed filters
    if (dwType>=MAX_REGTYPES || (rgRegTypes[dwType].dwFlags&REGTYPE_MASKED) ||
        ((lpFilterInfo->dwFlags&FILTER_PROCIDON)&&(lpFilterInfo->dwProcID!=dwProcID))) {
        goto errret;
    }

    uiRealHash=uiHashVal=MEMTR_hash(dwType,handle);

    do {
        if (!table[uiHashVal].Frames[0])
            break;

        if (++uiHashVal == uiTablesize)
            uiHashVal=0;
    } while (uiRealHash!=uiHashVal);

    if (table[uiHashVal].Frames[0])
    {
        ERRORMSG(1, (L"MEMTR: Out of resource tracking memory!\r\n"));
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        goto errret;
    }

    DEBUGMSG(ZONE_MEMTRACKER, (L"Adding handle %8.8lx at spot %8.8lx\r\n", (DWORD)handle, (DWORD)uiHashVal));    

	table[uiHashVal].handle = handle;
    table[uiHashVal].cb=cb;
    table[uiHashVal].dw1=dw1;
    table[uiHashVal].dw2=dw2;
    table[uiHashVal].dwProcID = (DWORD)HandleToPointer((HANDLE)dwProcID);
    table[uiHashVal].dwType = dwType;
    table[uiHashVal].dwSize = dwSize;
    table[uiHashVal].dwTime=GetTickCount();
    table[uiHashVal].hProc = hCurProc;
    {
		CALLBACKINFO cbstack = {ProcArray[0].hProc,(FARPROC)MEMTR_GetStackFrames,table[uiHashVal].Frames};
		PerformCallBack4(&cbstack);
    }
    table[uiHashVal].pnNext=NULL;
	table[uiHashVal].pnPrev = pnLast;

    DEBUGCHK((pnFirst || !pnLast) && (!pnFirst || pnLast));

    if (pnLast)
    {
    	pnLast->pnNext = &table[uiHashVal];
    }
    else
    {
        pnFirst=&table[uiHashVal];
    }

	pnLast = &table[uiHashVal];

	LeaveCriticalSection(&Trackcs);
    return TRUE;

errret:
	LeaveCriticalSection(&Trackcs);
#endif
	return FALSE;
}

/*
@func   BOOL | DeleteTrackedItem | Deregisters a resource item
@rdesc  TRUE if the item was found and removed, FALSE otherwise.
@parm   DWORD | dwType | TypeID this resource was registered with.
@parm   HANDLE | handle | Handle this resource was registered with
@xref   <f RegisterTrackedItem> <f AddTrackedItem> <f PrintTrackedItem> <f FilterTrackedItem> <f TrackerCallBack>
*/
BOOL SC_DeleteTrackedItem(DWORD dwType, HANDLE handle)
{
#ifdef MEMTRACKING
	UINT uiHashVal, uiRealHash;
	BOOL fFound = FALSE;

	EnterCriticalSection(&Trackcs);

	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
        goto errret;
    }
    if (!uiTablesize && !PerformCallBack4(&cbi,0)){
        ERRORMSG(1, (L"MEMTR: Out of resource tracking memory!\r\n"));
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        goto errret;
    }        
	DEBUGCHK(table);

    // check for type filters - cant check for process filters
    if (dwType>=MAX_REGTYPES || (rgRegTypes[dwType].dwFlags&REGTYPE_MASKED)) {
        goto errret;
    }

    uiRealHash=uiHashVal=MEMTR_hash(dwType,handle);
    
    do {
        if ((table[uiHashVal].handle==handle) && (table[uiHashVal].dwType==dwType) &&
        	!(table[uiHashVal].dwFlags&ITEM_DELETED)) {
            fFound = TRUE;
            break;
        }            
            
        if (++uiHashVal == uiTablesize)
            uiHashVal=0;
    } while (uiRealHash!=uiHashVal);

    if (!fFound)
    {
        DEBUGMSG(ZONE_MEMTRACKER, (L"MEMTR: Delete - invalid handle!\r\n"));
        SetLastError(ERROR_INVALID_TARGET_HANDLE);
        goto errret;
    }

    // check if it has been shown
    if (table[uiHashVal].dwFlags&ITEM_SHOWN) {
        // pend the delete
        table[uiHashVal].dwFlags |= ITEM_DELETED;
        DEBUGMSG(ZONE_MEMTRACKER, (L"Pending Delete handle %8.8lx at spot %8.8lx\r\n", 
            (DWORD)handle, (DWORD)uiHashVal));    
    } else {
        // really delete it
        DEBUGMSG(ZONE_MEMTRACKER, (L"Deleting handle %8.8lx at spot %8.8lx\r\n", 
            (DWORD)handle, (DWORD)uiHashVal));    
        MEMTR_deletenode(table+uiHashVal);
    }

	LeaveCriticalSection(&Trackcs);
    return TRUE;

errret:
	LeaveCriticalSection(&Trackcs);
#endif
	return FALSE;	
}

#ifdef MEMTRACKING

void MEMTR_deletenode(pTrack_Node pn)
{
	if (pn->pnPrev) {			
    	pn->pnPrev->pnNext = pn->pnNext;
    }

	if (pn->pnNext) {			
		pn->pnNext->pnPrev = pn->pnPrev;
	}

    if (pnFirst == pn)
        pnFirst = pn->pnNext;

    if (pnLast == pn)
        pnLast = pn->pnPrev;

	memset(pn, 0, sizeof(Track_Node));
}

// placeholder
int MEMTR_hash(DWORD dwType, HANDLE handle)
{
    return 0;
//	return ((DWORD)handle)%uiTablesize;
}

#ifdef WINCEPROFILE
//---------------------------------------------------------------------------
//	GetEXEProfEntry - returns the PROFentry for the specified process
//---------------------------------------------------------------------------
PROFentry *GetEXEProfEntry(ULONG Index) 
{
	// EXE
	PPROCESS pProc = (PPROCESS)&ProcArray[Index];
	PROFentry *pEntry;
	UINT i, j;

	LPTOCentry pTOCEntry = (LPTOCentry)((LPBYTE)pTOC + sizeof(ROMHDR));
	for(i=0; i < pTOC->nummods; i++, pTOCEntry++) {
		if(pProc->oe.tocptr == pTOCEntry) {
			break;
		}
	}
	if(i >= pTOC->nummods) {
		return NULL;
	}
	
	pEntry = (PROFentry *)pTOC->ulProfileOffset;
	for(j=0; j < pTOC->nummods; j++, pEntry++) {
		if(pEntry->ulModNum == i) {
			return pEntry;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------
//	GetFixedProfEntry - returns the PROFentry for the DLL in which the 
//						address was found.
//---------------------------------------------------------------------------
PROFentry *GetFixedProfEntry(ULONG p) 
{
	// DLL or Kernel
	UINT i;
	PROFentry *pEntry;

	pEntry = (PROFentry *)pTOC->ulProfileOffset;
	for(i=0; i < pTOC->nummods; i++, pEntry++) {
		if(p > pEntry->ulStartAddr && p < pEntry->ulEndAddr) {
			return pEntry;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------
//	GetSymbolText - searches symbol table for passed symbol number
//---------------------------------------------------------------------------
static LPBYTE GetSymbolText(LPBYTE lpSym, DWORD dwSymNum)
{
	while (dwSymNum > 0) {
		while (*lpSym++);
		dwSymNum--;
	}
	return lpSym;
}

//---------------------------------------------------------------------------
//	GetSymbol - searches symbol table for the specified address in the
//			    specified module.  (Returns symbol text + function base addr)
//---------------------------------------------------------------------------
LPSTR GetSymbol(PROFentry *pProfEntry, ULONG p, ULONG *pulFuncAddr) 
{
	UINT i;

	if(pProfEntry->ulNumSym) {
		SYMentry *pSym = (SYMentry *)pProfEntry->ulHitAddress;
		for(i=0; i < pProfEntry->ulNumSym; i++, pSym++) {
			if(p < pSym->ulFuncAddress) {
				break;
			}
		}

		if(i) {
			pSym--;
			*pulFuncAddr = pSym->ulFuncAddress;
			return GetSymbolText((LPBYTE)pProfEntry->ulSymAddress, i-1);
		}
	} 
	*pulFuncAddr = 0;
	return NULL;
}
#endif	// WINCEPROFILE


#pragma optimize ("", off)
void MEMTR_printnode(DWORD dwFlags, pTrack_Node pn)
{
	CALLBACKINFO cbiPrint;
    DWORD Cnt;
    ACCESSKEY ulOldKey;
    SWITCHKEY(ulOldKey,0xffffffff);

    if (dwFlags&PRINT_DETAILS)
    {
    	if (pn->cb) {
    		RETAILMSG(1,(L"  0x%08x:0x%08x(%8.8u):%6.6ld::",
    					 pn->dwProcID, pn->handle, pn->dwTime, pn->dwSize));
    		cbiPrint.hProc = pn->hProc;
    		cbiPrint.pfn = (FARPROC)(pn->cb);
    		cbiPrint.pvArg0 = (PVOID)dwFlags;
    		PerformCallBack (&cbiPrint, pn->dwType, pn->handle, pn->dwProcID,
    					pn->dwFlags&ITEM_DELETED, pn->dwSize, pn->dw1, pn->dw2);
    	} else {
    		RETAILMSG(1,(L"  0x%08x:0x%08x(%8.8u):%6.6ld",
    					 pn->dwProcID, pn->handle, pn->dwTime, pn->dwSize));
    		if (pn->dw1 || pn->dw2) {
		    RETAILMSG(1,(L"::%08lx:%08lx:", pn->dw1, pn->dw2));
		}
		if (pn->dwType == 0 && (pn->dwFlags & ITEM_DELETED) == 0) { // Look for special type 0
		    pTrack_Node pnTest;
		    for (pnTest = pnFirst; pnTest; pnTest = pnTest->pnNext) {
			if ((!(pnTest->dwFlags & ITEM_DELETED)) &&
			    (ULONG)pnTest->handle >= (ULONG)pn->handle &&
			    ((ULONG)pnTest->handle) < ((ULONG)pn->handle) + pn->dwSize &&
			    pnTest != pn) {
			    RETAILMSG(1,(L" %s", rgRegTypes[pnTest->dwType].szName));
			    break;
			}
		    }
		} 
		RETAILMSG(1,(L"\r\n"));
    	}
    }

    if (dwFlags&PRINT_TRACE)
    {
        DWORD Index, temp;
        LPWSTR lpsz;
        LPMODULE lpm;

        for (Cnt=0; Cnt < NUM_TRACK_STACK_FRAMES; Cnt++)
        {
#ifdef WINCEPROFILE
			PROFentry *pProfEntry = NULL;
			ULONG ulAddr = pn->Frames[Cnt] & ((1<<VA_SECTION) - 1);
#endif
			
			if (!(temp = pn->Frames[Cnt] & ~((1<<VA_SECTION) - 1)))
				break;
            for (Index=0; (Index< MAX_PROCESSES) && (ProcArray[Index].dwVMBase != temp); Index++)
            	;

            if (Index>= MAX_PROCESSES)
            {
                lpsz=L"Bad Frame";
                temp=pn->Frames[Cnt];
            }
            else if (lpm=MEMTR_ModuleFromAddress(pn->Frames[Cnt]))
            {
                //This is a DLL
                lpsz=lpm->lpszModName;
                temp=ZeroPtr(pn->Frames[Cnt])-ZeroPtr(lpm->BasePtr);
#ifdef WINCEPROFILE
				pProfEntry = GetFixedProfEntry(ulAddr);
#endif
            }
            else
            {
                lpsz=MEMTR_GetWin32ExeName((PPROCESS)&ProcArray[Index]);
                temp=(DWORD)ZeroPtr(pn->Frames[Cnt])-(DWORD)ZeroPtr(ProcArray[Index].BasePtr);
#ifdef WINCEPROFILE
				pProfEntry = GetEXEProfEntry(Index);
#endif
            }

#ifdef WINCEPROFILE
			{
				ULONG ulFuncAddr;
				LPSTR szSymbol;

				if(pProfEntry && (szSymbol = GetSymbol(pProfEntry, ulAddr, &ulFuncAddr))) {
					RETAILMSG(1, (L"%12.12s!%a+0x%04lx\r\n", lpsz, szSymbol, ulAddr - ulFuncAddr));
				} else {
					RETAILMSG(1, (L"%12.12s!0x%8.8lx\r\n", lpsz, temp));
				}
			}
#else
            if (Cnt)
                RETAILMSG(1, (L", "));
            else    
                RETAILMSG(1, (L"    "));

			if (Cnt && (Cnt%3==2)) 
				RETAILMSG(1, (L"\r\n"));

            RETAILMSG(1, (L"%12.12s!0x%8.8lx", lpsz, temp));
#endif
        }

		RETAILMSG(1,(L"\r\n\r\n"));
    }

	SETCURKEY(ulOldKey);
}
#pragma optimize ("", on)

#define SKIP_FRAMES 2

BOOL MEMTR_GetStackFrames(DWORD lpFrames[])
{
	CONTEXT CurrentContext;
    CONTEXT ContextRecord1;
    ULONG ControlPc;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG HighLimit;
    ULONG LowLimit;
    ULONG NestedFrame;
    ULONG NextPc;
    ULONG PrevSp;
    ULONG Cnt;
	PCALLSTACK pcstk;
	PPROCESS pprcSave;
	ACCESSKEY akySave;

    RtlCaptureContext(&CurrentContext);
	pprcSave = pCurProc;
	akySave = pCurThread->aky;
    NKGetStackLimits(pCurThread, &LowLimit, &HighLimit);
    memcpy(&ContextRecord1, &CurrentContext, sizeof(CONTEXT));
    ControlPc = CONTEXT_TO_PROGRAM_COUNTER(&ContextRecord1);
    NestedFrame = 0;
	pcstk = pCurThread->pcstkTop;
	Cnt = 0;
//	RETAILMSG(1,(L"Frames requested\r\n"));
    do {
	    PrevSp = ContextRecord1.STACKPTR;
        FunctionEntry = LookupFunctionEntry(pCurProc, ControlPc);
        // RETAILMSG(1,(L"Frame %d: %8.8lx\r\n",Cnt,MapPtrProc(ControlPc,pCurProc)));
		if (Cnt >= SKIP_FRAMES)
	    	lpFrames[Cnt-SKIP_FRAMES] = (DWORD)MapPtrProc(ControlPc,pCurProc);
		Cnt++;
        if (FunctionEntry != NULL) {
            NextPc = RtlVirtualUnwind(ControlPc,
                                      FunctionEntry,
                                      &ContextRecord1,
                                      &InFunction,
                                      &EstablisherFrame);
            if ((EstablisherFrame < LowLimit) || (EstablisherFrame > HighLimit) ||
            		((EstablisherFrame & STK_ALIGN) != 0))
                break;
            else if ((!InFunction || !FunctionEntry->ExceptionHandler) && (NextPc != ControlPc))
                PrevSp = 0;	// don't terminate loop because sp didn't change.
        } else {
            NextPc = ContextRecord1.RETADDR - INST_SIZE;
            PrevSp = 0;	// don't terminate loop because sp didn't change.
            if (NextPc == ControlPc)
                break;
        }
        ControlPc = NextPc;
        if (pcstk && (ControlPc == SYSCALL_RETURN || ControlPc+INST_SIZE == SYSCALL_RETURN
                || ControlPc == DIRECT_RETURN || ControlPc+INST_SIZE == DIRECT_RETURN)) {
        	PPROCESS pProc = pcstk->pprcLast;
        	ContextRecord1.RETADDR = (ULONG)pcstk->retAddr;
	        if ((ulong)pProc < 0x10000)
	            SetContextMode(&ContextRecord1, (ULONG)pProc);
	        else
                UpdateASID(pCurThread, pProc, pcstk->akyLast ? pcstk->akyLast : pCurThread->aky);
        	ControlPc = ContextRecord1.RETADDR - INST_SIZE;
        	pcstk = pcstk->pcstkNext;
        }
        else if (pcstk && (DWORD)pcstk < ContextRecord1.STACKPTR && !pcstk->retAddr)
        {
            PPROCESS pProc = pcstk->pprcLast;
            ContextRecord1.RETADDR = (DWORD)ZeroPtr(ControlPc);
            // RETAILMSG(1,(L"*** CallBack4 ControlPc %8.8l ZeroPtr %08x\r\n",ControlPc,ContextRecord1.RETADDR));
            if ((ulong)pProc < 0x10000)
                SetContextMode(&ContextRecord1, (ULONG)pProc);
            else
                UpdateASID(pCurThread, pProc, pcstk->akyLast ? pcstk->akyLast : pCurThread->aky);
            ControlPc = ContextRecord1.RETADDR - INST_SIZE;
            pcstk=pcstk->pcstkNext;
        }
    } while ((ContextRecord1.STACKPTR < HighLimit) && (ContextRecord1.STACKPTR > PrevSp) && (Cnt < NUM_TRACK_STACK_FRAMES + SKIP_FRAMES));
	UpdateASID(pCurThread, pprcSave, akySave);
	return TRUE;
}

extern PROCESS *kdProcArray;
LPMODULE MEMTR_ModuleFromAddress(DWORD dwAdd)
{
    DWORD fa; //Fixedup address
    DWORD ModZa;  //Module's zero based address
    LPMODULE pMod;

    //Make the given address zero based
    fa=ZeroPtr(dwAdd);

    //Walk module list to find this address
    pMod=pModList;							 
    while (pMod) {
		ModZa = (DWORD)pMod->BasePtr-kdProcArray[0].dwVMBase;
		if (fa >= ModZa && fa < ModZa+pMod->e32.e32_vsize) {
		    return pMod;
		}		    
		pMod=pMod->pMod;
    }

    return NULL;
}

static LPWSTR lpszUnknown=L"UNKNOWN";

LPWSTR MEMTR_GetWin32ExeName(PPROCESS pProc)
{
    if (pProc->lpszProcName)
		return pProc->lpszProcName;
	return lpszUnknown;
}

#endif

/*
@func   BOOL | PrintTrackedItem | Prints tracked items
@parm   DWORD | dwFlags | Flags controlling what gets printed. Can be a combination of:
        @flag PRINT_RECENT | Only prints resources allocated or freed since the last checkpoint
        @flag PRINT_SETCHECKPOINT | Sets a checkpoint for all resources dumped in this call
        @flag PRINT_DETAILS | Prints details for the resource items
        @flag PRINT_TRACE | Prints a stack trace for resource items
        @flag PRINT_FILTERTYPE | Only prints resources of this type
        @flag PRINT_FILTERPROCID | Only prints resources for this process id
        @flag PRINT_FILTERHANDLE | Only prints resources with this handle. Typically used while
            specifying the PRINT_FILTERTYPE flag also, since both are required for the handle to
            be unique.  The high word is reserved for application defined values.
@parm   DWORD | dwType | TypeID to filter for if PRINT_FILTERTYPE is specified
@parm   DWORD | dwProcID | ProcID to filter for if PRINT_FILTERPROCID is specified
@parm   HANDLE | handle | Handle to filter for if PRINT_FILTERHANDLE is specified
@comm   For the incremental flag to be useful you must make sure to checkpoint at the appropriate
        points. Note that only resources which meet the filter requirements are checkpointed - 
        in other words if you call this function with the checkpoint flag set and a filter set
        on type 4, then only resource items of that type will be checkpointed. To keep things 
        simple you should probably not use any filters when you are checkpointing. A typical mode
        of usage would be to checkpoint, do some operation, and then do an incremental dump to 
        see if the last operation caused any leaks in the system. Once you feel there is a leak,
        you can use the PRINT_DETAILS flag to see what specific items might have been leaked, and
        then print stack traces for them by using the PRINT_FILTERHANDLE and PRINT_TRACE flags.
@xref   <f RegisterTrackedItem> <f AddTrackedItem> <f DeleteTrackedItem> <f FilterTrackedItem> <f TrackerCallBack>
*/
BOOL SC_PrintTrackedItem(DWORD dwFlags, DWORD dwType, DWORD dwProcID, HANDLE handle)
{
#ifdef MEMTRACKING
  	pTrack_Node pnCur, pnFree;
    DWORD dwCurType;
    DWORD dwSize, dwCount;
  	  	
	EnterCriticalSection(&Trackcs);
	if (!pTOC->ulTrackingLen || !pTOC->ulTrackingStart)
    {
    	ERRORMSG(1, (L"No resource tracking memory!!\r\n"));
        goto errret;
    }
    if (!uiTablesize && !PerformCallBack4(&cbi,0)){
        ERRORMSG(1, (L"MEMTR: Out of resource tracking memory!\r\n"));
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        goto errret;
    }        
	DEBUGCHK(table);

    // Go through all nodes and print as required
 	RETAILMSG(dwFlags&PRINT_DETAILS,(L"LEGEND:- Process:Handle(AllocTime):Size::dwCustom1:dwCustom2\r\n"));
    for (dwCurType=0; rgRegTypes[dwCurType].dwFlags&REGTYPE_INUSE; dwCurType++) {
	    if ((dwFlags&PRINT_FILTERTYPE) && (dwCurType!=dwType)) continue;
	    if (rgRegTypes[dwCurType].dwFlags&REGTYPE_MASKED) continue;
	    RETAILMSG(1, (L"Resource Name: %s(%d)\r\n",
					  rgRegTypes[dwCurType].szName, dwCurType));
	    RETAILMSG(1, (L" Allocated:"));
	    RETAILMSG(dwFlags&PRINT_DETAILS, (L"\r\n"));
	    dwCount = dwSize = 0;
    	for (pnCur = pnFirst; pnCur; pnCur = pnCur->pnNext) {
    	    // do all the filtering
    	    if (pnCur->dwType != dwCurType) continue;
    	    if ((dwFlags&PRINT_FILTERHANDLE) && (pnCur->handle!=handle)) continue;
    	    if ((dwFlags&PRINT_FILTERPROCID) && (pnCur->dwProcID!=dwProcID)) continue;
    	    if ((dwFlags&PRINT_RECENT) && (pnCur->dwFlags&ITEM_SHOWN)) continue;
            if (!(pnCur->dwFlags&ITEM_DELETED)) {
        	    // print the item
        	    if ((dwFlags&PRINT_DETAILS) || (dwFlags&PRINT_TRACE)) {
            		MEMTR_printnode(dwFlags, pnCur);
                }            		
        		dwSize += pnCur->dwSize;
        		dwCount++;
    		}
    		if (dwFlags&PRINT_SETCHECKPOINT) {
        		pnCur->dwFlags |= ITEM_SHOWN;
            }        		
    	}
	    RETAILMSG(1, (L"  Total size: %6.6lu, Num items:%u\r\n", dwSize, dwCount));
	    RETAILMSG(1, (L" Freed    :"));
	    RETAILMSG(dwFlags&PRINT_DETAILS, (L"\r\n"));
	    dwCount = dwSize = 0;
   	    pnFree = NULL;
    	for (pnCur = pnFirst; pnCur; pnCur=pnCur->pnNext) {
    	    // free the last node if necessary
    	    if ((dwFlags&PRINT_SETCHECKPOINT) && pnFree) {
                DEBUGMSG(ZONE_MEMTRACKER, (L"X"));
    	        MEMTR_deletenode(pnFree);
        	    pnFree = NULL;
            }        	    
    	    
    	    // do all the filtering
    	    if (pnCur->dwType != dwCurType) continue;
    	    if ((dwFlags&PRINT_FILTERHANDLE) && (pnCur->handle!=handle)) continue;
    	    if ((dwFlags&PRINT_FILTERPROCID) && (pnCur->dwProcID!=dwProcID)) continue;
    	    if (pnCur->dwFlags&ITEM_DELETED) {
        	    // print the item
        	    if ((dwFlags&PRINT_DETAILS) || (dwFlags&PRINT_TRACE)) {
            		MEMTR_printnode(dwFlags, pnCur);
                }            		
        		dwSize += pnCur->dwSize;
        		dwCount++;
                pnFree = pnCur;
    		} 
    	}
	    RETAILMSG(1, (L"  Total size: %6.6lu, Num items %u\r\n", dwSize, dwCount));
    }

	LeaveCriticalSection(&Trackcs);
    return TRUE;

errret:
	LeaveCriticalSection(&Trackcs);
#endif
	return FALSE;	
}

