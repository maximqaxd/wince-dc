//      TITLE("Interrupt and Exception Processing")
//++
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    except.s
//
// Abstract:
//
//    This module implements the code necessary to field and process MIPS
//    interrupt and exception conditions.
//
//    WARNING: This module executes in KSEG0 and, in general, cannot
//       tolerate a TLB Miss. Registers k0 and k1 are used during initial
//	 interrupt and exception processing, and therefore, extreme care must
//	 be exercised when modifying this module.
//
//--

#include "ksmips.h"
#include "nkintr.h"
#include "kpage.h"
#include "mem_mips.h"
#include "psyscall.h"
#include "xtime.h"

	.text

	.globl	PtrCurMSec
PtrCurMSec: .word AddrCurMSec

PosTable:
  	.byte 0,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 8,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
  	.byte 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1

LEAF_ENTRY (GetHighPos)
	.set noreorder

	addi t0, zero, -1
	andi t1, a0, 0xff
	lb	v0, PosTable(t1)
	bne zero, v0, res
	srl a0, a0, 8

	addi t0, t0, 8
	andi t1, a0, 0xff
	lb	v0, PosTable(t1)
	bne zero, v0, res
	srl a0, a0, 8

	addi t0, t0, 8
	andi t1, a0, 0xff
	lb	v0, PosTable(t1)
	bne zero, v0, res
	srl a0, a0, 8

	addi t0, t0, 8
	andi t1, a0, 0xff
	lb	v0, PosTable(t1)
	bne zero, v0, res
	nop

	addi v0, v0, 9
res:
	j ra
	add v0, v0, t0
	.end GetHighPos

LEAF_ENTRY(INTERRUPTS_ON)
	.set noreorder
	lw	t0, BasePSR
	nop
	or	t0, 1
	j	ra
	mtc0	t0, psr
	.end INTERRUPTS_ON

LEAF_ENTRY(INTERRUPTS_OFF)
	.set noreorder
	mtc0	zero, psr
	nop			// 3 cycle delay
	j	ra
	nop
	.end INTERRUPTS_OFF

LEAF_ENTRY(SetCPUHardwareWatch)
	.set noreorder
	lui	t0, 0x7fff
	ori t0, t0, 0xfff8
	and a0, a0, t0
	or  a0, a0, a1
	mtc0 a0, $18
	nop
	nop
	mtc0 zero, $19
	nop
	nop
	j ra
	nop
	.end SetCPUHardwareWatch

LEAF_ENTRY(SetCPUASID)
	.set noreorder

#ifdef CELOG
        //
        // NOTE : To make things relatively consistent, I'm going to make
        // the registers v0, t0-t9, a0, a1 available for use. On entry to
        // CeLogThreadMigrateMIPS, a0 will contain the handle of the new process.
        //
        // I'm assuming that at this point the only registers of these that
        // I need to preserve are A0 and RA.
        //

        lw	t0, ThProc(a0)		// (t0) = ptr to new process
	lw	t1, PrcHandle(t0)	// (t1) = handle of new process
	lw	t0, hCurProc		// (t0) = current process handle
	beq	t0, t1, celog10		// if procs are equal then skip CeLog call
        nop

	subu	sp, 8                   // adjust sp
        sw      ra, 0(sp)               // save RA
        sw      a0, 4(sp)               // save arg

        move    a0, t1                  // (a0) = handle of new process

        jal     CELOG_ThreadMigrateMIPS
        nop

        lw      ra, 0(sp)               // restore RA
        lw      a0, 4(sp)               // restore arg
        addu	sp, 8           	// adjust sp

celog10:
#endif

        lw	t0, ThProc(a0)		// (t0) = ptr to current process
	lw	t1, ThAKey(a0)		// (t1) = thread's access key
	lw	t2, PrcHandle(t0)	// (t2) = handle of current process
	sw	t0, CurPrcPtr		// remember current process pointer
	sw	t2, hCurProc		// remember current process handle
	lw	t2, PrcVMBase(t0)	// (t2) = memory section base address
	lb	t0, PrcID(t0)		// (t0) = process ID
	srl	t2, VA_SECTION-2	// (t2) = index into section table
	lw	t2, SectionTable(t2)	// (t2) = process's memory section
	sw	t1, CurAKey		// save access key for TLB handler
#if ENTRYHI_PID != 0
	sll	t0, ENTRYHI_PID
#endif
	mtc0	t0, entryhi		// set ASID
	j	ra
	sw	t2, SectionTable(zero)	// swap in default process slot
	.end SetCPUASID

NESTED_ENTRY(xKCall, 0, zero)
//++
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//--
        .set    noreorder
        .set    noat
        sw      sp,20(sp)         // save stack pointer
        sw      ra,16(sp)         // save return address
        PROLOGUE_END

ALTERNATE_ENTRY(KCall)
// KCall - call kernel function
//
//	KCall invokes a kernel function in a non-preemtable state by incrementing
// the kernel nest level and switching onto a kernel stack.
//
//	While in a preemtible state, the thread's register save area is
// volatile. On the way in, nothing can be saved into the thread
// structure until KNest is set and on the way out anything needed from the
// thread structure must be loaded before restoring KNest.
//
//	The sequence of stack switching must be handled carefully because
// whenever KNest != 1, the general exception handler assumes that the kernel
// stack is current and will not switch stacks. On the way in, we must switch
// to the kernel stack before setting KNest but not use it until after KNest
// is set.  On the way out, we must reset KNest before restoring the thread's
// stack pointer.
//
//	Entry	(a0) = ptr to function to call
//		(a1) = first function arg
//		(a2) = second fucntion arg
//		(a3) = third function arg
//	Exit	(v0) = function return value
//	Uses	v0, a0-a3, t0-t9

	.set    noreorder
	move	t0, a0			// (t0) = ptr to function to call
	lh	t1, ReschedFlag		// (t1) = resched flag + nest level
	move	a0, a1			// ripple args down
	move	a1, a2
	subu	t1, 256
	bltz	t1, 50f			// nested kernel call
	move	a2, a3

// Entering non-preemptible state. We must switch onto the kernel stack
// before setting KNest in case an interrupt occurs during the switch.

	move	t2, sp			// (t2) = original stack pointer
	la	sp, KStack		// switch to kernel stack
	sh	t1, ReschedFlag		// enter non-preemtible state
	sw	ra, 16(sp)		// thread will resume at return address
	jal	t0
	sw	t2, 20(sp)		// save thread's stack pointer

// Function complete. Return to preemtible state after checking if a reschedule
// is needed.
	mtc0	zero, psr
	nop
	nop
	nop

	lh	t9, ReschedFlag		// (t9) = resched flag + nest level
	li	t1, 1
	beq	t9, t1, 20f		// reschedule needed
	lw	ra, 16(sp)		// (ra) = return address

	lw	t2, dwKCRes
	bne t2, zero, 20f	// reschedule needed
	nop

	lw	t2, 20(sp)		// (t2) = original stack pointer
	addu	t9, 256
	sh	t9, ReschedFlag		// leave non-preemtible state

	lw	t9, BasePSR
	or t9, 1
#ifdef MIPS_HAS_FPU
	lw t8, g_CurFPUOwner
	lw t7, CurThdPtr
	bne t7, t8, 19f
	lui t8, 0x2000
	or t9, t8
19:
#endif
	mtc0	t9, psr

	j	ra
	move	sp, t2			// restore stack pointer

// ReschedFlag set, so must run the scheduler to find which thread
// to dispatch next.
//
//	(v0) = KCall return value

20:
	lw	t9, BasePSR
	or t9, 1
	mtc0	t9, psr

	lw	t0, CurThdPtr		// (t0) = ptr to current thread
	lw	t2, 20(sp)		// (t2) = original stack pointer
	li	t3, KERNEL_MODE
	sw	ra, TcxFir(t0)		// thread will resume at return address
	sw	t3, TcxPsr(t0)		//  & in kernel mode
	sw	t2, TcxIntSp(t0)	// save stack pointer
	sw	v0, TcxIntV0(t0)	// save return value
	sw	s0, TcxIntS0(t0)	// save thread's permanent registers
	sw	s1, TcxIntS1(t0)	// ...
	sw	s2, TcxIntS2(t0)	// ...
	sw	s3, TcxIntS3(t0)	// ...
	sw	s4, TcxIntS4(t0)	// ...
	sw	s5, TcxIntS5(t0)	// ...
	sw	s6, TcxIntS6(t0)	// ...
	sw	s7, TcxIntS7(t0)	// ...
	sw	s8, TcxIntS8(t0)	// ...
	sw	zero, TcxContextFlags(t0)
	sw	gp, TcxIntGp(t0)

	j	resched
	move	s0, t0			// (s0) = ptr to current thread struct

25:	j	resched
	move	s0, zero		// no current thread

// Nested KCall. Just invoke the function directly.
//
//	(t0) = function address
//	(a0) = 1st function argument
//	(a1) = 2nd function argument
//	(a2) = 3rd function argument

50:	j	t0
	nop

	.set reorder
	.end xKCall

// Interlocked singly-linked list functions.
//
//	These functions are used internally by the kernel. They are not exported
// because they require privileged instructions on some CPUs.

// InterlockedPopList - remove first item from list
//
//	Entry	(a0) = ptr to list head
//	Return	(v0) = item at head of list
//		If (list not emtpy) next item is now head of list
//	Usea	t0, t1

LEAF_ENTRY(InterlockedPopList)
#if NO_LL
	la	t0, IPopRestart
	.set    noreorder
IPopRestart:
	move	k1, t0			// interlocked operation in progress
	lw	v0, (a0)		// (v0) = original contents
	beq	v0, zero, 10f
	nop
	lw	t1, (v0)		// (t1) = next item on the list
	sw	t1, (a0)		// update head of list
10:	j	ra
	move	k1, zero		// interlocked operation complete
#else
 #error Unimplemented.
#endif
	.end   InterlockedPopList

// InterlockedPushList - add an item to head of list
//
//	Entry	(a0) = ptr to list head
//		(a1) = ptr to item
//	Return	(v0) = old head of list
//	Uses	t0, v0

LEAF_ENTRY(InterlockedPushList)
#if NO_LL
	la	t0, IPushRestart
	.set    noreorder
IPushRestart:
	move	k1, t0			// interlocked operation in progress
	lw	v0, (a0)		// (v0) = original list head
	sw	v0, (a1)		// store linkage
	sw	a1, (a0)		// store new list head
	j	ra
	move	k1, zero		// interlocked operation complete
#else
 #error Unimplemented.
#endif
	.end   InterlockedPushList

#ifdef MIPS_HAS_FPU

LEAF_ENTRY(GetAndClearFloatCode)
	mfc0 a1, psr
	lui a2, 0x2000
	or a1, a2
	mtc0 a1, psr
	nop
	nop
	nop
	cfc1 v0, fsr

	li 	t0, 0xfffc0fff
	and	t0, t0, v0

	ctc1 t0, fsr
	j ra
	nop
	.end GetCode

LEAF_ENTRY(SaveFloatContext)
	mfc0 t0, psr
	lui t1, 0x2000
	or t0, t1
	mtc0 t0, psr
	nop
	nop
	nop
	cfc1 t0, fsr
	sw t0, TcxFsr(a0)
	swc1 f0, TcxFltF0(a0)
	swc1 f1, TcxFltF1(a0)
	swc1 f2, TcxFltF2(a0)
	swc1 f3, TcxFltF3(a0)
	swc1 f4, TcxFltF4(a0)
	swc1 f5, TcxFltF5(a0)
	swc1 f6, TcxFltF6(a0)
	swc1 f7, TcxFltF7(a0)
	swc1 f8, TcxFltF8(a0)
	swc1 f9, TcxFltF9(a0)
	swc1 f10, TcxFltF10(a0)
	swc1 f11, TcxFltF11(a0)
	swc1 f12, TcxFltF12(a0)
	swc1 f13, TcxFltF13(a0)
	swc1 f14, TcxFltF14(a0)
	swc1 f15, TcxFltF15(a0)
	swc1 f16, TcxFltF16(a0)
	swc1 f17, TcxFltF17(a0)
	swc1 f18, TcxFltF18(a0)
	swc1 f19, TcxFltF19(a0)
	swc1 f20, TcxFltF20(a0)
	swc1 f21, TcxFltF21(a0)
	swc1 f22, TcxFltF22(a0)
	swc1 f23, TcxFltF23(a0)
	swc1 f24, TcxFltF24(a0)
	swc1 f25, TcxFltF25(a0)
	swc1 f26, TcxFltF26(a0)
	swc1 f27, TcxFltF27(a0)
	swc1 f28, TcxFltF28(a0)
	swc1 f29, TcxFltF29(a0)
	swc1 f30, TcxFltF30(a0)
	swc1 f31, TcxFltF31(a0)
	j ra
	nop
	.end SaveFloatContext

LEAF_ENTRY(RestoreFloatContext)
	mfc0 t0, psr
	lui t1, 0x2000
	or t0, t1
	mtc0 t0, psr
	nop
	nop
	nop
	lw t0, TcxFsr(a0)
	ctc1 t0, fsr
	lwc1 f0, TcxFltF0(a0)
	lwc1 f1, TcxFltF1(a0)
	lwc1 f2, TcxFltF2(a0)
	lwc1 f3, TcxFltF3(a0)
	lwc1 f4, TcxFltF4(a0)
	lwc1 f5, TcxFltF5(a0)
	lwc1 f6, TcxFltF6(a0)
	lwc1 f7, TcxFltF7(a0)
	lwc1 f8, TcxFltF8(a0)
	lwc1 f9, TcxFltF9(a0)
	lwc1 f10, TcxFltF10(a0)
	lwc1 f11, TcxFltF11(a0)
	lwc1 f12, TcxFltF12(a0)
	lwc1 f13, TcxFltF13(a0)
	lwc1 f14, TcxFltF14(a0)
	lwc1 f15, TcxFltF15(a0)
	lwc1 f16, TcxFltF16(a0)
	lwc1 f17, TcxFltF17(a0)
	lwc1 f18, TcxFltF18(a0)
	lwc1 f19, TcxFltF19(a0)
	lwc1 f20, TcxFltF20(a0)
	lwc1 f21, TcxFltF21(a0)
	lwc1 f22, TcxFltF22(a0)
	lwc1 f23, TcxFltF23(a0)
	lwc1 f24, TcxFltF24(a0)
	lwc1 f25, TcxFltF25(a0)
	lwc1 f26, TcxFltF26(a0)
	lwc1 f27, TcxFltF27(a0)
	lwc1 f28, TcxFltF28(a0)
	lwc1 f29, TcxFltF29(a0)
	lwc1 f30, TcxFltF30(a0)
	lwc1 f31, TcxFltF31(a0)
	j ra
	nop
	.end RestoreFloatContext
#endif

LEAF_ENTRY(FirstSchedule)
	.set reorder
	la	s0, KData
	j	resched
	.end	FirstSchedule

NESTED_ENTRY(GeneralExceptionP, 0, zero)
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
//--
        .set    noreorder
        .set    noat
        sw      sp,TcxIntSp(sp)         // save stack pointer
        sw      ra,TcxIntRa(sp)         // save return address
        sw      ra,TcxFir(sp)           // save return address
        sw      gp,TcxIntGp(sp)		// save integer register gp
        sw	s0,TcxIntS0(sp)		// save S0
        move	s0, sp			// set pointer to thread
        PROLOGUE_END

ALTERNATE_ENTRY(GeneralException)
	.set    noreorder
	.set    noat


	mfc0	k0, epc			// (k0) = resume address
#if NO_LL
	bne	zero, k1, 30f		// interlocked operation restart
	nop
#endif

1:	ori	k1, k0, 0xFFFC		// (k1) = 0xFFFFFFFE iff an API call
	addu	k1, 2			// (k1) = 0 iff an API call
	beq	zero, k1, 200f		// go process an API call or API return
	mfc0	k1, cause		// (k1) = exception cause
	sw	t0, SaveT0		// get a working register
#if R4100
	andi t0, k1, 0xff		// ignore exception b's - We're getting extraneous exception B's
	xori t0, t0, 0x2c		// ignore exception b's - ignore them
	beq t0, zero, 104f		// ignore exception b's - 
#endif

// The read and write miss codes differ by exactly one bit such that they
// can be tested for by a single mask operation followed by a test for the
// read miss code.
    and     t0,k1,MISS_MASK   	// isolate exception code
    xori    t0,XCODE_READ_MISS      // get exception code for read miss
    beq     t0,zero,100f		// read or write TLB miss
5:	lb	t0, KNest		// (t0) = reentrancy count
	subu	t0, 1
	bne	zero, t0, 35f		// nested exception
	sb	t0, KNest
	lw	t0, CurThdPtr		// (t0) = ptr to current thread
	sw	sp, TcxIntSp(t0)
	sw	gp, TcxIntGp(t0)
	la	sp, KStack

// Reenter here from nested exception setup.

10:	sw	k0, TcxFir(t0)
	sw	s0, TcxIntS0(t0)
	move	s0, t0			// (s0) = ptr to thread structure
	sw	v0, TcxIntV0(s0)
	sw	a0, TcxIntA0(s0)
	and	t0, k1, XCODE_MASK
	bne	t0, zero, 40f		// not a h/w interrupt
	lw	t0, SaveT0		// restore T0
	lw	a0, BasePSR		// (a0) = interrupt mask
	sw	a1, TcxIntA1(s0)
	sw	AT, TcxIntAt(s0)
	.set at
	and	a0, k1			// (a0) = bits 15-10 pending interrupts
	srl	a0, CAUSE_INTPEND+2
	and	a0, 0x3F		// (a0) = hw interrupt cause bits
	lb	a0, IntrPriority(a0)	// (a0) = highest pri interrupt * 4
	sw	a2, TcxIntA2(s0)
	move	k1, zero		// reset atomic op. flag
	lw	v0, ISRTable(a0)	// (v0) = interrupt handler
	sw	ra, TcxIntRa(s0)	// save return address

	mfc0	k0, psr
	sw	k0, TcxPsr(s0)

	sra	a0, 2                   // Keep the sign for spurious interrupts
	addi a0, 1
	lbu	a0, IntrMask(a0)
	sll	a0, 10

	not		a0
	and		k0, k0, a0
	mtc0	k0, psr

#ifdef CELOG
        //
        // NOTE : To make things relatively consistent, I'm going to make
        // the registers v0, AT, a0-a3 available for use. On entry to CeLogInterruptMIPS
        // A0 will contain the value to be logged (cNest + SYSINTR value)
        //
        // I'm assuming that at this point the only register of these that
        // I need to preserve is V0 (ISR pointer)
        //
        move    k0, v0                  // save v0

        jal     CeLogInterruptMIPS
        li      a0, 0x80000000          // delay slot (a0 = mark as ISR entry)

        move    v0, k0                  // restore v0

	mfc0	k0, psr
#endif

#if R4000
	// clear User mode, EXL & ERL bits in PSR
	ori	k0, (1<<PSR_EXL) | (1<<PSR_ERL) | (1<<PSR_PMODE)
	xori	k0, (1<<PSR_EXL) | (1<<PSR_ERL) | (1<<PSR_PMODE)
#elif R3000
	ori	k0, 1 << PSR_IEC	// set int enable
#else
  #error Unknown processor type
#endif
	mtc0	k0, psr			// enable interrupts

	jal	v0
	sw	a3, TcxIntA3(s0)

#if R4000
	mfc0	a0, psr
	ori	a0, (1<<PSR_IE)
	xori	a0, (1<<PSR_IE)
	mtc0	a0, psr		// clear interrupt enable
#endif	
	lw		a0, TcxPsr(s0)
	nop
	mtc0	a0, psr		// now restore real psr, also toggling interrupt enable

	.set noat

#if SYSINTR_NOP != 0
 #error
#endif

#ifdef CELOG
        //
        // NOTE : To make things relatively consistent, I'm going to make
        // the registers v0, AT, a0-a3 available for use. On entry to CeLogInterruptMIPS
        // A0 will contain the value to be logged (cNest + SYSINTR value)
        //
        // I'm assuming that at this point the only register of these that
        // I need to preserve is V0 (SYSINTR value)
        //
        move    k0, v0                  // save V0
        lb      a0, KNest               // (a0) = nest level (0, -1, -2, etc)
        sub     a0, zero, a0            // (a0) = nest level (0,  1,  2, etc)
        sll     a0, a0, 16              // (a0) = (a0) << 16

        jal     CeLogInterruptMIPS
        or      a0, a0, v0              // delay slot (a0 = (-KNest << 16) | SYSINTR)

        move    v0, k0                  // restore V0
#endif
	beq	v0, zero, 20f		// return == SYSINTR_NOP
	li	a3, SYSINTR_BREAK
	beq	v0, a3, 25f		// debug break interrupt
	subu	v0, SYSINTR_DEVICES
	bltz	v0, 15f			// not device signal, treat as reschedule
	sltu	a3, v0, SYSINTR_MAX_DEVICES
	beq	a3, zero, 20f		// out of range, ignore
	lw	a1, PendEvents

// The ISR has requested that an interrupt event be signalled

	li	a2, 1
	sll	a3, a2, v0
	or a1, a1, a3
	sw	a1, PendEvents

15:	lb	v0, ReschedFlag
	or	v0, 1
	sb	v0, ReschedFlag		// set reschedule flag

// Interrupt processing is complete. If the reschedule flag has been set and
// this is not a nested exception, then save the full thread context and
// obtain a new thread to run.

20:	lh	k0, ReschedFlag		// (k0) = resched + nested exception
	li	a0, 1
	beq	k0, a0, 41f		// reschedule needed
	move	k1, zero		// (k1) = 0 (no atomic op in progress)
	addu	k0, 256			// remove one level of nesting
	sh	k0, ReschedFlag		// save updated nest info
	lw	AT, TcxIntAt(s0)
	lw	ra, TcxIntRa(s0)
	lw	a0, TcxIntA0(s0)		
	lw	a1, TcxIntA1(s0)
	lw	a2, TcxIntA2(s0)
	lw	a3, TcxIntA3(s0)
	lw	k0, TcxFir(s0)		// (k0) = exception return address
	lw	v0, TcxIntV0(s0)	// restore return value
#if R4000
	mtc0	k0, epc			// restore EPC to interrupted stream
#endif
	lw	sp, TcxIntSp(s0)	// restore stack pointer
	lw	gp, TcxIntGp(s0)
	lw	s0, TcxIntS0(s0)
#if R3000
	j	k0			// return to original caller
	rfe				// restore user status
#elif R4000
	eret				// restore user status
	nop
	nop
	eret
#else
  #error Unknown processor type
#endif

// HW Break button pushed. Pass an exception to the debugger.

25:	li	k1, 1			// (k1) = cause (1 for h/w break)
	j	41f			// route into vanilla exception path
	move	a2, zero		// clear BadVAddr for exception handler


// An atomic instruction sequence was interrupted. Check if we are past the
// store instruction and reset the EPC back to the starting point if not.
// Note that since we know what the interrupted stream is it is safe to
// trash certain registers.
//
// this code needs to be more careful about addressing user memory!
//
//	(k1) = start of sequence
//	(k0) = exception PC
//	(t0) = also start of sequence

#if NO_LL
30:	bne	t0, k1, 32f		// go check for possible nested fault
	nop
	li	k1, -1			// (k1) = -1 (in case fault loading instruction)
	lw	t6, (k0)		// (t6) = interrupted instruction
	lui	t5, 0x03E0
	ori	t5, 0x0008		// (t5) = "J RA" INSTRUCTION
	beq	t6, t5, 1b		// on the return instruction
	nop				//   so no restart needed
	b	1b
	move	k0, t0			// (k0) = restart address

// False interlocked trigger. If k1==-1, then the above interlocked api check
// generated a fault and we will just report the fault at the restart address.
//
//	(t0) = interlocked api restart address.

32:	addu	k1, 1
	bne	k1, zero, 1b		// not in NK's interlock code
	nop
	b	1b
	move	k0, t0			// report exception at restart address
#endif

// A nested exception has occured. If this a h/w interrupt, create a temporary
// thread structure on the stack and save the current state into that.

35:	subu	t0, sp, TcxSizeof	// (t0) = ptr to dummy kernel thread
	sw	sp, TcxIntSp(t0)	// save current stack ptr
	move	sp, t0
	sw	gp, TcxIntGp(t0)
	j	10b
	sw	zero, ThPcstkTop(t0)	// clear call stack pointer

// Handle an exception which is not a h/w interrupt nor an API call.
//
//	(k1) = cause
//	(s0) = ptr to thread structure
//	original V0, A0, S0, Sp, Gp, & EPC saved into thread struct

40:	sw	a1, TcxIntA1(s0)
	sw	a2, TcxIntA2(s0)
	sw	a3, TcxIntA3(s0)
	sw	ra, TcxIntRa(s0)
	sw	AT, TcxIntAt(s0)
	mfc0	a2, badvaddr

// Interrupt reschedules enter here to save register context.
//
//	(k1) = exception cause (0 if resched, 1 if h/w break button)

41:	mfc0	k0, psr			// (k0) = processor status
	move	a1, k1			// (a1) = exception cause
	move	k1, zero		// no interlocked api in progress
	and	v0, k0, MODE_MASK	// (v0) = thread's mode
#if R4000
	// clear User mode, EXL & ERL bits in PSR
	ori	k0, (1<<PSR_EXL) | (1<<PSR_ERL) | (1<<PSR_PMODE)
	xori	k0, (1<<PSR_EXL) | (1<<PSR_ERL) | (1<<PSR_PMODE)
#elif R3000
	ori	k0, 1 << PSR_IEC	// set int enable
#else
  #error Unknown processor type
#endif
	mtc0	k0, psr			// enable interrupts
	sw	v0, TcxPsr(s0)		// record the original mode in Thread struct
	sw	v1, TcxIntV1(s0)
	sw	t0, TcxIntT0(s0)
	sw	t1, TcxIntT1(s0)
	sw	t2, TcxIntT2(s0)
	sw	t3, TcxIntT3(s0)
	mfhi	t0
	mflo	t1
	sw	t4, TcxIntT4(s0)
	sw	t5, TcxIntT5(s0)
	sw	t6, TcxIntT6(s0)
	sw	t7, TcxIntT7(s0)
	sw	t8, TcxIntT8(s0)
	sw	t9, TcxIntT9(s0)
	sw	t0, TcxIntHi(s0)	// save HI mul/div register
	sw	t1, TcxIntLo(s0)	// save LO mul/div register
	sw	s1, TcxIntS1(s0)
	sw	s2, TcxIntS2(s0)
	sw	s3, TcxIntS3(s0)
	sw	s4, TcxIntS4(s0)
	sw	s5, TcxIntS5(s0)
	sw	s6, TcxIntS6(s0)
	sw	s7, TcxIntS7(s0)
	sw	s8, TcxIntS8(s0)
	li	t0, CONTEXT_CONTROL | CONTEXT_INTEGER

	beq	a1, zero, 50f
	sw	t0, TcxContextFlags(s0)
	jal	HandleException		// jump to handler
	move	a0, s0			// (a0) = ptr to thread
	bne	v0, zero, 60f		// resume current thread
	nop

//	The current thread has been blocked or a reschedule is pending.
//	Save the remaining CPU state and call the scheduler to obtain the
//	highest priority thread to run.
//
//	(s0) = CurThdPtr.

resched:
50:	lb	t0, BPowerOff		// if power off flag set
	bne	zero, t0, 90f
	nop

51:
	lh	t0, ReschedFlag
	beq	zero, t0, 52f
	nop

	jal	NextThread
	sh	zero, ReschedFlag	// clear reschedule, still in kernel

52:	lw	s1, dwKCRes
	beq zero, s1, 53f
	nop

	jal	KCNextThread
	sw	zero, dwKCRes		// clear KCreschedule

	lw	s1, dwKCRes
	bne zero, s1, 51b
	nop

53:
	la	t0, RunList
	lw	v0, 4(t0)

	beq	zero, v0, Idle		// no threads to run, do idle processing
	nop
	beq	s0, v0, 60f		// resume current thread
	move	s0, v0			// (s0) = new current thread
	lw	t0, ThHandle(s0)	// (t0) = thread's handle
	sw	s0, CurThdPtr		// remember current THREAD pointer
	sw	t0, hCurThread		//   and current thread handle
	lw	t0, ThProc(s0)		// (t0) = ptr to current process
	lw	t4, ThAKey(s0)		// (t4) = thread's access key
	lw	v0, PrcHandle(t0)	// (v0) = handle of current process
	sw	t0, CurPrcPtr		// remember current process pointer
	sw	v0, hCurProc		//   and current process handle
	lw	t2, PrcVMBase(t0)	// (t2) = memory section base address
	lw	t3, ThTlsPtr(s0)
	srl	t2, VA_SECTION-2	// (t2) = index into section table
	sw	t3, lpvTls
	lw	t2, SectionTable(t2)	// (t2) = process's memory section
	lb	t0, PrcID(t0)		// (t0) = process ID
	sw	t4, CurAKey		// save access key for TLB handler
#if ENTRYHI_PID != 0
	sll	t0, ENTRYHI_PID
#endif
	mtc0	t0, entryhi		// set ASID
	sw	t2, SectionTable(zero)	// swap in default process slot

// Restore the complete thread state.
//
//	(s0) = ptr to thread structure

60: lw	s1, TcxIntS1(s0)	// Restore thread's permanent registers
	lw	s2, TcxIntS2(s0)
	lw	s3, TcxIntS3(s0)
	lw	s4, TcxIntS4(s0)
	lw	s5, TcxIntS5(s0)
	lw	s6, TcxIntS6(s0)
	lw	v0, TcxContextFlags(s0)
	lw	s7, TcxIntS7(s0)

	andi	v1, v0, CONTEXT_INTEGER & 0xFF
	beq	v1, zero, 65f
	lw	s8, TcxIntS8(s0)
	lw	t0, TcxIntHi(s0)	// (t0) = HI mul/div register
	lw	t1, TcxIntLo(s0)	// (t1) = LO mul/div register
	mthi	t0
	mtlo	t1
	lw	v1, TcxIntV1(s0)
	lw	t0, TcxIntT0(s0)
	lw	t1, TcxIntT1(s0)
	lw	t2, TcxIntT2(s0)
	lw	t3, TcxIntT3(s0)
	lw	t4, TcxIntT4(s0)
	lw	t5, TcxIntT5(s0)
	lw	t6, TcxIntT6(s0)
	lw	t7, TcxIntT7(s0)
	lw	t8, TcxIntT8(s0)
	lw	t9, TcxIntT9(s0)
	lw	AT, TcxIntAt(s0)
	lw	a0, TcxIntA0(s0)
	lw	a1, TcxIntA1(s0)
	lw	a2, TcxIntA2(s0)
	lw	a3, TcxIntA3(s0)	

65:	mtc0	zero, psr		// all interrupts off!
	nop
	lw	ra, TcxIntRa(s0)
	lw	v0, TcxIntV0(s0)	// restore return value
	lh	k1, ReschedFlag		// (k1) = resched + nested exception
	li	k0, 1
	beq	k1, k0, 68f		// reschedule pending
	addu	k1, 256			// remove one level of nesting
	sh	k1, ReschedFlag
	lw	k0, BasePSR		// (k0) = global default status value
	lw	k1, TcxPsr(s0)		// (k1) = thread's default status
	lw	sp, TcxIntSp(s0)	// restore stack pointer
	or	k1, k0			// (k1) = thread + global status
#ifdef MIPS_HAS_FPU
	lw k0, g_CurFPUOwner
	bne k0, s0, 66f
	lui k0, 0x2000
	or k1, k0
66:
#endif
	mtc0	k1, psr			// restore status
	lw	k0, TcxFir(s0)		// (k0) = exception return address
	lw	gp, TcxIntGp(s0)
	lw	s0, TcxIntS0(s0)	// Restore thread's permanent registers
#if R4000
	mtc0	k0, epc			// set continuation address for Eret
#endif
	move	k1, zero		// (k1) = 0 (no atomic op in progress)
#if R3000
	j	k0			// return to original caller
	rfe				// restore user status
#elif R4000
	nop
	eret				// restore user status
	nop
	nop
	eret
#else
  #error Unknown processor type
#endif

// No threads to run. Check the resched flag and it is not set, then call OEMIdle.

Idle:	mtc0	zero, psr		// all interrupts off
	nop				//
	nop				// 3 cycle hazard
	nop
	lh	v0, ReschedFlag		// (v0) = resched + nested exception
	lw	a0, BasePSR		// (a0) = global default status value
	bgtz	v0, 68f			// reschedule pending
	nop
	jal	OEMIdle			// let OEM stop clocks, etc.
	nop
	li	v1, 1
	sb	v1, ReschedFlag

// Pending reschedule found during final dispatch, re-enable interrupts and
// try again.
//
//	(a0) = BasePSR
68:	lw	a0, BasePSR		// (a0) = global default status value
	move	k1, zero		// (k1) = 0 (no atomic op in progress)
#if R3000
	ori	a0, 1 << PSR_IEC	// (t0) = base status + int enable
#else
	ori	a0, 1 << PSR_IE         // (t0) = current status + int enable
#endif
	j	resched
	mtc0	a0, psr			// re-enable interrupts

//++++++
// The power off flag has been set. Call DoPowerOff() to notify the file system
// and window manager and invoke OEMPowerOff.

90:	jal	DoPowerOff		// call power off handler
	sb	zero, BPowerOff		// clear power off flag
	b	resched
	move	s0, zero		// no current thread


#if R4000
// TLB load or store exception. These exceptions are routed to the general
// exception vector either because the TLBMiss handler could not translate
// the virtual address or because there is a matching but invalid entry loaded
// into the TLB. If a TLB probe suceeds then, this is due to an invalid entry.
// In that case, the page tables are examined to find a matching valid entry
// for the page causing the fault. If a matching entry is found, then the TLB
// is updated and the instruction is restarted. If no match is found or the
// TLB probe fails, the exception is processed via the normal exception path.
//
//	(k0) = EPC (maybe updated for InterlockedXXX API)
//	(k1) = cause register
//	interrupted T0 saved in SaveT0.

100:	mtc0	k0, epc			// update resume address
	nop
	mfc0	k0, badvaddr		// (k0) = faulting virtual address
	tlbp				// 3 cycle hazard
	bltz	k0, 105f		// kernel address
	srl	t0, k0, VA_SECTION-2	//
	and	t0, SECTION_MASK*4	// (t0) = section * 4
	mfc0	k1, index		// (k1) = TLB index of invalid entry
	lw	t0, SectionTable(t0)	// (t0) = ptr to block table
#if R4100
	bltz	k1, 109f		// not an invalid entry fault
#else	
	bltz	k1, 105f		// not an invalid entry fault
#endif	
101:	srl	k1, k0, VA_BLOCK-2	//
	and	k1, BLOCK_MASK*4	// (k1) = block * 4
	addu	t0, k1			// (t0) = block table entry
	lw	t0, (t0)		// (t0) = ptr to MEMBLOCK structure
	srl	k0, VA_PAGE-2		//
	and	k0, PAGE_MASK*4		// (k0) = page # * 4
	bgez	t0, 105f		// unmapped memblock
	addu	k1, k0, t0		// (k1) = ptr to page entry
	lw	k1, mb_pages(k1)	// (k1) = page table entry
	and	k0, 0xfff8		// (k1) = even page index
	and	k1, PG_VALID_MASK	// (k1) = 0 if invalid page
	beq	zero, k1, 105f		// the page is invalid
	addu	k0, t0			// (k0) = ptr to even page of even/odd pair
	lw	k1, mb_lock(t0)		// (k1) = block access lock
	lw	t0, CurAKey		//
	and	k1, t0			// (k1) = 0 if access not allowed
	lw	t0, mb_pages(k0)	// (t0) = even page info
	beq	zero, k1, 105f		// access not allowed
	lw	k0, mb_pages+4(k0)	// (k0) = odd page info
	mtc0	t0, entrylo0		// set even entry to write into TLB
	mtc0	k0, entrylo1		// set odd entry to write into TLB
	nop
	nop
	tlbwi				// write indexed entry into TLB
	nop				// 3 cycle hazard
	nop				//
	nop
104:
	move	k1, zero		// no atomic op in progress
	lw	t0, SaveT0		// restore t0 value
	eret				//
	nop				// errata...
	nop				//
	eret				//

// Invalid access. Reload k0 & k1 and return to general exception processing.

105:	mfc0	k1, cause
	b	5b
	mfc0	k0, epc

#if R4100
// Sometimes TLB misses go to the general exception vector instead
// of the TLB miss vector. Select an entry to replace by copying random to index.

109:	mfc0	k1, random
	j	101b
	mtc0	k1, index
#endif

#else	// R3000 version
// TLB load or store exception. These exceptions are routed to the general
// exception vector either because the TLBMiss handler could not translate
// the virtual address or because there is a matching but invalid entry loaded
// into the TLB.
//
//	(k0) = EPC (maybe updated for InterlockedXXX API)
//	(k1) = cause register
//	interrupted T0 saved in SaveT0.

100:	sw	k0, SaveK1		// Not K1 but ...
	mfc0	k0, context		// (k0) = faulting VPN * 4
	li	k1, (SECTION_MASK+1)*4
	srl	t0, k0, CTX_SECTION-2
	sltu	k1, t0, k1		// (k1) = 0 if address out of range
	beq	k1, zero, 105f		// address out of range
	and	t0, SECTION_MASK*4	// (t0) = section * 4
	lw	t0, SectionTable(t0)	// (t0) = ptr to block table
	srl	k1, k0, CTX_BLOCK-2
	and	k1, BLOCK_MASK*4	// (k1) = block * 4
	addu	t0, k1			// (t0) = block table entry
	lw	t0, (t0)		// (t0) = ptr to MEMBLOCK structure
	and	k0, PAGE_MASK*4		// (k0) = page # * 4
	bgez	t0, 105f		// unmapped memblock
	addu	k0, t0			// (k0) = ptr to page entry
	lw	k1, mb_lock(t0)		// (k1) = block access lock
	lw	t0, CurAKey
	lw	k0, mb_pages(k0)	// (k0) = page info
	and	k1, t0			// (k1) = 0 if access not allowed
	beq	zero, k1, 105f		// access not allowed
	mtc0	k0, entrylo		// set info to write into TLB
	blez	k0, 105f		// invalid entry
        lw	t0, SaveT0
        tlbwr                           // write entry randomly into TLB
        nop                             // 3 cycle hazzard
        nop
        nop
	lw	k0, SaveK1		// (k0) = resume address
	move	k1, zero		// no atomic op in progress
	j	k0
	rfe

// Invalid access. Reload k0 & k1 and return to general exception processing.

105:	mfc0	k1, cause
	b	5b
	lw	k0, SaveK1
#endif
	.end	GeneralExceptionP

// Stack structure during API call processing.
        .struct 0
apiArg: .space  4 * 4                   // argument register save area
apiSaveRet: .space 4			// return value or return address
apiMode: .space 4			// (pMode) argument
apiSaveGp: .space 4			// extra cpu dependent info (Global Pointer)
apiSaveRa: .space 4			// for unwinding
size_api_args:                          // length of stack frame
apiArg0: .space 4                       // caller argument save area
apiArg1: .space 4
apiArg2: .space 4
apiArg3: .space 4

NESTED_ENTRY(APICall, 0, zero)
//++
// The following code is never executed. Its purpose is to support unwinding
// through the calls to ObjectCall or ServerCallReturn.
//--
        .set    noreorder
        .set    noat
        subu	sp, size_api_args
        sw	ra, apiSaveRa(sp)	// unwinder: (ra) = APICallReturn
        PROLOGUE_END

// Process an API Call or return.
//
//	(k0) = EPC (encodes the API set & method index)

200:	lb	t0, KNest		// (t0) = kernel nest depth
	mfc0	t1, psr			// (t1) = processor status
	blez	t0, 5b			// non-preemtible, API Calls not allowed
	subu	t0, k0, FIRST_METHOD
	lw	t3, BasePSR
	move	k1, zero		// reset atomic op. flag
	or	t3, 1			// (t3) = current interrupt mask + int enable
#ifdef MIPS_HAS_FPU
	lw t8, g_CurFPUOwner
	lw t7, CurThdPtr
	bne t7, t8, 201f
	lui t9, 0x2000
	or t3, t9
201:
#endif
	mtc0	t3, psr			// enable interrupts
	and	t1, MODE_MASK		// (t1) = thread's execution mode
	subu	sp, size_api_args	// make room for new args + temps
  #if APICALL_SCALE == 2
	sra	t0, 1			// (t0) = method index
  #elif APICALL_SCALE == 4
	sra	t0, 2			// (t0) = method index
  #else
	#error Invalid value for APICALL_SCALE
  #endif
	//--- interrupts enabled and preemptible
	li	t9, SYSCALL_RETURN
	sw	t9, apiSaveRa(sp)	// for the unwinder
	addu	t3, t0, 1		// (t3) = 0 iff API return
	beq	zero, t3, 250f		// go process API return
	sw	t1, apiMode(sp)

	// Save api arguments onto the stack
	sw	a0, apiArg0(sp)
	sw	a1, apiArg1(sp)
	sw	a2, apiArg2(sp)
	sw	a3, apiArg3(sp)

	move	a3, t0			// (a3) = method index
	addu	a2, sp, apiArg0		// (a2) = ptr to function args
	move	a1, ra			// (a1) = return address
	jal	ObjectCall
	addu	a0, sp, apiMode		// (a0) = pMode

// Invoke server function. If the thread is running in kernel mode, then
// we just call the function directly from here.
//
//	(v0) = address of server function

	lw	t0, apiMode(sp)		// (t0) = mode to invoke the function in
	lw	a0, apiArg0(sp)		// reload argument registers.
	lw	a1, apiArg1(sp)
	lw	a2, apiArg2(sp)
	and	t1, t0, 1 << PSR_PMODE
	beq	t1, zero, 210f		// invoke function in kernel mode
	lw	a3, apiArg3(sp)

// Call to user mode server. To do this: build a new PSR value from the thread's mode
// and BasePSR. This must be done with interrupts disabled so that BasePSR is not
// changing.
//
//	(t0) = new mode bits

	mtc0	zero, psr		// all interrupts off
	nop				// 3 cycle hazard
	li	ra, SYSCALL_RETURN
	addu	sp, size_api_args
	lw	t1, BasePSR
#if R4000
	mtc0	v0, epc
#endif
#ifdef MIPS_HAS_FPU
	lw t8, g_CurFPUOwner
	lw t7, CurThdPtr
	bne t7, t8, 202f
	lui t9, 0x2000
	or t1, t9
202:
#endif
	or	t1, t0			// (t1) = merged status
	mtc0	t1, psr			// reload status
	nop
#if R4000
	nop
	nop
	eret
#elif R3000
	j	v0
	rfe
#else
  #error Unknown processor type
#endif

// Call to kernel mode server. Call the function directly to save a trap
// on the return.
//
//	(v0) = address of fucntion
//	(a0-a3) = function arguments

210:	jal	v0
	addu	sp, size_api_args	// remove extra stuff from the stack

ALTERNATE_ENTRY(APICallReturn)
	subu	sp, size_api_args	// recreate entry frame
	li	t0, KERNEL_MODE
	sw	t0, apiMode(sp)
	// Fall through as if we had taken a trap to get here.

// Return from an API call. Pop the thread's CALLSTACK list and restore the
// thread's current process, access key, and mode.
//
//	(v0) = api return value (must be preserved)

250:	sw	v0, apiSaveRet(sp)	// save return value
	jal	ServerCallReturn
	addu	a0, sp, apiMode		// (a0) = pMode

	lw	t0, apiMode(sp)		// (t0) = mode to return to
	move	ra, v0			// (ra) = return address
	and	t1, t0, 1 << PSR_PMODE
	beq	t1, zero, 255f		// returning to kernel mode
	lw	v0, apiSaveRet(sp)

// Return to user mode. To do this: build a new PSR value from the thread's mode
// and BasePSR. This must be done with interrupts disabled so that BasePSR is not
// changing.
//
//	(t0) = new mode bits

	mtc0	zero, psr		// all interrupts off
	nop				// 3 cycle hazard
	nop
	addu	sp, size_api_args
	lw	t1, BasePSR
#if R4000
	mtc0	ra, epc
#endif
#ifdef MIPS_HAS_FPU
	lw t8, g_CurFPUOwner
	lw t7, CurThdPtr
	bne t7, t8, 251f
	lui t9, 0x2000
	or t1, t9
251:
#endif
	or	t1, t0			// (t1) = merged status
	mtc0	t1, psr			// reload status
	nop
#if R4000
	nop
	nop
	eret
#elif R3000
	j	ra
	rfe
#else
  #error Unknown processor type
#endif

// Return to kernel mode.

255:	j	ra
	addu	sp, size_api_args	// remove extra stuff from the stack

END_REGION(GeneralException_End)
	.set at
	.set reorder
	.end	APICall


LEAF_ENTRY(DisabledInterruptHandler)
// This routine services interrupts which have been disabled. It masks the
// interrupt enable bit in the PSR to prevent further interrupts and treats
// the interrupt as a NOP.
//
//	Entry	(a0) = interrupt level * 4
//	Exit	(v0) = SYSINTR_NOP
//	Uses	a0, a1, v0
	.set noreorder
	srl	a0, 2
	li	v0, 0x400
	sllv	a0, v0, a0
	li	v0, -1
	xor	a0, v0			// (a0) = ~(intr. mask)
	mfc0	v0, psr
	lw	a1, BasePSR
	and	v0, a0			// (v0) = psr w/intr disabled
	mtc0	v0, psr
	and	a1, a0			// (a0) = base PSR w/intr disabled
	sw	a1, BasePSR
	j	ra
	li	v0, SYSINTR_NOP
	.end DisabledIntrHandler


LEAF_ENTRY(FalseInterrupt)
        .set    noreorder

        j       ra
        li      v0, SYSINTR_NOP

        .set    reorder
        .end    FalseInterrupt

// CaptureContext is invoked in kernel context on the user thread's stack to
// build a context structure to be used for exception unwinding.
//
//	(sp) = aligned stack pointer

	LEAF_ENTRY(CaptureContext)
	.set noreorder
	.set noat
	subu	sp, ContextFrameLength	// (sp) = ptr to CONTEXT buffer
	sw	zero, 0(sp)		// make sure that the stack is addressable
	.end CaptureContext

	NESTED_ENTRY(xxCaptureContext, ContextFrameLength, zero)
	.set noreorder
	.set noat
	sw	sp, CxIntSp(sp)		// fixed up by ExceptionDispatch
	sw	a0, CxIntA0(sp)
	sw	a1, CxIntA1(sp)
	sw	a2, CxIntA2(sp)
	sw	a3, CxIntA3(sp)
	sw	gp, CxIntGp(sp)
	sw	s0, CxIntS0(sp)
	sw	s1, CxIntS1(sp)
	sw	s2, CxIntS2(sp)
	sw	s3, CxIntS3(sp)
	sw	s4, CxIntS4(sp)
	sw	s5, CxIntS5(sp)
	sw	s6, CxIntS6(sp)
	sw	s7, CxIntS7(sp)
	sw	s8, CxIntS8(sp)
	sw	v0, CxIntV0(sp)
	sw	v1, CxIntV1(sp)
	sw	AT, CxIntAt(sp)
	sw	ra, CxIntRa(sp)
	sw	t0, CxIntT0(sp)
	sw	t1, CxIntT1(sp)
	sw	t2, CxIntT2(sp)
	sw	t3, CxIntT3(sp)
	mfhi	t0
	mflo	t1
	sw	t4, CxIntT4(sp)
	sw	t5, CxIntT5(sp)
	sw	t6, CxIntT6(sp)
	sw	t7, CxIntT7(sp)
	sw	t8, CxIntT8(sp)
	sw	t0, CxIntHi(sp)	// save HI mul/div register
	sw	t1, CxIntLo(sp)	// save LO mul/div register
	sw	t9, CxIntT9(sp)
	sw	ra, CxFir(sp)		// fixed up by ExceptionDispatch
	sw	zero, CxIntZero(sp)
        PROLOGUE_END
	li	t0, CONTEXT_FULL
	sw	t0, CxContextFlags(sp)
	jal	ExceptionDispatch
	move	a0, sp			// (a0) = context record argument

// Reload updated context and resume thread execution.

	lw	s0, CxIntS0(sp)	// Restore thread's permanent registers
	lw	s1, CxIntS1(sp)
	lw	s2, CxIntS2(sp)
	lw	s3, CxIntS3(sp)
	lw	s4, CxIntS4(sp)
	lw	s5, CxIntS5(sp)
	lw	s6, CxIntS6(sp)
	lw	s7, CxIntS7(sp)
	lw	s8, CxIntS8(sp)
	lw	t0, CxIntHi(sp)	// (t0) = HI mul/div register
	lw	t1, CxIntLo(sp)	// (t1) = LO mul/div register
	mthi	t0
	mtlo	t1
	lw	v1, CxIntV1(sp)
	lw	t0, CxIntT0(sp)
	lw	t1, CxIntT1(sp)
	lw	t2, CxIntT2(sp)
	lw	t3, CxIntT3(sp)
	lw	t4, CxIntT4(sp)
	lw	t5, CxIntT5(sp)
	lw	t6, CxIntT6(sp)
	lw	t7, CxIntT7(sp)
	lw	t8, CxIntT8(sp)
	lw	t9, CxIntT9(sp)
	lw	AT, CxIntAt(sp)

	mtc0	zero, psr		// all interrupts off!
	lw	a3, CxIntA3(sp)		// restore regs while		
	lw	a2, CxIntA2(sp)		//  waiting for interruts to
	lw	a1, CxIntA1(sp)		//  be disabled.
	lw	a0, BasePSR		// (a0) = global default status value
	lw	v0, CxPsr(sp)		// (v0) = thread's default status
	lw	ra, CxIntRa(sp)
	or	v0, a0			// (v0) = thread + global status
#if R4000
#ifdef MIPS_HAS_FPU
	lw a0, g_CurFPUOwner
	lw gp, CurThdPtr
	bne a0,gp, ccnofp
	lui a0, 0x2000
	or v0, a0
ccnofp:
#endif
	sw		v0, SaveK0
#else
	mtc0	v0, psr			// restore status
#endif
	lw	a0, CxIntA0(sp)
	lw	k0, CxFir(sp)		// (k0) = exception return address
	lw	gp, CxIntGp(sp)
	lw	v0, CxIntV0(sp)	// restore return value
	lw	sp, CxIntSp(sp)	// restore stack pointer
#if R4000
	mtc0    k0, epc			// set continuation address
	lw		k0, SaveK0
	mtc0	k0, psr
	nop
	nop
	eret				// restore user status
	nop
	nop
	eret
#elif R3000
	j	k0
	rfe
#else
  #error Unknown processor type
#endif
	.set at
	.set reorder

	.end xxCaptureContext

//
// Define call frame for calling exception handlers.
//

        .struct 0
CfArg:  .space  4 * 4                   // argument register save area
        .space  3 * 4                   // fill for alignment
CfRa:   .space  4                       // saved return address
CfCstk:	.space	((CstkSizeof + 4) & ~7)	// CALLSTACK object for SYSCALL_RETURN
										// Must be 8 byte aligned
CfFrameLength:                          // length of stack frame
CfA0:   .space  4                       // caller argument save area
CfA1:   .space  4                       //
CfA2:   .space  4                       //
CfA3:   .space  4                       //
CfExr:  .space  4                       // address of exception routine
CfPsr:	.space	4			// mode to execute handler in

        SBTTL("Execute Handler for Exception")
//++
//
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
//
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
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (4 * 4(sp)) - supplies a pointer to the exception handler
//       that is to be called.
//
//    ExceptionMode (5 * 4(sp)) - PSR value for running ExceptionRoutine
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        EXCEPTION_HANDLER(RtlpExceptionHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForException, CfFrameLength, zero)

        subu    sp,sp,CfFrameLength     // allocate stack frame
        sw      ra,CfRa(sp)             // save return address
        PROLOGUE_END

        lw	t1, CfPsr(sp)		// (t1) = target status
        lw      t0,CfExr(sp)            // get address of exception routine
        and	t2, t1, 1 << PSR_PMODE	// (t2) = previous execution mode
        sw      a3,CfA3(sp)             // save address of dispatcher context
	bne	t2, zero, 20f		// must switch to user mode
        jal     t0                      // call exception exception handler
10:     lw      ra,CfRa(sp)             // restore return address
        addu    sp,sp,CfFrameLength     // deallocate stack frame
        j       ra                      // return

        .set    noreorder
20:	mtc0	zero, psr		// disable interrupts
	lw	t4, CurThdPtr		// (t4) = ptr to current THREAD struct
	la	t2, 10b			// (t2) = address to return to
	lw	t4, ThPcstkTop(t4)

	// Link a CALLSTACK object to the thread for the SYSCALL_RETURN
	addu	t3, sp, CfCstk		// t3 = pointer to our CALLSTACK object
	sw		t4, CstkNext(t3)	// t3->pcstkNext = pCurThread->pcstkTop
	lw		t4, CurThdPtr		// t4 = pCurThread
	sw		t3, ThPcstkTop(t4)	// pCurThread->pcstkTop = t3
	move	t4, t3				// t4 = pCurThread->pcstkTop

	lw	t3, BasePSR
	sw	t2, CstkRa(t4)
	li	t2, 0xFFFF89AB
	sw	t2, CstkAkyLast(t4)
	li	t2, KERNEL_MODE
	sw	t2, CstkPrcLast(t4)	// return to kernel mode
	or	t3, t1
	mtc0	t3, psr
	li	ra, SYSCALL_RETURN
#if R4000
	mtc0	t0, epc
	sw	zero, CstkAkyLast(t4)
	nop
	eret				// switch to user mode & jump to handler
	nop		//
	nop		// errata
	eret		//
#elif R3000
	sw	zero, CstkAkyLast(t4)
	j	t0
	rfe
#else
  #error Unknown processor type
#endif
	.set reorder

        .end    RtlpExecuteHandlerForException

        SBTTL("Local Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer from its establisher's
//    call frame, store this information in the dispatcher context record,
//    and return a disposition value of nested exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

        LEAF_ENTRY(RtlpExceptionHandler)
	.set noreorder
        lw      t0,ErExceptionFlags(a0) // get exception flags
        nop
        and     t0,t0,EXCEPTION_UNWIND  // check if unwind in progress
        .set reorder
        bne     zero,t0,10f             // if neq, unwind in progress

//
// Unwind is not in progress - return nested exception disposition.
//

        lw      t0,CfA3 - CfA0(a1)      // get dispatcher context address
        li      v0,ExceptionNestedException // set disposition value
        .set noreorder
        lw      t1,DcEstablisherFrame(t0) // copy the establisher frame pointer
        nop
        sw      t1,DcEstablisherFrame(a3) // to current dispatcher context
        .set reorder
        j       ra                      // return

//
// Unwind is in progress - return continue search disposition.
//

10:     li      v0,ExceptionContinueSearch // set disposition value
        j       ra                      // return

        .end    RtlpExceptionHandler)

        SBTTL("Execute Handler for Unwind")
//++
//
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
//
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
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (4 * 4(sp)) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        EXCEPTION_HANDLER(RtlpUnwindHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForUnwind, CfFrameLength, zero)

        subu    sp,sp,CfFrameLength     // allocate stack frame
        sw      ra,CfRa(sp)             // save return address
        PROLOGUE_END

        lw	t1, CxPsr(a2)		// (t1) = target status
        lw      t0,CfExr(sp)            // get address of exception routine
        and	t2, t1, 1 << PSR_PMODE	// (t2) = previous execution mode
        sw      a3,CfA3(sp)             // save address of dispatcher context
	bne	t2, zero, 20f		// must switch to user mode
        jal     t0                      // call exception exception handler
10:     lw      ra,CfRa(sp)             // restore return address
        addu    sp,sp,CfFrameLength     // deallocate stack frame
        j       ra                      // return

        .set    noreorder
20:	mtc0	zero, psr		// disable interrupts
	lw	t4, CurThdPtr		// (t4) = ptr to current THREAD struct
	la	t2, 10b			// (t2) = address to return to
	lw	t4, ThPcstkTop(t4)

	// Link a CALLSTACK object to the thread for the SYSCALL_RETURN
	addu	t3, sp, CfCstk		// t3 = pointer to our CALLSTACK object
	sw		t4, CstkNext(t3)	// t3->pcstkNext = pCurThread->pcstkTop
	lw		t4, CurThdPtr		// t4 = pCurThread
	sw		t3, ThPcstkTop(t4)	// pCurThread->pcstkTop = t3
	move	t4, t3				// t4 = pCurThread->pcstkTop

	lw	t3, BasePSR
	sw	t2, CstkRa(t4)
	sw	zero, CstkAkyLast(t4)
	or	t3, t1
	mtc0	t3, psr
	li	ra, SYSCALL_RETURN
#if R4000
	mtc0	t0, epc
	li	t2, 0x03
	sw	t2, CstkPrcLast(t4)
	nop
	eret				// switch to user mode & jump to handler
	nop		//
	nop		// errata
	eret		//
#elif R3000
	li	t2, 0x14
	sw	t2, CstkPrcLast(t4)	// return to kernel mode
	j	t0
	rfe
#else
  #error Unknown processor type
#endif
	.set reorder

        .end    RtlpExecuteHandlerForUnwind

        SBTTL("Local Unwind Handler")
//++
//
// EXCEPTION_DISPOSITION
// RtlpUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--

        LEAF_ENTRY(RtlpUnwindHandler)
	.set noreorder
        lw      t0,ErExceptionFlags(a0) // get exception flags
        nop
        and     t0,t0,EXCEPTION_UNWIND  // check if unwind in progress
        .set reorder
        beq     zero,t0,10f             // if eq, unwind not in progress

//
// Unwind is in progress - return collided unwind disposition.
//

        lw      t0,CfA3 - CfA0(a1)      // get dispatcher context address
        li      v0,ExceptionCollidedUnwind // set disposition value
        lw      t1,DcControlPc(t0)      // Copy the establisher frames'
        lw      t2,DcFunctionEntry(t0)  // dispatcher context to the current
        lw      t3,DcEstablisherFrame(t0) // dispatcher context
        lw      t4,DcContextRecord(t0)  //
        sw      t1,DcControlPc(a3)      //
        sw      t2,DcFunctionEntry(a3)  //
        sw      t3,DcEstablisherFrame(a3) //
        sw      t4,DcContextRecord(a3)  //
        j       ra                      // return

//
// Unwind is not in progress - return continue search disposition.
//

10:     li      v0,ExceptionContinueSearch // set disposition value
        j       ra                      // return
        .end    RtlpUnwindHandler

