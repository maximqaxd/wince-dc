/*
 *              NK Kernel scheduler code
 *
 *              Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *              schedule.c
 *
 * Abstract:
 *
 *              This file implements the scheduler for the NK kernel
 *
 *
 */

/* @doc EXTERNAL KERNEL */
/* @topic Kernel Entrypoints | Kernel Entrypoints */

#include "kernel.h"
#ifdef ARM
#include "nkarm.h"
#endif

BOOL PageOutForced, bLastIdle;

DWORD dwDefaultThreadQuantum;

typedef void (* LogThreadCreate_t)(DWORD, DWORD);
typedef void (* LogThreadDelete_t)(DWORD, DWORD);
typedef void (* LogProcessCreate_t)(DWORD);
typedef void (* LogProcessDelete_t)(DWORD);
typedef void (* LogThreadSwitch_t)(DWORD, DWORD);

LogThreadCreate_t pLogThreadCreate;
LogThreadDelete_t pLogThreadDelete;
LogProcessCreate_t pLogProcessCreate;
LogProcessDelete_t pLogProcessDelete;
LogThreadSwitch_t pLogThreadSwitch;

extern void (* pEdbgInitializeInterrupt)(void);

/* General design: 
	There are many queues in the system, for all categories of processes.  All
	processes are on exactly one queue, with the exception of the currently
	running process (if any) which is on the NULL queue.  Runnable processes are
	on one of the run queues (there are several, one for each priority).  Examples
	of other queues are for sleeping, blocked on a critical section (one per
	critical section), etc.  Preemption grabs the current thread (if any) and puts
	it at the end of the queue for that thread's priority, and then grabs the
	highest priority thread (which should, by definition, always be at the same
	priority as the preempted thread, else the other thread would have been
	running [in the case of a higher prio], or we would have continued running
	the preempted thread [in the case of a lower prio].  Kernel routines take
	threads off of queues and put them on other queues.  NextThread finds the
	next thread to run, or returns 0 if there are no runnable threads.  We preempt
	with a fixed timeslice (probably 10ms).
*/

CRITICAL_SECTION csDbg, DbgApiCS, NameCS, CompCS, LLcs, ModListcs, WriterCS, MapCS, PagerCS;
CRITICAL_SECTION MapNameCS, PhysCS, ppfscs, ODScs, RFBcs, VAcs, rtccs, PageOutCS;

#ifdef DEBUG
CRITICAL_SECTION EdbgODScs, TimerListCS;
#endif

THREADTIME *TTList;

#if defined(x86)
extern PTHREAD g_CurFPUOwner;
#endif

RunList_t RunList;
PTHREAD SleepList;
HANDLE hEventList;
HANDLE hMutexList;
HANDLE hSemList;
DWORD dwSleepMin;
DWORD dwPreempt;
DWORD dwPartialDiffMSec = 0;
BOOL bPreempt;
DWORD ticksleft;

DWORD currmaxprio = MAX_PRIORITY_LEVELS - 1;
DWORD CurrTime;
PTHREAD pOOMThread;

SYSTEMTIME CurAlarmTime;

HANDLE hAlarmEvent, hAlarmThreadWakeup; 
PTHREAD pCleanupThread;

PROCESS ProcArray[MAX_PROCESSES];
PROCESS *kdProcArray = ProcArray;
struct KDataStruct *kdpKData = &KData;

HANDLE hCoreDll;
void (*TBFf)(LPVOID, ulong);
void (*MTBFf)(LPVOID, ulong, ulong, ulong, ulong);
void (*CTBFf)(LPVOID, ulong, ulong, ulong, ulong);
BOOL (*KSystemTimeToFileTime)(LPSYSTEMTIME, LPFILETIME);
LONG (*KCompareFileTime)(LPFILETIME, LPFILETIME);
BOOL (*KLocalFileTimeToFileTime)(const FILETIME *, LPFILETIME);
BOOL (*KFileTimeToSystemTime)(const FILETIME *, LPSYSTEMTIME);
void (*pPSLNotify)(DWORD, DWORD, DWORD);
BOOL (*pGetHeapSnapshot)(THSNAP *pSnap, BOOL bMainOnly, LPVOID *pDataPtr);
HANDLE (*pGetProcessHeap)(void);

LPVOID pExitThread;
LPDWORD pIsExiting;
extern Name *pDebugger;

void DoPageOut(void);
extern DWORD PageOutNeeded;
BYTE GetHighPos(DWORD);

typedef struct RTs {
	PTHREAD pHelper;	// must be first
	DWORD dwBase, dwLen;// if we're freeing the stack
	PPROCESS pProc;		// if we're freeing the proc
	LPTHRDDBG pThrdDbg; // if we're freeing a debug structure
	HANDLE hThread; 	// if we're freeing a handle / threadtime
	PTHREAD pThread;	// if we're freeing a thread structure
	CLEANEVENT *lpce1;
	CLEANEVENT *lpce2;
	CLEANEVENT *lpce3;
	LPDWORD pdwDying;
} RTs;

const PFNVOID ThrdMthds[] = {
	(PFNVOID)SC_ThreadCloseHandle,
	(PFNVOID)0,
	(PFNVOID)UB_ThreadSuspend,
	(PFNVOID)SC_ThreadResume,
	(PFNVOID)SC_ThreadSetPrio,
	(PFNVOID)SC_ThreadGetPrio,
	(PFNVOID)SC_ThreadGetCode,
	(PFNVOID)SC_ThreadGetContext,
	(PFNVOID)SC_ThreadSetContext,
	(PFNVOID)SC_ThreadTerminate,
	(PFNVOID)SC_CeGetThreadPriority,
	(PFNVOID)SC_CeSetThreadPriority,
	(PFNVOID)SC_CeGetThreadQuantum,
	(PFNVOID)SC_CeSetThreadQuantum,
};

const PFNVOID ProcMthds[] = {
	(PFNVOID)SC_ProcCloseHandle,
	(PFNVOID)0,
	(PFNVOID)SC_ProcTerminate,
	(PFNVOID)SC_ProcGetCode,
	(PFNVOID)0,
	(PFNVOID)SC_ProcFlushICache,
	(PFNVOID)SC_ProcReadMemory,
	(PFNVOID)SC_ProcWriteMemory,
	(PFNVOID)SC_ProcDebug,
};

const PFNVOID EvntMthds[] = {
	(PFNVOID)SC_EventCloseHandle,
	(PFNVOID)0,
	(PFNVOID)SC_EventModify,
	(PFNVOID)SC_EventAddAccess,
};

const PFNVOID MutxMthds[] = {
	(PFNVOID)SC_MutexCloseHandle,
	(PFNVOID)0,
	(PFNVOID)SC_ReleaseMutex,
};

const PFNVOID SemMthds[] = {
	(PFNVOID)SC_SemCloseHandle,
	(PFNVOID)0,
	(PFNVOID)SC_ReleaseSemaphore,
};

const CINFO cinfThread = {
	"THRD",
	DISPATCH_KERNEL_PSL,
	SH_CURTHREAD,
	sizeof(ThrdMthds)/sizeof(ThrdMthds[0]),
	ThrdMthds
};

const CINFO cinfProc = {
	"PROC",
	DISPATCH_KERNEL_PSL,
	SH_CURPROC,
	sizeof(ProcMthds)/sizeof(ProcMthds[0]),
	ProcMthds
};

const CINFO cinfEvent = {
	"EVNT",
	DISPATCH_KERNEL_PSL,
	HT_EVENT,
	sizeof(EvntMthds)/sizeof(EvntMthds[0]),
	EvntMthds
};

const CINFO cinfMutex = {
	"MUTX",
	DISPATCH_KERNEL_PSL,
	HT_MUTEX,
	sizeof(MutxMthds)/sizeof(MutxMthds[0]),
	MutxMthds
};

const CINFO cinfSem = {
	"SEMP",
	DISPATCH_KERNEL_PSL,
	HT_SEMAPHORE,
	sizeof(SemMthds)/sizeof(SemMthds[0]),
	SemMthds
};

ERRFALSE(offsetof(EVENT, pProxList) == offsetof(PROCESS, pProxList));
ERRFALSE(offsetof(EVENT, pProxList) == offsetof(THREAD, pProxList));
ERRFALSE(offsetof(EVENT, pProxList) == offsetof(STUBEVENT, pProxList));

ERRFALSE(offsetof(CRIT, pProxList) == offsetof(SEMAPHORE, pProxList));
ERRFALSE(offsetof(CRIT, pProxList) == offsetof(MUTEX, pProxList));
ERRFALSE(offsetof(CRIT, pProxList) == offsetof(EVENT, pProxList));
ERRFALSE(offsetof(CRIT, pProxHash) == offsetof(SEMAPHORE, pProxHash));
ERRFALSE(offsetof(CRIT, pProxHash) == offsetof(MUTEX, pProxHash));
ERRFALSE(offsetof(CRIT, pProxHash) == offsetof(EVENT, pProxHash));

ERRFALSE(offsetof(CRIT, bListed) == offsetof(MUTEX, bListed));
ERRFALSE(offsetof(CRIT, bListedPrio) == offsetof(MUTEX, bListedPrio));
ERRFALSE(offsetof(CRIT, pPrevOwned) == offsetof(MUTEX, pPrevOwned));
ERRFALSE(offsetof(CRIT, pNextOwned) == offsetof(MUTEX, pNextOwned));
ERRFALSE(offsetof(CRIT, pUpOwned) == offsetof(MUTEX, pUpOwned));
ERRFALSE(offsetof(CRIT, pDownOwned) == offsetof(MUTEX, pDownOwned));

void GCFT(LPFILETIME lpFileTime) {
	SYSTEMTIME st;
	OEMGetRealTime(&st);
	KSystemTimeToFileTime(&st,lpFileTime);
	KLocalFileTimeToFileTime(lpFileTime,lpFileTime);
}

void DoLinkCritMut(LPCRIT lpcrit, PTHREAD pth, BYTE prio) {
	LPCRIT lpcrit2, lpcrit3;
	BYTE prio2;
	prio2 = prio/PRIORITY_LEVELS_HASHSCALE;
	DEBUGCHK(prio2 < PRIORITY_LEVELS_HASHSIZE);
	DEBUGCHK((DWORD)lpcrit & 0x80000000);
	if (!pth->pOwnedList) {
		lpcrit->pPrevOwned = lpcrit->pNextOwned = 0;
		lpcrit->pUpOwned = lpcrit->pDownOwned = pth->pOwnedHash[prio2] = pth->pOwnedList = lpcrit;
	} else if (prio < pth->pOwnedList->bListedPrio) {
		lpcrit->pPrevOwned = 0;
		lpcrit->pUpOwned = lpcrit->pDownOwned = pth->pOwnedHash[prio2] = pth->pOwnedList->pPrevOwned = lpcrit;
		lpcrit->pNextOwned = pth->pOwnedList;
		pth->pOwnedList = lpcrit;
	} else if (lpcrit2 = pth->pOwnedHash[prio2]) {
		if (prio < lpcrit2->bListedPrio) {
			lpcrit->pPrevOwned = lpcrit2->pPrevOwned;
			lpcrit->pNextOwned = lpcrit2;
			lpcrit->pUpOwned = lpcrit->pDownOwned = pth->pOwnedHash[prio2] = lpcrit->pPrevOwned->pNextOwned = lpcrit2->pPrevOwned = lpcrit;
		} else {
FinishLinkCrit:
			// bounded by MAX_PRIORITY_HASHSCALE
			while ((lpcrit3 = lpcrit2->pNextOwned) && (prio > lpcrit3->bListedPrio))
				lpcrit2 = lpcrit3;
			if (prio == lpcrit2->bListedPrio) {
				lpcrit->pUpOwned = lpcrit2->pUpOwned;
				lpcrit->pUpOwned->pDownOwned = lpcrit2->pUpOwned = lpcrit;
				lpcrit->pDownOwned = lpcrit2;
				lpcrit->pPrevOwned = lpcrit->pNextOwned = 0;
			} else if (!lpcrit3) {
				lpcrit->pNextOwned = 0;
				lpcrit->pPrevOwned = lpcrit2;
				lpcrit->pUpOwned = lpcrit->pDownOwned = lpcrit2->pNextOwned = lpcrit;
			} else {
				 if (prio == lpcrit3->bListedPrio) {
					lpcrit->pUpOwned = lpcrit3->pUpOwned;
					lpcrit->pUpOwned->pDownOwned = lpcrit3->pUpOwned = lpcrit;
					lpcrit->pDownOwned = lpcrit3;
					lpcrit->pPrevOwned = lpcrit->pNextOwned = 0;
				 } else {
					lpcrit->pUpOwned = lpcrit->pDownOwned = lpcrit2->pNextOwned = lpcrit3->pPrevOwned = lpcrit;
					lpcrit->pPrevOwned = lpcrit2;
					lpcrit->pNextOwned = lpcrit3;
				 }
			}
		}
	} else {
		pth->pOwnedHash[prio2] = lpcrit;
		// bounded by PRIORITY_LEVELS_HASHSIZE
		while (!(lpcrit2 = pth->pOwnedHash[--prio2]))
			;
		goto FinishLinkCrit;
	}
	lpcrit->bListed = 1;
}

void LinkCritMut(LPCRIT lpcrit, PTHREAD pth, BOOL bIsCrit) { 
	BYTE prio;
	DEBUGCHK((DWORD)lpcrit & 0x80000000);
	DEBUGCHK(lpcrit->bListed != 1);
	if (!bIsCrit || (lpcrit->lpcs->needtrap && !GET_BURIED(pth))) {
		prio = lpcrit->bListedPrio = (lpcrit->pProxList ? lpcrit->pProxList->prio : MAX_PRIORITY_LEVELS-1);
		DoLinkCritMut(lpcrit,pth,prio);
	}
}

void LaterLinkCritMut(LPCRIT lpcrit, BOOL bIsCrit) { 
	BYTE prio;
	KCALLPROFON(50);
	if (!lpcrit->bListed && (!bIsCrit || (lpcrit->lpcs->needtrap && !GET_BURIED(pCurThread)))) {
		prio = lpcrit->bListedPrio = (lpcrit->pProxList ? lpcrit->pProxList->prio : MAX_PRIORITY_LEVELS-1);
		DoLinkCritMut(lpcrit,pCurThread,prio);
	}
	KCALLPROFOFF(50);
}

void LaterLinkMutOwner(LPMUTEX lpm) { 
	BYTE prio;
	KCALLPROFON(55);
	if (!lpm->bListed) {
		prio = lpm->bListedPrio = (lpm->pProxList ? lpm->pProxList->prio : MAX_PRIORITY_LEVELS-1);
		DoLinkCritMut((LPCRIT)lpm,lpm->pOwner,prio);
	}
	KCALLPROFOFF(55);
}

void UnlinkCritMut(LPCRIT lpcrit, PTHREAD pth) { 
	LPCRIT pDown, pNext;
	WORD prio;
	if (lpcrit->bListed == 1) {
		prio = lpcrit->bListedPrio/PRIORITY_LEVELS_HASHSCALE;
		DEBUGCHK(prio < PRIORITY_LEVELS_HASHSIZE);
		pDown = lpcrit->pDownOwned;
		pNext = lpcrit->pNextOwned;
		if (pth->pOwnedHash[prio] == lpcrit) {
			pth->pOwnedHash[prio] = ((pDown != lpcrit) ? pDown :
				(pNext && (pNext->bListedPrio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
		}
		if (pDown == lpcrit) {
			if (!lpcrit->pPrevOwned) {
				DEBUGCHK(pth->pOwnedList == lpcrit);
				if (pth->pOwnedList = pNext)
					pNext->pPrevOwned = 0;
			} else {
				if (lpcrit->pPrevOwned->pNextOwned = pNext)
					pNext->pPrevOwned = lpcrit->pPrevOwned;
			}
		} else {
			pDown->pUpOwned = lpcrit->pUpOwned;
			lpcrit->pUpOwned->pDownOwned = pDown;
			if (lpcrit->pPrevOwned) {
				lpcrit->pPrevOwned->pNextOwned = pDown;
				pDown->pPrevOwned = lpcrit->pPrevOwned;
				goto FinishDequeue;
			} else if (lpcrit == pth->pOwnedList) {
				pth->pOwnedList = pDown;
				DEBUGCHK(!pDown->pPrevOwned);
	FinishDequeue:			
				if (pNext) {
					pNext->pPrevOwned = pDown;
					pDown->pNextOwned = pNext;
				}
			}
		}
		lpcrit->bListed = 0;
	}
}

void PreUnlinkCritMut(LPCRIT lpcrit) {
	KCALLPROFON(51);
	UnlinkCritMut(lpcrit,pCurThread);
	SET_NOPRIOCALC(pCurThread);
	KCALLPROFOFF(51);
}

void PostUnlinkCritMut(void) {
	WORD prio, prio2;
	KCALLPROFON(52);
	CLEAR_NOPRIOCALC(pCurThread);
	prio = GET_BPRIO(pCurThread);
	if (pCurThread->pOwnedList && (prio > (prio2 = pCurThread->pOwnedList->bListedPrio)))
		prio = prio2;
	if (prio != GET_CPRIO(pCurThread)) {
		SET_CPRIO(pCurThread,prio);
		CELOG_SystemInvert(pCurThread->hTh, prio);
      if (RunList.pRunnable && (prio > GET_CPRIO(RunList.pRunnable)))
			SetReschedule();
	}
	KCALLPROFOFF(52);
}

void ReprioCritMut(LPCRIT lpcrit, PTHREAD pth) {
	BYTE prio;
	if (lpcrit->bListed == 1) {
		UnlinkCritMut(lpcrit,pth);
		prio = lpcrit->bListedPrio = (lpcrit->pProxList ? lpcrit->pProxList->prio : MAX_PRIORITY_LEVELS-1);
		DoLinkCritMut(lpcrit,pth,prio);
	}
}

void RunqDequeue(PTHREAD pth, DWORD prio) {
	PTHREAD pDown, pNext;
	prio = prio/PRIORITY_LEVELS_HASHSCALE;
	pDown = pth->pDownRun;
	pNext = pth->pNextSleepRun;
	if (RunList.pHashThread[prio] == pth) {
		RunList.pHashThread[prio] = ((pDown != pth) ? pDown :
			(pNext && (GET_CPRIO(pNext)/PRIORITY_LEVELS_HASHSCALE == (WORD)prio)) ? pNext : 0);
	}
	if (pDown == pth) {
		if (!pth->pPrevSleepRun) {
			DEBUGCHK(RunList.pRunnable == pth);
			if (RunList.pRunnable = pNext)
				pNext->pPrevSleepRun = 0;
		} else {
			if (pth->pPrevSleepRun->pNextSleepRun = pNext)
				pNext->pPrevSleepRun = pth->pPrevSleepRun;
		}
	} else {
		pDown->pUpRun = pth->pUpRun;
		pth->pUpRun->pDownRun = pDown;
		if (pth->pPrevSleepRun) {
			pth->pPrevSleepRun->pNextSleepRun = pDown;
			pDown->pPrevSleepRun = pth->pPrevSleepRun;
			goto FinishDequeue;
		} else if (pth == RunList.pRunnable) {
			RunList.pRunnable = pDown;
			DEBUGCHK(!pDown->pPrevSleepRun);
FinishDequeue:			
			if (pNext) {
				pNext->pPrevSleepRun = pDown;
				pDown->pNextSleepRun = pNext;
			}
		}
	}
}

VOID MakeRun(PTHREAD pth) {
	DWORD prio, prio2;
	PTHREAD pth2, pth3;
	if (!pth->bSuspendCnt) {
		SET_RUNSTATE(pth,RUNSTATE_RUNNABLE);
		prio = GET_CPRIO(pth);
		prio2 = prio/PRIORITY_LEVELS_HASHSCALE;
		if (!RunList.pRunnable) {
			pth->pPrevSleepRun = pth->pNextSleepRun = 0;
			pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = RunList.pRunnable = pth;
		} else if (prio < GET_CPRIO(RunList.pRunnable)) {
			pth->pPrevSleepRun = 0;
			pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = RunList.pRunnable->pPrevSleepRun = pth;
			pth->pNextSleepRun = RunList.pRunnable;
			RunList.pRunnable = pth;
		} else if (pth2 = RunList.pHashThread[prio2]) {
			if (prio < GET_CPRIO(pth2)) {
				pth->pPrevSleepRun = pth2->pPrevSleepRun;
				pth->pNextSleepRun = pth2;
				pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = pth->pPrevSleepRun->pNextSleepRun = pth2->pPrevSleepRun = pth;
			} else {
FinishMakeRun:
				// bounded by MAX_PRIORITY_HASHSCALE
				while ((pth3 = pth2->pNextSleepRun) && (prio > GET_CPRIO(pth3)))
					pth2 = pth3;
				if (prio == GET_CPRIO(pth2)) {
					pth->pUpRun = pth2->pUpRun;
					pth->pUpRun->pDownRun = pth2->pUpRun = pth;
					pth->pDownRun = pth2;
					pth->pPrevSleepRun = pth->pNextSleepRun = 0;
				} else if (!pth3) {
					pth->pNextSleepRun = 0;
					pth->pPrevSleepRun = pth2;
					pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth;
				} else {
					 if (prio == GET_CPRIO(pth3)) {
						pth->pUpRun = pth3->pUpRun;
						pth->pUpRun->pDownRun = pth3->pUpRun = pth;
						pth->pDownRun = pth3;
						pth->pPrevSleepRun = pth->pNextSleepRun = 0;
					 } else {
						pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth3->pPrevSleepRun = pth;
						pth->pPrevSleepRun = pth2;
						pth->pNextSleepRun = pth3;
					 }
				}
			}
		} else {
			RunList.pHashThread[prio2] = pth;
			// bounded by PRIORITY_LEVELS_HASHSIZE
			while (!(pth2 = RunList.pHashThread[--prio2]))
				;
			goto FinishMakeRun;
		}
		if (!RunList.pth || (prio < GET_CPRIO(RunList.pth)))
			SetReschedule();
	} else {
		DEBUGCHK(!((pth->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
		SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
	}
}

void SleepqDequeue(PTHREAD pth) {
	PTHREAD pth2;
	DEBUGCHK(GET_SLEEPING(pth));
	pth->wCount++;
	if (SleepList == pth) {
		DEBUGCHK(!pth->pPrevSleepRun);
		if (SleepList = pth->pNextSleepRun) {
			SleepList->pPrevSleepRun = 0;
			if (!(SleepList->dwSleepCnt += pth->dwSleepCnt)) {
				++SleepList->dwSleepCnt;
				++ticksleft;
			}
			dwSleepMin = SleepList->dwSleepCnt;
		} else
			dwSleepMin = 0;
		dwPartialDiffMSec = 0;
	} else {
		DEBUGCHK(pth->pPrevSleepRun);
		pth2 = pth->pNextSleepRun;
		if (pth->pPrevSleepRun->pNextSleepRun = pth2) {
			pth2->pPrevSleepRun = pth->pPrevSleepRun;
			pth2->dwSleepCnt += pth->dwSleepCnt;
		}
	}
	CLEAR_SLEEPING(pth);
}

VOID MakeRunIfNeeded(HANDLE hth) {
	PTHREAD pth;
	KCALLPROFON(39);
	if ((pth = HandleToThread(hth)) && (GET_RUNSTATE(pth) == RUNSTATE_NEEDSRUN)) {
		if (GET_SLEEPING(pth))
			SleepqDequeue(pth);
		MakeRun(pth);
	}
	KCALLPROFOFF(39);
}

VOID DequeueFlatProxy(LPPROXY pprox) {
	LPPROXY pDown;
	LPEVENT pObject;
	KCALLPROFON(54);
	DEBUGCHK((pprox->bType == SH_CURPROC) || (pprox->bType == SH_CURTHREAD) || (pprox->bType == HT_MANUALEVENT));
	if (pDown = pprox->pQDown) { // not already dequeued
		if (pprox->pQPrev) { // we're first
			pObject = ((LPEVENT)pprox->pQPrev);
			DEBUGCHK(pObject->pProxList == pprox);
			if (pDown == pprox) { // we're alone
				pObject->pProxList = 0;
			} else {
				pDown->pQUp = pprox->pQUp;
				pprox->pQUp->pQDown = pObject->pProxList = pDown;
				pDown->pQPrev = (LPPROXY)pObject;
			}
		} else {
			pDown->pQUp = pprox->pQUp;
			pprox->pQUp->pQDown = pDown;
		}
		pprox->pQDown = 0;
	}
	KCALLPROFOFF(54);
}

BOOL DequeuePrioProxy(LPPROXY pprox) {
	LPCRIT lpcrit;
	LPPROXY pDown, pNext;
	WORD prio;
	BOOL bRet;
	KCALLPROFON(31);
	DEBUGCHK((pprox->bType == HT_EVENT) || (pprox->bType == HT_CRITSEC) || (pprox->bType == HT_MUTEX) || (pprox->bType == HT_SEMAPHORE));
	bRet = FALSE;
	if (pDown = pprox->pQDown) { // not already dequeued
		lpcrit = (LPCRIT)pprox->pObject;
		prio = pprox->prio/PRIORITY_LEVELS_HASHSCALE;
		pDown = pprox->pQDown;
		pNext = pprox->pQNext;
		if (lpcrit->pProxHash[prio] == pprox) {
			lpcrit->pProxHash[prio] = ((pDown != pprox) ? pDown :
				(pNext && (pNext->prio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
		}
		if (pDown == pprox) {
			if (!pprox->pQPrev) {
				DEBUGCHK(lpcrit->pProxList == pprox);
				if (lpcrit->pProxList = pNext)
					pNext->pQPrev = 0;
				bRet = TRUE;
			} else {
				if (pprox->pQPrev->pQNext = pNext)
					pNext->pQPrev = pprox->pQPrev;
			}
		} else {
			pDown->pQUp = pprox->pQUp;
			pprox->pQUp->pQDown = pDown;
			if (pprox->pQPrev) {
				pprox->pQPrev->pQNext = pDown;
				pDown->pQPrev = pprox->pQPrev;
				goto FinishDequeue;
			} else if (pprox == lpcrit->pProxList) {
				lpcrit->pProxList = pDown;
				DEBUGCHK(!pDown->pQPrev);
FinishDequeue:			
				if (pNext) {
					pNext->pQPrev = pDown;
					pDown->pQNext = pNext;
				}
			}
		}
		pprox->pQDown = 0;
	}
	KCALLPROFOFF(31);
	return bRet;
}

void DoReprioCrit(LPCRIT lpcrit) {
	HANDLE hth;
	PTHREAD pth;
	KCALLPROFON(4);
	if ((hth = lpcrit->lpcs->OwnerThread) && (pth = HandleToThread((HANDLE)((DWORD)hth & ~1))))
		ReprioCritMut(lpcrit,pth);
	KCALLPROFOFF(4);
}

void DoReprioMutex(LPMUTEX lpm) {
	KCALLPROFON(29);
	if (lpm->pOwner)
		ReprioCritMut((LPCRIT)lpm,lpm->pOwner);
	KCALLPROFOFF(29);
}

void DequeueProxy(LPPROXY pProx) {
	switch (pProx->bType) {
		case SH_CURPROC:
		case SH_CURTHREAD:
		case HT_MANUALEVENT:
			KCall((PKFN)DequeueFlatProxy,pProx);
			break;
		case HT_MUTEX:
			if (KCall((PKFN)DequeuePrioProxy,pProx))
				KCall((PKFN)DoReprioMutex,pProx->pObject);
			break;
		case HT_CRITSEC:
			if (KCall((PKFN)DequeuePrioProxy,pProx))
				KCall((PKFN)DoReprioCrit,pProx->pObject);
			break;
		case HT_EVENT:
		case HT_SEMAPHORE:
			KCall((PKFN)DequeuePrioProxy,pProx);
			break;
		default:
			DEBUGCHK(0);
	}
}

void BoostCPrio(PTHREAD pth, DWORD prio) {
	DWORD oldcprio;
	oldcprio = GET_CPRIO(pth);
	SET_CPRIO(pth,prio);
	if (GET_RUNSTATE(pth) == RUNSTATE_RUNNABLE) {
		RunqDequeue(pth,oldcprio);
		MakeRun(pth);
	}
}

void PostBoostMut(LPMUTEX lpm) {
	PTHREAD pth;
	WORD prio;
	KCALLPROFON(56);
	if (pCurThread->bWaitState == WAITSTATE_PROCESSING) {
		pth = lpm->pOwner;
		DEBUGCHK(pth);
		prio = GET_CPRIO(pCurThread);
		if (prio < GET_CPRIO(pth)) {
			BoostCPrio(pth,prio);
			CELOG_SystemInvert(pth->hTh, prio);
		}
		if (!GET_NOPRIOCALC(pth))
			LaterLinkMutOwner(lpm);
		ReprioCritMut((LPCRIT)lpm,pth);
	}
	KCALLPROFOFF(56);
}

void PostBoostCrit1(LPCRIT pcrit) {
	PTHREAD pth;
	KCALLPROFON(57);
	if (pCurThread->bWaitState == WAITSTATE_PROCESSING) {
		pth = HandleToThread((HANDLE)((DWORD)pcrit->lpcs->OwnerThread & ~1));
		DEBUGCHK(pth);
		ReprioCritMut(pcrit,pth);
	}
	KCALLPROFOFF(57);
}

void PostBoostCrit2(LPCRIT pcrit) {
	PTHREAD pth;
	BYTE prio;
	KCALLPROFON(60);
	if (pCurThread->bWaitState == WAITSTATE_PROCESSING) {
		pth = HandleToThread((HANDLE)((DWORD)pcrit->lpcs->OwnerThread & ~1));
		DEBUGCHK(pth);
		if (pcrit->pProxList && ((prio = pcrit->pProxList->prio) < GET_CPRIO(pth))) {
			BoostCPrio(pth,prio);
         CELOG_SystemInvert(pth->hTh, prio);
      }
	}
	KCALLPROFOFF(60);
}

void CritFinalBoost(LPCRITICAL_SECTION lpcs) {
	LPCRIT pcrit;
	DWORD prio;
	KCALLPROFON(59);
	DEBUGCHK(lpcs->OwnerThread == hCurThread);
	pcrit = (LPCRIT)lpcs->hCrit;
	if (!pcrit->bListed && pcrit->pProxList)
		LinkCritMut(pcrit,pCurThread,1);
	if (pcrit->pProxList && ((prio = pcrit->pProxList->prio) < GET_CPRIO(pCurThread))) {
		SET_CPRIO(pCurThread,prio);
      CELOG_SystemInvert(pCurThread->hTh, prio);
   }
	KCALLPROFOFF(59);
}

VOID AddToProcRunnable(PPROCESS pproc, PTHREAD pth) {
	KCALLPROFON(24);
	pth->pNextInProc = pproc->pTh;
	pth->pPrevInProc = 0;
	DEBUGCHK(!pproc->pTh->pPrevInProc);
	pproc->pTh->pPrevInProc = pth;
	pproc->pTh = pth;
	MakeRun(pth);
	KCALLPROFOFF(24);
}

HANDLE WakeOneThreadInterruptDelayed(LPEVENT lpe) {
	PTHREAD pth;
	LPPROXY pprox;
	HANDLE hRet;
	pprox = lpe->pProxList;
	DEBUGCHK(pprox->pObject == (LPBYTE)lpe);
	DEBUGCHK(pprox->bType == HT_MANUALEVENT);
	DEBUGCHK(pprox->pQDown == pprox);
	DEBUGCHK(pprox->pQPrev == (LPPROXY)lpe);
	DEBUGCHK(pprox->dwRetVal == WAIT_OBJECT_0);
	lpe->pProxList = 0;
	pth = pprox->pTh;
	DEBUGCHK(!pth->lpce);
	pth->wCount++;
	DEBUGCHK(pth->lpProxy == pprox);
	pth->lpProxy = 0;
	if (pth->bWaitState == WAITSTATE_BLOCKED) {
		DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
		DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
		DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
		pth->retValue = WAIT_OBJECT_0;
		hRet = pth->hTh;
		SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
	} else {
		DEBUGCHK(!GET_SLEEPING(pth));
		DEBUGCHK(pth->bWaitState == WAITSTATE_PROCESSING);
		pth->dwPendReturn = WAIT_OBJECT_0;
		pth->bWaitState = WAITSTATE_SIGNALLED;
		hRet = 0;
	}
	return hRet;
}

void WakeOneThreadFlat(LPEVENT pObject, HANDLE *phTh) {
	PTHREAD pth;
	LPPROXY pprox, pDown;
	KCALLPROFON(41);
	if (pprox = pObject->pProxList) {
	    DEBUGCHK((pprox->bType == SH_CURPROC) || (pprox->bType == SH_CURTHREAD) || (pprox->bType == HT_MANUALEVENT));
		DEBUGCHK(pprox->pQPrev = (LPPROXY)pObject);
	    pDown = pprox->pQDown;
	    DEBUGCHK(pDown);
	    if (pDown == pprox) { // we're alone
	    	pObject->pProxList = 0;
	    } else {
			pDown->pQUp = pprox->pQUp;
			pprox->pQUp->pQDown = pObject->pProxList = pDown;
			pDown->pQPrev = (LPPROXY)pObject;
	    }
		pprox->pQDown = 0;
		pth = pprox->pTh;
		DEBUGCHK(pth);
		if (pth->wCount == pprox->wCount) {
			DEBUGCHK(pth->lpce);
			pth->lpce->base = pth->lpProxy;
			pth->lpce->size = (DWORD)pth->lpPendProxy;
			pth->lpProxy = 0;
			pth->wCount++;
			if (pth->bWaitState == WAITSTATE_BLOCKED) {
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
				pth->retValue = pprox->dwRetVal;
				SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
				*phTh = pth->hTh;
			} else {
				DEBUGCHK(!GET_SLEEPING(pth));
				DEBUGCHK(pth->bWaitState == WAITSTATE_PROCESSING);
				pth->dwPendReturn = pprox->dwRetVal;
				pth->bWaitState = WAITSTATE_SIGNALLED;
			}
		}
	}
	KCALLPROFOFF(41);
}

DWORD EventModMan(LPEVENT lpe, LPSTUBEVENT lpse, DWORD action) {
	DWORD prio;
	KCALLPROFON(15);
	prio = lpe->bMaxPrio;
	lpe->bMaxPrio = THREAD_RT_PRIORITY_IDLE;
	if (lpse->pProxList = lpe->pProxList) {
		lpse->pProxList->pQPrev = (LPPROXY)lpse;
		DEBUGCHK(lpse->pProxList->pQDown);
		lpe->pProxList = 0;
	}
	lpe->state = (action == EVENT_SET);
	KCALLPROFOFF(15);
	return prio;
}

BOOL EventModAuto(LPEVENT lpe, DWORD action, HANDLE *phTh) {
	BOOL bRet;
	PTHREAD pth;
	LPPROXY pprox, pDown, pNext;
	BYTE prio;
	KCALLPROFON(16);
    bRet = 0;
	if (!(pprox = lpe->pProxList))
		lpe->state = (action == EVENT_SET);
	else {
		pDown = pprox->pQDown;
		if (lpe->onequeue) {
			DEBUGCHK(pprox->pQPrev = (LPPROXY)lpe);
			if (pDown == pprox) { // we're alone
				lpe->pProxList = 0;
			} else {
				pDown->pQUp = pprox->pQUp;
				pprox->pQUp->pQDown = lpe->pProxList = pDown;
				pDown->pQPrev = (LPPROXY)lpe;
			}
		} else {
			prio = pprox->prio/PRIORITY_LEVELS_HASHSCALE;
			pNext = pprox->pQNext;
			if (lpe->pProxHash[prio] == pprox) {
				lpe->pProxHash[prio] = ((pDown != pprox) ? pDown :
					(pNext && (pNext->prio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
			}
			if (pDown == pprox) {
				if (lpe->pProxList = pNext)
					pNext->pQPrev = 0;
			} else {
				pDown->pQUp = pprox->pQUp;
				pprox->pQUp->pQDown = pDown;
				lpe->pProxList = pDown;
				DEBUGCHK(!pDown->pQPrev);
				if (pNext) {
					pNext->pQPrev = pDown;
					pDown->pQNext = pNext;
				}
			}
		}
		pprox->pQDown = 0;
		pth = pprox->pTh;
		DEBUGCHK(pth);
		if (pth->wCount != pprox->wCount)
			bRet = 1;
		else {
			DEBUGCHK(pth->lpce);
			pth->lpce->base = pth->lpProxy;
			pth->lpce->size = (DWORD)pth->lpPendProxy;
			pth->lpProxy = 0;
			pth->wCount++;
			if (pth->bWaitState == WAITSTATE_BLOCKED) {
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
				pth->retValue = pprox->dwRetVal;
				SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
				*phTh = pth->hTh;
			} else {
				DEBUGCHK(!GET_SLEEPING(pth));
				DEBUGCHK(pth->bWaitState == WAITSTATE_PROCESSING);
				pth->dwPendReturn = pprox->dwRetVal;
				pth->bWaitState = WAITSTATE_SIGNALLED;
			}
			lpe->state = 0;
		}
	}
	KCALLPROFOFF(16);
	return bRet;
}

void DoFreeMutex(LPMUTEX lpm) {
	KCALLPROFON(34);
	UnlinkCritMut((LPCRIT)lpm,lpm->pOwner);
	KCALLPROFOFF(34);
}
void DoFreeCrit(LPCRIT lpcrit) {
	PTHREAD pth;
	KCALLPROFON(45);
	if (lpcrit->bListed == 1) {
		pth = HandleToThread(lpcrit->lpcs->OwnerThread);
		DEBUGCHK(pth);
		UnlinkCritMut(lpcrit,pth);
	}
	KCALLPROFOFF(45);
}

/* When a thread tries to closehandle an event */

BOOL SC_EventCloseHandle(HANDLE hEvent) {
	HANDLE hTrav;
	LPEVENT lpe, lpe2;
	DEBUGMSG(ZONE_ENTRY,(L"SC_EventCloseHandle entry: %8.8lx\r\n",hEvent));
   CELOG_EventCloseHandle(hEvent);
	if (DecRef(hEvent,pCurProc,FALSE)) {
      CELOG_EventDelete(hEvent);
		EnterCriticalSection(&NameCS);
		lpe = HandleToEvent(hEvent);
		DEBUGCHK(lpe);
		if (hEvent == hEventList)
			hEventList = lpe->hNext;
		else {
			hTrav = hEventList;
			lpe2 = HandleToEvent(hTrav);
			DEBUGCHK(lpe2);
			while (lpe2->hNext != hEvent) {
				hTrav = lpe2->hNext;
				lpe2 = HandleToEvent(hTrav);
				DEBUGCHK(lpe2);
			}
			lpe2->hNext = lpe->hNext;
		}
		LeaveCriticalSection(&NameCS);
		if (lpe->onequeue) {
			while (lpe->pProxList)
				KCall((PKFN)DequeueFlatProxy,lpe->pProxList);
		} else {
			while (lpe->pProxList)
				KCall((PKFN)DequeuePrioProxy,lpe->pProxList);
		}
		if (lpe->name)
			FreeName(lpe->name);
		if (lpe->pIntrProxy)
	    	FreeIntrFromEvent(lpe);
		FreeMem((LPVOID)lpe,HEAP_EVENT);
		FreeHandle(hEvent);
	}
   DEBUGMSG(ZONE_ENTRY,(L"SC_EventCloseHandle exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

/* When a thread tries to closehandle a semaphore */

BOOL SC_SemCloseHandle(HANDLE hSem) {
	HANDLE hTrav;
	LPSEMAPHORE lpsem, lpsem2;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SemCloseHandle entry: %8.8lx\r\n",hSem));
   CELOG_SemaphoreCloseHandle(hSem);
	if (DecRef(hSem,pCurProc,FALSE)) {
      CELOG_SemaphoreDelete(hSem);
		EnterCriticalSection(&NameCS);
		lpsem = HandleToSem(hSem);
		DEBUGCHK(lpsem);
		if (hSem == hSemList)
			hSemList = lpsem->hNext;
		else {
			hTrav = hSemList;
			lpsem2 = HandleToSem(hTrav);
			DEBUGCHK(lpsem2);
			while (lpsem2->hNext != hSem) {
				hTrav = lpsem2->hNext;
				lpsem2 = HandleToSem(hTrav);
				DEBUGCHK(lpsem2);
			}
			lpsem2->hNext = lpsem->hNext;
		}
		LeaveCriticalSection(&NameCS);
		while (lpsem->pProxList)
			KCall((PKFN)DequeuePrioProxy,lpsem->pProxList);
		if (lpsem->name)
			FreeName(lpsem->name);
		FreeMem((LPVOID)lpsem,HEAP_SEMAPHORE);
		FreeHandle(hSem);
	}
   DEBUGMSG(ZONE_ENTRY,(L"SC_SemCloseHandle exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

/* When a thread tries to closehandle a mutex */

BOOL SC_MutexCloseHandle(HANDLE hMutex) {
	HANDLE hTrav;
	LPMUTEX lpmutex, lpmutex2;
	DEBUGMSG(ZONE_ENTRY,(L"SC_MutexCloseHandle entry: %8.8lx\r\n",hMutex));
   CELOG_MutexCloseHandle(hMutex);
	EnterCriticalSection(&NameCS);
	if (DecRef(hMutex,pCurProc,FALSE)) {
      CELOG_MutexDelete(hMutex);
		lpmutex = HandleToMutex(hMutex);
		DEBUGCHK(lpmutex);
		if (hMutex == hMutexList)
			hMutexList = lpmutex->hNext;
		else {
			hTrav = hMutexList;
			lpmutex2 = HandleToMutex(hTrav);
			DEBUGCHK(lpmutex2);
			while (lpmutex2->hNext != hMutex) {
				hTrav = lpmutex2->hNext;
				lpmutex2 = HandleToMutex(hTrav);
				DEBUGCHK(lpmutex2);
			}
			lpmutex2->hNext = lpmutex->hNext;
		}
		while (lpmutex->pProxList)
			if (KCall((PKFN)DequeuePrioProxy,lpmutex->pProxList))
				KCall((PKFN)DoReprioMutex,lpmutex);
		KCall((PKFN)DoFreeMutex,lpmutex);
		if (lpmutex->name)
			FreeName(lpmutex->name);
		FreeMem((LPVOID)lpmutex,HEAP_MUTEX);
		FreeHandle(hMutex);
	}
	LeaveCriticalSection(&NameCS);
	DEBUGMSG(ZONE_ENTRY,(L"SC_MutexCloseHandle exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

BOOL SC_EventAddAccess(HANDLE hEvent) {
	BOOL retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_EventAddAccess entry: %8.8lx\r\n",hEvent));
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_EventAddAccess failed due to insufficient trust\r\n"));
		retval = FALSE;
	} else
		retval = IncRef(hEvent,pCurProc);
	DEBUGMSG(ZONE_ENTRY,(L"SC_EventAddAccess exit: %d\r\n",retval));
	return retval;
}

HANDLE EventModIntr(LPEVENT lpe, DWORD type) {
	HANDLE hRet;
	KCALLPROFON(42);
	if (!lpe->pProxList) {
		lpe->state = (type == EVENT_SET);
		hRet = 0;
	} else {
		lpe->state = 0;
		hRet = WakeOneThreadInterruptDelayed(lpe);
	}
	DEBUGCHK(!lpe->manualreset);
	KCALLPROFOFF(42);
	return hRet;
}

void AdjustPrioDown() {
    DWORD dwPrio, dwPrio2;
    KCALLPROFON(66);
    dwPrio = GET_BPRIO(pCurThread);
    if (pCurThread->pOwnedList && ((dwPrio2 = pCurThread->pOwnedList->bListedPrio) < dwPrio))
        dwPrio = dwPrio2;
    SET_CPRIO(pCurThread,dwPrio);
    if (RunList.pRunnable && (dwPrio > GET_CPRIO(RunList.pRunnable)))
        SetReschedule();
    KCALLPROFOFF(66);
}

/* When a thread tries to set/reset/pulse an event */

BOOL SC_EventModify(HANDLE hEvent, DWORD type) {
	LPEVENT lpe;
	HANDLE hWake;
	LPSTUBEVENT lpse;
	DEBUGMSG(ZONE_ENTRY,(L"SC_EventModify entry: %8.8lx %8.8lx\r\n",hEvent,type));
	if (!(lpe = (bAllKMode ? HandleToEventPerm(hEvent) : HandleToEvent(hEvent)))) {
        KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
		return FALSE;
	}
	
   CELOG_Event(hEvent, type);

   switch (type) {
		case EVENT_PULSE:
		case EVENT_SET:
			
         if (lpe->pIntrProxy) {
				DEBUGCHK(lpe->onequeue);
				if (hWake = (HANDLE)KCall((PKFN)EventModIntr,lpe,type))
					KCall((PKFN)MakeRunIfNeeded,hWake);
			} else if (lpe->manualreset) {
				DWORD dwOldPrio, dwNewPrio;
				DEBUGCHK(lpe->onequeue);
				// *lpse can't be stack-based since other threads won't have access and might dequeue/requeue
				if (!(lpse = AllocMem(HEAP_STUBEVENT))) {
					DEBUGMSG(ZONE_ENTRY,(L"SC_EventModify exit: %8.8lx\r\n",FALSE));
					KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
					return FALSE;
				}
				dwOldPrio = GET_BPRIO(pCurThread);
				dwNewPrio = KCall((PKFN)EventModMan,lpe,lpse,type);
				if (lpse->pProxList) {
					SET_NOPRIOCALC(pCurThread);
					if (dwNewPrio < dwOldPrio)
						SET_CPRIO(pCurThread,dwNewPrio);
					while (lpse->pProxList) {
						hWake = 0;
						KCall((PKFN)WakeOneThreadFlat,lpse,&hWake);
						if (hWake)
							KCall((PKFN)MakeRunIfNeeded,hWake);
					}
					CLEAR_NOPRIOCALC(pCurThread);
					if (dwNewPrio < dwOldPrio)
						KCall((PKFN)AdjustPrioDown);
				}
				FreeMem(lpse,HEAP_STUBEVENT);
			} else {
				hWake = 0;
				while (KCall((PKFN)EventModAuto,lpe,type,&hWake))
					;
				if (hWake)
					KCall((PKFN)MakeRunIfNeeded,hWake);
			}
			break;
		case EVENT_RESET:
         lpe->state = 0;
			break;
		default:
			DEBUGCHK(0);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_EventModify exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

BOOL IsValidIntrEvent(HANDLE hEvent) {
	EVENT *lpe;
	if (!(lpe = HandleToEvent(hEvent)) || lpe->manualreset || lpe->pProxList)
		return FALSE;
	return TRUE;
}

/* When a thread tries to create an event */

HANDLE SC_CreateEvent(LPSECURITY_ATTRIBUTES lpsa, BOOL fManReset, BOOL fInitState, LPCWSTR lpEventName) {
	HANDLE hEvent;
	LPEVENT lpe;
	int len;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateEvent entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
		lpsa,fManReset,fInitState,lpEventName));
	if (lpEventName) {
		len = strlenW(lpEventName) + 1;
		if (len > MAX_PATH) {
			KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
			DEBUGMSG(ZONE_ENTRY,(L"SC_CreateEvent exit: %8.8lx\r\n",0));
			return 0;
		}
		len *= sizeof(WCHAR);
		LockPages(lpEventName,len,0,LOCKFLAG_READ);
	}
	EnterCriticalSection(&NameCS);
	if (lpEventName) {
		for (hEvent = hEventList; hEvent; hEvent = lpe->hNext) {
			lpe = HandleToEvent(hEvent);
			DEBUGCHK(lpe);
			if (lpe->name && !strcmpW(lpe->name->name,lpEventName)) {
				IncRef(hEvent,pCurProc);
				KSetLastError(pCurThread,ERROR_ALREADY_EXISTS);
				goto exit;
			}
		}
		KSetLastError(pCurThread,0);
	}
	if (!(lpe = (LPEVENT)AllocMem(HEAP_EVENT))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		hEvent = 0;
		goto exit;
	}
	if (!(hEvent = AllocHandle(&cinfEvent,lpe,pCurProc))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		FreeMem(lpe,HEAP_EVENT);
		hEvent = 0;
		goto exit;
	}
	if (lpEventName) {
		if (!(lpe->name = (Name *)AllocName(len))) {
			KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
			FreeHandle(hEvent);
			FreeMem(lpe,HEAP_EVENT);
			hEvent = 0;
			goto exit;
		}
		memcpy(lpe->name->name,lpEventName,len);
	} else
		lpe->name = 0;
	memset(lpe->pProxHash,0,sizeof(lpe->pProxHash));
	lpe->pProxList = 0;
	lpe->hNext = hEventList;
	hEventList = hEvent;
	lpe->state = fInitState;
	lpe->manualreset = fManReset;
	lpe->onequeue = fManReset;
	lpe->pIntrProxy = 0;
	lpe->bMaxPrio = THREAD_RT_PRIORITY_IDLE;
exit:
	CELOG_EventCreate(hEvent, fManReset, fInitState, lpEventName);
	LeaveCriticalSection(&NameCS);
	if (lpEventName)
		UnlockPages(lpEventName,len);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateEvent exit: %8.8lx\r\n",hEvent));
	return hEvent;
}

/* When a thread tries to create a semaphore */

HANDLE SC_CreateSemaphore(LPSECURITY_ATTRIBUTES lpsa, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName) {
	HANDLE hSem;
	int len;
	LPSEMAPHORE lpsem;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateSemaphore entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",lpsa,lInitialCount,lMaximumCount,lpName));
	if (lpName) {
		len = strlenW(lpName) + 1;
		if (len > MAX_PATH) {
			KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
			DEBUGMSG(ZONE_ENTRY,(L"SC_CreateSemaphore exit: %8.8lx\r\n",0));
			return 0;
		}
		len *= sizeof(WCHAR);
		LockPages(lpName,len,0,LOCKFLAG_READ);
	}
	EnterCriticalSection(&NameCS);
	if (lpName) {
		for (hSem = hSemList; hSem; hSem = lpsem->hNext) {
			lpsem = HandleToSem(hSem);
			DEBUGCHK(lpsem);
			if (lpsem->name && !strcmpW(lpsem->name->name,lpName)) {
				IncRef(hSem,pCurProc);
				KSetLastError(pCurThread,ERROR_ALREADY_EXISTS);
				goto exit;
			}
		}
		KSetLastError(pCurThread,0);
	}
	if ((lInitialCount < 0) || (lMaximumCount <= 0) || lpsa || (lInitialCount > lMaximumCount)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		hSem = 0;
		goto exit;
	}
	if (!(lpsem = (LPSEMAPHORE)AllocMem(HEAP_SEMAPHORE))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		hSem = 0;
		goto exit;
	}
	if (!(hSem = AllocHandle(&cinfSem,lpsem,pCurProc))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		FreeMem(lpsem,HEAP_SEMAPHORE);
		hSem = 0;
		goto exit;
	}
	if (lpName) {
		if (!(lpsem->name = (Name *)AllocName(len))) {
			KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
			FreeHandle(hSem);
			FreeMem(lpsem,HEAP_SEMAPHORE);
			hSem = 0;
			goto exit;
		}
		memcpy(lpsem->name->name,lpName,len);
	} else
		lpsem->name = 0;
	memset(lpsem->pProxHash,0,sizeof(lpsem->pProxHash));
	lpsem->pProxList = 0;
	lpsem->hNext = hSemList;
	hSemList = hSem;
	lpsem->lCount = lInitialCount;
	lpsem->lMaxCount = lMaximumCount;
	lpsem->lPending = 0;
exit:
	CELOG_SemaphoreCreate(hSem, lInitialCount, lMaximumCount, lpName);
	LeaveCriticalSection(&NameCS);
	if (lpName)
		UnlockPages(lpName,len);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateSemaphore exit: %8.8lx\r\n",hSem));
	return hSem;
}

/* When a thread tries to create a mutex */

HANDLE SC_CreateMutex(LPSECURITY_ATTRIBUTES lpsa, BOOL bInitialOwner, LPCTSTR lpName) {
	int len;
	HANDLE hMutex;
	LPMUTEX lpmutex;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateMutex entry: %8.8lx %8.8lx %8.8lx\r\n",lpsa,bInitialOwner,lpName));
	if (lpName) {
		len = strlenW(lpName) + 1;
		if (len > MAX_PATH) {
			KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
			DEBUGMSG(ZONE_ENTRY,(L"SC_CreateMutex exit: %8.8lx\r\n",0));
			return 0;
		}
		len *= sizeof(WCHAR);
		LockPages(lpName,len,0,LOCKFLAG_READ);
	}
	EnterCriticalSection(&NameCS);
	if (lpName) {
		for (hMutex = hMutexList; hMutex; hMutex = lpmutex->hNext) {
			lpmutex = HandleToMutex(hMutex);
			DEBUGCHK(lpmutex);
			if (lpmutex->name && !strcmpW(lpmutex->name->name,lpName)) {
				IncRef(hMutex,pCurProc);
				KSetLastError(pCurThread,ERROR_ALREADY_EXISTS);
				goto exit;
			}
		}
		KSetLastError(pCurThread,0);
	}
	if (!(lpmutex = (LPMUTEX)AllocMem(HEAP_MUTEX))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		hMutex = 0;
		goto exit;
	}
	if (!(hMutex = AllocHandle(&cinfMutex,lpmutex,pCurProc))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		FreeMem(lpmutex,HEAP_EVENT);
		hMutex = 0;
		goto exit;
	}
	if (lpName) {
		if (!(lpmutex->name = AllocName(len))) {
			KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
			FreeHandle(hMutex);
			FreeMem(lpmutex,HEAP_EVENT);
			hMutex = 0;
			goto exit;
		}
		memcpy(lpmutex->name->name,lpName,len);
	} else
		lpmutex->name = 0;
	memset(lpmutex->pProxHash,0,sizeof(lpmutex->pProxHash));
	lpmutex->pProxList = 0;
	lpmutex->hNext = hMutexList;
	hMutexList = hMutex;
	lpmutex->bListed = 0;
	if (bInitialOwner) {
		lpmutex->LockCount = 1;
		lpmutex->pOwner = pCurThread;
		KCall((PKFN)LaterLinkCritMut,(LPCRIT)lpmutex,0);
	} else {
		lpmutex->LockCount = 0;
		lpmutex->pOwner = 0;
	}
exit:
   CELOG_MutexCreate(hMutex, lpmutex->name);
	LeaveCriticalSection(&NameCS);
	if (lpName)
		UnlockPages(lpName,len);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateMutex exit: %8.8lx\r\n",hMutex));
	return hMutex;
}

DWORD SC_SetLowestScheduledPriority(DWORD maxprio) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetLowestScheduledPriority entry: %8.8lx\r\n",maxprio));
	if (maxprio > MAX_WIN32_PRIORITY_LEVELS - 1)
		maxprio = MAX_WIN32_PRIORITY_LEVELS - 1;
	retval = currmaxprio;
	if (retval < MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS)
		retval = MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS;
	retval -= (MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS);		
	currmaxprio = maxprio + (MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS);
	pOOMThread = pCurThread;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetLowestScheduledPriority exit: %8.8lx\r\n",retval));
	return retval;
}

void NextThread(void) {
	PTHREAD pth;
	LPEVENT lpe;
	LPPROXY pprox;
	DWORD pend, intr;
	KCALLPROFON(10);
	/* Handle interrupts */
	
dointr:
	INTERRUPTS_OFF();
	pend = PendEvents;
	PendEvents = 0;
	INTERRUPTS_ON();
	if (pend) {
		do {
			intr = GetHighPos(pend);
			pend &= ~(1<<intr);
			if (lpe = IntrEvents[intr]) {
				if (!lpe->pProxList)
					lpe->state = 1;
				else {
					DEBUGCHK(!lpe->state);
					pprox = lpe->pProxList;
					DEBUGCHK(pprox->pObject == (LPBYTE)lpe);
					DEBUGCHK(pprox->bType == HT_MANUALEVENT);
					DEBUGCHK(pprox->pQDown == pprox);
					DEBUGCHK(pprox->pQPrev == (LPPROXY)lpe);
					DEBUGCHK(pprox->dwRetVal == WAIT_OBJECT_0);
					lpe->pProxList = 0;
					pth = pprox->pTh;
					DEBUGCHK(!pth->lpce);
					pth->wCount++;
					DEBUGCHK(pth->lpProxy == pprox);
					DEBUGCHK(pth->lpProxy == lpe->pIntrProxy);
					pth->lpProxy = 0;
					if (pth->bWaitState == WAITSTATE_BLOCKED) {
						DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
						DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
						DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
						pth->retValue = WAIT_OBJECT_0;
						if (GET_SLEEPING(pth))
							SleepqDequeue(pth);
						MakeRun(pth);
					} else {
						DEBUGCHK(!GET_SLEEPING(pth));
						DEBUGCHK(pth->bWaitState == WAITSTATE_PROCESSING);
						pth->dwPendReturn = WAIT_OBJECT_0;
						pth->bWaitState = WAITSTATE_SIGNALLED;
					}
				}
			}
		} while (pend);
		if (PendEvents)
			goto dointr;
		KCALLPROFOFF(10);
		return;
	}
	INTERRUPTS_OFF();
	pend = DiffMSec;
	DiffMSec = 0;
	INTERRUPTS_ON();
	if (dwPreempt) {
		if (pend >= dwPreempt) {
			bPreempt = 1;
			dwPreempt = 0;
			SetReschedule();
		} else
			dwPreempt -= pend;
	}
	if (ticksleft += pend) {
		/* Handle timeouts */
		if (!(pth = SleepList)) {
			ticksleft = 0;
			dwPartialDiffMSec = 0;
			DEBUGCHK(!dwSleepMin);
		} else if (pth->dwSleepCnt > ticksleft) {
			pth->dwSleepCnt -= ticksleft;
			dwSleepMin = pth->dwSleepCnt;
			ticksleft = 0;
		} else {
			ticksleft -= pth->dwSleepCnt;
			if (SleepList = pth->pNextSleepRun) {
				SleepList->pPrevSleepRun = 0;
				if (!SleepList->dwSleepCnt) {
					++SleepList->dwSleepCnt;
					++ticksleft;
				}
				dwSleepMin = SleepList->dwSleepCnt;
			} else
				dwSleepMin = 0;
			dwPartialDiffMSec = 0;
			DEBUGCHK(GET_SLEEPING(pth));
			CLEAR_SLEEPING(pth);
			pth->wCount++;
			if (GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN) {
				DEBUGCHK(GET_RUNSTATE(pth) == RUNSTATE_BLOCKED);
				if (pth->lpce) { // not set if purely sleeping
					pth->lpce->base = pth->lpProxy;
					pth->lpce->size = (DWORD)pth->lpPendProxy;
					pth->lpProxy = 0;
				} else if (pth->lpProxy) {
					// must be an interrupt event - special case it!
					pprox = pth->lpProxy;
					lpe = (LPEVENT)pprox->pObject;
					DEBUGCHK(pprox->bType == HT_MANUALEVENT);
					DEBUGCHK(pprox->pQDown == pprox);
					DEBUGCHK(pprox->pQPrev == (LPPROXY)lpe);
					DEBUGCHK(pprox->dwRetVal == WAIT_OBJECT_0);
					lpe->pProxList = 0;
					pprox->pQDown = 0;
					pth->lpProxy = 0;
				}
				MakeRun(pth);
			}
		}
	}
	KCALLPROFOFF(10);
}

void GoToUserTime() {
	INTERRUPTS_OFF();
	SET_TIMEUSER(pCurThread);
	pCurThread->dwKernTime += CurMSec - CurrTime;
	CurrTime = CurMSec;
	INTERRUPTS_ON();
	randdw1 = ((randdw1<<5) | (randdw1>>27)) ^ (CurMSec & 0x1f);
}

void GoToKernTime() {
	INTERRUPTS_OFF();
	SET_TIMEKERN(pCurThread);
	pCurThread->dwUserTime += CurMSec - CurrTime;
	CurrTime = CurMSec;
	INTERRUPTS_ON();
	randdw2 = ((randdw2<<5) | (randdw2>>27)) ^ (CurMSec & 0x1f);
}

void KCNextThread(void) {
	DWORD prio, prio2, NewTime, dwTmp;
	PTHREAD pth, pth2, pth3, pOwner;
	LPCRIT pCrit;
	LPCRITICAL_SECTION lpcs;
	KCALLPROFON(44);
	
	if (!bLastIdle && !RunList.pth)
		pCurThread->dwQuantLeft = pCurThread->dwQuantum;
	if ((pth = RunList.pth) && (pth2 = RunList.pRunnable) &&
		(((prio = GET_CPRIO(pth)) > (prio2 = GET_CPRIO(pth2))) ||
		 ((prio == prio2) && bPreempt))) {
		if (prio == prio2) {
			// Current thread and next runnable thread have same priority, same run list.  Use
			// runnable thread pointer (pth2, first on list) to place current thread (pth) last on list.
			pth->dwQuantLeft = pth->dwQuantum;
			pth->pUpRun = pth2->pUpRun;
			pth->pUpRun->pDownRun = pth2->pUpRun = pth;
			pth->pDownRun = pth2;
			pth->pPrevSleepRun = pth->pNextSleepRun = 0;
		} else {
			// Find appropriate place to enqueue current thread.
			dwTmp = DiffMSec;
			pth->dwQuantLeft = (dwPreempt > dwTmp) ? (dwPreempt - dwTmp) : 0;
			prio2 = prio/PRIORITY_LEVELS_HASHSCALE;
			if (pth2 = RunList.pHashThread[prio2]) {
				// Hash table entry for current thread priority already exists
				if (prio < GET_CPRIO(pth2)) {
					// Insert new run list for current thread priority before list pointed to by
					// hash table.
					pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = pth;
					pth->pPrevSleepRun = pth2->pPrevSleepRun;
					pth->pNextSleepRun = pth2;
					pth->pPrevSleepRun->pNextSleepRun = pth2->pPrevSleepRun = pth;
				} else if (prio == GET_CPRIO(pth2)) {
					// Run list for current thread priority exists, make current thread first if time
					// left in quantum or if quantum is 0 (run to completion).
					if (pth->dwQuantLeft || !pth->dwQuantum) {
						// make first
						pth->pPrevSleepRun = pth2->pPrevSleepRun;
						pth->pPrevSleepRun->pNextSleepRun = pth;
						if (pth->pNextSleepRun = pth2->pNextSleepRun)
							pth->pNextSleepRun->pPrevSleepRun = pth;
						pth2->pPrevSleepRun = pth2->pNextSleepRun = 0;
						pth->pUpRun = pth2->pUpRun;
						pth->pUpRun->pDownRun = pth2->pUpRun = pth;
						pth->pDownRun = pth2;

						// Update hash table entry to new first thread on list.
						RunList.pHashThread[prio2] = pth;
					} else {
						// make last
						pth->dwQuantLeft = pth->dwQuantum;
						pth->pUpRun = pth2->pUpRun;
						pth->pUpRun->pDownRun = pth2->pUpRun = pth;
						pth->pDownRun = pth2;
						pth->pPrevSleepRun = pth->pNextSleepRun = 0;
					}		
				} else {
					// Look for appropriate run list to enqueue current thread on.
	FinishEnqueue:
					// bounded by PRIORITY_LEVELS_HASHSCALE
					while ((pth3 = pth2->pNextSleepRun) && (prio > GET_CPRIO(pth3)))
						pth2 = pth3;
					if (prio == GET_CPRIO(pth2)) {
						// Found run list.  Make current thread first if time left in quantum
						// or if quantum is 0 (run to completion).
						if (pth->dwQuantLeft || !pth->dwQuantum) {
							// make first
						 	pth->pPrevSleepRun = pth2->pPrevSleepRun;
						 	pth->pPrevSleepRun->pNextSleepRun = pth;
						 	if (pth->pNextSleepRun = pth2->pNextSleepRun)
						 		pth->pNextSleepRun->pPrevSleepRun = pth;
						 	pth2->pPrevSleepRun = pth2->pNextSleepRun = 0;
							pth->pUpRun = pth2->pUpRun;
							pth->pUpRun->pDownRun = pth2->pUpRun = pth;
							pth->pDownRun = pth2;
						} else {
							// make last
							pth->dwQuantLeft = pth->dwQuantum;
							pth->pUpRun = pth2->pUpRun;
							pth->pUpRun->pDownRun = pth2->pUpRun = pth;
							pth->pDownRun = pth2;
							pth->pPrevSleepRun = pth->pNextSleepRun = 0;
						}		
					} else if (!pth3) {
						// New run list.
						pth->pNextSleepRun = 0;
						pth->pPrevSleepRun = pth2;
						pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth;
					} else {
						 if (prio == GET_CPRIO(pth3)) {
						 	// Found run list.  Make current thread first if time left in quantum
							// or if quantum is 0 (run to completion).
						 	if (pth->dwQuantLeft || !pth->dwQuantum) {
						 		// make first
							 	pth->pPrevSleepRun = pth3->pPrevSleepRun;
							 	pth->pPrevSleepRun->pNextSleepRun = pth;
							 	if (pth->pNextSleepRun = pth3->pNextSleepRun)
							 		pth->pNextSleepRun->pPrevSleepRun = pth;
							 	pth3->pPrevSleepRun = pth3->pNextSleepRun = 0;
								pth->pUpRun = pth3->pUpRun;
								pth->pUpRun->pDownRun = pth3->pUpRun = pth;
								pth->pDownRun = pth3;
							} else {
								// make last
								pth->dwQuantLeft = pth->dwQuantum;
								pth->pUpRun = pth3->pUpRun;
								pth->pUpRun->pDownRun = pth3->pUpRun = pth;
								pth->pDownRun = pth3;
								pth->pPrevSleepRun = pth->pNextSleepRun = 0;
							}	
						 } else {
						 	// New run list.
							pth->pPrevSleepRun = pth2;
							pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth3->pPrevSleepRun = pth;
							pth->pNextSleepRun = pth3;
						 }
					}
				}
			} else {
				// No hash table entry for current thread priority exists.  Point to current thread
				// and enqueue.
				RunList.pHashThread[prio2] = pth;
				// bounded by PRIORITY_LEVELS_HASHSIZE
				while (!(pth2 = RunList.pHashThread[--prio2]))
					;
				goto FinishEnqueue;
			}
		}
		SET_RUNSTATE(pth,RUNSTATE_RUNNABLE);
		RunList.pth = 0;
		bPreempt = 0;
	} else if (bPreempt) {
		if (!bLastIdle) {
			pCurThread->dwQuantLeft = pCurThread->dwQuantum;
			if (pCurThread->dwQuantum)
				dwPreempt = pCurThread->dwQuantum + DiffMSec;
			else
				dwPreempt = 0;		// 0 if Quantum=0
		}
		bPreempt = 0;
	}
findHighest:
	if (!RunList.pth && (pth = RunList.pRunnable)) {
		if ((prio = GET_CPRIO(pth)) > currmaxprio) {
			pOwner = pOOMThread;
			while (pOwner && !PendEvents) {
				if (GET_RUNSTATE(pOwner) == RUNSTATE_RUNNABLE) {
					pth = pOwner;
					prio = GET_CPRIO(pOwner);
					goto foundone;
				}
				if (GET_RUNSTATE(pOwner) == RUNSTATE_NEEDSRUN) {
					if (GET_SLEEPING(pOwner))
						SleepqDequeue(pOwner);
					MakeRun(pOwner);
					goto findHighest;
				}
				if ((GET_RUNSTATE(pOwner) == RUNSTATE_BLOCKED) && !GET_SLEEPING(pOwner) &&
					 pOwner->lpProxy && pOwner->lpProxy->pQDown &&
					 !pOwner->lpProxy->pThLinkNext &&
					 (pOwner->lpProxy->bType == HT_CRITSEC)) {
					pCrit = (LPCRIT)pOwner->lpProxy->pObject;
					DEBUGCHK(pCrit);
					SWITCHKEY(dwTmp, 0xffffffff);
					lpcs = pCrit->lpcs;
					DEBUGCHK(lpcs);
					DEBUGCHK(lpcs->OwnerThread);
					pOwner = HandleToThread((HANDLE)((DWORD)lpcs->OwnerThread & ~1));
					SETCURKEY(dwTmp);
					DEBUGCHK(pOwner);
				} else
					pOwner = 0;
			}
			goto nonewthread;
		}
foundone:
		RunList.pth = pth;
		SET_RUNSTATE(pth,RUNSTATE_RUNNING);
		RunqDequeue(pth,prio);
		pth->hLastCrit = 0;
		pth->lpCritProxy = 0;
		bPreempt = 0;
		if (!pth->dwQuantLeft)
			pth->dwQuantLeft = pth->dwQuantum;
		if (pth->dwQuantum)
			dwPreempt = pth->dwQuantLeft + DiffMSec;
		else
			dwPreempt = 0;		// 0 if Quantum=0
		if (!GET_NOPRIOCALC(pth)) {
			prio = GET_BPRIO(pth);
			if (pth->pOwnedList && ((prio2 = pth->pOwnedList->bListedPrio) < prio))
				prio = prio2;
			if (prio > GET_CPRIO(pth)) {
				SET_CPRIO(pth,prio);
				CELOG_SystemInvert(pth->hTh, prio);
            if (RunList.pRunnable && (prio > GET_CPRIO(RunList.pRunnable)))
					SetReschedule();
			}
		}
	}
nonewthread:
	NewTime = CurMSec;

	randdw2 = ((randdw2<<5) | (randdw2>>27)) ^ ((NewTime>>5) & 0x1f);
	
	if (!bLastIdle) {
		if (GET_TIMEMODE(pCurThread) == TIMEMODE_USER)
			pCurThread->dwUserTime += NewTime - CurrTime;
		else
			pCurThread->dwKernTime += NewTime - CurrTime;
	}
	CurrTime = NewTime;
	bLastIdle = !RunList.pth;
	if (!bLastIdle) {
      CELOG_ThreadSwitch(RunList.pth);
		if (pLogThreadSwitch)
			pLogThreadSwitch((DWORD)RunList.pth,(DWORD)RunList.pth->pOwnerProc);
	} else {
		dwPreempt = 0;
      CELOG_ThreadSwitch(0);
		if (pLogThreadSwitch)
			pLogThreadSwitch(0,0);
	}
#ifndef SHIP_BUILD
	if (pCurThread != RunList.pth)
		KInfoTable[KINX_DEBUGGER]++;
#endif
#ifdef SH4
	if (RunList.pth) {
		if (g_CurFPUOwner != RunList.pth)
			RunList.pth->ctx.Psr |= 0x8000;
		else
			RunList.pth->ctx.Psr &= ~0x8000;
	}
#endif
#ifdef SH3
    if (RunList.pth) {
        if (g_CurDSPOwner != RunList.pth)
            RunList.pth->ctx.Psr &= ~0x1000;
        else
            RunList.pth->ctx.Psr |= 0x1000;
    }
#endif
	KCALLPROFOFF(44);
}

LPTHRDDBG AllocDbg(HANDLE hProc) {
	LPTHRDDBG pdbg;
	if (!(pdbg = (LPTHRDDBG)AllocMem(HEAP_THREADDBG)))
		return 0;
	if (!(pdbg->hEvent = CreateEvent(0,0,0,0))) {
		FreeMem(pdbg,HEAP_THREADDBG);
		return 0;
	}
	if (!(pdbg->hBlockEvent = CreateEvent(0,0,0,0))) {
		CloseHandle(pdbg->hEvent);
		FreeMem(pdbg,HEAP_THREADDBG);
		return 0;
	}
	if (hProc) {
		SC_SetHandleOwner(pdbg->hEvent,hProc);
		SC_SetHandleOwner(pdbg->hBlockEvent,hProc);
	}
	pdbg->hFirstThrd = 0;
	pdbg->psavedctx = 0;
	pdbg->dbginfo.dwDebugEventCode = 0;
	pdbg->bDispatched = 0;
	return pdbg;
}

BOOL InitThreadStruct(PTHREAD pTh, HANDLE hTh, PPROCESS pProc, HANDLE hProc, WORD prio) {
	SetUserInfo(hTh,STILL_ACTIVE);
	pTh->pProc = pProc;
	pTh->pOwnerProc = pProc;
	pTh->pProxList = 0;
	pTh->pThrdDbg = 0;
	pTh->aky = pProc->aky;
	AddAccess(&pTh->aky,ProcArray[0].aky);
	pTh->hTh = hTh;
	pTh->pcstkTop = 0;
	pTh->bSuspendCnt = 0;
	pTh->wInfo = 0;
	pTh->bWaitState = WAITSTATE_SIGNALLED;
	pTh->lpCritProxy = 0;
	pTh->lpProxy = 0;
	pTh->lpce = 0;
	pTh->dwLastError = 0;
	pTh->dwUserTime = 0;
	pTh->dwKernTime = 0;
	pTh->dwQuantLeft = 0;
	pTh->dwQuantum = dwDefaultThreadQuantum;
	SET_BPRIO(pTh,prio);
	SET_CPRIO(pTh,prio);
	pTh->pOwnedList = 0;
	memset(pTh->pOwnedHash,0,sizeof(pTh->pOwnedHash));
	pTh->pSwapStack = 0;
	return TRUE;
}

DWORD ThreadSuspend(PTHREAD pth) {
	DWORD retval, cprio;
    ACCESSKEY ulOldKey;
	KCALLPROFON(5);
    SWITCHKEY(ulOldKey,0xffffffff);
	if (!pth) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		retval = 0xffffffff;
	} else if ((pth == pCurThread) && GET_DYING(pCurThread) && !GET_DEAD(pCurThread)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		retval = 0xfffffffe;
	} else if ((pth->bWaitState == WAITSTATE_PROCESSING) ||
			  (((GET_RUNSTATE(pth) == RUNSTATE_RUNNABLE) || (GET_RUNSTATE(pth) == RUNSTATE_NEEDSRUN)) && !GET_NEEDDBG(pth) &&
			   (((GetThreadIP(pth) >= pTOC->physfirst) && (GetThreadIP(pth) < pTOC->physlast) && !GET_USERBLOCK(pth)) ||
				(csDbg.OwnerThread == pth->hTh) || (ModListcs.OwnerThread == pth->hTh) ||
				(LLcs.OwnerThread == pth->hTh) || (DbgApiCS.OwnerThread == pth->hTh)))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		retval = 0xfffffffe;
	} else {
		retval = pth->bSuspendCnt;
		if (pth->bSuspendCnt) {
			if (pth->bSuspendCnt != MAX_SUSPEND_COUNT)
				pth->bSuspendCnt++;
			else {
				KSetLastError(pCurThread,ERROR_SIGNAL_REFUSED);
				retval = 0xffffffff;
			}
		} else {
			pth->bSuspendCnt = 1;
			switch (GET_RUNSTATE(pth)) {
				case RUNSTATE_RUNNING:
					DEBUGCHK(RunList.pth == pth);
					RunList.pth = 0;
					SetReschedule();
					break;
				case RUNSTATE_RUNNABLE:
					cprio = GET_CPRIO(pth);
					RunqDequeue(pth,cprio);
					break;
				case RUNSTATE_NEEDSRUN:
					if (GET_SLEEPING(pth))
						SleepqDequeue(pth);
					break;
			}
//			DEBUGCHK(!((pth->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
			SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
		}
	}
	SETCURKEY(ulOldKey);
	KCALLPROFOFF(5);
	return retval;
}

/* When a thread gets SuspendThread() */

DWORD SC_ThreadSuspend(HANDLE hTh) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSuspend entry: %8.8lx\r\n",hTh));
	if (hTh == GetCurrentThread())
		hTh = hCurThread;
   CELOG_ThreadSuspend(hTh);
	retval = KCall((PKFN)ThreadSuspend,HandleToThread(hTh));
	if (retval == 0xfffffffe)
		retval = 0xffffffff;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSuspend exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD UB_ThreadSuspend(HANDLE hTh) {
	DWORD retval;
	if ((hTh == (HANDLE)GetCurrentThread()) || (hTh == hCurThread))
		SET_USERBLOCK(pCurThread);
    retval = SC_ThreadSuspend(hTh);
	if ((hTh == (HANDLE)GetCurrentThread()) || (hTh == hCurThread))
		CLEAR_USERBLOCK(pCurThread);
	return retval;
}

DWORD ThreadResume(PTHREAD pth) {
	DWORD retval = 0;
	KCALLPROFON(47);
	if (pth && (retval = pth->bSuspendCnt) && !--pth->bSuspendCnt && (GET_RUNSTATE(pth) == RUNSTATE_BLOCKED) && !GET_SLEEPING(pth) && !pth->lpProxy)
		MakeRun(pth);
   CELOG_ThreadResume(pth->hTh);
	KCALLPROFOFF(47);
	return retval;
}

/* When a thread gets ResumeThread() */

DWORD SC_ThreadResume(HANDLE hTh) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadResume entry: %8.8lx\r\n",hTh));
	if (hTh == GetCurrentThread())
		hTh = hCurThread;
	retval = KCall((PKFN)ThreadResume,HandleToThread(hTh));
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadResume exit: %8.8lx\r\n",retval));
	return retval;
}

void ThreadYield(void) {
	KCALLPROFON(35);
	DEBUGCHK(GET_RUNSTATE(pCurThread) == RUNSTATE_RUNNING);
	DEBUGCHK(RunList.pth == pCurThread);
	RunList.pth = 0;
	SetReschedule();
	MakeRun(pCurThread);
	KCALLPROFOFF(35);
}

typedef struct sleeper_t {
	PTHREAD pth;
	WORD wCount;
	WORD wPad;
	DWORD dwMilli;
	DWORD dwRemain;
} sleeper_t;

DWORD PutThreadToSleep(sleeper_t *pSleeper) {
	PTHREAD pth, pth2;
	PTHREAD *pPrevNext;
	KCALLPROFON(2);
	DEBUGCHK(!GET_SLEEPING(pCurThread));
	if (!GET_DYING(pCurThread) || GET_DEAD(pCurThread) || !GET_USERBLOCK(pCurThread)) {
		if (pth = pSleeper->pth) {
			if (pth->wCount != pSleeper->wCount) {
				pSleeper->pth = 0;
				KCALLPROFOFF(2);
				return 1;
			}
#ifdef DEBUG
			if (pth->wCount == 0xabab && (DWORD) pth->pProc == 0xabababab) {
				DEBUGMSG(1, (TEXT("PutThreadToSleep : restarting...\r\n")));
				pSleeper->pth = 0;
				KCALLPROFOFF(2);
				return 1;
			}
#endif
			DEBUGCHK((GET_RUNSTATE(pth) == RUNSTATE_BLOCKED) || (GET_RUNSTATE(pth) == RUNSTATE_NEEDSRUN));
			DEBUGCHK(GET_SLEEPING(pth));
			pPrevNext = &pth->pNextSleepRun;
		} else
			pPrevNext = &SleepList;
		if (pCurThread->pNextSleepRun = pth2 = *pPrevNext) {
			if ((pth2->dwSleepCnt < pSleeper->dwMilli) || ((pth2->dwSleepCnt == pSleeper->dwMilli) && (GET_CPRIO(pth2) <= GET_CPRIO(pCurThread)))) {
				pSleeper->dwRemain = pth2->dwSleepCnt;
				pSleeper->wCount = pth2->wCount;
				pSleeper->pth = pth2;
				KCALLPROFOFF(2);
				return 1;
			}
			pth2->pPrevSleepRun = pCurThread;
			pth2->dwSleepCnt -= pSleeper->dwMilli;
		}
		*pPrevNext = pCurThread;
		pCurThread->pPrevSleepRun = pth;
		pCurThread->dwSleepCnt = pSleeper->dwMilli;
		if (pPrevNext == &SleepList)
			dwSleepMin = pCurThread->dwSleepCnt;
		RunList.pth = 0;
		SetReschedule();
		DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
		SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
		SET_SLEEPING(pCurThread);
	}
	KCALLPROFOFF(2);
	return 0;
}

void ThreadSleep(HTHREAD hth, DWORD time) {
	DWORD dwDiff, dwStart, period;
	sleeper_t sleeper;
	sleeper.pth = 0;
	dwStart = CurMSec;
	period = 1 + DiffMSec + ticksleft; // assume 1ms clock resolution
	if (time + period < time) {
		period = time;
		time = 0xffffffff;
	} else {
		time += period;
		period = time - period;
	}
	sleeper.dwMilli = time;
	sleeper.dwRemain = 0;
	while (KCall((PKFN)PutThreadToSleep,&sleeper) && ((dwDiff = CurMSec - dwStart) < period))
		sleeper.dwMilli = (sleeper.pth ? sleeper.dwMilli - sleeper.dwRemain : time - dwDiff);
}

void SC_Sleep(DWORD cMilliseconds) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_Sleep entry: %8.8lx\r\n",cMilliseconds));
	
	CELOG_Sleep(cMilliseconds);
	
	if (cMilliseconds == INFINITE)
		KCall((PKFN)ThreadSuspend,pCurThread);
	else if (!cMilliseconds)
		KCall((PKFN)ThreadYield);
	else
		ThreadSleep(hCurThread,cMilliseconds);
	DEBUGMSG(ZONE_ENTRY,(L"SC_Sleep exit\r\n"));
}

void UB_Sleep(DWORD cMilliseconds) {
	SET_USERBLOCK(pCurThread);
	SC_Sleep(cMilliseconds);
	CLEAR_USERBLOCK(pCurThread);
}

void ZeroTLS(PTHREAD pth) {
	memset(pth->tlsPtr,0,4*TLS_MINIMUM_AVAILABLE);
}

/* Creates a thread in a process */

HANDLE DoCreateThread(LPVOID lpStack, DWORD cbStack, LPVOID lpStart, LPVOID param, DWORD flags, LPDWORD lpthid, ulong mode, WORD prio) {
	PTHREAD pth, pdbgr;
	HANDLE hth;
	LPBYTE pSwapStack;
	DEBUGMSG(ZONE_SCHEDULE,(TEXT("Scheduler: DoCreateThread(%8.8lx, %8.8lx, %8.8lx, %8.8lx, %8.8lx, %8.8lx, %8.8lx %8.8lx)\r\n"),
			hCurThread,hCurProc,cbStack,lpStart,param,flags,lpthid,prio));
	if (!TBFf) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	if (flags & 0x80000000)
		pSwapStack = 0;
	else if (!(pSwapStack = GetHelperStack())) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	pth = AllocMem(HEAP_THREAD);
	if (!pth) {
		FreeHelperStack(pSwapStack);
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	hth = AllocHandle(&cinfThread,pth,pCurProc);
	if (!hth) {
		FreeHelperStack(pSwapStack);
		FreeMem(pth,HEAP_THREAD);
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	IncRef(hth,pCurProc);
	if (!InitThreadStruct(pth,hth,pCurProc,hCurProc,prio)) {
		FreeHelperStack(pSwapStack);
		FreeMem(pth,HEAP_THREAD);
		FreeHandle(hth);
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	pth->pSwapStack = pSwapStack;
	GCFT(&pth->ftCreate);
	if (flags & 0x80000000) {
		SET_DYING(pth);
		SET_DEAD(pth);
		MDCreateThread(pth, lpStack, cbStack, lpStart, param, pCurProc->dwVMBase, mode, (ulong)param);
	} else if (flags & 0x40000000)
		MDCreateThread(pth, lpStack, cbStack, (LPVOID)lpStart, param, pCurProc->dwVMBase, mode, (ulong)param);
	else
		MDCreateThread(pth, lpStack, cbStack, (LPVOID)TBFf, lpStart, pCurProc->dwVMBase, mode, (ulong)param);
	ZeroTLS(pth);
	pth->bSuspendCnt = 1;
	EnterCriticalSection(&DbgApiCS);
	if (pCurProc->hDbgrThrd) {
		pdbgr = HandleToThread(pCurProc->hDbgrThrd);
		DEBUGCHK(pdbgr);
		pth->pThrdDbg = AllocDbg(hCurProc);
		DEBUGCHK(pth->pThrdDbg);
		pth->pThrdDbg->hNextThrd = pdbgr->pThrdDbg->hFirstThrd;
		pdbgr->pThrdDbg->hFirstThrd = hth;
		IncRef(pth->hTh,pdbgr->pOwnerProc);
	}
	DEBUGCHK(!((pth->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
	SET_RUNSTATE(pth,RUNSTATE_BLOCKED);

   // Save off the thread's program counter for getting its name later.
   pth->dwStartAddr = (DWORD) lpStart;
   
   CELOG_ThreadCreate(pth, pth->pOwnerProc->hProc, NULL);
	if (pLogThreadCreate)
		pLogThreadCreate((DWORD)pth,(DWORD)pCurProc);
	KCall((PKFN)AddToProcRunnable,pCurProc,pth);
	LeaveCriticalSection(&DbgApiCS);
	if (!(flags & CREATE_SUSPENDED))
		KCall((PKFN)ThreadResume,pth);
	if (lpthid)
		*lpthid = (DWORD)hth;
	return hth;
}

// Creates a thread

HANDLE SC_CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD cbStack, LPTHREAD_START_ROUTINE lpStartAddr,
	LPVOID lpvThreadParm, DWORD fdwCreate, LPDWORD lpIDThread) {
	HANDLE retval;
	LPVOID lpStack;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
		lpsa,cbStack,lpStartAddr,lpvThreadParm,fdwCreate,lpIDThread));
	if (PageFreeCount < (48*1024/PAGE_SIZE)) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread exit: %8.8lx\r\n",0));
		return 0;
	}
	if (fdwCreate & ~(CREATE_SUSPENDED)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread exit: %8.8lx\r\n",0));
		return 0;
	}
	lpStack = VirtualAlloc((LPVOID)pCurProc->dwVMBase,pCurProc->e32.e32_stackmax,MEM_RESERVE|MEM_AUTO_COMMIT,PAGE_NOACCESS);
	if (!lpStack) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread exit: %8.8lx\r\n",0));
		return 0;
	}
	if (!VirtualAlloc((LPVOID)((DWORD)lpStack+pCurProc->e32.e32_stackmax-MIN_STACK_SIZE),MIN_STACK_SIZE, MEM_COMMIT,PAGE_READWRITE)) {
		VirtualFree(lpStack,0,MEM_RELEASE);
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread exit: %8.8lx\r\n",0));
		return 0;
	}
	retval = DoCreateThread(lpStack, pCurProc->e32.e32_stackmax, (LPVOID)lpStartAddr, lpvThreadParm, fdwCreate, lpIDThread, TH_UMODE,THREAD_RT_PRIORITY_NORMAL);
	if (!retval) {
		VirtualFree(lpStack,pCurProc->e32.e32_stackmax,MEM_DECOMMIT);
		VirtualFree(lpStack,0,MEM_RELEASE);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateThread exit: %8.8lx\r\n",retval));
	return retval;
}

// Creates a kernel thread

HANDLE CreateKernelThread(LPTHREAD_START_ROUTINE lpStartAddr, LPVOID lpvThreadParm, WORD prio, DWORD dwFlags) {
	LPVOID lpStack;
	if (!(lpStack = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,KRN_STACK_SIZE,MEM_RESERVE|MEM_AUTO_COMMIT,PAGE_NOACCESS)))
		return 0;
	if (!VirtualAlloc((LPVOID)((DWORD)lpStack+KRN_STACK_SIZE-MIN_STACK_SIZE),MIN_STACK_SIZE,MEM_COMMIT,PAGE_READWRITE)) {
		VirtualFree(lpStack,0,MEM_RELEASE);
		return 0;
	}
	return DoCreateThread(lpStack, KRN_STACK_SIZE, (LPVOID)lpStartAddr, lpvThreadParm, dwFlags, 0, TH_KMODE, prio);
}

BOOL SetThreadBasePrio(HANDLE hth, DWORD nPriority) {
	BOOL bRet;
	PTHREAD pth;
	DWORD oldb, oldc, prio;
	KCALLPROFON(28);
	bRet = FALSE;
	if (pth = HandleToThread(hth)) {
		oldb = GET_BPRIO(pth);
		if (oldb != nPriority) {
			oldc = GET_CPRIO(pth);
			SET_BPRIO(pth,(WORD)nPriority);
			if (pCurThread == pth) {
				if (pCurThread->pOwnedList && ((prio = pCurThread->pOwnedList->bListedPrio) < nPriority))
					nPriority = prio;
				if (nPriority != GET_CPRIO(pCurThread)) {
					SET_CPRIO(pCurThread,nPriority);
					if (RunList.pRunnable && (nPriority > GET_CPRIO(RunList.pRunnable)))
						SetReschedule();
				}
			} else if (nPriority < oldc) {
				SET_CPRIO(pth,nPriority);
				if (GET_RUNSTATE(pth) == RUNSTATE_RUNNABLE) {
					RunqDequeue(pth,oldc);
					MakeRun(pth);
				}
			}
		}
		bRet = TRUE;
	}
	KCALLPROFOFF(28);
	return bRet;
}

typedef struct sttd_t {
	HANDLE hThNeedRun;
	HANDLE hThNeedBoost;
} sttd_t;

HANDLE SetThreadToDie(HANDLE hTh, DWORD dwExitCode, sttd_t *psttd) {
	PTHREAD pth;
	HANDLE hRet;
	DWORD prio;
	LPPROXY pprox;
	KCALLPROFON(17);
	if (!hTh)
		hRet = pCurProc->pTh->hTh;
	else if (!(pth = HandleToThread(hTh)))
		hRet = 0;
	else {
		hRet = pth->pNextInProc ? pth->pNextInProc->hTh : 0;
		if (!GET_DYING(pth) && !GET_DEAD(pth)) {
			SET_DYING(pth);
			pth->tlsPtr[TLSSLOT_KERNEL] |= TLSKERN_TRYINGTODIE;
			SetUserInfo(hTh,dwExitCode);
			psttd->hThNeedBoost = hTh;
			switch (GET_RUNSTATE(pth)) {
				case RUNSTATE_RUNNABLE:
					if ((pth->pOwnerProc == pth->pProc) && (LLcs.OwnerThread != hTh) && !(GetThreadIP(pth) & 0x80000000)) {
						if (pth->bWaitState == WAITSTATE_PROCESSING) {
							if (pth->lpce) {
								pth->lpce->base = pth->lpProxy;
								pth->lpce->size = (DWORD)pth->lpPendProxy;
							} else if (pprox = pth->lpProxy) {
								// must be an interrupt event - special case it!
								DEBUGCHK(pprox->bType == HT_MANUALEVENT);
								DEBUGCHK(((LPEVENT)pprox->pObject)->pProxList == pprox);
								((LPEVENT)pprox->pObject)->pProxList = 0;
								pprox->pQDown = 0;
							}
							pth->lpProxy = 0;
							pth->wCount++;
							pth->bWaitState = WAITSTATE_SIGNALLED;
						} else if (!pth->pcstkTop || ((DWORD)pth->pcstkTop->pprcLast >= 0x10000)) {
							prio = GET_CPRIO(pth);
							RunqDequeue(pth,prio);
							SetThreadIP(pth, pExitThread);
#ifdef THUMBSUPPORT
				            if ((DWORD)pExitThread & 1)
				                pth->ctx.Psr |= THUMB_STATE;
							else
				                pth->ctx.Psr &= ~THUMB_STATE;
#endif
							SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
							psttd->hThNeedRun = hTh;
						}
					}
					break;
				case RUNSTATE_NEEDSRUN:
					if ((pth->pOwnerProc == pth->pProc) && (LLcs.OwnerThread != hTh) && !(GetThreadIP(pth) & 0x80000000)) {
						pth->bSuspendCnt = (hTh == hCurrScav ? 1 : 0);
						SetThreadIP(pth, pExitThread);
#ifdef THUMBSUPPORT
			            if ((DWORD)pExitThread & 1)
			                pth->ctx.Psr |= THUMB_STATE;
						else
			                pth->ctx.Psr &= ~THUMB_STATE;
#endif
						psttd->hThNeedRun = hTh;
					}
					break;
				case RUNSTATE_BLOCKED:
					if ((pth->pOwnerProc == pth->pProc) && (LLcs.OwnerThread != hTh) &&
						(GET_USERBLOCK(pth) || !(GetThreadIP(pth) & 0x80000000))) {
						if (!GET_USERBLOCK(pth)) {
							SetThreadIP(pth, pExitThread);
#ifdef THUMBSUPPORT
				            if ((DWORD)pExitThread & 1)
				                pth->ctx.Psr |= THUMB_STATE;
							else
				                pth->ctx.Psr &= ~THUMB_STATE;
#endif
						} else if (pprox = pth->lpProxy) {
							if (pth->lpce) {
								pth->lpce->base = pprox;
								pth->lpce->size = (DWORD)pth->lpPendProxy;
							} else {
								// must be an interrupt event - special case it!
								DEBUGCHK(pprox->bType == HT_MANUALEVENT);
								DEBUGCHK(((LPEVENT)pprox->pObject)->pProxList == pprox);
								((LPEVENT)pprox->pObject)->pProxList = 0;
								pprox->pQDown = 0;
							}
							pth->lpProxy = 0;
							pth->bWaitState = WAITSTATE_SIGNALLED;
						}
						pth->bSuspendCnt = (hTh == hCurrScav ? 1 : 0);
						pth->wCount++;
						SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
						psttd->hThNeedRun = hTh;
					}
					break;
				case RUNSTATE_RUNNING:
					break;
				default:
					DEBUGCHK(0);
					break;
			}
		}
	}
	KCALLPROFOFF(17);
	return hRet;
}

HANDLE KillOneThread(HANDLE hTh, DWORD dwRetVal) {
	sttd_t sttd;
	HANDLE hRet;
	ACCESSKEY ulOldKey;
	sttd.hThNeedRun = 0;
	sttd.hThNeedBoost = 0;
	SWITCHKEY(ulOldKey,0xffffffff);
	hRet = (HANDLE)KCall((PKFN)SetThreadToDie,hTh,dwRetVal,&sttd);
	SETCURKEY(ulOldKey);
	if (sttd.hThNeedBoost)
		KCall((PKFN)SetThreadBasePrio,sttd.hThNeedBoost,GET_CPRIO(pCurThread));
	if (sttd.hThNeedRun)
		KCall((PKFN)MakeRunIfNeeded,sttd.hThNeedRun);
	return hRet;
}

BOOL SC_ProcTerminate(HANDLE hProc, DWORD dwRetVal) {
    ACCESSKEY ulOldKey;
    PPROCESS pProc;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcTerminate entry: %8.8lx %8.8lx\r\n",hProc,dwRetVal));
	CELOG_ProcessTerminate(hProc);
	if (hProc == GetCurrentProcess())
		hProc = hCurProc;
	if (!(pProc = HandleToProc(hProc))) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		return FALSE;
	}
    SWITCHKEY(ulOldKey,0xffffffff);
	if (*(LPDWORD)MapPtrProc(pIsExiting,pProc)) {
		SETCURKEY(ulOldKey);
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ProcTerminate exit 1: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	KillOneThread(pProc->pMainTh->hTh,dwRetVal);

	SETCURKEY(ulOldKey);
	pPSLNotify(DLL_PROCESS_EXITING,(DWORD)hProc,(DWORD)NULL);
	SWITCHKEY(ulOldKey,0xffffffff);

	if ((hProc != hCurProc) && (WaitForMultipleObjects(1,&hProc,FALSE,3000) != WAIT_TIMEOUT)) {
		SETCURKEY(ulOldKey);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ProcTerminate exit 2: %8.8lx\r\n",TRUE));
		return TRUE;
	}
	SETCURKEY(ulOldKey);
	KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
   DEBUGMSG(ZONE_ENTRY,(L"SC_ProcTerminate exit 3: %8.8lx\r\n",FALSE));
	return FALSE;
}

void SC_TerminateSelf(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_TerminateSelf entry\r\n"));
	SC_ProcTerminate(hCurProc,0);
	DEBUGMSG(ZONE_ENTRY,(L"SC_TerminateSelf exit\r\n"));
}

BOOL SC_ThreadTerminate(HANDLE hThread, DWORD dwExitcode) {
	PTHREAD pTh;
   CELOG_ThreadTerminate(hThread);
   if (hThread == GetCurrentThread())
		hThread = hCurThread;
   if (pTh = HandleToThread(hThread)) {
		if (!pTh->pNextInProc)
			return SC_ProcTerminate(pTh->pOwnerProc->hProc, dwExitcode);
		KillOneThread(hThread,dwExitcode);
		if ((hThread != hCurThread) && (WaitForMultipleObjects(1,&hThread,FALSE,3000) != WAIT_TIMEOUT))
			return TRUE;
	}
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

/* Returns TRUE until only the main thread is around */

BOOL SC_OtherThreadsRunning(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_OtherThreadsRunning entry\r\n"));
	DEBUGCHK(pCurProc == pCurThread->pOwnerProc);
	if ((pCurProc->pTh != pCurThread) || pCurProc->dwDyingThreads) {
		DEBUGCHK(!(pCurProc->dwDyingThreads & 0x80000000));
		DEBUGMSG(ZONE_ENTRY,(L"SC_OtherThreadsRunning exit: %8.8lx\r\n",TRUE));
		return TRUE;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_OtherThreadsRunning exit: %8.8lx\r\n",FALSE));
	return FALSE;
}

// Final clean-up for a thread, and the process if it's the main thread
void RemoveThread(RTs *pRTs) {
	PTHREAD pth, pth2, pth3;
	DWORD prio, prio2;
	KCALLPROFON(43);
	DEBUGCHK(!pCurThread->pOwnedList);
	DEBUGCHK(RunList.pth == pCurThread);
	RunList.pth = 0;
	SetReschedule();
   if (pCurThread->pNextInProc) {
		if (!(pCurThread->pNextInProc->pPrevInProc = pCurThread->pPrevInProc)) {
			DEBUGCHK(pCurThread->pOwnerProc->pTh == pCurThread);
			pCurThread->pOwnerProc->pTh = pCurThread->pNextInProc;
		} else
			pCurThread->pPrevInProc->pNextInProc = pCurThread->pNextInProc;
	}
	pRTs->pThrdDbg = pCurThread->pThrdDbg;
	pRTs->hThread = hCurThread;
	SetObjectPtr(hCurThread,0);
	pth = pRTs->pHelper;
	DEBUGCHK(pth->bSuspendCnt == 1);
	pth->bSuspendCnt = 0;
	SET_RUNSTATE(pth,RUNSTATE_RUNNABLE);
	prio = GET_CPRIO(pth);
	prio2 = prio/PRIORITY_LEVELS_HASHSCALE;
	if (!RunList.pRunnable) {
		pth->pPrevSleepRun = pth->pNextSleepRun = 0;
		pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = RunList.pRunnable = pth;
	} else if (prio < GET_CPRIO(RunList.pRunnable)) {
		pth->pPrevSleepRun = 0;
		pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = RunList.pRunnable->pPrevSleepRun = pth;
		pth->pNextSleepRun = RunList.pRunnable;
		RunList.pRunnable = pth;
	} else if (pth2 = RunList.pHashThread[prio2]) {
		if (prio < GET_CPRIO(pth2)) {
			pth->pUpRun = pth->pDownRun = RunList.pHashThread[prio2] = pth;
			pth->pPrevSleepRun = pth2->pPrevSleepRun;
			pth->pNextSleepRun = pth2;
			pth->pPrevSleepRun->pNextSleepRun = pth2->pPrevSleepRun = pth;
		} else {
FinishMakeRun:
			// bounded by PRIORITY_LEVELS_HASHSCALE
			while ((pth3 = pth2->pNextSleepRun) && (prio > GET_CPRIO(pth3)))
				pth2 = pth3;
			if (prio == GET_CPRIO(pth2)) {
				pth->pUpRun = pth2->pUpRun;
				pth->pUpRun->pDownRun = pth2->pUpRun = pth;
				pth->pDownRun = pth2;
				pth->pPrevSleepRun = pth->pNextSleepRun = 0;
			} else if (!pth3) {
				pth->pNextSleepRun = 0;
				pth->pPrevSleepRun = pth2;
				pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth;
			} else {
				 if (prio == GET_CPRIO(pth3)) {
					pth->pUpRun = pth3->pUpRun;
					pth->pUpRun->pDownRun = pth;
					pth->pDownRun = pth3;
					pth3->pUpRun = pth;
					pth->pPrevSleepRun = pth->pNextSleepRun = 0;
				 } else {
					pth->pPrevSleepRun = pth2;
					pth->pUpRun = pth->pDownRun = pth2->pNextSleepRun = pth;
					pth->pNextSleepRun = pth3;
					pth3->pPrevSleepRun = pth;
				 }
			}
		}
	} else {
		RunList.pHashThread[prio2] = pth;
		// bounded by PRIORITY_LEVELS_HASHSIZE
		while (!(pth2 = RunList.pHashThread[--prio2]))
			;
		goto FinishMakeRun;
	}
	KCALLPROFOFF(43);
}



void SC_KillAllOtherThreads(void) {
	HANDLE hTh;
	DEBUGMSG(ZONE_ENTRY,(L"SC_KillAllOtherThreads entry\r\n"));
	hTh = pCurProc->pTh->hTh;
	while (hTh != hCurThread)
		hTh = KillOneThread(hTh,0);
	DEBUGMSG(ZONE_ENTRY,(L"SC_KillAllOtherThreads exit\r\n"));
}

BOOL SC_IsPrimaryThread(void) {
	BOOL retval;
	retval = (pCurThread->pNextInProc ? FALSE : TRUE);
	DEBUGMSG(ZONE_ENTRY,(L"SC_IsPrimaryThread exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD GetCurThreadKTime(void) {
    DWORD retval;
    KCALLPROFON(48);
    retval = pCurThread->dwKernTime;
    if (GET_TIMEMODE(pCurThread))
        retval += (CurMSec - CurrTime);
    KCALLPROFOFF(48);
    return retval;
}

DWORD GetCurThreadUTime(void) {
    DWORD retval;
    KCALLPROFON(65);
    retval = pCurThread->dwUserTime;
    if (!GET_TIMEMODE(pCurThread))
        retval += (CurMSec - CurrTime);
    KCALLPROFOFF(65);
    return retval;
}

BOOL SC_GetThreadTimes(HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime) {
	BOOL retval;
	DWORD dwKTime, dwUTime;
	PTHREAD pTh;
	THREADTIME *ptt;
	if (!(pTh = HandleToThreadPerm(hThread))) {
		LockPages(&lpCreationTime,sizeof(LPFILETIME),0,LOCKFLAG_WRITE);
		LockPages(&lpExitTime,sizeof(LPFILETIME),0,LOCKFLAG_WRITE);
		LockPages(&lpKernelTime,sizeof(LPFILETIME),0,LOCKFLAG_WRITE);
		LockPages(&lpUserTime,sizeof(LPFILETIME),0,LOCKFLAG_WRITE);
		EnterCriticalSection(&NameCS);
		for (ptt = TTList; ptt; ptt = ptt->pnext) {
			if (ptt->hTh == hThread) {
				__try {
					*lpCreationTime = ptt->CreationTime;
					*lpExitTime = ptt->ExitTime;
					*lpKernelTime = ptt->KernelTime;
					*lpUserTime = ptt->UserTime;
					retval = TRUE;
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					retval = FALSE;
				}
				break;
			}
		}
		LeaveCriticalSection(&NameCS);
		UnlockPages(&lpCreationTime,sizeof(LPFILETIME));
		UnlockPages(&lpExitTime,sizeof(LPFILETIME));
		UnlockPages(&lpKernelTime,sizeof(LPFILETIME));
		UnlockPages(&lpUserTime,sizeof(LPFILETIME));
		if (!ptt) {
			KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
			retval = FALSE;
		}
	} else {
		__try {
			dwUTime = (pTh != pCurThread) ? pTh->dwUserTime : KCall((PKFN)GetCurThreadUTime);
			dwKTime = (pTh != pCurThread) ? pTh->dwKernTime : KCall((PKFN)GetCurThreadKTime);
			*lpCreationTime = pTh->ftCreate;
			*(__int64 *)lpExitTime = 0;
			*(__int64 *)lpUserTime = dwUTime;
			*(__int64 *)lpUserTime *= 10000;
			*(__int64 *)lpKernelTime = dwKTime;
			*(__int64 *)lpKernelTime *= 10000;
			retval = TRUE;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			retval = FALSE;
		}
	}
	return retval;
}

#ifdef DEBUG

LPCRITICAL_SECTION csarray[] = {
	&ppfscs,
	&ODScs,
	&PhysCS,
	&VAcs,
	&CompCS,
	&MapCS,
	&PagerCS,
	&NameCS,
	&WriterCS,
	&ModListcs,
	&RFBcs,
	&MapNameCS,
	&DbgApiCS,
	&LLcs,
	&PageOutCS,
	&rtccs,
};

#define NUM_CSARRAY (sizeof(csarray) / sizeof(csarray[0]))

#define MAX_UNKCNT 20
LPCRITICAL_SECTION csUnknown[MAX_UNKCNT];
DWORD dwUnkCnt;

void CheckTakeCritSec(LPCRITICAL_SECTION lpcs) {
	DWORD loop;
	// anyone can call the debugger, and it may cause demand loads... but then they can't call the debugger
	if ((lpcs == &csDbg) || (lpcs == &EdbgODScs) || (lpcs == &TimerListCS))
		return;
	// We expect ethdbg's per-client critical sections to be unknown
	for (loop = 0; loop < NUM_CSARRAY; loop++)
		if (csarray[loop] == lpcs)
			break;
	if (loop == NUM_CSARRAY) {
		for (loop = 0; loop < dwUnkCnt; loop++)
			if (csUnknown[loop] == lpcs)
				return;
		if (dwUnkCnt == MAX_UNKCNT) {
			DEBUGMSG(1,(L"CHECKTAKECRITSEC: Unknown tracking space exceeded!\r\n"));
			return;
		}
		csUnknown[dwUnkCnt++] = lpcs;
		return;
	}
	for (loop = 0; loop < NUM_CSARRAY; loop++) {
		if (csarray[loop] == lpcs)
			return;
		if (csarray[loop]->OwnerThread == hCurThread) {
			DEBUGMSG(1,(L"CHECKTAKECRITSEC: Violation of critical section ordering at index %d, lpcs %8.8lx\r\n",loop,lpcs));
			DEBUGCHK(0);
			return;
		}
	}
	DEBUGMSG(1,(L"CHECKTAKECRITSEC: Bug in CheckTakeCritSec!\r\n"));
}

#endif

#ifdef START_KERNEL_MONITOR_THREAD

void Monitor1(void) {
	while (1) {
		Sleep(30000);
		NKDbgPrintfW(L"\r\n\r\n");
		NKDbgPrintfW(L" ODScs -> %8.8lx\r\n",ODScs.OwnerThread);
		NKDbgPrintfW(L" CompCS -> %8.8lx\r\n",CompCS.OwnerThread);
		NKDbgPrintfW(L" PhysCS -> %8.8lx\r\n",PhysCS.OwnerThread);
		NKDbgPrintfW(L" VAcs -> %8.8lx\r\n",VAcs.OwnerThread);
		NKDbgPrintfW(L" LLcs -> %8.8lx\r\n",LLcs.OwnerThread);
		NKDbgPrintfW(L" ModListcs -> %8.8lx\r\n",ModListcs.OwnerThread);
		NKDbgPrintfW(L" ppfscs -> %8.8lx\r\n",ppfscs.OwnerThread);
		NKDbgPrintfW(L" RFBcs -> %8.8lx\r\n",RFBcs.OwnerThread);
		NKDbgPrintfW(L" MapCS -> %8.8lx\r\n",MapCS.OwnerThread);
		NKDbgPrintfW(L" NameCS -> %8.8lx\r\n",NameCS.OwnerThread);
		NKDbgPrintfW(L" DbgApiCS -> %8.8lx\r\n",DbgApiCS.OwnerThread);
		NKDbgPrintfW(L" PagerCS -> %8.8lx\r\n",PagerCS.OwnerThread);
		NKDbgPrintfW(L" WriterCS -> %8.8lx\r\n",WriterCS.OwnerThread);
		NKDbgPrintfW(L" rtccs -> %8.8lx\r\n",rtccs.OwnerThread);
		NKDbgPrintfW(L" MapNameCS -> %8.8lx\r\n",MapNameCS.OwnerThread);
		NKDbgPrintfW(L"\r\n\r\n");
	}
}

#endif

void SC_ForcePageout(void) {
	ACCESSKEY ulOldKey;
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_ForcePageout failed due to insufficient trust\r\n"));
		return;
	}
	SWITCHKEY(ulOldKey,0xffffffff); // for pageout
	EnterPhysCS();
	ScavengeStacks(100000);     // Reclaim all extra stack pages.
	LeaveCriticalSection(&PhysCS);
	PageOutForced = 1;
	DoPageOut();
	PageOutForced = 0;
	SETCURKEY(ulOldKey);
}

LPCRIT lpCritList;

void FreeCrit(LPCRIT pcrit) {
	while (pcrit->pProxList)
		KCall((PKFN)DequeuePrioProxy,pcrit->pProxList);
	KCall((PKFN)DoFreeCrit,pcrit);
	FreeMem(pcrit,HEAP_CRIT);
}

HANDLE SC_CreateCrit(LPCRITICAL_SECTION lpcs) {
	LPCRIT pcrit, pTrav;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateCrit entry: %8.8lx\r\n",lpcs));
	if ((DWORD)lpcs & 1) {
		lpcs = MapPtr((LPCRITICAL_SECTION)((DWORD)lpcs & ~1));
		pcrit = (LPCRIT)(lpcs->hCrit);
		EnterCriticalSection(&NameCS);
		if (lpCritList) {
			for (pTrav = (LPCRIT)((DWORD)&lpCritList-offsetof(CRIT,pNext)); pTrav->pNext; pTrav = pTrav->pNext)
				if (pTrav->pNext == pcrit) {
					pTrav->pNext = pTrav->pNext->pNext;
					FreeCrit((LPCRIT)(lpcs->hCrit));
					break;
				}
		}
		LeaveCriticalSection(&NameCS);
	} else {
		lpcs = MapPtr(lpcs);
		if (!(pcrit = AllocMem(HEAP_CRIT))) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_CreateCrit exit: %8.8lx\r\n",0));
			return 0;
		}
		memset(pcrit->pProxHash,0,sizeof(pcrit->pProxHash));
		pcrit->pProxList = 0;
		pcrit->lpcs = lpcs;
		pcrit->bListed = 0;
		pcrit->iOwnerProc = pCurProc->procnum;
		EnterCriticalSection(&NameCS);
		pcrit->pNext = lpCritList;
		lpCritList = pcrit;
		LeaveCriticalSection(&NameCS);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateCrit exit: %8.8lx\r\n",pcrit));
	return pcrit;
}

void CloseAllCrits(void) {
	LPCRIT pTrav, pCrit;
	EnterCriticalSection(&NameCS);
	while (lpCritList && (lpCritList->iOwnerProc == pCurProc->procnum)) {
		pCrit = lpCritList;
		lpCritList = pCrit->pNext;
		FreeCrit(pCrit);
	}
	if (pTrav = lpCritList) {
		while (pCrit = pTrav->pNext) {
			if (pCrit->iOwnerProc == pCurProc->procnum) {
				pTrav->pNext = pCrit->pNext;
				FreeCrit(pCrit);
			} else
				pTrav = pCrit;
		}
	}
	LeaveCriticalSection(&NameCS);
}

void WaitConfig(LPPROXY pProx, CLEANEVENT *lpce, DWORD dwTimeout) {
	DWORD period;
	KCALLPROFON(1);
	DEBUGCHK(RunList.pth == pCurThread);
	DEBUGCHK(!pCurThread->lpProxy);
	pCurThread->lpPendProxy = pProx;
	pCurThread->lpce = lpce;
	pCurThread->bWaitState = WAITSTATE_PROCESSING;
	if (dwTimeout && (dwTimeout != INFINITE)) {
		period = 1 + DiffMSec + ticksleft;
		if ((dwTimeout + period < dwTimeout) || (dwTimeout + period == INFINITE))
			dwTimeout = INFINITE-1;
		else
			dwTimeout += period;
	}
	pCurThread->dwPendTime = dwTimeout;
	pCurThread->dwCrabTime = dwTimeout;
	pCurThread->dwPendStart = CurMSec;
	pCurThread->pCrabPth = 0;
	DEBUGCHK(RunList.pth == pCurThread);
	KCALLPROFOFF(1);
}

void PrioEnqueue(LPEVENT pevent, DWORD prio, LPPROXY pProx) {
	DWORD prio2;
	LPPROXY pProx2, pProx3;
	prio2 = prio/PRIORITY_LEVELS_HASHSCALE;
	if (!pevent->pProxList) {
		DEBUGCHK(!pProx->pQPrev && !pProx->pQNext);
		pProx->pQUp = pProx->pQDown = pevent->pProxHash[prio2] = pevent->pProxList = pProx;
	} else if (prio < pevent->pProxList->prio) {
		DEBUGCHK(!pProx->pQPrev);
		pProx->pQUp = pProx->pQDown = pevent->pProxHash[prio2] = pevent->pProxList->pQPrev = pProx;
		pProx->pQNext = pevent->pProxList;
		pevent->pProxList = pProx;
	} else if (pProx2 = pevent->pProxHash[prio2]) {
		if (prio < pProx2->prio) {
			pProx->pQPrev = pProx2->pQPrev;
			pProx->pQNext = pProx2;
			pProx->pQUp = pProx->pQDown = pevent->pProxHash[prio2] = pProx->pQPrev->pQNext = pProx2->pQPrev = pProx;
		} else {
FinishEnqueue:
			// bounded by MAX_PRIORITY_HASHSCALE
			while ((pProx3 = pProx2->pQNext) && (prio > pProx3->prio))
				pProx2 = pProx3;
			if (prio == pProx2->prio) {
				pProx->pQUp = pProx2->pQUp;
				pProx->pQUp->pQDown = pProx2->pQUp = pProx;
				pProx->pQDown = pProx2;
				DEBUGCHK(!pProx->pQPrev && !pProx->pQNext);
			} else if (!pProx3) {
				DEBUGCHK(!pProx->pQNext);
				pProx->pQPrev = pProx2;
				pProx->pQUp = pProx->pQDown = pProx2->pQNext = pProx;
			} else {
				 if (prio == pProx3->prio) {
					pProx->pQUp = pProx3->pQUp;
					pProx->pQUp->pQDown = pProx3->pQUp = pProx;
					pProx->pQDown = pProx3;
					DEBUGCHK(!pProx->pQPrev && !pProx->pQNext);
				 } else {
					pProx->pQUp = pProx->pQDown = pProx;
					pProx->pQPrev = pProx2;
					pProx2->pQNext = pProx3->pQPrev = pProx;
					pProx->pQNext = pProx3;
				 }
			}
		}
	} else {
		pevent->pProxHash[prio2] = pProx;
		// bounded by PRIORITY_LEVELS_HASHSIZE
		while (!(pProx2 = pevent->pProxHash[--prio2]))
			;
		goto FinishEnqueue;
	}
}

void FlatEnqueue(PTHREAD pth, LPPROXY pProx) {
	LPPROXY pProx2;
	if (!(pProx2 = pth->pProxList)) {
		pProx->pQUp = pProx->pQDown = pProx;
		pProx->pQPrev = (LPPROXY)pth;
		pth->pProxList = pProx;
	} else {
		DEBUGCHK(!pProx->pQPrev);
		pProx->pQUp = pProx2->pQUp;
		pProx->pQDown = pProx2;
		pProx2->pQUp = pProx->pQUp->pQDown = pProx;
	}
}

DWORD WaitOneMore(LPDWORD pdwContinue, LPCRIT *ppCritMut) {
	LPPROXY pProx;
	DWORD retval;
	PTHREAD pth, pth2, *pPrevNext;
	BOOL noenqueue;
	KCALLPROFON(18);
	noenqueue = !pCurThread->lpce && !pCurThread->dwPendTime;
	DEBUGCHK(RunList.pth == pCurThread);
	if (pCurThread->bWaitState == WAITSTATE_SIGNALLED) {
		*pdwContinue = 0;
		retval = pCurThread->dwPendReturn;
	} else if (GET_DYING(pCurThread) && !GET_DEAD(pCurThread) && GET_USERBLOCK(pCurThread)) {
		*pdwContinue = 0;
		retval = WAIT_FAILED;
		goto exitfunc;
	} else if (pProx = pCurThread->lpPendProxy) {
		DEBUGCHK(pCurThread->bWaitState == WAITSTATE_PROCESSING);
		DEBUGCHK(pCurThread == pProx->pTh);
		DEBUGCHK(pProx->pTh == pCurThread);
		DEBUGCHK(pProx->wCount == pCurThread->wCount);
		pProx->pObject = GetObjectPtr(pProx->pObject);
		switch (pProx->bType) {
			case SH_CURTHREAD: {
				PTHREAD pthWait;
				if (!(pthWait = (PTHREAD)pProx->pObject) || GET_DEAD(pthWait))
					goto waitok;
				if (!noenqueue)
					FlatEnqueue(pthWait,pProx);
				break;
			}
			case SH_CURPROC: {
				PPROCESS pprocWait;
				if (!(pprocWait = (PPROCESS)pProx->pObject))
					goto waitok;
				if (!noenqueue)
					FlatEnqueue((PTHREAD)pprocWait,pProx);
				break;
			}
			case HT_EVENT: {
				LPEVENT pevent;
				BYTE prio;
				if (!(pevent = (LPEVENT)pProx->pObject))
					goto waitbad;
				if (pevent->state) {	// event is already signalled
					DEBUGCHK(!pevent->pIntrProxy || !pevent->manualreset);
					pevent->state = pevent->manualreset;
					goto waitok;
				}
				if (!noenqueue) {
					prio = pProx->prio;
					if (!pevent->onequeue) {
						PrioEnqueue(pevent,prio,pProx);
					} else {
						pProx->bType = HT_MANUALEVENT;
						FlatEnqueue((PTHREAD)pevent,pProx);
						if (prio < pevent->bMaxPrio)
							pevent->bMaxPrio = prio;
					}
				}
				break;
			}
			case HT_SEMAPHORE: {
				LPSEMAPHORE psem;
				BYTE prio;
				if (!(psem = (LPSEMAPHORE)pProx->pObject))
					goto waitbad;
				if (psem->lCount) {
					psem->lCount--;
					goto waitok;
				}
				if (!noenqueue) {
					prio = pProx->prio;
					PrioEnqueue((LPEVENT)psem,prio,pProx);
				}
				break;
			}
			case HT_MUTEX: {
				LPMUTEX pmutex;
				BYTE prio;
				if (!(pmutex = (LPMUTEX)pProx->pObject))
					goto waitbad;
				if (!pmutex->pOwner) {
					pmutex->LockCount = 1;
					pmutex->pOwner = pCurThread;
					*ppCritMut = (LPCRIT)pmutex;
					goto waitok;
				}
				if (pmutex->pOwner == pCurThread) {
					pmutex->LockCount++;
					goto waitok;
				}
				if (!noenqueue) {
					prio = pProx->prio;
					if (!pmutex->pProxList || (prio < pmutex->pProxList->prio))
						*ppCritMut = (LPCRIT)pmutex;
					PrioEnqueue((LPEVENT)pmutex,prio,pProx);
				}
				break;
			}
			default:
				goto waitbad;
		}
		if (!noenqueue) {
			pCurThread->lpPendProxy = pProx->pThLinkNext;
			pProx->pThLinkNext = pCurThread->lpProxy;
			pCurThread->lpProxy = pProx;
			retval = 0; // doesn't matter, we're continuing
		} else {
			retval = WAIT_TIMEOUT;
			goto exitfunc;
		}
	} else if (pCurThread->dwPendTime == INFINITE) {
		*pdwContinue = 0;
		pCurThread->bWaitState = WAITSTATE_BLOCKED;
		DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
		SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
		RunList.pth = 0;
		SetReschedule();
		retval = 0; // doesn't matter, only an event can wake us
	} else {
		if (!pCurThread->dwPendTime || (CurMSec - pCurThread->dwPendStart >= pCurThread->dwPendTime)) {
			DEBUGCHK(pCurThread->lpce);
			retval = WAIT_TIMEOUT;
			goto exitfunc;
		}
		DEBUGCHK(GET_RUNSTATE(pCurThread) == RUNSTATE_RUNNING);
		DEBUGCHK(RunList.pth == pCurThread);
		*pdwContinue = 1;
		if (pth = pCurThread->pCrabPth) {
			if (pth->wCount != pCurThread->dwCrabCount) {
				pCurThread->pCrabPth = 0;
				KCALLPROFOFF(18);
				return 0; // doesn't matter, we're continuing
			}
#ifdef DEBUG
			if (pth->wCount == 0xabab && (DWORD) pth->pProc == 0xabababab) {
				DEBUGMSG(1, (TEXT("WaitOneMore : restarting...\r\n")));
				pCurThread->pCrabPth = 0;
				KCALLPROFOFF(18);
				return 0; // doesn't matter, we're continuing
			}
#endif
			pPrevNext = &pth->pNextSleepRun;
		} else
			pPrevNext = &SleepList;
		if (pCurThread->pNextSleepRun = pth2 = *pPrevNext) {
			if (pth2->dwSleepCnt <= pCurThread->dwCrabTime) {
				pCurThread->dwCrabRem = pth2->dwSleepCnt;
				pCurThread->dwCrabCount = pth2->wCount;
				pCurThread->pCrabPth = pth2;
				KCALLPROFOFF(18);
				return 0; // doesn't matter, we're continuing
			}
			pth2->pPrevSleepRun = pCurThread;
			pth2->dwSleepCnt -= pCurThread->dwCrabTime;
		}
		*pPrevNext = pCurThread;
		pCurThread->pPrevSleepRun = pth;;
		pCurThread->dwSleepCnt = pCurThread->dwCrabTime;
		if (pPrevNext == &SleepList)
			dwSleepMin = pCurThread->dwSleepCnt;
		RunList.pth = 0;
		SetReschedule();
		DEBUGCHK(!pCurThread->lpPendProxy);
		pCurThread->bWaitState = WAITSTATE_BLOCKED;
		DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
		SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
		SET_SLEEPING(pCurThread);
		*pdwContinue = 0;
		retval = WAIT_TIMEOUT;
	}
	KCALLPROFOFF(18);
	return retval;
waitbad:
	KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	retval = WAIT_FAILED;
	goto exitfunc;
waitok:
	retval = pProx->dwRetVal;
exitfunc:
	if (pCurThread->lpce) {
		pCurThread->lpce->base = pCurThread->lpProxy;
		pCurThread->lpce->size = (DWORD)pCurThread->lpPendProxy;
	}
	pCurThread->lpProxy = 0;
	pCurThread->wCount++;
	pCurThread->bWaitState = WAITSTATE_SIGNALLED;
	*pdwContinue = 0;
	KCALLPROFOFF(18);
	return retval;
}

DWORD SC_WaitForMultiple(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout) {
	DWORD loop, retval;
	DWORD dwContinue;
	LPPROXY pHeadProx, pCurrProx;
	CLEANEVENT *lpce;
	PCALLSTACK pcstk;
	LPEVENT lpe;
	LPCRIT pCritMut;
   cObjects &= ~0x80000000;
	DEBUGMSG(ZONE_ENTRY,(L"SC_WaitForMultiple entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
		cObjects,lphObjects,fWaitAll,dwTimeout));
	CELOG_WaitForMultipleObjects(cObjects, lphObjects, fWaitAll, dwTimeout);
   if (fWaitAll || !cObjects || (cObjects > MAXIMUM_WAIT_OBJECTS)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_WaitForMultiple exit: %8.8lx\r\n",WAIT_FAILED));
		return WAIT_FAILED;
	}
	if ((GetHandleType(lphObjects[0]) == HT_EVENT) && (lpe = HandleToEvent(lphObjects[0])) && lpe->pIntrProxy) {
		lpce = 0;
		DEBUGCHK(cObjects == 1);
		pHeadProx = lpe->pIntrProxy;
		if ((pcstk = pCurThread->pcstkTop) && ((DWORD)pcstk->pprcLast < 0x10000)) {
			memcpy(&pCurThread->IntrStk,pcstk,sizeof(CALLSTACK));
			pCurThread->pcstkTop = &pCurThread->IntrStk;
			if (IsValidKPtr(pcstk))
				FreeMem(pcstk,HEAP_CALLSTACK);
		}
	} else if (!(lpce = AllocMem(HEAP_CLEANEVENT)) || !(pHeadProx = (LPPROXY)AllocMem(HEAP_PROXY))) {
		if (lpce)
			FreeMem(lpce,HEAP_CLEANEVENT);
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		DEBUGMSG(ZONE_ENTRY,(L"SC_WaitForMultiple exit: %8.8lx\r\n",WAIT_FAILED));
		return WAIT_FAILED;
	}
	pHeadProx->pQNext = pHeadProx->pQPrev = 0;
	pHeadProx->wCount = pCurThread->wCount;
	pHeadProx->pObject = lphObjects[0];
	pHeadProx->bType = GetHandleType(lphObjects[0]);
	pHeadProx->pTh = pCurThread;
	pHeadProx->prio = (BYTE)GET_CPRIO(pCurThread);
	pHeadProx->dwRetVal = WAIT_OBJECT_0;
	pCurrProx = pHeadProx;
	for (loop = 1; loop < cObjects; loop++) {
		if (!(pCurrProx->pThLinkNext = (LPPROXY)AllocMem(HEAP_PROXY))) {
			for (pCurrProx = pHeadProx; pCurrProx; pCurrProx = pHeadProx) {
				pHeadProx = pCurrProx->pThLinkNext;
				FreeMem(pCurrProx,HEAP_PROXY);
			}
			if (lpce)
				FreeMem(lpce,HEAP_CLEANEVENT);
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			DEBUGMSG(ZONE_ENTRY,(L"SC_WaitForMultiple exit: %8.8lx\r\n",WAIT_FAILED));
			return WAIT_FAILED;
		}
		pCurrProx = pCurrProx->pThLinkNext;
		pCurrProx->pQNext = pCurrProx->pQPrev = 0;
		pCurrProx->wCount = pCurThread->wCount;
		pCurrProx->pObject = lphObjects[loop];
		pCurrProx->bType = GetHandleType(lphObjects[loop]);
		DEBUGCHK((pCurrProx->bType != HT_EVENT) || !HandleToEvent(pCurrProx->pObject)->pIntrProxy);
		pCurrProx->pTh = pCurThread;
		pCurrProx->prio = (BYTE)GET_CPRIO(pCurThread);
		pCurrProx->dwRetVal = WAIT_OBJECT_0 + loop;
	}
	pCurrProx->pThLinkNext = 0;
	KCall((PKFN)WaitConfig,pHeadProx,lpce,dwTimeout);
	dwContinue = 2;
	do {
		pCritMut = 0;
		retval = KCall((PKFN)WaitOneMore,&dwContinue,&pCritMut);
		if (dwContinue == 1)
			pCurThread->dwCrabTime = (pCurThread->pCrabPth ? pCurThread->dwCrabTime - pCurThread->dwCrabRem :
				pCurThread->dwPendTime - (CurMSec - pCurThread->dwPendStart));
		if (pCritMut) {
			if (!dwContinue)
				KCall((PKFN)LaterLinkCritMut,pCritMut,0);
			else
				KCall((PKFN)PostBoostMut,pCritMut);
		}
	} while (dwContinue);
	DEBUGCHK(lpce == pCurThread->lpce);
	if (lpce) {
		pCurThread->lpce = 0;
		pHeadProx = (LPPROXY)lpce->base;
		while (pHeadProx) {
			pCurrProx = pHeadProx->pThLinkNext;
			DequeueProxy(pHeadProx);
			FreeMem(pHeadProx,HEAP_PROXY);
			pHeadProx = pCurrProx;
		}
		pHeadProx = (LPPROXY)lpce->size;
		while (pHeadProx) {
			pCurrProx = pHeadProx->pThLinkNext;
			FreeMem(pHeadProx,HEAP_PROXY);
			pHeadProx = pCurrProx;
		}
		FreeMem(lpce,HEAP_CLEANEVENT);
	}
	DEBUGCHK(!pCurThread->lpce);
	DEBUGMSG(ZONE_ENTRY,(L"SC_WaitForMultiple exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD UB_WaitForMultiple(DWORD cObjects, CONST HANDLE *lphObjects, BOOL fWaitAll, DWORD dwTimeout) {
	DWORD dwRet;
	SET_USERBLOCK(pCurThread);
	dwRet = SC_WaitForMultiple(cObjects,lphObjects,fWaitAll,dwTimeout);
	CLEAR_USERBLOCK(pCurThread);
	return dwRet;
}

/* Set thread priority */

BOOL SC_ThreadSetPrio(HANDLE hTh, DWORD prio) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSetPrio entry: %8.8lx %8.8lx\r\n",hTh,prio));
   if (hTh == GetCurrentThread())
		hTh = hCurThread;
	CELOG_ThreadSetPriority(hTh, prio);
	if (bAllKMode && !HandleToThreadPerm(hTh)) {
        KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSetPrio exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	if (prio >= MAX_WIN32_PRIORITY_LEVELS) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSetPrio exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	prio += (MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS);
	if (!KCall((PKFN)SetThreadBasePrio,hTh,prio)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSetPrio exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadSetPrio exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

/* Get thread priority */

int SC_ThreadGetPrio(HANDLE hTh) {
	PTHREAD pth;
	int retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadGetPrio entry: %8.8lx\r\n",hTh));
	if (!(pth = (bAllKMode ? HandleToThreadPerm(hTh) : HandleToThread(hTh)))) {
		retval = THREAD_PRIORITY_ERROR_RETURN;
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	} else {
		retval = GET_CPRIO(pth);
		if (retval < MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS)
			retval = MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS;
		retval -= (MAX_PRIORITY_LEVELS - MAX_WIN32_PRIORITY_LEVELS);		
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadGetPrio exit: %8.8lx\r\n",retval));
	return retval;
}

int SC_CeGetThreadPriority(HANDLE hTh) {
	PTHREAD pth;
	int retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetThreadPriority entry: %8.8lx\r\n",hTh));
	if (pth = HandleToThread(hTh))
		retval = GET_CPRIO(pth);
	else {
		retval = THREAD_PRIORITY_ERROR_RETURN;
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetThreadPriority exit: %8.8lx\r\n",retval));
	return retval;
}

BOOL SC_CeSetThreadPriority(HANDLE hTh, DWORD prio) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadPriority entry: %8.8lx %8.8lx\r\n",hTh,prio));
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_CeSetThreadPriority failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadPriority exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	if (hTh == GetCurrentThread())
		hTh = hCurThread;
	CELOG_ThreadSetPriority(hTh, prio);
	if (prio >= MAX_PRIORITY_LEVELS) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadPriority exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	if (!KCall((PKFN)SetThreadBasePrio,hTh,prio)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadPriority exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadPriority exit: %8.8lx\r\n",TRUE));
	return TRUE;
}

DWORD SC_CeGetThreadQuantum(HANDLE hTh) {
	PTHREAD pth;
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetThreadQuantum entry: %8.8lx\r\n",hTh));
	if (pth = HandleToThread(hTh))
		retval = pth->dwQuantum;
	else {
		retval = (DWORD)-1;
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeGetThreadQuantum exit: %8.8lx\r\n",retval));
	return retval;
}

BOOL SC_CeSetThreadQuantum(HANDLE hTh, DWORD dwTime) {
	PTHREAD pth;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadQuantum entry: %8.8lx %8.8lx\r\n",hTh,dwTime));
   if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_CeSetThreadQuantum failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadQuantum exit: %8.8lx\r\n",FALSE));
		return FALSE;
	}
	if (pth = HandleToThread(hTh)) {
		pth->dwQuantum = dwTime;
		dwPreempt = dwTime;
		DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadQuantum exit: %8.8lx\r\n",TRUE));
      CELOG_ThreadSetQuantum(pth->hTh, dwTime);
		return TRUE;
	}
	KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CeSetThreadQuantum exit: %8.8lx\r\n",FALSE));
	return FALSE;
}

#define CSWAIT_TAKEN 0
#define CSWAIT_PEND 1
#define CSWAIT_ABORT 2

DWORD CSWaitPart1(LPCRIT *ppCritMut, LPPROXY pProx, LPCRIT pcrit) {
	BOOL retval;
	LPCRITICAL_SECTION lpcs;
	PTHREAD pOwner;
	BYTE prio;
	KCALLPROFON(19);
	DEBUGCHK(RunList.pth == pCurThread);
	DEBUGCHK(GET_RUNSTATE(pCurThread) != RUNSTATE_BLOCKED);
	if (pCurThread->bWaitState == WAITSTATE_SIGNALLED) {
		retval = CSWAIT_TAKEN;
		DEBUGCHK(RunList.pth == pCurThread);
	} else if (GET_DYING(pCurThread) && !GET_DEAD(pCurThread) && GET_USERBLOCK(pCurThread) &&
		(pCurThread->pOwnerProc == pCurProc)) {
		retval = CSWAIT_ABORT;
		DEBUGCHK(RunList.pth == pCurThread);
		goto exitcs1;
	} else {
		DEBUGCHK(RunList.pth == pCurThread);
		DEBUGCHK(pProx == pCurThread->lpPendProxy);
		DEBUGCHK(pCurThread->bWaitState == WAITSTATE_PROCESSING);
		DEBUGCHK(pCurThread == pProx->pTh);
		DEBUGCHK(pProx->pTh == pCurThread);
		DEBUGCHK(pProx->wCount == pCurThread->wCount);
		DEBUGCHK(pProx->bType == HT_CRITSEC);
		DEBUGCHK(pcrit = (LPCRIT)pProx->pObject);
		lpcs = pcrit->lpcs;
		if (!lpcs->OwnerThread || (!lpcs->needtrap && ((DWORD)lpcs->OwnerThread & 1)) ||
			!(pOwner = HandleToThread((HANDLE)((DWORD)lpcs->OwnerThread & ~1))) ||
			(!lpcs->needtrap && (pcrit->bListed != 1) && GET_BURIED(pOwner) && !((DWORD)lpcs & 0x80000000))) {
			DEBUGCHK(RunList.pth == pCurThread);
			lpcs->OwnerThread = hCurThread;
			lpcs->LockCount = 1;
			DEBUGCHK(!pcrit->pProxList);
			DEBUGCHK(pcrit->bListed != 1);
			retval = CSWAIT_TAKEN;
			DEBUGCHK(RunList.pth == pCurThread);
			goto exitcs1;
		} else if ((pOwner->hLastCrit == lpcs->hCrit) && (GET_CPRIO(pOwner) > GET_CPRIO(pCurThread))) {
			DEBUGCHK(RunList.pth == pCurThread);
			DEBUGCHK(lpcs->LockCount == 1);
			DEBUGCHK(pOwner->lpCritProxy);
			DEBUGCHK(!pOwner->lpProxy);
			DEBUGCHK(pOwner->wCount == (unsigned short)(pOwner->lpCritProxy->wCount + 1));
			lpcs->OwnerThread = hCurThread;
			DEBUGCHK(pcrit->bListed != 1);
			if (pcrit->pProxList) {
				*ppCritMut = pcrit;
				lpcs->needtrap = 1;
			} else
				DEBUGCHK(!lpcs->needtrap);
			retval = CSWAIT_TAKEN;
			DEBUGCHK(RunList.pth == pCurThread);
			goto exitcs1;
		} else {
			DEBUGCHK(RunList.pth == pCurThread);
			prio = pProx->prio;
			DEBUGCHK(!pcrit->pProxList || lpcs->needtrap);
			if (!pcrit->pProxList || (prio < pcrit->pProxList->prio))
				*ppCritMut = pcrit;
			PrioEnqueue((LPEVENT)pcrit,prio,pProx);
			lpcs->needtrap = 1;
			DEBUGCHK(RunList.pth == pCurThread);
			// get here if blocking
			DEBUGCHK(!pProx->pThLinkNext);
			pCurThread->lpPendProxy = 0;
			DEBUGCHK(!pCurThread->lpProxy);
			pProx->pThLinkNext = 0;
			pCurThread->lpProxy = pProx;
			retval = CSWAIT_PEND;
			DEBUGCHK(RunList.pth == pCurThread);
		}
	}
	DEBUGCHK(RunList.pth == pCurThread);
	DEBUGCHK(GET_RUNSTATE(pCurThread) != RUNSTATE_BLOCKED);
	KCALLPROFOFF(19);
	return retval;
exitcs1:
	DEBUGCHK(pCurThread->lpce);
	DEBUGCHK(!pCurThread->lpProxy);
	DEBUGCHK(pCurThread->lpPendProxy);
	pCurThread->lpce->base = 0;
	pCurThread->lpce->size = (DWORD)pCurThread->lpPendProxy;
	pCurThread->wCount++;
	pCurThread->bWaitState = WAITSTATE_SIGNALLED;
	KCALLPROFOFF(19);
	DEBUGCHK(RunList.pth == pCurThread);
	DEBUGCHK(GET_RUNSTATE(pCurThread) != RUNSTATE_BLOCKED);
	return retval;
}

void CSWaitPart2(void) {
	KCALLPROFON(58);
	if (pCurThread->bWaitState != WAITSTATE_SIGNALLED) {
		if (GET_DYING(pCurThread) && !GET_DEAD(pCurThread) && GET_USERBLOCK(pCurThread) && (pCurThread->pOwnerProc == pCurProc)) {
			DEBUGCHK(pCurThread->lpce);
			DEBUGCHK(pCurThread->lpProxy);
			pCurThread->lpce->base = pCurThread->lpProxy;
			DEBUGCHK(!pCurThread->lpPendProxy);
			pCurThread->lpce->size = 0;
			pCurThread->lpProxy = 0;
			pCurThread->wCount++;
			pCurThread->bWaitState = WAITSTATE_SIGNALLED;
		} else {
			LPCRIT pCrit;
			PTHREAD pth;
			pCrit = (LPCRIT)pCurThread->lpProxy->pObject;
			if (!pCrit->bListed) {
				pth = HandleToThread((HANDLE)((DWORD)pCrit->lpcs->OwnerThread & ~1));
				DEBUGCHK(pth);
				pth->hLastCrit = 0;
				LinkCritMut(pCrit,pth,1);
			}
			DEBUGCHK(pCrit->lpcs->needtrap);
			pCurThread->bWaitState = WAITSTATE_BLOCKED;
			DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
			SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
			RunList.pth = 0;
			SetReschedule();
		}
	}
	KCALLPROFOFF(58);
}

/* When a thread tries to get a Critical Section */

void SC_TakeCritSec(LPCRITICAL_SECTION lpcs) {
	BOOL bRetry;
	LPPROXY pProx;
	LPCRIT pCritMut;
	LPCLEANEVENT lpce;
	bRetry = 1;
	DEBUGCHK(IsValidKPtr(lpcs->hCrit));
	if (!IsValidKPtr(lpcs->hCrit))
		return;
	do {
		pProx = AllocMem(HEAP_PROXY);
		lpce = AllocMem(HEAP_CLEANEVENT);
		DEBUGCHK(pProx && lpce && lpcs->hCrit);
		pProx->pQNext = pProx->pQPrev = pProx->pQDown = 0;
		pProx->wCount = pCurThread->wCount;
		pProx->pObject = (LPBYTE)lpcs->hCrit;
		pProx->bType = HT_CRITSEC;
		pProx->pTh = pCurThread;
		pProx->prio = (BYTE)GET_CPRIO(pCurThread);
		pProx->dwRetVal = WAIT_OBJECT_0;
		pProx->pThLinkNext = 0;
#ifdef DEBUG
		DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
		pCurThread->wInfo |= (1 << DEBUG_LOOPCNT_SHIFT);
#endif
		KCall((PKFN)WaitConfig,pProx,lpce,INFINITE);
		pCritMut = 0;
		switch (KCall((PKFN)CSWaitPart1,&pCritMut,pProx,lpcs->hCrit)) {
			case CSWAIT_TAKEN: 
				DEBUGCHK(!pCritMut || (pCritMut == (LPCRIT)lpcs->hCrit));
				if (pCritMut && (hCurThread == lpcs->OwnerThread))
					KCall((PKFN)LaterLinkCritMut,pCritMut,1);
				break;
			case CSWAIT_PEND:
				DEBUGCHK(!pCritMut || (pCritMut == (LPCRIT)lpcs->hCrit));
				if (pCritMut) {
					KCall((PKFN)PostBoostCrit1,pCritMut);
					KCall((PKFN)PostBoostCrit2,pCritMut);
				}
	            CELOG_CSEnter(lpcs->hCrit, lpcs->OwnerThread);
#ifdef DEBUG
				pCurThread->wInfo &= ~(1 << DEBUG_LOOPCNT_SHIFT);
#endif	
				KCall((PKFN)CSWaitPart2);
				break;
			case CSWAIT_ABORT:
				bRetry = 0;
				break;
		}
#ifdef DEBUG
		pCurThread->wInfo &= ~(1 << DEBUG_LOOPCNT_SHIFT);
#endif	
		DEBUGCHK(!pCurThread->lpProxy);
		DEBUGCHK(pCurThread->lpce == lpce);
		DEBUGCHK(((lpce->base == pProx) && !lpce->size) || (!lpce->base && (lpce->size == (DWORD)pProx)));
		if (pProx->pQDown)
			if (KCall((PKFN)DequeuePrioProxy,pProx))
				KCall((PKFN)DoReprioCrit,lpcs->hCrit);
		DEBUGCHK(!pProx->pQDown);
		pCurThread->lpce = 0;
		FreeMem(pProx,HEAP_PROXY);
		FreeMem(lpce,HEAP_CLEANEVENT);
	} while ((lpcs->OwnerThread != hCurThread) && bRetry);
	if (lpcs->OwnerThread == hCurThread)
		KCall((PKFN)CritFinalBoost,lpcs);
}

void UB_TakeCritSec(LPCRITICAL_SECTION lpcs) {
	SET_USERBLOCK(pCurThread);
	SC_TakeCritSec(lpcs);
	CLEAR_USERBLOCK(pCurThread);
}

BOOL PreLeaveCrit(LPCRITICAL_SECTION lpcs) {
	BOOL bRet;
	KCALLPROFON(53);
	if (((DWORD)lpcs->OwnerThread != ((DWORD)hCurThread | 1)))
		bRet = FALSE;
	else {
		lpcs->OwnerThread = hCurThread;
		bRet = TRUE;
		SET_NOPRIOCALC(pCurThread);
		UnlinkCritMut((LPCRIT)lpcs->hCrit,pCurThread);
		((LPCRIT)lpcs->hCrit)->bListed = 2;
	}
	KCALLPROFOFF(53);
	return bRet;
}

BOOL LeaveCrit(LPCRITICAL_SECTION lpcs, HANDLE *phTh) {
	LPCRIT pcrit;
	PTHREAD pNewOwner;
	LPPROXY pprox, pDown, pNext;
	WORD prio;
	BOOL bRet;
	KCALLPROFON(20);
    bRet = FALSE;
	DEBUGCHK(lpcs->OwnerThread == hCurThread);
	pcrit = (LPCRIT)(lpcs->hCrit);
	DEBUGCHK(lpcs->LockCount == 1);
	if (!(pprox = pcrit->pProxList)) {
		DEBUGCHK(pcrit->bListed != 1);
		lpcs->OwnerThread = 0;
		lpcs->needtrap = 0;
	} else {
		// dequeue proxy
		prio = pprox->prio/PRIORITY_LEVELS_HASHSCALE;
		pDown = pprox->pQDown;
		pNext = pprox->pQNext;
		DEBUGCHK(pcrit->pProxHash[prio] == pprox);
		pcrit->pProxHash[prio] = ((pDown != pprox) ? pDown :
			(pNext && (pNext->prio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
		if (pDown == pprox) {
			if (pcrit->pProxList = pNext)
				pNext->pQPrev = 0;
		} else {
			pDown->pQUp = pprox->pQUp;
			pprox->pQUp->pQDown = pcrit->pProxList = pDown;
			DEBUGCHK(!pDown->pQPrev);
			if (pNext) {
				pNext->pQPrev = pDown;
				pDown->pQNext = pNext;
			}
		}
		pprox->pQDown = 0;
		// wake up new owner
		pNewOwner = pprox->pTh;
		DEBUGCHK(pNewOwner);
		if (pNewOwner->wCount != pprox->wCount)
			bRet = 1;
		else {
			DEBUGCHK(!GET_SLEEPING(pNewOwner));
			DEBUGCHK(pNewOwner->lpce);
			DEBUGCHK(pNewOwner->lpProxy == pprox);
			DEBUGCHK(!pNewOwner->lpPendProxy);
			pNewOwner->lpce->base = pprox;
			pNewOwner->lpce->size = 0;
			pNewOwner->wCount++;
			pNewOwner->lpProxy = 0;
			pNewOwner->lpCritProxy = pprox; // since we can steal it back, don't lose proxy
			if (pNewOwner->bWaitState == WAITSTATE_BLOCKED) {
				DEBUGCHK(GET_RUNSTATE(pNewOwner) != RUNSTATE_RUNNABLE);
				DEBUGCHK(GET_RUNSTATE(pNewOwner) != RUNSTATE_RUNNING);
				DEBUGCHK(GET_RUNSTATE(pNewOwner) != RUNSTATE_NEEDSRUN);
				SET_RUNSTATE(pNewOwner,RUNSTATE_NEEDSRUN);
				*phTh = pNewOwner->hTh;
			} else {
				DEBUGCHK(GET_RUNSTATE(pNewOwner) != RUNSTATE_BLOCKED);
				pNewOwner->bWaitState = WAITSTATE_SIGNALLED;
			}
			if (!pcrit->pProxList)
				lpcs->needtrap = 0;
			else
				DEBUGCHK(lpcs->needtrap);
			lpcs->LockCount = 1;
			lpcs->OwnerThread = pNewOwner->hTh;
			DEBUGCHK(!pNewOwner->hLastCrit);
			pNewOwner->hLastCrit = lpcs->hCrit;
		   CELOG_CSLeave(lpcs->hCrit, pNewOwner->hTh);
      }
	}
	if (!bRet) {
		DEBUGCHK(pcrit->bListed != 1);
		pcrit->bListed = 0;
	}
	KCALLPROFOFF(20);
	return bRet;
}

/* When a thread releases a Critical Section */

void SC_LeaveCritSec(LPCRITICAL_SECTION lpcs) {
	HANDLE hth;
	if (KCall((PKFN)PreLeaveCrit,lpcs)) {
		hth = 0;
		while (KCall((PKFN)LeaveCrit,lpcs,&hth))
			;
		if (hth)
			KCall((PKFN)MakeRunIfNeeded,hth);
		KCall((PKFN)PostUnlinkCritMut);
	}
}

void PuntCritSec(CRITICAL_SECTION *pcs) {
	if (pcs->OwnerThread == hCurThread) {
		ERRORMSG(1,(L"Abandoning CS %8.8lx in PuntCritSec\r\n",pcs));
		pcs->LockCount = 1;
		LeaveCriticalSection(pcs);
	}
}

void SurrenderCritSecs(void) {
    PuntCritSec(&rtccs);
    PuntCritSec(&CompCS);
    PuntCritSec(&PagerCS);
    PuntCritSec(&PhysCS);
    PuntCritSec(&VAcs);
    PuntCritSec(&ppfscs);
    PuntCritSec(&ODScs);
    PuntCritSec(&RFBcs);
    PuntCritSec(&MapCS);
    PuntCritSec(&MapNameCS);
    PuntCritSec(&WriterCS);
    PuntCritSec(&NameCS);
	PuntCritSec(&ModListcs);
	PuntCritSec(&LLcs);
	PuntCritSec(&DbgApiCS);
	PuntCritSec(&csDbg);
}

BOOL LeaveMutex(LPMUTEX lpm, HANDLE *phTh) {
	BOOL bRet;
	PTHREAD pth;
	LPPROXY pprox, pDown, pNext;
	WORD prio;
	KCALLPROFON(21);
	bRet = 0;
	DEBUGCHK(pCurThread == lpm->pOwner);
	if (!(pprox = lpm->pProxList)) {
		lpm->pOwner = 0;
	} else {
		// dequeue proxy
		prio = pprox->prio/PRIORITY_LEVELS_HASHSCALE;
		pDown = pprox->pQDown;
		pNext = pprox->pQNext;
		DEBUGCHK(lpm->pProxHash[prio] == pprox);
		lpm->pProxHash[prio] = ((pDown != pprox) ? pDown :
			(pNext && (pNext->prio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
		if (pDown == pprox) {
			if (lpm->pProxList = pNext)
				pNext->pQPrev = 0;
		} else {
			pDown->pQUp = pprox->pQUp;
			pprox->pQUp->pQDown = lpm->pProxList = pDown;
			DEBUGCHK(!pDown->pQPrev);
			if (pNext) {
				pNext->pQPrev = pDown;
				pDown->pQNext = pNext;
			}
		}
		pprox->pQDown = 0;
		// wake up new owner
		pth = pprox->pTh;
		DEBUGCHK(pth);
		if (pth->wCount != pprox->wCount)
			bRet = 1;
		else {
			DEBUGCHK(pth->lpProxy);
			DEBUGCHK(pth->lpce);
			pth->lpce->base = pth->lpProxy;
			pth->lpce->size = (DWORD)pth->lpPendProxy;
			pth->wCount++;
			pth->lpProxy = 0;
			if (pth->bWaitState == WAITSTATE_BLOCKED) {
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
				DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
				pth->retValue = pprox->dwRetVal;
				SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
				*phTh = pth->hTh;
			} else {
				DEBUGCHK(!GET_SLEEPING(pth));
				pth->dwPendReturn = pprox->dwRetVal;
				pth->bWaitState = WAITSTATE_SIGNALLED;
			}
			lpm->LockCount = 1;
			lpm->pOwner = pth;
			LinkCritMut((LPCRIT)lpm,pth,0);
		}
	}
	KCALLPROFOFF(21);
	return bRet;
}

void DoLeaveMutex(LPMUTEX lpm) {
	HANDLE hth;
	KCall((PKFN)PreUnlinkCritMut,lpm);
	hth = 0;
	while (KCall((PKFN)LeaveMutex,lpm,&hth))
		;
	if (hth)
		KCall((PKFN)MakeRunIfNeeded,hth);
	KCall((PKFN)PostUnlinkCritMut);
}

BOOL SC_ReleaseMutex(HANDLE hMutex) {
	LPMUTEX lpm;
	CELOG_MutexRelease(hMutex);
   if (!(lpm = (bAllKMode ? HandleToMutexPerm(hMutex) : HandleToMutex(hMutex)))) {
        KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (lpm->pOwner != pCurThread) {
		SetLastError(ERROR_NOT_OWNER);
		return FALSE;
	}
	if (lpm->LockCount != 1)
		lpm->LockCount--;
	else
		DoLeaveMutex(lpm);
	return TRUE;
}

LONG SemAdd(LPSEMAPHORE lpsem, LONG lReleaseCount) {
	LONG prev;
	KCALLPROFON(3);
	if ((lReleaseCount <= 0) ||
		(lpsem->lCount + lpsem->lPending + lReleaseCount > lpsem->lMaxCount) ||
		(lpsem->lCount + lpsem->lPending + lReleaseCount < lpsem->lCount + lpsem->lPending)) {
		KCALLPROFOFF(3);
		return -1;
	}
	prev = lpsem->lCount + lpsem->lPending;
	lpsem->lPending += lReleaseCount;
	KCALLPROFOFF(3);
	return prev;
}

BOOL SemPop(LPSEMAPHORE lpsem, LPLONG pRemain, HANDLE *phTh) {
	PTHREAD pth;
	LPPROXY pprox, pDown, pNext;
	WORD prio;
	BOOL bRet;
	KCALLPROFON(37);
	bRet = 0;
	if (*pRemain) {
		DEBUGCHK(*pRemain <= lpsem->lPending);
		if (!(pprox = lpsem->pProxList)) {
			lpsem->lCount += *pRemain;
			lpsem->lPending -= *pRemain;
		} else {
			bRet = 1;
			// dequeue proxy
			prio = pprox->prio/PRIORITY_LEVELS_HASHSCALE;
			pDown = pprox->pQDown;
			pNext = pprox->pQNext;
			DEBUGCHK(lpsem->pProxHash[prio] == pprox);
			lpsem->pProxHash[prio] = ((pDown != pprox) ? pDown :
				(pNext && (pNext->prio/PRIORITY_LEVELS_HASHSCALE == prio)) ? pNext : 0);
			if (pDown == pprox) {
				if (lpsem->pProxList = pNext)
					pNext->pQPrev = 0;
			} else {
				pDown->pQUp = pprox->pQUp;
				pprox->pQUp->pQDown = lpsem->pProxList = pDown;
				DEBUGCHK(!pDown->pQPrev);
				if (pNext) {
					pNext->pQPrev = pDown;
					pDown->pQNext = pNext;
				}
			}
			pprox->pQDown = 0;
			// wake up thread		
			pth = pprox->pTh;
			DEBUGCHK(pth);
			if (pth->wCount == pprox->wCount) {
				DEBUGCHK(pth->lpProxy);
				if (pth->lpce) {
					pth->lpce->base = pth->lpProxy;
					pth->lpce->size = (DWORD)pth->lpPendProxy;
				}
				pth->wCount++;
				pth->lpProxy = 0;
				if (pth->bWaitState == WAITSTATE_BLOCKED) {
					DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNABLE);
					DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_RUNNING);
					DEBUGCHK(GET_RUNSTATE(pth) != RUNSTATE_NEEDSRUN);
					pth->retValue = pprox->dwRetVal;
					SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
					*phTh = pth->hTh;
				} else {
					DEBUGCHK(!GET_SLEEPING(pth));
					pth->dwPendReturn = pprox->dwRetVal;
					pth->bWaitState = WAITSTATE_SIGNALLED;
				}
				lpsem->lPending--;
				(*pRemain)--;
			}
		}
	}
	KCALLPROFOFF(37);
	return bRet;
}

BOOL SC_ReleaseSemaphore(HANDLE hSem, LONG lReleaseCount, LPLONG lpPreviousCount) {
	LONG prev;
	HANDLE hth;
	LPSEMAPHORE pSem;
	BOOL bRet = FALSE;
	if (!(pSem = HandleToSem(hSem)) || ((prev = KCall((PKFN)SemAdd,pSem,lReleaseCount)) == -1))
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
	else {
		if (lpPreviousCount)
			*lpPreviousCount = prev;
		hth = 0;
		while (KCall((PKFN)SemPop,pSem,&lReleaseCount,&hth)) {
			if (hth) {
				KCall((PKFN)MakeRunIfNeeded,hth);
				hth = 0;
			}
		}
		bRet = TRUE;
	}
   CELOG_SemaphoreRelease(hSem, lReleaseCount, prev);
	return bRet;
}

PHDATA ZapHandle(HANDLE h);
extern PHDATA PhdPrevReturn, PhdNext;
extern LPBYTE pFreeKStacks;

void KillSpecialThread(void) {
	PHDATA phd;
	KCALLPROFON(36);
	DEBUGCHK(RunList.pth == pCurThread);
	RunList.pth = 0;
	SetReschedule();
	DEBUGCHK(pCurThread->pNextInProc);
	if (!pCurThread->pPrevInProc) {
		DEBUGCHK(pCurProc->pTh == pCurThread);
		pCurProc->pTh = pCurThread->pNextInProc;
		pCurProc->pTh->pPrevInProc = 0;
	} else {
		pCurThread->pPrevInProc->pNextInProc = pCurThread->pNextInProc;
		pCurThread->pNextInProc->pPrevInProc = pCurThread->pPrevInProc;
	}
	DEBUGCHK(!pCurThread->lpce);
	DEBUGCHK(!pCurThread->lpProxy);
	DEBUGCHK(!pCurThread->pProxList);
	DEBUGCHK(!pCurThread->pOwnedList);
	DEBUGCHK(!pCurThread->pcstkTop);
	DEBUGCHK(!pCurThread->pSwapStack);
	DEBUGCHK(IsValidKPtr(pCurThread->dwStackBase));
	*(LPBYTE *)(pCurThread->dwStackBase) = pFreeKStacks;
	pFreeKStacks = (LPBYTE)(pCurThread->dwStackBase);
	phd = (PHDATA)(((ulong)hCurThread & HANDLE_ADDRESS_MASK) + KData.handleBase);
    DEBUGCHK(phd->ref.count == 2);
    DEBUGCHK(((DWORD)phd->hValue & 0x1fffffff) == (((ulong)phd & HANDLE_ADDRESS_MASK) | 2));
 	// not really needed, just helpful to avoid stale handle issues
	// phd->hValue = (HANDLE)((DWORD)phd->hValue+0x20000000);
   	phd->linkage.fwd->back = phd->linkage.back;
   	phd->linkage.back->fwd = phd->linkage.fwd;
    PhdPrevReturn = 0; 
   	PhdNext = 0;
	FreeMem(phd, HEAP_HDATA);
	FreeMem(pCurThread,HEAP_THREAD);
	KCALLPROFOFF(36);
}

void FinishRemoveThread(RTs *pRT) {
	RTs RTinfo;
	DWORD retval;
    LPTHREADTIME p1,p2;
	HANDLE hWake;
	SETCURKEY((DWORD)-1);
	DEBUGCHK((DWORD)&RTinfo & 0x80000000);
	memcpy(&RTinfo,pRT,sizeof(RTs));
	if (RTinfo.dwBase) {
		retval = VirtualFree((LPVOID)RTinfo.dwBase,RTinfo.dwLen,MEM_DECOMMIT);
		DEBUGCHK(retval);
		retval = VirtualFree((LPVOID)RTinfo.dwBase,0,MEM_RELEASE);
		DEBUGCHK(retval);
	}
	if (RTinfo.hThread) {
		if (DecRef(RTinfo.hThread,RTinfo.pThread->pOwnerProc,0)) {
			CELOG_ThreadDelete(RTinfo.hThread);
			FreeHandle(RTinfo.hThread);
         EnterCriticalSection(&NameCS);
			p1 = NULL;
			p2 = TTList;
			while (p2 && p2->hTh != RTinfo.hThread) {
				p1 = p2;
				p2 = p2->pnext;
			}
			if (p2) {
				if (p1)
					p1->pnext = p2->pnext;
				else
					TTList = p2->pnext;
				FreeMem(p2,HEAP_THREADTIME);
			}
			LeaveCriticalSection(&NameCS);
		}
	}
	if (RTinfo.pThread)
		FreeMem(RTinfo.pThread,HEAP_THREAD);
	if (RTinfo.pThrdDbg)
		FreeMem(RTinfo.pThrdDbg,HEAP_THREADDBG);
	if (RTinfo.pProc) {
		EnterCriticalSection(&PageOutCS);
		DEBUGCHK(RTinfo.pProc != pCurProc);
      CELOG_ProcessTerminate(RTinfo.pProc->hProc);
      if (DecRef(RTinfo.pProc->hProc,RTinfo.pProc,TRUE)) {
         CELOG_ProcessDelete(RTinfo.pProc->hProc);
         FreeHandle(RTinfo.pProc->hProc);
      } else {
         SetObjectPtr(RTinfo.pProc->hProc,0);
      }
		while (RTinfo.pProc->pProxList) {
			hWake = 0;
			KCall((PKFN)WakeOneThreadFlat,RTinfo.pProc,&hWake);
			if (hWake)
				KCall((PKFN)MakeRunIfNeeded,hWake);
		}
		if (RTinfo.pProc->pStdNames[0])
			FreeName(RTinfo.pProc->pStdNames[0]);
		if (RTinfo.pProc->pStdNames[1])
			FreeName(RTinfo.pProc->pStdNames[1]);
		if (RTinfo.pProc->pStdNames[2])
			FreeName(RTinfo.pProc->pStdNames[2]);
      if (pLogProcessDelete)
			pLogProcessDelete((DWORD)RTinfo.pProc);
    	retval = RTinfo.pProc->dwVMBase;
		RTinfo.pProc->dwVMBase = 0;
		DeleteSection((LPVOID)retval);
		LeaveCriticalSection(&PageOutCS);
	} else {
		DEBUGCHK(*RTinfo.pdwDying);
		InterlockedDecrement(RTinfo.pdwDying);
	}
	KCall((PKFN)KillSpecialThread);
	DEBUGCHK(0);
}

void BlockWithHelper(FARPROC pFunc, FARPROC pHelpFunc, LPVOID param) {
	CALLBACKINFO cbi;
	HANDLE hTh;
	if (pCurProc == &ProcArray[0]) {
		hTh = DoCreateThread(pCurThread->pSwapStack,HELPER_STACK_SIZE,pHelpFunc,param,CREATE_SUSPENDED|0x80000000,0,TH_KMODE,GET_CPRIO(pCurThread));
	} else {
		cbi.hProc = ProcArray[0].hProc;
		cbi.pfn = (FARPROC)DoCreateThread;
		cbi.pvArg0 = pCurThread->pSwapStack;
		hTh = (HANDLE)PerformCallBack(&cbi,HELPER_STACK_SIZE,pHelpFunc,param,CREATE_SUSPENDED|0x80000000,0,TH_KMODE,GET_CPRIO(pCurThread));
	}
	DEBUGCHK(hTh);
	*(PTHREAD *)param = HandleToThread(hTh);
	KCall((PKFN)pFunc,param);
	DEBUGCHK(0);
}

void BlockWithHelperAlloc(FARPROC pFunc, FARPROC pHelpFunc, LPVOID param) {
	CALLBACKINFO cbi;
	HANDLE hTh;
	while (!(cbi.pvArg0 = (LPBYTE)GetHelperStack()))
		Sleep(500); // nothing we can do, so we'll just poll
	if (pCurProc == &ProcArray[0]) {
		hTh = DoCreateThread(cbi.pvArg0,HELPER_STACK_SIZE,pHelpFunc,param,CREATE_SUSPENDED|0x80000000,0,TH_KMODE,GET_CPRIO(pCurThread));
	} else {
		cbi.hProc = ProcArray[0].hProc;
		cbi.pfn = (FARPROC)DoCreateThread;
		hTh = (HANDLE)PerformCallBack(&cbi,HELPER_STACK_SIZE,pHelpFunc,param,CREATE_SUSPENDED|0x80000000,0,TH_KMODE,GET_CPRIO(pCurThread));
	}
	DEBUGCHK(hTh);
	*(PTHREAD *)param = HandleToThread(hTh);
	KCall((PKFN)pFunc,param);
	DEBUGCHK(0);
}

LPCRIT DoLeaveSomething() {
    KCALLPROFON(67);
    if (pCurThread->pOwnedList)
        return pCurThread->pOwnedList;
    SET_BURIED(pCurThread);
    KCALLPROFOFF(67);
    return 0;
}

/* Terminate current thread routine */

void SC_NKTerminateThread(DWORD dwExitCode) {
	int loop;
	LPCRIT pCrit;
	PCALLSTACK pcstk, pcnextstk;
	LPCLEANEVENT lpce;
	PTHREAD pdbg;
	HANDLE hClose, hWake;
	LPCRITICAL_SECTION lpcs;
	LPTHREADTIME lptt;
	LPPROXY pprox, nextprox;
	RTs RT;
	DEBUGMSG(ZONE_ENTRY,(L"SC_NKTerminateThread entry: %8.8lx\r\n",dwExitCode));
	SETCURKEY(0xffffffff);
	if (GET_DYING(pCurThread))
		dwExitCode = GetUserInfo(hCurThread);
	SET_DEAD(pCurThread);
	DEBUGCHK(!GET_BURIED(pCurThread));
	SetUserInfo(hCurThread,dwExitCode);
	if (!pCurThread->pNextInProc) {
		SetUserInfo(pCurThread->pOwnerProc->hProc,dwExitCode);
		CloseMappedFileHandles();
	}
#if defined(x86) || defined(SH4) || defined(MIPS_HAS_FPU)
    if (pCurThread == g_CurFPUOwner)
        g_CurFPUOwner = NULL;
#endif
#if defined(SH3)
    if (pCurThread == g_CurDSPOwner)
        g_CurDSPOwner = NULL;
#endif
	if (pCurThread == pOOMThread)
		pOOMThread = 0;
	EnterCriticalSection(&DbgApiCS);
	if (pCurThread->pThrdDbg && pCurThread->pThrdDbg->hFirstThrd) {
		for (loop = 0; loop < MAX_PROCESSES; loop++)
			if (ProcArray[loop].hDbgrThrd == hCurThread)
				ProcArray[loop].hDbgrThrd = 0;
		while (pCurThread->pThrdDbg->hFirstThrd) {
			pdbg = HandleToThread(pCurThread->pThrdDbg->hFirstThrd);
			pCurThread->pThrdDbg->hFirstThrd = pdbg->pThrdDbg->hNextThrd;
			hClose = pdbg->pThrdDbg->hEvent;
			pdbg->pThrdDbg->hEvent = 0;
			SetHandleOwner(hClose,hCurProc);
			CloseHandle(hClose);
			hClose = pdbg->pThrdDbg->hBlockEvent;
			pdbg->pThrdDbg->hBlockEvent = 0;
			SetHandleOwner(hClose,hCurProc);
			SetEvent(hClose);
			CloseHandle(hClose);
		}
	}
	LeaveCriticalSection(&DbgApiCS);
	if (pCurThread->lpce) {
		lpce = pCurThread->lpce;
		pCurThread->lpce = 0;
		pprox = (LPPROXY)lpce->base;
		while (pprox) {
			nextprox = pprox->pThLinkNext;
			DequeueProxy(pprox);
			FreeMem(pprox,HEAP_PROXY);
			pprox = nextprox;
		}
		pprox = (LPPROXY)lpce->size;
		while (pprox) {
			nextprox = pprox->pThLinkNext;
			FreeMem(pprox,HEAP_PROXY);
			pprox = nextprox;
		}
		FreeMem(lpce,HEAP_CLEANEVENT);
	}
	DEBUGCHK(!pCurThread->lpProxy);
	while (pCurThread->pProxList) {
		hWake = 0;
		KCall((PKFN)WakeOneThreadFlat,pCurThread,&hWake);
		if (hWake)
			KCall((PKFN)MakeRunIfNeeded,hWake);
	}
	for (pcstk = (PCALLSTACK)((DWORD)pCurThread->pcstkTop & ~1); pcstk; pcstk = pcnextstk) {
		pcnextstk = (PCALLSTACK)((DWORD)pcstk->pcstkNext & ~1);
		// Don't free on-stack ones
		if (IsValidKPtr(pcstk))
			FreeMem(pcstk,HEAP_CALLSTACK);
	}
	pCurThread->pcstkTop = 0;
	while (pCrit = (LPCRIT)KCall((PKFN)DoLeaveSomething)) {
		if (ISMUTEX(pCrit))
			DoLeaveMutex((LPMUTEX)pCrit);
		else {
			lpcs = pCrit->lpcs;
			lpcs->LockCount = 1;
			LeaveCriticalSection(lpcs);
		}
	}
	CELOG_ThreadTerminate(pCurThread->hTh);
	if (pLogThreadDelete)
		pLogThreadDelete((DWORD)pCurThread,(DWORD)pCurThread->pOwnerProc);
	if (*(__int64 *)&pCurThread->ftCreate && (lptt = AllocMem(HEAP_THREADTIME))) {
		lptt->hTh = hCurThread;
		lptt->CreationTime = pCurThread->ftCreate;
		GCFT(&lptt->ExitTime);
		*(__int64 *)&lptt->KernelTime = KCall((PKFN)GetCurThreadKTime);
		*(__int64 *)&lptt->KernelTime *= 10000;
		*(__int64 *)&lptt->UserTime = pCurThread->dwUserTime;
		*(__int64 *)&lptt->UserTime *= 10000;
		EnterCriticalSection(&NameCS);
		lptt->pnext = TTList;
		TTList = lptt;
		LeaveCriticalSection(&NameCS);
	}
	if (pCurThread->pNextInProc) {
		RT.dwBase = pCurThread->dwStackBase;
		RT.dwLen = pCurThread->pOwnerProc->e32.e32_stackmax;
		RT.pProc = 0;
	} else {
		if ((pCurThread->dwStackBase & (SECTION_MASK << VA_SECTION)) == ProcArray[0].dwVMBase) {
			RT.dwBase = pCurThread->dwStackBase;
			RT.dwLen = CNP_STACK_SIZE;
		} else {
			RT.dwBase = 0;
			RT.dwLen = 0;
		}
		RT.pProc = pCurThread->pOwnerProc;
		FreeAllProcLibraries(pCurThread->pOwnerProc);
	}
	RT.pThrdDbg = 0;
	RT.hThread = 0;
	RT.pThread = pCurThread;
	RT.pdwDying = &pCurThread->pOwnerProc->dwDyingThreads;
   InterlockedIncrement(RT.pdwDying);
	DEBUGCHK(*RT.pdwDying);
	BlockWithHelper((FARPROC)RemoveThread,(FARPROC)FinishRemoveThread,(LPVOID)&RT);
	DEBUGCHK(0);
}

#ifdef KCALL_PROFILE

BOOL ProfileThreadSetContext(HANDLE hTh, const CONTEXT *lpContext) {
	BOOL retval;
	KCALLPROFON(23);
	retval = DoThreadSetContext(hTh,lpContext);
	KCALLPROFOFF(23);
	return retval;
}

BOOL ProfileThreadGetContext(HANDLE hTh, LPCONTEXT lpContext) {
	BOOL retval;
	KCALLPROFON(22);
	retval = DoThreadGetContext(hTh,lpContext);
	KCALLPROFOFF(22);
	return retval;
}

#define DoThreadSetContext ProfileThreadSetContext
#define DoThreadGetContext ProfileThreadGetContext

#endif

BOOL SC_ThreadSetContext(HANDLE hTh, const CONTEXT *lpContext) {
	BOOL b;
	if (!LockPages((LPVOID)lpContext,sizeof(CONTEXT),0,LOCKFLAG_WRITE)) {
		return FALSE;
	}
	b = (BOOL)KCall((PKFN)DoThreadSetContext,hTh,lpContext);
	UnlockPages((LPVOID)lpContext,sizeof(CONTEXT));
	return b;
}

BOOL SC_ThreadGetContext(HANDLE hTh, LPCONTEXT lpContext) {
	BOOL b;
	if (!LockPages((LPVOID)lpContext,sizeof(CONTEXT),0,LOCKFLAG_WRITE)) {
		return FALSE;
	}
	b = (BOOL)KCall((PKFN)DoThreadGetContext,hTh,lpContext);
	UnlockPages((LPVOID)lpContext,sizeof(CONTEXT));
	return b;
}

#undef ExitProcess
#undef CreateThread

const char Hexmap[17] = "0123456789abcdef";

BOOL UserDbgTrap(EXCEPTION_RECORD *er, PCONTEXT pctx, BOOL bSecond) {
	if ((er->ExceptionCode != STATUS_BREAKPOINT) && pDebugger && (!pCurThread->pThrdDbg || !pCurThread->pThrdDbg->hEvent)) {
		PROCESS_INFORMATION pi;
		DWORD ExitCode;
		WCHAR cmdline[11];
		cmdline[0] = '0';
		cmdline[1] = 'x';
		cmdline[2] = (WCHAR)Hexmap[((((DWORD)hCurProc)>>28)&0xf)];
		cmdline[3] = (WCHAR)Hexmap[((((DWORD)hCurProc)>>24)&0xf)];
		cmdline[4] = (WCHAR)Hexmap[((((DWORD)hCurProc)>>20)&0xf)];
		cmdline[5] = (WCHAR)Hexmap[((((DWORD)hCurProc)>>16)&0xf)];
		cmdline[6] = (WCHAR)Hexmap[((((DWORD)hCurProc)>>12)&0xf)];
		cmdline[7] = (WCHAR)Hexmap[((((DWORD)hCurProc)>> 8)&0xf)];
		cmdline[8] = (WCHAR)Hexmap[((((DWORD)hCurProc)>> 4)&0xf)];
		cmdline[9] = (WCHAR)Hexmap[((((DWORD)hCurProc)>> 0)&0xf)];
		cmdline[10] = 0;
		if (CreateProcess(pDebugger->name,cmdline,0,0,0,0,0,0,0,&pi)) {
			SET_NEEDDBG(pCurThread);
			CloseHandle(pi.hThread);
			while (SC_ProcGetCode(pi.hProcess,&ExitCode) && (ExitCode == STILL_ACTIVE) && (!pCurThread->pThrdDbg || !pCurThread->pThrdDbg->hEvent))
				Sleep(250);
			CloseHandle(pi.hProcess);
			CLEAR_NEEDDBG(pCurThread);
		}
	}
	if (pCurThread->pThrdDbg && ProcStarted(pCurProc) && pCurThread->pThrdDbg->hEvent) {
		pCurThread->pThrdDbg->psavedctx = pctx;
		pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
		pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
		pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
		pCurThread->pThrdDbg->dbginfo.u.Exception.dwFirstChance = !bSecond;
		pCurThread->pThrdDbg->dbginfo.u.Exception.ExceptionRecord = *er;
		SetEvent(pCurThread->pThrdDbg->hEvent);
		SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
		pCurThread->pThrdDbg->psavedctx = 0;
		if (pCurThread->pThrdDbg->dbginfo.dwDebugEventCode == DBG_CONTINUE)
			return TRUE;
	}
	return FALSE;
}

void DoDebugAttach(PPROCESS pproc) {
	PTHREAD pTrav, pth;
	PMODULE pMod;
	DEBUGCHK(pproc);
	pth = pproc->pMainTh;
	DEBUGCHK(pth);
	pth->pThrdDbg->dbginfo.dwProcessId = (DWORD)pproc->hProc;
	pth->pThrdDbg->dbginfo.dwThreadId = (DWORD)pth->hTh;
	pth->pThrdDbg->dbginfo.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.hFile = NULL;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.hProcess = pproc->hProc;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.hThread = pth->hTh;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.lpStartAddress = (LPVOID)0;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.lpBaseOfImage = (LPVOID)pproc->dwVMBase;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.nDebugInfoSize = 0;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.lpThreadLocalBase = pth->tlsPtr;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.lpImageName = pproc->lpszProcName;
	pth->pThrdDbg->dbginfo.u.CreateProcessInfo.fUnicode = 1;
	SetEvent(pth->pThrdDbg->hEvent);
	SC_WaitForMultiple(1,&pth->pThrdDbg->hBlockEvent,FALSE,INFINITE);
	for (pTrav = pproc->pTh; pTrav->pNextInProc; pTrav = pTrav->pNextInProc) {
		pTrav->pThrdDbg->dbginfo.dwProcessId = (DWORD)pproc->hProc;
		pTrav->pThrdDbg->dbginfo.dwThreadId = (DWORD)pTrav->hTh;
		pTrav->pThrdDbg->dbginfo.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
		pTrav->pThrdDbg->dbginfo.u.CreateThread.hThread = pTrav->hTh;
		pTrav->pThrdDbg->dbginfo.u.CreateThread.lpThreadLocalBase = pTrav->tlsPtr;
		pTrav->pThrdDbg->dbginfo.u.CreateThread.lpStartAddress = NULL;
		SetEvent(pTrav->pThrdDbg->hEvent);
		SC_WaitForMultiple(1,&pTrav->pThrdDbg->hBlockEvent,FALSE,INFINITE);
	}
	EnterCriticalSection(&ModListcs);
	pMod = pModList;
	while (pMod) {
		if (HasModRefProcPtr(pMod,pproc)) {
			LeaveCriticalSection(&ModListcs);
			pth->pThrdDbg->dbginfo.dwProcessId = (DWORD)pproc->hProc;
			pth->pThrdDbg->dbginfo.dwThreadId = (DWORD)pth->hTh;
			pth->pThrdDbg->dbginfo.dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
			pth->pThrdDbg->dbginfo.u.LoadDll.hFile = NULL;
			pth->pThrdDbg->dbginfo.u.LoadDll.lpBaseOfDll = (LPVOID)ZeroPtr(pMod->BasePtr);
			pth->pThrdDbg->dbginfo.u.LoadDll.dwDebugInfoFileOffset = 0;
			pth->pThrdDbg->dbginfo.u.LoadDll.nDebugInfoSize = 0;
			pth->pThrdDbg->dbginfo.u.LoadDll.lpImageName = pMod->lpszModName;
			pth->pThrdDbg->dbginfo.u.LoadDll.fUnicode = 1;
			SetEvent(pth->pThrdDbg->hEvent);
			SC_WaitForMultiple(1,&pth->pThrdDbg->hBlockEvent,FALSE,INFINITE);
			EnterCriticalSection(&ModListcs);
		}
		pMod = pMod->pMod;
	}
	LeaveCriticalSection(&ModListcs);
}

HANDLE WakeIfDebugWait(HANDLE hThrd) {
	PTHREAD pth;
	KCALLPROFON(14);
	if ((pth = HandleToThread(hThrd)) && GET_DEBUGWAIT(pth) && (GET_RUNSTATE(pth) == RUNSTATE_BLOCKED)) {
		if (pth->lpProxy) {
			DEBUGCHK(pth->lpce);
			pth->lpce->base = pth->lpProxy;
			pth->lpce->size = (DWORD)pth->lpPendProxy;
			pth->lpProxy = 0;
			pth->bWaitState = WAITSTATE_SIGNALLED;
		}
		pth->wCount++;
		pth->retValue = WAIT_FAILED;
		SET_RUNSTATE(pth,RUNSTATE_NEEDSRUN);
	} else
		hThrd = 0;
	KCALLPROFOFF(14);
	return hThrd;
}

void SC_DebugNotify(DWORD dwFlags, DWORD data) {
	PMODULE pMod;
	HANDLE hth;
	PTHREAD pth;
	ACCESSKEY ulOldKey;
	if (pCurThread->pThrdDbg && ProcStarted(pCurProc) && pCurThread->pThrdDbg->hEvent) {
		pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
		pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
		switch (dwFlags) {
			case DLL_PROCESS_ATTACH:
				pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.hFile = NULL;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.hProcess = hCurProc;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.hThread = hCurThread;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.lpStartAddress = (LPTHREAD_START_ROUTINE)data;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.lpBaseOfImage = (LPVOID)pCurProc->dwVMBase;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.nDebugInfoSize = 0;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.lpThreadLocalBase = pCurThread->tlsPtr;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.lpImageName = pCurProc->lpszProcName;
				pCurThread->pThrdDbg->dbginfo.u.CreateProcessInfo.fUnicode = 1;
				break;
			case DLL_PROCESS_DETACH:
				pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
				pCurThread->pThrdDbg->dbginfo.u.ExitProcess.dwExitCode = GET_DYING(pCurThread) ? GetUserInfo(hCurThread) : data;
				break;
			case DLL_THREAD_ATTACH:
				pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
				pCurThread->pThrdDbg->dbginfo.u.CreateThread.hThread = hCurThread;
				pCurThread->pThrdDbg->dbginfo.u.CreateThread.lpThreadLocalBase = pCurThread->tlsPtr;
				pCurThread->pThrdDbg->dbginfo.u.CreateThread.lpStartAddress = (LPTHREAD_START_ROUTINE)data;
				break;
			case DLL_THREAD_DETACH:
				pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
				pCurThread->pThrdDbg->dbginfo.u.ExitThread.dwExitCode = GET_DYING(pCurThread) ? GetUserInfo(hCurThread) : data;
				break;
		}
		SWITCHKEY(ulOldKey,0xffffffff);
		if (hth = (HANDLE)KCall((PKFN)WakeIfDebugWait,pCurProc->hDbgrThrd))
			KCall((PKFN)MakeRunIfNeeded,hth);
		SETCURKEY(ulOldKey);
		SetEvent(pCurThread->pThrdDbg->hEvent);
		SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
		if ((dwFlags == DLL_THREAD_DETACH) && (pth = HandleToThread(pCurThread->pOwnerProc->hDbgrThrd)))
			DecRef(hCurThread,pth->pOwnerProc,0);
		if (dwFlags == DLL_PROCESS_ATTACH) {
			EnterCriticalSection(&ModListcs);
			pMod = pModList;
			while (pMod) {
				if (HasModRefProcPtr(pMod,pCurProc)) {
					LeaveCriticalSection(&ModListcs);
					pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
					pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
					pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.hFile = NULL;
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.lpBaseOfDll = (LPVOID)ZeroPtr(pMod->BasePtr);
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.dwDebugInfoFileOffset = 0;
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.nDebugInfoSize = 0;
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.lpImageName = pMod->lpszModName;
					pCurThread->pThrdDbg->dbginfo.u.LoadDll.fUnicode = 1;
					SetEvent(pCurThread->pThrdDbg->hEvent);
					SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
					EnterCriticalSection(&ModListcs);
				}
				pMod = pMod->pMod;
			}
			LeaveCriticalSection(&ModListcs);
		}
	}
}

BOOL SC_WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds) {
	HANDLE hWaits[MAXIMUM_WAIT_OBJECTS];
	HANDLE hThrds[MAXIMUM_WAIT_OBJECTS];
	DWORD ret;
	DWORD count;
	HANDLE hth;
	PTHREAD pth;
	do {
		count = 0;
		EnterCriticalSection(&DbgApiCS);
		if (pCurThread->pThrdDbg) {
			hth = pCurThread->pThrdDbg->hFirstThrd;
			while (hth && (count < MAXIMUM_WAIT_OBJECTS)) {
				pth = HandleToThread(hth);
				hThrds[count] = hth;
				hWaits[count++] = pth->pThrdDbg->hEvent;
				hth = pth->pThrdDbg->hNextThrd;
			}
		}
		LeaveCriticalSection(&DbgApiCS);
		if (!count)
			return FALSE;
		SET_USERBLOCK(pCurThread);
		SET_DEBUGWAIT(pCurThread);
		ret = SC_WaitForMultiple(count,hWaits,FALSE,dwMilliseconds);
		CLEAR_DEBUGWAIT(pCurThread);
		CLEAR_USERBLOCK(pCurThread);
		if (ret == WAIT_TIMEOUT)
			return FALSE;
		if (ret == WAIT_FAILED)
			lpDebugEvent->dwDebugEventCode = 0;
		else {
			pth = HandleToThread(hThrds[ret-WAIT_OBJECT_0]);
			EnterCriticalSection(&DbgApiCS);
			if (!pth || !pth->pThrdDbg || !pth->pThrdDbg->hEvent)
				lpDebugEvent->dwDebugEventCode = 0;
			else {
				*lpDebugEvent = pth->pThrdDbg->dbginfo;
				pth->pThrdDbg->bDispatched = 1;
			}
			LeaveCriticalSection(&DbgApiCS);
		}
	} while (!lpDebugEvent->dwDebugEventCode);
	return TRUE;
}

BOOL SC_ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus) {
	PTHREAD pth;
    ACCESSKEY ulOldKey;
	pth = HandleToThread((HANDLE)dwThreadId);
	if (!pth || !pth->pThrdDbg || !pth->pThrdDbg->hEvent || !pth->pThrdDbg->bDispatched) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	pth->pThrdDbg->dbginfo.dwDebugEventCode = dwContinueStatus;
	pth->pThrdDbg->bDispatched = 0;
    SWITCHKEY(ulOldKey,0xffffffff);
	SetEvent(pth->pThrdDbg->hBlockEvent);
	SETCURKEY(ulOldKey);
	return TRUE;
}

DWORD StopProc(HANDLE hProc, LPTHRDDBG pdbg) {
	DWORD retval, res;
	PPROCESS pproc;
	PTHREAD pth;
	KCALLPROFON(27);
	if (!(pproc = HandleToProc(hProc))) {
		KCALLPROFOFF(27);
		return 2;
	}
	pth = pproc->pMainTh;
	res = KCall((PKFN)ThreadSuspend,pth);
	if (res == 0xfffffffe) {
		KCALLPROFOFF(27);
		return 0;
	}
	if (res == 0xffffffff) {
		KCALLPROFOFF(27);
		return 2;
	}
	SET_DEBUGBLK(pth);
	IncRef(pth->hTh,pCurProc);
	if (!pth->pThrdDbg) {
		pth->pThrdDbg = pdbg;
		retval = 1;
	} else {
		DEBUGCHK(!pth->pThrdDbg->hEvent);
		pth->pThrdDbg->hEvent = pdbg->hEvent;
		pth->pThrdDbg->hBlockEvent = pdbg->hBlockEvent;
		retval = 3;
	}
	pth->pThrdDbg->hNextThrd = pCurThread->pThrdDbg->hFirstThrd;
	pCurThread->pThrdDbg->hFirstThrd = pth->hTh;
	KCALLPROFOFF(27);
	return retval;
}

DWORD StopProc2(PPROCESS pproc, LPTHRDDBG pdbg, PTHREAD *ppth) {
	DWORD retval;
	PTHREAD pth;
	KCALLPROFON(25);
	pth = *ppth;
	retval = 2;
	if (!GET_DEBUGBLK(pth)) {
		if (ThreadSuspend(pth) == 0xfffffffe) {
			KCALLPROFOFF(25);
			return 0;
		}
		SET_DEBUGBLK(pth);
		IncRef(pth->hTh,pCurProc);
		if (!pth->pThrdDbg) {
			pth->pThrdDbg = pdbg;
			retval = 1;
		} else {
			DEBUGCHK(!pth->pThrdDbg->hEvent);
			pth->pThrdDbg->hEvent = pdbg->hEvent;
			pth->pThrdDbg->hBlockEvent = pdbg->hBlockEvent;
			retval = 3;
		}
		pth->pThrdDbg->hNextThrd = pCurThread->pThrdDbg->hFirstThrd;
		pCurThread->pThrdDbg->hFirstThrd = pth->hTh;
	}
	*ppth = pth->pNextInProc;
	KCALLPROFOFF(25);
	return retval;
}

void DAPProc(PPROCESS pproc) {
	PTHREAD pth, pth2;
	SETCURKEY(0xffffffff);
	DoDebugAttach(pproc);
	for (pth = pproc->pTh; pth; pth = pth2) {
		pth2 = pth->pNextInProc;
		CLEAR_DEBUGBLK(pth);		
		KCall((PKFN)ThreadResume,pth);
	}
	SET_DYING(pCurThread);
	SET_DEAD(pCurThread);
	NKTerminateThread(0);
}

BOOL SC_ProcDebug(DWORD dwProcessId) {
	HANDLE hth;
	PTHREAD pth;
	DWORD res;
	PPROCESS pproc;
	LPTHRDDBG pdbg;
    ACCESSKEY ulOldKey;
   	if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
		ERRORMSG(1,(L"SC_ProcDebug failed due to insufficient trust\r\n"));
		KSetLastError(pCurThread,ERROR_ACCESS_DENIED);
		return FALSE;
	}
	pproc = HandleToProc((HANDLE)dwProcessId);
	if (!pproc || pproc->DbgActive)
		return FALSE;
	pproc->DbgActive = (BYTE)MAX_PROCESSES + (BYTE)pCurProc->procnum;
	if (!pCurThread->pThrdDbg) {
		if (!(pdbg = (LPTHRDDBG)AllocMem(HEAP_THREADDBG))) {
			pproc->DbgActive = 0;
			return FALSE;
		}
		pdbg->hFirstThrd = 0;
		pdbg->hEvent = 0;
		pdbg->psavedctx = 0;
		pdbg->bDispatched = 0;
		pdbg->dbginfo.dwDebugEventCode = 0;
		if (InterlockedTestExchange((LPDWORD)&pCurThread->pThrdDbg,0,(DWORD)pdbg))
			FreeMem(pdbg,HEAP_THREADDBG);
	}
	if (!(pdbg = AllocDbg((HANDLE)dwProcessId))) {
		pproc->DbgActive = 0;
		return FALSE;
	}
	EnterCriticalSection(&DbgApiCS);
	while (!(res = KCall((PKFN)StopProc,dwProcessId,pdbg)))
		Sleep(10);
	if (res == 2) {
		pproc->DbgActive = 0;
		LeaveCriticalSection(&DbgApiCS);
		return FALSE;
	}
	if (res == 3)
		FreeMem(pdbg,HEAP_THREADDBG);
	pth = pproc->pTh;
	res = 1;
	while (pth && pth->pNextInProc) {
		if (res && (res != 2) && !(pdbg = AllocDbg((HANDLE)dwProcessId))) {
			
			pproc->DbgActive = 0;
			LeaveCriticalSection(&DbgApiCS);
			return FALSE;
		}
		if (!(res = KCall((PKFN)StopProc2,pproc,pdbg,&pth))) {
			LeaveCriticalSection(&DbgApiCS);
			Sleep(10);
			pth = pproc->pTh;
			EnterCriticalSection(&DbgApiCS);
		}
		if (res == 3)
			FreeMem(pdbg,HEAP_THREADDBG);
	}
	LeaveCriticalSection(&DbgApiCS);
	if (res == 2) {
	   SWITCHKEY(ulOldKey,0xffffffff);
		SetHandleOwner(pdbg->hEvent,hCurProc);
		SetHandleOwner(pdbg->hBlockEvent,hCurProc);
		SETCURKEY(ulOldKey);
		CloseHandle(pdbg->hEvent);
		CloseHandle(pdbg->hBlockEvent);
		FreeMem(pdbg,HEAP_THREADDBG);
	}
	pproc->hDbgrThrd = hCurThread;
	pproc->bChainDebug = 0;
	hth = CreateKernelThread((LPTHREAD_START_ROUTINE)DAPProc,pproc,THREAD_RT_PRIORITY_NORMAL, 0x40000000);
	if (!hth) {
		
		pproc->DbgActive = 0;
		pproc->hDbgrThrd = 0;
		return FALSE;
	}
	CloseHandle(hth);
	return TRUE;
}

extern BOOL CheckLastRef(HANDLE hTh);

BOOL SC_ThreadCloseHandle(HANDLE hTh) {
	LPTHREADTIME p1,p2;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadCloseHandle entry: %8.8lx\r\n",hTh));
   CELOG_ThreadCloseHandle(hTh);
	if (!KCall((PKFN)CheckLastRef,hTh)) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadCloseHandle exit: %8.8lx\r\n", FALSE));
		return FALSE;
	}
	if (DecRef(hTh,pCurProc,0)) {
	   DEBUGCHK(!HandleToThread(hTh));
	   FreeHandle(hTh);
		EnterCriticalSection(&NameCS);
		p1 = NULL;
		p2 = TTList;
		while (p2 && p2->hTh != hTh) {
			p1 = p2;
			p2 = p2->pnext;
		}
		if (p2) {
			if (p1)
				p1->pnext = p2->pnext;
			else
				TTList = p2->pnext;
			FreeMem(p2,HEAP_THREADTIME);
		}
		LeaveCriticalSection(&NameCS);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadCloseHandle exit: %8.8lx\r\n", TRUE));
	return TRUE;
}

BOOL SC_ProcCloseHandle(HANDLE hProc) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcCloseHandle entry: %8.8lx\r\n",hProc));
   CELOG_ProcessCloseHandle(hProc);
	if (DecRef(hProc,pCurProc,0)) {
      CELOG_ProcessDelete(hProc);
		DEBUGCHK(!HandleToProc(hProc));
		FreeHandle(hProc);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcCloseHandle exit: %8.8lx\r\n", TRUE));
	return TRUE;
}

/* Creates a new process */

BOOL SC_CreateProc(LPCWSTR lpszImageName, LPCWSTR lpszCommandLine, LPSECURITY_ATTRIBUTES lpsaProcess,
	LPSECURITY_ATTRIBUTES lpsaThread, BOOL fInheritHandles, DWORD fdwCreate, LPVOID lpvEnvironment,
	LPWSTR lpszCurDir, LPSTARTUPINFO lpsiStartInfo, LPPROCESS_INFORMATION lppiProcInfo) {
	LPTHRDDBG pdbg;
	ProcStartInfo psi;
	PROCESS_INFORMATION procinfo;
	LPBYTE lpStack = 0;
	LPBYTE pSwapStack = 0;
	LPVOID lpvSection = 0;
	PPROCESS pNewproc = 0;
	PTHREAD pNewth = 0, pDbgrThrd;
	HANDLE hNewproc = 0, hNewth = 0;
	int loop;
    int length;
    LPWSTR uptr, procname;
    extern void KUnicodeToAscii(LPCHAR chptr, LPCWSTR wchptr, int maxlen);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateProc entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
		lpszImageName, lpszCommandLine, lpsaProcess, lpsaThread, fInheritHandles, fdwCreate, lpvEnvironment,
		lpszCurDir, lpsiStartInfo, lppiProcInfo));
	psi.lpszImageName = MapPtr((LPWSTR)lpszImageName);
	psi.lpszCmdLine = MapPtr((LPWSTR)(lpszCommandLine ? lpszCommandLine : TEXT("")));
	psi.lppi = MapPtr(lppiProcInfo ? lppiProcInfo : &procinfo);
	psi.lppi->hProcess = 0;
	psi.he = 0;
	if (!lpszImageName || fInheritHandles || (fdwCreate & ~(CREATE_NEW_CONSOLE|DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS|CREATE_SUSPENDED|0x80000000)) ||
		lpvEnvironment || lpszCurDir || (strlenW(lpszImageName) >= MAX_PATH)) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		goto exitcpw;
	}
	if (pCurProc->bTrustLevel != KERN_TRUST_FULL)
		fdwCreate &= ~(DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS);
	psi.fdwCreate = fdwCreate;
	if (!MTBFf || 
		!(lpStack = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,CNP_STACK_SIZE,MEM_RESERVE|MEM_AUTO_COMMIT,PAGE_NOACCESS)) ||
		!VirtualAlloc((LPVOID)((DWORD)lpStack+CNP_STACK_SIZE-MIN_STACK_SIZE),MIN_STACK_SIZE,MEM_COMMIT,PAGE_READWRITE) ||
		!(psi.he = CreateEvent(0,1,0,0)) ||
		!(lpvSection = CreateSection(0))) {
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		goto exitcpw;
	}
	loop = ((DWORD)lpvSection>>VA_SECTION)-1;
	pNewproc = &ProcArray[loop];
	pNewproc->pStdNames[0] = pNewproc->pStdNames[1] = pNewproc->pStdNames[2] = 0;
	pNewth = (PTHREAD)AllocMem(HEAP_THREAD);
	if (!pNewth) {
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		goto exitcpw;
	}
	if (!(hNewth = AllocHandle(&cinfThread,pNewth,pCurProc))) {
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		goto exitcpw;
	}
	if (!(hNewproc = AllocHandle(&cinfProc,pNewproc,pCurProc))) {
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		goto exitcpw;
	}
	if (!(pSwapStack = GetHelperStack()))
		goto exitcpw;
	SetUserInfo(hNewproc,STILL_ACTIVE);
	pNewproc->procnum = loop;
	pNewproc->pcmdline = &L"";
	pNewproc->lpszProcName = &L"";
	pNewproc->DbgActive = 0;
	pNewproc->hProc = hNewproc;
	pNewproc->dwVMBase = (DWORD)lpvSection;
	pNewproc->pTh = pNewproc->pMainTh = pNewth;
	pNewproc->aky = 1 << pNewproc->procnum;
	pNewproc->bChainDebug = 0;
	pNewproc->bTrustLevel = KERN_TRUST_FULL;
	if (fdwCreate & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) {
		EnterCriticalSection(&DbgApiCS);
		pNewproc->hDbgrThrd = hCurThread;
		if (!(fdwCreate & DEBUG_ONLY_THIS_PROCESS))
			pNewproc->bChainDebug = 1;
		if (!pCurThread->pThrdDbg) {
			if (!(pdbg = (LPTHRDDBG)AllocMem(HEAP_THREADDBG))) {
				LeaveCriticalSection(&DbgApiCS);
				KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
				goto exitcpw;
			}
			pdbg->hFirstThrd = 0;
			pdbg->hEvent = 0;
			pdbg->psavedctx = 0;
			pdbg->bDispatched = 0;
			pdbg->dbginfo.dwDebugEventCode = 0;
			if (InterlockedTestExchange((LPDWORD)&pCurThread->pThrdDbg,0,(DWORD)pdbg))
				FreeMem(pdbg,HEAP_THREADDBG);
		}
		LeaveCriticalSection(&DbgApiCS);
	} else if (pCurProc->hDbgrThrd && pCurProc->bChainDebug) {
		pNewproc->hDbgrThrd = pCurProc->hDbgrThrd;
		pNewproc->bChainDebug = 1;
	}
	else
		pNewproc->hDbgrThrd = 0;
	pNewproc->tlsLowUsed = TLSSLOT_RESERVE;
	pNewproc->tlsHighUsed = 0;
	pNewproc->pfnEH = 0;
	pNewproc->pExtPdata = 0;
	pNewproc->ZonePtr = 0;
	pNewproc->pProxList = 0;
	pNewproc->dwDyingThreads = 0;
	pNewproc->pmodResource = 0;
	pNewproc->oe.filetype = 0;
	if (!(fdwCreate & CREATE_NEW_CONSOLE)) {
		for (loop = 0; loop < 3; loop++) {
			if (!pCurProc->pStdNames[loop])
				pNewproc->pStdNames[loop] = 0;
			else {
				if (!(pNewproc->pStdNames[loop] = AllocName((strlenW(pCurProc->pStdNames[loop]->name)+1)*2))) {
					KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
					goto exitcpw;
				}
				kstrcpyW(pNewproc->pStdNames[loop]->name,pCurProc->pStdNames[loop]->name);
			}
		}
	}
	uptr = procname = psi.lpszImageName;
	while (*uptr)
		if (*uptr++ == (WCHAR)'\\')
			procname = uptr;
    length = (uptr - procname + 1)*sizeof(WCHAR);
    uptr = MapPtr((LPBYTE)((DWORD)lpStack + CNP_STACK_SIZE - ((length+7) & ~7)));
    memcpy(uptr, procname, length);
    pNewproc->lpszProcName = uptr;
	if (!InitThreadStruct(pNewth,hNewth,pNewproc,hNewproc,THREAD_RT_PRIORITY_NORMAL)) {
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		goto exitcpw;
	}
	pNewth->pSwapStack = pSwapStack;
	pNewth->pNextInProc = pNewth->pPrevInProc = 0;
	AddAccess(&pNewth->aky,pCurThread->aky);
	GCFT(&pNewth->ftCreate);
	MDCreateThread(pNewth,lpStack,CNP_STACK_SIZE,(LPVOID)CreateNewProc,0,pCurProc->dwVMBase,TH_KMODE,(ulong)&psi);
	ZeroTLS(pNewth);
	IncRef(hNewproc,pNewproc);
	IncRef(hNewth,pNewproc);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateProc switching to loader on thread %8.8lx\r\n",pNewth));
	if (pNewproc->hDbgrThrd) {
		EnterCriticalSection(&DbgApiCS);
		pDbgrThrd = HandleToThread(pNewproc->hDbgrThrd);
		pNewth->pThrdDbg = AllocDbg(hNewproc);
		pNewth->pThrdDbg->hNextThrd = pDbgrThrd->pThrdDbg->hFirstThrd;
		pDbgrThrd->pThrdDbg->hFirstThrd = hNewth;
		LeaveCriticalSection(&DbgApiCS);
	}
   CELOG_ProcessCreate(hNewproc, procname, (DWORD)lpvSection);
	if (pLogProcessCreate)
		pLogProcessCreate((DWORD)pNewproc);

   // Save off the thread's program counter for getting its name later.
   pNewth->dwStartAddr = (DWORD) CreateNewProc;
   
   // Null out the tocptr because it hasn't been set yet. (It will be fixed when
   // the thread runs through CreateNewProc, running OpenExe in the process.)
   // This is necessary because we look into the TOC for the process' name 
   // when getting the name of the primary thread of a process.
   pNewth->pOwnerProc->oe.tocptr = 0;
   pNewth->pOwnerProc->oe.filetype = 0;
   
   // The process name was overwritten when we ran ZeroTLS on the thread!
   // Set the process name now because we will need it when we are logging the
   // thread creation.  The process struct will be corrected when our new thread
   // begins running through CreateNewProc.
   pNewproc->lpszProcName = procname;
   
   CELOG_ThreadCreate(pNewth, hNewproc, procname);
	if (pLogThreadCreate)
		pLogThreadCreate((DWORD)pNewth,(DWORD)pNewproc);
	SET_RUNSTATE(pNewth,RUNSTATE_NEEDSRUN);
	KCall((PKFN)MakeRunIfNeeded,hNewth);
	WaitForMultipleObjects(1,&psi.he,FALSE,INFINITE);
	if (!psi.lppi->hProcess) {
		CloseHandle(hNewproc);
		CloseHandle(hNewth);
		KSetLastError(pCurThread,psi.lppi->dwThreadId);
	} else if ((psi.lppi == MapPtr(&procinfo) && !(fdwCreate & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)))) {
		CloseHandle(hNewproc);
		CloseHandle(hNewth);
	}
	goto done;
exitcpw:
	if (pNewproc) {
		for (loop = 0; loop < 3; loop++)
			if (pNewproc->pStdNames[loop])
				FreeName(pNewproc->pStdNames[loop]);
	}
	if (pNewth)
		FreeMem(pNewth,HEAP_THREAD);
	if (hNewth)
		FreeHandle(hNewth);
	if (hNewproc)
		FreeHandle(hNewproc);
	if (lpvSection)
		DeleteSection(lpvSection);
	if (psi.he)
		CloseHandle(psi.he);
	if (lpStack) {
		VirtualFree(lpStack,CNP_STACK_SIZE,MEM_DECOMMIT);
		VirtualFree(lpStack,0,MEM_RELEASE);
	}
	if (pSwapStack)
		FreeHelperStack(pSwapStack);
done:
	if (psi.he)
		CloseHandle(psi.he);
	DEBUGMSG(ZONE_ENTRY,(L"SC_CreateProc exit: %8.8lx\r\n", (psi.lppi->hProcess ? TRUE : FALSE)));
	return (psi.lppi->hProcess ? TRUE : FALSE);
}

#ifdef THUMB
// WinCE 11310/11387: THUMB doesn't like 32 bit shifts in 64 bit values
#pragma optimize("",off)
#endif
void sub64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres) {
	__int64 num1, num2;
	num1 = (((__int64)lpnum1->dwHighDateTime)<<32)+(__int64)lpnum1->dwLowDateTime;
	num2 = (((__int64)lpnum2->dwHighDateTime)<<32)+(__int64)lpnum2->dwLowDateTime;
	num1 -= num2;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}
#ifdef THUMB
// WinCE 11310/11387: THUMB doesn't like 32 bit shifts in 64 bit values
#pragma optimize("",on)
#endif

void SystemStartupFunc(ulong param) {
	DWORD base;
	DWORD first, last;
    ROMChain_t *pROM;
	SYSTEMTIME st;
	FILETIME ft,ft2,ft3;
#if x86
	LPVOID pEmul;
#endif
    KernelInit2();
	first = 0;
	last = 0;
    for (pROM = ROMChain; pROM; pROM = pROM->pNext) {
    	if (pROM->pTOC->dllfirst != pROM->pTOC->dlllast) {
    		if (!first && !last) {
				first = pROM->pTOC->dllfirst;
				last = pROM->pTOC->dlllast;
    		} else {
				if (first > pROM->pTOC->dllfirst)
					first = pROM->pTOC->dllfirst;
				if (last < pROM->pTOC->dlllast)
					last = pROM->pTOC->dlllast;
    		}
    	}
    }
	if (first != last) {
		base = (DWORD)VirtualAlloc((LPVOID)first,last-first,MEM_RESERVE|MEM_IMAGE, PAGE_NOACCESS);
		DEBUGCHK(ZeroPtr(base) == first);
		DllLoadBase = ZeroPtr(base);
	} else {
		DllLoadBase = 1<<VA_SECTION;
	}
	hCoreDll = LoadOneLibraryW(L"coredll.dll",1,0);
	pExitThread = GetProcAddressA(hCoreDll,(LPCSTR)6); // ExitThread
	pPSLNotify = (void (*)(DWORD, DWORD, DWORD))GetProcAddressA(hCoreDll,(LPCSTR)7); // PSLNotify
	pIsExiting = GetProcAddressA(hCoreDll,(LPCSTR)159); // IsExiting
	DEBUGCHK(pIsExiting == (LPDWORD)ZeroPtr(pIsExiting));
	MTBFf = (void (*)(LPVOID, ulong, ulong, ulong, ulong))GetProcAddressA(hCoreDll,(LPCSTR)14); // MainThreadBaseFunc
	CTBFf = (void (*)(LPVOID, ulong, ulong, ulong, ulong))GetProcAddressA(hCoreDll,(LPCSTR)1240); // ComThreadBaseFunc
	TBFf = (void (*)(LPVOID, ulong))GetProcAddressA(hCoreDll,(LPCSTR)13); // ThreadBaseFunc
#if x86
	if (!(pEmul = GetProcAddressA(hCoreDll,(LPCSTR)"NPXNPHandler"))) {
		DEBUGMSG(1,(L"Did not find emulation code for x86... using floating point hardware.\r\n"));
		dwFPType = 1;
	    KCall((PKFN)InitializeFPU);
	} else {
		DEBUGMSG(1,(L"Found emulation code for x86... using software for emulation of floating point.\r\n"));
		dwFPType = 2;
    	KCall((PKFN)InitializeEmx87);
		KCall((PKFN)InitNPXHPHandler,pEmul);
	}
#endif
	KSystemTimeToFileTime = (BOOL (*)(LPSYSTEMTIME, LPFILETIME))GetProcAddressA(hCoreDll,(LPCSTR)19); // SystemTimeToFileTime
	KLocalFileTimeToFileTime = (BOOL (*)(const FILETIME *, LPFILETIME))GetProcAddressA(hCoreDll,(LPCSTR)22); // LocalFileTimeToFileTime
	KFileTimeToSystemTime = (BOOL (*)(const FILETIME *, LPSYSTEMTIME))GetProcAddressA(hCoreDll,(LPCSTR)20); // FileTimeToSystemTime
	KCompareFileTime = (LONG (*)(LPFILETIME, LPFILETIME))GetProcAddressA(hCoreDll,(LPCSTR)18); // CompareFileTime
	pGetProcessHeap = (HANDLE (*)(void))GetProcAddressA(hCoreDll,(LPCSTR)50); // GetProcessHeap
	pGetHeapSnapshot = (BOOL (*)(THSNAP *, BOOL, LPVOID *))GetProcAddressA(hCoreDll,(LPCSTR)52); // GetHeapSnapshot
	// do this now, so that we continue running after we've created the new thread
#ifdef START_KERNEL_MONITOR_THREAD
	CreateKernelThread((LPTHREAD_START_ROUTINE)Monitor1,0,THREAD_RT_PRIORITY_ABOVE_NORMAL,0);
#endif
    
    // If logging enabled (NKPROF), spin up a background thread
    CELOGCREATETHREAD();

    // If platform supports debug Ethernet, register ISR now.
    if (pEdbgInitializeInterrupt)
        pEdbgInitializeInterrupt();

	SETCURKEY(0xffffffff);
	pCleanupThread = pCurThread;
	hAlarmThreadWakeup = CreateEvent(0,0,0,0);
	DEBUGCHK(hAlarmThreadWakeup);
	InitializeCriticalSection(&rtccs);
    IntrEvents[SYSINTR_RTC_ALARM-SYSINTR_DEVICES] = HandleToEvent(hAlarmThreadWakeup);
	IntrEvents[SYSINTR_RTC_ALARM-SYSINTR_DEVICES]->pIntrProxy = AllocMem(HEAP_PROXY);
	DEBUGCHK(IntrEvents[SYSINTR_RTC_ALARM-SYSINTR_DEVICES]->pIntrProxy);
	IntrEvents[SYSINTR_RTC_ALARM-SYSINTR_DEVICES]->onequeue = 1;
	CreateKernelThread((LPTHREAD_START_ROUTINE)RunApps,0,THREAD_RT_PRIORITY_NORMAL,0);
	
	while (1) {
		KCall((PKFN)SetThreadBasePrio, hCurThread, THREAD_RT_PRIORITY_IDLE);
		WaitForMultipleObjects(1,&hAlarmThreadWakeup,0,INFINITE);
		while (InterlockedTestExchange(&PageOutNeeded,1,2) == 1)
			DoPageOut();
		EnterCriticalSection(&rtccs);
		if (hAlarmEvent) {
			OEMGetRealTime(&st);
			KSystemTimeToFileTime(&st,&ft);
			KSystemTimeToFileTime(&CurAlarmTime,&ft2);
			ft3.dwLowDateTime = 100000000; // 10 seconds
			ft3.dwHighDateTime = 0;
			sub64_64_64(&ft2,&ft3,&ft3);
			if (KCompareFileTime(&ft,&ft3) >= 0) {
				SetEvent(hAlarmEvent);
				hAlarmEvent = NULL;
			} else
			    OEMSetAlarmTime(&CurAlarmTime);
		}
		LeaveCriticalSection(&rtccs);
	}
}

void SC_SetKernelAlarm(HANDLE hAlarm, LPSYSTEMTIME lpst) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetKernelAlarm entry: %8.8lx %8.8lx\r\n",hAlarm,lpst));
	EnterCriticalSection(&rtccs);
	hAlarmEvent = hAlarm;
	memcpy(&CurAlarmTime,lpst,sizeof(SYSTEMTIME));
	SetEvent(hAlarmThreadWakeup);
	LeaveCriticalSection(&rtccs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetKernelAlarm exit\r\n"));
}

void SC_RefreshKernelAlarm(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_RefreshKernelAlarm entry\r\n"));
	DEBUGCHK(hAlarmThreadWakeup);
	SetEvent(hAlarmThreadWakeup);
	DEBUGMSG(ZONE_ENTRY,(L"SC_RefreshKernelAlarm exit\r\n"));
}

void ProcInit(void) {
	if (!dwDefaultThreadQuantum)
		dwDefaultThreadQuantum = 100;
	hCurThread = (HANDLE)2;			// so that EnterCriticalSection doesn't put 0 for owner
	ProcArray[0].aky = 1;	// so that the AllocHandles do the right thing for permissions
	pCurProc = &ProcArray[0];
	hCurProc = AllocHandle(&cinfProc,pCurProc,pCurProc);
	DEBUGCHK(hCurProc);
	pCurThread = AllocMem(HEAP_THREAD);
	DEBUGCHK(pCurThread);
	hCurThread = AllocHandle(&cinfThread,pCurThread,pCurProc);
	DEBUGCHK(hCurThread);
	SetUserInfo(hCurProc,STILL_ACTIVE);
	pCurProc->procnum = 0;
	pCurProc->pcmdline = &L"";
	pCurProc->DbgActive = 0;
	pCurProc->hProc = hCurProc;
	pCurProc->dwVMBase = (pCurProc->procnum+1) << VA_SECTION;
	pCurProc->pTh = pCurProc->pMainTh = pCurThread;
	pCurProc->aky = 1 << pCurProc->procnum;
	pCurProc->hDbgrThrd = 0;
	pCurProc->lpszProcName = L"NK.EXE";
	pCurProc->tlsLowUsed = TLSSLOT_RESERVE;
	pCurProc->tlsHighUsed = 0;
	pCurProc->pfnEH = 0;
	pCurProc->pExtPdata = 0;
	pCurProc->pStdNames[0] = pCurProc->pStdNames[1] = pCurProc->pStdNames[2] = 0;
	pCurProc->dwDyingThreads = 0;
	pCurProc->pmodResource = 0;
	pCurProc->bTrustLevel = KERN_TRUST_FULL;
#ifdef DEBUG
	pCurProc->ZonePtr = &dpCurSettings;
#else
	pCurProc->ZonePtr = 0;
#endif
	pCurProc->pProxList = 0;
	pCurProc->o32_ptr = 0;
	pCurProc->e32.e32_stackmax = KRN_STACK_SIZE;
	InitThreadStruct(pCurThread,hCurThread,pCurProc,hCurProc,THREAD_RT_PRIORITY_ABOVE_NORMAL);
	SETCURKEY(GETCURKEY()); // for CPUs that cache the access key outside the thread structure
	pCurThread->pNextInProc = pCurThread->pPrevInProc = 0;
	*(__int64 *)&pCurThread->ftCreate = 0;
}

void SchedInit(void) {
	LPBYTE pStack;
	bAllKMode = (pTOC->ulKernelFlags & KFLAG_NOTALLKMODE) ? 0 : 1;
	InitNKSection();
	SetCPUASID(pCurThread);
#ifdef SHx
	SetCPUGlobals();
#endif
	if (OpenExe(L"nk.exe",&pCurProc->oe,0,0)) {
		LoadE32(&pCurProc->oe,&pCurProc->e32,0,0,0,1,&pCurProc->bTrustLevel);
		pCurProc->BasePtr = (LPVOID)pCurProc->e32.e32_vbase;
	}
	pStack = VirtualAlloc((LPVOID)pCurProc->dwVMBase,pCurProc->e32.e32_stackmax,MEM_RESERVE|MEM_AUTO_COMMIT,PAGE_NOACCESS);
	VirtualAlloc(pStack+pCurProc->e32.e32_stackmax-PAGE_SIZE,PAGE_SIZE,MEM_COMMIT,PAGE_READWRITE);
	MDCreateThread(pCurThread, pStack, pCurProc->e32.e32_stackmax, (LPVOID)SystemStartupFunc, 0, pCurProc->dwVMBase, TH_KMODE, 0);
	ZeroTLS(pCurThread);
	pCurThread->dwUserTime = 0;
	pCurThread->dwKernTime = 0;
	MakeRun(pCurThread);
	DEBUGMSG(ZONE_SCHEDULE,(TEXT("Scheduler: Created master thread %8.8lx\r\n"),pCurThread));

   // Save off the thread's program counter for getting its name later.
   pCurThread->dwStartAddr = (DWORD) SystemStartupFunc;
    
   CELOG_ThreadCreate(pCurThread, pCurThread->pOwnerProc->hProc, NULL);
}

typedef void PAGEFN();

extern PAGEFN PageOutProc, PageOutMod, PageOutFile;

PAGEFN * const PageOutFunc[3] = {PageOutProc,PageOutMod,PageOutFile};

void DoPageOut(void) {
	static int state;
	int tries = 0;
	EnterCriticalSection(&PageOutCS);
	while ((tries++ < 3) && (PageOutNeeded || PageOutForced)) {
		(PageOutFunc[state])();
		state = (state+1)%3;
	}
	LeaveCriticalSection(&PageOutCS);
}

