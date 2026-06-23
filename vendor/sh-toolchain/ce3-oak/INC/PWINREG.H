/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pwinreg.h

Abstract:

Private portion of winreg.h

Notes: 


--*/
    
   
    
#ifndef _PRIV_WINREG_H_
#define _PRIV_WINREG_H_

#ifdef __cplusplus
extern "C"  {
#endif

BOOL RegCopyFile(LPCWSTR lpszFile);
BOOL RegRestoreFile(LPCWSTR lpszFile);

#define REG_WRITE_BYTES_START 0x00000001
#define REG_WRITE_BYTES_PROBE 0x80000000
#define REG_READ_BYTES_START 0x00000001

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //_PRIV_WINREG_H_
    
