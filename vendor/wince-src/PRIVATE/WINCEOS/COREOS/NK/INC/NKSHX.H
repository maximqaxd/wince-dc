/*++ BUILD Version: 0016    Increment this if a change has global effects

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    nkshx.h

Abstract:

    User-mode visible SH3 specific structures and constants

--*/

#ifndef _NKSHX_
#define _NKSHX_

// begin_ntddk begin_nthal

#if defined(SHx)

#define _cdecl

// begin_windbgkd

#if defined(SHx)

//
// Define SHx specific kernel debugger information.
//
// The following structure contains machine specific data passed to
// the host system kernel debugger in a wait state change message.
//

#define DBGKD_MAXSTREAM 16

typedef struct _DBGKD_CONTROL_REPORT {
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;

//
// The following structure contains information that the host system
// kernel debugger wants to set on every continue operation and avoids
// the need to send extra packets of information.
//

typedef ULONG DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;

#endif                          // ntddk nthal

// end_windbgkd


//
// Define breakpoint codes.
//

#define USER_BREAKPOINT 0                   // user breakpoint
#define KERNEL_BREAKPOINT 1                 // kernel breakpoint
#define BREAKIN_BREAKPOINT 2                // break into kernel debugger
#define BRANCH_TAKEN_BREAKPOINT 3           // branch taken breakpoint
#define BRANCH_NOT_TAKEN_BREAKPOINT 4       // branch not taken breakpoint
#define SINGLE_STEP_BREAKPOINT 5            // single step breakpoint
#define DIVIDE_OVERFLOW_BREAKPOINT 6        // divide overflow breakpoint
#define DIVIDE_BY_ZERO_BREAKPOINT 7         // divide by zero breakpoint
#define RANGE_CHECK_BREAKPOINT 8            // range check breakpoint
#define STACK_OVERFLOW_BREAKPOINT 9         // MIPS code
#define MULTIPLY_OVERFLOW_BREAKPOINT 10     // multiply overflow breakpoint

#define DEBUG_PRINT_BREAKPOINT 20           // debug print breakpoint
#define DEBUG_PROMPT_BREAKPOINT 21          // debug prompt breakpoint
//#define DEBUG_STOP_BREAKPOINT 22            // debug stop breakpoint
//#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 23    // load symbols breakpoint
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 24  // unload symbols breakpoint

// begin_ntddk begin_nthal
//
// Define size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 2048

//
// Define length of exception code dispatch vector.
//

#define XCODE_VECTOR_LENGTH 32

//
// Define length of interrupt vector table.
//

#define MAXIMUM_VECTOR 32


#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Fir)

#define CONTEXT_LENGTH (sizeof(CPUCONTEXT))
#define CONTEXT_ALIGN (sizeof(ULONG))
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

// begin_nthal
//
// Define address space layout as defined by SHx memory management.
//

#define KUSEG_BASE 0x0                  // base of user segment
#define KSEG0_BASE 0x80000000           // base of cached kernel physical
#define KSEG1_BASE 0xa0000000           // base of uncached kernel physical
#define KSEG2_BASE 0xc0000000           // base of cached kernel virtual
// end_nthal


//
// Define SHx exception handling structures and function prototypes.
//
// Function table entry structure definition.
//

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PEXCEPTION_ROUTINE ExceptionHandler;
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

//
// Scope table structure definition.
//

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;

//
// Runtime Library function prototypes.
//

VOID
RtlCaptureContext (
    OUT PCONTEXT ContextRecord
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG ControlPc
    );

ULONG
RtlVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame
    );

//
// Define C structured exception handing function prototypes.
//

struct _EXCEPTION_POINTERS;

typedef
LONG
(*EXCEPTION_FILTER) (
    ULONG EstablisherFrame,
    struct _EXCEPTION_POINTERS *ExceptionPointers
    );

typedef
VOID
(*TERMINATION_HANDLER) (
    ULONG EstablisherFrame,
    BOOLEAN is_abnormal
    );

// begin_winnt

#if defined(SHx)

VOID
__jump_unwind (
    PVOID Fp,
    PVOID TargetPc
    );

#endif // SHx

// end_winnt

// begin_ntddk begin_nthal

#define CPUCONTEXT CONTEXT /* only need integer context */
#define retValue ctx.R0
#define ARG0    ctx.R4
#define SetThreadIP(pth, addr) ((pth)->ctx.Fir = (ULONG)(addr))
#define GetThreadIP(pth) ((pth)->ctx.Fir)

/* Macros for handling stack shrinkage. */
#define MDTestStack(pth)    (((pth)->ctx.R15 < 0x80000000          \
        && ((pth)->dwStackBound>>VA_PAGE) < (((pth)->ctx.R15-32)>>VA_PAGE))  \
        ? (pth)->dwStackBound : 0)

#define MDShrinkStack(pth)  ((pth)->dwStackBound += PAGE_SIZE)


#include "mem_shx.h"
#define INVALID_POINTER_VALUE	0xC0000000










struct KDataStruct {
	LPDWORD	lpvTls;			/* 0x000 Current thread local storage pointer */
    HANDLE 	ahSys[NUM_SYS_HANDLES]; /* 0x004 If this moves, change kapi.h */
    char    bResched;       /* 0x084 reschedule flag */
    char    cNest;          /* 0x085 kernel exception nesting */
    char    bPowerOff;      /* 0x086 TRUE during "power off" processing */
    char    bProfileOn;     /* 0x087 TRUE if profiling enabled */
    ulong   cMsec;          /* 0x088 # of milliseconds since boot */
    ulong   cDMsec;         /* 0x08c # of mSec since last TimerCallBack */
	PPROCESS pCurPrc;		/* 0x090 ptr to current PROCESS struct */
	PTHREAD	pCurThd;		/* 0x094 ptr to current THREAD struct */
	DWORD	dwKCRes;	    /* 0x098 */
	ulong	handleBase;		/* 0x09c handle table base address */
	PSECTION aSections[64]; /* 0x0a0 section table for virtual memory */
	LPEVENT alpeIntrEvents[SYSINTR_MAX_DEVICES];/* 0x1a0 */
	LPVOID  alpvIntrData[SYSINTR_MAX_DEVICES];  /* 0x220 */
	ulong	pAPIReturn;		/* 0x2a0 direct API return address for kernel mode */
	DWORD	dwPad2;			/* 0x2a4 Next interrupt index to read */
	DWORD	dwPad3;			/* 0x2a8 Next interrupt index to write */
	BYTE	bPad[32];		/* 0x2ac Circular buffer of interrupt id's */
	PTHREAD g_CurFPUOwner;  /* 0x2cc curfpuowner */
	DWORD	dwInDebugger;	/* 0x2d0 - !0 when in debugger */
	long    alPad[11];      /* 0x2d4 - padding */
	DWORD   aInfo[32];      /* 0x300 - misc. kernel info */
	                        /* 0x380 - interlocked api code */
	                        /* 0x400 - end */
};  /* KDataStruct */

#ifdef BUILDING_DEBUGGER
extern struct KDataStruct *kdpKData;
#define KData  (*(struct KDataStruct *)kdpKData)
#else
extern struct KDataStruct KData;
#endif

extern DWORD InterruptTable[];
extern volatile ulong CurMSec;

#define hCurThread   (KData.ahSys[SH_CURTHREAD])
#define hCurProc     (KData.ahSys[SH_CURPROC])
#define pCurThread	(KData.pCurThd)
#define pCurProc	(KData.pCurPrc)
#define ReschedFlag (KData.bResched)
#define PowerOffFlag (KData.bPowerOff)
#define ProfileFlag (KData.bProfileOn)
#define DiffMSec    (KData.cDMsec)
#define CurAKey     (pCurThread->aky)
#define SectionTable (KData.aSections)
#define InSysCall()	(KData.cNest != 1)
#define IntrEvents  (KData.alpeIntrEvents)
#define IntrData    (KData.alpvIntrData)
#define KPlpvTls	(KData.lpvTls)
#define KInfoTable  (KData.aInfo)
#define DIRECT_RETURN (KData.pAPIReturn)
#define g_CurFPUOwner (KData.g_CurFPUOwner)     // SH4 only
#define g_CurDSPOwner g_CurFPUOwner             // SH3(DSP) only
#define KCResched	(KData.dwKCRes)
#define	InDebugger	(KData.dwInDebugger)

extern void INTERRUPTS_ON(void);
extern void INTERRUPTS_OFF(void);

#define InterlockedDecrement \
        ((LONG (*)(LPLONG lpAddend))(PUserKData+0x3f0))
#define InterlockedIncrement \
        ((LONG (*)(LPLONG lpAddend))(PUserKData+0x3e0))
#define InterlockedTestExchange \
        ((LONG (*)(LPLONG Target, LONG oldValue, LONG newValue))\
        (PUserKData+0x3d0))
#define InterlockedExchange \
        ((LONG (*)(LPLONG Target, LONG Value))(PUserKData+0x3c0))

#define InterlockedPushList \
		((void *(*)(void *pHead, void *pItem))(PUserKData+0x3b0))
#define InterlockedPopList \
		((void *(*)(void *pHead))(PUserKData+0x390))

#define FIRST_INTERLOCK 0x380

// Defines for CPU specific IDs.
#define PROCESSOR_ARCHITECTURE PROCESSOR_ARCHITECTURE_SHX
#if defined(SH3) || defined(SH3e)
#define THISCPUID IMAGE_FILE_MACHINE_SH3
#define CEProcessorType PROCESSOR_HITACHI_SH3
#define ProcessorLevel	3
#elif defined(SH4)
#define THISCPUID IMAGE_FILE_MACHINE_SH4
#define CEProcessorType PROCESSOR_HITACHI_SH4
#define ProcessorLevel	4
#endif
#define ProcessorRevision 0

#endif // defined(SHx)
// end_ntddk end_nthal

#endif // _NKSHX_
