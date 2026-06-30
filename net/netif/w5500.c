//
// w5500.c - WIZnet W5500 in MACRAW mode as a netif.c LinkOps backend (probe/init/
// tx/poll of raw ethernet frames). MACRAW = the chip hands us / sends raw ethernet,
// so all of netif.c (ARP, DHCP, DNS) works unchanged - the W5500's own TCP/IP engine
// is bypassed. Transport is dcspi.dll (SCI hardware-SPI or SCIF bit-bang).
//
// SAFETY: probing bit-bangs SCIF/SCI + GPIO, which on a stock board would disturb the
// debug console / port pins. So the W5500 is OFF unless explicitly enabled+wired via
//   HKLM\Comm\Netif : "W5500Bus" = 1 (SCI, CS on PA7 GPIO) | 2 (SCIF, CS on RTS)
// (absent/0 = disabled -> BBA/modem path untouched). dcspi.dll is loaded on demand,
// so core networking has no link-time dependency on it.
//
#include <windows.h>
#include "dcspi.h" // DCSPI_BUS_* / DCSPI_CS_* constants only
#include "syslog.h"
#include "dcboot.h"

// dcspi.dll entry points, bound on demand (no static import).
typedef int (*PFN_Init)(int, int);
typedef void (*PFN_Shutdown)(int);
typedef void (*PFN_SetCS)(int, int);
typedef void (*PFN_RwData)(int, const unsigned char *, unsigned char *, int);
typedef int (*PFN_HwType)(void);
static PFN_Init s_pInit;
static PFN_Shutdown s_pShutdown;
static PFN_SetCS s_pSetCS;
static PFN_RwData s_pRw;
static PFN_HwType s_pHwType;

// Serialize every W5500 SPI transaction: the NetWorker RX-poll thread and microstk's TX path
// (connect/send -> OurTransmit -> W5500Tx) hit the chip from DIFFERENT threads. Without this
// their SCIF bit-bang interleaves at the CS/clock level and corrupts frames - a probabilistic
// race that lets DNS + the fast/close 1.1.1.1 survive but drops slower multi-round-trip
// connects. Mirrors KOS's w5500_spi_mutex and our own BBA s_g2cs. (CE critical sections are
// recursive, so nested W5* calls are safe.)
static CRITICAL_SECTION s_w5cs;
static int s_nW5csInit;
#define W5LOCK()   EnterCriticalSection(&s_w5cs)
#define W5UNLOCK() LeaveCriticalSection(&s_w5cs)

static int LoadDcspi(void)
{
	HMODULE hMod;
	if (!s_nW5csInit)
	{
		InitializeCriticalSection(&s_w5cs);
		s_nW5csInit = 1;
	} // before any W5* access
	if (s_pInit)
		return 1;
	hMod = LoadLibraryW(L"dcspi.dll");
	if (!hMod)
		return 0;
	s_pInit = (PFN_Init)GetProcAddress(hMod, L"SpiInit");
	s_pShutdown = (PFN_Shutdown)GetProcAddress(hMod, L"SpiShutdown");
	s_pSetCS = (PFN_SetCS)GetProcAddress(hMod, L"SpiSetCS");
	s_pRw = (PFN_RwData)GetProcAddress(hMod, L"SpiRwData");
	s_pHwType = (PFN_HwType)GetProcAddress(hMod, L"DcspiHwType"); // optional (Naomi vs DC)
	return (s_pInit && s_pSetCS && s_pRw) ? 1 : 0;
}

// ---- W5500 block-select (BSB) + control byte -----------------------------------
#define BSB_COMMON    0x00
#define BSB_S0_REG    0x01
#define BSB_S0_TX     0x02
#define BSB_S0_RX     0x03
#define CTRL(bsb, wr) ((BYTE)(((bsb) << 3) | ((wr) ? 0x04 : 0x00))) // OM=00 (VDM)

#define MR            0x0000
#define SHAR          0x0009
#define PHYCFGR       0x002E // common: PHY config (link/auto-neg)
#define VERSIONR      0x0039
#define PHYCFG_RST_AN 0xB8 // RST(no)|OPMD|OPMDC=AllAuto = software-override auto-neg
#define PHY_LINK_UP   0x01 // PHYCFGR bit0: link present
#define Sn_MR         0x0000
#define Sn_CR         0x0001
#define Sn_SR         0x0003
#define Sn_RXBUF      0x001E
#define Sn_TXBUF      0x001F
#define Sn_TX_FSR     0x0020
#define Sn_TX_WR      0x0024
#define Sn_RX_RSR     0x0026
#define Sn_RX_RD      0x0028
#define CMD_OPEN      0x01
#define CMD_SEND      0x20
#define CMD_RECV      0x40
#define SOCK_MACRAW   0x42
#define MR_MACRAW_MF  0x84

static unsigned char s_abW5mac[6] = {0x02, 0xDC, 0x00, 0x00, 0x00, 0x01};
static int s_nBus = -1, s_nCs;

// ---- raw register access via dcspi ----------------------------------------------
static void W5Write(BYTE bsb, WORD addr, const BYTE *pbData, int nLen)
{
	BYTE abHdr[3];
	abHdr[0] = (BYTE)(addr >> 8);
	abHdr[1] = (BYTE)addr;
	abHdr[2] = CTRL(bsb, 1);
	W5LOCK();
	s_pSetCS(s_nBus, 1);
	s_pRw(s_nBus, abHdr, 0, 3);
	if (nLen)
		s_pRw(s_nBus, pbData, 0, nLen);
	s_pSetCS(s_nBus, 0);
	W5UNLOCK();
}
static void W5Read(BYTE bsb, WORD addr, BYTE *pbData, int nLen)
{
	BYTE abHdr[3];
	abHdr[0] = (BYTE)(addr >> 8);
	abHdr[1] = (BYTE)addr;
	abHdr[2] = CTRL(bsb, 0);
	W5LOCK();
	s_pSetCS(s_nBus, 1);
	s_pRw(s_nBus, abHdr, 0, 3);
	if (nLen)
		s_pRw(s_nBus, 0, pbData, nLen);
	s_pSetCS(s_nBus, 0);
	W5UNLOCK();
}
static void W5W1(BYTE bsb, WORD a, BYTE bVal)
{
	W5Write(bsb, a, &bVal, 1);
}
static BYTE W5R1(BYTE bsb, WORD a)
{
	BYTE bVal = 0;
	W5Read(bsb, a, &bVal, 1);
	return bVal;
}
static void W5W16(BYTE bsb, WORD a, WORD wVal)
{
	BYTE ab[2];
	ab[0] = (BYTE)(wVal >> 8);
	ab[1] = (BYTE)wVal;
	W5Write(bsb, a, ab, 2);
}
static WORD W5R16(BYTE bsb, WORD a)
{
	BYTE ab[2];
	W5Read(bsb, a, ab, 2);
	return (WORD)((ab[0] << 8) | ab[1]);
}
// Stable 16-bit read for the chip-updated pointers (Sn_TX_FSR / Sn_RX_RSR): the W5500's
// MAC engine updates these asynchronously, so a single SPI read can latch a high byte from
// before an update and a low byte from after (a torn value). Loop until two CONSECUTIVE
// reads agree (a safe 16-bit register read). The old code gave up after 4 tries and returned
// a possibly-unstable value -> bogus FSR/RSR -> TX ring overrun / RX ring desync on real HW
// (idealized models update atomically, so it never bit there). Bounded at 16 to never hang.
static WORD W5R16s(BYTE bsb, WORD a)
{
	WORD wX, wY = W5R16(bsb, a);
	int i;
	for (i = 0; i < 16; i++)
	{
		wX = wY;
		wY = W5R16(bsb, a);
		if (wX == wY)
			return wY;
	}
	return wY;
}

// ---- LinkOps ---------------------------------------------------------------------
static int W5Try(int nBus, int nCs)
{
	BYTE bVer;
	SysLog(L"w5500: probe bus=%d cs=%d", nBus,
	       nCs); // (SCI init drives Port A; SCIF reconfigs SCIF)
	if (s_pInit(nBus, nCs) != 0)
	{
		SysLog(L"w5500: SpiInit FAILED bus=%d", nBus);
		return 0;
	}
	s_nBus = nBus;
	s_nCs = nCs;
	bVer = W5R1(BSB_COMMON, VERSIONR);
	SysLog(L"w5500: VERSIONR=0x%02x bus=%d (expect 0x04)", bVer, nBus);
	if (bVer == 0x04)
	{
		SysLog(L"w5500: DETECTED on bus %d", nBus);
		DcBootSet(DCB_NET, DCB_OK, nBus == DCSPI_BUS_SCIF ? L"W5500 (SCIF)" : L"W5500 (SCI)");
		return 1;
	}
	if (s_pShutdown)
		s_pShutdown(nBus);
	s_nBus = -1;
	return 0;
}

// Read the configured bus. HKLM\Comm\Netif : "W5500Bus" (REG_DWORD):
//   0 / absent = disabled (safe default)
//   1 = SCI  (bus 1, hardware sync-SPI, CS on PA7 GPIO)
//   2 = SCIF (bus 2, bit-bang SPI on SCSPTR2, CS on RTS)
//   3 = AUTO (probe SCI first, then SCIF)
static DWORD CfgBus(void)
{
	HKEY hKey;
	DWORD dwVal = 0, dwType, dwSz = sizeof(dwVal);
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\Netif", 0, KEY_QUERY_VALUE, &hKey) ==
	    ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, L"W5500Bus", 0, &dwType, (BYTE *)&dwVal, &dwSz) != ERROR_SUCCESS)
			dwVal = 0;
		RegCloseKey(hKey);
	}
	return dwVal;
}

int W5500Probe(void)
{
	DWORD dwBus = CfgBus();
	SysLog(L"w5500: W5500Bus=%d (0=off 1=SCI 2=SCIF 3=auto)", dwBus);
	if (dwBus == 0)
		return 0; // not configured -> skip (safe default)
	if (!LoadDcspi())
	{
		SysLog(L"w5500: dcspi.dll load FAILED");
		return 0;
	}
	if (s_pHwType)
	{
		int nTy = s_pHwType(); // log board: Naomi uses SCIF-RTS CS on SCI
		SysLog(L"w5500: hw=%s type=0x%x",
		       nTy == 0xA   ? L"NAOMI"
		       : nTy == 0x9 ? L"Set5"
		                    : L"Dreamcast",
		       nTy);
	}
	// SCI CS source is AUTO: PA7 GPIO on retail DC, SCIF RTS on Naomi/Set5 (resolved in dcspi).
	if (dwBus == 1)
		return W5Try(DCSPI_BUS_SCI, DCSPI_CS_AUTO);
	if (dwBus == 2)
		return W5Try(DCSPI_BUS_SCIF, DCSPI_CS_RTS);
	if (dwBus == 3) // AUTO-detect which bus the chip is on
	{
		// SCI FIRST: probing SCI only touches the SCI + PA7/RTS, never the SCIF data pins, so the
		// nkscifkd debug console (which lives on the SCIF) survives if the chip is on SCI.
		SysLog(L"w5500: auto-probe SCI first...");
		if (W5Try(DCSPI_BUS_SCI, DCSPI_CS_AUTO))
			return 1;
		// SCIF LAST: scif_init_raw reconfigures the SCIF to GPIO/SPI, which TAKES OVER the
		// SCIF and kills the kernel text console. Only do this if SCI had no chip.
		SysLog(L"w5500: auto-probe SCIF (takes over SCIF console)...");
		if (W5Try(DCSPI_BUS_SCIF, DCSPI_CS_RTS))
			return 1;
		SysLog(L"w5500: not found on SCI or SCIF");
	}
	return 0;
}

int W5500Init(unsigned char mac[6])
{
	int t, i;
	if (s_nBus < 0)
		return 0;

	// Soft reset, then WAIT for MR.RST to clear with a real delay (PLL/PHY settle). The old
	// tight 1000-spin (no delay) could fall through before reset finished on real silicon and
	// then configure a chip mid-reset (writes lost). Bail if it never clears.
	W5W1(BSB_COMMON, MR, 0x80);
	for (t = 0; t < 100 && (W5R1(BSB_COMMON, MR) & 0x80); t++)
		Sleep(1);
	if (W5R1(BSB_COMMON, MR) & 0x80)
	{
		OutputDebugStringW(L"w5500: soft-reset stuck\r\n");
		return 0;
	}

	W5Write(BSB_COMMON, SHAR, s_abW5mac, 6); // our MAC

	// Force the PHY to software-override auto-negotiation. On real boards the PHY power-up
	// mode is pin-strapped; without this it may never link. (0x00 then 0xB8 per datasheet.)
	W5W1(BSB_COMMON, PHYCFGR, 0x00);
	Sleep(1);
	W5W1(BSB_COMMON, PHYCFGR, PHYCFG_RST_AN);

	// Zero ALL 8 sockets' RX/TX buffer sizes (power-on default is 2K each = 16K used) BEFORE
	// giving socket 0 the full 16K. Setting S0=16 without zeroing the rest requests 16+7*2=30K
	// > the chip's 16K -> overlapping buffers, corrupt MACRAW reads/writes (the audit found
	// this; masked by an idealized buffer model). Block N reg-block = 1 + 4*N.
	for (i = 0; i < 8; i++)
	{
		BYTE bBlk = (BYTE)(1 + 4 * i);
		W5W1(bBlk, Sn_RXBUF, 0);
		W5W1(bBlk, Sn_TXBUF, 0);
	}
	W5W1(BSB_S0_REG, Sn_RXBUF, 16);
	W5W1(BSB_S0_REG, Sn_TXBUF, 16);

	W5W1(BSB_S0_REG, Sn_MR, MR_MACRAW_MF); // MACRAW + MAC filter
	W5W1(BSB_S0_REG, Sn_CR, CMD_OPEN);
	for (t = 0; t < 1000 && W5R1(BSB_S0_REG, Sn_CR); t++)
		;
	if (W5R1(BSB_S0_REG, Sn_SR) != SOCK_MACRAW)
	{
		OutputDebugStringW(L"w5500: MACRAW open failed\r\n");
		return 0;
	}
	memcpy(mac, s_abW5mac, 6);

	// Best-effort: note the PHY link bit (don't block boot on it - the netif DHCP worker
	// retries, so the lease lands once the link comes up).
	{
		WCHAR ab[80];
		int nUp = (W5R1(BSB_COMMON, PHYCFGR) & PHY_LINK_UP) ? 1 : 0;
		wsprintfW(ab, L"w5500: up on bus %d link=%d MAC=%02x:%02x:%02x:%02x:%02x:%02x\r\n", s_nBus,
		          nUp, s_abW5mac[0], s_abW5mac[1], s_abW5mac[2], s_abW5mac[3], s_abW5mac[4],
		          s_abW5mac[5]);
		OutputDebugStringW(ab);
	}
	return 1;
}

int W5500Tx(const unsigned char *frame, int len)
{
	WORD wFsr, wWr;
	int t = 0;
	if (len <= 0 || len > 1514)
		return 0;
	do
	{
		wFsr = W5R16s(BSB_S0_REG, Sn_TX_FSR);
	} while (wFsr < (WORD)len && ++t < 20000);
	if (wFsr < (WORD)len)
		return 0;
	wWr = W5R16(BSB_S0_REG, Sn_TX_WR);
	W5Write(BSB_S0_TX, wWr, frame, len); // chip wraps the ring internally
	W5W16(BSB_S0_REG, Sn_TX_WR, (WORD)(wWr + len));
	W5W1(BSB_S0_REG, Sn_CR, CMD_SEND);
	for (t = 0; t < 100000 && W5R1(BSB_S0_REG, Sn_CR); t++)
		;
	if (W5R1(BSB_S0_REG, Sn_CR))
		return 0; // SEND never accepted -> report failure
	return 1;
}

int W5500RxPoll(unsigned char *buf, int max)
{
	WORD wRsr, wRd, wPlen;
	int nFlen;
	BYTE abHdr[2];
	wRsr = W5R16s(BSB_S0_REG, Sn_RX_RSR);
	if (wRsr < 2)
		return 0;
	if (wRsr > 12288) // >3/4 of the 16K RX buffer: we're draining too
	{                 // slowly (SCIF bit-bang SPI is the bottleneck)
		static int s_nFull;
		if ((++s_nFull & 0x1f) == 1)
			SysLog(L"w5500: RX backlog rsr=%u (SPI too slow to drain)", wRsr);
	}
	wRd = W5R16(BSB_S0_REG, Sn_RX_RD);
	W5Read(BSB_S0_RX, wRd, abHdr, 2); // MACRAW per-packet 2-byte length (incl header)
	wPlen = (WORD)((abHdr[0] << 8) | abHdr[1]);
	if (wPlen < 3 || wPlen > wRsr) // bad length - on SCIF this is usually a TORN
	{                              // read of the 2-byte header, not a real desync
		BYTE abH2[2];
		WORD wP2; // so re-read it once before discarding the ring
		W5Read(BSB_S0_RX, wRd, abH2, 2);
		wP2 = (WORD)((abH2[0] << 8) | abH2[1]);
		if (wP2 >= 3 && wP2 <= wRsr)
			wPlen = wP2; // re-read agrees + valid -> recovered, no data lost
		else             // genuine desync -> resync to write ptr (lose ring)
		{
			static int s_nDesync;
			if ((++s_nDesync & 0x1f) == 1)
				SysLog(L"w5500: RX desync #%d plen=%u rsr=%u", s_nDesync, wPlen, wRsr);
			W5W16(BSB_S0_REG, Sn_RX_RD, (WORD)(wRd + wRsr));
			W5W1(BSB_S0_REG, Sn_CR, CMD_RECV);
			return -1;
		}
	}
	nFlen = wPlen - 2;
	if (nFlen > max)
		nFlen = max;
	W5Read(BSB_S0_RX, (WORD)(wRd + 2), buf, nFlen);
	W5W16(BSB_S0_REG, Sn_RX_RD, (WORD)(wRd + wPlen)); // advance past the whole packet
	W5W1(BSB_S0_REG, Sn_CR, CMD_RECV);
	return nFlen;
}
