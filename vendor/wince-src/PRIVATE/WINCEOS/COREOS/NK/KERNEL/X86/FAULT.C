/* Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved. */

/*+
    fault.c - iX86 fault handlers
 */
#include "kernel.h"

// disable short jump warning.
#pragma warning(disable:4414)

///#define LIGHTS(n)   mov dword ptr ss:[0AA001010h], ~(n)&0xFF

extern RETADDR ServerCallReturn(PTHREAD pth);
extern RETADDR ObjectCall(PTHREAD pth, RETADDR ra, void *args, long iMethod);
extern RETADDR MapArgs(const CINFO *pci, int iMethod, void *args);
extern BOOL HandleException(PTHREAD pth, int id, ulong addr);
extern void NextThread(void);
extern void KCNextThread(void);
extern void OEMIdle(void);
extern void OEMFlushCache(void);
extern KTSS MainTSS;
extern void Reschedule(void);
extern void RunThread(void);
extern void DumpTctx(PTHREAD pth, int id, ulong addr, int level);
extern void DoPowerOff(void);
extern unsigned __int64 g_aGlobalDescriptorTable[];
extern DWORD ticksleft;
#ifdef NKPROF
extern void ProfilerHit(unsigned long ra);
#endif
#ifdef CELOG
extern void CeLogInterrupt(DWORD dwLogValue);
#endif


#define LOAD_SEGS    0

#define ADDR_SLOT_SIZE  0x02000000
#define PDES_PER_SLOT   (ADDR_SLOT_SIZE / 1024 / PAGE_SIZE)

#define NUM_SLOTS        32
#define PID_TO_PT_INDEX(pid) ((pid+1) * PDES_PER_SLOT)


#define BLOCKS_PER_PAGE_TABLE (1024 / PAGES_PER_BLOCK)
#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE)

#define SYSCALL_INT			0x20
#define KCALL_INT           0x22

#define PT_PTR_TO_INDEX(pp) ((pp) - g_PageTablePool)

typedef struct _DIRTYRANGE {
    ULONG ulStartBlock;
    ULONG ulEndBlock;
} DIRTYRANGE, * PDIRTYRANGE;

DIRTYRANGE g_PTDirtyRegion[PTE_POOL_SIZE];

extern PAGETABLE g_PageTablePool[PTE_POOL_SIZE];
extern PAGETABLE g_PageDir;
extern PAGETABLE g_ShadowPageDir;       // NOTE:  This really only has 512 entries!
extern DWORD ProcessorFeatures;
ACCESSKEY g_PageDirAccess = 0;
ULONG g_PTMapIdx[PTE_POOL_SIZE];
ULONG g_AccessScanIdx = 0;

FXSAVE_AREA g_InitialFPUState;
PTHREAD g_CurFPUOwner;
PPROCESS g_CurASIDProc;

//
//  CR0 bit definitions for numeric coprocessor
//
#define MP_MASK     0x00000002
#define EM_MASK     0x00000004
#define TS_MASK     0x00000008
#define NE_MASK     0x00000020

#define NPX_CW_PRECISION_MASK	0x300
#define NPX_CW_PRECISION_24		0x000
#define NPX_CW_PRECISION_53		0x200
#define NPX_CW_PRECISION_64		0x300


PPAGETABLE  LowAddrSpacePageFault(PVOID, DWORD);
PPAGETABLE  AllocFreePTE(ULONG);

#ifdef DEBUG_VMEM
void        ValidateVirtualMemory();
#endif



#define hCurThd  [KData].ahSys[SH_CURTHREAD*4]
#define PtrCurThd  [KData].pCurThd

#define THREAD_CTX_ES  (THREAD_CONTEXT_OFFSET+8)
ERRFALSE(8 == offsetof(CPUCONTEXT, TcxEs));
#define THREAD_CTX_EDI  (THREAD_CONTEXT_OFFSET+16)
ERRFALSE(16 == offsetof(CPUCONTEXT, TcxEdi));

#define Naked void __declspec(naked)



#pragma warning(disable:4035)               // Disable warning about no return value

DWORD _inline PhysTLBFlush(void)
{
    __asm {
        mov     eax, cr3
        mov     cr3, eax
    }
}

void FlushCache(void) {
	OEMFlushCache();
}

#pragma warning(default:4035)               // Turn warning back on

void SetCPUASID(PTHREAD pth)
{
    PPROCESS pprc;
    DWORD aky;

    // Make sure this runs without preemption
    if (!InSysCall()) {
        KCall((PKFN)SetCPUASID, pth);
        return;
    } else
    	KCALLPROFON(62);
    pprc = pth->pProc;
    if (g_CurASIDProc != pprc)
    {
        ACCESSKEY akyReset;
        PDWORD pAliasPD = &g_PageDir.PTE[0];
        PDWORD pRealPD = &g_PageDir.PTE[PID_TO_PT_INDEX(pprc->procnum)];
        PDWORD pShadowOld = &g_ShadowPageDir.PTE[PID_TO_PT_INDEX(pCurProc->procnum)];
        PDWORD pShadowNew = &g_ShadowPageDir.PTE[PID_TO_PT_INDEX(pprc->procnum)];

        CELOG_ThreadMigrate(pprc->hProc, 0);

#ifdef DEBUG_VMEM
        ValidateVirtualMemory();
#endif
        //
        // Copy the accessed bits out for the current slot 0 process, and copy
        // in the new process' page tables
        //
        do {
            // Since page dir entries are either mirrors of the shadow with the
            // exception of the access bit, or they are 0, this simple OR will
            // only update the accessed bit in the shadow.  It is faster to
            // simply do the or than to test the accessed bit for each entry in
            // the real page dir
#ifdef DEBUG_VMEM
            if (g_CurASIDProc != 0)
            {
                if ((*pShadowOld & ~PG_ACCESSED_MASK) !=
                    (*pAliasPD & ~PG_ACCESSED_MASK))
                {
                    NKDbgPrintfW(
                        TEXT("SetCPUASID: Slot 0 PDE doesn't match Process slot PDE\r\n"));
                    NKDbgPrintfW(
                        TEXT("Slot index = %d, PDE index = %d\r\n"),
                        pprc->procnum+1, pAliasPD - &g_PageDir.PTE[0]);
                    NKDbgPrintfW(
                        TEXT("Slot 0 PDE = 0x%8.8X, Process Slot PDE = 0x%8.8X\r\n"),
                        *pAliasPD & ~PG_ACCESSED_MASK, *pShadowOld & ~PG_ACCESSED_MASK);
                    DebugBreak();
                }
            }
#endif
            *pShadowOld++ |= *pAliasPD;
            *pRealPD++ = *pShadowNew;
            *pAliasPD++ = *pShadowNew++;
        } while (pAliasPD < &g_PageDir.PTE[PDES_PER_SLOT]);

		AddAccess(&g_PageDirAccess,pprc->aky);

        //  Unmap any sections that the new thread is not allowed to access.
        aky = pth->aky | pprc->aky;
        akyReset = g_PageDirAccess & (~aky);
        if (akyReset)
        {
            UINT i;
            g_PageDirAccess &= aky;
            for (i = 0; i < ARRAY_SIZE(g_PageTablePool); i++)
            {
                UINT j = g_PTMapIdx[i];

#ifdef DEBUG_VMEM
                if (j < PDES_PER_SLOT)
                {
                    NKDbgPrintfW(
                        TEXT("SetCPUASID: PageTablePool has PDE owned by slot 0\r\n"));
                    NKDbgPrintfW(
                        TEXT("Pool index = %d, PDE index = %d\r\n"),
                        i, j);
                    DebugBreak();
                }
#endif
                if (akyReset & (1 << ((j / PDES_PER_SLOT) - 1))) {
                    DWORD dwTest = g_PageDir.PTE[j];
#ifdef DEBUG_VMEM
                    if (dwTest && ((dwTest&PG_PHYS_ADDR_MASK) == (g_PageDir.PTE[j%PDES_PER_SLOT] & PG_PHYS_ADDR_MASK))) {
                        NKDbgPrintfW(L"SetCPUASID: Zapping slot0 entry.  PDE=%8.8x j=%4.4x\r\n", dwTest, j);
                        NKDbgPrintfW(L"  akyReset=%8.8x g_PageDirAccess=%8.8x new key=%8.8x\r\n", akyReset, g_PageDirAccess, pth->aky);
                        NKDbgPrintfW(L"  pCurProc=%8.8lx, pprc=%8.8lx, pth = %8.8lx, &g_PageDir=%8.8lx, &g_ShadowPageDir=%8.8lx\r\n",
                        	pCurProc,pprc,pth,&g_PageDir,&g_ShadowPageDir);
                        DebugBreak();
                    }
#endif
                    g_PageDir.PTE[j] = 0;
                    if (dwTest & PG_ACCESSED_MASK)
                    {
#ifdef DEBUG_VMEM
                        if ((g_ShadowPageDir.PTE[j] & ~PG_ACCESSED_MASK) !=
                            (dwTest & ~PG_ACCESSED_MASK))
                        {
                            NKDbgPrintfW(
                                TEXT("SetCPUASID: Real PDE doesn't match Shadow PDE\r\n"));
                            NKDbgPrintfW(
                                TEXT("Pool index = %d, PDE index = %d\r\n"),
                                i, j);
                            NKDbgPrintfW(
                                TEXT("Real PDE = 0x%8.8X, Shadow PDE = 0x%8.8X\r\n"),
                                dwTest & ~PG_ACCESSED_MASK, g_ShadowPageDir.PTE[j] & ~PG_ACCESSED_MASK);
                            DebugBreak();
                        }
#endif
                        g_ShadowPageDir.PTE[j] = dwTest;
                    }
                } else if (aky & (1 << ((j / PDES_PER_SLOT) - 1))) {
					g_ShadowPageDir.PTE[j] |= g_PageDir.PTE[j];
					g_PageDir.PTE[j] = g_ShadowPageDir.PTE[j];
					AddAccess(&g_PageDirAccess,(1 << ((j / PDES_PER_SLOT) - 1)));
                }
            }
        }

        SectionTable[0] = SectionTable[(ULONG)pprc->dwVMBase >> VA_SECTION];

        g_CurASIDProc = pprc;

        PhysTLBFlush();
    }

    //
    //  Change CurProc to the new guy
    //
    pCurProc = pprc;
    hCurProc = pprc->hProc;
   	KCALLPROFOFF(62);
}



LPVOID VerifyAccess(LPVOID pvAddr, DWORD dwFlags, ACCESSKEY aky)
{
    PSECTION pscn;
    MEMBLOCK *pmb;
    ulong entry;

    if ((long)pvAddr >= 0) {
        if ((pscn = SectionTable[(ulong)pvAddr>>VA_SECTION]) != 0
                && (pmb = (*pscn)[((ulong)pvAddr>>VA_BLOCK)&BLOCK_MASK]) != 0
                && pmb != RESERVED_BLOCK
                && (pmb->alk & aky) != 0
                && (entry = pmb->aPages[((ulong)pvAddr>>VA_PAGE)&PAGE_MASK]) & PG_VALID_MASK
                && (!(dwFlags & VERIFY_WRITE_FLAG)
                || (entry&PG_PROTECTION) == PG_PROT_WRITE))
            return Phys2Virt(PFNfromEntry(entry) | ((ulong)pvAddr & (PAGE_SIZE-1)));
    } else {
        // Kernel mode only address. If the "kernel mode OK" flag is set or if the
        // thread is running in kernel mode, allow the access.
        if (dwFlags & VERIFY_KERNEL_OK || GetThreadMode(pCurThread) == KERNEL_MODE) {
            DWORD       dwPageDir;
            PPAGETABLE  pPageTable;

            //
            // Find entry in 1st level page dir
            //
            if ((dwPageDir = g_PageDir.PTE[((ulong)pvAddr) >> 22]) != 0) {
		if ((dwPageDir & (PG_LARGE_PAGE_MASK|PG_VALID_MASK)) == (PG_LARGE_PAGE_MASK|PG_VALID_MASK)) {
		    return pvAddr;
		} else {
		    pPageTable = (PPAGETABLE)PHYS_TO_LIN(dwPageDir & PG_PHYS_ADDR_MASK);
		    if (pPageTable->PTE[((ulong)pvAddr>>VA_PAGE)&0x3FF] & PG_VALID_MASK)
			return pvAddr;
		}
            }
        }
    }
    return 0;
}

//
// Function:
//  CommonFault
//
// Description:
//  CommonFault is jumped to by the specific fault handlers for unhandled
//  exceptions which are then dispatched to the C routine HandleException.
//
// At entry:
//  ESP     points to stack frame containing PUSHAD, ERROR CODE, EIP, CS,
//          EFLAGS, (and optionally Old ESP, Old SS).  Normally this is the
//          last part of the thread structure, the saved context.  In the case
//          of a nested exception the context has been saved on the ring 0
//          stack.  We will create a fake thread structure on the stack to hold
//          the captured context.  The remaining segment registers are added by
//          this routine.
//
//  ECX     is the faulting address which is passed to HandleException
//
//  ESI     is the exception id which is passed to HandleException
//
//  Return:
//   CommonFault jumps to Reschedule or resumes execution based on the return
//   value of HandleException.
//

Naked CommonFault()
{
    _asm {
        cld
        mov     eax, KGDT_R3_DATA
        mov     ds, ax
        mov     es, ax
        dec     [KData].cNest
        jnz     short cf20          // nested fault
        mov     esp, offset KData-4
        mov     edi, PtrCurThd
cf10:   sti
        push    ecx
        push    esi
        push    edi
        call    HandleException
        add     esp, 3*4
        test    eax, eax
        jnz		short NoReschedule
        jmp		Reschedule
NoReschedule:
        jmp     RunThread

// Nested exception. Create a fake thread structure on the stack
cf20:   push    ds
        push    es
        push    fs
        push    gs
        sub     esp, THREAD_CONTEXT_OFFSET
        mov     edi, esp            // (edi) = ptr to fake thread struct
        jmp     short cf10
    }
}

// Do a reschedule.
//
//  (edi) = ptr to current thread or 0 to force a context reload

Naked Reschedule()
{
	__asm {
        test    [KData].bPowerOff, 0FFh    // Was a PowerOff requested?
        jz      short rsd10
        mov     [KData].bPowerOff, 0
        call    DoPowerOff              // Yes - do it
rsd10:
        sti
		cmp 	word ptr ([KData].bResched), 1
		jne 	short rsd11
        mov     word ptr ([KData].bResched), 0
        call    NextThread
rsd11:
		cmp		dword ptr ([KData].dwKCRes), 1
		jne		short rsd12
		mov		dword ptr ([KData].dwKCRes), 0
		call	KCNextThread

		cmp		dword ptr ([KData].dwKCRes), 1
		je		short rsd10

rsd12:
		mov		eax, [RunList.pth]
        test    eax, eax
        jz      short rsd50           // nothing to run
        cmp     eax, edi
        jne		short rsd20
        jmp		RunThread			// redispatch the same thread

// Switch to a new thread's process context.
// Switching to a new thread. Update current process and address space
// information. Edit the ring0 stack pointer in the TSS to point to the
// new thread's register save area.
//
//      (eax) = ptr to thread structure

rsd20:  mov     edi, eax                // Save thread pointer
        mov     esi, (THREAD)[eax].hTh          // (esi) = thread handle
        push    edi
        call    SetCPUASID              // Sets hCurProc for us!
        pop     ecx                     // Clean up stack

        mov     hCurThd, esi            // set the current thread handle
        mov     PtrCurThd, edi          //   and the current thread pointer
        mov     ecx, [edi].tlsPtr       // (ecx) = thread local storage ptr
        mov     [KData].lpvTls, ecx     // set TLS pointer

        cmp     edi, g_CurFPUOwner
        jne     SetTSBit
        clts
        jmp     MuckWithFSBase

SetTSBit:
        mov     eax, CR0
        test    eax, TS_MASK
        jnz     MuckWithFSBase
        or      eax, TS_MASK
        mov     CR0, eax

MuckWithFSBase:
        mov     edx, offset g_aGlobalDescriptorTable+KGDT_PCR
        sub     ecx, FS_LIMIT+1         // (ecx) = ptr to NK_PCR base
        mov     word ptr [edx+2], cx    // set low word of FS base
        shr     ecx, 16
        mov     byte ptr [edx+4], cl    // set third byte of FS base
        mov     byte ptr [edx+7], ch    // set high byte of FS base

        lea     ecx, [edi].ctx.TcxSs+4  // (ecx) = ptr to end of context save area
        mov     [MainTSS].Esp0, ecx
        jmp     RunThread               // Run thread pointed to by edi

// No threads ready to run. Call OEMIdle to shutdown the cpu.

rsd50:  cli

        cmp     ticksleft, 0
        je		short skip_force_reched
        mov		byte ptr ([KData].bResched), 1

skip_force_reched:
        cmp     word ptr ([KData].bResched), 1
        je		short DoReschedule
        call    OEMIdle
        mov		byte ptr ([KData].bResched), 1
        jmp     Reschedule
DoReschedule:
        sti
        jmp		Reschedule
    }
}

Naked CommonIntDispatch()
{
    _asm {
        cld
        mov     eax, KGDT_R3_DATA
        mov     ds, ax
        mov     es, ax
        dec     [KData].cNest
        jnz     short cid20         // nested fault
        mov     esp, offset KData-4
        mov     edi, PtrCurThd
cid10:
#ifdef CELOG
		push	esi					// save ESI
		mov		eax, 80000000h		// mark as ISR entry
        push    eax                 // Arg 0, cNest + SYSINTR_xxx
        call    CeLogInterrupt
        pop     eax                 // cleanup the stack from the call
		pop		esi					// restore ESI
#endif

		sti

        call    esi

		cli

#ifdef CELOG
        push    eax                 // Save original SYSINTR return value.
        bswap   eax                 // Reverse endian
        mov     ah, [KData].cNest   // Nesting level (0 = no nesting, -1 = nested once)
        neg     ah                  // Nesting level (0 = no nesting,  1 = nested once)
        bswap   eax                 // Reverse endian
        push    eax                 // Arg 0, cNest + SYSINTR_xxx
        call    CeLogInterrupt
        pop     eax                 // cleanup the stack from the call
        pop     eax                 // restore original SYSINTR value
#endif
        test    eax, eax
        jz      short cid17         // SYSINTR_NOP: nothing more to do

#ifdef NKPROF
        cmp     eax, SYSINTR_PROFILE
        jne     short cid13
        call    ProfilerHit
        jmp     cid17               // Continue on our merry way...
cid13:
#endif

        cmp     eax, SYSINTR_RESCHED
        je      short cid15
        lea     ecx, [eax-SYSINTR_DEVICES]
        cmp     ecx, SYSINTR_MAX_DEVICES
        jae     short cid15         // force a reschedule for good measure

// A device interrupt has been signaled. Set the appropriate bit in the pending
// events mask and set the reschedule flag. The device event will be signaled
// by the scheduler.

        mov     eax, 1
        shl     eax, cl
        or      [KData].aInfo[KINX_PENDEVENTS*4], eax
cid15:  or      [KData].bResched, 1 // must reschedule
cid17:  jmp     RunThread

// Nested exception. Create a fake thread structure on the stack
cid20:  push    ds
        push    es
        push    fs
        push    gs
        sub     esp, THREAD_CONTEXT_OFFSET
        mov     edi, esp            // (edi) = ptr to fake thread struct
        jmp     short cid10
    }
}


// Continue thread execution.
//
//  (edi) = ptr to Thread structure

Naked RunThread()
{
    _asm {
        cli
        cmp     word ptr ([KData].bResched), 1
        jne	short NotReschedule
		jmp		Reschedule
NotReschedule:
        inc     [KData].cNest
        lea     esp, [edi].ctx.TcxGs
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
        add     esp, 4
        iretd
        cli
        hlt
    }
}



Naked PageFault()
{
    _asm {
        pushad
        mov     ebx, OFFSET KData
        mov     edi, cr2
        test    edi, edi
        js      short pf50              // Address > 2GB, get out now
        dec     [ebx].cNest             // count kernel reentrancy level
        mov     esi, esp                // (esi) = original stack pointer
        jnz     short pf10
        lea     esp, [ebx-4]            // switch to kernel stack (&KData-4)

//  Process a page fault for the "user" address space (0 to 0x7FFFFFFF)
//
//  (edi) = Faulting address
//  (ebx) = ptr to KData
//  (esi) = Original ESP

pf10:   cld                         // Need to do this since the page fault could have come from anywhere!
		cmp		dword ptr ([KData].dwInDebugger), 0	// see if debugger active
		jne		short pf20							// if so, skip turning on of interrupts
		sti											// enable interrupts

pf20:	push    [esi+32]
        push	edi
        call    LowAddrSpacePageFault
        cli
        test    eax, eax
        jz		short pf40          // page not found in the Virtual memory tree
        cmp     word ptr ([KData].bResched), 1
        je      short pf60          // must reschedule now
        inc     [ebx].cNest         // back out of kernel one level
        mov     esp, esi            // restore stack pointer
        popad
        add     esp, 4
        iretd

//  This one was not a good one!  Jump to common fault handler
//
//  (edi) = faulting address

pf40:   inc     [ebx].cNest         // back out of kernel one level
        mov     esp, esi            // restore stack pointer
pf50:   mov     ecx, edi            // (ecx) = fault effective address
        mov     esi, 0Eh
        jmp     CommonFault

// The reschedule flag was set and we are at the first nest level into the kernel
// so we must reschedule now.

pf60:   mov     edi, PtrCurThd      // (edi) = ptr to current THREAD
        jmp     Reschedule
    }
}

Naked GeneralFault()
{
    _asm {
        pushad
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        mov     esi, 13
        jmp     CommonFault
    }
}

Naked InvalidOpcode(void)
{
    __asm {
        push    eax
        pushad
        mov     esi, 6
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        jmp     CommonFault
    }
}

Naked ZeroDivide(void)
{
    __asm {
        push    eax
        pushad
        xor     esi, esi            // (esi) = 0 (divide by zero fault)
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        jmp     CommonFault
    }
}

const BYTE PosTable[256] = {
	0,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	8,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
	6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
};


void __declspec(naked) GetHighPos(DWORD foo) {
	_asm {
		mov ecx, dword ptr [esp + 4]
		push ebx
		lea ebx, PosTable

		mov dl, 0xff
		xor eax, eax
		mov al, cl
		xlatb
		test al, al
		jne res

		shr ecx, 8
		add dl, 8
		mov al, cl
		xlatb
		test al, al
		jne res

		shr ecx, 8
		add dl, 8
		mov al, cl
		xlatb
		test al, al
		jne res

		shr ecx, 8
		add dl, 8
		mov al, cl
		xlatb
		test al, al
		jne res

		mov al, 9
res:
		add al, dl
		pop ebx
		ret
	}
}

Naked Int1Fault(void)
{
    __asm {
        push    eax                             // Save orig EAX as fake error code
        cmp     word ptr [esp + 6], 0FFFFh      // Is it an API call fault?
        je      skip_debug                      // Yes - handle page fault first
        mov     eax, [esp + 4]                  // (eax) = faulting EIP
        and     dword ptr [esp + 12], not 0100h  // Clear TF if set
        test    byte ptr [esp + 8], 3           // Are we trying to SS ring 0?
        jz      skip_debug                      // Yes - get out quick
        mov     eax, dword ptr [esp]            // Restore original EAX

        pushad
        mov     esi, 1
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        jmp     CommonFault

skip_debug:
        pop     eax
        iretd
    }
}

Naked Int2Fault(void)
{
    __asm {
        push    eax                 // Fake error code
        pushad
        mov     esi, 2
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        jmp     CommonFault
    }
}

Naked Int3Fault(void)
{
    __asm {
        dec     dword ptr [esp]     // Back up EIP
        push    eax                 // Fake error code
        pushad
        mov     esi, 3
        xor     ecx, ecx            // (ecx) = 0 (fault effective address)
        jmp     CommonFault
    }
}

PPAGETABLE LowAddrSpacePageFault(LPVOID pvAddr, DWORD flags)
{
    PSECTION pscn;
    MEMBLOCK *pmb;
    ULONG entry;
    ACCESSKEY ReqKey;
    DWORD OldPTE = 0;
    DWORD OldPDE = 0;
    if (((pscn = SectionTable[(ulong)pvAddr >> VA_SECTION]) != 0) &&
        ((pmb = (*pscn)[((ulong)pvAddr >> VA_BLOCK)&BLOCK_MASK]) != 0) &&
        (pmb != RESERVED_BLOCK && (ReqKey = TestAccess(&pmb->alk, &CurAKey)) != 0) &&
        ((entry = pmb->aPages[((ulong)pvAddr >> VA_PAGE) & PAGE_MASK]) & PG_VALID_MASK) &&
        (!(flags&2) || (entry & (PG_WRITE_MASK | PG_WRITE_THRU_MASK)))) {
        PPAGETABLE pPageTable;
        ULONG ulDirFaultIdx;
        PDIRTYRANGE pDirtyRange;
        ULONG ulBlockIndex;
        PDWORD pDirEntry;
        //
        // ulDirFaultIdx is the index of the page directory corresponding to
        // the address which faulted.
        //
        ulDirFaultIdx = (ULONG)pvAddr / (ARRAY_SIZE(g_PageDir.PTE) * PAGE_SIZE);
        OldPDE = g_PageDir.PTE[ulDirFaultIdx];
        if (ulDirFaultIdx < PDES_PER_SLOT) {
            // The fault occurred in the alias area, update the index to the
            // appropriate "real" entry.
            ulDirFaultIdx += PID_TO_PT_INDEX(pCurProc->procnum);
        } else {
            if (g_PageDir.PTE[ulDirFaultIdx] == 0
                    && g_ShadowPageDir.PTE[ulDirFaultIdx] != 0)
                g_PageDir.PTE[ulDirFaultIdx] = g_ShadowPageDir.PTE[ulDirFaultIdx];
            // Remember the keys which have been used for SetCPUASID().
            g_PageDirAccess |= ReqKey;
        }

        // pDirEntry points to the entry of the page directory corresponding to
        // the address which faulted.  Unless the fault occurred in the alias
        // area, in which case pDirEntry is the real entry, we will fix up both
        // regardless
        pDirEntry = &g_PageDir.PTE[ulDirFaultIdx];
        if ((*pDirEntry & PG_VALID_MASK) == 0)
        {
            DWORD dwPageTableEntry;

            //
            // Allocate a page to be used as the 1st level Page Table in the
            // Page Directory
            //
            pPageTable = AllocFreePTE(ulDirFaultIdx);
#ifdef DEBUG_VMEM
            {
                int     i;

                for (i = 0; i < ARRAY_SIZE(pPageTable->PTE); i++)
                {
                    if (pPageTable->PTE[i] != 0)
                    {
                        NKDbgPrintfW(
                            TEXT("LowAddrSpacePageFault: AllocFreePTE returned a PDE with nonzero entries\r\n"));
                        NKDbgPrintfW(
                            TEXT("pPageTable = 0x%8.8X, PTE index = %d\r\n"),
                            pPageTable, i);
                        DebugBreak();
                    }
                }
            }
#endif
            dwPageTableEntry = LIN_TO_PHYS(pPageTable) + PG_READ_WRITE;
            *pDirEntry = g_ShadowPageDir.PTE[ulDirFaultIdx] = dwPageTableEntry;
            //
            // If the current process owns the page, then we need to initialize
            // the alias page table pointer in slot 0.
            //
            if ((DWORD)(pCurProc->procnum + 1) == ulDirFaultIdx / PDES_PER_SLOT)
            {
                g_PageDir.PTE[ulDirFaultIdx % PDES_PER_SLOT] = dwPageTableEntry;
            }
        }
        else
        {
            //
            // There is already a 1st level Page Table page in the
            // faulting Page Directory entry
            //
            pPageTable = (PPAGETABLE)PHYS_TO_LIN(*pDirEntry & PG_PHYS_ADDR_MASK);
            // Is this a spurious page fault?
            OldPTE =  pPageTable->PTE[((ULONG)pvAddr/PAGE_SIZE)&0x3ff];
        }

        //
        // Now pPageTable points to the 2nd level Page Table page
        //
        ulBlockIndex = (DWORD)pvAddr / BLOCK_SIZE % BLOCKS_PER_PAGE_TABLE;
        memcpy(&pPageTable->PTE[ulBlockIndex * PAGES_PER_BLOCK], pmb->aPages, sizeof(pmb->aPages));
        pDirtyRange = &g_PTDirtyRegion[PT_PTR_TO_INDEX(pPageTable)];
        if (pDirtyRange->ulStartBlock > ulBlockIndex)
            pDirtyRange->ulStartBlock = ulBlockIndex;
        ulBlockIndex++;
        if (pDirtyRange->ulEndBlock < ulBlockIndex)
            pDirtyRange->ulEndBlock = ulBlockIndex;

        // Workaround for TLB incoherency seen in some AMD processors, specifically K6-2 400
        //
        if (OldPDE & OldPTE & PG_VALID_MASK)
        {
            // The Page Tables were valid on entry. We should not have got this fault.
            // 
#ifdef DEBUG_VMEM  
            NKDbgPrintfW(
              TEXT("LowAddrSpacePageFault: spurious Page Fault @0x%8.8X, PDE = 0x%8.8X, PTE = 0x%8.8X\r\n"),
              pvAddr, OldPDE, OldPTE);
#endif                            
            PhysTLBFlush();
        }
        return pPageTable;
    }
    return NULL;
}

void _inline UpdateSlot0AccessedBits(void)
{
    PDWORD pRealPD = &g_PageDir.PTE[0];
    PDWORD pShadow = &g_ShadowPageDir.PTE[PID_TO_PT_INDEX(pCurProc->procnum)];
    do {
        DWORD dwTest = *pRealPD;
        if (dwTest & PG_ACCESSED_MASK)
        {
#ifdef DEBUG_VMEM
            if ((*pShadow & ~PG_ACCESSED_MASK) != (dwTest & ~PG_ACCESSED_MASK))
            {
                NKDbgPrintfW(
                    TEXT("UpdateSlot0AccessedBits: Real Slot 0 PDE doesn't match Shadow Process PDE\r\n"));
                NKDbgPrintfW(
                    TEXT("Slot index = %d, PDE index = %d\r\n"),
                    pCurProc->procnum + 1,
                    pRealPD - &g_PageDir.PTE[0]);
                NKDbgPrintfW(
                    TEXT("Real PDE = 0x%8.8X, Shadow PDE = 0x%8.8X\r\n"),
                    dwTest & ~PG_ACCESSED_MASK, *pShadow & ~PG_ACCESSED_MASK);
                DebugBreak();
            }
#endif
            *pShadow = dwTest;
            *pRealPD = dwTest & (~PG_ACCESSED_MASK);
        }
        pShadow++;
        pRealPD++;
    } while (pRealPD < &g_PageDir.PTE[PDES_PER_SLOT]);
}


DWORD ResetPage(PPAGETABLE pPageTable, ULONG ulIndex)
{
    ULONG ulStart = g_PTDirtyRegion[ulIndex].ulStartBlock;
    ULONG ulEnd = g_PTDirtyRegion[ulIndex].ulEndBlock;

    if (ulEnd)
    {
        g_PTDirtyRegion[ulIndex].ulEndBlock = 0;
        g_PTDirtyRegion[ulIndex].ulStartBlock = BLOCKS_PER_PAGE_TABLE;
        memset((PBYTE)pPageTable+(ulStart*PAGES_PER_BLOCK*sizeof(DWORD)), 0,
               (ulEnd-ulStart)*PAGES_PER_BLOCK*sizeof(DWORD));
    }
#ifdef DEBUG_VMEM
    {
        int     i;

        for (i = 0; i < ARRAY_SIZE(pPageTable->PTE); i++)
        {
            if (pPageTable->PTE[i] != 0)
            {
                NKDbgPrintfW(
                    TEXT("ResetPage: Bad dirty region, PTE not zero after reset\r\n"));
                NKDbgPrintfW(
                    TEXT("ulStartBlock = %d, ulEndBlock = %d, PTE index = %d, PTE value = 0x%8.8X\r\n"),
                    ulStart, ulEnd, i, pPageTable->PTE[i]);
                DebugBreak();
            }
        }
    }
#endif
    return(ulEnd);
}

PPAGETABLE AllocFreePTE(ULONG ulDirIdx)
{
    PPAGETABLE pPageTable;
    ULONG ulMapIdx;

    //
    // Copy the access bits from the alias slot entries to the corresponding
    // "real" slot entries.
    //
    UpdateSlot0AccessedBits();

    //
    // Start the scan where we left off last time
    //
    ulMapIdx = g_AccessScanIdx;

    while (TRUE)
    {
        ULONG ulTestPDIdx = g_PTMapIdx[ulMapIdx];
        DWORD dwTestPDE = g_PageDir.PTE[ulTestPDIdx];

        if ((dwTestPDE & PG_ACCESSED_MASK) == 0)
        {
            if ((g_ShadowPageDir.PTE[ulTestPDIdx] & PG_ACCESSED_MASK) == 0)
            {
                PDWORD pAlias = &g_PageDir.PTE[ulTestPDIdx % PDES_PER_SLOT];

                if ((*pAlias & PG_PHYS_ADDR_MASK) ==
                    (dwTestPDE & PG_PHYS_ADDR_MASK))
                {
                    *pAlias = 0;
                }
#ifdef DEBUG_VMEM
                else
                if ((DWORD)(pCurProc->procnum + 1) == ulTestPDIdx / PDES_PER_SLOT)
                {
                    NKDbgPrintfW(
                        TEXT("AllocFreePTE: Slot 0 PDE doesn't match Process Slot PDE\r\n"));
                    NKDbgPrintfW(
                        TEXT("Slot index = %d, PDE index = %d\r\n"),
                        ulTestPDIdx / PDES_PER_SLOT, ulTestPDIdx % PDES_PER_SLOT);
                    NKDbgPrintfW(
                        TEXT("Slot 0 PDE = 0x%8.8X, Process Slot PDE = 0x%8.8X\r\n"),
                        *pAlias & ~PG_ACCESSED_MASK, dwTestPDE & ~PG_ACCESSED_MASK);
                    DebugBreak();
                }
#endif

                g_ShadowPageDir.PTE[ulTestPDIdx] = 0;
                g_PageDir.PTE[ulTestPDIdx] = 0;
                g_AccessScanIdx = (ulMapIdx+1) % PTE_POOL_SIZE; // Set up the global for the next time
                g_PTMapIdx[ulMapIdx] = ulDirIdx;
                pPageTable = &g_PageTablePool[ulMapIdx];

                if (ResetPage(pPageTable, ulMapIdx))
                {
                    PhysTLBFlush();
                }

                return(pPageTable);
            }
        }
        else
        {
            g_PageDir.PTE[ulTestPDIdx] = dwTestPDE & ~PG_ACCESSED_MASK;
        }
#ifdef DEBUG_VMEM
        if (dwTestPDE != 0 && (g_ShadowPageDir.PTE[ulTestPDIdx] & ~PG_ACCESSED_MASK) !=
            (dwTestPDE & ~PG_ACCESSED_MASK))
        {
            NKDbgPrintfW(
                TEXT("AllocFreePTE: Shadow PDE doesn't match Process Slot PDE\r\n"));
            NKDbgPrintfW(
                TEXT("Slot index = %d, PDE index = %d\r\n"),
                ulTestPDIdx / PDES_PER_SLOT, ulTestPDIdx % PDES_PER_SLOT);
            NKDbgPrintfW(
                TEXT("Shadow PDE = 0x%8.8X, Process Slot PDE = 0x%8.8X\r\n"),
                g_ShadowPageDir.PTE[ulTestPDIdx] & ~PG_ACCESSED_MASK, dwTestPDE & ~PG_ACCESSED_MASK);
            DebugBreak();
        }
#endif
        g_ShadowPageDir.PTE[ulTestPDIdx] = dwTestPDE & ~PG_ACCESSED_MASK;
        ulMapIdx = (ulMapIdx+1) % PTE_POOL_SIZE;
    }
}

#ifdef DEBUG_VMEM
void ValidateVirtualMemory()
{
    int         iRealSlotNumber;
    int         iSlotIndex;
    PSECTION    pSection;
    int         i, j, k;
    int         iFirstPageDir;
    DWORD       dwPageDirEntry;
    PPAGETABLE  pPageTable;
    int         iFirstMemblock;
    MEMBLOCK   *pMemblock;
    int         iFirstPTE;
    DWORD       dwPageTableEntry;

    //
    // Find the real slot associated with the alias slot 0
    //
    iRealSlotNumber = pCurProc->procnum + 1;

    //
    // Check each process slot
    //    - If it is the current process ensure each entry matches the
    //      corresponding entry in slot 0
    //    - Check that each entry has a backing MEMBLOCK
    //    - Check that each entry matches it's corresponding MEMBLOCK entry.
    //

    for (iSlotIndex = 1; iSlotIndex <= 32; iSlotIndex++)
    {
        iFirstPageDir = iSlotIndex * PDES_PER_SLOT;
        pSection = SectionTable[iSlotIndex];

        for (i = 0; i < PDES_PER_SLOT; i++)
        {
            if (iSlotIndex == iRealSlotNumber)
            {
                //
                // If this slot is for the active process insure the alias
                // area at slot 0 matches
                //
                if ((g_PageDir.PTE[i] & ~PG_ACCESSED_MASK) !=
                    (g_PageDir.PTE[i + iFirstPageDir] & ~PG_ACCESSED_MASK))
                {
                    if (i != 0 ||
                        (g_PageDir.PTE[i] & ~PG_ACCESSED_MASK) != KPAGE_PTE ||
                        g_PageDir.PTE[i + iFirstPageDir] != 0)
                    {
                        NKDbgPrintfW(
                            TEXT("ValidateVirtualMemory: Slot 0 PDE doesn't match Process slot PDE\r\n"));
                        NKDbgPrintfW(
                            TEXT("Slot index = %d, PDE index = %d\r\n"), iSlotIndex, i);
                        NKDbgPrintfW(
                            TEXT("pSection = 0x%8.8X, Slot 0 PDE = 0x%8.8X, Process Slot PDE = 0x%8.8X\r\n"),
                            pSection, g_PageDir.PTE[i] & ~PG_ACCESSED_MASK,
                            g_PageDir.PTE[i + iFirstPageDir] & ~PG_ACCESSED_MASK);
                        DebugBreak();
                    }
                }
            }

            dwPageDirEntry = g_PageDir.PTE[iFirstPageDir + i];

            if (dwPageDirEntry != 0)
            {
                pPageTable = (PPAGETABLE)PHYS_TO_LIN(dwPageDirEntry & PG_PHYS_ADDR_MASK);

                iFirstMemblock = i * BLOCKS_PER_PAGE_TABLE;

                for (j = 0; j < BLOCKS_PER_PAGE_TABLE; j++)
                {
                    pMemblock = (*pSection)[iFirstMemblock + j];
                    iFirstPTE = j * PAGES_PER_BLOCK;

                    for (k = 0; k < PAGES_PER_BLOCK; k++)
                    {
                        dwPageTableEntry = pPageTable->PTE[iFirstPTE + k];

                        if (dwPageTableEntry != 0)
                        {
                            if (pMemblock != NULL && pMemblock != RESERVED_BLOCK)
                            {
                                if ((dwPageTableEntry & ~0xFFF) !=
                                    (pMemblock->aPages[k] & ~0xFFF) && pMemblock->aPages[k])
                                {
                                    NKDbgPrintfW(
                                        TEXT("ValidateVirtualMemory: PTE doesn't match MEMBLOCK entry\r\n"));
                                    NKDbgPrintfW(
                                        TEXT("Slot index = %d, PDE index = %d, MemBlock index = %d, Page index = %d\r\n"),
                                        iSlotIndex, i, j, k);
                                    NKDbgPrintfW(
                                        TEXT("pSection = 0x%8.8X, dwPageDirEntry = 0x%8.8X, dwPageTableEntry = 0x%8.8X, dwMemblockEntry = 0x%8.8X\r\n"),
                                        pSection, dwPageDirEntry, dwPageTableEntry, pMemblock->aPages[k]);
                                    DebugBreak();
                                }
                            }
                            else
                            {
                                //
                                // Special case the Kernel Data Page since it
                                // doesn't get unmapped when a process goes
                                // away.
                                //
                                if (i != 0 || j != 0 || k != 5 ||
                                    (dwPageTableEntry & ~PG_ACCESSED_MASK) != KPAGE_PTE)
                                {
                                    NKDbgPrintfW(
                                        TEXT("ValidateVirtualMemory: PTE exists without backing MEMBLOCK\r\n"));
                                    NKDbgPrintfW(
                                        TEXT("Slot index = %d, PDE index = %d, MemBlock index = %d, Page index = %d\r\n"),
                                        iSlotIndex, i, j, k);
                                    NKDbgPrintfW(
                                        TEXT("pSection = 0x%8.8X, dwPageDirEntry = 0x%8.8X, dwPageTableEntry = 0x%8.8X\r\n"),
                                        pSection, dwPageDirEntry, dwPageTableEntry);
                                    DebugBreak();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif

void InvalidatePageTables(PDWORD pDirEntry, PDWORD pLastDirEntry)
{
    BOOL bFlushTLB = FALSE;
    PPAGETABLE pPageTable;
    int idxEntry;

    KCALLPROFON(72);

    do {
        if (*pDirEntry & PG_VALID_MASK) {
            pPageTable = (PPAGETABLE)PHYS_TO_LIN(*pDirEntry & PG_PHYS_ADDR_MASK);
            idxEntry = pDirEntry - &g_ShadowPageDir.PTE[0];
            if (ResetPage(pPageTable, PT_PTR_TO_INDEX(pPageTable))
                    && (g_PageDir.PTE[idxEntry] & PG_VALID_MASK))
                bFlushTLB = TRUE;
        }
        pDirEntry++;
    } while (pDirEntry <= pLastDirEntry);

    if (bFlushTLB)
        PhysTLBFlush();

    KCALLPROFOFF(72);
}

void InvalidateRange(PVOID pvAddr, ULONG ulSize)
{
    if ((ulong)pvAddr < ADDR_SLOT_SIZE)
        (ULONG)pvAddr |= pCurProc->dwVMBase;
    if (ulSize)
        KCall((PKFN)InvalidatePageTables,
                &g_ShadowPageDir.PTE[(ULONG)pvAddr/(ARRAY_SIZE(g_ShadowPageDir.PTE)*PAGE_SIZE)],
                &g_ShadowPageDir.PTE[((ULONG)pvAddr + ulSize - 1) / (ARRAY_SIZE(g_ShadowPageDir.PTE) * PAGE_SIZE)]);
}


// System call trap handler.
//
// Pop the iret frame from the stack, switch back to the caller's stack, enable interrupts,
// and dispatch the system call.
//
//		CPU State:	ring1 stack & CS, interrupts disabled.

#pragma warning(disable:4035)

PVOID __declspec(naked) Int20SyscallHandler(void)
{
	__asm {
	    mov [KData.pAPIReturn], offset APICallReturn
	    mov eax, offset sc00
	    ret

sc00:	mov		ecx, 4[esp]			// (ecx) = caller's CS
		and		cl, 0FCh
		cmp		cl, KGDT_R1_CODE
		pop		ecx					// (ecx) = EIP of "int SYSCALL"
		jne		short sc10			// caller was in user mode
		add		esp, 8				// remove CS & flags from stack
		push	fs:dword ptr [0]	// save exception chain linkage
		push	KERNEL_MODE			// mark as kernel mode caller
		jmp		short sc12
sc10:	mov		esp, [esp+8]		// (esp) = caller's stack
		push	fs:dword ptr [0]	// save exception chain linkage
		push	USER_MODE			// mark as user mode caller
sc12:	sub		ecx, FIRST_METHOD+2	// (ecx) = iMethod * APICALL_SCALE
		sti							// interrupts OK now
		cmp		ecx, -APICALL_SCALE
		je		short sc25			// api call return
		sar		ecx, 1				// (ecx) = iMethod
		lea		eax, [esp+12]		// (eax) = ptr to api arguments
		push	ecx					// (arg3) = iMethod
		push	eax					// (arg2) = ptr to args
		push	dword ptr [eax-4]	// (arg1) = return address
		sub		eax, 12				// (eax) = ptr to thread mode
		push	eax					// (arg0) = pMode
		call	ObjectCall			// (eax) = api function address (0 if completed)
		add		esp, 16				// clear ObjectCall args off the stack
		pop		edx					// (edx) = thread mode
		add		esp, 4				// remove exc. linkage parm
		mov		fs:dword ptr [0], -2	// mark PSL boundary in exception chain
		mov     ecx, PtrCurThd      // (ecx) = ptr to THREAD struct
		mov     ecx, [ecx].pcstkTop // (ecx) = ptr to CALLSTACK struct
		mov     [ecx].ExEsp, esp    // .\     v
		mov     [ecx].ExEbp, ebp    // ..\    v
		mov     [ecx].ExEbx, ebx    // ...> save registers for possible exception recovery
		mov     [ecx].ExEsi, esi    // ../
		mov     [ecx].ExEdi, edi    // ./
		test	edx, edx
		jz		short sc20			// dispatch the function in kernel mode

// Dispatch to api function in user mode. To do this, a far call frame is constructed on
// the stack and a far return is issued.
//
//	(eax) = api function address

		mov		edx, esp
		mov		dword ptr [edx], SYSCALL_RETURN
		push	KGDT_R3_DATA | 3
		push	edx
		push	KGDT_R3_CODE | 3
		push	eax
		retf

// Dispatch to api function in kernel mode. Just call the function directly and fall through
// into the api return code.
//
//	(eax) = api function address

sc20:	pop		edx					// discard return address
		call	eax					// & call the api function
APICallReturn:
		push	0					// space for exception chain linkage
		push	KERNEL_MODE			// save current thread mode

// Api call return. Retrieve return address, mode, and exception linkage from the
// thread's call stack.
//
//	(eax:edx) = function return value
//	(TOS) = thread's execution mode
//	(TOS+4) = space to receive previous exception chain linkage

sc25:	push	eax					// save return value
		push	edx					// ...
		lea		eax, [esp+8]		// (eax) = ptr to thread's execution mode
		push	eax
		call	ServerCallReturn	// (eax) = api return address
		mov		[esp], eax			// save return address
		mov		edx, [esp+4]		// restore return value
		mov		eax, [esp+8]		// ...
		mov		ecx, [esp+16]		// (ecx) = saved exception linkage
		cmp		dword ptr [esp+12], KERNEL_MODE
		mov		fs:[0], ecx			// restore exception linkage
		je		short sc28			// dispatch thread in kernel mode
		lea		ecx, [esp+20]		// (ecx) = final stack pointer value
		mov		dword ptr [esp+4], KGDT_R3_CODE | 3
		mov		[esp+8], ecx		// ESP restore value
		mov		dword ptr [esp+12], KGDT_R3_DATA | 3
		retf						// return to ring3 & restore stack pointer

sc28:	ret		16					// return & clear working data from the stack
	}
}

int __declspec(naked) KCall(PKFN pfn, ...)
{
    __asm {
        push    ebp
        mov     ebp, esp
        push    ebx
        mov     eax, 12[ebp]    // (eax) = arg0
        mov     edx, 16[ebp]    // (edx) = arg1
        mov     ecx, 20[ebp]    // (ecx) = arg2
        mov     ebx, 8[ebp]     // (ebx) = function address
        cmp     [KData].cNest, 1
        jne     short kcl50     // Already in non-preemtible state
        int KCALL_INT           // trap to ring0 for non-preemtible stuff
        pop     ebx
        pop     ebp             // restore original EBP
        ret

kcl50:  push    ecx             // push Arg2
        push    edx             // push Arg1
        push    eax             // push Arg0
        call    ebx             // invoke function
        add     esp, 3*4        // remove args from stack
        pop     ebx
        pop     ebp             // restore original EBP
        ret
    }
}
#pragma warning(default:4035)               // Turn warning back on


Naked Int22KCallHandler(void)
{
    __asm {
        push    eax                 // fake Error Code
        pushad
        mov     esi, esp            // save ptr to register save area
        mov     esp, offset KData-4 // switch to kernel stack
        dec     [KData].cNest
        sti
        push    ecx                 // push Arg2
        push    edx                 // push Arg1
        push    eax                 // push Arg0
        call    ebx                 // invoke non-preemtible function
        mov     [esi+7*4], eax      // save return value into PUSHAD frame
        mov     ecx, OFFSET KData
        xor     edi, edi            // force Reschedule to reload the thread's state
    	cli

    	cmp		word ptr ([ecx].bResched), 1
    	jne		short NotReschedule
    	jmp		Reschedule
NotReschedule:
	    cmp     dword ptr ([ecx].dwKCRes), 1
        jne		short NotResched2
        jmp		Reschedule
NotResched2:
        mov     esp, esi
        inc     [ecx].cNest
        popad
        add     esp, 4              // throw away the error code
        iretd
    }
}

void InitializePageTables(void)
{
    UINT i;

    InitIDTEntry(SYSCALL_INT, KGDT_R1_CODE | 1, Int20SyscallHandler(), RING3_INT_GATE);
    InitIDTEntry(KCALL_INT, KGDT_R0_CODE, Int22KCallHandler, RING1_INT_GATE);

    // Not needed.  Done by init.asm --> memset(g_PageTablePool, 0, sizeof(g_PageTablePool));
    for (i = 0; i < ARRAY_SIZE(g_PageTablePool); i++) {
        g_PTDirtyRegion[i].ulStartBlock = BLOCKS_PER_PAGE_TABLE;
        g_PTDirtyRegion[i].ulEndBlock = 0;
        g_ShadowPageDir.PTE[i+PDES_PER_SLOT] = LIN_TO_PHYS(&g_PageTablePool[i]) + PG_READ_WRITE;
        g_PTMapIdx[i] = i+PDES_PER_SLOT;
    }
}

///////////////////////////// FLOATING POINT UNIT CODE /////////////////////////////////

void InitializeEmx87(void) {
    // Fast FP save/restore instructions are not available when emulating FP
    KCALLPROFON(70);
    ProcessorFeatures &= ~CPUID_FXSR;
	_asm {
		mov	eax, cr0
		or	eax, MP_MASK or EM_MASK
		and eax, NOT (TS_MASK or NE_MASK)
		mov cr0, eax
	}
    KCALLPROFOFF(70);
}

void InitNPXHPHandler(LPVOID NPXNPHandler) {
    KCALLPROFON(71);
	if(pTOC->ulKernelFlags & KFLAG_NOTALLKMODE)
	{
		InitIDTEntry(0x07, KGDT_R3_CODE, NPXNPHandler, RING3_TRAP_GATE);
	}
	else
	{
		InitIDTEntry(0x07, KGDT_R1_CODE, NPXNPHandler, RING1_TRAP_GATE);
	}
    KCALLPROFOFF(71);
}

Naked FPUNotPresentException(void)
{
    _asm {
        push    eax                 // Fake error code
        
        // We cannot be emulating FP if we arrive here. It is safe to not check
        // if CR0.EM is set.
        pushad
        clts

        mov     ebx, OFFSET KData
        dec     [ebx].cNest         // count kernel reentrancy level
        mov     esi, esp            // (esi) = original stack pointer
        jnz     short fpu10
        lea     esp, [ebx-4]        // switch to kernel stack (&KData-4)
 fpu10:
        sti

        mov     eax, PtrCurThd
        push    (PTHREAD)[eax].aky
        mov     (PTHREAD)[eax].aky, 0xffffffff

        mov     eax, g_CurFPUOwner
        test    eax, eax
        jz      NoCurOwner
        mov     eax, [eax].tlsPtr
        sub     eax, FLTSAVE_BACKOFF
        and     eax, 0xfffffff0             // and al, f0 causes processor stall
        test    ProcessorFeatures, CPUID_FXSR
        jz      fpu_fnsave
        FXSAVE_EAX
        jmp     NoCurOwner
fpu_fnsave:
        fnsave  [eax]
NoCurOwner:
        mov     eax, PtrCurThd
        mov     g_CurFPUOwner, eax
        pop     (PTHREAD)[eax].aky
        mov     eax, [eax].tlsPtr
        sub     eax, FLTSAVE_BACKOFF
        and     eax, 0xfffffff0             // and al, f0 causes processor stall
        test    ProcessorFeatures, CPUID_FXSR
        jz      fpu_frestor
        FXRESTOR_EAX
        jmp     fpu_done
fpu_frestor:
        frstor  [eax]
fpu_done:
        cli
        
        cmp     word ptr ([KData].bResched), 1
        je      short fpu_resched           // must reschedule now
        inc     [ebx].cNest                 // back out of kernel one level
        mov     esp, esi                    // restore stack pointer
        popad
        add     esp, 4                      // skip fake error code
        iretd
    
        // The reschedule flag was set and we are at the first nest level into the kernel
        // so we must reschedule now.

fpu_resched:
        mov     edi, PtrCurThd      // (edi) = ptr to current THREAD
        jmp     Reschedule
    }
}

void FPUFlushContext(void) {
    FLOATING_SAVE_AREA *pFSave;
    if (g_CurFPUOwner) {
        ACCESSKEY ulOldKey;
        SWITCHKEY(ulOldKey,0xffffffff);
        _asm {
            // If we are emulating FP, g_CurFPUOwner is always 0 so we don't
            // have to test if CR0.EM is set(i.e. fnsave will not GP fault).
            clts
        }
	    if (g_CurFPUOwner->pThrdDbg && g_CurFPUOwner->pThrdDbg->psavedctx) {
            pFSave = &g_CurFPUOwner->pThrdDbg->psavedctx->FloatSave;
            
            _asm {
                mov eax, pFSave
                fnsave [eax]
            }
        }  else  {
            pFSave = PTH_TO_FLTSAVEAREAPTR(g_CurFPUOwner);
		    _asm  {
                mov     eax, pFSave
                test    ProcessorFeatures, CPUID_FXSR
                jz      flush_fsave
                FXSAVE_EAX
                jmp     flush_done
            flush_fsave:
                fnsave   [eax]
                fwait
            flush_done:
            }
        }
        _asm  {
            mov     eax, CR0        // fnsave destroys FP state &
            or      eax, TS_MASK    // g_CurFPUOwner is 0 so we must force
            mov     CR0, eax        // trap 7 on next FP instruction
        }
        SETCURKEY(ulOldKey);
        g_CurFPUOwner = 0;
    }
}

Naked FPUException(void)
{
    _asm {
        push    eax                 // Fake error code
        pushad
        xor     ecx, ecx            // EA = 0
        mov     esi, 16
        jmp		CommonFault
    }
}

void InitializeFPU(void)
{
    KCALLPROFON(69);    
    InitIDTEntry(0x07, KGDT_R0_CODE, FPUNotPresentException, INTERRUPT_GATE);
    InitIDTEntry(0x10, KGDT_R0_CODE, FPUException, INTERRUPT_GATE);

    _asm {
        mov     eax, cr0
        or      eax, MP_MASK OR NE_MASK
        and     eax, NOT (TS_MASK OR EM_MASK)
        mov     cr0, eax

        finit

        fwait
        mov     ecx, offset g_InitialFPUState
        add     ecx, 10h                    // Force 16 byte alignment else
        and     cl, 0f0h                    // fxsave will fault
        test    ProcessorFeatures, CPUID_FXSR
        jz      no_fxsr

        MOV_EDX_CR4
        or      edx, CR4_FXSR
        MOV_CR4_EDX

        FXSAVE_ECX
        mov     [ecx].MXCsr, 01f80h                       // Mask KNI exceptions
        and     word ptr [ecx], NOT NPX_CW_PRECISION_MASK // Control word is
        or      word ptr [ecx], NPX_CW_PRECISION_53       // 16 bits wide here
        jmp     init_done
no_fxsr:
        fnsave  [ecx]
        // Win32 threads default to long real (53-bit significand).
        // Control word is 32 bits wide here
        and     dword ptr [ecx], NOT NPX_CW_PRECISION_MASK
        or      dword ptr [ecx], NPX_CW_PRECISION_53
init_done:

        or      eax, TS_MASK
        mov     cr0, eax
   }
   KCALLPROFOFF(69);    
}

