// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved

/*
 * mmu.h: MMU Header file
 *
 *
 */

#ifndef ASM_ONLY

typedef	union LevelOneDescriptor {
   struct {
	unsigned  sg_l2ba	: 20;	/* Level 2 table base address */
	unsigned  sg_resv	: 3;	/* Reserved */
	unsigned  sg_apg	: 4;	/* Access Protection Group */
	unsigned  sg_g		: 1;	/* Guarded storage attribute */
	unsigned  sg_ps		: 2;	/* Page Size level one */
	unsigned  sg_wt		: 1;	/* Write Through */
	unsigned  sg_v		: 1;	/* Valid bit */
   } bit;
   unsigned all;
} SEG;


typedef union LevelTwoDescriptor {	/* currently only defined for 4k page */
   struct {
	unsigned  pg_rpn	: 20;	/* Real Page Number */
	unsigned  pg_pm		: 2;	/* Protection Mode */
	unsigned  pg_enc	: 1;	/* Encoding mode */
	unsigned  pg_chg	: 1;	/* Change bit */
	unsigned  pg_acc	: 4;	/* Access mode */
	unsigned  pg_sps	: 1;	/* Small Page Size */
	unsigned  pg_sh		: 1;	/* Shared Page */
	unsigned  pg_ci		: 1;	/* Cache Inhitbited */
	unsigned  pg_v		: 1;	/* Valid page */
   } bit;
   unsigned all;
} PAGE;

typedef	union	MMUControllReg {
   struct {
	unsigned  mc_gpm	: 1;	/* Group Protection Mode */
	unsigned  mc_ppm	: 1;	/* Page Protection Mode */
	unsigned  mc_cidef	: 1;	/* Default Cache Inhitbit attribute */
	unsigned  mc_wtdef	: 1;	/* Default Write Thru mode (data only */
	unsigned  mc_rsv4	: 1;	/* Reseved 4 TLBs */
	unsigned  mc_twam	: 1;	/* Table Walk Assist mode (data only) */
	unsigned  mc_ppcs	: 1;	/* Privilege/Problem compare mode */
	unsigned  mc_resv0	: 12;	/* reserved */
	unsigned  mc_tlbidx	: 5;	/* TLB index */
	unsigned  mc_resv1	: 9;	/* reserved */
   } bit;
   unsigned all;
} MX_CTR;

#endif // ASM_ONLY

#define PAGE_VALID	0x001
#define PAGE_SHARED	0x004

#define PAGE_8MB	0x00c	/* 8mb page descriptor */

/* Special MMU registers */
#define	MI_CTR		784		/* IMMU Control Reg. */
#define	MI_AP		786		/* IMMU Access Protection Reg. */
#define	MI_EPN		787		/* IMMU Effective Number Reg. */
#define	MI_TWC		789		/* IMMU Table Walk Control Reg. */
#define	MI_RPN		790		/* IMMU Real Page Number Reg. */
#define	MI_DBCAM	816		/* IMMU CAM Read Reg. */
#define	MI_DBRAM0	817		/* IMMU RAM entry 0 Reg. */
#define	MI_DBRAM1	818		/* IMMU RAM entry 1 Reg. */

#define	MD_CTR		792		/* DMMU Control Reg. */
#define	M_CASID		793		/* MMU CASID Reg. */
#define	MD_AP		794		/* DMMU Access Protection Reg. */
#define	MD_EPN		795		/* DMMU Effective Number Reg. */
#define	M_TWB		796		/* MMU Table Walk Base Reg. */
#define	MD_TWC		797		/* DMMU Table Walk Control Reg. */
#define	MD_RPN		798		/* DMMU Real Page Number */
#define	M_TW		799		/* MMU Tabel Walk Reg. */
#define	MD_DBCAM	824		/* DMMU CAM Read Reg. */
#define	MD_DBRAM0	825		/* DMMU RAM entry 0 Reg. */
#define	MD_DBRAM1	826		/* DMMU RAM entry 1 Reg. */

/* Cache Special Registers */
#define	IC_CST		560		/* Instruction Cache Command/Status */
#define	DC_CST		568		/* Data Cache Command/Status */

/* Instruction and Data Cache commands */
#define	CacheUnlockAllCmd	0x0A000000	/* Cache Unlock_All command */
#define	CacheDisableCmd		0x04000000	/* Cache Disable command */
#define	CacheInvAllCmd		0x0C000000	/* Cache Invalidate_All cmd */
#define	CacheEnableCmd		0x02000000	/* DCache_Enable Command */

#define	CacheEnableBit		0x80000000	/* Cache Enable bit in I/DC_CST */

