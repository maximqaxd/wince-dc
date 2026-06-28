//
// flashrom.c - read the Dreamcast SYSTEM FLASH ISP config (DNS servers) as a DNS
// fallback for the netif shim. Ported from the reference DC flashrom routine.
//
// The DC system flash stores the user's network settings (set by DreamPassport /
// PlanetWeb / DreamKey): connection method, static IP/netmask/gateway, and 2 DNS
// servers. We read the DNS so name resolution works even when DHCP supplies an IP
// but no option-6 server (or a static-IP LAN with no DHCP).
//
// Mechanism: the DC BIOS exposes a flashrom syscall via the function pointer at
// 0x8C0000B8 (P1, below CE's image at 0x8C010000, so it survives our boot). It's a
// plain C call - the function selector rides in r7 as the 4th argument under the
// normal SH-4 ABI, so NO asm is needed:
//     fn(pos, dest, n, 1)  = FLASHROM_READ      fn(part, int[2], 0, 0) = FLASHROM_INFO
// Everything runs under SetKMode(TRUE) (P1 access + BIOS call) and inside __try, so a
// bad/absent vector on a given board degrades to "no flashrom DNS", never a crash.
//
#include <windows.h>

DWORD SetKMode(DWORD fMode);            // coredll export

#define FR_VEC_ADDR     0x8C0000B8u     // BIOS flashrom syscall function-pointer slot
#define FR_FUNC_INFO    0
#define FR_FUNC_READ    1
#define FR_PT_BLOCK_1   2               // 16K block-allocated partition (user settings)
#define FR_B1_DK_DNS    0xC8            // DreamPassport/DreamKey DNS block
#define FR_B1_PW_DNS    0xC2            // PlanetWeb DNS block
#define FR_OFFSET_CRC   62             // CRC is the last 2 bytes of every 64-byte block

typedef int (*FR_FN)(int a, void *b, int c, int d);
static FR_FN fr_fn(void) { return (FR_FN)(*(volatile DWORD *)FR_VEC_ADDR); }
static int fr_info(int part, int *ptrs)        { return fr_fn()(part, ptrs, 0, FR_FUNC_INFO); }
static int fr_read(int off, void *buf, int n)  { return fr_fn()(off, buf, n, FR_FUNC_READ); }

// CRC-16/CCITT over bytes [0,62) (Marcus Comstedt's algorithm).
static WORD fr_crc(const BYTE *b)
{
    int i, c, n = 0xffff;
    for (i = 0; i < FR_OFFSET_CRC; i++)
    {
        n ^= (int)b[i] << 8;
        for (c = 0; c < 8; c++) n = (n & 0x8000) ? ((n << 1) ^ 4129) : (n << 1);
    }
    return (WORD)((~n) & 0xffff);
}

static WORD rd16(const BYTE *p) { return (WORD)(p[0] | (p[1] << 8)); }   // LE, alignment-safe

// Read the latest valid 64-byte logical block 'blockid' from partition 'partid' into
// out64. Mirrors the reference flashrom_get_block: verify partition magic, scan the allocation
// bitmap (at the partition tail) for the newest used block, match the logical id, CRC.
static int fr_get_block(int partid, int blockid, BYTE *out64)
{
    int  ptrs[2], start, size, bmcnt, i;
    char magic[18];
    BYTE bitmap[128];

    if (fr_info(partid, ptrs)) return -1;
    start = ptrs[0]; size = ptrs[1];
    if (size <= 64 || size > (1 << 20)) return -1;
    if (fr_read(start, (BYTE *)magic, 18) < 0) return -1;
    if (memcmp(magic, "KATANA_FLASH____", 16) != 0) return -1;
    if (rd16((BYTE *)magic + 16) != partid) return -1;

    bmcnt = size / 64;                          // 1 bit per 64-byte block...
    bmcnt = (bmcnt + (64 * 8) - 1) & ~(64 * 8 - 1);
    bmcnt = bmcnt / 8;                          // ...bytes of bitmap at the partition tail
    if (bmcnt <= 0 || bmcnt > (int)sizeof(bitmap)) return -1;
    if (fr_read(start + size - bmcnt, bitmap, bmcnt) < 0) return -1;

    for (i = 0; i < bmcnt * 8; i++)             // find first FREE block (set bit)
    {
        if (bitmap[i / 8] == 0) i += 8;
        if (bitmap[i / 8] & (0x80 >> (i % 8))) break;
    }
    if (i == 0) return -1;
    for (i--; i > 0; i--)                       // scan back for the matching, valid block
    {
        if (fr_read(start + (i + 1) * 64, out64, 64) < 0) return -1;
        if (rd16(out64) != blockid) continue;
        if (fr_crc(out64) == rd16(out64 + FR_OFFSET_CRC)) return 0;
    }
    return -1;
}

// Fill dns[0..1] (network-order bytes packed into ULONG, matching netif's wire rep) from
// the system flash. Returns the count found (0/1/2). Tries DreamPassport (0xC8, DNS at
// block offsets 50/54) then PlanetWeb (0xC2, offsets 54/58). DNS bytes are stored
// big-endian in flash = exactly the on-wire order we want, so copy verbatim.
int FlashromGetDns(unsigned long dns[2])
{
    DWORD prev = SetKMode(TRUE);
    int   n = 0;
    __try
    {
        BYTE blk[64];
        if (fr_get_block(FR_PT_BLOCK_1, FR_B1_DK_DNS, blk) == 0)
        {
            memcpy(&dns[0], blk + 50, 4); memcpy(&dns[1], blk + 54, 4);
            n = 2;
        }
        else if (fr_get_block(FR_PT_BLOCK_1, FR_B1_PW_DNS, blk) == 0)
        {
            memcpy(&dns[0], blk + 54, 4); memcpy(&dns[1], blk + 58, 4);
            n = 2;
        }
        if (n == 2 && dns[1] == 0) n = 1;       // second server optional
        if (n >= 1 && dns[0] == 0) n = 0;       // 0.0.0.0 = not configured
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { n = 0; }
    SetKMode(prev);
    return n;
}
