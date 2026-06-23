/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

dbgprint.h

Abstract:

Debug Print macros used in the WCEDDK samples.

Notes: 


--*/

#ifndef __DBGPRINT_H__
#define __DBGPRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

// For NT emulation, we include emulation header, and 
// let the debug macro to use wprintf function.
#ifdef _WIN32_WCE_EMULATION  
#include <wceemul.h>
#define DbgPrintfW wprintf
#endif // _WIN32_WCE_EMULATION

// Set debug macro.
extern int DbgPrintfW(LPWSTR lpszFmt, ...);
#ifdef DEBUG
#define DEBUGMSG(cond,printf_exp) ((void)((cond)?(DbgPrintfW printf_exp),1:0))
#else
#define DEBUGMSG(cond,printf_exp) ((void)0)
#endif // DEBUG

#ifdef __cplusplus
}
#endif

#endif //__DBGPRINT_H__

