//
// bba.c - Dreamcast Broadband Adapter (HIT-0400, RTL8139C on the G2 bus) as a
// Dreamcast WinCE WDM driver. Provides NT-style DriverEntry, links wdm.lib (which
// supplies the DllMain that calls InitWDMDriver -> our DriverEntry), registered
// under HKLM\WDMDrivers\BuiltIn\BBA like maple.dll. NO own DllMain, no exports.
//
// Stage 2: GAPS bridge + RTL8139C init, MAC read, RX ring + TX descriptors, an RX
// worker thread that logs received frames, an ARP TX probe, and SEND/RECV IOCTLs.
// Polled (no interrupt yet). Hardware sequence for the broadband adapter
// (broadband_adapter.c, g2bus.c, fifo.h).
//
#include <windows.h>
#include <wdm.h>

DWORD SetKMode(DWORD dwMode); // coredll export, not in the SDK public headers

#ifndef CTL_CODE
#define CTL_CODE(t, f, m, a) (((t) << 16) | ((a) << 14) | ((f) << 2) | (m))
#define METHOD_BUFFERED      0
#define FILE_ANY_ACCESS      0
#endif
#define IOCTL_BBA_GET_MAC     CTL_CODE(FILE_DEVICE_NETWORK, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_GET_PRESENT CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_SEND        CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_RECV        CTL_CODE(FILE_DEVICE_NETWORK, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ---------------------------------------------------------------------------
// G2 bus access (P2 / uncached; drain the 32-byte G2 write FIFO before each).
// ---------------------------------------------------------------------------
#define G2_FIFO_STATUS (*(volatile DWORD *)0xA05F688C)
#define FIFO_AICA      0x0001
#define FIFO_G2        0x0010

static void G2FifoWait(void)
{
	int i;
	for (i = 0; i < 0x1800; i++)
		if (!(G2_FIFO_STATUS & (FIFO_AICA | FIFO_G2)))
			break;
}

static BYTE G2Read8(DWORD dwA)
{
	G2FifoWait();
	return *(volatile BYTE *)dwA;
}
static WORD G2Read16(DWORD dwA)
{
	G2FifoWait();
	return *(volatile WORD *)dwA;
}
static DWORD G2Read32(DWORD dwA)
{
	G2FifoWait();
	return *(volatile DWORD *)dwA;
}
static void G2Write8(DWORD dwA, BYTE bV)
{
	G2FifoWait();
	*(volatile BYTE *)dwA = bV;
}
static void G2Write16(DWORD dwA, WORD wV)
{
	G2FifoWait();
	*(volatile WORD *)dwA = wV;
}
static void G2Write32(DWORD dwA, DWORD dwV)
{
	G2FifoWait();
	*(volatile DWORD *)dwA = dwV;
}

static void G2ReadBlock(BYTE *pbDst, DWORD dwAddr, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if (!(i & 31))
			G2FifoWait();
		pbDst[i] = *(volatile BYTE *)(dwAddr + i);
	}
}
static void G2WriteBlock(DWORD dwAddr, const BYTE *pbSrc, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if (!(i & 31))
			G2FifoWait();
		*(volatile BYTE *)(dwAddr + i) = pbSrc[i];
	}
}

// ---------------------------------------------------------------------------
// GAPS bridge + RTL8139C. RTL_DMA = chip-side DMA
// offset (GAPS-relative); RTL_CPU = the same memory in the P2 window.
// ---------------------------------------------------------------------------
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
#define RT_RXBUFTAIL        0x38 // CAPR
#define RT_RXBUFHEAD        0x3a // CBR
#define RT_INTRMASK         0x3c
#define RT_INTRSTATUS       0x3e
#define RT_TXCONFIG         0x40
#define RT_RXCONFIG         0x44

#define RT_CMD_RESET        0x10
#define RT_CMD_RX_ENABLE    0x08
#define RT_CMD_TX_ENABLE    0x04
#define RT_CMD_RX_BUF_EMPTY 0x01
#define RT_TX_HOST_OWNS     0x00002000

#define RXC_RBLEN_16K       (1 << 11)
#define RXC_MXDMA_1K        (6 << 8)
#define RXC_WRAP            0x80
#define RXC_AB              0x08 // accept broadcast
#define RXC_APM             0x02 // accept physical-match
#define RX_CONFIG           (RXC_RBLEN_16K | RXC_MXDMA_1K | RXC_WRAP)
#define TX_CONFIG           (6 << 8)

#define RX_BUF_LEN          0x4000                // 16K (RBLEN=1)
#define TX_OFF              (RX_BUF_LEN + 0x2000) // 0x6000: RX(16K) + 8K wrap pad
#define TX_LEN              0x800                 // 2K per TX buffer
#define TX_N                4

static BYTE g_abMac[6];
static BOOL g_bPresent = FALSE;
static DWORD g_dwCurtx = 0, g_dwCurrx = 0;

static int GapsInit(void)
{
	char szStr[16];
	int i;
	G2ReadBlock((BYTE *)szStr, GAPS_BASE + 0x1400, 16);
	if (memcmp(szStr, "GAPSPCI_BRIDGE_2", 16) != 0)
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
	return 0;
}

static void RtlReset(void)
{
	int i;
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RESET);
	for (i = 1000; i > 0 && (G2Read8(NIC(RT_CHIPCMD)) & RT_CMD_RESET); i--)
		;
}

static void RtlReadMac(void)
{
	DWORD dwLo = G2Read32(NIC(RT_IDR0));
	DWORD dwHi = G2Read32(NIC(RT_IDR0 + 4));
	g_abMac[0] = (BYTE)dwLo;
	g_abMac[1] = (BYTE)(dwLo >> 8);
	g_abMac[2] = (BYTE)(dwLo >> 16);
	g_abMac[3] = (BYTE)(dwLo >> 24);
	g_abMac[4] = (BYTE)dwHi;
	g_abMac[5] = (BYTE)(dwHi >> 8);
}

// Bring up the RX ring + TX descriptors and enable RX/TX.
static void RtlStart(void)
{
	int i;
	G2Write32(NIC(RT_RXBUF), RTL_DMA);
	for (i = 0; i < TX_N; i++)
		G2Write32(NIC(RT_TXADDR0 + i * 4), RTL_DMA + i * TX_LEN + TX_OFF);
	G2Write16(NIC(RT_INTRMASK), 0); // polled: no interrupts
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
	G2Write32(NIC(RT_RXCONFIG), RX_CONFIG | RXC_APM | RXC_AB);
	G2Write32(NIC(RT_TXCONFIG), TX_CONFIG);
	G2Write8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
	G2Write32(NIC(RT_MAR0), 0xffffffff); // accept all multicast
	G2Write32(NIC(RT_MAR4), 0xffffffff);
	G2Write16(NIC(RT_INTRSTATUS), 0xffff); // clear any latched status
	g_dwCurtx = 0;
	g_dwCurrx = 0;
	G2Write16(NIC(RT_RXBUFTAIL), (WORD)(0 - 16));
}

// Transmit one frame (<=1514 bytes). Pads runts to 60. Polled OWN wait.
static BOOL BbaTx(const BYTE *pbPkt, int nLen)
{
	DWORD dwTsd = NIC(RT_TXSTATUS0 + 4 * g_dwCurtx);
	DWORD dwBuf = RTL_CPU + g_dwCurtx * TX_LEN + TX_OFF;
	int i;
	if (nLen <= 0 || nLen > 1514)
		return FALSE;
	for (i = 0; i < 1000 && !(G2Read32(dwTsd) & RT_TX_HOST_OWNS) && i == 0; i++)
		; // first use: OWN not yet meaningful
	G2WriteBlock(dwBuf, pbPkt, nLen);
	if (nLen < 60)
	{
		for (i = nLen; i < 60; i++)
			G2Write8(dwBuf + i, 0);
		nLen = 60;
	}
	G2Write32(dwTsd, (DWORD)nLen); // size + OWN=0 -> start TX
	g_dwCurtx = (g_dwCurtx + 1) & (TX_N - 1);
	return TRUE;
}

// Poll one received frame into pbBuf. Returns payload length, 0 if none, -1 on error.
static int BbaRxPoll(BYTE *pbBuf, int nMaxlen)
{
	DWORD dwStatus, dwOff;
	int nSize, nPkt;
	if (G2Read8(NIC(RT_CHIPCMD)) & RT_CMD_RX_BUF_EMPTY)
		return 0;
	dwOff = g_dwCurrx % RX_BUF_LEN;
	dwStatus = G2Read32(RTL_CPU + dwOff);
	nSize = (int)((dwStatus >> 16) & 0xffff);
	if (nSize == 0xfff0) // early-receive: not ready
		return 0;
	nPkt = nSize - 4; // drop the 4-byte CRC
	if (!(dwStatus & 1) || nPkt < 14 || nPkt > 1514)
	{
		RtlReset();
		RtlStart(); // ring desync -> re-init
		return -1;
	}
	if (nPkt > nMaxlen)
		nPkt = nMaxlen;
	G2ReadBlock(pbBuf, RTL_CPU + dwOff + 4, nPkt);
	g_dwCurrx = (g_dwCurrx + nSize + 4 + 3) & ~3;
	G2Write16(NIC(RT_RXBUFTAIL), (WORD)((g_dwCurrx - 16) & (RX_BUF_LEN - 1)));
	return nPkt;
}

// --- big-endian helper + IPv4 header checksum (for the DHCP self-test) ---
static void Be16(BYTE *pb, WORD wV)
{
	pb[0] = (BYTE)(wV >> 8);
	pb[1] = (BYTE)wV;
}
static WORD IpCsum(const BYTE *pb, int n)
{
	DWORD dwS = 0;
	int i;
	for (i = 0; i + 1 < n; i += 2)
		dwS += (DWORD)((pb[i] << 8) | pb[i + 1]);
	if (i < n)
		dwS += (DWORD)(pb[i] << 8);
	while (dwS >> 16)
		dwS = (dwS & 0xffff) + (dwS >> 16);
	return (WORD)~dwS;
}

// Build a broadcast DHCP DISCOVER (returns length). Subnet-agnostic, so it works
// in a built-in test BBA DHCP/NAT mode regardless of the host's subnet.
static int BuildDhcpDiscover(BYTE *pbOut, DWORD dwXid)
{
	BYTE *pbEth = pbOut, *pbIp = pbOut + 14, *pbUdp = pbOut + 34, *pbBp = pbOut + 42;
	int nDhcplen = 250, i;
	memset(pbOut, 0, 400);
	for (i = 0; i < 6; i++)
		pbEth[i] = 0xff; // dst broadcast
	for (i = 0; i < 6; i++)
		pbEth[6 + i] = g_abMac[i];
	Be16(pbEth + 12, 0x0800);
	pbBp[0] = 1;
	pbBp[1] = 1;
	pbBp[2] = 6; // op=BOOTREQUEST htype hlen
	pbBp[4] = (BYTE)(dwXid >> 24);
	pbBp[5] = (BYTE)(dwXid >> 16);
	pbBp[6] = (BYTE)(dwXid >> 8);
	pbBp[7] = (BYTE)dwXid;
	Be16(pbBp + 10, 0x8000); // flags = broadcast
	for (i = 0; i < 6; i++)
		pbBp[28 + i] = g_abMac[i]; // chaddr
	pbBp[236] = 0x63;
	pbBp[237] = 0x82;
	pbBp[238] = 0x53;
	pbBp[239] = 0x63; // magic cookie
	pbBp[240] = 53;
	pbBp[241] = 1;
	pbBp[242] = 1; // option 53: DHCP DISCOVER
	pbBp[243] = 55;
	pbBp[244] = 4;
	pbBp[245] = 1;
	pbBp[246] = 3;
	pbBp[247] = 6;
	pbBp[248] = 15;  // param req
	pbBp[249] = 255; // end
	Be16(pbUdp + 0, 68);
	Be16(pbUdp + 2, 67); // UDP src 68 -> dst 67
	Be16(pbUdp + 4, (WORD)(8 + nDhcplen));
	Be16(pbUdp + 6, 0); // len, csum 0 (legal for IPv4)
	pbIp[0] = 0x45;
	Be16(pbIp + 2, (WORD)(20 + 8 + nDhcplen)); // ver/ihl, total len
	pbIp[8] = 128;
	pbIp[9] = 17; // ttl, proto UDP
	for (i = 0; i < 4; i++)
		pbIp[16 + i] = 0xff; // dst 255.255.255.255 (src stays 0)
	Be16(pbIp + 10, IpCsum(pbIp, 20));
	return 14 + 20 + 8 + nDhcplen;
}

// If frame is a DHCP reply (UDP->68, BOOTREPLY), return yiaddr ("your" IP), else 0.
static DWORD ParseDhcpOffer(const BYTE *pbF, int n)
{
	if (n < 62)
		return 0;
	if (pbF[12] != 0x08 || pbF[13] != 0x00)
		return 0; // IPv4
	if (pbF[23] != 17)
		return 0; // UDP
	if (pbF[36] != 0 || pbF[37] != 68)
		return 0; // UDP dst port 68
	if (pbF[42] != 2)
		return 0; // BOOTREPLY
	return ((DWORD)pbF[58] << 24) | ((DWORD)pbF[59] << 16) | ((DWORD)pbF[60] << 8) |
	       pbF[61]; // yiaddr
}

// RX worker: poll the ring, log the first frames seen (proof of RX on the test's
// bridged network), and periodically send an ARP probe (proof of TX).
static DWORD WINAPI BbaRxThread(LPVOID pvParam)
{
	static BYTE abFrame[1600];
	static BYTE abDisc[400];
	int nLogged = 0, nPoll = 0, nDlen, nGotip = 0;
	DWORD dwTotal = 0;

	SetKMode(TRUE);
	nDlen = BuildDhcpDiscover(abDisc, 0xDC0DC0DCu);

	for (;;)
	{
		int n = BbaRxPoll(abFrame, sizeof(abFrame));
		if (n > 14)
		{
			DWORD dwYi;
			dwTotal++;
			if (nLogged < 8)
			{
				WCHAR ab[112];
				wsprintfW(ab, L"BBA RX[%u]: len=%d src=%02x:%02x:%02x:%02x:%02x:%02x type=%04x\r\n",
				          (unsigned)dwTotal, n, abFrame[6], abFrame[7], abFrame[8], abFrame[9],
				          abFrame[10], abFrame[11], (abFrame[12] << 8) | abFrame[13]);
				OutputDebugStringW(ab);
				if (++nLogged == 8)
					OutputDebugStringW(L"BBA RX: (further frames counted silently)\r\n");
			}
			dwYi = ParseDhcpOffer(abFrame, n);
			if (dwYi && !nGotip)
			{
				WCHAR ab[72];
				wsprintfW(ab, L"BBA: *** DHCP reply - your IP = %u.%u.%u.%u ***\r\n",
				          (unsigned)((dwYi >> 24) & 0xff), (unsigned)((dwYi >> 16) & 0xff),
				          (unsigned)((dwYi >> 8) & 0xff), (unsigned)(dwYi & 0xff));
				OutputDebugStringW(ab);
				nGotip = 1; // round-trip proven; stop probing
			}
		}
		else
		{
			if (!nGotip && (nPoll++ % 200) == 0) // ~every 2s until we get a reply
			{
				BbaTx(abDisc, nDlen);
				OutputDebugStringW(L"BBA TX: DHCP DISCOVER\r\n");
			}
			Sleep(10);
		}
	}
	return 0;
}

static BOOL BbaBringup(void)
{
	DWORD dwPrev = SetKMode(TRUE);
	BOOL bOk = FALSE;
	WCHAR ab[80];
	__try
	{
		if (GapsInit() == 0)
		{
			RtlReset();
			RtlReadMac();
			RtlStart();
			bOk = TRUE;
			wsprintfW(ab, L"BBA: up, MAC %02x:%02x:%02x:%02x:%02x:%02x\r\n", g_abMac[0], g_abMac[1],
			          g_abMac[2], g_abMac[3], g_abMac[4], g_abMac[5]);
			OutputDebugStringW(ab);
		}
		else
			OutputDebugStringW(L"BBA: no GAPS bridge / init failed (adapter present?)\r\n");
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		OutputDebugStringW(L"BBA: probe faulted (G2 access) - skipping\r\n");
	}
	SetKMode(dwPrev);
	return bOk;
}

// ---------------------------------------------------------------------------
// WDM dispatch (buffered I/O).
// ---------------------------------------------------------------------------
static NTSTATUS BbaDispatch(PDEVICE_OBJECT pDev, PIRP pIrp)
{
	PIO_STACK_LOCATION pSp = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS nSt = STATUS_SUCCESS;
	ULONG nInfo = 0;

	if (pSp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		ULONG nCode = pSp->Parameters.DeviceIoControl.IoControlCode;
		ULONG nInlen = pSp->Parameters.DeviceIoControl.InputBufferLength;
		ULONG nOutlen = pSp->Parameters.DeviceIoControl.OutputBufferLength;
		PVOID pvBuf = pIrp->AssociatedIrp.SystemBuffer;
		DWORD dwPrev;

		if (nCode == IOCTL_BBA_GET_MAC && pvBuf && nOutlen >= 6)
		{
			memcpy(pvBuf, g_abMac, 6);
			nInfo = 6;
		}
		else if (nCode == IOCTL_BBA_GET_PRESENT && pvBuf && nOutlen >= 4)
		{
			*(DWORD *)pvBuf = g_bPresent ? 1 : 0;
			nInfo = 4;
		}
		else if (nCode == IOCTL_BBA_SEND && pvBuf && nInlen >= 14 && g_bPresent)
		{
			dwPrev = SetKMode(TRUE);
			nSt = BbaTx((BYTE *)pvBuf, (int)nInlen) ? STATUS_SUCCESS : STATUS_IO_DEVICE_ERROR;
			SetKMode(dwPrev);
		}
		else if (nCode == IOCTL_BBA_RECV && pvBuf && nOutlen >= 14 && g_bPresent)
		{
			int n;
			dwPrev = SetKMode(TRUE);
			n = BbaRxPoll((BYTE *)pvBuf, (int)nOutlen);
			SetKMode(dwPrev);
			nInfo = (n > 0) ? (ULONG)n : 0;
		}
		else
			nSt = STATUS_NOT_IMPLEMENTED;
	}

	pIrp->IoStatus.Status = nSt;
	pIrp->IoStatus.Information = nInfo;
	IoCompleteRequest(pIrp, 0);
	return nSt;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrv, PUNICODE_STRING pRegpath)
{
	UNICODE_STRING name;
	PDEVICE_OBJECT pDev = NULL;
	NTSTATUS nSt;
	int i;

	OutputDebugStringW(L"BBA DriverEntry\r\n");
	g_bPresent = BbaBringup();

	RtlInitUnicodeString(&name, L"\\Device\\BBA1");
	nSt = IoCreateDevice(pDrv, 0, &name, FILE_DEVICE_NETWORK, 0, FALSE, &pDev);
	if (!NT_SUCCESS(nSt))
	{
		OutputDebugStringW(L"BBA: IoCreateDevice failed\r\n");
		return nSt;
	}

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		pDrv->MajorFunction[i] = BbaDispatch;

	if (g_bPresent)
	{
		HANDLE h = CreateThread(NULL, 0, BbaRxThread, NULL, 0, NULL);
		if (h)
			CloseHandle(h);
		OutputDebugStringW(L"BBA: RX worker started\r\n");
	}
	OutputDebugStringW(L"BBA: device created (DriverEntry ok)\r\n");
	return STATUS_SUCCESS;
}

// No DllMain: wdm.lib provides it (it calls InitWDMDriver -> DriverEntry).
