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

#define VMU_FILE "DCWINSHC.BIN"     // <= 12 chars (MAX_FLASH_FILE_NAME)

static GUID s_guid;
static int  s_found;

static BOOL PASCAL EnumCb(LPCMAPLEDEVICEINSTANCE inst, LPVOID ctx)
{
    (void)ctx;
    if (!s_found && inst) { s_guid = inst->guidDevice; s_found = 1; }
    return TRUE;                                  // keep going; we just take the first
}

// Enumerate storage devices, create the first one, query its IFlashDevice. NULL on failure.
static IFlashDevice *OpenDev(void)
{
    IUnknown     *pUnk = NULL;
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

static void FillDesc(FSFILEDESC *fd, int bytes)
{
    int blk = (bytes + 511) & ~511;
    memset(fd, 0, sizeof(*fd));
    fd->dwSize          = sizeof(*fd);
    fd->dwFlags         = FSFD_CREATE_FILE;
    fd->dwBytesRequired = blk < 512 ? 512 : blk;
    strcpy(fd->szFileName,       VMU_FILE);
    strcpy(fd->szVMSComment,     "DCWin");
    strcpy(fd->szBootROMComment, "DCWin desktop shortcuts");
    fd->bStatus = FS_STATUS_DATA_FILE;
    fd->bCopy   = FS_COPY_ENABLED;
    fd->fsfileicon.bAnimationFrames = 1;             // minimal (solid) icon, palette[0]=white
    fd->fsfileicon.palette[0]       = (FSARGB4)0xFFFF;
}

int VmuSave(const void *data, int len)
{
    IFlashDevice *pDev  = NULL;
    IFlashFile   *pFile = NULL;
    FSFILEDESC    fd;
    BYTE          blk[512];
    int           ok = 0;

    if (len < 0 || len > 512) return 0;

    __try
    {
        pDev = OpenDev();
        if (pDev)
        {
            HRESULT hr = pDev->lpVtbl->OpenFlashFileByName(pDev, &pFile, VMU_FILE);
            if (FAILED(hr) || !pFile)                // doesn't exist yet -> create it
            {
                pFile = NULL;
                FillDesc(&fd, len);
                hr = pDev->lpVtbl->CreateFlashFile(pDev, &pFile, &fd);
            }
            if (SUCCEEDED(hr) && pFile)
            {
                memset(blk, 0, sizeof(blk));
                memcpy(blk, data, len);
                if (SUCCEEDED(pFile->lpVtbl->Write(pFile, 0, sizeof(blk), blk))) ok = 1;
                pFile->lpVtbl->Release(pFile);
                pDev->lpVtbl->Flush(pDev);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { ok = 0; }

    if (pDev) pDev->lpVtbl->Release(pDev);
    return ok;
}

int VmuLoad(void *data, int maxlen, int *outlen)
{
    IFlashDevice *pDev  = NULL;
    IFlashFile   *pFile = NULL;
    BYTE          blk[512];
    int           ok = 0, n;

    if (outlen) *outlen = 0;

    __try
    {
        pDev = OpenDev();
        if (pDev && SUCCEEDED(pDev->lpVtbl->OpenFlashFileByName(pDev, &pFile, VMU_FILE)) && pFile)
        {
            if (SUCCEEDED(pFile->lpVtbl->Read(pFile, 0, sizeof(blk), blk)))
            {
                n = (maxlen < (int)sizeof(blk)) ? maxlen : (int)sizeof(blk);
                memcpy(data, blk, n);
                if (outlen) *outlen = n;
                ok = 1;
            }
            pFile->lpVtbl->Release(pFile);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { ok = 0; }

    if (pDev) pDev->lpVtbl->Release(pDev);
    return ok;
}
