/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdkernel.c

Abstract:
--*/

#include "kdp.h"



void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx)
{
    SHxCOMPILERPDATABug;
	
    *pCtx = *pCpuCtx;
    pCtx->ContextFlags = CONTEXT_FULL;
}

