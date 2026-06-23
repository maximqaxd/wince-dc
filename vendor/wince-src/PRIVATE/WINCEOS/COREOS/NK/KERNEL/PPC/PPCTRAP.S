//      TITLE("Interrupt and Exception Processing")
//++
//
// Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    ppctrap.s
//
// Abstract:
//
//    This module implements the code necessary to field and process PowerPC
//    interrupt and exception conditions.
//
// WARNING: Some of the code in this module runs with relocation disabled and
// cannot directly reference kernel global data. However, the kernel data page is
// chosen to have the same virtual and physical address (0x5800) and can be used
// with relocation disabled.
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksppc.h"
#include "nkintr.h"
#include "psyscall.h"
#include "mem_ppc.h"
#include "kpage.h"



//
// Constants
//

#define HwInterruptVector       0x500   // Hardware interrupt vector
#define DecrementerVector       0x900   // Decrementer exception vector


//
//  Externs from mdppc.c
//
.extern CEProcessorType                 // DWORD CEProcessorType;
.extern ProcessorLevel                  // WORD  ProcessorLevel;
.extern ProcessorRevision               // WORD  ProcessorRevision;


#if 0   // SAVEGP
    .globl  _gp                     // Global pointer value set by linker
#endif
    .globl    KPageBlock
    .globl    NKSignon
    .globl    ObjectCall
    .globl    HandleException
    .globl    ServerCallReturn
    .globl    KernelRelocate
    .globl    OEMInitDebugSerial
    .globl    OEMInit
    .globl    OEMWriteDebugString
    .globl    KernelFindMemory
    .globl    KernelInit
    .globl    ExceptionDispatch
#if CELOG
    .globl    CeLogInterrupt
#endif


#define MSR_CLR_EE(reg)\
    rlwinm    reg, reg, 0, ~MSR_EE

#define MSR_CLR_ME(reg)\
    rlwinm    reg, reg, 0, ~MSR_ME

#define MSR_CLR_RI(reg)\
    rlwinm    reg, reg, 0, ~MSR_RI

#define MSR_CLR_POW(reg) \
    rlwinm    reg, reg, 0, ~MSR_POW

//
//  TLB management routines.
//
//  Include the processor-specific TLB management routines here:
//

#if PPC403
#include "ppc403.s"
#endif

#if PPC821
#include "ppc821.s"
#endif

//
// Shared low memory interrupt code
//
// This code is put into the kernel data page so that it can execute
// with relocation enabled or disabled. It performs its initial work without
// relocation and then enables relocation and jumps to the kernel handler in
// the kernel "un-mapped" space.
//

    .text

KPageHandlers:

// Low memory API call and return handler.
//
// Prepare a thread for an API call. Since this is a C level function
// call, it is not necessary to save the volatile CPU state.
//
// This code is located in the kernel page at 0x5000 so that it can
// run with relocation enabled or disabled and in kernel mode or user mode.
// The sequence is:
//    Entry in kernel mode with relocation disabled
//    Enable relocation
//    decode the API
//    Switch to user mode
//    invoke the API or return to API caller
//
// Entry  (r3-r10) = API arguments
//        (r11) = API call address (SRR0) - FIRST_METHOD
//        (r12) = previous Status (SRR1)
// Exit to API dispatch code in kernel with:
//        (r1) = kernel stack pointer
//        (r11) = API set & index
//        (r12) = caller's stack pointer
// Uses   r0, r11, r12, CR0, CR5

//
//  Fake Prologue required for unwinding. Save GP and RA on stack
//

   .set KPageAPIHandler, ($ - KPageHandlers) + LowHandlers
    mfmsr   r0                      // Get current MSR settings
    ori     r0, r0, (MSR_IR | MSR_DR | MSR_EE | MSR_RI | MSR_ME)
                                    // OR in new bits (in low half word)
    mtmsr   r0                      // enable relocation & interrupts
    rlwinm  r12, r12, MSR_BIT_PR+1, 1
                                    // (r12) = previous processor mode
    cmpwi   r11, -APICALL_SCALE
    stw     r3, CrParameter0(r1)    // store args onto the stack
    stw     r4, CrParameter1(r1)    // ...
    beq     APIReturn               // go pop the callstack for an API return
    stw     r5, CrParameter2(r1)    // ...
    stw     r6, CrParameter3(r1)    // ...
    stw     r7, CrParameter4(r1)    // ...
    stw     r8, CrParameter5(r1)    // ...
    stw     r9, CrParameter6(r1)    // ...
    stw     r10, CrParameter7(r1)   // ...
    mflr    r4                      // (r4) = Arg1, return address
    addi    r5, r1, CrParameter0    // (r5) = Arg2, ptr to API parameters
    srawi   r6, r11, 2              // (r6) = Arg3, API set & index
    stwu    r1, -StackFrameHeaderLength-8(r1)
    lis     r3, [hia]ObjectCall
    addi    r3, r3, [lo]ObjectCall  // (r0) == ptr to ObjectCall
    mtctr   r3
    addi    r3, r1, StackFrameHeaderLength
                                    // (r3) = Arg0, ptr to OCALLINFO struct
    stw     r12, 0(r3)              // store previous thread execution mode
    stw     r2, 4(r3)               // store caller's Global pointer
    lwz     r2, NKGp(0)
    bctrl                           // call ObjectCall(poci, ra, args, iMethod)

// Reload argument registers and invoke an API function.
//
// (r3) = pointer to an API function to invoke (0 if API completed)

    lwz     r12, StackFrameHeaderLength(r1)
                                    // (r12) = thread mode
    lwz     r1, 0(r1)               // clear working frame from the stack
    cmpwi   r12, 0
    mtctr   r3
    lwz     r3, CrParameter0(r1)    // reload argument registers from the stack
    lwz     r4, CrParameter1(r1)    // ...
    lwz     r5, CrParameter2(r1)    // ...
    lwz     r6, CrParameter3(r1)    // ...
    lwz     r7, CrParameter4(r1)    // ...
    lwz     r8, CrParameter5(r1)    // ...
    lwz     r9, CrParameter6(r1)    // ...
    lwz     r10, CrParameter7(r1)   // ...
    beq     APICall                 // invoke the function in kernel mode
    li      r0, SYSCALL_RETURN
    mtlr    r0                      // setup for api return interrupt
    mfmsr   r0                      // Get current MSR
    ori     r0, r0, (MSR_PR | MSR_EE | MSR_ME)
                                    // Set problem state bit to user mode
    mtmsr   r0                      // switch to user mode

    bctr                            //  and jump to the API function

// Invoke an API function in kernel mode. This may be due to the api being a
// kernel function or the thread running in kernel mode.
//
//    (ctr) = function address
//    (r2) = function's Global pointer
//    (r3-r10) = API arguments

APICall:
    bctrl                           // call API function
   .set APICallReturn, ($ - KPageHandlers) + LowHandlers
    li      r12, 0                  // (r12) = KERNEL_MODE
    stw     r3, CrParameter0(r1)    // save return value onto the stack
    stw     r4, CrParameter1(r1)    // ...
APIReturn:
    stwu    r1, -StackFrameHeaderLength-16(r1)
    lis     r3, [hia]ServerCallReturn
    addi    r3, r3, [lo]ServerCallReturn
                                    // (r3) = Ptr to ServerCallReturn
    mtctr   r3
    addi    r3, r1, StackFrameHeaderLength
                                    // (r3) = Arg0, ptr to OCALLINFO struct
    stw     r12, 0(r3)              // store previous thread execution mode
    lwz     r2, NKGp(0)
    bctrl                           // call ServerCallReturn(poci)

    lwz     r12, StackFrameHeaderLength(r1)
                                    // (r12) = thread mode
    lwz     r2, StackFrameHeaderLength+4(r1)
                                    // (r2) = original caller's Global ptr
    lwz     r1, 0(r1)               // clear working frame from the stack
    mtlr    r3
    cmpwi   r12, 0
    lwz     r3, CrParameter0(r1)    // restore return value registers from
                                    //   the stack
    lwz     r4, CrParameter1(r1)    // ...
    beqlr                           // return to kernel mode caller
    mfmsr   r0                      // Get current MSR
    ori     r0, r0, (MSR_PR | MSR_EE | MSR_ME)
                                    // Set problem state bit to user mode,
                                    //   enable interrupts and machine check
    mtmsr   r0                      // Switch to user mode
    blr                             //  and return


//
// Low memory common interrupt handling code
//
// This code is put into the kernel data page at so that it can execute
// with relocation enabled or disabled. It performs its initial work without
// relocation and then enables relocation and jumps to the kernel handler in
// the kernel "un-mapped" space.
//

//
// KPageHwInterruptHandler - Handler used for decrementer and hw interrupt
//  exceptions.  The code assumes the following entry and is running w/o
//  relocations or interrupts.
//
//  (r5) = Address of OEMInterruptHandler or OEMDecrementer
//  (SPRG1) = Original r14
//  (SPRG2) = Original r5
//

.set KPageHwInterruptHandler, ($ - KPageHandlers) + LowHandlers

    lha     r14, ReschedFlag(0)
    mtspr   SPRG3, r0
    mfcr    r0                      // (r0) = saved condition register
    subi    r14, r14, 256
    mtcrf   0x80, r14                // copy r6 sign bit to bit 0 of CR
    sth     r14, ReschedFlag(0)
    lwz     r14, CurThdPtr(0)        // (r6) = ptr to Current Thread struct
    bt-     0, hwh10                 // nested interrupt
    rlwinm  r14, r14, 0, 0x7fffffff   // (r6) = real address of THREAD
    stw     r1, TcxR1(r14)
    li      r1, KStack              // switch to kernel's stack
    b   hwh15

    //
    // A nested exception has occured. Create a temporary thread structure
    // on the stack and save the current state into that.
    //

hwh10:
    subi    r14, r1, TcxSizeof + STK_SLACK_SPACE
    stw     r1, TcxR1(r14)
    subi    r1, r14, StackFrameHeaderLength

hwh15:
    stw     r0, TcxCr(r14)
    stw     r3, TcxR3(r14)
    stw     r4, TcxR4(r14)
    mfspr   r0, SRR0                    // (r0) = faulting instr. address
    mfspr   r3, SRR1                    // (r3) = MSR at fault
    stw     r0, TcxIar(r14)
    stw     r3, TcxMsr(r14)


    //
    // Save the remaining context to allow for C calling convention:
    //
    //  (r14) = non-volatile thread context
    //  (r5) = Address of OEMInterruptHandler or OEMDecrementer
    //  (r6) = real address ptr to THREAD
    //  (SPRG1) = original R14 value
    //  (SPRG2) = original R5 value
    //  (SPRG3) = original R0 value
    //
    //  R1, R3, CR, SRR0, & SRR1 saved into THREAD.ctx
    //

    stw     r7, TcxR7(r14)
    mfspr   r0, SPRG1                   // (r0) = original R4 value
    mfspr   r7, SPRG2                   // (r7) = original R5 value
    stw     r0, TcxR14(r14)
    stw     r7, TcxR5(r14)
    mfspr   r0, SPRG3                   // (r0) = original R0 value
    stw     r0, TcxR0(r14)
    mfxer   r7
    stw     r7, TcxXer(r14)
    mfctr   r0
    mflr    r7
    stw     r0, TcxCtr(r14)
    stw     r7, TcxLr(r14)
    stw     r2, TcxR2(r14)              // save Thread's Global pointer
    lwz     r2, NKGp(0)                 // (r2) = kernel's GP
    stw     r6, TcxR6(r14)
    stw     r8, TcxR8(r14)              // save volatile register state
    stw     r9, TcxR9(r14)              // ...
    stw     r10, TcxR10(r14)            // ...
    stw     r11, TcxR11(r14)            // ...
    stw     r12, TcxR12(r14)            // ...
    stw     r13, TcxR13(r14)            // ...


    //
    // Call the OEM supplied ISR.  The ISR requires that relocations be on so
    // we use the rfi instruction to simultaneously enable relocations and jump
    // to the ISR continue point (ISRContinue).  ISRContinue will call the OEM
    // supplied ISR code.
    //
    //  (r5) = KSEG address of the ISR (OEMDecrementer or OEMInterruptHandler)
    //  (r14) = non-volatile thread context
    // R0-R11, Cr, Lr, Ctr, SRR0, & SRR1 saved into THREAD.ctx
    //

    //
    // Setup the MSR that will be used while running ISR.  We
    // need:
    //
    //  - Relocations
    //  - Interrupts OFF
    //
    // NOTE: We do not explicitly turn interrupts off because this code enters
    //  with interrupts off.
    //
    //  (r14) = Non-volatile thread ctx
    //

    mfmsr   r0                          // Get current MSR
    MSR_CLR_ME(r0)                      // Machine check off
    ori r0, r0, (MSR_IR | MSR_DR | MSR_RI)
                                        // Relocations ON

    //
    // Setup SRR0 and SRR1 which are used for the rfi instruction
    //

    lis     r3, [hia]IsrContinue          // Load address of KSEG ISR continue point
    addi    r3, r3, [lo]IsrContinue       // ...

    mtspr   SRR1, r0			// MSR at fault
    mtspr   SRR0, r3			// address of IsrContinue code

    oris    r14, r14, 0x8000            // (r3) = virtual address of THREAD
    rfi                                 // jump to KSEG ISR continue point with relocations

//
// KPageGeneralHandler - General Handler for non-TLB and non-HW related interrupts.
//  Exceptions handled include: alignment, machine check, page fault, access
//  violation, etc.
//
// (r4) = interrupt type
// (r5) = interrupt data (DAR or SRR0)
// (SPRG1) = original R4 value
// (SPRG2) = original R5 value
//

.set KPageGeneralHandler, ($ - KPageHandlers) + LowHandlers
    stw     r6, SaveR6(0)
    lha     r6, ReschedFlag(0)
    mtspr   SPRG3, r0
    mfcr    r0                      // (r0) = saved condition register
    subi    r6, r6, 256
    mtcrf   0x80, r6                // copy r6 sign bit to bit 0 of CR
    sth     r6, ReschedFlag(0)
    lwz     r6, CurThdPtr(0)        // (r6) = ptr to Current Thread struct
    bt-     0, gh10                 // nested interrupt
    rlwinm  r6, r6, 0, 0x7fffffff   // (r6) = real address of THREAD
    stw     r1, TcxR1(r6)
    li      r1, KStack              // switch to kernel's stack
    b   gh15

    //
    // A nested exception has occured. Create a temporary thread structure
    // on the stack and save the current state into that.
    //
    //  (r4) = interrupt type
    //

gh10:
    subi    r6, r1, TcxSizeof + STK_SLACK_SPACE
    stw     r1, TcxR1(r6)
    subi    r1, r6, StackFrameHeaderLength

gh15:
    stw     r0, TcxCr(r6)
    stw     r3, TcxR3(r6)
    mfspr   r0, SRR0                // (r0) = faulting instr. address
    mfspr   r3, SRR1                // (r3) = MSR at fault
    stw     r0, TcxIar(r6)
    stw     r3, TcxMsr(r6)
    oris    r3, r6, 0x8000          // (r3) = virtual address of THREAD

// Handle a fault.
//
//  (r3) = ptr to THREAD (virtual address)
//  (r4) = interrupt type
//  (r5) = interrupt data (DAR or SRR0)
//  (r6) = real address ptr to THREAD
//  (SaveR6) = original R6 value
//  (SPRG1) = original R4 value
//  (SPRG2) = original R5 value
//  (SPRG3) = original R0 value
//  R1, R3, CR, SRR0, & SRR1 saved into THREAD.ctx

    stw     r7, TcxR7(r6)
    mfspr   r0, SPRG1               // (r0) = original R4 value
    mfspr   r7, SPRG2               // (r7) = original R5 value
    stw     r0, TcxR4(r6)
    stw     r7, TcxR5(r6)
    mfspr   r0, SPRG3               // (r0) = original R0 value
    lwz     r7, SaveR6(0)           // (r7) = original R6 value
    stw     r0, TcxR0(r6)
    stw     r7, TcxR6(r6)
    mfxer   r7
    stw     r7, TcxXer(r6)
    mfctr   r0
    mflr    r7
    stw     r0, TcxCtr(r6)
    stw     r7, TcxLr(r6)
    stw     r2, TcxR2(r6)           // save Thread's Global pointer
    lwz     r2, NKGp(0)             // (r2) = kernel's GP
    stw     r8, TcxR8(r6)           // save volatile register state
    stw     r9, TcxR9(r6)           // ...
    stw     r10, TcxR10(r6)         // ...
    stw     r11, TcxR11(r6)         // ...
    stw     r12, TcxR12(r6)         // ...
    stw     r13, TcxR13(r6)         // ...
    stw     r14, TcxR14(r6)         // save R14
    mr      r14, r3                 // (r14) = save pointer to THREAD


//
//  Determine whether load/store occurred, if appropriate
//

#if PPC821
    mfspr   r7, DSISR                           // (r6) = interrupt info
    rlwinm  r6, r7, 6+1, 31, 31                 // Isolate Load/Store bit
#endif

#if PPC403
    mfspr   r7, ESR                             // Exception Status Register

//
//  Map the Program trap bits from the ESR into the architected SRR1 positions:
//

    rlwinm. r0, r7, 32+6-14, 12, 14     // Copy bits to SRR1 positions
    beq     Srr1Zero                    // No bits set; continue
    lwz     r7, TcxMsr(r6)              // Get SRR1 value
    or      r7, r7, r0                  // Insert program trap bits
    stw     r7, TcxMsr(r6)              // and update
Srr1Zero:
    rlwinm  r6, r7, 8+1, 31, 31         // Isolate Load/Store bit

#endif


    //
    // Call HandleException.  This involves turning interrupts and relocations
    // on and jumping to HandleException.  This is done using the PPC rfi mechanism
    // to simultaneously enable interrupts and relocations and jump to
    // HandleException.
    //
    //
    //  (r3) = ptr to THREAD (virtual address)
    //  (r4) = interrupt type
    //  (r5) = interrupt data (DAR or SRR0)
    //  (r14) = Non-volatile thread ctx
    //  R0-R11, Cr, Lr, Ctr, SRR0, & SRR1 saved into THREAD.ctx
    //

    lis     r7, [hia]HandleException    // Get Ptr to HandleException
    addi    r7, r7, [lo]HandleException // ...

    //
    // Setup the MSR that will be used while running HandleException.  We
    // need:
    //
    //  - Relocations
    //  - Interrupts ON
    //

    mfmsr   r0                          // Get current MSR
    MSR_CLR_ME(r0)                      // Machine check off
    ori r0, r0, (MSR_IR | MSR_DR | MSR_RI | MSR_EE)
                                        // Relocation and interrupts on
    mtspr   SRR1, r0                    // MSR that will be used to execute handle
    mtspr   SRR0, r7                    // Address of the HandleException

    //
    // When the HandleException finishes it will return to KSEG address that
    // does the final portion of the interrupt processing.
    //

    lis     r8, [hia]HandleExceptionReturn      // Get Ptr to HandleExceptionReturn
    addi    r8, r8, [lo]HandleExceptionReturn   // ...
    mtlr    r8                                  // Upon return, run out of KSEG
    rfi                                         // jump to HandleException


   .set KPageJumpSwitch, ($ - KPageHandlers) + LowHandlers


//
// KPageJumpSwitch - switch to user mode and jump to user function
//  This code is used to invoke user mode exception handler functions.
//
//  Entry   (r3-r6) = function parameters
//          (r8) = Msr value
//          (ctr) = function address
//          (lr) = return address - 4 (must skip an instruction)
//

//
//

	lwz		r9, CurThdPtr(0)	// (r9) = pCurThread
	addi	r7, r1, CfCstk		// (r7) = pcstk (on our stack)
	lwz		r9, ThPcstkTop(r9)	// (r9) = pCurThread->pcstkTop
	stw		r9, CstkNext(r7)	// pcstk->pcstkNext = pCurThread->pcstkTop
	lwz		r9, CurThdPtr(0)	// (r9) = pCurThread
	stw		r7, ThPcstkTop(r9)	// pCurThread->pcstkTop = pcstk
	mr		r9, r7				// (r9) = pcstk
	mflr	r7
	addi	r7, r7, 4			// (r7) = return address

    li      r0, SYSCALL_RETURN
    stw     r7, CstkRa(r9)
    mtlr    r0                  // set return addr to trap
    li      r0, 0
    stw     r0, CstkPrcLast(r9) // return to kernel mode
    li      r0, 0
    stw     r0, CstkAkyLast(r9)
    mtmsr   r8
    bctr

KPageHandlers.End:
    .set KPageEnd, ($ - KPageHandlers) + LowHandlers


.text

//
// IsrContinue - KPage routine uses rfi to jump to here with relocations enabled
//
//  (r5) = KSEG address of the ISR (OEMDecrementer or OEMInterruptHandler)
//  (r14) = Virtual Address of THREAD
//

IsrContinue:

#if CELOG
        //
        // NOTE : "C" calling convention to CeLogInterrupt
        //
        // I'm assuming that at this point the only register of these that
        // I need to preserve is r5 (ISR address) and that R13 is available
        // as storage (non-volatile for function call)
        //

	mr	r13, r5			// save r5

	lis	r3, 0x8000		// r3 = mark as ISR entry
        bl      CeLogInterrupt		// call CeLogInterrupt function

	mr	r5, r13			// restore r5
#endif

	mtlr	r5			// LR = OEMDecrementer or OEMInterruptHandler address
	blrl				// call OEMDecrementer or OEMInterruptHandler

	//
	// Return from ISR
	//
	// (r3) = SYSINTR value
	//

    lwz     r4, TcxMsr(r14)
    MSR_CLR_POW(r4)
    stw     r4, TcxMsr(r14)


    //
    // Disable interrupts so that the setting of necessary reschedule flags
    // and PendEvents is atomic
    //

    mfmsr   r4                          // Get the MSR
    MSR_CLR_EE(r4)                      // Clear External Interrupts bit
    mtmsr   r4                          // Write back MSR

#if CELOG
        //
        // NOTE : "C" calling convention to CeLogInterrupt
        //
        // I'm assuming that at this point the only register of these that
        // I need to preserve is r3 (SYSINTR value) and that R13 is available
        // as storage (non-volatile for function call)
        //
        mr      r13, r3                 // save copy of R3
        lbz     r4, KNest(0)            // Get Nesting level
        extsb.  r4, r4                  // Sign extend KNest byte
        neg     r4, r4
        rlwinm  r4, r4, 16, 0, 31       // r4 = r4 << 16
        or      r3, r3, r4              // r3 = (-cNest << 16) | SYSINTR_val
        bl      CeLogInterrupt

        mr      r3, r13                 // restore R3

#endif

    //
    // Load up constant 1
    //

    li      r10, 1

	//
	// Dispatch to specific handler.  The default is a hardware interrupt.
	//		

    cmpwi   r3, SYSINTR_BREAK           // Check for a SYSINTR_BREAK
    beq     SysintrBreak                // ...
    cmpwi   r3, SYSINTR_NOP             // Check for a SYSINTR_NOP
    beq     SysintrNop                  // ...
    cmpwi   r3, SYSINTR_RESCHED         // Check for a SYSINTR_RESCHED
    beq     SysintrResched              // ...

SysintrDevice:

    //
    // If none of the other match, we have a device interrupt. The code block
    // below implements the following algorithm:
    //
    //      - Subtract SYSINTR_DEVICES from the ISR returned SYSINTR
    //      - Shift 1 left by this returned value and OR the resulting
    //        mask into PendEvents
    //

    subi    r11,r3,SYSINTR_DEVICES      // convert to Device ID
    cmpwi   r11, SYSINTR_MAX_DEVICES
    bge-    SysintrResched              // if r3 > SYSINTR_MAX_DEVICES, continue

    lwz     r9,PendEvents(0)            // Load current PendEvents
    slw     r11,r10,r11                 // (1 << SYSINTR - SYSINTR_DEVICES)
    or      r11,r11,r9                  // Or interrupt back into PendEvents
    stw     r11,PendEvents(0)           // Write PendEvents back

SysintrResched:

    //
    // Handle the reschedule case.  This will be entered through a device
    // interrupt or a direct SYSINTR_RESCHED
    //

	stb	    r10, ReschedFlag(0)         // set reschedule flag

SysintrNop:

    //
    // All paths fall through to the SYSINTR_NOP handler.  This includes
    // a SYSINTR_DEVICE, a SYSINTR_RESCHED and SYSINTR_NOP.
    //

    lwz     r3, ReschedFlag(0)          // Load ReschedFlag + KNest
    cmpwi   r3, 1                       // Compare flag to 1
    bne	    ContinueCurThread           // no reschedule
    lbz     r3, KNest(0)                // Get Nesting level
    extsb.  r3, r3                      // Sign extend KNest byte
    beq     Reschedule                  // Continue our current thread
    b       ContinueCurThread           // Reschedule

SysintrBreak:

    //
    // The break button has been pushed. Call HandleException with a FAKE
    // interrupt ID of ID_HW_BREAK
    //

    mr      r3, r14                     // Load context record in arg1
    .lwi    r4, ID_HW_BREAK             // HW break type exception
    bl      HandleException             // Call general exception handler

HandleExceptionReturn:

    //
    // (r14) contains the current thread
    //

    cmpwi   r3, 0                       // HandleException returns TRUE for cont
    bne     ContinueCurThread           // If value is TRUE, continue current thrd

Reschedule:

    //
    // Generic reschedule point. Several sources jump to this point to
    // trigger a reschedule.  It also is the result of returned reschedule
    // condition from HandleException.
    //
    //  (r14) = Contains the current thread context
    //


    mfmsr   r0                          // Get current MSR
    ori     r0, r0, MSR_EE              // Interrupts On
    mtmsr   r0                          // Update MSR


    //
    // Check for power off
    //

    lbz     r3, BPowerOff(0)            // Load current value of BPowerOff
    cmpwi   r3, 0                       // Sign Extend
    beq     CheckResched                // If power off is not needed check for resched
    bl      DoPowerOff                  // Call Kernels power down routine.
    li      r3, 0                       // Get zero register
    stb     r3, BPowerOff(0)            // Clear Power off


CheckResched:

    lbz     r5, ReschedFlag(0)          // Load the reschedule flag
    cmpwi   r5, 0                       // Sign extend the reschedule flag
    beq     CheckKCResched              // If resched not set,check KCResched
    li      r3, 0                       // Get zero register
    stb     r3, ReschedFlag(0)          // Clear reschedule flag
    bl      NextThread                  // Call Next Thread and let the scheduler do its thing

CheckKCResched:

    lwz     r4, dwKCRes(0)              // Load the KCResched Flag
    cmpwi   r4, 0                       // Check if it's zero.
    beq     CheckRunList                // If no KCResched, go look at runlist

    li      r3, 0                       // load a constant 0
    stw     r3, dwKCRes(0)              // Write 0 to KCResched Flag
    bl      KCNextThread                // Call KC Sceduler
    lwz     r4, dwKCRes(0)              // Load value of KCResched after func
    cmpwi   r4, 0                       // Compare KCResched to 0
    bne     CheckResched                // If it's non-zero do resched again

CheckRunList:

    lis     r4, [hia]RunList            // Load address of run list
    addi    r4, r4, [lo]RunList         // ..
    lwz     r3, 4(r4)                   // Get pth from runlist
    cmpwi   r3, 0                       // if it's zero, we're idle
    bne     DoNewThread                 // Go get a new thread if there's one

Idle:
	mfmsr   r0							// Turn off interrupts
    MSR_CLR_EE(r0)
    mtmsr   r0
	
    bl      OEMIdle                     // Call OEM's idle routine

	mfmsr   r0							// Turn on interrupts
    ori     r0, r0, MSR_EE
    mtmsr   r0

    b       CheckResched                // Try reschedule path

DoNewThread:

    //
    // Scheduling new thread:
    //
    //  (r3) = RunList.pth
    //

    bl      SetCPUASID
    lis     r4, [hia]RunList            // Load address of run list
    addi    r4, r4, [lo]RunList         // ..
    lwz     r3, 4(r4)                   // Get pth from runlist
    lwz     r4, 0x3c(r3)                // get RunList.pth->hTh
    stw     r4, hCurThread(0)           // Store back current thread

    //
    // Ready to load our new thread.
    //
    // (r3) = RunList.pth
    //


    cmpwi   r14, 0
    beq-    LoadNewThread

    stw     r15, TcxR15(r14)    // save remaining general registers
    stw     r16, TcxR16(r14)    // ...
    stw     r17, TcxR17(r14)    // ...
    stw     r18, TcxR18(r14)    // ...
    stw     r19, TcxR19(r14)    // ...
    stw     r20, TcxR20(r14)    // ...
    stw     r21, TcxR21(r14)    // ...
    stw     r22, TcxR22(r14)    // ...
    stw     r23, TcxR23(r14)    // ...
    stw     r24, TcxR24(r14)    // ...
    stw     r25, TcxR25(r14)    // ...
    stw     r26, TcxR26(r14)    // ...
    stw     r27, TcxR27(r14)    // ...
    stw     r28, TcxR28(r14)    // ...
    stw     r29, TcxR29(r14)    // ...
    stw     r30, TcxR30(r14)    // ...
    stw     r31, TcxR31(r14)    // ...

LoadNewThread:

    mr      r14, r3             // (r14) = new Current Thread
    stw     r3, CurThdPtr(0)    // remember it for next interrupt

    lwz     r3, ThTlsPtr(r14)   // load the TLS pointer for the new thread
    stw     r3, lpvTls(0)       // set the KPage TLS Pointer

    lwz     r15, TcxR15(r14)    // reload non-volatile registers
    lwz     r16, TcxR16(r14)    // ...
    lwz     r17, TcxR17(r14)    // ...
    lwz     r18, TcxR18(r14)    // ...
    lwz     r19, TcxR19(r14)    // ...
    lwz     r20, TcxR20(r14)    // ...
    lwz     r21, TcxR21(r14)    // ...
    lwz     r22, TcxR22(r14)    // ...
    lwz     r23, TcxR23(r14)    // ...
    lwz     r24, TcxR24(r14)    // ...
    lwz     r25, TcxR25(r14)    // ...
    lwz     r26, TcxR26(r14)    // ...
    lwz     r27, TcxR27(r14)    // ...
    lwz     r28, TcxR28(r14)    // ...
    lwz     r29, TcxR29(r14)    // ...
    lwz     r30, TcxR30(r14)    // ...
    lwz     r31, TcxR31(r14)    // ...

ContinueCurThread:

    mfmsr   r0                  // Get current MSR
    MSR_CLR_EE(r0)              // Interrupts off
    MSR_CLR_RI(r0)              // Non-recoverable interrupts off
    mtmsr   r0                  // Update MSR
    isync
    lha     r3, ReschedFlag(0)
    li      r4, 0               // fake interrupt id for rescheduling
    cmpwi   r3, 1
    addi    r3, r3, 256
    beq-    Reschedule          // must reschedule
    sth     r3, ReschedFlag(0)

    //
    // Reload volatile register state.
    //

    lwz     r3, TcxLr(r14)
    lwz     r4, TcxCtr(r14)
    lwz     r5, TcxXer(r14)
    lwz     r6, TcxCr(r14)
    mtlr    r3                  // restore link register
    mtctr   r4                  // restore count register
    mtxer   r5                  // restore integer exception register
    mtcrf   0xff, r6            // restore condition register
    lwz     r3, TcxR3(r14)      // restore argument registers
    lwz     r4, TcxR4(r14)      // ...
    lwz     r5, TcxR5(r14)      // ...
    lwz     r6, TcxR6(r14)      // ...
    lwz     r7, TcxR7(r14)      // ...
    lwz     r8, TcxR8(r14)      // ...
    lwz     r9, TcxR9(r14)      // ...
    lwz     r10, TcxR10(r14)    // ...
    lwz     r0, TcxIar(r14)
    lwz     r11, TcxMsr(r14)
    lwz     r1, TcxR1(r14)
    lwz     r2, TcxR2(r14)
    mtspr   SRR0, r0            // set continuation address
    mtspr   SRR1, r11           // set continuation status register
    lwz     r0, TcxR0(r14)
    lwz     r11, TcxR11(r14)
    lwz     r12, TcxR12(r14)
    lwz     r13, TcxR13(r14)
    lwz     r14, TcxR14(r14)
    rfi

//
// END - KSEG Interrupt Handling Routines
//




    LEAF_ENTRY(INTERRUPTS_ON)
// INTERRUPTS_ON - enable external interrupts
//
//  Entry   none
//  Exit    none
//  Uses    r0
    mfmsr   r0
    ori     r0, r0, MSR_EE
    mtmsr   r0
    LEAF_EXIT(INTERRUPTS_ON)

    LEAF_ENTRY(INTERRUPTS_OFF)
// INTERRUPTS_OFF - disable external interrupts
//
//  Entry   none
//  Exit    none
//  Uses    r0
    mfmsr   r0
    MSR_CLR_EE(r0)
    mtmsr   r0
    LEAF_EXIT(INTERRUPTS_OFF)

PosTable:
    .byte 0,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 8,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
    .byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1

    LEAF_ENTRY(GetHighPos)

    lis     r4, [hia]PosTable
    addi    r4, r4, [lo]PosTable                // r4 == Ptr to PosTable

    subi    r5, r0, 1                           // r5 = offset to bit 0 of byte
    rlwinm  r6, r3, 0, 0xff                     // r6 = current byte of DWORD
    lbzx    r7, r4, r6                          // r7 = bit offset within byte
    cmpwi   r7, 0
    bne     res

    addi    r5, r5, 8                           // r5 == 7
    rlwinm  r6, r3, 24, 0xff
    lbzx    r7, r4, r6
    cmpwi   r7, 0
    bne     res

    addi    r5, r5, 8                           // r5 == 15
    rlwinm  r6, r3, 16, 0xff
    lbzx    r7, r4, r6
    cmpwi   r7, 0
    bne     res

    addi    r5, r5, 8                           // r5 == 23
    rlwinm  r6, r3, 8, 0xff
    lbzx    r7, r4, r6
    cmpwi   r7, 0
    bne     res
    li      r7, 9                               // set r7 so r5+r7=32  (r3==0)
res:
    add     r3, r5, r7
    blr
    LEAF_EXIT(GetHighPos)

    LEAF_ENTRY(ZeroPage)
//  void ZeroPage(void *vpPage)
//
//  Entry   (r3) = (vpPage) = ptr to address of page to zero
//  Return  none
//  Uses    r3, r4
//
//
    li      r4, PAGE_SIZE/32          // Number of loop iterations
    mtctr   r4                        //  stored to count register
    li      r4, 0

zp10:
    stw     r4, 0(r3)
    stw     r4, 4(r3)
    stw     r4, 8(r3)
    stw     r4, 12(r3)
    stw     r4, 16(r3)
    stw     r4, 20(r3)
    stw     r4, 24(r3)
    stw     r4, 28(r3)

    addi    r3, r3, 32                // Move pointer ahead
    bdnz+   zp10                      // Continue if not end of page

    LEAF_EXIT(ZeroPage)


    LEAF_ENTRY(InterlockedPopList)
// InterlockedPopList - remove first item from list
//
//  Entry   (r3) = ptr to list head
//  Return  (r3) = item at head of list
//      If (list not emtpy) next item is now head of list
//  Uses    r4, r5
    mr  r5, r3
ipop:
    lwarx   r4, 0, r5
    mr      r3, r4          // (r3) = ptr to item
    cmpwi   r4, 0
    beq-    ipopx
    lwz     r4, 0(r4)       // (r4) = ptr to next item
ipopx:
    stwcx.  r4, 0, r5       // update linkage & clear reservation
    bne-    ipop            // do it again, if stwcx. failed
    LEAF_EXIT(InterlockedPopList)

    LEAF_ENTRY(InterlockedPushList)
// InterlockedPushList - add an item to head of list
//
//  Entry   (r3) = ptr to list head
//          (r4) = ptr to item
//  Return  (r3) = old head of list
//  Uses    r0
ipsh:
    lwarx   r0, 0, r3       // (r0) = old list head
    stw     r0, 0(r4)       // store linkage
    stwcx.  r4, 0, r3       // store new list head
    bne-    ipsh            // do it again, if stwcx. failed
    mr      r3, r0
    LEAF_EXIT(InterlockedPushList)


    DUMMY_ENTRY(xKCall)
//++
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//--
    stw     r1,0(r1)        // save stack pointer
    stw     r31,4(r1)       // save linkage register
    mflr    r31             // save return address
    PROLOGUE_END(xKCall)

    ALTERNATE_ENTRY(KCall)
// KCall - call kernel function
//
// KCall invokes a kernel function in a non-preemtable state by incrementing
// the kernel nest level and switching onto a kernel stack.
//
// While in a preemtible state, the thread's register save area is
// volatile. On the way in, nothing can be saved into the thread
// structure until KNest is set and on the way out anything needed from the
// thread structure must be loaded before restoring KNest.
//
// The sequence of stack switching must be handled carefully because
// whenever KNest != 1, the general exception handler assumes that the kernel
// stack is current and will not switch stacks. On the way in, we must switch
// to the kernel stack before setting KNest but not use it until after KNest
// is set.  On the way out, we must reset KNest before restoring the thread's
// stack pointer.
//
//  Entry   (r3) = ptr to function to call
//          (r4) = first function arg
//          (r5) = second fucntion arg
//          (r6) = third function arg
//  Exit    (r3) = function return value
//  Uses    r0, r3-r12, ctr

    lha     r12, ReschedFlag(0)
    mtctr   r3                  // (r3) = ptr to function to call
    subic.  r12, r12, 256
    mr      r3, r4              // ripple args down
    mr      r4, r5
    mr      r5, r6
    bltctr-                     // nested kernel call, jump directly
                                //  to the function

// Entering non-preemptible state. We must switch onto the kernel stack
// before setting KNest in case an interrupt occurs during the switch.

    mr      r11, r1             // (r11) = original stack pointer
    li      r1, KStack          // switch to kernel stack
    stw     r12, ReschedFlag(0) // switch to non-preemtible state
    isync
    stw     r11, 0(r1)          // save original stack pointer
    stw     r31, 4(r1)          // save register for linkage
    mflr    r31
    bctrl                       // invoke non-preemptible function

// Function complete. Return to preemtible state after checking if a
//   reschedule is needed.

    lha     r10, ReschedFlag(0)
    cmpwi   r10, 1
    beq-    kc20

    lwz     r11, dwKCRes(0)
	cmpwi	r11, 1
	beq-	kc20

    mtlr    r31
    addi    r10, r10, 256
    lwz     r31, 4(r1)          // restore caller's R31
    lwz     r11, 0(r1)          // (r11) = caller's stack pointer
    sth     r10, ReschedFlag(0) // exit non-preemptible state
    mr      r1, r11             // restore the caller's stack pointer
    blr                         //  & return to caller

// ReschedFlag set, so must run the scheduler to find which thread
// to dispatch next.
//
//  (r3) = KCall return value
//  (r31) = return address

kc20:
    lwz     r11, CurThdPtr(0)   // (r11) = ptr to current thread
    mfmsr   r0                  // Current MSR is the one we want
    stw     r31, TcxIar(r11)    // thread resumes at return address
    stw     r0, TcxMsr(r11)     //   & in kernel mode.
    lwz     r0, 0(r1)           // (r0) = caller's R1 (stack pointer)
    stw     r2, TcxR2(r11)      // save Global pointer
    stw     r3, TcxR3(r11)      // save return value
    stw     r0, TcxR1(r11)      // save caller's stack pointer
    stw     r14, TcxR14(r11)
    lwz     r31, 4(r1)          // (r31) = caller's R31 value
    mr      r14, r11
    mfcr    r0
    stw     r0, TcxCr(r11)

    li      r4, ID_RESCHEDULE       // fake interrupt id for rescheduling

    ///stw     r4, TcxContextFlags(r11)// mark as partial context
    b      Reschedule

    DUMMY_EXIT(xKCall)


// CaptureContext is invoked in kernel context on the user thread's stack to
// build a context structure to be used for exception unwinding.
//
//  (r1) = aligned stack pointer

	SPECIAL_ENTRY(CaptureContext)


    stwu    r1, -ContextFrameLength-StackFrameHeaderLength(r1)
    stw     r0, CxGpr0+StackFrameHeaderLength(r1)
	// CxGpr1 is filled by ExceptionDispatch.  The unwinder
	// will recover that value from this (otherwise useless)
	// prolog instruction
	stw		r1, CxGpr1+StackFrameHeaderLength(r1)
    stw     r2, CxGpr2+StackFrameHeaderLength(r1)
    stw     r3, CxGpr3+StackFrameHeaderLength(r1)
    stw     r4, CxGpr4+StackFrameHeaderLength(r1)
    stw     r5, CxGpr5+StackFrameHeaderLength(r1)
    stw     r6, CxGpr6+StackFrameHeaderLength(r1)
    stw     r7, CxGpr7+StackFrameHeaderLength(r1)
    stw     r8, CxGpr8+StackFrameHeaderLength(r1)
    stw     r9, CxGpr9+StackFrameHeaderLength(r1)
    stw     r10, CxGpr10+StackFrameHeaderLength(r1)
    stw     r11, CxGpr11+StackFrameHeaderLength(r1)
    stw     r12, CxGpr12+StackFrameHeaderLength(r1)
    stw     r13, CxGpr13+StackFrameHeaderLength(r1)
    stw     r14, CxGpr14+StackFrameHeaderLength(r1)
    stw     r15, CxGpr15+StackFrameHeaderLength(r1)
    stw     r16, CxGpr16+StackFrameHeaderLength(r1)
    stw     r17, CxGpr17+StackFrameHeaderLength(r1)
    stw     r18, CxGpr18+StackFrameHeaderLength(r1)
    stw     r19, CxGpr19+StackFrameHeaderLength(r1)
    stw     r20, CxGpr20+StackFrameHeaderLength(r1)
    stw     r21, CxGpr21+StackFrameHeaderLength(r1)
    stw     r22, CxGpr22+StackFrameHeaderLength(r1)
    stw     r23, CxGpr23+StackFrameHeaderLength(r1)
    stw     r24, CxGpr24+StackFrameHeaderLength(r1)
    stw     r25, CxGpr25+StackFrameHeaderLength(r1)
    stw     r26, CxGpr26+StackFrameHeaderLength(r1)
    stw     r27, CxGpr27+StackFrameHeaderLength(r1)
    stw     r28, CxGpr28+StackFrameHeaderLength(r1)
    stw     r29, CxGpr29+StackFrameHeaderLength(r1)
    stw     r30, CxGpr30+StackFrameHeaderLength(r1)
    stw     r31, CxGpr31+StackFrameHeaderLength(r1)
    mfcr    r0
    stw     r0, CxCr+StackFrameHeaderLength(r1)
    mfxer   r0
    stw     r0, CxXer+StackFrameHeaderLength(r1)
    mfspr   r0, SRR1
    stw     r0, CxMsr+StackFrameHeaderLength(r1)


    // removed because CxIar is fixed up (and restored) by
    // ExceptionDispatch

    // mfspr    r0, SRR0
    // stw      r0, CxIar+StackFrameHeaderLength(r1)


    mflr    r0
    stw     r0, CxLr+StackFrameHeaderLength(r1)

    // ExceptionDispatch will fix up the CxIar value so that during a
    // nested RaiseException we are able to unwind correctly through
    // capture context. The unwinder only restores the LR the first
    // time it reaches a mflr instruction, the mflr above
    // will be ignored during unwind.
    //
    // JWM

    mflr    r0
    stw     r0, CxIar+StackFrameHeaderLength(r1)


    mfctr   r0
    stw     r0, CxCtr+StackFrameHeaderLength(r1)
    PROLOGUE_END(CaptureContext)
    li      r0, CONTEXT_CONTROL | CONTEXT_INTEGER
    stw     r0, CxContextFlags+StackFrameHeaderLength(r1)
    lwz     r2, NKGp(0)                     // (r2) = kernel's Global pointer
    addi    r3, r1, StackFrameHeaderLength  // (r3) = ptr to ContextRecord
    bl      ExceptionDispatch

// Reload possibly edited context from the ContextRecord.
    lwz     r0, CxGpr0+StackFrameHeaderLength(r1)
    lwz     r2, CxGpr2+StackFrameHeaderLength(r1)
    lwz     r5, CxGpr5+StackFrameHeaderLength(r1)
    lwz     r6, CxGpr6+StackFrameHeaderLength(r1)
    lwz     r7, CxGpr7+StackFrameHeaderLength(r1)
    lwz     r8, CxGpr8+StackFrameHeaderLength(r1)
    lwz     r9, CxGpr9+StackFrameHeaderLength(r1)
    lwz     r10, CxGpr10+StackFrameHeaderLength(r1)
    lwz     r11, CxGpr11+StackFrameHeaderLength(r1)
    lwz     r12, CxGpr12+StackFrameHeaderLength(r1)
    lwz     r13, CxGpr13+StackFrameHeaderLength(r1)
    lwz     r14, CxGpr14+StackFrameHeaderLength(r1)
    lwz     r15, CxGpr15+StackFrameHeaderLength(r1)
    lwz     r16, CxGpr16+StackFrameHeaderLength(r1)
    lwz     r17, CxGpr17+StackFrameHeaderLength(r1)
    lwz     r18, CxGpr18+StackFrameHeaderLength(r1)
    lwz     r19, CxGpr19+StackFrameHeaderLength(r1)
    lwz     r20, CxGpr20+StackFrameHeaderLength(r1)
    lwz     r21, CxGpr21+StackFrameHeaderLength(r1)
    lwz     r22, CxGpr22+StackFrameHeaderLength(r1)
    lwz     r23, CxGpr23+StackFrameHeaderLength(r1)
    lwz     r24, CxGpr24+StackFrameHeaderLength(r1)
    lwz     r25, CxGpr25+StackFrameHeaderLength(r1)
    lwz     r26, CxGpr26+StackFrameHeaderLength(r1)
    lwz     r27, CxGpr27+StackFrameHeaderLength(r1)
    lwz     r28, CxGpr28+StackFrameHeaderLength(r1)
    lwz     r29, CxGpr29+StackFrameHeaderLength(r1)
    lwz     r30, CxGpr30+StackFrameHeaderLength(r1)
    lwz     r31, CxGpr31+StackFrameHeaderLength(r1)

    lwz     r3, CxCr+StackFrameHeaderLength(r1)
    lwz     r4, CxXer+StackFrameHeaderLength(r1)
    mtcrf   0xff, r3
    mtxer   r4
    lwz     r3, CxLr+StackFrameHeaderLength(r1)
    lwz     r4, CxCtr+StackFrameHeaderLength(r1)
    mtlr    r3
    mtctr   r4

    mfmsr   r3
    MSR_CLR_EE(r3)
    mtmsr   r3                  // disable interrupts before mucking w/SRRx

    //
    //  "prime" the TB/Cache by prefetching r1, r3, and r4. No TB miss can
    //  be allowed to occur after SRR0 and SRR1 get updated:
    //

    lwz     r3, CxGpr3+StackFrameHeaderLength(r1)
    lwz     r4, CxGpr4+StackFrameHeaderLength(r1)
    lwz     r3, CxGpr1+StackFrameHeaderLength(r1)

    lwz     r3, CxMsr+StackFrameHeaderLength(r1)
    lwz     r4, CxIar+StackFrameHeaderLength(r1)
    mtspr   SRR1, r3
    mtspr   SRR0, r4

    //
    //  No exceptions (including TB misses) may occur before the rfi:
    //

    lwz     r3, CxGpr3+StackFrameHeaderLength(r1)
    lwz     r4, CxGpr4+StackFrameHeaderLength(r1)
    lwz     r1, CxGpr1+StackFrameHeaderLength(r1)
    rfi
    DUMMY_EXIT(CaptureContext)



//
// HwInterruptVector - Hardware interrupt vector
//
// This is the handler for incoming hw interrupts.  This code will be copied to
// the HW interrupt vector
//

HwInterruptHandler:

    mtspr   SPRG1, r14
    mtspr   SPRG2, r5

    lis     r5, [hia]OEMInterruptHandler
    addi    r5, r5, [lo]OEMInterruptHandler

    ba      KPageHwInterruptHandler


.set    HwInterruptHandlerLen, $ - HwInterruptHandler

//
// DecrementerHandler - Decrementer Interrupt Vector
//
// This is the handler for incoming decrmenter interrupts.  This code will be copied
// to the decrementer interrupt vector
//

DecrementerHandler:

    mtspr   SPRG1, r14
    mtspr   SPRG2, r5

    lis     r5, [hia]OEMDecrementer
    addi    r5, r5, [lo]OEMDecrementer

    ba      KPageHwInterruptHandler

.set    DecrementerHandlerLen, $ - DecrementerHandler



//
// GeneralHandler - general interrupt handling code
//
//  This code is copied into each 256 byte vector location in low
// memory. The "li r4,0" is adjusted to be (vector address)/256.
//

GeneralHandler:
    mtspr   SPRG1, r4
    mtspr   SPRG2, r5
    li      r4, 0
    mfspr   r5, DAR
    ba      KPageGeneralHandler
    .set GeneralHandlerLen, $ - GeneralHandler

    .globl  NullSection
NullSection0:
    .long   KPageBlock
NullSection:
    .space  (BLOCK_MASK+1)*4

//
//  Copy Table:
//  This table specifies the special interrupt handlers and KPage handler
//  addresses which need to be copied into low memory and the KPage. The
//  format of each entry is (Dest, Src, Length). All bytes from <Start> to
//  <End> are copied into the location <Dest>.  The Src Address is specified
//  as the offset from label KSRefPoint.
//

CopyTable:
    .long   ITBMissVector,  ITBMiss-KSRefPoint, 0x100
    .long   DTBMissVector,  DTBMiss-KSRefPoint, 0x100
    .long   ITBErrorVector, ITBError-KSRefPoint, 0x100
    .long   DTBErrorVector, DTBError-KSRefPoint, 0x100
    .long   HwInterruptVector, HwInterruptHandler-KSRefPoint, HwInterruptHandlerLen
    .long   DecrementerVector, DecrementerHandler-KSRefPoint, DecrementerHandlerLen
    .long   LowHandlers,    KPageHandlers-KSRefPoint
    .long       KPageHandlers.End - KPageHandlers
#if PPC403
    .long   ClockIntVector, ClockHandler-KSRefPoint
    .long       ClockHandlerEnd - ClockHandler
#endif
    .long   -1  // end of table

    .align  2

    LEAF_ENTRY(KernelStart)
//
// KernelStart - kernel initial startup code
//
// Branch here from the boot vector code after the minimum OEM h/w
// initialization has been completed. At this point:
//  CPU in PowerPC Little-Endian mode.
//  interrupts are disabled
//  relocation is disabled
//  the cache is disabled
//  RAM & ROM accesses are allowed and RAM refresh is working
//  No interrupt vectors have been installed.
//
// The kernel will initialize the interrupt handlers in low memory, setup the kernel
// data page, enable internal interrupts (TLB misses, etc), enable relocation,
// call OEMInit() to finish OEM h/w init, enable external interrupts, init the
// kernel memory system and scheduler and jump to the scheduler to run the initial
// thread.
//
//  Entry   (r3) = address of the a physical to virtual table (if applicable)
//
//  Exit    never returns
//


//
// Set up Reference pointer. Used for accesssing data within this module.
//

    bl KSRefPoint

KSRefPoint:
    mflr    r31

//
// Zero out the kernel data page.
//
// (r31) = Address of KSRefPoint
//

    li      r30, KPAGE_BASE - 4             // Get the start of the KPAGE-4
    li      r29, KData_size - KPAGE_BASE    // Calculate length of KPAGE
    srwi    r29, r29, 2                     // copying by words
    mtctr   r29                             // Load length into ctr
    li      r28, 0                          // zero r28
ZeroKPageLoop:
    stwu    r28, 4(r30)                     // Zero KPAGE
    bdnz    ZeroKPageLoop

//
// Copy default interrupt handlers to low memory.
//
// (r31) = Address of KSRefPoint
//

    addi    r30, r31, GeneralHandler - KSRefPoint // (r30) = ptr to GeneralHandler

    lwz     r29, 0x00(r30)  // (r3) = "mtspr SPRG1, r4"
    lwz     r28, 0x04(r30)  // (r4) = "mtspr SPRG2, r5"
    lwz     r27, 0x08(r30)  // (r5) = "li r4, 0"
    lwz     r26, 0x0c(r30)  // (r6) = "mfspr r5, DAR"
    lwz     r25, 0x10(r30)  // (r7) = "ba KPageGeneralHandler"
    li      r30, -0x100
ks16:
    stwu    r29, 0x100(r30)
    stw     r28, 4(r30)
    stw     r27, 8(r30)
    stw     r26, 12(r30)
    stw     r25, 16(r30)
    cmplwi  r30, 0x3000
    addi    r27, r27, 1     // (r27) = "li r4, N"
    blt+    ks16


//
// Copy special interrupt handlers and KPage code to low memory.
//
// (r31) = Address of KSRefPoint
//

    addi    r30, r31, CopyTable - KSRefPoint// r30 = ptr to CopyTable
    addi    r30, r30, -12                   // start at -12 from table start

InitLoop:
    lwzu    r29, 12(r30)                    // (r29) = destination address
    cmpwi   r29, -1
    beq-    CopyDone                        // done copying

    lwz     r28, 4(r30)                     // (r28) = src addr -KSRefPoint
    add     r28, r31, r28                   // (r28) = src address
    lwz     r27, 8(r30)                     // (r27) = length

    srwi    r27, r27, 2                     // Copy word chunks
    mtctr   r27                             // load length to CTR

    addi    r29, r29, -4                    // Back off dest and source
    addi    r28, r28, -4                    //  pointers by 1 word for copy

CopyLoop:
    lwzu    r26, 4(r28)                     // Perform copy with pointer
    stwu    r26, 4(r29)                     //  index updates
    bdnz    CopyLoop

    b       InitLoop                        // Next table entry

CopyDone:

//
// Store the HandleBase into the KPAGE
//

    lis     r30, 0x8000
    stw     r30, HandleBase(0)

//
// Fill in virtual memory array: All entries point to NullSection by default.
//

    lis     r30, [hia]NullSection0          // Get pointer to NullSection
    addi    r30, r30, [lo]NullSection0
    li      r29, SectionTable-4

    stwu    r30, 4(r29)                     // Section zero -> KPage Block
    addi    r30, r30, 4                     // Other sections -> NULL_BLOCK

    li      r28, 64-1                       // Initialize remaining sections
    mtctr   r28
ks14:
    stwu    r30, 4(r29)
    bdnz    ks14

//
// Initialize the TLB.
//
// r3 - address of the a physical to virtual table (if applicable)
//

    bl      TLBInit

//
// Enable relocation & internal interrupts.
//

    li      r1, KStack                // (r1) = kernel stack
    mfmsr   r3                        // Get current MSR
    ori     r3, r3, (MSR_IR | MSR_DR)

    mtspr   SRR1, r3

    li      r4, kMSR                  // Store default kernel MSR in KPage
    ori     r3, r3, (MSR_EE | MSR_ME) // Default kernel MSR has EE & ME on
    stw     r3, 0(r4)


    lis     r4, [hia]KernelContinue   // (virtual) continuation addr
    addi    r4, r4, [lo]KernelContinue
    mtspr   SRR0, r4

    rfi                               // switch modes

//
// Relocate kernel data from ROM to RAM.
//
//  (r4) = virtual address of KernelContinue

KernelContinue:

#if  0      // SAVEGP
    lis     r2, [hia]_gp            // set the GP value in R2
    addi    r2, r2, [lo]_gp
#else
    li      r2, 0
#endif

    stw     r2, NKGp(0)             // save virtual address of Global Pointer

    lis     r3, [hia]pTOC           // (r3) = ptr to ROM Table of contents
    lwz     r3, [lo]pTOC(r3)

    bl      KernelRelocate
    bl      OEMInitDebugSerial

    lis     r3, [hia]NKSignon
    addi    r3, r3, [lo]NKSignon
    bl      OEMWriteDebugString

    li      r3, APICallReturn
    stw     r3, PtrAPIRet(0)
    li      r3, -1
    stw     r3, CurAKey(0)

    bl      OEMInit
    bl      KernelFindMemory
    bl      KernelInit
    bl      CPUIdentify             //  Returns with (r3) = processor type
    lis     r4, [hia]CEProcessorType
    stw     r3, [lo]CEProcessorType(r4)

    mfspr   r3, PVR




    rlwinm  r5, r3, 16, 0xFFFF      // ProcessorLevel = PVR[0:15]
    lis     r4, [hia]ProcessorLevel
    sth     r5, [lo]ProcessorLevel(r4)

    rlwinm  r5, r3, 0, 0xFFFF       // ProcessorLevel = PVR[16:31]
    lis     r4, [hia]ProcessorRevision
    sth     r5, [lo]ProcessorRevision(r4)

    li      r14, 0                  // no current thread
    li      r4, ID_RESCHEDULE       // fake interrupt id for rescheduling
    b       Reschedule

    DUMMY_EXIT(KernelStart)

//
//  Exception Handling support routines:
//

                .struct 0
CfBackChain:    .space  4           // chain to previous call frame
                .space  4           // Unused (align args at eight bytes)
                .space  8*4         // 8 words space for call args
CfDispContext:  .space  4           // space to save the incoming
                                    // Dispatcher Context ptr
CfCstk:			.space  CstkSizeof	// CALLSTACK struct (if needed) for mode switch.

CfRegs:			.space	19 * 4		// Save permanent registers around funclet calls.

                .align  3           // force frame length to multiple of
CfEnd:                              //   eight bytes

#define CfFrameSize	(CfEnd - CfDispContext)

#define SAVE_REGS(_base, reg)\
		stw		r13, _base+0(reg);\
		stw		r14, _base+4(reg);\
		stw		r15, _base+8(reg);\
		stw		r16, _base+12(reg);\
		stw		r17, _base+16(reg);\
		stw		r18, _base+20(reg);\
		stw		r19, _base+24(reg);\
		stw		r20, _base+28(reg);\
		stw		r21, _base+32(reg);\
		stw		r22, _base+36(reg);\
		stw		r23, _base+40(reg);\
		stw		r24, _base+44(reg);\
		stw		r25, _base+48(reg);\
		stw		r26, _base+52(reg);\
		stw		r27, _base+56(reg);\
		stw		r28, _base+60(reg);\
		stw		r29, _base+64(reg);\
		stw		r30, _base+68(reg);\
		stw		r31, _base+72(reg);

#define RESTORE_REGS(_base, reg)\
		lwz		r13, _base+0(reg);\
		lwz		r14, _base+4(reg);\
		lwz		r15, _base+8(reg);\
		lwz		r16, _base+12(reg);\
		lwz		r17, _base+16(reg);\
		lwz		r18, _base+20(reg);\
		lwz		r19, _base+24(reg);\
		lwz		r20, _base+28(reg);\
		lwz		r21, _base+32(reg);\
		lwz		r22, _base+36(reg);\
		lwz		r23, _base+40(reg);\
		lwz		r24, _base+44(reg);\
		lwz		r25, _base+48(reg);\
		lwz		r26, _base+52(reg);\
		lwz		r27, _base+56(reg);\
		lwz		r28, _base+60(reg);\
		lwz		r29, _base+64(reg);\
		lwz		r30, _base+68(reg);\
		lwz		r31, _base+72(reg);

//++
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//    This function allocates a call frame, stores the establisher frame
//    pointer in the frame, establishes an exception handler, and then calls
//    the specified exception handler as an exception handler. If a nested
//    exception occurs, then the exception handler of this function is called
//    and the establisher frame pointer is returned to the exception dispatcher
//    via the dispatcher context parameter. If control is returned to this
//    routine, then the frame is deallocated and the disposition status is
//    returned to the exception dispatcher.
//
// Arguments:
//    ExceptionRecord (r3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r4) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (r5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r6) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (r7) - supplies a pointer to the function descriptor
//       for the exception handler that is to be called.
//
//    ExceptionMode (r8) - PSR value for running ExceptionRoutine
//
// Return Value:
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//--
        NESTED_ENTRY_EX (RtlpExecuteHandlerForException,CfFrameSize,0,0,RtlpExceptionHandler,0)
		SAVE_REGS(CfRegs, r1)
        PROLOGUE_END (RtlpExecuteHandlerForException)

        mtctr   r7                      // CTR <- entry point addr
        stw     r6, CfDispContext(r1)   // save Dispatcher Context where
                                        //   RtlpExectionHandler can find it
        andi.   r0, r8, (1 << (31-MSR_BIT_PR))
        bnela   KPageJumpSwitch         // mode switch & call handler

        bctrl                           // call the exception handler

		RESTORE_REGS(CfRegs, r1)
        NESTED_EXIT (RtlpExecuteHandlerForException,CfFrameSize,0,0)

//++
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer from its establisher's
//    call frame, store this information in the dispatcher context record,
//    and return a disposition value of nested exception.
//
// Arguments:
//    ExceptionRecord (r3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r4) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (r5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r6) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//--
        LEAF_ENTRY (RtlpExceptionHandler)
        lwz     r0, ErExceptionFlags (r3)     // get exception flags
        li      r3, ExceptionContinueSearch   // set usual disposition value
        andi.   r0, r0, EXCEPTION_UNWIND      // check if unwind in progress
        bnelr                           // if neq, unwind in progress
                                        // return continue search disposition

//
// Unwind is not in progress - return nested exception disposition.
//

        lwz     r7,CfDispContext-CfEnd(r4)    // get dispatcher context address
        li      r3, ExceptionNestedException  // set disposition value
        lwz     r0, DcEstablisherFrame(r7)    // copy the establisher environment
        stw     r0, DcEstablisherFrame (r6)   //   to current dispatcher context
        LEAF_EXIT (RtlpExceptionHandler)

//++
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//    This function allocates a call frame, stores the establisher frame
//    pointer and the context record address in the frame, establishes an
//    exception handler, and then calls the specified exception handler as
//    an unwind handler. If a collided unwind occurs, then the exception
//    handler of of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//    ExceptionRecord (r3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r4) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (r5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r6) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (r7) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//--
        NESTED_ENTRY_EX (RtlpExecuteHandlerForUnwind,CfFrameSize,0,0,RtlpUnwindHandler,0)
		SAVE_REGS(CfRegs, r1)
        PROLOGUE_END (RtlpExecuteHandlerForUnwind)

        lwz r8, CxMsr(r5)               // (r8) = mode to dispatch function in
        mtctr   r7                      // CTR <- entry point addr
        stw     r6, CfDispContext(r1)   // save Dispatcher Context where
                                        //   RtlpExectionHandler can find it
        andi.   r0, r8, (1 << (31-MSR_BIT_PR))
        bnela   KPageJumpSwitch         // mode switch & call handler

        bctrl                           // call the exception handler

		RESTORE_REGS(CfRegs, r1)
        NESTED_EXIT (RtlpExecuteHandlerForUnwind,CfFrameSize,0,0)

//++
// EXCEPTION_DISPOSITION
// RtlpUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//    ExceptionRecord (r3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r4) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (r5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r6) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//--
        LEAF_ENTRY (RtlpUnwindHandler)
        lwz     r0, ErExceptionFlags(r3)    // get exception flags
        li      r3, ExceptionContinueSearch // set usual disposition value
        andi.   r0, r0, EXCEPTION_UNWIND    // check if unwind in progress
        beqlr                                   // if eq, unwind not in progress
                                                // return continue search disposition
//
// Unwind is in progress - return collided exception disposition.
//
        lwz     r7, CfDispContext-CfEnd(r4)  // get dispatcher context address
        lwz     r0, DcControlPc(r7)          // copy the establisher frame's
        lwz     r3, DcFunctionEntry(r7)      //   dispatcher context to the
        lwz     r4, DcEstablisherFrame(r7)   //   current dispatcher context
        lwz     r5, DcContextRecord(r7)
        stw     r0, DcControlPc(r6)
        stw     r3, DcFunctionEntry(r6)
        stw     r4, DcEstablisherFrame(r6)
        stw     r5, DcContextRecord(r6)
        li      r3, ExceptionCollidedUnwind // return collided unwind disposition
        LEAF_EXIT (RtlpUnwindHandler)


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




