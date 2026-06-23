/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdapi.c

Abstract:

    Implementation of Kernel Debugger portable remote APIs.


--*/

#include "kdp.h"

extern BOOL fDbgConnected;

//
// included to find the relocated kernel data section
//
extern ROMHDR * const volatile pTOC;

// This routine is called when it receives DbgKdTerminateApi
extern BOOLEAN (*KdpCleanup)(void);
//end

extern VOID KdTerminateTransport(VOID);
extern VOID KdInterruptsOff(VOID);
extern VOID KdInterruptsOn(VOID);

DWORD NkCEProcessorType; // Temp change [GH]: was CEProcessorType

#if defined(x86)
extern PTHREAD g_CurFPUOwner;
#endif


//
// Define external data.
//

extern PVOID KdpImageBase;

CHAR *socketReady;
DWORD packetsIndex;

BOOLEAN KdpSendContext = TRUE;
BOOL fDbgConnected;
BOOL fAtBreakState;

DBGKD_MANIPULATE_STATE ManipulateState;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpSendWaitContinue)
#pragma alloc_text(PAGEKD, KdpReadVirtualMemory)
#pragma alloc_text(PAGEKD, KdpWriteVirtualMemory)
#pragma alloc_text(PAGEKD, KdpGetContext)
#pragma alloc_text(PAGEKD, KdpSetContext)
#pragma alloc_text(PAGEKD, KdpWriteBreakpoint)
#pragma alloc_text(PAGEKD, KdpRestoreBreakpoint)
#pragma alloc_text(PAGEKD, KdpReportExceptionStateChange)
#pragma alloc_text(PAGEKD, KdpReportLoadSymbolsStateChange)
#pragma alloc_text(PAGEKD, KdpReadPhysicalMemory)
#pragma alloc_text(PAGEKD, KdpWritePhysicalMemory)
#pragma alloc_text(PAGEKD, KdpGetVersion)
#pragma alloc_text(PAGEKD, KdpWriteBreakPointEx)
#pragma alloc_text(PAGEKD, KdpRestoreBreakPointEx)
#endif // ALLOC_PRAGMA


typedef struct {
    DBGKD_WAIT_STATE_CHANGE StateChange;
    USHORT AdditionalDataLength;
    CHAR AdditionalData[80];
} STATE_PACKET;

#define MAX_LOAD_NOTIFICATIONS 45 // wraps around after 45
STATE_PACKET SavedPackets[MAX_LOAD_NOTIFICATIONS];

VOID
SendSavedPackets(VOID)
{
    PSTRING AdditionalData = NULL;
    STRING MessageHeader;
    STRING MessageData;
    KCONTINUE_STATUS Status;
    CONTEXT ContextRecord;
    DWORD i;

    for (i = 0; i < (packetsIndex%MAX_LOAD_NOTIFICATIONS); i++) {
        do {
            MessageHeader.Length = sizeof(DBGKD_WAIT_STATE_CHANGE);
            MessageHeader.Buffer = (PCHAR)&SavedPackets[i].StateChange;

            if (SavedPackets[i].AdditionalDataLength) {
                MessageData.Length = SavedPackets[i].AdditionalDataLength;
                MessageData.Buffer = SavedPackets[i].AdditionalData;
                AdditionalData = &MessageData;
            }

            Status = KdpSendWaitContinue(
                PACKET_TYPE_KD_STATE_CHANGE,
                &MessageHeader,
                AdditionalData,
                &ContextRecord
                );

        } while (Status == ContinueProcessorReselected);
    }

    packetsIndex = 0;
}


KCONTINUE_STATUS
KdpSendWaitContinue (
    IN ULONG OutPacketType,
    IN PSTRING OutMessageHeader,
    IN PSTRING OutMessageData OPTIONAL,
    IN OUT CONTEXT * ContextRecord
    )

/*++

Routine Description:

    This function sends a packet, and then waits for a continue message.
    BreakIns received while waiting will always cause a resend of the
    packet originally sent out.  While waiting, manipulate messages
    will be serviced.

    A resend always resends the original event sent to the debugger,
    not the last response to some debugger command.

Arguments:

    OutPacketType - Supplies the type of packet to send.

    OutMessageHeader - Supplies a pointer to a string descriptor that describes
        the message information.

    OutMessageData - Supplies a pointer to a string descriptor that describes
        the optional message data.

    ContextRecord - Exception context

Return Value:

    A value of TRUE is returned if the continue message indicates
    success, Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    USHORT ReturnCode;
    NTSTATUS Status;

    //
    // Loop servicing state manipulation message until a continue message
    // is received.
    //

    MessageHeader.MaximumLength = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)&ManipulateState;
    MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;
    MessageData.Buffer = (PCHAR)KdpMessageBuffer;

    KdInterruptsOff();
ResendPacket:

    //
    // Send event notification packet to debugger on host.  Come back
    // here any time we see a breakin sequence.
    //
   //DEBUGGERMSG(KDZONE_API, (L"SendPacket\n"));

    KdpSendPacket(
                  OutPacketType,
                  OutMessageHeader,
                  OutMessageData
                  );
    //
    // After sending packet, if there is no response from debugger
    // AND the packet is for reporting symbol (un)load, the debugger
    // will be decalred to be not present.  Note If the packet is for
    // reporting exception, the KdpSendPacket will never stop.
    //

    if (KdDebuggerNotPresent) {
        DEBUGGERMSG(KDZONE_API, (L"Debugger dead\r\n"));
        fDbgConnected=FALSE;
        KdInterruptsOn();
        return ContinueSuccess;
    }

    fAtBreakState = TRUE;
    while (TRUE) {

        //
        // Wait for State Manipulate Packet without timeout.
        //

        do {
            ReturnCode = KdpReceivePacket(
                            PACKET_TYPE_KD_STATE_MANIPULATE,
                            &MessageHeader,
                            &MessageData,
                            &Length
                            );
            if (ReturnCode == (USHORT)KDP_PACKET_RESEND) {
                goto ResendPacket;
            }
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        //
        // Switch on the return message API number.
        //

		DEBUGGERMSG(KDZONE_API, (L"Got Api %8.8lx\r\n", ManipulateState.ApiNumber));

        switch (ManipulateState.ApiNumber) {

        case DbgKdReadVirtualMemoryApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdReadVirtualMemoryApi\r\n"));
            KdpReadVirtualMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteVirtualMemoryApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWriteVirtualMemoryApi\r\n"));
            KdpWriteVirtualMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadPhysicalMemoryApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdReadPhysicalMemoryApi\r\n"));
            KdpReadPhysicalMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWritePhysicalMemoryApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWritePhysicalMemoryApi\r\n"));
            KdpWritePhysicalMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdGetContextApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdGetContextApi\r\n"));
            KdpGetContext(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdSetContextApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdSetContextApi\r\n"));
            KdpSetContext(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteBreakPointApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWriteBreakPointApi\r\n"));
            KdpWriteBreakpoint(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdRestoreBreakPointApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdRestoreBreakPointApi\r\n"));
            KdpRestoreBreakpoint(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadControlSpaceApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdReadControlSpaceApi\r\n"));
            KdpReadControlSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteControlSpaceApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWriteControlSpaceApi\r\n"));
            KdpWriteControlSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadIoSpaceApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdReadIoSpaceApi\r\n"));
            KdpReadIoSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteIoSpaceApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWriteIoSpaceApi\r\n"));
            KdpWriteIoSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdContinueApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdContinueApi\r\n"));
            fAtBreakState = FALSE;
			DEBUGGERMSG(KDZONE_API, (L"Continuing target system %8.8lx\r\n", ManipulateState.u.Continue.ContinueStatus));
            if (NT_SUCCESS(ManipulateState.u.Continue.ContinueStatus) != FALSE) {
                KdInterruptsOn();
                return ContinueSuccess;
            } else {
                KdInterruptsOn();
                return ContinueError;
            }
            break;

        case DbgKdTerminateApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdTerminateApi\r\n"));
            KdpCleanup();
            fDbgConnected=FALSE;
            fAtBreakState = FALSE;
            KdTerminateTransport();
            KdInterruptsOn();
            return ContinueSuccess;

        case DbgKdContinueApi2:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdContinueApi2\r\n"));
            fAtBreakState = FALSE;
            if (NT_SUCCESS(ManipulateState.u.Continue2.ContinueStatus) != FALSE) {
                KdpGetStateChange(&ManipulateState,ContextRecord);
                KdInterruptsOn();
                return ContinueSuccess;
            } else {
                KdInterruptsOn();
                return ContinueError;
            }
            break;

        case DbgKdRebootApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdRebootApi\r\n"));
			if (pKDIoControl) {
				pKDIoControl( KD_IOCTL_RESET, NULL, 0);
			}
            break;

        case DbgKdGetVersionApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdGetVersionApi\r\n"));
            KdpGetVersion(&ManipulateState);
            break;

        case DbgKdCauseBugCheckApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdCauseBugCheckApi\r\n"));
            MessageData.Length = 0;
            ManipulateState.ReturnStatus = STATUS_UNSUCCESSFUL;
            KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &MessageHeader, &MessageData);
            break;

        case DbgKdWriteBreakPointExApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdWriteBreakPointExApi\r\n"));
            Status = KdpWriteBreakPointEx(&ManipulateState,
                                                  &MessageData,
                                                  ContextRecord);
            if (Status) {
                ManipulateState.ApiNumber = DbgKdContinueApi;
                ManipulateState.u.Continue.ContinueStatus = Status;
                KdInterruptsOn();
                return ContinueError;
            }
            break;

        case DbgKdRestoreBreakPointExApi:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdRestoreBreakPointExApi\r\n"));
            KdpRestoreBreakPointEx(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdManipulateBreakpoint:
			DEBUGGERMSG( KDZONE_API, (L"DbgKdManipulateBreakpoint\r\n"));
            Status = KdpManipulateBreakPoint(&ManipulateState,
                                                  &MessageData,
                                                  ContextRecord);
            if (Status) {
                ManipulateState.ApiNumber = DbgKdContinueApi;
                ManipulateState.u.Continue.ContinueStatus = Status;
                KdInterruptsOn();
                return ContinueError;
            }
            break;
            //
            // Invalid message.
            //

        default:
			DEBUGGERMSG( KDZONE_API, (L"Unknown API ??\r\n"));
            MessageData.Length = 0;
            ManipulateState.ReturnStatus = STATUS_UNSUCCESSFUL;
            KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &MessageHeader, &MessageData);
            break;
        }

    }
    KdInterruptsOn();
}

VOID
KdpReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a read virtual memory
    state manipulation message. Its function is to read virtual memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_MEMORY a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // make sure that nothing but a read memory message was transmitted
    //

    KD_ASSERT(AdditionalData->Length == 0);

    //
    // Trim transfer count to fit in a single message
    //

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);
    } else {
        Length = a->TransferCount;
    }

    AdditionalData->Length = (USHORT)KdpMoveMemory(
                                        AdditionalData->Buffer,
                                        a->TargetBaseAddress,
                                        Length
                                        );

    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    a->ActualBytesRead = AdditionalData->Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a write virtual memory
    state manipulation message. Its function is to write virtual memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_MEMORY a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;


    Length = KdpMoveMemory(
                a->TargetBaseAddress,
                AdditionalData->Buffer,
                AdditionalData->Length
                );

    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    a->ActualBytesWritten = Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
    UNREFERENCED_PARAMETER(Context);
}


VOID
KdpGetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a get context state
    manipulation message.  Its function is to return the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    STRING MessageHeader;
#if defined(SH3e) || defined(SH4)
    DEBUG_REGISTERS DebugRegisters;
#endif

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    KD_ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;
    AdditionalData->Length = sizeof(CONTEXT);


#if defined(SH3)
    Context->DebugRegisters.BarA  = READ_REGISTER_ULONG(UBCBarA);
    Context->DebugRegisters.BasrA = READ_REGISTER_UCHAR(UBCBasrA);
    Context->DebugRegisters.BamrA = READ_REGISTER_UCHAR(UBCBamrA);
    Context->DebugRegisters.BbrA  = READ_REGISTER_USHORT(UBCBbrA);
    Context->DebugRegisters.BarB  = READ_REGISTER_ULONG(UBCBarB);
    Context->DebugRegisters.BasrB = READ_REGISTER_UCHAR(UBCBasrB);
    Context->DebugRegisters.BamrB = READ_REGISTER_UCHAR(UBCBamrB);
    Context->DebugRegisters.BbrB  = READ_REGISTER_USHORT(UBCBbrB);
    Context->DebugRegisters.BdrB  = READ_REGISTER_ULONG(UBCBdrB);
    Context->DebugRegisters.BdmrB = READ_REGISTER_ULONG(UBCBdmrB);
    Context->DebugRegisters.Brcr  = READ_REGISTER_USHORT(UBCBrcr);
    Context->DebugRegisters.Align = 0;
#elif defined(SH3e) || defined(SH4)
    DebugRegisters.BarA  = READ_REGISTER_ULONG(UBCBarA);
    DebugRegisters.BasrA = READ_REGISTER_UCHAR(UBCBasrA);
    DebugRegisters.BamrA = READ_REGISTER_UCHAR(UBCBamrA);
    DebugRegisters.BbrA  = READ_REGISTER_USHORT(UBCBbrA);
    DebugRegisters.BarB  = READ_REGISTER_ULONG(UBCBarB);
    DebugRegisters.BasrB = READ_REGISTER_UCHAR(UBCBasrB);
    DebugRegisters.BamrB = READ_REGISTER_UCHAR(UBCBamrB);
    DebugRegisters.BbrB  = READ_REGISTER_USHORT(UBCBbrB);
    DebugRegisters.BdrB  = READ_REGISTER_ULONG(UBCBdrB);
    DebugRegisters.BdmrB = READ_REGISTER_ULONG(UBCBdmrB);
    DebugRegisters.Brcr  = READ_REGISTER_USHORT(UBCBrcr);
    DebugRegisters.Align = 0;

    //
    // Follow the context in the buffer with the debug register values.
    //
    AdditionalData->Length += sizeof(DEBUG_REGISTERS);
#endif

#if defined(SH4)
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(Context->Psr), (PCHAR)&(pCurThread->ctx.Psr),sizeof(DWORD));
    KdpQuickMoveMemory((PCHAR)&(Context->Fpscr), (PCHAR)&(pCurThread->ctx.Fpscr),sizeof(DWORD)*34);
#elif defined(MIPS_HAS_FPU)
    // Get the floating point registers from the thread context
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(Context->FltF0), (PCHAR)&(pCurThread->ctx.FltF0),sizeof(DWORD)*32);
#elif defined(x86)
    if (g_CurFPUOwner) 
    {
        KCall((LPVOID)FPUFlushContext,0,0,0);
        Context->FloatSave = *(PTH_TO_FLTSAVEAREAPTR(pCurThread));
    }
#endif

    KdpQuickMoveMemory(AdditionalData->Buffer, (PCHAR)Context, sizeof(CONTEXT));
#if defined(SH3e) || defined(SH4)
    KdpQuickMoveMemory(AdditionalData->Buffer + sizeof(CONTEXT),
                       (PCHAR)&DebugRegisters, sizeof(DEBUG_REGISTERS));
#endif

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
}

VOID
KdpSetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a set context state
    manipulation message.  Its function is set the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    STRING MessageHeader;
#if defined(SH3e) || defined(SH4)
    PDEBUG_REGISTERS DebugRegisters = (PDEBUG_REGISTERS)(AdditionalData->Buffer + sizeof(CONTEXT));
#endif

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

#if defined(SH3e) || defined(SH4)
    //
    // Debug register values passed following the context
    //
    KD_ASSERT(AdditionalData->Length == sizeof(CONTEXT) + sizeof(DEBUG_REGISTERS));
#else
    KD_ASSERT(AdditionalData->Length == sizeof(CONTEXT));
#endif

    m->ReturnStatus = STATUS_SUCCESS;
    KdpQuickMoveMemory((PCHAR)Context, AdditionalData->Buffer, sizeof(CONTEXT));

    // copy the floating point registers into the thread context
#if defined(SH4)
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(pCurThread->ctx.Fpscr),(PCHAR)&(Context->Fpscr), sizeof(DWORD)*34);
#elif defined(MIPS_HAS_FPU)
    FPUFlushContext();
    KdpQuickMoveMemory((PCHAR)&(pCurThread->ctx.FltF0),(PCHAR)&(Context->FltF0), sizeof(DWORD)*32);
#endif



/*
#if defined(SH3)
    WRITE_REGISTER_ULONG(UBCBarA,  Context->DebugRegisters.BarA);
    WRITE_REGISTER_UCHAR(UBCBasrA, Context->DebugRegisters.BasrA);
    WRITE_REGISTER_UCHAR(UBCBamrA, Context->DebugRegisters.BamrA);
    WRITE_REGISTER_USHORT(UBCBbrA, Context->DebugRegisters.BbrA);
    WRITE_REGISTER_ULONG(UBCBarB,  Context->DebugRegisters.BarB);
    WRITE_REGISTER_UCHAR(UBCBasrB, Context->DebugRegisters.BasrB);
    WRITE_REGISTER_UCHAR(UBCBamrB, Context->DebugRegisters.BamrB);
    WRITE_REGISTER_USHORT(UBCBbrB, Context->DebugRegisters.BbrB);
    WRITE_REGISTER_ULONG(UBCBdrB,  Context->DebugRegisters.BdrB);
    WRITE_REGISTER_ULONG(UBCBdmrB, Context->DebugRegisters.BdmrB);
    WRITE_REGISTER_USHORT(UBCBrcr, Context->DebugRegisters.Brcr);
#elif defined(SH3e) || defined(SH4)
    WRITE_REGISTER_ULONG(UBCBarA,  DebugRegisters->BarA);
    WRITE_REGISTER_UCHAR(UBCBasrA, DebugRegisters->BasrA);
    WRITE_REGISTER_UCHAR(UBCBamrA, DebugRegisters->BamrA);
    WRITE_REGISTER_USHORT(UBCBbrA, DebugRegisters->BbrA);
    WRITE_REGISTER_ULONG(UBCBarB,  DebugRegisters->BarB);
    WRITE_REGISTER_UCHAR(UBCBasrB, DebugRegisters->BasrB);
    WRITE_REGISTER_UCHAR(UBCBamrB, DebugRegisters->BamrB);
    WRITE_REGISTER_USHORT(UBCBbrB, DebugRegisters->BbrB);
    WRITE_REGISTER_ULONG(UBCBdrB,  DebugRegisters->BdrB);
    WRITE_REGISTER_ULONG(UBCBdmrB, DebugRegisters->BdmrB);
    WRITE_REGISTER_USHORT(UBCBrcr, DebugRegisters->Brcr);
#endif
*/

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
}

VOID
KdpWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state
    manipulation message.  Its function is to write a breakpoint
    and return a handle to the breakpoint.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_BREAKPOINT a = &m->u.WriteBreakPoint;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    KD_ASSERT(AdditionalData->Length == 0);

    a->BreakPointHandle = KdpAddBreakpoint(a->BreakPointAddress);
    DEBUGGERMSG(KDZONE_API,(L"Handle returned is %8.8lx for address %8.8lx\r\n",a->BreakPointHandle, a->BreakPointAddress));
    if (a->BreakPointHandle != 0) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
#ifndef SPEED_HACK
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
#endif
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state
    manipulation message.  Its function is to restore a breakpoint
    using the specified handle.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_RESTORE_BREAKPOINT a = &m->u.RestoreBreakPoint;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    KD_ASSERT(AdditionalData->Length == 0);

    if (KdpDeleteBreakpoint(a->BreakPointHandle)) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
#ifndef SPEED_HACK
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
#endif

    UNREFERENCED_PARAMETER(Context);
}

BOOLEAN
KdpReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT CONTEXT * ContextRecord,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine sends an exception state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

    SecondChance - Supplies a boolean value that determines whether this is
        the first or second chance for the exception.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;


    do {

        //
        // Construct the wait state change message and message descriptor.
        //

        KdpSetStateChange(&WaitStateChange,
                            ExceptionRecord,
                            ContextRecord,
                            SecondChance
                            );

        MessageHeader.Length = sizeof(DBGKD_WAIT_STATE_CHANGE);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;

        MessageData.Length = 0;

        //
        // Send packet to the kernel debugger on the host machine,
        // wait for answer.
        //
        Status = KdpSendWaitContinue(
                    PACKET_TYPE_KD_STATE_CHANGE,
                    &MessageHeader,
                    &MessageData,
                    ContextRecord
                    );

    } while (Status == ContinueProcessorReselected) ;

    return (BOOLEAN) Status;
}


BOOLEAN
KdpReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN UnloadSymbols,
    IN OUT CONTEXT * ContextRecord
    )

/*++

Routine Description:

    This routine sends a load symbols state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    PathName - Supplies a pointer to the pathname of the image whose
        symbols are to be loaded.

    BaseOfDll - Supplies the base address where the image was loaded.

    ProcessId - Unique 32-bit identifier for process that is using
        the symbols.  -1 for system process.

    CheckSum - Unique 32-bit identifier from image header.

    UnloadSymbol - TRUE if the symbols that were previously loaded for
        the named image are to be unloaded from the debugger.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{

    PSTRING AdditionalData;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;


    if ((KdpUseTCPSockets || KdpUseUDPSockets) && socketReady[0] == '1')
    {
        if (packetsIndex)
            SendSavedPackets();
    }

    do {
        //
        // Construct the wait state change message and message descriptor.
        //

        WaitStateChange.NewState = DbgKdLoadSymbolsStateChange;
        WaitStateChange.ProcessorType = 0;
        WaitStateChange.Processor = (USHORT)0;
		WaitStateChange.NumberProcessors = (ULONG)1;
        WaitStateChange.Thread = pCurThread;

        WaitStateChange.ProgramCounter = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
        KdpSetLoadState(&WaitStateChange, ContextRecord);
        WaitStateChange.u.LoadSymbols.UnloadSymbols = UnloadSymbols;
        WaitStateChange.u.LoadSymbols.BaseOfDll = SymbolInfo->BaseOfDll;
        WaitStateChange.u.LoadSymbols.ProcessId = SymbolInfo->ProcessId;
        WaitStateChange.u.LoadSymbols.CheckSum = SymbolInfo->CheckSum;
        WaitStateChange.u.LoadSymbols.SizeOfImage = SymbolInfo->SizeOfImage;
        if (ARGUMENT_PRESENT( PathName )) {
            WaitStateChange.u.LoadSymbols.PathNameLength =
                KdpMoveMemory(
                    (PCHAR)KdpMessageBuffer,
                    (PCHAR)PathName->Buffer,
                    PathName->Length
                    ) + 1;
            MessageData.Buffer = KdpMessageBuffer;
            MessageData.Length = (USHORT)WaitStateChange.u.LoadSymbols.PathNameLength;
            MessageData.Buffer[MessageData.Length-1] = '\0';
            AdditionalData = &MessageData;
        } else {
            WaitStateChange.u.LoadSymbols.PathNameLength = 0;
            AdditionalData = NULL;
        }

        MessageHeader.Length = sizeof(DBGKD_WAIT_STATE_CHANGE);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;

        //
        // Send packet to the kernel debugger on the host machine, wait
        // for the reply.
        //
        DEBUGGERMSG(KDZONE_API, (L"Updating symbols for %a %8.8lx %8.8lx\r\n",
            PathName->Buffer,
            WaitStateChange.u.LoadSymbols.BaseOfDll,
            WaitStateChange.u.LoadSymbols.SizeOfImage));

        if (!KdpUseTCPSockets && !KdpUseUDPSockets) {
            Status = KdpSendWaitContinue(
                PACKET_TYPE_KD_STATE_CHANGE,
                &MessageHeader,
                AdditionalData,
                ContextRecord
                );
        }
        else {
            if (socketReady[0] == '1') {
                // The socket transport is ready.  Send packet.

                Status = KdpSendWaitContinue(
                    PACKET_TYPE_KD_STATE_CHANGE,
                    &MessageHeader,
                    AdditionalData,
                    ContextRecord
                    );
            }
            else {
                // The socket transport is not ready.  Save packet.
                memcpy(&SavedPackets[packetsIndex%MAX_LOAD_NOTIFICATIONS].StateChange,
                       &WaitStateChange,
                       sizeof(DBGKD_WAIT_STATE_CHANGE));

                if (AdditionalData) {
                    SavedPackets[packetsIndex%MAX_LOAD_NOTIFICATIONS].AdditionalDataLength = MessageData.Length;
                    strncpy(SavedPackets[packetsIndex%MAX_LOAD_NOTIFICATIONS].AdditionalData,
                            MessageData.Buffer,
                            MessageData.Length);
                }
                else
                    SavedPackets[packetsIndex%MAX_LOAD_NOTIFICATIONS].AdditionalDataLength = 0;

                packetsIndex++;
                return (BOOLEAN) ContinueSuccess;
            }
        }
    } while (Status == ContinueProcessorReselected);
    return (BOOLEAN) Status;
}

VOID
KdpReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response to a read physical memory
    state manipulation message. Its function is to read physical memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_MEMORY a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Source;
    PUCHAR Destination;
    USHORT NumberBytes;
    USHORT BytesLeft;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // make sure that nothing but a read memory message was transmitted
    //

    KD_ASSERT(AdditionalData->Length == 0);

    //
    // Trim transfer count to fit in a single message
    //

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);
    } else {
        Length = a->TransferCount;
    }

    //
    // Since the MmDbgTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //

    Source = (ULONG)a->TargetBaseAddress;

    Destination = AdditionalData->Buffer;
    BytesLeft = (USHORT)Length;
    if(PAGE_ALIGN((PUCHAR)a->TargetBaseAddress) ==
       PAGE_ALIGN((PUCHAR)(a->TargetBaseAddress)+Length)) {
        //
        // Memory move starts and ends on the same page.
        //

        VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Source);
        AdditionalData->Length = (USHORT)KdpMoveMemory(
                                            Destination,
                                            VirtualAddress,
                                            BytesLeft
                                            );
        BytesLeft -= AdditionalData->Length;
    } else {

        //
        // Memory move spans page boundaries
        //

        VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Source);
        NumberBytes = (USHORT)(PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
        AdditionalData->Length = (USHORT)KdpMoveMemory(
                                        Destination,
                                        VirtualAddress,
                                        NumberBytes
                                        );
        Source += NumberBytes;
        Destination += NumberBytes;
        BytesLeft -= NumberBytes;
        while(BytesLeft > 0) {

            //
            // Transfer a full page or the last bit,
            // whichever is smaller.
            //

            VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Source);
            NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);

            AdditionalData->Length += (USHORT)KdpMoveMemory(
                                            Destination,
                                            VirtualAddress,
                                            NumberBytes
                                            );
            Source += NumberBytes;
            Destination += NumberBytes;
            BytesLeft -= NumberBytes;
        }
    }

    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    a->ActualBytesRead = AdditionalData->Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
    UNREFERENCED_PARAMETER(Context);
}


VOID
KdpWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response to a write physical memory
    state manipulation message. Its function is to write physical memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_MEMORY a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Destination;
    PUCHAR Source;
    USHORT NumberBytes;
    USHORT BytesLeft;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // Since the MmDbgTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //

    Destination = (ULONG)a->TargetBaseAddress;

    Source = AdditionalData->Buffer;
    BytesLeft = (USHORT) a->TransferCount;

    if(PAGE_ALIGN((PUCHAR)Destination) ==
       PAGE_ALIGN((PUCHAR)(Destination)+BytesLeft)) {
        //
        // Memory move starts and ends on the same page.
        //
        VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Destination);
        Length = (USHORT)KdpMoveMemory(
                                       VirtualAddress,
                                       Source,
                                       BytesLeft
                                      );
        BytesLeft -= (USHORT) Length;
    } else {
        //
        // Memory move spans page boundaries
        //
        VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Destination);
        NumberBytes = (USHORT) (PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
        Length = (USHORT)KdpMoveMemory(
                                       VirtualAddress,
                                       Source,
                                       NumberBytes
                                      );
        Source += NumberBytes;
        Destination += NumberBytes;

        BytesLeft -= NumberBytes;
        while(BytesLeft > 0) {

            //
            // Transfer a full page or the last bit,
            // whichever is smaller.
            //

            VirtualAddress = (PVOID)MmDbgTranslatePhysicalAddress(Destination);
            NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);

            Length += (USHORT)KdpMoveMemory(
                                            VirtualAddress,
                                            Source,
                                            NumberBytes
                                           );
            Source += NumberBytes;
            Destination += NumberBytes;
            BytesLeft -= NumberBytes;
        }
    }


    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    a->ActualBytesWritten = Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
    UNREFERENCED_PARAMETER(Context);
}


const ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000;


typedef enum _TargetCodeType
{
    tcCeUnknown,
    tcCeSH3,
    tcCeSH4,
    tcCeMIPS,
    tcCeX86,
    tcCeARM,
    tcCePPC
} TargetCodeType;


#if defined(SH3)
#define CURRENT_TARGET_CODE (tcCeSH3)
#elif defined(SH4)
#define CURRENT_TARGET_CODE (tcCeSH4)
#elif defined(x86)
#define CURRENT_TARGET_CODE (tcCeX86)
#elif defined(PPC)
#define CURRENT_TARGET_CODE (tcCePPC)
#elif defined(MIPS)
#define CURRENT_TARGET_CODE (tcCeMIPS)
#elif defined(ARM)
#define CURRENT_TARGET_CODE (tcCeARM)
#else
#define CURRENT_TARGET_CODE (tcCeUnknown)
// warning if new cpu architecture is defined: please add new type in TargetCodeType (and in host kernel debugger side too)
#endif


VOID
KdpGetVersion(
    IN PDBGKD_MANIPULATE_STATE m
    )

/*++

Routine Description:

    This function returns to the caller a general information packet
    that contains useful information to a debugger.  This packet is also
    used for a debugger to determine if the writebreakpointex and
    readbreakpointex apis are available.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.

--*/

{
    STRING                  messageHeader;
    int i;

    messageHeader.Length = sizeof(*m);
    messageHeader.Buffer = (PCHAR)m;

    if (m->u.GetVersion.ProtocolVersion >= 1) {
        //
        // this flag causes the state change packet to contain
        // the current context record
        //
        KdpSendContext = TRUE;
    }

    m->u.GetVersion.dwProcessorName = NkCEProcessorType;
    m->u.GetVersion.MachineType = (USHORT) CURRENT_TARGET_CODE;

    //
    // the current build number
    //
    m->u.GetVersion.MinorVersion = (short)NtBuildNumber;
    m->u.GetVersion.MajorVersion = (short)((NtBuildNumber >> 28) & 0xFFFFFFF);

    //
    // kd protocol version number.  this should be incremented if the
    // protocol changes.
    //
    m->u.GetVersion.ProtocolVersion = 5;
    








    m->u.GetVersion.Flags = 0;

    //
    // address of the loader table
    //

    m->u.GetVersion.PsLoadedModuleList  = (ULONG)NULL;

    //
    // This is where the firmware loads the target program
    //

    m->u.GetVersion.KernBase = (ULONG)KdpImageBase;

    //
    // This is the relocated kernel data section offset.
    //

    m->u.GetVersion.KernDataSectionOffset = ((COPYentry *)(pTOC->ulCopyOffset))->ulDest;

    //
    // the usual stuff
    //
    m->ReturnStatus = STATUS_SUCCESS;
    m->ApiNumber = DbgKdGetVersionApi;


	if (pKDIoControl) {
		KCall(pKDIoControl, KD_IOCTL_INIT, NULL, 0);
	}
	for (i=0; i < BREAKPOINT_TABLE_SIZE; i++) {	
		KdpDeleteBreakpoint(i);
	}

    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &messageHeader,
                  NULL
                 );

    return;
} // End of KdpGetVersion


NTSTATUS
KdpWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state 'ex'
    manipulation message.  Its function is to clear breakpoints, write
    new breakpoints, and continue the target system.  The clearing of
    breakpoints is conditional based on the presence of breakpoint handles.
    The setting of breakpoints is conditional based on the presence of
    valid, non-zero, addresses.  The continueing of the target system
    is conditional based on a non-zero continuestatus.

    This api allows a debugger to clear breakpoints, add new breakpoint,
    and continue the target system all in one api packet.  This reduces the
    amount of traffic across the wire and greatly improves source stepping.


Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_BREAKPOINTEX       a = &m->u.BreakPointEx;
    PDBGKD_WRITE_BREAKPOINT   b;
    STRING                    MessageHeader;
    ULONG                     i;


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //

    if (AdditionalData->Length !=
                         a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT)) {

#ifndef SPEED_HACK

        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );

#else

        DEBUGGERMSG(KDZONE_API, (L"KdpWriteBreakPointEx: Length mismatch\n"));
#endif

    }


    //
    // assume success
    //

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //

    b = (PDBGKD_WRITE_BREAKPOINT) AdditionalData->Buffer;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointHandle) {
            if (!KdpDeleteBreakpoint(b->BreakPointHandle)) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }
            b->BreakPointHandle = 0;
        }
    }

    //
    // loop thru the breakpoint addesses passed in from the debugger and
    // add any new breakpoints that have a non-zero address
    //
    b = (PDBGKD_WRITE_BREAKPOINT) AdditionalData->Buffer;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointAddress) {
            b->BreakPointHandle = KdpAddBreakpoint( b->BreakPointAddress );
            if (!b->BreakPointHandle) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }

#ifndef SPEED_HACK

    //
    // send back our response
    //

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );

#endif

    //
    // return the caller's continue status value.  if this is a non-zero
    // value the system is continued using this value as the continuestatus.
    //

    return a->ContinueStatus;
}


VOID
KdpRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state 'ex'
    manipulation message.  Its function is to clear a list of breakpoints.

Arguments:

    m - Supplies the state manipulation message.

    Additionaldata - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_BREAKPOINTEX         a = &m->u.BreakPointEx;
    PDBGKD_RESTORE_BREAKPOINT   b = (PDBGKD_RESTORE_BREAKPOINT) AdditionalData->Buffer;
    STRING                      MessageHeader;
    ULONG                       i;


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //

    if (AdditionalData->Length !=
                       a->BreakPointCount*sizeof(DBGKD_RESTORE_BREAKPOINT)) {

#ifndef SPEED_HACK

        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );

#else

        DEBUGGERMSG(KDZONE_API, (L"KdpRestoreBreakPointEx: Length mismatch\n"));

#endif

    }


    //
    // assume success
    //

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (!KdpDeleteBreakpoint(b->BreakPointHandle)) {
            m->ReturnStatus = STATUS_UNSUCCESSFUL;
        }
    }

#ifndef SPEED_HACK

    //
    // send back our response
    //
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );

#endif

}


NTSTATUS
KdpManipulateBreakPoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN CONTEXT * Context
    )

/*++

Routine Description:

    This function is called in response of a manipulate breakpoint 
    message.  Its function is to query/write/set a breakpoint
    and return a handle to the breakpoint.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_MANIPULATE_BREAKPOINT a = &m->u.ManipulateBreakPoint;
    PDBGKD_MANIPULATE_BREAKPOINT_DATA b = (PDBGKD_MANIPULATE_BREAKPOINT_DATA) AdditionalData->Buffer;
    STRING MessageHeader;
    BOOL fSuccess = FALSE;
    DWORD i;
    BOOL fHALBP=FALSE;
    KD_BPINFO bpInfo;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

	DEBUGGERMSG(KDZONE_HAL, (L"KDManipulateBreakPoint\r\n"));
	// The request has the first breakpoint..if more than one than AdditionalData has the others
    if (AdditionalData->Length != (a->Count) *sizeof(DBGKD_MANIPULATE_BREAKPOINT_DATA)) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );

        DEBUGGERMSG(KDZONE_HAL, (L"KdpManipulateBreakPoint: Length mismatch\n"));
    }
	    
	DEBUGGERMSG(KDZONE_HAL, (L"Count = %ld\n", a->Count));
    for (i=0; i < a->Count; i++) {
		DEBUGGERMSG(KDZONE_HAL, (L"Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
		if ((b[i].Flags & DBGKD_MBP_FLAG_DP) && pKDIoControl) {
			if (b[i].Flags & DBGKD_MBP_FLAG_SET) {  
				bpInfo.nVersion = 1;
				bpInfo.ulAddress = ZeroPtr(b[i].Address);
				bpInfo.ulFlags = 0;
				bpInfo.ulHandle = 0;
				if (KCall(pKDIoControl, KD_IOCTL_SET_DBP, &bpInfo, sizeof(KD_BPINFO))) {
					fSuccess = TRUE;
					b[i].Handle = bpInfo.ulHandle | 0x80000000;
					DEBUGGERMSG(KDZONE_HAL, (L"Set Hard DBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
				}
			} else if (b[i].Handle & 0x80000000) {
				bpInfo.nVersion = 1;
				bpInfo.ulHandle = b[i].Handle & 0x7FFFFFFF;
				bpInfo.ulFlags = 0;
				bpInfo.ulAddress = 0;
				if (KCall(pKDIoControl, KD_IOCTL_CLEAR_DBP, &bpInfo, sizeof(KD_BPINFO))) {
					DEBUGGERMSG(KDZONE_HAL, (L"Clear Hard DBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
					fSuccess = TRUE;
				}
			}
		} else {
			if (pKDIoControl) {
				if (b[i].Flags & DBGKD_MBP_FLAG_SET) {
					bpInfo.nVersion = 1;
					bpInfo.ulAddress = ZeroPtr(b[i].Address);
					bpInfo.ulFlags = 0;
					if (KCall(pKDIoControl, KD_IOCTL_SET_CBP, &bpInfo, sizeof(KD_BPINFO))) {
						fSuccess = TRUE;
						fHALBP = TRUE;
						b[i].Handle = bpInfo.ulHandle | 0x80000000;
							DEBUGGERMSG(KDZONE_HAL, (L"Set  Hard CBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
					}
				} else 
				if (b[i].Handle & 0x80000000) {
					fHALBP = TRUE;
					bpInfo.nVersion = 1;
					bpInfo.ulHandle = b[i].Handle & 0x7FFFFFFF;
					bpInfo.ulFlags = 0;
					if (KCall(pKDIoControl, KD_IOCTL_CLEAR_CBP, &bpInfo, sizeof(KD_BPINFO))) {
						fSuccess = TRUE;
						DEBUGGERMSG(KDZONE_HAL, (L"Clear  Hard CBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
					}
	 			}
	 		}	
			if (!fHALBP && (b[i].Flags & DBGKD_MBP_FLAG_CP)) {
				if (b[i].Flags & DBGKD_MBP_FLAG_SET) {
					b[i].Flags |= DBGKD_MBP_SOFTWARE;
#if defined(THUMBSUPPORT) || defined(MIPS16SUPPORT)					
					if (b[i].Flags & DBGKD_MBP_16BIT) b[i].Address |= 0x1;
#endif
				    if (b[i].Handle = KdpAddBreakpoint((PVOID)b[i].Address)) {
						DEBUGGERMSG(KDZONE_HAL, (L"Set Soft CBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
				    	fSuccess = TRUE;
				    }
				} else {
				    if (KdpDeleteBreakpoint(b[i].Handle)) {
						DEBUGGERMSG(KDZONE_HAL, (L"Clear Soft CBP Address = %08X Flags=%08X Handle=%08X\n", b[i].Address, b[i].Flags, b[i].Handle));
				    	fSuccess = TRUE;
				    }
				}
			}	
		}
    }

    if (fSuccess) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
	DEBUGGERMSG(KDZONE_HAL, (L"Status = %ld\n", m->ReturnStatus));
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
    UNREFERENCED_PARAMETER(Context);
    return a->ContinueStatus;
}

