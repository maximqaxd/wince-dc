/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdp.h

Abstract:

    Private include file for the Kernel Debugger subcomponent

Environment:

    WinCE


--*/

#include "kernel.h"
#include "string.h"
#include "kdpcpu.h"
#include "dbg.h"


typedef ULONG NTSTATUS;


#define MmDbgReadCheck(Address)   VerifyAddress(Address)
#define MmDbgWriteCheck(Address)  VerifyAddress(Address)
#define MmDbgTranslatePhysicalAddress(Address) (Address)


#define PAGE_ALIGN(Va)  ((ULONG)(Va) & ~(PAGE_SIZE - 1))
#define BYTE_OFFSET(Va) ((ULONG)(Va) & (PAGE_SIZE - 1))


//
// Ke stub routines and definitions
//


#if defined(x86)

//
// There is no need to sweep the i386 cache because it is unified (no
// distinction is made between instruction and data entries).
// 

#define KeSweepCurrentIcache()

#elif defined(SHx)

//
// There is no need to sweep the SH3 cache because it is unified (no
// distinction is made between instruction and data entries).
// 

extern void FlushCache (void);

#define KeSweepCurrentIcache() FlushCache()

#else

extern void FlushICache (void);

#define KeSweepCurrentIcache() FlushICache()

#endif

//
// GetVersion API (VER_PRODUCTBUILD found in sdk/inc/ntverp.h)
//

#define VER_PRODUCTBUILD 1169


#define STATUS_SYSTEM_BREAK             ((NTSTATUS)0x80000114L)

                      
//
// TRAPA / BREAK immediate field value for breakpoints
//

#define DEBUGBREAK_LOAD_SYMBOLS_BREAKPOINT 0
#define DEBUGBREAK_STOP_BREAKPOINT         1

#define DEBUG_PROCESS_SWITCH_BREAKPOINT       2
#define DEBUG_THREAD_SWITCH_BREAKPOINT        3
#define DEBUG_BREAK_IN                        4
#define DEBUG_REGISTER_BREAKPOINT             5

#define DEBUGBREAK_UNLOAD_SYMBOLS_BREAKPOINT 6


//
// DbgKd APIs are for the portable kernel debugger
//

//
// KD_PACKETS are the low level data format used in KD. All packets
// begin with a packet leader, byte count, packet type. The sequence
// for accepting a packet is:
//
//  - read 4 bytes to get packet leader.  If read times out (10 seconds)
//    with a short read, or if packet leader is incorrect, then retry
//    the read.
//
//  - next read 2 byte packet type.  If read times out (10 seconds) with
//    a short read, or if packet type is bad, then start again looking
//    for a packet leader. 
//
//  - next read 2 byte byte count.  If read times out (10 seconds) with
//    a short read, or if byte count is greater than PACKET_MAX_SIZE,
//    then start again looking for a packet leader.
//    Byte Count is 0 in case of a Control Packet
//
//  - next read 4 byte packet Id.  If read times out (10 seconds)
//    with a short read, or if packet Id is not what we expect, then
//    ask for resend and restart again looking for a packet leader.
//    In the case of a Reset or Resend Control Packet, the packet Id
//    has no meaning 
//
//  - next read 4 byte packet data checksum.
//    In the case of a Control Packet, the packet data checksum has no 
//    meaning 
//
//  - The packet data immediately follows the packet header (not if control
//    packet). There should be ByteCount bytes following the packet header.  
//    Read the packet data, if read times out (10 seconds) then start again 
//    looking for a packet leader.
//
//  - The trailing byte immediately follows the packet data (not if control
//    packet).
//


typedef struct _KD_PACKET {
    ULONG PacketLeader;
    USHORT PacketType;
    USHORT ByteCount;
    ULONG PacketId;
    ULONG Checksum;
} KD_PACKET, *PKD_PACKET;


#define PACKET_MAX_SIZE 4000
#define INITIAL_PACKET_ID 0x80800000    // DON't use 0
#define SYNC_PACKET_ID    0x00000800    // Or in with INITIAL_PACKET_ID
                                        // to force a packet ID reset.

//
// BreakIn packet
//

#define BREAKIN_PACKET                  0x15151515
#define BREAKIN_PACKET_BYTE             0x15

//
// Packet lead in sequence
//

#define PACKET_LEADER                   0x1f1f1f1f //0x77000077
#define PACKET_LEADER_BYTE              0x1f

#define CONTROL_PACKET_LEADER           0x12121212
#define CONTROL_PACKET_LEADER_BYTE      0x12      //Must be greater than MAX packet type

//
// Packet Trailing Byte
//

#define PACKET_TRAILING_BYTE            0xAA

//
// Packet Types
//

#define PACKET_TYPE_UNUSED              0
#define PACKET_TYPE_KD_STATE_CHANGE     1
#define PACKET_TYPE_KD_STATE_MANIPULATE 2
#define PACKET_TYPE_KD_DEBUG_IO         3
#define PACKET_TYPE_KD_ACKNOWLEDGE      4       // Packet-control type
#define PACKET_TYPE_KD_RESEND           5       // Packet-control type
#define PACKET_TYPE_KD_RESET            6       // Packet-control type
#define PACKET_TYPE_MAX                 7

//
// If the packet type is PACKET_TYPE_KD_STATE_CHANGE, then
// the format of the packet data is as follows:
//

#define DbgKdExceptionStateChange   0x00003030L
#define DbgKdLoadSymbolsStateChange 0x00003031L

//
// Pathname Data follows directly
//

typedef struct _DBGKM_EXCEPTION {
    EXCEPTION_RECORD ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;

typedef struct _DBGKD_LOAD_SYMBOLS {
    ULONG PathNameLength;
    PVOID BaseOfDll;
    ULONG ProcessId;
    ULONG CheckSum;
    ULONG SizeOfImage;
    BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS, *PDBGKD_LOAD_SYMBOLS;

typedef struct _DBGKD_WAIT_STATE_CHANGE {
    ULONG NewState;
    USHORT ProcessorType;
    USHORT Processor;
    ULONG NumberProcessors;
    PVOID Thread;
    PVOID ProgramCounter;
    union {
        DBGKM_EXCEPTION Exception;
        DBGKD_LOAD_SYMBOLS LoadSymbols;
    } u;
    DBGKD_CONTROL_REPORT ControlReport;
#if defined(MIPS)
    DWORD Pad;
#endif
    CONTEXT Context;
#if defined(SH3e) || defined(SH4)
    DEBUG_REGISTERS DebugRegisters;
#endif    
} DBGKD_WAIT_STATE_CHANGE, *PDBGKD_WAIT_STATE_CHANGE;

//
// If the packet type is PACKET_TYPE_KD_STATE_MANIPULATE, then
// the format of the packet data is as follows:
//
// Api Numbers for state manipulation
//

#define DbgKdReadVirtualMemoryApi     0x00003130L
#define DbgKdWriteVirtualMemoryApi    0x00003131L
#define DbgKdGetContextApi            0x00003132L
#define DbgKdSetContextApi            0x00003133L
#define DbgKdWriteBreakPointApi       0x00003134L
#define DbgKdRestoreBreakPointApi     0x00003135L
#define DbgKdContinueApi              0x00003136L
#define DbgKdReadControlSpaceApi      0x00003137L
#define DbgKdWriteControlSpaceApi     0x00003138L
#define DbgKdReadIoSpaceApi           0x00003139L
#define DbgKdWriteIoSpaceApi          0x0000313AL
#define DbgKdRebootApi                0x0000313BL
#define DbgKdContinueApi2             0x0000313CL
#define DbgKdReadPhysicalMemoryApi    0x0000313DL
#define DbgKdWritePhysicalMemoryApi   0x0000313EL
#define DbgKdQuerySpecialCallsApi     0x0000313FL
#define DbgKdSetSpecialCallApi        0x00003140L
#define DbgKdClearSpecialCallsApi     0x00003141L
#define DbgKdSetInternalBreakPointApi 0x00003142L
#define DbgKdGetInternalBreakPointApi 0x00003143L
#define DbgKdReadIoSpaceExtendedApi   0x00003144L
#define DbgKdWriteIoSpaceExtendedApi  0x00003145L
#define DbgKdGetVersionApi            0x00003146L
#define DbgKdWriteBreakPointExApi     0x00003147L
#define DbgKdRestoreBreakPointExApi   0x00003148L
#define DbgKdCauseBugCheckApi         0x00003149L
#define DbgKdSwitchProcessor          0x00003150L
#define DbgKdPageInApi                      0x00003151L
#define DbgKdReadMachineSpecificRegister    0x00003152L
#define DbgKdWriteMachineSpecificRegister   0x00003153L
#define DbgKdManipulateBreakpoint			0x00003154L

#define DbgKdTerminateApi             0x00003155L
// end

//
// Response is a read memory message with data following
//

typedef struct _DBGKD_READ_MEMORY {
    PVOID TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesRead;
} DBGKD_READ_MEMORY, *PDBGKD_READ_MEMORY;

//
// Data follows directly
//

typedef struct _DBGKD_WRITE_MEMORY {
    PVOID TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY, *PDBGKD_WRITE_MEMORY;

//
// Response is a get context message with a full context record following
//

typedef struct _DBGKD_GET_CONTEXT {
    ULONG ContextFlags;
} DBGKD_GET_CONTEXT, *PDBGKD_GET_CONTEXT;

//
// Full Context record follows
//

typedef struct _DBGKD_SET_CONTEXT {
    ULONG ContextFlags;
} DBGKD_SET_CONTEXT, *PDBGKD_SET_CONTEXT;

typedef struct _DBGKD_WRITE_BREAKPOINT {
    PVOID BreakPointAddress;
    ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT, *PDBGKD_WRITE_BREAKPOINT;

typedef struct _DBGKD_RESTORE_BREAKPOINT {
    ULONG BreakPointHandle;
} DBGKD_RESTORE_BREAKPOINT, *PDBGKD_RESTORE_BREAKPOINT;

typedef struct _DBGKD_BREAKPOINTEX {
    ULONG     BreakPointCount;
    NTSTATUS  ContinueStatus;
} DBGKD_BREAKPOINTEX, *PDBGKD_BREAKPOINTEX;

typedef struct _DBGKD_CONTINUE {
    NTSTATUS ContinueStatus;
} DBGKD_CONTINUE, *PDBGKD_CONTINUE;

typedef struct _DBGKD_CONTINUE2 {
    NTSTATUS ContinueStatus;
    DBGKD_CONTROL_SET ControlSet;
} DBGKD_CONTINUE2, *PDBGKD_CONTINUE2;

typedef struct _DBGKD_READ_WRITE_IO {
    ULONG DataSize;                     // 1, 2, 4
    PVOID IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO, *PDBGKD_READ_WRITE_IO;

typedef struct _DBGKD_READ_WRITE_IO_EXTENDED {
    ULONG DataSize;                     // 1, 2, 4
    ULONG InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
    PVOID IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO_EXTENDED, *PDBGKD_READ_WRITE_IO_EXTENDED;

typedef struct _DBGKD_READ_WRITE_MSR {
    ULONG Msr;
    ULONG DataValueLow;
    ULONG DataValueHigh;
} DBGKD_READ_WRITE_MSR, *PDBGKD_READ_WRITE_MSR;

typedef struct _DBGKD_QUERY_SPECIAL_CALLS {
    ULONG NumberOfSpecialCalls;
    // ULONG SpecialCalls[];
} DBGKD_QUERY_SPECIAL_CALLS, *PDBGKD_QUERY_SPECIAL_CALLS;

typedef struct _DBGKD_SET_SPECIAL_CALL {
    ULONG SpecialCall;
} DBGKD_SET_SPECIAL_CALL, *PDBGKD_SET_SPECIAL_CALL;

typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT {
    ULONG BreakpointAddress;
    ULONG Flags;
} DBGKD_SET_INTERNAL_BREAKPOINT, *PDBGKD_SET_INTERNAL_BREAKPOINT;

typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT {
    ULONG BreakpointAddress;
    ULONG Flags;
    ULONG Calls;
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;
    ULONG MaxInstructions;
    ULONG TotalInstructions;
} DBGKD_GET_INTERNAL_BREAKPOINT, *PDBGKD_GET_INTERNAL_BREAKPOINT;

#define DBGKD_INTERNAL_BP_FLAG_COUNTONLY 0x00000001 // don't count instructions
#define DBGKD_INTERNAL_BP_FLAG_INVALID   0x00000002 // disabled BP
#define DBGKD_INTERNAL_BP_FLAG_SUSPENDED 0x00000004 // temporarily suspended
#define DBGKD_INTERNAL_BP_FLAG_DYING     0x00000008 // kill on exit

typedef struct _DBGKD_GET_VERSION {
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  ProtocolVersion;
    USHORT  Flags;
    ULONG   KernBase;
    ULONG   PsLoadedModuleList;
    USHORT  MachineType; // Processor Architecture (SHx, Intel x86, MIPS ...)

    //
    // help for walking stacks with user callbacks:
    //

    //
    // The address of the thread structure is provided in the
    // WAIT_STATE_CHANGE packet.  This is the offset from the base of
    // the thread structure to the pointer to the kernel stack frame
    // for the currently active usermode callback.
    //

    USHORT  ThCallbackStack;            // offset in thread data

    //
    // these values are offsets into that frame:
    //

    USHORT  NextCallback;               // saved pointer to next callback frame
    USHORT  FramePointer;               // saved frame pointer

    //
    // Address of the kernel callout routine.
    //

    ULONG   KiCallUserMode;             // kernel routine

    //
    // Address of the usermode entry point for callbacks.
    //

    ULONG   KeUserCallbackDispatcher;   // address in ntdll

    ULONG   dwProcessorName; // this one is used and is assigned to CEProcessorType
    ULONG   KernDataSectionOffset; // Relocated Kernel Data Section Offset
} DBGKD_GET_VERSION, *PDBGKD_GET_VERSION;


#define DBGKD_VERS_FLAG_MP      0x0001      // kernel is MP built

typedef struct _DBGKD_PAGEIN {
    ULONG   Address;
    ULONG   ContinueStatus;
} DBGKD_PAGEIN, *PDBGKD_PAGEIN;

#define DBGKD_MBP_FLAG_SET			0x00000001 // Set
#define DBGKD_MBP_FLAG_RESTORE		0x00000002 // Restore
// If both SET and RESTORE bits are not set then assumption is that it is a query.
#define DBGKD_MBP_HARDWARE			0x00000004 // Specify it is hardware
#define DBGKD_MBP_SOFTWARE 			0x00000008 // Specify it as software
// If both HARDWARE & SOFTWARE is not set that it is a don't Care.
// On a query this can be UNKNOWN.
#define DBGKD_MBP_FLAG_CP			0X80000000 // Hardware Code Breakpoint
#define DBGKD_MBP_FLAG_DP			0X40000000 // Hardware Data Breakpoint
// If both of CP or DP is not set than the assumption is that it is a temp breakpoint
#define DBGKD_MBP_16BIT				0x00000010 // Is it a 16 bit breakpoint ?

typedef struct _DBGKD_MANIPULATE_BREAKPOINT {
	ULONG	Count;
    NTSTATUS  ContinueStatus;
} DBGKD_MANIPULATE_BREAKPOINT, *PDBGKD_MANIPULATE_BREAKPOINT;

typedef struct _DBGKD_MANIPULATE_BREAKPOINT_DATA {
	ULONG	Flags;
	ULONG	Address;
	ULONG	Handle;
} DBGKD_MANIPULATE_BREAKPOINT_DATA, *PDBGKD_MANIPULATE_BREAKPOINT_DATA;

typedef struct _DBGKD_MANIPULATE_STATE {
    ULONG ApiNumber;
    USHORT ProcessorType;
    USHORT Processor;
    NTSTATUS ReturnStatus;
    union {
        DBGKD_READ_MEMORY ReadMemory;
        DBGKD_WRITE_MEMORY WriteMemory;
        DBGKD_GET_CONTEXT GetContext;
        DBGKD_SET_CONTEXT SetContext;
        DBGKD_WRITE_BREAKPOINT WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
        DBGKD_CONTINUE Continue;
        DBGKD_CONTINUE2 Continue2;
        DBGKD_READ_WRITE_IO ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT GetInternalBreakpoint;
        DBGKD_GET_VERSION GetVersion;
        DBGKD_BREAKPOINTEX BreakPointEx;
        DBGKD_PAGEIN PageIn;
        DBGKD_READ_WRITE_MSR ReadWriteMsr;
        DBGKD_MANIPULATE_BREAKPOINT ManipulateBreakPoint;
    } u;
} DBGKD_MANIPULATE_STATE, *PDBGKD_MANIPULATE_STATE;

//
// If the packet type is PACKET_TYPE_KD_DEBUG_IO, then
// the format of the packet data is as follows:
//

#define DbgKdPrintStringApi     0x00003230L
#define DbgKdGetStringApi       0x00003231L

//
// For print string, the Null terminated string to print
// immediately follows the message
//
typedef struct _DBGKD_PRINT_STRING {
    ULONG LengthOfString;
} DBGKD_PRINT_STRING, *PDBGKD_PRINT_STRING;

//
// For get string, the Null terminated promt string
// immediately follows the message. The LengthOfStringRead
// field initially contains the maximum number of characters
// to read. Upon reply, this contains the number of bytes actually
// read. The data read immediately follows the message.
//
//
typedef struct _DBGKD_GET_STRING {
    ULONG LengthOfPromptString;
    ULONG LengthOfStringRead;
} DBGKD_GET_STRING, *PDBGKD_GET_STRING;

typedef struct _DBGKD_DEBUG_IO {
    ULONG ApiNumber;
    USHORT ProcessorType;
    USHORT Processor;
    union {
        DBGKD_PRINT_STRING PrintString;
        DBGKD_GET_STRING GetString;
    } u;
} DBGKD_DEBUG_IO, *PDBGKD_DEBUG_IO;


//
// Status Constants for reading data from comport
//

#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

//
// Data structure for passing information to KdpReportLoadSymbolsStateChange
// function via the debug trap
//

typedef struct _KD_SYMBOLS_INFO {
    IN PVOID BaseOfDll;
    IN ULONG ProcessId;
    IN ULONG CheckSum;
    IN ULONG SizeOfImage;
} KD_SYMBOLS_INFO, *PKD_SYMBOLS_INFO;

extern BOOLEAN KdDebuggerNotPresent;


typedef enum {
    ContinueError = FALSE,
    ContinueSuccess = TRUE,
    ContinueProcessorReselected,
    ContinueNextProcessor
} KCONTINUE_STATUS;


typedef ULONG KSPIN_LOCK;  

//
// Miscellaneous
//

#if DBG

#define KD_ASSERT(exp) assert(exp)

#else

#define KD_ASSERT(exp)

#endif


//
// ReadControlSpace Api commands
//

#define HANDLE_PROCESS_INFO_REQUEST    0 
#define HANDLE_GET_NEXT_OFFSET_REQUEST 1
#define HANDLE_STACKWALK_REQUEST       2
#define HANDLE_THREADSTACK_REQUEST     3
#define HANDLE_THREADSTACK_TERMINATE   4
#define HANDLE_RELOAD_MODULES_REQUEST  5
#define HANDLE_RELOAD_MODULES_INFO	   6
#define HANDLE_PROCESS_ZONE_REQUEST    7
#define HANDLE_KERNEL_DATA_AREA		   8 
#define HANDLE_VERIFY_MODULE_LOAD	   9 
#define HANDLE_PROCESS_THREAD_INFO_REQ 10
#define HANDLE_GETCURPROCTHREAD 11


//
// WriteControlSpace Api commands
//

#define HANDLE_PROCESS_SWITCH_REQUEST  0
#define HANDLE_THREAD_SWITCH_REQUEST   1
#define HANDLE_STACKWALK_REQUEST       2
#define HANDLE_BKPT_GLOBAL_REQUEST     3
#define HANDLE_BKPT_THREAD_REQUEST     4


VOID NKDebuggerPrintfW(LPWSTR lpszFmt, ...);

/*************************************************************************/
extern int ppshfile;
extern int dwCurSetting;

#define KDZONE_MOVE   0x1
#define KDZONE_BREAK  0x2
#define KDZONE_API    0x4
#define KDZONE_TRAP	  0x8
#define KDZONE_DBG	  0x10
#define KDZONE_CTRL   0x20
#define KDZONE_STACKW 0x40
#define KDZONE_HAL	0x80

#define _O_RDONLY	0x0000	/* open for reading only */
#define _O_WRONLY	0x0001	/* open for writing only */
#define _O_RDWR 	0x0002	/* open for reading and writing */
#define _O_APPEND	0x0008	/* writes done at eof */

#define _O_CREAT	0x0100	/* create and open file */
#define _O_TRUNC	0x0200	/* open and truncate */
#define _O_EXCL 	0x0400	/* open only if file doesn't already exist */

WORD WINAPI ProcessZone(LPBYTE Buffer);
VOID NKOtherPrintfW(LPWSTR lpszFmt, ...);

#define DBGOTHER
#if defined(DBGOTHER)
#define DEBUGGERMSG(cond,printf_exp) \
		if (cond & dwCurSetting) NKOtherPrintfW printf_exp
#elif defined(DEBUG_DEBUGGER)
#pragma message("Hello from the debug debugger\r\n")
#define DEBUG 1
#define DEBUGGERMSG(cond,printf_exp)   \
   ((cond)?(NKDebuggerPrintfW printf_exp),1:0)

#define DEBUG_OUT(x)                  DebugPrint(x)
#define DEBUG_OUT1(x, a1)             DebugPrint(x, a1)
#define DEBUG_OUT2(x, a1, a2)         DebugPrint(x, a1, a2)
#define DEBUG_OUT3(x, a1, a2, a3)     DebugPrint(x, a1, a2, a3)
#define DEBUG_OUT4(x, a1, a2, a3, a4) DebugPrint(x, a1, a2, a3, a4)

#else
#define DEBUGGERMSG(cond,printf_exp)
#define DEBUG_OUT(x)
#define DEBUG_OUT1(x, a1)
#define DEBUG_OUT2(x, a1, a2)
#define DEBUG_OUT3(x, a1, a2, a3)
#define DEBUG_OUT4(x, a1, a2, a3, a4)
#endif
/*************************************************************************/

//
// Define constants.
//

#define BREAKPOINT_TABLE_SIZE 32

//
// Addresses above GLOBAL_BREAKPOINT_LIMIT are either in system space
// or part of dynlink, so we treat them as global.
//

#define GLOBAL_BREAKPOINT_LIMIT 1610612736L // 1.5gigabytes

//
// Define breakpoint table entry structure.
//

#define KD_BREAKPOINT_IN_USE        0x00000001
#define KD_BREAKPOINT_NEEDS_WRITE   0x00000002
#define KD_BREAKPOINT_SUSPENDED     0x00000004
#if defined(THUMBSUPPORT) || defined(MIPS16SUPPORT)
#define KD_BREAKPOINT_16BIT         0x00000008
#endif

//
// status Constants for Packet waiting
//

#define KDP_PACKET_RECEIVED 0
#define KDP_PACKET_TIMEOUT 1
#define KDP_PACKET_RESEND 2


typedef struct _BREAKPOINT_ENTRY {
    ULONG Flags;
    ULONG DirectoryTableBase;
    PVOID Address;
    PVOID KAddress;
    PTHREAD  pThrd;
    KDP_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;

extern BOOLEAN bGlobalBreakPoint;

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((long)(Status) >= 0)

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )


#if defined(SHx)
void LoadDebugSymbols(void);

//
// User Break Controller memory-mapped addresses
//
#if SH4
#define UBCBarA  0xFF200000        // 32 bit Break Address A
#define UBCBamrA 0xFF200004        // 8 bit  Break Address Mask A
#define UBCBbrA  0xFF200008        // 16 bit Break Bus Cycle A
#define UBCBasrA 0xFF000014        // 8 bit  Break ASID A
#define UBCBarB  0xFF20000C       // 32 bit Break Address B
#define UBCBamrB 0xFF200010       // 8 bit  Break Address Mask B
#define UBCBbrB  0xFF200014       // 16 bit Break Bus Cycle A
#define UBCBasrB 0xFF000018       // 8 bit  Break ASID B
#define UBCBdrB  0xFF200018       // 32 bit Break Data B
#define UBCBdmrB 0xFF20001C       // 32 bit Break Data Mask B
#define UBCBrcr  0xFF200020       // 16 bit Break Control Register
#else
#define UBCBarA    0xffffffb0
#define UBCBamrA   0xffffffb4
#define UBCBbrA    0xffffffb8
#define UBCBasrA   0xffffffe4
#define UBCBarB    0xffffffa0
#define UBCBamrB   0xffffffa4
#define UBCBbrB    0xffffffa8
#define UBCBasrB   0xffffffe8
#define UBCBdrB    0xffffff90
#define UBCBdmrB   0xffffff94
#define UBCBrcr    0xffffff98
#endif
#endif

#define READ_REGISTER_UCHAR(addr) (*(volatile unsigned char *)(addr))
#define READ_REGISTER_USHORT(addr) (*(volatile unsigned short *)(addr))
#define READ_REGISTER_ULONG(addr) (*(volatile unsigned long *)(addr))

#define WRITE_REGISTER_UCHAR(addr,val) (*(volatile unsigned char *)(addr) = (val))
#define WRITE_REGISTER_USHORT(addr,val) (*(volatile unsigned short *)(addr) = (val))
#define WRITE_REGISTER_ULONG(addr,val) (*(volatile unsigned long *)(addr) = (val))

//
// Define Kd function prototypes.
//
#if defined(MIPS_HAS_FPU) || defined(SH4) || defined(x86)
VOID FPUFlushContext (VOID);
#endif

VOID
KdpReboot (
    VOID
    );

BOOLEAN
KdpPrintString (
    IN LPCWSTR Output
    );

ULONG
KdpAddBreakpoint (
    IN PVOID Address
    );

BOOLEAN
KdpDeleteBreakpoint (
    IN ULONG Handle
    );

ULONG
KdpMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

VOID
KdpQuickMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

USHORT
KdpReceivePacket (
    IN ULONG ExpectedPacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength
    );

VOID
KdpSetLoadState(
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
    IN CONTEXT *ContextRecord
    );

VOID
KdpSetStateChange(
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN CONTEXT * ContextRecord,
    IN BOOLEAN SecondChance
    );

VOID
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE ManipulateState,
    IN CONTEXT * ContextRecord
    );

VOID
KdpSendPacket (
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

ULONG
KdpTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN CONTEXT * ContextRecord,
    IN BOOLEAN SecondChance
    );

VOID
KdpDisplayString (
    IN PCHAR Output
    );

VOID
KdpWriteComPacket (
    USHORT,
    USHORT,
    PVOID,
    PVOID,
    PVOID
    );

BOOLEAN
KdpReadComPacket (
    VOID
    );

BOOLEAN
KdpReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT CONTEXT * ContextRecord,
    IN BOOLEAN SecondChance
    );

BOOLEAN
KdpReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN UnloadSymbols,
    IN OUT CONTEXT * ContextRecord
    );

KCONTINUE_STATUS
KdpSendWaitContinue(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL,
    IN OUT CONTEXT * ContextRecord
    );

VOID
KdpReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpGetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpSetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpReadControlSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpWriteControlSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpReadIoSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpWriteIoSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

VOID
KdpGetVersion(
    IN PDBGKD_MANIPULATE_STATE m
    );

NTSTATUS
KdpWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );
 
VOID
KdpRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    );

NTSTATUS
KdpManipulateBreakPoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
);

	
//
// Define external references.
//
extern BOOL KdpUseTCPSockets;
extern BOOL KdpUseUDPSockets;
extern BOOL KdpUseEdbg;

#define KDP_MESSAGE_BUFFER_SIZE (16384)

extern BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE];
extern KSPIN_LOCK KdpDebuggerLock;
extern KDP_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern UCHAR KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE];
extern ULONG KdpOweBreakpoint;
extern ULONG KdpNextPacketIdToSend;
extern ULONG KdpPacketIdExpected;

typedef struct {
    ULONG Addr;                 // pc address of breakpoint
    ULONG Flags;                // Flags bits
    ULONG Calls;                // # of times traced routine called
    ULONG CallsLastCheck;       // # of calls at last periodic (1s) check
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;      // largest number of instructions for 1 call
    ULONG MaxInstructions;      // smallest # of instructions for 1 call
    ULONG TotalInstructions;    // total instructions for all calls
    ULONG Handle;               // handle in (regular) bpt table
    PVOID Thread;               // Thread that's skipping this BP
    ULONG ReturnAddress;        // return address (if not COUNTONLY)
} DBGKD_INTERNAL_BREAKPOINT, *PDBGKD_INTERNAL_BREAKPOINT;


#define DBGKD_MAX_INTERNAL_BREAKPOINTS 20
extern DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS];

void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx);


//
// Prototypes...
//

USHORT
KdPortGetByte (
    OUT PUCHAR Input
    );

VOID
KdPortPutByte (
    IN UCHAR Output
    );

VOID
KeStallExecutionProcessor (
    ULONG Seconds
    );



BOOLEAN
SH3KdInit(
    VOID
    );

VOID
DumpKdContext(
    IN CONTEXT *ContextRecord
    );

//
// Private procedure prototypes
//

VOID
KdpInitCom(
    VOID
    );

USHORT
KdpReceivePacketLeader (
    IN ULONG PacketType,
    OUT PULONG PacketLeader
    );

VOID
KdClearCommError (
    VOID
    );

// Ethernet debug routines
BOOL FillEdbgBuffer();
BOOL WriteEdbgBuffer();
// Function called to switch over to ethernet transport
BOOL SwitchKdbgToEther(BOOL ToEther);
