; Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
	INCLUDE kxarm.h

; API call defines: cloned from syscall.h

FIRST_METHOD	EQU 0xF0010000
SYSCALL_RETURN	EQU FIRST_METHOD-4

METHOD_MASK	EQU 0x00FF
HANDLE_MASK	EQU 0x003F
HANDLE_SHIFT	EQU 8

NUM_SYS_HANDLES	EQU 32
SYS_HANDLE_BASE	EQU 64
SH_WIN32	EQU 0
SH_CURTHREAD	EQU 1
SH_CURPROC	EQU 2

; a few Win32 error codes:
ERROR_INVALID_FUNCTION	EQU 1
ERROR_INVALID_HANDLE	EQU  6
ERROR_INVALID_ADDRESS	EQU 487

; ARM processor modes
USER_MODE	EQU	2_10000
FIQ_MODE	EQU 2_10001
IRQ_MODE	EQU 2_10010
SVC_MODE	EQU 2_10011
ABORT_MODE	EQU 2_10111
UNDEF_MODE	EQU 2_11011
SYSTEM_MODE EQU	2_11111

THUMB_STATE EQU 0x0020

; ID codes for HandleException
ID_RESCHEDULE		EQU		0
ID_UNDEF_INSTR		EQU		1
ID_SWI_INSTR		EQU		2
ID_PREFETCH_ABORT	EQU		3
ID_DATA_ABORT		EQU		4
ID_IRQ				EQU		5

;*
;* KDataStruct offsets
;*

USER_KPAGE EQU 0xFFFFC800
INTERLOCKED_START EQU USER_KPAGE+0x380
INTERLOCKED_END EQU USER_KPAGE+0x400

lpvTls	EQU 0x000	; Current thread local storage pointer
ahSys	EQU 0x004 ; system handle array
hCurThread	EQU 0x008   ; SH_CURTHREAD==1
hCurProc	EQU 0x00c     ; SH_CURPROC==2
bResched	EQU 0x084	; reschedule flag
cNest	    EQU 0x085	; kernel exception nesting
bPowerOff	EQU 0x086
bProfileOn	EQU 0x087
ptDesc		EQU 0x088 ; 2nd level page table descriptor
;; not used: cDMsec	EQU 0x08c	; # of mSec since last TimerCallBack
pCurPrc	EQU 0x090	; pointer to current PROCESS structure
pCurThd	EQU 0x094	; pointer to current THREAD structure
dwKCRes	EQU 0x098
hBase	EQU 0x09c	; handle table base address
aSections	EQU 0x0a0	; section table for virutal memory
alpeIntrEvents	EQU 0x1a0
alpvIntrData	EQU 0x220
pAPIReturn	EQU 0x2a0
MemoryMap	EQU 0x2a4
bIntrIndexLow	EQU 0x2a8
bIntrIndexHigh	EQU 0x2ac
bIntrNumber	EQU 0x2b0
PendEvents	EQU 0x340


;* MemoryInfo structure fields
MIpKData	EQU  0
MIpKEnd	EQU   4

;* CINFO structure fields
CIacName	EQU      0
CIdisp	EQU      	4
CItype	EQU		5
CIcMethods	EQU    6
CIppfnMethods	EQU 8
CIpdwSig	EQU      12
CIpServer	EQU     16

HEAP_HDATA	EQU 11

;*
;* Process structure fields
;*
PrcID	EQU 0
PrcHandle	EQU 0x08
PrcVMBase	EQU 0x0c

;*
;* IPC Call Stack structure fields
;*
CstkNext	EQU 0
CstkRa	EQU 4
CstkPrcLast	EQU 8
CstkAkyLast	EQU 12
CstkSizeof	EQU 16

;* HDATA structure fields
hd_ptr	EQU 0x18

;* Mask to isolate address bits from a handle
HANDLE_ADDRESS_MASK	EQU 0x1FFFFFFC

;* Thread structure fields

ThwInfo	EQU 0x0
ThProc	EQU 0x0c
ThAKey	EQU 0x14
ThPcstkTop	EQU 0x18
ThdwStackBase	EQU 0x1c
ThTlsPtr	EQU 0x24
ThdwLastError	EQU 0x38
ThHandle	EQU 0x3c

THREAD_CONTEXT_OFFSET	EQU 0x5c

TcxPsr	EQU	THREAD_CONTEXT_OFFSET+0x000
TcxR0	EQU	THREAD_CONTEXT_OFFSET+0x004
TcxR1	EQU	THREAD_CONTEXT_OFFSET+0x008
TcxR2	EQU	THREAD_CONTEXT_OFFSET+0x00c
TcxR3	EQU	THREAD_CONTEXT_OFFSET+0x010
TcxR4	EQU	THREAD_CONTEXT_OFFSET+0x014
TcxR5	EQU	THREAD_CONTEXT_OFFSET+0x018
TcxR6	EQU	THREAD_CONTEXT_OFFSET+0x01c
TcxR7	EQU	THREAD_CONTEXT_OFFSET+0x020
TcxR8	EQU	THREAD_CONTEXT_OFFSET+0x024
TcxR9	EQU	THREAD_CONTEXT_OFFSET+0x028
TcxR10	EQU	THREAD_CONTEXT_OFFSET+0x02c
TcxR11	EQU	THREAD_CONTEXT_OFFSET+0x030
TcxR12	EQU	THREAD_CONTEXT_OFFSET+0x034
TcxSp	EQU	THREAD_CONTEXT_OFFSET+0x038
TcxLr	EQU	THREAD_CONTEXT_OFFSET+0x03c
TcxPc	EQU	THREAD_CONTEXT_OFFSET+0x040

TcxSizeof	EQU	THREAD_CONTEXT_OFFSET+0x044

; Dispatcher Context Structure Offset Definitions

DcControlPc	EQU 0x0
DcFunctionEntry	EQU 0x4
DcEstablisherFrame	EQU 0x8
DcContextRecord	EQU 0xc

; Exception Record Offset, Flag, and Enumerated Type Definitions

EXCEPTION_NONCONTINUABLE	EQU 0x1
EXCEPTION_UNWINDING	EQU 0x2
EXCEPTION_EXIT_UNWIND	EQU 0x4
EXCEPTION_STACK_INVALID	EQU 0x8
EXCEPTION_NESTED_CALL	EQU 0x10
EXCEPTION_TARGET_UNWIND	EQU 0x20
EXCEPTION_COLLIDED_UNWIND	EQU 0x40
EXCEPTION_UNWIND	EQU 0x66

ExceptionContinueExecution	EQU 0x0
ExceptionContinueSearch	EQU 0x1
ExceptionNestedException	EQU 0x2
ExceptionCollidedUnwind	EQU 0x3

ErExceptionCode	EQU 0x0
ErExceptionFlags	EQU 0x4
ErExceptionRecord	EQU 0x8
ErExceptionAddress	EQU 0xc
ErNumberParameters	EQU 0x10
ErExceptionInformation	EQU 0x14
ExceptionRecordLength	EQU 0x50

;
; Context Frame Offset and Flag Definitions
;

CONTEXT_FULL	EQU		0x043
CONTEXT_CONTROL	EQU		0x041
CONTEXT_INTEGER	EQU		0x042

CtxFlags EQU	0x000
CtxR0	EQU		0x004
CtxR1	EQU		0x008
CtxR2	EQU		0x00c
CtxR3	EQU		0x010
CtxR4	EQU		0x014
CtxR5	EQU		0x018
CtxR6	EQU		0x01c
CtxR7	EQU		0x020
CtxR8	EQU		0x024
CtxR9	EQU		0x028
CtxR10	EQU		0x02c
CtxR11	EQU		0x030
CtxR12	EQU		0x034
CtxSp	EQU		0x038
CtxLr	EQU		0x03c
CtxPc	EQU		0x040
CtxPsr	EQU		0x044
CtxSizeof EQU	0x048

; Offsets of the components of a virtual address
VA_PAGE	EQU 12
VA_BLOCK	EQU 16
VA_SECTION	EQU 25
PAGE_MASK	EQU 0x00F
BLOCK_MASK	EQU 0x1ff
SECTION_MASK	EQU 0x03F

; MemBlock structure layout
mb_lock	EQU 0
mb_uses	EQU 4
mb_ixBase	EQU 6
mb_hPf	EQU	8
mb_pages	EQU 12

VERIFY_WRITE_FLAG	EQU   1
VERIFY_KERNEL_OK	EQU    2

;* NK interrupt defines

SYSINTR_NOP	EQU 0
SYSINTR_RESCHED	EQU 1
SYSINTR_BREAK	EQU 2
SYSINTR_DEVICES	EQU 8
SYSINTR_MAX_DEVICES	EQU	32

SYSINTR_FIRMWARE	EQU	SYSINTR_DEVICES+16
SYSINTR_MAXIMUM	EQU	SYSINTR_DEVICES+SYSINTR_MAX_DEVICES

	END
