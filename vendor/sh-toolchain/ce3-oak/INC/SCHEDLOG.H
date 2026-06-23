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
//  Module Name:  
//  
//      schedlog.h
//  
//  Abstract:  
//      
//      Header file for hooking the kernel's scheduler logging functions
//      in the OAL. There are two levels of support for scheduler logging. For 
//      minimal intrusiveness, the logging functions should be implemented
//      directly in the OAL. Optionally, the SCHEDLOG library can be used to export
//      the logging functions to a user process for greater flexbility. The user
//      process can hook the functions by making a KernelIoControl call.
//      
//------------------------------------------------------------------------------
#ifndef _SCHEDLOG_H
#define _SCHEDLOG_H

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
//
// Translate the kernel ID's to handles. pThreadHandle can be NULL if only a 
// process handle is being translated.
//
//------------------------------------------------------------------------------
void 
SchedLogTranslate(
    DWORD   dwThreadID,
    HANDLE* pThreadHandle,
    DWORD   dwProcID,
    HANDLE* pProcessHandle
    );


//------------------------------------------------------------------------------
//
// Returns a pointer to the name of the process.  If the name cannot be found, 
// the pointer returned is NULL.
//
//------------------------------------------------------------------------------
LPCSTR 
SchedLogGetProcessName(
    DWORD dwProcID
    );


//------------------------------------------------------------------------------
//
// Returns the program counter for the thread. The current module and function 
// name may be returned in the optional string pointers. If the string pointers
// are NULL, no name is returned. If the name cannot be found, the pointer is
// set to NULL. 
//
// NOTE : The pointers returned by this function are only valid within the 
// scheduler logging function callbacks.
//
//------------------------------------------------------------------------------
DWORD SchedLogGetThreadProgramCounter(
    DWORD dwThreadID,
    LPCSTR* ppszThreadModuleName,
    LPCSTR* ppszThreadFunctionName
    );

#ifdef __cplusplus
}
#endif


#endif

