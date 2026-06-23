/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pwinbase.h

Abstract:

Private portion of winbase.h

Notes: 


--*/
    
	#ifndef _PRIV_WINBASE_H__
	#define _PRIV_WINBASE_H__

	//
	// Define API decoration for direct importing of DLL references.
	//
	    
	#include <dbgapi.h>

#ifdef SHx
#define SHxCOMPILERPDATABug	int SHxCompilerPDATABug = 0
#else
#define	SHxCOMPILERPDATABug	(0)
#endif

	#ifdef __cplusplus
	extern "C" {
	#endif
	    
	// exported from coredll
	BOOL AttachDebugger(LPCWSTR dbgname);
	#define LocalDiscard( h )   LocalReAlloc( (h), 0, LMEM_MOVEABLE )
	HLOCAL WINAPI RemoteLocalAlloc( UINT uFlags, UINT uBytes );
	HLOCAL WINAPI RemoteLocalReAlloc( HLOCAL hMem, UINT uBytes, UINT uFlags );
	UINT WINAPI RemoteLocalSize( HLOCAL hMem ); 
	HLOCAL WINAPI RemoteLocalFree( HLOCAL hMem );
	UINT WINAPI LocalSizeInProcess( HLOCAL hMem, HANDLE	hProc ); 
	HLOCAL WINAPI LocalAllocInProcess( UINT	uFlags, UINT	uBytes, HANDLE	hProc ); 
	HLOCAL WINAPI LocalFreeInProcess( HLOCAL	hMem, HANDLE	hProc ); 
	VOID WINAPI SystemStarted(void);
	VOID WINAPI SystemMemoryLow(void);
	
	// exported from the filesystem
	void FileSystemPowerFunction (DWORD flags);
	void CloseAllFileHandles ();
	// @CESYSGEN IF FILESYS_FSPASS
	BOOL SetPassword (LPWSTR lpszOldpassword, LPWSTR lspzNewPassword);
	BOOL GetPasswordActive(void);
	BOOL SetPasswordActive (BOOL bActive, LPWSTR lpszPassword);
	// @CESYSGEN ENDIF
	BOOL ReadFileWithSeek(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);
	BOOL WriteFileWithSeek(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, PDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);

	// exported from device
  	void CloseAllDeviceHandles (HANDLE hProc);
	HANDLE CreateDeviceHandle (LPCWSTR, DWORD, DWORD, HPROCESS);
	BOOL GetDeviceByIndex(DWORD dwIndex, LPWIN32_FIND_DATA lpFindFileData);

	// special multi-language redirection resource
	#define RT_MUI	222	// type of the redirection resource
	#define ID_MUI	1	// ID of the redirection resource
	// the redirection resource is a wide-string, whose value is the basename of the resource DLL

#define FILE_SHARE_WRITE_OVERRIDE 	0x40000000
	
	#ifdef __cplusplus
	}
	#endif

	#endif /* _PRIV_WINBASE_H__ */
