/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

#include "kernel.h"

int FirstFreeSection = RESERVED_SECTIONS;

MEMBLOCK *PmbDecommitting;
extern CRITICAL_SECTION VAcs;
extern HANDLE GwesOOMEvent;
extern long GwesCriticalThreshold;

const DWORD AllocationType[MB_FLAG_PAGER_TYPE+1] = {MEM_PRIVATE, MEM_IMAGE, MEM_MAPPED};

ulong MakePagePerms(DWORD fdwProtect) {
	ulong ulMask;
	switch (fdwProtect & ~(PAGE_GUARD | PAGE_NOCACHE
#ifdef ARM
    		| PAGE_ARM_MINICACHE
#endif
#ifdef x86
			| PAGE_x86_WRITETHRU
#endif
			)) {
	    case PAGE_READONLY:
	        ulMask = PG_VALID_MASK | PG_PROT_READ;
	        break;
	    case PAGE_EXECUTE:
	    case PAGE_EXECUTE_READ:
	        ulMask = PG_VALID_MASK | PG_PROT_READ | PG_EXECUTE_MASK;
	        break;
	    case PAGE_EXECUTE_READWRITE:
	        ulMask = PG_VALID_MASK | PG_DIRTY_MASK | PG_EXECUTE_MASK | PG_PROT_WRITE;
	        break;
	    case PAGE_READWRITE:
	        ulMask = PG_VALID_MASK | PG_DIRTY_MASK | PG_PROT_WRITE;
	        break;
	    case PAGE_WRITECOPY:
	    case PAGE_EXECUTE_WRITECOPY:
	        KSetLastError(pCurThread, ERROR_NOT_SUPPORTED);
	        return 0;
	    case PAGE_NOACCESS:
	        if (fdwProtect != PAGE_NOACCESS) {
	            KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
	            return 0;
	        }
            ulMask = PG_GUARD;	/* dummy return value */
	        break;
	    default:
	        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
	        return 0;
    }
    DEBUGCHK(ulMask);
    if (fdwProtect & PAGE_GUARD)
   		ulMask = (ulMask & ~PG_VALID_MASK) | PG_GUARD;
#ifdef ARM
    ulMask |= (fdwProtect & PAGE_NOCACHE) ? PG_NOCACHE : (fdwProtect & PAGE_ARM_MINICACHE) ? PG_MINICACHE : PG_CACHE;
#elif defined(x86)
    ulMask |= (fdwProtect & PAGE_NOCACHE) ? PG_NOCACHE : (fdwProtect & PAGE_x86_WRITETHRU) ? PG_WRITE_THRU_MASK : PG_CACHE;
#else
    ulMask |= (fdwProtect & PAGE_NOCACHE) ? PG_NOCACHE : PG_CACHE;
#endif
    ulMask |= PG_SIZE_MASK;
	DEBUGMSG(ZONE_VIRTMEM, (TEXT("MakePagePerms: %8.8lx returns %8.8lx\r\n"), fdwProtect, ulMask));
    return ulMask;
}

DWORD ProtectFromPerms(ulong ulPage) {
    DWORD fdwProt;
    switch (ulPage & (PG_PROTECTION|PG_EXECUTE_MASK)) {
	    case PG_PROT_READ:
	        fdwProt = PAGE_READONLY;
	        break;
	    case PG_PROT_WRITE:
	        fdwProt = PAGE_READWRITE;
	        break;
#if PG_EXECUTE_MASK
	    case PG_EXECUTE_MASK|PG_PROT_READ:
	        fdwProt = PAGE_EXECUTE_READ;
	        break;
	    case PG_EXECUTE_MASK|PG_PROT_READ|PG_PROT_WRITE:
	        fdwProt = PAGE_EXECUTE_READWRITE;
	        break;
#endif
	    default: // invalid combinations get mapped to PAGE_NOACCESS
	        return PAGE_NOACCESS;
    }
    if ((ulPage & (PG_VALID_MASK|PG_GUARD)) == PG_GUARD)
        fdwProt |= PAGE_GUARD;
    if ((ulPage & PG_CACHE_MASK) == PG_NOCACHE)
        fdwProt |= PAGE_NOCACHE;
    return fdwProt;
}

int FindFirstBlock(PSECTION pscn, int ix) {
    MEMBLOCK *pmb;
    do {
        if (ix < 0 || (pmb = (*pscn)[ix]) == NULL_BLOCK)
            return UNKNOWN_BASE;
        --ix;
    } while (pmb == RESERVED_BLOCK);
    return pmb->ixBase;
}

int ScanRegion(
PSECTION pscn,          /* section array */
int ixFirB,             /* first block in region */
int ixBlock,            /* starting block to scan */
int ixPage,             /* starting page in block */
int cPgScan,            /* # of pages to scan */
int *pcPages)           /* ptr to count of pages in region */
{
    register int cPages;
    int cpAlloc;
    register MEMBLOCK *pmb;
    DWORD err;
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("Scanning %d pages. ixFirB=%x\r\n"), cPgScan, ixFirB));
    cpAlloc = 0;
    for (cPages = 0 ; cPages < cPgScan && ixBlock < BLOCK_MASK+1 ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("Scanning block %8.8lx (%x), ix=%d cPgScan=%d\r\n"),
                pmb, (pmb!=RESERVED_BLOCK&&pmb!=NULL_BLOCK)?pmb->ixBase:-1, ixPage, cPgScan));
        if (pmb == RESERVED_BLOCK) {
            /* Reserved block. If not just counting pages, allocate a new block
             * and initialize it as part of the current region.
             */
            if (!pcPages) {
                if (!(pmb = AllocMem(HEAP_MEMBLOCK))) {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    goto setError;
                }
	        	memset(pmb,0,sizeof(MEMBLOCK));
                pmb->alk = (*pscn)[ixFirB]->alk;
                pmb->flags = (*pscn)[ixFirB]->flags;
                pmb->hPf = (*pscn)[ixFirB]->hPf;
                pmb->cUses = 1;
                pmb->ixBase = ixFirB;
                (*pscn)[ixBlock] = pmb;
                DEBUGMSG(ZONE_VIRTMEM, (TEXT("ScanRegion: created block %8.8lx, ix=%d\r\n"), pmb, ixBlock));
            }
            ixPage = PAGES_PER_BLOCK - ixPage;	/* # of pages relevant to scan */
            if ((cPages += ixPage) > cPgScan) {
                cpAlloc += cPgScan - (cPages - ixPage);
                cPages = cPgScan;
            } else
                cpAlloc += ixPage;
        } else if (pmb == NULL_BLOCK || pmb->ixBase != ixFirB) {
            /* This block is not part of the orginal region. If not just
             * counting pages, then fail the request.
             */
            if (pcPages)
                break;      /* just counting, so not an error */
            err = ERROR_INVALID_PARAMETER;
            goto setError;
        } else {
            /* Scan block to count uncommited pages and to verify that the
             * protection on any existing pages is compatible with the protection
             * being requested.
             */
            for ( ; cPages < cPgScan && ixPage < PAGES_PER_BLOCK
                    ; ++ixPage, ++cPages) {
                if (pmb->aPages[ixPage] == BAD_PAGE) {
                    if (pcPages)
                        goto allDone;
                    err = ERROR_INVALID_PARAMETER;
                    goto setError;
                } else if (pmb->aPages[ixPage] == 0)
                    ++cpAlloc;      /* count # of pages to allocate */
            }
        }
        ixPage = 0;            /* start with first page of next block */
    }
allDone:
    if (pcPages)
        *pcPages = cPages;      /* return # of pages in the region */
    return cpAlloc;

setError:
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("ScanRegion failed err=%d\r\n"), err));
    KSetLastError(pCurThread, err);
    return -1;
}

void
DecommitPages(
PSECTION pscn,          /* section array */
int ixBlock,            /* starting block to decommit */
int ixPage,             /* starting page in block */
int cPages,             /* # of pages to decommit */
DWORD baseScn,			/* base address of section */
BOOL bIgnoreLock)		/* ignore lockcount when decommitting */
{
    register MEMBLOCK *pmb;
    PHYSICAL_ADDRESS paPFN;
#ifdef x86
    int cbInvalidate = cPages * PAGE_SIZE;
    PVOID pvInvalidate = (PVOID)(baseScn + (ixBlock<<VA_BLOCK) + (ixPage<<VA_PAGE));
#endif
    /* Walk the page tables to free the physical pages. */
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("DecommitPages: %d pages from %3.3x%x000\r\n"), cPages, ixBlock, ixPage));
	SC_CacheSync(CACHE_SYNC_DISCARD);
    for (; cPages > 0 ; ++ixBlock) {
        DEBUGCHK(ixBlock < BLOCK_MASK+1);
        if ((pmb = (*pscn)[ixBlock]) == RESERVED_BLOCK)
            cPages -= PAGES_PER_BLOCK - ixPage;
        else {
            DEBUGCHK(pmb != NULL_BLOCK);
            DEBUGMSG(ZONE_VIRTMEM,(TEXT("Decommiting block (%8.8lx) uc=%d ixPg=%d cPages=%d\r\n"),
                    pmb, pmb->cUses, ixPage, cPages));
            if (pmb->cUses > 1 && ixPage == 0 && (cPages >= PAGES_PER_BLOCK || pmb->aPages[cPages] == BAD_PAGE)) {
                /* Decommiting a duplicated block. Remove the additional
                 * use of the block and change the block entry to ReservedBlock */
                (*pscn)[ixBlock] = RESERVED_BLOCK;
                --pmb->cUses;
                cPages -= PAGES_PER_BLOCK;
            } else if (bIgnoreLock || !pmb->cLocks) {
                PmbDecommitting = pmb;
                for ( ; cPages && ixPage < PAGES_PER_BLOCK ; ++ixPage, --cPages) {
                    if ((paPFN = pmb->aPages[ixPage]) != 0 && paPFN != BAD_PAGE) {
                        paPFN = PFNfromEntry(paPFN);
                        DEBUGMSG(ZONE_VIRTMEM, (TEXT("Decommitting %8.8lx @%3.3x%x000\r\n"), paPFN, ixBlock, ixPage));
                        pmb->aPages[ixPage] = 0;
                        FreePhysPage(paPFN);
                    }
                }
                PmbDecommitting = 0;
            } else {
				cPages -= PAGES_PER_BLOCK - ixPage;
				DEBUGMSG(1,(L"DecommitPages: Cannot decommit block at %8.8lx, lock count %d\r\n",
					(PVOID)(baseScn + (ixBlock<<VA_BLOCK) + (ixPage<<VA_PAGE)),pmb->cLocks));
			}
        }
        ixPage = 0;            /* start with first page of next block */
    }
    // Since pages have been removed from the page tables, it is necessary to flush the TLB.
    InvalidateRange(pvInvalidate, cbInvalidate);
}

void
ReleaseRegion(
PSECTION pscn,          /* section array */
int ixFirB)             /* first block in region */
{
    register MEMBLOCK *pmb;
    register int ixBlock;
    /* Walk the section array to free the MEMBLOCK entries. */
    for (ixBlock = ixFirB ; ixBlock < BLOCK_MASK+1 ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        if (pmb == RESERVED_BLOCK)
            (*pscn)[ixBlock] = NULL_BLOCK;
        else if (pmb == NULL_BLOCK || pmb->ixBase != ixFirB)
            break;
        else {
#ifdef DEBUG
            int ix;
            ulong ulPFN;
            for (ix = 0 ; ix < PAGES_PER_BLOCK ; ++ix) {
                if ((ulPFN = pmb->aPages[ix]) != 0 && ulPFN != BAD_PAGE) {
                    DEBUGMSG(ZONE_VIRTMEM, (TEXT("ReleaseRegion: Commited page found: %8.8lx @%3.3x%x000\r\n"),
                    	ulPFN, ixBlock, ix));
                    DEBUGCHK(0);
                }
            }
#endif
            DEBUGMSG(ZONE_VIRTMEM, (TEXT("Releasing memblock %8.8lx @%3.3x0000\r\n"), pmb, ixBlock));
            (*pscn)[ixBlock] = NULL_BLOCK;
            FreeMem(pmb,HEAP_MEMBLOCK);
        }
    }
    /*Note: Since no pages are freed by this routine, no TLB flushing is needed */
}

/** IsAccessOK() - check access permissions for Address
 *
 *  This function checks the access permissions for an address. For user
 * virtual addresses, only the access key for the memory region is checked.
 * Kernel space addresses are always allowed because access to them is not
 * access key dependant.
 *
 * This function doesn't check that the page is either present or valid.
 *
 * Environment:
 *   Kernel mode, preemtible, running on the thread's stack.
 */
BOOL IsAccessOK(void *addr, ACCESSKEY aky) {
	register MEMBLOCK *pmb;
	PSECTION pscn;
	int ixSect, ixBlock;
	ixBlock = (ulong)addr>>VA_BLOCK & BLOCK_MASK;
	ixSect = (ulong)addr>>VA_SECTION;
	if (ixSect <= SECTION_MASK) {
	    pscn = SectionTable[ixSect];
		if ((pmb = (*pscn)[ixBlock]) != NULL_BLOCK) {
    		if (pmb == RESERVED_BLOCK)
    			pmb = (*pscn)[FindFirstBlock(pscn, ixBlock)];
		    if (!TestAccess(&pmb->alk, &aky)) {
            	DEBUGMSG(1, (TEXT("IsAccessOK returning FALSE\r\n")));
            	return FALSE;
		    }
    	}
	}
    return TRUE;
}

PVOID DbgVerify(PVOID pvAddr, int option) {
    PVOID ret;
    int flVerify = VERIFY_KERNEL_OK;
    int flLock = LOCKFLAG_QUERY_ONLY | LOCKFLAG_READ;
    switch (option) {
	    case DV_MODIFY:
    	    flVerify = VERIFY_KERNEL_OK | VERIFY_WRITE_FLAG;
        	flLock = LOCKFLAG_QUERY_ONLY | LOCKFLAG_WRITE;
        	// fall through
	    case DV_PROBE:
    	    if (!(ret = VerifyAccess(pvAddr, flVerify, (ACCESSKEY)-1)))
        	    if (((ulong)pvAddr & 0x80000000) == 0 && !InSysCall() && LockPages(pvAddr, 1, 0, flLock))
        	    	ret = VerifyAccess(pvAddr, flLock, (ACCESSKEY)-1);
	        break;
	    case DV_SETBP:
    	    if (((ulong)pvAddr & 0x80000000) != 0 || (!InSysCall() && LockPages(pvAddr, 1, 0, LOCKFLAG_READ)))
	        	ret = VerifyAccess(pvAddr, VERIFY_KERNEL_OK, (ACCESSKEY)-1);
    	    else
        	    ret = 0;
	        break;
	    case DV_CLEARBP:
    	    if (((ulong)pvAddr & 0x80000000) == 0 && !InSysCall())
        	    UnlockPages(pvAddr, 1);
	        ret = 0;
    	    break;
    }
    return ret;
}

VOID DeleteSection(LPVOID lpvSect) {
    PSECTION pscn;
    int ix;
    int ixScn;
    DEBUGMSG(ZONE_VIRTMEM,(TEXT("DeleteSection %8.8lx\r\n"), lpvSect));
	EnterCriticalSection(&VAcs);
    if ((ixScn = (ulong)lpvSect>>VA_SECTION) <= SECTION_MASK
            && ixScn >= RESERVED_SECTIONS
            && (pscn = SectionTable[ixScn]) != NULL_SECTION) {
        // For process sections, start freeing at block 1. Otherwise, this
        // should be a mapper section which starts freeing at block 0.
        ix = (*pscn)[0]->ixBase == 0 ? 0 : 1;
        for ( ; ix < BLOCK_MASK+1 ; ++ix) {
            if ((*pscn)[ix] != NULL_BLOCK) {
                if ((*pscn)[ix] != RESERVED_BLOCK) {
                    DecommitPages(pscn, ix, 0, PAGES_PER_BLOCK, (DWORD)lpvSect, TRUE);
                    if ((*pscn)[ix] != RESERVED_BLOCK)
                        FreeMem((*pscn)[ix], HEAP_MEMBLOCK);
                }
            }
        }
        SectionTable[ixScn] = NULL_SECTION;
        FreeSection(pscn);
        if (ixScn < FirstFreeSection)
        	FirstFreeSection = ixScn;
    } else
        DEBUGMSG(ZONE_VIRTMEM,(TEXT("DeleteSection failed.\r\n")));
	LeaveCriticalSection(&VAcs);
    return;
}

const MEMBLOCK KPageBlock = {
    ~0ul,       // alk
    0,          // cUses
    0,          // flags
    UNKNOWN_BASE,// ixBase
    0,          // hPf
    1,          // cLocks
    {
#if PAGE_SIZE == 4096
        0, 0, 0, 0, 0,
#elif PAGE_SIZE == 2048
        /* 0x0000: */ 0, 0, 0, 0,
        /* 0x2000: */ 0, 0, 0, 0,
        /* 0x4000: */ 0, 0, 0,
        /* 0x5800: (start of user visible kernel data page) */
#elif PAGE_SIZE == 1024
        /* 0x0000: */ 0, 0, 0, 0,
        /* 0x1000: */ 0, 0, 0, 0,
        /* 0x2000: */ 0, 0, 0, 0,
        /* 0x3000: */ 0, 0, 0, 0,
        /* 0x4000: */ 0, 0, 0, 0,
        /* 0x5000: */ 0, 0,
        /* 0x5800: (start of user visible kernel data page) */
#else
    #error Unsupported page size.
#endif
        KPAGE_PTE
    }
};

SECTION NKSection;

LPVOID InitNKSection(void) {
    SectionTable[1] = &NKSection;
    NKSection[0] = (MEMBLOCK*)&KPageBlock;
    return (LPVOID)(1<<VA_SECTION);
}

LPVOID CreateSection(LPVOID lpvAddr) {
    PSECTION pSect;
    uint ixSect;
	EnterCriticalSection(&VAcs);
    if (lpvAddr != 0) {
        if ((ixSect = (ulong)lpvAddr>>VA_SECTION) > SECTION_MASK
                || SectionTable[ixSect] != NULL_SECTION) {
            DEBUGMSG(1,(TEXT("CreateSection failed (1)\r\n")));
            lpvAddr = 0;
            goto exitCreate;
        }
    } else {
        /* scan for an available section table entry */
        for (;;) {
            for (ixSect = FirstFreeSection ; ixSect < MAX_PROCESSES+RESERVED_SECTIONS
                    ; ++ixSect) {
                if (SectionTable[ixSect] == NULL_SECTION)
                    goto foundSect;
            }
            if (FirstFreeSection == RESERVED_SECTIONS)
                break;
            FirstFreeSection = RESERVED_SECTIONS;
        }
        /* no sections available */
        DEBUGMSG(1, (TEXT("CreateSection failed (2)\r\n")));
        goto exitCreate;
        /* found a free section set new first free index */
foundSect:
        FirstFreeSection = ixSect+1;
    }
    /* Allocate a section and initialize it to invalid blocks */
    if ((pSect = GetSection()) == 0) {
        DEBUGMSG(1, (TEXT("CreateSection failed (3)\r\n")));
        lpvAddr = 0;
        goto exitCreate;
    }
    ///memcpy(pSect, &NullSection, sizeof(NullSection));
    (*pSect)[0] = (MEMBLOCK*)&KPageBlock;
    SectionTable[ixSect] = pSect;
    lpvAddr = (LPVOID)(ixSect << VA_SECTION);
exitCreate:
	LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("CreateSection done: addr=%8.8lx, pSect=%8.8lx\r\n"), lpvAddr, pSect));
    return lpvAddr;
}

// CreateMapperSection - allocate memory section for file mapping
//
//  This function will allocate a 32mb section and reserve the entire range for
// use by file mapping.

BOOL CreateMapperSection(DWORD dwBase) {
    BOOL bRet = FALSE;
    MEMBLOCK *pmb;
    PSECTION pscn;
    int ix;
    if (dwBase < FIRST_MAPPER_ADDRESS || dwBase >= LAST_MAPPER_ADDRESS) {
        DEBUGMSG(1, (TEXT("CreateMapperSection: %8.8lx is out of range.\r\n"), dwBase));
        return FALSE;
    }
    // First, create a normal section. Then convert it into a special section
    // for the Mapper.
	EnterCriticalSection(&VAcs);
    if (CreateSection((LPVOID)dwBase) != 0) {
        pscn = SectionTable[(dwBase>>VA_SECTION) & SECTION_MASK];
        if ((pmb = AllocMem(HEAP_MEMBLOCK)) != 0) {
        	memset(pmb,0,sizeof(MEMBLOCK));
            LockFromKey(&pmb->alk, &ProcArray[0].aky);
            pmb->cUses = 1;
          	pmb->flags = MB_PAGER_MAPPING;
            pmb->ixBase = 0;
            (*pscn)[0] = pmb;
            for (ix = 1 ; ix < BLOCK_MASK+1 ; ++ix)
                (*pscn)[ix] = RESERVED_BLOCK;
            bRet = TRUE;
        } else {
            DeleteSection((LPVOID)dwBase);
            DEBUGMSG(1, (TEXT("CreateMapperSection: unable to allocate MEMBLOCK\r\n")));
        }
    }
	LeaveCriticalSection(&VAcs);
	return bRet;
}

// DeleteMapperSection - delete a section created by CreateMapperSection()
//
//  This function must be used to delete a memory section which is created by
// CreateMapperSection.

void DeleteMapperSection(DWORD dwBase) {
	if ((dwBase >= FIRST_MAPPER_ADDRESS) && (dwBase < LAST_MAPPER_ADDRESS))
	    DeleteSection((LPVOID)dwBase);
    else 
        DEBUGMSG(1, (TEXT("DeleteMapperSection: %8.8lx is out of range.\r\n"), dwBase));
}

LPVOID SC_VirtualAlloc(
LPVOID lpvAddress,      /* address of region to reserve or commit */
DWORD cbSize,           /* size of the region */
DWORD fdwAllocationType,/* type of allocation */
DWORD fdwProtect)       /* type of access protection */
{
    int ixBlock;
    int ixPage;
    int ix;
    int ixFirB;        /* index of first block in region */
    PSECTION pscn;
    int cPages;         /* # of pages to allocate */
    int cNeed;
    int cpAlloc;        /* # of physical pages to allocate */
    ulong ulPgMask;     /* page permissions */
    ulong ulPFN;        /* page physical frame number */
    MEMBLOCK *pmb;
    DWORD err;
    LPBYTE lpbEnd;
    LPVOID lpvResult = NULL;
    ixFirB = UNKNOWN_BASE;
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualAlloc(%8.8lx, %lx, %lx, %lx)\r\n"),
	        lpvAddress, cbSize, fdwAllocationType, fdwProtect));
	if (!cbSize) {
        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!(ulPgMask = MakePagePerms(fdwProtect)))
        return FALSE;       /* invalid protection flags, error # already set */
    lpbEnd = (LPBYTE)lpvAddress + cbSize;
    /* Lockout other changes to the virtual memory state. */
    EnterCriticalSection(&VAcs);
    if ((fdwAllocationType & MEM_RESERVE) || (!lpvAddress && (fdwAllocationType & MEM_COMMIT))) {
        // Set MEM_RESERVE so the error cleanup works properly.
        fdwAllocationType |= MEM_RESERVE;
        /* Validate input parameters */
        if ((ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
            goto invalidParm;
        pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
        if (pscn == NULL_SECTION)
            goto invalidParm;
        ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
        if (((ulong)lpvAddress & ~(SECTION_MASK<<VA_SECTION)) != 0) {
	        if (cbSize > (1<<VA_SECTION))
    	    	goto invalidParm;
            /* The client has asked to reserve a specific region of memory.
             * Verify that the requested region is within range and within an
             * existing memory section that the client is allowed to access.
             */
            /* adjust lpvAddress to 64K boundary. */
            lpvAddress = (LPVOID)((ulong)lpvAddress & 0xFFFF0000l);
            /* Verify that the entire range is available to be reserved. */
            cPages = (ulong)(lpbEnd - (LPBYTE)lpvAddress + PAGE_SIZE-1) / PAGE_SIZE;
            for (cNeed = cPages, ix = ixBlock ; cNeed > 0
                    ; ++ix, cNeed -= PAGES_PER_BLOCK) {
                if ((*pscn)[ix] != NULL_BLOCK)
                    goto invalidParm;
            }
        } else {
            /* No specific address given. Go find a region of free blocks */
            cPages = (ulong)(lpbEnd - (LPBYTE)lpvAddress + PAGE_SIZE-1) / PAGE_SIZE;
            cNeed = cPages;
			if (fdwAllocationType & MEM_TOP_DOWN) {
	    	    if (cbSize > (1<<VA_SECTION))
    	    		goto invalidParm;
                /* Scan backwards from the end of the section */
                for (ix = BLOCK_MASK+1 ; --ix > 0 ; ) {
                    if ((*pscn)[ix] != NULL_BLOCK)
                        cNeed = cPages;
                    else if ((cNeed -= PAGES_PER_BLOCK) <= 0) {
                        ixBlock = ix;
                        goto foundRegion;
                    }
                }
            } else if ((cPages * PAGE_SIZE >= 2*1024*1024) && !ixBlock &&
	           		   (fdwAllocationType == MEM_RESERVE) && (fdwProtect == PAGE_NOACCESS)) {
				DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualAlloc(%8.8lx, %lx, %lx, %lx): doing HugeVirtualAlloc\r\n"),
	        		lpvAddress, cbSize, fdwAllocationType, fdwProtect));
            	LeaveCriticalSection(&VAcs);
            	lpvResult = HugeVirtualReserve(cbSize);
               CELOG_VirtualAlloc((DWORD)lpvResult, (DWORD)lpvAddress, cbSize, fdwAllocationType, fdwProtect);
               return lpvResult;
            } else {
		        if (cbSize > (1<<VA_SECTION))
        			goto invalidParm;
            	/* Scan forwards from the beginning of the section */
                ixBlock = 1;
                for (ix = 1 ; ix < BLOCK_MASK+1 ; ++ix) {
                    if ((*pscn)[ix] != NULL_BLOCK) {
                        ixBlock = ix+1;
                        cNeed = cPages;
                    } else if ((cNeed -= PAGES_PER_BLOCK) <= 0)
                        goto foundRegion;
                }
            }
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto setError;
foundRegion:
            lpvAddress = (LPVOID)(((ulong)lpvAddress&(SECTION_MASK<<VA_SECTION))
                    | ((ulong)ixBlock << VA_BLOCK));
        }
        /* Reserve the range of blocks */
        if (!(pmb = AllocMem(HEAP_MEMBLOCK))) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto setError;
        }
       	memset(pmb,0,sizeof(MEMBLOCK));
        if (((ulong)lpvAddress&(SECTION_MASK<<VA_SECTION)) == ProcArray[0].dwVMBase)
            LockFromKey(&pmb->alk, &ProcArray[0].aky);
        else
            LockFromKey(&pmb->alk, &pCurProc->aky);
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualAlloc: reserved lock=%lx\r\n"),
                pmb->alk));
        pmb->cUses = 1;
        switch (fdwAllocationType & (MEM_MAPPED|MEM_IMAGE)) {
        case MEM_MAPPED:
        	pmb->flags = MB_PAGER_MAPPING;
        	break;
		case MEM_IMAGE:
			pmb->flags = MB_PAGER_LOADER;
			break;
		case 0:
        	pmb->flags = MB_PAGER_NONE;
        	break;
        default:
        	goto invalidParm;
        }
        if (fdwAllocationType & MEM_AUTO_COMMIT)
        	pmb->flags |= MB_FLAG_AUTOCOMMIT;
        pmb->ixBase = ixBlock;
        (*pscn)[ixBlock] = pmb;
        DEBUGMSG(ZONE_VIRTMEM,
                (TEXT("VirtualAlloc: created head block pmb=%8.8lx (%x)\r\n"),
                pmb, ixBlock));
        ixFirB = ixBlock;   /* note start of region for error recovery */
        for (cNeed = cPages, ix = ixBlock+1 ; (cNeed -= PAGES_PER_BLOCK) > 0 ; ++ix) {
            if (cNeed < PAGES_PER_BLOCK || (fdwAllocationType & MEM_AUTO_COMMIT)) {
                /* The last block is not full so must allocate a MEMBLOCK
                 * and mark the extra pages as invalid or this is an auto-commit
                 * region which must have all memblocks filled in so that
                 * pages can be committed without entering the virtual memory
                 * critical section. */
                if (!(pmb = AllocMem(HEAP_MEMBLOCK))) {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    goto setError;
                }
	        	memset(pmb,0,sizeof(MEMBLOCK));
                pmb->alk = (*pscn)[ixFirB]->alk;
                pmb->flags = (*pscn)[ixFirB]->flags;
                pmb->cUses = 1;
                pmb->ixBase = ixFirB;
                (*pscn)[ix] = pmb;
                DEBUGMSG(ZONE_VIRTMEM,
                        (TEXT("VirtualAlloc: created a tail block pmb=%8.8lx (%x)\r\n"),
                        pmb, ixBlock));
            } else
                (*pscn)[ix] = RESERVED_BLOCK;
        }
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("Reserved %d pages @%8.8lx\r\n"), cPages, lpvAddress));
        if (cNeed) {
            /* Set unused entries to BAD_PAGE */
            DEBUGMSG(ZONE_VIRTMEM, (TEXT("Reserved %d extra pages.\r\n"), -cNeed));
            cNeed += PAGES_PER_BLOCK;
            for (ix = cNeed ; ix < PAGES_PER_BLOCK ; ++ix)
                pmb->aPages[ix] = BAD_PAGE;
        }
        /* If not committing pages, then return the address of the
         * reserved region */
        if (!(fdwAllocationType & MEM_COMMIT)) {
            LeaveCriticalSection(&VAcs);
            ERRORMSG(!lpvAddress,(L"Failed VirtualAlloc/reserve of %8.8lx bytes\r\n",cbSize));
            CELOG_VirtualAlloc((DWORD)lpvAddress, (DWORD)lpvAddress, cbSize, fdwAllocationType, fdwProtect);
            return lpvAddress;
        }
    } else {
        /* Not reserving memory, so must be committing. Verify that the
         * requested region is within range and within an existing reserved
         * memory region that the client is allowed to access.
         */
        if (cbSize > (1<<VA_SECTION))
        	goto invalidParm;
        if (!(fdwAllocationType & MEM_COMMIT) || lpvAddress == 0)
            goto invalidParm;
        if ((ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
            goto invalidParm;
        pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
        if (pscn == NULL_SECTION)
            goto invalidParm;
        ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
        /* Adjust lpvAddress to PAGE boundary and calculate the number
         * of pages to commit. */
        lpvAddress = (LPVOID)((ulong)lpvAddress & ~(PAGE_SIZE-1));
        cPages = (ulong)(lpbEnd - (LPBYTE)lpvAddress + PAGE_SIZE-1) / PAGE_SIZE;
        /* locate the starting block of the region */
        if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
            goto invalidParm;
    }
    /* Verify that cPages of memory starting with the first page indicated by
     * lpvAddress can be committed within the region.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page to commit
     *  (cPages) = # of pages to commit
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    cpAlloc = ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, 0);
    if (cpAlloc == -1)
        goto cleanUp;
    /* Commit cPages of memory starting with the first page indicated by
     * lpvAddress. Allocate all required pages before any changes to the
     * virtual region are performed.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page to commit
     *  (cPages) = # of pages to commit
     *  (cpAlloc) = # of physical pages required
     */
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("Allocating %d pages.\r\n"), cpAlloc));
    if (!HoldPages(cpAlloc, FALSE)) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto setError;
    }
	ixBlock = (ixBlock*PAGES_PER_BLOCK+ixPage+cPages-1)/PAGES_PER_BLOCK;
	ix = ((ixPage + cPages - 1) % PAGES_PER_BLOCK) + 1;
    /* Walk the page tables to map in the physical pages. */
    for (; cPages ; --ixBlock) {
        pmb = (*pscn)[ixBlock];
        for (; cPages && ix-- > 0 ; --cPages) {
            if (pmb->aPages[ix] == 0) {
            	DWORD dwRetries;
            	for (dwRetries = 0; (dwRetries < 20) && !(ulPFN = GetHeldPage()); dwRetries++)
            		Sleep(100);
            	if (ulPFN) {
	                DEBUGMSG(ZONE_VIRTMEM, (TEXT("Mapping %8.8lx @%3.3x%x000 perm=%x\r\n"),
    	                    ulPFN, ixBlock, ix, ulPgMask));
        	        pmb->aPages[ix] = ulPFN | ulPgMask;
        	    } else {
			    	InterlockedIncrement(&PageFreeCount);
					RETAILMSG(1,(L"--->>> VirtualAlloc: FATAL ERROR!  COMPLETELY OUT OF MEMORY (%8.8lx)!\r\n",PageFreeCount));
				}
            } else
                pmb->aPages[ix] = (pmb->aPages[ix] & ~PG_PERMISSION_MASK) | ulPgMask;
        }
        ix = PAGES_PER_BLOCK;            /* start with last page of previous block */
    }
    InvalidateRange(lpvAddress, cbSize); // in case we changed permissions above
    LeaveCriticalSection(&VAcs);
    ERRORMSG(!lpvAddress,(L"Failed VirtualAlloc(%8.8lx) of %8.8lx bytes\r\n",fdwAllocationType,cbSize));
    
    CELOG_VirtualAlloc((DWORD)lpvAddress, (DWORD)lpvAddress, cbSize, fdwAllocationType, fdwProtect);
    return lpvAddress;
    /* There was an error reserving or commiting a range of pages. If reserving
     * pages, release any pages which were reserved before the failure occured.
     */
invalidParm:
    err = ERROR_INVALID_PARAMETER;
setError:
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualAlloc failed. error=%d\r\n"), err));
    KSetLastError(pCurThread, err);
cleanUp:
    if (fdwAllocationType & MEM_RESERVE && ixFirB != UNKNOWN_BASE) {
        /* Remove the reservation */
        ReleaseRegion(pscn, ixFirB);
    }
    LeaveCriticalSection(&VAcs);
    ERRORMSG(!lpvAddress,(L"Failed VirtualAlloc(%8.8lx) of %8.8lx bytes\r\n",fdwAllocationType,cbSize));
    return 0;
}

BOOL SC_VirtualFree(
LPVOID lpvAddress,  /* address of region of committed pages */
DWORD cbSize,       /* size of the region */
DWORD fdwFreeType)  /* type of free operation */
{
    int ixBlock;
    int ixPage;
    int ixFirB;         /* index of first block in region */
    PSECTION pscn;
    int cpReserved;     /* # of reserved (not commited) pages in region */
    int cpRegion;       /* total # of pages in region */
    int cPages;         /* # of pages to free */
    DWORD baseScn;		/* base address of section */
    BOOL bForceDecommit;
    /* Verify that the requested region is within range and within an
     * existing reserved memory region that the client is allowed to
     * access and locate the starting block of the region.
     */
    bForceDecommit = (fdwFreeType & 0x80000000);
    fdwFreeType &= ~0x80000000;
    CELOG_VirtualFree((DWORD)lpvAddress, cbSize, fdwFreeType);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree @%8.8lx size=%lx freetype=%lx\r\n"),
            lpvAddress, cbSize, fdwFreeType));
    if (((DWORD)lpvAddress >= FIRST_MAPPER_ADDRESS) && !cbSize && (fdwFreeType == MEM_RELEASE)) {
	    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree @%8.8lx size=%lx freetype=%lx : doing HugeVirtualRelease\r\n"),
            lpvAddress, cbSize, fdwFreeType));
		return HugeVirtualRelease(lpvAddress);
    }
    /* Lockout other changes to the virtual memory state. */
    EnterCriticalSection(&VAcs);
    if (lpvAddress == 0 || (ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
        goto invalidParm;
    pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    baseScn = (DWORD)lpvAddress & (SECTION_MASK<<VA_SECTION);
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify the status of the region based upon the type of free operation
     * being performed.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page of lpvAddress
     *  (ixFirB) = index of first block in the region
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    if (fdwFreeType == MEM_RELEASE) {
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree releasing region ixBlock=%x.\r\n"),ixBlock));
        if (cbSize != 0 || ixPage != 0 || ixBlock != ixFirB)
            goto invalidParm;
        cpReserved = ScanRegion(pscn, ixFirB, ixFirB, 0, (BLOCK_MASK+1)*PAGES_PER_BLOCK, &cpRegion);
        DEBUGCHK(cpReserved != -1);
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree: cpReserved=%d cpRegion=%d\r\n"), cpReserved, cpRegion));
        /* The entire region must be either reserved or commited. */
        if (cpReserved != cpRegion) {
            if (cpReserved != 0)
                goto invalidParm;
            DecommitPages(pscn, ixFirB, 0, cpRegion, baseScn, bForceDecommit);
        }
        ReleaseRegion(pscn, ixFirB);
        LeaveCriticalSection(&VAcs);
        return TRUE;
    } else if (fdwFreeType == MEM_DECOMMIT) {
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree decommitting block %x page %x.\r\n"), ixBlock, ixPage));
        if (cbSize == 0)
            goto invalidParm;
        cPages = (((ulong)lpvAddress + cbSize + PAGE_SIZE-1) / PAGE_SIZE)
                - ((ulong)lpvAddress / PAGE_SIZE);
        cpReserved = ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, &cpRegion);
        if (cpRegion < cPages)
            goto invalidParm;
        if (cpReserved != cPages)
            DecommitPages(pscn, ixBlock, ixPage, cPages, baseScn, bForceDecommit);
        LeaveCriticalSection(&VAcs);
        return TRUE;
    }
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualFree failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*
	@doc INTERNAL
	@func BOOL | VirtualCopy | Duplicate a virtual memory range (Windows CE Only)
	@parm LPVOID | lpvDest | address of destination pages 
	@parm LPVOID | lpvSrc | address of source pages 
    @parm DWORD | cbSize | number of bytes to copy 
    @parm DWORD | fdwProtect) | access protection for destination pages 
	@comm Description unavailable at this time.
*/

BOOL DoVirtualCopy(
LPVOID lpvDest,         /* address of destination pages */
LPVOID lpvSrc,          /* address of source pages */
DWORD cbSize,           /* # of bytes to copy */
DWORD fdwProtect)       /* access protection for destination pages */
{
    int ixDestBlk, ixSrcBlk;
    int ixPage;
    int ixDestFirB;     /* index of first block in destination region */
    int ixSrcFirB;      /* index of first block in destination region */
    PSECTION pscnDest;
    PSECTION pscnSrc;
    int cpReserved;     /* # of reserved (not commited) pages in region */
    int cpRegion;       /* total # of pages in region */
    int cPages;         /* # of pages to copy */
    ulong ulPFN;        /* page physical frame number */
    ulong ulPgMask;
    BOOL bPhys = FALSE; /* TRUE if mapping physical pages */
    /* Verify that the requested regions are within range and within
     * existing reserved memory regions that the client is allowed to
     * access and locate the starting block of both regions.
     */
    CELOG_VirtualCopy((DWORD)lpvDest, (DWORD)lpvSrc, cbSize, fdwProtect);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualCopy %8.8lx <= %8.8lx size=%lx prot=%lx\r\n"),
            lpvDest, lpvSrc, cbSize, fdwProtect));
    if (fdwProtect & PAGE_PHYSICAL) {
        bPhys = TRUE;
        fdwProtect &= ~PAGE_PHYSICAL;
    }
    if ((ulPgMask = MakePagePerms(fdwProtect)) == 0)
        return FALSE;   /* invalid protection flags, error # already set */
    /* Lockout other changes to the virtual memory state. */
    EnterCriticalSection(&VAcs);
    /* Validate the destination parameters */
    if (!cbSize || !lpvDest || (ulong)lpvDest>>VA_SECTION > SECTION_MASK)
        goto invalidParm;
    pscnDest = SectionTable[((ulong)lpvDest>>VA_SECTION) & SECTION_MASK];
    if (pscnDest == NULL_SECTION)
        goto invalidParm;
    ixDestBlk = ((ulong)lpvDest >> VA_BLOCK) & BLOCK_MASK;
    ixDestFirB = FindFirstBlock(pscnDest, ixDestBlk);
    if (ixDestFirB == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify that all of the destination pages are reserved (not committed). */
    ixPage = ((ulong)lpvDest >> VA_PAGE) & PAGE_MASK;
    cPages = (((ulong)lpvDest + cbSize + PAGE_SIZE-1) / PAGE_SIZE) - ((ulong)lpvDest / PAGE_SIZE);
    cpReserved = ScanRegion(pscnDest, ixDestFirB, ixDestBlk, ixPage, cPages, 0);
    if (cpReserved != cPages)
        goto invalidParm;
    /* Validate the source address parameters */
    if (bPhys) {
        ulPFN = PFNfrom256(lpvSrc);
        if (((ulong)lpvDest&(PAGE_SIZE-1)) != ((ulong)lpvSrc<<8 &(PAGE_SIZE-1)))
            goto invalidParm;
    } else if ((ulong)lpvSrc>>VA_SECTION > SECTION_MASK) {
        /* Mapping pages from a physical region. */
        bPhys = TRUE;
        ulPFN = GetPFN(lpvSrc);
        if (((ulong)lpvDest&(PAGE_SIZE-1)) != ((ulong)lpvSrc&(PAGE_SIZE-1)))
            goto invalidParm;
    } else {
        /* Mapping pages from another virtual region. */
        bPhys = FALSE;
        if (lpvSrc == 0 || ((ulong)lpvDest&0xFFFFL) != ((ulong)lpvSrc&0xFFFFL))
            goto invalidParm;
        pscnSrc = SectionTable[((ulong)lpvSrc>>VA_SECTION) & SECTION_MASK];
        if (pscnSrc == NULL_SECTION)
            goto invalidParm;
        ixSrcBlk = ((ulong)lpvSrc >> VA_BLOCK) & BLOCK_MASK;
        ixSrcFirB = FindFirstBlock(pscnSrc, ixSrcBlk);
        if (ixSrcFirB == UNKNOWN_BASE)
            goto invalidParm;
        /* Verify that all of the source pages are committed */
        cpReserved = ScanRegion(pscnSrc, ixSrcFirB, ixSrcBlk, ixPage, cPages, &cpRegion);
        if (cpReserved || cpRegion != cPages)
            goto invalidParm;
    }
    /* Walk the the page tables mapping the pages in the source region into
     * the destination region. */
    for (; cPages > 0 ; ++ixDestBlk, ++ixSrcBlk) {
        MEMBLOCK *pmbSrc;
        MEMBLOCK *pmbDest = (*pscnDest)[ixDestBlk];
        if (!bPhys)
            pmbSrc = (*pscnSrc)[ixSrcBlk];
        if (!bPhys && ixDestFirB == ixSrcFirB && ixPage == 0
                && (cPages >= PAGES_PER_BLOCK
                || pmbSrc->aPages[cPages] == BAD_PAGE)
                && (pmbSrc->aPages[0]&PG_PERMISSION_MASK) == ulPgMask) {
            /* Copying an entire block with the same access permissions into
             * the same offset within the two sections.  Share the same MEMBLOCK
             * by bumping the use count on the MEMBLOCK. */
            DEBUGCHK(pmbDest != NULL_BLOCK && pmbDest != RESERVED_BLOCK);
            FreeMem(pmbDest,HEAP_MEMBLOCK);
            ++pmbSrc->cUses;
            AddAccess(&pmbSrc->alk, pCurProc->aky);
            (*pscnDest)[ixDestBlk] = pmbSrc;
            cPages -= PAGES_PER_BLOCK;
#if defined(SH4) && (PAGE_SIZE==4096)
        } else if (bPhys && !ixPage && !(ixDestBlk & 15) && !(ulPFN&1048575) && (cPages >= 256)) {
			int loop, loop2;
			DWORD dwSetting = ulPgMask | PG_1M_MASK;
			for (loop = 0; loop < 16; loop++) {
				pmbDest = (*pscnDest)[ixDestBlk+loop];
				for (loop2 = 0; loop2 < 16; loop2++) {
		        	pmbDest->aPages[loop2] = dwSetting | ulPFN;
   			        ulPFN = NextPFN(ulPFN);
   			    }
   			}
			cPages-=256;
			ixDestBlk+=15;
#endif				
		} else {
            for ( ; cPages && ixPage < PAGES_PER_BLOCK ; ++ixPage, --cPages) {
                if (bPhys) {
                    DEBUGMSG(ZONE_VIRTMEM,
                        (TEXT("Mapping physical page %8.8lx @%3.3x%x000 perm=%x\r\n"),
                        ulPFN, ixDestBlk, ixPage, ulPgMask));
#if 0
#if defined(SH3) && (PAGE_SIZE==1024)
					if (!(ixPage&3) && !(ulPFN&4095) && (cPages >= 4)) {
						int loop;
						DWORD dwSetting = ulPgMask | PG_4K_MASK;
						for (loop = 0; loop < 4; loop++) {
			                DEBUGCHK(pmbDest->aPages[ixPage+loop] == 0);
		                    pmbDest->aPages[ixPage+loop] = dwSetting | ulPFN;
	   		                ulPFN = NextPFN(ulPFN);
	   		            }
						ixPage+=3;
						cPages-=3;
					} else {
		                DEBUGCHK(pmbDest->aPages[ixPage] == 0);
	                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask;
    	                ulPFN = NextPFN(ulPFN);
    	            }
#elif defined(SH4) && (PAGE_SIZE==4096)
					if (!(ixPage&15) && !(ulPFN&65535) && (cPages >= 16)) {
						int loop;
						DWORD dwSetting = (ulPgMask & ~PG_1M_MASK) | PG_64K_MASK;
						for (loop = 0; loop < 16; loop++) {
			                DEBUGCHK(pmbDest->aPages[ixPage+loop] == 0);
		                    pmbDest->aPages[ixPage+loop] = dwSetting | ulPFN;
	   		                ulPFN = NextPFN(ulPFN);
	   		            }
						ixPage+=15;
						cPages-=15;
					} else {
		                DEBUGCHK(pmbDest->aPages[ixPage] == 0);
	                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask;
    	                ulPFN = NextPFN(ulPFN);
    	            }
#else
	                DEBUGCHK(pmbDest->aPages[ixPage] == 0);
                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask;
                    ulPFN = NextPFN(ulPFN);
#endif
#else
	                DEBUGCHK(pmbDest->aPages[ixPage] == 0);
                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask;
                    ulPFN = NextPFN(ulPFN);
#endif
                } else {
                	DWORD dwOrig = pmbSrc->aPages[ixPage];
                    DEBUGCHK(dwOrig != 0 && dwOrig != BAD_PAGE);
                    ulPFN = PFNfromEntry(dwOrig);
                    DupPhysPage(ulPFN);
                    DEBUGMSG(ZONE_VIRTMEM, (TEXT("Copying page %8.8lx @%3.3x%x000 perm=%x\r\n"),
                            ulPFN, ixDestBlk, ixPage, ulPgMask));
#if defined(SH3) && (PAGE_SIZE==1024)
                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask | (dwOrig & PG_4K_MASK);
#elif defined(SH4) && (PAGE_SIZE==4096)
                    pmbDest->aPages[ixPage] = ulPFN | (ulPgMask & ~PG_1M_MASK) | (dwOrig & PG_1M_MASK);
#else
                    pmbDest->aPages[ixPage] = ulPFN | ulPgMask;
#endif
                }
            }
        }
        ixPage = 0;            /* start with first page of next block */
    }
    LeaveCriticalSection(&VAcs);
    return TRUE;
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualCopy failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL SC_VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect) {
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_VirtualCopy failed due to insufficient trust\r\n"));
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	return DoVirtualCopy(lpvDest, lpvSrc, cbSize, fdwProtect);
}

#ifdef SH4
BOOL SC_VirtualSetPageFlags(LPVOID lpvAddress, DWORD cbSize, DWORD dwFlags, LPDWORD lpdwOldFlags) {
	DWORD bits;
    int ixBlock;
    int ixPage;
    int ixFirB;         /* index of first block in region */
    PSECTION pscn;
    int cPages;         /* # of pages to adjust */
    MEMBLOCK *pmb;
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_VirtualSetPageFlags failed due to insufficient trust\r\n"));
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	switch (dwFlags & ~VSPF_TC) {
		case VSPF_NONE:
			bits = 0;
			break;
		case VSPF_VARIABLE:
			bits = 1;
			break;
		case VSPF_IOSPACE:
			bits = 2;
			break;
		case VSPF_IOSPACE | VSPF_16BIT:
			bits = 3;
			break;
		case VSPF_COMMON:
			bits = 4;
			break;
		case VSPF_COMMON | VSPF_16BIT:
			bits = 5;
			break;
		case VSPF_ATTRIBUTE:
			bits = 6;
			break;
		case VSPF_ATTRIBUTE | VSPF_16BIT:
			bits = 7;
			break;
		default:
			KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
			return FALSE;
	}
	bits <<= 29;
	if (dwFlags & VSPF_TC)
	   bits |= (1 << 9);
    EnterCriticalSection(&VAcs);
	if (!cbSize || !lpvAddress || (cbSize & 0x80000000) || ((ulong)lpvAddress>>VA_SECTION > SECTION_MASK))
		goto invalidParm;
    pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify that all of the pages within the specified range belong to the same VirtualAlloc region and are committed.
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page of lpvAddress
     *  (ixFirB) = index of first block in the region
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    cPages = (((ulong)lpvAddress + cbSize + PAGE_SIZE-1) / PAGE_SIZE)
            - ((ulong)lpvAddress / PAGE_SIZE);
    if (ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, 0) != 0)
        goto invalidParm;
    /* Walk the page tables to set the new page permissions */
    if (lpdwOldFlags) {
    	switch ((*pscn)[ixBlock]->aPages[ixPage] >> 29) {
		    	case 0:
				*lpdwOldFlags = VSPF_NONE;
				break;
			case 1:
				*lpdwOldFlags = VSPF_VARIABLE;
				break;
			case 2:
				*lpdwOldFlags = VSPF_IOSPACE;
				break;
			case 3:
				*lpdwOldFlags = VSPF_IOSPACE | VSPF_16BIT;
				break;
			case 4:
				*lpdwOldFlags = VSPF_COMMON;
				break;
			case 5:
				*lpdwOldFlags = VSPF_COMMON | VSPF_16BIT;
				break;
			case 6:
				*lpdwOldFlags = VSPF_ATTRIBUTE;
				break;
			case 7:
				*lpdwOldFlags = VSPF_ATTRIBUTE | VSPF_16BIT;
				break;
		}
		if (((*pscn)[ixBlock]->aPages[ixPage] >> 9) & 1)
		    *lpdwOldFlags |= VSPF_TC;
	}
    for (; cPages ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        do {
            DEBUGCHK(pmb->aPages[ixPage] != 0 && pmb->aPages[ixPage] != BAD_PAGE);
            DEBUGMSG(ZONE_VIRTMEM, (TEXT("Setting extra bits @%3.3x%x000 bits=%x\r\n"), ixBlock, ixPage, bits));
            pmb->aPages[ixPage] = (pmb->aPages[ixPage] & 0x1ffffdff) | bits;
        } while (--cPages && ++ixPage < PAGES_PER_BLOCK);
        ixPage = 0;            /* start with first page of next block */
    }
    LeaveCriticalSection(&VAcs);
    return TRUE;
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualSetPageFlags failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return FALSE;
}
#endif

BOOL SC_VirtualProtect(
LPVOID lpvAddress,      /* address of region of committed pages */
DWORD cbSize,           /* size of the region */
DWORD fdwNewProtect,    /* desired access protection */
PDWORD pfdwOldProtect)  /* address of variable to get old protection */
{
    int ixBlock;
    int ixPage;
    int ixFirB;         /* index of first block in region */
    PSECTION pscn;
    int cPages;         /* # of pages to adjust */
    ulong ulPgMask;     /* page permissions */
    MEMBLOCK *pmb;
    BOOL bRet = TRUE;
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualProtect @%8.8lx size=%lx fdwNewProtect=%lx\r\n"),
            lpvAddress, cbSize, fdwNewProtect));
    if ((ulPgMask = MakePagePerms(fdwNewProtect)) == 0)
        return FALSE;       /* invalid protection flags, error # already set */
    /* Verify that the requested region is within range and within an
     * existing reserved memory region that the client is allowed to
     * access and locate the starting block of the region.
     */
    /* Lockout other changes to the virtual memory state. */
    EnterCriticalSection(&VAcs);
    if (cbSize == 0 || pfdwOldProtect == 0)
        goto invalidParm;
    if (lpvAddress == 0 || (ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
        goto invalidParm;
    pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify that all of the pages within the specified range belong to the
     * same VirtualAlloc region and are committed.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page of lpvAddress
     *  (ixFirB) = index of first block in the region
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    cPages = (((ulong)lpvAddress + cbSize + PAGE_SIZE-1) / PAGE_SIZE)
            - ((ulong)lpvAddress / PAGE_SIZE);
    if (ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, 0) != 0)
        goto invalidParm;
    /* Walk the page tables to set the new page permissions */
    *pfdwOldProtect = ProtectFromPerms((*pscn)[ixBlock]->aPages[ixPage]);
    for (; cPages ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        do {
	        if (!pmb->cLocks || ((ulPgMask & (PG_VALID_MASK | PG_PROT_WRITE)) == (PG_VALID_MASK | PG_PROT_WRITE))) {
    	        DEBUGCHK(pmb->aPages[ixPage] != 0 && pmb->aPages[ixPage] != BAD_PAGE);
        	    DEBUGMSG(ZONE_VIRTMEM, (TEXT("Changing perms @%3.3x%x000 perm=%x\r\n"),
            	        ixBlock, ixPage, ulPgMask));
#if defined(SH3) && (PAGE_SIZE == 1024)
	            pmb->aPages[ixPage] = (pmb->aPages[ixPage] & ~(PG_PERMISSION_MASK & ~PG_4K_MASK)) | ulPgMask;
#elif defined(SH4) && (PAGE_SIZE == 4096)
        	    pmb->aPages[ixPage] = (pmb->aPages[ixPage] & ~(PG_PERMISSION_MASK & ~PG_1M_MASK)) | (ulPgMask & ~PG_1M_MASK);
#else
	            pmb->aPages[ixPage] = (pmb->aPages[ixPage] & ~PG_PERMISSION_MASK) | ulPgMask;
#endif
	    	} else {
	    		DEBUGMSG(1,(L"VirtualProtect: Cannot reduce access at %8.8lx, lock count %d\r\n",
					((DWORD)lpvAddress & (SECTION_MASK<<VA_SECTION)) + (ixBlock<<VA_BLOCK) + (ixPage<<VA_PAGE), pmb->cLocks));
				if (cbSize == PAGE_SIZE)
					bRet = FALSE;
	    	}
   	    } while (--cPages && ++ixPage < PAGES_PER_BLOCK);
        ixPage = 0;            /* start with first page of next block */
    }
    // Since page permissions have been modified, it is necessary to flush the TLB.
    InvalidateRange(lpvAddress, cbSize);
    LeaveCriticalSection(&VAcs);
    return bRet;
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualProtect failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return FALSE;
}

DWORD SC_VirtualQuery(
LPVOID lpvAddress,      /* address of region */
PMEMORY_BASIC_INFORMATION pmbiBuffer,   /* address of information buffer */
DWORD cbLength)         /* size of buffer */
{
    int ixBlock;
    int ixPage;
    int ixFirB;         /* index of first block in region */
    PSECTION pscn;
    MEMBLOCK *pmb;
    int cPages;
    ulong ulPgPerm;
    if (cbLength < sizeof(MEMORY_BASIC_INFORMATION)) {
        KSetLastError(pCurThread, ERROR_BAD_LENGTH);
        return 0;
    }
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualQuery @%8.8lx buf=%8.8lx cbLen=%d\r\n"),
            lpvAddress, pmbiBuffer, cbLength));
    if (!pmbiBuffer || !lpvAddress || (ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
        goto invalidParm;
    pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((pmb = (*pscn)[ixBlock]) == NULL_BLOCK ||
        (pmb != RESERVED_BLOCK && pmb->aPages[ixPage] == BAD_PAGE) ||
        (ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE) {
        /* The given page is in a free region, walk the section to compute
         * the size of the region */
        cPages = PAGES_PER_BLOCK - ixPage;
        while (++ixBlock < BLOCK_MASK+1 && (*pscn)[ixBlock] == NULL_BLOCK)
            cPages += PAGES_PER_BLOCK;
        pmbiBuffer->AllocationBase = 0;
        pmbiBuffer->State = MEM_FREE;
        pmbiBuffer->AllocationProtect = PAGE_NOACCESS;
        pmbiBuffer->Protect = PAGE_NOACCESS;
        pmbiBuffer->Type = 0;
    } else {
        cPages = 0;
        pmbiBuffer->AllocationProtect = PAGE_NOACCESS;
        pmbiBuffer->Type = MEM_PRIVATE;
        if (pmb == RESERVED_BLOCK || pmb->aPages[ixPage] == 0) {
            /* Count reserved pages */
            pmbiBuffer->State = MEM_RESERVE;
            pmbiBuffer->Protect = PAGE_NOACCESS;
            do {
                if (pmb == RESERVED_BLOCK)
                    cPages += PAGES_PER_BLOCK - ixPage;
                else {
                    pmbiBuffer->Type = AllocationType[pmb->flags & MB_FLAG_PAGER_TYPE];
                    do {
                        if (pmb->aPages[ixPage] != 0)
                            goto allCounted;
                        ++cPages;
                    } while (++ixPage < PAGES_PER_BLOCK);
                }
                ixPage = 0;
            } while (++ixBlock < BLOCK_MASK+1 &&
                     ((pmb = (*pscn)[ixBlock]) == RESERVED_BLOCK ||
                      (pmb != NULL_BLOCK && pmb->ixBase == ixFirB)));
        } else {
            /* Count committed pages */
            pmbiBuffer->State = MEM_COMMIT;
            ulPgPerm = pmb->aPages[ixPage] & PG_PERMISSION_MASK;
            pmbiBuffer->Protect = ProtectFromPerms(pmb->aPages[ixPage]);
            pmbiBuffer->Type = AllocationType[pmb->flags & MB_FLAG_PAGER_TYPE];
            do {
                do {
                    ulong ulPgInfo = pmb->aPages[ixPage];
                    if (ulPgInfo == 0 || ulPgInfo == BAD_PAGE
                            || (ulPgInfo&PG_PERMISSION_MASK) != ulPgPerm)
                        goto allCounted;
                    ++cPages;
                } while (++ixPage < PAGES_PER_BLOCK);
                ixPage = 0;
            } while (++ixBlock < BLOCK_MASK+1 &&
                     (pmb = (*pscn)[ixBlock]) != NULL_BLOCK &&
                     pmb != RESERVED_BLOCK && pmb->ixBase == ixFirB);
        }
allCounted:
        pmbiBuffer->AllocationBase =
                    (LPVOID)(((ulong)lpvAddress&(SECTION_MASK<<VA_SECTION)) |
                    		 ((ulong)ixFirB << VA_BLOCK));
    }
    pmbiBuffer->RegionSize = (DWORD)cPages << VA_PAGE;
    pmbiBuffer->BaseAddress = (LPVOID)((ulong)lpvAddress & ~(PAGE_SIZE-1));
    return sizeof(MEMORY_BASIC_INFORMATION);
invalidParm:
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("VirtualQuery failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return 0;
}

BOOL DoLockPages(
LPCVOID lpvAddress,      /* address of region of committed pages */
DWORD cbSize,           /* size of the region */
PDWORD pPFNs,           /* address of array to receive real PFN's */
int fOptions)           /* options: see LOCKFLAG_* in kernel.h */
{
    int ixBlock;
    int ixPage;
    int ixFirB;         /* index of first block in region */
    int ixStart;        /* index of start block of lock region */
    PSECTION pscn;
    int cPages;         /* # of pages to adjust */
    ulong ulBase;       /* base virtual address */
    MEMBLOCK *pmb;
    int err = ERROR_INVALID_PARAMETER;
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("LockPages @%8.8lx size=%lx pPFNs=%lx options=%x\r\n"),
            lpvAddress, cbSize, pPFNs, fOptions));
    /* Verify that the requested region is within range and within an
     * existing reserved memory region that the client is allowed to
     * access and locate the starting block of the region.
     */
    if ((ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
    	return TRUE;
    /* Lockout other changes to the virtual memory state. */
    EnterCriticalSection(&VAcs);
    if (!cbSize || !lpvAddress)
        goto invalidParm;
    ulBase = (ulong)lpvAddress & (SECTION_MASK << VA_SECTION);
    pscn = SectionTable[ulBase>>VA_SECTION];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify that all of the pages within the specified range belong to the
     * same VirtualAlloc region and are committed.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page of lpvAddress
     *  (ixFirB) = index of first block in the region
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    cPages = (((ulong)lpvAddress + cbSize + PAGE_SIZE-1) / PAGE_SIZE) - ((ulong)lpvAddress / PAGE_SIZE);
    if (ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, 0) == -1)
        goto invalidParm;
    /* Walk the page tables to check that all pages are present and to
     * increment the lock count for each MEMBLOCK in the region.
     */
    ixStart = ixBlock;
    for (; cPages ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        if (!(fOptions & LOCKFLAG_QUERY_ONLY)) {
            ++pmb->cLocks;
            DEBUGMSG(ZONE_VIRTMEM, (TEXT("Locking pages @%8.8x LC=%d\r\n"), 
                    ulBase | (ixBlock<<VA_BLOCK), pmb->cLocks));
        }
        do {
	        ulong addr = ulBase | (ixBlock<<VA_BLOCK) | (ixPage<<VA_PAGE);
            DEBUGCHK(pmb->aPages[ixPage] != BAD_PAGE);
            if (pmb->aPages[ixPage] == 0) {
                if (pmb->flags&MB_FLAG_AUTOCOMMIT) {
                    if (SC_VirtualAlloc((void*)addr, cPages*PAGE_SIZE, MEM_COMMIT,
                            PAGE_READWRITE) == 0)
                        goto cleanUpLocks;
                } else {
                	BOOL bRet;
                	LeaveCriticalSection(&VAcs);
                	bRet = ProcessPageFault(TRUE, addr);
                	if (!bRet && !(fOptions & LOCKFLAG_WRITE))
	                	bRet = ProcessPageFault(FALSE, addr);
                	EnterCriticalSection(&VAcs);
                	if (!bRet) {
                		err = ERROR_NOACCESS;
                    	goto cleanUpLocks;
                    }
                }
            } else {
                if (fOptions & LOCKFLAG_WRITE) {
                	if (!IsPageWritable(pmb->aPages[ixPage])) {
	                	BOOL bRet;
	                	LeaveCriticalSection(&VAcs);
	                	bRet = ProcessPageFault(TRUE, addr);
	                	EnterCriticalSection(&VAcs);
	                	if (!bRet) {
	                		err = ERROR_NOACCESS;
	                    	goto cleanUpLocks;
	                    }
	                }
                } else if (!IsPageReadable(pmb->aPages[ixPage])) {
                    err = ERROR_NOACCESS;
                    goto cleanUpLocks;
                }
                if ((fOptions & LOCKFLAG_READ) && !IsPageReadable(pmb->aPages[ixPage])) {
                    err = ERROR_NOACCESS;
	                goto cleanUpLocks;
                }
            }
            if (pPFNs)
   	            *pPFNs++ = PFNfromEntry(pmb->aPages[ixPage]);
        } while (--cPages && ++ixPage < PAGES_PER_BLOCK);
        ixPage = 0;            /* start with first page of next block */
    }
    LeaveCriticalSection(&VAcs);
    return TRUE;
cleanUpLocks:
    /* Unable to page in a page or this thread doesn't have the desired access
     * to a page in the range. Back out any locks which have been set.
     */
    if (!(fOptions & LOCKFLAG_QUERY_ONLY)) {
        do {
            --pmb->cLocks;
            DEBUGMSG(ZONE_VIRTMEM, (TEXT("Restoring lock count @%8.8x LC=%d\r\n"), 
                    ulBase | (ixBlock<<VA_BLOCK), pmb->cLocks));
        } while (ixBlock != ixStart && (pmb = (*pscn)[--ixBlock]));
    }
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("LockPages failed.\r\n")));
    KSetLastError(pCurThread, err);
    return FALSE;
}

BOOL SC_LockPages(LPVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions) {
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_LockPages failed due to insufficient trust\r\n"));
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	return DoLockPages(lpvAddress, cbSize, pPFNs, fOptions);
}

BOOL DoUnlockPages(
LPCVOID lpvAddress,      /* address of region of committed pages */
DWORD cbSize)           /* size of the region */
{
    int ixPage;
    int ixBlock;
    int ixFirB;         /* index of first block in region */
    int ixLastBlock;    /* last block to be unlocked */
    PSECTION pscn;
    int cPages;         /* # of pages to adjust */
    MEMBLOCK *pmb;
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("UnlockPages @%8.8lx size=%lx\r\n"),
            lpvAddress, cbSize));
    /* Verify that the requested region is within range and within an
     * existing reserved memory region that the client is allowed to
     * access and locate the starting block of the region.
     */
    /* Lockout other changes to the virtual memory state. */
    if ((ulong)lpvAddress>>VA_SECTION > SECTION_MASK)
    	return TRUE;
    EnterCriticalSection(&VAcs);
    if (!cbSize || !lpvAddress)
        goto invalidParm;
    pscn = SectionTable[((ulong)lpvAddress>>VA_SECTION) & SECTION_MASK];
    if (pscn == NULL_SECTION)
        goto invalidParm;
    ixBlock = ((ulong)lpvAddress >> VA_BLOCK) & BLOCK_MASK;
    if ((ixFirB = FindFirstBlock(pscn, ixBlock)) == UNKNOWN_BASE)
        goto invalidParm;
    /* Verify that all of the pages within the specified range belong to the
     * same VirtualAlloc region and are committed.
     *
     *  (pscn) = ptr to section array
     *  (ixBlock) = index of block containing the first page of lpvAddress
     *  (ixFirB) = index of first block in the region
     */
    ixPage = ((ulong)lpvAddress >> VA_PAGE) & PAGE_MASK;
    cPages = (((ulong)lpvAddress + cbSize + PAGE_SIZE-1) / PAGE_SIZE)
            - ((ulong)lpvAddress / PAGE_SIZE);
    if (ScanRegion(pscn, ixFirB, ixBlock, ixPage, cPages, 0) == -1)
        goto invalidParm;
    /* Walk the page tables to decrement the lock count for 
     * each MEMBLOCK in the region.
     */
    ixLastBlock = ixBlock + ((ixPage+cPages+PAGES_PER_BLOCK-1) >> (VA_BLOCK-VA_PAGE));
    for (; ixBlock < ixLastBlock ; ++ixBlock) {
        pmb = (*pscn)[ixBlock];
        if (pmb->cLocks)
            --pmb->cLocks;
        else
            DEBUGCHK(0);
        DEBUGMSG(ZONE_VIRTMEM, (TEXT("Unlocking pages @%8.8x LC=%d\r\n"), 
                ((ulong)lpvAddress&(SECTION_MASK<<VA_SECTION)) | (ixBlock<<VA_BLOCK),
                pmb->cLocks));
    }
    LeaveCriticalSection(&VAcs);
    return TRUE;
invalidParm:
    LeaveCriticalSection(&VAcs);
    DEBUGMSG(ZONE_VIRTMEM, (TEXT("LockPages failed.\r\n")));
    KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL SC_UnlockPages(LPVOID lpvAddress, DWORD cbSize) {
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_UnlockPages failed due to insufficient trust\r\n"));
	    KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
	}
	return DoUnlockPages(lpvAddress, cbSize);
}

LPBYTE GrabFirstPhysPage(DWORD dwCount);

/* AutoCommit - auto-commit a page
 *
 *	This function is called for a TLB miss due to a memory load or store.
 *  It is invoked from the general exception handler on the kernel stack.
 *  If the faulting page, is within an auto-commit region then a new page
 *  will be allocated and committed.
 */
BOOL AutoCommit(ulong addr) {
	register MEMBLOCK *pmb;
	PSECTION pscn;
	int ixSect, ixBlock, ixPage;
	ulong ulPFN;
	LPBYTE pMem;
	DWORD loop;
	ixPage = addr>>VA_PAGE & PAGE_MASK;
	ixBlock = addr>>VA_BLOCK & BLOCK_MASK;
	ixSect = addr>>VA_SECTION;
	if ((ixSect <= SECTION_MASK) && ((pscn = SectionTable[ixSect]) != NULL_SECTION) &&
		((pmb = (*pscn)[ixBlock]) != NULL_BLOCK) && (pmb != RESERVED_BLOCK) &&
		(pmb->flags&MB_FLAG_AUTOCOMMIT) && !pmb->aPages[ixPage]) {
		for (loop = 0; loop < 20; loop++) {
			//
			// Notify OOM thread if we're low on memory.
			//
			if (GwesOOMEvent && (PageFreeCount < GwesCriticalThreshold)) {
				SetEvent(GwesOOMEvent);
			}
			//
			// We don't want to let AutoCommit deplete all physical memory. 
			// Yes, the thread needing the memory committed is going to fault
			// unexpectedly if we can't allocate, so we'll loop for 2 seconds
			// checking for available RAM. We need to reserve a small amount
			// so that DemandCommit can allocate stack for the exception
			// handlers if required.
			//
			if ((PageFreeCount > 5) && (pMem = (LPBYTE)KCall((PKFN)GrabFirstPhysPage,1))) {
				ulPFN = GetPFN(pMem);
				/* Map the new page in as read/write. */
				DEBUGMSG(ZONE_VIRTMEM, (TEXT("Auto-committing %8.8lx @%8.8lx\r\n"),
						ulPFN, (ixSect<<VA_SECTION)+(ixBlock<<VA_BLOCK)+(ixPage*PAGE_SIZE)));
				pmb->aPages[ixPage] = ulPFN | PG_READ_WRITE;
				return TRUE;
			}
			Sleep(100);
		}
		RETAILMSG(1, (TEXT("WARNING (low memory) : Failed auto-commit of 0x%08X @ %d free pages\r\n"), addr, PageFreeCount));
		DEBUGCHK(0);
	}
	return FALSE;
}

void GuardCommit(ulong addr) {
	register MEMBLOCK *pmb;
	PSECTION pscn;
	int ixSect, ixBlock, ixPage;
	ixPage = addr>>VA_PAGE & PAGE_MASK;
	ixBlock = addr>>VA_BLOCK & BLOCK_MASK;
	ixSect = addr>>VA_SECTION;
	if ((ixSect <= SECTION_MASK) && ((pscn = SectionTable[ixSect]) != NULL_SECTION) &&
		((pmb = (*pscn)[ixBlock]) != NULL_BLOCK) && (pmb != RESERVED_BLOCK) && 
		(pmb->aPages[ixPage] & ~PG_PERMISSION_MASK)) {
		pmb->aPages[ixPage] |= PG_VALID_MASK;
#ifdef MUST_CLEAR_GUARD_BIT
		pmb->aPages[ixPage] &= ~PG_GUARD;
#endif
	}
}

ERRFALSE(MB_PAGER_FIRST==1);
FN_PAGEIN * const PageInFuncs[MB_MAX_PAGER_TYPE] = { LoaderPageIn, MappedPageIn };

/** ProcessPageFault - general page fault handler
 *
 *  This function is called from the exception handling code to attempt to handle
 * a fault due to a missing page. If the function is able to resolve the fault,
 * it returns TRUE.
 *
 * Environment:
 *   Kernel mode, preemtible, running on the thread's stack.
 */
BOOL ProcessPageFault(BOOL bWrite, ulong addr) {
	DWORD dwLast;
	register MEMBLOCK *pmb;
	PSECTION pscn;
	int ixSect, ixBlock, ixPage, pageresult, retrycnt = 0;
	uint pagerType = MB_PAGER_NONE;
	BOOL bRet = FALSE;
	CELOG_SystemPage(addr, bWrite);
   DEBUGMSG(ZONE_PAGING, (TEXT("ProcessPageFault: %a @%8.8lx\r\n"), bWrite?"Write":"Read", addr));
#ifdef DEBUG	
    if (InSysCall()) {
        DEBUGMSG(1, (TEXT("ProcessPageFault: <<In SYSCALL>> %a 0x%08x\r\n"), bWrite?"Write to":"Read from", addr));
        DEBUGCHK(0);
    }
#endif
	ixPage = addr>>VA_PAGE & PAGE_MASK;
	ixBlock = addr>>VA_BLOCK & BLOCK_MASK;
	ixSect = addr>>VA_SECTION;
retryPagein:
	EnterCriticalSection(&VAcs);
	if ((ixSect <= SECTION_MASK) && ((pscn = SectionTable[ixSect]) != NULL_SECTION) &&
		((pmb = (*pscn)[ixBlock]) != NULL_BLOCK)) {
		if (pmb == RESERVED_BLOCK) {
   			pmb = (*pscn)[FindFirstBlock(pscn, ixBlock)];
			pagerType = pmb->flags & MB_FLAG_PAGER_TYPE;
		} else {
    		if (pmb->aPages[ixPage] & PG_VALID_MASK) {
    		    if (!bWrite || IsPageWritable(pmb->aPages[ixPage]))
        	        bRet = TRUE;
				else
	    			pagerType = pmb->flags & MB_FLAG_PAGER_TYPE;
    		} else if (pmb->aPages[ixPage] != BAD_PAGE)
    			pagerType = pmb->flags & MB_FLAG_PAGER_TYPE;
        }
		DEBUGCHK(pagerType <= MB_MAX_PAGER_TYPE);
		if (TestAccess(&pmb->alk, &CurAKey)) {
    		if (pagerType != MB_PAGER_NONE) {
    			DEBUGMSG(ZONE_PAGING, (TEXT("Calling Page In Function. pagerType=%d\r\n"), pagerType));
	   			LeaveCriticalSection(&VAcs);
	   			dwLast = KGetLastError(pCurThread);
    			pageresult = PageInFuncs[pagerType-MB_PAGER_FIRST](bWrite, addr);
	   			KSetLastError(pCurThread,dwLast);
    			if ((pageresult == 2) || (pageresult == 3)) {
   					DEBUGMSG(ZONE_PAGING, (TEXT("Page function returned 'retry' %d\r\n"),pageresult));
   					Sleep(250);	// arbitrary number, should be > 0 but pretty small
   					bRet = FALSE;
   					pagerType = MB_PAGER_NONE;
   					if (retrycnt++ < 20)	// arbitrary number, total of 2 second "timeout" on pagein
	   					goto retryPagein;
                    
					RETAILMSG(1, (TEXT("ProcessPageFault : PageIn failed for address 0x%08X\r\n"), addr));
//					DEBUGCHK(0);
					goto FailPageInNoCS;
   				}
				bRet = (BOOL)pageresult;
    			EnterCriticalSection(&VAcs);
    		}
    	} else
    	    bRet = FALSE;   // Thread not allowed access, return failure.
	}
	LeaveCriticalSection(&VAcs);
FailPageInNoCS:
	DEBUGMSG(ZONE_PAGING, (TEXT("ProcessPageFault returning: %a\r\n"), bRet?"TRUE":"FALSE"));
	return bRet;
}

LPVOID pProfBase;
DWORD dwProfState;
DWORD dwOldEntry;

void ProfInit(void) {
	pProfBase = VirtualAlloc(0,PAGE_SIZE,MEM_RESERVE,PAGE_NOACCESS);
	pProfBase = MapPtr(pProfBase);
}

LPVOID SC_GetProfileBaseAddress(void) {
	EnterCriticalSection(&VAcs);
	if (!dwProfState) {
		pProfBase = VirtualAlloc(pProfBase,PAGE_SIZE,MEM_COMMIT,PAGE_READWRITE);
		dwProfState = 1;
	}
	LeaveCriticalSection(&VAcs);
	return pProfBase;
}

VOID SC_SetProfilePortAddress(LPVOID lpv) {
    PSECTION pscn;
    MEMBLOCK *pmb;
	EnterCriticalSection(&VAcs);
	if (!dwProfState) {
		pProfBase = VirtualAlloc(pProfBase,PAGE_SIZE,MEM_COMMIT,PAGE_READWRITE);
		dwProfState = 1;
	}
    pscn = SectionTable[((ulong)pProfBase>>VA_SECTION) & SECTION_MASK];
   	pmb = (*pscn)[((ulong)pProfBase >> VA_BLOCK) & BLOCK_MASK];
	if (!lpv) {
		if (dwProfState == 2) {
        	pmb->aPages[0] = dwOldEntry;
	        dwProfState = 1;
        }
	} else {
		if (dwProfState == 1)
			dwOldEntry = pmb->aPages[0];
        pmb->aPages[0] = PFNfrom256(lpv) | MakePagePerms(PAGE_READWRITE|PAGE_NOCACHE);
		dwProfState = 2;
	}
    InvalidateRange(pProfBase, PAGE_SIZE);
	LeaveCriticalSection(&VAcs);
}

