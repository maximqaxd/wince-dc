/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mextfile.h

Abstract:  

Notes: 


--*/
#ifndef __MEXTFILE_H__
#define __MEXTFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

// Mounted file system api indicies.
#define AFS_API_Unmount 0
#define AFS_API_CreateDirectoryW 2
#define AFS_API_RemoveDirectoryW 3
#define AFS_API_GetFileAttributesW 4
#define AFS_API_SetFileAttributesW 5
#define AFS_API_CreateFileW 6
#define AFS_API_DeleteFileW 7
#define AFS_API_MoveFileW 8
#define AFS_API_FindFirstFileW 9
// 10 was RegisterFileSystemNotification, now obsolete
// 11 was OidGetInfo, now obsolete
#define AFS_API_PrestoChangoFileName 12
#define AFS_API_CloseAllFileHandles 13
#define AFS_API_GetDiskFreeSpace 14
#define AFS_API_NotifyMountedFS 15
#define AFS_API_RegisterFileSystemFunction 16

#define AFS_MAC_Unmount(decl, a0) decl(BOOL, Unmount, (a0))
#define AFS_MAC_CreateDirectoryW(decl, a0) decl(BOOL, CreateDirectoryW, (a0, LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes))
#define AFS_MAC_RemoveDirectoryW(decl, a0) decl(BOOL, RemoveDirectoryW, (a0, LPCWSTR lpPathName))
#define AFS_MAC_GetFileAttributesW(decl, a0) decl(DWORD, GetFileAttributesW, (a0, LPCWSTR  lpFileName))
#define AFS_MAC_SetFileAttributesW(decl, a0) decl(BOOL, SetFileAttributesW, (a0, LPCWSTR lpszName, DWORD dwAttributes))
#define AFS_MAC_CreateFileW(decl, a0) decl(HANDLE, CreateFileW, (a0, HANDLE hProc, LPCWSTR lpFileName, DWORD dwDesiredAccess, \
    DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))
#define AFS_MAC_DeleteFileW(decl, a0) decl(BOOL, DeleteFileW, (a0, LPCWSTR lpFilename))
#define AFS_MAC_MoveFileW(decl, a0) decl(BOOL, MoveFileW, (a0, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName))
#define AFS_MAC_FindFirstFileW(decl, a0) decl(HANDLE, FindFirstFileW, (a0, HANDLE hProc, LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))
#define AFS_MAC_RegisterFileSystemFunction(decl, a0) decl(BOOL, RegisterFileSystemFunction, (a0, SHELLFILECHANGEFUNC_t pShellFunc))
#define AFS_MAC_OidGetInfo(decl, a0) decl(BOOL, OidGetInfo, (a0, CEOID oid, CEOIDINFO *oidInfo))
#define AFS_MAC_PrestoChangoFileName(decl, a0) decl(BOOL, PrestoChangoFileName, (a0, LPCWSTR lpszOldFile, LPCWSTR lpszNewFile))
#define AFS_MAC_CloseAllFileHandles(decl, a0) decl(BOOL, CloseAllFileHandles, (a0, HANDLE hProc))
#define AFS_MAC_GetDiskFreeSpace(decl, a0) decl(BOOL, GetDiskFreeSpace, (a0,	\
	LPCWSTR lpPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, 	\
	LPDWORD lpFreeClusters, LPDWORD lpClusters))
#define AFS_MAC_NotifyMountedFS(decl, a0) decl(void, NotifyMountedFS, (a0, DWORD flags))

#define AFS_DECL(type, api, args)	METHOD_DECL(type, AFS_API_ ## api, args)

#define AFS_Unmount AFS_MAC_Unmount(AFS_DECL, HANDLE hFileSys)
#define AFS_CreateDirectoryW AFS_MAC_CreateDirectoryW(AFS_DECL, HANDLE hFileSys)
#define AFS_RemoveDirectoryW AFS_MAC_RemoveDirectoryW(AFS_DECL, HANDLE hFileSys)
#define AFS_GetFileAttributesW AFS_MAC_GetFileAttributesW(AFS_DECL, HANDLE hFileSys)
#define AFS_SetFileAttributesW AFS_MAC_SetFileAttributesW(AFS_DECL, HANDLE hFileSys)
#define AFS_CreateFileW AFS_MAC_CreateFileW(AFS_DECL, HANDLE hFileSys)
#define AFS_DeleteFileW AFS_MAC_DeleteFileW(AFS_DECL, HANDLE hFileSys)
#define AFS_MoveFileW AFS_MAC_MoveFileW(AFS_DECL, HANDLE hFileSys)
#define AFS_FindFirstFileW AFS_MAC_FindFirstFileW(AFS_DECL, HANDLE hFileSys)
#define AFS_RegisterFileSystemFunction AFS_MAC_RegisterFileSystemFunction(AFS_DECL, HANDLE hFileSys)
#define AFS_PrestoChangoFileName AFS_MAC_PrestoChangoFileName(AFS_DECL, HANDLE hFileSys)
#define AFS_CloseAllFileHandles AFS_MAC_CloseAllFileHandles(AFS_DECL, HANDLE hFileSys)
#define AFS_GetDiskFreeSpace AFS_MAC_GetDiskFreeSpace(AFS_DECL, HANDLE hFileSys)
#define AFS_NotifyMountedFS AFS_MAC_NotifyMountedFS(AFS_DECL, HANDLE hFileSys)


#ifdef __cplusplus
}
#endif

#endif  // __EXTFILE_H__
