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
    pCpuCtx->Gpr0 = pCtx->Gpr0;
    pCpuCtx->Gpr1 = pCtx->Gpr1;
    pCpuCtx->Gpr2 = pCtx->Gpr2;
    pCpuCtx->Gpr3 = pCtx->Gpr3;
    pCpuCtx->Gpr4 = pCtx->Gpr4;
    pCpuCtx->Gpr5 = pCtx->Gpr5;
    pCpuCtx->Gpr6 = pCtx->Gpr6;
    pCpuCtx->Gpr7 = pCtx->Gpr7;
    pCpuCtx->Gpr8 = pCtx->Gpr8;
    pCpuCtx->Gpr9 = pCtx->Gpr9;
    pCpuCtx->Gpr10 = pCtx->Gpr10;
    pCpuCtx->Gpr11 = pCtx->Gpr11;
    pCpuCtx->Gpr12 = pCtx->Gpr12;
    pCpuCtx->Gpr13 = pCtx->Gpr13;
    pCpuCtx->Gpr14 = pCtx->Gpr14;
    pCpuCtx->Gpr15 = pCtx->Gpr15;
    pCpuCtx->Gpr16 = pCtx->Gpr16;
    pCpuCtx->Gpr17 = pCtx->Gpr17;
    pCpuCtx->Gpr18 = pCtx->Gpr18;
    pCpuCtx->Gpr19 = pCtx->Gpr19;
    pCpuCtx->Gpr20 = pCtx->Gpr20;
    pCpuCtx->Gpr21 = pCtx->Gpr21;
    pCpuCtx->Gpr22 = pCtx->Gpr22;
    pCpuCtx->Gpr23 = pCtx->Gpr23;
    pCpuCtx->Gpr24 = pCtx->Gpr24;
    pCpuCtx->Gpr25 = pCtx->Gpr25;
    pCpuCtx->Gpr26 = pCtx->Gpr26;
    pCpuCtx->Gpr27 = pCtx->Gpr27;
    pCpuCtx->Gpr28 = pCtx->Gpr28;
    pCpuCtx->Gpr29 = pCtx->Gpr29;
    pCpuCtx->Gpr30 = pCtx->Gpr30;
    pCpuCtx->Gpr31 = pCtx->Gpr31;

    pCpuCtx->Cr = pCtx->Cr;
    pCpuCtx->Xer = pCtx->Xer;
    pCpuCtx->Msr = pCtx->Msr;
    pCpuCtx->Iar = pCtx->Iar;
    pCpuCtx->Lr = pCtx->Lr;
    pCpuCtx->Ctr = pCtx->Ctr;
}


void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx)
{
    memset(pCtx, 0, sizeof(CONTEXT));

    pCtx->Gpr0 = pCpuCtx->Gpr0;
    pCtx->Gpr1 = pCpuCtx->Gpr1;
    pCtx->Gpr2 = pCpuCtx->Gpr2;
    pCtx->Gpr3 = pCpuCtx->Gpr3;
    pCtx->Gpr4 = pCpuCtx->Gpr4;
    pCtx->Gpr5 = pCpuCtx->Gpr5;
    pCtx->Gpr6 = pCpuCtx->Gpr6;
    pCtx->Gpr7 = pCpuCtx->Gpr7;
    pCtx->Gpr8 = pCpuCtx->Gpr8;
    pCtx->Gpr9 = pCpuCtx->Gpr9;
    pCtx->Gpr10 = pCpuCtx->Gpr10;
    pCtx->Gpr11 = pCpuCtx->Gpr11;
    pCtx->Gpr12 = pCpuCtx->Gpr12;
    pCtx->Gpr13 = pCpuCtx->Gpr13;
    pCtx->Gpr14 = pCpuCtx->Gpr14;
    pCtx->Gpr15 = pCpuCtx->Gpr15;
    pCtx->Gpr16 = pCpuCtx->Gpr16;
    pCtx->Gpr17 = pCpuCtx->Gpr17;
    pCtx->Gpr18 = pCpuCtx->Gpr18;
    pCtx->Gpr19 = pCpuCtx->Gpr19;
    pCtx->Gpr20 = pCpuCtx->Gpr20;
    pCtx->Gpr21 = pCpuCtx->Gpr21;
    pCtx->Gpr22 = pCpuCtx->Gpr22;
    pCtx->Gpr23 = pCpuCtx->Gpr23;
    pCtx->Gpr24 = pCpuCtx->Gpr24;
    pCtx->Gpr25 = pCpuCtx->Gpr25;
    pCtx->Gpr26 = pCpuCtx->Gpr26;
    pCtx->Gpr27 = pCpuCtx->Gpr27;
    pCtx->Gpr28 = pCpuCtx->Gpr28;
    pCtx->Gpr29 = pCpuCtx->Gpr29;
    pCtx->Gpr30 = pCpuCtx->Gpr30;
    pCtx->Gpr31 = pCpuCtx->Gpr31;

    pCtx->Cr = pCpuCtx->Cr;
    pCtx->Xer = pCpuCtx->Xer;
    pCtx->Msr = pCpuCtx->Msr;
    pCtx->Iar = pCpuCtx->Iar;
    pCtx->Lr = pCpuCtx->Lr;
    pCtx->Ctr = pCpuCtx->Ctr;

}
