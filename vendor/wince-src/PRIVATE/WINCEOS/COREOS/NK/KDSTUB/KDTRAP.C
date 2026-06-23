/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

--*/

#include "kdp.h"

extern PVOID KdpImageBase;
extern BOOL fDbgConnected;
extern struct KDataStruct	*kdpKData;

CRITICAL_SECTION csDbg;

void FlushDCache(void);


ULONG
KdpTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN CONTEXT *ContextRecord,
//    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the kernel
    debugger is active.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    BOOLEAN bRet=TRUE;
    KD_SYMBOLS_INFO SymbolInfo;
    STRING ImageName;
    ULONG ImageSize, OldFir;
    ACCESSKEY ulOldKey;
    NAMEANDBASE NameandBase;
    DWORD BreakCode;
    KD_EXCEPTION_INFO kei;

    if (!fDbgConnected)
        return FALSE;

    //
    // If this is a breakpoint instruction, determine what kind
    //

    SWITCHKEY(ulOldKey,0xffffffff);

    if (!InSysCall()) {
        EnterCriticalSection(&csDbg);
    }

    if (InterlockedIncrement(&kdpKData->dwInDebugger) > 1)
    {
        NKOtherPrintfW(L"Already in debugger !!!\r\n");
        bRet=FALSE;
//        goto exit;
    }

    OldFir=CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    if ((ZeroPtr(CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)) > (1 << VA_SECTION)) ||
        (ZeroPtr(CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)) < (DWORD)DllLoadBase)) {
         CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)=(UINT)MapPtrProc(CONTEXT_TO_PROGRAM_COUNTER(ContextRecord), pCurProc);
    }

	if (pKDIoControl) {
		kei.nVersion = 1;
		kei.ulAddress = (ULONG)ExceptionRecord->ExceptionAddress;
		kei.ulExceptionCode = ExceptionRecord->ExceptionCode;
		kei.ulFlags = 0;
		if (KCall( pKDIoControl, KD_IOCTL_MAP_EXCEPTION, &kei, sizeof(KD_EXCEPTION_INFO)))
			ExceptionRecord->ExceptionCode = kei.ulExceptionCode;
	}


#ifdef MIPS
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
	  (ExceptionRecord->ExceptionInformation[0] > DEBUGBREAK_UNLOAD_SYMBOLS_BREAKPOINT)) {
#if defined(MIPS16SUPPORT)
		if (CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) & 1) {
			BreakCode = (ExceptionRecord->ExceptionInformation[0] >> 5) & 0x3f;
        } else {
			BreakCode = (ExceptionRecord->ExceptionInformation[0] >> 16) & 0x3FF;
	    }
#else
		BreakCode = (ExceptionRecord->ExceptionInformation[0] >> 16) & 0x3FF;
#endif
	} else {
		BreakCode = ExceptionRecord->ExceptionInformation[0];
	}
    DEBUGGERMSG( KDZONE_TRAP, (TEXT("KDTrap Exception at %08X Breakcode = %08X\r\n"),
        CONTEXT_TO_PROGRAM_COUNTER(ContextRecord),
        BreakCode));
#else
    BreakCode = ExceptionRecord->ExceptionInformation[0];
    DEBUGGERMSG( KDZONE_TRAP, (TEXT("KDTrap Exception at %08X Breakcode = %08X ExceptionCode=%08X\r\n"),
        CONTEXT_TO_PROGRAM_COUNTER(ContextRecord),
        ExceptionRecord->ExceptionInformation[0],
        ExceptionRecord->ExceptionCode));
#endif


   //
   // If this is a breakpoint instruction, determine what kind
   //
   
   if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
        (ExceptionRecord->ExceptionCode == STATUS_SYSTEM_BREAK)) {
        //
        // Switch on the breakpoint code.
        //

        switch (BreakCode) {
        case DEBUGBREAK_LOAD_SYMBOLS_BREAKPOINT:
		case DEBUGBREAK_UNLOAD_SYMBOLS_BREAKPOINT:
           if (!GetNameandImageBase(pCurProc,
                                    CONTEXT_TO_PROGRAM_COUNTER(ContextRecord),
                                    &NameandBase,
                                    TRUE,
                                    BreakCode)) {
#ifdef MIPS
#if defined(MIPS16SUPPORT)
               if (CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) & 1) {
                   CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=2;
               } else {
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=4;
               }
#else
               CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=4;
#endif
#elif defined(SHx)
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=2;
#elif defined(x86)
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=2;
#elif defined(PPC)
		CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)+=4;
#elif defined(ARM)

#if defined(THUMBSUPPORT)
                if ( CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) & 0x01 ){
                    CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) += 2;
                } else
#endif
				    CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) += 4;

#endif
                goto exit;
           }

           ImageName.Buffer = (PCHAR)NameandBase.szName;
           ImageName.Length = strlen(ImageName.Buffer);
           KdpImageBase = (PVOID)NameandBase.ImageBase;
           ImageSize = NameandBase.ImageSize;

           SymbolInfo.BaseOfDll = KdpImageBase;
           SymbolInfo.SizeOfImage = ImageSize;
           SymbolInfo.ProcessId = (ULONG)-1;
           SymbolInfo.CheckSum = 0;
           bRet = BreakCode == DEBUGBREAK_UNLOAD_SYMBOLS_BREAKPOINT ? TRUE:FALSE;
           if (!KdpReportLoadSymbolsStateChange(&ImageName,
                                                &SymbolInfo,
                                                bRet,
                                                ContextRecord)) {
               DEBUGGERMSG(KDZONE_TRAP, (L"Load symbols failed\n"));
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)=OldFir;
                bRet=FALSE;
                goto exit;
           } else {
           		bRet = TRUE;
			}

            goto exit;

        case DEBUGBREAK_STOP_BREAKPOINT:
            break;

        case DEBUG_PROCESS_SWITCH_BREAKPOINT:
            DEBUGGERMSG(ZONE_DEBUGGER,(L"Process switch\r\n"));
            break;

        case DEBUG_THREAD_SWITCH_BREAKPOINT:
            DEBUGGERMSG(ZONE_DEBUGGER,(L"Thread switch\r\n"));
            ClearThreadSwitch(ContextRecord);
            break;

        case DEBUG_BREAK_IN:
            DEBUGGERMSG(ZONE_DEBUGGER,(L"Hit black button\r\n"));
            break;
        default:
            break;
        }
    }

    // copy over the floating point registers from the thread context if necessary
#if defined(SH4)
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(ContextRecord->Psr), (PCHAR)&(pCurThread->ctx.Psr),sizeof(DWORD));
    KdpQuickMoveMemory((PCHAR)&(ContextRecord->Fpscr), (PCHAR)&(pCurThread->ctx.Fpscr),sizeof(DWORD)*34);
#elif defined(MIPS_HAS_FPU)
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(ContextRecord->FltF0), (PCHAR)&(pCurThread->ctx.FltF0),sizeof(DWORD)*32);
#endif

    if (KdpUseTCPSockets || KdpUseUDPSockets) {
        if (pCurThread->pProc != pCurThread->pOwnerProc) {
            // we are in some sort of PSL
            bRet = FALSE;
            goto exit;
        }

        if (CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) & 0x80000000) {
            // we are in some unmapped code (ie:kernel)
            bRet = FALSE;
            goto exit;
        }
#if 0
        if (CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) is in ROM) {
            // we are in ROM
            bRet = FALSE;
            goto exit;
        }
#endif
    }

    DEBUGGERMSG(KDZONE_TRAP,(TEXT("Exception StateChange at %08X\r\n"),CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)));
    if (!(bRet = KdpReportExceptionStateChange(ExceptionRecord,
                                       ContextRecord,
                                       SecondChance)))
        CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)=OldFir;

    CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)=ZeroPtr(CONTEXT_TO_PROGRAM_COUNTER(ContextRecord));

exit:
#ifdef MIPS
    FlushICache();
#elif defined(SHx)
    FlushCache();
#elif defined(PPC)
    FlushDCache();
    FlushICache();
#elif defined(ARM)
	DEBUGGERMSG(KDZONE_TRAP,(L"KdTrap, FlushD/ICache.\r\n"));
    FlushDCache();
    FlushICache();
#endif
	SETCURKEY(ulOldKey);

    InterlockedDecrement(&kdpKData->dwInDebugger);
    if (!InSysCall())
        LeaveCriticalSection(&csDbg);
	DEBUGGERMSG(KDZONE_TRAP, (L"Exit KdTrap\r\n"));
    return bRet;
}


void UpdateSymbols(DWORD dwAddr, BOOL UnloadSymbols)
{
	EXCEPTION_RECORD er;
	CONTEXT cr;

	memset(&er, 0, sizeof(er));
	memset(&cr, 0, sizeof(cr));
	er.ExceptionAddress = (PVOID)dwAddr;
	er.ExceptionInformation[0] = UnloadSymbols ? DEBUGBREAK_UNLOAD_SYMBOLS_BREAKPOINT : DEBUGBREAK_LOAD_SYMBOLS_BREAKPOINT;
	DEBUGGERMSG(KDZONE_TRAP,(TEXT("Kdtrap, UpdateSymbols, er.ExceptionInformation[0] = %08X\r\n"),er.ExceptionInformation[0]));
	er.ExceptionCode = STATUS_BREAKPOINT;
	CONTEXT_TO_PROGRAM_COUNTER(&cr) = dwAddr;
	KdpTrap(&er, &cr, FALSE);
	DEBUGGERMSG(KDZONE_TRAP, (L"Exit UpdateSymbols\r\n"));
}



