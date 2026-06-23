/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdpcpu.h

Abstract:

    Machine specific kernel debugger data types and constants

Revision History:

--*/

#ifndef _KDPCPU_
#define _KDPCPU_

#ifdef MIPS

#define KDP_BREAKPOINT_TYPE  ULONG

#if defined(MIPS16SUPPORT)
#define KDP_BREAKPOINT_32BIT_TYPE               ULONG
#define KDP_BREAKPOINT_32BIT_VALUE              0x0001000D     // Break 1
#define KDP_BREAKPOINT_16BIT_TYPE               USHORT
#define KDP_BREAKPOINT_16BIT_VALUE              0xE825         // Break 1
#else
#define KDP_BREAKPOINT_ALIGN 3
#define KDP_BREAKPOINT_VALUE 0x0001000D                        // Break 1
#endif

#elif SHx

#define KDP_BREAKPOINT_TYPE  USHORT
#define KDP_BREAKPOINT_ALIGN 1
#define KDP_BREAKPOINT_VALUE 0xC301    // TRAPA 1

#elif x86

#define KDP_BREAKPOINT_TYPE  UCHAR
#define KDP_BREAKPOINT_ALIGN 0
#define KDP_BREAKPOINT_VALUE 0xCC		// int 3

#elif PPC

#define KDP_BREAKPOINT_TYPE  ULONG
#define KDP_BREAKPOINT_ALIGN 3
#define KDP_BREAKPOINT_VALUE 0x0FE00016    // twi 31, 0, 0x16

#elif ARM

#define KDP_BREAKPOINT_TYPE  ULONG
#define KDP_BREAKPOINT_ALIGN 3
#define KDP_BREAKPOINT_VALUE 0xE6000010         // undefined instruction

#if defined(THUMBSUPPORT)
typedef ULONG KDP_BREAKPOINT_32BIT_TYPE;
typedef USHORT KDP_BREAKPOINT_16BIT_TYPE;

#define KDP_BREAKPOINT_32BIT_VALUE          0x06000010     // Undefined Inst.
#define KDP_BREAKPOINT_16BIT_VALUE          0xDEFE         // Undefined Inst.
#endif

#else
 #error unknown cpu type
#endif

#endif // _KDPCPU_
