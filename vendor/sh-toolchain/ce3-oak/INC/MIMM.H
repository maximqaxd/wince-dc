// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __MIMM_H__
#define __MIMM_H__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef USER_CALL

#define ImmGetContextFromWindowGwe USER_CALL ( HIMC, ImmGetContextFromWindowGwe, (	\
	HWND	hwnd ))


#define ImmAssociateContextWithWindowGwe USER_CALL (BOOL, ImmAssociateContextWithWindowGwe, (	\
	HWND	hwnd,				\
	HIMC	himc,				\
	DWORD	dwFlags,			\
	HIMC	*phimcPrev,			\
	HWND	*phwndFocus,		\
	HIMC	*phimcFocusPrev,	\
	HIMC	*phimcFocusNew))


#define ImmSetHotKey USER_CALL ( BOOL, ImmSetHotKey, ( \
	DWORD	dwHotKeyId,		\
	UINT	uModifiers,		\
	UINT	uVkey,			\
	HKL		hkl))

#define ImmGetHotKey USER_CALL ( BOOL, ImmGetHotKey, (	\
	DWORD	dwHotKeyId,		\
	UINT	*puModifiers,	\
	UINT	*puVkey,		\
	HKL		*phkl))

#elif defined(COREDLL)

HIMC xxx_ImmGetContextFromWindowGwe(HWND hwnd);
#define ImmGetContextFromWindowGwe xxx_ImmGetContextFromWindowGwe

BOOL xxx_ImmAssociateContextWithWindowGwe (HWND	hwnd,HIMC	himc,DWORD	dwFlags,HIMC	*phimcPrev,HWND	*phwndFocus,HIMC	*phimcFocusPrev,HIMC	*phimcFocusNew);
#define ImmAssociateContextWithWindowGwe xxx_ImmAssociateContextWithWindowGwe

BOOL xxx_ImmSetHotKey (DWORD	dwHotKeyId,UINT	uModifiers,UINT	uVkey, HKL		hkl);
#define ImmSetHotKey xxx_ImmSetHotKey

BOOL xxx_ImmGetHotKey (DWORD	dwHotKeyId,UINT	*puModifiers,UINT	*puVkey,HKL		*phkl);
#define ImmGetHotKey xxx_ImmGetHotKey

#endif

#ifdef __cplusplus
}
#endif

#endif

