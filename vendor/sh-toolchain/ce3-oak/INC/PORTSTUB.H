/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    portstub.h

Abstract:

    Miscellaneous useful utilities

--*/

#if ! defined (__portstub_H__)
#define __portstub_H__	1

#if ! defined (__cplusplus)
#define inline __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
//	Missing symbols
//
#if ! defined (NT_SUCCESS)
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#if ! defined (NT_INFORMATION)
#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)
#endif

#if ! defined (NT_WARNING)
#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)
#endif

#if ! defined (NT_ERROR)
#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)
#endif

#define MAX_COMPUTERNAME_LENGTH 31
#define LPEXCEPTION_POINTERS PEXCEPTION_POINTERS

#define MAXULONG    0xffffffff  // winnt

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

//
//	Function stubs
//
inline time_t time (time_t *t) {
	SYSTEMTIME st;
	LARGE_INTEGER ft;

	GetLocalTime (&st);

	SystemTimeToFileTime (&st, (FILETIME *)&ft);

    ft.QuadPart -= 0x019db1ded53e8000;
    ft.QuadPart /= 10000000;

	if (t)
		*t = ft.LowPart;

    return ft.LowPart;
}

inline HANDLE OpenEvent (DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName) {
	return CreateEvent (NULL, FALSE, FALSE, lpName);
}

inline LONG RegQueryValue(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, PLONG lpcbValue) {
	HKEY hkey = NULL;
	LONG hr;

	if (! lpSubKey) {
		LONG hr = RegQueryValueEx (hKey, lpSubKey, NULL, NULL, (LPBYTE)lpValue, (LPDWORD)lpcbValue);
		return (hr == ERROR_INVALID_PARAMETER) ? ERROR_FILE_NOT_FOUND : hr;
	}

	hr = RegOpenKeyEx (hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hkey);
	if (hr != ERROR_SUCCESS)
		return (hr == ERROR_INVALID_PARAMETER) ? ERROR_FILE_NOT_FOUND : hr;

	hr = RegQueryValueEx (hkey, NULL, NULL, NULL, (LPBYTE)lpValue, (LPDWORD)lpcbValue);
	RegCloseKey (hkey);

	return (hr == ERROR_INVALID_PARAMETER) ? ERROR_FILE_NOT_FOUND : hr;
}

inline LONG RegSetValue(  HKEY hKey,         // handle to key to set value for
  LPCTSTR lpSubKey,  // address of subkey name
  DWORD dwType,      // type of value, MUST BE REG_SZ
  LPCTSTR lpData,    // address of value data
  DWORD cbData       // size of value data, IN CHARACTERS excl NULL
) {
	HKEY hkey = NULL;
	LONG hr;

	if (! lpSubKey)
		return RegSetValueEx (hKey, lpSubKey, 0, dwType, (LPBYTE)lpData, cbData);

	hr = RegCreateKeyEx (hKey, lpSubKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, NULL);
	if (hr != ERROR_SUCCESS)
		return hr;

	hr = RegSetValueEx (hkey, NULL, 0, dwType, (LPBYTE)lpData, (cbData + 1) * sizeof(WCHAR));
	RegCloseKey (hkey);

	return hr;
}


inline LONG RegOpenKey(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult) {
	LONG hr = RegOpenKeyEx (hKey, lpSubKey, 0, KEY_ALL_ACCESS, phkResult);
	return (hr == ERROR_INVALID_PARAMETER) ? ERROR_FILE_NOT_FOUND : hr;
}

#define RegOpenKeyW	RegOpenKey

inline LONG RegEnumKey(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cbName) {
	FILETIME ft;
	return RegEnumKeyEx (hKey, dwIndex, lpName, &cbName, 0, NULL, NULL, &ft);
}

inline LONG RegNotifyChangeKeyValue(
  HKEY hKey,             // handle to key to watch
  BOOL bWatchSubtree,    // flag for subkey notification
  DWORD dwNotifyFilter,  // changes to be reported
  HANDLE hEvent,         // handle to signaled event
  BOOL fAsynchronous     // flag for asynchronous reporting
) {
	return ERROR_SUCCESS;
}

inline LPVOID MapViewOfFileEx(
  HANDLE hFileMappingObject,  // file-mapping object to map into 
                              // address space
  DWORD dwDesiredAccess,      // access mode
  DWORD dwFileOffsetHigh,     // high-order 32 bits of file offset
  DWORD dwFileOffsetLow,      // low-order 32 bits of file offset
  DWORD dwNumberOfBytesToMap, // number of bytes to map
  LPVOID lpBaseAddress        // suggested starting address for mapped view
) {
	return MapViewOfFile (hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
}


inline HANDLE OpenFileMapping (DWORD dwAccess, BOOL bInherit, LPCTSTR lpName) {
	return CreateFileMapping ((HANDLE)0xffffffff, NULL, 0, 0, 0, lpName);
}

inline LCID GetThreadLocale (VOID) {
	return GetUserDefaultLCID ();
}

inline DWORD GetShortPathName(
  LPCTSTR lpszLongPath,  // pointer to a null-terminated path string
  LPTSTR lpszShortPath,  // pointer to a buffer to receive the 
                         // null-terminated short form of the path
  DWORD cchBuffer        // specifies the size of the buffer pointed 
                         // to by lpszShortPath
) {
	DWORD cch = _tcslen (lpszLongPath) + 1;
	if (cch > cchBuffer)
		return cch;

	memcpy (lpszShortPath, lpszLongPath, cch * sizeof(TCHAR));

	return cch - 1;
}

inline BOOL AreFileApisANSI(VOID) {
	return TRUE;
}

#define GetDriveType  GetDriveTypeW

inline UINT GetDriveTypeW(LPCTSTR lpRootPathName) {
    if (wcsnicmp (lpRootPathName, L"\\Network\\", 9) == 0)
        return DRIVE_REMOTE;

	return DRIVE_FIXED;
}


inline DWORD GetFullPathName(
  LPCTSTR lpFileName,  // pointer to name of file to find path for
  DWORD nBufferLength, // size, in characters, of path buffer
  LPTSTR lpBuffer,     // pointer to path buffer
  LPTSTR *lpFilePart   // pointer to filename in path
  ) {
   DWORD cch = _tcslen (lpFileName) + 1;

   if (*lpFileName == TEXT('\\')) {
      if (cch > nBufferLength)
         return cch;
      memcpy (lpBuffer, lpFileName, sizeof(TCHAR) * cch);

      return cch - 1;
   }
  
   if ((cch + 1) > nBufferLength)
      return cch + 1;

   lpBuffer[0] = TEXT('\\');
   memcpy (lpBuffer + 1, lpFileName, sizeof(TCHAR) * cch);

   return cch;
}

#define GetCurrentTime() GetTickCount()

inline DWORD GetProfileString(
  LPCTSTR lpAppName,        // address of section name
  LPCTSTR lpKeyName,        // address of key name
  LPCTSTR lpDefault,        // address of default string
  LPTSTR lpReturnedString,  // address of destination buffer
  DWORD nSize               // size of destination buffer
) {
	TCHAR szKeyName[512];
	HKEY hkey 	   = NULL;

	_tcscpy (szKeyName, TEXT("Profile\\"));
	_tcscat (szKeyName, lpAppName);

	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
		DWORD dwType = 0;
		DWORD dwSize = nSize * sizeof(TCHAR);

		if ((RegQueryValueEx (hkey, lpKeyName, NULL, &dwType, (LPBYTE)lpReturnedString, &dwSize) == ERROR_SUCCESS) &&
			(dwType == REG_SZ)) {
			RegCloseKey (hkey);
			return dwSize / sizeof(TCHAR);
		}

		RegCloseKey (hkey);
	}

	if (lpDefault) {
		int sz = _tcslen (lpDefault);
		if (sz >= (int)nSize)
			sz = nSize - 1;

		memcpy (lpReturnedString, lpDefault, sz * sizeof(TCHAR));
		lpReturnedString[sz] = TEXT('\0');

		return sz;
	}

	return 0;
}

inline BOOL WriteProfileString(  LPCTSTR lpAppName, // pointer to section name
  LPCTSTR lpKeyName, // pointer to key name
  LPCTSTR lpString   // pointer to string to write
) {
	TCHAR szKeyName[512];
	HKEY hkey 	   = NULL;
	DWORD dwDisp   = 0;

	_tcscpy (szKeyName, TEXT("Profile\\"));
	_tcscat (szKeyName, lpAppName);

	if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp) == ERROR_SUCCESS) {
		if (! lpString) {
			if (RegDeleteValue (hkey, lpKeyName) == ERROR_SUCCESS) {
				RegCloseKey (hkey);
				return TRUE;
			}
		} else if (RegSetValueEx (hkey, lpKeyName, 0, REG_SZ, (LPBYTE)lpString, (_tcslen(lpString) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
			RegCloseKey (hkey);
			return TRUE;
		}

		RegCloseKey (hkey);
	}

	return FALSE;
}

inline HANDLE RegisterEventSource(
const WCHAR *lpUNCServerName,
const WCHAR *lpSourceName) {
    WCHAR szBuffer[_MAX_PATH];
    wsprintf (szBuffer, L"\\Temp\\%s - %s.log", lpUNCServerName ? lpUNCServerName : L"$localhost$", lpSourceName);
    return (HANDLE)_wfopen (szBuffer, L"a");
}

inline BOOL DeregisterEventSource(  HANDLE hEventLog) {
	if (hEventLog)
		fclose ((FILE *)hEventLog);

	return TRUE;
}

inline BOOL ReportEvent(
  HANDLE hEventLog,    // handle returned by RegisterEventSource
  WORD wType,          // event type to log
  WORD wCategory,      // event category
  DWORD dwEventID,     // event identifier
  PSID lpUserSid,      // user security identifier (optional)
  WORD wNumStrings,    // number of strings to merge with message
  DWORD dwDataSize,    // size of binary data, in bytes
  LPCWSTR *lpStrings,  // array of strings to merge with message
  LPVOID lpRawData     // address of binary data
  ) {
   	SYSTEMTIME st;
   	int i;

   	FILE *fp = (FILE *)hEventLog;
   	if (! fp)
   		return FALSE;

   	GetLocalTime (&st);
   	fwprintf (fp, L"Event %04x-%04x-%08x @ %02d:%02d:%02d.%03d %02d/%02d/%04d\n",
   					wType, wCategory, dwEventID,
   					st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
   					st.wMonth, st.wDay, st.wYear);

   	for (i = 0 ; i < wNumStrings ; ++i )
   		fwprintf (fp, L"\t%s\n", lpStrings[i]);

   	if (lpRawData && (dwDataSize > 0)) {
   		for (i = 0 ; i < (int)dwDataSize ; ++i) {
   		    if ((i % 16) == 0)
   		    	fwprintf (fp, L"\n\t");
   		    fwprintf (fp, L"%02x ", *((unsigned char *)lpRawData + i));
   		}
   	}

	fwprintf (fp, L"\n\n");

	return TRUE;
}

inline BOOL GetComputerName(WCHAR *lpBuffer, LPDWORD nSize) {
	TCHAR szBuffer[_MAX_PATH];
	DWORD dwType = REG_SZ;
	DWORD dwSize = sizeof(szBuffer);
	HKEY hKey = NULL;
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Ident"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return FALSE;

	if ((RegQueryValueEx (hKey, TEXT("name"), 0, &dwType, (LPBYTE)szBuffer, &dwSize) != ERROR_SUCCESS) ||
		(dwType != REG_SZ) || (dwSize > sizeof(szBuffer))) {
		RegCloseKey (hKey);
		return FALSE;
	}
	RegCloseKey (hKey);

	dwSize = wcslen (szBuffer) + 1;

	if (dwSize > *nSize) {
	    *nSize = dwSize;
	    SetLastError (ERROR_INSUFFICIENT_BUFFER);
	    return FALSE;
	}

    *nSize = dwSize;
	memcpy (lpBuffer, szBuffer, dwSize * sizeof(TCHAR));
	return TRUE;
}

#define GetComputerNameW GetComputerName

#define GetModuleFileNameA GetModuleFileNameCEA

inline DWORD GetModuleFileNameCEA(
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
    ) {
    WCHAR szBuffer[_MAX_PATH];
    if (! GetModuleFileName (hModule, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
    	return 0;

    return WideCharToMultiByte (CP_ACP, 0, szBuffer, -1, lpFilename, nSize, NULL, NULL);
}

#define wsprintfA sprintf

inline LARGE_INTEGER
RtlLargeIntegerSubtract (
    LARGE_INTEGER Minuend,
    LARGE_INTEGER Subtrahend
    ) {
    LARGE_INTEGER res;
    
    res.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;

    return res;
}

inline LARGE_INTEGER
RtlLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER Divisor,
    LARGE_INTEGER *Remainder
    ) {
    LARGE_INTEGER res;

    if (Remainder)
    	Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;

    res.QuadPart = Dividend.QuadPart / Divisor.QuadPart;

    return res;
}

inline LARGE_INTEGER
RtlExtendedIntegerMultiply (
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
    ) {
    LARGE_INTEGER res;

    res.QuadPart = Multiplicand.QuadPart * Multiplier;

    return res;
}

inline LONG
NtQueryPerformanceCounter (
    LARGE_INTEGER *PerformanceCounter,
    LARGE_INTEGER *PerformanceFrequency
    ) {
    PerformanceCounter->QuadPart = GetTickCount ();
    PerformanceFrequency->QuadPart = 1000;
    return 0;
}
 
#ifdef __cplusplus
}
#endif

#endif

