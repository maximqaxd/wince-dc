//
// sdcard.c - DCWin SD-card storage driver. For now: mount the FAT volume off the SD
// block driver. NEXT: the WDM (InitWDMDriver) shell + the AFS / API-set tables that
// surface the volume as "\External Storage" in the CE namespace (see the
// wince-fsd-afs-mount notes: 3 CreateAPISet tables -> RegisterAFSName/RegisterAFS).
//
#include <windows.h>
#include "fatfs/ff.h"
#include "sdblk.h"

static FATFS g_fs;

// Mount the SD card's first FAT volume (physical drive 0). 0 = ok.
int SdcardMount(void)
{
    return (f_mount(&g_fs, L"", 1) == FR_OK) ? 0 : -1;
}

BOOL WINAPI DllMain(HANDLE hInst, DWORD reason, LPVOID reserved)
{
    (void)hInst; (void)reason; (void)reserved;
    return TRUE;
}
