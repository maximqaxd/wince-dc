/*++

Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdkernel.c
--*/

#include "kdp.h"


void ThreadStopFunc(void)
{
    _asm int 2
}

VOID
KeStallExecutionProcessor (
    ULONG Seconds
    )

/*++

Routine Description:

    This routine attempts to stall the processor for the specified number
    of seconds. 
    
Arguments:

    Seconds - Number of seconds to stall processor

Return Value:

    None.

--*/

{
    ULONG i;

    //
    // Since processor is running at 60 MHz. Assuming for loop is around
    // 10 lines with the instructions delaying an extra order of magnitude,
    // that means the loop has to be executed 600,000 times to delay for
    // 1 second.
    //

    for (i = 0; i < 600000 * Seconds; i++)
        ;
}

void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx)
{
	pCtx->SegGs		=pCpuCtx->TcxGs;
	pCtx->SegFs		=pCpuCtx->TcxFs;
	pCtx->SegEs		=pCpuCtx->TcxEs;
	pCtx->SegDs		=pCpuCtx->TcxDs;
	pCtx->Edi		=pCpuCtx->TcxEdi;
	pCtx->Esi		=pCpuCtx->TcxEsi;
	pCtx->Ebp		=pCpuCtx->TcxEbp;
//	pCtx->Esp		=pCpuCtx->TcxNotEsp;	???
	pCtx->Ebx		=pCpuCtx->TcxEbx;
	pCtx->Edx		=pCpuCtx->TcxEdx;
	pCtx->Ecx		=pCpuCtx->TcxEcx;
	pCtx->Eax		=pCpuCtx->TcxEax;
	pCtx->Eip		=pCpuCtx->TcxEip;
	pCtx->SegCs		=pCpuCtx->TcxCs;
	pCtx->EFlags	=pCpuCtx->TcxEFlags;
	pCtx->Esp		=pCpuCtx->TcxEsp;
	pCtx->SegSs		=pCpuCtx->TcxSs;
//  FLOATING_SAVE_AREA TcxFPU;
}	
