//
// sdblk.h - SD/SDHC card block access over the dcspi SPI transport (SCIF bus, CS on RTS).
// 512-byte sectors. The FatFs diskio glue (diskio.c) sits on top of this.
//
#ifndef SDBLK_H
#define SDBLK_H

int           SdInit(void);                                          // 0 = ok, <0 = error
int           SdReadSectors(unsigned long lba, int count, void *buf);        // 0 = ok
int           SdWriteSectors(unsigned long lba, int count, const void *buf); // 0 = ok
unsigned long SdSectorCount(void);                                   // total 512B sectors (0=unknown)

#endif // SDBLK_H
