/* Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved. */

#include "kernel.h"

#ifdef THUMBSUPPORT
#define THUMB_SUPPORT_STR TEXT("(Thumb Enabled)")
#else
#define THUMB_SUPPORT_STR TEXT(" ")
#endif

#ifdef THUMB
const wchar_t NKSignon[] = TEXT("Windows CE Kernel for THUMB ") THUMB_SUPPORT_STR TEXT(" Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n");
#else
const wchar_t NKSignon[] = TEXT("Windows CE Kernel for ARM ") THUMB_SUPPORT_STR TEXT(" Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n");
#endif

DWORD CEProcessorType;
WORD ProcessorLevel = 4;
WORD ProcessorRevision;

DWORD CurMSec;
DWORD DiffMSec;

// Fault Status register fields:
#define FSR_DOMAIN      0xF0
#define FSR_STATUS      0x0D
#define FSR_PAGE_ERROR  0x02

#define FSR_ALIGNMENT	    0x01
#define FSR_TRANSLATION	    0x05
#define FSR_DOMAIN_ERROR	0x09
#define FSR_PERMISSION	    0x0D

#define BREAKPOINT 0x06000010	// special undefined instruction
#define THM_BREAKPOINT  0xDEFE              // Thumb equivalent

const LPCSTR IdStrings[] = {
    "RaiseException", "Reschedule", "Undefined Instruction", "SWI",
    "Prefetch Abort", "Data Abort", "IRQ", "<Invalid>", "<Invalid>", "[Stack fault]", "[HW Break]",
};

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

void DumpFrame(PTHREAD pth, PCPUCONTEXT pctx, int id, ulong addr, int level) {
    NKDbgPrintfW(L"Exception '%a' Thread=%8.8lx AKY=%8.8lx PC=%8.8lx BVA=%8.8lx\r\n",
    		IdStrings[id+1], pth, pCurThread->aky, pctx->Pc, addr);
    NKDbgPrintfW(L" R0=%8.8lx  R1=%8.8lx  R2=%8.8lx  R3=%8.8lx\r\n",
    		pctx->R0, pctx->R1, pctx->R2, pctx->R3);
    NKDbgPrintfW(L" R4=%8.8lx  R5=%8.8lx  R6=%8.8lx  R7=%8.8lx\r\n",
    		pctx->R4, pctx->R5, pctx->R6, pctx->R7);
    NKDbgPrintfW(L" R8=%8.8lx  R9=%8.8lx R10=%8.8lx R11=%8.8lx\r\n",
    		pctx->R8, pctx->R9, pctx->R10, pctx->R11);
    NKDbgPrintfW(L"R12=%8.8lx  SP=%8.8lx  Lr=%8.8lx Psr=%8.8lx\r\n",
    		pctx->R12, pctx->Sp, pctx->Lr, pctx->Psr);
}

void NextThread(void);
void KCNextThread(void);
void OEMIdle(void);
void DoPowerOff(void);
void __declspec(iw32) TLBClear(void);
ulong OEMInterruptHandler(void);

extern char InterlockedAPIs[], InterlockedEnd[];

void ARMInit(int cpuType, int abSp, int iSp, int uSp) {
    int ix;
    /* Initialize SectionTable in KPage */
    for (ix = 1 ; ix <= SECTION_MASK ; ++ix)
        SectionTable[ix] = NULL_SECTION;
    /* Copy kernel data to RAM & zero out BSS */
	KernelRelocate(pTOC);
	OEMInitDebugSerial();			// initialize serial port
	OEMWriteDebugString((PWSTR)NKSignon);
	/* Copy interlocked api code into the kpage */
	DEBUGCHK(sizeof(KData) <= FIRST_INTERLOCK);
	DEBUGCHK((InterlockedEnd-InterlockedAPIs)+FIRST_INTERLOCK <= 0x400);
	memcpy((char *)&KData+FIRST_INTERLOCK, InterlockedAPIs, InterlockedEnd-InterlockedAPIs);
	/* setup processor version information */
	CEProcessorType = cpuType >> 4 & 0xFFF;
	ProcessorRevision = cpuType & 0x0f;
	NKDbgPrintfW(L"ProcessorType=%4.4x  Revision=%d\r\n", CEProcessorType, ProcessorRevision);
	NKDbgPrintfW(L"sp_abt=%8.8x sp_irq=%8.8x sp_undef=%8.8x\r\n", abSp, iSp, uSp);
	OEMInit();			// initialize firmware
	KernelFindMemory();
	NKDbgPrintfW(L"Sp=%8.8x\r\n", &cpuType);
#ifdef DEBUG
	OEMWriteDebugString(TEXT("ARMInit done.\r\n"));
#endif
}

typedef struct ExcInfo {
    DWORD   linkage;
	ULONG	oldPc;
	UINT    oldMode;
	char    id;
	BYTE    lowSpBits;
	ushort  fsr;
	ULONG	addr;
} EXCINFO;
typedef EXCINFO *PEXCINFO;

ERRFALSE(sizeof(EXCINFO) <= sizeof(CALLSTACK));
ERRFALSE(offsetof(EXCINFO,linkage) == offsetof(CALLSTACK,pcstkNext));
ERRFALSE(offsetof(EXCINFO,oldPc) == offsetof(CALLSTACK,retAddr));
ERRFALSE(offsetof(EXCINFO,oldMode) == offsetof(CALLSTACK,pprcLast));
ERRFALSE(64 >= sizeof(CALLSTACK));

PTHREAD HandleException(PTHREAD pth, int id, ulong addr, ushort info) {
	PEXCINFO pexi;
	DWORD stackaddr;
    if (id != ID_RESCHEDULE) {
#if 0
	    NKDbgPrintfW(L"%a: Thread=%8.8lx Proc=%8.8lx AKY=%8.8lx\r\n",
   			IdStrings[id+1], pth, pCurProc, pCurThread->aky);
   	    NKDbgPrintfW(L"PC=%8.8lx Lr=%8.8lx Sp=%8.8lx Psr=%4.4x\r\n",
   			pth->ctx.Pc, pth->ctx.Lr, pth->ctx.Sp, pth->ctx.Psr);
      	if (id == ID_DATA_ABORT)
       	    NKDbgPrintfW(L"FAR=%8.8lx FSR=%4.4x\r\n", addr, info);
#endif
		
		KCALLPROFON(0);
	    pexi = (struct ExcInfo *)((pth->ctx.Sp & ~63) - sizeof(CALLSTACK));
	   	if (!((DWORD)pexi & 0x80000000) && DemandCommit((DWORD)pexi)) {
			stackaddr = (DWORD)pexi & ~(PAGE_SIZE-1);
			if ((stackaddr >= pth->dwStackBound) || (stackaddr < pth->dwStackBase) ||
				((pth->dwStackBound = stackaddr) >= (pth->dwStackBase + MIN_STACK_RESERVE)) ||
				TEST_STACKFAULT(pth)) {
				KCALLPROFOFF(0);
				return pth; // restart instruction
			}
			SET_STACKFAULT(pth);
	    	id = ID_STACK_FAULT;   // stack fault exception code
			addr = (DWORD)pexi;
		}
	   	if (pth->ctx.Pc != (ulong)CaptureContext+4) {
	   		pexi->id = id;
	   		pexi->lowSpBits = (uchar)pth->ctx.Sp & 63;
	   		pexi->oldPc = pth->ctx.Pc;
	   		pexi->oldMode = pth->ctx.Psr & 0xFF;
	   		pexi->addr = addr;
	   		pexi->fsr = info;
	        pexi->linkage = (DWORD)pth->pcstkTop | 1;
    	    pth->pcstkTop = (PCALLSTACK)pexi;
	   		pth->ctx.Sp = (DWORD)pexi;
	   		if ((pexi->oldMode & 0x1F) == USER_MODE)
   			    pth->ctx.Psr = (pth->ctx.Psr & ~0xFF) | SYSTEM_MODE;
	   		else
    	   		pth->ctx.Psr &= ~THUMB_STATE;
    		pth->ctx.Pc = (ULONG)CaptureContext;
    		KCALLPROFOFF(0);
	   		return pth;			// continue execution
	   	}
		DumpFrame(pth, &pth->ctx, id, addr, 10);
	    RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pth));
		SurrenderCritSecs();
		SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
		RunList.pth = 0;
		SetReschedule();
		KCALLPROFOFF(0);
		return 0;
    }
    if (PowerOffFlag) {
        DoPowerOff();
        PowerOffFlag = 0;
    }
reschedTop:
  	if (ReschedFlag) {
        ReschedFlag = 0;
        NextThread();
    }
	if (KCResched) {
		KCResched = 0;
		KCNextThread();
	}
	if (KCResched)
		goto reschedTop;

	if (!RunList.pth) {
		INTERRUPTS_OFF();
		if (!ReschedFlag && !KCResched) {
			OEMIdle();
			INTERRUPTS_ON();
			ReschedFlag = 1;
			goto reschedTop;
		} else {
			INTERRUPTS_ON();
			goto reschedTop;
		}
	}

    SetCPUASID(RunList.pth);
	hCurThread = RunList.pth->hTh;
    pCurThread = RunList.pth;
    KPlpvTls = RunList.pth->tlsPtr;
	return RunList.pth;
}

// LoadPageTable - load entries into page tables from kernel structures
//
//  LoadPageTable is called for prefetch and data aborts to copy page table
// entries from the kernel virtual memory tables to the ARM h/w page tables.

DWORD currsecond;
DWORD secondpos[16];
DWORD secondslot[16];
DWORD used[16];

LPDWORD SecondAddr(DWORD ind, DWORD ent) {
	return (LPDWORD)&ArmHigh->aPT[ind].PTEs[ent];
}

LPDWORD FirstAddr(DWORD ind) {
	return (LPDWORD)&ArmHigh->firstPT[ind];
}

int LoadPageTable(ulong addr) {
    PSECTION pscn;
    MEMBLOCK *pmb;
    ulong entry;
    unsigned ix1st, ix2nd;
    int loop;
    if ((addr < 0x80000000) && (pscn = SectionTable[addr>>VA_SECTION]) &&
        (pmb = (*pscn)[(addr>>VA_BLOCK)&BLOCK_MASK]) && (pmb != RESERVED_BLOCK) && TestAccess(&pmb->alk, &CurAKey) &&
        ((entry = pmb->aPages[(addr>>VA_PAGE)&PAGE_MASK]) & PG_VALID_MASK)) {
        ix1st = addr >> 20;               // index into 1st level page table
        ix2nd = (addr >> VA_PAGE) & L2_MASK; // index into 2nd level page table
        // Remove previous entry from the page tables.
		if (*FirstAddr(ix1st)) {
			for (loop = 0; loop < 16; loop++)
				if (used[loop] && (secondpos[loop] == ix1st))
					break;
			DEBUGCHK(loop != 16);
			*SecondAddr(loop,secondslot[loop]) = 0;
			secondslot[loop] = ix2nd;
			*SecondAddr(loop,ix2nd) = entry;
		} else {
			if (used[currsecond]) {
				*FirstAddr(secondpos[currsecond]) = 0;
				*SecondAddr(currsecond,secondslot[currsecond]) = 0;
			} else
	    	    used[currsecond] = 1;
			secondpos[currsecond] = ix1st;
			secondslot[currsecond] = ix2nd;
			*SecondAddr(currsecond,ix2nd) = entry;
    	    *FirstAddr(ix1st) = PageTableDescriptor + currsecond*sizeof(PAGETBL);
        	currsecond = (currsecond+1)&0xf;
        }
        return TRUE;    // tell abort handler to retry
    }
    return FALSE;
}

void InvalidateSoftwareTLB(void) {
	int loop;
	if (!InSysCall()) {
        KCall((PKFN)InvalidateSoftwareTLB);
       	return;
    }
    KCALLPROFON(63);
    for (loop = 0; loop < 16; loop++) {
	    // Remove previous entry from the page tables.
	    if (used[loop]) {
	    	*FirstAddr(secondpos[loop]) = 0;
			*SecondAddr(loop,secondslot[loop]) = 0;
		    used[loop] = 0;
		}
	}
    KCALLPROFOFF(63);
}

// FlushTLB - clear TLBs and cached page table entries
//
//  FlushTLB is called by the virtual memory system whenever the permissions on
// any pages are changed or when any pages are freed.

void FlushTLB(void) {
    FlushICache();
    FlushDCache();
    InvalidateSoftwareTLB();
    TLBClear();     // clear h/w TLBs
}

void ExceptionDispatch(PCONTEXT pctx) {
	EXCEPTION_RECORD er;
	ULONG FaultAddr;
	ULONG addr;
	int id;
	BOOL bHandled;
    BOOL ThumbMode;
	PTHREAD pth; 
	PEXCINFO pexi;
	uint fsr;
	pth = pCurThread;
	pexi = (PEXCINFO)pth->pcstkTop;
	DEBUGMSG(ZONE_SEH, (TEXT("ExceptionDispatch: pexi=%8.8lx Pc=%8.8lx\r\n"), pexi, pexi->oldPc));
	pctx->Pc = pexi->oldPc;
	SetContextMode(pctx, pexi->oldMode);
	pctx->Sp = (ULONG)pctx + sizeof(CONTEXT);
	memset(&er, 0, sizeof(er));
	er.ExceptionAddress = (PVOID)pctx->Pc;
#if defined(THUMBSUPPORT)
    ThumbMode = (pctx->Psr & THUMB_STATE) != 0;
#else
    ThumbMode = FALSE;
#endif
    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    // HandleExcepion sets the LSB of the callstack linkage but ObjectCall
    // does not.
    if (!(pexi->linkage & 1)) {
		


		DEBUGMSG(ZONE_SEH, (TEXT("Raising exception %x flags=%x args=%d pexi=%8.8lx\r\n"),
				pctx->R0, pctx->R1, pctx->R2, pexi));
		er.ExceptionCode = pctx->R0;
		er.ExceptionFlags = pctx->R1;
		if (pctx->R3 && pctx->R2) {
			if (pctx->R2 > EXCEPTION_MAXIMUM_PARAMETERS) {
				er.ExceptionCode = STATUS_INVALID_PARAMETER;
				er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
			} else {
				memcpy(er.ExceptionInformation, (void*)pctx->R3, pctx->R2*sizeof(DWORD));
				er.NumberParameters = pctx->R2;
			}
		}
		id = -1;
    } else {
        // CPU detected exception. Extract some additional information about
        // the cause of the exception from the EXCINFO (CALLSTACK) structure.

        //
        //  The exception handling code uses bit 0 to indicate Thumb vs. ARM 
        //  execution. This bit is not set by the hardware on exceptions. Set
        //  the bit here:
        //
        if ( ThumbMode ){
            pctx->Pc |= 0x01;
        }
        
    	addr = pexi->addr;
    	id = pexi->id;
    	pctx->Sp += pexi->lowSpBits + sizeof(CALLSTACK);
    	fsr = pexi->fsr;
		if (((id == ID_DATA_ABORT) && ((fsr & FSR_STATUS) == FSR_TRANSLATION)) && AutoCommit(addr)) {
			pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
			goto continueExecution;
		}
    	switch (id) {
	    	case ID_UNDEF_INSTR:
	            er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
	            if ( ThumbMode ){
	                FaultAddr = pctx->Pc & ~0x01;
	                if ( *(PUSHORT)FaultAddr == THM_BREAKPOINT ){
	                    er.ExceptionCode = STATUS_BREAKPOINT;
	                    er.ExceptionInformation[0] = *(PUSHORT)FaultAddr;
	                }
	            } else if ( (*(PULONG)pctx->Pc & 0x0fffffff) == BREAKPOINT ){
	                er.ExceptionCode = STATUS_BREAKPOINT;
	                er.ExceptionInformation[0] = *(PULONG)pctx->Pc;
	            }
	    	    break;
	        case ID_SWI_INSTR:
   		        er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
    		    break;
	    	case ID_DATA_ABORT:		// Data page fault
	            //
    	        // Determine load vs. store depending on whether ARM or Thumb
        	    // mode:
	            if ( ThumbMode ){
	                ULONG Instr;
	                FaultAddr = pctx->Pc & ~0x01;               // Clear Thumb bit
    	            Instr = *(PUSHORT)FaultAddr;
        	        if ((Instr & 0xFE00) == 0x5600){
            	        // Discontinuity in the Thumb instruction set:
                	    //  LDRSB instruction does not have bit 11 set.
                    	er.ExceptionInformation[0] = 0;
	                } else {
    	                //  All other load instructions have bit 11 set. The
        	            //  corresponding store instructions have bit 11 clear
        		        er.ExceptionInformation[0] = !(Instr & (1<<11));
                	}
	            } else {
    	            FaultAddr = pctx->Pc;
        		    er.ExceptionInformation[0] = !((*(PULONG)FaultAddr) & (1<<20));
            	}
    	    	switch(fsr & FSR_STATUS) {
	    		    case FSR_TRANSLATION:
    	    	    case FSR_DOMAIN_ERROR:
		            case FSR_PERMISSION:
		
						DEBUGMSG(ZONE_PAGING, (L"ExD: ID_DATA_ABORT\r\n"));

						goto doPageFault;
		    	    case FSR_ALIGNMENT:
       					er.ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
		       			break;
		       		default:
		       		    er.ExceptionCode = 0xdfff0123;  
		       		    break;
	    	    }
    		    break;
	    	case ID_PREFETCH_ABORT:		// Code page fault
    		    addr = pctx->Pc;
				
				DEBUGMSG(ZONE_PAGING, (L"ExD: ID_PREFETCH_ABORT\r\n"));

    doPageFault:
				DEBUGMSG(ZONE_PAGING, (L"ExD: addr = %8.8lx\r\n",addr));

				if (ProcessPageFault(er.ExceptionInformation[0], addr)) {

					pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);

					DEBUGMSG(ZONE_PAGING, (L"ExD: continuing.  lr = %8.8lx\r\n", pctx->Lr));
					
					goto continueExecution;
    			}
	    		er.ExceptionCode = STATUS_ACCESS_VIOLATION;
    			er.ExceptionInformation[1] = addr;
    			er.NumberParameters = 2;
    			break;
	    	case ID_STACK_FAULT:    // Stack overflow
    		    er.ExceptionCode = STATUS_STACK_OVERFLOW;
    			er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    	    	break;
    	}
    }
	if (er.ExceptionCode != STATUS_BREAKPOINT) {
	    NKDbgPrintfW(L"%a: Thread=%8.8lx Proc=%8.8lx '%s'\r\n",IdStrings[id+1], pth, pCurProc, pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
	    NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx RA=%8.8lx BVA=%8.8lx\r\n", pCurThread->aky, pctx->Pc, pctx->Lr, addr);
	    if (UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) {
	        NKDbgPrintfW(L"TLSKERN_NOFAULT set... bypassing kernel debugger.\r\n");
	    }
    }
	// Unlink the EXCINFO structure from the threads call stack chain.
	pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);

    // Invoke the kernel debugger to attempt to debug the exception before
    // letting the program resolve the condition via SEH.
	if (!UserDbgTrap(&er,pctx,FALSE) && ((UTlsPtr()[TLSSLOT_KERNEL] & TLSKERN_NOFAULT) || !KDTrap(&er, pctx, FALSE))) {
		if (er.ExceptionCode == STATUS_BREAKPOINT) {
			RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx MD=%2x Ignored.\r\n"), pctx->Pc,
			        pctx->Psr & 0xFF));
			pctx->Pc += ThumbMode ? 2 : 4;  // skip over the BREAK instruction
		} else {
			bHandled = NKDispatchException(pth, &er, pctx);
			if (!bHandled) {
			    if (!UserDbgTrap(&er, pctx, TRUE) && !KDTrap(&er, pctx, TRUE)) {
					// Terminate the process.
					RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"), er.ExceptionCode));
					DumpFrame(pth, (PCPUCONTEXT)&pctx->Psr, id, addr, 0);
					if (InSysCall()) {
	    			    OutputDebugStringW(L"Halting system\r\n");
	    			    for (;;)
	    			        ;
	    			} else {
	    				if (!GET_DEAD(pth)) {
	    					SET_DEAD(pth);
		    				pctx->Pc = (ULONG)pExitThread;
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

#if defined(THUMBSUPPORT)
            //
            // Update PSR based on ARM/Thumb continuation:
            //
            if ( pctx->Pc & 0x01 ){
                pctx->Psr |= THUMB_STATE;
            } else {
                pctx->Psr &= ~THUMB_STATE;
            }
#endif

		}
	}
	if ((id == ID_DATA_ABORT) || (id == ID_PREFETCH_ABORT))
		GuardCommit(addr);
continueExecution:
	// If returning from handling a stack overflow, reset the thread's stack overflow
	// flag. It would be good to free the tail of the stack at this time
	// so that the thread will stack fault again if the stack gets too big. But we
	// are currently using that stack page.
	if (id == ID_STACK_FAULT)
    	CLEAR_STACKFAULT(pth);
    if (GET_DYING(pth) && !GET_DEAD(pth) && (pCurProc == pth->pOwnerProc)) {
		SET_DEAD(pth);
		CLEAR_USERBLOCK(pth);
		CLEAR_DEBUGWAIT(pth);
		pctx->Pc = (ULONG)pExitThread;
		pctx->R0 = 0;
#if defined(THUMBSUPPORT)
        //
        // Update PSR based on ARM/Thumb continuation:
        //
        if ( pctx->Pc & 0x01 ){
            pctx->Psr |= THUMB_STATE;
        } else {
            pctx->Psr &= ~THUMB_STATE;
        }
#endif
	}   
}

//
// Page Table Entry from OEMAddressTable
//
typedef struct {
    DWORD dwVA;
    DWORD dwPA;
    DWORD dwSize;
} PTE, *PPTE;

//
// The Physical to Virtual mapping table is supplied by OEM.
//
extern PTE OEMAddressTable[];

PVOID Phys2Virt(DWORD pfn) {
    int i = 0;
    DWORD va;       // Virtual Base Address of section
    DWORD pa;       // Physical Base Address of section
    DWORD pau;      // Physical Address Upper Bound of section
    DWORD pfnmb;    // PFN rounded down to 1MB
    //
    // The end of the table is marked by an entry with a ZERO size.
    //
    while(OEMAddressTable[i].dwSize) {
        va = OEMAddressTable[i].dwVA & 0x1FF00000;
        pa = OEMAddressTable[i].dwPA & 0xFFF00000;
        pau = pa + (OEMAddressTable[i].dwSize << 20) - 1;
        pfnmb = pfn & 0xfff00000;
        if ((pfnmb >= pa) && (pfnmb <= pau))
            return ((PVOID) ((pfn - pa) + va + 0x80000000));
        i++;
    }
    DEBUGMSG(ZONE_PHYSMEM, (TEXT("Phys2Virt() : PFN (0x%08X) not found!\r\n"), pfn));
    return NULL;
}

LPVOID VerifyAccess(LPVOID pvAddr, DWORD dwFlags, ACCESSKEY aky) {
    PSECTION pscn;
    MEMBLOCK *pmb;
    ulong entry;
    int i=0;
    if ((long)pvAddr >= 0) {
        if ((pscn = SectionTable[(ulong)pvAddr>>VA_SECTION]) &&
			(pmb = (*pscn)[((ulong)pvAddr>>VA_BLOCK)&BLOCK_MASK]) &&
            (pmb != RESERVED_BLOCK) && (pmb->alk & aky) &&
            ((entry = pmb->aPages[((ulong)pvAddr>>VA_PAGE)&PAGE_MASK]) & PG_VALID_MASK) &&
            (!(dwFlags & VERIFY_WRITE_FLAG) || ((entry&PG_PROTECTION) == PG_PROT_WRITE)))
            return Phys2Virt(PFNfromEntry(entry) | ((ulong)pvAddr & (PAGE_SIZE-1)));
    } else {
        // Kernel mode only address. If the "kernel mode OK" flag is set or if the
        // thread is running in kernel mode, allow the access.
        if (((dwFlags & VERIFY_KERNEL_OK) || (GetThreadMode(pCurThread) == KERNEL_MODE)) &&
            (((ulong)pvAddr < 0xC0000000) || ((ulong)pvAddr >= 0xFFFD0000))) {

			// kernel data and page table territory...
            if(((ulong)pvAddr >= 0xFFFD0000) && ((ulong)pvAddr < 0xFFFD8000)) 
            	return pvAddr;
            if(((ulong)pvAddr >= 0xFFFF0000) && ((ulong)pvAddr < 0xFFFF1000))
            	return pvAddr;
            if(((ulong)pvAddr >= 0xFFFF2000) && ((ulong)pvAddr < 0xFFFF3000))
            	return pvAddr;
            if(((ulong)pvAddr >= 0xFFFF4000) && ((ulong)pvAddr < 0xFFFF5000))
            	return pvAddr;
            if(((ulong)pvAddr >= 0xFFFFC000) && ((ulong)pvAddr < 0xFFFFD000))
            	return pvAddr;
            	
			// Need to check the OEM address map to see if the address we've been 
			// handed is indeed mapped...
			while(OEMAddressTable[i].dwSize != 0) {
				if((OEMAddressTable[i].dwVA < (ulong)pvAddr) && 
					((ulong)pvAddr < (OEMAddressTable[i].dwVA + (OEMAddressTable[i].dwSize * 0x00100000)))) {
					return pvAddr;
				}
				i++;
			}

    	}
    }
    return 0;
}

/* Machine dependent thread creation */

#define STKALIGN 8
#define STKMSK (STKALIGN-1)

// normal thread stack: from top, TLS then args then free

void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase,
	LPVOID lpStart, DWORD dwVMBase, BOOL kmode, ulong param) {
    PDWORD pTLS;
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)lpStack + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
	// Leave room for arguments and TLS on the stack
	pTLS = (PDWORD)((PBYTE)lpStack + cbStack - TLS_MINIMUM_AVAILABLE*4);
	pTh->ctx.Sp = (ulong)pTLS;
	pTh->dwStackBound = pTh->ctx.Sp & ~(PAGE_SIZE-1);
	pTh->tlsPtr = pTLS;
	pTh->ctx.R0 = (ulong)lpStart;
	pTh->ctx.R1 = param;
	pTh->ctx.Lr = 4;
    pTh->ctx.Pc = (ULONG)lpBase;
	pTh->ctx.Psr = (kmode || bAllKMode) ? KERNEL_MODE : USER_MODE;
#if defined(THUMBSUPPORT)
    if ( (pTh->ctx.Pc & 0x01) != 0 ){
        pTh->ctx.Psr |= THUMB_STATE;
    }
#endif

    DEBUGMSG(ZONE_SCHEDULE, (L"MDCT: pTh=%8.8lx Pc=%8.8lx Psr=%4.4x GP=%8.8lx Sp=%8.8lx\r\n", pTh, pTh->ctx.Pc, pTh->ctx.Psr, pTh->ctx.R9, pTh->ctx.Sp));
}

// main thread stack: from top, TLS then buf then buf2 then buf2 (ascii) then args then free

LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
	LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2) {
    PBYTE buffer;
	LPCWSTR pcmdline;
	
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
	buffer = (PBYTE)lpStack + cbStack - TLS_MINIMUM_AVAILABLE*4;
	KPlpvTls = pTh->tlsPtr = (PDWORD)buffer;

    // Allocate space for extra parameters on the stack and copy the bytes there.
    buffer -= (buflen+STKMSK)&~STKMSK;
    pcmdline = (LPCWSTR)buffer;
    memcpy(buffer, buf, buflen);

    buffer -= (buflen2+STKMSK)&~STKMSK;
    memcpy(buffer, buf2, buflen2);

	pTh->pOwnerProc->lpszProcName = (LPWSTR)buffer;
	return pcmdline;
}

void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart,
	BOOL kmode, ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4) {
    LPBYTE buffer;
    LPBYTE arg3; 
	// Allocate space for TLS & arguments to the start function.
	arg3 = (PBYTE)pTh->dwStackBase + cbStack - TLS_MINIMUM_AVAILABLE*4 - ((buflen+STKMSK)&~STKMSK);
	// Allocate space for process name.
	buffer = arg3 - ((buflen2+STKMSK)&~STKMSK);
	DEBUGCHK((LPBYTE)pTh->pOwnerProc->lpszProcName == buffer);
	// Allocate space for arguments to the start function.
	buffer -= sizeof(ulong);
	pTh->ctx.Sp = (ulong)buffer;
	pTh->dwStackBound = pTh->ctx.Sp & ~(PAGE_SIZE-1);
	pTh->ctx.R0 = (ulong)lpStart;
	pTh->ctx.R1 = p1;
	pTh->ctx.R2 = p2;
	pTh->ctx.R3 = (ulong)arg3;
	*(ulong*)buffer = p4;
	pTh->ctx.Lr = 4;
    pTh->ctx.Pc = (ULONG)lpBase;
	pTh->ctx.Psr = (kmode || bAllKMode) ? KERNEL_MODE : USER_MODE;
#if defined(THUMBSUPPORT)
    if ( (pTh->ctx.Pc & 0x01) != 0 ){
        pTh->ctx.Psr |= THUMB_STATE;
    }
#endif

    DEBUGMSG(ZONE_SCHEDULE, (L"MDCMainT: pTh=%8.8lx Pc=%8.8lx Psr=%4.4x TOC=%8.8lx Sp=%8.8lx\r\n", pTh, pTh->ctx.Pc, pTh->ctx.Psr, pTh->ctx.R9, pTh->ctx.Sp));
}

BOOL DoThreadGetContext(HANDLE hTh, PCONTEXT lpContext) {
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
		ULONG ulOldAky = CurAKey;
		SETCURKEY((DWORD)-1);
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			lpContext->Psr = pth->pThrdDbg->psavedctx->Psr;
			lpContext->Pc = pth->pThrdDbg->psavedctx->Pc;
			lpContext->Lr = pth->pThrdDbg->psavedctx->Lr;
			lpContext->Sp = pth->pThrdDbg->psavedctx->Sp;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
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
		}
		SETCURKEY(ulOldAky);
	} else {
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			lpContext->Psr = pth->ctx.Psr;
			lpContext->Pc = pth->ctx.Pc;
			lpContext->Lr = pth->ctx.Lr;
			lpContext->Sp = pth->ctx.Sp;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
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
		ULONG ulOldAky = CurAKey;
		SETCURKEY((DWORD)-1);
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			pth->pThrdDbg->psavedctx->Psr = (pth->pThrdDbg->psavedctx->Psr & 0xdf) | (lpContext->Psr & ~0xdf);
			pth->pThrdDbg->psavedctx->Pc = lpContext->Pc;
			pth->pThrdDbg->psavedctx->Lr = lpContext->Lr;
			pth->pThrdDbg->psavedctx->Sp = lpContext->Sp;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
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
		}
		SETCURKEY(ulOldAky);
	} else {
		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
			pth->ctx.Psr = (pth->ctx.Psr & 0xdf) | (lpContext->Psr & ~0xdf);
			pth->ctx.Pc = lpContext->Pc;
			pth->ctx.Lr = lpContext->Lr;
			pth->ctx.Sp = lpContext->Sp;
		}
		if (lpContext->ContextFlags & CONTEXT_INTEGER) {
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
		}
	}
	return TRUE;
}

//
// Define procedure prototypes for exception filter and termination handler
// execution routines defined in jmpunwnd.s
//

// For ARM the compiler generated exception functions expect the following
// input arguments:
//
//	Exception Filter: r0 = exception pointers, r11 = frame pointer
//
//	Termination Handler: r0 = abnormal term flag, r11 = frame pointer

long __declspec(iw32) __C_ExecuteExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG EstablisherFrame);

void __declspec(iw32) __C_ExecuteTerminationHandler(
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
                Value = __C_ExecuteExceptionFilter(&ExceptionPointers,ExceptionFilter,(ULONG)EstablisherFrame);
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
        TargetPc = CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
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
                    DispatcherContext->ControlPc = ScopeTable->ScopeRecord[Index].EndAddress + 4;
                    TerminationHandler = (TERMINATION_HANDLER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                    __C_ExecuteTerminationHandler(TRUE, TerminationHandler, (ULONG)EstablisherFrame);
                }
            }
        }
    }
    // Continue search for exception or termination handlers.
    return ExceptionContinueSearch;
}

