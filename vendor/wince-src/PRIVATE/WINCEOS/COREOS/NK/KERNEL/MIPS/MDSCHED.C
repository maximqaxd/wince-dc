/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include "kernel.h"

#ifdef MIPS_HAS_FPU
void SaveFloatContext(PTHREAD);
void RestoreFloatContext(PTHREAD);
DWORD GetAndClearFloatCode(void);
BOOL HandleHWFloatException(EXCEPTION_RECORD *er, PCONTEXT pctx, CAUSE cause);
void FPUFlushContext(void);
#endif

#ifdef MIPS16SUPPORT
const wchar_t NKSignon[] = TEXT("Windows CE Kernel for MIPS16 Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n");
#else
const wchar_t NKSignon[] = TEXT("Windows CE Kernel for MIPS Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n");
#endif

// Define breakpoint instruction values.

#define MIPS32_BREAK(_t) ((SPEC_OP << 26) | ((_t) << 16) | BREAK_OP)
#if defined(MIPS16SUPPORT)
#define MIPS16_BREAK(_t) ((RR_OP16 << 11) | ((_t) << 5) | BREAK_OP16)
#endif

void DumpDwords(PDWORD pdw, int len) {
    int lc;
    lc = 0;
    NKDbgPrintfW(L"Dumping %d dwords", len);
    for (lc = 0 ; len ; ++pdw, ++lc, --len) {
        if (!(lc & 3))
            NKDbgPrintfW(L"\r\n%8.8lx -", pdw);
        NKDbgPrintfW(L" %8.8lx", *pdw);
    }
    NKDbgPrintfW(L"\r\n");
}

void DumpFrame(PTHREAD pth, PICONTEXT pctx, CAUSE cause, ULONG badVAddr, int level) {
    ulong addr;
    PDWORD pdw;
    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx AKY=%8.8lx PC=%8.8lx BVA=%8.8lx\r\n",
            cause.XCODE, pth, pCurThread->aky, pctx->Fir, badVAddr);
    NKDbgPrintfW(L"SR=%8.8lx AT=%8.8lx V0=%8.8lx V1=%8.8lx\r\n",
            pctx->Psr, pctx->IntAt, pctx->IntV0, pctx->IntV1);
    NKDbgPrintfW(L"A0=%8.8lx A1=%8.8lx A2=%8.8lx A3=%8.8lx\r\n",
            pctx->IntA0, pctx->IntA1, pctx->IntA2, pctx->IntA3);
    NKDbgPrintfW(L"T0=%8.8lx T1=%8.8lx T2=%8.8lx T3=%8.8lx\r\n",
            pctx->IntT0, pctx->IntT1, pctx->IntT2, pctx->IntT3);
    NKDbgPrintfW(L"T4=%8.8lx T5=%8.8lx T6=%8.8lx T7=%8.8lx\r\n",
            pctx->IntT4, pctx->IntT5, pctx->IntT6, pctx->IntT7);
    NKDbgPrintfW(L"S0=%8.8lx S1=%8.8lx S2=%8.8lx S3=%8.8lx\r\n",
            pctx->IntS0, pctx->IntS1, pctx->IntS2, pctx->IntS3);
    NKDbgPrintfW(L"S4=%8.8lx S5=%8.8lx S6=%8.8lx S7=%8.8lx\r\n",
            pctx->IntS4, pctx->IntS5, pctx->IntS6, pctx->IntS7);
    NKDbgPrintfW(L"T8=%8.8lx T9=%8.8lx LO=%8.8lx HI=%8.8lx\r\n",
            pctx->IntT8, pctx->IntT9, pctx->IntLo, pctx->IntHi);
    NKDbgPrintfW(L"GP=%8.8lx SP=%8.8lx S8=%8.8lx RA=%8.8lx\r\n",
            pctx->IntGp, pctx->IntSp, pctx->IntS8, pctx->IntRa);
    if (level > 1) {
        addr = (pctx->Fir & -4) - 8*4;
        pdw = VerifyAccess((PVOID)addr, VERIFY_KERNEL_OK, CurAKey);
        if (pdw)
            DumpDwords((PDWORD)addr, 12);
    }
}

typedef struct _tlbentry {
    ulong   lo0;
    ulong   lo1;
    ulong   hi;
    ulong   mask;
} TLBENTRY;
typedef TLBENTRY *PTLBENTRY;

// Value of PRId register (least significant 16 bits)
WORD ProcessorRevision;

BOOL HookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    if (hwInterruptNumber > 5)
        return FALSE;
    ISRTable[hwInterruptNumber] = (DWORD)pfnHandler;
    KData.basePSR |= (0x0400 << hwInterruptNumber);
    return TRUE;
}

BOOL UnhookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    extern int DisabledInterruptHandler();
    if (hwInterruptNumber > 5 || ISRTable[hwInterruptNumber] != (DWORD)pfnHandler)
        return FALSE;
    ISRTable[hwInterruptNumber] = (DWORD)DisabledInterruptHandler;
    KData.basePSR &= ~(0x0400 << hwInterruptNumber);
    return TRUE;
}

/* Machine dependent thread creation */

#define STKALIGN 8
#define STKMSK (STKALIGN-1)

// normal thread stack: from top, TLS then args then free

void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, DWORD dwVMBase, BOOL kmode, ulong param) {
    if (!((ulong)lpStack>>VA_SECTION))
        lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
    pTh->dwStackBase = (DWORD)lpStack;
    
    memset(&pTh->ctx, 0xBD, sizeof(pTh->ctx));
    // Leave room for arguments and TLS on the stack
    pTh->ctx.IntSp = (ulong)lpStack + cbStack - (TLS_MINIMUM_AVAILABLE*4) - 4*4;
    pTh->dwStackBound = pTh->ctx.IntSp & ~(PAGE_SIZE-1);
    pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
    pTh->ctx.IntA0 = (ulong)lpStart;
    pTh->ctx.IntA1 = param;
    pTh->ctx.IntK0 = 0;
    pTh->ctx.IntK1 = 0;
    pTh->ctx.IntRa = 0;
    pTh->ctx.Fir = (ULONG)lpBase;
	pTh->ctx.Psr = (kmode || bAllKMode) ? KERNEL_MODE : USER_MODE;
#ifdef MIPS_HAS_FPU
    pTh->ctx.Fsr = 0x01000000; // handle no exceptions
#endif
    pTh->ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
}

// main thread stack: from top, TLS then buf then buf2 then buf2 (ascii) then args then free

LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
    LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2) {
    LPCWSTR pcmdline;
    if (!((ulong)lpStack>>VA_SECTION))
        lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
    pTh->dwStackBase = (DWORD)lpStack;
    pcmdline = (LPCWSTR)((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK));
    memcpy((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK),
        buf,buflen);
    memcpy((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK)-
        ((buflen2+STKMSK)&~STKMSK),buf2,buflen2);
    pTh->pOwnerProc->lpszProcName = (LPWSTR)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK)-((buflen2+STKMSK)&~STKMSK));
    KPlpvTls = pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
    return pcmdline;
}

void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, BOOL kmode,
    ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4) {
    
    memset(&pTh->ctx, 0xBD, sizeof(pTh->ctx));
    // Leave room for arguments on the stack
    pTh->ctx.IntSp = pTh->dwStackBase + cbStack -
            (TLS_MINIMUM_AVAILABLE*4) - 8*4 - ((buflen+STKMSK)&~STKMSK) -
            ((buflen2+STKMSK)&~STKMSK);
    pTh->dwStackBound = pTh->ctx.IntSp & ~(PAGE_SIZE-1);
    pTh->ctx.IntA0 = (ulong)lpStart;
    pTh->ctx.IntA1 = p1;
    pTh->ctx.IntA2 = p2;
    pTh->ctx.IntA3 = pTh->dwStackBase+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK);
    ((LPDWORD)pTh->ctx.IntSp)[4] = p4;
    pTh->ctx.IntK0 = 0;
    pTh->ctx.IntK1 = 0;
    pTh->ctx.IntRa = 0;
    pTh->ctx.Fir = (ULONG)lpBase;
	pTh->ctx.Psr = (kmode || bAllKMode) ? KERNEL_MODE : USER_MODE;
#ifdef MIPS_HAS_FPU
    pTh->ctx.Fsr = 0x01000000; // handle no exceptions
#endif
    pTh->ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
}

typedef struct ExcInfo {
    DWORD   linkage;
    ULONG   oldFir;
    UINT    oldMode;
    CAUSE   causeAndMode;
    ULONG   badVAddr;
    UCHAR	lowSp;
    UCHAR	pad[3];
} EXCINFO;
typedef EXCINFO *PEXCINFO;

ERRFALSE(sizeof(EXCINFO) <= sizeof(CALLSTACK));
ERRFALSE(offsetof(EXCINFO,linkage) == offsetof(CALLSTACK,pcstkNext));
ERRFALSE(offsetof(EXCINFO,oldFir) == offsetof(CALLSTACK,retAddr));
ERRFALSE(offsetof(EXCINFO,oldMode) == offsetof(CALLSTACK,pprcLast));
ERRFALSE(64 >= sizeof(CALLSTACK));

BOOL HandleException(PTHREAD pth, CAUSE cause, ULONG badVAddr) {
    PEXCINFO pexi;
    DWORD stackaddr;
    KCALLPROFON(0);
#if 0
    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx(%8.8lx) Proc=%8.8lx '%s'\r\n",
		cause.XCODE, pCurThread,pth, hCurProc, pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
	NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx RA=%8.8lx BVA=%8.8lx\r\n",
		pCurThread->aky, pth->ctx.Fir, pth->ctx.IntRa, badVAddr);
#endif
    pexi = (struct ExcInfo *)((pth->ctx.IntSp & ~63) - sizeof(CALLSTACK));
    if (!((DWORD)pexi & 0x80000000) && DemandCommit((DWORD)pexi)) {
		stackaddr = (DWORD)pexi & ~(PAGE_SIZE-1);
		if ((stackaddr >= pth->dwStackBound) || (stackaddr < pth->dwStackBase) ||
			((pth->dwStackBound = stackaddr) >= (pth->dwStackBase + MIN_STACK_RESERVE)) ||
			TEST_STACKFAULT(pth)) {
			KCALLPROFOFF(0);
			return 1; // restart instruction
		}
		SET_STACKFAULT(pth);
       	cause.XCODE = 30;   // stack fault exception code
		badVAddr = (DWORD)pexi;
	}
    if (pth->ctx.Fir != (ulong)CaptureContext+4) {
        pexi->causeAndMode = cause;
        pexi->lowSp = (UCHAR)(pth->ctx.IntSp & 63);
        pexi->oldFir = pth->ctx.Fir;
        pexi->oldMode = GetThreadMode(pth);
        pexi->badVAddr = badVAddr;
        pexi->linkage = (DWORD)pCurThread->pcstkTop | 1;
        pCurThread->pcstkTop = (PCALLSTACK)pexi;
        pth->ctx.IntSp = (DWORD)pexi;
        pth->ctx.Psr = KERNEL_MODE;
        pth->ctx.Fir = (ulong)CaptureContext;
        KCALLPROFOFF(0);
        return TRUE;            // continue execution
    }
    DumpFrame(pth, &pth->ctx, cause, badVAddr, 10);
    RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pth));
    SurrenderCritSecs();
    SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
    RunList.pth = 0;
    SetReschedule();
    KCALLPROFOFF(0);
    return 0;
}

typedef struct _EXCARGS {
    DWORD dwExceptionCode;      /* exception code   */
    DWORD dwExceptionFlags;     /* continuable exception flag   */
    DWORD cArguments;           /* number of arguments in array */
    DWORD *lpArguments;         /* address of array of arguments    */
} EXCARGS;
typedef EXCARGS *PEXCARGS;
    
extern BOOL SetCPUHardwareWatch(LPVOID, DWORD);

#ifdef MIPS_HAS_FPU
void SwitchFPUOwner(PCONTEXT pctx) {
	KCALLPROFON(61);
	if (g_CurFPUOwner != pCurThread) {
		if (g_CurFPUOwner)
			SaveFloatContext(g_CurFPUOwner);
		g_CurFPUOwner = pCurThread;
		RestoreFloatContext(pCurThread);
	}
	KCALLPROFOFF(61);
}
#endif

void ExceptionDispatch(PCONTEXT pctx) {
    PTHREAD pth;
    PEXCINFO pexi;
    EXCEPTION_RECORD er;
    ULONG badVAddr;
    int xcode;
    CAUSE cause;
    PEXCARGS pea;
    BOOL bHandled;
    pth = pCurThread;
    
	// Get the EXCINFO off the thread's callstack.
	pexi = (PEXCINFO)pth->pcstkTop;

    DEBUGMSG(ZONE_SEH, (TEXT("ExceptionDispatch: pexi=%8.8lx Fir=%8.8lx\r\n"),pexi, pexi->oldFir));
    // Update CONTEXT with infomation saved in the EXCINFO structure
    pctx->Fir = pexi->oldFir;
    SetContextMode(pctx, pexi->oldMode);
    pctx->IntSp = (ULONG)pctx + sizeof(CONTEXT);
    memset(&er, 0, sizeof(er));
    er.ExceptionAddress = (PVOID)pctx->Fir;
    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    // HandleExcepion sets the LSB of the callstack linkage but ObjectCall
    // does not.
    if (!(pexi->linkage & 1)) {
        


        pea = (PEXCARGS)pctx->IntSp;
        xcode = -1;
        pctx->Fir -= 4;     // to avoid boundary problems at the end of a try block.
        DEBUGMSG(ZONE_SEH, (TEXT("Raising exception %x flags=%x args=%d pexi=%8.8lx\r\n"),
                pea->dwExceptionCode, pea->dwExceptionFlags, pea->cArguments, pexi));
        er.ExceptionCode = pea->dwExceptionCode;
        er.ExceptionFlags = pea->dwExceptionFlags;
        if (pea->lpArguments && pea->cArguments) {
            if (pea->cArguments > EXCEPTION_MAXIMUM_PARAMETERS) {
                er.ExceptionCode = STATUS_INVALID_PARAMETER;
                er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            } else {
                memcpy(er.ExceptionInformation, pea->lpArguments,
                        pea->cArguments*sizeof(DWORD));
                er.NumberParameters = pea->cArguments;
            }
        }
    } else {
        // CPU detected exception. Extract some additional information about
        // the cause of the exception from the EXCINFO (CALLSTACK) structure.
        badVAddr = pexi->badVAddr;
        cause = pexi->causeAndMode;
        xcode = pexi->causeAndMode.XCODE;
        pctx->Fir = pexi->oldFir;
        pctx->IntSp += pexi->lowSp + sizeof(CALLSTACK);
        if (((xcode == 2) || (xcode == 3)) && AutoCommit(badVAddr)) {
			pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
			goto continueExecution;
        }
		switch (xcode) {
        case 0:     // RaiseException or invalid system call
            er.ExceptionCode = STATUS_INVALID_SYSTEM_SERVICE;
            if ((pea = (PEXCARGS)badVAddr) != 0) {
                


                er.ExceptionCode = pea->dwExceptionCode;
                er.ExceptionFlags = pea->dwExceptionFlags;
                if (pea->lpArguments && pea->cArguments) {
                    if (pea->cArguments > EXCEPTION_MAXIMUM_PARAMETERS) {
                        er.ExceptionCode = STATUS_INVALID_PARAMETER;
                        er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                    } else {
                        memcpy(er.ExceptionInformation, pea->lpArguments,
                                pea->cArguments*sizeof(DWORD));
                        er.NumberParameters = pea->cArguments;
                    }
                }
            }
            break;
        case 5:     // Address error (store)
        case 4:     // Address error (load or instruction fetch)
            if (GetContextMode(pctx) != USER_MODE || !(badVAddr&0x80000000)) {
                er.ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }
            goto accessError;
        case 2:     // TLB miss (load or instruction fetch)
        case 3:     // TLB Miss (store)
        case 1:     // TLB modification
            if (ProcessPageFault((xcode==3) || (xcode == 1), badVAddr)) {
                pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
                goto continueExecution;
            }
    accessError:
            er.ExceptionInformation[0] = xcode & 1;
            er.ExceptionCode = STATUS_ACCESS_VIOLATION;
            er.ExceptionInformation[1] = badVAddr;
            er.NumberParameters = 2;
            break;
        case 6:     // Bus error (instruction fetch)
        case 7:     // Bus error (data reference)
            er.ExceptionInformation[0] = xcode & 1;
            er.ExceptionCode = STATUS_ACCESS_VIOLATION;
            er.ExceptionInformation[1] = badVAddr;
            er.NumberParameters = 2;
            break;
        case 9:     // Breakpoint
#ifdef MIPS16SUPPORT
            if (pctx->Fir & 1) {  // MIPS16 mode
                er.ExceptionInformation[0] = *(USHORT *)(pctx->Fir & ~1);
                er.ExceptionCode = STATUS_BREAKPOINT;
                if (er.ExceptionInformation[0] == MIPS16_BREAK (DIVIDE_BY_ZERO_BREAKPOINT)) {
                    er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
                } else if (er.ExceptionInformation[0] == MIPS16_BREAK (MULTIPLY_OVERFLOW_BREAKPOINT) ||
                           er.ExceptionInformation[0] == MIPS16_BREAK (DIVIDE_OVERFLOW_BREAKPOINT)) {
                    er.ExceptionCode = STATUS_INTEGER_OVERFLOW;
                } else if (er.ExceptionInformation[0] == MIPS16_BREAK (BREAKIN_BREAKPOINT)) {
                    pctx->Fir += 2;
                }
            } else
#endif
            {  // MIPS32 mode
            	er.ExceptionInformation[0] = *(ULONG *)pctx->Fir;
	            er.ExceptionCode = STATUS_BREAKPOINT;
                if (er.ExceptionInformation[0] == MIPS32_BREAK (DIVIDE_BY_ZERO_BREAKPOINT))
	                er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
                else if (er.ExceptionInformation[0] == MIPS32_BREAK (MULTIPLY_OVERFLOW_BREAKPOINT) ||
                           er.ExceptionInformation[0] == MIPS32_BREAK (DIVIDE_OVERFLOW_BREAKPOINT))
	                er.ExceptionCode = STATUS_INTEGER_OVERFLOW;
                else if (er.ExceptionInformation[0] == MIPS32_BREAK (BREAKIN_BREAKPOINT))
       	    		pctx->Fir += 4;
            }
            break;
        case 10:    // Reserved instruction
            er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
            break;
        case 12:    // arithmetic overflow
            er.ExceptionCode = STATUS_INTEGER_OVERFLOW;
            break;
#ifdef MIPS_HAS_FPU
		case 11:
			KCall((PKFN)SwitchFPUOwner,pctx);
          	pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
      		goto continueExecution;
        case 15: {
            DWORD code;
            code = GetAndClearFloatCode();
            DEBUGMSG(1,(L"FPU Exception register: %8.8lx\r\n",code));
	        if (code & 0x10000)
                er.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            else if (code & 0x08000)
                er.ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            else if (code & 0x04000)
                er.ExceptionCode = STATUS_FLOAT_OVERFLOW;
            else if (code & 0x02000)
                er.ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            else if (code & 0x01000)
                er.ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            else
                er.ExceptionCode = STATUS_FLOAT_DENORMAL_OPERAND;
            FPUFlushContext();
		    memcpy(&pctx->FltF0,&pth->ctx.FltF0,sizeof(DWORD)*32);
		    pctx->Fsr = pth->ctx.Fsr;
	    	if (HandleHWFloatException(&er,pctx,cause)) {
		    	memcpy(&pth->ctx.FltF0,&pctx->FltF0,sizeof(DWORD)*32);
			    pth->ctx.Fsr = pctx->Fsr;
                pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
		    	goto continueExecution;
		    }
           	break;
        }
#else
		case 11:
            er.ExceptionCode = STATUS_PRIVILEGED_INSTRUCTION;
            break;
#endif      
        case 23:    // watch breakpoint
            er.ExceptionCode = STATUS_BREAKPOINT;
#ifdef MIPS16SUPPORT
            if (pctx->Fir & 1) {  // MIPS16
                er.ExceptionInformation[0] = MIPS16_BREAK (BREAKIN_BREAKPOINT);
                pctx->Fir -= 2;
            } else
#endif
            {
                er.ExceptionInformation[0] = MIPS32_BREAK (BREAKIN_BREAKPOINT);
            }
            pctx->Fir -= 4;
            // If you are using the SetCPUHardware watch function you will probably 
            // want to uncomment the following line so that it will clear the register
            // automatically on exception
//            SetCPUHardwareWatch(0,0);
            break;
        case 30:    // Stack overflow
            er.ExceptionCode = STATUS_STACK_OVERFLOW;
            er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            break;
        }
    }
    if (xcode != 9) {
        NKDbgPrintfW(L"Exception %03x Thread=%8.8lx Proc=%8.8lx\r\n",
                cause.XCODE, pth, hCurProc);
        NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx RA=%8.8lx BVA=%8.8lx\r\n",
                pCurThread->aky, pctx->Fir, pctx->IntRa, badVAddr);
        NKDbgPrintfW(L"Process '%s'\r\n",
                pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
        
        if (UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) {
            NKDbgPrintfW(L"TLSKERN_NOFAULT set... bypassing kernel debugger.\r\n");
        }
    }

    // Invoke the kernel debugger to attempt to debug the exception before
    // letting the program resolve the condition via SEH.
    pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
	if (!UserDbgTrap(&er,pctx,FALSE) && ((UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) || !KDTrap(&er, pctx, FALSE))) {
        if (er.ExceptionCode == STATUS_BREAKPOINT) {
            RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx Ignored.\r\n"), pctx->Fir));
            pctx->Fir += 4;     // skip over the BREAK instruction
        } else {
			bHandled = NKDispatchException(pth, &er, pctx);
            if (!bHandled) {
                if (!UserDbgTrap(&er, pctx, TRUE) && !KDTrap(&er, pctx, TRUE)) {
                    // Terminate the process.
                    RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"),
                            er.ExceptionCode));
                    pctx->IntZero = badVAddr;
                    DumpFrame(pth, (PICONTEXT)&pctx->IntZero, cause, badVAddr, 0);
                    if (InSysCall()) {
                        OutputDebugStringW(L"Halting system\r\n");
                        for (;;)
                            ;
                    } else {
                        if (!GET_DEAD(pth)) {
                            SET_DEAD(pth);
                            pctx->Fir = (ULONG)pExitThread;
                            pctx->IntA0 = 0;
                            RETAILMSG(1, (TEXT("Terminating thread %8.8lx\r\n"), pth));
                        } else {
                            RETAILMSG(1, (TEXT("Can't terminate thread %8.8lx, sleeping forever\r\n"), pth));
                            SurrenderCritSecs();
                            Sleep(INFINITE);
                            DEBUGCHK(0);    // should never get here
                        }
                    }
                }
            }
        }
    }
    if ((xcode == 2) || (xcode == 3))
        GuardCommit(badVAddr);
continueExecution:
    // If returning from handling a stack overflow, reset the thread's stack overflow
    // flag. It would be good to free the tail of the stack at this time
    // so that the thread will stack fault again if the stack gets too big. But we
    // are currently using that stack page.
    if (xcode == 30)
        CLEAR_STACKFAULT(pth);
    if (GET_DYING(pth) && !GET_DEAD(pth) && (pCurProc == pth->pOwnerProc)) {
		SET_DEAD(pth);
		CLEAR_USERBLOCK(pth);
		CLEAR_DEBUGWAIT(pth);
		pctx->Fir = (ULONG)pExitThread;
		pctx->IntA0 = 0;
	}   
}

LONG __C_ExecuteExceptionFilter (
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG EstablisherFrame
    );

VOID __C_ExecuteTerminationHandler (
    BOOLEAN AbnormalTermination,
    TERMINATION_HANDLER TerminationHandler,
    ULONG EstablisherFrame
    );
    
/*++
Routine Description:
    This function scans the scope tables associated with the specified
    procedure and calls exception and termination handlers as necessary.

Arguments:
    ExceptionRecord - Supplies a pointer to an exception record.

    EstablisherFrame - Supplies a pointer to frame of the establisher function.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to the exception dispatcher or
        unwind dispatcher context.

Return Value:
    If the exception is handled by one of the exception filter routines, then
    there is no return from this routine and RtlUnwind is called. Otherwise,
    an exception disposition value of continue execution or continue search is
    returned.
--*/
EXCEPTION_DISPOSITION __C_specific_handler(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext) {
    
    ULONG ControlPc;
    EXCEPTION_FILTER ExceptionFilter;
    EXCEPTION_POINTERS ExceptionPointers;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    PSCOPE_TABLE ScopeTable;
    ULONG TargetPc;
    TERMINATION_HANDLER TerminationHandler;
    LONG Value;
    // Get address of where control left the establisher, the address of the
    // function table entry that describes the function, and the address of
    // the scope table.
    ControlPc = DispatcherContext->ControlPc;
    FunctionEntry = DispatcherContext->FunctionEntry;
    ScopeTable = (PSCOPE_TABLE)(FunctionEntry->HandlerData);
    // If an unwind is not in progress, then scan the scope table and call
    // the appropriate exception filter routines. Otherwise, scan the scope
    // table and call the appropriate termination handlers using the target
    // PC obtained from the context record.
    // are called.
    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags)) {
        // Scan the scope table and call the appropriate exception filter
        // routines.
        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord = ContextRecord;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress) &&
                ScopeTable->ScopeRecord[Index].JumpTarget) {
                // Call the exception filter routine.
                ExceptionFilter = (EXCEPTION_FILTER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                Value = __C_ExecuteExceptionFilter(&ExceptionPointers, ExceptionFilter, (ULONG)EstablisherFrame);
                // If the return value is less than zero, then dismiss the
                // exception. Otherwise, if the value is greater than zero,
                // then unwind to the target exception handler. Otherwise,
                // continue the search for an exception filter.
                if (Value < 0)
                    return ExceptionContinueExecution;
                if (Value > 0) {
                    DispatcherContext->ControlPc = ScopeTable->ScopeRecord[Index].JumpTarget;
                    return ExceptionExecuteHandler;
                }
            }
        }
    } else {
        // Scan the scope table and call the appropriate termination handler
        // routines.
        TargetPc = ContextRecord->Fir;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                    (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress)) {
                // If the target PC is within the same scope the control PC
                // is within, then this is an uplevel goto out of an inner try
                // scope or a long jump back into a try scope. Terminate the
                // scan termination handlers.
                //
                // N.B. The target PC can be just beyond the end of the scope,
                //      in which case it is a leave from the scope.
                if ((TargetPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                    (TargetPc < ScopeTable->ScopeRecord[Index].EndAddress))
                    break;
                // If the scope table entry describes an exception filter
                // and the associated exception handler is the target of
                // the unwind, then terminate the scan for termination
                // handlers. Otherwise, if the scope table entry describes
                // a termination handler, then record the address of the
                // end of the scope as the new control PC address and call
                // the termination handler.
                if (ScopeTable->ScopeRecord[Index].JumpTarget) {
                    if (TargetPc == ScopeTable->ScopeRecord[Index].JumpTarget)
                        break;
                } else {
                    DispatcherContext->ControlPc =
                            ScopeTable->ScopeRecord[Index].EndAddress + 4;
                    TerminationHandler = (TERMINATION_HANDLER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                    __C_ExecuteTerminationHandler(TRUE, TerminationHandler, (ULONG)EstablisherFrame);
                }
            }
        }
    }
    // Continue search for exception or termination handlers.
    return ExceptionContinueSearch;
}

#ifdef MIPS_HAS_FPU
void FPUFlushContext(void) {
    if (g_CurFPUOwner) {
        SaveFloatContext(g_CurFPUOwner);
        g_CurFPUOwner = 0;
    }
}
#endif

BOOL DoThreadGetContext(HANDLE hTh, LPCONTEXT lpContext) {
    PTHREAD pth;
    if (!(pth = HandleToThread(hTh))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (lpContext->ContextFlags & ~CONTEXT_FULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
        ACCESSKEY ulOldKey;
        SWITCHKEY(ulOldKey,0xffffffff);
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
#ifdef MIPS_HAS_FPU
            FPUFlushContext();
            lpContext->Fsr = pth->ctx.Fsr;
            lpContext->FltF0 = pth->ctx.FltF0;
            lpContext->FltF1 = pth->ctx.FltF1;
            lpContext->FltF2 = pth->ctx.FltF2;
            lpContext->FltF3 = pth->ctx.FltF3;
            lpContext->FltF4 = pth->ctx.FltF4;
            lpContext->FltF5 = pth->ctx.FltF5;
            lpContext->FltF6 = pth->ctx.FltF6;
            lpContext->FltF7 = pth->ctx.FltF7;
            lpContext->FltF8 = pth->ctx.FltF8;
            lpContext->FltF9 = pth->ctx.FltF9;
            lpContext->FltF10 = pth->ctx.FltF10;
            lpContext->FltF11 = pth->ctx.FltF11;
            lpContext->FltF12 = pth->ctx.FltF12;
            lpContext->FltF13 = pth->ctx.FltF13;
            lpContext->FltF14 = pth->ctx.FltF14;
            lpContext->FltF15 = pth->ctx.FltF15;
            lpContext->FltF16 = pth->ctx.FltF16;
            lpContext->FltF17 = pth->ctx.FltF17;
            lpContext->FltF18 = pth->ctx.FltF18;
            lpContext->FltF19 = pth->ctx.FltF19;
            lpContext->FltF20 = pth->ctx.FltF20;
            lpContext->FltF21 = pth->ctx.FltF21;
            lpContext->FltF22 = pth->ctx.FltF22;
            lpContext->FltF23 = pth->ctx.FltF23;
            lpContext->FltF24 = pth->ctx.FltF24;
            lpContext->FltF25 = pth->ctx.FltF25;
            lpContext->FltF26 = pth->ctx.FltF26;
            lpContext->FltF27 = pth->ctx.FltF27;
            lpContext->FltF28 = pth->ctx.FltF28;
            lpContext->FltF29 = pth->ctx.FltF29;
            lpContext->FltF30 = pth->ctx.FltF30;
            lpContext->FltF31 = pth->ctx.FltF31;
#endif
        }
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            lpContext->IntGp = pth->pThrdDbg->psavedctx->IntGp;
            lpContext->IntSp = pth->pThrdDbg->psavedctx->IntSp;
            lpContext->IntRa = pth->pThrdDbg->psavedctx->IntRa;
            lpContext->Fir = pth->pThrdDbg->psavedctx->Fir;
            lpContext->Psr = pth->pThrdDbg->psavedctx->Psr;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            lpContext->IntZero = 0;
            lpContext->IntAt = pth->pThrdDbg->psavedctx->IntAt;
            lpContext->IntV0 = pth->pThrdDbg->psavedctx->IntV0;
            lpContext->IntV1 = pth->pThrdDbg->psavedctx->IntV1;
            lpContext->IntA0 = pth->pThrdDbg->psavedctx->IntA0;
            lpContext->IntA1 = pth->pThrdDbg->psavedctx->IntA1;
            lpContext->IntA2 = pth->pThrdDbg->psavedctx->IntA2;
            lpContext->IntA3 = pth->pThrdDbg->psavedctx->IntA3;
            lpContext->IntT0 = pth->pThrdDbg->psavedctx->IntT0;
            lpContext->IntT1 = pth->pThrdDbg->psavedctx->IntT1;
            lpContext->IntT2 = pth->pThrdDbg->psavedctx->IntT2;
            lpContext->IntT3 = pth->pThrdDbg->psavedctx->IntT3;
            lpContext->IntT4 = pth->pThrdDbg->psavedctx->IntT4;
            lpContext->IntT5 = pth->pThrdDbg->psavedctx->IntT5;
            lpContext->IntT6 = pth->pThrdDbg->psavedctx->IntT6;
            lpContext->IntT7 = pth->pThrdDbg->psavedctx->IntT7;
            lpContext->IntS0 = pth->pThrdDbg->psavedctx->IntS0;
            lpContext->IntS1 = pth->pThrdDbg->psavedctx->IntS1;
            lpContext->IntS2 = pth->pThrdDbg->psavedctx->IntS2;
            lpContext->IntS3 = pth->pThrdDbg->psavedctx->IntS3;
            lpContext->IntS4 = pth->pThrdDbg->psavedctx->IntS4;
            lpContext->IntS5 = pth->pThrdDbg->psavedctx->IntS5;
            lpContext->IntS6 = pth->pThrdDbg->psavedctx->IntS6;
            lpContext->IntS7 = pth->pThrdDbg->psavedctx->IntS7;
            lpContext->IntT8 = pth->pThrdDbg->psavedctx->IntT8;
            lpContext->IntT9 = pth->pThrdDbg->psavedctx->IntT9;
            lpContext->IntS8 = pth->pThrdDbg->psavedctx->IntS8;
            lpContext->IntLo = pth->pThrdDbg->psavedctx->IntLo;
            lpContext->IntHi = pth->pThrdDbg->psavedctx->IntHi;
        }
        SETCURKEY(ulOldKey);
    } else {
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
#ifdef MIPS_HAS_FPU
            FPUFlushContext();
            lpContext->Fsr = pth->ctx.Fsr;
            lpContext->FltF0 = pth->ctx.FltF0;
            lpContext->FltF1 = pth->ctx.FltF1;
            lpContext->FltF2 = pth->ctx.FltF2;
            lpContext->FltF3 = pth->ctx.FltF3;
            lpContext->FltF4 = pth->ctx.FltF4;
            lpContext->FltF5 = pth->ctx.FltF5;
            lpContext->FltF6 = pth->ctx.FltF6;
            lpContext->FltF7 = pth->ctx.FltF7;
            lpContext->FltF8 = pth->ctx.FltF8;
            lpContext->FltF9 = pth->ctx.FltF9;
            lpContext->FltF10 = pth->ctx.FltF10;
            lpContext->FltF11 = pth->ctx.FltF11;
            lpContext->FltF12 = pth->ctx.FltF12;
            lpContext->FltF13 = pth->ctx.FltF13;
            lpContext->FltF14 = pth->ctx.FltF14;
            lpContext->FltF15 = pth->ctx.FltF15;
            lpContext->FltF16 = pth->ctx.FltF16;
            lpContext->FltF17 = pth->ctx.FltF17;
            lpContext->FltF18 = pth->ctx.FltF18;
            lpContext->FltF19 = pth->ctx.FltF19;
            lpContext->FltF20 = pth->ctx.FltF20;
            lpContext->FltF21 = pth->ctx.FltF21;
            lpContext->FltF22 = pth->ctx.FltF22;
            lpContext->FltF23 = pth->ctx.FltF23;
            lpContext->FltF24 = pth->ctx.FltF24;
            lpContext->FltF25 = pth->ctx.FltF25;
            lpContext->FltF26 = pth->ctx.FltF26;
            lpContext->FltF27 = pth->ctx.FltF27;
            lpContext->FltF28 = pth->ctx.FltF28;
            lpContext->FltF29 = pth->ctx.FltF29;
            lpContext->FltF30 = pth->ctx.FltF30;
            lpContext->FltF31 = pth->ctx.FltF31;
#endif
        }
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            lpContext->IntGp = pth->ctx.IntGp;
            lpContext->IntSp = pth->ctx.IntSp;
            lpContext->IntRa = pth->ctx.IntRa;
            lpContext->Fir = pth->ctx.Fir;
            lpContext->Psr = pth->ctx.Psr;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            lpContext->IntZero = 0;
            lpContext->IntAt = pth->ctx.IntAt;
            lpContext->IntV0 = pth->ctx.IntV0;
            lpContext->IntV1 = pth->ctx.IntV1;
            lpContext->IntA0 = pth->ctx.IntA0;
            lpContext->IntA1 = pth->ctx.IntA1;
            lpContext->IntA2 = pth->ctx.IntA2;
            lpContext->IntA3 = pth->ctx.IntA3;
            lpContext->IntT0 = pth->ctx.IntT0;
            lpContext->IntT1 = pth->ctx.IntT1;
            lpContext->IntT2 = pth->ctx.IntT2;
            lpContext->IntT3 = pth->ctx.IntT3;
            lpContext->IntT4 = pth->ctx.IntT4;
            lpContext->IntT5 = pth->ctx.IntT5;
            lpContext->IntT6 = pth->ctx.IntT6;
            lpContext->IntT7 = pth->ctx.IntT7;
            lpContext->IntS0 = pth->ctx.IntS0;
            lpContext->IntS1 = pth->ctx.IntS1;
            lpContext->IntS2 = pth->ctx.IntS2;
            lpContext->IntS3 = pth->ctx.IntS3;
            lpContext->IntS4 = pth->ctx.IntS4;
            lpContext->IntS5 = pth->ctx.IntS5;
            lpContext->IntS6 = pth->ctx.IntS6;
            lpContext->IntS7 = pth->ctx.IntS7;
            lpContext->IntT8 = pth->ctx.IntT8;
            lpContext->IntT9 = pth->ctx.IntT9;
            lpContext->IntS8 = pth->ctx.IntS8;
            lpContext->IntLo = pth->ctx.IntLo;
            lpContext->IntHi = pth->ctx.IntHi;
        }
    }
    return TRUE;
}

BOOL DoThreadSetContext(HANDLE hTh, const CONTEXT *lpContext) {
    PTHREAD pth;
    if (!(pth = HandleToThread(hTh))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (lpContext->ContextFlags & ~CONTEXT_FULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
        ACCESSKEY ulOldKey;
        SWITCHKEY(ulOldKey,0xffffffff);
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
#ifdef MIPS_HAS_FPU
            FPUFlushContext();
            pth->ctx.Fsr = lpContext->Fsr;
            pth->ctx.FltF0 = lpContext->FltF0;
            pth->ctx.FltF1 = lpContext->FltF1;
            pth->ctx.FltF2 = lpContext->FltF2;
            pth->ctx.FltF3 = lpContext->FltF3;
            pth->ctx.FltF4 = lpContext->FltF4;
            pth->ctx.FltF5 = lpContext->FltF5;
            pth->ctx.FltF6 = lpContext->FltF6;
            pth->ctx.FltF7 = lpContext->FltF7;
            pth->ctx.FltF8 = lpContext->FltF8;
            pth->ctx.FltF9 = lpContext->FltF9;
            pth->ctx.FltF10 = lpContext->FltF10;
            pth->ctx.FltF11 = lpContext->FltF11;
            pth->ctx.FltF12 = lpContext->FltF12;
            pth->ctx.FltF13 = lpContext->FltF13;
            pth->ctx.FltF14 = lpContext->FltF14;
            pth->ctx.FltF15 = lpContext->FltF15;
            pth->ctx.FltF16 = lpContext->FltF16;
            pth->ctx.FltF17 = lpContext->FltF17;
            pth->ctx.FltF18 = lpContext->FltF18;
            pth->ctx.FltF19 = lpContext->FltF19;
            pth->ctx.FltF20 = lpContext->FltF20;
            pth->ctx.FltF21 = lpContext->FltF21;
            pth->ctx.FltF22 = lpContext->FltF22;
            pth->ctx.FltF23 = lpContext->FltF23;
            pth->ctx.FltF24 = lpContext->FltF24;
            pth->ctx.FltF25 = lpContext->FltF25;
            pth->ctx.FltF26 = lpContext->FltF26;
            pth->ctx.FltF27 = lpContext->FltF27;
            pth->ctx.FltF28 = lpContext->FltF28;
            pth->ctx.FltF29 = lpContext->FltF29;
            pth->ctx.FltF30 = lpContext->FltF30;
            pth->ctx.FltF31 = lpContext->FltF31;
#endif
        }
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            pth->pThrdDbg->psavedctx->IntGp = lpContext->IntGp;
            pth->pThrdDbg->psavedctx->IntSp = lpContext->IntSp;
            pth->pThrdDbg->psavedctx->IntRa = lpContext->IntRa;
            pth->pThrdDbg->psavedctx->Fir = lpContext->Fir;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            pth->pThrdDbg->psavedctx->IntAt = lpContext->IntAt;
            pth->pThrdDbg->psavedctx->IntV0 = lpContext->IntV0;
            pth->pThrdDbg->psavedctx->IntV1 = lpContext->IntV1;
            pth->pThrdDbg->psavedctx->IntA0 = lpContext->IntA0;
            pth->pThrdDbg->psavedctx->IntA1 = lpContext->IntA1;
            pth->pThrdDbg->psavedctx->IntA2 = lpContext->IntA2;
            pth->pThrdDbg->psavedctx->IntA3 = lpContext->IntA3;
            pth->pThrdDbg->psavedctx->IntT0 = lpContext->IntT0;
            pth->pThrdDbg->psavedctx->IntT1 = lpContext->IntT1;
            pth->pThrdDbg->psavedctx->IntT2 = lpContext->IntT2;
            pth->pThrdDbg->psavedctx->IntT3 = lpContext->IntT3;
            pth->pThrdDbg->psavedctx->IntT4 = lpContext->IntT4;
            pth->pThrdDbg->psavedctx->IntT5 = lpContext->IntT5;
            pth->pThrdDbg->psavedctx->IntT6 = lpContext->IntT6;
            pth->pThrdDbg->psavedctx->IntT7 = lpContext->IntT7;
            pth->pThrdDbg->psavedctx->IntS0 = lpContext->IntS0;
            pth->pThrdDbg->psavedctx->IntS1 = lpContext->IntS1;
            pth->pThrdDbg->psavedctx->IntS2 = lpContext->IntS2;
            pth->pThrdDbg->psavedctx->IntS3 = lpContext->IntS3;
            pth->pThrdDbg->psavedctx->IntS4 = lpContext->IntS4;
            pth->pThrdDbg->psavedctx->IntS5 = lpContext->IntS5;
            pth->pThrdDbg->psavedctx->IntS6 = lpContext->IntS6;
            pth->pThrdDbg->psavedctx->IntS7 = lpContext->IntS7;
            pth->pThrdDbg->psavedctx->IntT8 = lpContext->IntT8;
            pth->pThrdDbg->psavedctx->IntT9 = lpContext->IntT9;
            pth->pThrdDbg->psavedctx->IntS8 = lpContext->IntS8;
            pth->pThrdDbg->psavedctx->IntLo = lpContext->IntLo;
            pth->pThrdDbg->psavedctx->IntHi = lpContext->IntHi;
        }
        SETCURKEY(ulOldKey);
    } else {
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
#ifdef MIPS_HAS_FPU
            FPUFlushContext();
            pth->ctx.Fsr = lpContext->Fsr;
            pth->ctx.FltF0 = lpContext->FltF0;
            pth->ctx.FltF1 = lpContext->FltF1;
            pth->ctx.FltF2 = lpContext->FltF2;
            pth->ctx.FltF3 = lpContext->FltF3;
            pth->ctx.FltF4 = lpContext->FltF4;
            pth->ctx.FltF5 = lpContext->FltF5;
            pth->ctx.FltF6 = lpContext->FltF6;
            pth->ctx.FltF7 = lpContext->FltF7;
            pth->ctx.FltF8 = lpContext->FltF8;
            pth->ctx.FltF9 = lpContext->FltF9;
            pth->ctx.FltF10 = lpContext->FltF10;
            pth->ctx.FltF11 = lpContext->FltF11;
            pth->ctx.FltF12 = lpContext->FltF12;
            pth->ctx.FltF13 = lpContext->FltF13;
            pth->ctx.FltF14 = lpContext->FltF14;
            pth->ctx.FltF15 = lpContext->FltF15;
            pth->ctx.FltF16 = lpContext->FltF16;
            pth->ctx.FltF17 = lpContext->FltF17;
            pth->ctx.FltF18 = lpContext->FltF18;
            pth->ctx.FltF19 = lpContext->FltF19;
            pth->ctx.FltF20 = lpContext->FltF20;
            pth->ctx.FltF21 = lpContext->FltF21;
            pth->ctx.FltF22 = lpContext->FltF22;
            pth->ctx.FltF23 = lpContext->FltF23;
            pth->ctx.FltF24 = lpContext->FltF24;
            pth->ctx.FltF25 = lpContext->FltF25;
            pth->ctx.FltF26 = lpContext->FltF26;
            pth->ctx.FltF27 = lpContext->FltF27;
            pth->ctx.FltF28 = lpContext->FltF28;
            pth->ctx.FltF29 = lpContext->FltF29;
            pth->ctx.FltF30 = lpContext->FltF30;
            pth->ctx.FltF31 = lpContext->FltF31;
#endif
        }
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            pth->ctx.IntGp = lpContext->IntGp;
            pth->ctx.IntSp = lpContext->IntSp;
            pth->ctx.IntRa = lpContext->IntRa;
            pth->ctx.Fir = lpContext->Fir;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            pth->ctx.IntAt = lpContext->IntAt;
            pth->ctx.IntV0 = lpContext->IntV0;
            pth->ctx.IntV1 = lpContext->IntV1;
            pth->ctx.IntA0 = lpContext->IntA0;
            pth->ctx.IntA1 = lpContext->IntA1;
            pth->ctx.IntA2 = lpContext->IntA2;
            pth->ctx.IntA3 = lpContext->IntA3;
            pth->ctx.IntT0 = lpContext->IntT0;
            pth->ctx.IntT1 = lpContext->IntT1;
            pth->ctx.IntT2 = lpContext->IntT2;
            pth->ctx.IntT3 = lpContext->IntT3;
            pth->ctx.IntT4 = lpContext->IntT4;
            pth->ctx.IntT5 = lpContext->IntT5;
            pth->ctx.IntT6 = lpContext->IntT6;
            pth->ctx.IntT7 = lpContext->IntT7;
            pth->ctx.IntS0 = lpContext->IntS0;
            pth->ctx.IntS1 = lpContext->IntS1;
            pth->ctx.IntS2 = lpContext->IntS2;
            pth->ctx.IntS3 = lpContext->IntS3;
            pth->ctx.IntS4 = lpContext->IntS4;
            pth->ctx.IntS5 = lpContext->IntS5;
            pth->ctx.IntS6 = lpContext->IntS6;
            pth->ctx.IntS7 = lpContext->IntS7;
            pth->ctx.IntT8 = lpContext->IntT8;
            pth->ctx.IntT9 = lpContext->IntT9;
            pth->ctx.IntS8 = lpContext->IntS8;
            pth->ctx.IntLo = lpContext->IntLo;
            pth->ctx.IntHi = lpContext->IntHi;
        }
    }
    return TRUE;
}

