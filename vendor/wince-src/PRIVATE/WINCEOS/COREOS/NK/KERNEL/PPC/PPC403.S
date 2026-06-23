//      TITLE("TLB Management and support routines")
//++
//
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// Module Name:
//
//    ppc403.s
//
// Abstract:
//
//    Initialization of the TLB support for the PPC403 kernel
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

.machine ppc403gc

#define UNCACHED    0                   // Disable caching

//
//  Definition of Special Purpose Registers (SPRs) used on PPC403
//

#define ESR     0x3d4                       // Exception Status Register
#define DAR     0x3d5                       // == DEAR on PPC403

//
//  The TLB Handlers are defined here and then copied into low memory
//  The four handlers ITBMiss, DTBMiss, ITBError, and DTBError are copied
//  into the correct location for each processor
//  Each vector must be limited to 256 bytes (64 instructions)
//

#define DTBErrorVector  0x0300
#define ITBErrorVector  0x0400
#define DTBMissVector   0x1100
#define ITBMissVector   0x1200

#define ClockIntVector  0x1000

//
//  The PPC403 uses a software TLB replacement algorithm. This is currently
//  a simple round robin method. There are 64 (unified) TB entries on the 403,
//  and two are reserved for the KPAGE and KSEG mappings.
//

#define TB_ENTRIES  62                      // # of replaceable TB Entries
#define TB_KSEG     62                      // TB Entry for Kernel mapping
#define TB_KPAGE    63                      // TB Entry for KPage mapping


//
//  The following TLB registers are used within the TLB routines
//
	
#define PID     0x3B1
#define ZPR     0x3B0
#define DEAR    0x3D5

#undef  DAR
#define DAR     DEAR                        // 403 doesn't use architected DAR
	
//
//  Storage access control is maintained via the Zone Protection register.
//  Each TLB entry contains a ZSEL field which is an index into one of 16
//  storage control zones. For Wince, three zones are defined:
//
//  Zone 0: Normal mapping          All access controlled by PTE Wr & Ex fields
//  Zone 1: KSEG mapping            Kernel-only Read, Write and Execute
//  Zone 2: KPAGE mapping           Kernel rwx, User Mode Read/Execute only
//
//  The ZSEL field is in bits 24:27 of the TLBLO register. Since only three
//  zones are defined for Windows CE, only two bits are required to identify
//  the zone. The ZPR is setup so that the top two bits of the ZSEL field are
//  ignored. The upper two bits of the ZSEL field (bits 24:25) are used as
//  software PTE flags
//

#define PTE_ZONE0   0x0000                      // PTE Field Bits for Zone 0
#define PTE_ZONE1   0x0010
#define PTE_ZONE2   0x0020

#define ZPR_ZONE0   0x40000000                  // ZPR<0> = 01b
#define ZPR_ZONE1   0x00000000                  // ZPR<1> = 00b
#define ZPR_ZONE2   0x08000000                  // ZPR<2> = 10b

#define ZPR_INIT    0x48484848                  // Zone selection mapping

#define PTE_VALID   0x0040                      // Valid bit in PTEHI
#define PTE_LE      0x0020                      // Little endian bit in
                                                //   PTEHI on PPC401; on
                                                //   PPC403 it's reserved, but
                                                //   okay to write to

    .text

//
// Data TLB miss handler
//
//    Entry:    (MSR)   External interrupts disabled
//                  Instruction Relocate OFF
//                  Data Relocate OFF
//            (SRR0)    Instruction address at time of interrupt
//          (SRR1)    MSR at time of interrupt
//          (DEAR)  Effective address of the failed access
//
//    Usage:    
//      r3  == Condition register           (r3 => SPRG0)
//      r4  == TLB Low                      (r4 => SPRG1)
//      r5  == Exception address (TLBHI)    (r5 => SPRG2)
//      r6  == misc. pointer                (r6 => SPRG3)
//
//  This routine sets up registers and branches to the common TB fill code
//

DTBMiss:

    mtspr   SPRG2, r5
    mfspr   r5, DEAR                       // (r5) = faulting address

    mtspr   SPRG0, r3
    mfcr    r3                             // (r3) = saved condition register
    mtcrf   0x80, r5                       // (CR0) = Upper nibble of EA

    mtspr   SPRG1, r4
    mtspr   SPRG3, r6

    bt-     0, TBKMap                       // else create a KSEG mapping

//
//  Common TB Fill handler used by ITBMiss and DTBMiss.
//  Note: This code is duplicated, but identical, between ITBMISS & DTBMiss
//
//      r5  == Effective address which caused the miss
//

DTBFill:                                     // Common UTB Fill code:

//
// Lookup a user virtual address in the kernel virtual memory tables to find the
// physical address translation to be loaded into the TLB.
//

    rlwinm  r6, r5, (32-VA_SECTION)+2, SECTION_MASK*4   // (r6) = section index
    lwz     r4, SectionTable(r6)                        // (r4) = ptr to SECTION
    rlwinm  r6, r4, 8, 27, 31                           // Make r4 physical by
    lbz     r6, kPFNMap(r6)                             //   doing V->P memory
    rlwimi  r4, r6, 24, 0xFF000000                      //   map lookup

    rlwinm  r6, r5, (32-VA_BLOCK)+2, BLOCK_MASK*4       // (r6) = block index

    lwzx    r6, r6, r4                                  // -> MEMBLOCK structure

    cmplwi  r6, 0x1000                                  // Invalid MEMBLOCK ??
    rlwinm  r4, r6, 8, 27, 31                           // Extract PFN
    blt-    DTBInvalid

    //
    //  memblock[0] will reside in ROM, other memblock entries may reside in
    //  RAM. The PFNMap must be used to properly retreive the physical address
    //  of the memblock structure.
    //

    lbz     r4, kPFNMap(r4)                             // Load translated PFN
    rlwimi  r6, r4, 24,0xFF000000                       // make it physical

    lwz     r4, CurAKey(0)                              // (r4) = current AKey

    lwz     r5, mb_lock(r6)                             // MEMBLOCK access lock
    and.    r4, r4, r5
    mfspr   r5, DEAR                                    // restore EA
    beq-    DTBInvalid                                  // access not allowed

    rlwinm  r4, r5, (32-VA_PAGE)+2, PAGE_MASK*4         // (r4) = page index
    add     r4, r6, r4
    lwz     r4, mb_pages(r4)                            // (r4) = PTE entry
    andi.   r6, r4, 0x01                                // Test Valid bit.
    beq-    DTBInvalid                                  // Invalid access

    rlwinm  r5, r5, 0, 0, 21                            // Clear out page offset

//  End of Common fill code.

    b       TBUpdate                                // go to common update code

//
//  Invalid address.
//  Set up parameters and branch to the General exception handler.
//
//  (r5) == Exception address
//

DTBInvalid:

    li      r4, ID_DPAGE_FAULT                      // Exception type in r4

    mtcr    r3                                      // restore saved parms
    mfspr   r3, SPRG0
    mfspr   r6, SPRG3

    ba      KPageGeneralHandler

//
//  Kernel space address. Create a large (16MB) page mapping for the address:
//  TODO: Set PID to zero and make this a global translation.
//

TBKMap:

//    bt-     1, DTBInvalid                           // Invalid 0xC quadrant
    rlwinm. r4, r5, 0, 1, 1                         // Isolate EA bit 30
    bne     DTBInvalid                              // Invalid if bit 30 set

    rlwinm  r5, r5, 0, 0, 7                     // Align to 16MBytes
    ori     r5, r5, 0x3c0                       // Insert Valid, Size=16MB

    rlwinm  r4, r5, 8, 27, 31                   // Extract PFN
	lbz     r4, kPFNMap(r4)					    // Load translated PFN
	slwi    r4, r4, 24                          // align to 16MBytes
	rlwimi  r4, r5, 5, 29, 29                   // Copy Uncached Bit from VA

    ori     r4, r4, (PTE_ZONE1 | PG_PROT_WRITE | PG_PROT_EXECUTE) 

    b       TBUpdate

    .org    DTBMiss+0x100                       // Fill out the vector


//
// Instruction TLB miss handler
//
//    Entry:    (MSR)   External interrupts disabled
//                      Instruction Relocate OFF
//                      Data Relocate OFF
//              (SRR0)  Instruction address at time of interrupt
//              (SRR1)  MSR at time of interrupt
//
//    Usage:    
//      r3  == Condition register           (r3 => SPRG0)
//      r4  == TLB Low                      (r4 => SPRG1)
//      r5  == Exception address (TLBHI)    (r5 => SPRG2)
//      r6  == misc. pointer                (r6 => SPRG3)
//

ITBMiss:

    mtspr   SPRG2, r5
    mfspr    r5, SRR0                       // (r5) = fauling address

    mtspr    SPRG0, r3
    mfcr     r3                             // (r3) = saved condition register
    mtcrf    0x80, r5                       // (CR0) = Upper nibble of EA

    mtspr    SPRG1, r4
    mtspr    SPRG3, r6

    bt-        0, TBKSpace                  // high bit set, kernel space addr

//
//  Common TB Fill handler used by ITBMiss and DTBMiss.
//  Note: This code is duplicated, but identical, between ITBMISS & DTBMiss
//
//      r5  == Effective address which caused the miss
//

ITBFill:                                    // Common UTB Fill code:

//
// Lookup a user virtual address in the kernel virtual memory tables to find the
// physical address translation to be loaded into the TLB.
//

    rlwinm  r6, r5, (32-VA_SECTION)+2, SECTION_MASK*4   // (r6) = section index
    lwz     r4, SectionTable(r6)                        // (r4) = ptr to SECTION
    rlwinm  r6, r5, (32-VA_BLOCK)+2, BLOCK_MASK*4       // (r6) = block index
    rlwinm  r4, r4, 0, 0x0fffffff                       // make it physical
    lwzx    r6, r6, r4                                  // -> MEMBLOCK structure

    rlwinm  r6, r6, 0, 0x0fffffff                       // make it physical
	cmplwi	r6, 0x1000                                  // Invalid MEMBLOCK ??
    lwz     r4, CurAKey(0)                              // (r4) = current AKey
    blt-    ITBInvalid

    lwz     r5, mb_lock(r6)                             // MEMBLOCK access lock
    and.    r4, r4, r5
    mfspr   r5, SRR0                                    // restore EA
    beq-    ITBInvalid                                  // access not allowed

    rlwinm  r4, r5, (32-VA_PAGE)+2, PAGE_MASK*4         // (r4) = page index
    add     r4, r6, r4
    lwz     r4, mb_pages(r4)                            // (r4) = PTE entry
    andi.   r6, r4, 0x01                                // Test Valid bit.
    beq-    ITBInvalid                                  // Invalid access

    rlwinm  r5, r5, 0, 0, 21                            // Clear out page offset

//  End of Common fill code.

//
//  Load a new entry into the TB. 
//
//      (r5)    == TLBHI  (EPN and size fields)
//      (r4)    == TLBLO  (RPN, Wr, Ex, Zone, and cache-inhibit fields)
//                 The cache-inhibit bit is copied to the Guarded bit.
//

TBUpdate:

    lwz     r6, TBIndex(0)                          // Get next TB index
#if UNCACHED 
    rlwinm  r4, r4, 0, 0, 30    // TESTING: Clear valid (guarded) bit
    ori     r4, r4, PG_NOCACHE
#else
    rlwimi  r4, r4, 32-2, 31, 31                    // Copy Inhibit to Guarded
#endif
    tlbwelo r4, r6                                  // write entry low part

    ori     r5, r5, PTE_VALID | PTE_LE              // Insert valid, little
                                                    //   endian bits
    tlbwehi r5, r6                                  // write entry hi part
    addi    r6, r6, 1                               // bump TB index
    cmpwi   r6, TB_ENTRIES
    blt     TBExit
    li      r6, 0                                   // reset TB index
TBExit:
    stw     r6, TBIndex(0)                          // update the index

//
// New entry loaded into the TLB. Restore registers and continue.
//
//    (r0) = condition register
//    (SPRG0) = saved R3
//    (SPRG1) = saved R4
//    (SPRG2) = saved R5
//    (SPRG3) = saved R6

    mtcrf    0xff, r3                            // restore condition register
    mfspr    r3, SPRG0
    mfspr    r4, SPRG1
    mfspr    r5, SPRG2
    mfspr    r6, SPRG3
    rfi

//
// Address in kernel space. Check if kernel server call or reference to
// an "un-mapped" space.  For an "un-mapped" space reference, generate
// a large (16mb) TLB entry to map the region.  For an api call, jump to
// the initial api handler code in the KPage.
//
//    (r3) = saved CR value
//    (r5) = missing virtual address
//    (M_TW) = saved R0
//    (SPRG0) = saved R3
//    (SPRG1) = saved R4
//    (SPRG2) = saved R5
//    (SPRG3) = saved R6
//

TBKSpace:

	lis     r6, API_MAX >> 16
	ori     r6, r6, API_MAX & 0xFFFF
	cmplw	r6, r5
	bgt-	TBKMap 			                    // not an api call or return

//
// Setup to invoke the API call or return code.
//
// NOTE: Since this is a C or C++ function call or return,
//       it is not necessary to restore r11, r12, and CR0.
//

    subi    r11, r5, FIRST_METHOD               // FIRST_METHOD based index
    mfspr   r12, SRR1
    mfspr   r3, SPRG0
    mfspr   r4, SPRG1
    mfspr   r5, SPRG2
    mfspr   r6, SPRG3
    ba      KPageAPIHandler

//
//  Invalid address.
//  Set up parameters and branch to the General exception handler.
//
//  (r5) == Exception address
//

ITBInvalid:

    li      r4, ID_CPAGE_FAULT                      // Exception type in r4

    mtcr    r3                                      // restore saved parms
    mfspr   r3, SPRG0
    mfspr   r6, SPRG3

    ba      KPageGeneralHandler

    .org    ITBMiss+0x100                           // Fill out the vector


//
// Instruction TLB error handler
//
//	Entry:	(MSR)   External interrupts disabled
//              	Instruction Relocate OFF
//              	Data Relocate OFF
//		(SRR0)	Instruction address at time of interrupt
//      	(SRR1)	MSR at time of interrupt
//
//

ITBError:		
	mtspr	SPRG1, r4
	mtspr	SPRG2, r5
	li	r4, ID_CPAGE_FAULT	// (r4) = ITLB error interrupt code
	mfspr	r5, SRR0
	ba	KPageGeneralHandler	

//   .org   ITBError + 0x100	

	
//  Data TLB error handler
//
//	Entry:	(MSR)   External interrupts disabled
//              	Instruction Relocate OFF
//              	Data Relocate OFF
//		(DEAR)	Instruction address at time of interrupt
//      	(SRR1)	MSR at time of interrupt
//

	
DTBError:

	mtspr	SPRG1, r4
	mtspr	SPRG2, r5
	li	r4, ID_DPAGE_FAULT
	mfspr	r5, DEAR
	ba	KPageGeneralHandler     // Call the general exception handler

//    .org    DTBError+0x100
	
//
//  ClockHandler
//
//  The clock interrupts are routed on the PPC403 so that the PIT, FIT, and
//  watchdog timers all dispatch to the 0x10xx exception vectors. Since there
//  is not an architected PowerPC Timer interrupt, the kernel will simply
//  convert these to hardware interrupts, and dispatch them via
//  OEMInterruptHandler.
//
//

ClockHandler:

    ba      0x0500                      // Offset 0x1000    - PIT Interrupt
    nop
    nop
    nop
    ba      0x0500                      // Offset 0x1010    - FIT Interrupt
    nop
    nop
    nop
    ba      0x0500                      // Offset 0x1020    - Watchdog int.

ClockHandlerEnd:


//
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
//
// Parameters:
//
// r3 - Address of OEMMemoryMap of Physical -> Virtual mappings
//

    LEAF_ENTRY(TLBInit)

// 
// Build up the Virtual to Physical translation table for the 512MB superpage.
// This table is based upon the Physical to Virtual mappings stored in
// OEMMemoryMap.
// 
// 

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

//
//	Init TLBs
//

    tlbia                                       // clear out all TLB entries

//
//  Set up the Zone Protection Register (ZPR) for the zones used by
//  the TLB entries
//

    LWI(r3, ZPR_INIT)
    mtspr   ZPR, r3

    li      r3, 0
    mtspr   PID, r3                             // Setup PID for global mappings
    isync

//
// Load reserved entry for KPage into the Data TLB.
// The KPage is mapped so that <KPAGE Virtual> == <KPAGE Physical> == KPAGE_BASE
// The KPage is actually 4KB (four one-K pages) mapped as a single page
//
//  Map user mode only for the 2 pages at 0x5800 (requires 2 more tlbs)
//

    li      r4, TB_KPAGE                        // load TB index
    li      r3, (KPAGE_BASE | PG_PROT_EXECUTE | PTE_ZONE2)
#if UNCACHED
    ori     r3, r3, PG_NOCACHE      // Disable kpage caching for testing
#endif
    tlbwelo r3, r4                              // write low part

    li      r3, (KPAGE_BASE | 0xc0)             // Set EPN, Valid, Size=4KB
    tlbwehi r3, r4                              // write high part
    
    isync                                       // Sync the istream

    LEAF_EXIT(TLBInit)


//
//  FlushTLB
//
//  Flush all TLB entries.
//
//  Use tlbia to invalidate all entries, and then refill the reserved KPage
//  translation
//

    LEAF_ENTRY(FlushTLB)

    //
    //  Prevent interrupt from occuring between tlbia and the
    //  KPage translation update:
    //
    
    mfmsr   r6
	rlwinm	r0, r6, 0, ~MSR_EE
    mtmsr   r0

    //
    //  Set PID to zero. KSeg TBMiss following tlbia will be global
    //

    mfspr   r5, PID                             // save the current PID value
    li      r3, 0                               // set PID=0 for shared entry
    mtspr   PID, r3
    isync

    tlbia                                       // Invalidate all entries
    isync                                       // Ensure ITLB consistent

    //
    //  Reset the valid bit for the reserved Kpage translation:
    //  PID == 0. This is a global translation:
    //

    li      r4, TB_KPAGE                        // KPage translation index
    tlbrehi r3, r4                              // read the translation
    ori     r3, r3, PTE_VALID | PTE_LE          // set the valid and little
                                                //   endian bits
    tlbwehi r3, r4                              // and update the entry

    mtspr   PID, r5                             // restore original PID
    isync
    mtmsr   r6                                  // restore MSR

    LEAF_EXIT(FlushTLB)




// SetPID - set PID register
//
// This function is needed because the C compiler won't accept registers
// it doesn't know about in the _sregister_set() call.
//
//	Entry	(r3) = new address space ID
//	Exit	none
//	Uses	none

    LEAF_ENTRY(SetPID)

    isync
    mtspr PID, r3
    isync

    LEAF_EXIT(SetPID)



//
//  LEAF_ENTRY(CPUIdentify)
//
//  Returns information about the current processor.
//
//  Entry   none
//  Exit    (r3) = Processor type (401 or 403)
//
    LEAF_ENTRY(CPUIdentify)

    //  The MEM (Family Member) field of the PVR (PVR[12:15]) is used to
    //  determine if we have a 401 or 403.
    //  MEM=0x1  =>  401
    //  MEM=0x0  =>  403
    //
    //  PVR          CPU
    //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //  0x00200200 - 403GC
    //  0x00201400 - 403GCX
    //  0x0022xxxx - 401B2
    //  0x0023xxxx - 401C2
    //  0x0024xxxx - 401D2

    mfspr   r4, PVR                     // Grab the PVR
    rlwinm. r4, r4, 0, 0x00070000       // PVR[12:15]
    li      r3, 403                     // Assume 403
    beq     ProcType403                 // MEM == 0 so have a 403

ProcType401:                            // Have a 401
    li      r3, 401

ProcType403:

    LEAF_EXIT(CPUIdentify)
