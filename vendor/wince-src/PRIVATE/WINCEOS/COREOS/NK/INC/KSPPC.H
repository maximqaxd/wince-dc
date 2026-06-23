// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#include "kxppc.h"

#define STK_SLACK_SPACE 0xe8

//
// Process structure fields
//
#define PrcID  0
#define PrcHandle 0x08
#define PrcVMBase 0x0c

// IPC Call Stack structure fields
#define CstkNext        0
#define CstkRa          4   // return address
#define CstkPrcLast     8   // previous process
#define CstkAkyLast     12  // previous access key
#define CstkExtra		16	// TOC or global pointer
#define CstkSizeof      20  // sizeof this structure

// HINFO structure fields
#define hd_ptr	0x18

// Mask to isolate address bits from a handle
#define HANDLE_ADDRESS_MASK	0x1FFFFFFC

// Thread structure fields

#define ThProc 0x0c
#define ThAKey 0x14
#define ThPcstkTop 0x18
#define ThTlsPtr 0x24
#define ThHandle 0x3c

#define THREAD_CONTEXT_OFFSET   0x5c

#define TcxR0		THREAD_CONTEXT_OFFSET+0x000
#define TcxR1		THREAD_CONTEXT_OFFSET+0x004
#define TcxR2		THREAD_CONTEXT_OFFSET+0x008
#define TcxR3		THREAD_CONTEXT_OFFSET+0x00c
#define TcxR4		THREAD_CONTEXT_OFFSET+0x010
#define TcxR5		THREAD_CONTEXT_OFFSET+0x014
#define TcxR6		THREAD_CONTEXT_OFFSET+0x018
#define TcxR7		THREAD_CONTEXT_OFFSET+0x01c
#define TcxR8		THREAD_CONTEXT_OFFSET+0x020
#define TcxR9		THREAD_CONTEXT_OFFSET+0x024
#define TcxR10		THREAD_CONTEXT_OFFSET+0x028
#define TcxR11		THREAD_CONTEXT_OFFSET+0x02c
#define TcxR12		THREAD_CONTEXT_OFFSET+0x030
#define TcxR13		THREAD_CONTEXT_OFFSET+0x034
#define TcxR14		THREAD_CONTEXT_OFFSET+0x038
#define TcxR15		THREAD_CONTEXT_OFFSET+0x03c
#define TcxR16		THREAD_CONTEXT_OFFSET+0x040
#define TcxR17		THREAD_CONTEXT_OFFSET+0x044
#define TcxR18		THREAD_CONTEXT_OFFSET+0x048
#define TcxR19		THREAD_CONTEXT_OFFSET+0x04c
#define TcxR20		THREAD_CONTEXT_OFFSET+0x050
#define TcxR21		THREAD_CONTEXT_OFFSET+0x054
#define TcxR22		THREAD_CONTEXT_OFFSET+0x058
#define TcxR23		THREAD_CONTEXT_OFFSET+0x05c
#define TcxR24		THREAD_CONTEXT_OFFSET+0x060
#define TcxR25		THREAD_CONTEXT_OFFSET+0x064
#define TcxR26		THREAD_CONTEXT_OFFSET+0x068
#define TcxR27		THREAD_CONTEXT_OFFSET+0x06c
#define TcxR28		THREAD_CONTEXT_OFFSET+0x070
#define TcxR29		THREAD_CONTEXT_OFFSET+0x074
#define TcxR30		THREAD_CONTEXT_OFFSET+0x078
#define TcxR31		THREAD_CONTEXT_OFFSET+0x07c
#define TcxCr		THREAD_CONTEXT_OFFSET+0x080
#define TcxXer		THREAD_CONTEXT_OFFSET+0x084
#define TcxMsr		THREAD_CONTEXT_OFFSET+0x088
#define TcxIar		THREAD_CONTEXT_OFFSET+0x08c
#define TcxLr		THREAD_CONTEXT_OFFSET+0x090
#define TcxCtr		THREAD_CONTEXT_OFFSET+0x094

#define TcxSizeof THREAD_CONTEXT_OFFSET+0x098

//
// Dispatcher Context Structure Offset Definitions
//

#define DcControlPc 0x0
#define DcFunctionEntry 0x4
#define DcEstablisherFrame 0x8
#define DcContextRecord 0xc

//
// Exception Record Offset, Flag, and Enumerated Type Definitions
//

#define EXCEPTION_NONCONTINUABLE 0x1
#define EXCEPTION_UNWINDING 0x2
#define EXCEPTION_EXIT_UNWIND 0x4
#define EXCEPTION_STACK_INVALID 0x8
#define EXCEPTION_NESTED_CALL 0x10
#define EXCEPTION_TARGET_UNWIND 0x20
#define EXCEPTION_COLLIDED_UNWIND 0x40
#define EXCEPTION_UNWIND 0x66
#define EXCEPTION_EXECUTE_HANDLER 0x1
#define EXCEPTION_CONTINUE_SEARCH 0x0
#define EXCEPTION_CONTINUE_EXECUTION 0xffffffff

#define ExceptionContinueExecution 0x0
#define ExceptionContinueSearch 0x1
#define ExceptionNestedException 0x2
#define ExceptionCollidedUnwind 0x3

#define ErExceptionCode 0x0
#define ErExceptionFlags 0x4
#define ErExceptionRecord 0x8
#define ErExceptionAddress 0xc
#define ErNumberParameters 0x10
#define ErExceptionInformation 0x14
#define ExceptionRecordLength 0x50

//
// Large Integer Structure Offset Definitions
//

#define LiLowPart 0x0
#define LiHighPart 0x4

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// Status Code Definitions
//

#define STATUS_ACCESS_VIOLATION 0xc0000005
#define STATUS_ARRAY_BOUNDS_EXCEEDED 0xc000008c
#define STATUS_BAD_COMPRESSION_BUFFER 0xc0000242
#define STATUS_BREAKPOINT 0x80000003
#define STATUS_DATATYPE_MISALIGNMENT 0x80000002
#define STATUS_FLOAT_DENORMAL_OPERAND 0xc000008d
#define STATUS_FLOAT_DIVIDE_BY_ZERO 0xc000008e
#define STATUS_FLOAT_INEXACT_RESULT 0xc000008f
#define STATUS_FLOAT_INVALID_OPERATION 0xc0000090
#define STATUS_FLOAT_OVERFLOW 0xc0000091
#define STATUS_FLOAT_STACK_CHECK 0xc0000092
#define STATUS_FLOAT_UNDERFLOW 0xc0000093
#define STATUS_GUARD_PAGE_VIOLATION 0x80000001
#define STATUS_ILLEGAL_FLOAT_CONTEXT 0xc000014a
#define STATUS_ILLEGAL_INSTRUCTION 0xc000001d
#define STATUS_INSTRUCTION_MISALIGNMENT 0xc00000aa
#define STATUS_INVALID_HANDLE 0xc0000008
#define STATUS_INVALID_LOCK_SEQUENCE 0xc000001e
#define STATUS_INVALID_OWNER 0xc000005a
#define STATUS_INVALID_PARAMETER_1 0xc00000ef
#define STATUS_INVALID_SYSTEM_SERVICE 0xc000001c
#define STATUS_INTEGER_DIVIDE_BY_ZERO 0xc0000094
#define STATUS_INTEGER_OVERFLOW 0xc0000095
#define STATUS_IN_PAGE_ERROR 0xc0000006
#define STATUS_KERNEL_APC 0x100
#define STATUS_LONGJUMP 0x80000026
#define STATUS_NO_CALLBACK_ACTIVE 0xc0000258
#define STATUS_NO_EVENT_PAIR 0xc000014e
#define STATUS_PRIVILEGED_INSTRUCTION 0xc0000096
#define STATUS_SINGLE_STEP 0x80000004
#define STATUS_STACK_OVERFLOW 0xc00000fd
#define STATUS_SUCCESS 0x0
#define STATUS_THREAD_IS_TERMINATING 0xc000004b
#define STATUS_TIMEOUT 0x102
#define STATUS_UNWIND 0xc0000027
#define STATUS_WAKE_SYSTEM_DEBUGGER 0x80000007


//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x7
#define CONTEXT_CONTROL 0x1
#define CONTEXT_FLOATING_POINT 0x2
#define CONTEXT_INTEGER 0x4

#define CxFpr0 0x0
#define CxFpr1 0x8
#define CxFpr2 0x10
#define CxFpr3 0x18
#define CxFpr4 0x20
#define CxFpr5 0x28
#define CxFpr6 0x30
#define CxFpr7 0x38
#define CxFpr8 0x40
#define CxFpr9 0x48
#define CxFpr10 0x50
#define CxFpr11 0x58
#define CxFpr12 0x60
#define CxFpr13 0x68
#define CxFpr14 0x70
#define CxFpr15 0x78
#define CxFpr16 0x80
#define CxFpr17 0x88
#define CxFpr18 0x90
#define CxFpr19 0x98
#define CxFpr20 0xa0
#define CxFpr21 0xa8
#define CxFpr22 0xb0
#define CxFpr23 0xb8
#define CxFpr24 0xc0
#define CxFpr25 0xc8
#define CxFpr26 0xd0
#define CxFpr27 0xd8
#define CxFpr28 0xe0
#define CxFpr29 0xe8
#define CxFpr30 0xf0
#define CxFpr31 0xf8
#define CxFpscr 0x100
#define CxGpr0 0x108
#define CxGpr1 0x10c
#define CxGpr2 0x110
#define CxGpr3 0x114
#define CxGpr4 0x118
#define CxGpr5 0x11c
#define CxGpr6 0x120
#define CxGpr7 0x124
#define CxGpr8 0x128
#define CxGpr9 0x12c
#define CxGpr10 0x130
#define CxGpr11 0x134
#define CxGpr12 0x138
#define CxGpr13 0x13c
#define CxGpr14 0x140
#define CxGpr15 0x144
#define CxGpr16 0x148
#define CxGpr17 0x14c
#define CxGpr18 0x150
#define CxGpr19 0x154
#define CxGpr20 0x158
#define CxGpr21 0x15c
#define CxGpr22 0x160
#define CxGpr23 0x164
#define CxGpr24 0x168
#define CxGpr25 0x16c
#define CxGpr26 0x170
#define CxGpr27 0x174
#define CxGpr28 0x178
#define CxGpr29 0x17c
#define CxGpr30 0x180
#define CxGpr31 0x184
#define CxCr 0x188
#define CxXer 0x18c
#define CxMsr 0x190
#define CxIar 0x194
#define CxLr 0x198
#define CxCtr 0x19c
#define CxContextFlags 0x1a0
#define CxDr0 0x1b0
#define CxDr1 0x1b4
#define CxDr2 0x1b8
#define CxDr3 0x1bc
#define CxDr4 0x1c0
#define CxDr5 0x1c4
#define CxDr6 0x1c8
#define CxDr7 0x1cc
#define ContextFrameLength 0x1d0

//
// Call/Return Stack Frame Header Offset Definitions and Length
//

#define CrBackChain 0x0
#define CrReserved1 0x4
#define CrParameter0 0x08
#define CrParameter1 0x0c
#define CrParameter2 0x10
#define CrParameter3 0x14
#define CrParameter4 0x18
#define CrParameter5 0x1c
#define CrParameter6 0x20
#define CrParameter7 0x24
#define StackFrameHeaderLength 0x28

//
// Jump Offset Definitions and Length
//

#define JbFpr14 0x0
#define JbFpr15 0x8
#define JbFpr16 0x10
#define JbFpr17 0x18
#define JbFpr18 0x20
#define JbFpr19 0x28
#define JbFpr20 0x30
#define JbFpr21 0x38
#define JbFpr22 0x40
#define JbFpr23 0x48
#define JbFpr24 0x50
#define JbFpr25 0x58
#define JbFpr26 0x60
#define JbFpr27 0x68
#define JbFpr28 0x70
#define JbFpr29 0x78
#define JbFpr30 0x80
#define JbFpr31 0x88
#define JbGpr1 0x90
#define JbGpr2 0x94
#define JbGpr13 0x98
#define JbGpr14 0x9c
#define JbGpr15 0xa0
#define JbGpr16 0xa4
#define JbGpr17 0xa8
#define JbGpr18 0xac
#define JbGpr19 0xb0
#define JbGpr20 0xb4
#define JbGpr21 0xb8
#define JbGpr22 0xbc
#define JbGpr23 0xc0
#define JbGpr24 0xc4
#define JbGpr25 0xc8
#define JbGpr26 0xcc
#define JbGpr27 0xd0
#define JbGpr28 0xd4
#define JbGpr29 0xd8
#define JbGpr30 0xdc
#define JbGpr31 0xe0
#define JbCr 0xe4
#define JbIar 0xe8
#define JbType 0xec

//
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x0
#define KERNEL_BREAKPOINT 0x1
#define BREAKIN_BREAKPOINT 0x2
#define BRANCH_TAKEN_BREAKPOINT 0x3
#define BRANCH_NOT_TAKEN_BREAKPOINT 0x4
#define SINGLE_STEP_BREAKPOINT 0x5
#define DIVIDE_OVERFLOW_BREAKPOINT 0x6
#define DIVIDE_BY_ZERO_BREAKPOINT 0x7
#define RANGE_CHECK_BREAKPOINT 0x8
#define STACK_OVERFLOW_BREAKPOINT 0x9
#define MULTIPLY_OVERFLOW_BREAKPOINT 0xa
#define DEBUG_PRINT_BREAKPOINT 0x14
#define DEBUG_PROMPT_BREAKPOINT 0x15
#define DEBUG_STOP_BREAKPOINT 0x16
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 0x17
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 0x18

