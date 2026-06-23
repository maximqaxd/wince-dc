/*****************************************************************************\
*                                                                             *
* shsftkey.h -  SOFTKEY.DLL functions, types, and definitions                  *
*                                                                             *
* Copyright (c) 1999, Microsoft Corp.  All rights reserved                    *
*                                                                             *
\*****************************************************************************/

#ifndef __SHSFTKEY_H__
#define __SHSFTKEY_H__


#include <pshpack1.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

BOOL WINAPI Shell_SetSoftkeyLabels(HWND hwnd, 
								   LPCTSTR szLeftLabel, HICON hIconLeft, 
								   LPCTSTR szRightLabel, HICON hIconRight);

// Does the default handling of pressing the back button.
void WINAPI SHNavigateBack();


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include <poppack.h>

//
// BUGBUG This should be moved to aygshell after rapier forks
//
#ifdef TPC
#define SHCMBF_NODEFAULT    0x00000001 // do not do default handling of this key
#define SHCMBF_NOTIFY       0x00000002 // send us the WM_* messages for this key

// Thia is used to modify the default handling of key messages sent
// to the menu bar of the foreground app.  The only keys that are
// currently valid are VK_TSOFT1, VK_TSOFT2, and VK_TBACK
#define SHCMBM_OVERRIDEKEY  (WM_USER + 403) // wParam = nVirtkey, dwMask = (DWORD)LOWORD(lParam), dwBits = (DWORD)HIWORD(lParam)
#endif


#endif // !__SHSFTKEY_H__

