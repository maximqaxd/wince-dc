/*++
    Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    shunwind.c

Abstract:
    This module implements the unwinding of procedure call frames for
    exception processing on Hitachi SH-3 & SH-4.
--*/

#include "kernel.h"
#include "shxinst.h"

#define sp   0xf                      /* register 15 */

// The saved registers are the permanent general registers (ie. those
// that get restored in the epilog)
#define SAVED_INTEGER_MASK 0x0000ff00 /* saved integer registers */
#define IS_REGISTER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)

#if SH4
#define SAVED_FLOATING_MASK 0x0000f000 /* saved floating pt registers */
#define IS_FLOAT_REGISTER_SAVED(Register) \
            ((SAVED_FLOATING_MASK >> Register) & 1L)
#define FR_MASK 0x00200000             /* masks FR bit in fpscr */
#define SZ_MASK 0x00100000             /* masks SZ bit in fpscr */
#define PR_MASK 0x00080000             /* masks PR bit in fpscr */
#endif


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

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

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

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:
    The address where control left the previous frame is returned as the
    function value.
--*/
{
    ULONG            Address;
    ULONG            TemporaryValue;
    ULONG            TemporaryRegister;
    SH3IW            Instruction;
    USHORT           instrProlog;
    PULONG           IntegerRegister;
#if SH4
    PULONG           FloatingRegister;
#endif
    ULONG            NextPc;
    ULONG            Offset;
    ULONG            Rm;
    ULONG            Rn;

    LONG             PendingValue;
    BOOL             PendingAdd;
    ULONG            PendingSource;
    ULONG            PendingTarget;

    IntegerRegister = &ContextRecord->R0;
    NextPc = ContextRecord->PR - 2;
    *InFunction = FALSE;
#if SH4
    // We assume FR bit always 0 in prolog.
    FloatingRegister = ContextRecord->FRegs;
#endif

    // If we are at the end of the function, and the delay slot instruction
    // is one of the following, then just execute the instruction...
    //      MOV   Rm,sp
    //      ADD   Rm,sp
    //      ADD   #imm,sp
    //      MOV.L @sp+,Rn
    // For SH4:
    //      FSCHG
    //      FMOV.S @sp+,FRn
    // There is no need reverse execute the prolog, because the epilog has
    // already taken care of restoring all the other permanent registers.
	instrProlog = *(PUSHORT)ControlPc;
    if (instrProlog == RTS) {
        ControlPc += 2;
        Instruction = *(PSH3IW)ControlPc;

        // MOV Rm,sp
        if ((Instruction.instr3.opcode   == 0x6) &&
            	(Instruction.instr3.ext_disp == 0x3)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            if (Rn == sp)
                IntegerRegister[sp] = IntegerRegister[Rm];
        }

        // ADD Rm,sp
        else if ((Instruction.instr3.opcode   == 0x3) && 
                 (Instruction.instr3.ext_disp == 0xc)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            if (Rn == sp)
                IntegerRegister[sp] += IntegerRegister[Rm];
        }

        // ADD #imm,sp
        //
        // Note: the offset added to sp will always be positive...
        else if (Instruction.instr2.opcode == 0x7) {
            Rn     = Instruction.instr2.reg;
            Offset = Instruction.instr2.imm_disp;
            if (Rn == sp)
                IntegerRegister[sp] += Offset;
        }

        // MOV.L @sp+,Rn
        else if ((Instruction.instr3.opcode   == 0x6) &&
                 (Instruction.instr3.ext_disp == 0x6)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            if ((Rm == sp) && IS_REGISTER_SAVED(Rn)) {
                IntegerRegister[Rn] = *(PULONG)IntegerRegister[sp];
                IntegerRegister[sp] += 4;
            }
        }

#if SH4
        // FMOV.S @sp+, FRn
        else if ((Instruction.instr3.opcode   == 0xf) &&
                 (Instruction.instr3.ext_disp == 0x9) &&
                 (Instruction.instr3.regM     == sp)) {
            Rn = Instruction.instr3.regN;
            if (IS_FLOAT_REGISTER_SAVED(Rn)) {
                FloatingRegister[Rn] = *(PULONG)IntegerRegister[sp];
                IntegerRegister[sp] += 4;
            }
        }

        // FSCHG
        else if (Instruction.instr6.opcode == 0xf3fd) {
            ContextRecord->Fpscr ^= SZ_MASK;    // toggle the SZ bit
        }
#endif
        *EstablisherFrame = ContextRecord->R15;
        return NextPc;
    }

#if SH4
    // Always expect FR and PR bits to be 0 throughout the prolog
    ContextRecord->Fpscr &= ~(FR_MASK | PR_MASK);
#endif

    // If the address where control left the specified function is outside
    // the limits of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., it is within the prologue).
    if ((ControlPc < FunctionEntry->BeginAddress) ||
        	(ControlPc >= FunctionEntry->PrologEndAddress)) {
        *InFunction = TRUE;
        ControlPc = FunctionEntry->PrologEndAddress;
#if SH4
        // Always expect SZ bit to be 0 at end of prolog
        ContextRecord->Fpscr &= ~(SZ_MASK);
#endif
    }

    // Scan backward through the prologue and reload callee registers that
    // were stored.
    //
    // Cannot initialize holding registers to 0 because that is a valid
    // register value...
    PendingSource = (ULONG)-1;
    TemporaryRegister = (ULONG)-1;
    while (ControlPc > FunctionEntry->BeginAddress) {
        ControlPc -= 2;
        Instruction = *(PSH3IW)ControlPc;

        // MOV #imm,Rn
        if (Instruction.instr2.opcode == 0xe) {
            Rn = Instruction.instr2.reg;
            if (Rn == PendingSource) {
                PendingValue = (signed char)Instruction.instr2.imm_disp;
                if (PendingAdd) {
                    PendingValue = -PendingValue;
                }
                IntegerRegister[PendingTarget] += PendingValue;
            }
        }

        // MOV Rm,Rn
        else if ((Instruction.instr3.opcode   == 0x6) &&
                 (Instruction.instr3.ext_disp == 0x3)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            IntegerRegister[Rm] = IntegerRegister[Rn];
        }

        // MOV.L Rm,@-sp
        //
        // Used to push permanent registers to stack
        else if ((Instruction.instr3.opcode   == 0x2) &&
                 (Instruction.instr3.ext_disp == 0x6) &&
                 (Instruction.instr3.regN     == sp )) {
            Rm = Instruction.instr3.regM;
            IntegerRegister[Rm] = *(PULONG)IntegerRegister[sp];
            IntegerRegister[sp] += 4;
        }

#if SH4
        // FMOV FRm,FRn (or any combination of DRm,DRn,XDm,XDn)
        else if ((Instruction.instr3.opcode   == 0xf) &&
                 (Instruction.instr3.ext_disp == 0xc)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            if (ContextRecord->Fpscr & SZ_MASK) {
                Rm += (Rm & 0x1)*15; // DRm or XDm
                Rn += (Rn & 0x1)*15; // DRn or XDn
                // the other 32 bits of the double precision move
                FloatingRegister[Rm+1] = FloatingRegister[Rn+1];
            }
            FloatingRegister[Rm] = FloatingRegister[Rn];
        }

        // FMOV(.S) FRm,@-sp (must check for sp)
        //
        // Used to push permanent fp registers to stack
        else if ((Instruction.instr3.opcode   == 0xf) &&
                 (Instruction.instr3.ext_disp == 0xb) &&
                 (Instruction.instr3.regN     == sp)) {
            Rm = Instruction.instr3.regM;
            if (ContextRecord->Fpscr & SZ_MASK) {
                // low order bits in FR(m+1), and high order in FRm
                FloatingRegister[Rm+1] = *(PULONG)IntegerRegister[sp];
                IntegerRegister[sp] += 4;
            }
            FloatingRegister[Rm] = *(PULONG)IntegerRegister[sp];
            IntegerRegister[sp] += 4;
        }

        // FSCHG
        else if (Instruction.instr6.opcode == 0xf3fd) {
            ContextRecord->Fpscr ^= SZ_MASK;    // toggle the SZ bit
        }
#endif

        // MOV.L @(disp,pc),Rn
        //
        // Used to load stack decrement value into a register
        else if (Instruction.instr2.opcode == 0xD) {
            Rn = Instruction.instr2.reg;
            if (Rn == PendingSource) {
                // Address = 4-byte aligned current instruction address +
                //           4 (SH3 pc points 2 instructions downstream) +
                //           4-byte scaled displacement (from instruction)
                Address = (ControlPc & 0xfffffffc) + 4 +
                          (Instruction.instr2.imm_disp << 2);
    			PendingValue = *(PLONG)Address;
                if (PendingAdd) {
                    PendingValue = -PendingValue;
                }
                IntegerRegister[PendingTarget] += PendingValue;
            }
        }

        // MOV.W @(disp,pc),Rn
        //
        // Used to load stack decrement value into a register
        else if (Instruction.instr2.opcode == 0x9) {
            Rn = Instruction.instr2.reg;
            if (Rn == PendingSource) {
                // Address = current instruction address +
                //           4 (SH3 pc points 2 instructions downstream) +
                //           2-byte scaled displacement (from instruction)
                Address = ControlPc + 4 + (Instruction.instr2.imm_disp << 1);
                PendingValue = *(short *)Address;
                if (PendingAdd) {
                    PendingValue = -PendingValue;
                }
                IntegerRegister[PendingTarget] += PendingValue;
            }
        }

        // ADD #imm,Rn
        //
        // Used to allocate space on the stack (#imm < 0)
        else if (Instruction.instr2.opcode == 0x7) {
            Rn = Instruction.instr2.reg;
            Offset = (signed char)Instruction.instr2.imm_disp;
///            if (Offset & 0x80) {
///                // Offset is negative, so add 2's-complement
///                Offset = (~Offset + 1) & 0xff;
///                IntegerRegister[Rn] += Offset;
///            } else {
                // Offset is positive, so subtract it
            IntegerRegister[Rn] -= Offset;
///            }
        }

        // SUB Rm,Rn
        //
        // Used to allocate space on the stack
        else if ((Instruction.instr3.opcode   == 0x3) &&
                 (Instruction.instr3.ext_disp == 0x8)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            PendingSource = Rm;
            PendingTarget = Rn;
            PendingAdd = FALSE;
        }

        // ADD Rm,Rn
        //
        // Used to allocate space on the stack
        else if ((Instruction.instr3.opcode   == 0x3) &&
                 (Instruction.instr3.ext_disp == 0xc)) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            PendingSource = Rm;
            PendingTarget = Rn;
            PendingAdd = TRUE;
        }


        // MOV.L Rm,@(disp,Rn)
        //
        // Used to store register values to stack
        else if (Instruction.instr3.opcode == 0x1) {
            Rm = Instruction.instr3.regM;
            Rn = Instruction.instr3.regN;
            TemporaryRegister = Rm;
            Address = IntegerRegister[Rn] + (Instruction.instr3.ext_disp << 2);
            TemporaryValue = *(PULONG)Address;
            // Added for unwinding parameter values
            IntegerRegister[Rm] = TemporaryValue;
        }

        // STS mach,Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x0a)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                ContextRecord->MACH = TemporaryValue;
            }
        }

        // STS macl,Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x1a)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                ContextRecord->MACL = TemporaryValue;
            }
        }

        // STS pr,Rn
        //
        // (special prolog only)
        //
        // NOTE: since this instruction is only in the prolog it cannot
        //       be the NextPc...only STC spc,Rn can
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x2a)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                ContextRecord->PR = TemporaryValue;
            }
        }

        // STC gbr,Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x12)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                ContextRecord->GBR = TemporaryValue;
            }
        }

        // STC Psr,Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x32)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                ContextRecord->Psr = TemporaryValue;
            }
        }

        // STC Fir,Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x0) &&
                 (Instruction.instr1.function == 0x42)) {
            Rn = Instruction.instr1.reg;
            if (Rn == TemporaryRegister) {
                NextPc = TemporaryValue;
            }
        }

#if SH4
        // STS.L fpscr,@-Rn
        //
        // (special prolog only)
        else if ((Instruction.instr1.opcode   == 0x4) &&
                 (Instruction.instr1.function == 0x62)) {
            Rn = Instruction.instr1.reg;
            ContextRecord->Fpscr = *(PULONG)IntegerRegister[Rn];
            IntegerRegister[Rn] += 4;
        }
#endif

        // STS.L pr,@-Rn
        else if ((Instruction.instr1.opcode   == 0x4) &&
                 (Instruction.instr1.function == 0x22)) {
            Rn = Instruction.instr1.reg;
            ContextRecord->PR = *(PULONG)IntegerRegister[Rn];
            IntegerRegister[Rn] += 4;
            NextPc = ContextRecord->PR - 2;
        } else {
            DEBUGMSG(ZONE_SEH, (TEXT("RtlVirtualUnwind: unknown instruction in proglog @%8.8lx=%4.4x\r\n"),
                    ControlPc, Instruction));
        }
    }
    *EstablisherFrame = ContextRecord->R15;
    return NextPc;
}

