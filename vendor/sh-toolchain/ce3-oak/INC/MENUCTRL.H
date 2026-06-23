/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

menuctrl.h

Abstract:  

Notes: 


--*/
#ifndef _MENUCTRL_H_
#define _MENUCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif


/* When creating a window of class "Menu", pass a PMENUCONTROLINFO as the 
   lpParam parameter to CreateWindow.
 */

#define MENUCONTROLCLASS TEXT("Menu")
 
typedef struct
{
    HINSTANCE hinst;        /* instance containing the menu resource */
    LPCTSTR   lpszMenuName; /* menu resource name or ID */
    HWND      hwndNotify;   /* the window that will receive menu notifications*/
} MENUCONTROLINFO, *PMENUCONTROLINFO, *LPMENUCONTROLINFO;

#ifdef __cplusplus
}
#endif

#endif /* _MENUCTRL_H_ */
