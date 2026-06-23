/*++

Copyright (c) 1993  IBM Corporation

Module Name:

    nkppc.h

Abstract:

    Windows CE Kernel PowerPC specific structures and constants

Revision History:

	Richard Hooker : Updated on 03 April 2000 for use with WindowsCE on PowerPC

--*/

#ifndef _NKPPC_
#define _NKPPC_
#include "ppcinst.h"

// begin_ntddk begin_nthal begin_windbgkd begin_winnt

#if defined(PPC)

// end_windbgkd end_winnt

//
// Define system time structure.
//

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

//
// Define unsupported "keywords".
//

#define _cdecl

// end_ntddk end_nthal
// begin_windbgkd

//
// Define PowerPC specific kernel debugger information.
//
// The following structure contains machine specific data passed to
// the host system kernel debugger in a wait state change message.
//
// **FINISH**  Copied without change from MIPS; may need modification

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

// end_windbgkd


//
// Define breakpoint codes.
//
// **FINISH**  Use MIPS codes unless there's a reason not to
//

#define USER_BREAKPOINT                  0  // user breakpoint
#define KERNEL_BREAKPOINT                1  // kernel breakpoint
#define BREAKIN_BREAKPOINT               2  // break into kernel debugger
#define BRANCH_TAKEN_BREAKPOINT          3  // branch taken breakpoint
#define BRANCH_NOT_TAKEN_BREAKPOINT      4  // branch not taken breakpoint
#define SINGLE_STEP_BREAKPOINT           5  // single step breakpoint
#define DIVIDE_OVERFLOW_BREAKPOINT       6  // divide overflow breakpoint
#define DIVIDE_BY_ZERO_BREAKPOINT        7  // divide by zero breakpoint
#define RANGE_CHECK_BREAKPOINT           8  // range check breakpoint
#define STACK_OVERFLOW_BREAKPOINT        9  // MIPS code
#define MULTIPLY_OVERFLOW_BREAKPOINT    10  // multiply overflow breakpoint

#define DEBUG_PRINT_BREAKPOINT          20  // debug print breakpoint
#define DEBUG_PROMPT_BREAKPOINT         21  // debug prompt breakpoint
#define DEBUG_STOP_BREAKPOINT           22  // debug stop breakpoint
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT   23  // load symbols breakpoint
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 24  // unload symbols breakpoint

//
// Define bus error routine type.
//

struct _EXCEPTION_RECORD;

//
// Macros to emit eieio, sync, and isync instructions.
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
void __emit( unsigned const __int32 );
#define __eieio() 	__emit( 0x7C0006AC )
#define __sync()  	__emit( 0x7C0004AC )
#define __isync() 	__emit( 0x4C00012C )
#define __tlbia() 	__emit( 0x7C0002E4 )
#define __tlbsync() __emit( 0x7C00046C )
unsigned __sregister_get(unsigned const regnum);
void __sregister_set(unsigned const regnum, unsigned const value);
#else
void __builtin_eieio(void);
void __builtin_sync(void);
void __builtin_isync(void);
#endif

// begin_winnt

//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_CONTROL         0x00000001L
#define CONTEXT_FLOATING_POINT  0x00000002L
#define CONTEXT_INTEGER         0x00000004L
#define CONTEXT_DEBUG_REGISTERS 0x00000008L

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

#endif

//
// Thread Context Structure
//

typedef struct _CPUCONTEXT {
    ULONG Gpr0;                         // General registers 0..31
    ULONG Gpr1;
    ULONG Gpr2;
    ULONG Gpr3;
    ULONG Gpr4;
    ULONG Gpr5;
    ULONG Gpr6;
    ULONG Gpr7;
    ULONG Gpr8;
    ULONG Gpr9;
    ULONG Gpr10;
    ULONG Gpr11;
    ULONG Gpr12;
    ULONG Gpr13;
    ULONG Gpr14;
    ULONG Gpr15;
    ULONG Gpr16;
    ULONG Gpr17;
    ULONG Gpr18;
    ULONG Gpr19;
    ULONG Gpr20;
    ULONG Gpr21;
    ULONG Gpr22;
    ULONG Gpr23;
    ULONG Gpr24;
    ULONG Gpr25;
    ULONG Gpr26;
    ULONG Gpr27;
    ULONG Gpr28;
    ULONG Gpr29;
    ULONG Gpr30;
    ULONG Gpr31;

    ULONG Cr;                           // Condition register
    ULONG Xer;                          // Fixed point exception register
    ULONG Msr;                          // Machine status register
    ULONG Iar;                          // Instruction address register
    ULONG Lr;                           // Link register
    ULONG Ctr;                          // Count register
} CPUCONTEXT, *PCPUCONTEXT;


// end_winnt


#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Iar)

#define CONTEXT_LENGTH (sizeof(CONTEXT))
#define CONTEXT_ALIGN (sizeof(double))
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

// begin_nthal
//
// PowerPC special-purpose registers
//

//
// Define Machine Status Register (MSR) fields
//

typedef struct _MSR {
    ULONG LE   : 1;     // 31     Little-Endian execution mode
    ULONG RI   : 1;     // 30     Recoverable Interrupt
    ULONG Rsv1 : 2;     // 29..28 reserved
    ULONG DR   : 1;     // 27     Data Relocate
    ULONG IR   : 1;     // 26     Instruction Relocate
    ULONG IP   : 1;     // 25     Interrupt Prefix
    ULONG Rsv2 : 1;     // 24     reserved
    ULONG FE1  : 1;     // 23     Floating point Exception mode 1
    ULONG BE   : 1;     // 22     Branch trace Enable
    ULONG SE   : 1;     // 21     Single-step trace Enable
    ULONG FE0  : 1;     // 20     Floating point Exception mode 0
    ULONG ME   : 1;     // 19     Machine check Enable
    ULONG FP   : 1;     // 18     Floating Point available
    ULONG PR   : 1;     // 17     Problem state
    ULONG EE   : 1;     // 16     External interrupt Enable
    ULONG ILE  : 1;     // 15     Interrupt Little-Endian mode
    ULONG IMPL : 1;     // 14     Implementation dependent
    ULONG POW  : 1;     // 13     Power management enable
    ULONG Rsv3 : 13;    // 12..0  reserved
} MSR, *PMSR;

//
// Define Processor Version Register (PVR) fields
//

typedef struct _PVR {
    ULONG Revision : 16;
    ULONG Version  : 16;
} PVR, *PPVR;

// end_nthal

// begin_nthal

//
// Define Condition Register (CR) fields
//
// We name the structure CondR rather than CR, so that a pointer
// to a condition register structure is PCondR rather than PCR.
// (PCR is an NT data structure, the Processor Control Region.)

typedef struct _CondR {
    ULONG CR7 : 4;      // Eight 4-bit fields; machine numbers
    ULONG CR6 : 4;      //   them in Big-Endian order
    ULONG CR5 : 4;
    ULONG CR4 : 4;
    ULONG CR3 : 4;
    ULONG CR2 : 4;
    ULONG CR1 : 4;
    ULONG CR0 : 4;
} CondR, *PCondR;

//
// Define Fixed Point Exception Register (XER) fields
//

typedef struct _XER {
    ULONG Rsv : 29;     // 31..3 Reserved
    ULONG CA  : 1;      // 2     Carry
    ULONG OV  : 1;      // 1     Overflow
    ULONG SO  : 1;      // 0     Summary Overflow
} XER, *PXER;

//
// Define Floating Point Status/Control Register (FPSCR) fields
//

typedef struct _FPSCR {
    ULONG RN     : 2;   // 31..30 Rounding control
    ULONG NI     : 1;   // 29     Non-IEEE mode
    ULONG XE     : 1;   // 28     Inexact exception Enable
    ULONG ZE     : 1;   // 27     Zero divide exception Enable
    ULONG UE     : 1;   // 26     Underflow exception Enable
    ULONG OE     : 1;   // 25     Overflow exception Enable
    ULONG VE     : 1;   // 24     Invalid operation exception Enable
    ULONG VXCVI  : 1;   // 23     Invalid op exception (integer convert)
    ULONG VXSQRT : 1;   // 22     Invalid op exception (square root)
    ULONG VXSOFT : 1;   // 21     Invalid op exception (software request)
    ULONG Res1   : 1;   // 20     reserved
    ULONG FU     : 1;   // 19     Result Unordered or NaN
    ULONG FE     : 1;   // 18     Result Equal or zero
    ULONG FG     : 1;   // 17     Result Greater than or positive
    ULONG FL     : 1;   // 16     Result Less than or negative
    ULONG C      : 1;   // 15     Result Class descriptor
    ULONG FI     : 1;   // 14     Fraction Inexact
    ULONG FR     : 1;   // 13     Fraction Rounded
    ULONG VXVC   : 1;   // 12     Invalid op exception (compare)
    ULONG VXIMZ  : 1;   // 11     Invalid op exception (infinity * 0)
    ULONG VXZDZ  : 1;   // 10     Invalid op exception (0 / 0)
    ULONG VXIDI  : 1;   // 9      Invalid op exception (infinity / infinity)
    ULONG VXISI  : 1;   // 8      Invalid op exception (infinity - infinity)
    ULONG VXSNAN : 1;   // 7      Invalid op exception (signalling NaN)
    ULONG XX     : 1;   // 6      Inexact exception
    ULONG ZX     : 1;   // 5      Zero divide exception
    ULONG UX     : 1;   // 4      Underflow exception
    ULONG OX     : 1;   // 3      Overflow exception
    ULONG VX     : 1;   // 2      Invalid operation exception summary
    ULONG FEX    : 1;   // 1      Enabled Exception summary
    ULONG FX     : 1;   // 0      Exception summary
} FPSCR, *PFPSCR;

// end_nthal

//
// Define PowerPC exception handling structures and function prototypes.
//
// These are adopted without change from the MIPS implementation.
//

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
    struct _EXCEPTION_POINTERS *ExceptionPointers
    );

typedef
VOID
(*TERMINATION_HANDLER) (
    BOOLEAN is_abnormal
    );

// end_winnt

#define retValue ctx.Gpr3
#define ARG0    ctx.Gpr3
#define SetThreadIP(pth, addr) ((pth)->ctx.Iar = (ULONG)(addr))
#define GetThreadIP(pth) ((pth)->ctx.Iar)

/* Macros for handling stack shrinkage. */
#define STK_SLACK_SPACE	232

#define MDTestStack(pth)    (((pth)->ctx.Gpr1 < 0x80000000  \
        && ((pth)->dwStackBound>>VA_PAGE) 						\
        	< (((pth)->ctx.Gpr1-STK_SLACK_SPACE)>>VA_PAGE))  	\
        ? (pth)->dwStackBound : 0)

#define MDShrinkStack(pth)  ((pth)->dwStackBound += PAGE_SIZE)

/* Query & set thread's kernel vs. user mode state */
#define KERNEL_MODE     0
#define USER_MODE       1
#define SR_MODE_BITS    14
#define GetThreadMode(pth) (((pth)->ctx.Msr >> SR_MODE_BITS) & 1)
#define SetThreadMode(pth, mode) \
        ((pth)->ctx.Msr = ((pth)->ctx.Msr&(~(1<<SR_MODE_BITS))) | ((mode&1)<<SR_MODE_BITS))

/* Query & set kernel vs. user mode state via Context */
#define GetContextMode(pctx) (((pctx)->Msr >> SR_MODE_BITS) & 1)
#define SetContextMode(pctx, mode)	\
		((pctx)->Msr = ((pctx)->Msr&(~(1<<SR_MODE_BITS))) | ((mode&1)<<SR_MODE_BITS))


#include "mem_ppc.h"










struct KDataStruct {
	LPDWORD	lpvTls;			/* 0x000 Current thread local storage pointer */
    HANDLE ahSys[NUM_SYS_HANDLES]; /* 0x004 If this moves, change kapi.h */
    char    bResched;       /* 0x084 reschedule flag */
    char    cNest;       	/* 0x085 kernel exception nesting level */
    char	pad;			/* 0x086 alignment padding (must never be used) */
    char	pad2;			/* 0x087 alignment padding (must never be used) */
    ulong   cMsec;          /* 0x088 # of milliseconds since boot */
    ulong   cDMsec;         /* 0x08c # of mSec since last TimerCallBack */
	ACCESSKEY akyCur;       /* 0x090 current access key */
	PTHREAD	pCurThd;		/* 0x094 ptr to current THREAD struct */
	DWORD	dwPad;		    /* 0x098 was process breakpoint */
	ulong	handleBase;		/* 0x09c handle table base address */

	PSECTION aSections[64]; /* 0x0a0 section table for virtual memory */
	LPEVENT alpeIntrEvents[SYSINTR_MAX_DEVICES];/* 0x1a0 */
	LPVOID  alpvIntrData[SYSINTR_MAX_DEVICES];  /* 0x220 */
    char    bPowerOff;      /* 0x2a0 TRUE during "power off" processing */
    char    bProfileOn;     /* 0x2a1 TRUE if profiling enabled */
    char	bPad[2];		/* 0x2a2 */
    ulong	pAPIReturn;		/* 0x2a4 direct API call return for kernel mode */
	PPROCESS pCurPrc;		/* 0x2a8 ptr to current PROCESS struct */
    long	kGP;			/* 0x2ac kernel's Global Data pointer */
    ulong 	kMSR;		    /* 0x2b0 base kernel MSR */
    uchar   kPFNMap[KPFN_CNT];      /* 0x2b4 - 0x2d4 KVA to Physical Mappings */

	char    bReserved[0x2fc-0x2d4]; /* 0x2d4 - Processor specific reserved */
	DWORD	dwInDebugger;	/* 0x2fc - !0 when in debugger */
	DWORD   aInfo[32];      /* 0x300 - misc. kernel info */
	DWORD	dwKCRes;		/* 0x380 */
							/* 0x384 - 0x390 - ununsed */
	                        /* 0x390 - low memory interrupt handlers */
	                        /* 0x800 - end */
};  /* KDataStruct */

#define KData  (*(struct KDataStruct *)(KPAGE_BASE+0x800))
#define VKData  (*(volatile struct KDataStruct *)(KPAGE_BASE+0x800))

#define hCurThread          (KData.ahSys[SH_CURTHREAD])
#define hCurProc            (KData.ahSys[SH_CURPROC])
#define pCurThread          (KData.pCurThd)
#define pCurProc            (KData.pCurPrc)
#define ReschedFlag         (KData.bResched)
#define PowerOffFlag        (KData.bPowerOff)
#define ProfileFlag         (KData.bProfileOn)
#define CurAKey             (KData.akyCur)
#define SectionTable        (KData.aSections)
#define InSysCall()         (KData.cNest != 1)
#define MustReschedule()    (*(ulong*)&KData.bResched == 1)
#define IntrEvents          (KData.alpeIntrEvents)
#define IntrData            (KData.alpvIntrData)
#define KPlpvTls            (KData.lpvTls)
#define KInfoTable          (KData.aInfo)
#define DIRECT_RETURN       (KData.pAPIReturn)
#define BaseMSR             (KData.kMSR)
#define KPfnMap             (KData.kPFNMap)
#define KCResched			(KData.dwKCRes)
#define	InDebugger			(KData.dwInDebugger)

ERRFALSE(AddrCurMSec == offsetof(struct KDataStruct, cMsec)+KPAGE_BASE+0x800);
ERRFALSE(AddrDiffMSec == offsetof(struct KDataStruct, cDMsec)+KPAGE_BASE+0x800);
#undef DiffMSec
#define DiffMSec (KData.cDMsec)
#undef CurMSec
#define CurMSec (KData.cMsec)

extern void INTERRUPTS_ON(void);
extern void INTERRUPTS_OFF(void);

extern void *InterlockedPopList(void *pHead);
extern void *InterlockedPushList(void *pHead, void *pItem);

// Defines for CPU specific IDs.
#define THISCPUID IMAGE_FILE_MACHINE_POWERPC
#define PROCESSOR_ARCHITECTURE PROCESSOR_ARCHITECTURE_PPC
extern DWORD CEProcessorType;
extern WORD ProcessorLevel;
extern WORD ProcessorRevision;

#endif // defined(PPC)

#endif // _NKPPC_

