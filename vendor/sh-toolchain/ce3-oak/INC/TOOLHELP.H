// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __TOOLHELP_H__
#define __TOOLHELP_H__

#include "tlhelp32.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THSNAP_RESERVE (1024*1024)

#define TH32CS_SNAPNOHEAPS	0x40000000	// optimization for text shell to not snapshot heaps

typedef struct TH32HEAPENTRY {
	HEAPENTRY32 heapentry;
	struct TH32HEAPENTRY *pNext;
} TH32HEAPENTRY;

typedef struct TH32HEAPLIST {
	HEAPLIST32 heaplist;
	TH32HEAPENTRY *pHeapEntry;
	struct TH32HEAPLIST *pNext;
} TH32HEAPLIST, *PTH32HEAPLIST;	

typedef struct TH32PROC {
	PROCESSENTRY32 procentry;
	TH32HEAPENTRY *pMainHeapEntry;
	struct TH32PROC *pNext;
} TH32PROC;

typedef struct TH32MOD {
	MODULEENTRY32 modentry;
	struct TH32MOD *pNext;
} TH32MOD;

typedef struct TH32THREAD {
	THREADENTRY32 threadentry;
	struct TH32THREAD *pNext;
} TH32THREAD;

typedef struct THSNAP {
	LPBYTE pNextFree;
	LPBYTE pHighCommit;
	LPBYTE pHighReserve;
	TH32PROC *pProc;
	TH32MOD *pMod;
	TH32THREAD *pThread;
	TH32HEAPLIST *pHeap;
} THSNAP;

#ifdef __cplusplus
}
#endif

#endif

