/** Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. **/

/*+
	TLB Miss handler

	This module contains the code to handle reloading the hardware
 Translation Lookaside Buffer.

 */

#include "ksmips.h"
#include "nkintr.h"
#include "kpage.h"
#include "mem_mips.h"

#define jalfix(func)	\
	jal	func;	\
	nop;

	.text
	.globl	NullSection
NullSection:
	.repeat	BLOCK_MASK+1
	.word	0
	.endr


//-----------------------------------------------------
// CELOG replaces the TLB MISS Handler with its own.
//
#ifdef CELOG
LEAF_ENTRY(CeLogTLBMissHandler)
        .set noreorder

        sw      t0, SaveT0              //                              :1
        la      k0, dwCeLogTLBMiss      // Global variable              :2-3
        lw      t0, 0(k0)               //                              :4
        addi    t0, t0, 1               // increment                    :5
        sw      t0, 0(k0)               // Update counter               :6
        
        lw      t0, SaveT0              //                              :7
        la      k0, TLBMissHandler      // Load address of real handler :8-9
        j       k0                      //                              :10
        nop                             //                              :11
        
        END_REGION(CeLogTLBMissHandler_End)

        .set    reorder
        .set    at
        .end    CeLogTLBMissHandler
#endif


//	The user mode virtual address space is 2GB split into 64 sections
// of 512 blocks of 16 4K pages. For some platforms, the # of blocks in
// a section may be limited to fewer than 512 blocks to reduce the size
// of the intermeditate tables. E.G.: Since the PeRP only has 5MB of total
// RAM, the sections are limited to 4MB. This results in 64 block sections.
//
// Virtual address format:
//	3322222 222221111 1111 110000000000
//	1098765 432109876 5432 109876543210
//	zSSSSSS BBBBBBBBB PPPP oooooooooooo
//
// Context register format:
//	33222222222211111 111110000 0000 00
//	10987654321098765 432109876 5432 10
//	zzzzzzzzzzzSSSSSS BBBBBBBBB PPPP zz


LEAF_ENTRY(TLBMissHandler)
	.set    noreorder
	.set    noat
#if R3000
	mfc0	k0, context		// (k0) = faulting VPN * 4	:1
	sw	t0, SaveT0		//				:2
	sw	k1, SaveK1		//				:3
	srl	t0, k0, CTX_SECTION-2	//				:4
	and	t0, SECTION_MASK*4	// (t0) = section * 4		:5
	lw	t0, SectionTable(t0)	// (t0) = ptr to block table	:6
	srl	k1, k0, CTX_BLOCK-2	//				:7
	and	k1, BLOCK_MASK*4	// (k1) = block * 4		:8
	addu	t0, k1			// (t0) = block table entry	:9
	lw	t0, (t0)		// (t0) = ptr to MEMBLOCK structure
	and	k0, PAGE_MASK*4		// (k0) = page # * 4		:11
	bgez	t0, 80f			// unmapped memblock		:12
	addu	k0, t0			// (k0) = ptr to page entry	:13
	lw	k1, mb_lock(t0)		// (k1) = block access lock	:14
	lw	t0, CurAKey		//				:15
	lw	k0, mb_pages(k0)	// (k0) = page info		:16
	and	k1, t0			// (k1) = 0 if access not allowed
	beq	zero, k1, 80f		// access not allowed		:18
	mtc0	k0, entrylo		// set info to write into TLB	:19
	blez	k0, 81f			// invalid entry		:20
	lw	t0, SaveT0		// restore t0 register		:21
        tlbwr                           // write entry randomly into TLB:22
        nop                             // 3 cycle hazzard		:23
        nop				//				:24
	mfc0	k0, epc			//				:25
	lw	k1, SaveK1		// restore atomic restart flag	:26
	j	k0			//				:27
	rfe				//				:28
	nop
	nop

80:	lw	t0, SaveT0		//				:31
81:	lw	k1, SaveK1		//				:32
	// fall through into general exception handler
	///j	0x80000080

#elif R4000
	mfc0	k0, badvaddr		// (k0) = faulting virtual addr	:1
	sw	t0, SaveT0		//				:2
	sw	k1, SaveK1		//				:3
	srl	t0, k0, VA_SECTION-2	//				:4
	bltz	k0, 80f			// address out of range		:5
	and	t0, SECTION_MASK*4	// (t0) = section * 4		:6
	lw	t0, SectionTable(t0)	// (t0) = ptr to block table	:7
	srl	k1, k0, VA_BLOCK-2	//				:8
	and	k1, BLOCK_MASK*4	// (k1) = block * 4		:9
	addu	t0, k1			// (t0) = block table entry	:10
	lw	t0, (t0)		// (t0) = ptr to MEMBLOCK structure
	srl	k0, VA_PAGE-2		//				:12
	and	k0, (PAGE_MASK/2)*8	// (k0) = (even page #) * 4	:13
	bgez	t0, 80f			// unmapped memblock		:14
	addu	k0, t0			// (k0) = ptr to page entry	:15
	lw	k1, mb_lock(t0)		// (k1) = block access lock	:16
	lw	t0, CurAKey		//				:17
	and	k1, t0			// (k1) = 0 if access not allowed
	lw	t0, mb_pages(k0)	// (t0) = even page info	:19
	beq	zero, k1, 80f		// access not allowed		:20
	lw	k0, mb_pages+4(k0)	// (k0) = odd page info		:21
	mtc0	t0, entrylo0		// set even entry to write into TLB
	mtc0	k0, entrylo1		// set odd entry to write into TLB
	lw	t0, SaveT0		// restore t0 register		:24
	lw	k1, SaveK1		//				:25
	tlbwr                           // write entry randomly into TLB:26
	nop				//				:27
	nop                             // 3 cycle hazzard		:28
70:	eret				//				:29

80:	lw	k1, SaveK1		//				:30
	.word	0x08000060		// j 0x80000180			:31
	lw	t0, SaveT0		//				:32
#else
 #error unknown processor type
#endif

	END_REGION(TLBMissHandler_End)

#ifdef CELOG
#if R3000
        // if we don't relocate the TLBMissHandler for CELOG the "fall-through"
        // doesn't happen. So Jump.
        .word   0x08000020              // j 0x80000080                 :31
        nop
#endif
#endif

	.set    reorder
	.set    at
	.end	TLBMissHandler

LEAF_ENTRY(VerifyAccess)
// VerifyAccess(PVOID pvAddr, DWORD dwFlags, ACCESSKEY aky)
//		- verify access to a pointer
//
//	VerifyAccess checks to see if the current thread has access to
// the given address. If bWrite is TRUE, both read & write access are
// verified.
//
//	Entry	(a0) = virtual address to verify
//		(a1) = type of access to verify
//		(a2) = access key to use for validation
//	Return	TRUE - access is OK
//		FALSE - if access is not valid
//	Uses	t0, t1, t2
	.set noreorder
	bltz	a0, 50f			// check system addresses specially
	srl	t0, a0, VA_SECTION-2
	and	t0, SECTION_MASK*4	// (t0) = section * 4
	lw	t0, SectionTable(t0)	// (t0) = ptr to block table
	srl	t1, a0, VA_BLOCK-2
	and	t1, BLOCK_MASK*4	// (t1) = block * 4
	addu	t0, t1			// (t0) = block table entry
	lw	t0, (t0)		// (t0) = ptr to MEMBLOCK structure
	srl	t2, a0, VA_PAGE-2
	bgez	t0, 30f			// unmapped memblock
	and	t2, PAGE_MASK*4		// (t2) = page # * 4
	lw	t1, mb_lock(t0)		// (t1) = block access lock
	addu	t0, t2			//
	lw	t0, mb_pages(t0)	// (t0) = page info
	and	t1, a2			// (t1) = 0 if access not allowed
	beq	zero, t1, 30f		// access not allowed
	and	a1, VERIFY_WRITE_FLAG
	blez	t0, 30f			// invalid entry
	and	t1, t0, 1<<ENTRYLO_D
	and	a0, PAGE_SIZE-1		// (a0) = offset within page
	lui	v0, 0x8000
	or	a0, v0
	lui	v0, 0x1FFF
	ori	v0, ~(PAGE_SIZE-1) & 0xFFFF
#if PFN_SHIFT
	sll	t0, PFN_SHIFT
#endif
	bne	zero, a1, 25f		// check for write access
	and	v0, t0			// (v0) = physical page address
20:	j	ra
	or	v0, a0			// (v0) = unmapped version of pvAddr

25:	bne	zero, t1, 20b		// the page is writable
	nop
30:	j	ra
	move	v0, zero

// The address is in the system address range. If the thread's current mode
// is user mode, then fail the access. Also, disallow access to addresses in
// the system mapped region except for the Kpage.

50:	srl	t2, a0, 12		// (t2) = 4k page addr
	subu	t2, KData >> 12
	beq	zero, t2, 51f		// kpage is in system mapped space
	lui	t2, 0xC000
	sltu	t2, a0, t2		// (t2) = 0 if Addr >= 0xC0000000
	beq	zero, t2, 30b		// access not allowed.
51:	lw	t0, CurThdPtr
	and	a1, VERIFY_KERNEL_OK
	lw	t1, TcxPsr(t0)
	bne	zero, a1, 20b		// kernel access allowed
	move	v0, zero
	and	t1, 1 << PSR_PMODE
	beq	zero, t1, 20b		// Kernel mode thread: access allowed
	nop
	j	ra
	move	v0, zero		// return access failed
	.end	VerifyAccess



LEAF_ENTRY(FlushTLB)

	// Clear out all tlb entries

	.set noreorder

#if R3000
	mfc0    t0, psr		// Save current psr
	mfc0    t1, entryhi     // Save current asid
	andi    v0, t0, 0xFFFE	// Mask interrupts
	mtc0    v0, psr		// Disable interrupts
	lw	v1, OEMTLBSize	// (v1) = index & loop counter
	lw	t2, OEMTLBWired	// (t2) = loop limit
	li      v0, 0x80000000	// Unmapped address

10:	mtc0    zero, entrylo	// Clear entrylo - Invalid entry
	mtc0    v0, entryhi	// Clear entryhi - Ivalid address
	mtc0    v1, index	// Set index
	nop                     // Fill delay slot
	tlbwi                   // Write entry (indexed)
	bne     v1, t2, 10b	// If not zero do next one
	addiu   v1, v1, -256	// Decrement index, loop counter

	mtc0    t1, entryhi	// Restore asid
	mtc0    t0, psr		// Restore psr
	j	ra
	nop

#elif R4000
	mfc0    t0, psr		// Save current psr
	mfc0    t1, entryhi     // Save current asid
	nop
	andi    v0, t0, 0xFFFE	// Mask interrupts
	mtc0    v0, psr		// Disable interrupts
	nop                     // fill delay slot
	li      v0, 0x80000000	// Unmapped address
	lw	v1, OEMTLBSize	// (v1) = index & loop counter
	mfc0	t2, wired	// (t2) = loop limit
10:	mtc0    zero, entrylo0	// Clear entrylo0 - Invalid entry
	mtc0    zero, entrylo1	// Clear entrylo1 - Invalid entry
	mtc0    v0, entryhi	// Clear entryhi - Ivalid address
	mtc0    v1, index	// Set index
	add	v0, 0x2000
	nop                     // Fill delay slot
	tlbwi                   // Write entry (indexed)
	nop
	bne     v1, t2, 10b	// If not zero do next one
	addiu   v1, v1, -1	// Decrement index, loop counter

	mtc0    t1, entryhi	// Restore asid
	mtc0    t0, psr		// Restore psr

	// now flush the ICache and the DCache
    addiu   sp, sp, -4
	sw      ra, 4(sp)      // save ra
	jalfix(FlushICache)
	jalfix(FlushDCache)
	lw      ra, 4(sp)      // restore ra
	j	ra
    addiu   sp, sp, 4
#else
 #error unknown processor type
#endif    // R4000

	.set reorder
	.end	FlushTLB

