/*
 *		NK Kernel stubs code
 *
 *		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *		stubs.c
 *
 * Abstract:
 *
 *		This file implements stubs/duplicates for functions in CoreDll
 *
 */

/* Copies of functions in CoreDll which are needed here because they're needed
   to load CoreDll, but also need to be user-callable without trapping (thus they
   are also in CoreDll). */

#include "kernel.h"

HANDLE hCoreDll;

/* -------------------- functions chained to DLL ------------------- */

typedef HLOCAL (* LA_t)(UINT, UINT);

// Win32 LocalAlloc call

HLOCAL WINAPI LocalAlloc(UINT uFlags, UINT uBytes) {
	static LA_t LAf = 0;
	if (!hCoreDll || (!LAf && !(LAf = (LA_t)GetProcAddressA(hCoreDll,"LocalAlloc"))))
		return FALSE;
	return LAf(uFlags,uBytes);
}

typedef HLOCAL (* LF_t)(HLOCAL);

// Win32 CreateProcess call

HLOCAL WINAPI LocalFree(HLOCAL hMem) {
	static LF_t LFf = 0;
	if (!hCoreDll || (!LFf && !(LFf = (LF_t)GetProcAddressA(hCoreDll,"LocalFree"))))
		return FALSE;
	return LFf(hMem);
}

/* -------------------- functions duplicated from DLL ------------------- */

#ifdef DEBUG
#define DOCHECKCS
#endif

#include "..\..\core\dll\cscode.c"

