/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pehdr.h

Abstract:



Functions:


Notes: 


--*/

/* Standard number of extra information units placed in the header; */
/* this includes the following tables: */
/* export, import, resource, exception, security, fixup, debug, */
/* image description, machine specific tables */

#ifndef __PEHDR_H__
#define __PEHDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STD_EXTRA       16

#define EXP             0           /* Export table position               */
#define IMP             1           /* Import table position               */
#define RES             2           /* Resource table position             */
#define EXC             3           /* Exception table position            */
#define SEC             4           /* Security table position             */
#define FIX             5           /* Fixup table position                */
#define DEB             6           /* Debug table position                */
#define IMD             7           /* Image description table position    */
#define MSP             8           /* Machine specific table position     */
#define TLS             9           /* Thread Local Storage                */
#define CBK            10           /* Callbacks                           */
#define RS1            11           /* Reserved                            */
#define RS2            12           /* Reserved                            */
#define RS3            13           /* Reserved                            */
#define RS4            14           /* Reserved                            */
#define RS5            15           /* Reserved                            */

#define LITE_EXTRA      6           /* Only first 6 used by NK */

struct info {                       /* Extra information header block      */
    unsigned long   rva;            /* Virtual relative address of info    */
    unsigned long   size;           /* Size of information block           */
};

typedef struct e32_exe {            /* PE 32-bit .EXE header               */
    unsigned char   e32_magic[4];   /* Magic number E32_MAGIC              */
    unsigned short  e32_cpu;        /* The CPU type                        */
    unsigned short  e32_objcnt;     /* Number of memory objects            */
    unsigned long   e32_timestamp;  /* Time EXE file was created/modified  */
    unsigned long   e32_symtaboff;  /* Offset to the symbol table          */
    unsigned long   e32_symcount;   /* Number of symbols                   */
    unsigned short  e32_opthdrsize; /* Optional header size                */
    unsigned short  e32_imageflags; /* Image flags                         */
    unsigned short  e32_coffmagic;  /* Coff magic number (usually 0x10b)   */
    unsigned char   e32_linkmajor;  /* The linker major version number     */
    unsigned char   e32_linkminor;  /* The linker minor version number     */
    unsigned long   e32_codesize;   /* Sum of sizes of all code sections   */
    unsigned long   e32_initdsize;  /* Sum of all initialized data size    */
    unsigned long   e32_uninitdsize;/* Sum of all uninitialized data size  */
    unsigned long   e32_entryrva;   /* Relative virt. addr. of entry point */
    unsigned long   e32_codebase;   /* Address of beginning of code section*/
    unsigned long   e32_database;   /* Address of beginning of data section*/
    unsigned long   e32_vbase;      /* Virtual base address of module      */
    unsigned long   e32_objalign;   /* Object Virtual Address align. factor*/
    unsigned long   e32_filealign;  /* Image page alignment/truncate factor*/
    unsigned short  e32_osmajor;    /* The operating system major ver. no. */
    unsigned short  e32_osminor;    /* The operating system minor ver. no. */
    unsigned short  e32_usermajor;  /* The user major version number       */
    unsigned short  e32_userminor;  /* The user minor version number       */
    unsigned short  e32_subsysmajor;/* The subsystem major version number  */
    unsigned short  e32_subsysminor;/* The subsystem minor version number  */
    unsigned long   e32_res1;       /* Reserved bytes - must be 0  */
    unsigned long   e32_vsize;      /* Virtual size of the entire image    */
    unsigned long   e32_hdrsize;    /* Header information size             */
    unsigned long   e32_filechksum; /* Checksum for entire file            */
    unsigned short  e32_subsys;     /* The subsystem type                  */
    unsigned short  e32_dllflags;   /* DLL flags                           */
    unsigned long   e32_stackmax;   /* Maximum stack size                  */
    unsigned long   e32_stackinit;  /* Initial committed stack size        */
    unsigned long   e32_heapmax;    /* Maximum heap size                   */
    unsigned long   e32_heapinit;   /* Initial committed heap size         */
    unsigned long   e32_res2;       /* Reserved bytes - must be 0  */
    unsigned long   e32_hdrextra;   /* Number of extra info units in header*/
    struct info     e32_unit[STD_EXTRA]; /* Array of extra info units      */
} e32_exe, *LPe32_exe;

/* Internal reduced form of e32 header */

typedef struct e32_lite {           /* PE 32-bit .EXE header               */
    unsigned short  e32_objcnt;     /* Number of memory objects            */
    BYTE            e32_cevermajor; /* version of CE built for             */
    BYTE            e32_ceverminor; /* version of CE built for             */
    unsigned long   e32_stackmax;   /* Maximum stack size                  */
    unsigned long   e32_vbase;      /* Virtual base address of module      */
    unsigned long   e32_vsize;      /* Virtual size of the entire image    */
    unsigned long	e32_sect14rva;  /* section 14 rva */
    unsigned long	e32_sect14size; /* section 14 size */
    struct info     e32_unit[LITE_EXTRA]; /* Array of extra info units     */
} e32_lite, *LPe32_list;

/*
 *  OBJECT TABLE
 */

#define E32OBJNAMEBYTES 8               /* Name bytes                       */

typedef struct o32_obj {                /* .EXE memory object table entry   */
    unsigned char       o32_name[E32OBJNAMEBYTES];/* Object name            */
    unsigned long       o32_vsize;      /* Virtual memory size              */
    unsigned long       o32_rva;        /* Object relative virtual address  */
    unsigned long       o32_psize;      /* Physical file size of init. data */
    unsigned long       o32_dataptr;    /* Image pages offset               */
    unsigned long       o32_realaddr;   /* pointer to actual                */
    unsigned long       o32_access;     /* assigned access                  */
    unsigned long       o32_temp3;
    unsigned long       o32_flags;      /* Attribute flags for the object   */
} o32_obj, *LPo32_obj;

typedef struct o32_lite {
    unsigned long       o32_vsize;
    unsigned long       o32_rva;
    unsigned long       o32_realaddr;
    unsigned long       o32_access;
    unsigned long       o32_flags;
    unsigned long       o32_psize;
    unsigned long       o32_dataptr;
} o32_lite, *LPo32_lite;

/*
 *  EXPORT ADDRESS TABLE - Previously called entry table
 */

struct ExpHdr                           /* Export directory table          */
{
    unsigned long       exp_flags;      /* Export table flags, must be 0   */
    unsigned long       exp_timestamp;  /* Time export data created        */
    unsigned short      exp_vermajor;   /* Major version stamp             */
    unsigned short      exp_verminor;   /* Minor version stamp             */
    unsigned long       exp_dllname;    /* Offset to the DLL name          */
    unsigned long       exp_ordbase;    /* First valid ordinal             */
    unsigned long       exp_eatcnt;     /* Number of EAT entries           */
    unsigned long       exp_namecnt;    /* Number of exported names        */
    unsigned long       exp_eat;        /* Export Address Table offset     */
    unsigned long       exp_name;       /* Export name pointers table off  */
    unsigned long       exp_ordinal;    /* Export ordinals table offset    */
};

/*
 *  IMPORT MODULE DESCRIPTOR TABLE
 *
 *  Import Directory Table consists of an array of ImpHdr structures (one
 *  for each DLL imported), and is terminated by a zeroed ImpHdr structure.
 */

struct ImpHdr                           /* Import directory table          */
{
    unsigned long       imp_lookup;
    unsigned long       imp_timestamp;
    unsigned long       imp_forwarder;
    unsigned long       imp_dllname;
    unsigned long       imp_address;
};

/*
 *  IMPORT PROCEDURE NAME TABLE
 */

struct ImpProc
{
    unsigned short  ip_hint;            /* Hint value                   */
    char            ip_name[1];         /* Zero terminated importe name */
};

#define IMAGE_REL_BASED_ABSOLUTE        0
#define IMAGE_REL_BASED_HIGH            1
#define IMAGE_REL_BASED_LOW             2
#define IMAGE_REL_BASED_HIGHLOW         3
#define IMAGE_REL_BASED_HIGHADJ         4
#define IMAGE_REL_BASED_MIPS_JMPADDR    5
#define IMAGE_REL_BASED_MIPS_JMPADDR16  9

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_MINIMAL_OBJECT            0x0010  // Reserved.
#define IMAGE_FILE_UPDATE_OBJECT             0x0020  // Reserved.
#define IMAGE_FILE_16BIT_MACHINE             0x0040  // 16 bit word machine.
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_PATCH                     0x0400  // Reserved.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_SCN_TYPE_REGULAR               0x00000000  //
#define IMAGE_SCN_TYPE_DUMMY                 0x00000001  // Reserved.
#define IMAGE_SCN_TYPE_NO_LOAD               0x00000002  // Reserved.
#define IMAGE_SCN_TYPE_GROUPED               0x00000004  // Used for 16-bit offset code.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
#define IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
#define IMAGE_SCN_LNK_OVERLAY                0x00000400  // Section contains an overlay.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.

#define IMAGE_SCN_COMPRESSED                 0x00002000  // Section is compressed

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //

#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

#ifdef __cplusplus
}
#endif

#endif

