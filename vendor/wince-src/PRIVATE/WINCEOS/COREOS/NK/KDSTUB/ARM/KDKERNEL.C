/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdkernel.c

Abstract:

    This module contains code that somewhat emulates KiDispatchException

--*/

#include "kdp.h"



void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx)
{
    pCtx->R0 = pCpuCtx->R0;
    pCtx->R1 = pCpuCtx->R1;
    pCtx->R2 = pCpuCtx->R2;
    pCtx->R3 = pCpuCtx->R3;
    pCtx->R4 = pCpuCtx->R4;
    pCtx->R5 = pCpuCtx->R5;
    pCtx->R6 = pCpuCtx->R6;
    pCtx->R7 = pCpuCtx->R7;
    pCtx->R8 = pCpuCtx->R8;
    pCtx->R9 = pCpuCtx->R9;
    pCtx->R10 = pCpuCtx->R10;
    pCtx->R11 = pCpuCtx->R11;
    pCtx->R12 = pCpuCtx->R12;
    pCtx->Sp = pCpuCtx->Sp;
    pCtx->Lr = pCpuCtx->Lr;
    pCtx->Pc = pCpuCtx->Pc;
    pCtx->Psr = pCpuCtx->Psr;
///    pCtx->ContextFlags = pCpuCtx->ContextFlags;
    pCtx->ContextFlags = CONTEXT_FULL;
}

