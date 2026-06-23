/*		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
#include "kernel.h"

//#define PHYSPAGESTOLEDS 1

#ifdef PHYSPAGESTOLEDS
#define LogPhysicalPages(PageFreeCount) OEMWriteDebugLED(0,PageFreeCount);
#else
#define LogPhysicalPages(PageFreeCount) (0)
#endif

#define STACK_RESERVE		15
#define MIN_PROCESS_PAGES   STACK_RESERVE

#define PAGEOUT_LOW			(( 68*1024)/PAGE_SIZE)	// do pageout once below this mark, reset when above high
#define PAGEOUT_HIGH		((132*1024)/PAGE_SIZE)
#define PAGE_OUT_TRIGGER	(( 24*1024)/PAGE_SIZE)	// page out if below highwater and this much paged in recently

void ZeroPage(void *pvPage);
BOOL ScavengeStacks(int cNeed);

HANDLE GwesOOMEvent;
long GwesLowThreshold;
long GwesCriticalThreshold;
long GwesLowBlockSize;
long GwesCriticalBlockSize;
LPBYTE pFreeKStacks;

extern void FreeSpareBytes(LPBYTE lpb, uint size);

extern CRITICAL_SECTION PhysCS;
extern PTHREAD pCleanupThread;

// PageOutNeeded is set to 1 when the number of free pages drops below
// PageOutTrigger and the cleanup thread's event is signalled. When the
// cleanup thread starts performing page out, it sets the variable to 2.
// When the page free count rises above PageOutLevel, PageOutNeeded is
// reset to 0.
//
// The free page count thresholds are set based upon the Gwes OOM level.

long PageOutNeeded;
long PageOutTrigger;    // Threshold level to start page out.
long PageOutLevel;      // Threshold to stop page out.

/* GWE calls this function to register an Out Of Memory event, which
 * HoldPages sets when free physical memory drops below the given threshold.
 */
VOID SC_SetGwesOOMEvent(HANDLE hEvent, DWORD cpLow, DWORD cpCritical,
						DWORD cpLowBlockSize, DWORD cpCriticalBlockSize) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetGwesOOMEvent entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
		hEvent,cpLow,cpCritical,cpLowBlockSize,cpCriticalBlockSize));
	GwesOOMEvent = hEvent;
	GwesLowThreshold = cpLow;
	GwesCriticalThreshold = cpCritical;
	GwesLowBlockSize = cpLowBlockSize;
	GwesCriticalBlockSize = cpCriticalBlockSize;
	PageOutLevel = GwesLowThreshold + PAGEOUT_HIGH;
	PageOutTrigger = GwesLowThreshold + PAGEOUT_LOW;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetGwesOOMEvent exit\r\n"));
}

#if PAGE_SIZE < 2048

void UnlinkPhysPage(LPBYTE pMem) {
	KCALLPROFON(38);
	if (*(DWORD *)((DWORD)pMem + 0x20000000))
		*(LPBYTE *)(*(DWORD *)((DWORD)pMem + 0x20000000) + 0x20000004) =
			*(LPBYTE *)((DWORD)pMem + 0x20000004);
	if (*(DWORD *)((DWORD)pMem + 0x20000004))
		*(LPBYTE *)(*(DWORD *)((DWORD)pMem + 0x20000004) + 0x20000000) =
			*(LPBYTE *)((DWORD)pMem + 0x20000000);
	else
		LogPtr->pKList = *(LPBYTE *)((DWORD)pMem + 0x20000000);
	*(LPDWORD)(pMem + 0x20000000) = 0;
	*(LPDWORD)(pMem + 0x20000004) = 0;
	KCALLPROFOFF(38);
}

#endif

void LinkPhysPage(LPBYTE pMem) {
	KCALLPROFON(32);
	*(LPBYTE *)((DWORD)pMem + 0x20000000) = LogPtr->pKList;
	*(LPBYTE *)((DWORD)pMem + 0x20000004) = 0;
	if (LogPtr->pKList)
		*(LPBYTE *)((DWORD)LogPtr->pKList + 0x20000004) = pMem;
	LogPtr->pKList = pMem;
    PageFreeCount++;
	LogPhysicalPages(PageFreeCount);
	KCALLPROFOFF(32);
}

LPBYTE GrabFirstPhysPage(DWORD dwCount) {
	PFREEINFO pfi;
    uint ix;
    PHYSICAL_ADDRESS paRet;
	LPBYTE pMem;
	KCALLPROFON(33);
	if (pMem = LogPtr->pKList) {
		paRet = GetPFN(pMem);
		if (LogPtr->pKList = *(LPBYTE *)((DWORD)pMem + 0x20000000))
			*(LPBYTE *)((DWORD)LogPtr->pKList + 0x20000004) = 0;
		*(LPDWORD)(pMem + 0x20000000) = 0;
		*(LPDWORD)(pMem + 0x20000004) = 0;
		pfi = GetRegionFromAddr(paRet);
		ix = (paRet - pfi->paStart) / PFN_INCR;
	    DEBUGCHK(pfi->pUseMap[ix] == 0);
	    DEBUGCHK(dwCount && (dwCount <= 0xff));
		pfi->pUseMap[ix] = (BYTE)dwCount;
	    PageFreeCount--;
		LogPhysicalPages(PageFreeCount);
		DEBUGCHK(PageFreeCount >= 0);
	}
	DEBUGCHK(pMem);
	KCALLPROFOFF(33);
	return pMem;	
}

// called by loader.c when moving memory "slider"

void RemovePage(DWORD dwAddr) {
	DWORD pfn, ix;
	FREEINFO *pfi;
	pfn = GetPFN(dwAddr);
	pfi = GetRegionFromAddr(pfn);
	DEBUGCHK(pfi);
    ix = (pfn - pfi->paStart) / PFN_INCR;
    pfi->pUseMap[ix] = 0xff;
}



BOOL DemandCommit(ulong addr) {
	MEMBLOCK *pmb;
	LPDWORD pPage;
	PFREEINFO pfi;
   	uint ix;
	PHYSICAL_ADDRESS paRet;
	LPBYTE pMem;
	pmb = (*SectionTable[addr>>VA_SECTION])[(addr>>VA_BLOCK) & BLOCK_MASK];
	
	if (!*(pPage = &pmb->aPages[(addr>>VA_PAGE) & PAGE_MASK])) {
		pMem = LogPtr->pKList;

		if (GwesOOMEvent && (PageFreeCount < GwesCriticalThreshold)) {
			SetEvent(GwesOOMEvent);
		}
        
		if (!PageFreeCount || !pMem) {
			RETAILMSG(1,(L"--->>> DemandCommit: FATAL ERROR!  COMPLETELY OUT OF MEMORY (%8.8lx %8.8lx)!\r\n",pMem,PageFreeCount));
			INTERRUPTS_OFF();
            while (1) {
                // we don't recover if there's no memory, so better off to just stop
            }
			return FALSE;
		}
		paRet = GetPFN(pMem);
		pMem = (LPBYTE)((DWORD)pMem + 0x20000000);
		if (LogPtr->pKList = *(LPBYTE *)pMem)
			*(LPBYTE *)((DWORD)LogPtr->pKList + 0x20000004) = 0;
		*(LPDWORD)pMem = 0;
		*(LPDWORD)(pMem + 4) = 0;
		pfi = GetRegionFromAddr(paRet);
		ix = (paRet - pfi->paStart) / PFN_INCR;
    	DEBUGCHK(!pfi->pUseMap[ix]);
		pfi->pUseMap[ix] = 1;
		*pPage = paRet | PG_READ_WRITE;
    	PageFreeCount--;
		LogPhysicalPages(PageFreeCount);
		DEBUGCHK(PageFreeCount >= 0);
		return TRUE;
	}
	return FALSE;
}

void EnterPhysCS(void) {
    LPVOID pMem;
	EnterCriticalSection(&PhysCS);
	DEBUGCHK(PhysCS.OwnerThread == hCurThread);
	if (PhysCS.LockCount == 1)
	    while (pMem = InterlockedPopList(&pFreeKStacks))
			FreeHelperStack(pMem);
}

BOOL SC_GiveKPhys(void *ptr, ulong length) {
	BOOL bRet = FALSE;
	LPBYTE pPage;
	DWORD loop;
	PFREEINFO pfi;
	PHYSICAL_ADDRESS paPFN;
	uint ix;
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_GiveKPhys entry: %8.8lx %8.8lx\r\n",ptr,length));
	SC_CacheSync(CACHE_SYNC_DISCARD);
	EnterPhysCS();
	for (loop = 0; loop < length; loop++) {
		pPage = *((LPBYTE *)ptr+loop);
		ZeroPage(pPage + 0x20000000);
		paPFN = GetPFN(pPage);
		pfi = GetRegionFromAddr(paPFN);
		DEBUGCHK(pfi);
        ix = (paPFN - pfi->paStart) / PFN_INCR;
        DEBUGCHK(pfi->pUseMap[ix] == 0xff);
        pfi->pUseMap[ix] = 0;
		KCall((PKFN)LinkPhysPage,pPage);
    }
	KInfoTable[KINX_NUMPAGES] += length;
    // If there are enough free pages, clear the PageOutNeeded flag.
    if (PageFreeCount > PageOutLevel)
        PageOutNeeded = 0;
	bRet = TRUE;
	LeaveCriticalSection(&PhysCS);
	DEBUGMSG(ZONE_ENTRY,(L"SC_GiveKPhys exit: %8.8lx\r\n",bRet));
	return bRet;
}

BOOL SC_GetKPhys(void *ptr, ulong length) {
	LPVOID *pPages;
	BOOL bRet = FALSE;
    DWORD dwCount;
    LPBYTE pMem;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetKPhys entry: %8.8lx %8.8lx\r\n",ptr,length));
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	pPages = (LPVOID *)ptr;
	EnterPhysCS();
	ScavengeStacks(100000);     // Reclaim all extra stack pages.
	if ((length == 1) || ((DWORD)PageFreeCount > length + PageOutTrigger)) {
		for (dwCount = length; dwCount; dwCount--) {
			if (!(pMem = (LPBYTE)KCall((PKFN)GrabFirstPhysPage,0xff))) {
				LeaveCriticalSection(&PhysCS);
				SC_GiveKPhys(ptr,length - dwCount);
				DEBUGCHK(0);
				RETAILMSG(1,(L"Error getting pages!\r\n"));
				return 0;
			}
			KInfoTable[KINX_NUMPAGES]--;
			*pPages++ = pMem;
	    }
	    bRet = TRUE;
	}
	LeaveCriticalSection(&PhysCS);
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetKPhys exit: %8.8lx\r\n",bRet));
	return bRet;
}
	
void InitMemoryPool() {
	PFREEINFO pfi, pfiEnd;
	DWORD loop;
	LPBYTE pPage;
	/* Fill in data fields in user visible kernel data page */
	KInfoTable[KINX_PROCARRAY] = (long)ProcArray;
	KInfoTable[KINX_PAGESIZE] = PAGE_SIZE;
#ifdef PFN_SHIFT
	KInfoTable[KINX_PFN_SHIFT] = PFN_SHIFT;
#endif
	KInfoTable[KINX_PFN_MASK] = (DWORD)PFNfromEntry(~0);
	KInfoTable[KINX_SECTIONS] = (long)SectionTable;
	KInfoTable[KINX_MEMINFO] = (long)&MemoryInfo;
	KInfoTable[KINX_KDATA_ADDR] = (long)&KData;
	SC_CacheSync(CACHE_SYNC_DISCARD);
	for (pfi = MemoryInfo.pFi, pfiEnd = pfi+MemoryInfo.cFi ; pfi < pfiEnd ; ++pfi) {
		DEBUGMSG(ZONE_MEMORY, (TEXT("InitMemoryPool: Init range: map=%8.8lx start=%8.8lx end=%8.8lx\r\n"),
				pfi->pUseMap, pfi->paStart, pfi->paEnd));
		pPage = 0;
		for (loop = pfi->paStart; loop < pfi->paEnd; loop += PFN_INCR) {
			if (!pfi->pUseMap[(loop-pfi->paStart)/PFN_INCR]) {
				pPage = Phys2Virt(loop);
				ZeroPage(pPage + 0x20000000);
				LinkPhysPage(pPage);
				LogPtr->pKList = pPage;
	    	    KInfoTable[KINX_NUMPAGES]++;
			}
		}
	}
    DEBUGMSG(ZONE_MEMORY,(TEXT("InitMemoryPool done, free=%d\r\n"),PageFreeCount));
}

PTHREAD PthScavTarget;
PPROCESS PprcScavTarget = &ProcArray[0];
int StackScavCount;

HTHREAD ScavengeOnePage(void) {
	HTHREAD ret;
	KCALLPROFON(13);
    if (!PthScavTarget) {
	    if (++PprcScavTarget >= &ProcArray[MAX_PROCESSES])
			PprcScavTarget = ProcArray;
	   	++StackScavCount;
	    if (!PprcScavTarget->dwVMBase || !(PthScavTarget = PprcScavTarget->pTh)) {
	    	KCALLPROFOFF(13);
	    	return 0;
	    }
	    if (PthScavTarget == pCurThread)
	    	PthScavTarget = pCurThread->pNextInProc;
	    KCALLPROFOFF(13);
	    return 0;
    }
    if (HandleToThread(PthScavTarget->hTh) != PthScavTarget) {
	    PthScavTarget = 0;
    	KCALLPROFOFF(13);
    	return 0;
    }
    ret = ((pCurThread != PthScavTarget) && MDTestStack(PthScavTarget)) ? PthScavTarget->hTh : 0;
    if ((PthScavTarget = PthScavTarget->pNextInProc) == pCurThread)
    	PthScavTarget = pCurThread->pNextInProc;
	KCALLPROFOFF(13);
    return ret;
}

HANDLE hCurrScav;

BOOL ScavengeStacks(int cNeed) {
    HTHREAD hth;
    PTHREAD pth;
	ulong addr, pages, base;
    ulong pte;
    int ixPage, ixBlock;
    PSECTION pscn;
    MEMBLOCK *pmb;
    ACCESSKEY ulOldKey;
    DEBUGMSG(ZONE_MEMORY, (TEXT("Scavenging stacks for %d pages.\r\n"), cNeed-PageFreeCount));
    StackScavCount = 0;
    SWITCHKEY(ulOldKey,0xffffffff);
    while ((StackScavCount <= MAX_PROCESSES) && (cNeed > PageFreeCount)) {
    	if (hth = (HTHREAD)KCall((PKFN)ScavengeOnePage)) {
        	if (SuspendThread(hth) != -1) {
        		hCurrScav = hth;
        		pages = 0;
        		pth = HandleToThread(hth);
				SC_CacheSync(CACHE_SYNC_DISCARD);
				while (addr = MDTestStack(pth)) {
				    pscn = SectionTable[addr>>VA_SECTION];
				    ixBlock = (addr>>VA_BLOCK) & BLOCK_MASK;
				    ixPage = (addr>>VA_PAGE) & PAGE_MASK;
				    if ((pmb = (*pscn)[ixBlock]) == NULL_BLOCK || pmb == RESERVED_BLOCK ||
						!(pmb->flags&MB_FLAG_AUTOCOMMIT) || pmb == PmbDecommitting ||
						!(pte = pmb->aPages[ixPage]) || pte == BAD_PAGE)
						break;
			        pmb->aPages[ixPage] = 0;
		            MDShrinkStack(pth);
					FreePhysPage(PFNfromEntry(pte));
		            DEBUGCHK(!pages || (base == addr - PAGE_SIZE));
		            base = addr;
		            pages++;
				}
				if (pages)
		            InvalidateRange((PVOID)base, PAGE_SIZE*pages);
        		hCurrScav = 0;
				ResumeThread(hth);
			}
		}
	}
	SETCURKEY(ulOldKey);
	return (StackScavCount > MAX_PROCESSES) ? FALSE : TRUE;
}

PHYSICAL_ADDRESS GetHeldPage(void) {
    PHYSICAL_ADDRESS paRet;
	LPBYTE pMem;
	if (pMem = (LPBYTE)KCall((PKFN)GrabFirstPhysPage,1)) {
	    PageFreeCount++; // since we already reserved it
		LogPhysicalPages(PageFreeCount);
		paRet = GetPFN(pMem);
	} else
		paRet = 0;
	DEBUGMSG(ZONE_PHYSMEM,(TEXT("GetHeldPage: Returning %8.8lx\r\n"), paRet));
    return paRet;
}

BOOL HoldPages(int cpNeed, BOOL bForce) {
	BOOL fSetGweOomEvent = FALSE;
	BOOL bRet = FALSE;
	WORD prio;
    EnterPhysCS();
    // Check if this request will drop the page free count below the
    // page out trigger and signal the clean up thread to start doing
    // pageouts if so.
    if ((cpNeed+PageOutTrigger > PageFreeCount) &&
    	(!PageOutNeeded || (PagedInCount > PAGE_OUT_TRIGGER))) {
        PageOutNeeded = 1;
        PagedInCount = 0;
        if (prio = GET_CPRIO(pCurThread))
        	prio--;
        if (prio < GET_CPRIO(pCleanupThread))
	        KCall((PKFN)SetThreadBasePrio, pCleanupThread->hTh, (DWORD)prio);
	    SetEvent(hAlarmThreadWakeup);
    }
    do {
        if (cpNeed+GwesLowThreshold <= PageFreeCount) {
        	DWORD pfc;
        	do {
				pfc = PageFreeCount;
			} while (InterlockedTestExchange(&PageFreeCount,pfc,pfc-cpNeed) != (LONG)pfc);
			LogPhysicalPages(PageFreeCount);
			bRet = TRUE;
			goto hpDone;
        }
    } while (ScavengeStacks(cpNeed+GwesLowThreshold));

	// Even after scavenging stacks, we were unable to satisfy the request
	// without going below the GWE low threshold.

	// Do not allow a request of size GwesLowBlockSize to succeed if
	// doing so would leave less than the low threshold.  Same with
	// GwesCriticalBlockSize and GwesCriticalThreshold.
	if (bForce || !((cpNeed > GwesLowBlockSize
	        && cpNeed+GwesLowThreshold > PageFreeCount)
		    || (cpNeed > GwesCriticalBlockSize
		    && cpNeed + GwesCriticalThreshold > PageFreeCount))) {

      	// Memory is low. Notify GWE, so that GWE can ask
    	// the user to close some apps.
    	if (GwesOOMEvent &&
			((PageFreeCount >= GwesLowThreshold) ||
			 ((PageFreeCount < cpNeed + GwesCriticalThreshold))))
           	fSetGweOomEvent = TRUE;
        
        if ((cpNeed + (bForce?0:STACK_RESERVE)) <= PageFreeCount) {
        	DWORD pfc;
        	do {
				pfc = PageFreeCount;
			} while (InterlockedTestExchange(&PageFreeCount,pfc,pfc-cpNeed) != (LONG)pfc);
			LogPhysicalPages(PageFreeCount);
        	bRet = TRUE;
        }
	}
hpDone:
    LeaveCriticalSection(&PhysCS);
	if (fSetGweOomEvent)
		SetEvent(GwesOOMEvent);
    return bRet;
}

void DupPhysPage(PHYSICAL_ADDRESS paPFN) {
	PFREEINFO pfi;
    uint ix;

    //NOTE: DupPhysage and FreePhysPage are only called by the virtual memory
    // system with the VA critical section held so DupPhysPage does not need to
    // claim the physical memory critical section because it does not change
    // the page free count.
	if ((pfi = GetRegionFromAddr(paPFN)) != 0) {
        ix = (paPFN - pfi->paStart) / PFN_INCR;
        DEBUGCHK(pfi->pUseMap[ix] != 0);
        DEBUGMSG(ZONE_PHYSMEM, (TEXT("DupPhysPage: PFN=%8.8lx ix=%x rc=%d\r\n"), paPFN,
                ix, pfi->pUseMap[ix]));
        ++pfi->pUseMap[ix];
    }
}

void FreePhysPage(PHYSICAL_ADDRESS paPFN) {
	PFREEINFO pfi;
	uint ix;

	if ((pfi = GetRegionFromAddr(paPFN)) != 0) {
        ix = (paPFN - pfi->paStart) / PFN_INCR;
        DEBUGMSG(ZONE_PHYSMEM, (TEXT("FreePhysPage: PFN=%8.8lx ix=%x rc=%d\r\n"), paPFN,
                ix, pfi->pUseMap[ix]));
        EnterPhysCS();
        DEBUGCHK(pfi->pUseMap[ix] != 0);
        if ((pfi->pUseMap[ix] != 0xff) && (!--pfi->pUseMap[ix])) {
            DEBUGMSG(ZONE_PHYSMEM, (TEXT("FreePhysPage: PFN=%8.8lx\r\n"), paPFN));
            ZeroPage((LPBYTE)Phys2Virt(paPFN)+0x20000000);
            KCall((PKFN)LinkPhysPage,Phys2Virt(paPFN));
            // If there are enough free pages, clear the PageOutNeeded flag.
            if (PageFreeCount > PageOutLevel)
                PageOutNeeded = 0;
        }
        LeaveCriticalSection(&PhysCS);
    }
}

ERRFALSE(sizeof(SECTION) == 2048);

#if PAGE_SIZE == 4096
PSECTION GetSection(void)
{
	PHYSICAL_ADDRESS pAddr;
    if ((PageFreeCount > 1+MIN_PROCESS_PAGES) && HoldPages(1, FALSE)) {
    	if (pAddr = GetHeldPage()) {
			InterlockedIncrement(&KInfoTable[KINX_SYSPAGES]);
    	    return Phys2Virt(pAddr); // can't put GetHeldPage, since Phys2Virt's a macro!
    	}
    	InterlockedIncrement(&PageFreeCount);
    }
    return 0;
}

void FreeSection(PSECTION pscn) {
	SC_CacheSync(CACHE_SYNC_DISCARD);
    FreePhysPage(GetPFN(pscn));
	InterlockedDecrement(&KInfoTable[KINX_SYSPAGES]);
}

LPVOID GetHelperStack(void) {
	return AllocMem(HEAP_HLPRSTK);
//	return GetSection();
}

void FreeHelperStack(LPVOID pMem) {
	FreeMem(pMem,HEAP_HLPRSTK);
//	FreeSection(pMem);
}

#else

ERRFALSE((sizeof(SECTION)+PAGE_SIZE-1)/PAGE_SIZE == 2);

void FreeSection(PSECTION pscn) {
    PHYSICAL_ADDRESS paSect;
    paSect = GetPFN(pscn);
	SC_CacheSync(CACHE_SYNC_DISCARD);
    FreePhysPage(paSect);
    FreePhysPage(NextPFN(paSect));
	InterlockedDecrement(&KInfoTable[KINX_SYSPAGES]);
	InterlockedDecrement(&KInfoTable[KINX_SYSPAGES]);
}

LPBYTE TakeTwoPages(PFREEINFO pfi, uint ix, LPBYTE pMem) {
	KCALLPROFON(40);
	if (pfi->pUseMap[ix] || pfi->pUseMap[ix+1]) {
		KCALLPROFOFF(40);
		return 0;
	}
	pfi->pUseMap[ix] = 1;
    pfi->pUseMap[ix+1] = 1;
    UnlinkPhysPage(pMem);
    UnlinkPhysPage(pMem+PAGE_SIZE);
	KCALLPROFOFF(40);
    return pMem;
}

PSECTION GetSection(void) {
	PFREEINFO pfi, pfiEnd;
    uint ix;
    PHYSICAL_ADDRESS paSect;
    PSECTION pscn = 0;
    LPVOID pMem;
    while (pMem = InterlockedPopList(&pFreeKStacks))
		FreeHelperStack(pMem);
    if ((PageFreeCount > 2+MIN_PROCESS_PAGES) && HoldPages(2, FALSE)) {
    	pfi = &MemoryInfo.pFi[0];
   	    for (pfi = &MemoryInfo.pFi[0], pfiEnd = pfi+MemoryInfo.cFi ; pfi < pfiEnd ; ++pfi) {
	   		paSect = pfi->paEnd - 2 * PFN_INCR;
	   	    ix = (paSect - pfi->paStart) / PFN_INCR;
	   	    while (paSect >= pfi->paStart) {
    			if (pscn = (PSECTION)KCall((PKFN)TakeTwoPages,pfi,ix,Phys2Virt(paSect))) {
        			InterlockedIncrement(&KInfoTable[KINX_SYSPAGES]);
        			InterlockedIncrement(&KInfoTable[KINX_SYSPAGES]);
        			goto foundPages;
        		}
				ix--;
				paSect -= PFN_INCR;
	   	    }
		}
		InterlockedIncrement(&PageFreeCount);
		InterlockedIncrement(&PageFreeCount);
	}
foundPages:
	DEBUGMSG(ZONE_PHYSMEM,(TEXT("GetSection: Returning %8.8lx\r\n"), pscn));
    return pscn;
}

LPVOID GetHelperStack(void) {
	PHYSICAL_ADDRESS pAddr;
    if (HoldPages(1, FALSE)) {
    	if (pAddr = GetHeldPage()) {
			InterlockedIncrement(&KInfoTable[KINX_SYSPAGES]);
    		return Phys2Virt(pAddr); // can't put GetHeldPage, since Phys2Virt's a macro!
    	}
    	InterlockedIncrement(&PageFreeCount);
    }
    return 0;
}

void FreeHelperStack(LPVOID pMem) {
	SC_CacheSync(CACHE_SYNC_DISCARD);
    FreePhysPage(GetPFN(pMem));
	InterlockedDecrement(&KInfoTable[KINX_SYSPAGES]);
}

#endif

