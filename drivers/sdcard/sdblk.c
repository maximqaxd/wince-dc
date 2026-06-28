//
// sdblk.c - SD / SDHC card driver in SPI mode, over the reusable dcspi transport.
// Single-block CMD17/CMD24 I/O (simple + correct; multi-block can come later).
// Reference: the standard elm-chan / KOS SPI-mode SD bring-up sequence.
//
#include "../dcspi/dcspi.h"
#include "sdblk.h"
#include "syslog.h"

#define BUS  DCSPI_BUS_SCIF        // SD adapter wired to the serial port (CS on RTS)
#define CSM  DCSPI_CS_RTS

// SD commands (index, sent as 0x40|idx)
#define CMD0    0     // GO_IDLE_STATE
#define CMD8    8     // SEND_IF_COND
#define CMD9    9     // SEND_CSD
#define CMD16   16    // SET_BLOCKLEN
#define CMD17   17    // READ_SINGLE_BLOCK
#define CMD24   24    // WRITE_BLOCK
#define CMD55   55    // APP_CMD
#define CMD58   58    // READ_OCR
#define ACMD41  41    // SD_SEND_OP_COND

static int           g_inited   = 0;
static int           g_byteAddr = 1;     // 1 = SDSC (byte address), 0 = SDHC/SDXC (block address)
static unsigned long g_sectors  = 0;

static unsigned char rb(void)             { return SpiRwByte(BUS, 0xFF); }
static void          wb(unsigned char b)  { SpiRwByte(BUS, b); }
static void          cs(int assert)       { SpiSetCS(BUS, assert); }   // assert=1 -> CS low

// Send a command frame [0x40|cmd][arg32][crc], return the R1 response (0xFF on timeout).
static unsigned char sd_cmd(int cmd, unsigned long arg)
{
    unsigned char crc = 0x01, r;
    int i;
    if (cmd == CMD0) crc = 0x95;          // valid CRC7 needed before CRC is turned off
    if (cmd == CMD8) crc = 0x87;
    wb((unsigned char)(0x40 | cmd));
    wb((unsigned char)(arg >> 24));
    wb((unsigned char)(arg >> 16));
    wb((unsigned char)(arg >> 8));
    wb((unsigned char)(arg));
    wb(crc);
    for (i = 0; i < 16; i++) { r = rb(); if (!(r & 0x80)) return r; }   // R1: top bit clears
    return 0xFF;
}

static int sd_wait_ready(void)            // poll until the card releases busy (returns 0xFF)
{
    int i;
    for (i = 0; i < 200000; i++) if (rb() == 0xFF) return 0;
    return -1;
}

static void sd_read_csd(void)             // CMD9 -> 16-byte CSD -> decode capacity into g_sectors
{
    unsigned char csd[16];
    int i;
    if (sd_cmd(CMD9, 0) != 0) return;
    for (i = 0; i < 100000; i++) if (rb() == 0xFE) break;       // data start token
    if (i == 100000) return;
    for (i = 0; i < 16; i++) csd[i] = rb();
    rb(); rb();                                                 // CSD CRC16
    if ((csd[0] >> 6) == 1)                                     // CSD v2 (SDHC/SDXC)
    {
        unsigned long csize = ((unsigned long)(csd[7] & 0x3F) << 16) |
                              ((unsigned long)csd[8] << 8) | csd[9];
        g_sectors = (csize + 1) * 1024;                        // (C_SIZE+1) * 512KB / 512
    }
    else                                                       // CSD v1 (SDSC)
    {
        unsigned long csize = ((unsigned long)(csd[6] & 0x03) << 10) |
                              ((unsigned long)csd[7] << 2) | (csd[8] >> 6);
        int csmult  = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
        int rdbllen = csd[5] & 0x0F;
        unsigned long blocks   = (csize + 1) * (1UL << (csmult + 2));
        unsigned long blocklen = 1UL << rdbllen;
        g_sectors = blocks * (blocklen / 512);
    }
}

int SdInit(void)
{
    unsigned char r = 0xFF, ocr[4];
    int i, v2 = 0;

    g_inited = 0; g_byteAddr = 1; g_sectors = 0;
    SysLog(L"sd: SdInit enter");
    if (SpiInit(BUS, CSM)) { SysLog(L"sd: SpiInit FAILED"); return -1; }
    SysLog(L"sd: SpiInit ok (SCIF)");

    cs(0);                                          // CS high during the power-up clocks
    for (i = 0; i < 10; i++) wb(0xFF);              // >= 74 clocks
    cs(1);

    r = sd_cmd(CMD0, 0);
    SysLog(L"sd: CMD0 -> %02x", r);
    if (r != 0x01) { cs(0); return -2; }                         // enter idle/SPI mode

    if (sd_cmd(CMD8, 0x000001AA) == 0x01)                        // v2 card
    {
        for (i = 0; i < 4; i++) ocr[i] = rb();                   // R7 trailer
        if (ocr[2] != 0x01 || ocr[3] != 0xAA) { cs(0); return -3; }
        v2 = 1;
    }

    for (i = 0; i < 20000; i++)                                  // ACMD41 init loop
    {
        sd_cmd(CMD55, 0);
        r = sd_cmd(ACMD41, v2 ? 0x40000000UL : 0);              // HCS bit for v2
        if (r == 0x00) break;
    }
    if (r != 0x00) { cs(0); return -4; }

    if (v2 && sd_cmd(CMD58, 0) == 0x00)                          // read OCR -> CCS (bit30)
    {
        for (i = 0; i < 4; i++) ocr[i] = rb();
        g_byteAddr = (ocr[0] & 0x40) ? 0 : 1;                   // CCS=1 -> block addressing
    }
    if (g_byteAddr) sd_cmd(CMD16, 512);                         // fix block length for SDSC

    sd_read_csd();

    cs(0); rb();
    g_inited = 1;
    SysLog(L"sd: init OK sectors=%u byteAddr=%d", g_sectors, g_byteAddr);
    return 0;
}

int SdReadSectors(unsigned long lba, int count, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned long addr;
    int i;

    if (!g_inited) return -1;
    addr = g_byteAddr ? lba * 512 : lba;
    cs(1);
    while (count-- > 0)
    {
        unsigned char t = 0xFF;
        if (sd_cmd(CMD17, addr) != 0) { cs(0); return -1; }
        for (i = 0; i < 100000; i++) { t = rb(); if (t != 0xFF) break; }
        if (t != 0xFE) { cs(0); return -1; }                    // data start token
        SpiRwData(BUS, 0, p, 512);                              // read 512 (tx NULL -> 0xFF)
        rb(); rb();                                             // data CRC16
        p += 512;
        addr += g_byteAddr ? 512 : 1;
    }
    cs(0); rb();
    return 0;
}

int SdWriteSectors(unsigned long lba, int count, const void *buf)
{
    const unsigned char *p = (const unsigned char *)buf;
    unsigned long addr;

    if (!g_inited) return -1;
    addr = g_byteAddr ? lba * 512 : lba;
    cs(1);
    while (count-- > 0)
    {
        if (sd_cmd(CMD24, addr) != 0) { cs(0); return -1; }
        wb(0xFF);                                               // 1-byte gap before token
        wb(0xFE);                                               // data start token
        SpiRwData(BUS, p, 0, 512);                              // write 512
        wb(0xFF); wb(0xFF);                                     // dummy CRC16
        if ((rb() & 0x1F) != 0x05) { cs(0); return -1; }        // data response: 010 = accepted
        if (sd_wait_ready()) { cs(0); return -1; }              // wait out the program-busy
        p += 512;
        addr += g_byteAddr ? 512 : 1;
    }
    cs(0); rb();
    return 0;
}

unsigned long SdSectorCount(void) { return g_sectors; }
