/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdtran.c

Abstract:

    This module implements the I/O comunications for the serial transport.

Revision History:

--*/
#include <windows.h>
#include <ethdbg.h>
#include <halether.h>
#include "kdp.h"

#if 0
#define DBGPRT(cp) OEMWriteOtherDebugString(cp)
#else
#define DBGPRT(cp)
#endif

#define MAXIMUM_RETRIES 1024*128

BOOL KdpUseTCPSockets;
BOOL KdpUseUDPSockets;

BOOL KdpUseEdbg;

static BOOL fIntsOff;

// Pointer to buffer for formatting KDBG commands to send over ether.  Is set in the
// EdbgRegisterDfltClient function. Size of buffer is EDBG_MAX_DATA_SIZE.
static UCHAR *KdbgEthBuf;

static int  (*ReadDebugByte)()          = OEMReadDebugByte;
static void (*WriteDebugByte)(BYTE ch)  = OEMWriteDebugByte;

// Locals for handling translation of the packet based ethernet commands to the byte read and
// write commands used by kdstub.
static UCHAR *pWritePtr, *pReadPtr;
static DWORD dwRemainingBytes;
static DWORD dwBytesWrittenToBuffer;

// Fill receive buffer - note that we must call the EDBG routines with interrupts
// disabled, since for the debugger we can't use critical sections or KCalls to
// protect accesses to the hardware (we can't block, and KCalls turn on ints
// for some systems).
BOOL FillEdbgBuffer()
{
    DWORD dwLen = EDBG_MAX_DATA_SIZE;
    // Note that we can't trust our flag to tell us whether ints are enabled
    // at this point - if the debugger needs to access the loader for example
    // for a page fault, ints may be turned on by the kernel.
    INTERRUPTS_OFF();
    if (!pEdbgRecv(EDBG_SVC_KDBG, KdbgEthBuf,&dwLen, 0)) {
        if (!fIntsOff) {
            INTERRUPTS_ON();
        }
        return FALSE;
    }
    dwRemainingBytes = dwLen;
    pReadPtr = KdbgEthBuf;
    if (!fIntsOff) {
        INTERRUPTS_ON();
    }
    return TRUE;
}

BOOL WriteEdbgBuffer()
{
    INTERRUPTS_OFF();
    if (dwBytesWrittenToBuffer && !pEdbgSend(EDBG_SVC_KDBG,KdbgEthBuf,dwBytesWrittenToBuffer)) {
        if (!fIntsOff) {
            INTERRUPTS_ON();
        }
        return FALSE;
    }
    dwBytesWrittenToBuffer = 0;
    pWritePtr = KdbgEthBuf;

    if (!fIntsOff) {
        INTERRUPTS_ON();
    }
    return TRUE;
}

static void send_byte_ether(BYTE ch)
{
    *pWritePtr++ = ch;
    if (++dwBytesWrittenToBuffer == EDBG_MAX_DATA_SIZE)
        WriteEdbgBuffer();
}

static int get_byte_ether()
{
    // Refill buffer if necessary and return next byte
    if (dwRemainingBytes == 0) 
         if (!FillEdbgBuffer())
             return OEM_DEBUG_READ_NODATA;
    dwRemainingBytes--;
    return *pReadPtr++;
}

// Attempt to switch Kdbg over to ethernet.  Will fail if no support
// for this in the HAL.
BOOL SwitchKdbgToEther(BOOL ToEther)
{
    BOOL Ret=TRUE;
    if (ToEther) {
        if (pEdbgRegisterDfltClient &&
            pEdbgRegisterDfltClient(EDBG_SVC_KDBG,0,&KdbgEthBuf,&KdbgEthBuf)) {
            dwBytesWrittenToBuffer = 0;
            pWritePtr = KdbgEthBuf;            
            KdpUseEdbg = TRUE;
            ReadDebugByte  = get_byte_ether;
            WriteDebugByte = send_byte_ether;            
        }
        else 
            Ret = KdpUseEdbg = FALSE;
    }
    else {
        ReadDebugByte  = OEMReadDebugByte;
        WriteDebugByte = OEMWriteDebugByte;
    }
    return Ret;
}

/*++

Routine Description:

    This routine gets a byte from the serial port used by the kernel
    debugger.

Arguments:

    Input - Supplies a pointer to a variable that receives the input
    data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
    kernel debugger line.

    CP_GET_ERROR is returned if an error is encountered during reading.

    CP_GET_NODATA is returned if timeout occurs.

--*/
USHORT KdPortGetByte(OUT PUCHAR Input)
{
    int ret;
    UINT limitcount = MAXIMUM_RETRIES;

    while (limitcount != 0) {
        limitcount--;

        ret = ReadDebugByte();
        if (ret == OEM_DEBUG_COM_ERROR)
            return CP_GET_ERROR;
        else if (ret != OEM_DEBUG_READ_NODATA)
        {
            *Input=(unsigned char)ret;
            return CP_GET_SUCCESS;
        }
    }
    return CP_GET_NODATA;
}


/*++

Routine Description:

    This routine puts a byte to the serial port used by the kernel debugger.

Arguments:

    Output - Supplies the output data byte.

Return Value:

    None.

--*/
VOID KdPortPutByte(IN UCHAR Output)
{
    WriteDebugByte(Output);
}


/*++

Routine Description:

    This routine clears the channel B error flag

Arguments:

    None.

Return Value:

    None.

--*/
VOID KdClearCommError(VOID) 
{
    OEMClearDebugCommError();
}


VOID KdInterruptsOff(VOID) 
{
    fIntsOff = TRUE;
    INTERRUPTS_OFF();
}


VOID KdInterruptsOn(VOID) 
{
    fIntsOff = FALSE;
    INTERRUPTS_ON();
}


VOID KdInitTransport(VOID) 
{
    // This routine does not do anything when using serial transport.
}


VOID KdTerminateTransport(VOID) 
{
    // This routine does not do anything when using serial transport.
}

