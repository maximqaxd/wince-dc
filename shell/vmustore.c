//
// vmustore.c - persist a small blob to the first VMU using the CE "persistent store"
// (perstore.h IFlashDevice/IFlashFile) reached through the Maple device factory
// (mapledev.h MapleEnumerateDevices / MapleCreateDevice). C-style COM via lpVtbl.
//
// The whole path is wrapped in __try so a missing / unformatted VMU, or any COM failure,
// degrades to "no persistence" instead of taking down the shell.
//
#include <windows.h>
#include <string.h>
#include <mapledev.h>
#include <perstore.h>
#include "vmustore.h"

#define VMU_FILE "DCWINSHC.BIN" // <= 12 chars (MAX_FLASH_FILE_NAME)

static GUID s_guid;
static int s_found;

static BOOL PASCAL EnumCb(LPCMAPLEDEVICEINSTANCE pInst, LPVOID pvCtx)
{
	(void)pvCtx;
	if (!s_found && pInst)
	{
		s_guid = pInst->guidDevice;
		s_found = 1;
	}
	return TRUE; // keep going; we just take the first
}

// Enumerate storage devices, create the first one, query its IFlashDevice. NULL on failure.
static IFlashDevice *OpenDev(void)
{
	IUnknown *pUnk = NULL;
	IFlashDevice *pDev = NULL;

	s_found = 0;
	if (FAILED(MapleEnumerateDevices(MDT_STORAGE, EnumCb, NULL, 0)) || !s_found)
		return NULL;
	if (FAILED(MapleCreateDevice(&s_guid, &pUnk)) || !pUnk)
		return NULL;
	if (FAILED(pUnk->lpVtbl->QueryInterface(pUnk, &IID_IFlashDevice, (void **)&pDev)))
		pDev = NULL;
	pUnk->lpVtbl->Release(pUnk);
	return pDev;
}

static void FillDesc(FSFILEDESC *pFd, int bytes)
{
	int nBlk = (bytes + 511) & ~511;
	memset(pFd, 0, sizeof(*pFd));
	pFd->dwSize = sizeof(*pFd);
	pFd->dwFlags = FSFD_CREATE_FILE;
	pFd->dwBytesRequired = nBlk < 512 ? 512 : nBlk;
	strcpy(pFd->szFileName, VMU_FILE);
	strcpy(pFd->szVMSComment, "DCWin");
	strcpy(pFd->szBootROMComment, "DCWin desktop shortcuts");
	pFd->bStatus = FS_STATUS_DATA_FILE;
	pFd->bCopy = FS_COPY_ENABLED;
	pFd->fsfileicon.bAnimationFrames = 1; // minimal (solid) icon, palette[0]=white
	pFd->fsfileicon.palette[0] = (FSARGB4)0xFFFF;
}

int VmuSave(const void *pvData, int len)
{
	IFlashDevice *pDev = NULL;
	IFlashFile *pFile = NULL;
	FSFILEDESC fd;
	BYTE abBlk[512];
	int bOk = 0;

	if (len < 0 || len > 512)
		return 0;

	__try
	{
		pDev = OpenDev();
		if (pDev)
		{
			HRESULT hr = pDev->lpVtbl->OpenFlashFileByName(pDev, &pFile, VMU_FILE);
			if (FAILED(hr) || !pFile) // doesn't exist yet -> create it
			{
				pFile = NULL;
				FillDesc(&fd, len);
				hr = pDev->lpVtbl->CreateFlashFile(pDev, &pFile, &fd);
			}
			if (SUCCEEDED(hr) && pFile)
			{
				memset(abBlk, 0, sizeof(abBlk));
				memcpy(abBlk, pvData, len);
				if (SUCCEEDED(pFile->lpVtbl->Write(pFile, 0, sizeof(abBlk), abBlk)))
					bOk = 1;
				pFile->lpVtbl->Release(pFile);
				pDev->lpVtbl->Flush(pDev);
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bOk = 0;
	}

	if (pDev)
		pDev->lpVtbl->Release(pDev);
	return bOk;
}

int VmuLoad(void *pvData, int maxlen, int *outlen)
{
	IFlashDevice *pDev = NULL;
	IFlashFile *pFile = NULL;
	BYTE abBlk[512];
	int bOk = 0, n;

	if (outlen)
		*outlen = 0;

	__try
	{
		pDev = OpenDev();
		if (pDev && SUCCEEDED(pDev->lpVtbl->OpenFlashFileByName(pDev, &pFile, VMU_FILE)) && pFile)
		{
			if (SUCCEEDED(pFile->lpVtbl->Read(pFile, 0, sizeof(abBlk), abBlk)))
			{
				n = (maxlen < (int)sizeof(abBlk)) ? maxlen : (int)sizeof(abBlk);
				memcpy(pvData, abBlk, n);
				if (outlen)
					*outlen = n;
				bOk = 1;
			}
			pFile->lpVtbl->Release(pFile);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bOk = 0;
	}

	if (pDev)
		pDev->lpVtbl->Release(pDev);
	return bOk;
}
