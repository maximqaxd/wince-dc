// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#define MAX_SUSPEND_COUNT 127

#define MAX_PRIORITY_LEVELS 256
#define PRIORITY_LEVELS_HASHSIZE 32

#define PRIORITY_LEVELS_HASHSCALE (MAX_PRIORITY_LEVELS/PRIORITY_LEVELS_HASHSIZE)
#define MAX_WIN32_PRIORITY_LEVELS 8

#define THREAD_RT_PRIORITY_ABOVE_NORMAL    (MAX_PRIORITY_LEVELS-6)
#define THREAD_RT_PRIORITY_NORMAL          (MAX_PRIORITY_LEVELS-5)
#define THREAD_RT_PRIORITY_IDLE            (MAX_PRIORITY_LEVELS-1)

typedef struct {
	PTHREAD	pRunnable;	/* List of runnable threads */
	PTHREAD pth;		/* Currently running thread */
	PTHREAD pHashThread[PRIORITY_LEVELS_HASHSIZE];
} RunList_t;

typedef struct PROXY *LPPROXY;

typedef struct PROXY {
	LPPROXY pQPrev;			/* Previous proxy for this object queue, must be first for ReQueueByPriority */
	LPPROXY pQNext;			/* Next proxy for this object queue */
	LPPROXY pQUp;
	LPPROXY pQDown;
	LPPROXY pThLinkNext;	/* Next proxy for this thread */
	LPBYTE  pObject;		/* Pointer to object we're blocked on */
	BYTE 	bType;			/* Type of object we're blocked on */
	BYTE	prio;			/* Current prio we're enqueued on */
	WORD	wCount;			/* Count matching thread's wCount */
	PTHREAD pTh;			/* Thread "owning" this proxy */
	DWORD	dwRetVal;		/* Return value if this is why we wake up */
} PROXY;

// if this structure changes, must change AllocName

typedef struct {
	WORD wPool;
	WCHAR name[MAX_PATH];	/* name of item */
} Name, * LPName;

typedef struct EVENT *LPEVENT;

typedef struct EVENT {
	HANDLE hNext;			/* Next event in list */
	LPPROXY pProxList;
	LPPROXY pProxHash[PRIORITY_LEVELS_HASHSIZE];
	BYTE onequeue;
	BYTE state;				/* TRUE: signalled, FALSE: unsignalled */
	BYTE manualreset;		/* TRUE: manual reset, FALSE: autoreset */
	BYTE bMaxPrio;
	Name *name;				/* 0x50: points to name of event */
	LPPROXY pIntrProxy;
} EVENT;

#define HT_MANUALEVENT	0xff // special handle type for identifying manual reset events in proxy code
#define HT_CRITSEC		0xfe // special handle type for identifying critical sections in proxy code

typedef struct STUBEVENT *LPSTUBEVENT;

typedef struct STUBEVENT {
	DWORD	dwPad;
	LPPROXY pProxList;		/* Root of stub event blocked list */
} STUBEVENT;

typedef struct CRIT *LPCRIT;

typedef struct CRIT {
	LPCRITICAL_SECTION lpcs;  /* Pointer to critical_section structure */
	LPPROXY pProxList;
	LPPROXY pProxHash[PRIORITY_LEVELS_HASHSIZE];
	BYTE bListed;			  /* Is this on someone's owner list */
	BYTE bListedPrio;
	BYTE iOwnerProc;		  /* Index of owner process */
	BYTE bPad;
	struct CRIT * pPrevOwned; /* Prev crit/mutex (for prio inversion) */
	struct CRIT * pNextOwned; /* Next crit/mutex section owned (for prio inversion) */
	struct CRIT * pUpOwned;
	struct CRIT * pDownOwned;
	LPCRIT pNext;			  /* Next CRIT in list */
} CRIT;

typedef struct MUTEX *LPMUTEX;

typedef struct MUTEX {
	HANDLE hNext;		/* Next mutex in list */
	LPPROXY pProxList;
	LPPROXY pProxHash[PRIORITY_LEVELS_HASHSIZE];
	BYTE bListed;
	BYTE bListedPrio;
	WORD LockCount;			/* current lock count */
	struct MUTEX *pPrevOwned; /* Prev crit/mutex owned (for prio inversion) */
	struct MUTEX *pNextOwned; /* Next crit/mutex owned (for prio inversion) */
	struct MUTEX *pUpOwned;
	struct MUTEX *pDownOwned;
	struct Thread *pOwner; 	/* owner thread */
	Name *name;				/* points to name of event */
} MUTEX;

ERRFALSE(offsetof(CRIT, lpcs) == offsetof(MUTEX, hNext));

typedef struct SEMAPHORE *LPSEMAPHORE;

typedef struct SEMAPHORE {
	HANDLE hNext;		/* Next semaphore in list */
	LPPROXY pProxList;
	LPPROXY pProxHash[PRIORITY_LEVELS_HASHSIZE];
	LONG lCount;			/* current count */
	LONG lMaxCount;			/* Maximum count */
	LONG lPending;			/* Pending count */
	Name *name;			/* points to name of event */
} SEMAPHORE;

// How to tell a mutex from a crit
#define ISMUTEX(P) (!((LPCRIT)(P))->lpcs || ((DWORD)((LPCRIT)(P))->lpcs & 0x2))

typedef struct CLEANEVENT {
	struct CLEANEVENT *ceptr;
	LPVOID base;
	DWORD size;
	DWORD op;
} CLEANEVENT, *LPCLEANEVENT;

typedef struct THREADTIME {
	struct THREADTIME *pnext;
	HANDLE hTh;
	FILETIME CreationTime;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;
} THREADTIME, *LPTHREADTIME;

extern void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, DWORD dwVMBase, BOOL kMode, ulong param);
LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
	LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2);
void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, BOOL kmode,
	ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4);

VOID MakeRun(PTHREAD pth);
DWORD ThreadResume(PTHREAD pth);
void KillSpecialThread(void);

#endif

