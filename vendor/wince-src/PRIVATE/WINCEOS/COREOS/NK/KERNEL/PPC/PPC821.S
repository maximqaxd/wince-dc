//      TITLE("TLB Management and support routines")
//++
//
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
//
// Module Name:
//
//    ppc821.s
//
// Abstract:
//
//    Initialization of the TLB support for the PPC821 kernel
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "mem_ppc.h"
#include "mmu821.h"

//
//  Definition of Special Purpose Registers (SPRs) used on PPC821
//

#define DSISR   18
#define DAR     19
#define IMMR    638

#define INVALID_PAGE    0x2f1

//#define CPU2_ERRATUM	1
//
// See the MPC821 errata for information on the CPU2 hardware issue. The 
// code inside the conditional CPU2_ERRATUM is specific to this issue.
//
#if defined(CPU2_ERRATUM)
// We need a little memory that we can guarantee will always be present.
// This is located in the kernel data structure 'reserved' block.
#define kErrata 0x5AF0
#endif

//
//  The TLB Handlers are defined here and then copied into low memory
//  The four handlers ITBMiss, DTBMiss, ITBError, and DTBError are copied
//  into the correct location for each processor
//  Each vector must be limited to 256 bytes (64 instructions)
//

#define ITBMissVector   0x1100
#define DTBMissVector   0x1200
#define ITBErrorVector  0x1300
#define DTBErrorVector  0x1400

    .text

// Instruction TLB miss handler
//
//      Entry:  (MSR)   External interrupts disabled
//                      Instruction Relocate OFF
//                      Data Relocate OFF
//                  (SRR0)      Instruction address at time of interrupt
//              (SRR1)  MSR at time of interrupt
//
//      Exit:   
//
//      Uses:   M_TW

ITBMiss:

#if defined(CPU2_ERRATUM)
	mtspr	SPRG0, r1
	li		r1, 0x3f80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
	mtspr	M_TW, r0
#else
	mtspr   M_TW, r0                // save r0 for a scratch register
	mtspr   SPRG0, r1
#endif
	mfspr   r1, SRR0                // (r1) = fauling address
	mfcr    r0                      // (r0) = saved condition register
	mtcrf   0x80, r1                // (CR0) = faulting address upper nibble
	mtspr   SPRG3, r3

	bt-     0, itmh50               // high bit of address is set
	mtspr   SPRG1, r4
	mtspr   SPRG2, r5

// Lookup a user virtual address in the kernel virtual memory tables to find the
// physical address translation to be loaded into the TLB.

	rlwinm  r3, r1, (32-VA_SECTION)+2, SECTION_MASK*4 // (r3) = section index
	lwz     r4, SectionTable(r3)    // (r4) = ptr to SECTION
	lwz     r5, CurAKey(0)          // (r5) = current access key
	rlwinm  r3, r1, (32-VA_BLOCK)+2, BLOCK_MASK*4   // (r3) = block index
	rlwinm  r4, r4, 0, 0x1fffffff   // make it physical
	lwzx    r3, r3, r4              // (r3) = ptr to MEMBLOCK structure
	rlwinm  r4, r1, (32-VA_PAGE)+2, PAGE_MASK*4     // (r4) = page index
	rlwinm  r3, r3, 0, 0x1fffffff   // make it physical
	cmplwi  r3, 0x1000
	blt-    itmh40                  // invalid block address
	add     r4, r3, r4
	lwz     r3, mb_lock(r3)         // (r3) = access lock for memory block
	lwz     r4, mb_pages(r4)        // (r4) = PTE entry for the page
	and.    r5, r5, r3
	beq-    itmh40                  // access not allowed
	andi.   r5, r4, PAGE_VALID
	beq-    itmh40                  // invalid entry
itmh30:

#if defined(CPU2_ERRATUM)
	li		r1, 0x2d80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
#endif
	mtspr   MI_RPN, r4              // add entry to the TLB

// New entry loaded into the TLB. Restore registers and continue.
//
//      (r0) = condition register
//      (M_TW) = saved R0
//      (SPRG0) = saved R1
//      (SPRG1) = saved R4
//      (SPRG2) = saved R5
//      (SPRG3) = saved R3

	mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r4, SPRG1
	mfspr   r5, SPRG2
	mfspr   r3, SPRG3
	rfi

// Invalid address reference. Load invalid entry into the TLB and continue.
// This will cause an ITLB error interrupt.
itmh40: mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	
	li      r4, ID_CPAGE_FAULT      // (r4) = ITLB error interrupt code
	mfspr   r5, SRR0                // (r5) = address of fault
	
	ba      KPageGeneralHandler
	
// Address in kernel space. Check if kernel server call or reference to
// an "un-mapped" space.  For an "un-mapped" space reference, generate
// a large (8mb) TLB entry to map the region.  For an api call, jump to
// the initial api handler code in the KPage.
//
//      (r0) = saved CR value
//      (r1) = missing virtual address
//      (M_TW) = saved R0
//      (SPRG0) = saved R1
//      (SPRG3) = saved R3

//
// For CPU2, this is too big, so we moved it to spare space in ITBError
//
#if !defined(CPU2_ERRATUM)
itmh50: 
	lis     r3, API_MAX >> 16
	ori     r3, r3, API_MAX & 0xFFFF
	cmplw   r3, r1
	bgt-    itmKMap                 // not an api call or return

// Setup to invoke the API call or return code.
//
//NOTE: Since this is a C or C++ function call or return, it is not necessary to restore
//      R12 and CR0.

	subi    r11, r1, FIRST_METHOD   // (r11) = iMethod * API_SCALE
	mfspr   r12, SRR1
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	ba      KPageAPIHandler 
	
itmKMap:                
	li      r3, PAGE_8MB | PAGE_VALID
	mtspr   MI_TWC, r3              // setup for 8mb valid page

	rlwinm	r3, r1, 8, 0x1f			// (r3) = 16M page number
	lbz		r3, kPFNMap(r3)			// (r3) = physical page number

	rlwinm	r3, r3, 24, 0xff000000	// Puts physical page number into high byte
	rlwimi	r3, r1, 0, 0x00800000	// Preserve 8M bit

	ori     r3, r3, PAGE_VALID | PAGE_SHARED | 0xf8
	rlwimi  r3, r1, 4, 2            // insert cache-inhibit bit

itmh55:
	mtspr   MI_RPN, r3
	mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	rfi
#endif

    .org    ITBMiss+0x100                       // Fill out the vector

// Data TLB miss handler
//
//      Entry:  (MSR)   External interrupts disabled
//                      Instruction Relocate OFF
//                      Data Relocate OFF
//              (SRR0)  Instruction address at time of interrupt
//              (SRR1)  MSR at time of interrupt
//
//      Exit:   
//
//      Uses:   SPRG3 and M_TW

DTBMiss:
#if defined(CPU2_ERRATUM)
	mtspr   SPRG0, r1
	li		r1, 0x3f80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
	mtspr   M_TW, r0
#else
	mtspr   M_TW, r0                            // save r0 for a scratch register
	mtspr   SPRG0, r1
#endif
	mfspr   r1, MD_EPN                          // (r1) = fauling address
	mfcr    r0                                      // (r0) = saved condition register
	mtcrf   0x80, r1                            // (CR0) = faulting address upper nibble
	mtspr   SPRG3, r3
	bt-         0, dtmh50                       // high bit of address is set
	mtspr   SPRG1, r4
	mtspr   SPRG2, r5

// Lookup a user virtual address in the kernel virtual memory tables to find the
// physical address translation to be loaded into the TLB.

	rlwinm  r3, r1, (32-VA_SECTION)+2, SECTION_MASK*4 // (r3) = section index
	lwz     r4, SectionTable(r3)    // (r4) = ptr to SECTION
	rlwinm  r3, r1, (32-VA_BLOCK)+2, BLOCK_MASK*4   // (r3) = block index
	rlwinm  r4, r4, 0, 0x1fffffff   // make it physical
	lwzx    r3, r3, r4              // (r3) = ptr to MEMBLOCK structure
	rlwinm  r4, r1, (32-VA_PAGE)+2, PAGE_MASK*4     // (r4) = page index
	rlwinm  r3, r3, 0, 0x1fffffff   // make it physical
	cmplwi  r3, 0x1000
	lwz     r1, CurAKey(0)          // (r1) = current access key
	blt-    dtmh40                  // invalid block address
	add     r4, r3, r4
	lwz     r3, mb_lock(r3)         // (r3) = access lock for memory block
	lwz     r4, mb_pages(r4)        // (r4) = PTE entry for the page
	and.    r1, r1, r3
	beq-    dtmh40                  // access not allowed
	andi.   r1, r4, PAGE_VALID
	beq-    dtmh40                  // invalid entry

dtmh30:
#if defined(CPU2_ERRATUM)
	li		r1, 0x3d80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
#endif
    mtspr       MD_RPN, r4              // add entry to the TLB

// New entry loaded into the TLB. Restore registers and continue.
//
//      (r0) = condition register
//      (M_TW) = saved R0
//      (SPRG0) = saved R1
//      (SPRG1) = saved R4
//      (SPRG2) = saved R5
//      (SPRG3) = saved R3

	mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r4, SPRG1
	mfspr   r5, SPRG2
	mfspr   r3, SPRG3
	rfi

// Invalid address reference. Load invalid entry into the TLB and continue.
// This will cause an ITLB error interrupt.

dtmh40:
	mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3

	li      r4, ID_DPAGE_FAULT      // (r4) = ITLB error interrupt code
	mfspr   r5, DAR
		
	ba      KPageGeneralHandler

// Address in kernel space. For an "un-mapped" space reference, generate
// a large (8mb) TLB entry to map the region.
//
// Should the uncached space also be marked as guarded?
//
//      (r0) = saved CR value
//      (r1) = missing virtual address
//      (M_TW) = saved R0
//      (SPRG0) = saved R1
//      (SPRG3) = saved R3

//
// For CPU2, this is moved to DTBError
//
#if !defined(CPU2_ERRATUM)
dtmh50:
    bt-     1, dtmh40                               // Bit 30 set - Invalid address
	li      r3, PAGE_8MB | PAGE_VALID
	mtspr   MD_TWC, r3                                  // setup for 8mb valid page

	rlwinm	r3, r1, 8, 0x1f			// (r3) = 16M page number
	lbz		r3, kPFNMap(r3)			// (r3) = physical page number

	rlwinm	r3, r3, 24, 0xff000000	// Puts physical page number into high byte
	rlwimi	r3, r1, 0, 0x00800000	// Preserve 8M bit

	ori     r3, r3, PAGE_VALID | PAGE_SHARED | 0x1f8
	rlwimi  r3, r1, 4, 2            // insert cache-inhibit bit

dtmh55:
    mtspr   MD_RPN, r3
	mtcrf   0xff, r0                                    // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	rfi
#endif

    .org    DTBMiss+0x100                       // Fill out the vector

// Instruction TLB error handler
//
//      Entry:  (MSR)   External interrupts disabled
//                      Instruction Relocate OFF
//                      Data Relocate OFF
//              (SRR0)  Instruction address at time of interrupt
//              (SRR1)  MSR at time of interrupt
//

ITBError:               
	mtspr   SPRG1, r4
	mtspr   SPRG2, r5
	li      r4, ID_CPAGE_FAULT      // (r4) = ITLB error interrupt code
	mfspr   r5, SRR0
	ba      KPageGeneralHandler     

//
// For CPU2, we moved this from ITBMiss because the handler got
// too big.
//
#if defined(CPU2_ERRATUM)
itmh50: 
	lis     r3, API_MAX >> 16
	ori     r3, r3, API_MAX & 0xFFFF
	cmplw   r3, r1
	bgt-    itmKMap                 // not an api call or return

// Setup to invoke the API call or return code.
//
//NOTE: Since this is a C or C++ function call or return, it is not necessary to restore
//      R12 and CR0.

	subi    r11, r1, FIRST_METHOD   // (r11) = iMethod * API_SCALE
	mfspr   r12, SRR1
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	ba      KPageAPIHandler 
	
itmKMap:                
	li      r3, PAGE_8MB | PAGE_VALID
	li		r1, 0x2b80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
	mtspr   MI_TWC, r3              // setup for 8mb valid page

	rlwinm	r3, r1, 8, 0x1f			// (r3) = 16M page number
	lbz		r3, kPFNMap(r3)			// (r3) = physical page number

	rlwinm	r3, r3, 24, 0xff000000	// Puts physical page number into high byte
	rlwimi	r3, r1, 0, 0x00800000	// Preserve 8M bit

	ori     r3, r3, PAGE_VALID | PAGE_SHARED | 0xf8
	rlwimi  r3, r1, 4, 2            // insert cache-inhibit bit

itmh55:
	li		r1, 0x2d80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
	mtspr   MI_RPN, r3
	mtcrf   0xff, r0                // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	rfi
#endif

   .org   ITBError + 0x100      

	
//  Data TLB error handler
//
//      Entry:  (MSR)   External interrupts disabled
//                      Instruction Relocate OFF
//                      Data Relocate OFF
//              (DAR)   Instruction address at time of interrupt
//              (SRR1)  MSR at time of interrupt
//

	
DTBError:

	mtspr   SPRG1, r4
	mtspr   SPRG2, r5
	li      r4, ID_DPAGE_FAULT
	mfspr   r5, DAR
	ba      KPageGeneralHandler     // Call the general exception handler

//
// Moved from DTBMiss because DTBMiss got too big
//
#if defined(CPU2_ERRATUM)
dtmh50:
    bt-     1, dtmh40                               // Bit 30 set - Invalid address
	li		r3, 0x3b80
	stw		r3, kErrata(r0)
	lwz		r3, kErrata(r0)
	li      r3, PAGE_8MB | PAGE_VALID
	mtspr   MD_TWC, r3                                  // setup for 8mb valid page

	rlwinm	r3, r1, 8, 0x1f			// (r3) = 16M page number
	lbz		r3, kPFNMap(r3)			// (r3) = physical page number

	rlwinm	r3, r3, 24, 0xff000000	// Puts physical page number into high byte
	rlwimi	r3, r1, 0, 0x00800000	// Preserve 8M bit

	ori     r3, r3, PAGE_VALID | PAGE_SHARED | 0x1f8
	rlwimi  r3, r1, 4, 2            // insert cache-inhibit bit

dtmh55:
	li		r1, 0x3d80
	stw		r1, kErrata(r0)
	lwz		r1, kErrata(r0)
    mtspr   MD_RPN, r3
	mtcrf   0xff, r0                                    // restore condition register
	mfspr   r0, M_TW
	mfspr   r1, SPRG0
	mfspr   r3, SPRG3
	rfi
#endif

    .org    DTBError+0x100

//
//  TLBInit
//
//  Perform the processor specific TLB Initialization:
//
//  Any processor specific TLB initialization is performed here, and two
//  fixed translations are created:
//  
//  1) KSEG Mapping. All address with bit 0 set are mapped 1-to-1
//
//  2) KPage Mapping. The KPAGE is mapped Virtual=Physical. The mapping
//                    is writable and executable in kernel mode, but
//                    read-only in user mode
//
//  On Entry:
//		(r3)	Physical address of OEMMemoryMap
//

    LEAF_ENTRY(TLBInit)

// Translate OEMMemoryMap to KPfnMap
//	(r4)	Virtual Address
//	(r5)	Physical Address
//	(r6)	Size

KPFNMapStart:

	// Get entry size
	lwz		r6, 8(r3)

	// Divide size by 16.  This gives us the number of 
	// KPFN entries covered.
	srawi	r6, r6, 4

	// If the size is zero, we're done
	cmpi	0, 0, r6, 0
	beq		KPFNMapDone
		
	// Get physical and virtual address
	lwz		r4, 0(r3)
	lwz		r5, 4(r3)

	// For each address, rotate the upper byte into the low byte 
	// and mask it off.
    rlwinm  r4, r4, 8, 0x0000001F       // KSEG address high byte.  Bits 31-29 are always 100.
    rlwinm  r5, r5, 8, 0x000000FF       // Physical address high byte.

	// Now fill the KPFN entries
	mtctr	r6
FillLoop:
	stb		r5, kPFNMap(r4)
	addi	r4, r4, 1
	addi	r5, r5, 1
	bdnz FillLoop
	
	// Next entry...
	addi	r3, r3, 12
	b		KPFNMapStart

KPFNMapDone:

    lis r3, 0xF000
	mtspr   SPRG0, r3               // setup "stack" for nested TLB exceptions
	tlbia                           // clear out all TLB entries
	li      r3, 0x5555
	oris    r3, r3, 0x5555
	mtspr   MI_AP, r3               // all instr. access based on Page tables
	mtspr   MD_AP, r3               // all data access based on Page tables

// Load reserved entry for KPage into the Data TLB.
	li      r3, 0x1f00
	mtspr   MD_CTR, r3              // select entry #31
	li      r3, KPAGE_BASE | 0x200
	mtspr   MD_EPN, r3
	li      r3, PAGE_VALID
	mtspr   MD_TWC, r3
	li      r3, KPAGE_BASE|PG_VALID_MASK|PG_GLOBAL_MASK|PG_CACHE|PG_PROT_URO_KRW|PG_DIRTY_MASK
	mtspr   MD_RPN, r3              // load the entry
	lis     r3, 0x0C00
	mtspr   MD_CTR, r3              // set RSV4D bit to lock the entry

    LEAF_EXIT(TLBInit)


//
//  FlushTLB
//
//  Flush all TLB entries.
//

    LEAF_ENTRY(FlushTLB)

	isync                                       // Sync the I-stream
    tlbia                                       // Invalidate all entries
    isync                                       // Re-Sync the I-stream

    LEAF_EXIT(FlushTLB)



    LEAF_ENTRY(SetCASID)
// SetCASID - set M_CASID register
//
//      This function is needed because the C compiler won't accept registers
// it doesn't know about in the _sregister_set() call.
//
//      Entry   (r3) = new address space ID
//      Exit    none
//      Uses    none
#if defined(CPU2_ERRATUM)
	li		r4, 0x3380
	stw		r4, kErrata(r0)
	lwz		r4, kErrata(r0)
#endif
	mtspr   M_CASID, r3
    LEAF_EXIT(SetCASID)



//
//  LEAF_ENTRY(CPUIdentify)
//
//  Returns information about the current processor.
//
//  Entry   none
//  Exit    (r3) = Processor type (821 or 823)
//
    LEAF_ENTRY(CPUIdentify)

    // Detect PPC 821 vs. PPC823 using the IMMR.  The Partnum field
    //   (IMMR[16:23]) on the PPC 823 is 0x20.  On the PPC 821, it's 0.
    mfspr   r4, IMMR                    // Get IMMR
    rlwinm. r4, r4, 0, 0x00002000       // Mask off the important bit
    li      r3, 821                     // Assume 821
    beq     ProcType821                 // Bit is clear so have an 821

ProcType823:                            // Got an 823
    li      r3, 823

ProcType821:

    LEAF_EXIT(CPUIdentify)
