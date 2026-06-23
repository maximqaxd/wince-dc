/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

cmnmacro.h

Abstract:

Some common macros

Notes: 


--*/
#ifndef _CMNMACRO_H_
#define _CMNMACRO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOINT16(l)           ((INT16)(l))
#define HIINT16(l)           ((INT16)(UINT16)(((UINT32)(l) >> 16) & 0xFFFF))

#define LOUINT16(l)          ((UINT16)(l))
#define HIUINT16(l)          ((UINT16)(((UINT32)(l) >> 16) & 0xFFFF))

#define CR_BLACK  RGB(0x00,0x00,0x00)
#define CR_DKGRAY RGB(0x80,0x80,0x80)
#define CR_LTGRAY RGB(0xc0,0xc0,0xc0)
#define CR_WHITE  RGB(0xff,0xff,0xff)

#ifdef __cplusplus
}
#endif

#endif /*_CMNMACRO_H_*/

