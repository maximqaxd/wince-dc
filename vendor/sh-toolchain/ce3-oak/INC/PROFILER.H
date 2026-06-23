//------------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
//------------------------------------------------------------------------------
//
//  This module contains the function prototypes and constant, type and
//  structure definitions for the Windows CE profiling functionality.
//
//------------------------------------------------------------------------------
#ifndef _PROFILER_H_
#define _PROFILER_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// dwOptions to ProfileStart
//
#define PROFILE_OBJCALL       1  // record object calls
#define PROFILE_KCALL   		2	// record kcalls
#define PROFILE_BUFFER  		4	// record hits to buffer
#define PROFILE_PAUSE	   	8	// 
#define PROFILE_CONTINUE		16	// 
#define PROFILE_STARTPAUSED	32	// 

#define PROFILE_CODETEST    0x80000000	// AMC CodeTEST profiling



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD ProfileStart(DWORD dwUSecInterval, DWORD dwOptions);
void ProfileStop(void);



//------------------------------------------------------------------------------
//
// OEM-implemented profiler functions.
//
//------------------------------------------------------------------------------
void OEMProfileTimerDisable(void);
void OEMProfileTimerEnable(DWORD dwUSecInterval);



//------------------------------------------------------------------------------
//
// Returns a pointer to the name of the module which started the given thread. 
// If the module cannot be found, the return value is NULL.
//
//------------------------------------------------------------------------------
LPCSTR GetModName(
   PTHREAD pth,
   unsigned int pc
   ); 


//------------------------------------------------------------------------------
//
// Returns a pointer to the name of the function the given thread was started 
// with.  If the function cannot be found, the return value is NULL.
//
//------------------------------------------------------------------------------
LPCSTR GetFunctionName(
   PTHREAD pth,
   unsigned int pc
   );


//------------------------------------------------------------------------------
//
// The current module handle and function name will be copied into the given args. 
// If the values cannot be found, the handle will be INVALID_HANDLE_VALUE and 
// the string will be empty. 
//
//------------------------------------------------------------------------------
void GetThreadName(
    PTHREAD pth,
    HANDLE* hModule,
    WCHAR*  szThreadFunctionName
    );

#ifdef __cplusplus
}
#endif


#endif // _PROFILER_H_
