/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    alloc.c

Abstract:

    This file handles the creation/destruction of all FSDMGR data
    structures (ie, FSDs, DSKs, VOLs and HDLs).

--*/

#include "fsdmgrp.h"


/*  AllocFSD - Allocate FSD structure and obtain FSD entry points
 *
 *  ENTRY
 *      hFSD == handle to new FSD
 *
 *  EXIT
 *      Pointer to internal FSD structure, or NULL if out of memory or
 *      FSD entry points could not be obtained (use GetLastError if you really
 *      want to know the answer).
 */

PFSD AllocFSD(HANDLE hFSD)
{
    PFSD pFSD;
	DWORD cb;
	WCHAR baseName[MAX_FSD_NAME_SIZE];

    pFSD = dlFSDList.pFSDNext;
    while (pFSD != (PFSD)&dlFSDList) {
        if (CompareFSDs(pFSD->hFSD, hFSD))
            break;
        pFSD = pFSD->dlFSD.pFSDNext;
    }

    if (pFSD != (PFSD)&dlFSDList) {

        // This FSD has already been loaded once, which means we no
        // longer need the current handle reference.

        FreeLibrary(hFSD);
    }
    else {

        // This is a new FSD, so allocate a structure for it and save the handle.

		if (GetProcAddress(hFSD, L"FSD_CreateFileW"))
		{
			/* this guy uses simply FSD_ as a function prefix */
			wcscpy(baseName, L"FSD_");
		}
		else
		{
        DWORD cch, i;
        WCHAR wsTmp[MAX_PATH];

			/* derive the function prefix from the module's name. This is
			 * a tricky step, since the letter case of the module name must
			 * match the function prefix within the DLL.
			 */

            cch = GetModuleFileName(hFSD, wsTmp, ARRAY_SIZE(wsTmp));
            if (cch)
            {
                PWSTR pwsTmp = &wsTmp[cch];
                while (pwsTmp != wsTmp)
                {
                    pwsTmp--;
                    if (*pwsTmp == '\\' || *pwsTmp == '/')
                    {
                		pwsTmp++;
                		break;
            		}
        		}

        		i = 0;
        		while (*pwsTmp && *pwsTmp != '.' && i < ARRAY_SIZE(baseName)-2)
            		baseName[i++] = *pwsTmp++;

        	    baseName[i]   = L'_';
			    baseName[i+1] = 0;
			}
			else
			{
				return NULL;
			}
		}

        cb = (wcslen(baseName) + 1) * sizeof(baseName[0]);

        pFSD = (PFSD)LocalAlloc(LPTR, sizeof(FSD) - (sizeof(pFSD->wsFSD) - cb));

        if (pFSD) {
            INITSIG(pFSD, FSD_SIG);
            InitList((PDLINK)&pFSD->dlDskList);
            AddListItem((PDLINK)&dlFSDList, (PDLINK)&pFSD->dlFSD);
            pFSD->hFSD = hFSD;
			memcpy(pFSD->wsFSD, baseName, cb);
        }
        else
            DEBUGMSG(ZONE_INIT || ZONE_ERRORS,(DBGTEXT("FSDMGR!AllocFSD: out of memory!\n")));
    }

    if (pFSD) {
        if (!pFSD->pfnMountDisk || !pFSD->pfnUnmountDisk) {
            pFSD->pfnMountDisk = (PFNMOUNTDISK)GetFSDProcAddress(pFSD, TEXT("MountDisk"));
            pFSD->pfnUnmountDisk = (PFNMOUNTDISK)GetFSDProcAddress(pFSD, TEXT("UnmountDisk"));
        }
        if (!pFSD->pfnMountDisk || !pFSD->pfnUnmountDisk) {
            DeallocFSD(pFSD);
            pFSD = NULL;
        }
    }

    return pFSD;
}


/*  DeallocFSD - Deallocate FSD structure
 *
 *  ENTRY
 *      pFSD -> FSD
 *
 *  EXIT
 *      TRUE is FSD was successfully deallocated, FALSE if not (eg, if the FSD
 *      structure still has some DSK structures attached to it).
 */

BOOL DeallocFSD(PFSD pFSD)
{
    ASSERT(VALIDSIG(pFSD, FSD_SIG));

    if (IsListEmpty((PDLINK)&pFSD->dlDskList)) {
        FreeLibrary(pFSD->hFSD);
        RemoveListItem((PDLINK)&pFSD->dlFSD);
        LocalFree((HLOCAL)pFSD);
        return TRUE;
    }
    return FALSE;
}


/*  AllocDisk - Allocate DSK structure for an FSD
 *
 *  ENTRY
 *      pFSD -> FSD structure
 *      pwsDsk -> disk name
 *      dwFlags == intial disk flags
 *
 *  EXIT
 *      Pointer to internal DSK structure, or NULL if out of memory
 */

PDSK AllocDisk(PFSD pFSD, PCWSTR pwsDsk)
{
    PDSK pDsk;
    DWORD dwFlags = DSK_NONE;
    HANDLE hDsk = INVALID_HANDLE_VALUE;

    if (pFSD == NULL)
        return NULL;

    pDsk = pFSD->dlDskList.pDskNext;
    while (pDsk != (PDSK)&pFSD->dlDskList) {
        if (wcscmp(pDsk->wsDsk, pwsDsk) == 0) {
            hDsk = pDsk->hDsk;
            dwFlags = (pDsk->dwFlags | DSK_REMOUNTED) & ~(DSK_CLOSED);
            break;
        }
        pDsk = pDsk->dlDsk.pDskNext;
    }

    if (hDsk == INVALID_HANDLE_VALUE) {
        hDsk = CreateFileW(pwsDsk,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL, OPEN_EXISTING, 0, NULL);

        if (hDsk != INVALID_HANDLE_VALUE)
            dwFlags &= ~DSK_READONLY;
        else if (GetLastError() == ERROR_ACCESS_DENIED) {
            hDsk = CreateFileW(pwsDsk,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
            if (hDsk != INVALID_HANDLE_VALUE)
                dwFlags |= DSK_READONLY;
        }
    }

    if (hDsk == INVALID_HANDLE_VALUE) {
        DEBUGMSGW(ZONE_INIT || ZONE_ERRORS, (DBGTEXTW("FSDMGR!AllocDisk: CreateFile(%s) failed (%d)\n"), pwsDsk, GetLastError()));
        return NULL;
    }

    if (pDsk == (PDSK)&pFSD->dlDskList) {

        // We take a look at the actual length of the disk name and
        // reduce the size of the FSD structure by "sizeof(wsDsk)-cb".

        DWORD cb = (wcslen(pwsDsk) + 1) * sizeof(pwsDsk[0]);

        pDsk = (PDSK)LocalAlloc(LPTR, sizeof(DSK) - (sizeof(pDsk->wsDsk)-cb));

        if (pDsk) {
            INITSIG(pDsk, DSK_SIG);
            InitList((PDLINK)&pDsk->dlVolList);
            AddListItem((PDLINK)&pFSD->dlDskList, (PDLINK)&pDsk->dlDsk);
            pDsk->pFSD = pFSD;
            memcpy(pDsk->wsDsk, pwsDsk, cb);
        }
        else {
            DEBUGMSG(ZONE_INIT || ZONE_ERRORS,(DBGTEXT("FSDMGR!AllocDisk: out of memory!\n")));
            CloseHandle(hDsk);
        }
    }

    if (pDsk) {
        pDsk->hDsk = hDsk;
        pDsk->dwFlags = dwFlags;
    }

    return pDsk;
}


/*  MarkDisk - Mark a DSK structure with one or more flags
 *
 *  ENTRY
 *      pDsk -> DSK structure
 *      dwFlags == one or flags to mark (see DSK_*)
 *
 *  EXIT
 *      None
 */

void MarkDisk(PDSK pDsk, DWORD dwFlags)
{
    ASSERT(VALIDSIG(pDsk, DSK_SIG));

    pDsk->dwFlags |= dwFlags;
    if (pDsk->dwFlags & DSK_CLOSED) {
        CloseHandle(pDsk->hDsk);
        pDsk->hDsk = INVALID_HANDLE_VALUE;
    }
}


/*  UnmarkDisk - Unmark a DSK structure with one or more flags
 *
 *  ENTRY
 *      pDsk -> DSK structure
 *      dwFlags == one or flags to unmark (see DSK_*)
 *
 *  EXIT
 *      None
 */

void UnmarkDisk(PDSK pDsk, DWORD dwFlags)
{
    ASSERT(VALIDSIG(pDsk, DSK_SIG));

    pDsk->dwFlags &= ~dwFlags;
}


/*  DeallocDisk - Deallocate DSK structure
 *
 *  ENTRY
 *      pDsk -> DSK structure
 *
 *  EXIT
 *      TRUE is DSK was successfully deallocated, FALSE if not (eg, if the DSK
 *      structure still has some VOL structures attached to it).
 */

BOOL DeallocDisk(PDSK pDsk)
{
    ASSERT(VALIDSIG(pDsk, DSK_SIG));

    if (IsListEmpty((PDLINK)&pDsk->dlVolList)) {
        if (pDsk->hDsk != INVALID_HANDLE_VALUE)
            CloseHandle(pDsk->hDsk);
        RemoveListItem((PDLINK)&pDsk->dlDsk);
        LocalFree((HLOCAL)pDsk);
        return TRUE;
    }
    return FALSE;
}


/*  AllocVolume - Allocate VOL structure for a DSK
 *
 *  ENTRY
 *      pDSK -> DSK structure
 *      dwVolData == FSD-defined volume data
 *
 *  EXIT
 *      Pointer to internal VOL structure, or NULL if out of memory
 */

PVOL AllocVolume(PDSK pDsk, DWORD dwVolData)
{
    PVOL pVol;

    ASSERT(OWNCRITICALSECTION(&csFSD));

    if (pDsk == NULL)
        return NULL;

    pVol = pDsk->dlVolList.pVolNext;
    while (pVol != (PVOL)&pDsk->dlVolList) {
        if (pVol->dwVolData == dwVolData)
            break;
        pVol = pVol->dlVol.pVolNext;
    }

    if (pVol == (PVOL)&pDsk->dlVolList) {

        // This is a new VOL...

        pVol = (PVOL)LocalAlloc(LPTR, sizeof(VOL));

        if (pVol) {
            INITSIG(pVol, VOL_SIG);
            InitList((PDLINK)&pVol->dlHdlList);
            AddListItem((PDLINK)&pDsk->dlVolList, (PDLINK)&pVol->dlVol);
            pVol->pDsk = pDsk;
            pVol->dwVolData = dwVolData;
            pVol->iAFS = INVALID_AFS;
            
            pVol->hevPowerUp  = CreateEvent(NULL, TRUE, TRUE, NULL);
            pVol->hevPowerDown = CreateEvent(NULL, FALSE, FALSE, NULL);
        }
        else
            DEBUGMSG(ZONE_INIT || ZONE_ERRORS,(DBGTEXT("FSDMGR!AllocVolume: out of memory!\n")));
    }

    return pVol;
}


/*  DeallocVolume - Deallocate VOL structure
 *
 *  ENTRY
 *      pVol -> VOL structure
 *
 *  EXIT
 *      TRUE is VOL was successfully deallocated, FALSE if not (eg, if the VOL
 *      structure still has some HDL structures attached to it).
 *
 *      Currently, we never fail this function.  If handles are still attached to
 *      the volume, we forcibly close them;  that decision may need to be revisited....
 */

BOOL DeallocVolume(PVOL pVol)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG) && OWNCRITICALSECTION(&csFSD));

    if (pVol->iAFS != INVALID_AFS)
        return FALSE;

    while (TRUE) {
        PHDL pHdl = pVol->dlHdlList.pHdlNext;
        if (pHdl == (PHDL)&pVol->dlHdlList)
            break;
        CloseHandle(pHdl->h);
    }

    if (pVol->hevPowerUp)
        CloseHandle(pVol->hevPowerUp);

    if (pVol->hevPowerDown)
        CloseHandle(pVol->hevPowerDown);

    RemoveListItem((PDLINK)&pVol->dlVol);
    LocalFree((HLOCAL)pVol);

    return TRUE;
}


/*  AllocFSDHandle - Allocate HDL structure for a VOL
 *
 *  ENTRY
 *      pVol -> VOL structure
 *      hProc == originating process handle
 *      dwHldData == value from FSD's call to FSDMGR_CreateXXXXHandle
 *      dwFlags == initial flags for HDL (eg, HDL_FILE or HDL_FIND)
 *
 *  EXIT
 *      An application handle, or INVALID_HANDLE_VALUE if error
 */

HANDLE AllocFSDHandle(PVOL pVol, HANDLE hProc, DWORD dwHdlData, DWORD dwFlags)
{
    PHDL pHdl;
    HANDLE h = INVALID_HANDLE_VALUE;

    if (pVol == NULL)
        return NULL;

    pHdl = (PHDL)LocalAlloc(LPTR, sizeof(HDL));

    if (pHdl) {
        EnterCriticalSection(&csFSD);

        INITSIG(pHdl, (dwFlags & HDL_FILE)? HFILE_SIG : HSEARCH_SIG);
        AddListItem((PDLINK)&pVol->dlHdlList, (PDLINK)&pHdl->dlHdl);
        pHdl->pVol = pVol;
        pHdl->dwFlags = dwFlags;
        pHdl->dwHdlData = dwHdlData;
        pHdl->h = CreateAPIHandle((dwFlags & HDL_FILE)? hFileAPI : hFindAPI, pHdl);
        if (pHdl->h) {
            h = pHdl->h;
            if (hProc == NULL) {
                if (hProc = GetCurrentProcess()) {
                    BOOL f = SetHandleOwner(h, hProc);
                    DEBUGMSG(ZONE_INIT,(DBGTEXT("FSDMGR!AllocFSDHandle: hProc switched to 0x%08x (%d)\n"), hProc, f));
                }
            }
            pHdl->hProc = hProc;
        }
        else
            DeallocFSDHandle(pHdl);

        LeaveCriticalSection(&csFSD);
    }
    else
        DEBUGMSG(ZONE_INIT || ZONE_ERRORS,(DBGTEXT("FSDMGR!AllocFSDHandle: out of memory!\n")));

    return h;
}


/*  DeallocFSDHandle - Deallocate HDL structure
 *
 *  ENTRY
 *      pHdl -> HDL structure
 *
 *  EXIT
 *      None
 */

void DeallocFSDHandle(PHDL pHdl)
{
    EnterCriticalSection(&csFSD);

    ASSERT(VALIDSIG(pHdl, (pHdl->dwFlags & HDL_FILE)? HFILE_SIG : HSEARCH_SIG));

    RemoveListItem((PDLINK)&pHdl->dlHdl);
    LeaveCriticalSection(&csFSD);

    LocalFree((HLOCAL)pHdl);
}
