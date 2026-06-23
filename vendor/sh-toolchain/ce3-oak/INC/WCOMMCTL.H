/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

	wcommctl.h

Abstract:

	This file contains macros that allow you to use the common control
	functions (such as listview etc) found at runtime rather than at
	load time.  This allows OAK customers to customize their own common
	control library to have features specific to their platform.

Functions:


Notes: 


--*/

#ifndef _WCOMMCTL_H_
#define _WCOMMCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

//	InitCommonControls
typedef void (WINAPI *PFN_InitCommonControls)(void);

//	InitCommonControlsEx
typedef void (WINAPI *PFN_InitCommonControlsEx)(LPINITCOMMONCONTROLSEX picce);

//    CreateStatusWindowW
typedef HWND (WINAPI *PFN_CreateStatusWindowW)(LONG style,
	LPCWSTR lpszText, HWND hwndParent, UINT wID);

//    DrawStatusTextW
typedef void (WINAPI *PFN_DrawStatusTextW)(HDC hDC,
	LPRECT lprc, LPCWSTR pszText, UINT uFlags);

//	CommandBar_Create
typedef HWND (WINAPI *PFN_CommandBar_Create)(HINSTANCE hInst,
	HWND hwndParent, int idCmdBar);

//	CommandBar_Show
typedef BOOL (WINAPI *PFN_CommandBar_Show)(HWND hwndCB,
	BOOL fShow);

//	CommandBar_AddBitmap
typedef int (WINAPI *PFN_CommandBar_AddBitmap)(HWND hwndCB,
	HINSTANCE hInst, int idBitmap, int iNumImages, int iImageWidth,
	int iImageHeight);

//	CommandBar_InsertComboBox
typedef HWND (WINAPI *PFN_CommandBar_InsertComboBox)(
	HWND hwndCB, HINSTANCE hInstance,
	int  iWidth, UINT dwStyle,
	WORD idComboBox, WORD iButton);

//	CommandBar_InsertMenubar
typedef BOOL (WINAPI *PFN_CommandBar_InsertMenubar)(
	HWND hwndCB, HINSTANCE hInst, WORD idMenu, WORD iButton);


//	CommandBar_GetMenu
typedef HMENU (WINAPI *PFN_CommandBar_GetMenu)(HWND hwndCB,
	WORD iButton);

//	CommandBar_AddAdornments
typedef BOOL (WINAPI *PFN_CommandBar_AddAdornments)(HWND hwndCB,
	DWORD dwFlags, DWORD dwReserved);

// 	CommandBar_Height
typedef int (WINAPI *PFN_CommandBar_Height)(HWND hwndCB);


//	IsCommandBarMessage
typedef BOOL (WINAPI *PFN_IsCommandBarMessage)(
	HWND hwndCB, LPMSG lpMsg);

//	PropertySheetW
typedef int (WINAPI *PFN_PropertySheetW)(LPCPROPSHEETHEADERW);

//	CreatePropertySheetPageW
typedef HPROPSHEETPAGE (WINAPI *PFN_CreatePropertySheetPageW)(
	LPCPROPSHEETPAGEW);

//	DestroyPropertySheetPage
typedef BOOL (WINAPI *PFN_DestroyPropertySheetPage)(
	HPROPSHEETPAGE);

//  CreateUpDownControl
typedef HWND (WINAPI *PFN_CreateUpDownControl)(DWORD dwStyle,
		int x, int y, int cx, int cy,
		HWND hParent, int nID, HINSTANCE hInst,
		HWND hBuddy,
		int nUpper, int nLower, int nPos);

// The following aren't in the public headers...
//
//	CommandBar_InsertControl
//	CommandBar_GetItemWindow
//  InvertRect
//	DSA_Create                 
//	DSA_Destroy                
//	DSA_GetItem                 
//	DSA_GetItemPtr            
//	DSA_InsertItem             
//	DSA_SetItem                 
//	DSA_DeleteItem              
//	DSA_DeleteAllItems          
//	
//	DPA_Destroy                
//	DPA_GetPtr              
//
//	Str_SetPtrW
//	StrToIntW                     
//
//	CenterWindow



#define IND_InitCommonControls			0
#define IND_InitCommonControlsEx		1
#define IND_CreateStatusWindowW			2
#define IND_DrawStatusTextW				3
#define IND_CommandBar_Create			4
#define IND_CommandBar_Show				5
#define IND_CommandBar_AddBitmap		6
#define IND_CommandBar_InsertComboBox	7
#define IND_CommandBar_InsertMenubar	8
#define IND_CommandBar_GetMenu			9
#define IND_CommandBar_AddAdornments	10
#define IND_CommandBar_Height			11
#define IND_IsCommandBarMessage			12
#define IND_PropertySheetW				13
#define IND_CreatePropertySheetPageW	14
#define IND_DestroyPropertySheetPage	15
#define IND_CreateUpDownControl			16
#define MAX_COMMCTRL_FUNCTIONS			17

#define DECLARE_COMMCTRL_TABLE	DWORD	CommCtrlFunctions[MAX_COMMCTRL_FUNCTIONS]
extern DWORD CommCtrlFunctions[];

#define CallCommCtrlFunc(name) (*((PFN_##name)(CommCtrlFunctions[IND_##name])))
#define ValidCommCtrlFunc(name) (CommCtrlFunctions[IND_##name] ? TRUE : FALSE)

_inline BOOL
InitCommCtrlTable()
{
    HINSTANCE   hInst;
	int			i;

    if (NULL == (hInst = LoadLibrary(TEXT("commctrl.dll")))) {
        return FALSE;
    }

	CommCtrlFunctions[IND_InitCommonControls] =
		(DWORD)GetProcAddressW(hInst, TEXT("InitCommonControls"));
	CommCtrlFunctions[IND_InitCommonControlsEx] =
		(DWORD)GetProcAddressW(hInst, TEXT("InitCommonControlsEx"));

	CommCtrlFunctions[IND_CreateStatusWindowW] =
		(DWORD)GetProcAddressW(hInst, TEXT("CreateStatusWindowW"));
	
	CommCtrlFunctions[IND_DrawStatusTextW] =
		(DWORD)GetProcAddressW(hInst, TEXT("DrawStatusTextW"));
	
	CommCtrlFunctions[IND_CommandBar_Create] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_Create"));
	
	CommCtrlFunctions[IND_CommandBar_Show] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_Show"));
	
	CommCtrlFunctions[IND_CommandBar_AddBitmap] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_AddBitmap"));
	
	CommCtrlFunctions[IND_CommandBar_InsertComboBox] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_InsertComboBox"));
	
	CommCtrlFunctions[IND_CommandBar_InsertMenubar] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_InsertMenubar"));
	
	CommCtrlFunctions[IND_CommandBar_GetMenu] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_GetMenu"));
	
	CommCtrlFunctions[IND_CommandBar_AddAdornments] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_AddAdornments"));
	
	CommCtrlFunctions[IND_CommandBar_Height] =
		(DWORD)GetProcAddressW(hInst, TEXT("CommandBar_Height"));
	
	CommCtrlFunctions[IND_IsCommandBarMessage] =
		(DWORD)GetProcAddressW(hInst, TEXT("IsCommandBarMessage"));
	
	CommCtrlFunctions[IND_PropertySheetW] =
		(DWORD)GetProcAddressW(hInst, TEXT("PropertySheetW"));
	
	CommCtrlFunctions[IND_CreatePropertySheetPageW] =
		(DWORD)GetProcAddressW(hInst, TEXT("CreatePropertySheetPageW"));
	
	CommCtrlFunctions[IND_DestroyPropertySheetPage] =
		(DWORD)GetProcAddressW(hInst, TEXT("DestroyPropertySheetPage"));
	
	CommCtrlFunctions[IND_CreateUpDownControl] =
		(DWORD)GetProcAddressW(hInst, TEXT("CreateUpDownControl"));

	// Check for errors.
	for (i=0; i < MAX_COMMCTRL_FUNCTIONS; i++) {
		if (CommCtrlFunctions[i] == (DWORD)NULL) {
			return FALSE;
		}
	}

	return TRUE;
}

#ifdef __cplusplus
}
#endif


#endif // _WCOMMCTL_H_
