/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include <kernel.h>

extern void KernelFindMemory(void);
extern void InitializePageTables(void);

// This table is NOT const data since it is edited to initialize the TSS entries.
unsigned __int64 g_aGlobalDescriptorTable[] = {
    0,
    0x00CF9A000000FFFF,         // Ring 0 code, Limit = 4G
    0x00CF92000000FFFF,         // Ring 0 data, Limit = 4G
    0x00CFBA000000FFFF,         // Ring 1 code, Limit = 4G
    0x00CFB2000000FFFF,         // Ring 1 data, Limit = 4G
    0x00CFDA000000FFFF,         // Ring 2 code, Limit = 4G
    0x00CFD2000000FFFF,         // Ring 2 data, Limit = 4G
    0x00CFFA000000FFFF,         // Ring 3 code, Limit = 4G
    0x00CFF2000000FFFF,         // Ring 3 data, Limit = 4G
    0,                          // Will be main TSS
    0,                          // Will be NMI TSS
    0,                          // Will be Double Fault TSS
    0x0040F20000000000+FS_LIMIT,    // PCR selector
    0x00CFBE000000FFFF,			// Ring 1 (conforming) code, Limit = 4G
};

#pragma pack(push, 1)           // We want this structure packed exactly as declared!

typedef struct {
    BYTE        PushEax;
    BYTE        Pushad;
    BYTE        MovEsi;
    FARPROC     pfnHandler;
    BYTE        JmpNear;
    DWORD       JmpOffset;
    DWORD       Padding;
} INTHOOKSTUB, * PINTHOOKSTUB;

INTHOOKSTUB g_aIntHookStubs[32] = {0};

typedef struct {
    USHORT Size;
    PVOID Base;
} FWORDPtr;

const FWORDPtr g_KernelGDTBase = {sizeof(g_aGlobalDescriptorTable)-1, &g_aGlobalDescriptorTable };


KIDTENTRY g_aIntDescTable[MAXIMUM_IDTVECTOR+1];

const FWORDPtr g_IDTBase = {sizeof(g_aIntDescTable), &g_aIntDescTable };

#pragma pack(pop)

KTSS MainTSS;
KTSS DoubleTSS;
KTSS NMITSS;

BYTE TASKStack[0x800];      

char SyscallStack[128];		// temporary stack for syscall trap

void Int1Fault(void);
void Int2Fault(void);
void Int3Fault(void);
void GeneralFault(void);
void PageFault(void);
void InvalidOpcode(void);
void ZeroDivide(void);
void CommonIntDispatch(void);
void OEMNMIHandler(void);

void DumpTSS(KTSS *ptss) {
    NKDbgPrintfW(L"TSS=%8.8lx EIP=%8.8lx Flags=%8.8lx\r\n",
    		ptss, ptss->Eip, ptss->Eflags);
    NKDbgPrintfW(L"Eax=%8.8lx Ebx=%8.8lx Ecx=%8.8lx Edx=%8.8lx\r\n",
            ptss->Eax, ptss->Ebx, ptss->Ecx, ptss->Edx);
    NKDbgPrintfW(L"Esi=%8.8lx Edi=%8.8lx Ebp=%8.8lx Esp=%8.8lx\r\n",
            ptss->Esi, ptss->Edi, ptss->Ebp, ptss->Esp);
    NKDbgPrintfW(L"CS=%4.4lx DS=%4.4lx ES=%4.4lx SS=%4.4lx FS=%4.4lx GS=%4.4lx\r\n",
            ptss->Cs, ptss->Ds, ptss->Es, ptss->Ss, ptss->Fs, ptss->Gs);
}

void NonMaskableInterrupt(void) {
    KData.cNest = -1;
    NKDbgPrintfW(L"\r\nNMI -- backlink=%4.4x\r\n", NMITSS.Backlink);
    DumpTSS(&MainTSS);
	OEMNMIHandler();
    __asm {
        cli
        hlt
    }
}

void DoubleFault(void) {
    KData.cNest = -1;
    NKDbgPrintfW(L"\r\nDouble Fault -- backlink=%4.4x\r\n", DoubleTSS.Backlink);
    DumpTSS(&MainTSS);
    __asm {
        cli
        hlt
    }
}

void InitTSSSelector(ULONG uSelector, PKTSS pTSS) {
    PKGDTENTRY pTSSSel = (PKGDTENTRY)(((ULONG)&g_aGlobalDescriptorTable[0]) + uSelector);
    pTSSSel->LimitLow = sizeof(KTSS)-1;
    pTSSSel->BaseLow = (USHORT)((DWORD)pTSS) & 0xFFFF;
    pTSSSel->HighWord.Bytes.BaseMid = (UCHAR)(((DWORD)pTSS) >> 16) & 0xFF;
    pTSSSel->HighWord.Bytes.BaseHi = (UCHAR)(((DWORD)pTSS) >> 24) & 0xFF;
    pTSSSel->HighWord.Bits.Type = TYPE_TSS;
    pTSSSel->HighWord.Bits.Pres = 1;
}

void InitializeTaskStates(void) {
    ulong pageDir;
    __asm {
        mov eax, CR3
        mov pageDir, eax
    }
    InitTSSSelector(KGDT_MAIN_TSS, &MainTSS);

    MainTSS.Esp0 = 0xB00B00;
    MainTSS.Ss0 = KGDT_R0_DATA;
    MainTSS.Esp1 = (ulong)(SyscallStack+sizeof(SyscallStack));
    MainTSS.Ss1 = KGDT_R1_DATA | 1;

    InitTSSSelector(KGDT_NMI_TSS, &NMITSS);
    NMITSS.Eip = (ULONG)&NonMaskableInterrupt;
    NMITSS.Cs = KGDT_R0_CODE;
    NMITSS.Ss = KGDT_R0_DATA;
    NMITSS.Ds = KGDT_R0_DATA;
    NMITSS.Es = KGDT_R0_DATA;
    NMITSS.Ss0 = KGDT_R0_DATA;
    NMITSS.Esp = NMITSS.Esp0 = ((ULONG)&TASKStack) + sizeof(TASKStack);
    NMITSS.CR3 = pageDir;

    InitTSSSelector(KGDT_DOUBLE_TSS, &DoubleTSS);
    DoubleTSS.Eip = (ULONG)&DoubleFault;
    DoubleTSS.Cs = KGDT_R0_CODE;
    DoubleTSS.Ss = KGDT_R0_DATA;
    DoubleTSS.Ds = KGDT_R0_DATA;
    DoubleTSS.Es = KGDT_R0_DATA;
    DoubleTSS.Ss0 = KGDT_R0_DATA;
    DoubleTSS.Esp = DoubleTSS.Esp0 = ((ULONG)&TASKStack) + sizeof(TASKStack);
    DoubleTSS.CR3 = pageDir;
 
	// Change the KGDT_EMX87 entry in the global descriptor table to be Ring 3 conforming
	// if KFLAG_NOTALLKMODE set (not all running in K-mode), else keep at Ring 1
	if (pTOC->ulKernelFlags & KFLAG_NOTALLKMODE)
	{
		PKGDTENTRY pTSSSel = (PKGDTENTRY)(((ULONG)&g_aGlobalDescriptorTable[0]) + KGDT_EMX87);
		pTSSSel->HighWord.Bits.Dpl = 3;
	}

    _asm {
        mov     eax, KGDT_MAIN_TSS
        ltr     ax
    }
}

void InitIDTEntry(int i, USHORT usSelector, PVOID pFaultHandler, USHORT usGateType) {
    PKIDTENTRY pCur = &g_aIntDescTable[i];
    pCur->Offset = LOWORD((DWORD)pFaultHandler);
    pCur->ExtendedOffset = HIWORD((DWORD)pFaultHandler);
    pCur->Selector = usSelector;
    pCur->Access = usGateType;
}

void InitializeIDT(void) {
    InitIDTEntry(0x00, KGDT_R0_CODE, ZeroDivide, INTERRUPT_GATE);
    InitIDTEntry(0x01, KGDT_R0_CODE, Int1Fault, RING3_INT_GATE);
    InitIDTEntry(0x02, KGDT_NMI_TSS, 0, TASK_GATE);
    InitIDTEntry(0x02, KGDT_R0_CODE, Int2Fault, INTERRUPT_GATE);
    InitIDTEntry(0x03, KGDT_R0_CODE, Int3Fault, RING3_INT_GATE);
    InitIDTEntry(0x06, KGDT_R0_CODE, InvalidOpcode, INTERRUPT_GATE);
    InitIDTEntry(0x08, KGDT_DOUBLE_TSS, 0, TASK_GATE);
    InitIDTEntry(0x0D, KGDT_R0_CODE, GeneralFault, INTERRUPT_GATE);
    InitIDTEntry(0x0E, KGDT_R0_CODE, PageFault, INTERRUPT_GATE);
    _asm { lidt [g_IDTBase] };
}

BOOL HookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    if (hwInterruptNumber < ARRAY_SIZE(g_aIntDescTable)) {
        PKIDTENTRY pIDTEntry = &g_aIntDescTable[hwInterruptNumber];
        if (pIDTEntry->Offset == 0 && pIDTEntry->Selector == 0) {
            PINTHOOKSTUB pStub = &g_aIntHookStubs[0];
            for (pStub = &g_aIntHookStubs[0]; pStub <= LAST_ELEMENT(g_aIntHookStubs); pStub++) {
                if (pStub->pfnHandler == NULL) {
                    pStub->PushEax   = 0x50;
                    pStub->Pushad    = 0x60;
                    pStub->MovEsi    = 0xBE;
                    pStub->pfnHandler= pfnHandler;
                    pStub->JmpNear   = 0xE9;
                    pStub->JmpOffset = ((DWORD)&CommonIntDispatch) - ((DWORD)(&pStub->JmpOffset) + 4);
                    InitIDTEntry(hwInterruptNumber, KGDT_R0_CODE, pStub, INTERRUPT_GATE);
                    return(TRUE);
                }
            }
        }
    }
    return(FALSE);
}

BOOL UnhookInterrupt(int hwInterruptNumber, FARPROC pfnHandler) {
    if (hwInterruptNumber < ARRAY_SIZE(g_aIntDescTable)) {
        PKIDTENTRY pIDTEntry = &g_aIntDescTable[hwInterruptNumber];
        PINTHOOKSTUB pStub = (PINTHOOKSTUB)(pIDTEntry->Offset + (pIDTEntry->ExtendedOffset << 16));
        if (pStub >= &g_aIntHookStubs[0] && pStub <= LAST_ELEMENT(g_aIntHookStubs)) {
            InitIDTEntry(hwInterruptNumber, 0, 0, 0);
            pStub->pfnHandler = NULL;
            return(TRUE);
        }
    }
    return(FALSE);
}

extern DWORD PageDir[1024];

void SystemInitialization(void) {
    int i;

    /* Zero out kernel data page. */
    memset(&KData, 0, sizeof(KData));
    KData.handleBase = 0x80000000;

    /* Initialize SectionTable in KPage */
    for (i = 1 ; i <= SECTION_MASK ; ++i)
        SectionTable[i] = NULL_SECTION;

    /* Copy kernel data to RAM & zero out BSS */
    KernelRelocate(pTOC);

    OEMInitDebugSerial();			// initialize serial port

    OEMWriteDebugString(TEXT("\r\nSysInit: "));

    NKDbgPrintfW(L"GDTBase=%8.8lx IDTBase=%8.8lx KData=%8.8lx\r\n", &g_aGlobalDescriptorTable, &g_aIntDescTable, &KData);

    /* Switch to our GDT now that kernel relocation is done */
    __asm {
        lgdt    [g_KernelGDTBase]
        push    KGDT_R0_CODE
        push    offset OurCSIsLoadedNow
        retf
OurCSIsLoadedNow:
        mov     eax, KGDT_R0_DATA
        mov     ss, ax
        mov     eax, KGDT_R3_DATA
        mov     ds, ax
        mov     es, ax
    }

    // Initialize the interrupt dispatch tables
    InitializeTaskStates();
    InitializeIDT();

    OEMWriteDebugString(TEXT("Windows CE Kernel for i486 Built on ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n"));

    //  Initialize the page tables and the floating point unit
    InitializePageTables();
    OEMInit();			// initialize firmware
	KernelFindMemory();
    OEMWriteDebugString(TEXT("X86Init done.\r\n"));
}

