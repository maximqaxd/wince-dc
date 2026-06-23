/*
 * serial.c - Dreamcast OAL serial glue.
 * SerialInit hooks the SCIF interrupt vectors (0x28..2B) as in the shipped OEMInit.
 * We drive the debug console POLLED (dbgserial.c), so SCIFISR is a stub. The
 * parallel-port debug path (shipped: ASE Debug Adapter) is unused -> NoPPFS=1, stubs.
 */
#include <windows.h>
#include "dc_hw.h"

#define SYSINTR_NOP 0

extern BOOL HookInterrupt(int idInt, FARPROC pfn);
ULONG SCIFISR(void);

/* 1 => no parallel-port file system / debug link present (we use SCIF). */
DWORD NoPPFS = 1;

void SerialInit(void)
{
    HookInterrupt(INTRVEC_SCIF + 0, (FARPROC)SCIFISR);   /* ERI */
    HookInterrupt(INTRVEC_SCIF + 1, (FARPROC)SCIFISR);   /* RXI */
    HookInterrupt(INTRVEC_SCIF + 2, (FARPROC)SCIFISR);   /* BRI */
    HookInterrupt(INTRVEC_SCIF + 3, (FARPROC)SCIFISR);   /* TXI */
}

/* No interrupt-driven serial during bring-up; debug I/O is polled. */
ULONG SCIFISR(void)
{
    return SYSINTR_NOP;
}

/* Parallel-port debug link (ASE Debug Adapter) - unused; stubbed. */
int  OEMParallelPortInit(void)            { NoPPFS = 1; return 1; }
int  OEMParallelPortGetByte(void)         { return -1; }
void OEMParallelPortSendByte(BYTE ch)     { }

/* Debug comm error clear (KITL/debugger path) - unused. */
void OEMClearDebugCommError(void)         { }
