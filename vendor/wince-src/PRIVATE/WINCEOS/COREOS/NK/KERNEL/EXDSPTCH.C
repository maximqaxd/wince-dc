/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames.

--*/

#include "kernel.h"
#include "excpt.h"

//
// Define local macros.
//
// Raise noncontinuable exception with associated exception record.
//

#if 0
 #define RAISE_EXCEPTION(Status, ExceptionRecordt) { \
    EXCEPTION_RECORD ExceptionRecordn; \
                                            \
    ExceptionRecordn.ExceptionCode = Status; \
    ExceptionRecordn.ExceptionFlags = EXCEPTION_NONCONTINUABLE; \
    ExceptionRecordn.ExceptionRecord = ExceptionRecordt; \
    ExceptionRecordn.NumberParameters = 0; \
    RtlRaiseException(&ExceptionRecordn); \
    }
#else
#define RAISE_EXCEPTION(Status, ExceptionRecordt) DEBUGCHK(0)
#endif

// Obtain current stack limits
#define NKGetStackLimits(pth, LL, HL)   \
        ((*(LL) = (pth)->dwStackBase), (*(HL) = (ULONG)(pth)->tlsPtr))


#ifdef x86

#define EXCEPTION_CHAIN_END ((PEXCEPTION_REGISTRATION_RECORD)-1)

//
// Define private function prototypes.
//

EXCEPTION_DISPOSITION RtlpExecuteHandlerForException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine,
    IN ULONG ExceptionMode);

EXCEPTION_DISPOSITION RtlpExecuteHandlerForUnwind(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine,
    IN ULONG ExceptionMode);

BOOLEAN NKUnwind(
    IN PTHREAD pth,
    IN PCALLSTACK TargetCStk OPTIONAL,
    IN PEXCEPTION_REGISTRATION_RECORD TargetFrame OPTIONAL,
    IN ULONG TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG ReturnValue,
    IN PCONTEXT ContextRecord);

void UpdateASID(IN PTHREAD pth, IN PPROCESS pProc, IN ACCESSKEY aky);

extern EXCEPTION_ROUTINE KPSLExceptionRoutine;


void __inline StoreRegistrationPointer(void *RegistrationPointer)
{
    __asm {
        mov eax, [RegistrationPointer]
        mov dword ptr fs:[0], eax
    }
}

#pragma warning(disable:4035)

PVOID __inline GetRegistrationHead(void)
{
    __asm mov eax, dword ptr fs:[0];
}
        
#pragma warning(default:4035)



BOOLEAN
NKDispatchException(
    IN PTHREAD pth,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord)
{
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD NestedRegistration;
    DISPATCHER_CONTEXT DispatcherContext;
    UINT HighAddress;
    UINT HighLimit;
    UINT LowLimit;
    PCALLSTACK pcstk;
    PCALLSTACK TargetCStk;
    PPROCESS pProc;
    PPROCESS pprcSave;
    ACCESSKEY akySave;
    EXCEPTION_REGISTRATION_RECORD TempRegistration;
    CONTEXT ContextRecord1;

    // Get current stack limits
    NKGetStackLimits(pth, &LowLimit, &HighLimit);

    DEBUGMSG(ZONE_SEH, (TEXT("X86 DispatchException(%x %x) pc %x code %x address %x\r\n"),
    ExceptionRecord, ContextRecord, ContextRecord->Eip,
    ExceptionRecord->ExceptionCode, ExceptionRecord->ExceptionAddress));
	pcstk = pth->pcstkTop;
    akySave = pth->aky;
    pprcSave = pCurProc;

    memcpy(&ContextRecord1, ContextRecord, sizeof(CONTEXT));

    /*
       Start with the frame specified by the context record and search
       backwards through the call frame hierarchy attempting to find an
       exception handler that will handler the exception.
     */
    RegistrationPointer = GetRegistrationHead();
    NestedRegistration = 0;
    TargetCStk = 0;
    while (RegistrationPointer && RegistrationPointer != EXCEPTION_CHAIN_END) {
        // Check for PSL call boundary.
        DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException: Proc=%8.8lx RP=%8.8lx\r\n"),
                pCurProc, RegistrationPointer));
        while (RegistrationPointer == (PEXCEPTION_REGISTRATION_RECORD)-2) {
            PEXCEPTION_ROUTINE pEH = KPSLExceptionRoutine;

            if (!pcstk) {
                RETAILMSG(1, (TEXT("NKDispatchException: no CALLSTACK with RP=%d\r\n"),
                        RegistrationPointer));
                break;
            }

            // in a PSL call.
            pProc = pcstk->pprcLast;
            if (!TargetCStk && pcstk->akyLast && ((ulong)pProc < 0x10000
                    || (pEH = pCurProc->pfnEH) != 0)) {
                TargetCStk = pcstk;
                RegistrationPointer = &TempRegistration;
                RegistrationPointer->Next = (PEXCEPTION_REGISTRATION_RECORD)-2;
                RegistrationPointer->Handler = pEH;
                break;
            } else {
                TargetCStk = 0;
                RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)pcstk->extra;
                if ((ulong)pProc < 0x10000) {
                    /* This call is returning from kernel mode server. Restore
                     * the thread's previous operating mode and return.
                     */
                    SetContextMode(&ContextRecord1, (ULONG)pProc);
                } else {
                    /* Returning from a user mode server, restore the previous
                     * address space information and return.
                     */
                    UpdateASID(pth, pProc, pcstk->akyLast ? pcstk->akyLast : pth->aky);
                }
                pcstk = pcstk->pcstkNext;
                DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException: PSL return Proc=%8.8lx RP=%8.8lx pcstk=%8.8lx\r\n"),
                        pProc, RegistrationPointer, pcstk));
            }
        }

        /*
           If the call frame is not within the specified stack limits or the
           call frame is unaligned, then set the stack invalid flag in the
           exception record and return FALSE. Else check to determine if the
           frame has an exception handler.
         */
        HighAddress = (UINT)RegistrationPointer + sizeof(EXCEPTION_REGISTRATION_RECORD);
        if ((UINT)RegistrationPointer < LowLimit || HighAddress > HighLimit
                || ((UINT)RegistrationPointer & 0x3) != 0) { /* Not aligned */
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        } else {
            /* The handler must be executed by calling another routine
             that is written in assembler. This is required because
             up level addressing of the handler information is required
             when a nested exception is encountered.
             */
            DispatcherContext.ControlPc = 0;
            DispatcherContext.RegistrationPointer = RegistrationPointer;
            DEBUGMSG(1, (TEXT("Calling exception handler @%8.8lx RP=%8.8lx\r\n"),
                    RegistrationPointer->Handler, RegistrationPointer));

            Disposition = RtlpExecuteHandlerForException(ExceptionRecord,
                    RegistrationPointer, ContextRecord, &DispatcherContext,
                    RegistrationPointer->Handler, GetContextMode(&ContextRecord1));
            DEBUGMSG(1, (TEXT("exception dispostion = %d\r\n"), Disposition));

            /* If the current scan is within a nested context and the frame
               just examined is the end of the context region, then clear
               the nested context frame and the nested exception in the
               exception flags.
             */
            if (NestedRegistration == RegistrationPointer) {
                ExceptionRecord->ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                NestedRegistration = 0;
            }

            // Case on the handler disposition.
            switch (Disposition) {
                    // The disposition is to execute a frame based handler. The
                    // target handler address is in DispatcherContext.ControlPc.
                    // Reset to the original context and switch to unwind mode.
                case ExceptionExecuteHandler:
                    DEBUGMSG(ZONE_SEH && DispatcherContext.ControlPc,
                            (TEXT("Unwinding to %8.8lx RegistrationPointer=%8.8lx\r\n"),
                            DispatcherContext.ControlPc, RegistrationPointer));
                    DEBUGMSG(ZONE_SEH && !DispatcherContext.ControlPc,
                            (TEXT("Unwinding to %8.8lx RegistrationPointer=%8.8lx Ebx=%8.8lx Esi=%d\r\n"),
                            ContextRecord->Eip, RegistrationPointer,
                            ContextRecord->Ebx, ContextRecord->Esi));
                    UpdateASID(pth, pprcSave, akySave);
                    // Repair the CONTEXT for PSL handlers.
                    if (TargetCStk) {
                        ContextRecord->Esp = TargetCStk->ExEsp;
                        ContextRecord->Ebp = TargetCStk->ExEbp;
                        ContextRecord->Ebx = TargetCStk->ExEbx;
                        ContextRecord->Esi = TargetCStk->ExEsi;
                        ContextRecord->Edi = TargetCStk->ExEdi;
                    }
                    return NKUnwind(pth, TargetCStk, RegistrationPointer,
                            DispatcherContext.ControlPc, ExceptionRecord,
                            ExceptionRecord->ExceptionCode, ContextRecord);

                /* The disposition is to continue execution. If the
                   exception is not continuable, then raise the exception
                   EXC_NONCONTINUABLE_EXCEPTION. Otherwise return
                   TRUE. */
            case ExceptionContinueExecution :
                if ((ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0) {
                    RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                    break;
                } else {
                    UpdateASID(pth, pprcSave, akySave);
                    return TRUE;
                }

                /*
                   The disposition is to continue the search. Get next
                   frame address and continue the search.
                 */

            case ExceptionContinueSearch :
                break;

                /*
                   The disposition is nested exception. Set the nested
                   context frame to the establisher frame address and set
                   nested exception in the exception flags.
                 */

            case ExceptionNestedException :
                ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;

                if (DispatcherContext.RegistrationPointer > NestedRegistration){
                    NestedRegistration = DispatcherContext.RegistrationPointer;
                }
                break;

                /*
                   All other disposition values are invalid. Raise
                   invalid disposition exception.
                 */

            default :
                RETAILMSG(1, (TEXT("NKDispatchException: invalid dispositon (%d)\r\n"),
                        Disposition));
                RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                break;
            }

            RegistrationPointer = RegistrationPointer->Next;
        }
    }
    DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException: ran out of exceptions pc=%x code=%x\n"),
        ContextRecord1.Eip, ExceptionRecord->ExceptionCode));
    RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
    UpdateASID(pth, pprcSave, akySave);
    return FALSE;
}


BOOLEAN
NKUnwind(
    IN PTHREAD pth,
    IN PCALLSTACK TargetCStk OPTIONAL,
    IN PEXCEPTION_REGISTRATION_RECORD TargetFrame OPTIONAL,
    IN ULONG TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG ReturnValue,
    IN PCONTEXT ContextRecord)
/*++
Routine Description:
    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    walk through the procedure call frames is then performed to find the target
    of the unwind operation.

    N.B.    The captured context passed to unwinding handlers will not be
            a  completely accurate context set for the 386.  This is because
            there isn't a standard stack frame in which registers are stored.

            Only the integer registers are affected.  The segement and
            control registers (ebp, esp) will have correct values for
            the flat 32 bit environment.

Arguments:
    pth - Supplies a pointer to thread structure

    TargetCStk - Supplies an optional pointer to a PSL callstack to unwind to.
        This parameter is used instead of TargetFrame.

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

    ContextRecord - Supplies a pointer to a context record that can be used
        to store context druing the unwind operation.

Return Value:
    TRUE if target frame or callstack reached.
--*/
{
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD PriorPointer;
    ULONG ExceptionFlags;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
    PCALLSTACK pcstk;
    PPROCESS pProc;

    // Get current stack limits.
    NKGetStackLimits(pth, &LowLimit, &HighLimit);
	pcstk = pth->pcstkTop;

    // If the target frame of the unwind is specified, then set EXCEPTION_UNWINDING
    // flag in the exception flags. Otherwise set both EXCEPTION_EXIT_UNWIND and
    // EXCEPTION_UNWINDING flags in the exception flags.
    ExceptionFlags = ExceptionRecord->ExceptionFlags | EXCEPTION_UNWINDING;
    if (!TargetFrame && !TargetCStk)
        ExceptionFlags |= EXCEPTION_EXIT_UNWIND;

    ContextRecord->Eax = ReturnValue;
    if (TargetIp)
        ContextRecord->Eip = TargetIp;

    // Scan backward through the call frame hierarchy, calling exception
    // handlers as they are encountered, until the target frame of the unwind
    // is reached.
    RegistrationPointer = GetRegistrationHead();
    while (RegistrationPointer && RegistrationPointer != EXCEPTION_CHAIN_END) {
        DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: Proc=%8.8lx RP=%8.8lx\r\n"),
                pCurProc, RegistrationPointer));

        // Check for PSL call boundary.
        while (RegistrationPointer == (PEXCEPTION_REGISTRATION_RECORD)-2) {
            if (!pcstk) {
                RETAILMSG(1, (TEXT("NKUnwind: no CALLSTACK with RP=%d\r\n"),
                        RegistrationPointer));
                goto Unwind_exit;
            }
            if (pcstk == TargetCStk) {
                DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: TargetCStk reached.\r\n")));
                goto Unwind_exit;
            }

            // in a PSL call.
            RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)pcstk->extra;
            pProc = pcstk->pprcLast;
            if ((ulong)pProc < 0x10000) {
                /* This call is returning from kernel mode server. Restore
                 * the thread's previous operating mode and return.
                 */
                SetContextMode(ContextRecord, (ULONG)pProc);
            } else {
                /* Returning from a user mode server, restore the previous
                 * address space information and return.
                 */
                UpdateASID(pth, pProc, pcstk->akyLast ? pcstk->akyLast : pth->aky);
            }
            pth->pcstkTop = pcstk->pcstkNext;
            if (IsValidKPtr(pcstk))
                FreeMem(pcstk,HEAP_CALLSTACK);

			pcstk = pth->pcstkTop;

            // Update the FS:[0] exception chain
            StoreRegistrationPointer(RegistrationPointer);
            DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: PSL return Proc=%8.8lx RP=%8.8lx pcstk=%8.8lx\r\n"),
                    pProc, RegistrationPointer, pcstk));
        }

#if 0
		// [Mercator 15263]
		// Removed this check because it is possible that NKDispatchException has
		// passed us its local TempRegistation structure as the target frame.  
		// TempRegistration lives much higher on the stack (i.e. lower in memory)
		// than the current RegistrationPointer.
		
        // If the target frame is lower in the stack than the current frame,
        // then raise STATUS_INVALID_UNWIND exception.
        if (TargetFrame && TargetFrame < RegistrationPointer) {
            RAISE_EXCEPTION(STATUS_INVALID_UNWIND_TARGET, ExceptionRecord);
            break;
        }
#endif

        // If the call frame is not within the specified stack limits or the
        // call frame is unaligned, then raise the exception STATUS_BAD_STACK.
        // Else restore the state from the specified frame to the context
        // record.
        HighAddress = (ULONG)RegistrationPointer + sizeof(EXCEPTION_REGISTRATION_RECORD);
        if ((ULONG)RegistrationPointer < LowLimit || HighAddress > HighLimit
                || ((ULONG)RegistrationPointer & 0x3) != 0) {
            RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);
            break;
        } else {

            // Call the exception handler.
            do {
                // If the establisher frame is the target of the unwind
                // operation, then set the target unwind flag.
                if (RegistrationPointer == TargetFrame){
                    ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                }
                ExceptionRecord->ExceptionFlags = ExceptionFlags;


                // The handler must be executed by calling another routine
                // that is written in assembler. This is required because
                // up level addressing of the handler information is required
                // when a collided unwind is encountered.
                DEBUGMSG(1, (TEXT("Calling termination handler @%8.8lx RP=%8.8lx\r\n"),
                        RegistrationPointer->Handler, RegistrationPointer));

                Disposition = RtlpExecuteHandlerForUnwind(ExceptionRecord,
                    (PVOID)RegistrationPointer, ContextRecord, (PVOID)&DispatcherContext,
                    RegistrationPointer->Handler, GetContextMode(ContextRecord));
                DEBUGMSG(1, (TEXT("  dispostion = %d\r\n"), Disposition));

                // Clear target unwind and collided unwind flags.
                ExceptionFlags &= ~(EXCEPTION_COLLIDED_UNWIND |
                                                    EXCEPTION_TARGET_UNWIND);

                // Case on the handler disposition.
                switch (Disposition) {
                    // The disposition is to continue the search. Get next
                    // frame address and continue the search.
                case ExceptionContinueSearch :
                    break;

                    // The disposition is colided unwind. Maximize the target
                    // of the unwind and change the context record pointer.
                case ExceptionCollidedUnwind :
                    // Pick up the registration pointer that was active at
                    // the time of the unwind, and simply continue.
                    ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    RegistrationPointer = DispatcherContext.RegistrationPointer;
                    break;

                    // All other disposition values are invalid. Raise
                    // invalid disposition exception.
                default :
                    RETAILMSG(1, (TEXT("NKUnwind: invalid dispositon (%d)\r\n"),
                            Disposition));
                    RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                    break;
                }

            } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            // If this is the target of the unwind, then continue execution
            // by calling the continue system service.
            if (RegistrationPointer == TargetFrame)
                break;

            // Step to next registration record and unlink
            // the unwind handler since it has been called.
            PriorPointer = RegistrationPointer;
            RegistrationPointer = RegistrationPointer->Next;
            StoreRegistrationPointer(RegistrationPointer);

            











        }
    }

Unwind_exit:
    // If the registration pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    if ((TargetFrame && RegistrationPointer == TargetFrame)
            || (TargetCStk && pcstk == TargetCStk)) {
        DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: continuing @%8.8lx\r\n"),
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)));
        return TRUE;
    }
    return FALSE;
}
#endif

#ifndef x86

#if MIPS
#if MIPS16SUPPORT
#define COMBINED_PDATA           // define to enable combined pdata format
#else
#define OLD_PDATA                // define to enable original pdata format
#endif

#define INST_SIZE   4
#define STK_ALIGN   0x7
#define RetValue    IntV0

#define RESTORE_EXTRA_INFO(Context, extra) ((Context)->IntGp = (extra))
#define FIXTOC(Context)
#endif

#if SHx
#define COMPRESSED_PDATA        // define to enable compressed pdata format

#define INST_SIZE   2
#define STK_ALIGN   0x3
#define IntRa       PR
#define IntSp       R15
#define RetValue    R0

#define RESTORE_EXTRA_INFO(Context, extra)
#define FIXTOC(Context)
#endif

#if PPC
#define COMPRESSED_PDATA        // define to enable compressed pdata format

#define INST_SIZE   4
#define STK_ALIGN   0x7
#define IntRa       Lr
#define IntSp       Gpr1
#define RetValue    Gpr3
#define Psr         Msr
#define RESTORE_EXTRA_INFO(Context, extra) ((Context)->Gpr2 = (extra))
#define FIXTOC(Context)

#endif

#if ARM
#define COMPRESSED_PDATA        // define to enable compressed pdata format

#define INST_SIZE   4
#define STK_ALIGN   0x3
#define IntRa       Lr
#define IntSp       Sp
#define RetValue    R0
#define RESTORE_EXTRA_INFO(Context, extra) ((Context)->R9 = (extra))
#define FIXTOC(Context)

#endif


#ifdef INST_SIZE
//
// Define private function prototypes.
//

ULONG
RtlpVirtualUnwind(
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame);

EXCEPTION_DISPOSITION RtlpExecuteHandlerForException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine,
    IN ULONG ExceptionMode);

EXCEPTION_DISPOSITION RtlpExecuteHandlerForUnwind(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine);

BOOLEAN NKUnwind(
    IN PTHREAD pth,
    IN PCALLSTACK TargetCStk OPTIONAL,
    IN ULONG TargetFrame OPTIONAL,
    IN ULONG TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG ReturnValue,
    IN PCONTEXT ContextRecord);

PRUNTIME_FUNCTION NKLookupFunctionEntry(
    IN PPROCESS pProc,
    IN ULONG ControlPc,
    PRUNTIME_FUNCTION prf);

void UpdateASID(IN PTHREAD pth, IN PPROCESS pProc, IN ACCESSKEY aky);

extern EXCEPTION_ROUTINE KPSLExceptionRoutine;

#if defined(COMPRESSED_PDATA) || defined(COMBINED_PDATA)
typedef struct PDATA {
    ULONG pFuncStart;
    ULONG PrologLen : 8;        // low order bits
    ULONG FuncLen   : 22;       // high order bits
    ULONG ThirtyTwoBits : 1;    // 0/1 == 16/32 bit instructions
    ULONG ExceptionFlag : 1;    // highest bit
} PDATA, *PPDATA;
#endif

typedef struct PDATA_EH {
    PEXCEPTION_ROUTINE pHandler;
    PVOID pHandlerData;
} PDATA_EH, *PPDATA_EH;

#if defined(COMBINED_PDATA)
typedef struct COMBINED_HEADER {
    DWORD zero1;               // must be zero if header present
    DWORD zero2;               // must be zero if header present
    DWORD Uncompressed;        // count of uncompressed entries
    DWORD Compressed;          // count of compressed entries
} COMBINED_HEADER, *PCOMBINED_HEADER;
#endif

BOOLEAN
NKDispatchException(
    IN PTHREAD pth,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord)
/*++
Routine Description:
    This function attempts to dispatch an exception to a frame based
    handler by searching backwards through the stack based call frames.
    The search begins with the frame specified in the context record and
    continues backward until either a handler is found that handles the
    exception, the stack is found to be invalid (i.e., out of limits or
    unaligned), or the end of the call hierarchy is reached.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called. If the
    handler does not handle the exception, then the prologue of the routine
    is executed backwards to "unwind" the effect of the prologue and then
    the next frame is examined.

Arguments:
    pth - Supplies a pointer to thread structure
    ExceptionRecord - Supplies a pointer to an exception record.
    ContextRecord - Supplies a pointer to a context record.

Return Value:
    If the exception is handled by one of the frame based handlers, then
    a value of TRUE is returned. Otherwise a value of FALSE is returned.
--*/
{
    CONTEXT ContextRecord1;
    CONTEXT LocalContext;
    PCONTEXT OriginalContext;
    ULONG ControlPc;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG EstablisherFrame;
    ULONG ExceptionFlags;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    BOOLEAN Handled;
    ULONG HighLimit;
    ULONG LowLimit;
    ULONG NestedFrame;
    ULONG NextPc;
    ULONG PrevSp;
    PCALLSTACK pcstk;
    PCALLSTACK TargetCStk;
    PPROCESS pprcSave;
    ACCESSKEY akySave;
    RUNTIME_FUNCTION TempFunctionEntry;

    //
    // Redirect ContextRecord to a local copy of the incoming context. The
    // exception dispatch process modifies the PC of this context. Unwinding
    // of nested exceptions will fail when unwinding through CaptureContext
    // with a modified PC.
    //

    OriginalContext = ContextRecord;
    ContextRecord = &LocalContext;
    memcpy(ContextRecord, OriginalContext, sizeof(CONTEXT));

    // Get current stack limits, copy the context record, get the initial
    // PC value, capture the exception flags, and set the nested exception
    // frame pointer.
    NKGetStackLimits(pth, &LowLimit, &HighLimit);
    memcpy(&ContextRecord1, ContextRecord, sizeof(CONTEXT));
    ControlPc = CONTEXT_TO_PROGRAM_COUNTER(&ContextRecord1);
    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;
    NestedFrame = 0;

	pcstk = pth->pcstkTop;

    akySave = pth->aky;
    pprcSave = pCurProc;

    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handle the exception.
    do {
        // Lookup the function table entry using the point at which control
        // left the procedure.
        DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException(%08x): Proc=%8.8lx ControlPc=%8.8lx SP=%8.8lx\r\n"),
ContextRecord,
                pCurProc, ControlPc, ContextRecord1.IntSp));
        PrevSp = ContextRecord1.IntSp;
        TargetCStk = 0;
        ControlPc = ZeroPtr(ControlPc);
        FunctionEntry = NKLookupFunctionEntry(pCurProc, ControlPc, &TempFunctionEntry);

        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the virtual
        // frame pointer of the establisher and check if there is an exception
        // handler for the frame.
        if (FunctionEntry != NULL) {
            NextPc = RtlVirtualUnwind(ControlPc,
                                      FunctionEntry,
                                      &ContextRecord1,
                                      &InFunction,
                                      &EstablisherFrame);
            // If the virtual frame pointer is not within the specified stack
            // limits or the virtual frame pointer is unaligned, then set the
            // stack invalid flag in the exception record and return exception
            // not handled. Otherwise, check if the current routine has an
            // exception handler.
            if ((EstablisherFrame < LowLimit) || (EstablisherFrame > HighLimit) ||
                    ((EstablisherFrame & STK_ALIGN) != 0)) {
                DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException: Bad stack frame=%8.8lx Low=%8.8lx High=%8.8lx\r\n"),
                    EstablisherFrame, LowLimit, HighLimit));
                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if ((FunctionEntry->ExceptionHandler != NULL) && InFunction) {
                // The frame has an exception handler. The handler must be
                // executed by calling another routine that is written in
                // assembler. This is required because up level addressing
                // of the handler information is required when a nested
                // exception is encountered.
invokeHandler:
                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.ContextRecord = ContextRecord;

                ExceptionRecord->ExceptionFlags = ExceptionFlags;
                DEBUGMSG(1, (TEXT("Calling exception handler @%8.8lx(%8.8lx) Frame=%8.8lx\r\n"),
                        FunctionEntry->ExceptionHandler, FunctionEntry->HandlerData,
                        EstablisherFrame));
                Disposition = RtlpExecuteHandlerForException(ExceptionRecord,
                        EstablisherFrame, ContextRecord, &DispatcherContext,
                        FunctionEntry->ExceptionHandler, ContextRecord1.Psr);
                ExceptionFlags |=
                    (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);
                DEBUGMSG(1, (TEXT("exception dispostion = %d\r\n"), Disposition));

                // If the current scan is within a nested context and the frame
                // just examined is the end of the nested region, then clear
                // the nested context frame and the nested exception flag in
                // the exception flags.
                if (NestedFrame == EstablisherFrame) {
                    ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                    NestedFrame = 0;
                }

                // Case on the handler disposition.
                switch (Disposition) {
                    // The disposition is to execute a frame based handler. The
                    // target handler address is in DispatcherContext.ControlPc.
                    // Reset to the original context and switch to unwind mode.
                case ExceptionExecuteHandler:
                    DEBUGMSG(ZONE_SEH, (TEXT("Unwinding to %8.8lx Frame=%8.8lx\r\n"),
                            DispatcherContext.ControlPc, EstablisherFrame));
                    UpdateASID(pth, pprcSave, akySave);
                    Handled =  NKUnwind(pth, TargetCStk, EstablisherFrame,
                                DispatcherContext.ControlPc, ExceptionRecord,
                                ExceptionRecord->ExceptionCode, ContextRecord);

                    memcpy(OriginalContext, ContextRecord, sizeof(CONTEXT));
                    return Handled;

                    // The disposition is to continue execution.
                    // If the exception is not continuable, then raise the
                    // exception STATUS_NONCONTINUABLE_EXCEPTION. Otherwise
                    // return exception handled.
                case ExceptionContinueExecution :
                    DEBUGMSG(ZONE_SEH, (TEXT("Continuing execution ...\r\n")));
                    if ((ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0)
                        RAISE_EXCEPTION(STATUS_NONCONTINUABLE_EXCEPTION, ExceptionRecord);
                    UpdateASID(pth, pprcSave, akySave);

                    memcpy(OriginalContext, ContextRecord, sizeof(CONTEXT));
                    return TRUE;

                    // The disposition is to continue the search.
                    // Get next frame address and continue the search.
                case ExceptionContinueSearch :
                    break;

                    // The disposition is nested exception.
                    // Set the nested context frame to the establisher frame
                    // address and set the nested exception flag in the
                    // exception flags.
                case ExceptionNestedException :
                    DEBUGMSG(ZONE_SEH, (TEXT("Nested exception: frame=%8.8lx\r\n"), 
                            DispatcherContext.EstablisherFrame));
                    ExceptionFlags |= EXCEPTION_NESTED_CALL;
                    if (DispatcherContext.EstablisherFrame > NestedFrame)
                        NestedFrame = DispatcherContext.EstablisherFrame;
                    break;

                    // All other disposition values are invalid.
                    // Raise invalid disposition exception.
                default :
                    RETAILMSG(1, (TEXT("NKDispatchException: invalid dispositon (%d)\r\n"),
                            Disposition));
                    RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                }
            } else {
                if (NextPc != ControlPc)
                    PrevSp = 0; // don't terminate loop because sp didn't change.
            }
                    
        } else {
            // Set point at which control left the previous routine.
            NextPc = ContextRecord1.IntRa - INST_SIZE;
            PrevSp = 0; // don't terminate loop because sp didn't change.

            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            if (NextPc == ControlPc)
                break;
        }

        // Set point at which control left the previous routine.
        ControlPc = NextPc;
        if (pcstk && (ControlPc == SYSCALL_RETURN || ControlPc+INST_SIZE == SYSCALL_RETURN
                || ControlPc == DIRECT_RETURN || ControlPc+INST_SIZE == DIRECT_RETURN)) {
            // Server IPC return. If the server process has registered an
            // exception handler, the call that handler before unwinding
            // in the calling process.
            PPROCESS pProc = pcstk->pprcLast;
            PEXCEPTION_ROUTINE pEH = KPSLExceptionRoutine;

            if (TargetCStk == 0 && pcstk->akyLast && ((ulong)pProc < 0x10000
                    || (pEH = pCurProc->pfnEH) != 0)) {
                TargetCStk = pcstk;
                FunctionEntry = &TempFunctionEntry;
                FunctionEntry->ExceptionHandler = pEH;
                FunctionEntry->BeginAddress = 0;
                FunctionEntry->EndAddress = 0;
                FunctionEntry->HandlerData = 0;
                EstablisherFrame = 0;
                goto invokeHandler;
            }

            // Update the return address and process context
            // information from the thread's call stack list.
            ContextRecord1.IntRa = (ULONG)pcstk->retAddr;
            RESTORE_EXTRA_INFO(&ContextRecord1, pcstk->extra);
            if ((ulong)pProc < 0x10000) {
                /* This call is returning from kernel mode server. Restore
                 * the thread's previous operating mode and return.
                 */
                SetContextMode(&ContextRecord1, (ULONG)pProc);
            } else {
                /* Returning from a user mode server, restore the previous
                 * address space information and return.
                 */
                UpdateASID(pth, pProc, pcstk->akyLast ? pcstk->akyLast : pth->aky);
            }
            ControlPc = ContextRecord1.IntRa - INST_SIZE;
            DEBUGMSG(ZONE_SEH, (TEXT("NKDispatchException: PSL ret=%8.8lx\r\n"),
                    ContextRecord1.IntRa));
            pcstk = pcstk->pcstkNext;
        }
    } while (ContextRecord1.IntSp < HighLimit && ContextRecord1.IntSp > PrevSp);

    // Set final exception flags and return exception not handled.
    DEBUGMSG(1, (TEXT("NKDispatchException: returning failure. Flags=%x\r\n"),
            ExceptionFlags));
    UpdateASID(pth, pprcSave, akySave);
    ExceptionRecord->ExceptionFlags = ExceptionFlags;

    memcpy(OriginalContext, ContextRecord, sizeof(CONTEXT));
    return FALSE;
}


#if defined(COMPRESSED_PDATA)
PRUNTIME_FUNCTION
NKLookupFunctionEntry(
IN PPROCESS pProc,
IN ULONG ControlPc,
OUT PRUNTIME_FUNCTION prf)
/*++
Routine Description:
    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:
    pProc - process context for ControlPC

    ControlPc - Supplies the address of an instruction within the specified
        function.

    prf - ptr to RUNTIME_FUNCTION structure to be filled in & returned.

Return Value:
    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.
--*/
{
    PPDATA table;
    PPDATA entry;
    PPDATA_EH peh;
    ULONG SizeOfExceptionTable;
    ULONG InstSize;
    LONG High;
    LONG Low;
    LONG Middle;

    __try {
        // Search for the image that includes the specified PC value and locate
        // its function table.
        table = PDataFromPC(ControlPc, pProc, &SizeOfExceptionTable);

        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        if (table != NULL) {
            // Initialize search indicies.
            Low = 0;
            High = (SizeOfExceptionTable / sizeof(PDATA)) - 1;

            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            while (High >= Low) {
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                Middle = (Low + High) >> 1;
                entry = &table[Middle];
                if (ControlPc < entry->pFuncStart) {
                    High = Middle - 1;

                } else if (Middle != High && ControlPc >= (entry+1)->pFuncStart) {
                    Low = Middle + 1;

                } else {
                    // The ControlPc is between the middle entry and the middle+1 entry
                    // or this is the last entry that will be examined. Check ControlPc
                    // against the function length.
                    InstSize = entry->ThirtyTwoBits ? 4 : 2;

                    prf->BeginAddress = entry->pFuncStart;
                    prf->EndAddress = entry->pFuncStart + entry->FuncLen*InstSize;
                    prf->PrologEndAddress = entry->pFuncStart + entry->PrologLen*InstSize;
                    if (ControlPc >= prf->EndAddress)
                        break;  // Not a match, stop searching

                    // Fill in the remaining fields in the RUNTIME_FUNCTION structure.
                    if (entry->ExceptionFlag) {
                        peh = (PPDATA_EH)(entry->pFuncStart & ~(InstSize-1))-1;
                        prf->ExceptionHandler = peh->pHandler;
                        prf->HandlerData = peh->pHandlerData;
                    } else {
                        prf->ExceptionHandler = 0;
                        prf->HandlerData = 0;
                    }

#if 0 // This feature is not supported by compressed pdata at this time
                    // The capability exists for more than one function entry
                    // to map to the same function. This permits a function to
                    // have discontiguous code segments described by separate
                    // function table entries. If the ending prologue address
                    // is not within the limits of the begining and ending
                    // address of the function able entry, then the prologue
                    // ending address is the address of a function table entry
                    // that accurately describes the ending prologue address.

                    if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                        (FunctionEntry->PrologEndAddress >= FunctionEntry->EndAddress)) {
                        FunctionEntry = (PRUNTIME_FUNCTION)FunctionEntry->PrologEndAddress;
                    }
#endif
                    return prf;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    // A function table entry for the specified PC was not found.
    return NULL;
}
#endif // COMPRESSED_PDATA

#if defined(OLD_PDATA)
PRUNTIME_FUNCTION
NKLookupFunctionEntry(
IN PPROCESS pProc,
IN ULONG ControlPc,
OUT PRUNTIME_FUNCTION prf)
/*++
Routine Description:
    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:
    pProc - process context for ControlPC

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:
    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.
--*/
{
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    LONG High;
    LONG Low;
    LONG Middle;

    __try {
        // Search for the image that includes the specified PC value and locate
        // its function table.
        FunctionTable = PDataFromPC(ControlPc, pProc, &SizeOfExceptionTable);

        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        if (FunctionTable != NULL) {
            // Initialize search indicies.
            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;

            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            while (High >= Low) {
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];
                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {
                    // The capability exists for more than one function entry
                    // to map to the same function. This permits a function to
                    // have discontiguous code segments described by separate
                    // function table entries. If the ending prologue address
                    // is not within the limits of the begining and ending
                    // address of the function able entry, then the prologue
                    // ending address is the address of a function table entry
                    // that accurately describes the ending prologue address.

                    if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                        (FunctionEntry->PrologEndAddress >= FunctionEntry->EndAddress)) {
                        FunctionEntry = (PRUNTIME_FUNCTION)FunctionEntry->PrologEndAddress;
                    }
                    return FunctionEntry;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    // A function table entry for the specified PC was not found.
    return NULL;
}
#endif  // OLD_PDATA

#if defined(COMBINED_PDATA)
PRUNTIME_FUNCTION
NKLookupFunctionEntry(
IN PPROCESS pProc,
IN ULONG ControlPc,
OUT PRUNTIME_FUNCTION prf)
/*++
Routine Description:
    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:
    pProc - process context for ControlPC

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:
    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.
--*/
{
    PCOMBINED_HEADER Header;
    PRUNTIME_FUNCTION OldEntry;
    PRUNTIME_FUNCTION OldTable;
    PPDATA NewEntry;
    PPDATA NewTable;
    PPDATA_EH NewEH;
    ULONG TotalSize;
    ULONG EndAddress;
    LONG NewHigh;
    LONG OldHigh;
    LONG Low;
    LONG Middle;
    UINT InstrShift;

    __try {
        // Search for the image that includes the specified PC value and locate
        // its function table.
        Header = PDataFromPC(ControlPc, pProc, &TotalSize);

        // If the exception table doesn't exist or the size is invalid, just return
        if (Header == NULL || TotalSize < sizeof (COMBINED_HEADER)) {
            return NULL;
        }

        // If the PDATA is all old style then the header will actually be the
        // first uncompressed entry and the zero fields will be non-zero
        if (Header->zero1 || Header->zero2) {
            OldTable = (PRUNTIME_FUNCTION)Header;
            OldHigh = (TotalSize / sizeof(RUNTIME_FUNCTION)) - 1;
            NewTable = NULL;
        } else {
            OldTable = (PRUNTIME_FUNCTION)(Header+1);
            OldHigh  = Header->Uncompressed - 1;
            NewTable = (PPDATA)(&OldTable[Header->Uncompressed]);
            NewHigh  = Header->Compressed - 1;
        }

        // Initialize search indicies.
        Low = 0;
                    
        // Perform binary search on the function table for a function table
        // entry that subsumes the specified PC.
        while (OldHigh >= Low) {

            // Compute next probe index and test entry. If the specified PC
            // is greater than of equal to the beginning address and less
            // than the ending address of the function table entry, then
            // return the address of the function table entry. Otherwise,
            // continue the search.
            Middle = (Low + OldHigh) >> 1;
            OldEntry = &OldTable[Middle];

            if (ControlPc < OldEntry->BeginAddress) {
                OldHigh = Middle - 1;

            } else if (ControlPc >= OldEntry->EndAddress) {
                Low = Middle + 1;

            } else {

                // The capability exists for more than one function entry
                // to map to the same function. This permits a function to
                // have discontiguous code segments described by separate
                // function table entries. If the ending prologue address
                // is not within the limits of the begining and ending
                // address of the function able entry, then the prologue
                // ending address is the address of a function table entry
                // that accurately describes the ending prologue address.
                if ((OldEntry->PrologEndAddress < OldEntry->BeginAddress) ||
                    (OldEntry->PrologEndAddress >= OldEntry->EndAddress)) {
                    OldEntry = (PRUNTIME_FUNCTION)OldEntry->PrologEndAddress;
                }
                return OldEntry;
            }
        }

        // If a new function table is located, then search the function table
        // for a function table entry for the specified PC.
        if (NewTable != NULL) {
            // Initialize search indicies.
            Low = 0;

            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            while (NewHigh >= Low) {

                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                Middle = (Low + NewHigh) >> 1;
                NewEntry = &NewTable[Middle];

                InstrShift = 1 + NewEntry->ThirtyTwoBits;
                EndAddress = NewEntry->pFuncStart + (NewEntry->FuncLen << InstrShift);

                if (ControlPc < NewEntry->pFuncStart) {
                    NewHigh = Middle - 1;

                } else if (ControlPc >= EndAddress) {
                    Low = Middle + 1;

                } else {
                    prf->BeginAddress = NewEntry->pFuncStart;
                    prf->EndAddress = EndAddress;
                    prf->PrologEndAddress = NewEntry->pFuncStart + (NewEntry->PrologLen << InstrShift);

                    if (NewEntry->ExceptionFlag) {
                        NewEH = (PPDATA_EH)NewEntry->pFuncStart - 1;
                        prf->ExceptionHandler = NewEH->pHandler;
                        prf->HandlerData = NewEH->pHandlerData;
                    } else {
                        prf->ExceptionHandler = 0;
                        prf->HandlerData = 0;
                    }

                    return prf;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    // A function table entry for the specified PC was not found.
    return NULL;
}
#endif // COMBINED_PDATA

BOOLEAN
NKUnwind(
    IN PTHREAD pth,
    IN PCALLSTACK TargetCStk OPTIONAL,
    IN ULONG TargetFrame OPTIONAL,
    IN ULONG TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG ReturnValue,
    IN PCONTEXT ContextRecord)
/*++
Routine Description:
    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

Arguments:
    pth - Supplies a pointer to thread structure

    TargetCStk - Supplies an optional pointer to a PSL callstack to unwind to.
        This parameter is used instead of TargetFrame.

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

    ContextRecord - Supplies a pointer to a context record that can be used
        to store context druing the unwind operation.

Return Value:
    None.
--*/
{
    ULONG ControlPc;
    DISPATCHER_CONTEXT DispatcherContext;
    PCONTEXT OriginalContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG EstablisherFrame;
    ULONG ExceptionFlags;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG HighLimit;
    ULONG LowLimit;
    ULONG NextPc;
    PCALLSTACK pcstk;
    RUNTIME_FUNCTION TempFunctionEntry;

    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    NKGetStackLimits(pth, &LowLimit, &HighLimit);
	pcstk = pth->pcstkTop;
    ControlPc = CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) = (ULONG)TargetIp;
    EstablisherFrame = ContextRecord->IntSp;
    OriginalContext = ContextRecord;

    // If the target frame of the unwind is specified, then a normal unwind
    // is being performed. Otherwise, an exit unwind is being performed.
    ExceptionFlags = EXCEPTION_UNWINDING;
    if (!TargetFrame && !TargetCStk)
        ExceptionFlags |= EXCEPTION_EXIT_UNWIND;

    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    do {
        DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind(%08X): Proc=%8.8lx ControlPc=%8.8lx SP=%8.8lx\r\n"),
ContextRecord,
                pCurProc, ControlPc, ContextRecord->IntSp));
        if (ControlPc == SYSCALL_RETURN || ControlPc+INST_SIZE == SYSCALL_RETURN
                || ControlPc == DIRECT_RETURN || ControlPc+INST_SIZE == DIRECT_RETURN) {
            // Server IPC return. If doing a PSL unwind, and this is the
            // target process, then terminate to unwind loop.
            PPROCESS pProc = pcstk->pprcLast;
            if (pcstk == TargetCStk) {
                DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: TargetCStk reached.\r\n")));
                break;
            }

            // Update the return address and process context
            // information from the thread's call stack list.
            ContextRecord->IntRa = (ULONG)pcstk->retAddr;
            RESTORE_EXTRA_INFO(ContextRecord, pcstk->extra);
            if ((ulong)pProc < 0x10000) {
                /* This call is returning from kernel mode server. Restore
                 * the thread's previous operating mode and return.
                 */
                SetContextMode(ContextRecord, (ULONG)pProc);
            } else {
                /* Returning from a user mode server, restore the previous
                 * address space information and return.
                 */
                UpdateASID(pth, pProc, pcstk->akyLast ? pcstk->akyLast : pth->aky);
            }
            ControlPc = ContextRecord->IntRa - INST_SIZE;           
			// Unlink the most recent CALLSTACK object
			pth->pcstkTop = pcstk->pcstkNext;
            if (IsValidKPtr(pcstk))
                FreeMem(pcstk,HEAP_CALLSTACK);
			pcstk = pth->pcstkTop;
            DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: PSL ret=%8.8lx\r\n"),
                    ContextRecord->IntRa));
        }
        // Lookup the function table entry using the point at which control
        // left the procedure.
        ControlPc = ZeroPtr(ControlPc);
        FunctionEntry = NKLookupFunctionEntry(pCurProc, ControlPc, &TempFunctionEntry);

        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the routine to obtain the virtual frame
        // pointer of the establisher, but don't update the context record.
        if (FunctionEntry != NULL) {
            NextPc = RtlpVirtualUnwind(ControlPc,
                                       FunctionEntry,
                                       ContextRecord,
                                       &InFunction,
                                       &EstablisherFrame);

            // If the virtual frame pointer is not within the specified stack
            // limits, the virtual frame pointer is unaligned, or the target
            // frame is below the virtual frame and an exit unwind is not being
            // performed, then raise the exception STATUS_BAD_STACK. Otherwise,
            // check to determine if the current routine has an exception
            // handler.
            if ((EstablisherFrame < LowLimit) || (EstablisherFrame > HighLimit) ||
                    (TargetFrame && (TargetFrame < EstablisherFrame)) ||
                    ((EstablisherFrame & STK_ALIGN) != 0)) {
                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if ((FunctionEntry->ExceptionHandler != NULL) && InFunction) {
                // The frame has an exception handler.
                // The control PC, establisher frame pointer, the address
                // of the function table entry, and the address of the
                // context record are all stored in the dispatcher context.
                // This information is used by the unwind linkage routine
                // and can be used by the exception handler itself.
                //
                // A linkage routine written in assembler is used to actually
                // call the actual exception handler. This is required by the
                // exception handler that is associated with the linkage
                // routine so it can have access to two sets of dispatcher
                // context when it is called.
                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.ContextRecord = ContextRecord;

                // Call the exception handler.
                do {
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    if ((ULONG)TargetFrame == EstablisherFrame)
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    // Set the specified return value in case the exception
                    // handler directly continues execution.
                    ContextRecord->RetValue = (ULONG)ReturnValue;
                    DEBUGMSG(ZONE_SEH, (TEXT("Calling unwind handler @%8.8lx Frame=%8.8lx\r\n"),
                            FunctionEntry->ExceptionHandler, EstablisherFrame));
                    Disposition = RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame,
                                                    ContextRecord,
                                                    &DispatcherContext,
                                                    FunctionEntry->ExceptionHandler);

                    DEBUGMSG(1, (TEXT("  disposition = %d\r\n"), Disposition));

                    // Clear target unwind and collided unwind flags.
                    ExceptionFlags &= ~(EXCEPTION_COLLIDED_UNWIND |
                                                        EXCEPTION_TARGET_UNWIND);

                    // Case on the handler disposition.
                    switch (Disposition) {
                        // The disposition is to continue the search.
                        //
                        // If the target frame has not been reached, then
                        // virtually unwind to the caller of the current
                        // routine, update the context record, and continue
                        // the search for a handler.
                    case ExceptionContinueSearch :
                        if (EstablisherFrame != (ULONG)TargetFrame) {
                            NextPc = RtlVirtualUnwind(ControlPc,
                                                      FunctionEntry,
                                                      ContextRecord,
                                                      &InFunction,
                                                      &EstablisherFrame);
                        }
                        break;

                        // The disposition is collided unwind.
                        //
                        // Set the target of the current unwind to the context
                        // record of the previous unwind, and reexecute the
                        // exception handler from the collided frame with the
                        // collided unwind flag set in the exception record.
                    case ExceptionCollidedUnwind :
                        ControlPc = DispatcherContext.ControlPc;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        ContextRecord = DispatcherContext.ContextRecord;
                        CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) = (ULONG)TargetIp;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        break;

                        // All other disposition values are invalid.
                        // Raise invalid disposition exception.
                    default :
                        RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                    }
                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            } else {
                // If the target frame has not been reached, then virtually unwind to the
                // caller of the current routine and update the context record.
                if (EstablisherFrame != (ULONG)TargetFrame) {
                    NextPc = RtlVirtualUnwind(ControlPc,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame);
                }
            }
        } else {
            // Set point at which control left the previous routine.
            NextPc = ContextRecord->IntRa - INST_SIZE;

            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            if (NextPc == ControlPc)
                break;  ///RtlRaiseStatus(STATUS_BAD_FUNCTION_TABLE);
        }

        // Set point at which control left the previous routine.
        //
        // N.B. Make sure the address is in the delay slot of the jal
        //      to prevent the boundary condition of the return address
        //      being at the front of a try body.
        ControlPc = NextPc;
    } while ((EstablisherFrame < HighLimit) &&
            (EstablisherFrame != (ULONG)TargetFrame));

    // If the establisher stack pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    if ((TargetFrame && EstablisherFrame == (ULONG)TargetFrame)
            || (TargetCStk && pcstk == TargetCStk)) {

        if ( ContextRecord != OriginalContext ){        // collided unwind
            //
            //  A collided unwind ocurred and unwinding proceeded
            //  with the collided context. Copy the collided context
            //  back into the original context structure
            //
            memcpy(OriginalContext, ContextRecord, sizeof(CONTEXT));
            ContextRecord = OriginalContext;
        }

        FIXTOC(ContextRecord);
        ContextRecord->RetValue = (ULONG)ReturnValue;
        DEBUGMSG(ZONE_SEH, (TEXT("NKUnwind: continuing @%8.8lx\r\n"),
                CONTEXT_TO_PROGRAM_COUNTER(ContextRecord)));
        return TRUE;
    } else {
        return FALSE;
    }
}

ULONG
RtlpVirtualUnwind(
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame)

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This function copies the specified context record and only computes
         the establisher frame and whether control is actually in a function.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/
{
    CONTEXT LocalContext;

    //
    // Copy the context record so updates will not be reflected in the
    // original copy and then virtually unwind to the caller of the
    // specified control point.
    //

    memcpy(&LocalContext, ContextRecord, sizeof(CONTEXT));
    return RtlVirtualUnwind(ControlPc,
                            FunctionEntry,
                            &LocalContext,
                            InFunction,
                            EstablisherFrame);
}

#endif  // defined INST_SIZE
#endif // !x86

void
UpdateASID(IN PTHREAD pth, IN PPROCESS pProc, IN ACCESSKEY aky)
/*++
Routine Description:
    This function updates the thread's current process and access key information
    from the context struture.

Arguments:
    pth - pointer to thread structure
    pctx - pointer to context record

Return Value:
    None.
--*/
{
    if (pth->pProc != pProc || pth->aky != aky) {
        pth->pProc = pProc;
        pth->aky = aky;
        SetCPUASID(pth);
    }
}

long KPSLCleanUp(void)
/*++
Routine Description:
    This function is invoked to clean up from a kernel PSL call which has faulted. It
    sets an error code and returns FALSE from the api call.

Arguments:
    none

Return Value:
    FALSE
--*/
{
    KSetLastError(pCurThread,ERROR_INVALID_ADDRESS);
    return 0;
}


EXCEPTION_DISPOSITION KPSLExceptionRoutine(
struct _EXCEPTION_RECORD *ExceptionRecord,
void *EstablisherFrame,
struct _CONTEXT *ContextRecord,
struct _DISPATCHER_CONTEXT *DispatcherContext)
/*++
Routine Description:
    This function is invoked when an exception occurs in a kernel PSL call which is
    not caught by specific handler. It simply forces an error return from the api
    call.

Arguments:
    ExceptionRecord - Supplies a pointer to an exception record.

    EstablisherFrame - Supplies a pointer to frame of the establisher function.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to the exception dispatcher or
        unwind dispatcher context.

Return Value:
    An exception disposition value of execute handler is returned.
--*/
{
	extern void SurrenderCritSecs(void);
    RETAILMSG(1, (TEXT("KPSLExceptionHandler: flags=%x ControlPc=%8.8lx\r\n"),
            ExceptionRecord->ExceptionFlags, DispatcherContext->ControlPc));

	SurrenderCritSecs();
    // If an unwind is not in progress, then return the address of the psl cleanup
    // routine in DispatcherContext->ControlPc.
    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags)
            && ExceptionRecord->ExceptionCode != STATUS_INVALID_SYSTEM_SERVICE
            && ExceptionRecord->ExceptionCode != ERROR_INVALID_PARAMETER) {
        DispatcherContext->ControlPc = (DWORD)KPSLCleanUp;
        return ExceptionExecuteHandler;
    }

    // Continue search for exception or termination handlers.
    return ExceptionContinueSearch;
}

