/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    init.c

Abstract:

    This file contains the FSDMGR initialization/deinitialization entry
    points that initiate the loading/unloading other installable file
    system drivers (ie, FSDs such as FATFS.DLL).

    DEVICE.EXE still loads v1 and v2 FSDs and communicates with them via
    their FSD_Init and FSD_Deinit exports, but it also supports new FSDs
    indirectly via this new "FSD Manager", named FSDMGR.DLL.

    DEVICE.EXE detects a new FSD by the existence of an FSD_MountDisk export.
    In that case, it loads FSDMGR.DLL (if it hasn't already), and notifies
    FSDMGR_InitEx of (1) the module handle of the new FSD, and (2) the name
    of the new device (eg, "DSK1").

    FSDMGR.DLL interacts with the new FSD (XXX.DLL) through the following
    interfaces, which XXX.DLL must export:

      XXX_MountDisk(PDSK pDsk)

        Mounts the specified disk, returning TRUE if success, FALSE if error.
        The FSD must be able to cope with this call being issued redundantly
        (ie, it must determine from the media itself, not the uniqueness of
        the given DSK pointer, whether or not this disk has already been mounted).
        This is due to the nature of removable media and the fact that media
        may or may not have been removed -- or may have changed slots -- while
        the system was powered off.

        All I/O to the disk must be performed via calls to FSDMGR_ReadDisk and
        FSDMGR_WriteDisk in FSDMGR.DLL.  Similarly, FSDMGR_RegisterVolume should
        be used to register each volume present on the disk with the file system.

        Note that this function must also be exported as "FSD_MountDisk".

      XXX_UnmountDisk(PDSK pDsk)

        Unmounts the specified disk.  Returns TRUE if XXX.DLL successfully
        unmounted the disk, FALSE if not.  A requirement of returning
        TRUE is that all volumes on the disk previously registered with
        FSDMGR_RegisterVolume must first be deregistered with FSDMGR_DeregisterVolume.

        Note that this function must also be exported as "FSD_UnmountDisk".

    These rest of these exports define the functionality of the FSD.  Any
    function not exported is replaced with a stub function in FSDMGR that returns
    failure.

      XXX_CloseVolume
      XXX_CreateDirectoryW
      XXX_RemoveDirectoryW
      XXX_GetFileAttributesW
      XXX_SetFileAttributesW
      XXX_DeleteFileW
      XXX_MoveFileW
      XXX_DeleteAndRenameFileW (formerly known as "PrestoChangoFileName")
      XXX_GetDiskFreeSpaceW (formerly known as "GetDiskFreeSpace")
      XXX_Notify
      XXX_RegisterFileSystemFunction
      XXX_CreateFileW
      XXX_CloseFile
      XXX_ReadFile
      XXX_WriteFile
      XXX_GetFileSize
      XXX_SetFilePointer
      XXX_GetFileInformationByHandle
      XXX_FlushFileBuffers
      XXX_GetFileTime
      XXX_SetFileTime
      XXX_SetEndOfFile
      XXX_DeviceIoControl
      XXX_ReadFileWithSeek
      XXX_WriteFileWithSeek
      XXX_FindFirstFileW
      XXX_FindNextFileW
      XXX_FindClose

    There are also some noteworthy omissions.  We neither look for nor install handlers
    for the following APIs because they were not supported after v1.0:

      XXX_CeRegisterFileSystemNotification

    Similarly, the following APIs were not supported after v2.0:

      XXX_CeOidGetInfo

    And last but not least, XXX_CloseAllFiles is not supported because FSDMGR.DLL can
    enumerate all known handles for a given process and call XXX_CloseFile, eliminating
    the need for a special process termination handler.

--*/

#include "fsdmgrp.h"


/*  Globals
 */

HINSTANCE   hDLL;               // module handle of this DLL
HANDLE      hAFSAPI;
HANDLE      hFileAPI;
HANDLE      hFindAPI;
FSD_DLINK   dlFSDList;          // global FSD list
CRITICAL_SECTION csFSD;         // global CS for this DLL



/*  FSDMGR_InitEx - Entry point for DEVICE.EXE to initialize new FSD
 *
 *  ENTRY
 *      hFSD == handle to new FSD
 *      pwsDsk -> disk name
 *      pReserved == NULL (reserved for future use)
 *
 *  EXIT
 *      Pointer to internal DSK structure if successful, NULL if error
 *
 *  NOTES
 *      When DEVICE.EXE initializes an old-style FSD directly, it keeps
 *      track of how many devices the FSD claims to be managing.
 *
 *      DEVICE.EXE maintains a similar count for devices passed to FSDMGR,
 *      so for each device, we allocate a DSK structure and make sure we
 *      report a NEW (as opposed to REMOUNTED) DSK only once.
 *
 *      This function assumes it is single-thread in virtue of the fact that
 *      DEVICE.EXE is single-threaded when it calls it.
 */

PDSK FSDMGR_InitEx(HANDLE hFSD, PCWSTR pwsDsk, LPVOID pReserved)
{
    PFSD pFSD;
    PDSK pDsk;

    DEBUGMSGW(ZONE_INIT, (DBGTEXTW("FSDMGR_InitEx: initializing FSD for %s\n"), pwsDsk));

	



//    LoadLibrary(L"FSDMGR.DLL");

    pFSD = AllocFSD(hFSD);
    pDsk = AllocDisk(pFSD, pwsDsk);

    if (pDsk)
        FSDMGR_GetDiskInfo(pDsk, &pDsk->fdi);

    if (pFSD && pDsk && pFSD->pfnMountDisk(pDsk)) {
        ASSERT(VALIDSIG(pFSD, FSD_SIG) && VALIDSIG(pDsk, DSK_SIG));
        UnmarkDisk(pDsk, DSK_UNCERTAIN);
    }
    else {
        if (pDsk) {
            DeallocDisk(pDsk);
            pDsk = NULL;
        }
        if (pFSD) {
            DeallocFSD(pFSD);
            pFSD = NULL;
        }
    }

    // Here's where we indicate to DEVICE.EXE whether or not the disk
    // is being remounted; make sure you don't dereference pDsk past this
    // point.

    if (pDsk && (pDsk->dwFlags & DSK_REMOUNTED))
        (DWORD)pDsk |= DSK_REMOUNTED;

    DEBUGMSG(ZONE_INIT, (DBGTEXT("FSDMGR_InitEx returned 0x%x\n"), pDsk));
    return pDsk;
}


/*  FSDMGR_DeinitEx - Entry point for DEVICE.EXE to deinitialize new FSD
 *
 *  ENTRY
 *      pDsk -> DSK structure, or NULL to deinit all disks that are no
 *      longer present (ie, whose UNCERTAIN bit is still set)
 *
 *  EXIT
 *      TRUE if the specified DSK structure has been freed, FALSE if not;
 *      or, if no DSK structure was specified, we return the number of DSKs
 *      we were able to free.
 *
 *  NOTES
 *      This function assumes it is single-thread in virtue of the fact that
 *      DEVICE.EXE is single-threaded when it calls it.
 */

int FSDMGR_DeinitEx(PDSK pDsk)
{
    PFSD pFSD;
    int cDisks = 0;

    DEBUGMSGW(ZONE_INIT, (DBGTEXTW("FSDMGR_DeinitEx(0x%x): deinit'ing FSD for %s\n"), pDsk, pDsk? ((PDSK)((DWORD)pDsk & ~0x1))->wsDsk : TEXTW("all disks")));

    // We intercept the FSNOTIFY_POWER_ON notification that all volumes
    // receive and mark their respective DSK structures as UNCERTAIN.
    //
    // Afterward, FSDMGR_DeinitEx will start getting called for every disk
    // in the system, which we will simply ignore for UNCERTAIN disks (EXCEPT
    // to close their handle, because that puppy's dead as far as DEVICE.EXE
    // is concerned).
    //
    // Then we will start getting FSDMGR_InitEx calls for disks still present;
    // those should turn into XXX_MountDisk calls and the corresponding
    // UNCERTAIN bits cleared.  Then, finally, when we get FSNOTIFY_DEVICES_ON,
    // we need to locate disks still marked UNCERTAIN and call XXX_UnmountDisk.

    if (pDsk == NULL) {

        // We get this call *after* FSNOTIFY_DEVICES_ON; ie, it's the last major
        // event we get after the system has been reinitialized.  So let's walk all
        // the disk structures and call XXX_UnmountDisk for any disk still marked
        // UNCERTAIN.  Of course, the FSD is free to decline (eg, if there's still
        // open files in volumes on the disk).

loop1:  pFSD = dlFSDList.pFSDNext;
        while (pFSD != (PFSD)&dlFSDList) {
            ASSERT(VALIDSIG(pFSD, FSD_SIG));

loop2:      pDsk = pFSD->dlDskList.pDskNext;
            while (pDsk != (PDSK)&pFSD->dlDskList) {
                ASSERT(VALIDSIG(pDsk, DSK_SIG));

                if (pDsk->dwFlags & DSK_UNCERTAIN) {
                    if (pFSD->pfnUnmountDisk(pDsk)) {
                        if (DeallocDisk(pDsk))
                            cDisks++;
                        goto loop2;
                    }
                }
                pDsk = pDsk->dlDsk.pDskNext;
            }

            // We call DeallocFSD just in case all the disks attached
            // to the FSD were freed;  if they weren't, nothing changes.

            if (DeallocFSD(pFSD))
                goto loop1;
            pFSD = pFSD->dlFSD.pFSDNext;
        }
        DEBUGONLY(pDsk = NULL); // just for the benefit of my DEBUGMSG below
    }
    else {
        (DWORD)pDsk &= ~DSK_REMOUNTED;
        ASSERT(VALIDSIG(pDsk, DSK_SIG));

        // At the point where DEVICE.EXE issues this notification (inside
        // DeregisterDevice), our disk handle has already been close.  Note
        // that even if it hadn't been, there's no guarantee the driver would
        // be in any condition to respond, since we could be coming back up
        // from a powered-down state *or* the device may have been removed.
        // So it's perfectly reasonable to unconditionally mark the disk closed.

        MarkDisk(pDsk, DSK_CLOSED);

        if (!(pDsk->dwFlags & DSK_UNCERTAIN)) {

            pFSD = pDsk->pFSD;
            ASSERT(VALIDSIG(pFSD, FSD_SIG));

            if (pFSD->pfnUnmountDisk(pDsk)) {

                // Note that, despite XXX_UnmountDisk's apparent success,
                // DeallocDisk still won't deallocate the disk if it still has
                // volumes attached.

                if (DeallocDisk(pDsk))
                    cDisks++;

                // Similarly, DeallocFSD won't actually deallocate the FSD if it
                // still has disks attached.

                DeallocFSD(pFSD);
            }
        }
    }

    DEBUGMSG(ZONE_INIT, (DBGTEXT("FSDMGR_DeinitEx(0x%x) returned %d\n"), pDsk, cDisks));
    return cDisks;
}


/*  FSDMGR_Attach - DLL_PROCESS_ATTACH handler
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      TRUE if successful, FALSE if not.  Most failures can probably
 *      be attributed to insufficient memory.
 *
 *  NOTES
 *      This is assumed to be protected by a critical section.  Since it's
 *      currently called only by FSDMGR_Main, we are protected by the loader's
 *      critical section.
 */

BOOL FSDMGR_Attach()
{
    DEBUGREGISTER(hDLL);

    InitList((PDLINK)&dlFSDList);
    InitializeCriticalSection(&csFSD);

    hAFSAPI = CreateAPISet("PFSD", ARRAY_SIZE(apfnAFSAPIs), apfnAFSAPIs, asigAFSAPIs);
    hFileAPI = CreateAPISet("HFSD", ARRAY_SIZE(apfnFileAPIs), apfnFileAPIs, asigFileAPIs);
    RegisterAPISet(hFileAPI, HT_FILE | REGISTER_APISET_TYPE);
    hFindAPI = CreateAPISet("FFSD", ARRAY_SIZE(apfnFindAPIs), apfnFindAPIs, asigFindAPIs);
    RegisterAPISet(hFindAPI, HT_FIND | REGISTER_APISET_TYPE);

    return hAFSAPI && hFileAPI && hFindAPI;
}


/*  FSDMGR_Detach - DLL_PROCESS_DETACH handler
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      TRUE if successful, FALSE if not (currently, always successful)
 *
 *  NOTES
 *      This is assumed to be protected by a critical section.  Since it's
 *      currently called only by FSDMGR_Main, we are protected by the loader's
 *      critical section.
 */

BOOL FSDMGR_Detach(void)
{
    CloseHandle(hFindAPI);
    CloseHandle(hFileAPI);
    CloseHandle(hAFSAPI);
    DeleteCriticalSection(&csFSD);
    return TRUE;
}


/*  FSDMGR_Main - FSDMGR.DLL initialization entry point
 *
 *  ENTRY
 *      DllInstance == DLL module handle
 *      Reason == DLL_* initialization message
 *      pReserved == NULL (reserved for future use)
 *
 *  EXIT
 *      TRUE if successful, FALSE if not.  Most failures can probably
 *      be attributed to insufficient memory.
 */

BOOL WINAPI FSDMGR_Main(HINSTANCE DllInstance, INT Reason, LPVOID pReserved)
{
    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        DEBUGMSG(ZONE_INIT, (DBGTEXT("FSDMGR_Main: DLL_PROCESS_ATTACH\n")));
        hDLL = DllInstance;
        return FSDMGR_Attach();

    case DLL_PROCESS_DETACH:
        DEBUGMSG(ZONE_INIT, (DBGTEXT("FSDMGR_Main: DLL_PROCESS_DETACH\n")));
        return FSDMGR_Detach();

    default:
      //DEBUGMSG(ZONE_INIT, (DBGTEXT("FSDMGR_Main: Reason #%d ignored\n"), Reason));
        break;
    }
    return TRUE;
}
