/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

prapi.h

Abstract:

Contains private portion of the Remote API

Notes: 


--*/

#ifndef PRAPI_H
#define PRAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNDER_CE
#define GetDccManWindow         RAPI_EXP_10
#define CeEventHasOccurred     RAPI_EXP_11
#define CeSyncTimeToPc         RAPI_EXP_12
#define CeStartReplication     RAPI_EXP_13
#define CeRegCopyFile          RAPI_EXP_14
#define CeRegRestoreFile       RAPI_EXP_15
#define CeSetPasswordActive    RAPI_EXP_16
#define CeSetPassword          RAPI_EXP_17
#endif // UNDER_CE

STDAPI          GetDccManWindow(HWND *phwnd);

STDAPI_(BOOL  ) CeEventHasOccurred  (LONG, LPWSTR);
STDAPI_(BOOL  ) CeSyncTimeToPc      (DWORD);
STDAPI          CeStartReplication  (void);
STDAPI_(BOOL  ) CeRegCopyFile       (LPCWSTR);
STDAPI_(BOOL  ) CeRegRestoreFile    (LPCWSTR);
STDAPI_(BOOL  ) CeSetPasswordActive (BOOL, LPWSTR);
STDAPI_(BOOL  ) CeSetPassword       (LPWSTR, LPWSTR);

#ifdef __cplusplus
}
#endif

#endif // #ifndef PRAPI_H
