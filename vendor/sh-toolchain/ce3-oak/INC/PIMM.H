// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef _PRIV_IMM_
#define _PRIV_IMM_

#ifdef __cplusplus
extern "C"  {
#endif

#define IGSC_DEFIMCFALLBACK		0x0002
HIMC
WINAPI
ImmGetContextFromWindowGwe(
	HWND	hWnd
	);


BOOL
WINAPI
ImmAssociateContextWithWindowGwe(
	HWND	hwnd,
	HIMC	himc,
	DWORD	dwFlags,
	HIMC	*phimcPrev,
	HWND	*phwndFocus,
	HIMC	*phimcFocusPrev,
	HIMC	*phimcFocusNew
	);



void
WINAPI
ImmSetImeWndIMC(
	HIMC	hImc,
	HWND	hImeWnd
	);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif
