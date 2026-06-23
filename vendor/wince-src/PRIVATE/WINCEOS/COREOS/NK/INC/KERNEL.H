/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*
    Kernel.h - internal kernel data

    This file contains the kernel's internal data structures, function prototypes,
    macros, etc. It is not intended for use outside of the kernel.
*/

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "windows.h"
#include "osver.h"

#ifndef offsetof
  #define offsetof(s,m) ((unsigned)&(((s *)0)->m))
#endif
#define ERRFALSE(exp)   extern char __ERRXX[(exp)!=0]

#include "pehdr.h"
#include "romldr.h"
#include "patcher.h"
#define C_ONLY
#include "xtime.h"

typedef ulong PHYSICAL_ADDRESS;
typedef int KAFFINITY;
typedef void (*PKINTERRUPT_ROUTINE)(void);
#define IN
#define OUT
#define OPTIONAL

typedef void (*RETADDR)();

typedef struct Thread THREAD;
typedef THREAD *PTHREAD;

typedef struct Process PROCESS;
typedef PROCESS *PPROCESS;

typedef ulong ACCESSKEY;
typedef ulong ACCESSLOCK;

typedef struct Module MODULE;
typedef MODULE *PMODULE, *LPMODULE;

typedef RETADDR FNDISP(PTHREAD pth, RETADDR ra, void *args);
typedef FNDISP *PFNDISP;
typedef BOOL (*PFNKDBG)(DWORD dwCause, PTHREAD pictx);

#include "kwin32.h"

typedef struct FreeInfo {
    PHYSICAL_ADDRESS    paStart;    /* start of available region */
    PHYSICAL_ADDRESS    paEnd;      /* end of region (last byte + 1) */
    PBYTE               pUseMap;    /* ptr to page usage count map */
} FREEINFO; /* FreeInfo */
typedef FREEINFO *PFREEINFO;

typedef struct MemoryInfo {
    PVOID       pKData;         /* start of kernel's data */
    PVOID       pKEnd;          /* end of kernel's data & bss */
    uint        cFi;            /* # of entries in free memory array */
    FREEINFO    *pFi;           /* Pointer to cFi FREEINFO entries */
} MEMORYINFO; /* MemoryInfo */

#define GetRegionFromAddr(addr) ((((addr) >= MemoryInfo.pFi[0].paStart) && ((addr) < MemoryInfo.pFi[0].paEnd)) ? \
	&MemoryInfo.pFi[0] : (((addr) >= MemoryInfo.pFi[1].paStart) && ((addr) < MemoryInfo.pFi[1].paEnd)) ? \
	&MemoryInfo.pFi[1] : 0)

extern MEMORYINFO MemoryInfo;

extern DWORD randdw1,randdw2;

/* Macros for maniputing access keys and locks */

#define AddAccess(pakDest, akSrc)       (*(pakDest) |= (akSrc))
#define RemoveAccess(pakDest, akSrc)    (*(pakDest) &= ~(akSrc))
#define TestAccess(palk, paky)  (*(palk) & *(paky))
#define LockFromKey(palk, paky) (*(palk) = *(paky))

#define IsValidKPtr(p)  ((char *)(p) >= (char *)MemoryInfo.pKData   \
	    && (char *)(p) < (char *)MemoryInfo.pKEnd)
#define IsValidModule(p) (IsValidKPtr(p) && ((p)->lpSelf == (p)))

/** Macros for retrieving and walking the client argument list */
#define roundedsizeof(t)    (sizeof(t)+sizeof(int)-1 & ~(sizeof(int)-1))
#define Arg(ap, type)       (*(type *)(ap))
#define NextArg(ap, type)   ((ap) = (char *)(ap) + roundedsizeof(type))

/**
 * Data structures and functions for handle manipulations
 */

typedef struct cinfo {
    char        acName[4];  /* 00: object type ID string */
    uchar       disp;     	/* 04: type of dispatch */
    uchar		type;		/* 05: api handle type */
    ushort      cMethods;   /* 06: # of methods in dispatch table */
    const PFNVOID *ppfnMethods;/* 08: ptr to array of methods (in server address space) */
    const DWORD *pdwSig;    /* 0C: ptr to array of method signatures */
    PPROCESS   	pServer;	/* 10: ptr to server process */
} CINFO;    /* cinfo */
typedef CINFO *PCINFO;

#define NUM_SYSTEM_SETS	32
extern const CINFO *SystemAPISets[NUM_SYSTEM_SETS];

#define DISPATCH_KERNEL     0   /* dispatch directly in kernel */
#define DISPATCH_I_KERNEL   1   /* dispatch implicit handle in kernel */
#define DISPATCH_KERNEL_PSL 2   /* dispatch as thread in kernel */
#define DISPATCH_I_KPSL     3   /* implicit handle as kernel thread */
#define DISPATCH_PSL        4   /* dispatch as user mode PSL */
#define DISPATCH_I_PSL      5   /* implicit handle as user mode PSL */

BOOL SC_Nop();
BOOL SC_NotSupported();

#define MAX_PROCESSES 32

typedef struct APISet {
    CINFO   cinfo;		/* description of the API set */
    int     iReg;       /* registered API set index (-1 if not registered) */
} APISET;
typedef APISET *PAPISET;


#include "schedule.h"
#include "nkintr.h"

//	The user mode virtual address space is 2GB split into 64 32M sections
// of 512 64K blocks of 16 4K pages.
//
// Virtual address format:
//	3322222 222221111 1111 110000000000
//	1098765 432109876 5432 109876543210
//	zSSSSSS BBBBBBBBB PPPP oooooooooooo

#define BLOCK_MASK      0x1FF
#define SECTION_MASK	0x03F
#define RESERVED_SECTIONS 1     // reserve section 0 for current process

#define FIRST_MAPPER_ADDRESS ((MAX_PROCESSES+RESERVED_SECTIONS) << VA_SECTION)
#define LAST_MAPPER_ADDRESS	0x7E000000
#define MAPPER_INCR (1<<VA_SECTION)
#define NUM_MAPPER_SECTIONS ((LAST_MAPPER_ADDRESS - FIRST_MAPPER_ADDRESS)/MAPPER_INCR)
ERRFALSE(NUM_MAPPER_SECTIONS <= 32);

// Bit offsets of block & section in a virtual address:
#define VA_BLOCK        16
#define VA_SECTION      25

// Values for mb_flags:
#define MB_FLAG_PAGER_TYPE	0x0f
#define MB_FLAG_AUTOCOMMIT 	0x10

#define MB_PAGER_NONE		0x00
#define MB_PAGER_FIRST		0x01
#define MB_PAGER_LOADER		0x01
#define MB_PAGER_MAPPING	0x02
#define MB_MAX_PAGER_TYPE	0x02

#define UNKNOWN_BASE    (~0)

#define VERIFY_READ_FLAG    0
#define VERIFY_EXECUTE_FLAG 0
#define VERIFY_WRITE_FLAG   1
#define VERIFY_KERNEL_OK    2

/** Address translation tables */

typedef struct MemBlock MEMBLOCK;

typedef MEMBLOCK *SECTION[BLOCK_MASK+1];
typedef SECTION *PSECTION;

extern SECTION NullSection;
#define NULL_SECTION    (&NullSection)
#define NULL_BLOCK      0
#define RESERVED_BLOCK  ((MEMBLOCK*)1)

#if defined(MIPS)
#include "nkmips.h"
#elif defined(SHx)
#include "nkshx.h"
#elif defined(x86)
#include "nkx86.h"
#elif defined(PPC)
#include "nkppc.h"
#elif defined(ARM)
#include "nkarm.h"
#else
#pragma error("No CPU Defined")
#endif
#define PendEvents  (KData.aInfo[KINX_PENDEVENTS])

ERRFALSE(SYSHANDLE_OFFSET == offsetof(struct KDataStruct, ahSys));
ERRFALSE(CURTLSPTR_OFFSET == offsetof(struct KDataStruct, lpvTls));
ERRFALSE(KINFO_OFFSET == offsetof(struct KDataStruct, aInfo));

#if PAGE_SIZE == 4096
  #define VA_PAGE       12
  #define PAGE_MASK     0x00F
#elif PAGE_SIZE == 2048
  #define VA_PAGE		11
  #define PAGE_MASK		0x01F
#elif PAGE_SIZE == 1024
  #define VA_PAGE		10
  #define PAGE_MASK		0x03F
#else
  #error "Unsupported Page Size"
#endif

#define PAGES_PER_BLOCK  (0x10000 / PAGE_SIZE)

ERRFALSE(SECTION_SHIFT == VA_SECTION);

/* Memory Block
 *   This structure maps a 64K block of memory. All memory reservations
 * must begin on a 64k boundary.
 */
struct MemBlock {
    ACCESSLOCK  alk;        /* 00: key code for this set of pages */
    uchar       cUses;      /* 04: # of page table entries sharing this leaf */
    uchar       flags;      /* 05: mapping flags */
    short       ixBase;     /* 06: first block in region */
    short		hPf;		/* 08: handle to pager */
    short		cLocks;		/* 0a: lock count */
    ulong       aPages[PAGES_PER_BLOCK]; /* 12: entrylo values */
}; /* MemBlock */

extern MEMBLOCK *PmbDecommitting;
extern HANDLE hCurrScav;
extern PTHREAD PthScavTarget;
extern PPROCESS PprcScavTarget;
extern RunList_t RunList;
extern void (*MTBFf)(LPVOID, ulong, ulong, ulong, ulong);

void SC_NKTerminateThread(DWORD dwExitCode);

extern LPVOID pExitThread;

extern BOOL bAllKMode;

typedef HANDLE (* PFNOPEN)(LPCWSTR);
typedef void (* PFNCLOSE)(HANDLE);

extern DWORD (*JITf)(LPCWSTR, LPWSTR, HANDLE *);

HANDLE JitOpenFile(LPCWSTR);
void JitCloseFile(HANDLE);

typedef int FN_PAGEIN(BOOL bWrite, DWORD addr);
typedef BOOL FN_PAGEOUT(DWORD addr);

extern FN_PAGEIN LoaderPageIn;
extern FN_PAGEOUT LoaderPageOut;
extern FN_PAGEIN MappedPageIn;
extern FN_PAGEOUT MappedPageOut;

/* Thread Call stack structure
 *
 *  This structure is used by the IPC mechanism to track
 * current process, access key, and return addresses for
 * IPC calls which are in progress. It is also used by
 * the exception handling code to hold critical thread
 * state while switching modes.
 */

typedef struct CALLSTACK {
    struct CALLSTACK *pcstkNext;
    RETADDR     retAddr;    /* return address */
    PPROCESS	pprcLast;	/* previous process */
    ACCESSKEY   akyLast;    /* previous access key */
    uint		extra;		/* extra CPU dependent data */
#if defined(MIPS)
	ulong		pPad;		/* so that excinfo fits in a callstack */
#endif
#if defined(x86)
	ulong		ExEsp;		/* saved Esp value for exception */
	ulong		ExEbp;		/* saved Ebp   " */
	ulong		ExEbx;		/* saved Ebx   " */
	ulong		ExEsi;		/* saved Esi   " */
	ulong		ExEdi;		/* saved Edi   " */
#endif
} CALLSTACK;    /* CallStack */
typedef CALLSTACK *PCALLSTACK;

#define PM_NOPAGING 0
#define PM_FULLPAGING 1
#define PM_NOPAGEOUT 2

#define FT_PPFSFILE 1
#define FT_OBJSTORE 2
#define FT_ROMIMAGE 3

ERRFALSE(sizeof(CEOID) == 4);

typedef struct openexe_t {
	union {
		int hppfs;			// ppfs handle
		HANDLE hf;			// object store handle
		TOCentry *tocptr;	// rom entry pointer
	};
	BYTE filetype;
	BYTE bIsOID;
	WORD pagemode;
	DWORD offset;
	union {
		Name *lpName;
		CEOID ceOid;
	};
} openexe_t;

typedef struct OEinfo_t {
	WCHAR plainname[MAX_PATH], tmpname[MAX_PATH];
	CEOIDINFO ceoi;
	BY_HANDLE_FILE_INFORMATION bhfi;
	WIN32_FIND_DATA wfd;
} OEinfo_t;

BOOL OpenExe(LPWSTR lpszName, openexe_t *oeptr, BOOL bIsDll, BOOL bAllowPaging);
BOOL SafeOpenExe(LPWSTR lpszName, openexe_t *oeptr, BOOL bIsDll, BOOL bAllowPaging, OEinfo_t *poeinfo);
void CloseExe(openexe_t *oeptr);
DWORD LoadE32(openexe_t *oeptr, e32_lite *eptr, DWORD *lpFlags, DWORD *lpEntry, BOOL bIgnoreCPU, BOOL bAllowPaging, LPBYTE pbTrustLevel);
BOOL GetNameFromOE(CEOIDINFO *pceoi, openexe_t *oeptr);

void CloseAllCrits(void);

extern DWORD PagedInCount;

typedef struct {
	HANDLE hFirstThrd;	// first thread being debugged by this thread
	HANDLE hNextThrd;	// next thread being debugged
	PCONTEXT psavedctx;	// pointer to saved context, if any
	HANDLE hEvent;		// handle to wait on for debug event for this thread
	HANDLE hBlockEvent;	// handle that thread is waiting on
	DEBUG_EVENT dbginfo;// debug info
	BOOL bDispatched;
} THRDDBG, *LPTHRDDBG;

#define KERN_TRUST_FULL 2
#define KERN_TRUST_RUN 1

/* Thread data structures */

#define RUNSTATE_RUNNING 0	// must be 0
#define RUNSTATE_RUNNABLE 1
#define RUNSTATE_BLOCKED 2
#define RUNSTATE_NEEDSRUN 3 // on way to being runnable

#define WAITSTATE_SIGNALLED 0
#define WAITSTATE_PROCESSING 1
#define WAITSTATE_BLOCKED 2

#define TIMEMODE_USER 0
#define TIMEMODE_KERNEL 1

#define RUNSTATE_SHIFT		 0	// 2 bits
#define DYING_SHIFT			 2	// 1 bit
#define DEAD_SHIFT			 3	// 1 bit
#define BURIED_SHIFT		 4	// 1 bit
#define SLEEPING_SHIFT		 5	// 1 bit
#define TIMEMODE_SHIFT		 6	// 1 bit
#define NEEDDBG_SHIFT		 7	// 1 bit
#define STACKFAULT_SHIFT	 8	// 1 bit
#define DEBUGBLK_SHIFT		 9	// 1 bit
#define NOPRIOCALC_SHIFT	10	// 1 bit
#define DEBUGWAIT_SHIFT		11	// 1 bit
#define USERBLOCK_SHIFT		12	// 1 bit
#ifdef DEBUG	
#define DEBUG_LOOPCNT_SHIFT 13 // 1 bit - only in debug
#endif
#define PROFILE_SHIFT		15	// 1 bit, must be 15!  Used by assembly code!

#define GET_BPRIO(T)		((WORD)((T)->bBPrio))							/* base priority */
#define GET_CPRIO(T)		((WORD)((T)->bCPrio))							/* current priority */
#define SET_BPRIO(T,S)		((T)->bBPrio = (BYTE)(S))
#define SET_CPRIO(T,S)		((T)->bCPrio = (BYTE)(S))

#define GET_TIMEMODE(T)		(((T)->wInfo >> TIMEMODE_SHIFT)&0x1)		/* What timemode are we in */
#define GET_RUNSTATE(T)		(((T)->wInfo >> RUNSTATE_SHIFT)&0x3)		/* Is thread runnable */
#define GET_BURIED(T)		(((T)->wInfo >> BURIED_SHIFT)&0x1)			/* Is thread 6 feet under */
#define GET_SLEEPING(T)		(((T)->wInfo >> SLEEPING_SHIFT)&0x1)		/* Is thread sleeping */
#define GET_DEBUGBLK(T)		(((T)->wInfo >> DEBUGBLK_SHIFT)&0x1)		/* Has DebugActive suspended thread */
#define GET_DYING(T)		(((T)->wInfo >> DYING_SHIFT)&0x1)			/* Has been set to die */
#define TEST_STACKFAULT(T)	((T)->wInfo & (1<<STACKFAULT_SHIFT))
#define GET_DEAD(T)			(((T)->wInfo >> DEAD_SHIFT)&0x1)
#define GET_NEEDDBG(T)		(((T)->wInfo >> NEEDDBG_SHIFT)&0x1)
#define GET_PROFILE(T)		(((T)->wInfo >> PROFILE_SHIFT)&0x1)			/* montecarlo profiling */
#define GET_NOPRIOCALC(T)	(((T)->wInfo >> NOPRIOCALC_SHIFT)&0x1)
#define GET_DEBUGWAIT(T)	(((T)->wInfo >> DEBUGWAIT_SHIFT)&0x1)
#define GET_USERBLOCK(T)	(((T)->wInfo >> USERBLOCK_SHIFT)&0x1)		/* did thread voluntarily block? */

#define SET_RUNSTATE(T,S)	((T)->wInfo = (WORD)(((T)->wInfo & ~(3<<RUNSTATE_SHIFT)) | ((S)<<RUNSTATE_SHIFT)))
#define SET_BURIED(T)		((T)->wInfo |= (1<<BURIED_SHIFT))
#define SET_SLEEPING(T)		((T)->wInfo |= (1<<SLEEPING_SHIFT))
#define CLEAR_SLEEPING(T)	((T)->wInfo &= ~(1<<SLEEPING_SHIFT))
#define SET_DEBUGBLK(T)		((T)->wInfo |= (1<<DEBUGBLK_SHIFT))
#define CLEAR_DEBUGBLK(T)	((T)->wInfo &= ~(1<<DEBUGBLK_SHIFT))
#define SET_DYING(T)		((T)->wInfo |= (1<<DYING_SHIFT))
#define SET_STACKFAULT(T)	((T)->wInfo |= (1<<STACKFAULT_SHIFT))
#define CLEAR_STACKFAULT(T)	((T)->wInfo &= ~(1<<STACKFAULT_SHIFT))
#define SET_DEAD(T)			((T)->wInfo |= (1<<DEAD_SHIFT))
#define SET_TIMEUSER(T)		((T)->wInfo &= ~(1<<TIMEMODE_SHIFT))
#define SET_TIMEKERN(T)		((T)->wInfo |= (1<<TIMEMODE_SHIFT))
#define SET_NEEDDBG(T)		((T)->wInfo |= (1<<NEEDDBG_SHIFT))
#define CLEAR_NEEDDBG(T)	((T)->wInfo &= ~(1<<NEEDDBG_SHIFT))
#define SET_PROFILE(T)		((T)->wInfo |= (1<<PROFILE_SHIFT))
#define CLEAR_PROFILE(T)	((T)->wInfo &= ~(1<<PROFILE_SHIFT))
#define SET_NOPRIOCALC(T)	((T)->wInfo |= (1<<NOPRIOCALC_SHIFT))
#define CLEAR_NOPRIOCALC(T)	((T)->wInfo &= ~(1<<NOPRIOCALC_SHIFT))
#define SET_DEBUGWAIT(T)	((T)->wInfo |= (1<<DEBUGWAIT_SHIFT))
#define CLEAR_DEBUGWAIT(T)	((T)->wInfo &= ~(1<<DEBUGWAIT_SHIFT))
#define SET_USERBLOCK(T)	((T)->wInfo |= (1<<USERBLOCK_SHIFT))
#define CLEAR_USERBLOCK(T)	((T)->wInfo &= ~(1<<USERBLOCK_SHIFT))

struct Thread {
	WORD      	wInfo;  	/* 00: various info about thread, see above */
	BYTE		bSuspendCnt;/* 02: thread suspend count */
    BYTE		bWaitState;	/* 03: state of waiting loop */
    LPPROXY		pProxList;	/* 04: list of proxies to threads blocked on this thread */
	PTHREAD		pNextInProc;/* 08: next thread in this process */	
    PPROCESS	pProc;		/* 0C: pointer to current process */
    PPROCESS	pOwnerProc;	/* 10: pointer to owner process */
    ACCESSKEY   aky;        /* 14: keys used by thread to access memory & handles */
    PCALLSTACK  pcstkTop;   /* 18: current api call info */
	DWORD       dwStackBase;/* 1C: stack base */
    DWORD		dwStackBound;/* 20: lower bound of commited stack space */
	LPDWORD     tlsPtr;     /* 24: tls pointer */
	DWORD		dwSleepCnt; /* 28: sleep count, also pending sleepcnt on waitmult */
    DWORD		dwKernTime;	/* 2C: elapsed kernel time */
    DWORD		dwUserTime;	/* 30: elapsed user time */
	LPPROXY		lpProxy;	/* 34: first proxy this thread is blocked on */
	DWORD       dwLastError;/* 38: last error */
    HANDLE		hTh;		/* 3C: Handle to this thread, needed by NextThread */
    BYTE		bBPrio;		/* 40: base priority */
    BYTE		bCPrio;		/* 41: curr priority */
	WORD		wCount;		/* 42: nonce for blocking lists */
	PTHREAD		pPrevInProc;/* 44: previous thread in this process */
	LPTHRDDBG	pThrdDbg;	/* 48: pointer to thread debug structure, if any */
	LPBYTE		pSwapStack;	/* 4c */
    FILETIME	ftCreate;	/* 50: time thread is created */
    CLEANEVENT *lpce;		/* 58: cleanevent for unqueueing blocking lists */
    CPUCONTEXT  ctx;        /* 5c: thread's cpu context information */
	PTHREAD		pNextSleepRun; /* ??: next sleeping thread, if sleeping, else next on runq if runnable */
	PTHREAD		pPrevSleepRun; /* ??: back pointer if sleeping or runnable */
	PTHREAD		pUpRun;		/* ??: up run pointer */
	PTHREAD		pDownRun;	/* ??: down run pointer */
	LPCRIT		pOwnedList; /* ??: list of crits and mutexes for priority inversion */
	LPCRIT		pOwnedHash[PRIORITY_LEVELS_HASHSIZE];
    DWORD		dwQuantum;	/* ??: thread quantum */
    DWORD		dwQuantLeft;/* ??: quantum left */
    LPPROXY		lpCritProxy;/* ??: proxy from last critical section block, in case stolen back */
	LPPROXY		lpPendProxy;/* ??: pending proxies for queueing */
	DWORD		dwPendTime;	/* ??: pending timeout delay */
	DWORD		dwPendReturn;/* ??: return value from pended wait */
	DWORD		dwPendStart;/* ??: time at which pendoperation started */
	PTHREAD 	pCrabPth;
	DWORD		dwCrabRem;
	HANDLE		hLastCrit;	/* ??: Last crit taken, cleared by nextthread */
	DWORD		dwCrabTime;
	DWORD		dwCrabCount;
	CALLSTACK	IntrStk;
   DWORD       dwStartAddr; /* thread PC at creation, used to get thread name */
};  /* Thread */

#define THREAD_CONTEXT_OFFSET   0x5c

ERRFALSE(THREAD_CONTEXT_OFFSET == offsetof(THREAD, ctx));

#define KSetLastError(pth, err) (pth->dwLastError = err)
#define KGetLastError(pth) (pth->dwLastError)

// Any time this structure is redefined, we need to recalculate
// the offset used in the SHx profiler ISR located at
// %_WINCEROOT%\platform\ODO\kernel\profiler\shx\profisr.src
struct Process {
    BYTE        procnum;    /* 00: ID of this process [ie: it's slot number] */
    BYTE		DbgActive;	/* 01: ID of process currently DebugActiveProcess'ing this process */
    BYTE		bChainDebug;/* 02: Did the creator want to debug child processes? */
    BYTE		bTrustLevel;/* 03: level of trust of this exe */
	LPPROXY		pProxList;	/* 04: list of proxies to threads blocked on this process */
    HANDLE		hProc;		/* 08: handle for this process, needed only for SC_GetProcFromPtr */
    DWORD       dwVMBase;   /* 0C: base of process's memory section, or 0 if not in use */
    PTHREAD     pTh;        /* 10: first thread in this process */
    ACCESSKEY   aky;        /* 14: default address space key for process's threads */
	LPVOID      BasePtr;    /* 18: Base pointer of exe load */
	HANDLE		hDbgrThrd;	/* 1C: handle of thread debugging this process, if any */
	LPWSTR      lpszProcName;/* 20: name of process */
	DWORD       tlsLowUsed; /* 24: TLS in use bitmask (first 32 slots) */
	DWORD       tlsHighUsed;/* 28: TLS in use bitmask (second 32 slots) */
	PEXCEPTION_ROUTINE pfnEH;/*2C: process exception handler */
	LPDBGPARAM  ZonePtr;    /* 30: Debug zone pointer */
	PTHREAD		pMainTh;	/* 34  primary thread in this process*/
	PMODULE		pmodResource;	/* 38: module that contains the resources */
	LPName		pStdNames[3]; /* 3C: Pointer to names for stdio */
	LPCWSTR		pcmdline;	/* 48: Pointer to command line */
	DWORD		dwDyingThreads;	/* 4C: number of pending dying threads */
	openexe_t   oe;			/* 50: Pointer to executable file handle */
	e32_lite    e32;        /* ??: structure containing exe header */
	o32_lite    *o32_ptr;   /* ??: o32 array pointer for exe */
	LPVOID		pExtPdata;
};  /* Process */

#define ProcStarted(P) (((P)->dwVMBase >> VA_SECTION) == ((DWORD)((P)->lpszProcName)>>VA_SECTION))

#define MIN_STACK_SIZE PAGE_SIZE
#define CNP_STACK_SIZE (64*1024) // the size of the stack when bootstrapping a new process
#define KRN_STACK_SIZE (64*1024) // the size of kernel thread stacks
// MIN_STACK_RESERVE is the minimum amount of stack space for the debugger and/or SEH code to run.
// When there is less than this amount of stack left and another stack page is committed,
// a stack overflow exception will be raised.
#if (PAGE_SIZE == 1024)
#define MIN_STACK_RESERVE 4096
#else
#define MIN_STACK_RESERVE 8192
#endif
#define MAX_STACK_RESERVE (1024*1024)

#define MapPtr(P) (!(P) || ((DWORD)(P)>>VA_SECTION) ? (LPVOID)(P) : \
		(LPVOID)((DWORD)(P)|(DWORD)pCurProc->dwVMBase))

#define MapPtrProc(Ptr, Proc) (!(Ptr) || ((DWORD)(Ptr)>>VA_SECTION) ? (LPVOID)(Ptr) : \
		(LPVOID)((DWORD)(Ptr)|(DWORD)(Proc)->dwVMBase))

extern PROCESS ProcArray[MAX_PROCESSES]; // in schedule.c

// dwFlags for MODULES
// #define DONT_RESOLVE_DLL_REFERENCES     0x00000001	// defined in winbase.h
// #define LOAD_LIBRARY_AS_DATAFILE        0x00000002	// defined in winbase.h
// #define LOAD_WITH_ALTERED_SEARCH_PATH   0x00000008	// defined in winbase.h

ERRFALSE(!(DONT_RESOLVE_DLL_REFERENCES & 0xffff0000));
ERRFALSE(!(LOAD_LIBRARY_AS_DATAFILE & 0xffff0000));
ERRFALSE(!(LOAD_WITH_ALTERED_SEARCH_PATH & 0xffff0000));

typedef struct Module {
	LPVOID lpSelf;					/* Self pointer for validation */
	PMODULE pMod;                   /* Next module in chain */
	LPWSTR lpszModName;              /* Module name */
	DWORD inuse;                    /* Bit vector of use */
	DWORD calledfunc;				/* Called entry but not exit */
	WORD refcnt[MAX_PROCESSES];/* Reference count per process*/
	LPVOID BasePtr;                 /* Base pointer of dll load (not 0 based) */
	DWORD DbgFlags;                 /* Debug flags */
	LPDBGPARAM ZonePtr;             /* Debug zone pointer */
	ulong startip;                  /* 0 based entrypoint */
	openexe_t oe;					/* Pointer to executable file handle */
	e32_lite e32;                   /* E32 header */
	o32_lite *o32_ptr;              /* O32 chain ptr */
	DWORD breadcrumb;
	DWORD dwNoNotify;				/* 1 bit per process, set if notifications disabled */
	WORD wFlags;
	BYTE bTrustLevel;
	BYTE bPadding;
	PMODULE pmodResource;			/* module that contains the resources */
} Module;

// List of all modules
#define pModList ((PMODULE)KInfoTable[KINX_MODULES])

#define HasModRefProcPtr(pMod,pProc) ((pMod)->refcnt[(pProc)->procnum] != 0)
#define HasModRefProcIndex(pMod,iProc) ((pMod)->refcnt[(iProc)] != 0)
#define AllowThreadMessage(pMod,pProc) (!((pMod)->dwNoNotify & (1 << (pProc)->procnum)))


typedef struct FSMAP *LPFSMAP;

typedef struct FSMAP {
	HANDLE hNext;		/* Next map in list */
	HANDLE hFile;		/* File, or INVALID_HANDLE_VALUE for just vm */
	LPBYTE pBase;		/* pointer to start of kernel mapped region */
	LPBYTE pDirty;		/* non-null if r/w real file, points to dirty bitmap */
	DWORD length;		/* length of mapped region */
	DWORD filelen;		/* length of file if hFile != INVALID_HANDLE_VALUE */
	DWORD reslen;		/* length of reservation */
	Name *name;			/* points to name of event */
	CLEANEVENT *lpmlist;/* List of mappings */
	DWORD dwDirty;		/* Count of dirty pages */
	BYTE bRestart;		/* Has been flushed */
	BYTE bNoAutoFlush;	/* Disallow automatic flushing */
	WORD wPad;
} FSMAP;

// Lowest DLL load address
#define DllLoadBase KInfoTable[KINX_DLL_LOW]

#define DBG_IS_DEBUGGER    0x00000001
#define DBG_SYMBOLS_LOADED 0x80000000

#define PAGEALIGN_UP(X) (((X)+(PAGE_SIZE-1))&~(PAGE_SIZE-1))
#define PAGEALIGN_DOWN(X) ((X)&~(PAGE_SIZE-1))

// LOG_MAGIC, mem_t, fslogentry_t, and fslog_t MUST be kept in sync with filesys.h version

#define LOG_MAGIC 0x4d494b45

typedef struct mem_t {
	DWORD startptr;
	DWORD length;
	DWORD extension;
} mem_t;

typedef struct fslogentry_t {
	DWORD type;
	DWORD d1, d2, d3;
} fslogentry_t;

typedef struct fshash_t {
	DWORD hashdata[4];
} fshash_t;

#define CURRENT_FSLOG_VERSION 1

// MUST be kept in sync with data structure in coreos\filesys\filesys.h

#pragma warning(disable:4200) // nonstandard extensions warning

typedef struct fslog_t {
	DWORD version;		// version of this structure, must stay as first DWORD
	DWORD magic1;		// LOG_MAGIC if memory tables valid
	DWORD magic2;		// LOG_MAGIC if heap initialized
	union {
		struct {
			mem_t fsmemblk[2]; 	// Memory blocks to use for file system
			LPBYTE pFSList;
			LPBYTE pKList;
		};
		struct {
			CEGUID guid;
			DWORD dwRestoreFlags;
			DWORD dwRestoreStart;
			DWORD dwRestoreSize;
		};
	};	
	DWORD virtbase;		// VirtBase when last booted
	DWORD entries;		// number of entries in recovery log
	DWORD hDbaseRoot;   // handle to first dbase, else 0
	DWORD hReg;		// Handle to registry header
	DWORD dwReplMask;	// replication mask
	DWORD flags;		// file system flags
	fshash_t pwhash;	// hashed password
	fslogentry_t log[];	// log entries
} fslog_t;

#pragma warning(default:4200) // nonstandard extensions warning

LPVOID AllocMem(ulong poolnum);
VOID FreeMem(LPVOID pMem, ulong poolnum);
LPName AllocName(DWORD dwLen);
#define FreeName(lpn) FreeMem(lpn,lpn->wPool);
PHYSICAL_ADDRESS GetHeldPage(void);
PHYSICAL_ADDRESS GetReservedPage(void);
BOOL HoldPages(int cpNeed, BOOL bForce);
void DupPhysPage(PHYSICAL_ADDRESS paPFN);
void FreePhysPage(PHYSICAL_ADDRESS paPFN);
LPVOID InitNKSection(void);
BOOL AutoCommit(ulong addr);
BOOL DemandCommit(DWORD);
void GuardCommit(ulong addr);
BOOL ProcessPageFault(BOOL bWrite, ulong addr);
void FreeSection(PSECTION pscn);
PSECTION GetSection(void);
LPVOID GetHelperStack(void);
void FreeHelperStack(LPVOID);
BOOL CreateMapperSection(DWORD dwBase);
void DeleteMapperSection(DWORD dwBase);
void CloseMappedFileHandles();
void FreeIntrFromEvent(LPEVENT lpe);
void EnterPhysCS(void);
BOOL ScavengeStacks(int cNeed);
void CreateNewProc(ulong nextfunc, ulong param);
void KernelInit2(void);
void AlarmThread(LPVOID param);
void RunApps(ulong param);
#if x86
void InitNPXHPHandler(LPVOID);
void InitializeEmx87(void);
void InitializeFPU(void);
extern DWORD dwFPType;
#endif

/** Return values from AutoCommit(...): */
#define AUTO_COMMIT_RESCHED 0
#define AUTO_COMMIT_HANDLED 1
#define AUTO_COMMIT_FAULT   2

void SetCPUASID(PTHREAD pth);
#ifdef SHx
void SetCPUGlobals(void);
#endif

LPVOID VerifyAccess(LPVOID pvAddr, DWORD dwFlags, ACCESSKEY aky);
BOOL IsAccessOK(void *addr, ACCESSKEY aky);

void KUnicodeToAscii(LPCHAR, LPCWSTR, int);
void KAsciiToUnicode(LPWSTR wchptr, LPBYTE chptr, int maxlen);

LPVOID GetOneProcAddress(HANDLE, LPCVOID, BOOL);
HANDLE LoadOneLibraryW(LPCWSTR,BOOL,WORD);
PMODULE LoadOneLibraryPart2(LPWSTR lpszFileName, BOOL bAllowPaging, WORD dwFlags);
VOID FreeAllProcLibraries(PPROCESS);
PVOID PDataFromPC(ULONG pc, PPROCESS pProc, PULONG pSize);
LPVOID SC_GetProcAddressW(HANDLE hInst, LPWSTR lpszProc);
LPVOID SC_GetProcAddressA(HANDLE hInst, LPCSTR lpszProc);
BOOL SC_DisableThreadLibraryCalls(PMODULE hLibModule);

LPWSTR SC_GetCommandLineW(VOID);
PFNVOID DBG_CallCheck(PTHREAD pth, DWORD dwJumpAddress);
#define DBG_ReturnCheck(pth) ((pth)->pcstkTop ? (pth)->pcstkTop->retAddr : 0)
BOOL UserDbgTrap(EXCEPTION_RECORD *er, PCONTEXT pctx, BOOL bFirst);
void SurrenderCritSecs(void);
BOOL SetThreadBasePrio(HANDLE hth, DWORD nPriority);
HANDLE EventModIntr(LPEVENT lpe, DWORD type);
VOID MakeRunIfNeeded(HANDLE hth);

extern void (*TBFf)(LPVOID, ulong);
extern void (*MTBFf)(LPVOID, ulong, ulong, ulong, ulong);
extern void (*CTBFf)(LPVOID, ulong, ulong, ulong, ulong);
extern BOOL (*KSystemTimeToFileTime)(LPSYSTEMTIME, LPFILETIME);
extern LONG (*KCompareFileTime)(LPFILETIME, LPFILETIME);
extern BOOL (*KFileTimeToSystemTime)(const FILETIME *, LPSYSTEMTIME);
extern BOOL (*KLocalFileTimeToFileTime)(const FILETIME *, LPFILETIME);
extern void (*pPSLNotify)(DWORD, DWORD, DWORD);
extern BOOL (*pGetHeapSnapshot)(THSNAP *pSnap, BOOL bMainOnly, LPVOID *pDataPtr);
extern HANDLE (*pGetProcessHeap)(void);

#ifdef IN_KERNEL
extern ROMHDR * const volatile pTOC;
#endif

void KernelRelocate(ROMHDR *const pTOC);
void KernelFindMemory(void);

extern ROMChain_t *ROMChain;

DWORD CECompress(LPBYTE  lpbSrc, DWORD cbSrc, LPBYTE lpbDest, DWORD cbDest, WORD wStep, DWORD dwPagesize);
DWORD CEDecompress(LPBYTE  lpbSrc, DWORD cbSrc, LPBYTE  lpbDest, DWORD cbDest, DWORD dwSkip, WORD wStep, DWORD dwPagesize);

typedef struct LSInfo_t {
	PTHREAD pHelper;// must be first
	HTHREAD hth;	// hthread to start
	DWORD startip;	// starting ip after switch
	DWORD flags;	// creation flags
	LPVOID lpStack;	// pointer to new stack
	LPVOID lpOldStack; // pointer to old stack
	DWORD cbStack;	// bytes in stack
	DWORD cmdlen;	// length of command line
	DWORD namelen;	// length of process name
	HANDLE hEvent;	// event to signal
} LSInfo_t;

extern HANDLE hAlarmThreadWakeup;
DWORD ThreadSuspend(PTHREAD hTh);
void BlockWithHelper(FARPROC pFunc, FARPROC pHelpFunc, LPVOID param);
void BlockWithHelperAlloc(FARPROC pFunc, FARPROC pHelpFunc, LPVOID param);

int ropen(WCHAR *name, int mode);
int rread(int fd, char *buf, int cnt);
int rreadseek(int fd, char *buf, int cnt, int off);
int rlseek(int fd, int off, int mode);
int rclose(int fd);
int rwrite(int fd, char *buf, int cnt);

void kstrcpyW(LPWSTR p1, LPCWSTR p2);
unsigned int strlenW(LPCWSTR str);
unsigned int strlen(const char *str);
int kstrcmpi(LPWSTR str1, LPWSTR str2);
int strcmpW(LPCWSTR lpu1, LPCWSTR lpu2);
int strcmp(const char *lpc1, const char *lpc2);

LPVOID memset(LPVOID dest, int c, unsigned int count);
LPVOID memcpy(LPVOID dest, const void *src, unsigned int count);

#ifdef x86
void InvalidateRange(PVOID, ULONG);
#else
void FlushTLB(void);
#define InvalidateRange(x, y) FlushTLB()
#endif

#if defined(WIN32_CALL) && !defined(KEEP_SYSCALLS)
#undef InputDebugCharW
int WINAPI InputDebugCharW(void);
#undef OutputDebugStringW
VOID WINAPI OutputDebugStringW(LPCWSTR str);
#undef NKvDbgPrintfW
VOID NKvDbgPrintfW(LPCWSTR lpszFmt, CONST VOID * lpParms);
#endif

#if defined(SH3)
void FlushCache(void);
void FlushCacheNoDiscard(void);
#elif defined(x86)
void FlushCache(void);
#else
void FlushICache(void);
void FlushDCache(void);
#endif

void DumpStringToSerial(LPCWSTR);
HANDLE DoCreateThread(LPVOID lpStack, DWORD cbStack, LPVOID lpStart, LPVOID param, DWORD flags, LPDWORD lpthid, ulong mode, WORD prio);
LPVOID CreateSection(LPVOID lpvAddr);
VOID DeleteSection(LPVOID lpvSect);
LPVOID HugeVirtualReserve(DWORD dwSize);
BOOL HugeVirtualRelease(LPVOID pMem);
void InitMUILanguages(void);
int NKwvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, CONST VOID *lpParms, int maxchars);

BOOL DoThreadSetContext(HANDLE hTh, const CONTEXT *lpContext);
BOOL DoThreadGetContext(HANDLE hTh, LPCONTEXT lpContext);

extern fslog_t *LogPtr;

#ifdef IN_KERNEL
DWORD PerformCallBack4Int(CALLBACKINFO *pcbi, ...);
#undef ResumeThread
#define ResumeThread SC_ThreadResume
#undef SuspendThread
#define SuspendThread SC_ThreadSuspend
#undef GetTickCount
#define GetTickCount SC_GetTickCount
#undef CreateFileForMappingW
#define CreateFileForMappingW SC_CreateFileForMapping
#undef CreateFileMapping
#define CreateFileMapping SC_CreateFileMapping
#undef MapUncompressedFileW
#define MapUncompressedFileW SC_MapUncompressedFileW
#undef MapViewOfFile
#define MapViewOfFile SC_MapViewOfFile
#undef UnmapViewOfFile
#define UnmapViewOfFile SC_UnmapViewOfFile
#undef VirtualAlloc
#define VirtualAlloc SC_VirtualAlloc
#undef VirtualQuery
#define VirtualQuery SC_VirtualQuery
#undef VirtualProtect
#define VirtualProtect SC_VirtualProtect
#undef VirtualCopy
#define VirtualCopy DoVirtualCopy
#undef VirtualSetPageFlags
#define VirtualSetPageFlags SC_VirtualSetPageFlags
#undef LockPages
#define LockPages DoLockPages
#undef UnlockPages
#define UnlockPages DoUnlockPages
#undef VirtualFree
#define VirtualFree SC_VirtualFree
#undef SetThreadPriority	// don't use - confusing with two sets of prio numbers
#undef TakeCritSec
#define TakeCritSec SC_TakeCritSec
#undef LeaveCritSec
#define LeaveCritSec SC_LeaveCritSec
#undef CreateCrit
#define CreateCrit SC_CreateCrit
#undef GetProcAddressA
#define GetProcAddressA SC_GetProcAddressA
#undef GetProcAddressW
#define GetProcAddressW SC_GetProcAddressW
#undef CreateProcessW
#define CreateProcessW SC_CreateProc
#undef WaitForMultipleObjects
#define WaitForMultipleObjects SC_WaitForMultiple
#undef CreateEventW
#define CreateEventW SC_CreateEvent
#undef EventModify
#define EventModify SC_EventModify
#undef Sleep
#define Sleep SC_Sleep
#undef THGrow
#define THGrow SC_THGrow
#undef SuspendThread
#define SuspendThread SC_ThreadSuspend
#undef SetHandleOwner
#define SetHandleOwner SC_SetHandleOwner
#define SetEvent(h) EventModify(h,EVENT_SET)
#define ResetEvent(h) EventModify(h,EVENT_RESET)
#define PulseEvent(h) EventModify(h, EVENT_PULSE)
#undef SetLastError
#define SetLastError(err) (pCurThread->dwLastError = err)
#undef GetLastError
#define GetLastError() (pCurThread->dwLastError)
#undef CacheSync
#define CacheSync SC_CacheSync
#undef GetModuleFileName
#define GetModuleFileName SC_GetModuleFileNameW
#undef LoadLibraryEx
#define LoadLibraryEx SC_LoadLibraryExW
#undef FreeLibrary
#define FreeLibrary SC_FreeLibrary
#undef NKTerminateThread
#define NKTerminateThread SC_NKTerminateThread
#undef PerformCallBack4
#define PerformCallBack4 SC_PerformCallBack4
#undef RegCloseKey
#define RegCloseKey SC_RegCloseKey
#undef CreateFileW
#define CreateFileW SC_CreateFileW
#undef ReadFile
#define ReadFile SC_ReadFile
#undef ReadFileWithSeek
#define ReadFileWithSeek SC_ReadFileWithSeek
#undef WriteFileWithSeek
#define WriteFileWithSeek SC_WriteFileWithSeek
#undef RegOpenKeyExW
#define RegOpenKeyExW SC_RegOpenKeyExW
#undef RegQueryValueExW
#define RegQueryValueExW SC_RegQueryValueExW
#undef RegEnumValueW
#define RegEnumValueW PRIV_WIN32_FS_CALL(LONG, 21, (HKEY hKey, DWORD dwIndex, 	LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData))
#undef CeOidGetInfoEx
#define CeOidGetInfoEx PRIV_WIN32_FS_CALL(BOOL, 12, (PCEGUID pguid, CEOID oid, CEOIDINFO *oidInfo))
#undef FindFirstFileW
#define FindFirstFileW PRIV_WIN32_FS_CALL(HANDLE, 8, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))
#undef GetFileInformationByHandle
#define GetFileInformationByHandle PRIV_WIN32_FILE_CALL(DWORD, 6, (HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation))
#undef FlushFileBuffers
#define FlushFileBuffers PRIV_WIN32_FILE_CALL(BOOL, 7, (HANDLE hFile))
#undef WriteFile
#define WriteFile PRIV_WIN32_FILE_CALL(BOOL, 3, (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped))
#undef SetFilePointer
#define SetFilePointer PRIV_WIN32_FILE_CALL(DWORD, 5, (HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod))
#undef SetEndOfFile
#define SetEndOfFile PRIV_WIN32_FILE_CALL(BOOL, 10, (HANDLE hFile))
#undef CreateDirectoryW
#define CreateDirectoryW PRIV_WIN32_FS_CALL(BOOL, 2, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes))
#endif

/* Kernel zones */

extern DBGPARAM dpCurSettings;

#define ZONE_SCHEDULE   DEBUGZONE(0)	/* 0x0001 */
#define ZONE_MEMORY     DEBUGZONE(1)	/* 0x0002 */
#define ZONE_OBJDISP    DEBUGZONE(2)	/* 0x0004 */
#define ZONE_DEBUGGER   DEBUGZONE(3)	/* 0x0008 */
#define ZONE_NEXTTHREAD DEBUGZONE(4)	/* 0x0010 */
#define ZONE_LOADER1    DEBUGZONE(5)	/* 0x0020 */
#define ZONE_VIRTMEM    DEBUGZONE(6)	/* 0x0040 */
#define ZONE_LOADER2    DEBUGZONE(7)	/* 0x0080 */
#define ZONE_DEBUG      DEBUGZONE(8)	/* 0x0100 */
#define ZONE_GETINFO    DEBUGZONE(9)	/* 0x0200 */
#define ZONE_PHYSMEM    DEBUGZONE(10)	/* 0x0400 */
#define ZONE_SEH        DEBUGZONE(11)	/* 0x0800 */
#define ZONE_OPENEXE    DEBUGZONE(12)	/* 0x1000 */
#define ZONE_MEMTRACKER DEBUGZONE(13)	/* 0x2000 */
#define ZONE_PAGING		DEBUGZONE(14)	/* 0x4000 */
#define ZONE_ENTRY		DEBUGZONE(15)	/* 0x8000 */

/* Kernel Debugger interfaces */
extern BOOLEAN (*PKDInit)(LPVOID *, LPVOID *, LPVOID *, LPVOID, LPVOID *, LPVOID *);
extern ULONG (*KDTrap)(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN CONTEXT *ContextRecord,
    IN BOOLEAN SecondChance);
extern BOOLEAN (*KDPrintString)(IN LPCWSTR Output);
extern void (*KDUpdateSymbols)(DWORD dwAddr, BOOL bUnload);
extern BOOL ReadyForStrings;

BOOLEAN NKDispatchException(PTHREAD pth, PEXCEPTION_RECORD ExceptionRecord, PCONTEXT ContextRecord);

PVOID DbgVerify(PVOID pvAddr, int option);

#define DV_PROBE	0	// probe address for read access
#define DV_MODIFY	1	// probe address for write access
#define DV_SETBP	2	// prepare address for breakpoint (lock if necessary)
#define DV_CLEARBP	3	// undo break breakpoint (unlock)

#ifndef COREDLL

typedef struct kernptr_t {
	void (* OEMClearDebugCommError)(void);
	void (* OEMWriteDebugByte)(unsigned char ch);
	int (* OEMReadDebugByte)(void);
	PVOID (* DbgVerify)(PVOID pvAddr, int flags);
	PFNVOID (* DBG_CallCheck)(PTHREAD pth, DWORD dwJumpAddress);
    void (* OEMWriteOtherDebugString)(unsigned char * ch);
	struct KDataStruct *pKData;
#if defined(SHx) || defined(x86)
	void (* FlushCache)(void);
#else
	void (* FlushDCache)(void);
	void (* FlushICache)(void);
#endif
#if defined(MIPS_HAS_FPU) || defined(SH4)
	void (* FPUFlushContext)(void);
#endif
} kerndata_t;

#endif

//
//  Useful
//
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define LAST_ELEMENT(x) (&x[ARRAY_SIZE(x)-1])
#define CCHSIZEOF(sz)	(sizeof(sz)/sizeof(TCHAR))

DWORD WireBuffer(LPBYTE buf, DWORD len);
#define UnWireBuffer(buf,len) (0)

#define PageFreeCount ((long)KInfoTable[KINX_PAGEFREE])

int rRegGet(DWORD hKey, CHAR *szName, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpdwSize);
int rRegOpen(DWORD hKey, CHAR *szName, LPDWORD lphKey);
int rRegClose(DWORD hKey);
int rRegEnum(DWORD hKey, DWORD dwIndex, LPBYTE lpbData, LPDWORD lpdwSize);

// DList - double linked list
//
// WARNING: The double list routine are NOT preemtion safe. The list must
// be protected with a critical section or the functions should be invoked
// via KCall().

typedef struct _DList DList;
struct _DList {
	DList *fwd;
	DList *back;
};

// Insert an item into a double linked list
void AddToDList(DList *head, DList *item);

// Remove an item from a double linked list
void RemoveDList(DList *item);

// REFINFO - reference info for Handle Data.
typedef struct FULLREF {
	ushort	usRefs[MAX_PROCESSES];
} FULLREF;
typedef union REFINFO {
	ulong	count;
	FULLREF	*pFr;
} REFINFO;

// HDATA - handle data structure
typedef struct _HDATA HDATA, *PHDATA;
struct _HDATA {
	DList		linkage;	/* 00: links for active handle list */
	HANDLE		hValue;		/* 08: Current value of handle (nonce) */
    ACCESSLOCK  lock;      	/* 0C: access information */
    REFINFO		ref;		/* 10: reference information */
    const CINFO *pci;       /* 14: ptr to object class description structure */
	PVOID		pvObj;		/* 18: ptr to object */
	DWORD		dwInfo;		/* 1C: extra handle info */
};							/* 20: sizeof(HDATA) */

#define HANDLE_ADDRESS_MASK	0x1ffffffc

HANDLE AllocHandle(const CINFO *pci, PVOID pvObj, PPROCESS pprc);
BOOL FreeHandle(HANDLE h);
PHDATA HandleToPointer(HANDLE h);
#define PointerToHandle(phd) ((phd)->hValue)

int GetRef(HANDLE h, PPROCESS pprc);

// Returns FALSE if handle not valid or refcnt==0.
BOOL IncRef(HANDLE h, PPROCESS pprc);

// Returns TRUE if all references removed.
BOOL DecRef(HANDLE h, PPROCESS pprc, BOOL fAll);

// Returns 0 if handle is not valid.
DWORD GetUserInfo(HANDLE h);

// Returns FALSE if handle is not valid.
BOOL SetUserInfo(HANDLE h, DWORD info);

// Returns NULL if handle is not valid.
PVOID GetObjectPtr(HANDLE h);

// Returns NULL if handle is not valid or not correct type.
PVOID GetObjectPtrByType(HANDLE h, int type);

// Returns NULL if handle is not valid or not correct type or wrong permissions
PVOID GetObjectPtrByTypePermissioned(HANDLE h, int type);

// Returns FALSE if handle is not valid.
BOOL SetObjectPtr(HANDLE h, PVOID pvObj);

// Returns 0 if handle is not valid.
int GetHandleType(HANDLE h);

// Force a reschedule when KCall returns.
#define SetReschedule()	(KCResched = 1)

// Call a function in non-preemtible kernel mode.
//   Returns the return value from the function.
typedef int (*PKFN)();
int KCall(PKFN pfn, ...);

#define HandleToThread(h) ((THREAD *)GetObjectPtrByType((h),SH_CURTHREAD))
#define HandleToThreadPerm(h) ((THREAD *)GetObjectPtrByTypePermissioned((h),SH_CURTHREAD))
#define HandleToProc(h) ((PROCESS *)GetObjectPtrByType((h),SH_CURPROC))
#define HandleToEvent(h) ((EVENT *)GetObjectPtrByType((h),HT_EVENT))
#define HandleToEventPerm(h) ((EVENT *)GetObjectPtrByTypePermissioned((h),HT_EVENT))
#define HandleToMutex(h) ((MUTEX *)GetObjectPtrByType((h),HT_MUTEX))
#define HandleToMutexPerm(h) ((MUTEX *)GetObjectPtrByTypePermissioned((h),HT_MUTEX))
#define HandleToMap(h) ((FSMAP *)GetObjectPtrByType((h),HT_FSMAP))
#define HandleToMapPerm(h) ((FSMAP *)GetObjectPtrByTypePermissioned((h),HT_FSMAP))
#define HandleToAPISet(h) ((APISET *)GetObjectPtrByType((h),HT_APISET))
#define HandleToAPISetPerm(h) ((APISET *)GetObjectPtrByTypePermissioned((h),HT_APISET))
#define HandleToSem(h) ((SEMAPHORE *)GetObjectPtrByType((h),HT_SEMAPHORE))

// Test if a value is a handle or a pointer.
//	NOTE: NULL is considered to be a pointer.
#define IsHandle(v)	(((int)(v) & 0x02) != 0)

#include "heap.h"


#define MAX_KCALL_PROFILE 76
#define CELOG_KCALL_ID    75

#include "celognk.h"

#if defined(KCALL_PROFILE)

typedef struct KPRF_t {
	DWORD hits;
	DWORD max;
	DWORD min;
	DWORD total;
	DWORD tmp;
} KPRF_t;

extern KPRF_t KPRFInfo[MAX_KCALL_PROFILE];

#ifdef NKPROF
extern BOOL bProfileKCall;
#endif
_inline void KCALLPROFON(int IND) {
	LARGE_INTEGER liPerf;
#ifdef NKPROF
	if (bProfileKCall) {
#endif
	DEBUGCHK(InSysCall());
	DEBUGCHK(!KPRFInfo[IND].tmp);
	DEBUGCHK(IND<MAX_KCALL_PROFILE);
	SC_QueryPerformanceCounter(&liPerf);
	KPRFInfo[IND].tmp = liPerf.LowPart;
#ifdef NKPROF
	}
   CELOG_KCallEnter(IND);
#endif
}

_inline void KCALLPROFOFF(int IND) {
	LARGE_INTEGER liPerf;
	DWORD t2;
#ifdef NKPROF
	if (bProfileKCall) {
#endif
	DEBUGCHK(InSysCall());
	DEBUGCHK(KPRFInfo[IND].tmp);
	DEBUGCHK(IND<MAX_KCALL_PROFILE);
	SC_QueryPerformanceCounter(&liPerf);
	t2 = liPerf.LowPart - KPRFInfo[IND].tmp;
	KPRFInfo[IND].tmp = 0;
	if (t2 > KPRFInfo[IND].max)
		KPRFInfo[IND].max = t2;
	if (t2 && (!KPRFInfo[IND].min || (t2 < KPRFInfo[IND].min)))
		KPRFInfo[IND].min = t2;
	KPRFInfo[IND].total += t2;
	KPRFInfo[IND].hits++;
#ifdef NKPROF
	}
   CELOG_KCallLeave(IND);
#endif
}

#else

#define KCALLPROFON(IND) 0
#define KCALLPROFOFF(IND) 0

#endif

#ifdef	IN_KERNEL	// if we are in the kernel

#ifndef	SHIP_BUILD	// if not SHIP_BUILD
#ifdef	DEBUG		// and is DEBUG

#ifdef	DBGCHK		// if this is already defined
#undef	DBGCHK		// then undef it
#endif	// DBGCHK

#define DBGCHK(module,exp) \
   ((void)((exp)?1:(          \
       NKDbgPrintfW ( TEXT("%s: DEBUGCHK failed in file %s at line %d \r\n"), \
                 (LPWSTR)module, TEXT(__FILE__) ,__LINE__ ),    \
	   ((InDebugger)?1:(DebugBreak())), \
	   0  \
   )))


#endif	// DEBUG
#endif	// SHIP_BUILD

#endif	// IN_KERNEL

#endif

