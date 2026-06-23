/*++
    Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    thmunwnd.c

Abstract:

    This module implements the unwinding of Thumb mode procedure call frames
    for exception processing.

--*/

#include "kernel.h"
#include "arminst.h"
#include <stdlib.h>

extern BOOL ReadMemory(void *src, void *dest, int length);      // unwind.c

ULONG
ThumbVirtualUnwind(
                   IN ULONG ControlPc,
                   IN PRUNTIME_FUNCTION FunctionEntry,
                   IN OUT PCONTEXT Context,
                   OUT PBOOLEAN InFunction,
                   OUT PULONG EstablisherFrame
                  )
/*++

Routine Description:

    This is the Thumb specific implementation of RtlVirtualUnwind

Arguments:

    See RtlVirtualUnwind

Return Value:

    See RtlVirtualUnwind


--*/
{
    THUMBI  instr;
    ARMI    instrA;
    PULONG  RegPtr;
    PULONG  SrcPtr, DestPtr;
    ULONG   SpAdjReg, SpAdjVal;
    ULONG   BlOffs;
    ULONG   BeginAddress;
    ULONG   RegBit;
    ULONG   Data;
    
    
#if 0
    NKDbgPrintfW(L"Unwind: Pc=%8.8x Begin=%8.8x PrologEnd=%8.8x End=%8.8x\r\n",
        ControlPc, FunctionEntry->BeginAddress, FunctionEntry->PrologEndAddress,
        FunctionEntry->EndAddress);
#endif
    *InFunction = FALSE;
    
    if (FunctionEntry->PrologEndAddress-FunctionEntry->BeginAddress == 0) {
        // No prolog.
        *EstablisherFrame = Context->Sp;
        return Context->Lr - 4;
    }
    
    //
    // Check to see if we're already in the prolog.
    //
    if ( ControlPc >= FunctionEntry->PrologEndAddress ){

        *InFunction = TRUE;
        ControlPc = FunctionEntry->PrologEndAddress - 2;

    }

    //
    // Reverse execute starting with the last instruction in the prolog
    // that has been executed.
    //

    ControlPc &= ~0x01;                                 // Clear Thumb bit
    BeginAddress = FunctionEntry->BeginAddress & ~0x01;

    SpAdjVal = 0;                                       // No SP Sequence
    while ( ControlPc >= BeginAddress ){

        if ( !ReadMemory((PVOID)ControlPc, &instr, sizeof(instr)) ){
            return ControlPc;
        }

		if ( ThumbPushInst(instr.instruction) ){

            //
            //  Push zero or more GPRs or the Link Register. The GPRs to
            //  push are specified in an eight bit register mask. The L bit
            //  specifies that the LR is to be pushed.
            //

            RegPtr = &Context->R0;
            for ( RegBit = 0x01; RegBit < 0x100; RegBit <<= 1 ){

                if ( instr.push.reglist & RegBit ){

                    if ( !ReadMemory((PVOID)Context->Sp, RegPtr, 4) ){
                        return(ControlPc);
                    }
                    Context->Sp += 4;

                    DEBUGMSG(ZONE_SEH, (L"Push R%d = 0x%x. Sp=0x%x\r\n",
                                RegPtr-&Context->R0, *RegPtr, Context->Sp));
                }
                RegPtr ++;
            }

            if ( instr.push.r == 1 ){

                if ( !ReadMemory((PVOID)Context->Sp, &Context->Lr, 4) ){
                    return(ControlPc);
                }
                Context->Sp += 4;

                DEBUGMSG(ZONE_SEH, (L"Push LR = 0x%x. Sp=0x%x\r\n",
                                Context->Lr, Context->Sp));

            }

		} else if ( ThumbAdjSPInst(instr.instruction) ){

            //
            //  Adjust the stack pointer by adding or subtracting a scaled
            //  7-bit immediate (immediate is shifted by 2 bits)
            //

            if ( instr.adjsp.s ){                           // Reverse execute
                Context->Sp += instr.adjsp.immed << 2;
            } else {
                Context->Sp -= instr.adjsp.immed << 2;
            }

            DEBUGMSG(ZONE_SEH, (L"SP += 0x%02x == 0x%x\r\n",
                                        instr.adjsp.immed << 2, Context->Sp));

		} else if ( ThumbAddSPInst(instr.instruction) ){

            //
            //  If the stack frame size exceeds 508 bytes a sequence of
            //  instructions is used to load the stack frame size into a
            //  register and then update the stack pointer.
            //  If an Add Hi instruction is found which updates the stack
            //  pointer, record the source register until the unwind reaches
            //  the load instruction which sets the source register.
            //

            SpAdjReg = instr.spcdataproc.rm;
            SpAdjVal = 1;

            DEBUGMSG(ZONE_SEH, (L"Add Sp Instruction: Source Reg = R%d\r\n",
                                                        instr.spcdataproc.rm));

		} else if ( ThumbMovHiInst(instr.instruction) ){

            //
            //  Move Register Rm -> Rd.
            //
            //  One or both registers are high registers, as specified
            //  by H1 (Rd) and H2 (Rm)
            //

            RegPtr = &Context->R0;
            SrcPtr = &RegPtr[instr.spcdataproc.rm + (instr.spcdataproc.H2 * 8)];
            DestPtr = &RegPtr[instr.spcdataproc.rd + (instr.spcdataproc.H1*8)];

            DEBUGMSG(ZONE_SEH, (L"Move Hi: R%d (0x%08x) -> R%d (0x%08x)\r\n",
                                                DestPtr-RegPtr, *DestPtr,
                                                SrcPtr-RegPtr, *SrcPtr));

            *SrcPtr = *DestPtr;                     // Reverse Execute

		} else if ( ThumbLdrPCInst(instr.instruction) ){

            //
            //  Load from Literal Pool into register. This instruction is
            //  used in the prologue for functions with large stack frames.
            //  The stack frame size is loaded from the literal pool before
            //  being used to update the stack pointer.
            //


            if ( SpAdjVal != 0 && (instr.ldrpc.rd == SpAdjReg) ){

                SrcPtr = (PULONG)((ControlPc & ~0x03) + 4 +
                                                    (instr.ldrpc.offset * 4));

                if ( !ReadMemory((PVOID)SrcPtr, &Data, 4) ){
                    return(ControlPc);
                }

                Context->Sp -= (Data * SpAdjVal);   // Reverse Execute Add inst.
                SpAdjVal = 0;                       // No SP adjust sequence
            }

            DEBUGMSG(ZONE_SEH, (L"Load Literal: Addr=0x%08x  Val=0x%x\r\n",
                                                                SrcPtr, Data));

		} else if ( ThumbNegInst(instr.instruction) ){

            //
            //  The instruction sequence to create a large stack frame may
            //  include a negate instruction. If detected, update the SpAdjVal
            //  variable:
            //

            if ( SpAdjVal != 0 && (instr.dataproc.rd == SpAdjReg) ){

                SpAdjVal = -1;

            }
            DEBUGMSG(ZONE_SEH, (L"Negate Instruction: rm=R%d rd=%d\r\n",
                                        instr.dataproc.rm, instr.dataproc.rd));

		} else if ( ThumbBlPrefInst(instr.instruction) ){

            //
            // The first instruction of a BL sequence has been found. update
            // the BlOffs value which was initialized in the ThumbBlInst case.
            // Only calls to register save millicode routines are valid within
            // the prologue. Register save routines have the form:
            //
            //  bx  pc              ; Thumb instruction
            //  nop                 ; Thumb instruction
            //  stm sp!, {reglist}  ; ARM instruction
            //  bx  lr              ; ARM instruction
            //
            // The following code loads the instruction at the function
            // destination address plus four bytes and verifies that a store
            // multiple instruction exists at that location. If not, this is
            // a bad function call and unwinding stops. Otherwise the store
            // multiple instruction is unwound
            //

            BlOffs += (instr.bl.offset << 12) + 4;
            if ( BlOffs & (0x01 << 22) ){
                BlOffs |= 0xFF800000;               // sign extend the offset
            }

            DEBUGMSG(ZONE_SEH, (L"BL Instruction: Destination=0x%08x\r\n",
                                                    ControlPc+BlOffs));

            // verify stmdb sp!, {reglist}:

            BlOffs += 4;
            if ( !ReadMemory((PVOID)(ControlPc+BlOffs), &instrA, 4) ||
                  (instrA.instruction & 0xFFFF0000) != 0xE92D0000 ){
                return(ControlPc);
            }

            DEBUGMSG(ZONE_SEH, (L"Register Save Instruction @%08x = 0x%08x\r\n",
                                        ControlPc+BlOffs, instrA.instruction));

            RegPtr = &Context->R0;
            for ( RegBit = 0x01; RegBit < 0x10000; RegBit <<= 1 ){

                if ( instrA.ldm.reglist & RegBit ){

                    if ( !ReadMemory((PVOID)Context->Sp, RegPtr, 4) ){
                        return(ControlPc);
                    }
                    Context->Sp += 4;

                    DEBUGMSG(ZONE_SEH, (L" Save R%d = 0x%x. Sp=0x%x\r\n",
                                RegPtr-&Context->R0, *RegPtr, Context->Sp));
                }
                RegPtr ++;
            }

		} else if ( ThumbBlInst(instr.instruction) ){

            BlOffs = instr.bl.offset << 1;

            DEBUGMSG(ZONE_SEH, (L"BL Instruction: offset=0x%03x\r\n",
                                                            instr.bl.offset));

		} else if ( ThumbBxInst(instr.instruction) ){

            NKDbgPrintfW(L"BX Instruction: ControlPc=0x%x  instr=0x%x\r\n",
                                             ControlPc, instr.instruction);
        } else {

            DEBUGMSG(ZONE_SEH, (L"Unknown Prolog Instruction Ignored. ControlPc=0x%08x  Inst=0x%04x\r\n", ControlPc, instr.instruction));

        }

        ControlPc -= sizeof(instr);
    }
    
    *EstablisherFrame = Context->Sp;
    
    return Context->Lr - 4;                 // Address of calling instruction
}
