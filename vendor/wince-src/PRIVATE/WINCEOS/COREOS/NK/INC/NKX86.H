/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    nkx86.h

Abstract:

    User-mode visible x86 specific x86 structures and constants


--*/

#ifndef _NTx86_
#define _NTx86_

// begin_ntddk begin_nthal

#if defined(x86)

//
// Define system time structure.
//

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#endif

// end_ntddk end_nthal

// begin_windbgkd

#ifdef x86

//
// DBGKD_CONTROL_REPORT
//
// This structure contains machine specific data passed to the debugger
// when a Wait_State_Change message is sent.  Idea is to allow debugger
// to do what it needes without reading any more packets.
// Structure is filled in by KdpSetControlReport
//

#define DBGKD_MAXSTREAM 16

typedef struct _DBGKD_CONTROL_REPORT {
    ULONG   Dr6;
    ULONG   Dr7;
    USHORT  InstructionCount;
    USHORT  ReportFlags;
    UCHAR   InstructionStream[DBGKD_MAXSTREAM];
    USHORT  SegCs;
    USHORT  SegDs;
    USHORT  SegEs;
    USHORT  SegFs;
    ULONG   EFlags;
} DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;

#define REPORT_INCLUDES_SEGS    0x0001  // this is for backward compatibility

//
// DBGKD_CONTROL_SET
//
// This structure control value the debugger wants to set on every
// continue, and thus sets here to avoid packet traffic.
//

typedef struct _DBGKD_CONTROL_SET {
    ULONG   TraceFlag;                  // WARNING: This must NOT be a BOOLEAN,
                                        //     or host and target will end
                                        //     up with different alignments!
    ULONG   Dr7;
    ULONG   CurrentSymbolStart;         // Range in which to trace locally
    ULONG   CurrentSymbolEnd;
} DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;

#endif //x86

// end_windbgkd


// begin_ntddk begin_nthal begin_winnt begin_ntminiport

#ifdef x86

//
// Disable these two pramas that evaluate to "sti" "cli" on x86 so that driver
// writers to not leave them inadvertantly in their code.
//

#if !defined(MIDL_PASS)
#if !defined(RC_INVOKED)

#pragma warning(disable:4164)   // disable C4164 warning so that apps that
                                // build with /Od don't get weird errors !
#ifdef _M_IX86
#if defined(__cplusplus)
extern "C" void _enable(void);
extern "C" void _disable(void);
#else
extern void _enable(void);
extern void _disable(void);
#endif
#pragma intrinsic(_enable)
#pragma intrinsic(_disable)
#endif

#pragma warning(default:4164)   // reenable C4164 warning

#endif
#endif

// end_ntddk end_nthal end_winnt end_ntminiport

//
//  Values put in ExceptionRecord.ExceptionInformation[0]
//  First parameter is always in ExceptionInformation[1],
//  Second parameter is always in ExceptionInformation[2]
//

#define BREAKPOINT_BREAK            0
#define BREAKPOINT_PRINT            1
#define BREAKPOINT_PROMPT           2
#define BREAKPOINT_LOAD_SYMBOLS     3
#define BREAKPOINT_UNLOAD_SYMBOLS   4

#define EMX87_DATA_SIZE 40
#define SIZE_OF_FX_REGISTERS        128
#define SIZE_OF_FXSAVE_AREA         528
#define CPUID_FXSR                  0x01000000      // CPUID bit for fxsr
#define CR4_FXSR                    0x00000200      // CR4 fxsr enable bit
#define FXRESTOR_EAX    __asm {_emit 0fh} __asm {_emit 0aeh} __asm {_emit 08h}
#define FXSAVE_EAX      __asm {_emit 0fh} __asm {_emit 0aeh} __asm {_emit 00h}
#define FXSAVE_ECX      __asm {_emit 0fh} __asm {_emit 0aeh} __asm {_emit 01h}
#define MOV_EDX_CR4     __asm {_emit 0fh} __asm {_emit 020h} __asm {_emit 0e2h}
#define MOV_CR4_EDX     __asm {_emit 0fh} __asm {_emit 022h} __asm {_emit 0e2h}

typedef struct _FXSAVE_AREA {       // FXSAVE *has* to operate on a 16 byte
    USHORT  ControlWord;            // aligned buffer
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   Reserved2;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
    UCHAR   Align16Bytes[16];
} FXSAVE_AREA, *PFXSAVE_AREA;

typedef struct _NK_PCR {
    DWORD   ExceptionList;
    DWORD   InitialStack;
    DWORD   StackLimit;
    union {
        DWORD   Emx87Data[EMX87_DATA_SIZE];
        FLOATING_SAVE_AREA tcxFPU;
        FXSAVE_AREA tcxExtended;
    };
    DWORD   tls[TLS_MINIMUM_AVAILABLE];
} NK_PCR;

#define FS_LIMIT    (12+SIZE_OF_FXSAVE_AREA-1) // PCR visble thru FS:

ERRFALSE(sizeof(FLOATING_SAVE_AREA) < SIZE_OF_FXSAVE_AREA);

// Note: To enforce 16 byte alignment, FLTSAVE_BACKOFF must be ANDed with 0xF0
#define FLTSAVE_BACKOFF (SIZE_OF_FXSAVE_AREA-16)
#define PTH_TO_FLTSAVEAREAPTR(pth) ((FLOATING_SAVE_AREA *)((((DWORD)(pth->tlsPtr))-FLTSAVE_BACKOFF) & 0xfffffff0))

//
// Call frame record definition.
//
// There is no standard call frame for NT/x86, but there is a linked
// list structure used to register exception handlers, this is it.
//

// begin_nthal
//
// Exception Registration structure
//

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

//
// Define constants for system IDTs
//

#define MAXIMUM_IDTVECTOR 0xff
#define MAXIMUM_PRIMARY_VECTOR 0xff
#define PRIMARY_VECTOR_BASE 0x30        // 0-2f are x86 trap vectors

// begin_ntddk

// end_ntddk end_nthal end_winnt end_ntminiport

#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Eip)

#define CONTEXT_LENGTH  (sizeof(CONTEXT))
#define CONTEXT_ALIGN   (sizeof(ULONG))
#define CONTEXT_ROUND   (CONTEXT_ALIGN - 1)

//
//  GDT selectors - These defines are R0 selector numbers, which means
//                  they happen to match the byte offset relative to
//                  the base of the GDT.
//

#define KGDT_NULL           0x0000
#define KGDT_R0_CODE        0x0008
#define KGDT_R0_DATA        0x0010
#define KGDT_R1_CODE        0x0018
#define KGDT_R1_DATA        0x0020
#define KGDT_R3_CODE        0x0038
#define KGDT_R3_DATA        0x0040
#define KGDT_MAIN_TSS       0x0048
#define KGDT_NMI_TSS        0x0050
#define KGDT_DOUBLE_TSS     0x0058
#define KGDT_PCR			0x0060
#define KGDT_EMX87			0x0068

//
// Process Ldt Information
//  NtQueryInformationProcess using ProcessLdtInformation
//

typedef struct _LDT_INFORMATION {
    ULONG Start;
    ULONG Length;
    LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

//
// Process Ldt Size
//  NtSetInformationProcess using ProcessLdtSize
//

typedef struct _LDT_SIZE {
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

//
// Thread Descriptor Table Entry
//  NtQueryInformationThread using ThreadDescriptorTableEntry
//

// begin_windbgkd

typedef struct _DESCRIPTOR_TABLE_ENTRY {
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

// end_windbgkd

typedef struct TContext CPUCONTEXT;
struct TContext {
	ULONG	TcxGs;
	ULONG	TcxFs;
	ULONG	TcxEs;
	ULONG	TcxDs;
	ULONG	TcxEdi;
	ULONG	TcxEsi;
	ULONG	TcxEbp;
	ULONG	TcxNotEsp;
	ULONG	TcxEbx;
	ULONG	TcxEdx;
	ULONG	TcxEcx;
	ULONG	TcxEax;
	ULONG	TcxError;
	ULONG	TcxEip;
	ULONG	TcxCs;
	ULONG	TcxEFlags;
	ULONG	TcxEsp;
	ULONG	TcxSs;
};

#define retValue ctx.TcxEax
#define SetThreadIP(pth, addr) ((pth)->ctx.TcxEip = (ULONG)(addr))
#define GetThreadIP(pth) ((pth)->ctx.TcxEip)

/* Query & set thread's kernel vs. user mode state */
#define KERNEL_MODE     0
#define USER_MODE       1

#define GetThreadMode(pth) ((pth)->ctx.TcxCs == (KGDT_R3_CODE|3))
#define SetThreadMode(pth, mode) ((mode) ?	\
        	((pth)->ctx.TcxCs = (KGDT_R3_CODE|3),	\
        	(pth)->ctx.TcxSs = (KGDT_R3_DATA|3))	\
        :	((pth)->ctx.TcxCs = (KGDT_R1_CODE|1),	\
        	(pth)->ctx.TcxSs = (KGDT_R1_DATA|1)) )

/* Query & set kernel vs. user mode state via Context */
#define GetContextMode(pctx) ((pctx)->SegCs == (KGDT_R3_CODE|3))
#define SetContextMode(pctx, mode)	((mode) ?	\
        	((pctx)->SegCs = (KGDT_R3_CODE|3),	\
        	(pctx)->SegSs = (KGDT_R3_DATA|3))	\
        :	((pctx)->SegCs = (KGDT_R1_CODE|1),	\
        	(pctx)->SegSs = (KGDT_R1_DATA|1)) )

/* Macros for handling stack shrinkage. */
#define MDTestStack(pth)    (((pth)->ctx.TcxEsp < 0x80000000          \
        && ((pth)->dwStackBound>>VA_PAGE) < (((pth)->ctx.TcxEsp-8*4)>>VA_PAGE))  \
        ? (pth)->dwStackBound : 0)

#define MDShrinkStack(pth)  ((pth)->dwStackBound += PAGE_SIZE)


#include "mem_x86.h"










struct KDataStruct {
	LPDWORD	lpvTls;			/* 0x000 Current thread local storage pointer */
    HANDLE	ahSys[NUM_SYS_HANDLES]; /* 0x004 If this moves, change kapi.h */
    char    bResched;       /* 0x084 reschedule flag */
    char    cNest;          /* 0x085 kernel exception nesting */
    char    bPowerOff;      /* 0x086 TRUE during "power off" processing */
    char    bProfileOn;     /* 0x087 TRUE if profiling enabled */
    ulong   cMsec;          /* 0x088 # of milliseconds since boot */
    ulong   cDMsec;         /* 0x08c # of mSec since last TimerCallBack */
	DWORD	dwKCRes;	    /* 0x090 was process breakpoint */
	ulong	handleBase;		/* 0x094 base address of handle table */
	PTHREAD	pCurThd;		/* 0x098 ptr to current THREAD struct */
	PPROCESS pCurPrc;		/* 0x09c ptr to current PROCESS struct */
	PSECTION aSections[64]; /* 0x0a0 section table for virutal memory */
	LPEVENT alpeIntrEvents[SYSINTR_MAX_DEVICES];/* 0x1a0 */
	LPVOID  alpvIntrData[SYSINTR_MAX_DEVICES];  /* 0x220 */
	ulong	pAPIReturn;		/* 0x2a0 direct API return address for kernel mode */
	DWORD	dwInDebugger;	/* 0x2a4 - !0 when in debugger */

	long    alPad[22];      /* 0x2a8 - padding */
	DWORD   aInfo[32];      /* 0x300 - misc. kernel info */
	                        /* 0x380-0x400 reserved */
	                        /* 0x400 - end */
};  /* KDataStruct */

#ifdef BUILDING_DEBUGGER
extern struct KDataStruct *kdpKData;
#define KData  (*(struct KDataStruct *)kdpKData)
#else
extern struct KDataStruct KData;
#endif

extern volatile ulong CurMSec;

#define hCurThread   (KData.ahSys[SH_CURTHREAD])
#define hCurProc     (KData.ahSys[SH_CURPROC])
#define pCurThread   (KData.pCurThd)
#define pCurProc	(KData.pCurPrc)
#define ReschedFlag (KData.bResched)
#define KCResched	(KData.dwKCRes)
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
#define	InDebugger	(KData.dwInDebugger)

#define INTERRUPTS_ON() _enable()
#define INTERRUPTS_OFF() _disable()

__inline void *InterlockedPopList(void *pHead)
{
	void *ret;

	_disable();
	if ((ret = *(void **)pHead) != 0)
		*(void **)pHead = *(void **)ret;
	_enable();
	return ret;
}

#pragma warning(disable:4035)               // re-enable below

__inline void *InterlockedPushList(volatile void *pHead, void *pItem)
{
	__asm {
		mov     ecx, pHead
		mov     edx, pItem
		mov		eax, [ecx]
	x:	mov		[edx], eax
		cmpxchg [ecx], edx
		jnz		x
	}
}

#pragma warning(default:4035)

// Defines for CPU specific IDs.
#define THISCPUID IMAGE_FILE_MACHINE_I386
#define PROCESSOR_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
extern DWORD CEProcessorType;
extern WORD ProcessorLevel;
extern WORD ProcessorRevision;

// begin_ntddk begin_nthal
#endif // x86
// end_ntddk end_nthal

//
// Library function prototypes.
//

VOID
RtlCaptureContext (
    OUT PCONTEXT ContextRecord
    );

//
// Additional information supplied in QuerySectionInformation for images.
//

#define SECTION_ADDITIONAL_INFO_USED 0

//
// GDT Entry
//

typedef struct _KGDTENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

#define TYPE_CODE   0x10  // 11010 = Code, Readable, NOT Conforming, Accessed
#define TYPE_DATA   0x12  // 10010 = Data, ReadWrite, NOT Expanddown, Accessed
#define TYPE_TSS    0x09  // 01001 = NonBusy 486 TSS
#define TYPE_LDT    0x02  // 00010 = LDT

#define DPL_USER    3
#define DPL_SYSTEM  0

#define GRAN_BYTE   0
#define GRAN_PAGE   1

#define SELECTOR_TABLE_INDEX 0x04

//
// Entry of Interrupt Descriptor Table (IDTENTRY)
//

typedef struct _KIDTENTRY {
   USHORT Offset;
   USHORT Selector;
   USHORT Access;
   USHORT ExtendedOffset;
} KIDTENTRY;

typedef KIDTENTRY *PKIDTENTRY;

//
// Access types for IDT entries
//
#define TRAP_GATE       0x8F00
#define RING1_TRAP_GATE 0xAF00
#define RING3_TRAP_GATE 0xEF00
#define INTERRUPT_GATE  0x8E00
#define RING1_INT_GATE  0xAE00
#define RING3_INT_GATE  0xEE00
#define TASK_GATE       0x8500

//
//  Function to initialize IDT entries
//
extern void InitIDTEntry(int i, USHORT usSelector, PVOID pFaultHandler, USHORT usGateType);


//
// TSS (Task switch segment) NT only uses to control stack switches.
//
//  The only fields we use are Esp0, Ss0, the IoMapBase
//  and the IoAccessMaps themselves.
//
//
//  Size of TSS must be <= 0xDFFF
//


typedef struct _KTSS {

    USHORT  Backlink;
    USHORT  Reserved0;

    ULONG   Esp0;
    USHORT  Ss0;
    USHORT  Reserved1;

    ULONG   Esp1;
    USHORT  Ss1;
    USHORT  Reserved2;

    ULONG   Esp2;
    USHORT  Ss2;
    USHORT  Reserved3;

    ULONG   CR3;

    ULONG   Eip;
    ULONG   Eflags;

	ULONG	Eax;
	ULONG	Ecx;
	ULONG	Edx;
	ULONG	Ebx;
	ULONG	Esp;
	ULONG	Ebp;
	ULONG	Esi;
	ULONG	Edi;

    USHORT  Es;
    USHORT  Reserved4;

    USHORT  Cs;
    USHORT  Reserved5;

    USHORT  Ss;
    USHORT  Reserved6;

    USHORT  Ds;
    USHORT  Reserved7;

    USHORT  Fs;
    USHORT  Reserved8;

    USHORT  Gs;
    USHORT  Reserved9;

    USHORT  LDT;
    USHORT  Reserved10;

    USHORT  Flags;

    USHORT  IoMapBase;
} KTSS, *PKTSS;

#endif // _NTx86_
