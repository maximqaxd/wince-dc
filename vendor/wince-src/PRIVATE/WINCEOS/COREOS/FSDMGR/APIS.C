/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    apis.c

Abstract:

    This file contains the FSDMGR entry points for all supported file
    system APIs.

--*/

#define KEEP_SYSCALLS           // for kernel.h only
#define NO_DEFINE_KCALL         // for kernel.h only

#include "kernel.h"
#include "fsdmgrp.h"


#define KERNELCALLSTACK(pcstk)  (((DWORD)(pcstk)->retAddr & 0xf0000000) == 0x80000000)


/*  FSDMGR_SystemCanDeadlock - Determine if caller is kernel
 *
 *  ENTRY
 *      pvol -> VOL structure
 *
 *  EXIT
 *      TRUE if caller is kernel, FALSE if not
 *
 *  NOTES
 *      We need this special power-down test before calling FSDMGR_Enter so
 *      that if/when the loader tries to open PCMCIA drivers before we have
 *      received the FSNOTIFY_DEVICES_ON notification, we will fail the
 *      call instead of blocking it.
 */










BOOL FSDMGR_SystemCanDeadlock(PVOL pVol)
{
    // If we're in a "shutdown" state, meaning all calls to FATEnter will
    // block until FSNOTIFY_DEVICES_ON is issued, then we need to make this
    // check in a few key places (ie, where the kernel may call us) in order
    // to prevent a deadlock.

//#if 0
    PTHREAD pth;
    PCALLSTACK pcstk;

    if (!(pVol->dwFlags & VOL_POWERDOWN))
        return FALSE;

    pth = ((PHDATA)((GetCurrentThreadId() & HANDLE_ADDRESS_MASK) + 0x80000000))->pvObj;
    if (pcstk = pth->pcstkTop) {
        if (KERNELCALLSTACK(pcstk) || (pcstk = pcstk->pcstkNext) && KERNELCALLSTACK(pcstk)) {
            SetLastError(ERROR_ACCESS_DENIED);
            return TRUE;
        }
    }
//#endif
    return FALSE;
}


/*  FSDMGR_Enter - Tracks (and potentially blocks) threads entering an FSD
 *
 *  ENTRY
 *      pVol -> VOL structure
 *
 *  EXIT
 *      None
 *
 *  NOTES
 *      It's not strictly necessary to keep track of threads entering
 *      an FSD on a per-volume basis, or to block them on such a basis --
 *      these functions could just as easily perform their task using globals.
 */

 





void FSDMGR_Enter(PVOL pVol)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    if (pVol->dwFlags & VOL_POWERDOWN)
        WaitForSingleObject(pVol->hevPowerUp, INFINITE);

    InterlockedIncrement(&pVol->cThreads);
}


/*  FSDMGR_Exit - Tracks threads leaving an FSD (and can signal when the last one leaves)
 *
 *  ENTRY
 *      pVol -> VOL structure
 *
 *  EXIT
 *      None
 *
 *  NOTES
 *      See FSDMGR_Enter for notes.
 */

void FSDMGR_Exit(PVOL pVol)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    if (InterlockedDecrement(&pVol->cThreads) == 0) {
        if (pVol->dwFlags & VOL_POWERDOWN)
            SetEvent(pVol->hevPowerDown);
    }
}


/*  FSDMGR_CloseVolume - Called when a volume is being deregistered
 *
 *  ENTRY
 *      pVol -> VOL structure
 *
 *  EXIT
 *      Always returns TRUE (that's what OUR stub always does, and it's
 *      expected that that's what FSDs will do too, if they even care to hook
 *      this entry point).
 */

BOOL FSDMGR_CloseVolume(PVOL pVol)
{
    return pVol->pDsk->pFSD->apfnAFS[AFSAPI_CLOSEVOLUME](pVol->dwVolData);
}


/*  FSDMGR_CreateDirectoryW - Create a new subdirectory
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsPathName - pointer to name of new subdirectory
 *      pSecurityAttributes - pointer to security attributes (ignored)
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_CreateDirectoryW(PVOL pVol, PCWSTR pwsPathName, PSECURITY_ATTRIBUTES pSecurityAttributes)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_CREATEDIRECTORYW](pVol->dwVolData, pwsPathName, pSecurityAttributes);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_RemoveDirectoryW - Destroy an existing subdirectory
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsPathName - pointer to name of existing subdirectory
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_RemoveDirectoryW(PVOL pVol, PCWSTR pwsPathName)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_REMOVEDIRECTORYW](pVol->dwVolData, pwsPathName);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_GetFileAttributesW - Get file/subdirectory attributes
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsFileName - pointer to name of existing file/subdirectory
 *
 *  EXIT
 *      Attributes of file/subdirectory if it exists, 0xFFFFFFFF if it
 *      does not (call GetLastError for error code).
 */

DWORD FSDMGR_GetFileAttributesW(PVOL pVol, PCWSTR pwsFileName)
{
    DWORD dw;

    FSDMGR_Enter(pVol);
    dw = pVol->pDsk->pFSD->apfnAFS[AFSAPI_GETFILEATTRIBUTESW](pVol->dwVolData, pwsFileName);
    FSDMGR_Exit(pVol);
    return dw;
}


/*  FSDMGR_SetFileAttributesW - Set file/subdirectory attributes
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsFileName - pointer to name of existing file/subdirectory
 *      dwAttributes - new attributes for file/subdirectory
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_SetFileAttributesW(PVOL pVol, PCWSTR pwsFileName, DWORD dwAttributes)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_SETFILEATTRIBUTESW](pVol->dwVolData, pwsFileName, dwAttributes);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_DeleteFileW - Delete file
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsFileName - pointer to name of existing file
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 *
 *  NOTES
 *      A file marked FILE_ATTRIBUTE_READONLY cannot be deleted.  You have to
 *      remove that attribute first, with SetFileAttributes.
 *
 *      An open file cannot be deleted.  All open handles must be closed first.
 *
 *      A subdirectory cannot be deleted with this call either.  You have to
 *      use RemoveDirectory instead.
 */

BOOL FSDMGR_DeleteFileW(PVOL pVol, PCWSTR pwsFileName)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_DELETEFILEW](pVol->dwVolData, pwsFileName);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_MoveFileW
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsOldFileName - pointer to name of existing file
 *      pwsNewFileName - pointer to new name for file
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 *
 *  NOTES
 *      We call FindFirst once to obtain the source directory stream for the
 *      for the existing file, and if it really exists, we call FindFirst
 *      again to obtain the destination directory stream for the new file,
 *      verifying that the new name does NOT exist.  Then we create the new
 *      name and destroy the old.
 *
 *      When moving a directory, we must make sure that our traversal
 *      of the destination path does not cross the source directory, otherwise
 *      we will end up creating a circular directory chain.
 */

BOOL FSDMGR_MoveFileW(PVOL pVol, PCWSTR pwsOldFileName, PCWSTR pwsNewFileName)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_MOVEFILEW](pVol->dwVolData, pwsOldFileName, pwsNewFileName);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_DeleteAndRenameFileW
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsOldFileName - pointer to name of existing file
 *      pwsNewFileName - pointer to name of file to be renamed to existing file
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_DeleteAndRenameFileW(PVOL pVol, PCWSTR pwsOldFileName, PCWSTR pwsNewFileName)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_PRESTOCHANGOFILENAME](pVol->dwVolData, pwsOldFileName, pwsNewFileName);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_GetFreeDiskSpaceW
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pwsPathName -> volume name (eg, "\Storage Card")
 *      pSectorsPerCluster -> DWORD to receive sectors/cluster
 *      pBytesPerSector -> DWORD to receive bytes/sector
 *      pFreeClusters -> DWORD to receive available clusters on volume
 *      pClusters -> DWORD to receive total clusters on volume
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_GetDiskFreeSpaceW(PVOL pVol, PCWSTR pwsPathName, PDWORD pSectorsPerCluster, PDWORD pBytesPerSector, PDWORD pFreeClusters, PDWORD pClusters)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_GETDISKFREESPACE](pVol->dwVolData, pwsPathName, pSectorsPerCluster, pBytesPerSector, pFreeClusters, pClusters);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_CloseAllFiles (no FSD equivalent)
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      hProc == handle of terminating process
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_CloseAllFiles(PVOL pVol, HANDLE hProc)
{
    PHDL pHdl;

    FSDMGR_Enter(pVol);
loop1:
    pHdl = pVol->dlHdlList.pHdlNext;
    while (pHdl != (PHDL)&pVol->dlHdlList) {
        if (pHdl->hProc == hProc) {
            SetHandleOwner(pHdl->h, GetCurrentProcess());
            if (CloseHandle(pHdl->h))
                goto loop1;     // handle was freed, state of our list has changed
        }
        pHdl = pHdl->dlHdl.pHdlNext;
    }
    FSDMGR_Exit(pVol);
    return TRUE;
}


/*  FSDMGR_CommitAllFiles (no FSD equivalent)
 *
 *  ENTRY
 *      pVol -> VOL structure
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_CommitAllFiles(PVOL pVol)
{
    PHDL pHdl;

    // There ain't no steenkin' FSDMGR_Enter/FSDMGR_Exit calls in this function
    // because it's called during FSNOTIFY_POWER_OFF and we don't want to hang ourselves.

    pHdl = pVol->dlHdlList.pHdlNext;
    while (pHdl != (PHDL)&pVol->dlHdlList) {
        pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_FLUSHFILEBUFFERS](pHdl->dwHdlData);
        pHdl = pHdl->dlHdl.pHdlNext;
    }
    return TRUE;
}


/*  FSDMGR_Notify - FSD notification handler
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      dwFlags == one or more FSNOTIFY_* flags
 *
 *  EXIT
 *      None.
 *
 *  NOTES
 *      This function is invoked once per registered volume every time the
 *      power goes off, comes back, or devices have finished initializing.  I'm
 *      only interested in hooking the FSNOTIFY_POWER_ON notification, so that I
 *      can flag all relevant disks as UNCERTAIN, but unfortunately, I cannot
 *      avoid being called for all events, not to mention multiple times per disk
 *      if there are multiple volumes per disk.
 */

void FSDMGR_Notify(PVOL pVol, DWORD dwFlags)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    if (dwFlags & FSNOTIFY_POWER_OFF) {

        DEBUGMSGW(ZONE_POWER,(DBGTEXTW("FSDMGR_Notify(FSNOTIFY_POWER_OFF)\n")));

        // hevPowerDown is signalled (set) as soon as the last thread exits
        // during VOL_POWERDOWN.  Even though it is auto-reset, we manually
        // reset it now, because the last thread on a previous power-down could
        // have decremented pVol->cThreads to zero and set hevPowerDown before
        // we made it to the test-and-wait code below, thus leaving hevPowerDown
        // in a signaled state.

        ResetEvent(pVol->hevPowerDown);

        // hevPowerUp stops threads in FSDMGR_Enter during VOL_POWERDOWN, and is
        // signalled (set) as soon as we receive FSNOTIFY_DEVICES_ON notification.

        ResetEvent(pVol->hevPowerUp);
        pVol->dwFlags |= VOL_POWERDOWN;

        // We know at this point that no more threads can get in.  There
        // may still be threads inside.  If not, pVol->cThreads is zero.
        // Otherwise, we will block, and the last thread's FSDMGR_Exit call will
        // wake us up.  We have to do this repeatedly, because a thread may
        // have erroneously thought it was the last one.

        while (pVol->cThreads)
            WaitForSingleObject(pVol->hevPowerDown, INFINITE);

        // Now make sure all files are committed;  this function calls the FSD's
        // FlushFileBuffers entry point directly, instead of calling our external
        // FSDMGR_FlushFileBuffers function, thereby insuring that we won't deadlock
        // ourselves, now that VOL_POWERDOWN has been set....

        FSDMGR_CommitAllFiles(pVol);

        DEBUGMSGW(ZONE_POWER,(DBGTEXTW("FSDMGR_Notify(FSNOTIFY_POWER_OFF): all threads blocked for %s\n"), pVol->wsVol));
    }
    else if (dwFlags & FSNOTIFY_POWER_ON) {

        DEBUGMSGW(ZONE_POWER && !(pVol->pDsk->dwFlags & DSK_UNCERTAIN),(DBGTEXTW("FSDMGR_Notify(FSNOTIFY_POWER_ON): marking %s uncertain\n"), pVol->pDsk->wsDsk));

        MarkDisk(pVol->pDsk, DSK_UNCERTAIN);
    }
    else if (dwFlags & FSNOTIFY_DEVICES_ON) {

        DEBUGMSG(ZONE_POWER,(DBGTEXT("FSDMGR_Notify(FSNOTIFY_DEVICES_ON): unblocking all threads for %s\n"), pVol->wsVol));

        pVol->dwFlags &= ~VOL_POWERDOWN;
        SetEvent(pVol->hevPowerUp);
    }

    // Now pass the notification on to the FSD as well; in general, they shouldn't
    // need to do very much on these events, because we've tried to take care of the
    // hard problems already, and these events need to be responsive.

    // However, one common requirement is flushing all non-handle-based data (eg,
    // dirty buffers) on the FSNOTIFY_POWER_OFF event.  We could ask the FSDs to
    // export a special "Flush" interface that we can call from here, but that seems
    // like a pointless abstraction, especially given that other interesting FSNOTIFY
    // events may be added in the future and I'd rather not have to rev FSDMGR just
    // for that.

    



    pVol->pDsk->pFSD->apfnAFS[AFSAPI_NOTIFY](pVol->dwVolData, dwFlags);
}


/*  FSDMGR_RegisterFileSystemFunction
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      pfn -> function address to call
 *
 *  EXIT
 *      TRUE if supported, FALSE if not
 */

BOOL FSDMGR_RegisterFileSystemFunction(PVOL pVol, SHELLFILECHANGEFUNC_t pfn)
{
    BOOL f;

    FSDMGR_Enter(pVol);
    f = pVol->pDsk->pFSD->apfnAFS[AFSAPI_REGISTERFILESYSTEMFUNCTION](pVol->dwVolData, pfn);
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_FindFirstFileW
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      hProc == handle of originating process
 *      pwsFileSpec -> name of file(s), wildcards allowed
 *      pfd -> WIN32_FIND_DATAW to be filled in on success
 *
 *  EXIT
 *      HANDLE to be used with subsequent FindNextFileW() and FindClose() APIs,
 *      or INVALID_HANDLE_VALUE if error.
 */

HANDLE FSDMGR_FindFirstFileW(PVOL pVol, HANDLE hProc, PCWSTR pwsFileSpec, PWIN32_FIND_DATAW pfd)
{
    HANDLE h;

    FSDMGR_Enter(pVol);
    h = (HANDLE)pVol->pDsk->pFSD->apfnAFS[AFSAPI_FINDFIRSTFILEW](pVol->dwVolData, hProc, pwsFileSpec, pfd);
    FSDMGR_Exit(pVol);
    return h;
}


/*  FSDMGR_FindNextFileW
 *
 *  ENTRY
 *      pHdl -> HDL structure
 *      pfd -> WIN32_FIND_DATAW to be filled in on success
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_FindNextFileW(PHDL pHdl, PWIN32_FIND_DATAW pfd)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFind[FINDAPI_FINDNEXTFILEW](pHdl->dwHdlData, pfd);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_FindClose
 *
 *  ENTRY
 *      pHdl -> HDL structure
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

BOOL FSDMGR_FindClose(PHDL pHdl)
{
    BOOL f = FALSE;
    PVOL pVol = pHdl->pVol;

    FSDMGR_Enter(pVol);
    if (pVol->pDsk->pFSD->apfnFind[FINDAPI_FINDCLOSE](pHdl->dwHdlData)) {
        DeallocFSDHandle(pHdl);
        f = TRUE;
    }
    FSDMGR_Exit(pVol);
    return f;
}


/*  FSDMGR_CreateFileW
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      hProc == handle of originating process
 *      pwsFileName
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (call GetLastError for error code)
 */

HANDLE FSDMGR_CreateFileW(PVOL pVol, HANDLE hProc, PCWSTR pwsFileName, DWORD dwAccess, DWORD dwShareMode, PSECURITY_ATTRIBUTES pSecurityAttributes, DWORD dwCreate, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE h;

    if (FSDMGR_SystemCanDeadlock(pVol))
        return INVALID_HANDLE_VALUE;

    FSDMGR_Enter(pVol);
    h = (HANDLE)pVol->pDsk->pFSD->apfnAFS[AFSAPI_CREATEFILEW](pVol->dwVolData, hProc, pwsFileName, dwAccess, dwShareMode, pSecurityAttributes, dwCreate, dwFlagsAndAttributes, hTemplateFile);
    FSDMGR_Exit(pVol);
    return h;
}


/*  FSDMGR_ReadFile
 */

BOOL FSDMGR_ReadFile(PHDL pHdl, PVOID pBuffer, DWORD cbRead, PDWORD pcbRead, OVERLAPPED *pOverlapped)
{
    BOOL f;

    if (FSDMGR_SystemCanDeadlock(pHdl->pVol))
        return FALSE;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_READFILE](pHdl->dwHdlData, pBuffer, cbRead, pcbRead, pOverlapped);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_ReadFileWithSeek
 */

BOOL FSDMGR_ReadFileWithSeek(PHDL pHdl, PVOID pBuffer, DWORD cbRead, PDWORD pcbRead, OVERLAPPED *pOverlapped, DWORD dwLowOffset, DWORD dwHighOffset)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_READFILEWITHSEEK](pHdl->dwHdlData, pBuffer, cbRead, pcbRead, pOverlapped, dwLowOffset, dwHighOffset);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_WriteFile
 */

BOOL FSDMGR_WriteFile(PHDL pHdl, PVOID pBuffer, DWORD cbWrite, PDWORD pcbWritten, OVERLAPPED *pOverlapped)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_WRITEFILE](pHdl->dwHdlData, pBuffer, cbWrite, pcbWritten, pOverlapped);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_WriteFileWithSeek
 */

BOOL FSDMGR_WriteFileWithSeek(PHDL pHdl, PVOID pBuffer, DWORD cbWrite, PDWORD pcbWritten, OVERLAPPED *pOverlapped, DWORD dwLowOffset, DWORD dwHighOffset)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_WRITEFILEWITHSEEK](pHdl->dwHdlData, pBuffer, cbWrite, pcbWritten, pOverlapped, dwLowOffset, dwHighOffset);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_SetFilePointer
 */

DWORD FSDMGR_SetFilePointer(PHDL pHdl, LONG lDistanceToMove, PLONG pDistanceToMoveHigh, DWORD dwMoveMethod)
{
    DWORD dw;

    FSDMGR_Enter(pHdl->pVol);
    dw = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_SETFILEPOINTER](pHdl->dwHdlData, lDistanceToMove, pDistanceToMoveHigh, dwMoveMethod);
    FSDMGR_Exit(pHdl->pVol);
    return dw;
}


/*  FSDMGR_GetFileSize
 */

DWORD FSDMGR_GetFileSize(PHDL pHdl, PDWORD pFileSizeHigh)
{
    DWORD dw;

    FSDMGR_Enter(pHdl->pVol);
    dw = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_GETFILESIZE](pHdl->dwHdlData, pFileSizeHigh);
    FSDMGR_Exit(pHdl->pVol);
    return dw;
}


/*  FSDMGR_GetFileInformationByHandle
 */

BOOL FSDMGR_GetFileInformationByHandle(PHDL pHdl, PBY_HANDLE_FILE_INFORMATION pFileInfo)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_GETFILEINFORMATIONBYHANDLE](pHdl->dwHdlData, pFileInfo);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_FlushFileBuffers
 */

BOOL FSDMGR_FlushFileBuffers(PHDL pHdl)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_FLUSHFILEBUFFERS](pHdl->dwHdlData);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_GetFileTime
 */

BOOL FSDMGR_GetFileTime(PHDL pHdl, FILETIME *pCreation, FILETIME *pLastAccess, FILETIME *pLastWrite)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_GETFILETIME](pHdl->dwHdlData, pCreation, pLastAccess, pLastWrite);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_SetFileTime
 */

BOOL FSDMGR_SetFileTime(PHDL pHdl, CONST FILETIME *pCreation, CONST FILETIME *pLastAccess, CONST FILETIME *pLastWrite)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_SETFILETIME](pHdl->dwHdlData, pCreation, pLastAccess, pLastWrite);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_SetEndOfFile
 */

BOOL FSDMGR_SetEndOfFile(PHDL pHdl)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_SETENDOFFILE](pHdl->dwHdlData);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_DeviceIoControl
 */

BOOL FSDMGR_DeviceIoControl(PHDL pHdl, DWORD dwIoControlCode, PVOID pInBuf, DWORD nInBufSize, PVOID pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned, OVERLAPPED *pOverlapped)
{
    BOOL f;

    FSDMGR_Enter(pHdl->pVol);
    f = pHdl->pVol->pDsk->pFSD->apfnFile[FILEAPI_DEVICEIOCONTROL](pHdl->dwHdlData, dwIoControlCode, pInBuf, nInBufSize, pOutBuf, nOutBufSize, pBytesReturned, pOverlapped);
    FSDMGR_Exit(pHdl->pVol);
    return f;
}


/*  FSDMGR_CloseFile
 */

BOOL FSDMGR_CloseFile(PHDL pHdl)
{
    BOOL f = FALSE;
    PVOL pVol = pHdl->pVol;

    FSDMGR_Enter(pVol);
    if (pVol->pDsk->pFSD->apfnFile[FILEAPI_CLOSEFILE](pHdl->dwHdlData)) {
        DeallocFSDHandle(pHdl);
        f = TRUE;
    }
    FSDMGR_Exit(pVol);
    return f;
}
