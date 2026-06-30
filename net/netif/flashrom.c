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

DWORD SetKMode(DWORD fMode); // coredll export

#define FR_VEC_ADDR   0x8C0000B8u // BIOS flashrom syscall function-pointer slot
#define FR_FUNC_INFO  0
#define FR_FUNC_READ  1
#define FR_PT_BLOCK_1 2    // 16K block-allocated partition (user settings)
#define FR_B1_DK_DNS  0xC8 // DreamPassport/DreamKey DNS block
#define FR_B1_PW_DNS  0xC2 // PlanetWeb DNS block
#define FR_OFFSET_CRC 62   // CRC is the last 2 bytes of every 64-byte block

typedef int(*PFN_FR)(int nArg0, void *pArg1, int nArg2, int nFunc);

static PFN_FR FrVector(void)
{
	return (PFN_FR)(*(volatile DWORD *)FR_VEC_ADDR);
}
static int FrInfo(int nPart, int *pnPtrs)
{
	return FrVector()(nPart, pnPtrs, 0, FR_FUNC_INFO);
}
static int FrRead(int nOff, void *pvBuf, int nLen)
{
	return FrVector()(nOff, pvBuf, nLen, FR_FUNC_READ);
}

// CRC-16/CCITT over bytes [0,62) (Marcus Comstedt's algorithm).
static WORD FrCrc(const BYTE *pb)
{
	int i, iBit, nCrc = 0xffff;
	for (i = 0; i < FR_OFFSET_CRC; i++)
	{
		nCrc ^= (int)pb[i] << 8;
		for (iBit = 0; iBit < 8; iBit++)
			nCrc = (nCrc & 0x8000) ? ((nCrc << 1) ^ 4129) : (nCrc << 1);
	}
	return (WORD)((~nCrc) & 0xffff);
}

static WORD Rd16(const BYTE *pb)
{
	return (WORD)(pb[0] | (pb[1] << 8));
} // LE, alignment-safe

// Read the latest valid 64-byte logical block 'nBlockId' from partition 'nPartId' into
// pbOut. Mirrors the reference flashrom_get_block: verify partition magic, scan the allocation
// bitmap (at the partition tail) for the newest used block, match the logical id, CRC.
static int FrGetBlock(int nPartId, int nBlockId, BYTE *pbOut)
{
	int anPtrs[2], nStart, nSize, nBmCnt, i;
	char achMagic[18];
	BYTE abBitmap[128];

	if (FrInfo(nPartId, anPtrs))
		return -1;
	nStart = anPtrs[0];
	nSize = anPtrs[1];
	if (nSize <= 64 || nSize > (1 << 20))
		return -1;
	if (FrRead(nStart, (BYTE *)achMagic, 18) < 0)
		return -1;
	if (memcmp(achMagic, "KATANA_FLASH____", 16) != 0)
		return -1;
	if (Rd16((BYTE *)achMagic + 16) != nPartId)
		return -1;

	nBmCnt = nSize / 64; // 1 bit per 64-byte block...
	nBmCnt = (nBmCnt + (64 * 8) - 1) & ~(64 * 8 - 1);
	nBmCnt = nBmCnt / 8; // ...bytes of bitmap at the partition tail
	if (nBmCnt <= 0 || nBmCnt > (int)sizeof(abBitmap))
		return -1;
	if (FrRead(nStart + nSize - nBmCnt, abBitmap, nBmCnt) < 0)
		return -1;

	for (i = 0; i < nBmCnt * 8; i++) // find first FREE block (set bit)
	{
		if (abBitmap[i / 8] == 0)
			i += 8;
		if (abBitmap[i / 8] & (0x80 >> (i % 8)))
			break;
	}
	if (i == 0)
		return -1;
	for (i--; i > 0; i--) // scan back for the matching, valid block
	{
		if (FrRead(nStart + (i + 1) * 64, pbOut, 64) < 0)
			return -1;
		if (Rd16(pbOut) != nBlockId)
			continue;
		if (FrCrc(pbOut) == Rd16(pbOut + FR_OFFSET_CRC))
			return 0;
	}
	return -1;
}

// Fill adwDns[0..1] (network-order bytes packed into ULONG, matching netif's wire rep) from
// the system flash. Returns the count found (0/1/2). Tries DreamPassport (0xC8, DNS at
// block offsets 50/54) then PlanetWeb (0xC2, offsets 54/58). DNS bytes are stored
// big-endian in flash = exactly the on-wire order we want, so copy verbatim.
int FlashromGetDns(unsigned long adwDns[2])
{
	DWORD dwPrevMode = SetKMode(TRUE);
	int cDns = 0;
	__try
	{
		BYTE abBlk[64];
		if (FrGetBlock(FR_PT_BLOCK_1, FR_B1_DK_DNS, abBlk) == 0)
		{
			memcpy(&adwDns[0], abBlk + 50, 4);
			memcpy(&adwDns[1], abBlk + 54, 4);
			cDns = 2;
		}
		else if (FrGetBlock(FR_PT_BLOCK_1, FR_B1_PW_DNS, abBlk) == 0)
		{
			memcpy(&adwDns[0], abBlk + 54, 4);
			memcpy(&adwDns[1], abBlk + 58, 4);
			cDns = 2;
		}
		if (cDns == 2 && adwDns[1] == 0)
			cDns = 1; // second server optional
		if (cDns >= 1 && adwDns[0] == 0)
			cDns = 0; // 0.0.0.0 = not configured
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		cDns = 0;
	}
	SetKMode(dwPrevMode);
	return cDns;
}
