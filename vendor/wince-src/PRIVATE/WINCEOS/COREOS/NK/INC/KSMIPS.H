// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#include "kxmips.h"

#define NO_LL 	1	// no load linked/store condition instructions

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
#define CstkExtra		16	// saved GP pointer
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

#define TcxBadVAddr 0x00+THREAD_CONTEXT_OFFSET
#define TcxIntAt 0x04+THREAD_CONTEXT_OFFSET
#define TcxIntV0 0x08+THREAD_CONTEXT_OFFSET
#define TcxIntV1 0x0C+THREAD_CONTEXT_OFFSET
#define TcxIntA0 0x10+THREAD_CONTEXT_OFFSET
#define TcxIntA1 0x14+THREAD_CONTEXT_OFFSET
#define TcxIntA2 0x18+THREAD_CONTEXT_OFFSET
#define TcxIntA3 0x1c+THREAD_CONTEXT_OFFSET
#define TcxIntT0 0x20+THREAD_CONTEXT_OFFSET
#define TcxIntT1 0x24+THREAD_CONTEXT_OFFSET
#define TcxIntT2 0x28+THREAD_CONTEXT_OFFSET
#define TcxIntT3 0x2c+THREAD_CONTEXT_OFFSET
#define TcxIntT4 0x30+THREAD_CONTEXT_OFFSET
#define TcxIntT5 0x34+THREAD_CONTEXT_OFFSET
#define TcxIntT6 0x38+THREAD_CONTEXT_OFFSET
#define TcxIntT7 0x3c+THREAD_CONTEXT_OFFSET
#define TcxIntS0 0x40+THREAD_CONTEXT_OFFSET
#define TcxIntS1 0x44+THREAD_CONTEXT_OFFSET
#define TcxIntS2 0x48+THREAD_CONTEXT_OFFSET
#define TcxIntS3 0x4c+THREAD_CONTEXT_OFFSET
#define TcxIntS4 0x50+THREAD_CONTEXT_OFFSET
#define TcxIntS5 0x54+THREAD_CONTEXT_OFFSET
#define TcxIntS6 0x58+THREAD_CONTEXT_OFFSET
#define TcxIntS7 0x5c+THREAD_CONTEXT_OFFSET
#define TcxIntT8 0x60+THREAD_CONTEXT_OFFSET
#define TcxIntT9 0x64+THREAD_CONTEXT_OFFSET
#define TcxIntK0 0x68+THREAD_CONTEXT_OFFSET
#define TcxIntK1 0x6c+THREAD_CONTEXT_OFFSET
#define TcxIntGp 0x70+THREAD_CONTEXT_OFFSET
#define TcxIntSp 0x74+THREAD_CONTEXT_OFFSET
#define TcxIntS8 0x78+THREAD_CONTEXT_OFFSET
#define TcxIntRa 0x7c+THREAD_CONTEXT_OFFSET
#define TcxIntLo 0x80+THREAD_CONTEXT_OFFSET
#define TcxIntHi 0x84+THREAD_CONTEXT_OFFSET
#define TcxFsr 0x88+THREAD_CONTEXT_OFFSET
#define TcxFir 0x8c+THREAD_CONTEXT_OFFSET
#define TcxPsr 0x90+THREAD_CONTEXT_OFFSET
#define TcxContextFlags 0x94+THREAD_CONTEXT_OFFSET

#ifndef MIPS_HAS_FPU
#define TcxSizeof 0x98+THREAD_CONTEXT_OFFSET
#else
#define TcxFltF0 0x98+THREAD_CONTEXT_OFFSET
#define TcxFltF1 0x9c+THREAD_CONTEXT_OFFSET
#define TcxFltF2 0xa0+THREAD_CONTEXT_OFFSET
#define TcxFltF3 0xa4+THREAD_CONTEXT_OFFSET
#define TcxFltF4 0xa8+THREAD_CONTEXT_OFFSET
#define TcxFltF5 0xac+THREAD_CONTEXT_OFFSET
#define TcxFltF6 0xb0+THREAD_CONTEXT_OFFSET
#define TcxFltF7 0xb4+THREAD_CONTEXT_OFFSET
#define TcxFltF8 0xb8+THREAD_CONTEXT_OFFSET
#define TcxFltF9 0xbc+THREAD_CONTEXT_OFFSET
#define TcxFltF10 0xc0+THREAD_CONTEXT_OFFSET
#define TcxFltF11 0xc4+THREAD_CONTEXT_OFFSET
#define TcxFltF12 0xc8+THREAD_CONTEXT_OFFSET
#define TcxFltF13 0xcc+THREAD_CONTEXT_OFFSET
#define TcxFltF14 0xd0+THREAD_CONTEXT_OFFSET
#define TcxFltF15 0xd4+THREAD_CONTEXT_OFFSET
#define TcxFltF16 0xd8+THREAD_CONTEXT_OFFSET
#define TcxFltF17 0xdc+THREAD_CONTEXT_OFFSET
#define TcxFltF18 0xe0+THREAD_CONTEXT_OFFSET
#define TcxFltF19 0xe4+THREAD_CONTEXT_OFFSET
#define TcxFltF20 0xe8+THREAD_CONTEXT_OFFSET
#define TcxFltF21 0xec+THREAD_CONTEXT_OFFSET
#define TcxFltF22 0xf0+THREAD_CONTEXT_OFFSET
#define TcxFltF23 0xf4+THREAD_CONTEXT_OFFSET
#define TcxFltF24 0xf8+THREAD_CONTEXT_OFFSET
#define TcxFltF25 0xfc+THREAD_CONTEXT_OFFSET
#define TcxFltF26 0x100+THREAD_CONTEXT_OFFSET
#define TcxFltF27 0x104+THREAD_CONTEXT_OFFSET
#define TcxFltF28 0x108+THREAD_CONTEXT_OFFSET
#define TcxFltF29 0x10c+THREAD_CONTEXT_OFFSET
#define TcxFltF30 0x110+THREAD_CONTEXT_OFFSET
#define TcxFltF31 0x114+THREAD_CONTEXT_OFFSET
#define TcxSizeof 0x118+THREAD_CONTEXT_OFFSET
#endif

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
// List Entry Structure Offset Definitions
//

#define LsFlink 0x0
#define LsBlink 0x4

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x10007
#define CONTEXT_CONTROL 0x10001
#define CONTEXT_FLOATING_POINT 0x10002
#define CONTEXT_INTEGER 0x10004

#define CxFltF0 0x10
#define CxFltF1 0x14
#define CxFltF2 0x18
#define CxFltF3 0x1c
#define CxFltF4 0x20
#define CxFltF5 0x24
#define CxFltF6 0x28
#define CxFltF7 0x2c
#define CxFltF8 0x30
#define CxFltF9 0x34
#define CxFltF10 0x38
#define CxFltF11 0x3c
#define CxFltF12 0x40
#define CxFltF13 0x44
#define CxFltF14 0x48
#define CxFltF15 0x4c
#define CxFltF16 0x50
#define CxFltF17 0x54
#define CxFltF18 0x58
#define CxFltF19 0x5c
#define CxFltF20 0x60
#define CxFltF21 0x64
#define CxFltF22 0x68
#define CxFltF23 0x6c
#define CxFltF24 0x70
#define CxFltF25 0x74
#define CxFltF26 0x78
#define CxFltF27 0x7c
#define CxFltF28 0x80
#define CxFltF29 0x84
#define CxFltF30 0x88
#define CxFltF31 0x8c
#define CxIntZero 0x90
#define CxIntAt 0x94
#define CxIntV0 0x98
#define CxIntV1 0x9c
#define CxIntA0 0xa0
#define CxIntA1 0xa4
#define CxIntA2 0xa8
#define CxIntA3 0xac
#define CxIntT0 0xb0
#define CxIntT1 0xb4
#define CxIntT2 0xb8
#define CxIntT3 0xbc
#define CxIntT4 0xc0
#define CxIntT5 0xc4
#define CxIntT6 0xc8
#define CxIntT7 0xcc
#define CxIntS0 0xd0
#define CxIntS1 0xd4
#define CxIntS2 0xd8
#define CxIntS3 0xdc
#define CxIntS4 0xe0
#define CxIntS5 0xe4
#define CxIntS6 0xe8
#define CxIntS7 0xec
#define CxIntT8 0xf0
#define CxIntT9 0xf4
#define CxIntK0 0xf8
#define CxIntK1 0xfc
#define CxIntGp 0x100
#define CxIntSp 0x104
#define CxIntS8 0x108
#define CxIntRa 0x10c
#define CxIntLo 0x110
#define CxIntHi 0x114
#define CxFsr 0x118
#define CxFir 0x11c
#define CxPsr 0x120
#define CxContextFlags 0x124
#define CxHProc 0x128
#define CxAkyCur 0x12c
#define ContextFrameLength 0x130

//
// Jump Offset Definitions and Length
//

#define JbFltF20 0x0
#define JbFltF21 0x4
#define JbFltF22 0x8
#define JbFltF23 0xc
#define JbFltF24 0x10
#define JbFltF25 0x14
#define JbFltF26 0x18
#define JbFltF27 0x1c
#define JbFltF28 0x20
#define JbFltF29 0x24
#define JbFltF30 0x28
#define JbFltF31 0x2c
#define JbIntS0 0x30
#define JbIntS1 0x34
#define JbIntS2 0x38
#define JbIntS3 0x3c
#define JbIntS4 0x40
#define JbIntS5 0x44
#define JbIntS6 0x48
#define JbIntS7 0x4c
#define JbIntS8 0x50
#define JbIntSp 0x54
#define JbType 0x58
#define JbFir 0x5c

//
// Address Space Layout Definitions
//

#define KUSEG_BASE 0x0
#define KSEG0_BASE 0x80000000
#define KSEG1_BASE 0xa0000000
#define KSEG2_BASE 0xc0000000
#define CACHE_ERROR_VECTOR 0xa0000400
#define SYSTEM_BASE 0xc0800000
#define PDE_BASE 0xc0300000
#define PTE_BASE 0xc0000000

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

//
// Status Code Definitions
//

#define STATUS_SUCCESS 0x0
#define STATUS_ACCESS_VIOLATION 0xc0000005
#define STATUS_ARRAY_BOUNDS_EXCEEDED 0xc000008c
#define STATUS_DATATYPE_MISALIGNMENT 0x80000002
#define STATUS_GUARD_PAGE_VIOLATION 0x80000001
#define STATUS_INVALID_SYSTEM_SERVICE 0xc000001c
#define STATUS_IN_PAGE_ERROR 0xc0000006
#define STATUS_ILLEGAL_INSTRUCTION 0xc000001d
#define STATUS_KERNEL_APC 0x100
#define STATUS_BREAKPOINT 0x80000003
#define STATUS_SINGLE_STEP 0x80000004
#define STATUS_INTEGER_OVERFLOW 0xc0000095
#define STATUS_INVALID_LOCK_SEQUENCE 0xc000001e
#define STATUS_INSTRUCTION_MISALIGNMENT 0xc00000aa
#define STATUS_FLOAT_STACK_CHECK 0xc0000092
#define STATUS_NO_EVENT_PAIR 0xc000014e
#define STATUS_INVALID_PARAMETER_1 0xc00000ef
#define STATUS_INVALID_OWNER 0xc000005a
#define STATUS_STACK_OVERFLOW 0xc00000fd
#define STATUS_LONGJUMP 0x80000026
#define STATUS_NO_CALLBACK_ACTIVE 0xc0000258
#define STATUS_BAD_COMPRESSION_BUFFER 0xc0000242

