/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    m16inst.h

Abstract:

    MIPS-16 instruction and floating constant definitions.

--*/

#ifndef _MP16INST_
#define _MP16INST_

//
// Define MIPS-16 instruction format structures.
//

typedef union _MIPS16_INSTRUCTION {
    USHORT Short;
    UCHAR Byte[2];

    struct
    {
        USHORT Immediate : 11;
        USHORT Opcode : 5;
    } i_format;

    struct
    {
        SHORT Simmediate : 11;
        USHORT Opcode : 5;
    } b_format;

    struct
    {
        USHORT Immediate : 8;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } ri_format;
      
    struct
    {
        SHORT Simmediate : 8;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } rsi_format;
      
    struct
    {
        USHORT Function : 5;
        USHORT Ry : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } rr_format;

    struct
    {
        USHORT Immediate : 5;
        USHORT Ry : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } rri_format;

    struct
    {
        USHORT Function : 2;
        USHORT Rz : 3;
        USHORT Ry : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } rrr_format;

    struct
    {
        USHORT Immediate : 4;
        USHORT Function : 1;
        USHORT Ry : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } rria_format;

    struct
    {
        USHORT Function : 2;
        USHORT Shift : 3;
        USHORT Ry : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } shift_format;

    struct
    {
        USHORT Immediate : 8;
        USHORT Function : 3;
        USHORT Opcode : 5;
    } i8_format;

    struct
    {
        USHORT R32 : 5;
        USHORT Ry : 3;
        USHORT Function : 3;
        USHORT Opcode : 5;
    } movr32_format;
      
    struct
    {
        USHORT Rz : 3;
        USHORT R32 : 5;            // mangled
        USHORT Function : 3;
        USHORT Opcode : 5;
    } mov32r_format;

    struct
    {
        USHORT Immediate : 8;
        USHORT Function : 3;
        USHORT Opcode : 5;
    } i64_format;

    struct
    {
        USHORT Immediate : 5;
        USHORT Ry : 3;
        USHORT Function : 3;
        USHORT Opcode : 5;
    } ri64_format;

    struct
    {
        USHORT Target21 : 5;
        USHORT Target16 : 5;
        USHORT Ext : 1;
        USHORT Opcode : 5;
    } j_format;

    struct
    {
        USHORT Function : 5;
        USHORT Ry : 3;
        USHORT Shift : 3;
        USHORT Opcode : 5;
    } shift64_format;

    struct
    {
        USHORT Zero1 : 5;
        USHORT Function : 3;
        USHORT Rx : 3;
        USHORT Opcode : 5;
    } jr_format;

    struct
    {
        USHORT Break : 5;
        USHORT Code : 6;
        USHORT Opcode : 5;
    } break_format;

// EXTEND formats

    struct
    {
        SHORT Simmediate11 : 5;
        USHORT Immediate5 : 6;
        USHORT Opcode : 5;
    } ei_format;

    struct
    {
        USHORT Immediate11 : 4;
        USHORT Immediate4 : 7;
        USHORT Opcode : 5;
    } erria_format;

    struct
    {
        USHORT Zero : 5;
        USHORT Shift5 : 1;
        USHORT Shift0 : 5;
        USHORT Opcode : 5;
    } eshift_format;

} MIPS16_INSTRUCTION, *PMIPS16_INSTRUCTION;

//
// Define MIPS-16 instruction major opcode values.
//

#define I8_OP16             0x0c         // I8-type - use minor opcode in bits [10:8]
#define RRR_OP16            0x1c         // RRR-type - use minor opcode in bits [1:0]
#define RR_OP16             0x1d         // RR-type - use minor opcode in bits [4:0]

#define B_OP16              0x02         // unconditional branch
#define JALX_OP16           0x03         // JAL & JALX
#define ADDIU8_OP16         0x09         // add 8-bit immediate
#define LI_OP16             0x0d         // load 8-bit unsigned immediate 
#define LWPC_OP16           0x16         // load word base pc
#define SWSP_OP16           0x1a         // store word base sp
#define SW_OP16             0x1b         // store word base integer register
#define EXTEND_OP16         0x1e         // extend immediate of next instruction

//
// Define I8-type minor opcode values (bits [10:8]).
//

#define SWRASP_OP16         0x02         // store word in ra, base sp
#define ADJSP_OP16          0x03         // add immediate unsigned integer to sp
#define MOV32R_OP16         0x05         // move general register to any of 32 registers
#define MOVR32_OP16         0x07         // move any of 32 registers to general register

//
// Define RRR-type minor opcode values (bits [1:0]).
//

#define ADDU_OP16           0x01         // add unsigned integer
#define SUBU_OP16           0x03         // subtract unsigned integer

//
// Define RR-type minor opcode values (bits [4:0]).
//

#define JR_OP16             0x00         // JR and JALR
#define BREAK_OP16          0x05         // BREAK

#endif // MP16INST





