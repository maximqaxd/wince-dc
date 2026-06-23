/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*
    mdmem.h - machine dependant memory management structures
*/

#ifndef MEM_ARM_H
#define MEM_ARM_H

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

#define BLOCK_MASK      0x1FF
#define SECTION_MASK	0x03F
#define RESERVED_SECTIONS 1     // reserve section 0 for current process

// Bit offsets of page, block & section in a virtual address:
#define VA_BLOCK        16
#define VA_SECTION      25

#if defined(ARM720)		// uses 4K page tables
#define VA_PAGE         12
#define L2_MASK			0xFF	// For a 4K page size (small pages)
#define PAGE_SIZE 		4096
#elif defined(SA1100)		// uses 4K page tables
#define VA_PAGE         12
#define L2_MASK			0xFF	// For a 4K page size (small pages)
#define PAGE_SIZE 		4096
#elif defined(ARM920)	// uses 1K page tables
#define VA_PAGE         10
#define L2_MASK			0x3FF
#define PAGE_SIZE		1024
#endif

#define PAGES_PER_BLOCK  (0x10000 / PAGE_SIZE)





/* Page permission bits */
#define PG_PERMISSION_MASK  0x00000FFF
#define PG_PHYS_ADDR_MASK   0xFFFFF000    
#define PG_VALID_MASK       0x00000002
#define PG_1K_MASK			0x00000001
#define PG_CACHE_MASK       0x0000000C
#define PG_GUARD	        0x0000000C
#define PG_CACHE            0x0000000C
#define PG_MINICACHE		0x00000008
#define PG_NOCACHE          0x00000000
#define PG_DIRTY_MASK       0x00000010
#define PG_EXECUTE_MASK     0x00000000  /* not supported by HW */

#define PG_PROTECTION       0x00000FF0
#define PG_PROT_READ        0x00000000
#define PG_PROT_WRITE       0x00000FF0
#define PG_PROT_URO_KRW		0x00000AA0
#define PG_SIZE_MASK		0x00000000

#if defined(ARM720)
#define PG_READ_WRITE       (PG_VALID_MASK | PG_DIRTY_MASK | PG_CACHE | PG_PROT_WRITE)
#elif defined(SA1100)
#define PG_READ_WRITE       (PG_VALID_MASK | PG_DIRTY_MASK | PG_CACHE | PG_PROT_WRITE)
#elif defined(ARM920)
#define PG_READ_WRITE		(PG_1K_MASK | PG_VALID_MASK | PG_DIRTY_MASK | PG_CACHE | PG_PROT_WRITE)
#endif 

#define INVALID_POINTER_VALUE	0xD0000000

#define UNKNOWN_BASE    (~0)
#define BAD_PAGE        (~0x0fUL)

#ifndef VERIFY_READ_FLAG
#define VERIFY_READ_FLAG    0
#define VERIFY_EXECUTE_FLAG 0
#define VERIFY_WRITE_FLAG   1
#define VERIFY_KERNEL_OK    2
#endif

/* Test the write access on a page table entry */
#define IsPageWritable(entry)	(((entry)&(PG_PROTECTION|PG_DIRTY_MASK)) \
		== (PG_PROT_WRITE|PG_DIRTY_MASK))
#define IsPageReadable(entry)	(((entry)&PG_VALID_MASK) == PG_VALID_MASK)

#define SETAKY(pth,val) (CurAKey = (pth)->aky = (val))

#define PFN_INCR  PAGE_SIZE
/* Find the page frame # from a GPINFO pointer */
#define GetPFN(pgpi)  ((FirstPT[(ulong)(pgpi)>>20]&0xFFF00000) \
						| ((ulong)(pgpi)&0x000FF000))
#define NextPFN(pfn)  ((pfn) + PFN_INCR)
#define PFNfromEntry(entry)   ((entry) & 0xFFFFF000)
#define PFNfrom256(pg256)	((ulong)(pg256)<<8 & ~(PAGE_SIZE-1))

PVOID Phys2Virt(DWORD pfn);

#define KPAGE_PTE	0	// Not used on ARM

#endif // MEM_ARM_H

