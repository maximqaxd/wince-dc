//
// diskio.c - FatFs low-level disk glue: maps ChaN FatFs's disk_* contract onto the SD
// block driver (single physical drive 0 = the SD card). Also provides get_fattime().
//
#include <windows.h>
#include "fatfs/diskio.h"
#include "sdblk.h"

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    return SdInit() ? STA_NOINIT : 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, DWORD count)
{
    (void)pdrv;
    return SdReadSectors(sector, (int)count, buff) ? RES_ERROR : RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, DWORD count)
{
    (void)pdrv;
    return SdWriteSectors(sector, (int)count, buff) ? RES_ERROR : RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;
    switch (cmd)
    {
    case CTRL_SYNC:        return RES_OK;
    case GET_SECTOR_COUNT: *(DWORD *)buff = SdSectorCount(); return RES_OK;
    case GET_SECTOR_SIZE:  *(WORD  *)buff = 512;             return RES_OK;
    case GET_BLOCK_SIZE:   *(DWORD *)buff = 1;               return RES_OK;
    }
    return RES_PARERR;
}

// Packed DOS date/time: bits 31..25 year-1980, 24..21 month, 20..16 day,
// 15..11 hour, 10..5 minute, 4..0 second/2.
DWORD get_fattime(void)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    return ((DWORD)(t.wYear - 1980) << 25) | ((DWORD)t.wMonth << 21) | ((DWORD)t.wDay << 16)
         | ((DWORD)t.wHour << 11) | ((DWORD)t.wMinute << 5) | ((DWORD)t.wSecond >> 1);
}
