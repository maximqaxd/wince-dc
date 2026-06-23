/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    serv.c

Abstract:

    This file contains the FSDMGR services provided to other FSDs.

--*/

#include "fsdmgrp.h"


/*  FSDMGR_GetDiskInfo - Called by an FSD to query disk info
 *
 *  ENTRY
 *      pDsk -> DSK passed to FSD_MountDisk
 *      pfdi -> FSD_DISK_INFO structure to be filled in
 *
 *  EXIT
 *      ERROR_SUCCESS if structure successfully filled in, or an error code.
 */

DWORD FSDMGR_GetDiskInfo(PDSK pDsk, FSD_DISK_INFO *pfdi)
{
    DWORD cb;

    ASSERT(
        VALIDSIG(pDsk, DSK_SIG) &&
        offsetof(FSD_DISK_INFO, cSectors)           == offsetof(DISK_INFO, di_total_sectors) &&
        offsetof(FSD_DISK_INFO, cbSector)           == offsetof(DISK_INFO, di_bytes_per_sect) &&
        offsetof(FSD_DISK_INFO, cCylinders)         == offsetof(DISK_INFO, di_cylinders) &&
        offsetof(FSD_DISK_INFO, cHeadsPerCylinder)  == offsetof(DISK_INFO, di_heads) &&
        offsetof(FSD_DISK_INFO, cSectorsPerTrack)   == offsetof(DISK_INFO, di_sectors) &&
        offsetof(FSD_DISK_INFO, dwFlags)            == offsetof(DISK_INFO, di_flags)
    );

    // Return cached information whenever possible

    if (pfdi != &pDsk->fdi && pDsk->fdi.cbSector != 0) {
        *pfdi = pDsk->fdi;
        return ERROR_SUCCESS;
    }

    if (DeviceIoControl(pDsk->hDsk,
                        IOCTL_DISK_GETINFO,
                        NULL, 0,
                        pfdi, sizeof(*pfdi),
                        &cb, NULL)) {
        pDsk->dwFlags |= DSK_NEW_IOCTLS;
    }
    else
    if (DeviceIoControl(pDsk->hDsk,
                        DISK_IOCTL_GETINFO,
                        pfdi, sizeof(*pfdi),
                        NULL, 0,
                        &cb, NULL)) {
        ASSERT(!(pDsk->dwFlags & DSK_NEW_IOCTLS));
    }
    else {
        DWORD status = GetLastError();
        DEBUGMSG(ZONE_ERRORS, (DBGTEXT("FSDMGR_GetDiskInfo: DISK_IOCTL_GETINFO failed (%d)\n"), status));
        pDsk->dwFlags |= DSK_NO_IOCTLS;
        return status;
    }

    DEBUGMSG(ZONE_DISKIO, (DBGTEXT("FSDMGR_GetDiskInfo: DISK_IOCTL_GETINFO returned %d bytes\n"), cb));

    DEBUGMSG(ZONE_DISKIO, (DBGTEXT("FSDMGR_GetDiskInfo: %d sectors, BPS=%d, CHS=%d:%d:%d\n"),
              pfdi->cSectors, pfdi->cSectors, pfdi->cbSector, pfdi->cCylinders, pfdi->cHeadsPerCylinder, pfdi->cSectorsPerTrack));

    DEBUGMSG(pfdi->dwFlags & ~(FDI_MBR | FDI_CHS_UNCERTAIN | FDI_UNFORMATTED | FDI_PAGEABLE), (DBGTEXT("FSDMGR_GetDiskInfo: DISK_IOCTL_GETINFO returned unknown flags (0x%08x)\n"), pfdi->dwFlags));

    pfdi->dwFlags &= (FDI_MBR | FDI_CHS_UNCERTAIN | FDI_UNFORMATTED | FDI_PAGEABLE);

    if (pDsk->dwFlags & DSK_READONLY)
        pfdi->dwFlags |= FDI_READONLY;

    return ERROR_SUCCESS;
}


/* process an I/O operation using ReadFile() or WriteFile() */
static DWORD PlainReadWrite(FSD_SCATTER_GATHER_INFO *sgi,
					 		FSD_SCATTER_GATHER_RESULTS *sgr,
							DWORD ioctl)
{
DWORD numBytes;
DWORD i;
PDSK  pDsk;
BOOL  ok;

	pDsk = (PDSK)sgi->idDsk;

	if (SetFilePointer(pDsk->hDsk, sgi->dwSector * pDsk->fdi.cbSector, NULL, FILE_BEGIN) == 0xFFFFFFFF)
       	return GetLastError();

	for (i = 0; i < sgi->cfbi; i++)
	{
        if (ioctl == DISK_IOCTL_READ)
        	ok = ReadFile(pDsk->hDsk, sgi->pfbi[i].pBuffer, sgi->pfbi[i].cbBuffer, &numBytes, NULL);
		else
        	ok = WriteFile(pDsk->hDsk, sgi->pfbi[i].pBuffer, sgi->pfbi[i].cbBuffer, &numBytes, NULL);

		if (!ok)
			return GetLastError();

        if (numBytes != sgi->pfbi[i].cbBuffer)
			return ERROR_GEN_FAILURE;
	}

	return ERROR_SUCCESS;
}


/*  ReadWriteDiskEx - Read/write sectors from/to a disk to/from discontiguous buffers
 *
 *  ENTRY
 *      pfsgi -> FSD_SCATTER_GATHER_INFO
 *      pfsgr -> FSD_SCATTER_GATHER_RESULTS
 *      ioctl -> I/O op to perform
 *
 *  EXIT
 *      ERROR_SUCCESS if successful, or an error code
 */

static DWORD ReadWriteDiskEx(FSD_SCATTER_GATHER_INFO *sgi,
							 FSD_SCATTER_GATHER_RESULTS *sgr,
							 DWORD ioctl)
{
SG_REQ *sgreq;
PDSK    pDsk;
DWORD   result;
DWORD   i;
DWORD   dummy;

	pDsk = (PDSK)sgi->idDsk;
    ASSERT(VALIDSIG(pDsk, DSK_SIG) && pDsk->hDsk);

    // make sure the caller is not passing inappropriate flags
    if (sgi->dwFlags)
        return ERROR_INVALID_PARAMETER;

    //DEBUGMSGW(ZONE_DISKIO,(DBGTEXTW("ReadWriteDiskEd: %s sector %d (0x%08x), total %d)\n"), cmd == DISK_IOCTL_READ? TEXTW("READ") : TEXTW("WRITE"), dwSector, dwSector, cSectors));

    if (pDsk->hDsk == INVALID_HANDLE_VALUE)
    {
        DEBUGMSGW(ZONE_ERRORS,(DBGTEXTW("ReadWriteDiskEx: %s not open, returning error %d\n"), pDsk->wsDsk, ERROR_BAD_DEVICE));
        return ERROR_BAD_DEVICE;
    }

	if (pDsk->dwFlags & DSK_NO_IOCTLS)
	{
		result = PlainReadWrite(sgi, sgr, ioctl);
	}
    else
	{
        if (pDsk->dwFlags & DSK_NEW_IOCTLS)
		{
        	if (ioctl == DISK_IOCTL_READ)
            	ioctl = IOCTL_DISK_READ;
            else if (ioctl == DISK_IOCTL_WRITE)
                ioctl = IOCTL_DISK_WRITE;
        }

		sgreq              = alloca(sizeof(SG_REQ) + sgi->cfbi * sizeof(SG_BUF) - sizeof(SG_BUF));
		sgreq->sr_start    = sgi->dwSector;
		sgreq->sr_num_sec  = sgi->cSectors;
		sgreq->sr_num_sg   = sgi->cfbi;
		sgreq->sr_status   = 0;
		sgreq->sr_callback = NULL;

		for (i = 0; i < sgi->cfbi; i++)
		{
	    	sgreq->sr_sglist[i].sb_buf = sgi->pfbi[i].pBuffer;
			sgreq->sr_sglist[i].sb_len = sgi->pfbi[i].cbBuffer;
		}

    	if (!DeviceIoControl(pDsk->hDsk, ioctl, sgreq, sizeof(sgreq) + sgi->cfbi * sizeof(SG_BUF) - sizeof(SG_BUF), NULL, 0, &dummy, NULL))
			result = GetLastError();
		else
			result = ERROR_SUCCESS;
    }

	if (sgr)
	{
	    sgr->dwFlags = 0;

		if (result == ERROR_SUCCESS)
			sgr->cSectorsTransferred = sgi->cSectors;
		else
		    sgr->cSectorsTransferred = 0;
	}

	return result;
}


/*  FSDMGR_ReadDisk - Called by an FSD to read sector(s) from a disk
 *
 *  ENTRY
 *      pDsk -> DSK passed to FSD_MountDisk
 *      dwsector == starting sector #
 *      cSectors == number of sectors to read
 *      pBuffer -> buffer to read sectors into
 *      cbBuffer == size of buffer, in bytes
 *
 *  EXIT
 *      ERROR_SUCCESS if successful, or an error code
 */

DWORD FSDMGR_ReadDisk(PDSK pDsk, DWORD dwSector, DWORD cSectors, PBYTE pBuffer, DWORD cbBuffer)
{
FSD_BUFFER_INFO         bi;
FSD_SCATTER_GATHER_INFO sgi;

	bi.pBuffer      = pBuffer;
	bi.cbBuffer     = cbBuffer;

	sgi.dwFlags     = 0;
	sgi.idDsk       = (DWORD)pDsk;
	sgi.dwSector    = dwSector;
	sgi.cSectors    = cSectors;
	sgi.pfdi        = NULL;
	sgi.cfbi        = 1;
	sgi.pfbi        = &bi;
	sgi.pfnCallBack = NULL;

    return ReadWriteDiskEx(&sgi, NULL, DISK_IOCTL_READ);
}


/*  FSDMGR_WriteDisk - Called by an FSD to write sector(s) to a disk
 *
 *  ENTRY
 *      pDsk -> DSK passed to FSD_MountDisk
 *      dwsector == starting sector #
 *      cSectors == number of sectors to write
 *      pBuffer -> buffer to write sectors from
 *      cbBuffer == size of buffer, in bytes
 *
 *  EXIT
 *      ERROR_SUCCESS if successful, or an error code
 */

DWORD FSDMGR_WriteDisk(PDSK pDsk, DWORD dwSector, DWORD cSectors, PBYTE pBuffer, DWORD cbBuffer)
{
FSD_BUFFER_INFO         bi;
FSD_SCATTER_GATHER_INFO sgi;

	bi.pBuffer      = pBuffer;
	bi.cbBuffer     = cbBuffer;

	sgi.dwFlags     = 0;
	sgi.idDsk       = (DWORD)pDsk;
	sgi.dwSector    = dwSector;
	sgi.cSectors    = cSectors;
	sgi.pfdi        = NULL;
	sgi.cfbi        = 1;
	sgi.pfbi        = &bi;
	sgi.pfnCallBack = NULL;

    return ReadWriteDiskEx(&sgi, NULL, DISK_IOCTL_WRITE);
}


/*  FSDMGR_ReadDiskEx - Called by an FSD to read sectors from a disk into discontiguous buffers
 *
 *  ENTRY
 *      pfsgi -> FSD_SCATTER_GATHER_INFO
 *      pfsgr -> FSD_SCATTER_GATHER_RESULTS
 *
 *  EXIT
 *      ERROR_SUCCESS if successful, or an error code
 */

DWORD FSDMGR_ReadDiskEx(FSD_SCATTER_GATHER_INFO *pfsgi, FSD_SCATTER_GATHER_RESULTS *pfsgr)
{
    return ReadWriteDiskEx(pfsgi, pfsgr, DISK_IOCTL_READ);
}


/*  FSDMGR_WriteDiskEx - Called by an FSD to write sectors to a disk from discontiguous buffers
 *
 *  ENTRY
 *      pfsgi -> FSD_SCATTER_GATHER_INFO
 *      pfsgr -> FSD_SCATTER_GATHER_RESULTS
 *
 *  EXIT
 *      ERROR_SUCCESS if successful, or an error code
 */

DWORD FSDMGR_WriteDiskEx(FSD_SCATTER_GATHER_INFO *pfsgi, FSD_SCATTER_GATHER_RESULTS *pfsgr)
{
    return ReadWriteDiskEx(pfsgi, pfsgr, DISK_IOCTL_WRITE);
}


/*  FSDMGR_RegisterVolume - Called by an FSD to register a volume
 *
 *  ENTRY
 *      pDsk -> DSK passed to FSD_MountDisk
 *      pwsName -> desired volume name, NULL for default name (eg, "Storage Card")
 *      pVolume == FSD-defined volume-specific data
 *
 *  EXIT
 *      Volume identifier, NULL if none
 */

PVOL FSDMGR_RegisterVolume(PDSK pDsk, PWSTR pwsName, PVOLUME pVolume)
{
    PVOL pVol;

    ASSERT(VALIDSIG(pDsk, DSK_SIG));

    EnterCriticalSection(&csFSD);

    if (pVol = AllocVolume(pDsk, (DWORD)pVolume)) {

        PFSD pFSD = pVol->pDsk->pFSD;

        if (GetFSDProcArray(pFSD, pFSD->apfnAFS,  apfnAFSStubs,  apwsAFSAPIs,  ARRAY_SIZE(apwsAFSAPIs)) &&
            GetFSDProcArray(pFSD, pFSD->apfnFile, apfnFileStubs, apwsFileAPIs, ARRAY_SIZE(apwsFileAPIs)) &&
            GetFSDProcArray(pFSD, pFSD->apfnFind, apfnFindStubs, apwsFindAPIs, ARRAY_SIZE(apwsFindAPIs))) {

            if (pVol->iAFS == INVALID_AFS) {

                // Determine what AFS index to use.  We try to use OID_FIRST_AFS
                // first, because FILESYS.EXE pre-registers the name "Storage Card"
                // at that index (to prevent anyone from creating a folder or file
                // with that name).  However, if that fails, then we'll try to create
                // a unique derivation of whatever name FILESYS.EXE had reserved at
                // that index, register the new name, and use the resulting AFS index.

                



                int cch;
                int iAFS, iSuffix = 1;
                WCHAR wsAFS[128];

                cch = GetAFSName(OID_FIRST_AFS, wsAFS, ARRAY_SIZE(wsAFS));

                if (pwsName == NULL && cch) {

                    // The FSD did not provide a name, and the default name is already in use,
                    // so we should just try derivations of the default name.

                    iSuffix = 2;
                    pwsName = wsAFS;
                }

                if (pwsName == NULL) {

                    // The FSD did not provide a name, and the default name is NOT in use,
                    // so we should be able to use the default name.

                    if (RegisterAFS(OID_FIRST_AFS, hAFSAPI, (DWORD)pVol, AFS_VERSION)) {
                        pVol->iAFS = OID_FIRST_AFS;
#ifdef DEBUG
                        VERIFYTRUE(GetAFSName(pVol->iAFS, wsAFS, ARRAY_SIZE(wsAFS)));
                        memcpy(pVol->wsVol, wsAFS, min(sizeof(pVol->wsVol),sizeof(wsAFS)));
                        DEBUGMSGW(ZONE_EVENTS,(DBGTEXTW("FSDMGR_RegisterVolume: registered primary volume as %s\n"), pVol->wsVol));
#endif
                    }
                    else
                        DEBUGMSGW(ZONE_EVENTS,(DBGTEXTW("FSDMGR_RegisterVolume: primary volume could not be registered (%d)\n"), GetLastError()));
                }
                else {

                    // Either the FSD provided a name, or the default name is in use,
                    // so we should try all the appropriate derivations.

                    wcscpy(wsAFS, pwsName[0] == '\\'? pwsName+1 : pwsName);
                    cch = wcslen(wsAFS);

                    do {
                        if (iSuffix > 1) {
                            wsAFS[cch] = '0'+iSuffix;
                            wsAFS[cch+1] = 0;
                        }

                        







                        iAFS = RegisterAFSName(wsAFS);
                        if (iAFS != INVALID_AFS && GetLastError() == 0) {
                            DEBUGMSGW(ZONE_EVENTS,(DBGTEXTW("FSDMGR_RegisterVolume: registering secondary volume as %s\n"), wsAFS));
                            if (RegisterAFS(iAFS, hAFSAPI, (DWORD)pVol, AFS_VERSION)) {
                                pVol->iAFS = iAFS;
                                break;
                            }
                            DEBUGMSGW(ZONE_EVENTS,(DBGTEXTW("FSDMGR_RegisterVolume: registering secondary volume failed (%d)\n"), GetLastError()));
                            VERIFYTRUE(DeregisterAFSName(iAFS));
                        }
                    } while (iSuffix++ < 9);
                }
            }

            // End of "if pVol->iAFS == INVALID_AFS".  Note, however, that pVol->iAFS may
            // *still* be INVALID_AFS if all our attempts to register a volume name were rebuffed.
        }

        // End of "if GetFSDProcArray()" calls

        if (pVol->iAFS == INVALID_AFS) {
            DeallocVolume(pVol);
            pVol = NULL;
        }
    }

    LeaveCriticalSection(&csFSD);

    return pVol;
}


/*  FSDMGR_GetVolumeName - Called by an FSD to query a volume's registered name
 *
 *  ENTRY
 *      pVol -> VOL structure returned from FSDMGR_RegisterVolume
 *      pwsName -> buffer, or NULL to query size of name
 *      cchMax == max chars allowed in buffer (including terminating NULL), ignored if pwsName is NULL
 *
 *  EXIT
 *      Number of characters returned, NOT including terminating NULL, or 0 if error
 */

int FSDMGR_GetVolumeName(PVOL pVol, PWSTR pwsName, int cchMax)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    return GetAFSName(pVol->iAFS, pwsName, cchMax);
}


/*  FSDMGR_DeregisterVolume - Called by an FSD to deregister a volume
 *
 *  ENTRY
 *      pVol -> VOL structure returned from FSDMGR_RegisterVolume
 *
 *  EXIT
 *      None
 */

void FSDMGR_DeregisterVolume(PVOL pVol)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    EnterCriticalSection(&csFSD);

    if (pVol) {

        if (pVol->iAFS != INVALID_AFS) {
            VERIFYTRUE(DeregisterAFS(pVol->iAFS));
            if (pVol->iAFS > OID_FIRST_AFS)
                VERIFYTRUE(DeregisterAFSName(pVol->iAFS));
            pVol->iAFS = INVALID_AFS;
        }

        DeallocVolume(pVol);
    }

    LeaveCriticalSection(&csFSD);
}


/*  FSDMGR_CreateFileHandle - Called by an FSD to create 'file' handles
 *
 *  ENTRY
 *      pVol -> VOL structure returned from FSDMGR_RegisterVolume
 *      hProc == originating process handle
 *      pFile == FSD-defined file-specific data for the new handle
 *
 *  EXIT
 *      A 'file' handle associated with the originating process, or INVALID_HANDLE_VALUE
 */

HANDLE FSDMGR_CreateFileHandle(PVOL pVol, HANDLE hProc, PFILE pFile)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    return AllocFSDHandle(pVol, hProc, (DWORD)pFile, HDL_FILE);
}


/*  FSDMGR_CreateSearchHandle - Called by an FSD to create 'search' (aka 'find') handles
 *
 *  ENTRY
 *      pVol -> VOL structure returned from FSDMGR_RegisterVolume
 *      hProc == originating process handle
 *      pSearch == FSD-defined search-specific data for the new handle
 *
 *  EXIT
 *      A 'search' handle associated with the originating process, or INVALID_HANDLE_VALUE
 */

HANDLE FSDMGR_CreateSearchHandle(PVOL pVol, HANDLE hProc, PSEARCH pSearch)
{
    ASSERT(VALIDSIG(pVol, VOL_SIG));

    return AllocFSDHandle(pVol, hProc, (DWORD)pSearch, HDL_SEARCH);
}
