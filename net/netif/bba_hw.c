//
// bba_hw.c - RTL8139C (Dreamcast Broadband Adapter, GAPS bridge on the G2 bus)
// hardware behind the netif.c LinkOps. Factored from drivers/bba/bba.c (the WDM
// driver); here it is plain code linked into the microstk link DLL (mppp.dll), with
// SetKMode(TRUE) around every G2 access so it works from any caller thread.
//
#include <windows.h>

DWORD SetKMode(DWORD fMode); // coredll export (P2/G2 needs kernel mode)

// The G2 bus is shared with the AICA (sound) + its DMA engines. On real silicon a PIO
// burst that is interleaved by another G2 master - an IRQ-context G2 access, or a G2 DMA
// in flight - corrupts/locks the bus (idealized bus models tolerate it, which is why this only
// bites on hardware). So we match the standard g2_lock()/g2_unlock() sequence around EVERY
// transaction: mask interrupts + suspend G2 DMA + drain the SH4/G2 FIFO. Interrupt
// masking is an SH-4 asm stub (g2lock.src) - the MS SH compiler has no IMASK intrinsic;
// it is privileged (SR.MD=1), legal because every accessor runs under SetKMode(TRUE).
extern unsigned long DcIrqDisable(void); // SR.IMASK<-0xF, returns old SR
extern void DcIrqRestore(unsigned long); // SR<-old

#define G2_FIFO_STATUS  (*(volatile DWORD *)0xA05F688C)
#define FIFO_G2         0x0010                          // bit4: G2 write FIFO non-empty
#define FIFO_SH4        0x0020                          // bit5: SH4 write FIFO non-empty
#define G2_DMA_SUSP_SPU (*(volatile DWORD *)0xA05F781C) // suspend = 1, resume = 0
#define G2_DMA_SUSP_BBA (*(volatile DWORD *)0xA05F783C)
#define G2_DMA_SUSP_CH2 (*(volatile DWORD *)0xA05F785C)

// Drain until both the SH4->G2 store FIFO and the G2 FIFO are empty (mask G2|SH4;
// the old code wrongly waited on AICA|G2 and missed the SH4 store FIFO).
static void G2FifoWait(void)
{
	int i;
	for (i = 0; i < 0x1800; i++)
		if (!(G2_FIFO_STATUS & (FIFO_G2 | FIFO_SH4)))
			break;
}

// G2 access guard, taken per SHORT burst (one register, or one 32-byte chunk of a packet
// copy - see the block fns). Suspends all G2 DMA + drains the SH4/G2 FIFO + masks interrupts,
// so the burst is atomic vs every other G2 master: AICA sound (its DMA *and* its ISR's PIO)
// and the G2-DMA-complete ISR. The IRQ mask is REQUIRED for sound coexistence - a WinCE game
// using our shim can have AICA audio whose interrupt handler pokes G2, and that must not
// interleave a burst. The catch is the MAPLE bus (controllers/keyboard, a SEPARATE bus):
// masking IRQs too long starves its poll and breaks controller input. So every locked window
// is kept TINY - the block copies re-lock every 32 bytes (the G2 FIFO size) and run UNLOCKED
// between chunks, letting Maple + the AICA ISR be serviced ~once per 32 bytes. Short windows
// are both sound-safe and Maple-safe; the bug was locking once around a whole ~1.5KB packet.
static unsigned long G2Lock(void)
{
	unsigned long dwSr = DcIrqDisable();
	G2_DMA_SUSP_SPU = 1;
	G2_DMA_SUSP_BBA = 1;
	G2_DMA_SUSP_CH2 = 1;
	G2FifoWait();
	return dwSr;
}
static void G2Unlock(unsigned long dwSr)
{
	G2_DMA_SUSP_SPU = 0;
	G2_DMA_SUSP_BBA = 0;
	G2_DMA_SUSP_CH2 = 0;
	DcIrqRestore(dwSr);
}

static BYTE G2Read8(DWORD dwAddr)
{
	unsigned long dwLk = G2Lock();
	BYTE bVal = *(volatile BYTE *)dwAddr;
	G2Unlock(dwLk);
	return bVal;
}
static WORD G2Read16(DWORD dwAddr)
{
	unsigned long dwLk = G2Lock();
	WORD wVal = *(volatile WORD *)dwAddr;
	G2Unlock(dwLk);
	return wVal;
}
static DWORD G2Read32(DWORD dwAddr)
{
	unsigned long dwLk = G2Lock();
	DWORD dwVal = *(volatile DWORD *)dwAddr;
	G2Unlock(dwLk);
	return dwVal;
}
static void G2Write8(DWORD dwAddr, BYTE bVal)
{
	unsigned long dwLk = G2Lock();
	*(volatile BYTE *)dwAddr = bVal;
	G2Unlock(dwLk);
}
static void G2Write16(DWORD dwAddr, WORD wVal)
{
	unsigned long dwLk = G2Lock();
	*(volatile WORD *)dwAddr = wVal;
	G2Unlock(dwLk);
}
static void G2Write32(DWORD dwAddr, DWORD dwVal)
{
	unsigned long dwLk = G2Lock();
	*(volatile DWORD *)dwAddr = dwVal;
	G2Unlock(dwLk);
}
// Block ops lock ONCE around the whole loop (as the reference g2_*_block ops do), draining the FIFO
// every 32 bytes within the held lock.
static void G2ReadBlock(BYTE *pbDst, DWORD dwAddr, int n)
{
	unsigned long dwLk = G2Lock();
	int i;
	for (i = 0; i < n; i++)
	{
		if (i && !(i & 31))
			G2FifoWait();
		pbDst[i] = *(volatile BYTE *)(dwAddr + i);
	}
	G2Unlock(dwLk);
}
static void G2WriteBlock(DWORD dwAddr, const BYTE *pbSrc, int n)
{
	unsigned long dwLk = G2Lock();
	int i;
	for (i = 0; i < n; i++)
	{
		if (i && !(i & 31))
			G2FifoWait();
		*(volatile BYTE *)(dwAddr + i) = pbSrc[i];
	}
	G2Unlock(dwLk);
}
// 32-bit block copies for the RTL8139 DMA window (the GAPS-mapped RX/TX frame buffers).
// The GAPS bridge + G2 bus latch 32-bit; sub-word (byte) access to the DMA window returns
// or writes INCOHERENT bytes on real silicon (idealized models return clean bytes either way, which
// is why byte copies "worked" there). The reference uses g2_read_block_32/g2_write_block_32 for the
// frame body and byte access ONLY for the GAPS config-register signature. Length is rounded
// up to a dword; both buffers are 4-aligned and sized >= the rounded length (RX off and TX
// buf are dword-aligned; the SH-4 frame buffers are stack/aligned). FIFO drains every 8 dw.
// Lock per 8-dword (32-byte = one G2 FIFO) chunk, NOT once around the whole packet: each
// chunk is IRQ-masked + atomic, but IRQs (Maple poll, AICA ISR) run between chunks. A 1.5KB
// frame is ~47 chunks, so Maple is serviced ~47x per packet instead of being starved for it.
static void G2ReadBlock32(BYTE *pbDst, DWORD dwAddr, int n)
{
	int dw = (n + 3) >> 2, i = 0;
	DWORD *pdw = (DWORD *)pbDst;
	while (i < dw)
	{
		int e = i + 8;
		unsigned long dwLk;
		if (e > dw)
			e = dw;
		dwLk = G2Lock();
		for (; i < e; i++)
			pdw[i] = *(volatile DWORD *)(dwAddr + (DWORD)i * 4);
		G2Unlock(dwLk);
	}
}
static void G2WriteBlock32(DWORD dwAddr, const BYTE *pbSrc, int n)
{
	int dw = (n + 3) >> 2, i = 0;
	const DWORD *pdw = (const DWORD *)pbSrc;
	while (i < dw)
	{
		int e = i + 8;
		unsigned long dwLk;
		if (e > dw)
			e = dw;
		dwLk = G2Lock();
		for (; i < e; i++)
			*(volatile DWORD *)(dwAddr + (DWORD)i * 4) = pdw[i];
		G2Unlock(dwLk);
	}
}

#define GAPS_BASE           0xA1000000
#define RTL_DMA             0x01840000
#define RTL_CPU             0xA1840000
#define NIC(reg)            (GAPS_BASE + 0x1700 + (reg))
#define RT_IDR0             0x00
#define RT_MAR0             0x08
#define RT_MAR4             0x0c
#define RT_TXSTATUS0        0x10
#define RT_TXADDR0          0x20
#define RT_RXBUF            0x30
#define RT_CHIPCMD          0x37
#define RT_RXBUFTAIL        0x38
#define RT_INTRMASK         0x3c
#define RT_INTRSTATUS       0x3e
#define RT_TXCONFIG         0x40
#define RT_RXCONFIG         0x44
#define RT_RXMISSED         0x4C // missed-packet counter (write clears)
#define RT_CFG9346          0x50 // 93C46 EEPROM command / config-write unlock
#define RT_CONFIG1          0x52
#define RT_CONFIG4          0x5A
#define RT_MULTIINTR        0x5C
#define RT_MII_BMCR         0x62 // PHY basic-mode control (16-bit)
#define RT_MII_BMSR         0x64 // PHY basic-mode status  (16-bit, RO)
#define RT_CONFIG5          0xD8
#define RT_CMD_RESET        0x10
#define RT_CMD_RX_ENABLE    0x08
#define RT_CMD_TX_ENABLE    0x04
#define RT_CMD_RX_BUF_EMPTY 0x01
#define RT_TX_HOST_OWNS     0x00002000
#define RT_CFG_UNLOCK       0xC0 // CFG9346: enable CONFIG0-5 writes
#define RT_CONFIG1_LWACT    0x10
#define RT_CONFIG1_LED0     0x40
#define RT_CONFIG1_DVRLOAD  0x20
#define RT_CONFIG1_LED1     0x80
#define RT_CONFIG4_RXFIFOAC 0x80 // auto-clear RX FIFO overflow
#define RT_CONFIG5_LDPS     0x04 // disable link-down power-save (keeps PHY alive)
#define RT_MII_RESET        0x8000
#define RT_MII_AN_ENABLE    0x1000
#define RT_MII_AN_START     0x0200
#define RT_MII_LINK         0x0004
#define RXC_RBLEN_16K       (1 << 11)
#define RXC_MXDMA_1K        (6 << 8)
#define RXC_WRAP            0x80
#define RXC_AB              0x08
#define RXC_APM             0x02
#define RX_CONFIG           (RXC_RBLEN_16K | RXC_MXDMA_1K | RXC_WRAP)
#define TX_CONFIG           (6 << 8)
#define RX_BUF_LEN          0x4000
#define TX_OFF              (RX_BUF_LEN + 0x2000)
#define TX_LEN              0x800
#define TX_N                4

static BYTE s_abMac[6];
static DWORD s_dwCurtx, s_dwCurrx;
static CRITICAL_SECTION s_g2cs; // serialize G2 + ring state (TX thread vs RX worker)
static int s_nG2csInit;

static int GapsInit(void)
{
	char achStr[16];
	int i;
	G2ReadBlock((BYTE *)achStr, GAPS_BASE + 0x1400, 16);
	if (memcmp(achStr, "GAPSPCI_BRIDGE_2", 16) != 0)
		return -1;
	G2Write32(GAPS_BASE + 0x1418, 0x5a14a501);
	for (i = 10000; i > 0 && !(G2Read32(GAPS_BASE + 0x1418) & 1); i--)
		;
	if (!(G2Read32(GAPS_BASE + 0x1418) & 1))
		return -2;
	G2Write32(GAPS_BASE + 0x1420, 0x01000000);
	G2Write32(GAPS_BASE + 0x1424, 0x01000000);
	G2Write32(GAPS_BASE + 0x1428, RTL_DMA);
	G2Write32(GAPS_BASE + 0x142c, RTL_DMA + 32 * 1024);
	G2Write32(GAPS_BASE + 0x1414, 0x00000001);
	G2Write32(GAPS_BASE + 0x1434, 0x00000001);
	G2Write16(GAPS_BASE + 0x1606, 0xf900);
	G2Write32(GAPS_BASE + 0x1630, 0x00000000);
	G2Write8(GAPS_BASE + 0x163c, 0x00);
	G2Write8(GAPS_BASE + 0x160d, 0xf0);
	G2Write16(GAPS_BASE + 0x1604, G2Read16(GAPS_BASE + 0x1604) | 0x6);
	G2Write32(GAPS_BASE + 0x1614, 0x01000000);
	if (G2Read8(GAPS_BASE + 0x1650) & 0x1)
		G2Write16(GAPS_BASE + 0x1654, (G2Read16(GAPS_BASE + 0x1654) & 0xfffc) | 0x8000);
	G2Write32(GAPS_BASE + 0x1414, 0x00000001);
	// GAPS 0x141c "SEGA" magic handshake: write the 3-step sequence and restore
	// 'SEGA'. The final write makes GAPS pull RSTB low ~120ns, which autoloads the
	// RTL8139 registers from its EEPROM - notably the MAC. Without this, IDR0 reads
	// can come back zero on real silicon (idealized models pre-populate them, hiding it).
	if (G2Read32(GAPS_BASE + 0x141c) == 0x41474553)
	{ // 'SEGA' LE
		G2Write32(GAPS_BASE + 0x141c, 0x55aaff00);
		if (G2Read32(GAPS_BASE + 0x141c) == 0x55aaff00)
		{
			G2Write32(GAPS_BASE + 0x141c, 0xaa5500ff);
			if (G2Read32(GAPS_BASE + 0x141c) == 0xaa5500ff)
				G2Write32(GAPS_BASE + 0x141c, 0x41474553); // restore -> RSTB/EEPROM autoload
		}
	}
	return 0;
}
// Reset must complete before we touch any other register. The reference polls reset-done on a real
// wall-clock timeout (yielding); the old iteration-count spin completes "instantly" in the
// an idealized model but on a cold chip can run out before reset finishes -> we'd then program a
// chip mid-reset (writes silently lost / wedge). Yield 1ms per poll, up to ~100ms.
static void RtlReset(void)
{
	int i;
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RESET);
	for (i = 0; i < 100 && (G2Read8(NIC(RT_CHIPCMD)) & RT_CMD_RESET); i++)
		Sleep(1);
}
static void RtlReadMac(void)
{
	DWORD dwLo = G2Read32(NIC(RT_IDR0)), dwHi = G2Read32(NIC(RT_IDR0 + 4));
	s_abMac[0] = (BYTE)dwLo;
	s_abMac[1] = (BYTE)(dwLo >> 8);
	s_abMac[2] = (BYTE)(dwLo >> 16);
	s_abMac[3] = (BYTE)(dwLo >> 24);
	s_abMac[4] = (BYTE)dwHi;
	s_abMac[5] = (BYTE)(dwHi >> 8);
}
// Faithful port of KOS bba_hw_init. The critical real-HW fix vs the old order: program RBSTART
// (RX buffer base) + TX descriptor bases FIRST, then issue a SECOND RtlReset - the RTL8139
// soft-reset preserves RBSTART but re-inits the RX DMA state machine, so the receiver actually
// latches the buffer pointer. The old code set RBSTART after the (single) reset and enabled RX
// immediately, leaving the RX engine un-latched on real silicon: receiver "on", MAC reads fine,
// but accepted frames DMA nowhere -> no DHCP OFFER ever delivered (idealized models latch on
// every touch, hiding it). Also include KOS's two magic readback dances and the final
// enable-then-auto-negotiate ordering.
static void RtlStart(void)
{
	int i;
	G2Write32(NIC(RT_RXBUF), RTL_DMA); // RBSTART (RX disabled)
	for (i = 0; i < TX_N; i++)
		G2Write32(NIC(RT_TXADDR0 + i * 4), RTL_DMA + i * TX_LEN + TX_OFF);
	RtlReset(); // KOS: re-init engine to latch RBSTART

	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE); // magic enable/disable dance
	if (G2Read8(NIC(RT_CHIPCMD)) == RT_CMD_RX_ENABLE)
	{
		G2Write8(NIC(RT_CHIPCMD), RT_CMD_TX_ENABLE);
		if (G2Read8(NIC(RT_CHIPCMD)) == RT_CMD_TX_ENABLE)
			G2Write8(NIC(RT_CHIPCMD), 0);
	}
	G2Write32(NIC(RT_MAR0), 0x55aaff00); // magic MAR readback dance
	G2Write32(NIC(RT_MAR4), 0xaa5500ff);
	if (G2Read32(NIC(RT_MAR0)) == 0x55aaff00 && G2Read32(NIC(RT_MAR4)) == 0xaa5500ff)
	{
		G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
		G2Write32(NIC(RT_MAR0), 0xffffffff);
		G2Write32(NIC(RT_MAR4), 0xffffffff);
	}

	G2Write16(NIC(RT_INTRMASK), 0); // we POLL; no RTL/G2 IRQ
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
	G2Write32(NIC(RT_RXCONFIG), RX_CONFIG);
	G2Write32(NIC(RT_TXCONFIG), TX_CONFIG);

	// Config-register dance (CFG9346 unlock 0xC0): CONFIG1 DVRLOAD, CONFIG4 RX-FIFO auto-clear,
	// CONFIG5 LDPS off (keep the PHY powered to negotiate). Relock after.
	G2Write8(NIC(RT_CFG9346), RT_CFG_UNLOCK);
	G2Write8(NIC(RT_CONFIG1),
	         (BYTE)((G2Read8(NIC(RT_CONFIG1)) & ~(RT_CONFIG1_LWACT | RT_CONFIG1_LED0)) |
	                RT_CONFIG1_DVRLOAD | RT_CONFIG1_LED1));
	G2Write8(NIC(RT_CONFIG4), (BYTE)(G2Read8(NIC(RT_CONFIG4)) | RT_CONFIG4_RXFIFOAC));
	G2Write8(NIC(RT_CONFIG5), (BYTE)(G2Read8(NIC(RT_CONFIG5)) | RT_CONFIG5_LDPS));
	G2Write8(NIC(RT_CFG9346), 0);

	G2Write32(NIC(RT_MAR0), 0xffffffff); // accept all multicast (bcast via AB)
	G2Write32(NIC(RT_MAR4), 0xffffffff);
	G2Write16(NIC(RT_MULTIINTR), 0);
	G2Write16(NIC(RT_INTRSTATUS), 0xffff); // ack pending
	G2Write32(NIC(RT_RXMISSED), 0);
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE); // final enable (KOS:388)

	// Kick auto-negotiation LAST. The netif DHCP worker retries every 1.5s, so the lease lands
	// once link comes up (~2-3s); we don't block boot on it.
	G2Write16(NIC(RT_MII_BMCR), RT_MII_RESET | RT_MII_AN_ENABLE | RT_MII_AN_START);

	s_dwCurtx = 0;
	s_dwCurrx = 0;
	G2Write32(NIC(RT_RXCONFIG),
	          G2Read32(NIC(RT_RXCONFIG)) | RXC_APM | RXC_AB);          // accept phys-match + bcast
	G2Write16(NIC(RT_RXBUFTAIL), (WORD)((0 - 16) & (RX_BUF_LEN - 1))); // CAPR = -16 (empty ring)
}

// PHY link state (BMSR link bit). Exposed so the shim/app can show link-up before DHCP.
int BbaLinkUp(void)
{
	DWORD dwPrev = SetKMode(TRUE);
	int nUp = 0;
	__try
	{
		nUp = (G2Read16(NIC(RT_MII_BMSR)) & RT_MII_LINK) ? 1 : 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nUp = 0;
	}
	SetKMode(dwPrev);
	return nUp;
}
static BOOL Tx(const BYTE *pbPkt, int nLen)
{
	DWORD dwTsd = NIC(RT_TXSTATUS0 + 4 * s_dwCurtx), dwBuf = RTL_CPU + s_dwCurtx * TX_LEN + TX_OFF;
	int i;
	if (nLen <= 0 || nLen > 1514)
		return FALSE;
	G2WriteBlock32(dwBuf, pbPkt, nLen); // 32-bit PIO to the TX DMA window (G2 width rule)
	if (nLen < 60)
	{
		for (i = nLen; i < 60; i++)
			G2Write8(dwBuf + i, 0);
		nLen = 60;
	}
	G2Write32(dwTsd, (DWORD)nLen);
	s_dwCurtx = (s_dwCurtx + 1) & (TX_N - 1);
	return TRUE;
}
static int Rx(BYTE *pbBuf, int nMaxlen)
{
	DWORD dwStatus, dwOff;
	int nSize, nPkt;
	if (G2Read8(NIC(RT_CHIPCMD)) & RT_CMD_RX_BUF_EMPTY)
		return 0;
	dwOff = s_dwCurrx % RX_BUF_LEN;
	dwStatus = G2Read32(RTL_CPU + dwOff);
	nSize = (int)((dwStatus >> 16) & 0xffff);
	if (nSize == 0xfff0)
		return 0;
	nPkt = nSize - 4;
	if (!(dwStatus & 1) || nPkt < 14 || nPkt > 1514)
	{
		RtlReset();
		RtlStart();
		return -1;
	}
	if (nPkt > nMaxlen)
		nPkt = nMaxlen;
	G2ReadBlock32(pbBuf, RTL_CPU + dwOff + 4,
	              nPkt); // 32-bit PIO from the RX DMA window (G2 width rule)
	s_dwCurrx = (s_dwCurrx + nSize + 4 + 3) & ~3;
	G2Write16(NIC(RT_RXBUFTAIL), (WORD)((s_dwCurrx - 16) & (RX_BUF_LEN - 1)));
	return nPkt;
}

// ---- LinkOps hooks (SetKMode-wrapped) ------------------------------------------
int BbaProbe(void)
{
	DWORD dwPrev = SetKMode(TRUE);
	char achStr[16];
	int nOk = 0;
	__try
	{
		G2ReadBlock((BYTE *)achStr, GAPS_BASE + 0x1400, 16);
		nOk = (memcmp(achStr, "GAPSPCI_BRIDGE_2", 16) == 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nOk = 0;
	}
	SetKMode(dwPrev);
	return nOk;
}
int BbaInit(unsigned char mac[6])
{
	DWORD dwPrev;
	int nOk = 0;
	if (!s_nG2csInit)
	{
		InitializeCriticalSection(&s_g2cs);
		s_nG2csInit = 1;
	} // before any TX/RX
	dwPrev = SetKMode(TRUE);
	__try
	{
		if (GapsInit() == 0)
		{
			RtlReset();
			RtlReadMac();
			RtlStart();
			memcpy(mac, s_abMac, 6);
			nOk = 1;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nOk = 0;
	}
	SetKMode(dwPrev);
	return nOk;
}
int BbaTx(const unsigned char *frame, int len)
{
	DWORD dwPrev;
	int nOk;
	EnterCriticalSection(&s_g2cs);
	dwPrev = SetKMode(TRUE);
	nOk = Tx((const BYTE *)frame, len);
	SetKMode(dwPrev);
	LeaveCriticalSection(&s_g2cs);
	return nOk;
}
int BbaRxPoll(unsigned char *buf, int max)
{
	DWORD dwPrev;
	int n;
	EnterCriticalSection(&s_g2cs);
	dwPrev = SetKMode(TRUE);
	n = Rx((BYTE *)buf, max);
	SetKMode(dwPrev);
	LeaveCriticalSection(&s_g2cs);
	return n;
}
