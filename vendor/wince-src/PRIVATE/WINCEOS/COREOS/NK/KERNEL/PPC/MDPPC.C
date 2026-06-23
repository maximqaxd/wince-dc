/* Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved. */

/*+
    mdppc.c - PowerPC machine dependent code & data
 */

#include "kernel.h"
#include "kxppc.h"

#if defined(PPC403)

extern void SetPID(int);

#elif defined(PPC821)

#include "mmu821.h"

#define MAX_ASID    15      // Maximum Address Space ID supported by the 821

uchar ASIDMap[MAX_PROCESSES] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
}; // map from process number to address space ID

int NextASID = 0;           // next available address space ID
extern void SetCASID(int);

#endif

// These are filled in by KernelStart
DWORD CEProcessorType   = 0;     // 821, 823, 401, or 403
WORD  ProcessorLevel    = 0;     // PVR[0:15]
WORD  ProcessorRevision = 0;     // PVR[16:31]

const wchar_t NKSignon[] = TEXT("Windows CE Kernel for PowerPC Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n");

void NextThread(void);
void KCNextThread(void);
ulong OEMInterruptHandler(void);
ulong OEMDecrementer(void);
void OEMIdle(void);
void DoPowerOff(void);

#define KERNEL_MSR  (KData.kMSR)            // Default kernel MSR
#define USER_MSR    (KERNEL_MSR | MSR_PR)   // Problem state=1

#define SRR1_TRAP   0x20000     // SRR1 bit indicating program trap
#define TO_BKP      0x16        // trap BREAKPOINT
#define TO_DIV0     0x06        // trap Integer DIV by zero
#define TO_DIV0U    0x07        // trap unconditional DIV by 0

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

void DumpFrame(PTHREAD pth, PCPUCONTEXT pctx, int id, ulong excInfo, int level) {
    UINT32 addr;
    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx AKY=%8.8lx PC=%8.8lx excInfo=%8.8lx\r\n",
		 id, pth, pCurThread->aky, pctx->Iar, excInfo);
    NKDbgPrintfW(L"SR=%8.8lx CR=%8.8lx XER=%8.8lx LR=%8.8lx CTR=%8.8lx\r\n",
		 pctx->Msr, pctx->Cr, pctx->Xer, pctx->Lr, pctx->Ctr);
    NKDbgPrintfW(L" G0=%8.8lx  G1=%8.8lx  G2=%8.8lx  G3=%8.8lx\r\n",
		 pctx->Gpr0, pctx->Gpr1, pctx->Gpr2, pctx->Gpr3);
    NKDbgPrintfW(L" G4=%8.8lx  G5=%8.8lx  G6=%8.8lx  G7=%8.8lx\r\n",
		 pctx->Gpr4, pctx->Gpr5, pctx->Gpr6, pctx->Gpr7);
    NKDbgPrintfW(L" G8=%8.8lx  G9=%8.8lx G10=%8.8lx G11=%8.8lx\r\n",
		 pctx->Gpr8, pctx->Gpr9, pctx->Gpr10, pctx->Gpr11);
    NKDbgPrintfW(L"G12=%8.8lx G13=%8.8lx G14=%8.8lx G15=%8.8lx\r\n",
		 pctx->Gpr12, pctx->Gpr13, pctx->Gpr14, pctx->Gpr15);
    NKDbgPrintfW(L"G16=%8.8lx G17=%8.8lx G18=%8.8lx G19=%8.8lx\r\n",
		 pctx->Gpr16, pctx->Gpr17, pctx->Gpr18, pctx->Gpr19);
    NKDbgPrintfW(L"G20=%8.8lx G21=%8.8lx G22=%8.8lx G23=%8.8lx\r\n",
		 pctx->Gpr20, pctx->Gpr21, pctx->Gpr22, pctx->Gpr23);
    NKDbgPrintfW(L"G24=%8.8lx G25=%8.8lx G26=%8.8lx G27=%8.8lx\r\n",
		 pctx->Gpr24, pctx->Gpr25, pctx->Gpr26, pctx->Gpr27);
    NKDbgPrintfW(L"G28=%8.8lx G29=%8.8lx G30=%8.8lx G31=%8.8lx\r\n",
		 pctx->Gpr28, pctx->Gpr29, pctx->Gpr30, pctx->Gpr31);
    if (level > 1) {
		addr = (pctx->Iar & -4) - 8*4;
		if (VerifyAccess((PVOID)addr, VERIFY_KERNEL_OK, CurAKey))
		    DumpDwords((PDWORD)addr, 12);
    }
}

typedef struct ExcInfo {
    DWORD   linkage;
	ULONG   oldIar;
	UINT    oldMode;
	char    id;
	char    StoreOp;
	WORD 	lowSp;
	ULONG   excInfo;
} EXCINFO;
typedef EXCINFO *PEXCINFO;

ERRFALSE(sizeof(EXCINFO) <= sizeof(CALLSTACK));
ERRFALSE(offsetof(EXCINFO,linkage) == offsetof(CALLSTACK,pcstkNext));
ERRFALSE(offsetof(EXCINFO,oldIar) == offsetof(CALLSTACK,retAddr));
ERRFALSE(offsetof(EXCINFO,oldMode) == offsetof(CALLSTACK,pprcLast));
ERRFALSE(1024 >= sizeof(CALLSTACK) + STK_SLACK_SPACE + sizeof(CONTEXT) + 0x28); // 0x28 == StackFrameHeaderLength



BOOL HandleException(PTHREAD pth, int id, ulong addr, uint StoreOp) {
	PEXCINFO pexi;
    DWORD stackaddr;

#if 0
    NKDbgPrintfW(L"Exception %02x00 Thread=%8.8lx Proc=%8.8lx\r\n",id, pth, hCurProc);
    NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx Lr=%8.8lx DAR=%8.8lx\r\n",pCurThread->aky, pth->ctx.Iar, pth->ctx.Lr, addr);
#endif

	KCALLPROFON(0);
	pexi = (struct ExcInfo *)((pth->ctx.Gpr1 & ~1023) - ALIGN8(STK_SLACK_SPACE + sizeof(CALLSTACK)));
    if (!((DWORD)pexi & 0x80000000) && DemandCommit((DWORD)pexi)) {
		stackaddr = (DWORD)pexi & ~(PAGE_SIZE-1);
		if ((stackaddr >= pth->dwStackBound) || (stackaddr < pth->dwStackBase) ||
			((pth->dwStackBound = stackaddr) >= (pth->dwStackBase + MIN_STACK_RESERVE)) ||
			TEST_STACKFAULT(pth)) {
			KCALLPROFOFF(0);
			return TRUE; // restart instruction
		}
		SET_STACKFAULT(pth);
       	id = ID_STACK_FAULT;   // stack fault exception code
		addr = (DWORD)pexi;
	}
	if (pth->ctx.Iar != (ulong)CaptureContext) {
		pexi->id = id;
		pexi->lowSp = (WORD)(pth->ctx.Gpr1 & 1023);
		pexi->oldIar = pth->ctx.Iar;
		pexi->oldMode = GetThreadMode(pth);
		pexi->excInfo = (id == ID_PROGRAM) ? pth->ctx.Msr : addr;
		pexi->StoreOp = StoreOp;
	    pexi->linkage = (DWORD)pCurThread->pcstkTop | 1;
		pCurThread->pcstkTop = (PCALLSTACK)pexi;
		pth->ctx.Gpr1 = (DWORD)pexi;
		
		SetThreadMode(pth, KERNEL_MODE);
		pth->ctx.Iar = (ULONG)CaptureContext;
		KCALLPROFOFF(0);
		return TRUE;                     // continue execution
	}
	DumpFrame(pth, &pth->ctx, id, addr, 10);
	RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pth));
	SurrenderCritSecs();
	SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
	RunList.pth = 0;
	SetReschedule();
	KCALLPROFOFF(0);
	return FALSE;
}



// SetCPUASID - setup processor specific address space information
//
//  SetCPUASID will update the virtual memory tables so that new current process
// is mapped into slot 0 and set the h/w address space ID register for the new
// process.
//
//  Entry   (pth) = ptr to THREAD structure
//  Return  nothing

void SetCPUASID(PTHREAD pth) {
    PPROCESS pprc;
    int ix;
    CurAKey = pth->aky;
    pCurProc = pprc = pth->pProc;
#ifdef CELOG
    if (hCurProc != pprc->hProc) {
        CELOG_ThreadMigrate(pprc->hProc, 0);
    }
#endif
    hCurProc = pprc->hProc;
    SectionTable[0] = SectionTable[pprc->dwVMBase>>VA_SECTION];
    ix = pprc->procnum;
#if defined(PPC821)     
	INTERRUPTS_OFF();
	if (ASIDMap[ix] == 0xFF) {
	// Assign an address space ID to this process
		if (NextASID > MAX_ASID) {
			memset(ASIDMap, 0xff, sizeof(ASIDMap));
			NextASID = 0;
			FlushTLB();
		}
	ASIDMap[ix] = NextASID++;
    }
    SetCASID(ASIDMap[ix]);
	INTERRUPTS_ON();
#elif defined(PPC403)
	SetPID(ix+1);
#else
	#error "Unsupported CPU type"
#endif
}

typedef struct _EXCARGS {
	DWORD dwExceptionCode;          /* exception code       */
	DWORD dwExceptionFlags;         /* continuable exception flag   */
	DWORD cArguments;                       /* number of arguments in array */
	DWORD *lpArguments;                     /* address of array of arguments        */
} EXCARGS;

void ExceptionDispatch(PCONTEXT pctx) {
    EXCEPTION_RECORD er;
    int id;
    EXCARGS *pea;
    BOOL bHandled;
    PTHREAD pth; 
    PEXCINFO pexi;
    pth = pCurThread;
    pexi = (PEXCINFO)pth->pcstkTop;
    DEBUGMSG(ZONE_SEH, (TEXT("ExceptionDispatch: pexi=%8.8lx Iar=%8.8lx Id=0x%x\r\n"), pexi, pexi->oldIar, pexi->id));

	pctx->Iar = pexi->oldIar;
    pctx->Msr = pexi->oldMode==KERNEL_MODE ? KERNEL_MSR : USER_MSR;
    pctx->Gpr1 = (ULONG)pctx + sizeof(CONTEXT);

	memset(&er, 0, sizeof(er));
    er.ExceptionAddress = (PVOID)pctx->Iar;
    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    // HandleExcepion sets the LSB of the callstack linkage but ObjectCall
    // does not.
    if (!(pexi->linkage & 1)) {
		


		pea = (EXCARGS *)(pctx->Gpr1 + offsetof(STACK_FRAME_HEADER, Parameter0));
		DEBUGMSG(ZONE_SEH, (TEXT("Raising exception %x flags=%x args=%d pexi=%8.8lx\r\n"),
			    pea->dwExceptionCode, pea->dwExceptionFlags, pea->cArguments, pexi));
	    er.ExceptionCode = pea->dwExceptionCode;
		er.ExceptionFlags = pea->dwExceptionFlags;
		if (pea->lpArguments && pea->cArguments) {
		    if (pea->cArguments > EXCEPTION_MAXIMUM_PARAMETERS) {
			    er.ExceptionCode = STATUS_INVALID_PARAMETER;
				er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
		    } else {
				memcpy(er.ExceptionInformation, pea->lpArguments,pea->cArguments*sizeof(DWORD));
				er.NumberParameters = pea->cArguments;
			}
		}
	} else {
		// CPU detected exception. Extract some additional information about
		// the cause of the exception from the EXCINFO (CALLSTACK) structure.
		id = pexi->id;
		pctx->Gpr1 += pexi->lowSp + ALIGN8(sizeof(CALLSTACK) + STK_SLACK_SPACE);

		if (((id == ID_CPAGE_FAULT) || (id == ID_DPAGE_FAULT)) && AutoCommit(pexi->excInfo)) {
			pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
			goto continueExecution;
		}
		switch (id) {
			case ID_ALIGNMENT:
				er.ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
			    break;
			case ID_PROGRAM:
				//
			    // Extract the exception cause from the SRR1 value.
			    // In the case of program exceptions the general handler stores
			    // the SRR1 value in the pexi->excInfo field.
			    //
			    if (pexi->excInfo & SRR1_TRAP) {                // program trap ??
					er.ExceptionInformation[0] = *(ULONG *)pctx->Iar & 0xffff;
					if ( er.ExceptionInformation[0] == TO_BKP )
					    er.ExceptionCode = STATUS_BREAKPOINT;
					else // if ( er.ExceptionInformation[0] == TO_DIV0 )
					    er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
			    }
			    break;
			case ID_DPAGE_FAULT:            // Data page fault
			    er.ExceptionInformation[0] = pexi->StoreOp;         // == 1 if Store
			    // fall through:
			case ID_CPAGE_FAULT:            // Code page fault
				// The general fault handler records the faulting address in
			    // pexi->excInfo for both data and code page faults
			    if (ProcessPageFault(er.ExceptionInformation[0], pexi->excInfo)) {
					pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
					goto continueExecution;
			    }
			    er.ExceptionCode = STATUS_ACCESS_VIOLATION;
			    er.ExceptionInformation[1] = pexi->excInfo;
			    er.NumberParameters = 2;
			    break;
			case ID_STACK_FAULT:    // Stack overflow
			    er.ExceptionCode = STATUS_STACK_OVERFLOW;
			    er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
			    break;
		}
    }
	if (er.ExceptionCode != STATUS_BREAKPOINT) {
		NKDbgPrintfW(L"Exception %02x00 Thread=%8.8lx Proc=%8.8lx '%s'\r\n",
		     id, pth, hCurProc, pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
		NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx RA=%8.8lx excInfo=%8.8lx\r\n",
		     pCurThread->aky, pctx->Iar, pctx->Lr, pexi->excInfo);
		if (UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) {
		    NKDbgPrintfW(L"TLSKERN_NOFAULT set... bypassing kernel debugger.\r\n");
		}
    }
    // Invoke the kernel debugger to attempt to debug the exception before
    // letting the program resolve the condition via SEH.
    pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
	if (!UserDbgTrap(&er,pctx,FALSE) && ((UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) || !KDTrap(&er, pctx, FALSE))) {
		if (er.ExceptionCode == STATUS_BREAKPOINT) {
		    RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx Ignored.\r\n"), pctx->Iar));
	    	pctx->Iar += 4;             // skip over the BREAK instruction
		} else {
		    // Turn the EXCINFO into a CALLSTACK and link it into the
	    	// thread's call list.
		    ((PCALLSTACK)pexi)->akyLast = 0;
		    bHandled = NKDispatchException(pth, &er, pctx);
		    if (!bHandled) {
				if (!UserDbgTrap(&er, pctx, TRUE) && !KDTrap(&er, pctx, TRUE)) {
				    // Terminate the process.
				    RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"),er.ExceptionCode));
				    DumpFrame(pth, (PCPUCONTEXT)&pctx->Gpr0,id,pexi->excInfo,0);
				    if (InSysCall()) {
						OutputDebugStringW(L"Halting system\r\n");
						for (;;)
				    		;
				    } else {
						if (!GET_DEAD(pth)) {
					    	SET_DEAD(pth);
						    pctx->Iar = (ULONG)pExitThread;
						    pctx->Gpr3 = 0;
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
    if ((id == ID_CPAGE_FAULT) || (id == ID_DPAGE_FAULT))
    	GuardCommit(pexi->excInfo);
continueExecution:
    // If returning from handling a stack overflow, reset the thread's stack
    // overflow flag. It would be good to free the tail of the stack at this
    // time so that the thread will stack fault again if the stack gets too
    // big. But we are currently using that stack page.
    if (id == ID_STACK_FAULT)
		CLEAR_STACKFAULT(pth);
    if (GET_DYING(pth) && !GET_DEAD(pth) && (pCurProc == pth->pOwnerProc)) {
		SET_DEAD(pth);
		CLEAR_USERBLOCK(pth);
		CLEAR_DEBUGWAIT(pth);
		pctx->Iar = (ULONG)pExitThread;
		pctx->Gpr3 = 0;
	}   
}

#if PPC821
#define WriteAccess(entry)  ((entry & PG_PROTECTION) == PG_PROT_WRITE)
#else
#define WriteAccess(entry)  ((entry & PG_PROT_WRITE) == PG_PROT_WRITE)
#endif

LPVOID VerifyAccess(LPVOID pvAddr, DWORD dwFlags, ACCESSKEY aky) {
    PSECTION pscn;
    MEMBLOCK *pmb;
    ulong entry;
    if ((long)pvAddr >= 0) {
        if ((pscn = SectionTable[(ulong)pvAddr>>VA_SECTION]) &&
            (pmb = (*pscn)[((ulong)pvAddr>>VA_BLOCK)&BLOCK_MASK]) &&
            (pmb != RESERVED_BLOCK) && (pmb->alk & aky) &&
            ((entry = pmb->aPages[((ulong)pvAddr>>VA_PAGE)&PAGE_MASK]) & PG_VALID_MASK) &&
            (!(dwFlags & VERIFY_WRITE_FLAG) || WriteAccess(entry)))
            return Phys2Virt(PFNfromEntry(entry) | ((ulong)pvAddr & (PAGE_SIZE-1)));
        //  Access to the Kernel Page. (4K data area at address KPAGE_BASE)
        if (((ulong)pvAddr >> 12) == (KPAGE_BASE >> 12))
            return(pvAddr);
    } else {
        // Kernel mode only address. If the "kernel mode OK" flag is set or if the
        // thread is running in kernel mode, allow the access.
        if (((dwFlags & VERIFY_KERNEL_OK) || (GetThreadMode(pCurThread) == KERNEL_MODE)) && ((ulong)pvAddr < 0xC0000000))
	        return pvAddr;
    }
    return 0;
}

/* Machine dependent thread creation */

#define STKALIGN 8
#define STKMSK (STKALIGN-1)

// normal thread stack: from top, TLS then args then free


void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase,
	LPVOID lpStart, DWORD dwVMBase, BOOL kmode, ulong param) {
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)lpStack + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
	// Leave room for arguments and TLS on the stack
	pTh->ctx.Gpr1 = (ulong)lpStack + cbStack - (TLS_MINIMUM_AVAILABLE*4)
		- sizeof(STACK_FRAME_HEADER);
	pTh->dwStackBound = pTh->ctx.Gpr1 & ~(PAGE_SIZE-1);
	pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
	pTh->ctx.Gpr3 = (ulong)lpStart;
	pTh->ctx.Gpr4 = param;
	pTh->ctx.Lr = 4;
    pTh->ctx.Iar = (ULONG)lpBase;
	pTh->ctx.Msr = (kmode || bAllKMode) ? KERNEL_MSR : USER_MSR;
    DEBUGMSG(ZONE_SCHEDULE, (L"MDCT: pTh=%8.8lx Iar=%8.8lx Msr=%4.4x GP=%8.8lx Sp=%8.8lx\r\n", pTh, pTh->ctx.Iar, pTh->ctx.Msr, pTh->ctx.Gpr2, pTh->ctx.Gpr1));
}

// main thread stack: from top, TLS then buf then buf2 then buf2 (ascii) then args then free

LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
	LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2) {
	LPCWSTR pcmdline;
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)lpStack + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
	pcmdline = (LPCWSTR)((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK));
	memcpy((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK),
		buf,buflen);
	memcpy((LPBYTE)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK)-
		((buflen2+STKMSK)&~STKMSK),buf2,buflen2);
	KPlpvTls = pTh->tlsPtr = (LPDWORD)((ulong)lpStack+cbStack-(TLS_MINIMUM_AVAILABLE*4));
	pTh->pOwnerProc->lpszProcName = (LPWSTR)((ulong)lpStack + cbStack
	    - (TLS_MINIMUM_AVAILABLE*4) - ((buflen+STKMSK)&~STKMSK) -
		((buflen2+STKMSK)&~STKMSK));
	return pcmdline;
}

void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart,
	BOOL kmode, ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4) {
	// Leave room for arguments on the stack
	pTh->ctx.Gpr1 = pTh->dwStackBase + cbStack - (TLS_MINIMUM_AVAILABLE*4) -
		sizeof(STACK_FRAME_HEADER) - ((buflen+STKMSK)&~STKMSK) -
		((buflen2+STKMSK)&~STKMSK);
	pTh->dwStackBound = pTh->ctx.Gpr1 & ~(PAGE_SIZE-1);
	pTh->ctx.Gpr3 = (ulong)lpStart;
	pTh->ctx.Gpr4 = p1;
	pTh->ctx.Gpr5 = p2;
	pTh->ctx.Gpr6 = pTh->dwStackBase+cbStack-(TLS_MINIMUM_AVAILABLE*4)-((buflen+STKMSK)&~STKMSK);
	pTh->ctx.Gpr7 = p4;
	pTh->ctx.Lr = 4;
    pTh->ctx.Iar = (ULONG)lpBase;
	pTh->ctx.Msr = (kmode || bAllKMode) ? KERNEL_MSR : USER_MSR;
    DEBUGMSG(ZONE_SCHEDULE, (L"MDCMainT: pTh=%8.8lx Iar=%8.8lx Msr=%4.4x GP=%8.8lx Sp=%8.8lx\r\n", pTh, pTh->ctx.Iar, pTh->ctx.Msr, pTh->ctx.Gpr2, pTh->ctx.Gpr1));
}

LONG __C_ExecuteExceptionFilter (
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG EstablisherFrame);

VOID __C_ExecuteTerminationHandler (
    BOOLEAN AbnormalTermination,
    TERMINATION_HANDLER TerminationHandler,
    ULONG EstablisherFrame);
    
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
			(ScopeTable->ScopeRecord[Index].JumpTarget != 0)) {
		// Call the exception filter routine.
		ExceptionFilter =
		    (EXCEPTION_FILTER)ScopeTable->ScopeRecord[Index].HandlerAddress;
		Value = __C_ExecuteExceptionFilter(&ExceptionPointers,
						   ExceptionFilter,
						   (ULONG)EstablisherFrame);
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
	TargetPc = ContextRecord->Iar;
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
	    if (ScopeTable->ScopeRecord[Index].JumpTarget != 0) {
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

typedef PCONTEXT LPCONTEXT;

BOOL DoThreadGetContext(HANDLE hTh, LPCONTEXT lpContext) {
	PTHREAD pth;
	if (!(pth = HandleToThread(hTh))) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (lpContext->ContextFlags & ~(CONTEXT_FULL|CONTEXT_DEBUG_REGISTERS)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
		ULONG ulOldAky = CurAKey;
		SETCURKEY((DWORD)-1);
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			lpContext->Msr = pth->pThrdDbg->psavedctx->Msr;
			lpContext->Iar = pth->pThrdDbg->psavedctx->Iar;
			lpContext->Lr = pth->pThrdDbg->psavedctx->Lr;
			lpContext->Ctr = pth->pThrdDbg->psavedctx->Ctr;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
			lpContext->Gpr0 = pth->pThrdDbg->psavedctx->Gpr0;
			lpContext->Gpr1 = pth->pThrdDbg->psavedctx->Gpr1;
			lpContext->Gpr2 = pth->pThrdDbg->psavedctx->Gpr2;
			lpContext->Gpr3 = pth->pThrdDbg->psavedctx->Gpr3;
			lpContext->Gpr4 = pth->pThrdDbg->psavedctx->Gpr4;
			lpContext->Gpr5 = pth->pThrdDbg->psavedctx->Gpr5;
			lpContext->Gpr6 = pth->pThrdDbg->psavedctx->Gpr6;
			lpContext->Gpr7 = pth->pThrdDbg->psavedctx->Gpr7;
			lpContext->Gpr8 = pth->pThrdDbg->psavedctx->Gpr8;
			lpContext->Gpr9 = pth->pThrdDbg->psavedctx->Gpr9;
			lpContext->Gpr10 = pth->pThrdDbg->psavedctx->Gpr10;
			lpContext->Gpr11 = pth->pThrdDbg->psavedctx->Gpr11;
			lpContext->Gpr12 = pth->pThrdDbg->psavedctx->Gpr12;
			lpContext->Gpr13 = pth->pThrdDbg->psavedctx->Gpr13;
			lpContext->Gpr14 = pth->pThrdDbg->psavedctx->Gpr14;
			lpContext->Gpr15 = pth->pThrdDbg->psavedctx->Gpr15;
			lpContext->Gpr16 = pth->pThrdDbg->psavedctx->Gpr16;
			lpContext->Gpr17 = pth->pThrdDbg->psavedctx->Gpr17;
			lpContext->Gpr18 = pth->pThrdDbg->psavedctx->Gpr18;
			lpContext->Gpr19 = pth->pThrdDbg->psavedctx->Gpr19;
			lpContext->Gpr20 = pth->pThrdDbg->psavedctx->Gpr20;
			lpContext->Gpr21 = pth->pThrdDbg->psavedctx->Gpr21;
			lpContext->Gpr22 = pth->pThrdDbg->psavedctx->Gpr22;
			lpContext->Gpr23 = pth->pThrdDbg->psavedctx->Gpr23;
			lpContext->Gpr24 = pth->pThrdDbg->psavedctx->Gpr24;
			lpContext->Gpr25 = pth->pThrdDbg->psavedctx->Gpr25;
			lpContext->Gpr26 = pth->pThrdDbg->psavedctx->Gpr26;
			lpContext->Gpr27 = pth->pThrdDbg->psavedctx->Gpr27;
			lpContext->Gpr28 = pth->pThrdDbg->psavedctx->Gpr28;
			lpContext->Gpr29 = pth->pThrdDbg->psavedctx->Gpr29;
			lpContext->Gpr30 = pth->pThrdDbg->psavedctx->Gpr30;
			lpContext->Gpr31 = pth->pThrdDbg->psavedctx->Gpr31;
			lpContext->Cr = pth->pThrdDbg->psavedctx->Cr;
			lpContext->Xer = pth->pThrdDbg->psavedctx->Xer;
		}
		if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {

		}
		if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

		}
		SETCURKEY(ulOldAky);
	} else {
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			lpContext->Msr = pth->ctx.Msr;
			lpContext->Iar = pth->ctx.Iar;
			lpContext->Lr = pth->ctx.Lr;
			lpContext->Ctr = pth->ctx.Ctr;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
			lpContext->Gpr0 = pth->ctx.Gpr0;
			lpContext->Gpr1 = pth->ctx.Gpr1;
			lpContext->Gpr2 = pth->ctx.Gpr2;
			lpContext->Gpr3 = pth->ctx.Gpr3;
			lpContext->Gpr4 = pth->ctx.Gpr4;
			lpContext->Gpr5 = pth->ctx.Gpr5;
			lpContext->Gpr6 = pth->ctx.Gpr6;
			lpContext->Gpr7 = pth->ctx.Gpr7;
			lpContext->Gpr8 = pth->ctx.Gpr8;
			lpContext->Gpr9 = pth->ctx.Gpr9;
			lpContext->Gpr10 = pth->ctx.Gpr10;
			lpContext->Gpr11 = pth->ctx.Gpr11;
			lpContext->Gpr12 = pth->ctx.Gpr12;
			lpContext->Gpr13 = pth->ctx.Gpr13;
			lpContext->Gpr14 = pth->ctx.Gpr14;
			lpContext->Gpr15 = pth->ctx.Gpr15;
			lpContext->Gpr16 = pth->ctx.Gpr16;
			lpContext->Gpr17 = pth->ctx.Gpr17;
			lpContext->Gpr18 = pth->ctx.Gpr18;
			lpContext->Gpr19 = pth->ctx.Gpr19;
			lpContext->Gpr20 = pth->ctx.Gpr20;
			lpContext->Gpr21 = pth->ctx.Gpr21;
			lpContext->Gpr22 = pth->ctx.Gpr22;
			lpContext->Gpr23 = pth->ctx.Gpr23;
			lpContext->Gpr24 = pth->ctx.Gpr24;
			lpContext->Gpr25 = pth->ctx.Gpr25;
			lpContext->Gpr26 = pth->ctx.Gpr26;
			lpContext->Gpr27 = pth->ctx.Gpr27;
			lpContext->Gpr28 = pth->ctx.Gpr28;
			lpContext->Gpr29 = pth->ctx.Gpr29;
			lpContext->Gpr30 = pth->ctx.Gpr30;
			lpContext->Gpr31 = pth->ctx.Gpr31;
			lpContext->Cr = pth->ctx.Cr;
			lpContext->Xer = pth->ctx.Xer;
		}
		if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {

		}
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
	if (lpContext->ContextFlags & ~(CONTEXT_FULL|CONTEXT_DEBUG_REGISTERS)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
		ULONG ulOldAky = CurAKey;
		SETCURKEY((DWORD)-1);
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			pth->pThrdDbg->psavedctx->Msr = (pth->pThrdDbg->psavedctx->Msr & 0xfffff9ff) | (lpContext->Msr & 0x00000600);
			pth->pThrdDbg->psavedctx->Iar = lpContext->Iar;
			pth->pThrdDbg->psavedctx->Lr = lpContext->Lr;
			pth->pThrdDbg->psavedctx->Ctr = lpContext->Ctr;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
			pth->pThrdDbg->psavedctx->Gpr0 = lpContext->Gpr0;
			pth->pThrdDbg->psavedctx->Gpr1 = lpContext->Gpr1;
			pth->pThrdDbg->psavedctx->Gpr2 = lpContext->Gpr2;
			pth->pThrdDbg->psavedctx->Gpr3 = lpContext->Gpr3;
			pth->pThrdDbg->psavedctx->Gpr4 = lpContext->Gpr4;
			pth->pThrdDbg->psavedctx->Gpr5 = lpContext->Gpr5;
			pth->pThrdDbg->psavedctx->Gpr6 = lpContext->Gpr6;
			pth->pThrdDbg->psavedctx->Gpr7 = lpContext->Gpr7;
			pth->pThrdDbg->psavedctx->Gpr8 = lpContext->Gpr8;
			pth->pThrdDbg->psavedctx->Gpr9 = lpContext->Gpr9;
			pth->pThrdDbg->psavedctx->Gpr10 = lpContext->Gpr10;
			pth->pThrdDbg->psavedctx->Gpr11 = lpContext->Gpr11;
			pth->pThrdDbg->psavedctx->Gpr12 = lpContext->Gpr12;
			pth->pThrdDbg->psavedctx->Gpr13 = lpContext->Gpr13;
			pth->pThrdDbg->psavedctx->Gpr14 = lpContext->Gpr14;
			pth->pThrdDbg->psavedctx->Gpr15 = lpContext->Gpr15;
			pth->pThrdDbg->psavedctx->Gpr16 = lpContext->Gpr16;
			pth->pThrdDbg->psavedctx->Gpr17 = lpContext->Gpr17;
			pth->pThrdDbg->psavedctx->Gpr18 = lpContext->Gpr18;
			pth->pThrdDbg->psavedctx->Gpr19 = lpContext->Gpr19;
			pth->pThrdDbg->psavedctx->Gpr20 = lpContext->Gpr20;
			pth->pThrdDbg->psavedctx->Gpr21 = lpContext->Gpr21;
			pth->pThrdDbg->psavedctx->Gpr22 = lpContext->Gpr22;
			pth->pThrdDbg->psavedctx->Gpr23 = lpContext->Gpr23;
			pth->pThrdDbg->psavedctx->Gpr24 = lpContext->Gpr24;
			pth->pThrdDbg->psavedctx->Gpr25 = lpContext->Gpr25;
			pth->pThrdDbg->psavedctx->Gpr26 = lpContext->Gpr26;
			pth->pThrdDbg->psavedctx->Gpr27 = lpContext->Gpr27;
			pth->pThrdDbg->psavedctx->Gpr28 = lpContext->Gpr28;
			pth->pThrdDbg->psavedctx->Gpr29 = lpContext->Gpr29;
			pth->pThrdDbg->psavedctx->Gpr30 = lpContext->Gpr30;
			pth->pThrdDbg->psavedctx->Gpr31 = lpContext->Gpr31;
			pth->pThrdDbg->psavedctx->Cr = lpContext->Cr;
			pth->pThrdDbg->psavedctx->Xer = lpContext->Xer;
		}
		if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {

		}
		if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

		}
		SETCURKEY(ulOldAky);
	} else {
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			pth->ctx.Msr = (pth->ctx.Msr & 0xfffff9ff) | (lpContext->Msr & 0x00000600);
			pth->ctx.Iar = lpContext->Iar;
			pth->ctx.Lr = lpContext->Lr;
			pth->ctx.Ctr = lpContext->Ctr;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
			pth->ctx.Gpr0 = lpContext->Gpr0;
			pth->ctx.Gpr1 = lpContext->Gpr1;
			pth->ctx.Gpr2 = lpContext->Gpr2;
			pth->ctx.Gpr3 = lpContext->Gpr3;
			pth->ctx.Gpr4 = lpContext->Gpr4;
			pth->ctx.Gpr5 = lpContext->Gpr5;
			pth->ctx.Gpr6 = lpContext->Gpr6;
			pth->ctx.Gpr7 = lpContext->Gpr7;
			pth->ctx.Gpr8 = lpContext->Gpr8;
			pth->ctx.Gpr9 = lpContext->Gpr9;
			pth->ctx.Gpr10 = lpContext->Gpr10;
			pth->ctx.Gpr11 = lpContext->Gpr11;
			pth->ctx.Gpr12 = lpContext->Gpr12;
			pth->ctx.Gpr13 = lpContext->Gpr13;
			pth->ctx.Gpr14 = lpContext->Gpr14;
			pth->ctx.Gpr15 = lpContext->Gpr15;
			pth->ctx.Gpr16 = lpContext->Gpr16;
			pth->ctx.Gpr17 = lpContext->Gpr17;
			pth->ctx.Gpr18 = lpContext->Gpr18;
			pth->ctx.Gpr19 = lpContext->Gpr19;
			pth->ctx.Gpr20 = lpContext->Gpr20;
			pth->ctx.Gpr21 = lpContext->Gpr21;
			pth->ctx.Gpr22 = lpContext->Gpr22;
			pth->ctx.Gpr23 = lpContext->Gpr23;
			pth->ctx.Gpr24 = lpContext->Gpr24;
			pth->ctx.Gpr25 = lpContext->Gpr25;
			pth->ctx.Gpr26 = lpContext->Gpr26;
			pth->ctx.Gpr27 = lpContext->Gpr27;
			pth->ctx.Gpr28 = lpContext->Gpr28;
			pth->ctx.Gpr29 = lpContext->Gpr29;
			pth->ctx.Gpr30 = lpContext->Gpr30;
			pth->ctx.Gpr31 = lpContext->Gpr31;
			pth->ctx.Cr = lpContext->Cr;
			pth->ctx.Xer = lpContext->Xer;
		}
		if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {

		}
		if (lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) {

		}
	}
	return TRUE;
}

//
// Walk the HAL KVA Memory Map and find the KVA for
// a given physical address.
//
PVOID Phys2Virt(DWORD pfn)
{
    int		i = 0;
    DWORD	va;       // Virtual Base Address of section
    DWORD	pa;       // Physical Base Address of section
    DWORD	pau;      // Physical Address Upper Bound of section
    DWORD	pfnmb;    // PFN rounded down to 1MB

    //
    // The end of the table is marked by an entry with a ZERO size.
    //
	while( OEMMemoryMap[i].Size )
	{
        va = OEMMemoryMap[i].KernelVirtualAddress & 0x1FF00000;
        pa = OEMMemoryMap[i].PhysicalAddress & 0xFFF00000;
        pau = pa + (OEMMemoryMap[i].Size << 20) - 1;
        pfnmb = pfn & 0xfff00000;
        if ((pfnmb >= pa) && (pfnmb <= pau))
		{
            return ((PVOID) ((pfn - pa) + va + 0x80000000));
		}
        i++;
    }
    DEBUGMSG(ZONE_PHYSMEM, (TEXT("Phys2Virt() : PFN (0x%08X) not found!\r\n"), pfn));
    return ((PVOID) (pfn | 0xFFF00000));
}
