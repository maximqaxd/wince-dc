/*
 *		NK Kernel heap code
 *
 *		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *		heap.c
 *
 * Abstract:
 *
 *		This file implements the NK kernel heap
 *
 */

/* The kernel heap is designed to support objects which are smaller than one page
   in length.  The design assumes that kernel objects are either a divisor of the
   page size, or small enough that the remainder when the page size is divided by
   the object size is very small.
   The design of the heap is a structure (heapptr) which contains the size of each
   object, whether the memory is to be zero'ed by the alloc routine for that object,
   and a pointer to a list of pages.  Pages, once added to a pool, are never freed.
   When a page is added to a pool, it is linked into the master pointer for that
   pool as a linked list of objects.  Each object has as it's first DWORD a pointer
   to the next object on the list.  So, to allocate an object, if the pointer it
   non-null, the block pointed to is returned, and the new pointer is set to be
   the first DWORD of where the pointer was pointing.  If the pointer is NULL, then
   a new page is threaded onto the list, and the same procedure is followed.  When
   an object is freed, it is simply added to the head of the list.  Because the
   pool number is passed to the free and alloc routines, and since the block sizes
   are fixed at compile time, it is very efficient time-wise. */

#include "kernel.h"

#define ALIGNED_SIZE(n) ((HEAP_SIZE ## n + 3) & ~3)

#define HEAPENTRY(n) {ALIGNED_SIZE(n), 0, HEAP_NAME ## n, 0, 0, 0}

heapptr_t heapptr[NUMARENAS] = {
	HEAPENTRY(0),	HEAPENTRY(1),	HEAPENTRY(2),	HEAPENTRY(3),
	HEAPENTRY(4),	HEAPENTRY(5),	HEAPENTRY(6),
#if PAGE_SIZE == 4096
	HEAPENTRY(7),
#endif
};

DWORD dwIndex; // ptr to arena to receive tail of heap page
uint SmallestSize;

LPBYTE GrabFirstPhysPage(DWORD dwCount);

// Distrubute unusable bytes on the end of a heap page to the heap item
// free lists.

void FreeSpareBytes(LPBYTE lpb, uint size) {
	DWORD index;
	heapptr_t *hptr;
	BOOL bDidFour;
    DEBUGMSG(ZONE_MEMORY, (TEXT("FSB: %8.8lx (%d)\r\n"), lpb, size));
    do {
   	    index = dwIndex;
    } while ((DWORD)InterlockedTestExchange(&dwIndex, index, (index+1)%NUMARENAS) != index);
	if (size >= heapptr[3].size) {
	   	InterlockedPushList(&heapptr[3].fptr, lpb);
		InterlockedIncrement(&heapptr[3].cMax);
       	size -= heapptr[3].size;
       	lpb += heapptr[3].size;
	}
    bDidFour = 0;
	while (size >= SmallestSize) {
	    hptr = &heapptr[index];
	    if ((size >= hptr->size) && (!bDidFour || (index != 4))) {
	    	InterlockedPushList(&hptr->fptr, lpb);
			InterlockedIncrement(&hptr->cMax);
        	size -= hptr->size;
        	lpb += hptr->size;
	    }
	    if (index == 4)
	    	bDidFour = 1;
		index = (index+1) % NUMARENAS;
	}
    DEBUGMSG(ZONE_MEMORY, (TEXT("FSB: waste=%d\r\n"), size));
    KInfoTable[KINX_HEAP_WASTE] += size;
}

// Initialize the kernel heap

void HeapInit(void) {
	int loop;
	KInfoTable[KINX_KHEAP] = (long)heapptr;
	/* Find the smallest size of block. */
	SmallestSize = heapptr[0].size;
	for (loop = 0; loop < NUMARENAS; loop++) {
		DEBUGMSG(ZONE_MEMORY,(TEXT("HeapInit: Entry %2.2d, size %4.4d\r\n"),loop, heapptr[loop].size));
	    DEBUGCHK((heapptr[loop].size & 3) == 0);
	    if (heapptr[loop].size < SmallestSize)
	        SmallestSize = heapptr[loop].size;
	}
   	KInfoTable[KINX_HEAP_WASTE] = 0;
}

LPBYTE pFree;
DWORD dwLen;

LPVOID GetKHeap(DWORD size) {
	LPBYTE pRet, pNeedsFree;
	DWORD dwNeedsFreeLen;
	pRet = 0;
	INTERRUPTS_OFF();
	if (dwLen >= size) {
		pRet = pFree;
		dwLen -= size;
		pFree += size;
		INTERRUPTS_ON();
	} else {
		INTERRUPTS_ON();
		if (pRet = (LPBYTE)KCall((PKFN)GrabFirstPhysPage,1)) {
			InterlockedIncrement(&KInfoTable[KINX_SYSPAGES]);
			pNeedsFree = pRet + size;
			dwNeedsFreeLen = PAGE_SIZE - size;
			INTERRUPTS_OFF();
			if (dwLen < PAGE_SIZE - size) {
				pNeedsFree = pFree;
				dwNeedsFreeLen = dwLen;
				pFree = pRet + size;
				dwLen = PAGE_SIZE - size;
			}
			INTERRUPTS_ON();
			FreeSpareBytes(pNeedsFree,dwNeedsFreeLen);
		}
	}
	return pRet;
}

// Allocate a block from the kernel heap

LPVOID AllocMem(ulong poolnum) {
	LPBYTE pptr;
	heapptr_t *hptr;
	DEBUGCHK(poolnum<NUMARENAS);
	hptr = &heapptr[poolnum];
	if (!(pptr = InterlockedPopList(&hptr->fptr))) {
		if (!(pptr = GetKHeap(hptr->size)))
			return 0;
		InterlockedIncrement(&hptr->cMax);
	}
	InterlockedIncrement(&hptr->cUsed);
	return pptr;
}

// Free a block from the kernel heap

VOID FreeMem(LPVOID pMem, ulong poolnum) {
	heapptr_t *hptr;
	DEBUGCHK(poolnum<NUMARENAS);
	hptr = &heapptr[poolnum];
#ifdef DEBUG
	memset(pMem,0xab,hptr->size);
#endif
   	InterlockedPushList(&hptr->fptr, pMem);
	InterlockedDecrement(&hptr->cUsed);
}

LPName AllocName(DWORD dwLen) {
	WORD wPool, wBestLen = 0xffff, wBestPool = 0;
	LPName lpn;
	dwLen += 2; // for pool number
	DEBUGCHK(dwLen <= (2+(MAX_PATH*2)));
	for (wPool = 0; wPool < NUMARENAS; wPool++) {
		if ((heapptr[wPool].size >= dwLen) && (heapptr[wPool].size < wBestLen)) {
			wBestLen = heapptr[wPool].size;
			wBestPool = wPool;
		}
	}
	lpn = AllocMem(wBestPool);
	if (lpn)
		lpn->wPool = wBestPool;
	return lpn;
}

