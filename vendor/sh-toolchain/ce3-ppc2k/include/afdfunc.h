/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-1998  Microsoft Corporation

Module Name:  

afdfunc.h

Abstract:

Functions exposed from the AFD system process

Notes: 


--*/

// @CESYSGEN IF CE_MODULES_AFD

#ifndef _AFDFUNC_H_
#define _AFDFUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SOCKHAND_DEFINED
DECLARE_HANDLE(SOCKHAND);
typedef SOCKHAND *PSOCKHAND;
#define SOCKHAND_DEFINED
#endif

#include "ras.h"
#include "tapi.h"
			    
#ifdef __cplusplus
}
#endif

#ifdef WINCEOEM
#ifdef WINCEMACRO
#include <mafdfunc.h>	// macros
#endif
#endif

#endif  // _AFDFUNC_H_

// @CESYSGEN ENDIF
