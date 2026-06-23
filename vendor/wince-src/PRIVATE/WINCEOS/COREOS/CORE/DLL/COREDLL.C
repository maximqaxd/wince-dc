/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
#include <windows.h>
#include <coredll.h>
#include <..\..\gwe\inc\dlgmgr.h>

HANDLE hInstCoreDll;

#ifdef DEBUG
DBGPARAM dpCurSettings = { TEXT("Coredll"), {
    TEXT("FixHeap"),    TEXT("LocalMem"),  TEXT("Mov"),       TEXT("SmallBlock"),
	TEXT("VirtMem"),    TEXT("Devices"),   TEXT("Undefined"), TEXT("Undefined"),
	TEXT("Stdio"),   TEXT("Stdio HiFreq"), TEXT("Shell APIs"), TEXT("Imm"),
	TEXT("Undefined"),  TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined") },
	0x00000000 };
#endif

size_t mbstowcs(wchar_t *wcstr, const char *mbstr, size_t count) {
	int	RetVal;

	if (NULL == wcstr) {
		// Determine how many characters are required
		RetVal = MultiByteToWideChar(CP_ACP, 0, mbstr, -1, NULL, 0);
		if (0 == RetVal) {
			RetVal = -1;
		} else {
			// MultiByteToWideChar includes the terminator.
			RetVal = RetVal--;
		}
		return RetVal;
	}
	RetVal = MultiByteToWideChar(CP_ACP, 0, mbstr, (strlen (mbstr) < count)? -1 : count, wcstr, count);
	
	// Fix up return code.  MultiByteToWideChar returns 0 on error
	// mbstowcs should return -1.
	if ((0 == RetVal) && GetLastError()) {
		RetVal = -1;
	} else if (RetVal && (TEXT('\0') == wcstr[RetVal - 1])) {
		// MultiByteToWideChar returned length includes the null.  mbstowcs does not
		RetVal--;
	}
	return (size_t)RetVal;
}

size_t wcstombs(char *mbstr, const wchar_t *wcstr, size_t count) {
	int	RetVal;

	if (NULL == mbstr) {
		RetVal = WideCharToMultiByte(CP_ACP, 0, wcstr, -1, NULL, 0, NULL, NULL);
		if (0 == RetVal) {
			RetVal = -1;
		} else {
			RetVal--;
		}
		return RetVal;
	}
	RetVal = WideCharToMultiByte(CP_ACP, 0, wcstr, (wcslen (wcstr) < count)? -1 : count, mbstr, count, NULL, NULL);

	// Fix up return code.  WideCharToMultiByte returns 0 on error
	// wcstombs should return -1.
	if ((0 == RetVal) && GetLastError()) {
		RetVal = -1;
	} else if (RetVal && ('\0' == mbstr[RetVal - 1])) {
		// WideCharToMultiByte returned length includes the null.  wcstombs does not
		RetVal--;
	}
	return (size_t)RetVal;
}

LPVOID *Win32Methods;
LPVOID *pFns;
DWORD bAllKMode;
DWORD bProfilingKernel;

void InitLocale(void);
BOOL Imm_DllEntry(HANDLE hinstDll, DWORD dwReason, LPVOID lpvReserved);
BOOL WINAPI _CRTDLL_INIT(HANDLE hinstDll, DWORD dwReason, LPVOID lpreserved);

#ifdef WINCECODETEST
DWORD ProfileInit(void);
static BOOL FirstTime = TRUE;
#endif

BOOL WINAPI CoreDllInit (HANDLE  hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

#ifdef WINCECODETEST
	if (FirstTime)
	{
		ProfileInit();
		FirstTime = FALSE;
	}
#endif
	
	hInstCoreDll = hinstDLL;
	if (GetCurrentProcessIndex()) {
	    if (fdwReason == DLL_PROCESS_ATTACH) {
			BOOL IsAPIReady(DWORD hAPI);
	    	GetRomFileInfo(3,(LPWIN32_FIND_DATA)&Win32Methods,(DWORD)&bAllKMode);
	    	GetRomFileInfo(4,(LPWIN32_FIND_DATA)&pFns, 0);
	    	GetRomFileInfo(5,(LPWIN32_FIND_DATA)&bProfilingKernel, 0);
	        DEBUGREGISTER(hinstDLL);
	        if(!LMemInit())
	           DEBUGCHK(0);
   			InitLocale();
			Imm_DllEntry(hinstDLL, fdwReason, lpvReserved);
			if (IsAPIReady(SH_WMGR)) {
#define DIALOGCLASSNAME TEXT("Dialog")
				WNDCLASS  wc;
				wc.style         = 0;//CS_SYSTEMCLASS/*CS_HREDRAW | CS_VREDRAW*/;
				wc.lpfnWndProc   = xxx_DefDlgProcW;
				wc.cbClsExtra    = 0;
				wc.cbWndExtra    = sizeof(DLG1);
				wc.hInstance     = hinstDLL;
				wc.hIcon         = NULL;
				wc.hCursor       = NULL;
				wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
				wc.lpszMenuName  = NULL;
				wc.lpszClassName = DIALOGCLASSNAME;
				RegisterClass(&wc);
			}
		} else if ( fdwReason == DLL_THREAD_ATTACH) {
			Imm_DllEntry(hinstDLL, fdwReason, lpvReserved);
		} else if (fdwReason == DLL_THREAD_DETACH) {
	    	LPVOID pBuf;
			Imm_DllEntry(hinstDLL, fdwReason, lpvReserved);
			if ((pBuf = TlsGetValue(TLSSLOT_RUNTIME)) && ((DWORD)pBuf >= 0x10000))
				LocalFree((LPVOID)ZeroPtr(pBuf));
	    } else if (fdwReason == DLL_PROCESS_DETACH) {
			Imm_DllEntry(hinstDLL, fdwReason, lpvReserved);
		}
	    _CRTDLL_INIT(hinstDLL,fdwReason,lpvReserved);
	}
    return(TRUE);
}

