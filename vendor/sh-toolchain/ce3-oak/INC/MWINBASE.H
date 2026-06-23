/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mwinbase.h

Abstract:  

Notes: 


--*/

#ifndef __MWINBASE_H__
#define __MWINBASE_H__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

/* Win32 Base Dir API Function Indices */
#ifdef BUILDING_FS_STUBS
#define WIN32_FS_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(SH_FILESYS_APIS, api))
#define WIN32_FILE_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(HT_FILE, api))
#define WIN32_FIND_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(HT_FIND, api))
#define WIN32_DBFILE_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(HT_DBFILE, api))
#define WIN32_DBFIND_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(HT_DBFIND, api))
#endif
#define PRIV_WIN32_FS_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(SH_FILESYS_APIS, api))
#define PRIV_WIN32_FILE_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(HT_FILE, api))
#define WIN32_DEV_CALL(type, api, args) (*(type (*) args)IMPLICIT_CALL(SH_DEVMGR_APIS, api))

// external exports from filesys
#ifdef WIN32_FS_CALL
#define CreateDirectoryW WIN32_FS_CALL(BOOL, 2, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes))
#define RemoveDirectoryW WIN32_FS_CALL(BOOL, 3, (LPCWSTR lpPathName))
#define MoveFileW WIN32_FS_CALL(BOOL, 4, (LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName))
#define CopyFileW WIN32_FS_CALL(BOOL, 5, (LPCTSTR  lpszExistingFile, LPCTSTR  lpszNewFile, BOOL  fFailIfExists))
#define DeleteFileW WIN32_FS_CALL(BOOL, 6, (LPCWSTR lpFileName))
#define GetFileAttributesW WIN32_FS_CALL(DWORD, 7, (LPCTSTR  lpFileName))
#define FindFirstFileW WIN32_FS_CALL(HANDLE, 8, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))
#define CreateFileW WIN32_FS_CALL(HANDLE, 9, (LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))
#define SetFileAttributesW WIN32_FS_CALL(BOOL, 31, (LPCWSTR lpszName, DWORD dwAttributes))
#define ReadFile WIN32_FILE_CALL(BOOL, 2, (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped))
#define WriteFile WIN32_FILE_CALL(BOOL, 3, (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped))
#define GetFileSize WIN32_FILE_CALL(DWORD, 4, (HANDLE hFile, LPDWORD lpFileSizeHigh))
#define SetFilePointer WIN32_FILE_CALL(DWORD, 5, (HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod))
#define GetFileInformationByHandle WIN32_FILE_CALL(DWORD, 6, (HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation))
#define FlushFileBuffers WIN32_FILE_CALL(BOOL, 7, (HANDLE hFile))
#define GetFileTime WIN32_FILE_CALL(BOOL, 8, (HANDLE hFile, LPFILETIME lpCreation, LPFILETIME lpLastAccess, LPFILETIME lpLastWrite))
#define SetFileTime WIN32_FILE_CALL(BOOL, 9, (HANDLE hFile, CONST FILETIME *lpCreation, CONST FILETIME *lpLastAccess, CONST FILETIME *lpLastWrite))
#define SetEndOfFile WIN32_FILE_CALL(BOOL, 10, (HANDLE hFile))
#define DeviceIoControl WIN32_FILE_CALL(BOOL, 11, (HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped))
#define FindClose WIN32_FIND_CALL(BOOL, 0, (HANDLE hFindFile))
#define FindNextFileW WIN32_FIND_CALL(BOOL, 2, (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData))
#define DeleteAndRenameFile WIN32_FS_CALL (BOOL, 43, (LPCWSTR lpszOldFile, LPCWSTR lpszNewFile))
#define CheckPassword WIN32_FS_CALL (BOOL, 29, (LPWSTR lpszPassword))
#define GetDiskFreeSpaceExW WIN32_FS_CALL (BOOL, 54, (LPCTSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes))
#define GetTempPathW WIN32_FS_CALL (DWORD, 27, (DWORD nBufferLength, LPWSTR lpBuffer))
#define FindFirstFileExW WIN32_FS_CALL(HANDLE, 63, (LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags))

#elif defined(COREDLL)

BOOL xxx_CreateDirectoryW (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#define CreateDirectoryW xxx_CreateDirectoryW
BOOL xxx_RemoveDirectoryW (LPCWSTR lpPathName);
#define RemoveDirectoryW xxx_RemoveDirectoryW
BOOL xxx_MoveFileW (LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
#define MoveFileW xxx_MoveFileW
BOOL xxx_CopyFileW (LPCTSTR  lpszExistingFile, LPCTSTR  lpszNewFile, BOOL  fFailIfExists);
#define CopyFileW xxx_CopyFileW
BOOL xxx_DeleteFileW (LPCWSTR lpFileName);
#define DeleteFileW xxx_DeleteFileW
DWORD xxx_GetFileAttributesW (LPCTSTR  lpFileName);
#define GetFileAttributesW xxx_GetFileAttributesW
HANDLE xxx_FindFirstFileW (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
#define FindFirstFileW xxx_FindFirstFileW
HANDLE xxx_CreateFileW (LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#define CreateFileW xxx_CreateFileW
BOOL xxx_SetFileAttributesW (LPCWSTR lpszName, DWORD dwAttributes);
#define SetFileAttributesW xxx_SetFileAttributesW
BOOL xxx_ReadFile (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
#define ReadFile xxx_ReadFile
BOOL xxx_WriteFile (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
#define WriteFile xxx_WriteFile
DWORD xxx_GetFileSize (HANDLE hFile, LPDWORD lpFileSizeHigh);
#define GetFileSize xxx_GetFileSize
DWORD xxx_SetFilePointer (HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
#define SetFilePointer xxx_SetFilePointer
DWORD xxx_GetFileInformationByHandle (HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
#define GetFileInformationByHandle xxx_GetFileInformationByHandle
BOOL xxx_FlushFileBuffers (HANDLE hFile);
#define FlushFileBuffers xxx_FlushFileBuffers
BOOL xxx_GetFileTime (HANDLE hFile, LPFILETIME lpCreation, LPFILETIME lpLastAccess, LPFILETIME lpLastWrite);
#define GetFileTime xxx_GetFileTime
BOOL xxx_SetFileTime (HANDLE hFile, CONST FILETIME *lpCreation, CONST FILETIME *lpLastAccess, CONST FILETIME *lpLastWrite);
#define SetFileTime xxx_SetFileTime
BOOL xxx_SetEndOfFile (HANDLE hFile);
#define SetEndOfFile xxx_SetEndOfFile
BOOL xxx_DeviceIoControl (HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
#define DeviceIoControl xxx_DeviceIoControl
BOOL xxx_FindClose (HANDLE hFindFile);
#define FindClose xxx_FindClose
BOOL xxx_FindNextFileW (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
#define FindNextFileW xxx_FindNextFileW
BOOL xxx_DeleteAndRenameFile (LPCWSTR lpszOldFile, LPCWSTR lpszNewFile);
#define DeleteAndRenameFile xxx_DeleteAndRenameFile
BOOL xxx_CheckPassword (LPWSTR lpszPassword);
#define CheckPassword xxx_CheckPassword
BOOL xxx_GetDiskFreeSpaceExW (LPCTSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes);
#define GetDiskFreeSpaceExW xxx_GetDiskFreeSpaceExW
DWORD xxx_GetTempPathW (DWORD nBufferLength, LPWSTR lpBuffer);
#define GetTempPathW xxx_GetTempPathW
HANDLE xxx_FindFirstFileExW (LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
#define FindFirstFileExW xxx_FindFirstFileExW
#endif

// external exports from device
#define RegisterDevice WIN32_DEV_CALL(HANDLE, 2, (LPCWSTR lpszName, DWORD index, LPCWSTR lpszLib, DWORD dwInfo))
#define DeregisterDevice WIN32_DEV_CALL(BOOL, 3, (HANDLE hDevice))
#define LoadFSD WIN32_DEV_CALL(BOOL, 6, (HANDLE hDevice, LPCWSTR lpFSDName))
#define LoadFSDEx WIN32_DEV_CALL(BOOL, 9, (HANDLE hDevice, LPCWSTR lpFSDName, DWORD dwFlag))
#define ActivateDevice WIN32_DEV_CALL(HANDLE, 7, (LPCWSTR lpszDevKey, DWORD dwClientInfo))
#define DeactivateDevice WIN32_DEV_CALL(BOOL, 8, (HANDLE hDevice))
#define CeResyncFilesys WIN32_DEV_CALL(BOOL, 11, (HANDLE hDevice))

// internal exports from filesystem
#ifdef WIN32_FS_CALL
#define FileSystemPowerFunction WIN32_FS_CALL(void, 50, (DWORD flags))
#define CloseAllFileHandles WIN32_FS_CALL (void, 42, ())
#define SetPassword WIN32_FS_CALL (BOOL, 30, (LPWSTR lpszOldpassword, LPWSTR lspzNewPassword))
#define GetPasswordActive WIN32_FS_CALL (BOOL, 47, (void))
#define SetPasswordActive WIN32_FS_CALL (BOOL, 48, (BOOL bActive, LPWSTR lpszPassword))
#define ReadFileWithSeek WIN32_FILE_CALL(BOOL, 12, (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset))
#define WriteFileWithSeek WIN32_FILE_CALL(BOOL, 13, (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset))

#elif defined(COREDLL)

void xxx_FileSystemPowerFunction (DWORD flags);
#define FileSystemPowerFunction xxx_FileSystemPowerFunction
void xxx_CloseAllFileHandles ();
#define CloseAllFileHandles xxx_CloseAllFileHandles
BOOL xxx_SetPassword (LPWSTR lpszOldpassword, LPWSTR lspzNewPassword);
#define SetPassword xxx_SetPassword
BOOL xxx_GetPasswordActive (void);
#define GetPasswordActive xxx_GetPasswordActive
BOOL xxx_SetPasswordActive (BOOL bActive, LPWSTR lpszPassword);
#define SetPasswordActive xxx_SetPasswordActive
BOOL xxx_ReadFileWithSeek(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);
#define ReadFileWithSeek xxx_ReadFileWithSeek
BOOL xxx_WriteFileWithSeek(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped, DWORD dwLowOffset, DWORD dwHighOffset);
#define WriteFileWithSeek xxx_WriteFileWithSeek
   
#endif

// internal exports from device
#define CloseAllDeviceHandles WIN32_DEV_CALL(void, 4, (HANDLE hProc))
#define CreateDeviceHandle WIN32_DEV_CALL(HANDLE, 5, (LPCWSTR, DWORD, DWORD, HPROCESS))
#define GetDeviceByIndex WIN32_DEV_CALL(BOOL, 10, (DWORD dwIndex, LPWIN32_FIND_DATA lpFindFileData))

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif //__MWINBASE_H__

