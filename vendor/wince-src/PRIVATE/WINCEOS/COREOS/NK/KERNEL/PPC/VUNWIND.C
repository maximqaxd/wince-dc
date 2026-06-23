/*++

Copyright (c) 1993-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    vunwind.c

Abstract:

    This module contains the instruction classifying and virtual
    unwinding routines for structured exception handling on PowerPC.

    Virtual Unwind was moved to this file from exdsptch.c so that it
    can be used directly in the kernel debugger.

 --*/

#include "kernel.h"

typedef double *PDOUBLE;

#define READ_ULONG(addr,dest) ((dest) = (*(PULONG)(addr)))
#define READ_DOUBLE(addr,dest) ((dest) = (*(PDOUBLE)(addr)))

#if 0 ///ifdef _IMAGEHLP_SOURCE_
#define FUNCTION_ENTRY_IS_IMAGE_STYLE
#define NOT_IMAGEHLP(E)
#else
#define NOT_IMAGEHLP(E) E
#endif

extern PRUNTIME_FUNCTION NKLookupFunctionEntry(
    IN PPROCESS pProc,
    IN ULONG ControlPc,
    PRUNTIME_FUNCTION prf);

//
// The `ClassifyInstruction' function returns an enum that identifies
// the type of processing needed for an instruction.
//

typedef enum _INSTR_CLASS {
    InstrIgnore,        // Do not process
    InstrMFLR,          // Move from Link Register
    InstrMFCR,          // Move from Condition Register
    InstrSTW,           // Store word
    InstrSTWU,          // Store word with update
    InstrSTWUr12,       // Store word with update during UnwindR12
    InstrSTFD,          // Store float double
    InstrMR,            // Move register
    InstrMRr12,         // Move register during UnwindR12
    InstrMRfwd,         // Move register during UnwindForward
    InstrADDIr12,       // Add immediate during UnwindR12
    InstrADDIfwd,       // Add immediate during UnwindForward
    InstrSaveCode,      // Branch and link to GPR or FPR saving millicode
    InstrRestoreCode,   // Branch to GPR or FPR saving millicode
    InstrGlue,          // Branch or Branch and link to glue code
    InstrBLR,           // Branch to Link Register
    InstrSetEstablisher // Special instruction used to set establisher frame
} INSTR_CLASS;

//
// If `ClassifyInstruction' returns `InstrSaveCode' or `InstrRestoreCode',
// the following information is completed.
//

typedef struct _MILLICODE_INFO {
    ULONG  TargetPc;    // Millicode entry point
    PRUNTIME_FUNCTION FunctionEntry; // Millicode function table entry
    RUNTIME_FUNCTION  FETemp;       // storage to hold FE if using compressed PData
} MILLICODE_INFO, *PMILLICODE_INFO;

//
// `ClassifyInstruction' interprets the instruction based on the intent.
//

typedef enum _UNWIND_INTENT {
    UnwindForward,      // Performing a forward execution
    UnwindR12,          // Performing a reverse execution to get r.12
    UnwindReverse,      // Performing a reverse execution
    UnwindReverseR12    // Performing a reverse execution allowing r.12
} UNWIND_INTENT;

//
// The simulated execution by `RtlVirtualUnwind' is controlled by this
// data type.
//

typedef struct _ITERATOR {
    ULONG BeginPc;      // Address of first instruction to simulate
    ULONG EndPc;        // Address after the last instruction to simulate
    LONG  Increment;    // Simulation direction
    UNWIND_INTENT Intent; // Simulation intent
} ITERATOR, *PITERATOR;

#define GPR1     1      // GPR 1 in an RA, RB, RT, etc. field
#define GPR2     2      // GPR 2 in an RA, RB, RT, etc. field
#define GPR12    12     // GPR 12 in an RA, RB, RT, etc. field
#define LINKREG  0x100  // Link Reg in a MFSPR instruction

static INSTR_CLASS
ClassifyInstruction (PPC_INSTRUCTION *I,
                     UNWIND_INTENT Intent,
                     ULONG Pc,
                     PMILLICODE_INFO Info)
/*++

Routine description:
    This function inspects the instruction identified by the "Pc"
    argument and determines what sort of processing is needed in order
    to simulate its execution.  Some instructions can be safely
    ignored altogether, in which case "InstrIgnore" is returned.  For
    others, a value is returned indicating what kind of instruction
    was found.  The interpreation depends on the value of "Intent".

Arguments:
    I - Address of a struct containing the instruction to be examined.
    Intent - Type of unwinding being performed.
    Pc - Address of the instruction, used for computing relative branch
        addresses.
    Info - Address to store a description of the register save/restore
        millicode.

Return value:
    One of the enum values defined above is returned.

 --*/

{
// Unique value combining an opcode and an UNWIND_INTENT value.
#define OP_INTENT(OP,INTENT) ((OP) << 2 | (INTENT))

    switch (OP_INTENT (I->Primary_Op, Intent)) {

      //
      // Store word: recognize "stw r.n, disp(r.1)".  Allow a base of
      // r.12 if we have computed its value.
      //
      case OP_INTENT (STW_OP, UnwindReverseR12):
        if (I->Dform_RA == GPR12)
            return InstrSTW;
        // fall thru
      case OP_INTENT (STW_OP, UnwindReverse):
        if (I->Dform_RA == GPR1)
            return InstrSTW;
        break;

      //
      // Load word:  recognize "lwz r.n, disp(r.x)" in epilogue millicode.
      //
      case OP_INTENT (LWZ_OP, UnwindForward):
        return InstrSTW;

      //
      // Store word with update:  recognize "stwu r.1, r.1, disp"
      //
      case OP_INTENT (STWU_OP, UnwindReverse):
      case OP_INTENT (STWU_OP, UnwindReverseR12):
      case OP_INTENT (STWU_OP, UnwindR12):
        if (I->Dform_RS == GPR1 &&
            I->Dform_RA == GPR1)
            return (Intent == UnwindR12 ? InstrSTWUr12 : InstrSTWU);
        break;

      //
      // Store float double: recognize "stfd f.n, disp(r.1)".  Allow a
      // base of r.12 if we have computed its value.
      //
      case OP_INTENT (STFD_OP, UnwindReverseR12):
        if (I->Dform_RA == GPR12)
            return InstrSTFD;
        // fall thru
      case OP_INTENT (STFD_OP, UnwindReverse):
        if (I->Dform_RA == GPR1)
            return InstrSTFD;
        break;

      //
      // Load float double:  recognize "lfd f.n, disp(r.x)"
      //
      case OP_INTENT (LFD_OP, UnwindForward):
        return InstrSTFD;

      //
      // Add immediate:  recognize "addi r.12, r.1, delta"
      //
      case OP_INTENT (ADDI_OP, UnwindR12):
        if (I->Dform_RS == GPR12 &&
            I->Dform_RA == GPR1)
            return InstrADDIr12;
        break;
      case OP_INTENT (ADDI_OP, UnwindForward):
        return InstrADDIfwd;

      //
      // Branch (long form):  recognize "bl[a] saveregs"and "b[a] restregs"
      //
      case OP_INTENT (B_OP, UnwindReverse):
        //
        // Compute branch target address, allowing for branch-relative
        // and branch-absolute.
        //
        Pc = ((LONG)(I->Iform_LI) << 2) + (I->Iform_AA ? 0 : Pc);

        //
        // Determine whether the target address is part of a register
        // save or register restore sequence or is a direct branch out
        // by checking it's function table entry.
        //
        if ((Info->FunctionEntry = NKLookupFunctionEntry(pCurProc, Pc, &Info->FETemp)) != NULL
                && Info->FunctionEntry->ExceptionHandler == 0) {
            Info->TargetPc = Pc;
            switch ((ULONG)Info->FunctionEntry->HandlerData) {
            case 1:
                if (I->Iform_LK)
                    return InstrSaveCode;
                break;
            case 2:
                if (!I->Iform_LK)
                    return InstrRestoreCode;
                break;
            case 3:
                return InstrGlue;
            }
        }
        break; // unrecognized entry point

      //
      // Extended ops -- primary opcode 19
      //
      case OP_INTENT (X19_OP, UnwindForward):
        //
        // BLR: recognized "bclr 20,0".
        //
        if (I->Long == RETURN_INSTR)
            return InstrBLR;
        break;

      case OP_INTENT (X19_OP, UnwindR12):
      case OP_INTENT (X19_OP, UnwindReverse):
      case OP_INTENT (X19_OP, UnwindReverseR12):
        //
        // RFI: this instruction is used in special kernel fake prologues
        //      to indicate that the establisher frame address should be
        //      updated using the current value of sp.
        //
        if (I->Xform_XO == RFI_OP)
            return InstrSetEstablisher;
        break;

      //
      // Extended ops -- primary opcode 31
      //
      case OP_INTENT (X31_OP, UnwindForward):
      case OP_INTENT (X31_OP, UnwindR12):
      case OP_INTENT (X31_OP, UnwindReverse):
      case OP_INTENT (X31_OP, UnwindReverseR12):
        switch (OP_INTENT (I->Xform_XO, Intent)) {

          //
          // OR register: recognize "or r.x, r.y, r.y" as move-reg
          //
          case OP_INTENT (OR_OP, UnwindReverse):

            if (I->Xform_RS == I->Xform_RB)
                return InstrMR;
            break;

          case OP_INTENT (OR_OP, UnwindForward):
            if (I->Xform_RS == I->Xform_RB)
                return InstrMRfwd;
            break;

          //
          // Store word with update indexed:  recognize "stwux r.1, r.1, r.x"
          //
          case OP_INTENT (STWUX_OP, UnwindReverse):
          case OP_INTENT (STWUX_OP, UnwindReverseR12):
          case OP_INTENT (STWUX_OP, UnwindR12):
            if (I->Xform_RS == GPR1 && I->Xform_RA == GPR1)
                return (Intent == UnwindR12 ? InstrSTWUr12 : InstrSTWU);
            break;

          //
          // Move to/from special-purpose reg:  recognize "mflr", "mtlr"
          //
          case OP_INTENT (MFSPR_OP, UnwindReverse):
          case OP_INTENT (MTSPR_OP, UnwindForward):
            if (I->XFXform_spr == LINKREG)
                return InstrMFLR;
            break;

          //
          // Move from Condition Register:  "mfcr r.x"
          //
          case OP_INTENT (MFCR_OP, UnwindReverse):
          case OP_INTENT (MFCR_OP, UnwindReverseR12):
            return InstrMFCR;

          //
          // Move to Condition Register:  "mtcrf 255,r.x"
          //
          case OP_INTENT (MTCRF_OP, UnwindForward):
            if (I->XFXform_FXM == 255)
                return InstrMFCR;
            break;

          default:              // unrecognized
            break;
        }

      default:                  // unrecognized
        break;
    }

    //
    // Instruction not recognized; just ignore it and carry on
    //
    return InstrIgnore;
#undef OP_INTENT
}

ULONG
RtlVirtualUnwind(
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
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

    If the function is register save millicode, it is treated as a leaf
    function.  If the function is register restore millicode, the remaining
    body is executed forwards and the address where control left the
    previous frame is obtained from the final blr instruction.

    If the function was called via glue code and is not that glue code,
    the prologe of the glue code is executed backwards in addition to the
    above actions.

    Otherwise, an exception or interrupt entry to the system is being
    unwound and a specially coded prologue restores the return address
    twice. Once from the fault instruction address and once from the saved
    return address register. The first restore is returned as the function
    value and the second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:
    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function or NULL if the function is a leaf function.

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
    ITERATOR Iterator[8];
    PITERATOR Piterator;
    ULONG Address;
    PDOUBLE FloatingRegister;
    PPC_INSTRUCTION I;
    PULONG IntegerRegister;
    ULONG NextPc, Pc;
    BOOLEAN RestoredLr = FALSE;
    BOOLEAN RestoredSp = FALSE;
    BOOLEAN ComputedSp = FALSE;
    ULONG Rt;
    MILLICODE_INFO Info;
    INSTR_CLASS InstrClass;
    ULONG EstablisherFrameValue;

	//
    // Set the base address of the integer and floating register arrays.
    //

    FloatingRegister = &ContextRecord->Fpr0;
    IntegerRegister = &ContextRecord->Gpr0;

    //
    // If the function is a leaf function, perform the default unwinding
    // action and check to see if the function was called via glue.
    //
    if (FunctionEntry == NULL) {
        //
        // Set point at which control left the previous routine.
        //

        return( ContextRecord->Lr - 4 );
    }
    //
    // Set initial values for EstablisherFrame and Offset.
    // (this may need more careful planning IBMPLJ).
    //

    NOT_IMAGEHLP (*EstablisherFrame =
                  EstablisherFrameValue = ContextRecord->Gpr1);

    READ_ULONG (ControlPc, I.Long);
    if (I.Long == RETURN_INSTR) {
        //
        // If the instruction at the point where control left the specified
        // function is a return, then any saved registers have been restored
        // and the control PC is not considered to be in the function
        // (i.e., in an epilogue).
        //
        NOT_IMAGEHLP(*InFunction = FALSE);
        return( ContextRecord->Lr );
    }
    InstrClass = ClassifyInstruction(&I, UnwindReverse, ControlPc, &Info);
    if (InstrClass == InstrRestoreCode) {
        //
        // If the instruction at the point where control left the
        // specified function is a branch to register restore
        // millicode, the state is restored by simulating the
        // execution of the restore millicode.  The control PC is in
        // an epilogue.
        //
        Iterator[0].BeginPc = Info.TargetPc;
        Iterator[0].EndPc = Info.FunctionEntry->EndAddress;
        Iterator[0].Increment = 4;
        Iterator[0].Intent = UnwindForward;
        NOT_IMAGEHLP(*InFunction = FALSE);

    } else if (FunctionEntry->ExceptionHandler == 0 &&
               (ULONG)FunctionEntry->HandlerData == 2) {
        //
        // If the address is in register restore millicode, the state
        // is restored by completing the execution of the restore
        // millicode.  The control PC is in an epilogue.
        //
        Iterator[0].BeginPc = ControlPc;
        Iterator[0].EndPc = FunctionEntry->EndAddress;
        Iterator[0].Increment = 4;
        Iterator[0].Intent = UnwindForward;
        NOT_IMAGEHLP(*InFunction = FALSE);

    } else {

        //
        // If the address where control left the specified function is
        // outside the limits of the prologue, then the control PC is
        // considered to be within the function and the control
        // address is set to the end of the prologue. Otherwise, the
        // control PC is not considered to be within the function
        // (i.e., in the prologue).
        //
        Iterator[0].EndPc = FunctionEntry->BeginAddress - 4;
        Iterator[0].Increment = -4;
        Iterator[0].Intent = UnwindReverse;
        if (ControlPc < FunctionEntry->BeginAddress
                || ControlPc >= (FunctionEntry->PrologEndAddress & ~3)) {
            NOT_IMAGEHLP(*InFunction = TRUE);
            Iterator[0].BeginPc = ((FunctionEntry->PrologEndAddress & ~3) - 4);
        } else {
            NOT_IMAGEHLP(*InFunction = FALSE);
            Iterator[0].BeginPc = ControlPc - 4;
        }
    }

    //
    // Scan through the given instructions and reload callee registers
    // as indicated.
    //
    NextPc = ContextRecord->Lr - 4;
    for (Piterator = Iterator; Piterator >= Iterator; Piterator--) {

        for (Pc = Piterator->BeginPc;
             Pc != Piterator->EndPc;
             Pc += Piterator->Increment) {

            READ_ULONG (Pc, I.Long);
            Address = IntegerRegister[I.Dform_RA] + I.Dform_D;
            Rt = I.Dform_RT;
            switch (ClassifyInstruction (&I, Piterator->Intent, Pc, &Info)) {

              //
              // Move from Link Register (save LR in a GPR)
              //
              // In the usual case, the link register gets set by a call
              // instruction so the PC value should point to the
              // instruction that sets the link register.  In an interrupt
              // or exception frame, the link register and PC value are
              // independent.  By convention, fake prologues for these
              // frames store the link register twice: once to the link
              // register location, once to the faulting PC location.
              //
              // If this is the first time that RA is being restored,
              // then set the address of where control left the previous
              // frame. Otherwise, this is an interrupt or exception and
              // the return PC should be biased by 4 and the link register
              // value should be updated.
              //
              case InstrMFLR:

                ContextRecord->Lr = IntegerRegister[Rt];
                if ( RestoredLr == FALSE ) {
                    NextPc = ContextRecord->Lr - 4;
                    RestoredLr = TRUE;
                } else {
                    NextPc += 4;
                }
                continue; // Next PC

              //
              // Branch to Link Register (forward execution).
              //
              case InstrBLR:
                NextPc = ContextRecord->Lr - 4;
                break; // Terminate simulation--start next iterator.

              //
              // Move from Condition Register (save CR in a GPR)
              //
              case InstrMFCR:
                ContextRecord->Cr = IntegerRegister[Rt];
                continue; // Next PC

              //
              // Store word (save a GPR)
              //
              case InstrSTW:
              //
              // Even though a stw  r.sp, xxxx in general is an invalid
              // proloque instruction there are places in the kernel
              // fake prologues (KiExceptionExit) where we must use this,
              // so handle it.
              //
				READ_ULONG (Address, IntegerRegister[Rt]);
				// Check for an SP update
				if ( Rt == GPR1 )
					RestoredSp = TRUE;
                continue; // Next PC

              //
              // Store word with update, Store word with update indexed
              // (buy stack frame, updating stack pointer and link
              // cell in storage)
              //
              case InstrSTWU:
				// STWU should only be used for updating SP
				DEBUGCHK(I.Dform_RT == GPR1);
				DEBUGCHK(I.Dform_RA == GPR1);
				  
				// Do not update SP twice.  CaptureContext is an example of 
				// a prolog where only the last sp update (the first one the 
				// unwinder encounters) should be unwound.
				if ( RestoredSp == FALSE )
				{
					Address = IntegerRegister[GPR1];
					READ_ULONG(Address,IntegerRegister[GPR1]);
					
                    *EstablisherFrame = ContextRecord->Gpr1;
					EstablisherFrameValue = ContextRecord->Gpr1;
                    RestoredSp = TRUE;
				}
                continue; // Next PC

              //
              // Store floating point double (save an FPR)
              //
              case InstrSTFD:
                READ_DOUBLE (Address, FloatingRegister[Rt]);
                continue; // Next PC

              //
              // Move register.  Certain forms are ignored based on the intent.
              //
              case InstrMR:
                IntegerRegister[Rt] = IntegerRegister[I.Xform_RA];
                continue; // Next PC
              case InstrMRfwd:
                IntegerRegister[I.Xform_RA] = IntegerRegister[Rt];
                continue; // Next PC

              //
              // Add immediate.  Certain forms are ignored based on the intent.
              //
              case InstrADDIfwd:
                IntegerRegister[Rt] = Address;
                continue; // Next PC
              case InstrADDIr12:
                if (!ComputedSp) {
                  // No intervening instruction changes r.1, so compute
                  // addi r.12,r.1,N instead of addi r.12,r.12,N.
                  IntegerRegister[Rt] = IntegerRegister[GPR1];
                }
                IntegerRegister[Rt] += I.Dform_SI;
                break; // Terminate search--start next iterator.

              //
              // Store with update while searching for the value of r.12
              //
              case InstrSTWUr12:
                ComputedSp = TRUE;
                Address = IntegerRegister[GPR1];
                READ_ULONG(Address,IntegerRegister[GPR12]);
                continue; // Next PC

              //
              // A call to a register save millicode.
              //
              case InstrSaveCode:

                //
                // Push an iterator to incorporate the actions of the
                // millicode.
                //
                Piterator++;
                Piterator->BeginPc = Info.FunctionEntry->EndAddress - 4;
                Piterator->EndPc = Info.TargetPc - 4;
                Piterator->Increment = -4;
                Piterator->Intent = UnwindReverseR12;
                //
                // Push an iterator to determine the current value of r.12
                //
                Piterator++;
                Piterator->BeginPc = Pc - 4;
                Piterator->EndPc = Piterator[-2].EndPc;
                Piterator->Increment = -4;
                Piterator->Intent = UnwindR12;
                ComputedSp = FALSE;
                //
                // Update the start of the original iterator so it can later
                // resume where it left off.
                //
                Piterator[-2].BeginPc = Pc - 4;
                Piterator++;
                break; // Start the next iterator.

              //
              // A branch was encountered in the prologue to a routine
              // identified as glue code.  This should only happen from
              // fake prologues such as those in the system exception
              // handler.
              //
              // We handle it by pushing an iterator to incorporate
              // the actions of the glue code prologue.
              //
             case InstrGlue:
                //
                // Check that we don't nest too deeply.  Verify that
                // we can push this iterator and the iterators for a
                // glue sequence.  There's no need to make this check
                // elsewhere because B_OP is only recognized during
                // UnwindReverse.  Returing zero is the only error action
                // we have.
                //

                if (Piterator - Iterator + 4
                    > sizeof (Iterator) / sizeof (Iterator[0]))
                    return 0;
                //
                // Push an iterator to incorporate the actions of the glue
                // code's prologue.  Check that we don't nest too deeply.
                // Verify that we can push this iterator and the iterators
                // for a glue sequence.
                //
                Piterator++;
                Piterator->BeginPc
                  = (Info.FunctionEntry->PrologEndAddress & ~3) - 4;
                Piterator->EndPc = Info.FunctionEntry->BeginAddress - 4;
                Piterator->Increment = -4;
                Piterator->Intent = UnwindReverse;
                //
                // Update the start of the original iterator so it can later
                // resume where it left off.
                //
                Piterator[-1].BeginPc = Pc - 4;
                Piterator++;
                break; // Start the next iterator.

              //
              // Special "set establisher" instruction (rfi).
              //
              // Kernel fake prologues that can't use stwu (KiExceptionExit,
              // KiAlternateExit) use an rfi instruction to tell the
              // unwinder to update the establisher frame pointer using
              // the current value of sp.
              //
              case InstrSetEstablisher:
                NOT_IMAGEHLP (*EstablisherFrame =
                                  EstablisherFrameValue = ContextRecord->Gpr1);
                continue; // Next PC

              //
              // None of the above.  Just ignore the instruction.  It
              // is presumed to be non-prologue code that has been
              // merged into the prologue for scheduling purposes.  It
              // may also be improper code in a register save/restore
              // millicode routine or unimportant code when
              // determining the value of r.12.
              //
              case InstrIgnore:
              default:
                continue; // Next PC
            }
            break; // Start the next iterator.
        } // end foreach Pc
    } // end foreach Iterator

    return NextPc;
}

#undef NOT_IMAGEHLP

