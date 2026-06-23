//------------------------------------------------------------------------------
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
//------------------------------------------------------------------------------
//
//  Module Name:  
//  
//      celogisr.s
//  
//  Abstract:  
//
//      Implements the event logging functions for MIPS interrupt service
//      routines.
//      
//------------------------------------------------------------------------------
#include "ksmips.h"



	.data
	
Save_t:	.space	4*12

	.text

LEAF_ENTRY(CeLogInterruptMIPS)
	.set noreorder
	la	v0, Save_t		// save temps
	sw	t0, 0(v0)
	sw	t1, 4(v0)
	sw	t2, 8(v0)
	sw	t3,12(v0)
	sw	t4,16(v0)
	sw	t5,20(v0)
	sw	t6,24(v0)
	sw	t7,28(v0)
	sw	ra,32(v0)
	sw	t8,36(v0)
	sw	t9,40(v0)
	sw	v1,44(v0)

	jal	CeLogInterrupt
	nop

	la	v0, Save_t		// restore temps
	lw	t0, 0(v0)
	lw	t1, 4(v0)
	lw	t2, 8(v0)
	lw	t3,12(v0)
	lw	t4,16(v0)
	lw	t5,20(v0)
	lw	t6,24(v0)
	lw	t7,28(v0)
	lw	ra,32(v0)
	lw	t8,36(v0)
	lw	t9,40(v0)
	lw	v1,44(v0)
	
        j	ra
	nop
	.end


//------------------------------------------------------------------------------
//
// SetCPUASID calls this routine.  On entry, A0 = hProc.
//
//------------------------------------------------------------------------------
LEAF_ENTRY(CELOG_ThreadMigrateMIPS)
	.set noreorder
       	
        subu	sp,52           	// adjust sp
	sw	ra, 0(sp)               // save RA
	sw	t0, 4(sp)               // save temps
	sw	t1, 8(sp)
	sw	t2,12(sp)
	sw	t3,16(sp)
	sw	t4,20(sp)
	sw	t5,24(sp)
	sw	t6,28(sp)
	sw	t7,32(sp)
	sw	t8,36(sp)
	sw	t9,40(sp)
	sw	v0,44(sp)
	sw	v1,48(sp)

        move    a1, zero                // (a1) = 0
        
        //
        // Call the C function, CeLogThreadMigrate
        //
        subu	sp, 8           	// leave space for args
        jal	CeLogThreadMigrate
        nop
       	addu	sp, 8           	// remove arg space
	
        lw	ra, 0(sp)               // restore RA
	lw	t0, 4(sp)               // restore temps
	lw	t1, 8(sp)
	lw	t2,12(sp)
	lw	t3,16(sp)
	lw	t4,20(sp)
	lw	t5,24(sp)
	lw	t6,28(sp)
	lw	t7,32(sp)
	lw	t8,36(sp)
	lw	t9,40(sp)
	lw	v0,44(sp)
	lw	v1,48(sp)
       	addu	sp,52           	// adjust sp
	
        j	ra
	nop
	.end

