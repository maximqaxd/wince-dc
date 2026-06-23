//      TITLE("Jump to Unwind")
//++
//
// Copyright (c) 1992-2000 Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    jmpuwind.s
//
// Abstract:
//
//    This module implements the MIPS specific routine to jump to the runtime
//    time library unwind routine.
//
// Environment:
//
//    Any mode.
//
//--

#include "ksppc.h"

//++
//
// ULONG
// __C_ExecuteExceptionFilter (
//    PEXCEPTION_POINTERS ExceptionPointers,
//    EXCEPTION_FILTER ExceptionFilter,
//    ULONG EstablisherFrame
//    )
//
// Routine Description:
//
//    This function calls the specified exception filter routine with the
//    establisher frame passed in r12
//
// Arguments:
//
//    ExceptionPointers (r.3) - Supplies a pointer to the exception pointers
//       structure.
//
//    ExceptionFilter (r.4) - Supplies the address of the exception filter
//       routine.
//
//    EstablisherFrame (r.5) - Supplies the establisher frame pointer.
//
// Return Value:
//
//    The value returned by the exception filter routine.
//
//--

//++
//
// VOID
// __C_ExecuteTerminationHandler (
//    BOOLEAN AbnormalTermination,
//    TERMINATION_HANDLER TerminationHandler,
//    ULONG EstablisherFrame
//    )
//
// Routine Description:
//
//    This function calls the specified termination handler routine with the
//    establisher frame passed in r12.
//
// Arguments:
//
//    AbnormalTermination (r.3) - Supplies a boolean value that determines
//       whether the termination is abnormal.
//
//    TerminationHandler (r.4) - Supplies the address of the termination
//       handler routine.
//
//    EstablisherFrame (r.5) - Supplies the establisher frame pointer.
//
// Return Value:
//
//    None.
//
//--
        SPECIAL_ENTRY(__C_ExecuteExceptionFilter)
        ALTERNATE_ENTRY(__C_ExecuteTerminationHandler)

        PROLOGUE_END(__C_ExecuteExceptionFilter)

        mtctr   r4                              // set address of filter
        mr      r12, r5                         // pass establisher frame
        bctr                                    // branch to filter routine

        SPECIAL_EXIT(__C_ExecuteExceptionFilter)

