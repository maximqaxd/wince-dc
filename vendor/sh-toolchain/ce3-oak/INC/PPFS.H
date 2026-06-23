/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

ppfs.h

Abstract:  

    These are the constants used for ppfs internally
    
Notes: 


--*/
#ifndef _PPFS_H_
#define _PPFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_TYPE_SH3       -1
#define BOOT_TYPE_R4200     -2
#define BOOT_TYPE_R4100     -3
#define BOOT_TYPE_X86       -4
#define BOOT_TYPE_R3000     -5
#define BOOT_TYPE_UNKNOWN   -10000

//
// Registry keys
//
#define REG_KEY_BASE                "SYSTEM\\CurrentControlSet\\Services"
#define REG_KEY_PPFS                "ppfs"
#define REG_KEY_PARMS               "Parameters"

#ifdef __cplusplus
}
#endif

#endif  // #ifndef  _PPFS_H_
