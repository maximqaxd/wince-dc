/*++
    Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    mipsuwnd.c

Abstract:
    This module implements the unwinding of procedure call frames for
    exception processing on MIPS processors.
--*/

#include "kernel.h"

// Define stack register and zero register numbers.
#define RA 0x1f                         // integer register 31
#define SP 0x1d                         // integer register 29
#define ZERO 0x0                        // integer register 0
#define NOREG -1                        // invalid register number

#if defined(MIPS16SUPPORT)

extern PRUNTIME_FUNCTION NKLookupFunctionEntry(PPROCESS, ULONG, PRUNTIME_FUNCTION);

// Maximum number of instructions encoded by block with given start and end addresses
#define BYTES_TO_OPCOUNT(_begin, _end)  (((_end) - (_begin)) >> (2 >> ((_end) & 1)))

// Op count of longest prologue helper function called from the current mode
#define MIPS_PROLHELPER_OPCOUNT(_pc) 13 // store 9 callee-saved regs, store RA, subtract saved-reg and arg area
                                        // size from SP, subtract local var area size from SP, and return
                                        // = 13 instructions.

// Address of instruction adjacent to _pc: +/- 4 for MIPS32, +/- 2 for MIPS16
#define MIPS_PREV_INSTR(_pc)  ((_pc) - (4 >> ((_pc) & 1)))
#define MIPS_NEXT_INSTR(_pc)  ((_pc) + (4 >> ((_pc) & 1)))

// MIPS16->MIPS32 register encoding conversion
#define FIX_MIPS16_REG(r)     (((r) < 2) ? (r)+16 : (r))

// MIPS16 MOV32R register encoding
#define FIX_MOV32R_REG(r)     (((r) >> 2) | (((r) & 0x3) << 3))
#define UNFIX_MOV32R_REG(r)   (((r) << 2) | (((r) & 0x18) >> 3))

// MIPS16 instruction tests
#define IS_MIPS16_MOV32RSP(i) (((i) & 0xfff8) == ((I8_OP16 << 11) | (MOV32R_OP16 << 8) | (UNFIX_MOV32R_REG (SP) << 3)))
#define IS_MIPS16_ADDIUSP(i)  (((i) & 0xff00) == ((I8_OP16 << 11) | (ADJSP_OP16 << 8)))
#define IS_MIPS16_JR(i)       (((i) & 0xf8ff) == ((RR_OP16 << 11) | JR_OP16))
#define IS_MIPS16_JRRA(i)     ((i) == ((RR_OP16 << 11) | (0x1 << 5) | JR_OP16))
#define IS_MIPS16_RETURN(i)   (IS_MIPS16_JR(i) || IS_MIPS16_JRRA(i))
                
// MIPS32 instruction tests
#define IS_MIPS32_ADDIUSP(i) (((i) & 0xffff0000) == ((ADDIU_OP << 26) | (SP << 16) | (SP << 21)))
#define IS_MIPS32_ADDUSP(i) (((i) & 0xffe0ffff) == ((SPEC_OP << 26) | ADDU_OP | (SP << 11) | (SP << 21)))
#define IS_MIPS32_RETURN(i) ((i) == JUMP_RA)

// Interpreter opcodes
enum {
    OP_NOP = 0,
    OP_ADD,
    OP_ADDI,
    OP_SUB,
    OP_OR,
    OP_MOVE,
    OP_IMM,
    OP_STORE,
    OP_FSTORE,
    OP_DSTORE,
};

// Interpreter operation structure
typedef struct Operation {
    BYTE Opcode;
    BYTE Rd;
    BYTE Rs;
    BYTE Rt;
    ULONG Imm;
} OPERATION, *POPERATION;

POPERATION
BuildSimpleOp16 (
    IN OUT POPERATION Op,
    IN ULONG Address,
    IN ULONG PCAddress,
    IN ULONG Extend)
/*++
Routine Description
    This function converts a MIPS16 instruction at the specified address into the
    equivalent interpreter instruction.

Arguments
    Op - A pointer to an interpreter operation structure to receive the translated
        opcode.

    Address - The address of the MIPS16 instruction.

    PCAddress - The PC value at execution time (PCAddress != Address when the
        instruction is being executed in a delay slot).

    Extend - The value of a previous EXTEND opcode immediate field.  Set to -1
        if the instruction is not extended.
--*/
{
    MIPS16_INSTRUCTION Instruction;
    ULONG UImm8, UImm5, SImm8;

    Instruction.Short = *(PUSHORT) Address;
    UImm8 = Instruction.ri_format.Immediate << 2;
    UImm5 = Instruction.rri_format.Immediate << 2;
    SImm8 = Instruction.rsi_format.Simmediate;
    if (Extend != (ULONG) -1) {
        SImm8 = UImm8 = UImm5 = Extend | Instruction.rri_format.Immediate;
    }
    Op->Opcode = OP_NOP;
    Op->Rt = Op->Rs = Op->Rd = NOREG;
    switch (Instruction.i_format.Opcode) {
    case LI_OP16:
        Op->Opcode = OP_IMM;
        Op->Rd = FIX_MIPS16_REG (Instruction.ri_format.Rx);
        Op->Imm = Instruction.ri_format.Immediate;
        if (Extend != (ULONG) -1) {
            Op->Imm = Extend | Instruction.rri_format.Immediate;
        }
        break;
    case SW_OP16:
        Op->Opcode = OP_STORE;
        Op->Rs = FIX_MIPS16_REG (Instruction.rri_format.Rx);
        Op->Rt = FIX_MIPS16_REG (Instruction.rri_format.Ry);
        Op->Imm = UImm5;
        break;
    case SWSP_OP16:
        Op->Opcode = OP_STORE;
        Op->Rs = SP;
        Op->Rt = FIX_MIPS16_REG (Instruction.ri_format.Rx);
        Op->Imm = UImm8;
        break;
    case LWPC_OP16:
        Op->Opcode = OP_IMM;
        Op->Rd = FIX_MIPS16_REG (Instruction.ri_format.Rx);
        Op->Imm = *(PULONG) ((PCAddress & ~3) + UImm8);
        break;
    case ADDIU8_OP16:
        Op->Opcode = OP_ADDI;
        Op->Rs = Op->Rd = FIX_MIPS16_REG (Instruction.ri_format.Rx);
        Op->Imm = SImm8;
        break;
    case RRR_OP16:
        Op->Rd = FIX_MIPS16_REG (Instruction.rrr_format.Rz);
        Op->Rs = FIX_MIPS16_REG (Instruction.rrr_format.Rx);
        Op->Rt = FIX_MIPS16_REG (Instruction.rrr_format.Ry);
        if (Op->Rs != Op->Rd) break;
        switch (Instruction.rrr_format.Function) {
        case SUBU_OP16:
            Op->Opcode = OP_SUB;
            break;
        case ADDU_OP16:
            Op->Opcode = OP_ADD;
            break;
        }
        break;
    case I8_OP16:
        switch (Instruction.i8_format.Function) {
        case SWRASP_OP16:
            Op->Opcode = OP_STORE;
            Op->Rs = SP;
            Op->Rt = RA;
            Op->Imm = UImm8;
            break;
        case ADJSP_OP16:
            Op->Opcode = OP_ADDI;
            Op->Rs = Op->Rd = SP;
            Op->Imm = SImm8;
            if (Extend == (ULONG) -1) {
                Op->Imm = SImm8 << 3;
            }
            break;
        case MOV32R_OP16:
            Op->Opcode = OP_MOVE;
            Op->Rd = FIX_MOV32R_REG (Instruction.mov32r_format.R32);
            Op->Rs = FIX_MIPS16_REG (Instruction.mov32r_format.Rz);
            break;
        case MOVR32_OP16:
            Op->Opcode = OP_MOVE;
            Op->Rd = FIX_MIPS16_REG (Instruction.movr32_format.Ry);
            Op->Rs = (BYTE) Instruction.movr32_format.R32;
            break;
        }
        break;
    default:
        break;
    }
    return Op + (Op->Opcode != OP_NOP);
}

POPERATION
BuildSimpleOp32 (
    IN OUT POPERATION Op,
    IN ULONG Address,
    IN BOOLEAN NotFirst)
/*++
Routine Description
    This function converts a MIPS32 instruction at the specified address into the
    equivalent interpreter instruction.  It will attempt to compress LUI-ORI pairs
    into a single OP_IMM operation.

Arguments
    Op - A pointer to an interpreter operation structure to receive the translated
        opcode.

    Address - The address of the MIPS32 instruction.

    NotFirst - True if this is not the first Op in the list, to tell LUI-ORI
        compression to look at the previous operation at Op-1. 
--*/
{
    MIPS_INSTRUCTION I32;

    I32.Long = *(PULONG) Address;
    Op->Rs = (BYTE) I32.i_format.Rs;
    Op->Rt = (BYTE) I32.i_format.Rt;
    Op->Rd = (BYTE) I32.r_format.Rd;
    Op->Imm = I32.i_format.Simmediate;
    switch (I32.i_format.Opcode) {
    case SW_OP:
        Op->Opcode = OP_STORE;
        goto store;
    case SWC1_OP:
        Op->Opcode = OP_FSTORE;
        goto store;
    case SDC1_OP:
        Op->Opcode = OP_DSTORE;
    store:
        if (Op->Rs == SP) {
            Op->Rd = NOREG;
            Op++;
        }
        break;
    case ADDIU_OP:
        Op->Rd = Op->Rt;
        if (Op->Rs == Op->Rt) {
            Op->Opcode = OP_ADDI;
            Op++;
        } else if (Op->Rs == ZERO) {
            Op->Opcode = OP_IMM;
            Op++;
        }
        break;
    case ORI_OP:
        Op->Opcode = OP_IMM;
        Op->Imm &= 0xffff;
        Op->Rd = Op->Rt;
        if (Op->Rs == Op->Rt) {
            // Look for matching LUI
            if (NotFirst && (Op-1)->Opcode == OP_IMM && (Op-1)->Rd == Op->Rd) {
                (Op-1)->Imm |= Op->Imm;
            } else {
                Op++;
            }
        } else if (Op->Rs == ZERO) {
            Op++;
        }
        break;
    case SPEC_OP:
        switch (I32.r_format.Function) {
        case OR_OP:
            Op->Opcode = OP_OR;
            goto spec;
        case ADDU_OP:
            Op->Opcode = OP_ADD;
        spec:
            if (Op->Rs == ZERO) {
                Op->Opcode = OP_MOVE;
                Op->Rs = Op->Rt;
                Op++;
            } else if (Op->Rt == ZERO) {
                Op->Opcode = OP_MOVE;
                Op++;
            } else if (Op->Rs == Op->Rd) {
                Op++;
            } else if (Op->Rt == Op->Rd) {
                Op->Rt = Op->Rs;
                Op++;
            }
            break;
        case SUBU_OP:
            if (Op->Rs == Op->Rd) {
                Op->Opcode = OP_SUB;
                Op++;
            }
            break;
        }
        break;
    case LUI_OP:
        Op->Opcode = OP_IMM;
        Op->Imm <<= 16;
        Op->Rd = Op->Rt;
        Op++;
        break;
    }
    return Op;
}
    
INT
BuildOps (
    IN OUT POPERATION Operations,
    IN INT Max,
    IN ULONG Address,
    IN ULONG EndAddress)
/*++
Routine Description
    This function constructs an interpreter operation list based upon the machine
    instructions found between a start and end address.  

    The code is smart enough to handle a single level of calls, this is required
    if prolog helpers are being used.

Arguments
    Operations - A pointer to an interpreter operation array to receive the translated
        instructions.

    Max - Size of the operation array.

    Address - The address of the first instruction.

    EndAddress - The address of the last instruction.

Return Value:
    The number of operations generated.
--*/
{
    MIPS16_INSTRUCTION I16;
    MIPS_INSTRUCTION I32;
    ULONG RetAddress = (ULONG) -1;
    ULONG Extend = (ULONG) -1;
    ULONG PCAddress = Address;
    POPERATION Op = Operations;
    POPERATION LastOp = Operations + Max;

    while (Address != EndAddress && Op < LastOp) {
        if (Address & 1) {
            // Get MIPS16 instruction
            I16.Short = *((PUSHORT) (Address - 1));
            switch (I16.i_format.Opcode) {
            case EXTEND_OP16:
                Extend = (I16.ei_format.Simmediate11 << 11) | (I16.ei_format.Immediate5 << 5);
                PCAddress = Address;
                Address += 2;
                break;
            case B_OP16:
                if (Extend == (ULONG) -1) {
                    Address += (I16.b_format.Simmediate << 1) + 2;
                } else {
                    Address += (I16.rri_format.Immediate | Extend) + 4;
                }
                PCAddress = Address;
                Extend = (ULONG) -1;
                break;
            case JALX_OP16:
                Op = BuildSimpleOp16 (Op, Address + 3, Address, (ULONG) -1);
                RetAddress = Address + 6;
                Address = (I16.j_format.Target16 << 18) | (I16.j_format.Target21 << 23) | (*(PUSHORT) (Address+1) << 2) | ((Address+3) & 0xf0000000);
                Address |= (I16.j_format.Ext == 0);
                PCAddress = Address;
                Extend = (ULONG) -1;
                break;
            case RR_OP16:
                switch (I16.rr_format.Function) {
                case JR_OP16:
                    if (I16.rr_format.Ry == 1 && I16.rr_format.Rx == 0) {
                        // If this is a "JR RA" and we've nested then decode from return address
                        if (RetAddress != (ULONG) -1) {
                            Op = BuildSimpleOp16 (Op, Address + 1, PCAddress, (ULONG) -1);
                            PCAddress = Address = RetAddress;
                            RetAddress = (ULONG) -1;
                            Extend = (ULONG) -1;
                            continue;
                        }
                    }
                    // Anything other than JR RA is not acceptable in helper functions
                    return -1;
                }
                Address += 2;
                PCAddress = Address;
                Extend = (ULONG) -1;
                break;
            default:
                Op = BuildSimpleOp16 (Op, Address - 1, PCAddress, Extend);
                Address += 2;
                PCAddress = Address;
                Extend = (ULONG) -1;
                break;
            }
        } else {
            // Get MIPS32 instruction
            I32.Long = *(PULONG) Address;
            switch (I32.i_format.Opcode) {
            case JAL_OP:
                Op = BuildSimpleOp32 (Op, Address + 4, (BOOLEAN)(Op != Operations));
                RetAddress = Address + 8;
                Address = (I32.j_format.Target << 2) | ((Address + 4) & 0xf0000000);
                if (I32.j_format.Target >> 26) Address |= 1;
                PCAddress = Address;
                break;
            case SPEC_OP:
                if (I32.Long == JUMP_RA && RetAddress != (ULONG) -1) {
                    Op = BuildSimpleOp32 (Op, Address + 4, (BOOLEAN)(Op != Operations));
                    Address = PCAddress = RetAddress;
                    RetAddress = (ULONG) -1;
                    continue;
                }
                // Note fallthrough
            default:
                Op = BuildSimpleOp32 (Op, Address, (BOOLEAN)(Op != Operations));
                Address += 4;
                break;
            }
        }
    }
    if (Address != EndAddress) {
        return -1;
    }
    return Op - Operations;
}
        
void
FindImmed (
    IN POPERATION Ops,
    IN DWORD NumOps,
    IN DWORD RegNum,
    IN OUT PULONG Regs)
/*++
Routine Description
    This function searches backwards through the Op array looking for OP_IMM
    assignments to a given register.

Arguments
    Op - A pointer to an interpreter operation array.

    NumOps - Number of operations in the array.

    RegNum - Register to search for.

    Regs - Register value array to update.
--*/
{
    while (NumOps-- > 0) {
        if (Ops->Opcode == OP_IMM && Ops->Rd == RegNum) {
            Regs [RegNum] = Ops->Imm;
            return;
        }
        Ops--;
    }
}

ULONG
Reverse (
    IN POPERATION Op,
    IN DWORD NumOps,
    IN OUT PCONTEXT Context)
/*++
Routine Description
Arguments
--*/
{
    PULONG IntRegs = &Context->IntZero;
    PULONG FpRegs = &Context->FltF0;
    ULONG NextPc;
    BOOL RestoredRa = FALSE;
    
    Op += NumOps - 1;
    while (NumOps-- > 0) {
        switch (Op->Opcode) {
        case OP_ADD:
            FindImmed (Op-1, NumOps, Op->Rt, IntRegs);
            IntRegs [Op->Rd] -= IntRegs [Op->Rt];
            break;
        case OP_SUB:
            FindImmed (Op-1, NumOps, Op->Rt, IntRegs);
            IntRegs [Op->Rd] += IntRegs [Op->Rt];
            break;
        case OP_ADDI:
            IntRegs [Op->Rd] -= Op->Imm;
            break;
        case OP_STORE:
            IntRegs [Op->Rt] = *(PULONG) (Op->Imm + IntRegs [Op->Rs]);
            if (Op->Rt == RA) {
                if (RestoredRa == FALSE) {
                    NextPc = MIPS_PREV_INSTR(IntRegs[RA]);
                    RestoredRa = TRUE;
                } else
                    NextPc = MIPS_NEXT_INSTR(NextPc);
            }
            break;
        case OP_FSTORE:
            FpRegs [Op->Rt] = *(PULONG) (Op->Imm + IntRegs [Op->Rs]);
            break;
        case OP_DSTORE:
            FpRegs [Op->Rt] = *(PULONG) (Op->Imm + IntRegs [Op->Rs]);
            FpRegs [Op->Rt+1] = *(PULONG) (Op->Imm + IntRegs [Op->Rs] + 4);
            break;
        case OP_IMM:
            IntRegs [Op->Rd] = Op->Imm;
            break;
        case OP_MOVE:
            IntRegs [Op->Rs] = IntRegs [Op->Rd];
            break;
        }
        Op--;
    }
    return RestoredRa ? NextPc : MIPS_PREV_INSTR(IntRegs[RA]);
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
    RUNTIME_FUNCTION rf;
    PULONG FloatingRegister;
    PULONG IntegerRegister;
    MIPS_INSTRUCTION I32;
    MIPS16_INSTRUCTION I16;
    ULONG Rs;
    ULONG NextPc;
    INT OpCount;
    BOOL WithinHelper;
    POPERATION Ops;

    DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: (ControlPc=%x)\r\n", ControlPc));
    DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: entry RA=%x SP=%x\r\n", ContextRecord->IntRa, ContextRecord->IntSp));

    // Set the base address of the integer and floating register arrays.
    FloatingRegister = &ContextRecord->FltF0;
    IntegerRegister = &ContextRecord->IntZero;
    WithinHelper = FALSE;
    *InFunction = FALSE;

    // If the function is a frameless leaf function, then RA holds
    // the control PC for the previous function
    if (FunctionEntry == NULL) {
        *EstablisherFrame = ContextRecord->IntSp;
        DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: returning due to NULL function entry RA=%x\r\n", ContextRecord->IntRa));
        return MIPS_PREV_INSTR(ContextRecord->IntRa);
    }

    // Determine if the PC is within a prolog helper.  If it is then
    // the RA register will hold the address that we left the prolog
    // within the calling function.  Get the function entry for this
    // function.
    if (FunctionEntry->ExceptionHandler == 0 && (ULONG)FunctionEntry->HandlerData == 2) {
        // Get function entry based upon ContextRecord->IntRA
        DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: in prolog helper RA=%x\r\n", ContextRecord->IntRa));
        WithinHelper = TRUE;
        FunctionEntry = NKLookupFunctionEntry (pCurProc, ContextRecord->IntRa, &rf);
        if (FunctionEntry == NULL) {
            DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: returning due to NULL function entry RA=%x\r\n", ContextRecord->IntRa));
            return 0;
        }
    } else {
        // If we are currently pointing at a return, then any saved registers
        // have been restored with the possible exception of the stack pointer.
        // The control PC is considered to be in the calling function.  If the
        // instruction in the delay slot is a stack pointer restore then it is
        // executed.
        if (ControlPc & 1) {
            DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: MIPS16 code\r\n"));
            I16.Short = *(PUSHORT) (ControlPc - 1);
            if (IS_MIPS16_RETURN (I16.Short)) {
                // Determine return address (RA or Rx)
                Rs = RA;
                if (IS_MIPS16_JR (I16.Short)) {
                    Rs = FIX_MIPS16_REG (I16.rr_format.Rx);
                }

                // Get MIPS16 instruction in delay slot
                I16.Short = *(PUSHORT)(ControlPc + 1);

                // Handle:  MOV32R  SP, Rx
                //          ADDIU   SP, Imm
                if (IS_MIPS16_MOV32RSP (I16.Short)) {
                    IntegerRegister[SP] = IntegerRegister[FIX_MIPS16_REG (I16.mov32r_format.Rz)];
                } else if (IS_MIPS16_ADDIUSP (I16.Short)) {
                    IntegerRegister[SP] += (I16.rsi_format.Simmediate << 3);
                }

                *EstablisherFrame = ContextRecord->IntSp;
                return MIPS_PREV_INSTR(IntegerRegister [Rs]);
            }
        } else {
            DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: MIPS32 code\r\n"));
            I32.Long = *(PULONG) ControlPc;
            if (IS_MIPS32_RETURN (I32.Long)) {
                DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: at return instruction\r\n"));
                // Get MIPS32 instruction in delay slot
                I32.Long = *((PULONG)ControlPc + 1);

                // Handle:  ADDIU  SP, SP, Imm
                //          ADDU   SP, SP, Rt
                if (IS_MIPS32_ADDIUSP (I32.Long)) {
                    IntegerRegister[SP] += I32.i_format.Simmediate;
                } else if (IS_MIPS32_ADDUSP (I32.Long)) {
                    IntegerRegister[SP] += IntegerRegister [I32.i_format.Rt];
                }
        
                *EstablisherFrame = ContextRecord->IntSp;
                return MIPS_PREV_INSTR(ContextRecord->IntRa);
            }
        }
    }

    // If the PC is within an epilog helper then the RA register will
    // hold the address that we left the calling function.  Use this
    // to get the function entry.
    if (FunctionEntry->ExceptionHandler == 0 && (ULONG)FunctionEntry->HandlerData == 3) {
        DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: within epilog helper\r\n"));
        // Get function entry based upon ContextRecord->IntRA - 4;
        FunctionEntry = NKLookupFunctionEntry (pCurProc, ContextRecord->IntRa - 4, &rf);
        if (FunctionEntry == NULL) {
            return 0;
        }

        // Reverse execute to the end of the prologue
        ControlPc = FunctionEntry->PrologEndAddress;
    }

    // If the address where control left the specified function is outside
    // the limits of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., it is within the prologue).
    if ((ControlPc < FunctionEntry->BeginAddress ||
         ControlPc >= FunctionEntry->PrologEndAddress) &&
        !WithinHelper) {
        *InFunction = TRUE;
        ControlPc = FunctionEntry->PrologEndAddress;
        DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: within function, new ControPc=%x\r\n", ControlPc));
    }

    // Allocate an array of OPERATION's that will hold the entire prologue.
    // Note that the prologue may include the body of a helper function. If this is the case,
    // the call and return won't be stored in Ops, so we add the size of the largest
    // prologue helper, minus the call to and return from the helper.
    OpCount = BYTES_TO_OPCOUNT(FunctionEntry->BeginAddress, ControlPc) + MIPS_PROLHELPER_OPCOUNT(ControlPc) - 2;
    Ops = (POPERATION) _alloca(OpCount * sizeof(OPERATION));

    // Build an operation list of all instructions from the function start
    // to the control PC.
    OpCount = BuildOps (Ops, OpCount, FunctionEntry->BeginAddress, ControlPc);
    DEBUGMSG(ZONE_SEH,(L"RtlVirtualUnwind: build op list with %d elements\r\n", OpCount));

    // We were unable to build the op list
    if (OpCount < 0) {
        return 0;
    }

    NextPc = Reverse (Ops, OpCount, ContextRecord);
    DEBUGMSG(ZONE_SEH,(L"Reverse executed RA=%x SP=%x\r\n", ContextRecord->IntRa, ContextRecord->IntSp));
    
    *EstablisherFrame = ContextRecord->IntSp;
    // Make sure that integer register zero is really zero.
    ContextRecord->IntZero = 0;
    return NextPc;
}

#else

#define INST_SIZE   4

// Define saved register masks.
#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers


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
    ULONG Address;
    ULONG DecrementOffset;
    ULONG DecrementRegister;
    PULONG FloatingRegister;
    ULONG Function;
    MIPS_INSTRUCTION Instruction;
    PULONG IntegerRegister;
    ULONG NextPc;
    LONG Offset;
    ULONG Opcode;
    ULONG Rd;
    BOOLEAN RestoredRa;
    BOOLEAN RestoredSp;
    ULONG Rs;
    ULONG Rt;

    // Set the base address of the integer and floating register arrays.
    FloatingRegister = &ContextRecord->FltF0;
    IntegerRegister = &ContextRecord->IntZero;
    *InFunction = FALSE;

    // If the instruction at the point where control left the specified
    // function is a return, then any saved registers have been restored
    // with the possible exception of the stack pointer and the control
    // PC is not considered to be in the function (i.e., an epilogue).
    if (*((PULONG)ControlPc) == JUMP_RA) {
        Instruction.Long = *((PULONG)ControlPc + 1);
        Opcode = Instruction.i_format.Opcode;
        Offset = Instruction.i_format.Simmediate;
        Rd = Instruction.r_format.Rd;
        Rs = Instruction.i_format.Rs;
        Rt = Instruction.i_format.Rt;
        Function = Instruction.r_format.Function;

        // If the opcode is an add immediate unsigned op and both the source
        // and destination registers are SP, then add the signed offset value
        // to SP. Otherwise, if the opcode is a special op, the operation is
        // an add unsigned, and the source and destination registers are both
        // SP, then add the register specified by Rd to SP.
        if ((Opcode == ADDIU_OP) && (Rt == SP) && (Rs == SP)) {
            IntegerRegister[SP] += Offset;

        } else if ((Opcode == SPEC_OP) && (Function == ADDU_OP) &&
                   (Rd == SP) && (Rs == SP)) {
            IntegerRegister[SP] += IntegerRegister[Rt];
        }

        *EstablisherFrame = ContextRecord->IntSp;
        return ContextRecord->IntRa - INST_SIZE;
    }

    // If the address where control left the specified function is outside
    // the limits of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., it is within the prologue).
    if ((ControlPc < FunctionEntry->BeginAddress) ||
        (ControlPc >= FunctionEntry->PrologEndAddress)) {
        *InFunction = TRUE;
        ControlPc = FunctionEntry->PrologEndAddress;
    }

    // Scan backward through the prologue and reload callee registers that
    // were stored.
    DecrementRegister = 0;
    *EstablisherFrame = ContextRecord->IntSp;
    NextPc = ContextRecord->IntRa - INST_SIZE;
    RestoredRa = FALSE;
    RestoredSp = FALSE;
    while (ControlPc > FunctionEntry->BeginAddress) {
        // Get instruction value, decode fields, case of opcode value, and
        // reverse store operations.
        ControlPc -= INST_SIZE;
        Instruction.Long = *((PULONG)ControlPc);
        Opcode = Instruction.i_format.Opcode;
        Offset = Instruction.i_format.Simmediate;
        Rd = Instruction.r_format.Rd;
        Rs = Instruction.i_format.Rs;
        Rt = Instruction.i_format.Rt;
        Address = Offset + IntegerRegister[Rs];
        if (Opcode == SW_OP) {
            // Store word.
            //
            // If the base register is SP and the source register is an
            // integer register, then reload the register value.
            if (Rs == SP) {
                IntegerRegister[Rt] = *((PULONG)Address);

                // If the destination register is RA and this is the first
                // time that RA is being restored, then set the address of
                // where control left the previous frame. Otherwise, this
                // is an interrupt or exception and the return PC should be
                // biased by INST_SIZE. Otherwise, if the destination register is
                // SP and this is the first time that SP is being restored,
                // then set the establisher frame pointer.
                if (Rt == RA) {
                    if (RestoredRa == FALSE) {
                        NextPc = ContextRecord->IntRa - INST_SIZE;
                        RestoredRa = TRUE;
                    } else
                        NextPc += INST_SIZE;
                } else if (Rt == SP) {
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = ContextRecord->IntSp;
                        RestoredSp = TRUE;
                    }
                }
            }

        } else if (Opcode == SWC1_OP) {
            // Store word coprocessor 1.
            //
            // If the base register is SP and the source register is a
            // floating register, then reload the register value.
            if (Rs == SP) {
                FloatingRegister[Rt] = *((PULONG)Address);
            }

        } else if (Opcode == SDC1_OP) {
            // Store double coprocessor 1.
            //
            // If the base register is SP and the source register is a
            // floating register, then reload the register and the next
            // register values.
            if (Rs == SP) {
                FloatingRegister[Rt] = *((PULONG)Address);
                FloatingRegister[Rt + 1] = *((PULONG)(Address + 4));
            }

        } else if (Opcode == ADDIU_OP) {
            // Add immediate unsigned.
            //
            // If both the source and destination registers are SP, then
            // a standard stack allocation was performed and the signed
            // displacement value should be subtracted from SP. Otherwise,
            // if the destination register is the decrement register and
            // the source register is zero, then add the decrement value
            // to SP.
            if ((Rs == SP) && (Rt == SP)) {
                IntegerRegister[SP] -= Offset;
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += Offset;
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }

        } else if (Opcode == ORI_OP) {
            // Or immediate.
            //
            // If both the destination and source registers are the decrement
            // register, then save the decrement value. Otherwise, if the
            // destination register is the decrement register and the source
            // register is zero, then add the decrement value to SP.
            if ((Rs == DecrementRegister) && (Rt == DecrementRegister)) {
                DecrementOffset = (Offset & 0xffff);
            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += (Offset & 0xffff);
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }

        } else if (Opcode == SPEC_OP) {
            // Special operation.
            //
            // The real opcode is in the function field of special opcode
            // instructions.
            Function = Instruction.r_format.Function;
            if ((Function == ADDU_OP) || (Function == OR_OP)) {
                // Add unsigned or an or operation.
                //
                // If one of the source registers is ZERO, then the
                // operation is a move operation and the destination
                // register should be moved to the appropriate source
                // register.
                if (Rt == ZERO) {
                    IntegerRegister[Rs] = IntegerRegister[Rd];

                    // If the destination register is RA and this is the
                    // first time that RA is being restored, then set the
                    // address of where control left the previous frame.
                    // Otherwise, this an interrupt or exception and the
                    // return PC should be biased by INST_SIZE.
                    if (Rs == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = ContextRecord->IntRa - INST_SIZE;
                            RestoredRa = TRUE;
                        } else
                            NextPc += INST_SIZE;
                    }
                } else if (Rs == ZERO) {
                    IntegerRegister[Rt] = IntegerRegister[Rd];

                    // If the destination register is RA and this is the
                    // first time that RA is being restored, then set the
                    // address of where control left the previous frame.
                    // Otherwise, this an interrupt or exception and the
                    // return PC should be biased by INST_SIZE.
                    if (Rt == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = ContextRecord->IntRa - INST_SIZE;
                            RestoredRa = TRUE;
                        } else
                            NextPc += INST_SIZE;
                    }
                }

            } else if (Function == SUBU_OP) {
                // Subtract unsigned.
                //
                // If the destination register is SP and the source register
                // is SP, then a stack allocation greater than 32kb has been
                // performed and source register number of the decrement must
                // be saved for later use.
                if ((Rd == SP) && (Rs == SP))
                    DecrementRegister = Rt;
            }

        } else if (Opcode == LUI_OP) {
            // Load upper immediate.
            //
            // If the destination register is the decrement register, then
            // compute the decrement value, add it from SP, and clear the
            // decrement register number.
            if (Rt == DecrementRegister) {
                DecrementRegister = 0;
                IntegerRegister[SP] += (DecrementOffset + (Offset << 16));
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }
        }
    }

    // Make sure that integer register zero is really zero.
    ContextRecord->IntZero = 0;
    return NextPc;
}

#endif
