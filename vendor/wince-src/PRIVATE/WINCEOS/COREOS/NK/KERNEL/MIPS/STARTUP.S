//      TITLE("Kernel Initialization")
//++
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    startup.s
//
// Abstract:
//
//    This module implements the code necessary to initialize the Kernel to
// run on an R3000 series processor.
//
//
// Environment:
//
//    Kernel mode only.
//
//--

#include "ksmips.h"
#include "nkintr.h"
#include "kpage.h"
#include "mem_mips.h"

#define jalfix(func)	\
	jal	func;	\
	nop;

#if 0	// PERP
#define LIGHTS(t0, t1, value)	\
	lui     t0, 0xaa00;	\
	li      t1, value;	\
	sw      t1, 0x1010(t0);
#endif

#if 0
#define LIGHTS(t0, t1, value)	\
	lui     t0, 0xb040;	\
	lh	t1, 2(t0);	\
	and	t1, 0xff;	\
	or	t1, value<<8;	\
	sh      t1, 2(t0);
#endif

#ifndef LIGHTS
#define LIGHTS(t0, t1, value)
#endif
	.text

LEAF_ENTRY(KernelStart)
	.set noreorder
#if R3000
	mtc0	zero, cause		// Clear interrupts
	mtc0	zero, entryhi		// Clear asid
	mtc0	zero, context		// clear the context register
	mtc0	zero, entrylo
#elif R4000
	mtc0	zero, cause		// Clear interrupts
	mtc0	zero, entryhi		// Clear asid
	mtc0	zero, context		// clear the context register
	mtc0	zero, entrylo0
	mtc0	zero, entrylo1
	mtc0	zero, pagemask
	mtc0	zero, count			// clear count (time since restart)
#else
 #error unknown processor type
#endif
	move	k0, zero
	move	k1, zero

// Zero out kernel data page.

	li	t0, 0xA0000000 | KPAGE_PHYS
	li	t1, 4096
2:	sw	zero, (t0)
	subu	t1, 4
	bgtz	t1, 2b
	addiu	t0, 4

	LIGHTS(t0,t1, 0xCC)

// Initialize SectionTable in KPage
	li	t0, 0xA0000000 | (KPAGE_PHYS+(SectionTable-KData))
	li	t1, SECTION_MASK+1
	la	t2, NullSection
3:	sw	t2, (t0)
	subu	t1, 1
	bgtz	t1, 3b
	addiu	t0, 4

// Initialize the interrupt dispatch tables
	li	t0, 0xA0000000 | (KPAGE_PHYS+(FalseInt-KData))
	la	t1, FalseInterrupt
	sw	t1, (t0)
	la	t1, DisabledInterruptHandler
	sw	t1, 4(t0)
	sw	t1, 8(t0)
	sw	t1, 12(t0)
	sw	t1, 16(t0)
	sw	t1, 20(t0)
	sw	t1, 24(t0)
	

// Load temp stack pointer & global pointer.
	li	sp, 0xA0000000 | (KStack-KData) + KPAGE_PHYS
	li	s0, 0xA0000000
	li	gp, 0
	li	t1, 0x80000000
	sw	t1, ((HandleBase-KData)+KPAGE_PHYS)(s0)
	la	t1, APICallReturn
	sw	t1, ((PtrAPIRet-KData)+KPAGE_PHYS)(s0)

	LIGHTS(t0,t1, 0xF0)

//	Initialize address translation hardware.
#if R3000
	lw	v1, OEMTLBSize	// (v1) = index & loop counter
	li      v0, 0x80000000	// Unmapped address

10:	mtc0    zero, entrylo	// Clear entrylo - Invalid entry
	mtc0    v0, entryhi	// Clear entryhi - Ivalid address
	mtc0    v1, index	// Set index
	nop                     // Fill delay slot
	tlbwi                   // Write entry (indexed)
	bne     v1, zero, 10b	// If not zero do next one
	addiu   v1, v1, -256	// Decrement index, loop counter

// Initialize wired TLB entries as follows:
//	0: virtual 0x00005000 to the kernel's data page (read-only)
//	1: virtual 0x00007000 to kernel's data page (read-write)

	li	v0, KPAGE_USER
	mtc0	v0, entryhi
	li	v1, KPAGE_PHYS | 0x0300	// valid, global
	mtc0	v1, entrylo
	mtc0	zero, index
	nop
	tlbwi			// write indexed entry
	nop
	li	v0, KData
	mtc0	v0, entryhi
	ori	v1, 0x0700	// dirty, valid, global
	mtc0	v1, entrylo
	li	t2, 256
	mtc0	t2, index
	addu	t2, 256
	tlbwi			// write indexed entry
	nop
	nop

#elif R4000
#if R4100 && PAGE_SIZE==4096
	li	v0, 0x00001800
	mtc0	v0, pagemask	// set for 4k pages on Vr4100
#else
	mtc0	zero, pagemask	// set for 1k pages on Vr4100 or 4K pages on R4200
#endif
	li      v0, 0x80000000	// Unmapped address
	lw	v1, OEMTLBSize	// (v1) = index & loop counter
10:	mtc0    zero, entrylo0	// Clear entrylo0 - Invalid entry
	mtc0    zero, entrylo1	// Clear entrylo1 - Invalid entry
	mtc0    v0, entryhi	// Clear entryhi - Ivalid address
	mtc0    v1, index	// Set index
	add	v0, 0x2000
	nop                     // Fill delay slot
	tlbwi                   // Write entry (indexed)
	addiu   v1, v1, -1	// Decrement index, loop counter
	bne     v1, zero, 10b	// If not zero do next one
	nop

// Initialize wired TLB entries as follows:
//	0: virtual 0x00005000 to the kernel's data page (read-only)
//	1: virtual 0xFFFFD000 to kernel's data page (read-write)

#if R4100 && PAGE_SIZE==1024
	li	v0, 0x00001800
	mtc0	v0, pagemask	// wired entries are always 4k pages on Vr4100
#endif
	li      v0,  2         // load 2 into wired
	mtc0    v0, wired      // load 2 into wired
	li	v0, KPAGE_USER
	mtc0	v0, entryhi
#if !(KPAGE_USER & 0x1000)
  #error KPAGE mapping expected to be odd page
#endif
	li      v0, 1		// global entry
	li	v1, KPAGE_PHYS_LO | 0x03 | PG_CACHE // valid, global,cached
	mtc0    v0, entrylo0	// even page info
	mtc0	v1, entrylo1	// odd page info
	mtc0    zero, index
	nop
	nop
	tlbwi			// write indexed entry
	nop
	nop

	li	v0, KData
	mtc0	v0, entryhi
	li	v0, 1		// global entry
	li	v1, KPAGE_PHYS_LO | 0x07 | PG_CACHE // valid, dirty, global,cached
	mtc0    v0, entrylo0	// even page info
	mtc0	v1, entrylo1	// odd page info
	mtc0	v0, index	// index = 1
	nop
	nop
	tlbwi			// write indexed entry
	nop
	nop
	nop
#if R4100 && PAGE_SIZE==1024
	mtc0	zero, pagemask	// set for 1k pages on Vr4100
#endif
#else
 #error unknown processor type
#endif    // R4000

// Install TLBMiss handler
	li	a0, 0x0000
#ifdef CELOG
	la	a1, CeLogTLBMissHandler
	la	a2, CeLogTLBMissHandler_End
#else	
	la	a1, TLBMissHandler
	la	a2, TLBMissHandler_End
#endif
	jalfix(InstallHandler)

// Install General Exception handler
#if R3000
	li	a0, 0x0080
#elif R4000
	li	a0, 0x0180
#else
 #error unknown processor type
#endif
	la	a1, GeneralException
	la	a2, GeneralException_End
	jalfix(InstallHandler)

#if R4100	
// Install CacheError handler
	li	a0, 0x0100
	la	a1, CacheErrorHandler
	la	a2, CacheErrorHandler_End
	jalfix(InstallHandler)
	nop
	nop
#endif

	LIGHTS(t0,t1, 0xF8)

//	Flush various cpu caches.
	jalfix(FlushICache)
	jalfix(FlushDCache)

	LIGHTS(t0,t1, 0xFC)

// Switch execution and stack to cached, un-mapped region.
	la	t1, cached
	j	t1			// jump to cached segment
	nop

cached:	li	sp, KStack		// (sp) = 0xFFFFF???

	LIGHTS(t0,t1, ~1)

	lw	a0, pTOC
	jalfix(KernelRelocate)
	
        LIGHTS(t0,t1, ~2)
        
	jal     OEMInitDebugSerial			// initialize serial port
	nop

	LIGHTS(t0,t1, ~4)

	la	a0, NKSignon
	jal	OEMWriteDebugString
	nop

	mfc0	t0, prid		// (t0) = process ID information
	nop				// 1 cycle hazard??
	sh	t0, ProcessorRevision
	LIGHTS(t0,t1, 0)

	jalfix(OEMInit)

	jalfix(KernelFindMemory)

// Load new status register with:
//	interrupts enabled (clock interrupts enabled)
//	BEV reset

	lw	t0, BasePSR
	nop
	ori	t0, 1			// set interrupt enable
	mtc0	t0, psr			// enable interrupts
	nop

	jalfix(KernelInit)
	j	FirstSchedule
	nop
	.end	StartUp


LEAF_ENTRY(InstallHandler)
// InstallHandler(vector, handler start, handler end)
//
//	Install an exception handler by copying the code into memory at the
// exception vector location. This method requires that a handler fit in the
// space between its vector location and the next vector.
//
// 	Entry	(a0) = vector address
//		(a1) = ptr to start of handler code
//		(a2) = ptr to end of handler code
//	Exit	none
//	Uses	t0-2

	.set noreorder
	lui	t0, 0xA000
	or	a0, t0			// force into un-cached segment
	
	subu	t2, a2, a1		// (t2) = # of bytes to copy
	sltu	t1, t2, 0x81		// (t1) = 0 iff length > 0x80
	beq	zero, t1, 20f		// handler too large to copy
	nop
	or	a1, t0			// force into un-cached segment
10:	lw	t0, (a1)
	lw	t1, 4(a1)
	sw	t0, (a0)
	sw	t1, 4(a0)
	subu	t2, 8			// (t2) = remaining bytes copy count
	addu	a0, 8			// (a0) = ptr to dest for next 8 bytes
	bgtz	t2, 10b
	addu	a1, 8			// (a1) = ptr to src of next 8 bytes

	j	ra
	nop

// The handler is more than 0x80 bytes, just install a jump to it instead
// of copying the entire handler.

20:     
#if R4100
	sw	zero, 0(a0)
	sw	zero, 4(a0)
	srl     a2, a1, 16
        sh      a2, 8(a0)
        li      a2, 0x3c1a
        sh      a2, 10(a0)               // lui k0, [high 16 bits of address]
	sh      a1, 12(a0)

        li      a2, 0x375a
        sh      a2, 14(a0)               // ori k0, k0, [low 16 bits of address
        li      a2, 0x03400008
        sw      a2, 16(a0)               // j k0
	j	ra
        sw      zero, 20(a0)            // nop
#else
	srl     a2, a1, 16
        sh      a2, 0(a0)
        li      a2, 0x3c1a
        sh      a2, 2(a0)               // lui k0, [high 16 bits of address]
	sh      a1, 4(a0)

        li      a2, 0x375a
        sh      a2, 6(a0)               // ori k0, k0, [low 16 bits of address
        li      a2, 0x03400008
        sw      a2, 8(a0)               // j k0
	j	ra
        sw      zero, 12(a0)            // nop
#endif

	.set reorder
	.end	InstallHandler
	

