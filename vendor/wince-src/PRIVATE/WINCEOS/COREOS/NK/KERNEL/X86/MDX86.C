/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

#include "kernel.h"

#define Naked void __declspec(naked)

extern FXSAVE_AREA g_InitialFPUState;
extern KIDTENTRY g_aIntDescTable[];

// Default processor type & revision level information.
DWORD CEProcessorType = PROCESSOR_INTEL_486;
WORD ProcessorLevel = 0;
WORD ProcessorRevision = 0;
DWORD ProcessorFeatures = 0;

#define FN_BITS_PER_TAGWORD         16
#define FN_TAG_EMPTY                0x3
#define FN_TAG_MASK                 0x3
#define FX_TAG_VALID                0x1
#define NUMBER_OF_FP_REGISTERS      8
#define BYTES_PER_FP_REGISTER       10
#define BYTES_PER_FX_REGISTER       16
#define TS_MASK                     0x00000008
#define FPTYPE_HARDWARE 1
#define FPTYPE_SOFTWARE 2
DWORD dwFPType;

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

void DumpFrame(PTHREAD pth, PCONTEXT pctx, int id, int level) {
    NKDbgPrintfW(L"Exception %02x Thread=%8.8lx AKY=%8.8lx EIP=%8.8lx\r\n",
    		id, pth, pCurThread->aky, pctx->Eip);
    NKDbgPrintfW(L"Eax=%8.8lx Ebx=%8.8lx Ecx=%8.8lx Edx=%8.8lx\r\n",
            pctx->Eax, pctx->Ebx, pctx->Ecx, pctx->Edx);
    NKDbgPrintfW(L"Esi=%8.8lx Edi=%8.8lx Ebp=%8.8lx Esp=%8.8lx\r\n",
            pctx->Esi, pctx->Edi, pctx->Ebp, pctx->Esp);
    NKDbgPrintfW(L"CS=%4.4lx DS=%4.4lx ES=%4.4lx SS=%4.4lx FS=%4.4lx GS=%4.4lx\r\n",
            pctx->SegCs, pctx->SegDs, pctx->SegEs, pctx->SegSs, pctx->SegFs, pctx->SegGs);
    NKDbgPrintfW(L"Flags=%8.8lx\r\n",
            pctx->EFlags);
}

void DumpTctx(PTHREAD pth, int id, ulong addr, int level) {
    ulong espValue, ssValue;
    if ((pth->ctx.TcxCs&0xFFFF) == KGDT_R0_CODE) {
        espValue = pth->ctx.TcxNotEsp+16;
        ssValue = KGDT_R0_DATA;
    } else {
        espValue = pth->ctx.TcxEsp;
        ssValue = pth->ctx.TcxSs;
    }
    NKDbgPrintfW(L"Exception %02x Thread=%8.8lx Proc=%8.8lx '%s'\r\n",
   			id, pth, hCurProc, pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
    NKDbgPrintfW(L"EIP=%8.8lx AKY=%8.8lx Flags=%8.8lx EA=%8.8lx\r\n", 
            pth->ctx.TcxEip, pCurThread->aky, pth->ctx.TcxEFlags, addr);
    NKDbgPrintfW(L"Eax=%8.8lx Ebx=%8.8lx Ecx=%8.8lx Edx=%8.8lx\r\n",
            pth->ctx.TcxEax, pth->ctx.TcxEbx, pth->ctx.TcxEcx, pth->ctx.TcxEdx);
    NKDbgPrintfW(L"Esi=%8.8lx Edi=%8.8lx Ebp=%8.8lx Esp=%8.8lx\r\n",
            pth->ctx.TcxEsi, pth->ctx.TcxEdi, pth->ctx.TcxEbp, espValue);
    NKDbgPrintfW(L"CS=%4.4lx DS=%4.4lx ES=%4.4lx SS=%4.4lx FS=%4.4lx GS=%4.4lx\r\n",
            pth->ctx.TcxCs, pth->ctx.TcxDs, pth->ctx.TcxEs, ssValue,
            pth->ctx.TcxFs, pth->ctx.TcxGs);
   	if (level > 1) {
   	    DWORD addr;
   	    PDWORD pdw;
   	    int count = 16;
   		addr = espValue;
	    if (pth->ctx.TcxEbp >= espValue)
	        count = (pth->ctx.TcxEbp+16 - espValue) / 4;
	    pdw = VerifyAccess((PVOID)addr, VERIFY_KERNEL_OK, CurAKey);
	    if (pdw)
		    DumpDwords((PDWORD)addr, count);
    }
}

typedef struct ExcInfo {
    DWORD   linkage;
	ULONG	oldEip;
	UINT    oldMode;
	UCHAR	id;
	UCHAR   oldTFlag;
	USHORT  error;
	ULONG	info;
	UCHAR	lowSp;
	UCHAR	pad[3];
} EXCINFO;
typedef EXCINFO *PEXCINFO;

ERRFALSE(sizeof(EXCINFO) <= sizeof(CALLSTACK));
ERRFALSE(offsetof(EXCINFO,linkage) == offsetof(CALLSTACK,pcstkNext));
ERRFALSE(offsetof(EXCINFO,oldEip) == offsetof(CALLSTACK,retAddr));
ERRFALSE(offsetof(EXCINFO,oldMode) == offsetof(CALLSTACK,pprcLast));
ERRFALSE(64 >= sizeof(CALLSTACK));


Naked CpuIdTrap6Handler(void)
{
    __asm {
        // Illegal opcode handler when executing cpuid instruction
        add     dword ptr [esp], 2  // skip the cpuid instr by incrementing eip
        xor     eax, eax        // simulate an unsupported cpuid instruction
        xor     ebx, ebx        // by returning 0 for return values
        xor     ecx, ecx
        xor     edx, edx
        iretd
    }
}

void IdentifyCpu(void)
{
    __asm {
        pushfd
        cli
        mov     ecx, offset g_aIntDescTable
        push    dword ptr [ecx+30h]     // Save Trap06 handler in case
        push    dword ptr [ecx+34h]     // the CPUID instruction faults
        mov     eax, offset CpuIdTrap6Handler
        mov     word ptr [ecx+30h], ax  // Set LowWord
        shr     eax, 16
        mov     word ptr [ecx+36h], ax  // Set HighWord
        xor     eax, eax                // argument to CPUID
        _emit   0fh                     // cpuid instruction
        _emit   0a2h                    // sets eax, ebx, ecx, edx
        cmp     ebx, 756e6547h          // check for "Genu" string
        jne     short cpuid_trap        // not an Intel processor
        cmp     edx, 49656e69h          // check for "ineI" string
        jne     short cpuid_trap        // not an Intel processor
        cmp     ecx, 6c65746eh          // check for "ntel" string
        jne     short cpuid_trap        // not an Intel processor
        cmp     eax, 1                  // make sure level 1 is supported
        jc      short cpuid_trap        // no, then keep defaults
        mov     eax, 1                  // get the family and stepping
        _emit   0fh                     // cpuid instruction
        _emit   0a2h
        mov     ebx, eax
        mov     ecx, eax
        shr     ebx, 4
        and     ebx, 0Fh                // (ebx) = model
        and     ecx, 0Fh                // (ecx) = stepping
        shr     eax, 8
        and     eax, 0Fh                // (eax) = family
        cmp     eax, 4
        jne     cpu_not_p4
        mov     eax, PROCESSOR_INTEL_486
        jmp     short cpuid_store
cpu_not_p4:
        cmp     eax, 5
        jne     short cpu_not_p5
        mov     eax, PROCESSOR_INTEL_PENTIUM
        jmp     short cpuid_store
cpu_not_p5:
        mov     eax, PROCESSOR_INTEL_PENTIUMII
cpuid_store:
        mov     CEProcessorType, eax
        mov     ProcessorLevel, bx
        mov     ProcessorRevision, cx
        mov     ProcessorFeatures, edx
cpuid_trap:
        mov     ecx, offset g_aIntDescTable
        pop     dword ptr [ecx+34h]     // restore trap6 handler
        pop     dword ptr [ecx+30h]
        popfd
    }
}

BOOL HandleException(PTHREAD pth, int id, ulong addr) {
	PEXCINFO pexi;
	DWORD stackaddr;
	KCALLPROFON(0);
#if 0
    NKDbgPrintfW(L"Exception %02x Thread=%8.8lx(%8.8lx) IP=%8.8lx EA=%8.8lx Err=%4.4x\r\n",
   			id, pCurThread,pth, pth->ctx.TcxEip, addr, pth->ctx.TcxError & 0xFFFF);
#endif
    pexi = (struct ExcInfo *)((pth->ctx.TcxEsp & ~63) - sizeof(CALLSTACK));
    if (!((DWORD)pexi & 0x80000000) && DemandCommit((DWORD)pexi)) {
		stackaddr = (DWORD)pexi & ~(PAGE_SIZE-1);
		if ((stackaddr >= pth->dwStackBound) || (stackaddr < pth->dwStackBase) ||
			((pth->dwStackBound = stackaddr) >= (pth->dwStackBase + MIN_STACK_RESERVE)) ||
			TEST_STACKFAULT(pth)) {
			KCALLPROFOFF(0);
			return 1; // restart instruction
		}
		SET_STACKFAULT(pth);
		id = 0xFF;		// stack fault exception code
		addr = (DWORD)pexi;
	}
	// Setup to capture the exception context in kernel mode but
	// running in thread context to allow preemption and stack growth.
	if (pth->ctx.TcxEip != (ulong)CaptureContext) {
		pexi->id = id;
		pexi->lowSp = (CHAR)(pth->ctx.TcxEsp & 63);
		pexi->oldEip = pth->ctx.TcxEip;
		pexi->oldMode = GetThreadMode(pth);
		pexi->info = addr;
		if (id != 16)
			pexi->error = (USHORT)pth->ctx.TcxError;
		else {
			WORD status;
		    _asm fnstsw status;
		    pexi->error = status;
			_asm fnclex;			
		}
		pexi->oldTFlag = (UCHAR)(pth->ctx.TcxEFlags >> 8) & 1;
        pexi->linkage = (DWORD)pCurThread->pcstkTop | 1;
        pCurThread->pcstkTop = (PCALLSTACK)pexi;
        pth->ctx.TcxEsp = (DWORD)pexi;
		pth->ctx.TcxEFlags &= ~0x0100; // Unset TF while in the debugger
		pth->ctx.TcxEip = (ulong)CaptureContext;
		if (pexi->oldMode != KERNEL_MODE)
    		SetThreadMode(pth, KERNEL_MODE);
    	KCALLPROFOFF(0);
		return TRUE;			// continue execution
	}
	DumpTctx(pth, id, addr, 10);
	RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pCurThread));
	SurrenderCritSecs();
	SET_RUNSTATE(pth,RUNSTATE_BLOCKED);
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
} EXCARGS;
typedef EXCARGS *PEXCARGS;

BOOL SetCPUHardwareWatch(LPVOID lpv, DWORD dwFlags) {
    static PPROCESS pOwnerProc;
    
    if (!InSysCall()) 
        return KCall((PKFN)SetCPUHardwareWatch, lpv, dwFlags);
        
    KCALLPROFON(73);

    if (lpv == 0) {
        // dwFlags=0 means check to see if we want to take this breakpoint
        if (dwFlags == (DWORD)-1) {
            // check to see which breakpoint went off and
            // clear the debug register
            DWORD dwReason;
            DWORD dwMask=~0xf;
            _asm {
                mov eax, dr6
                mov dwReason, eax
                and eax, dwMask
                mov dr6, eax
            }
            // was the zeroptr breakpoint hit?
            if (dwReason&0x2 && pOwnerProc != pCurProc) {
                OutputDebugString(L"zero mapped breakpoint hit in wrong process!\r\n");
                KCALLPROFOFF(73);
                return 0;
            }
            KCALLPROFOFF(73);
            return 1;
        } else {
            // dwFlags=1 means disable the data watchpoint
            dwFlags = ~0xf;
            _asm {
                mov eax, dr7
                and eax, dwFlags
                mov dr7, eax
            }
            KCALLPROFOFF(73);
            return 1;
        }
    } else {
        DWORD dwMask=~0xff000f;
        LPVOID lpvZero=(LPVOID)UnMapPtr(lpv);
        if (dwFlags == HARDWARE_WATCH_READ) {
            dwFlags = 0xff000a;
        } else {
            dwFlags = 0xdd000a;
        }
        _asm {
            mov eax, lpv
            mov dr0, eax
            mov eax, lpvZero
            mov dr1, eax
            mov eax, dr7
            and eax, dwMask
            or  eax, dwFlags
            mov dr7, eax
        }
        pOwnerProc = pCurProc;
        KCALLPROFOFF(73);
        return 1;
    }

    KCALLPROFOFF(73);
}
    
void ExceptionDispatch(PCONTEXT pctx) {
	PTHREAD pth;
	PEXCINFO pexi;
	int id;
	ULONG info;
	BOOL bHandled;
	PEXCARGS pea;
	EXCEPTION_RECORD er;
	pth = pCurThread;
	pexi = (PEXCINFO)pth->pcstkTop;
	DEBUGMSG(ZONE_SEH, (TEXT("ExceptionDispatch: pexi=%8.8lx Eip=%8.8lx id=%x\r\n"),
			pexi, pexi->oldEip, pexi->id));
	// Update CONTEXT with infomation saved in the EXCINFO structure
	pctx->Eip = pexi->oldEip;
	pctx->Esp = (DWORD)pctx + sizeof(CONTEXT);
	SetContextMode(pctx, pexi->oldMode);
	memset(&er, 0, sizeof(er));
	er.ExceptionAddress = (PVOID)pctx->Eip;
    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    // HandleExcepion sets the LSB of the callstack linkage but ObjectCall
    // does not.
    if (!(pexi->linkage & 1)) {
        NK_PCR *pcr = (NK_PCR *)((char *)pth->tlsPtr - offsetof(NK_PCR, tls));
		// Fill in exception record information from the parameters passed to
		// the RaiseException call.
		// Restore exception list linkage.
		DEBUGCHK(pcr->ExceptionList == -2);
		pcr->ExceptionList = ((PCALLSTACK)pexi)->extra;
		
		pea = (PEXCARGS)(pctx->Esp + 4);
		id = -1;
		pctx->Esp += 4;     // Remove return address from the stack
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
    	id = pexi->id;
    	info = pexi->info;
        pctx->EFlags |= pexi->oldTFlag << 8;
        pctx->Esp += pexi->lowSp + sizeof(CALLSTACK);
		if ((id == 14) && AutoCommit(info)) {
			pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
			goto continueExecution;
		}
    	// Construct an EXCEPTION_RECORD from the EXCINFO structure
    	er.ExceptionInformation[1] = info;
    	switch (id) {
    	case 14:	// Page fault
    		er.ExceptionInformation[0] = (pexi->error >> 1) & 1;
    		goto accessError;
    	case 13:		// General Protection Fault
    		er.ExceptionInformation[0] = 1; 
accessError:
    		if (ProcessPageFault(er.ExceptionInformation[0], info)) {
            	pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
    			goto continueExecution;
    		}
    		er.ExceptionCode = STATUS_ACCESS_VIOLATION;
    		er.NumberParameters = 2;
    		break;

    	case 3:		// Breakpoint
    		er.ExceptionInformation[0] = 1; // DEBUGBREAK_STOP_BREAKPOINT
    		er.ExceptionCode = STATUS_BREAKPOINT;
    		break;

    	case 2: // Stop thread breakpoint
    		er.ExceptionInformation[0] = 3; // DEBUG_THREAD_SWITCH_BREAKPOINT
    		er.ExceptionCode = STATUS_BREAKPOINT;
    		break;


    	case 1:		// Breakpoint
    		er.ExceptionInformation[0] = 0;
    		er.ExceptionCode = STATUS_SINGLE_STEP;
            // If you are using the SetCPUHardware watch function you will probably 
            // want to uncomment the following lines so that it will clear the register
            // automatically on exception
/*    		
    		if(!SetCPUHardwareWatch(0, (DWORD)-1)) {
            	pth->pcstkTop = (PCALLSTACK)(pexi->linkage & ~1);
    			goto continueExecution;
    		}
*/   		
	   		break;

    	case 0:		// Divide by zero
    		er.ExceptionInformation[0] = 0;
    		er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
    		break;

    	case 6:	// Reserved instruction
    		er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
    		break;

		case 16: {
			if (pexi->error & 0x01)
				er.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
			else if (pexi->error & 0x4)
				er.ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
			else if (pexi->error & 0x8)
				er.ExceptionCode = STATUS_FLOAT_OVERFLOW;
			else if (pexi->error & 0x10)
				er.ExceptionCode = STATUS_FLOAT_UNDERFLOW;
			else if (pexi->error & 0x20)
				er.ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
			else
				er.ExceptionCode = STATUS_FLOAT_DENORMAL_OPERAND;
			break;
		}
    	case 0xFF:  // Stack overflow
    	    er.ExceptionCode = STATUS_STACK_OVERFLOW;
    		er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    	    break;
    	    
    	}
    }
	if (id != 1 && id != 3) {
	    NKDbgPrintfW(L"Exception %03x Thread=%8.8lx Proc=%8.8lx '%s'\r\n",
    			id, pth, hCurProc, 
    			pCurProc->lpszProcName ? pCurProc->lpszProcName : L"");
	    NKDbgPrintfW(L"AKY=%8.8lx PC=%8.8lx ESP=%8.8lx EA=%8.8lx\r\n",
    			pCurThread->aky, pctx->Eip, pctx->Esp, info);
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
					RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx Ignored.\r\n"), pctx->Eip));
					DumpFrame(pth, pctx, id, 0);
					if (*(uchar*)pctx->Eip == 0xCC)
					    ++pctx->Eip;
					else if (*(uchar*)pctx->Eip == 0xCD)
					    pctx->Eip += 2;
				} else {
					// Terminate the process.
					RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"),
							er.ExceptionCode));
					DumpFrame(pth, pctx, id, 2);
					if (InSysCall()) {
	    			    OutputDebugStringW(L"Halting system\r\n");
	    			    for (;;)
	    			        ;
	    			} else {
	    				if (!GET_DEAD(pth)) {
	    					SET_DEAD(pth);
		    				pctx->Eip = (ULONG)pExitThread;
							((ulong*)pctx->Esp)[1] = 0;
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
	if (id == 14)
		GuardCommit(info);
continueExecution:
	// If returning from handling a stack overflow, reset the thread's stack overflow
	// flag. It would be good to free the tail of the stack at this time
	// so that the thread will stack fault again if the stack gets too big. But we
	// are currently using that stack page.
	if (id == 0xFF)
    	CLEAR_STACKFAULT(pth);
    if (GET_DYING(pth) && !GET_DEAD(pth) && (pCurProc == pth->pOwnerProc)) {
		SET_DEAD(pth);
		CLEAR_USERBLOCK(pth);
		CLEAR_DEBUGWAIT(pth);
		pctx->Eip = (ULONG)pExitThread;
		((ulong*)pctx->Esp)[1] = 0;
	}   
}

// Capture processor state into a full CONTEXT structure for debugging
// and exception handling.
//
//  (eax) = ptr to EXCINFO structure

Naked CaptureContext(void) {
    __asm {
        push    ss
        push    esp     // will be repaired by ExceptionDispatch
        pushfd
        push    cs
        push    eax     // Eip filled in by ExceptionDispatch
        push    ebp
        push    eax
        push    ecx
        push    edx
        push    ebx
        push    esi
        push    edi
        push    ds
        push    es
        push    fs
        push    gs
        mov     ecx, KGDT_R3_DATA
        mov     ds, cx
        mov     es, cx
        cld
	    sub     esp, size FLOATING_SAVE_AREA
        push    0       // dr7
        push    0       // dr6
        push    0       // dr3
        push    0       // dr2
        push    0       // dr1
        push    0       // dr0
        push    CONTEXT_FULL
        mov     ebx, esp        // (ebx) = ptr to context structure
        push    ebx             // (arg0) = ptr to CONTEXT
        call    ExceptionDispatch

        // Reload processor state from possibly edited 
        lea     esp, [ebx].SegGs
        pop     gs
        pop     fs
        pop     es
        pop     ds
        pop     edi
        pop     esi
        pop     ebx
        pop     edx
        pop     ecx
        mov     ax, cs              // (ax) = current code selector
        xor     eax, [esp+12]       // (ax) = CS ^ target selector
        and     eax, 0000fffcH      // (eax) = selector bits that differ
        jz      short no_ring_switch
        pop     eax
        pop     ebp
        iretd

// Restore frame without a ring switch.  Special care must be taken here because
// we need to restore Esp but an IRETD without a ring switch will not restore
// the SS:ESP.  Since we are returning the same ring, all that needs to be restored
// are Esp and Eip. This is done by copying the new Eip and new Ebp values onto the
// new stack, switching to the new stack and then restoring Ebp and returning.
// The return must be via an IRETD so that single stepping works correctly.
//
// At this point the stack frame is:
//      24  ss
//      20  esp
//      16  flags
//      12  cs
//      08  eip
//      04  ebp
//      00  eax
//      ---------< esp points here

no_ring_switch:
        mov     ebp, [esp+20]       // (ebp) = new stack pointer
        mov     eax, [esp+16]       // (eax) = new EFlags
        mov     [ebp-4], eax        // may overwrite SS value
        mov     eax, [esp+8]        // (eax) = new eip
        mov     [ebp-12], eax       // may overwrite EFlags value
        mov     eax, [esp+4]        // (eax) = new ebp
        mov     [ebp-16], eax       // may overwrite CS value
        mov     [ebp-8], cs
        pop     eax                 // restore Eax
        lea     esp, [ebp-16]       // (esp) = new stack (with Ebp & IRET frame pushed on)
        pop     ebp                 // (ebp) = original Ebp
        iretd                       // continue at the desired address
    }
}

// normal thread stack: from top, TLS then args then free

void MDCreateThread(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, DWORD dwVMBase, BOOL kmode, ulong param) {
    ulong *args;
    NK_PCR *pcr;
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
	// Allocate space for TLS & PCR info.
	pcr = (NK_PCR *)((ulong)lpStack+cbStack - sizeof(NK_PCR));
	// Allocate space for arguments to the start function.
	pTh->ctx.TcxEsp = (ulong)pcr - 3*4;
	pTh->dwStackBound = pTh->ctx.TcxEsp & ~(PAGE_SIZE-1);
	args = (ulong*)pTh->ctx.TcxEsp;
	pcr->ExceptionList = 0;
	pcr->InitialStack = pTh->ctx.TcxEsp;
	pcr->StackLimit = (DWORD)lpStack;
	pTh->tlsPtr = pcr->tls;
	args[1] = (ulong)lpStart;
	args[2] = param;
	args[0] = 0;                        // set return address to fault
	pTh->ctx.TcxEip = (ULONG)lpBase;
	pTh->ctx.TcxDs = KGDT_R3_DATA;
	pTh->ctx.TcxEs = KGDT_R3_DATA;
	pTh->ctx.TcxFs = KGDT_PCR;
	pTh->ctx.TcxGs = 0;
	pTh->ctx.TcxEFlags = 0x3200;        // IOPL=3, IF=1
	if (dwFPType == FPTYPE_HARDWARE) {
        if (ProcessorFeatures & CPUID_FXSR)  {
            memcpy(&pcr->tcxExtended, &g_InitialFPUState, 
                sizeof(pcr->tcxExtended));
        } else {
            memcpy(&pcr->tcxFPU, &g_InitialFPUState, (sizeof(pcr->tcxFPU)+16));
        }
    } else if (dwFPType == FPTYPE_SOFTWARE)
        memset(pcr->Emx87Data, 0, sizeof(pcr->Emx87Data));
	SetThreadMode(pTh, ((kmode || bAllKMode) ? KERNEL_MODE : USER_MODE));
}

// main thread stack: from top, PCR then buf then buf2 then buf2 (ascii) then args then free

LPCWSTR MDCreateMainThread1(PTHREAD pTh, LPVOID lpStack, DWORD cbStack, DWORD dwVMBase,
	LPBYTE buf, ulong buflen, LPBYTE buf2, ulong buflen2) {
    LPBYTE buffer;
	LPCWSTR pcmdline;
	if (!((ulong)lpStack>>VA_SECTION))
		lpStack = (LPVOID)((ulong)(lpStack) + dwVMBase);
	pTh->dwStackBase = (DWORD)lpStack;
    // Allocate space for extra parameters on the stack and copy the bytes there.
    buffer = (LPBYTE)((ulong)lpStack+cbStack-sizeof(NK_PCR) - (buflen+3&~3));
    pcmdline = (LPCWSTR)buffer;
    memcpy(buffer, buf, buflen);
    buffer -= (buflen2+3&~3);
    memcpy(buffer, buf2, buflen2);
	KPlpvTls = pTh->tlsPtr = ((NK_PCR *)(pTh->dwStackBase+cbStack - sizeof(NK_PCR)))->tls;
	pTh->pOwnerProc->lpszProcName = (LPWSTR)buffer;
	return pcmdline;
}

void MDCreateMainThread2(PTHREAD pTh, DWORD cbStack, LPVOID lpBase, LPVOID lpStart, BOOL kmode,
	ulong p1, ulong p2, ulong buflen, ulong buflen2, ulong p4) {
    LPBYTE buffer;
    ulong *args;
    NK_PCR *pcr;
	// Allocate space for TLS & PCR info.
	pcr = (NK_PCR *)(pTh->dwStackBase+cbStack - sizeof(NK_PCR));
	pcr->ExceptionList = 0;
	pcr->InitialStack = pTh->ctx.TcxEsp;
	pcr->StackLimit = pTh->dwStackBase;
	KPlpvTls = pTh->tlsPtr = pcr->tls;
	// Allocate space for arguments to the start function.
    buffer = (LPBYTE)(pTh->dwStackBase+cbStack-sizeof(NK_PCR) - (buflen+3&~3));
    buffer -= (buflen2+3&~3);
	pTh->ctx.TcxEsp = (ulong)buffer - 6*4;
	pTh->dwStackBound = pTh->ctx.TcxEsp & ~(PAGE_SIZE-1);
	args = (ulong*)pTh->ctx.TcxEsp;
	args[1] = (ulong)lpStart;
	args[2] = p1;
	args[3] = p2;
	args[4] = pTh->dwStackBase+cbStack-sizeof(NK_PCR) - (buflen+3&~3);
	args[5] = p4;
	args[0] = 0;                        // set return address to fault
	pTh->ctx.TcxEip = (ULONG)lpBase;
	pTh->ctx.TcxDs = KGDT_R3_DATA;
	pTh->ctx.TcxEs = KGDT_R3_DATA;
	pTh->ctx.TcxFs = KGDT_PCR;
	pTh->ctx.TcxGs = 0;
	pTh->ctx.TcxEFlags = 0x3200;        // IOPL=3, IF=1
	if (dwFPType == FPTYPE_HARDWARE)  {
        if (ProcessorFeatures & CPUID_FXSR)  {
            memcpy(&pcr->tcxExtended, &g_InitialFPUState, 
                sizeof(pcr->tcxExtended));
        } else {
            memcpy(&pcr->tcxFPU, &g_InitialFPUState, (sizeof(pcr->tcxFPU)+16));
        }
    } else if (dwFPType == FPTYPE_SOFTWARE)
        memset(pcr->Emx87Data, 0, sizeof(pcr->Emx87Data));
	SetThreadMode(pTh, ((kmode || bAllKMode) ? KERNEL_MODE : USER_MODE));
}

void ZeroPage(void *pvPage) {
    _asm {
        mov edi, pvPage
        mov ecx, PAGE_SIZE/4
        xor eax, eax
        rep stosd
    }
}

extern void FPUFlushContext(void);
 
BOOL DoThreadGetContext(HANDLE hTh, LPCONTEXT lpContext) {
	PTHREAD pth;
	PFXSAVE_AREA FxArea;
	PFLOATING_SAVE_AREA FnArea;
    ACCESSKEY ulOldKey;

	if (!(pth = HandleToThread(hTh))) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (lpContext->ContextFlags & ~(CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
   	SWITCHKEY(ulOldKey,0xffffffff);
	if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
		if ((lpContext->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
			lpContext->Ebp = pth->pThrdDbg->psavedctx->Ebp;
			lpContext->Eip = pth->pThrdDbg->psavedctx->Eip;
			lpContext->SegCs = pth->pThrdDbg->psavedctx->SegCs;
			lpContext->EFlags = pth->pThrdDbg->psavedctx->EFlags;
			lpContext->Esp = pth->pThrdDbg->psavedctx->Esp;
			lpContext->SegSs = pth->pThrdDbg->psavedctx->SegSs;
		}
		if ((lpContext->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
			lpContext->Edi = pth->pThrdDbg->psavedctx->Edi;
			lpContext->Esi = pth->pThrdDbg->psavedctx->Esi;
			lpContext->Ebx = pth->pThrdDbg->psavedctx->Ebx;
			lpContext->Edx = pth->pThrdDbg->psavedctx->Edx;
			lpContext->Ecx = pth->pThrdDbg->psavedctx->Ecx;
			lpContext->Eax = pth->pThrdDbg->psavedctx->Eax;
		}
		if ((lpContext->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {
			lpContext->SegGs = pth->pThrdDbg->psavedctx->SegGs;
			lpContext->SegFs = pth->pThrdDbg->psavedctx->SegFs;
			lpContext->SegEs = pth->pThrdDbg->psavedctx->SegEs;
			lpContext->SegDs = pth->pThrdDbg->psavedctx->SegDs;
		}
		if ((lpContext->ContextFlags & CONTEXT_FLOATING_POINT) == 
            CONTEXT_FLOATING_POINT) {
			FPUFlushContext();
			lpContext->FloatSave = pth->pThrdDbg->psavedctx->FloatSave;
		}
		if ((lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) == 
            CONTEXT_DEBUG_REGISTERS) {

		}
	} else {
		if ((lpContext->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
			lpContext->Ebp = pth->ctx.TcxEbp;
			lpContext->Eip = pth->ctx.TcxEip;
			lpContext->SegCs = pth->ctx.TcxCs;
			lpContext->EFlags = pth->ctx.TcxEFlags;
			lpContext->Esp = pth->ctx.TcxEsp;
			lpContext->SegSs = pth->ctx.TcxSs;
		}
		if ((lpContext->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
			lpContext->Edi = pth->ctx.TcxEdi;
			lpContext->Esi = pth->ctx.TcxEsi;
			lpContext->Ebx = pth->ctx.TcxEbx;
			lpContext->Edx = pth->ctx.TcxEdx;
			lpContext->Ecx = pth->ctx.TcxEcx;
			lpContext->Eax = pth->ctx.TcxEax;
		}
		if ((lpContext->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {
			lpContext->SegGs = pth->ctx.TcxGs;
			lpContext->SegFs = pth->ctx.TcxFs;
			lpContext->SegEs = pth->ctx.TcxEs;
			lpContext->SegDs = pth->ctx.TcxDs;
		}
		if ((lpContext->ContextFlags & CONTEXT_FLOATING_POINT) == 
            CONTEXT_FLOATING_POINT) {
            if (ProcessorFeatures & CPUID_FXSR)  {
                FxArea = (PFXSAVE_AREA) PTH_TO_FLTSAVEAREAPTR(pth);
                FnArea = &lpContext->FloatSave;
                __asm {
                    // We won't get here if emulating FP, so CR0.EM will be 0
                    call    FPUFlushContext     // FPUFlushContext sets CR0.TS
                    clts 
                    mov     eax, FxArea
                    FXRESTOR_EAX                // convert from fxsave format
                    mov     eax, FnArea         // in NK_PCR to fnsave format
                    fnsave  [eax]               // in CONTEXT structure
                    fwait
                    mov     eax, cr0 
                    or      eax, TS_MASK
                    mov     cr0, eax
                }
            } else {
    			
                FPUFlushContext();
                lpContext->FloatSave = *(PTH_TO_FLTSAVEAREAPTR(pth));
            }
        }
		if ((lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) == 
            CONTEXT_DEBUG_REGISTERS) {

		}
	}
	SETCURKEY(ulOldKey);
	return TRUE;
}

BOOL DoThreadSetContext(HANDLE hTh, const CONTEXT *lpContext) {
	PTHREAD pth;
	PFXSAVE_AREA FxArea;
	DWORD i, j, TagWord;
    ACCESSKEY ulOldKey;
	if (!(pth = HandleToThread(hTh))) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (lpContext->ContextFlags & ~(CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
   	SWITCHKEY(ulOldKey,0xffffffff);
	if (pth->pThrdDbg && pth->pThrdDbg->psavedctx) {
		if ((lpContext->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
			pth->pThrdDbg->psavedctx->Ebp = lpContext->Ebp;
			pth->pThrdDbg->psavedctx->Eip = lpContext->Eip;
			pth->pThrdDbg->psavedctx->SegCs = lpContext->SegCs;
			pth->pThrdDbg->psavedctx->EFlags = (pth->pThrdDbg->psavedctx->EFlags & 0xfffff200) |
								 (lpContext->EFlags& 0x00000dff);
			pth->pThrdDbg->psavedctx->Esp = lpContext->Esp;
			pth->pThrdDbg->psavedctx->SegSs = lpContext->SegSs;
		}
		if ((lpContext->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
			pth->pThrdDbg->psavedctx->Edi = lpContext->Edi;
			pth->pThrdDbg->psavedctx->Esi = lpContext->Esi;
			pth->pThrdDbg->psavedctx->Ebx = lpContext->Ebx;
			pth->pThrdDbg->psavedctx->Edx = lpContext->Edx;
			pth->pThrdDbg->psavedctx->Ecx = lpContext->Ecx;
			pth->pThrdDbg->psavedctx->Eax = lpContext->Eax;
		}
		if ((lpContext->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {
			pth->pThrdDbg->psavedctx->SegGs = lpContext->SegGs;
			pth->pThrdDbg->psavedctx->SegFs = lpContext->SegFs;
			pth->pThrdDbg->psavedctx->SegEs = lpContext->SegEs;
			pth->pThrdDbg->psavedctx->SegDs = lpContext->SegDs;
		}
		if ((lpContext->ContextFlags & CONTEXT_FLOATING_POINT) == 
            CONTEXT_FLOATING_POINT) {
			FPUFlushContext();
			pth->pThrdDbg->psavedctx->FloatSave = lpContext->FloatSave;
			
		}
		if ((lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) == 
            CONTEXT_DEBUG_REGISTERS) {

		}
	} else {
		if ((lpContext->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
			pth->ctx.TcxEbp = lpContext->Ebp;
			pth->ctx.TcxEip = lpContext->Eip;
			pth->ctx.TcxCs = lpContext->SegCs;
			pth->ctx.TcxEFlags = (pth->ctx.TcxEFlags & 0xfffff200) |
								 (lpContext->EFlags& 0x00000dff);
			pth->ctx.TcxEsp = lpContext->Esp;
			pth->ctx.TcxSs = lpContext->SegSs;
		}
		if ((lpContext->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
			pth->ctx.TcxEdi = lpContext->Edi;
			pth->ctx.TcxEsi = lpContext->Esi;
			pth->ctx.TcxEbx = lpContext->Ebx;
			pth->ctx.TcxEdx = lpContext->Edx;
			pth->ctx.TcxEcx = lpContext->Ecx;
			pth->ctx.TcxEax = lpContext->Eax;
		}
		if ((lpContext->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) {
			pth->ctx.TcxGs = lpContext->SegGs;
			pth->ctx.TcxFs = lpContext->SegFs;
			pth->ctx.TcxEs = lpContext->SegEs;
			pth->ctx.TcxDs = lpContext->SegDs;
		}
		if ((lpContext->ContextFlags & CONTEXT_FLOATING_POINT) == 
            CONTEXT_FLOATING_POINT)  {
			
			FPUFlushContext();
            if  (ProcessorFeatures & CPUID_FXSR)  {
                // Convert from fnsave format in CONTEXT to fxsave format in PCR
                FxArea = (PFXSAVE_AREA) PTH_TO_FLTSAVEAREAPTR(pth);
                FxArea->ControlWord = (USHORT) lpContext->FloatSave.ControlWord;
                FxArea->StatusWord = (USHORT) lpContext->FloatSave.StatusWord;
                FxArea->TagWord = 0;
                TagWord = lpContext->FloatSave.TagWord;
                for (i = 0; i < FN_BITS_PER_TAGWORD; i+=2) {
                    if (((TagWord >> i) & FN_TAG_MASK) != FN_TAG_EMPTY) 
                        FxArea->TagWord |= (FX_TAG_VALID << (i/2));
                }
                FxArea->ErrorOffset = lpContext->FloatSave.ErrorOffset;
                FxArea->ErrorSelector = lpContext->FloatSave.ErrorSelector & 0xffff;
                FxArea->ErrorOpcode = (USHORT)
                    ((lpContext->FloatSave.ErrorSelector >> 16) & 0xFFFF);
                FxArea->DataOffset = lpContext->FloatSave.DataOffset;
                FxArea->DataSelector = lpContext->FloatSave.DataSelector;
                memset(&FxArea->RegisterArea[0], 0, SIZE_OF_FX_REGISTERS);
                for (i = 0; i < NUMBER_OF_FP_REGISTERS; i++) {
                    for (j = 0; j < BYTES_PER_FP_REGISTER; j++) {
                        FxArea->RegisterArea[i*BYTES_PER_FX_REGISTER+j] =
                        lpContext->FloatSave.RegisterArea[i*BYTES_PER_FP_REGISTER+j];
                    }
                }
			} else {
			    *(PTH_TO_FLTSAVEAREAPTR(pth)) = lpContext->FloatSave;
			}
		}
		if ((lpContext->ContextFlags & CONTEXT_DEBUG_REGISTERS) == 
            CONTEXT_DEBUG_REGISTERS) {

		}
	}
	SETCURKEY(ulOldKey);
	return TRUE;
}

//
//   ExecuteHandler is the common tail for RtlpExecuteHandlerForException
//   and RtlpExecuteHandlerForUnwind.
//
//   (edx) = handler (Exception or Unwind) address
//

///ExceptionRecord     equ [ebp+8]
///EstablisherFrame    equ [ebp+12]
///ContextRecord       equ [ebp+16]
///DispatcherContext   equ [ebp+20]
///ExceptionRoutine    equ [ebp+24]

#pragma warning(disable:4035)

EXCEPTION_DISPOSITION __declspec(naked) ExecuteHandler(
IN PEXCEPTION_RECORD ExceptionRecord,
IN PVOID EstablisherFrame,
IN OUT PCONTEXT ContextRecord,
IN OUT PDISPATCHER_CONTEXT DispatcherContext,
IN PEXCEPTION_ROUTINE ExceptionRoutine,
IN ULONG ExceptionMode) {
    __asm {
        push    ebp
        mov     ebp, esp
        push    EstablisherFrame        // Save context of exception handler
                                        // that we're about to call.
        push    edx                     // Set Handler address
        push    dword ptr fs:[0]        // Set next pointer
        mov     fs:[0], esp             // Link us on

// Call the specified exception handler.

        push    DispatcherContext
        push    ContextRecord
        push    EstablisherFrame
        push    ExceptionRecord

        call    [ExceptionRoutine]
        mov     esp, fs:[0]

// Don't clean stack here, code in front of ret will restore initial state

// Disposition is in eax, so all we do is deregister handler and return

        pop     dword ptr fs:[0]
        mov     esp, ebp
        pop     ebp
        ret
    }
}

//++
//
// EXCEPTION_DISPOSITION
// ExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer and handler address from
//    its establisher's call frame, store this information in the dispatcher
//    context record, and return a disposition value of nested exception.
//
// Arguments:
//
//    ExceptionRecord (exp+4) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (esp+8) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (esp+12) - Supplies a pointer to a context record.
//
//    DispatcherContext (esp+16) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

Naked ExceptionHandler(void) {
    __asm {
        mov     ecx, dword ptr [esp+4]          // (ecx) -> ExceptionRecord
        test    dword ptr [ecx.ExceptionFlags], EXCEPTION_UNWINDING
        mov     eax, ExceptionContinueSearch    // Assume unwind
        jnz     eh10                            // unwind, go return

// Unwind is not in progress - return nested exception disposition.
        mov     ecx,[esp+8]             // (ecx) -> EstablisherFrame
        mov     edx,[esp+16]            // (edx) -> DispatcherContext
        mov     eax,[ecx+8]             // (eax) -> EstablisherFrame for the
                                        //          handler active when we
                                        //          nested.
        mov     [edx], eax              // Set DispatcherContext field.
        mov     eax, ExceptionNestedException
eh10:   ret
    }
}

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine,
//    IN BOOL ExceptionMode
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the handler address and
//    establisher frame pointer in the frame, establishes an exception
//    handler, and then calls the specified exception handler as an exception
//    handler. If a nested exception occurs, then the exception handler of
//    of this function is called and the handler address and establisher
//    frame pointer are returned to the exception dispatcher via the dispatcher
//    context parameter. If control is returned to this routine, then the
//    frame is deallocated and the disposition status is returned to the
//    exception dispatcher.
//
// Arguments:
//
//    ExceptionRecord (ebp+8) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (ebp+12) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (ebp+16) - Supplies a pointer to a context record.
//
//    DispatcherContext (ebp+20) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (ebp+24) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

EXCEPTION_DISPOSITION __declspec(naked) RtlpExecuteHandlerForException(
IN PEXCEPTION_RECORD ExceptionRecord,
IN PVOID EstablisherFrame,
IN OUT PCONTEXT ContextRecord,
IN OUT PDISPATCHER_CONTEXT DispatcherContext,
IN PEXCEPTION_ROUTINE ExceptionRoutine,
IN ULONG ExceptionMode) {
    __asm {
        mov     edx,offset ExceptionHandler     // Set who to register
        jmp     ExecuteHandler                  // jump to common code
    }
}

//++
// EXCEPTION_DISPOSITION
// UnwindHandler(
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext)
//
// Routine Description:
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher frame pointer and handler address from
//    its establisher's call frame, store this information in the dispatcher
//    context record, and return a disposition value of nested unwind.
//
// Arguments:
//    ExceptionRecord (esp+4) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (esp+8) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (esp+12) - Supplies a pointer to a context record.
//
//    DispatcherContext (esp+16) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise a value of ExceptionContinueSearch is returned.
//--

Naked UnwindHandler(void) {
    __asm {
        mov     ecx,dword ptr [esp+4]           // (ecx) -> ExceptionRecord
        test    dword ptr [ecx.ExceptionFlags], EXCEPTION_UNWINDING
        mov     eax,ExceptionContinueSearch     // Assume NOT unwind
        jz      uh10                            // not unwind, go return

// Unwind is in progress - return collided unwind disposition.
        mov     ecx,[esp+8]             // (ecx) -> EstablisherFrame
        mov     edx,[esp+16]            // (edx) -> DispatcherContext
        mov     eax,[ecx+8]             // (eax) -> EstablisherFrame for the
                                        //          handler active when we
                                        //          nested.
        mov     [edx],eax               // Set DispatcherContext field.
        mov     eax,ExceptionCollidedUnwind
uh10:   ret
    }
}

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    IN BOOL ExceptionMode
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the handler address and
//    establisher frame pointer in the frame, establishes an exception
//    handler, and then calls the specified exception handler as an unwind
//    handler. If a collided unwind occurs, then the exception handler of
//    of this function is called and the handler address and establisher
//    frame pointer are returned to the unwind dispatcher via the dispatcher
//    context parameter. If control is returned to this routine, then the
//    frame is deallocated and the disposition status is returned to the
//    unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (ebp+8) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (ebp+12) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (ebp+16) - Supplies a pointer to a context record.
//
//    DispatcherContext (ebp+20) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (ebp+24) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

EXCEPTION_DISPOSITION __declspec(naked) RtlpExecuteHandlerForUnwind(
IN PEXCEPTION_RECORD ExceptionRecord,
IN PVOID EstablisherFrame,
IN OUT PCONTEXT ContextRecord,
IN OUT PDISPATCHER_CONTEXT DispatcherContext,
IN PEXCEPTION_ROUTINE ExceptionRoutine,
IN ULONG ExceptionMode) {
    __asm {
        mov     edx,offset UnwindHandler
        jmp     ExecuteHandler                      // jump to common code
    }
}

#pragma warning(default:4035)

