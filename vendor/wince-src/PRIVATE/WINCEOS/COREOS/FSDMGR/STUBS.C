

















#include "fsdmgrp.h"


/*  FSDMGRStub_CloseVolume
 */

BOOL FSDMGRStub_CloseVolume(PVOL pVol)
{
    return TRUE;
}


/*  FSDMGRStub_CreateDirectoryW
 */

BOOL FSDMGRStub_CreateDirectoryW(PVOL pVol, PCWSTR pwsPathName, PSECURITY_ATTRIBUTES pSecurityAttributes)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_RemoveDirectoryW
 */

BOOL FSDMGRStub_RemoveDirectoryW(PVOL pVol, PCWSTR pwsPathName)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_GetFileAttributesW
 */

DWORD FSDMGRStub_GetFileAttributesW(PVOL pVol, PCWSTR pwsFileName)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0xFFFFFFFF;
}


/*  FSDMGRStub_SetFileAttributesW
 */

BOOL FSDMGRStub_SetFileAttributesW(PVOL pVol, PCWSTR pwsFileName, DWORD dwAttributes)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_DeleteFileW
 */

BOOL FSDMGRStub_DeleteFileW(PVOL pVol, PCWSTR pwsFileName)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_MoveFileW
 */

BOOL FSDMGRStub_MoveFileW(PVOL pVol, PCWSTR pwsOldFileName, PCWSTR pwsNewFileName)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_DeleteAndRenameFileW
 */

BOOL FSDMGRStub_DeleteAndRenameFileW(PVOL pVol, PCWSTR pwsOldFileName, PCWSTR pwsNewFileName)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_GetFreeDiskSpaceW
 */

BOOL FSDMGRStub_GetDiskFreeSpaceW(PVOL pVol, PCWSTR pwsPathName, PDWORD pSectorsPerCluster, PDWORD pBytesPerSector, PDWORD pFreeClusters, PDWORD pClusters)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_Notify - FSD notification handler
 */

void FSDMGRStub_Notify(PVOL pVol, DWORD dwFlags)
{
}


/*  FSDMGRStub_RegisterFileSystemFunction
 */

BOOL FSDMGRStub_RegisterFileSystemFunction(PVOL pVol, SHELLFILECHANGEFUNC_t pfn)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_FindFirstFileW
 */

HANDLE FSDMGRStub_FindFirstFileW(PVOL pVol, HANDLE hProc, PCWSTR pwsFileSpec, PWIN32_FIND_DATAW pfd)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
}


/*  FSDMGRStub_FindNextFileW
 */

BOOL FSDMGRStub_FindNextFileW(PHDL pHdl, PWIN32_FIND_DATAW pfd)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_FindClose
 */

BOOL FSDMGRStub_FindClose(PHDL pHdl)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_CreateFileW
 */

HANDLE FSDMGRStub_CreateFileW(PVOL pVol, HANDLE hProc, PCWSTR pwsFileName, DWORD dwAccess, DWORD dwShareMode, PSECURITY_ATTRIBUTES pSecurityAttributes, DWORD dwCreate, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
}


/*  FSDMGRStub_ReadFile
 */

BOOL FSDMGRStub_ReadFile(PHDL pHdl, PVOID pBuffer, DWORD cbRead, PDWORD pcbRead, OVERLAPPED *pOverlapped)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_ReadFileWithSeek
 */

BOOL FSDMGRStub_ReadFileWithSeek(PHDL pHdl, PVOID pBuffer, DWORD cbRead, PDWORD pcbRead, OVERLAPPED *pOverlapped, DWORD dwLowOffset, DWORD dwHighOffset)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_WriteFile
 */

BOOL FSDMGRStub_WriteFile(PHDL pHdl, PVOID pBuffer, DWORD cbWrite, PDWORD pcbWritten, OVERLAPPED *pOverlapped)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_WriteFileWithSeek
 */

BOOL FSDMGRStub_WriteFileWithSeek(PHDL pHdl, PVOID pBuffer, DWORD cbWrite, PDWORD pcbWritten, OVERLAPPED *pOverlapped, DWORD dwLowOffset, DWORD dwHighOffset)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_SetFilePointer
 */

DWORD FSDMGRStub_SetFilePointer(PHDL pHdl, LONG lDistanceToMove, PLONG pDistanceToMoveHigh, DWORD dwMoveMethod)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0xFFFFFFFF;
}


/*  FSDMGRStub_GetFileSize
 */

DWORD FSDMGRStub_GetFileSize(PHDL pHdl, PDWORD pFileSizeHigh)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0xFFFFFFFF;
}


/*  FSDMGRStub_GetFileInformationByHandle
 */

BOOL FSDMGRStub_GetFileInformationByHandle(PHDL pHdl, PBY_HANDLE_FILE_INFORMATION pFileInfo)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_FlushFileBuffers
 */

BOOL FSDMGRStub_FlushFileBuffers(PHDL pHdl)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_GetFileTime
 */

BOOL FSDMGRStub_GetFileTime(PHDL pHdl, FILETIME *pCreation, FILETIME *pLastAccess, FILETIME *pLastWrite)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_SetFileTime
 */

BOOL FSDMGRStub_SetFileTime(PHDL pHdl, CONST FILETIME *pCreation, CONST FILETIME *pLastAccess, CONST FILETIME *pLastWrite)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_SetEndOfFile
 */

BOOL FSDMGRStub_SetEndOfFile(PHDL pHdl)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_DeviceIoControl
 */

BOOL FSDMGRStub_DeviceIoControl(PHDL pHdl, DWORD dwIoControlCode, PVOID pInBuf, DWORD nInBufSize, PVOID pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned, OVERLAPPED *pOverlapped)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*  FSDMGRStub_CloseFile
 */

BOOL FSDMGRStub_CloseFile(PHDL pHdl)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
