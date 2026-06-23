/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

romldr.h

Abstract:

ROM loader structures

Functions:


Notes: 


--*/

#ifndef __ROMLDR_H__
#define __ROMLDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#define KFLAG_DISALLOW_PAGING 0x00000001
#define KFLAG_NOTALLKMODE	  0x00000002
#define KFLAG_CELOGENABLE     0x00000004
#define ROM_SIGNATURE_OFFSET 64
#define ROM_SIGNATURE 0x43454345

#ifndef ASM_ONLY

#define ROM_EXTRA 9

typedef struct e32_rom {
    unsigned short  e32_objcnt;     /* Number of memory objects            */
    unsigned short  e32_imageflags; /* Image flags                         */
    unsigned long   e32_entryrva;   /* Relative virt. addr. of entry point */
    unsigned long   e32_vbase;      /* Virtual base address of module      */
    unsigned short  e32_subsysmajor;/* The subsystem major version number  */
    unsigned short  e32_subsysminor;/* The subsystem minor version number  */
    unsigned long   e32_stackmax;   /* Maximum stack size                  */
    unsigned long   e32_vsize;      /* Virtual size of the entire image    */
    unsigned long	e32_sect14rva;  /* section 14 rva */
    unsigned long	e32_sect14size; /* section 14 size */
    struct info     e32_unit[ROM_EXTRA]; /* Array of extra info units      */
    unsigned short  e32_subsys;     /* The subsystem type                  */
} e32_rom;

typedef struct o32_rom {
    unsigned long       o32_vsize;      /* Virtual memory size              */
    unsigned long       o32_rva;        /* Object relative virtual address  */
    unsigned long       o32_psize;      /* Physical file size of init. data */
    unsigned long       o32_dataptr;    /* Image pages offset               */
    unsigned long   o32_realaddr;       /* pointer to actual                */
    unsigned long       o32_flags;      /* Attribute flags for the object   */
} o32_rom;

#endif

//
//  ROM Header Structure - pTOC in NK points here
//
#ifdef ASM_ONLY
#define ROMHDR_dllfirst         0
#define ROMHDR_dlllast          4
#define ROMHDR_physfirst        8
#define ROMHDR_physlast         12
#define ROMHDR_nummods          16
#define ROMHDR_ulRAMStart       20
#define ROMHDR_ulRAMFree        24
#define ROMHDR_ulRAMEnd         28
#define ROMHDR_ulCopyEntries    32
#define ROMHDR_ulCopyOffset     36
#define ROMHDR_ulProfileLen     40
#define ROMHDR_ulProfileOffset  44
#define ROMHDR_numfiles         48
#define ROMHDR_ulObjstoreStart  52
#define ROMHDR_ulObjstoreLen    56
#define ROMHDR_ulDrivglobStart  60
#define ROMHDR_ulDrivglobLen    64
#define ROMHDR_usCPUType        68
#define ROMHDR_usMiscFlags		70
#define ROMHDR_pExtensions       72
#define ROMHDR_ulTrackingStart  76
#define ROMHDR_ulTrackingLen    80
#define SIZEOF_ROMHDR           84
#else
typedef struct ROMHDR {
    ULONG   dllfirst;               // first DLL address
    ULONG   dlllast;                // last DLL address
    ULONG   physfirst;              // first physical address
    ULONG   physlast;               // highest physical address
    ULONG   nummods;                // number of TOCentry's
    ULONG   ulRAMStart;             // start of RAM
    ULONG   ulRAMFree;              // start of RAM free space
    ULONG   ulRAMEnd;               // end of RAM
    ULONG   ulCopyEntries;          // number of copy section entries
    ULONG   ulCopyOffset;           // offset to copy section
    ULONG   ulProfileLen;           // length of PROFentries RAM 
    ULONG   ulProfileOffset;        // offset to PROFentries
    ULONG   numfiles;               // number of FILES
    ULONG   ulKernelFlags;          // optional kernel flags from ROMFLAGS .bib config option
    ULONG   ulFSRamPercent;         // Percentage of RAM used for filesystem 
                                        // from FSRAMPERCENT .bib config option
                                        // byte 0 = #4K chunks/Mbyte of RAM for filesystem 0-2Mbytes 0-255
                                        // byte 1 = #4K chunks/Mbyte of RAM for filesystem 2-4Mbytes 0-255
                                        // byte 2 = #4K chunks/Mbyte of RAM for filesystem 4-6Mbytes 0-255
                                        // byte 3 = #4K chunks/Mbyte of RAM for filesystem > 6Mbytes 0-255

    ULONG   ulDrivglobStart;        // device driver global starting address
    ULONG   ulDrivglobLen;          // device driver global length
    USHORT  usCPUType;       		// CPU (machine) Type
    USHORT  usMiscFlags;         	// Miscellaneous flags
	PVOID	pExtensions;			// pointer to ROM Header extensions
    ULONG   ulTrackingStart;        // tracking memory starting address
    ULONG   ulTrackingLen;          // tracking memory ending address
} ROMHDR;

typedef struct ROMChain_t {
	struct ROMChain_t *pNext;
	ROMHDR *pTOC;
} ROMChain_t;

#endif //ASM_ONLY

#define PID_LENGTH 10
// ROM Header extension: PID
typedef struct ROMPID {
	DWORD dwPID[PID_LENGTH];		// PID
	PVOID pNextExt;				// pointer to next extension if any
} ROMPID;

//
//  Module Table of Contents - follows ROMHDR in image
//
#ifdef ASM_ONLY
#define TOCentry_dwFileAttributes 0
#define TOCentry_ftTime         4
#define TOCentry_lpszFileSize   12
#define TOCentry_lpszFileName   16
#define TOCentry_ulE32Offset    20
#define TOCentry_ulO32Offset    24
#define TOCentry_ulLoadOffset   28
#define SIZEOF_TOCentry         32
#else
typedef struct TOCentry {           // MODULE BIB section structure
    DWORD dwFileAttributes;
    FILETIME ftTime;
    DWORD nFileSize;
    LPSTR   lpszFileName;
    ULONG   ulE32Offset;            // Offset to E32 structure
    ULONG   ulO32Offset;            // Offset to O32 structure
    ULONG   ulLoadOffset;           // MODULE load buffer offset
} TOCentry, *LPTOCentry;
#endif

//
//  Files Section Structure
//
#ifdef ASM_ONLY
#else
typedef struct FILESentry {         // FILES BIB section structure
    DWORD dwFileAttributes;
    FILETIME ftTime;
    DWORD nRealFileSize;
    DWORD nCompFileSize;
    LPSTR   lpszFileName;
    ULONG   ulLoadOffset;           // FILES load buffer offset
} FILESentry, *LPFILESentry;
#endif

//
//  Copy Section Structure
//
#ifdef ASM_ONLY
#define COPYentry_ulSource      0
#define COPYentry_ulDest        4
#define COPYentry_ulCopyLen     8
#define COPYentry_ulDestLen     12
#define SIZEOF_COPYentry        16
#else
typedef struct COPYentry {
    ULONG   ulSource;               // copy source address
    ULONG   ulDest;                 // copy destination address
    ULONG   ulCopyLen;              // copy length
    ULONG   ulDestLen;              // copy destination length 
                                    // (zero fill to end if > ulCopyLen)
} COPYentry;
#endif

//
//  Profile Code Section Structure
//
#ifdef ASM_ONLY
#define PROFentry_ulModNum      0
#define PROFentry_ulSectNum     4
#define PROFentry_ulStartAddr   8
#define PROFentry_ulEndAddr     12
#define PROFentry_ulHits        16
#define PROFentry_ulNumSym      20
#define PROFentry_ulHitAddress  24
#define PROFentry_ulSymAddress  28
#define SIZEOF_PROFentry        32
#else
typedef struct PROFentry {          // code section profile entry
    ULONG   ulModNum;               // module number in table of contents
    ULONG   ulSectNum;              // section number in o32
    ULONG   ulStartAddr;            // starting address of section
    ULONG   ulEndAddr;              // ending address of section
    ULONG   ulHits;                 // number of hits in section
    ULONG   ulNumSym;               // number of symbols in section
    ULONG   ulHitAddress;           // address to hit table 
    ULONG   ulSymAddress;           // address to symbol table 
} PROFentry;
#endif

//
//  Profile Symbol Section
//
//  Profile symbol section contains function address and hit counter for
//  each function followed by ASCIIZ symbols.
//
#ifdef ASM_ONLY
#define SYMentry_ulFuncAddress  0
#define SYMentry_ulFuncHits     4
#define SIZEOF_SYMentry         8
#else
typedef struct SYMentry {
    ULONG   ulFuncAddress;          // function starting address
    ULONG   ulFuncHits;             // function hit counter
} SYMentry;
#endif

#ifdef __cplusplus
}
#endif

#endif

