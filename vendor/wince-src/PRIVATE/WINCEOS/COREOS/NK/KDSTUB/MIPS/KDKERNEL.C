/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdkernel.c

Abstract:

    This module contains code that somewhat emulates KiDispatchException

--*/

#include "kdp.h"


void ContextToCpuContext(CPUCONTEXT *pCpuCtx, CONTEXT *pCtx)
{
    pCpuCtx->IntAt = pCtx->IntAt;
    pCpuCtx->IntV0 = pCtx->IntV0;
    pCpuCtx->IntV1 = pCtx->IntV1;
    pCpuCtx->IntA0 = pCtx->IntA0;
    pCpuCtx->IntA1 = pCtx->IntA1;
    pCpuCtx->IntA2 = pCtx->IntA2;
    pCpuCtx->IntA3 = pCtx->IntA3;
    pCpuCtx->IntT0 = pCtx->IntT0;
    pCpuCtx->IntT1 = pCtx->IntT1;
    pCpuCtx->IntT2 = pCtx->IntT2;
    pCpuCtx->IntT3 = pCtx->IntT3;
    pCpuCtx->IntT4 = pCtx->IntT4;
    pCpuCtx->IntT5 = pCtx->IntT5;
    pCpuCtx->IntT6 = pCtx->IntT6;
    pCpuCtx->IntT7 = pCtx->IntT7;
    pCpuCtx->IntS0 = pCtx->IntS0;
    pCpuCtx->IntS1 = pCtx->IntS1;
    pCpuCtx->IntS2 = pCtx->IntS2;
    pCpuCtx->IntS3 = pCtx->IntS3;
    pCpuCtx->IntS4 = pCtx->IntS4;
    pCpuCtx->IntS5 = pCtx->IntS5;
    pCpuCtx->IntS6 = pCtx->IntS6;
    pCpuCtx->IntS7 = pCtx->IntS7;
    pCpuCtx->IntT8 = pCtx->IntT8;
    pCpuCtx->IntT9 = pCtx->IntT9;
    pCpuCtx->IntK0 = pCtx->IntK0;
    pCpuCtx->IntK1 = pCtx->IntK1;
    pCpuCtx->IntGp = pCtx->IntGp;
    pCpuCtx->IntSp = pCtx->IntSp;
    pCpuCtx->IntS8 = pCtx->IntS8;
    pCpuCtx->IntRa = pCtx->IntRa;
    pCpuCtx->IntLo = pCtx->IntLo;
    pCpuCtx->IntHi = pCtx->IntHi;

    pCpuCtx->Fsr = pCtx->Fsr;

    pCpuCtx->Fir = pCtx->Fir;
    pCpuCtx->Psr = pCtx->Psr;


    pCpuCtx->ContextFlags = pCtx->ContextFlags;

}


void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx)
{
    memset(pCtx, 0, sizeof(CONTEXT));

    pCtx->IntZero = 0;
    pCtx->IntAt = pCpuCtx->IntAt;
    pCtx->IntV0 = pCpuCtx->IntV0;
    pCtx->IntV1 = pCpuCtx->IntV1;
    pCtx->IntA0 = pCpuCtx->IntA0;
    pCtx->IntA1 = pCpuCtx->IntA1;
    pCtx->IntA2 = pCpuCtx->IntA2;
    pCtx->IntA3 = pCpuCtx->IntA3;
    pCtx->IntT0 = pCpuCtx->IntT0;
    pCtx->IntT1 = pCpuCtx->IntT1;
    pCtx->IntT2 = pCpuCtx->IntT2;
    pCtx->IntT3 = pCpuCtx->IntT3;
    pCtx->IntT4 = pCpuCtx->IntT4;
    pCtx->IntT5 = pCpuCtx->IntT5;
    pCtx->IntT6 = pCpuCtx->IntT6;
    pCtx->IntT7 = pCpuCtx->IntT7;
    pCtx->IntS0 = pCpuCtx->IntS0;
    pCtx->IntS1 = pCpuCtx->IntS1;
    pCtx->IntS2 = pCpuCtx->IntS2;
    pCtx->IntS3 = pCpuCtx->IntS3;
    pCtx->IntS4 = pCpuCtx->IntS4;
    pCtx->IntS5 = pCpuCtx->IntS5;
    pCtx->IntS6 = pCpuCtx->IntS6;
    pCtx->IntS7 = pCpuCtx->IntS7;
    pCtx->IntT8 = pCpuCtx->IntT8;
    pCtx->IntT9 = pCpuCtx->IntT9;
    pCtx->IntK0 = pCpuCtx->IntK0;
    pCtx->IntK1 = pCpuCtx->IntK1;
    pCtx->IntGp = pCpuCtx->IntGp;
    pCtx->IntSp = pCpuCtx->IntSp;
    pCtx->IntS8 = pCpuCtx->IntS8;
    pCtx->IntRa = pCpuCtx->IntRa;
    pCtx->IntLo = pCpuCtx->IntLo;
    pCtx->IntHi = pCpuCtx->IntHi;

    pCtx->Fsr = pCpuCtx->Fsr;

    pCtx->Fir = pCpuCtx->Fir;
    pCtx->Psr = pCpuCtx->Psr;


    pCtx->ContextFlags = pCpuCtx->ContextFlags;

}


VOID
DumpKdContext(
    IN CONTEXT *ContextRecord
    )
{
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntZero = %8.8lx\r\n"),  ContextRecord->IntZero));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntAt = %8.8lx\r\n"),  ContextRecord->IntAt));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntV0 = %8.8lx\r\n"),  ContextRecord->IntV0));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntV1 = %8.8lx\r\n"),  ContextRecord->IntV1));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntA0 = %8.8lx\r\n"),  ContextRecord->IntA0));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntA1 = %8.8lx\r\n"),  ContextRecord->IntA1));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntA2 = %8.8lx\r\n"),  ContextRecord->IntA2));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntA3 = %8.8lx\r\n"),  ContextRecord->IntA3));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT0 = %8.8lx\r\n"),  ContextRecord->IntT0));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT1 = %8.8lx\r\n"),  ContextRecord->IntT1));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT2 = %8.8lx\r\n"),  ContextRecord->IntT2));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT3 = %8.8lx\r\n"),  ContextRecord->IntT3));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT4 = %8.8lx\r\n"),  ContextRecord->IntT4));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT5 = %8.8lx\r\n"),  ContextRecord->IntT5));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT6 = %8.8lx\r\n"),  ContextRecord->IntT6));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT7 = %8.8lx\r\n"),  ContextRecord->IntT7));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS0 = %8.8lx\r\n"),  ContextRecord->IntS0));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS1 = %8.8lx\r\n"),  ContextRecord->IntS1));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS2 = %8.8lx\r\n"),  ContextRecord->IntS2));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS3 = %8.8lx\r\n"),  ContextRecord->IntS3));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS4 = %8.8lx\r\n"),  ContextRecord->IntS4));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS5 = %8.8lx\r\n"),  ContextRecord->IntS5));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS6 = %8.8lx\r\n"),  ContextRecord->IntS6));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS7 = %8.8lx\r\n"),  ContextRecord->IntS7));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT8 = %8.8lx\r\n"),  ContextRecord->IntT8));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntT9 = %8.8lx\r\n"),  ContextRecord->IntT9));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntK0 = %8.8lx\r\n"),  ContextRecord->IntK0));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntK1 = %8.8lx\r\n"),  ContextRecord->IntK1));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntGp = %8.8lx\r\n"),  ContextRecord->IntGp));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntSp = %8.8lx\r\n"),  ContextRecord->IntSp));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntS8 = %8.8lx\r\n"),  ContextRecord->IntS8));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntRa = %8.8lx\r\n"),  ContextRecord->IntRa));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntLo = %8.8lx\r\n"),  ContextRecord->IntLo));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("IntHi = %8.8lx\r\n"),  ContextRecord->IntHi));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("Fsr = %8.8lx\r\n"),  ContextRecord->Fsr));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("Fir   = %8.8lx\r\n"),  ContextRecord->Fir));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("Psr   = %8.8lx\r\n"),  ContextRecord->Psr));
        DEBUGGERMSG(ZONE_DEBUGGER, (TEXT("Context Flags = %8.8lx\r\n"), ContextRecord->ContextFlags));
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

    for (i = 0; i < 200000 * Seconds; i++)
        ;
}
