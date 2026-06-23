/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdinit.c

Abstract:

    This module implements the initialization for the portable kernel debgger.

Revision History:

--*/

#include "kdp.h"

//
// Global Data
//

BOOLEAN KdDebuggerEnabled;
KDP_BREAKPOINT_TYPE KdpBreakpointInstruction;
ULONG KdpOweBreakpoint;
ULONG KdpNextPacketIdToSend;
ULONG KdpPacketIdExpected;
PVOID KdpImageBase;           
LPVOID lpPrintString = NULL;

extern CRITICAL_SECTION csDbg;
extern BOOL fDbgConnected;
extern UCHAR CommDev[];



void SetKernLoadAddress(void);

/* Kernel Debugger interface pointers */
// added one more arg arguments
extern BOOLEAN KdInit(LPVOID *, LPVOID *, LPVOID *,LPVOID,LPVOID *,LPVOID *);
BOOLEAN (*PKDInit)(LPVOID *, LPVOID *, LPVOID *,LPVOID,LPVOID *,LPVOID *) = KdInit;


// KdpCleanup gets initialised to KDCleanup() in kwin32.c when KdInit() is called.
// To start with, it points to a dummy function
BOOLEAN FakeCleanup(void);
BOOLEAN (*KdpCleanup)(void)=FakeCleanup;
// end

// These 2 functions are called to set and reset LoadSYmbolsFlag from SC_ConnectDebugger
// to force LoadSymbol break points. 
char LoadSymbolsFlag=0;
BOOLEAN SetLoadSymbolsFlag();
BOOLEAN ResetLoadSymbolsFlag();
//end


extern void UpdateSymbols(DWORD dwAddr, BOOL bUnload);
extern BOOLEAN (*KDPrintString)(IN LPCWSTR Output);
extern void (*KDUpdateSymbols)(DWORD dwAddr, BOOL bUnload);

// added argument p4
BOOLEAN
KdInit(LPVOID *p1, LPVOID *p2, LPVOID *p3,LPVOID p4,LPVOID *p5,LPVOID *p6) {

/*++

Routine Description:

    This routine initializes the portable kernel debugger.

Arguments:

    None.

Return Value:

    None.

--*/

    ULONG Index;
#if !defined(MIPS16SUPPORT) && !defined(THUMBSUPPORT)
    KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
#endif
    KdpOweBreakpoint = FALSE;

    //
    // Initialize the breakpoint table.
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
        KdpBreakpointTable[Index].Flags = 0L;
        KdpBreakpointTable[Index].Address = NULL;
        KdpBreakpointTable[Index].KAddress = NULL;
        KdpBreakpointTable[Index].DirectoryTableBase = 0L;
    }

    KeSweepCurrentIcache();

    KdDebuggerEnabled = TRUE;

    //
    // Initialize ID for NEXT packet to send and Expect ID of next incoming
    // packet.
    //

    KdpNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
    KdpPacketIdExpected = INITIAL_PACKET_ID;

#ifdef SHx
    
    //
    // Initialize the registers belonging to the User Break Controller
    //

    WRITE_REGISTER_ULONG(UBCBarA, 0);
    WRITE_REGISTER_UCHAR(UBCBasrA, 0);
    WRITE_REGISTER_UCHAR(UBCBamrA, 0);
    WRITE_REGISTER_USHORT(UBCBbrA, 0); 
    WRITE_REGISTER_ULONG(UBCBarB, 0);
    WRITE_REGISTER_UCHAR(UBCBasrB, 0);
    WRITE_REGISTER_UCHAR(UBCBamrB, 0);
    WRITE_REGISTER_USHORT(UBCBbrB, 0); 
    WRITE_REGISTER_ULONG(UBCBdrB, 0);
    WRITE_REGISTER_ULONG(UBCBdmrB, 0);
    WRITE_REGISTER_USHORT(UBCBrcr, 0);
#endif

    InitializeCriticalSection(&csDbg);
    *p1 = (LPVOID)(lpPrintString ? lpPrintString : KdpPrintString);
    *p2 = (LPVOID)UpdateSymbols;
    *p3 = (LPVOID)KdpTrap;

// KdpCleanup now points to KDCleanup() in kwin32.c
        KdpCleanup=(BOOLEAN (*)(void))p4;
// end

        *p5=(LPVOID)SetLoadSymbolsFlag;
        *p6=(LPVOID)ResetLoadSymbolsFlag;
//end

    // Check whether to switch to EDBG for kernel debugging using the
    // debug Ethernet board.  This flag is set by the OEM by calling
    // SetKernelCommDev(KERNEL_SVC_KDBG, KERNEL_COMM_ETHER) in OEMInit.
    if (CommDev[KERNEL_SVC_KDBG] == KERNEL_COMM_ETHER)
        SwitchKdbgToEther(TRUE);
        
    SetKernLoadAddress();
    fDbgConnected = TRUE;
#ifdef DBGOTHER
//    ppshfile = U_ropen (TEXT("kdlog.log"), _O_CREAT | _O_TRUNC | _O_WRONLY);
    ppshfile = U_ropen (TEXT("con"), 10000);
#endif
    return(TRUE);

}

BOOLEAN FakeCleanup(void)
{
        return TRUE;
}
// end

BOOLEAN SetLoadSymbolsFlag()
{
        LoadSymbolsFlag=1;
        return TRUE;
}

BOOLEAN ResetLoadSymbolsFlag()
{
        LoadSymbolsFlag=0;
        return TRUE;
}

