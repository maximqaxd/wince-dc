/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

#include "kernel.h"
#include "shx.h"

extern void UnusedHandler(void);
extern void OEMInitDebugSerial(void);
extern void InitClock(void);
extern void FlushTLB(void);
extern void LoadKPage(void);
extern void DumpFrame(PTHREAD pth, PCONTEXT pctx, DWORD dwExc, DWORD info);
extern void APICallReturn(void);

#ifdef SH4
void SaveFloatContext(PTHREAD);
void RestoreFloatContext(PTHREAD);
DWORD GetAndClearFloatCode(void);
DWORD GetAndClearFloatCode(void);
DWORD GetCauseFloatCode(void);
extern BOOL HandleHWFloatException(EXCEPTION_RECORD *ExceptionRecord,
                                   PCONTEXT pctx);
extern unsigned int get_fpscr();
extern void set_fpscr(unsigned int);
extern void clr_fpscr(unsigned int);
#endif

#ifdef SH3
// frequency control register value
extern unsigned short SH3FQCR_Fast;
extern unsigned int SH3DSP;
void SaveSH3DSPContext(PTHREAD);
void RestoreSH3DSPContext(PTHREAD);
#endif

// OEM definable extra bits for the Cache Control Register
unsigned long OEMExtraCCR;

extern char InterlockedAPIs[], InterlockedEnd[];

void SH3Init(int cpuType) {
    int ix;
#ifdef SH3
    /* initialize frequency control register */
    if (SH3FQCR_Fast)
        *(volatile ushort *)0xffffff80 = SH3FQCR_Fast;
#endif

    /* Disable the cpu cache & flush it. */
    CCR = 0;
    CCR = CACHE_FLUSH;

    /* Zero out kernel data page. */
    memset(&KData, 0, sizeof(KData));
    KData.handleBase = 0x80000000;
    KData.pAPIReturn = (ulong)APICallReturn;

    /* Initialize SectionTable in KPage */
    for (ix = 1 ; ix <= SECTION_MASK ; ++ix)
        SectionTable[ix] = NULL_SECTION;

    /* Copy kernel data to RAM & zero out BSS */
    KernelRelocate(pTOC);

    OEMInitDebugSerial();			// initialize serial port
    OEMWriteDebugString(TEXT("\r\nWindows CE Kernel for Hitachi SH Built on ") TEXT(__DATE__)
			TEXT(" at ") TEXT(__TIME__) TEXT("\r\n"));
#if defined(SH4)
    OEMWriteDebugString(TEXT("SH-4 Kernel\r\n"));
#else
    NKDbgPrintfW(L"SH-3 Kernel. FQCR=%x\r\n", *(volatile ushort *)0xffffff80);
#endif

    /* Initialize address translation hardware. */
    MMUTEA = 0;			/* clear transation address */
    MMUTTB = (DWORD)SectionTable; /* set translation table base address */
    MMUPTEH = 0;		/* clear ASID */
    MMUCR = TLB_FLUSH | TLB_ENABLE;
    LoadKPage();

    /* Copy interlocked api code into the kpage */
    DEBUGCHK(sizeof(KData) <= FIRST_INTERLOCK);
    DEBUGCHK((InterlockedEnd-InterlockedAPIs)+FIRST_INTERLOCK == 0x400);
    memcpy((char *)&KData+FIRST_INTERLOCK, InterlockedAPIs, InterlockedEnd-InterlockedAPIs);

    // Enable the CPU cache. Can't do this before KernelRelocate because OEMExtraCCR
    // may not be properly initialized before that point.
    CCR = CACHE_ENABLE | OEMExtraCCR;
    NKDbgPrintfW(L"CCR=%4.4x\r\n", CCR);

    OEMInit();			// initialize firmware
    KernelFindMemory();
#ifdef DEBUG
    OEMWriteDebugString(TEXT("SH3Init done.\r\n"));
#endif
}

BOOL HookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    if (hwInterruptNumber > 112)
        return FALSE;
    InterruptTable[hwInterruptNumber] = (DWORD)pfnHandler;
    return TRUE;
}

BOOL UnhookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    if (hwInterruptNumber > 112 ||
        InterruptTable[hwInterruptNumber] != (DWORD)pfnHandler)
        return FALSE;
    InterruptTable[hwInterruptNumber] = (DWORD)UnusedHandler;
    return TRUE;
}

#ifdef SH4
void FlushCache(void) {
    FlushDCache();
    FlushICache();
}

void SwitchFPUOwner(PCONTEXT pctx) {
    KCALLPROFON(61);
    if (g_CurFPUOwner != pCurThread) {
        if (g_CurFPUOwner)
            SaveFloatContext(g_CurFPUOwner);
        g_CurFPUOwner = pCurThread;
        RestoreFloatContext(pCurThread);
    }
    KCALLPROFOFF(61);
    pctx->Psr &= ~0x8000;
}

void FPUFlushContext(void) {
    if (g_CurFPUOwner) {
        SaveFloatContext(g_CurFPUOwner);
        g_CurFPUOwner->ctx.Psr |= 0x8000;
        g_CurFPUOwner = 0;
    }
}

DWORD dwStoreQueueBase;
BOOL DoSetRAMMode(BOOL bEnable, LPVOID *lplpvAddress, LPDWORD lpLength);

BOOL SC_SetRAMMode(BOOL bEnable, LPVOID *lplpvAddress, LPDWORD lpLength) {
    if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
        ERRORMSG(1,(L"SC_SetRAMMode failed due to insufficient trust\r\n"));
        KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
    }
    return DoSetRAMMode(bEnable, lplpvAddress, lpLength);
}

LPVOID SC_SetStoreQueueBase(DWORD dwPhysPage) {
    if (pCurProc->bTrustLevel != KERN_TRUST_FULL) {
        ERRORMSG(1,(L"SC_SetStoreQueueBase failed due to insufficient trust\r\n"));
        KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
    	return 0;
    }
    if (dwPhysPage & (1024*1024-1)) {
        KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (dwPhysPage & 0xe0000000) {
        KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
        return 0;
    }
    dwStoreQueueBase = dwPhysPage | PG_VALID_MASK | PG_1M_MASK | PG_PROT_WRITE | PG_DIRTY_MASK | 1;
    FlushTLB();
    return (LPVOID)0xE0000000;
}

#else   // !SH4

void SwitchDSPOwner(PCONTEXT pctx) {
    KCALLPROFON(68);
    if (g_CurDSPOwner != pCurThread) {
        if (g_CurDSPOwner)
            SaveSH3DSPContext(g_CurDSPOwner);
        g_CurDSPOwner = pCurThread;
        RestoreSH3DSPContext(pCurThread);
    }
    pctx->Psr |= 0x1000;            // enable the DSP
    KCALLPROFOFF(68);
}

#endif

void FlushTLB(void) {
    MMUCR = TLB_FLUSH | TLB_ENABLE;
#if defined(SH4) || (PAGE_SIZE == 1024)
    FlushCache();
#endif
}

typedef struct ExcInfo {
    DWORD   linkage;
    ULONG	oldFir;
    UINT    oldMode;
    UCHAR	exc;
    UCHAR	lowSp;
    UCHAR	pad[2];
    ULONG	info;
} EXCINFO, *PEXCINFO;

ERRFALSE(sizeof(EXCINFO) <= sizeof(CALLSTACK));
ERRFALSE(offsetof(EXCINFO,linkage) == offsetof(CALLSTACK,pcstkNext));
ERRFALSE(offsetof(EXCINFO,oldFir) == offsetof(CALLSTACK,retAddr));
ERRFALSE(offsetof(EXCINFO,oldMode) == offsetof(CALLSTACK,pprcLast));
ERRFALSE(64 >= sizeof(CALLSTACK));

BOOL HandleException(PTHREAD pth, DWORD dwExc, DWORD info) {
    PEXCINFO pexi;
    DWORD stackaddr;
    KCALLPROFON(0);
#if 0
    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx(%8.8lx) PC=%8.8lx TEA=%8.8lx AKY=%8.8lx\r\n",
                 dwExc, pCurThread, pth, pth->ctx.Fir, info, CurAKey);
#endif
    // avoid crossing page boundary in structure
    pexi = (struct ExcInfo *)((pth->ctx.R15 & ~63) - sizeof(CALLSTACK));
    if (!((DWORD)pexi & 0x80000000) && DemandCommit((DWORD)pexi)) {
        stackaddr = (DWORD)pexi & ~(PAGE_SIZE-1);
        if ((stackaddr >= pth->dwStackBound) || (stackaddr < pth->dwStackBase) ||
            ((pth->dwStackBound = stackaddr) >= (pth->dwStackBase + MIN_STACK_RESERVE)) ||
            TEST_STACKFAULT(pth)) {
            KCALLPROFOFF(0);
            return 1; // restart instruction
        }
        SET_STACKFAULT(pth);
        dwExc = 0x1fe0;
        info = (DWORD)pexi;
    }
    // Setup to capture the exception context in kernel mode but
    // running in thread context to allow preemption and stack growth.
    if (pth->ctx.Fir != (ulong)CaptureContext+4) {
        pexi->exc = (UCHAR)(dwExc >> 5);
        pexi->lowSp = (UCHAR)(pth->ctx.R15 & 63);
        pexi->oldFir = pth->ctx.Fir;
        pexi->oldMode = GetThreadMode(pth);
        pexi->info = info;
        pexi->linkage = (DWORD)pCurThread->pcstkTop | 1;
        pCurThread->pcstkTop = (PCALLSTACK)pexi;
        pth->ctx.R15 = (DWORD)pexi;
        pth->ctx.Psr |= 0x40000000;	// Kernel mode
        pth->ctx.Fir = (ulong)CaptureContext;
        KCALLPROFOFF(0);
        return TRUE;			// continue execution
    }
    DumpFrame(pth, (PCONTEXT)&pth->ctx, dwExc, info);
    RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pCurThread));
    SurrenderCritSecs();
    DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
    SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
    RunList.pth = 0;
    SetReschedule();
    KCALLPROFOFF(0);
    return 0;
}

typedef struct _EXCARGS {
    DWORD dwExceptionCode;		/* exception code	*/
    DWORD dwExceptionFlags;		/* continuable exception flag	*/
    DWORD cArguments;			/* number of arguments in array	*/
    DWORD *lpArguments;			/* address of array of arguments	*/
} EXCARGS, *PEXCARGS;

void ExceptionDispatch(PCONTEXT pctx) {
    EXCEPTION_RECORD er;
    int exc;
    ULONG info;
    BOOL bHandled;
    PEXCARGS pea;
    PTHREAD pth; 
    PEXCINFO pexi;

    pth = pCurThread;
    pexi = (PEXCINFO)pth->pcstkTop;
    DEBUGMSG(ZONE_SEH, (TEXT("ExceptionDispatch: pexi=%8.8lx Fir=%8.8lx RA=%8.8lx exc=%x\r\n"),
			pexi, pexi->oldFir, pctx->PR, pexi->exc << 5));

    // Update CONTEXT with infomation saved in the EXCINFO structure
    pctx->Fir = pexi->oldFir;
    if (pexi->oldMode != KERNEL_MODE)
        pctx->Psr &= ~0x40000000;
    pctx->R15 = (ULONG)pctx + sizeof(CONTEXT);
    memset(&er, 0, sizeof(er));
    er.ExceptionAddress = (PVOID)pctx->Fir;

    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    // HandleExcepion sets the LSB of the callstack linkage but ObjectCall
    // does not.
    if (!(pexi->linkage & 1)) {
        


        pea = (PEXCARGS)pctx->R15;
        exc = -1;
        pctx->Fir -= 2;     // to avoid boundary problems at the end of a try block.
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
        pctx->R15 += pexi->lowSp + sizeof(CALLSTACK);
        exc = pexi->exc;
        info = pexi->info;

        // Construct an EXCEPTION_RECORD from the EXCINFO structure
        er.ExceptionInformation[1] = info;
        // TLB Miss on load or store. Attempt to auto-commit the page. If that fails,
        // fall through into general exception processing.
        if (((exc == 0x3) || (exc == 2)) && AutoCommit(info)) {
            pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
            goto continueExecution;
        }
        switch (exc) {
        case 8:		// Address error (store)
            er.ExceptionInformation[0] = 1;
        case 7:		// Address error (load or instruction fetch)
            if (GetContextMode(pctx) != USER_MODE || !(info&0x80000000)) {
                er.ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }
            goto accessError;

        case 3:		// TLB Miss (store)
        case 4:		// TLB modification
        case 6:		// TLB protection violation (store)
            er.ExceptionInformation[0] = 1;
        case 2:		// TLB miss (load or instruction fetch)
        case 5:		// TLB protection violation (load)
            if (ProcessPageFault((exc==3)||(exc==4)||(exc==6), info)) {
                pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
                goto continueExecution;
            }
	accessError:        	
            er.ExceptionCode = STATUS_ACCESS_VIOLATION;
            er.NumberParameters = 2;
            break;
#ifdef SH4
        case 0x40:
        case 0x41:
            KCall((PKFN)SwitchFPUOwner,pctx);
            pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
            goto continueExecution;
        case 9: {			// floating point exception
            DWORD code;
            code = GetCauseFloatCode();
#if 0
            NKDbgPrintfW(L"ExceptionAddress 0x%x\r\n", er.ExceptionAddress);
            NKDbgPrintfW(L"code 0x%x, exc 0x%x\r\n",code,exc);
#endif
            if (code & 0x10)
                er.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            else if (code & 0x8)
                er.ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            else if (code & 0x4)
                er.ExceptionCode = STATUS_FLOAT_OVERFLOW;
            else if (code & 0x2)
                er.ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            else if (code & 0x1)
                er.ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            else
            {
                //
                // Reach here 
                // --if code is 0x20 (FPU error)
                // --if code is 0x0 (processor thinks an ieee
                //   exception is possible)
                //
                // both cases require that fp operation be emulated
                // for correct ieee result.
                //
                // save the fpscr before FPUFlushContext will flush it
                // to zero.
                //
                // FPUFlushContext clears fpscr
                //
                FPUFlushContext();

                //
                // Copy current thread fregs and xfreg to user context
                //
                memcpy(&pctx->FRegs[0],&pCurThread->ctx.FRegs[0],sizeof(DWORD)*16);
                memcpy(&pctx->xFRegs[0],&pCurThread->ctx.xFRegs[0],sizeof(DWORD)*16);
                pctx->Fpscr = pCurThread->ctx.Fpscr;
                pctx->Fpul = pCurThread->ctx.Fpul;

                if (HandleHWFloatException(&er,pctx)) 
                {
                    //
                    // update current thread context with user context
                    //
                    memcpy(&pCurThread->ctx.FRegs[0],&pctx->FRegs[0],sizeof(DWORD)*16);
                    memcpy(&pCurThread->ctx.xFRegs[0],&pctx->xFRegs[0],sizeof(DWORD)*16);

                    pCurThread->ctx.Fpul = pctx->Fpul;
                    pCurThread->ctx.Fpscr = pctx->Fpscr;
                    pCurThread->ctx.Psr = pctx->Psr;

                    pctx->Fir+=2; // +2: return control to instruction successor
                    pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
                    goto continueExecution;
                }
            }
            //
            // Update user context fpscr and fpul
            //
            pCurThread->ctx.Fpscr = pctx->Fpscr;
            pCurThread->ctx.Fpul = pctx->Fpul;
            pCurThread->ctx.Psr = pctx->Psr;
            break;
        }
#endif
        case 11:		// Breakpoint
            pctx->Fir -= 2;		// backup to the trapa instruction
            er.ExceptionInformation[0] = info;
            er.ExceptionCode = STATUS_BREAKPOINT;
            break;

        case 12:	// Reserved instruction
        case 13:	// Illegal slot instruction
#ifdef SH3
            //
            // Assume DSP instruction.
            // If DSP processor and DSP not enabled, enable the DSP.
            //

            if ( SH3DSP && ((pctx->Psr & 0x1000) == 0) ){
                KCall((PKFN)SwitchDSPOwner,pctx);
                pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
                goto continueExecution;
            }
#endif
            er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
            break;
        case 15:	// Hardware breakpoint
            er.ExceptionInformation[0] = info;
            er.ExceptionCode = STATUS_USER_BREAK;
            break;

        case 0xFF:  // Stack overflow
            er.ExceptionCode = STATUS_STACK_OVERFLOW;
            er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            break;
        }
    }

    if (exc != 11 && exc != 15) {
        NKDbgPrintfW(L"Exception %03x Thread=%8.8lx Proc=%8.8lx '%s'\r\n",
                     exc<<5, pth, hCurProc, 
                     pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
        NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx RA=%8.8lx TEA=%8.8lx\r\n",
                     pCurThread->aky, pctx->Fir, pctx->PR, info);
        if (UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) {
            NKDbgPrintfW(L"TLSKERN_NOFAULT set... bypassing kernel debugger.\r\n");
        }
    }

    // Invoke the kernel debugger to attempt to debug the exception before
    // letting the program resolve the condition via SEH.
    pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
    if (!UserDbgTrap(&er,pctx,FALSE) && ((UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) || !KDTrap(&er, pctx, FALSE))) {
        bHandled = NKDispatchException(pth, &er, pctx);
        if (!bHandled) {
            if (!UserDbgTrap(&er, pctx, TRUE) && !KDTrap(&er, pctx, TRUE)) {
                if (er.ExceptionCode == STATUS_BREAKPOINT) {
                    RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx Ignored.\r\n"), pctx->Fir));
                    DumpFrame(pth, pctx, exc<<5, info);
                    pctx->Fir += 2;		// skip over the trapa instruction
                } else {
                    // Terminate the process.
                    RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"),
                                  er.ExceptionCode));
                    DumpFrame(pth, pctx, exc<<5, info);
                    if (InSysCall()) {
                        OutputDebugStringW(L"Halting system\r\n");
                        for (;;)
                            ;
                    } else {
                        if (!GET_DEAD(pth)) {
                            SET_DEAD(pth);
                            pctx->Fir = (ULONG)pExitThread;
                            pctx->R0 = 0;
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
    if (exc == 2 || exc == 3)
        GuardCommit(info);
 continueExecution:

    // If returning from handling a stack overflow, reset the thread's stack overflow
    // flag. It would be good to free the tail of the stack at this time
    // so that the thread will stack fault again if the stack gets too big. But we
    // are currently using that stack page.
    if (exc == 0xFF)
    	CLEAR_STACKFAULT(pth);
    if (GET_DYING(pth) && !GET_DEAD(pth) && (pCurProc == pth->pOwnerProc)) {
        SET_DEAD(pth);
        CLEAR_USERBLOCK(pth);
        CLEAR_DEBUGWAIT(pth);
        pctx->Fir = (ULONG)pExitThread;
        pctx->R0 = 0;
    }   
    return;
}

void DumpFrame(PTHREAD pth, PCONTEXT pctx, DWORD dwExc, DWORD info) {
    DWORD oldCCR;
    oldCCR = CCR;
    CCR = 0;		// disable the cache to avoid changing its state
    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx AKY=%8.8lx PC=%8.8lx\r\n",
                 dwExc, pth, pCurThread->aky, pctx->Fir);
    NKDbgPrintfW(L" R0=%8.8lx  R1=%8.8lx  R2=%8.8lx  R3=%8.8lx\r\n",
                 pctx->R0, pctx->R1, pctx->R2, pctx->R3);
    NKDbgPrintfW(L" R4=%8.8lx  R5=%8.8lx  R6=%8.8lx  R7=%8.8lx\r\n",
                 pctx->R4, pctx->R5, pctx->R6, pctx->R7);
    NKDbgPrintfW(L" R8=%8.8lx  R9=%8.8lx R10=%8.8lx R11=%8.8lx\r\n",
                 pctx->R8, pctx->R9, pctx->R10, pctx->R11);
    NKDbgPrintfW(L"R12=%8.8lx R13=%8.8lx R14=%8.8lx R15=%8.8lx\r\n",
                 pctx->R12, pctx->R13, pctx->R14, pctx->R15);
    NKDbgPrintfW(L" PR=%8.8lx  SR=%8.8lx\r\n TEA/TRPA=%8.8x",
                 pctx->PR, pctx->Psr, info);
    NKDbgPrintfW(L"PTEL=%8.8lx PTEH=%8.8lx MMUCR=%8.8lx TTB=%8.8lx\r\n",
                 MMUPTEL, MMUPTEH, MMUCR, MMUTTB);
    NKDbgPrintfW(L"CCR=%8.8lx\r\n", oldCCR);
    CCR = oldCCR;	// restore original cache state
}

// normal thread stack: from top, TLS then args then free

void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, DWORD dwVMBase, BOOL kmode, ulong param) {
    if (!((ulong)lpStack>>VA_SECTION))
        lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
    pTh->dwStackBase = (DWORD)lpStack;
    // Clear all registers: Esp. fpu state for SH-4
    memset(&pTh->ctx, 0, sizeof(pTh->ctx));
    // Leave room for arguments and TLS on the stack
    pTh->ctx.R15 = (ulong)lpStack + cbStack - (TLS_MINIMUM_AVAILABLE*4) - 4*4;
    pTh->dwStackBound = pTh->ctx.R15 & ~(PAGE_SIZE-1);
    pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
    pTh->ctx.R4 = (ulong)lpStart;
    pTh->ctx.R5 = param;
    pTh->ctx.PR = 0;
#ifdef SH4
    pTh->ctx.Psr = 0x8000; // disable floating point
    pTh->ctx.Fpscr = 0x40000;  // handle no exceptions
#else
    pTh->ctx.Psr = 0;
#endif
    pTh->ctx.Fir = (ULONG)lpBase;
    pTh->ctx.ContextFlags = CONTEXT_FULL;
    SetThreadMode(pTh, ((kmode || bAllKMode) ? KERNEL_MODE : USER_MODE));
}

// main thread stack: from top, TLS then buf then buf2 then buf2 (ascii) then args then free

LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
                            LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2) {
    LPCWSTR pcmdline;
    if (!((ulong)lpStack>>VA_SECTION))
        lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
    pTh->dwStackBase = (DWORD)lpStack;
    pcmdline = (LPCWSTR)((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+3)&~3));
    memcpy((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+3)&~3),buf,buflen);
    pTh->pOwnerProc->lpszProcName = (LPWSTR)((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+3)&~3)-((buflen2+3)&~3));
    memcpy(pTh->pOwnerProc->lpszProcName,buf2,buflen2);
    KPlpvTls = pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
    return pcmdline;
}

void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, BOOL kmode,
                         ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4) {

    // Clear all registers: Esp. fpu state for SH-4
    memset(&pTh->ctx, 0, sizeof(pTh->ctx));
    // Leave room for arguments on the stack
    pTh->ctx.R15 = (pTh->dwStackBase + cbStack - (TLS_MINIMUM_AVAILABLE*4) - 8*4 - ((buflen+3)&~3) - ((buflen2+3)&~3)) & ~7;
    pTh->dwStackBound = pTh->ctx.R15 & ~(PAGE_SIZE-1);
    pTh->ctx.R4 = (ulong)lpStart;
    pTh->ctx.R5 = p1;
    pTh->ctx.R6 = p2;
    pTh->ctx.R7 = pTh->dwStackBase+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+3)&~3);
    ((LPDWORD)pTh->ctx.R15)[4] = p4;
    pTh->ctx.PR = 0;
#ifdef SH4
    pTh->ctx.Psr = 0x8000; // disable floating point
    pTh->ctx.Fpscr = 0x40000;  // handle no exceptions
#else
    pTh->ctx.Psr = 0;
#endif
    pTh->ctx.Fir = (ULONG)lpBase;
    SetThreadMode(pTh, ((kmode || bAllKMode) ? KERNEL_MODE : USER_MODE));
    pTh->ctx.ContextFlags = CONTEXT_FULL;
}

#ifdef XTIME
extern DWORD ExceptionTime;// tick at exception entry

void SyscallTime(int iSyscall) {
    DWORD	dwTime;
    dwTime = ExceptionTime - TMUADDR->tcnt1;
    xt.dwTime[iSyscall]+= dwTime;
    if ((++xt.dwCount[iSyscall]) == 1)
        xt.dwMax[iSyscall]=xt.dwMin[iSyscall]= dwTime;
    if (xt.dwMax[iSyscall] < dwTime)
        xt.dwMax[iSyscall]= dwTime;
    if (xt.dwMin[iSyscall] > dwTime)
        xt.dwMin[iSyscall]= dwTime;
}
#endif

// For the SH3 the compiler generated exception functions expect the following input arguments:
//	Exception Filter: r4 = frame, r5 = exception pointers
//	Termination Handler: r4 = frame, r5 = abnormal term flag

#define __C_ExecuteExceptionFilter(eptr, fn, efrm) ((fn)((efrm), (eptr)))
#define __C_ExecuteTerminationHandler(bAb, fn, efrm) ((fn)((efrm), (bAb)))

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
EXCEPTION_DISPOSITION __C_specific_handler(PEXCEPTION_RECORD ExceptionRecord,
                                           PVOID EstablisherFrame, PCONTEXT ContextRecord, PDISPATCHER_CONTEXT DispatcherContext) {
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
                Value = __C_ExecuteExceptionFilter(&ExceptionPointers,ExceptionFilter,(ULONG)EstablisherFrame);
                // If the return value is less than zero, then dismiss the
                // exception. Otherwise, if the value is greater than zero,
                // then unwind to the target exception handler. Otherwise,
                // continue the search for an exception filter.
                if (Value < 0)
                    return ExceptionContinueExecution;
                else if (Value > 0) {
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
                    DispatcherContext->ControlPc = ScopeTable->ScopeRecord[Index].EndAddress + 4;
                    TerminationHandler = (TERMINATION_HANDLER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                    __C_ExecuteTerminationHandler(TRUE,TerminationHandler,(ULONG)EstablisherFrame);
                }
            }
        }
    }
    // Continue search for exception or termination handlers.
    return ExceptionContinueSearch;
}

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
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            lpContext->PR = pth->pThrdDbg->psavedctx->PR;
            lpContext->R15 = pth->pThrdDbg->psavedctx->R15;
            lpContext->Fir = pth->pThrdDbg->psavedctx->Fir;
            lpContext->Psr = pth->pThrdDbg->psavedctx->Psr;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            lpContext->MACH = pth->pThrdDbg->psavedctx->MACH;
            lpContext->MACL = pth->pThrdDbg->psavedctx->MACL;
            lpContext->GBR = pth->pThrdDbg->psavedctx->GBR;
            lpContext->R0 = pth->pThrdDbg->psavedctx->R0;
            lpContext->R1 = pth->pThrdDbg->psavedctx->R1;
            lpContext->R2 = pth->pThrdDbg->psavedctx->R2;
            lpContext->R3 = pth->pThrdDbg->psavedctx->R3;
            lpContext->R4 = pth->pThrdDbg->psavedctx->R4;
            lpContext->R5 = pth->pThrdDbg->psavedctx->R5;
            lpContext->R6 = pth->pThrdDbg->psavedctx->R6;
            lpContext->R7 = pth->pThrdDbg->psavedctx->R7;
            lpContext->R8 = pth->pThrdDbg->psavedctx->R8;
            lpContext->R9 = pth->pThrdDbg->psavedctx->R9;
            lpContext->R10 = pth->pThrdDbg->psavedctx->R10;
            lpContext->R11 = pth->pThrdDbg->psavedctx->R11;
            lpContext->R12 = pth->pThrdDbg->psavedctx->R12;
            lpContext->R13 = pth->pThrdDbg->psavedctx->R13;
            lpContext->R14 = pth->pThrdDbg->psavedctx->R14;
        }
#ifdef SH4
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
            FPUFlushContext();
            lpContext->Fpscr = pth->pThrdDbg->psavedctx->Fpscr;
            lpContext->Fpul = pth->pThrdDbg->psavedctx->Fpul;
            memcpy(lpContext->FRegs,pth->pThrdDbg->psavedctx->FRegs,sizeof(lpContext->FRegs));
            memcpy(lpContext->xFRegs,pth->pThrdDbg->psavedctx->xFRegs,sizeof(lpContext->xFRegs));
        }
#endif
        if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

        }
        SETCURKEY(ulOldKey);
    } else {
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            lpContext->PR = pth->ctx.PR;
            lpContext->R15 = pth->ctx.R15;
            lpContext->Fir = pth->ctx.Fir;
            lpContext->Psr = pth->ctx.Psr;
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            lpContext->MACH = pth->ctx.MACH;
            lpContext->MACL = pth->ctx.MACL;
            lpContext->GBR = pth->ctx.GBR;
            lpContext->R0 = pth->ctx.R0;
            lpContext->R1 = pth->ctx.R1;
            lpContext->R2 = pth->ctx.R2;
            lpContext->R3 = pth->ctx.R3;
            lpContext->R4 = pth->ctx.R4;
            lpContext->R5 = pth->ctx.R5;
            lpContext->R6 = pth->ctx.R6;
            lpContext->R7 = pth->ctx.R7;
            lpContext->R8 = pth->ctx.R8;
            lpContext->R9 = pth->ctx.R9;
            lpContext->R10 = pth->ctx.R10;
            lpContext->R11 = pth->ctx.R11;
            lpContext->R12 = pth->ctx.R12;
            lpContext->R13 = pth->ctx.R13;
            lpContext->R14 = pth->ctx.R14;
        }
#ifdef SH4
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
            FPUFlushContext();
            lpContext->Fpscr = pth->ctx.Fpscr;
            lpContext->Fpul = pth->ctx.Fpul;
            memcpy(lpContext->FRegs,pth->ctx.FRegs,sizeof(lpContext->FRegs));
            memcpy(lpContext->xFRegs,pth->ctx.xFRegs,sizeof(lpContext->xFRegs));
        }
#endif
        if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

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
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            pth->pThrdDbg->psavedctx->PR = lpContext->PR;
            pth->pThrdDbg->psavedctx->R15 = lpContext->R15;
            pth->pThrdDbg->psavedctx->Fir = lpContext->Fir;
            pth->pThrdDbg->psavedctx->Psr = (pth->ctx.Psr & 0xfffffcfc) | (lpContext->Psr & 0x00000303);
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            pth->pThrdDbg->psavedctx->MACH = lpContext->MACH;
            pth->pThrdDbg->psavedctx->MACL = lpContext->MACL;
            pth->pThrdDbg->psavedctx->GBR = lpContext->GBR;
            pth->pThrdDbg->psavedctx->R0 = lpContext->R0;
            pth->pThrdDbg->psavedctx->R1 = lpContext->R1;
            pth->pThrdDbg->psavedctx->R2 = lpContext->R2;
            pth->pThrdDbg->psavedctx->R3 = lpContext->R3;
            pth->pThrdDbg->psavedctx->R4 = lpContext->R4;
            pth->pThrdDbg->psavedctx->R5 = lpContext->R5;
            pth->pThrdDbg->psavedctx->R6 = lpContext->R6;
            pth->pThrdDbg->psavedctx->R7 = lpContext->R7;
            pth->pThrdDbg->psavedctx->R8 = lpContext->R8;
            pth->pThrdDbg->psavedctx->R9 = lpContext->R9;
            pth->pThrdDbg->psavedctx->R10 = lpContext->R10;
            pth->pThrdDbg->psavedctx->R11 = lpContext->R11;
            pth->pThrdDbg->psavedctx->R12 = lpContext->R12;
            pth->pThrdDbg->psavedctx->R13 = lpContext->R13;
            pth->pThrdDbg->psavedctx->R14 = lpContext->R14;
        }
#ifdef SH4
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
            FPUFlushContext();
            pth->pThrdDbg->psavedctx->Fpscr = lpContext->Fpscr;
            pth->pThrdDbg->psavedctx->Fpul = lpContext->Fpul;
            memcpy(pth->pThrdDbg->psavedctx->FRegs,lpContext->FRegs,sizeof(lpContext->FRegs));
            memcpy(pth->pThrdDbg->psavedctx->xFRegs,lpContext->xFRegs,sizeof(lpContext->xFRegs));
        }
#endif
        if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

        }
        SETCURKEY(ulOldKey);
    } else {
        if (lpContext->ContextFlags & CONTEXT_CONTROL) {
            pth->ctx.PR = lpContext->PR;
            pth->ctx.R15 = lpContext->R15;
            pth->ctx.Fir = lpContext->Fir;
            pth->ctx.Psr = (pth->ctx.Psr & 0xfffffcfc) | (lpContext->Psr & 0x00000303);
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER) {
            pth->ctx.MACH = lpContext->MACH;
            pth->ctx.MACL = lpContext->MACL;
            pth->ctx.GBR = lpContext->GBR;
            pth->ctx.R0 = lpContext->R0;
            pth->ctx.R1 = lpContext->R1;
            pth->ctx.R2 = lpContext->R2;
            pth->ctx.R3 = lpContext->R3;
            pth->ctx.R4 = lpContext->R4;
            pth->ctx.R5 = lpContext->R5;
            pth->ctx.R6 = lpContext->R6;
            pth->ctx.R7 = lpContext->R7;
            pth->ctx.R8 = lpContext->R8;
            pth->ctx.R9 = lpContext->R9;
            pth->ctx.R10 = lpContext->R10;
            pth->ctx.R11 = lpContext->R11;
            pth->ctx.R12 = lpContext->R12;
            pth->ctx.R13 = lpContext->R13;
            pth->ctx.R14 = lpContext->R14;
        }
#ifdef SH4
        if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
            FPUFlushContext();
            pth->ctx.Fpscr = lpContext->Fpscr;
            pth->ctx.Fpul = lpContext->Fpul;
            memcpy(pth->ctx.FRegs,lpContext->FRegs,sizeof(lpContext->FRegs));
            memcpy(pth->ctx.xFRegs,lpContext->xFRegs,sizeof(lpContext->xFRegs));
        }
#endif
        if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

        }
    }
    return TRUE;
}

