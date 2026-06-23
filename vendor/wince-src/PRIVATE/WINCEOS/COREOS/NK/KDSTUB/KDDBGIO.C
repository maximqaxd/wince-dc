/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kddbgio.c

Abstract:

    This module implements kernel debugger based Dbg I/O. This
    is the foundation for DbgPrint and DbgPrompt.

Revision History:

--*/

#include "kdp.h"

extern struct KDataStruct	*kdpKData;
extern CRITICAL_SECTION csDbg;
CHAR szBuf[MAX_PATH*2];

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpPrintString)
#endif

// Prevent HIGHADJ/LOW/LOW fixup problem
#if defined(PPC)
#pragma optimize("", off)
#endif

BOOLEAN
KdpPrintString (
    IN LPCWSTR Output
    )
/*++

Routine Description:

    This routine prints a string.

Arguments:

    Output - Supplies a pointer to a string descriptor for the output string.

Return Value:

    TRUE if Control-C present in input buffer after print is done.
    FALSE otherwise.

--*/

{
    ULONG Length=0, i=0;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_DEBUG_IO DebugIo;
    BOOL bRet=TRUE;

    if (!InSysCall())
        EnterCriticalSection(&csDbg);

    if (InterlockedIncrement(&kdpKData->dwInDebugger) > 1)
    {
//        DEBUGGERMSG(ZONE_DEBUGGER, (L"Tried to print string but already in debugger %8.8lx\r\n", kdpKData->dwInDebugger));
        bRet=FALSE;
        goto exit;
    }

    //
    // Move the output string to the message buffer.
    //

	while ((*Output) && (i<sizeof(szBuf)))
		szBuf[i++] = (CHAR)*Output++;

    i < sizeof(szBuf) ? (szBuf[i]='\0') : (szBuf[sizeof(szBuf)-1]='\0');

//    DEBUGGERMSG(ZONE_DEBUGGER, (L"Printing %a\r\n", szBuf));

    if (i>=sizeof(szBuf))
        Length=sizeof(szBuf);
    else
        Length = i+1;

    Length = KdpMoveMemory(
                (PCHAR)KdpMessageBuffer,
                szBuf,
                Length
                );

    //
    // If the total message length is greater than the maximum packet size,
    // then truncate the output string.
    //

    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    //
    // Construct the print string message and message descriptor.
    //

    DebugIo.ApiNumber = DbgKdPrintStringApi;
    DebugIo.ProcessorType = 0;
    DebugIo.Processor = (USHORT)0;
    DebugIo.u.PrintString.LengthOfString = Length;
    MessageHeader.Length = sizeof(DBGKD_DEBUG_IO);
    MessageHeader.Buffer = (PCHAR)&DebugIo;

    //
    // Construct the print string data and data descriptor.
    //

    MessageData.Length = (USHORT)Length;
    MessageData.Buffer = KdpMessageBuffer;

    //
    // Send packet to the kernel debugger on the host machine.
    //

    KdpSendPacket(
                  PACKET_TYPE_KD_DEBUG_IO,
                  &MessageHeader,
                  &MessageData
                  );

exit:
    InterlockedDecrement(&kdpKData->dwInDebugger);

    if (!InSysCall())
        LeaveCriticalSection(&csDbg);
    return bRet;
}

#if defined(PPC)
#pragma optimize("", on)
#endif
