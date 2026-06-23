// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

#ifndef FWTABLE_BASE
#define FWTABLE_BASE 0x9fc21000
#endif
#if !defined(_MMOSA_H_) && !defined(__mmmach_h__)
typedef unsigned long UINT32,*PUINT32;
typedef unsigned char UINT8,*PUINT8;
#endif	// _MMOSA_H_
typedef signed long   LONG,*PLONG;

typedef void (*PFUNCPTR) (unsigned int);
typedef void (*PPFUNCPTR) ();
typedef void *(*PVPFUNCPTR) ();
typedef void (*PCFUNCPTR) (unsigned char);
typedef void (*PVFUNCPTR) (void);
typedef UINT32 (*PULVFUNCPTR) (void);
typedef UINT32 (*PULFUNCPTR) ();

#define ENABLEHWINT(_x) (1 | (0x400 << _x))

/*****************************************************/
// Define Interrupt numbers and priorities

/*  ** WARNING: Private defines.  Not to be used outside this file!! */
#define SERIAL_INTR_PRTY  3
#define TIMER_INTR_PRTY 1
#define EXTERN_INTR_PRTY 5

#define IDTCNT0_INTR  0
#define IDTCNT1_INTR  1
#define IDTCENTR_INTR 3
#define IDTSERIAL_INTR  5
#define IDTEXTRN_INTR   2
#define FW_INTR_OFFSET  2
#define PURP_INTR_OFFSET 8
#define PURPLCD_INTR     0
#define PURPIO_INTR      1
#define PURPRADIO_INTR   2
#define PURPHALT_INTR    3
#define PURPCNTS_INTR    6
#define PURPCNTL_INTR    7
#define PURPIO_INTR_OFFSET 0x10
#define PURPIO_PEN_INTR  0
#define PURPIO_PWR_INTR  1
#define PURPIO_ADC_INTR  2
#define PURPIO_AUDIO_INTR  3
#define PURPIO_SMRT_INTR   4
#define PURPIO_SERL_INTR   5

/*****************************************************/
/*  Define Exportable Interrupt values */

#define SERIAL_INTR  (IDTSERIAL_INTR + FW_INTR_OFFSET)
#define TIMER_INTR  (IDTCNT1_INTR + FW_INTR_OFFSET)
#define EXTERN_INTR  (IDTEXTRN_INTR + FW_INTR_OFFSET)
#define RADIO_INTR   (PURPRADIO_INTR + PURP_INTR_OFFSET)
#define LCD_INTR     (PURPLCD_INTR + PURP_INTR_OFFSET)
#define IO_INTR      (PURPIO_INTR + PURP_INTR_OFFSET)
#define COUNTS_INTR  (PURPCNTS_INTR + PURP_INTR_OFFSET)
#define COUNTL_INTR  (PURPCNTL_INTR + PURP_INTR_OFFSET)
#define HALT_INTR    (PURPHALT_INTR + PURP_INTR_OFFSET)
#define PEN_INTR     (PURPIO_PEN_INTR + PURPIO_INTR_OFFSET)
#define PWR_INTR     (PURPIO_PWR_INTR + PURPIO_INTR_OFFSET)
#define ADC_INTR     (PURPIO_ADC_INTR + PURPIO_INTR_OFFSET)
#define AUDIO_INTR   (PURPIO_AUDIO_INTR + PURPIO_INTR_OFFSET)
#define SMRT_INTR    (PURPIO_SMRT_INTR + PURPIO_INTR_OFFSET)
#define SERL_INTR    (PURPIO_SERL_INTR + PURPIO_INTR_OFFSET)



#if 0
/********  Undefine private defines *******************/
#undef IDTCNT0_INTR  
#undef IDTCNT1_INTR  
#undef IDTCENTR_INTR 
#undef IDTSERIAL_INTR  
#undef IDTEXTRN_INTR   
#undef FW_INTR_OFFSET  
#undef PURP_INTR_OFFSET 
#undef PURPLCD_INTR     
#undef PURPIO_INTR      
#undef PURPRADIO_INTR   
#undef PURPHALT_INTR    
#undef PURPCNTS_INTR    
#undef PURPCNTL_INTR    
#endif

/****************************/
/* Define Exception Types   */

#define EXCPT_EXTRN         0
#define EXCPT_TLB_MOD       1
#define EXCPT_TLBL_MISS     2
#define EXCPT_TLBS_MISS     3 
#define EXCPT_ADDR_LOAD     4
#define EXCPT_ADDR_STOR     5
#define EXCPT_BUS_IFTCH     6
#define EXCPT_BUS_DATA      7
#define EXCPT_SYSCALL       8
#define EXCPT_BRKPNT        9
#define EXCPT_RSRVD_INSTR   10
#define EXCPT_COP_NUSE      11
#define EXCPT_OVFLW         12
#define EXCPT_RESERVED      13

/*****************************/
/* Define Debugger Choices */
#define DEBUG_DEFAULT  2
#define DEBUG_INTERNAL 4
#define DEBUG_REMOTE   8

/****************************/

#if 0 // Changed semi-permanent temporarily
#define CALL_FUNC(x)  ( (PPFUNCPTR) (8*x + FWTABLE_BASE))
#define CALL_VPFUNC(x)  ( (PVPFUNCPTR) (8*x + FWTABLE_BASE))
#define CALL_ULVFUNC(x)  ( (PULVFUNCPTR) (8*x + FWTABLE_BASE))
#endif
#define CALL_FUNC(x)  ( (PULFUNCPTR) (8*x + FWTABLE_BASE))
#define CALL_VPFUNC(x)  ( (PVPFUNCPTR) (8*x + FWTABLE_BASE))


#define FW_INITEXCPT  0
#define FW_REGHWINTR  1
#define FW_REGEXCPT   2
#define FW_FLSHI      3
#define FW_FLSHD      4
#define FW_CM2PUTS    5
#define FW_CM1PUTS    6
#define FW_NM2ASCII   7
#define FW_FLSHTLB    8
#define FW_ADDBP      9
#define FW_DBGR       10
#define FW_USEALT     11
#define FW_CM1GETB    12
#define FW_INITTMR    13
#define FW_GETSTAT    14
#define FW_LOADSYM    15
#define FW_REGOS      16
#define FW_REGMM      17
#define FW_ENBINT     18
#define FW_DISINT     19
#define FW_RETXCPT    20 
#define FW_DBGBRK     21
#define FW_PRNTSTR    22 
#define FW_CLRDISP    23 
#define FW_ENBLINT    24 
#define FW_GETDIAG    25 
#define FW_SETDIAG    26 
#define FW_CM2GETB    27
#define FW_REGMHWINTR 28
#define FW_CM2PUTC    29
#define FW_CM1PUTC    30

#define FFWInitExcptHandler(_a) CALL_FUNC(FW_INITEXCPT)(_a)
/*  VOID FWInitExcptHandler(UINT32 DebuggerChoice)  */

#define FFWRegisterHWIntrHndlr(_a,_b,_c) CALL_FUNC(FW_REGHWINTR)(_a,_b,_c)
/*  VOID FWRegisterHWIntrHndlr(PVFUNCPTR Handler,UINT32 IntrNumber, UINT32 Priority)  */

#define FFWRegisterExcptHndlr(_a,_b)  CALL_FUNC(FW_REGEXCPT)(_a,_b)
/*  VOID FWRegisterExcptHndlr(PVFUNCPTR Handler,UINT32 ExcptNumber)     */

#define FFWFlushICache() CALL_FUNC(FW_FLSHI)()
/*  VOID FWFlushICache(VOID)     */

#define FFWFlushDCache() CALL_FUNC(FW_FLSHD)()
/*  VOID FWFlushDCache(VOID)     */

#define FFWPort2Puts(_a) CALL_FUNC(FW_CM2PUTS)(_a)
/*  VOID FWPort2Puts(PUINT8)     */

#define FFWPort1Puts(_a) CALL_FUNC(FW_CM1PUTS)(_a)
/*  VOID FWPort1Puts(PUINT8)     */

#define FFWNum2AsciiHex(_a,_b) CALL_FUNC(FW_NM2ASCII)(_a,_b)
/*  VOID FWNum2AsciiHex(PUINT8 DestBuffer,UINT32 Number)   */

#define FFWFlushTLB()     CALL_FUNC(FW_FLSHTLB)()
/*  VOID FWFlushTLB(VOID)        */

#define FFWAddBp(_a)      CALL_FUNC(FW_ADDBP)(_a)
/*  VOID FWAddBp(PUINT32 VAddr)   */

#define FFWEnterDebugger() CALL_FUNC(FW_DBGR)()
/*  VOID FWEnterDebugger(VOID)   */

#define FFWUseAlternateDebugger(_a) CALL_FUNC(FW_USEALT)(_a)
/*  UINT32 FWUseAlternateDebugger(UINT32 DebuggerChoice)   */

#define FFWPort1GetByte(_a) CALL_FUNC(FW_CM1GETB)(_a)
/*  UINT32 FWPort1GetByte(PUINT8 DestinationByte)  */

#define FFWInitializeTimer(_a) CALL_FUNC(FW_INITTMR)(_a)
/*  UINT32 FWInitializeTimer(TMR_STRUCT pTimer)  */

#define FFWGetStatusInfo() CALL_VPFUNC(FW_GETSTAT)()
/*  pHW_STATUSINFO FWGetStatusInfo()  */

#define FFWReportLoadExe(_a,_b) CALL_FUNC(FW_LOADSYM)(_a,_b)
/*  pHW_STATUSINFO FWGetStatusInfo(PUINT8 ExeName,pKD_SYMBOLS_INFO SymbolInfo)*/

#define FFWRegisterOSQueries(_a,_b,_c) CALL_FUNC(FW_REGOS)(_a,_b,_c)
/*  pHW_STATUSINFO FWRegisterOSQueries(PFUNCPTR GetCurrentThread,PFUNCPTR GetCurrentProcess,PFUNCPTR GetThreadContext)*/

#define FFWRegisterMMQueries(_a,_b) CALL_FUNC(FW_REGMM)(_a,_b)
/*  pHW_STATUSINFO FWRegisterMMQueries(PFUNCPTR XlateVMRead,PFUNCPTR XlateVMWrite)*/

#define FFWEnableInterrupts(_a)  CALL_FUNC(FW_ENBINT)(_a)
/*  VOID EnableInterrupts(UINT32 OrInMask)    */

#define FFWDisableInterrupts()  CALL_FUNC(FW_DISINT)()
/*  IntStatus EnableInterrupts()    */

#define FFWReturnFromExcpt()  CALL_FUNC(FW_RETXCPT)()
/*  NORETURN ReturnFromException()    */

#define FFWDebugBreak(_a)  CALL_FUNC(FW_DBGBRK)(_a)
/*  VOID _DebugBreak()    */

#define FFWPrintString(_a,_b,_c,_d)  CALL_FUNC(FW_PRNTSTR)(_a,_b,_c,_d)
/*  VOID _FwPrintString(PUINT8 String,UINT32 x,UINT32 y,UINT8 color)    */

#define ClearDisplay()  CALL_FUNC(FW_CLRDISP)()
/*  VOID FwClearLCD(VOID)    */

#define FFWEnableInt()  CALL_FUNC(FW_ENBLINT)()
/*  VOID FwEnableInt()    */

#define FFWGetDiagCounter()  CALL_FUNC(FW_GETDIAG)()
/*  VOID FwGetDiagCounter()    */

#define FFWSetDiagCtr()  CALL_FUNC(FW_SETDIAG)()
/*  VOID FwSetDiagCounter()    */

#define FFWPort2GetByte(_a) CALL_FUNC(FW_CM2GETB)(_a)
/*  UINT32 FWPort2GetByte(PUINT8 DestinationByte)  */

#define FFWRegisterWrappedHWIntrHndlr(_a,_b,_c,_d,_e) CALL_FUNC(FW_REGMHWINTR)(_a,_b,_c,_d,_e)
/*VOID FWRegisterWrappedIntrHndlr(PVFUNCPTR Handler,ULONG IntrNumber,ULONG IntrPriority,UINT32 Data, UINT32 fSaveContext) */

#define FFWPort2PutC(_a) CALL_FUNC(FW_CM2PUTC)(_a)
/*  VOID FWPort2PutC(UINT8)     */

#define FFWPort1PutC(_a) CALL_FUNC(FW_CM1PUTC)(_a)
/*  VOID FWPort1PutC(UINT8)     */

/**************************************************************/
/* define MISC structures required...  */

#ifndef KDSYMBOLSDEFINED

typedef void *PVOID;

typedef struct _KD_SYMBOLS_INFO {
     void * BaseOfDll;
     UINT32 ProcessId;
     UINT32 CheckSum;
     UINT32 SizeOfImage;
} KD_SYMBOLS_INFO, *PKD_SYMBOLS_INFO;
#define KDSYMBOLSDEFINED 1

#endif

/**************************************************************/

