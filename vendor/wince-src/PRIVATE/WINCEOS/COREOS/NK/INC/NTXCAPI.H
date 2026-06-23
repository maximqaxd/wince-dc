// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef _NTXCAPI_
#define _NTXCAPI_

//
// Define IEEE exception information.
//
// Define 32-, 64-, 80-, and 128-bit IEEE floating operand structures.
//

typedef struct _FP_32 {
    ULONG W[1];
} FP_32, *PFP_32;

typedef struct _FP_64 {
    ULONG W[2];
} FP_64, *PFP_64;

typedef struct _FP_80 {
    ULONG W[3];
} FP_80, *PFP_80;

typedef struct _FP_128 {
    ULONG W[4];
} FP_128, *PFP_128;

//
// Define IEEE compare result values.
//

typedef enum _FP_IEEE_COMPARE_RESULT {
    FpCompareEqual,
    FpCompareGreater,
    FpCompareLess,
    FpCompareUnordered
} FP_IEEE_COMPARE_RESULT;

//
// Define IEEE format and result precision values.
//

typedef enum _FP__IEEE_FORMAT {
    FpFormatFp32,
    FpFormatFp64,
    FpFormatFp80,
    FpFormatFp128,
    FpFormatI16,
    FpFormatI32,
    FpFormatI64,
    FpFormatU16,
    FpFormatU32,
    FpFormatU64,
    FpFormatCompare,
    FpFormatString
} FP_IEEE_FORMAT;

//
// Define IEEE operation code values.
//

typedef enum _FP_IEEE_OPERATION_CODE {
    FpCodeUnspecified,
    FpCodeAdd,
    FpCodeSubtract,
    FpCodeMultiply,
    FpCodeDivide,
    FpCodeSquareRoot,
    FpCodeRemainder,
    FpCodeCompare,
    FpCodeConvert,
    FpCodeRound,
    FpCodeTruncate,
    FpCodeFloor,
    FpCodeCeil,
    FpCodeAcos,
    FpCodeAsin,
    FpCodeAtan,
    FpCodeAtan2,
    FpCodeCabs,
    FpCodeCos,
    FpCodeCosh,
    FpCodeExp,
    FpCodeFabs,
    FpCodeFmod,
    FpCodeFrexp,
    FpCodeHypot,
    FpCodeLdexp,
    FpCodeLog,
    FpCodeLog10,
    FpCodeModf,
    FpCodePow,
    FpCodeSin,
    FpCodeSinh,
    FpCodeTan,
    FpCodeTanh,
    FpCodeY0,
    FpCodeY1,
    FpCodeYn
} FP_OPERATION_CODE;

//
// Define IEEE rounding modes.
//

typedef enum _FP__IEEE_ROUNDING_MODE {
    FpRoundNearest,
    FpRoundMinusInfinity,
    FpRoundPlusInfinity,
    FpRoundChopped
} FP_IEEE_ROUNDING_MODE;

//
// Define IEEE floating exception operand structure.
//

typedef struct _FP_IEEE_VALUE {
    union {
        SHORT I16Value;
        USHORT U16Value;
        LONG I32Value;
        ULONG U32Value;
        PVOID StringValue;
        ULONG CompareValue;
        FP_32 Fp32Value;
        LARGE_INTEGER I64Value;
        ULARGE_INTEGER U64Value;
        FP_64 Fp64Value;
        FP_80 Fp80Value;
        FP_128 Fp128Value;
    } Value;

    struct {
        ULONG RoundingMode : 2;
        ULONG Inexact : 1;
        ULONG Underflow : 1;
        ULONG Overflow : 1;
        ULONG ZeroDivide : 1;
        ULONG InvalidOperation : 1;
        ULONG OperandValid : 1;
        ULONG Format : 4;
        ULONG Precision : 4;
        ULONG Operation : 12;
        ULONG Spare : 3;
        ULONG HardwareException : 1;
    } Control;

} FP_IEEE_VALUE, *PFP_IEEE_VALUE;

//
// Define IEEE exception infomation structure.
//

#include "pshpack4.h"
typedef struct _FP_IEEE_RECORD {
    FP_IEEE_VALUE Operand1;
    FP_IEEE_VALUE Operand2;
    FP_IEEE_VALUE Result;
} FP_IEEE_RECORD, *PFP_IEEE_RECORD;
#include "poppack.h"

#endif //_NTXCAPI_

