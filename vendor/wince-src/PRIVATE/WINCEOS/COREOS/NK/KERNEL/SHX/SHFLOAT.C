/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#ifdef SH4
#include "kernel.h"
#include "shxinst.h"
// fp instruction opcodes
#define FADD_OP   0x0
#define FSUB_OP   0x1
#define FMUL_OP   0x2
#define FDIV_OP   0x3
#define FCMP_EQ   0x4
#define FCMP_GT   0x5
#define FMAC_OP   0xe

// all these instrs have a last 
// common opcode is 0xd
#define FLOAT_OP  0x2
#define FTRC_OP   0x3
#define FSQRT_OP  0x6
#define FCNVSD_OP 0xa
#define FCNVDS_OP 0xb
#define FIPR_OP   0xe
#define FTRV_OP   0xf

// fpscr bit masks
#define FR              0x00200000       // FR
#define SZ              0x00100000       // SZ
#define PR              0x00080000       // PR
#define DN              0x00040000       // DN

#define IMCW_ENABLE     0x00000f80       // Enable mask
#define E_INVALID       0x00000800       // Invalid operation
#define E_ZERODIVIDE    0x00000400       // Divide by zero
#define E_OVERFLOW      0x00000200       // Overflow
#define E_UNDERFLOW     0x00000100       // Underflow
#define E_INEXACT       0x00000080       // Inexact result

#define IMCW_CAUSE	0x0001f000	 // Cause mask
#define C_INVALID       0x00010000	 // Invalid operation
#define C_ZERODIVIDE    0x00008000	 // Divide by 0
#define C_OVERFLOW      0x00004000	 // Overflow
#define C_UNDERFLOW     0x00002000	 // Underflow
#define C_INEXACT       0x00001000	 // Inexact result

#define IMCW_FLAG	0x0000007c	 // Flag mask

extern float _adds(float,float);
extern float _subs(float,float);
extern float _muls(float,float);
extern float _divs(float,float);
extern int _eqs(float,float);
extern int _gts(float,float);

extern double _addd(double, double);
extern double _subd(double, double);
extern double _muld(double, double);
extern double _divd(double, double);
extern int _eqd(double,double);
extern int _gtd(double,double);

extern float _itos(int);
extern double _itod(int);
extern int _stoi(float);
extern int _dtoi(double);
extern double sqrt(double);
extern float  fsqrt(float);
extern double _stod(float);
extern float  _dtos(double);

extern ULONG __asm1(const char *,...);

typedef struct _FP_DOUBLE_OPERAND {
    union {
        struct {
            ULONG low;
            LONG  high;
        };
        double d;
    };
} FP_DOUBLE_OPERAND, *PFP_DOUBLE_OPERAND;

typedef union _FP_SINGLE_OPERAND {
    ULONG i;
    float f;
} FP_SINGLE_OPERAND, *PFP_SINGLE_OPERAND;


void set_fpscr(ULONG mask)
{
    __asm("sts  fpscr, r0\n"
          "or   r4, r0\n"
          "lds  r0, fpscr\n",
          mask);
}

void clr_fpscr(ULONG mask)
{
    __asm("sts fpscr, r0\n"
          "not r4, r4\n"
          "and r4, r0\n"
          "lds r0, fpscr\n",
          mask);
}

ULONG get_cause()
{
    return __asm1("sts fpscr, r0\n"
                  "and r4,r0\n"
                  "shlr8 r0\n"
                  "shlr2 r0\n"
                  "shlr2 r0\n",
                  0x3f00);
}

ULONG get_fpscr(void)
{
    return __asm1("sts fpscr,r0\n");
}

void fipr(ULONG *pFVm, ULONG *pFVn)
{
    float result=0.0f;
    FP_SINGLE_OPERAND fp1,fp2;

    fp1.i = (*pFVm)++;
    fp2.i = (*pFVn)++;
    result = _muls(fp1.f,fp2.f);
    
    fp1.i = (*pFVm)++;
    fp2.i = (*pFVn)++;
    fp2.f = _muls(fp1.f,fp2.f);
    result = _adds(fp2.f,result);

    fp1.i = (*pFVm)++;
    fp2.i = (*pFVn)++;
    fp2.f = _muls(fp1.f,fp2.f);
    result = _adds(fp2.f,result);

    fp1.i = (*pFVm);
    fp2.i = (*pFVn);
    fp2.f = _muls(fp1.f,fp2.f);
    fp2.f = _adds(fp2.f,result);
    *pFVn = fp2.i;
}

void ftrv(ULONG *pXMTRX, ULONG *pFVn)
{
    int i;
    float result=0.0f;
    FP_SINGLE_OPERAND fp1,fp2;
    ULONG *pResult = pFVn;
    ULONG *pFV = pFVn;

    for (i=0; i < 4; i++)
    {
        result = 0.0f;
        fp1.i = (*pXMTRX)++;
        fp2.i = (*pFV)++;
        fp2.f =  _muls(fp1.f,fp2.f);
        result = _adds(fp2.f,result);

        fp1.i = (*pXMTRX)++;
        fp2.i = (*pFV)++;
        fp2.f = _muls(fp1.f,fp2.f);
        result = _adds(fp2.f,result);

        fp1.i = (*pXMTRX)++;
        fp2.i = (*pFV)++;
        fp2.f = _muls(fp1.f,fp2.f);
        result = _adds(fp2.f,result);

        fp1.i = (*pXMTRX)++;
        fp2.i = (*pFV);
        fp2.f = _muls(fp1.f,fp2.f);
        fp2.f = _adds(fp2.f,result);
        //
        // store result to FV[n,n+1,n+2,n+3]
        //
        *pResult++ = fp2.i;
        //
        // reset the pFV to beginning of pFVn
        //
        pFV = pFVn;
    }
}

BOOL HandleHWFloatException(EXCEPTION_RECORD *ExceptionRecord,
                            PCONTEXT pctx) 
{
    PVOID ExceptionAddress;
    SH3IW Instruction;
    ULONG OpcodeExt, RegM, RegN, Opcode;
    ULONG prbit, frbit, CauseField;

    FP_DOUBLE_OPERAND dp1,dp2;
    FP_SINGLE_OPERAND fp1,fp2;
    ULONG *fpRegN1, *fpRegM1, *fpRegN2, *fpRegM2;
    ULONG *pFVn, *pFVm, *pXMTRX;

    char f[10];
    void *function = f;

    int retVal = 1;

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    __try {
        __try {

            Instruction = *(PSH3IW)ExceptionRecord->ExceptionAddress;
            OpcodeExt = Instruction.instr3.ext_disp;
            RegM = Instruction.instr3.regM;
            RegN = Instruction.instr3.regN;
            Opcode = Instruction.instr3.opcode;

            // check for double operation
            prbit =  ((pctx->Fpscr & PR) != 0);

            // check for float-point bank operation
            frbit =  ((pctx->Fpscr & FR) != 0);

            // reset fpscr to thread state
            clr_fpscr(0xffffffff);
            set_fpscr(pctx->Fpscr);

            // clear PR bit
            clr_fpscr(PR);

            if(!frbit)
            {
                fpRegN1 = pctx->FRegs + RegN;
                fpRegM1 = pctx->FRegs + RegM;

                if (prbit)
                {
                    fpRegN2 = pctx->FRegs + RegN+1;
                    fpRegM2 = pctx->FRegs + RegM+1;
                }                
            }else {
                fpRegN1 = pctx->xFRegs + RegN;
                fpRegM1 = pctx->xFRegs + RegM;

                if (prbit)
                {
                    fpRegN2 = pctx->xFRegs + RegN + 1;
                    fpRegM2 = pctx->xFRegs + RegM + 1;
                }
            }
            
            switch(OpcodeExt)
            {
            case FADD_OP:
            case FSUB_OP:
            case FMUL_OP:
            case FDIV_OP:

                if(prbit)
                {
                    dp1.high = *fpRegN1;
                    dp1.low  = *fpRegN2;
                    dp2.high = *fpRegM1;
                    dp2.low  = *fpRegM2;

                    switch(OpcodeExt)
                    {
                    case FADD_OP:
                        function = (void*)&_addd;
                        break;
                    case FSUB_OP:    
                        function = (void*)&_subd;
                        break;
                    case FMUL_OP:    
                        function = (void*)&_muld;
                        break;
                    case FDIV_OP:
                        function = (void*)&_divd;
                        break;
                    case FCMP_EQ:
                        function = (void*)&_eqd;
                        break;
                    case FCMP_GT:
                        function = (void*)&_gtd;
                        break;
                    }

                    if ((OpcodeExt == FCMP_EQ) || (OpcodeExt == FCMP_GT))
                    {
                        dp2.low = ((int(*)(double,double))(function))(dp1.d,dp2.d);

                        // set user t-bit to comparison result
                        pctx->Psr &= (0xfffffffe & (dp2.low != 0));
                        
                    }else {
                    
                        dp2.d = ((double(*)(double,double))(function))(dp1.d,dp2.d);

                        // store emulation result back to context
                        *fpRegN1 = dp2.high;
                        *fpRegN2 = dp2.low;
                    }
                }else {

                    fp1.i = *fpRegN1;
                    fp2.i = *fpRegM1;

                    switch(OpcodeExt)
                    {
                    case FADD_OP:
                        function = (void*)&_adds;
                        break;
                    case FSUB_OP:    
                        function = (void*)&_subs;
                        break;
                    case FMUL_OP:    
                        function = (void*)&_muls;
                        break;
                    case FDIV_OP:
                        function = (void*)&_divs;
                        break;
                    case FCMP_EQ:
                        function = (void*)&_eqs;
                        break;
                    case FCMP_GT:
                        function = (void*)&_gts;
                        break;

                    }
                    if ((OpcodeExt == FCMP_EQ) || (OpcodeExt == FCMP_GT))
                    {
                        fp1.i = ((int(*)(float,float))(function))(fp1.f, fp2.f);

                        //
                        // If SR-tbit is not matched retval from CRT compare,
                        // then update SR-tbit to match retval from
                        // CRT compare.
                        //
                        pctx->Psr &= (0xfffffffe & (fp1.i != 0));

                    }else {
                        fp1.f = ((float(*)(float,float))(function))(fp1.f,fp2.f);
                        *fpRegN1 = fp1.i;
                    }
                }
                break;

            case 0xd:
                //
                // All these functions take one operand only.
                //
                switch(RegM)
                {
                case FLOAT_OP:  
                    if (prbit)  
                    {
                        // float fpul,drn
                        fp1.i = pctx->Fpul;
                        dp1.d = _itod(fp1.i);
                        *fpRegN1 = dp1.high;
                        *fpRegN2 = dp1.low;

                    }else {
                        // float fpul, frn
                        fp1.i = pctx->Fpul;
                        fp1.f = _itos(fp1.i);
                        *fpRegN1 = fp1.i;
                    }
                    break;

                case FTRC_OP:  

                    if (prbit)
                    {
                        // ftrc drn,fpul
                        dp1.high = *fpRegM1;
                        dp1.low = *fpRegM2;
                        dp1.high = _dtoi(dp1.d);
                        *fpRegN1 = dp1.high;
                        
                    }else{

                        // ftrc frn,fpul
                        fp1.i = *fpRegM1;
                        fp1.i = _stoi(fp1.f);
                        *fpRegN1 = fp1.i;
                    }
                    break;

                case FSQRT_OP:  

                    // fsqrt drn
                    if (prbit)
                    {
                        dp1.high = *fpRegN1;
                        dp1.low = *fpRegN2;
                        dp1.d = sqrt(dp1.d);

                        *fpRegN1 = dp1.high;
                        *fpRegN2 = dp1.low;

                    }else {
                        fp1.i = *fpRegN1;
                        fp1.f = (float)sqrt((double)fp1.f);
                        *fpRegN1 = fp1.i;
                    }
                    break;

                case FCNVSD_OP:    
                    
                    //fcnvsd fpul, drn
                    if (prbit)
                    {
                        fp1.i = pctx->Fpul;
                        dp1.d = _stod(fp1.f);
                        // copy result back to user context
                        *fpRegN1 = dp1.high;
                        *fpRegN2 = dp1.low;

                    }else{
                        // else undefined operation
                        // propagate exception to user
                        retVal = 0;
                    }
                    break;

                case FCNVDS_OP:    

                    // fcnvds drn,fpul
                    if (prbit)
                    {
                        dp1.high = *fpRegN1;
                        dp1.low = *fpRegN2;
                        fp1.f = _dtos(dp1.d);
                        // copy result back to user context
                        pctx->Fpul = fp1.i;
                    }else{
                        // else undefined operation
                        // propagate exception to user
                        retVal = 0;
                    }
                    break;

                case FIPR_OP:
                    //
                    // Recalculated RegN and RegM for FIPR instr
                    // fipr has opcode 1111 nnmm 1110 1101
                    // nn -- represent fvn
                    // mm -- represent fvm
                    //
                    if (prbit)
                    {
                        // else undefined operation
                        // propagate exception to user
                        retVal = 0;
                    }else {
                    
                        RegN = (RegN & 0xc);
                        RegM = ((RegM & 0x3) << 2);

                        if (!frbit)
                        {
                            pFVn = pctx->FRegs + RegN;
                            pFVm = pctx->FRegs + RegM;

                        } else {
                            pFVn = pctx->xFRegs + RegN;
                            pFVm = pctx->xFRegs + RegM;
                        }
                        fipr(pFVm, pFVn);
                    }
                    break;

                case FTRV_OP:
                    if (prbit)
                    {
                        // else undefined operation
                        // propagate exception to user
                        retVal = 0;
                    }else{
                        //
                        // Recalculated RegN for ftrv instruction
                        // ftrv has opcode 1111 nn01 1111 1101
                        // instruction format for ftrv xmtrx,fvn 
                        // nn -- represent fvn
                        // 
                        if (!frbit)
                        {
                            pXMTRX = pctx->xFRegs;
                            pFVn = pctx->FRegs + (RegN -1);

                        }else {

                            pXMTRX = pctx->FRegs;
                            pFVn = pctx->xFRegs + (RegN -1);
                        }
                        ftrv(pXMTRX, pFVn);
                    }
                    break;

                } // switch(RegM)

                break; // end case 0xd

            case FMAC_OP:
                //
                // need to handle differently, 
                // call _muls, then _adds
                //
                if (prbit)
                {
                    // undefined operation
                    // propagate exception to user
                    retVal = 0;
                } else { 

                    if (!frbit)
                    {
                        fp1.i = pctx->FRegs[0];
                    }else{
                        fp1.i = pctx->xFRegs[0];
                    }

                    fp2.i = *fpRegM1;
                    fp2.f = _muls(fp1.f,fp2.f);
                    fp1.i = *fpRegN1;
                    fp2.f= _adds(fp1.f,fp2.f);
                    *fpRegN1 = fp2.i;
                }
                break;

            default:
                NKDbgPrintfW(L"HandleHWFloatException: UNKNOWN INSTRUCTION.\r\n");
                break;

            } // switch(opcode)

        }_finally{

            // pr in fpscr is currently cleared
            // write fpscr into user thread
            pctx->Fpscr = get_fpscr();

            // ensure that pr is set correctly in user context fpscr
            if (prbit) 
            {
                pctx->Fpscr |= PR;
            }

            CauseField = (pctx->Fpscr & 0x0003f000)>>12;
            
            if (CauseField & 0x10)
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            else if (CauseField & 0x8)
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            else if (CauseField & 0x4)
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            else if (CauseField & 0x2)
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            else if (CauseField & 0x1)
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
//#define DEBUG_STRING
#ifdef DEBUG_STRING
            else if(_abnormal_termination())
            {
                NKDbgPrintfW(L"An exception occur after enumlation call in HandleHWFloatException\r\n");
                NKDbgPrintfW(L"fpscr status: 0x%x\r\n", get_fpscr());
            }
#endif
        }
    }__except(1){
        //
        // fp emulation raised exception,
        // return 0 causes exception to be propagated to user.
        //
        return 0;
    }

    //
    // fp operation successfully emulated, return 1 causes control to
    // return to succeeding instruction.
    //
    return retVal;
}

#endif




