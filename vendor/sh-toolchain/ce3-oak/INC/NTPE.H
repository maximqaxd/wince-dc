/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

ntpe.h

Abstract:  

Windows CE Emulation on NT - Functions to Access PEPCMCIA.SYS NT Driver

Notes: 


--*/


#ifndef _ntpe_h_
#define _ntpe_h_

#ifdef __cplusplus
extern "C" {
#endif

// To open and close the pepcmcia device.
extern BOOL ntpe_open(void);
extern BOOL ntpe_close(void);

// Convenience functions to read/write 82365 registers.
extern UINT8 PD365Read(UINT8 index);
extern UINT8 PD365Write(UINT8 index, UINT8 value);

// To map and unmap physical memory.
extern PVOID ntpe_mapmem(PVOID physAdr, int len);
extern BOOL ntpe_unmapmem(PVOID virtAdr);

// To get the ISA windows values.
extern BOOL ntpe_getvals(ULONG *AttrMemWinStart, ULONG *AttrMemWinLength, 
	ULONG *CommMemWinStart, ULONG *CommMemWinLength);

// For debugging.
extern void ntpe_writebytes(PUCHAR adr, PUCHAR src, int len); 
extern void ntpe_readbytes(PUCHAR adr, int len); 

// Interrupt functions (do nothing now).
extern BOOL SetInterruptEvent();
extern void InterruptDisable();
extern BOOL InterruptInitialize();
extern void InterruptDone();

#ifdef __cplusplus
}
#endif

#endif // _ntpe_h_

