/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*
    mdmem.h - machine dependant memory management structures
*/

#ifndef MEM_SHx_H
#define MEM_SHx_H

#ifdef SH4
#define PAGE_SIZE 4096		/* page size: must be 1024, 2048 or 4096 */
#else
#define PAGE_SIZE 1024		/* page size: must be 1024, 2048 or 4096 */
#endif

/* Page permission bits */
#define PG_PERMISSION_MASK  0x000001FF
#define PG_EXECUTE_MASK     0x00000000 //0x00000200  /* unsued by HW */
#define PG_VALID_MASK       0x00000101
#define PG_GLOBAL_MASK      0x00000002
#define PG_DIRTY_MASK       0x00000004
#define PG_CACHE	     	0x00000008
#define PG_CACHE_MASK     	0x00000008
#define PG_GUARD	     	0x00000008
#define PG_NOCACHE		   	0x00000000
#define PG_1K_MASK			0x00000000
#define PG_4K_MASK			0x00000010
#define PG_64K_MASK			0x00000080
#define PG_1M_MASK			0x00000090
#define PG_PROTECTION		0x00000060
#define PG_PROT_KREAD		0x00000000	/* kernel read-only */
#define PG_PROT_KWRITE		0x00000020	/* kernel read/write */
#define PG_PROT_READ		0x00000040	/* user read-only */
#define PG_PROT_WRITE		0x00000060	/* user read/write */

#if PAGE_SIZE == 4096
#define PG_SIZE_MASK PG_4K_MASK
#else
#define PG_SIZE_MASK PG_1K_MASK
#endif

#define PG_READ_WRITE   (PG_SIZE_MASK | PG_VALID_MASK | PG_DIRTY_MASK | PG_CACHE_MASK | PG_PROT_WRITE)

#define UNKNOWN_BASE    (~0)
#define BAD_PAGE        (4)

#ifndef ASM_ONLY

#define SWITCHKEY(oldval, newval) ((oldval) = (pCurThread)->aky, (pCurThread)->aky = (newval))
#define GETCURKEY() ((pCurThread)->aky)
#define SETCURKEY(newval) ((pCurThread)->aky = (newval))

#define PFN_INCR  PAGE_SIZE
#define PFN_MASK (~(PAGE_SIZE-1) & 0x1FFFFFFF)

/* Find the page frame # from a GPINFO pointer */
#define GetPFN(pgpi)  ((ulong)(pgpi) & PFN_MASK)
#define NextPFN(pfn)  ((pfn) + PFN_INCR)
#define PFNfromEntry(entry)   ((entry) & ~(PAGE_SIZE-1))
#define PFNfrom256(pg256)	((ulong)(pg256)<<8 & ~(PAGE_SIZE-1))

/* Test the write access on a page table entry */
#define IsPageWritable(entry)	(((entry)&PG_PROTECTION) == PG_PROT_WRITE)
#define IsPageReadable(entry)	(((entry)&PG_VALID_MASK) == PG_VALID_MASK)

/* Query & set thread's kernel vs. user mode state */
#define KERNEL_MODE     0x40
#define USER_MODE       0x00
#define SR_MODE_BITS    24
#define GetThreadMode(pth) (((pth)->ctx.Psr >> SR_MODE_BITS) & 0x40)
#define SetThreadMode(pth, mode) \
        ((pth)->ctx.Psr = ((pth)->ctx.Psr & 0x3fffffff) | ((mode)<<SR_MODE_BITS))

/* Query & set kernel vs. user mode state via Context */
#define GetContextMode(pctx) (((pctx)->Psr >> SR_MODE_BITS) & 0x40)
#define SetContextMode(pctx, mode)	\
		((pctx)->Psr = ((pctx)->Psr & 0x3fffffff) | ((mode)<<SR_MODE_BITS))


/* Return virtual address from page frame number */
#define Phys2Virt(pfn)  ((PVOID)((pfn) | 0x80000000))
#define Phys2VirtUC(pfn)  ((PVOID)((pfn) | 0xA0000000))

#if PAGE_SIZE==4096
extern KDBase[];
#define KPAGE_PTE ((ulong)KDBase + 0x1000 + 0x80000000 \
			+ PG_4K_MASK + PG_VALID_MASK \
			+ PG_GLOBAL_MASK + PG_CACHE_MASK + PG_PROT_READ)
#else
#define KPAGE_PTE ((ulong)&KData + 0x80000000 + PG_VALID_MASK \
			+ PG_GLOBAL_MASK + PG_CACHE_MASK + PG_PROT_READ)
#endif

#endif // not ASM_ONLY

#endif // MEM_SHx_H

