/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*
    mdmem.h - machine dependant memory management structures
*/

#ifndef MEM_MIPS_H
#define MEM_MIPS_H

#if R4100
#define PAGE_SIZE 4096		/* R4100 page size can be 1024 or 4096 */
#else
#define PAGE_SIZE 4096		/* R3000 & R4000 page size */
#endif

//++
// These symbols are duplicated from kernel.h so they can
// be used by the MIPS assembler code without including kernel.h

// Bit offsets of block & section in a virtual address:
#define VA_BLOCK        16
#define VA_SECTION      25
#if PAGE_SIZE==4096
#define VA_PAGE         12
#define PAGE_MASK       0x00F
#else
#define VA_PAGE			10
#define PAGE_MASK       0x03F
#endif

#define BLOCK_MASK      0x1FF
#define SECTION_MASK	0x03F

#define VERIFY_READ_FLAG    0
#define VERIFY_EXECUTE_FLAG 0
#define VERIFY_WRITE_FLAG   1
#define VERIFY_KERNEL_OK    2
//-- End of duplicated symbols

// Bit offsets of page, block & section in the context register:
#if R3000
  #define CTX_PAGE      2
  #define CTX_BLOCK     6
  #define CTX_SECTION   15
#endif
#if R4000
  #if R4100
    #define CTX_PAGE      (VA_PAGE-7)
    #define CTX_BLOCK     9
    #define CTX_SECTION   18
  #else
    #define CTX_PAGE      3
    #define CTX_BLOCK     7
    #define CTX_SECTION   16
  #endif
#endif

#if R3000
  #define ASID_MASK 0x3f
#endif
#if R4000
  #define ASID_MASK 0xFF
#endif

#if R3000
/* Page permission bits */
#define PG_PERMISSION_MASK  0x80000F80
#define PG_GUARD            0x80000000  /* unused by HW */
#define PG_EXECUTE_MASK     0x00000080  /* unsued by HW */
#define PG_GLOBAL_MASK      0x00000100
#define PG_VALID_MASK       0x00000200
#define PG_DIRTY_MASK       0x00000400
#define PG_CACHE_MASK       0x00000800
#define PG_NOCACHE          0x00000800
#define PG_CACHE            0x00000000

#define MUST_CLEAR_GUARD_BIT 1
#endif

#if R4000
/* Page permission bits */
#define PG_PERMISSION_MASK  0xC000003F
#define PG_GUARD            0x80000000  /* unused by HW */
#define PG_EXECUTE_MASK     0x40000000  /* unused by HW */
#define PG_GLOBAL_MASK      0x00000001
#define PG_VALID_MASK       0x00000002
#define PG_DIRTY_MASK       0x00000004
#define PG_CACHE_MASK       0x00000038  /* should be ANDed to find out cache state */
#define PG_NOCACHE          0x00000010
#define PG_CACHE            0x00000018
#endif

#define PG_PROT_READ        0x00000000
#define PG_PROT_WRITE       PG_DIRTY_MASK
#define PG_SIZE_MASK        0x00000000

#define PG_PROTECTION		(PG_PROT_READ | PG_PROT_WRITE) 
#define PG_READ_WRITE       (PG_DIRTY_MASK | PG_VALID_MASK | PG_CACHE)

// MemBlock structure layout
#define mb_lock     0
#define mb_uses     4
#define mb_flags    5
#define mb_ixBase   6
#define mb_hPf		8
#define mb_pages    12

#define INVALID_POINTER_VALUE	0xC0000000

#if R3000
  #define BAD_PAGE        (~0ul)
  #define PFN_INCR  0x1000
#endif

#if R4000
  #define BAD_PAGE        (~0x3Ful)
  #if R4100
    #define PFN_SHIFT   4
  #else
    #define PFN_SHIFT   6
  #endif
  #define PFN_INCR  (PAGE_SIZE >> PFN_SHIFT)
#endif

#define SWITCHKEY(oldval, newval) ((oldval) = (pCurThread)->aky, CurAKey = (pCurThread)->aky = (newval))
#define GETCURKEY() ((pCurThread)->aky)
#define SETCURKEY(newval) (CurAKey = (pCurThread)->aky = (newval))

/* Query & set thread's kernel vs. user mode state */
#define GetThreadMode(pth) ((pth)->ctx.Psr & MODE_MASK)
#define SetThreadMode(pth, mode) ((pth)->ctx.Psr = (mode))

/* Query & set kernel vs. user mode state via Context */
#define GetContextMode(pctx) ((pctx)->Psr & MODE_MASK)
#define SetContextMode(pctx, mode) ((pctx)->Psr = (mode))

#if R3000
  /* Find the page frame # from a GPINFO pointer */
  #define GetPFN(pgpi)  ((ulong)(pgpi) & 0x1FFFF000)
  #define NextPFN(pfn)  ((pfn) + PFN_INCR)
  #define PFNfromEntry(entry)   ((entry) & 0x7FFFF000)
  #define PFNfrom256(x)	((ulong)(x)<<8 & 0x7FFFF000)

  #define KERNEL_MODE 0x04
  #define USER_MODE 0x0c
  #define MODE_MASK 0x0c

  /* Return virtual address from page frame number */
  #define Phys2Virt(pfn)  ((PVOID)((pfn) | 0x80000000))
  #define Phys2VirtUC(pfn)  ((PVOID)((pfn) | 0xA0000000))
#endif


#if R4000
  #define PFN_MASK (~(PAGE_SIZE-1) & 0x1FFFFFFF)
  /* Find the page frame # from a GPINFO pointer */
  #define GetPFN(pgpi)  (((ulong)(pgpi) & PFN_MASK) >> PFN_SHIFT)
  #define NextPFN(pfn)  ((pfn) + PFN_INCR)
  #define PFNfromEntry(entry)   ((entry) & 0x3FFFFFC0)
  #define PFNfrom256(x)	((ulong)(x)<<(8-PFN_SHIFT) & 0x3FFFFFC0)

  #define KERNEL_MODE 0x03
  #define USER_MODE 0x13
  #define MODE_MASK 0x3F

  /* Return virtual address from page frame number */
  #define Phys2Virt(pfn)  ((PVOID)(((pfn)<< PFN_SHIFT) | 0x80000000))
  #define Phys2VirtUC(pfn)  ((PVOID)(((pfn)<< PFN_SHIFT) | 0xA0000000))
#endif

/* Test the write access on a page table entry */
#define IsPageWritable(entry)	(((entry)&PG_DIRTY_MASK) != 0)
#define IsPageReadable(entry)	(((entry)&PG_VALID_MASK) == PG_VALID_MASK)

#define KPAGE_PHYS  0x00001000

#if R4000
#define KPAGE_PHYS_LO (KPAGE_PHYS>>PFN_SHIFT)
#endif

#define KPAGE_PTE (GetPFN(KPAGE_PHYS+0x800) + PG_VALID_MASK \
			+ PG_GLOBAL_MASK + PG_CACHE)

#endif // MEM_MIPS_H

