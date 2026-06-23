/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include <windows.h>
#include <coredll.h>
#include "heap.h"
#include <memory.h>

// layout: (DBG = with sentinels, RET = without sentinels)
//	[4 byte size in bytes not including headers, H_FREEBLOCK set if free]
//	[4 byte signature set to FREESIGHEAD/ALLOCSIGHEAD in DBG, unused on 64 bit
//		machines RET, not present on 32 bit machines RET]
//	[n byte free or data]
//	[4 byte signature set to FREESIGTAIL/ALLOCSIGTAIL in DBG, not present in RET]
//	[4 byte unused on 64 bit machines in DBG, else not present]

extern DWORD pagemask;

extern heap ProcessHeap;

_inline void INITFREEBLOCK(LPVOID p, UINT size) {
	HEADPTR(p)->size = (size & H_SIZEMASK) | H_FREEBLOCK;
#if HEAP_SENTINELS
	HEADPTR(p)->sig = FREESIGHEAD;
	TAILPTR(p)->sig = FREESIGTAIL;
#endif
#ifdef DEBUG
	memset(HEAPTOPTR(p),0xcc,size);
#endif
}

_inline void INITUSEDBLOCK(LPVOID p, UINT size) {
	HEADPTR(p)->size = size & H_SIZEMASK;
#if HEAP_SENTINELS
	HEADPTR(p)->sig = ALLOCSIGHEAD;
	TAILPTR(p)->sig = ALLOCSIGTAIL;
#endif
}

_inline void INITDEADBLOCK(LPVOID p, UINT size) {
	HEADPTR(p)->size = (size & H_SIZEMASK) | H_DEADBLOCK;
#if HEAP_SENTINELS
	HEADPTR(p)->sig = DEADSIGHEAD;
	TAILPTR(p)->sig = DEADSIGTAIL;
#endif
}

BOOL IsSigValid(pheap pHeap, LPVOID ptr, DWORD allowfree) {
	if (!pHeap->pMem || ((DWORD)ptr < (DWORD)pHeap->pMem) || ((DWORD)ptr > (DWORD)pHeap->pHigh - HDRSIZE - TLRSIZE))
        return FALSE;
	if (!allowfree && !ISINUSE(ptr))
		return FALSE;
#if HEAP_SENTINELS
	if (HEADPTR(ptr)->sig != (ISFREE(ptr) ? FREESIGHEAD : ISDEAD(ptr) ? DEADSIGHEAD : ALLOCSIGHEAD))
		return FALSE;
#endif
	if ((DWORD)TAILPTR(ptr) + TLRSIZE > (DWORD)pHeap->pHigh)
		return FALSE;
#if HEAP_SENTINELS
	if (TAILPTR(ptr)->sig != (ISFREE(ptr) ? FREESIGTAIL : ISDEAD(ptr) ? DEADSIGTAIL :ALLOCSIGTAIL))
		return FALSE;
#endif
	return TRUE;
}

// Extend a free block, only through other free blocks
void GrowFree(pheap pHeap, LPVOID ptr) {
	LPVOID newptr;
	DWORD growth, numnew;

	growth = numnew = 0;
	newptr = NEXTBLOCK(pHeap, ptr);
	while ((newptr > ptr) && ISFREE(newptr)) {
		growth += ITEMSIZE(newptr) + HDRSIZE + TLRSIZE;
		numnew++;
		newptr = NEXTBLOCK(pHeap, newptr);
	}
	if (growth) {
		HEADPTR(ptr)->size += growth;
		pHeap->dwFree += (HDRSIZE+TLRSIZE)*numnew;
#ifdef DEBUG
		memset(HEAPTOPTR(ptr),0xcc,ITEMSIZE(ptr));
#endif
	}
	if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
		pHeap->dwMaxLeft = ITEMSIZE(ptr);
}

// Extend a free/dead block through other free/dead blocks if it will yield a free block of 'size'
DWORD MakePotentialSize(pheap pHeap, LPBYTE ptr, DWORD size) {
	LPBYTE ptr2 = ptr;
	int deadbytes = 0, numpieces = 0, lastdead = 0, size2, maxhit;
	do {
		numpieces++;
		if (lastdead = ISDEAD(ptr2)) {
			maxhit = (ITEMSIZE(ptr2) == pHeap->dwMaxLeft);
			deadbytes += ITEMSIZE(ptr2);
		}
		ptr2 = NEXTBLOCK(pHeap,ptr2);
	} while ((ptr2 != pHeap->pMem) && !ISINUSE(ptr2) && (ptr2 < ptr + size + HDRSIZE + TLRSIZE));
	if (ptr2 == pHeap->pMem)
		ptr2 = pHeap->pHigh;
	if (ptr + HDRSIZE + TLRSIZE + size > ptr2)
		return FALSE;
    pHeap->flOptions |= HEAP_IS_CHANGING_VM;
	if (lastdead) {
		LPBYTE tmpptr1, tmpptr2;
		tmpptr1 = (LPBYTE)((DWORD)(ptr + HDRSIZE + size + TLRSIZE + HDRSIZE + pagemask) & ~pagemask);
		tmpptr2 = (LPBYTE)((DWORD)(ptr2 - TLRSIZE) & ~pagemask);
		if (tmpptr1 < tmpptr2) {
			deadbytes -= ptr2 - TLRSIZE - tmpptr1;
			size2 = ptr2 - TLRSIZE - tmpptr1;
			ptr2 = tmpptr1 - HDRSIZE;
		} else
			lastdead = 0;
	}
	// since this does whole pages, it'll get the header for ptr2 if we're doing a lastdead
	if (!VirtualAlloc(ptr,ptr2-ptr,MEM_COMMIT, PAGE_READWRITE)) {
	    pHeap->flOptions &= ~HEAP_IS_CHANGING_VM;
		return FALSE;
	}
	if (lastdead) {
		INITDEADBLOCK(ptr2,size2);
		numpieces--;
		if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr2)))
			pHeap->dwMaxLeft = ITEMSIZE(ptr2);
		else if (maxhit)
			pHeap->dwMaxLeft = 0;
	}
    pHeap->flOptions &= ~HEAP_IS_CHANGING_VM;
	INITFREEBLOCK(ptr,ptr2-ptr-HDRSIZE-TLRSIZE);
	numpieces--;
	pHeap->dwCommitted += deadbytes;
	pHeap->dwFree += deadbytes;
    pHeap->dwFree += numpieces*(HDRSIZE+TLRSIZE);
	if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
		pHeap->dwMaxLeft = ITEMSIZE(ptr);

	return TRUE;
}

// Break a free block into two pieces (if enough room)
void SplitFreeBlock(pheap pHeap, LPVOID ptr, UINT size) {
	UINT cursize;
	LPVOID ptr2;
	cursize = ITEMSIZE(ptr);
	if (pHeap->dwMaxLeft == cursize)
		pHeap->dwMaxLeft = 0;
	if (cursize - size > HDRSIZE + TLRSIZE) {
		ptr2 = (LPVOID)((DWORD)ptr + HDRSIZE + TLRSIZE + size);
		INITFREEBLOCK(ptr,size);
		INITFREEBLOCK(ptr2,cursize-size-HDRSIZE-TLRSIZE);
		// needed because of the "extension" case, since if we allowed the original free extension to be
		// added, it would immediately be removed and we'd set to 0.
		if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr2)))
			pHeap->dwMaxLeft = ITEMSIZE(ptr2);
		DEBUGCHK(pHeap->dwFree >= HDRSIZE + TLRSIZE);
		pHeap->dwFree -= HDRSIZE + TLRSIZE;
	}
}

// Shrink a block (splitting off another free block)
void ShrinkBlock(pheap pHeap, LPVOID ptr, UINT size) {
	UINT cursize;
	LPVOID ptr2;

	cursize = ITEMSIZE(ptr);
	if (cursize - size > HDRSIZE + TLRSIZE) {
		ptr2 = (LPVOID)((DWORD)ptr + HDRSIZE + TLRSIZE + size);
		HEADPTR(ptr)->size = size;
#if HEAP_SENTINELS
		TAILPTR(ptr)->sig = ALLOCSIGTAIL;
#endif
		INITFREEBLOCK(ptr2,cursize-size-HDRSIZE-TLRSIZE);
		if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr2)))
			pHeap->dwMaxLeft = ITEMSIZE(ptr2);
		pHeap->dwFree += cursize-size-HDRSIZE-TLRSIZE;
	}
}

#ifdef DEBUG

UINT WINAPI _HeapDump(HLOCAL hHeap) {
    pheap  pHeap = (pheap) hHeap;
	LPVOID ptr;
	UINT retval = 0;
	vaheapalloc *pvalloc;
    if (!pHeap)
        pHeap = &ProcessHeap;
    EnterCriticalSection(&hcs);
    try {
		if (!pHeap->pMem) {
	        DEBUGMSG(1, (TEXT("_HeapDump: Heap empty\r\n")));
	        goto done;
	    }
		while (pHeap) {
		    DEBUGMSG(1, (TEXT("flOptions:     0x%08X   pMem:    0x%08X\r\npHigh:         0x%08X   pLast:   0x%08X\r\ndwMaximumSize: 0x%08X   pCur:    0x%08X\r\ndwCommitted:   0x%08X   HDRSIZE: 0x%08X\r\ndwFree:        0x%08X   TLRSIZE: 0x%08X\r\n\r\n"),
    		             pHeap->flOptions,
        		         pHeap->pMem,
            		     pHeap->pHigh,
	                	 pHeap->dwLastCompact,
		                 pHeap->dwMaximumSize,
	    	             pHeap->pCur,
	        	         pHeap->dwCommitted,
	            	     HDRSIZE,
		                 pHeap->dwFree,
		                 TLRSIZE));
			ptr = pHeap->pMem;
			do {
				if (!IsSigValid(pHeap,ptr,1)) {
					retval = 1;
					goto done;
				}
#if HEAP_SENTINELS
		        DEBUGMSG(1, (TEXT("0x%08X %s size: 0x%08X sig: 0x%08X\r\n"),
	    	             ptr, ISFREE(ptr) ? TEXT("FREEBLOCK") : ISDEAD(ptr) ? TEXT("DEADBLOCK") : TEXT("USEDBLOCK"),
	        	         ITEMSIZE(ptr), HEADPTR(ptr)->sig));
		        DEBUGMSG(1, (TEXT("0x%08X                            sig: 0x%08X\r\n"),
	    	             TAILPTR(ptr), TAILPTR(ptr)->sig));
#else
		        DEBUGMSG(1, (TEXT("0x%08X %s size: 0x%08X\r\n"),
	    	             ptr, ISFREE(ptr) ? TEXT("FREEBLOCK") : ISDEAD(ptr) ? TEXT("DEADBLOCK") :TEXT("USEDBLOCK"),
	        	         ITEMSIZE(ptr)));
#endif
			} while ((ptr = NEXTBLOCK(pHeap, ptr)) != pHeap->pMem);
			for (pvalloc = pHeap->pVallocs; pvalloc; pvalloc = pvalloc->pnext)
				DEBUGMSG(1,(TEXT("Extended block at 0x%8.8lx, length 0x%8.8lx\r\n"),pvalloc->pBase,pvalloc->dwSize));
			pHeap = pHeap->pGrowthHeap;
		}
	    DEBUGMSG(1, (TEXT("-----------------------------------------------------\r\n")));
done:
		;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		retval = 1;
	}
    LeaveCriticalSection(&hcs);
	return retval;
}

#endif

BOOL WINAPI HeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) {
	BOOL retval = FALSE;
	LPVOID ptr, ptr2;
	pheap pHeap;
	vaheapalloc *pvalloc;
    if (dwFlags & ~HEAP_NO_SERIALIZE) {
	    SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
    }
	EnterCriticalSection(&hcs);
	try {
		if (lpMem) {
			ptr = PTRTOHEAP(lpMem);
			for (pHeap = (pheap)hHeap; pHeap; pHeap = pHeap->pGrowthHeap) {
				 if ((ptr >= pHeap->pMem) && (ptr < pHeap->pHigh)) {
				 	retval = IsSigValid(pHeap,ptr,0);
				 	goto done;
				}
				for (pvalloc = pHeap->pVallocs; pvalloc; pvalloc = pvalloc->pnext) {
					if (pvalloc->pBase == lpMem) {
						if (!((DWORD)pvalloc->pBase & 0xffff) && (pvalloc->dwSize >= CE_VALLOC_MINSIZE))
							retval = TRUE;
						goto done;
					}
				}
			}
		} else {
			for (pHeap = (pheap)hHeap; pHeap; pHeap = pHeap->pGrowthHeap) {
				if (ptr = pHeap->pMem) {
					do {
						if (!IsSigValid(pHeap,ptr,1))
							goto done;
						ptr2 = (LPVOID)((DWORD)ptr + HDRSIZE + ITEMSIZE(ptr) + TLRSIZE);
						if (ptr2 <= ptr)
							goto done;
					} while ((ptr = ptr2) != pHeap->pHigh);
				}
				for (pvalloc = pHeap->pVallocs; pvalloc; pvalloc = pvalloc->pnext) {
					if (((DWORD)pvalloc->pBase & 0xffff) || (pvalloc->dwSize < CE_VALLOC_MINSIZE))
						goto done;
				}
			}
			retval = TRUE;
		}
done:
		;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		retval = FALSE;
	}
	LeaveCriticalSection(&hcs);
#ifdef DEBUG
	if (!retval)
		_HeapDump(hHeap);
#endif
	return retval;
}

// Initialize a heap
BOOL SetupFixedHeap(pheap pHeap) {
	LPVOID pMem;
	if (!(pMem = VirtualAlloc(NULL,pHeap->dwMaximumSize,MEM_RESERVE,PAGE_NOACCESS)))
        return FALSE;
	pHeap->dwCommitted = pagemask + 1;
    if (!VirtualAlloc(pMem,pHeap->dwCommitted, MEM_COMMIT, PAGE_READWRITE)) {
    	VirtualFree(pMem,0,MEM_RELEASE);
		return FALSE;
	}
	DEBUGCHK(pHeap->dwCommitted >= HDRSIZE + TLRSIZE);
	pHeap->dwFree = pHeap->dwCommitted - HDRSIZE - TLRSIZE;
	pHeap->pHigh = (LPVOID)((LPBYTE)pMem + pHeap->dwCommitted);
	INITFREEBLOCK(pMem,pHeap->dwFree);
	pHeap->pCur = pMem;
	pHeap->pMem = pMem;	// heap is now initialized - this must come LAST!
	pHeap->dwMaxLeft = ITEMSIZE(pHeap->pCur);
	return TRUE;
}

LPBYTE InitSharedHeap(LPBYTE pMem, DWORD size, DWORD reserve) {
	LPBYTE pReserved, pData;
	pheap pHeap;
	LPVOID pAlloc;
	pReserved = pMem + ((sizeof(heap)+ALIGNMASK)&~ALIGNMASK);
	if (size) {
	    pAlloc = VirtualAlloc(pMem,pagemask+1, MEM_COMMIT, PAGE_READWRITE);
    	DEBUGCHK(pAlloc == pMem);
		pHeap = (pheap)pMem;
		pData = pReserved + ((reserve+ALIGNMASK)&~ALIGNMASK);
		size -= (pData - pMem);
		size &= ~ALIGNMASK;
		pHeap->dwCommitted   = pagemask + 1 - (pData - pMem);
		pHeap->pVallocs      = 0;
		pHeap->pGrowthHeap   = 0;
	    pHeap->flOptions     = HEAP_IS_SHARED;
	    pHeap->dwMaximumSize = size;
	    pHeap->pMem          = pData;
	    pHeap->pCur          = pData;
	    pHeap->pHigh         = (LPVOID)((LPBYTE)pData+pHeap->dwCommitted);
		DEBUGCHK(pHeap->dwCommitted >= HDRSIZE-TLRSIZE);
	    pHeap->dwFree        = pHeap->dwCommitted-HDRSIZE-TLRSIZE;
	    pHeap->dwLastCompact = 0;
		INITFREEBLOCK(pData,pHeap->dwFree);
	}
	return pReserved;
}

// Get size of heap block
DWORD FixedHeapSize(pheap pHeap, LPVOID ptr) {
	LPVOID ptr2;
	vaheapalloc *pva;
	pheap pTrav;
	EnterCriticalSection(&hcs);
	ptr2 = PTRTOHEAP(ptr);
	for (pTrav = pHeap; pTrav; pTrav = pTrav->pGrowthHeap) {
		if (pTrav->pMem && IsSigValid(pTrav,ptr2,0)) {
			LeaveCriticalSection(&hcs);
			return ITEMSIZE(ptr2);
		}
	}
	for (pva = pHeap->pVallocs; pva; pva = pva->pnext)
		if (pva->pBase == ptr) {
			LeaveCriticalSection(&hcs);
			return pva->dwSize;
		}
	LeaveCriticalSection(&hcs);
	return (DWORD)-1;
}

// Free a heap block
BOOL FixedHeapFree(pheap pHeap, LPVOID ptr) {
	LPVOID ptr2;
	vaheapalloc *pva, *pvaprev;
	pheap pTrav;
	ptr2 = PTRTOHEAP(ptr);
	for (pTrav = pHeap; pTrav; pTrav = pTrav->pGrowthHeap) {
		if (pTrav->pMem && IsSigValid(pTrav,ptr2,0)) {
			pTrav->dwFree += ITEMSIZE(ptr2);
			INITFREEBLOCK(ptr2,ITEMSIZE(ptr2));
			GrowFree(pTrav,ptr2);
			pTrav->pCur = ptr2;
			if ((pTrav->dwFree > (pTrav->dwCommitted >> 3)) &&
				((pTrav->dwFree > pTrav->dwLastCompact + (pagemask+1)*6) ||
				 ((UserKInfo[KINX_PAGEFREE]*UserKInfo[KINX_PAGESIZE] < 2*1024*1024) &&
				  (pTrav->dwFree > pTrav->dwLastCompact + (pagemask+1)*2))))
				FixedHeapCompact(pTrav);
			return TRUE;
		}
	}
	pvaprev = 0;
	for (pva = pHeap->pVallocs; pva; pvaprev = pva, pva = pva->pnext)
		if (pva->pBase == ptr) {
    		if (!pvaprev)
    			pHeap->pVallocs = pva->pnext;
    		else
    			pvaprev->pnext = pva->pnext;
	    	VirtualFree(pva->pBase,0,MEM_RELEASE);
    		FixedHeapFree(pHeap,pva);
    		return TRUE;
		}
	DEBUGCHK(0);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

// Alloc a heap block
LPVOID FixedHeapAlloc(pheap pHeapIn, UINT size, BOOL zero) {
   	pheap pHeap2;
	heap newheap;
	LPVOID ptr;
	UINT increase, presize;
	pheap pHeap;
	DWORD  dwMaxLeft;
#ifdef MEMTRACKING
	WCHAR Name[14];
#endif
	if (size & 0x80000000) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	size = (size + ALIGNMASK) & ~ALIGNMASK;
    if (size >= CE_VALLOC_MINSIZE) {
    	vaheapalloc *pva;
		if (!(ptr=VirtualAlloc(0,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)))
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		else {
			if (!(pva = FixedHeapAlloc(pHeapIn,sizeof(vaheapalloc),0))) {
				VirtualFree(ptr,0,MEM_RELEASE);
				ptr = 0;
			} else {
				pva->pBase = ptr;
				pva->dwSize = (size + pagemask) & ~pagemask;
				pva->pnext = pHeapIn->pVallocs;
				pHeapIn->pVallocs = pva;
			}
		}
#ifdef DEBUG
        if (!zero && ptr)
			memset(ptr, 0xcc, pva->dwSize);
#endif    
		return ptr;
	}
	for (pHeap = pHeapIn; pHeap; pHeap = pHeap->pGrowthHeap) {
		if (!pHeap->pMem)
			if (!SetupFixedHeap(pHeap))
				return 0;
		if ((!pHeap->dwMaxLeft && (size <= pHeap->dwFree)) || (size <= pHeap->dwMaxLeft)) {
			ptr = pHeap->pCur;
			dwMaxLeft = 0;
			do {
				if (!ISINUSE(ptr)) {
					if (ISFREE(ptr)) {
						GrowFree(pHeap,ptr);
						if (ITEMSIZE(ptr) >= size)
							goto FoundBlock;
					}
					DEBUGCHK(!pHeap->dwMaxLeft || (ITEMSIZE(ptr) <= pHeap->dwMaxLeft));
					if (ITEMSIZE(ptr) > dwMaxLeft)
						dwMaxLeft = ITEMSIZE(ptr);
				}
				ptr = NEXTBLOCK(pHeap,ptr);
			} while (((DWORD)ptr > (DWORD)pHeap->pCur) || 
	                 (((DWORD)ptr + ITEMSIZE(ptr)) < (DWORD)pHeap->pCur));
	        pHeap->dwMaxLeft = dwMaxLeft;
		}
		pHeap->pCur = pHeap->pMem;
	}
	for (pHeap = pHeapIn; pHeap; pHeap = pHeap->pGrowthHeap) {
		DEBUGCHK(pHeap->pMem);
		DEBUGCHK(pHeap->pCur == pHeap->pMem);
		ptr = pHeap->pMem;
		if (size <= (pHeap->dwFree + pHeap->dwMaximumSize - pHeap->dwCommitted)) {
			do {
				if (!ISINUSE(ptr) && MakePotentialSize(pHeap,ptr,size))
					goto FoundBlock;
				pHeap->pNexttolast = ptr;
				ptr = NEXTBLOCK(pHeap,ptr);
			} while (ptr != pHeap->pMem);
		}
	}
	for (pHeap = pHeapIn; pHeap; pHeap = pHeap->pGrowthHeap) {
		DEBUGCHK(pHeap->pMem);
		DEBUGCHK(pHeap->pCur == pHeap->pMem);
		if (size <= (pHeap->dwFree + ((DWORD)pHeap->pMem + pHeap->dwMaximumSize - (DWORD)pHeap->pHigh))) {
			if (!ISFREE(pHeap->pNexttolast) || ISDEAD(pHeap->pNexttolast)) {
				ptr = pHeap->pHigh;
				presize = 0;
			} else {
				ptr = pHeap->pNexttolast;
				presize = ITEMSIZE(pHeap->pNexttolast) + HDRSIZE + TLRSIZE;
			}
			increase = (size - presize + HDRSIZE + TLRSIZE + pagemask) & ~pagemask;
			if ((DWORD)pHeap->pHigh + increase <= (DWORD)pHeap->pMem + pHeap->dwMaximumSize) {
		        pHeap->flOptions |= HEAP_IS_CHANGING_VM;
				if (VirtualAlloc(pHeap->pHigh,increase,MEM_COMMIT,PAGE_READWRITE)) {
					pHeap->pHigh = (LPVOID)((DWORD)pHeap->pHigh + increase);
					pHeap->dwCommitted += increase;
					pHeap->dwFree += increase;
					if (!presize)
						pHeap->dwFree -= (HDRSIZE+TLRSIZE);
					INITFREEBLOCK(ptr,increase-HDRSIZE-TLRSIZE+presize);
					if (pHeap->dwMaxLeft && (ITEMSIZE(ptr) > pHeap->dwMaxLeft))
						pHeap->dwMaxLeft = ITEMSIZE(ptr);
				} else
					ptr = 0;
		        pHeap->flOptions &= ~HEAP_IS_CHANGING_VM;
		        goto FoundBlock;
		    }
		}
	}
	if (pHeapIn->flOptions & HEAP_IS_NEW)
		return 0;
	newheap.pNextHeap	  = 0;
	newheap.pGrowthHeap   = 0;
	newheap.pVallocs 	  = 0;
    newheap.flOptions     = (pHeapIn->flOptions & ~HEAP_IS_CHANGING_VM) | HEAP_IS_NEW;
    newheap.dwMaximumSize = CE_FIXED_HEAP_MAXSIZE;
    newheap.pMem          = NULL;
    newheap.pHigh         = NULL;
    newheap.pCur          = NULL;
    newheap.dwCommitted   = 0;
    newheap.dwFree        = 0;
    newheap.dwLastCompact = 0;
    newheap.dwMaxLeft     = 0;
   	if ((ptr = FixedHeapAlloc(&newheap,size,zero)) && (pHeap2 = FixedHeapAlloc(&newheap,sizeof(heap),0))) {
		*pHeap2 = newheap;
		pHeap2->pGrowthHeap = pHeapIn->pGrowthHeap;
		pHeapIn->pGrowthHeap = pHeap2;
		pHeap2->flOptions &= ~HEAP_IS_NEW;
#ifdef MEMTRACKING
		swprintf(Name, L"Heap %08X", pHeap2);
		pHeap2->dwMemType     = RegisterTrackedItem(Name);
#endif
		goto done;
	}
	if (newheap.pMem) {
		VirtualFree(newheap.pMem,newheap.dwMaximumSize,MEM_DECOMMIT);
		VirtualFree(newheap.pMem, 0, MEM_RELEASE);
	}
   	return 0;
FoundBlock:
	if (ptr) {
		SplitFreeBlock(pHeap,ptr,size);
		DEBUGCHK(pHeap->dwFree >= ITEMSIZE(ptr));
		pHeap->dwFree -= ITEMSIZE(ptr);
		if (pHeap->dwMaxLeft == ITEMSIZE(ptr))
			pHeap->dwMaxLeft = 0;
		INITUSEDBLOCK(ptr,ITEMSIZE(ptr));
		pHeap->pCur = NEXTBLOCK(pHeap,ptr);
		ptr = HEAPTOPTR(ptr);
		if (zero)
			memset(ptr,0,ITEMSIZE(PTRTOHEAP(ptr)));
#ifdef DEBUG
        if (!zero) {
        	UINT loop;
        	for (loop = 0; loop < ITEMSIZE(PTRTOHEAP(ptr)); loop++)
        		DEBUGCHK(((LPBYTE)ptr)[loop] == 0xcc);
        }
#endif    
	} else
		pHeap->pCur = pHeap->pMem;
done:
	return ptr;
}

// Realloc a heap block
LPVOID FixedHeapReAlloc(pheap pHeap, LPVOID ptr, UINT size, BOOL zero, BOOL move) {
	LPVOID newptr;
	UINT oldsize;
	pheap pTrav;
	vaheapalloc *pva, *pvaprev;
	for (pva = pHeap->pVallocs; pva; pva = pva->pnext) {
		if (pva->pBase == ptr) {
			if (size <= pva->dwSize) {
				if (!move)
					return ptr;
				if (!(newptr = FixedHeapAlloc(pHeap,size,0)))
					return ptr;
				memcpy(newptr,ptr,size);
			} else {
				if (!move)
					return 0;
				if (!(newptr = FixedHeapAlloc(pHeap,size,0)))
					return 0;
				memcpy(newptr,ptr,pva->dwSize);
				if (zero)
					memset((LPBYTE)newptr+pva->dwSize,0,size-pva->dwSize);
			}
			VirtualFree(pva->pBase,0,MEM_RELEASE);
			if (pHeap->pVallocs == pva)
				pHeap->pVallocs = pva->pnext;
			else {
				for (pvaprev = pHeap->pVallocs; pvaprev->pnext != pva; pvaprev = pvaprev->pnext)
					;
				pvaprev->pnext = pva->pnext;
			}
    		FixedHeapFree(pHeap,pva);
			return newptr;
		}
	}
	if (size & 0x80000000) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	size = (size + ALIGNMASK) & ~ALIGNMASK;
	newptr = ptr = PTRTOHEAP(ptr);
	for (pTrav = pHeap; pTrav; pTrav = pTrav->pGrowthHeap) {
		if (pTrav->pMem && IsSigValid(pTrav,ptr,0)) {
			oldsize = ITEMSIZE(ptr);
			if (size <= oldsize) {
				ShrinkBlock(pTrav,ptr,size);
				pTrav->pCur = NEXTBLOCK(pTrav,ptr);
				newptr = HEAPTOPTR(ptr);
				goto DoneRealloc;
			}
			newptr = NEXTBLOCK(pTrav,newptr);
			if ((newptr > ptr) && ISFREE(newptr)) {
				GrowFree(pTrav,newptr);
				if (ITEMSIZE(newptr)+HDRSIZE+TLRSIZE+oldsize >= size) {
					DEBUGCHK(pTrav->dwFree >= ITEMSIZE(newptr));
					pTrav->dwFree -= ITEMSIZE(newptr);
					if (pTrav->dwMaxLeft == ITEMSIZE(newptr))
						pTrav->dwMaxLeft = 0;
					INITUSEDBLOCK(ptr,oldsize+ITEMSIZE(newptr)+HDRSIZE+TLRSIZE);
					ShrinkBlock(pTrav,ptr,size);
					pTrav->pCur = NEXTBLOCK(pTrav,ptr);
					newptr = HEAPTOPTR(ptr);
					goto DoneRealloc;
				}
				pTrav->pCur = newptr;
			}
			if (move && (newptr = FixedHeapAlloc(pHeap,size,0))) {
				memcpy(newptr,HEAPTOPTR(ptr),oldsize);
				FixedHeapFree(pHeap,HEAPTOPTR(ptr));
				goto DoneRealloc;
			}
			return 0;
		}
	}
	DEBUGCHK(0);
	return 0;
DoneRealloc:
	if (zero && (size > oldsize))
		memset((LPBYTE)newptr+oldsize,0,FixedHeapSize(pHeap,newptr)-oldsize);
	return newptr;
}

// Compact a heap
void FixedHeapCompact(pheap pHeap) {
	LPBYTE ptr, ptr2;
	int numpieces, deadbytes, size, size2;
	if (!(ptr = pHeap->pMem) || (pHeap->flOptions & HEAP_IS_CHANGING_VM) || (pHeap->flOptions & HEAP_IS_SHARED))
		return;
	do {
		if (ISINUSE(ptr))
			ptr = NEXTBLOCK(pHeap, ptr);
		else {
			numpieces = deadbytes = 0;
			ptr2 = ptr;
			do {
				numpieces--;
				if (ITEMSIZE(ptr2) == pHeap->dwMaxLeft)
					pHeap->dwMaxLeft = 0;
				if (ISDEAD(ptr2))
					deadbytes -= ITEMSIZE(ptr2);
			} while (((ptr2 = NEXTBLOCK(pHeap, ptr2)) != pHeap->pMem) && !ISINUSE(ptr2));
			if (ptr2 != pHeap->pMem) {
				// see if there are going to be dead pages
				if (((DWORD)(ptr + HDRSIZE + pagemask) & ~pagemask) < ((DWORD)(ptr2 - TLRSIZE) & ~pagemask)) {
					// see if there is room at the start for a live free block
					size = pagemask + 1 - ((DWORD)ptr & pagemask);
					if (size > HDRSIZE + TLRSIZE + HDRSIZE) {
						INITFREEBLOCK(ptr, size - HDRSIZE - TLRSIZE - HDRSIZE);
						if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
							pHeap->dwMaxLeft = ITEMSIZE(ptr);
						ptr += size - HDRSIZE;
						numpieces++;
					}
					// now do the dead block
					size = ptr2 - ptr;
					if (((DWORD)(ptr2 - TLRSIZE) & pagemask) >= HDRSIZE + TLRSIZE)
						size -= ((DWORD)ptr2 - TLRSIZE) & pagemask;
					INITDEADBLOCK(ptr,size - HDRSIZE - TLRSIZE);
					if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
						pHeap->dwMaxLeft = ITEMSIZE(ptr);
					numpieces++;
					size2 = ((DWORD)(ptr2 - TLRSIZE) & ~pagemask) - ((DWORD)(ptr + HDRSIZE + pagemask) & ~pagemask);
					VirtualFree((LPVOID)((DWORD)(ptr+HDRSIZE+pagemask) & ~pagemask), size2, MEM_DECOMMIT);
					deadbytes += size - HDRSIZE - TLRSIZE;
					ptr += size;
				}
				// Do the final free block, if any
				if (ptr != ptr2) {
					DEBUGCHK(ptr + HDRSIZE + TLRSIZE <= ptr2);
					INITFREEBLOCK(ptr, ptr2 - ptr - HDRSIZE - TLRSIZE);
					if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
						pHeap->dwMaxLeft = ITEMSIZE(ptr);
					numpieces++;
				}
				pHeap->pCur = pHeap->pMem;
				pHeap->dwCommitted -= deadbytes;
				DEBUGCHK((deadbytes + numpieces * (int)(HDRSIZE + TLRSIZE) < 0) || 
					(pHeap->dwFree >= deadbytes + numpieces * (HDRSIZE + TLRSIZE)));
				pHeap->dwFree -= deadbytes + numpieces*(HDRSIZE+TLRSIZE);
				ptr = NEXTBLOCK(pHeap, ptr2);
			} else {
				if (ptr == pHeap->pMem) {
					VirtualFree(pHeap->pMem,(LPBYTE)pHeap->pHigh - ptr,MEM_DECOMMIT);
					VirtualFree(pHeap->pMem,0,MEM_RELEASE);
				    pHeap->pMem = NULL;
				    pHeap->pHigh = NULL;
				    pHeap->pCur = NULL;
				    pHeap->dwCommitted = 0;
				    pHeap->dwFree = 0;
				    pHeap->dwMaxLeft = 0;
				} else {
					// see if there are going to be free bytes
					if ((DWORD)ptr & pagemask) {
						size = pagemask + 1 - ((DWORD)ptr & pagemask);
						if (size < HDRSIZE + TLRSIZE) {
#ifdef DEBUG
							// The place we want to put the tail signature could be on a free
							// block.  Since this is only in debug, and pretty rare, just
							// ignore the case.
							pHeap->pCur = pHeap->pMem;
							break;
#else						
							size += pagemask + 1;
#endif
						}
						INITFREEBLOCK(ptr, size - HDRSIZE - TLRSIZE);
						if (pHeap->dwMaxLeft && (pHeap->dwMaxLeft < ITEMSIZE(ptr)))
							pHeap->dwMaxLeft = ITEMSIZE(ptr);
						numpieces++;
						ptr = ptr + size;
					}
					if (ptr != pHeap->pHigh) {
						size2 = (LPBYTE)pHeap->pHigh - ptr;
						VirtualFree(ptr, size2, MEM_DECOMMIT);
						deadbytes += size2;
						pHeap->pHigh = ptr;
					}
					pHeap->pCur = pHeap->pMem;
					pHeap->dwCommitted -= deadbytes;
					DEBUGCHK((deadbytes + numpieces * (int)(HDRSIZE + TLRSIZE) < 0) || 
						(pHeap->dwFree >= deadbytes + numpieces * (HDRSIZE + TLRSIZE)));
					pHeap->dwFree -= deadbytes + numpieces * (HDRSIZE + TLRSIZE);
				}
				break;
			}
		}
	} while (ptr != pHeap->pMem);
	pHeap->dwLastCompact = pHeap->dwFree;
}

