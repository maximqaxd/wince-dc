/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

hwcomapi.h

Abstract:

Holds pdd level prototypes of win32 comm apis.

Notes: 


--*/

#ifdef __cplusplus
extern "C" {
#endif


ULONG
HWGetStatus(
    PVOID	pHead,
    LPCOMSTAT	lpStat
    );

VOID
HWReset(
    PVOID   pHead
    );

VOID
HWGetModemStatus(
    PVOID   pHead,
    PULONG  pModemStatus
    );

VOID
HWGetCommProperties(
    PVOID	pHead,
    LPCOMMPROP	pCommProp
    );

VOID
HWPurgeComm(
    PVOID pHead,
    DWORD   fdwAction
    );

#ifdef __cplusplus
}
#endif

    
