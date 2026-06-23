// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef _HEAP_H
#define _HEAP_H

#define CE_FIXED_HEAP_MAXSIZE		0x00060000	// each heap has at most 384Kbytes
#define CE_VALLOC_MINSIZE			0x00030000	// allocations of 192K or more are VirtualAlloc's

#define FREESIGHEAD		0x7f362a4bUL
#define FREESIGTAIL		0x379fe902UL
#define ALLOCSIGHEAD	0xa9e4b6f1UL
#define ALLOCSIGTAIL	0x58637fb5UL
#define DEADSIGHEAD		0x12cd15eaUL
#define DEADSIGTAIL		0x34baf830UL

#define H_FREEBLOCK		0x00000001
#define H_DEADBLOCK		0x00000002	// no backing store
#define H_FLAGMASK		0x00000003
#define H_SIZEMASK		0xfffffffc

#define HEAP_IS_PROC_HEAP	0x00001000
#define HEAP_IS_CHANGING_VM	0x00002000
#define HEAP_IS_SHARED		0x00004000
#define HEAP_IS_NEW			0x00008000

#ifdef DEBUG
#define HEAP_SENTINELS 1
#else
#define HEAP_SENTINELS 0
#endif

#define HEAP_8BYTE_ALIGN	1	// if 1, heap is 8 byte aligned; if 0 heap is 4 byte aligned

#define ALIGNMASK (3 + 4*(HEAP_8BYTE_ALIGN))	// should be 3 or 7

extern CRITICAL_SECTION hcs;

typedef struct vaheapalloc {
	LPVOID pBase;
	DWORD dwSize;
	struct vaheapalloc *pnext;
} vaheapalloc;

typedef struct heap {
	struct heap *pNextHeap;	// next heap in linked list of all heaps
	struct heap *pGrowthHeap;	// next heap chained in as part of this heap for growth
	struct vaheapalloc *pVallocs;
    DWORD  flOptions;
    DWORD  dwMaximumSize;
    LPVOID pMem;
    LPVOID pHigh;
    LPVOID pCur;
    LPVOID pNexttolast;
    DWORD  dwCommitted;
    DWORD  dwFree;
    DWORD  dwLastCompact;
    DWORD  dwMaxLeft;
#ifdef MEMTRACKING
    DWORD  dwMemType;
#endif
} heap, *pheap;

typedef struct heaphdr {
	DWORD size;
#if HEAP_SENTINELS
	DWORD sig;
#elif HEAP_8BYTE_ALIGN
	DWORD unused;
#endif
} heaphdr, *pheaphdr;

#define HDRSIZE sizeof(heaphdr)

#if HEAP_SENTINELS

typedef struct heaptlr {
	DWORD sig;
#if HEAP_8BYTE_ALIGN
	DWORD unused;
#endif
} heaptlr, *pheaptlr;

#define TLRSIZE sizeof(heaptlr)

#else

#define TLRSIZE 0
typedef LPVOID pheaptlr;

#endif

#define ITEMSIZE(p) (((pheaphdr)(p))->size & H_SIZEMASK)
#define ITEMFLAGS(p) (((pheaphdr)(p))->size & H_FLAGMASK)
#define TAILPTR(p) ((pheaptlr)((DWORD)(p) + HDRSIZE + ITEMSIZE(p)))
#define HEADPTR(p) ((pheaphdr)(p))
#define ISFREE(p) (((pheaphdr)(p))->size & H_FREEBLOCK)
#define ISDEAD(p) (((pheaphdr)(p))->size & H_DEADBLOCK)
#define ISINUSE(p) (!(((pheaphdr)(p))->size & (H_FREEBLOCK|H_DEADBLOCK)))

#define PTRTOHEAP(p) ((LPVOID)((DWORD)(p) - HDRSIZE))
#define HEAPTOPTR(p) ((LPVOID)((DWORD)(p) + HDRSIZE))

_inline LPVOID NEXTBLOCK(pheap pHeap, LPVOID p) {
	LPVOID p2 = (LPVOID)((DWORD)(p) + HDRSIZE + ITEMSIZE(p) + TLRSIZE);
	return (p2 >= pHeap->pHigh ? pHeap->pMem : p2);
}

BOOL IsSigValid(pheap pHeap, LPVOID ptr, DWORD allowfree);
void GrowFree(pheap pHeap, LPVOID ptr);
void SplitFreeBlock(pheap pHeap, LPVOID ptr, UINT size);
void ShrinkBlock(pheap pHeap, LPVOID ptr, UINT size);
UINT WINAPI _HeapDump(HLOCAL hHeap);
BOOL SetupFixedHeap(pheap pHeap);

DWORD FixedHeapSize(pheap pHeap, LPCVOID ptr);
BOOL FixedHeapFree(pheap pHeap, LPVOID ptr);
LPVOID FixedHeapAlloc(pheap pHeap, UINT size, BOOL zero);
LPVOID FixedHeapReAlloc(pheap pHeap, LPVOID ptr, UINT size, 
                        BOOL zero, BOOL move);
void FixedHeapCompact(pheap pHeap);

#endif // _HEAP_H

