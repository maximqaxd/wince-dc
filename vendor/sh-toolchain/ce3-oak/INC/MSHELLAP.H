/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mshellap.h

Abstract:  

Notes: 


--*/
#ifndef _INC_MSHELLAPI
#define _INC_MSHELLAPI

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif


#define Shell_NotifyIcon IMPLICIT_DECL(BOOL, SH_SHELL, 6, \
					  (DWORD,PNOTIFYICONDATA))
#define SHAddToRecentDocs IMPLICIT_DECL(void, SH_SHELL, 7, \
							(DWORD, LPCVOID))
#define SHCreateExplorerInstance IMPLICIT_DECL(BOOL, SH_SHELL, 9, \
					  (LPCTSTR, UINT))

#ifdef __cplusplus
}
#endif

#endif  /* _INC_MSHELLAPI */
