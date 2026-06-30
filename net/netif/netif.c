//
// netif.c - universal microstk link adapter (drop-in "mppp.dll" replacement).
//
// microstk binds its one link via LoadLibraryW("mppp.dll") + InterfaceInitialize.
// We replace mppp.dll with this adapter, which at init runs a DETECT LADDER and
// binds the first link present:  W5500 -> BBA -> modem -> (none/loopback).
// Each backend is a LinkOps (probe/init/tx/poll/getmac). Ethernet backends (BBA,
// W5500) own ARP + framing + DHCP here, because microstk speaks only raw IP.
//
// Exports (must match the SDK mppp.dll ordinals - see netif.def):
//   15 InterfaceInitialize  - the link adapter (this file)
//   14 FCSTable             - PPP FCS table (dummy for ethernet)
//   1..13 AfdRas*           - RAS backend; stubbed to report "connected" so dial-
//                             based titles proceed (the link is up via DHCP).  [ras.c]
//
// The link contract (ifnet / NDIS / TX / RX) is in microstk_if.h, extracted from
// mppp.pdb. The BBA hardware (RTL8139 RX/TX rings + DHCP) is reused from the
// drivers/bba work via the Bba* hooks below.
//
#include "microstk_if.h"
#include "syslog.h"
#include "dcboot.h"
#include <ras.h> // RASENTRY (the dial-config DNS the user set lives in ipaddrDns)

// ---- link backend abstraction --------------------------------------------------
typedef struct
{
	const char *name;
	int (*probe)(void);                                // 1 if the adapter is present
	int (*init)(unsigned char mac[6]);                 // bring HW up, return MAC
	int (*tx)(const unsigned char *pbFrame, int nLen); // send a raw ethernet frame
	int (*poll)(unsigned char *pbBuf, int nMax);       // get one rx frame, return len or 0
} LinkOps;

// BBA backend (RTL8139 on the G2 bus) - hardware reused from drivers/bba/bba.c.
extern int BbaProbe(void); // GAPS magic 0x5a14a501 present?
extern int BbaInit(unsigned char mac[6]);
extern int BbaTx(const unsigned char *frame, int len);
extern int BbaRxPoll(unsigned char *buf, int max);
static LinkOps s_bba = {"BBA", BbaProbe, BbaInit, BbaTx, BbaRxPoll};

// W5500 backend (MACRAW over dcspi.dll: SCI hardware-SPI / SCIF bit-bang) - see w5500.c.
extern int W5500Probe(void);
extern int W5500Init(unsigned char mac[6]);
extern int W5500Tx(const unsigned char *frame, int len);
extern int W5500RxPoll(unsigned char *buf, int max);
static LinkOps s_w5500 = {"W5500", W5500Probe, W5500Init, W5500Tx, W5500RxPoll};

// Modem backend: rather than re-implement PPP, DELEGATE to the original SDK dial-up driver
// (mpppdial.dll = the stock mppp.dll, vendored under a non-recursive name). When no ethernet
// adapter is present we LoadLibrary it and forward InterfaceInitialize + the 13 AfdRas* exports;
// it does the whole serial(COM6)+AT-dial+LCP/PAP-CHAP/IPCP+HDLC path (config from HKLM\Modem\
// Sega-DreamcastBuiltIn). g_nUseDial/g_apfnDialRas are read by ras.c to forward the RAS exports
// too.
int g_nUseDial = 0;        // 1 = modem path: forward everything to mpppdial
FARPROC g_apfnDialRas[14]; // [1..13] = original AfdRas* by ordinal
static int (*s_pfnDialIfInit)(unsigned short *, ifnet **);

static int LoadDialDriver(void)
{
	HINSTANCE hInst;
	int i;
	hInst = LoadLibraryW(L"mpppdial.dll");
	if (!hInst)
	{
		SysLog(L"netif: mpppdial.dll load FAILED");
		return 0;
	}
	s_pfnDialIfInit = (int (*)(unsigned short *, ifnet **))GetProcAddress(
	    hInst, (LPCWSTR)15); // InterfaceInitialize
	for (i = 1; i <= 13; i++)
		g_apfnDialRas[i] = (FARPROC)GetProcAddress(hInst, (LPCWSTR)(DWORD)i);
	if (!s_pfnDialIfInit)
	{
		SysLog(L"netif: mpppdial InterfaceInitialize missing");
		return 0;
	}
	SysLog(L"netif: no ethernet -> delegating modem/PPP to original mpppdial.dll");
	return 1;
}

// Null backend - no NIC present. A no-op link so microstk STILL gets a valid ifnet and
// initialises (the stack comes up with just loopback); without it InterfaceInitialize
// returns 0, microstk wedges on a NULL ifnet, and the whole boot hangs before the shell.
// The interface stays DOWN (no IP); enable a real link to actually network.
static int NullInit(unsigned char mac[6])
{
	mac[0] = 0x02;
	mac[1] = 0xDC;
	mac[2] = mac[3] = mac[4] = 0;
	mac[5] = 0xFF;
	return 1;
}
static int NullTx(const unsigned char *pbFrame, int nLen)
{
	(void)pbFrame;
	(void)nLen;
	return 0;
} // drop
static int NullRxPoll(unsigned char *pbBuf, int nMax)
{
	(void)pbBuf;
	(void)nMax;
	return 0;
} // nothing
static LinkOps s_null = {"NONE", 0, NullInit, NullTx, NullRxPoll};

static LinkOps *s_pLink; // the chosen backend
static ifnet *s_pIfn;    // our interface (microstk filled the callbacks)
static unsigned char s_abMac[6];
static int s_bHaveIP;
static void DhcpRx(const unsigned char *pbFrame, int nLen); // fwd: raw DHCP reply handler

// ---- tiny ARP cache (ethernet backends) ----------------------------------------
#define ARP_N 16
static struct
{
	ulong ip;
	unsigned char mac[6];
	int used;
} s_arp[ARP_N];
static ulong s_dwMyIP, s_dwMask, s_dwGw; // kept in network-order-as-LE (wire/ARP rep)
static ulong s_adwOffDns[5];             // DHCP option-6 DNS servers (network order)
static int s_nOffDnsN;
static int s_nTxLog, s_nRxLog;         // limit TX/RX diagnostics
static DWORD s_dwRxBytes, s_dwTxBytes; // cumulative link RX (delivered) / TX (microstk produced)
static DWORD s_dwStatTick, s_dwRxPrev,
    s_dwTxPrev; // periodic throughput log (spot where a stream stalls)

// ---- revival / game-server redirection (mirrors Flycast picoppp + DreamPi) ---------
// Online DC games reach now-dead first-party master servers two ways, so we redirect both:
//  (A) by HOSTNAME - hand the guest a revival DNS as PRIMARY so the game's master host resolves
//      to a current revival IP. DreamPi (dnsmasq -> 46.101.91.123) and Flycast (config::DNS
//      default 46.101.91.123 == dns.flyca.st) both do this; it covers most games.
//  (B) by HARDCODED IP - a few games dial a literal dead IP (no DNS). We DNAT those on the way
//      out and restore the original src on the way in (Flycast special-cases the same two).
// Both configurable in HKLM\Comm\Netif ("RevivalDns"/"RevivalMaster", dotted strings; "0.0.0.0"
// disables). RevivalDns defaults to dns.flyca.st (178.156.255.64), the current Flycast/DCNet
// revival resolver - NOT the stale legacy 46.101.91.123, which fails to resolve live revival hosts.
#define AFO_ORIG_IP 0x83f2fb3fUL         // 63.251.242.131  Alien Front Online  (wire bytes in mem)
#define IGP_ORIG_IP 0xef2bd2ccUL         // 204.210.43.239  Internet Game Pack
static ulong s_dwRevivalDns;             // primary DNS to prepend (0 = off)
static ulong s_dwRevivalMaster;          // DNAT target for the hardcoded-IP games (0 = off)
static ulong s_dwNatOrig, s_dwNatTarget; // active 1-entry reverse-NAT (one online game at a time)
static int s_bRevivalRead;               // config read yet?
static void TcpFixChecksum(unsigned char *pbIp); // used by both RxFrame (above) and OurTransmit
static void IpFixChecksum(unsigned char *pbIp);
static void UdpFixChecksum(unsigned char *pbIp);
static ulong RedirectDest(ulong dwDst);

static int ArpLookup(ulong dwIp, unsigned char abMac[6])
{
	int i;
	for (i = 0; i < ARP_N; i++)
		if (s_arp[i].used && s_arp[i].ip == dwIp)
		{
			memcpy(abMac, s_arp[i].mac, 6);
			return 1;
		}
	return 0;
}
static void ArpInsert(ulong dwIp, const unsigned char *pbMac)
{
	int i, nSlot = 0;
	for (i = 0; i < ARP_N; i++)
	{
		if (s_arp[i].used && s_arp[i].ip == dwIp)
		{
			nSlot = i;
			break;
		}
		if (!s_arp[i].used)
			nSlot = i;
	}
	s_arp[nSlot].ip = dwIp;
	memcpy(s_arp[nSlot].mac, pbMac, 6);
	s_arp[nSlot].used = 1;
}

static void EthSend(const unsigned char abDst[6], ushort wType, const unsigned char *pbPayload,
                    int nLen)
{
	unsigned char abFrame[1600];
	if (nLen > 1500)
		return;
	memcpy(abFrame, abDst, 6);
	memcpy(abFrame + 6, s_abMac, 6);
	abFrame[12] = (unsigned char)(wType >> 8);
	abFrame[13] = (unsigned char)wType;
	memcpy(abFrame + 14, pbPayload, nLen);
	s_pLink->tx(abFrame, nLen + 14);
}

static void ArpRequest(ulong dwIp)
{
	unsigned char abArp[28], abBcast[6];
	memset(abBcast, 0xff, 6);
	abArp[0] = 0;
	abArp[1] = 1;
	abArp[2] = 8;
	abArp[3] = 0;
	abArp[4] = 6;
	abArp[5] = 4;
	abArp[6] = 0;
	abArp[7] = 1; // eth/ip, req
	memcpy(abArp + 8, s_abMac, 6);
	memcpy(abArp + 14, &s_dwMyIP, 4);
	memset(abArp + 18, 0, 6);
	memcpy(abArp + 24, &dwIp, 4);
	EthSend(abBcast, 0x0806, abArp, 28);
}

// Skip one DNS name (labels until a 0 length byte, or a 0xC0 compression pointer).
static const unsigned char *DnsSkipName(const unsigned char *pb, const unsigned char *pbEnd)
{
	while (pb < pbEnd)
	{
		if ((*pb & 0xC0) == 0xC0)
			return pb + 2; // compression pointer (2 bytes)
		if (*pb == 0)
			return pb + 1; // root label = end of name
		pb += *pb + 1;     // ordinary label
	}
	return pbEnd;
}

// 16-bit ones-complement checksum over a network-order byte range (folded), with a running seed.
static unsigned short Csum16(const unsigned char *pb, int nLen, unsigned long dwSeed)
{
	unsigned long dwSum = dwSeed;
	int i;
	for (i = 0; i + 1 < nLen; i += 2)
		dwSum += (unsigned long)((pb[i] << 8) | pb[i + 1]);
	if (nLen & 1)
		dwSum += (unsigned long)(pb[nLen - 1] << 8);
	while (dwSum >> 16)
		dwSum = (dwSum & 0xFFFF) + (dwSum >> 16);
	return (unsigned short)(~dwSum & 0xFFFF);
}

// Flatten a DNS reply so coredll's broken DnsQueryAddress (which takes answer[0].RDATA without
// checking TYPE) returns the right address. We do exactly what KOS's getaddrinfo RR-walk does -
// scan every answer RR, pick the TYPE==A record, skip CNAME(5)/others - then rebuild the packet
// with a SINGLE A answer named via a compression pointer to the question. This mirrors what
// Flycast achieves by handing the guest a flattening DNS server (dns.flyca.st), done in-flight.
// Returns the new IP-datagram length in `out` (>0) when it rewrote, else 0 (deliver original).
// Only rewrites CNAME-bearing replies that DO carry an A - plain A-only replies pass through.
static int DnsFlatten(const unsigned char *pbIp, int nIpLen, unsigned char *pbOut)
{
	int ihl = (pbIp[0] & 0x0f) * 4;
	const unsigned char *udp = pbIp + ihl, *dns = udp + 8, *pbEnd = pbIp + nIpLen, *p;
	int qd, an, i, nQLen, nDnsLen, nUdpLen, nTotal, bHaveA = 0, bCname = 0;
	unsigned char abAip[4] = {0, 0, 0, 0};
	unsigned char *od, *a;
	if (dns + 12 > pbEnd)
		return 0;
	qd = (dns[4] << 8) | dns[5]; // QDCOUNT
	an = (dns[6] << 8) | dns[7]; // ANCOUNT
	p = dns + 12;
	for (i = 0; i < qd && p < pbEnd; i++)
	{
		p = DnsSkipName(p, pbEnd);
		p += 4;
	} // skip questions
	nQLen = (int)(p - (dns + 12));        // question-section length (to copy verbatim)
	for (i = 0; i < an && p < pbEnd; i++) // KOS-style answer walk
	{
		int ty, rdl;
		p = DnsSkipName(p, pbEnd);
		if (p + 10 > pbEnd)
			break;
		ty = (p[0] << 8) | p[1];  // TYPE (1=A, 5=CNAME, 28=AAAA, ...)
		rdl = (p[8] << 8) | p[9]; // RDLENGTH
		if (ty == 5)
			bCname = 1;
		if (ty == 1 && rdl == 4 && p + 14 <= pbEnd && !bHaveA)
		{
			memcpy(abAip, p + 10, 4);
			bHaveA = 1;
		}
		p += 10 + rdl;
	}
	SysLog(L"dns: an=%d cname=%d haveA=%d ip=%u.%u.%u.%u", an, bCname, bHaveA, abAip[0], abAip[1],
	       abAip[2], abAip[3]);
	if (!bCname || !bHaveA)
		return 0; // only fix CNAME-confused replies that carry an A

	memcpy(pbOut, pbIp, ihl);    // IP header (length+csum fixed below)
	memcpy(pbOut + ihl, udp, 8); // UDP header (length+csum fixed below)
	od = pbOut + ihl + 8;
	memcpy(od, dns, 12); // DNS header
	od[6] = 0;
	od[7] = 1;                           // ANCOUNT = 1
	od[8] = od[9] = od[10] = od[11] = 0; // NSCOUNT = ARCOUNT = 0
	memcpy(od + 12, dns + 12, nQLen);    // copy question verbatim
	a = od + 12 + nQLen;
	a[0] = 0xC0;
	a[1] = 0x0C; // NAME -> pointer to question name (offset 12)
	a[2] = 0x00;
	a[3] = 0x01; // TYPE  = A
	a[4] = 0x00;
	a[5] = 0x01; // CLASS = IN
	a[6] = a[7] = a[8] = 0;
	a[9] = 60; // TTL   = 60s
	a[10] = 0x00;
	a[11] = 0x04;             // RDLENGTH = 4
	memcpy(a + 12, abAip, 4); // RDATA = the A address
	nDnsLen = 12 + nQLen + 16;
	nUdpLen = 8 + nDnsLen;
	pbOut[ihl + 4] = (unsigned char)(nUdpLen >> 8);
	pbOut[ihl + 5] = (unsigned char)nUdpLen;
	pbOut[ihl + 6] = pbOut[ihl + 7] = 0; // zero UDP checksum before recompute
	nTotal = ihl + nUdpLen;
	pbOut[2] = (unsigned char)(nTotal >> 8);
	pbOut[3] = (unsigned char)nTotal; // IP total length
	pbOut[10] = pbOut[11] = 0;        // zero IP header checksum before recompute
	{
		unsigned short c = Csum16(pbOut, ihl, 0);
		pbOut[10] = (unsigned char)(c >> 8);
		pbOut[11] = (unsigned char)c;
	}
	{ // UDP checksum over the pseudo-header (src+dst IP, proto=17, nUdpLen) + UDP segment
		unsigned long dwSeed = 17 + nUdpLen;
		unsigned short c;
		for (i = 12; i < 20; i += 2)
			dwSeed += (pbOut[i] << 8) | pbOut[i + 1]; // src + dst IP
		c = Csum16(pbOut + ihl, nUdpLen, dwSeed);
		if (c == 0)
			c = 0xFFFF;
		pbOut[ihl + 6] = (unsigned char)(c >> 8);
		pbOut[ihl + 7] = (unsigned char)c;
	}
	return nTotal;
}

// Build an NDIS packet for an inbound IP datagram and hand it to microstk (the exact
// sequence from mppp's Receive @ 1000ab5c).
static void DeliverIP(const unsigned char *pbIp, int nLen)
{
	NDIS_PACKET *pPkt;
	NDIS_BUFFER *pBuf, *pLast;
	unsigned char *pbMem = 0;
	if (!s_pIfn || !s_pIfn->ifn_AllocatePacket || !s_pIfn->ifn_IPInput)
		return;
	pPkt = s_pIfn->ifn_AllocatePacket();
	if (!pPkt)
		return;
	pBuf = s_pIfn->ifn_AllocateBufferWithMemory((ulong)nLen, &pbMem);
	if (!pBuf || !pbMem)
	{
		s_pIfn->ifn_FreePacket(pPkt);
		return;
	}
	memcpy(pbMem, pbIp, nLen);
	for (pLast = pBuf; pLast->Next; pLast = pLast->Next)
		;
	if (pPkt->Private.Head == 0)
		pPkt->Private.Tail = pLast;
	pLast->Next = pPkt->Private.Head;
	pPkt->Private.Head = pBuf;
	pPkt->Private.ValidCounts = 0;
	s_pIfn->ifn_IPInput(s_pIfn, pPkt);
}

// Handle one raw ethernet frame from the link (ARP ourselves, IP -> microstk).
static void RxFrame(const unsigned char *pbFrame, int nLen)
{
	ushort wType;
	if (nLen < 14)
		return;
	wType = (ushort)((pbFrame[12] << 8) | pbFrame[13]);
	if (wType == 0x0806 && nLen >= 42) // ARP
	{
		const unsigned char *a = pbFrame + 14;
		ulong dwSip;
		memcpy(&dwSip, a + 14, 4);
		ArpInsert(dwSip, a + 8);    // cache sender
		if (a[6] == 0 && a[7] == 1) // request -> reply if for us
		{
			ulong dwTip;
			memcpy(&dwTip, a + 24, 4);
			if (dwTip == s_dwMyIP && s_dwMyIP)
			{
				unsigned char abReply[28];
				memcpy(abReply, a, 28);
				abReply[7] = 2; // opcode = reply
				memcpy(abReply + 8, s_abMac, 6);
				memcpy(abReply + 14, &s_dwMyIP, 4);
				memcpy(abReply + 18, a + 8, 6);
				memcpy(abReply + 24, a + 14, 4);
				EthSend(pbFrame + 6, 0x0806, abReply, 28);
			}
		}
	}
	else if (wType == 0x0800) // IPv4
	{
		const unsigned char *ip = pbFrame + 14;
		int ihl = (ip[0] & 0x0f) * 4; // IP header length (IHL*4); NOT always 20
		const unsigned char *udp;     // UDP starts after the (variable) IP header
		unsigned char abNatBuf[1600];
		// (B) reverse the game-master DNAT: replies arrive FROM the revival IP, but the guest's
		// stack expects them from the original (dead) IP it dialled. Restore src into a copy.
		if (s_dwNatTarget && nLen - 14 >= 20 && nLen - 14 <= (int)sizeof(abNatBuf))
		{
			ulong dwSip;
			memcpy(&dwSip, ip + 12, 4);
			if (dwSip == s_dwNatTarget)
			{
				memcpy(abNatBuf, ip, nLen - 14);
				memcpy(abNatBuf + 12, &s_dwNatOrig, 4); // src: revival -> original
				IpFixChecksum(abNatBuf);
				if (abNatBuf[9] == 6)
					TcpFixChecksum(abNatBuf);
				else if (abNatBuf[9] == 17)
					UdpFixChecksum(abNatBuf);
				ip = abNatBuf;
			}
		}
		udp = ip + ihl;
		if (ip[9] == 17 && nLen >= 14 + ihl + 8 && udp[2] == 0 &&
		    udp[3] == 68) // UDP dest port 68 = DHCP reply
			DhcpRx(pbFrame, nLen);
		else
		{
			if (s_nRxLog < 8)
			{
				WCHAR w[96];
				wsprintfW(w, L"netif RX[%d]: src=%u.%u.%u.%u proto=%u len=%d\r\n", s_nRxLog, ip[12],
				          ip[13], ip[14], ip[15], ip[9], nLen - 14);
				OutputDebugStringW(w);
				s_nRxLog++;
			}
			if (ip[9] == 6) // TCP: log every inbound handshake/control segment
			{
				const unsigned char *tcp = ip + ihl;
				BYTE bFlags = tcp[13];
				if (bFlags & 0x07) // SYN(-ACK=0x12) / FIN / RST(0x04)
					SysLog(L"tcp RX %u.%u.%u.%u:%u fl=%02x", ip[12], ip[13], ip[14], ip[15],
					       (tcp[0] << 8) | tcp[1], bFlags);
			}
			// DNS reply (UDP src port 53)? Flatten CNAME chains so coredll's resolver returns the
			// real A record (see DnsFlatten). Otherwise hand the datagram up unchanged.
			if (ip[9] == 17 && nLen >= 14 + ihl + 8 && udp[0] == 0 && udp[1] == 53)
			{
				unsigned char abFlat[600];
				int nFlatLen = DnsFlatten(ip, nLen - 14, abFlat);
				if (nFlatLen > 0)
				{
					DeliverIP(abFlat, nFlatLen);
					return;
				}
			}
			DeliverIP(ip, nLen - 14); // everything else -> microstk
		}
	}
}

// Recompute the TCP checksum over the pseudo-header (src/dst IP, proto 6, TCP length) + segment,
// after we normalize a SYN's flags. Reuses Csum16 (the same routine the DNS-flatten path uses).
static void TcpFixChecksum(unsigned char *pbIp)
{
	int ihl = (pbIp[0] & 0x0f) * 4;
	int nTcpLen = ((pbIp[2] << 8) | pbIp[3]) - ihl;
	unsigned char *tcp = pbIp + ihl;
	unsigned long dwSeed = 6 + (unsigned long)nTcpLen;
	unsigned short c;
	int i;
	if (nTcpLen < 20)
		return;
	tcp[16] = tcp[17] = 0;
	for (i = 12; i < 20; i += 2)
		dwSeed += (unsigned long)((pbIp[i] << 8) | pbIp[i + 1]); // src + dst IP
	c = Csum16(tcp, nTcpLen, dwSeed);
	tcp[16] = (unsigned char)(c >> 8);
	tcp[17] = (unsigned char)c;
}

// ---- microstk TX/Ioctl (the ifnet vtable) --------------------------------------
// ifn_IPOutput: gather the NDIS buffer chain into one IP datagram, resolve the
// next hop (ARP), prepend ethernet, send; ALWAYS FreePacket; return 0 = ok.
static ulong OurTransmit(ifnet *ifn, NDIS_PACKET *pkt, ulong nextHop)
{
	unsigned char abIpBuf[1600], abMac[6];
	NDIS_BUFFER *b;
	int nOff = 0, bHit;
	ulong dwDst;
	(void)nextHop; // microstk has no routes; use the IP header dest
	for (b = pkt->Private.Head; b && nOff + (int)b->BufferLength <= (int)sizeof(abIpBuf);
	     b = b->Next)
	{
		memcpy(abIpBuf + nOff, b->VirtualAddress, b->BufferLength);
		nOff += (int)b->BufferLength;
	}
	s_dwTxBytes += (DWORD)nOff; // microstk's TX intent (ACKs/data) - even if dropped below

	memcpy(&dwDst, abIpBuf + 16, 4); // IP dest (network order = our s_dw* rep)
	{                                // (B) game-master DNAT: dead hardcoded IP -> revival
		ulong dwNewDst = RedirectDest(dwDst);
		if (dwNewDst != dwDst)
		{
			memcpy(abIpBuf + 16, &dwNewDst, 4);
			IpFixChecksum(abIpBuf);
			if (abIpBuf[9] == 6)
				TcpFixChecksum(abIpBuf);
			else if (abIpBuf[9] == 17)
				UdpFixChecksum(abIpBuf);
			SysLog(L"netif: game master %u.%u.%u.%u -> revival %u.%u.%u.%u", ((BYTE *)&dwDst)[0],
			       ((BYTE *)&dwDst)[1], ((BYTE *)&dwDst)[2], ((BYTE *)&dwDst)[3],
			       ((BYTE *)&dwNewDst)[0], ((BYTE *)&dwNewDst)[1], ((BYTE *)&dwNewDst)[2],
			       ((BYTE *)&dwNewDst)[3]);
			dwDst = dwNewDst;
		}
	}
	if (s_dwMask && (dwDst & s_dwMask) != (s_dwMyIP & s_dwMask))
		dwDst = s_dwGw; // off-link -> gateway

	bHit = ArpLookup(dwDst, abMac);
	if (s_nTxLog < 6)
	{
		WCHAR w[96];
		ulong d;
		memcpy(&d, abIpBuf + 16, 4);
		wsprintfW(w, L"netif TX[%d]: dst=%u.%u.%u.%u via=%u.%u.%u.%u %s\r\n", s_nTxLog, d & 0xff,
		          (d >> 8) & 0xff, (d >> 16) & 0xff, (d >> 24) & 0xff, dwDst & 0xff,
		          (dwDst >> 8) & 0xff, (dwDst >> 16) & 0xff, (dwDst >> 24) & 0xff,
		          bHit ? L"(arp hit)" : L"(arp miss->req)");
		OutputDebugStringW(w);
		s_nTxLog++;
	}

	if (abIpBuf[9] == 6) // TCP
	{
		int ih = (abIpBuf[0] & 0x0f) * 4;
		unsigned char *tcp = abIpBuf + ih;
		BYTE bFlags = tcp[13];
		// microstk sets PSH on its SYNs (fl=0x0a). Cloudflare/Google tolerate it, but a stricter
		// firewall (Akamai-fronted hosts, the game master) can drop a SYN+PSH as malformed -> the
		// handshake never starts. Normalize a PURE SYN (SYN set, ACK clear) to a clean 0x02 and
		// recompute the checksum so it's a well-formed SYN every stack accepts.
		if ((bFlags & 0x02) && !(bFlags & 0x10) && (bFlags & 0x08))
		{
			tcp[13] = (BYTE)(bFlags & ~0x08);
			TcpFixChecksum(abIpBuf);
			bFlags = tcp[13];
		}
		if (bFlags & 0x07) // SYN / FIN / RST
			SysLog(L"tcp TX %u.%u.%u.%u:%u fl=%02x %s", abIpBuf[16], abIpBuf[17], abIpBuf[18],
			       abIpBuf[19], (tcp[2] << 8) | tcp[3], bFlags, bHit ? L"sent" : L"DROP(arp)");
	}
	if (bHit)
		EthSend(abMac, 0x0800, abIpBuf, nOff);
	else
		ArpRequest(dwDst); // drop this one; ARP for next time (TCP retransmits)

	ifn->ifn_FreePacket(pkt); // contract: we own the packet
	return 0;
}

// Recompute the IPv4 header checksum after we rewrite an address (game-master DNAT).
static void IpFixChecksum(unsigned char *pbIp)
{
	int ihl = (pbIp[0] & 0x0f) * 4;
	unsigned short c;
	pbIp[10] = pbIp[11] = 0;
	c = Csum16(pbIp, ihl, 0);
	pbIp[10] = (unsigned char)(c >> 8);
	pbIp[11] = (unsigned char)c;
}

// Recompute the UDP checksum (pseudo-header src/dst/proto17/len + datagram) after a DNAT.
static void UdpFixChecksum(unsigned char *pbIp)
{
	int ihl = (pbIp[0] & 0x0f) * 4;
	unsigned char *udp = pbIp + ihl;
	int nUdpLen = (udp[4] << 8) | udp[5];
	unsigned long dwSeed = 17 + (unsigned long)nUdpLen;
	unsigned short c;
	int i;
	if (nUdpLen < 8)
		return;
	udp[6] = udp[7] = 0;
	for (i = 12; i < 20; i += 2)
		dwSeed += (unsigned long)((pbIp[i] << 8) | pbIp[i + 1]); // src + dst
	c = Csum16(udp, nUdpLen, dwSeed);
	if (c == 0)
		c = 0xffff; // UDP: a 0 checksum means "none"; send 0xffff instead
	udp[6] = (unsigned char)(c >> 8);
	udp[7] = (unsigned char)c;
}

// Map a dead hardcoded master-server IP to the configured revival master (game-master DNAT, TX).
// Records the pair so RxFrame can restore the original src on replies (one online game at a time).
static ulong RedirectDest(ulong dwDst)
{
	if (s_dwRevivalMaster && (dwDst == AFO_ORIG_IP || dwDst == IGP_ORIG_IP))
	{
		s_dwNatOrig = dwDst;
		s_dwNatTarget = s_dwRevivalMaster;
		return s_dwRevivalMaster;
	}
	return dwDst;
}

static int OurIoctl(ifnet *ifn, ulong dwCmd, void *pvArg)
{
	(void)ifn;
	(void)dwCmd;
	(void)pvArg;
	return 0;
}

// Write the DHCP-supplied DNS server(s) where the winsock resolver reads them: HKLM\Comm
// value "DnsServers", REG_BINARY = [count][ip...] (network order) - exactly the layout
// mppp's IPCP RegAddData uses. Without this gethostbyname has no resolver (microstk has
// no DNS code of its own).
extern int FlashromGetDns(unsigned long dns[2]); // DC system-flash ISP DNS (flashrom.c)

// Pack a dotted IP into the network-order-as-LE rep s_adwOffDns uses (wire bytes in memory).
static ulong DnsIp(BYTE a, BYTE b, BYTE c, BYTE d)
{
	return (ulong)a | ((ulong)b << 8) | ((ulong)c << 16) | ((ulong)d << 24);
}

// DNS resolution order, matching the reference path: DHCP option-6 -> DC system-flash ISP
// config -> public resolver last resort (the reference hardcodes 8.8.4.4). This guarantees
// gethostbyname always has a server, so name resolution never silently dies when the
// network routes but advertised no DNS server.
// Read the user-configured Primary/Secondary DNS from the RAS dial entry (the game's "Configure
// Internet Connection" UI). ras.c persists the whole RASENTRY blob as "Entry" under
// HKLM\Comm\RasBook\<name>; the DNS the player typed lives in RASENTRY.ipaddrDns/ipaddrDnsAlt.
// Players set these to point a dead master server's hostname at a revival server, so they take
// priority over the DHCP-advertised DNS. Returns the count (0..2) found, in our network-order
// rep (RASIPADDR is {a,b,c,d}, same byte order as a wire DNS address). First entry with a
// primary DNS wins. This is what stock mppp does via IPCP; we apply it directly to DnsServers.
static int ReadRasDns(ulong adwDns[2])
{
	HKEY hBook, hEntry;
	DWORD i = 0;
	int n = 0;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\RasBook", 0, KEY_READ, &hBook) != ERROR_SUCCESS)
		return 0;
	for (;;)
	{
		WCHAR achName[64], achPath[128];
		DWORD nl = 64, t, cb;
		RASENTRY re;
		if (RegEnumKeyExW(hBook, i++, achName, &nl, 0, 0, 0, 0) != ERROR_SUCCESS)
			break;
		lstrcpyW(achPath, L"Comm\\RasBook\\");
		lstrcatW(achPath, achName);
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, achPath, 0, KEY_READ, &hEntry) != ERROR_SUCCESS)
			continue;
		cb = sizeof(re);
		if (RegQueryValueExW(hEntry, L"Entry", 0, &t, (BYTE *)&re, &cb) == ERROR_SUCCESS &&
		    cb >= (DWORD)((BYTE *)&re.ipaddrDnsAlt - (BYTE *)&re) + sizeof(re.ipaddrDnsAlt))
		{
			ulong dwDns1 = 0, dwDns2 = 0;
			memcpy(&dwDns1, &re.ipaddrDns, 4);
			memcpy(&dwDns2, &re.ipaddrDnsAlt, 4);
			if (dwDns1)
			{
				adwDns[n++] = dwDns1;
				if (dwDns2 && n < 2)
					adwDns[n++] = dwDns2;
			}
		}
		RegCloseKey(hEntry);
		if (n > 0)
			break;
	}
	RegCloseKey(hBook);
	return n;
}

// Parse a dotted "a.b.c.d" into our wire-bytes-in-memory rep (0 if malformed / "0.0.0.0").
static ulong ParseIpStr(const WCHAR *psz)
{
	ulong dwVal = 0;
	int nPart = 0, n = 0, bSeen = 0;
	for (;; psz++)
	{
		if (*psz >= L'0' && *psz <= L'9')
		{
			n = n * 10 + (*psz - L'0');
			bSeen = 1;
		}
		else
		{
			if (bSeen && nPart < 4)
				((BYTE *)&dwVal)[nPart++] = (BYTE)n;
			n = 0;
			bSeen = 0;
			if (!*psz)
				break;
		}
	}
	return nPart == 4 ? dwVal : 0;
}

// Load the revival config (HKLM\Comm\Netif) once. Defaults to dns.flyca.st (178.156.255.64), the
// CURRENT Flycast/DCNet revival resolver; the legacy 46.101.91.123 is stale (it fails to resolve
// e.g. master*.4x4evolution.com, which public DNS resolves fine). IP-DNAT off unless configured.
static void ReadRevivalConfig(void)
{
	HKEY h;
	WCHAR achStr[32];
	DWORD t, cb;
	if (s_bRevivalRead)
		return;
	s_bRevivalRead = 1;
	s_dwRevivalDns = DnsIp(178, 156, 255, 64); // dns.flyca.st == dcnet.flyca.st == DCNet's resolver
	                                           // (set RevivalDns in HKLM\Comm\Netif to override)
	s_dwRevivalMaster = 0;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\Netif", 0, KEY_READ, &h) == ERROR_SUCCESS)
	{
		cb = sizeof(achStr);
		if (RegQueryValueExW(h, L"RevivalDns", 0, &t, (BYTE *)achStr, &cb) == ERROR_SUCCESS &&
		    t == REG_SZ)
			s_dwRevivalDns = ParseIpStr(achStr);
		cb = sizeof(achStr);
		if (RegQueryValueExW(h, L"RevivalMaster", 0, &t, (BYTE *)achStr, &cb) == ERROR_SUCCESS &&
		    t == REG_SZ)
			s_dwRevivalMaster = ParseIpStr(achStr);
		RegCloseKey(h);
	}
}

static void WriteDnsServers(void)
{
	HKEY h;
	ulong adwBuf[6];
	int i;
	LONG rc1, rc2 = -1;
	ulong adwRasDns[2];
	int nRas = ReadRasDns(adwRasDns);
	if (nRas > 0) // user's dial-config DNS overrides DHCP/flash
	{
		for (i = 0; i < nRas; i++)
			s_adwOffDns[i] = adwRasDns[i];
		s_nOffDnsN = nRas;
		OutputDebugStringW(L"netif: DNS: using RAS-entry (user-configured) DNS\r\n");
		SysLog(L"netif: DNS = RAS entry %u.%u.%u.%u", ((BYTE *)&adwRasDns[0])[0],
		       ((BYTE *)&adwRasDns[0])[1], ((BYTE *)&adwRasDns[0])[2], ((BYTE *)&adwRasDns[0])[3]);
	}
	if (s_nOffDnsN <= 0)
	{
		unsigned long adwFlashDns[2] = {0, 0};
		int nFlash = FlashromGetDns(adwFlashDns); // 2nd: DC system flash (DreamPassport/PlanetWeb)
		if (nFlash > 0)
		{
			for (i = 0; i < nFlash; i++)
				s_adwOffDns[i] = adwFlashDns[i];
			s_nOffDnsN = nFlash;
			OutputDebugStringW(L"netif: DNS: using flashrom (ISP) DNS servers\r\n");
		}
		else // 3rd: public resolver (last resort)
		{
			s_adwOffDns[0] = DnsIp(8, 8, 4, 4); // Google public DNS
			s_adwOffDns[1] = DnsIp(8, 8, 8, 8);
			s_nOffDnsN = 2;
			OutputDebugStringW(
			    L"netif: DNS: no option-6/flashrom - using public 8.8.4.4/8.8.8.8\r\n");
		}
	}
	ReadRevivalConfig();
	// (A) Prepend the revival DNS as PRIMARY so game master-server hostnames resolve to revival
	// IPs - UNLESS the user set an explicit per-game RAS-entry DNS (nRas>0), which we honor as-is.
	if (s_dwRevivalDns && nRas <= 0 && s_adwOffDns[0] != s_dwRevivalDns)
	{
		int j;
		if (s_nOffDnsN > 4)
			s_nOffDnsN = 4;
		for (j = s_nOffDnsN; j > 0; j--)
			s_adwOffDns[j] = s_adwOffDns[j - 1]; // shift the chain down
		s_adwOffDns[0] = s_dwRevivalDns;
		s_nOffDnsN++;
		SysLog(L"netif: DNS = revival %u.%u.%u.%u (primary)", ((BYTE *)&s_dwRevivalDns)[0],
		       ((BYTE *)&s_dwRevivalDns)[1], ((BYTE *)&s_dwRevivalDns)[2],
		       ((BYTE *)&s_dwRevivalDns)[3]);
	}
	adwBuf[0] = (ulong)s_nOffDnsN;
	for (i = 0; i < s_nOffDnsN && i < 5; i++)
		adwBuf[1 + i] = s_adwOffDns[i]; // network order, as received
	h = 0;
	rc1 =
	    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, KEY_ALL_ACCESS, &h); // [HKLM\Comm] preexists
	if (rc1 != ERROR_SUCCESS)
		rc1 = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, 0, 0, 0, 0, &h, 0);
	if (rc1 == ERROR_SUCCESS)
	{
		rc2 = RegSetValueExW(h, L"DnsServers", 0, REG_BINARY, (const BYTE *)adwBuf,
		                     (DWORD)(s_nOffDnsN * 4 + 4));
		RegCloseKey(h);
	}
	{
		WCHAR b[96];
		wsprintfW(b, L"netif: DNS write n=%d openOrCreate.rc=%d setVal.rc=%d\r\n", s_nOffDnsN,
		          (int)rc1, (int)rc2);
		OutputDebugStringW(b);
	}
}

// Re-apply DNS to the registry. ras.c calls this when a title dials (AfdRasDial): the player may
// have just set a custom Primary/Secondary DNS in the dial config AFTER the boot-time DHCP bind
// already wrote DnsServers, so re-run to pick it up. Safe to call repeatedly (WriteDnsServers
// overwrites, never appends).
void NetifApplyDns(void)
{
	WriteDnsServers();
}

// Called by the DHCP client on lease: configure microstk + go UP. Returns 1 once bound;
// returns 0 if microstk hasn't back-filled ifn_IPInterfaceConfigure yet (DHCP raced
// ahead) so the caller keeps DHCP alive and retries.
static int NetifOnLease(ulong dwIp, ulong dwMask, ulong dwGw)
{
	if (s_bHaveIP)
		return 1; // already bound (ignore duplicate ACK)
	s_dwMyIP = dwIp;
	s_dwMask = dwMask;
	s_dwGw = dwGw; // routing is ours, network-order rep
	if (!s_pIfn || !s_pIfn->ifn_IPInterfaceConfigure)
		return 0;
	// arg4 = MTU (NOT a gateway), arg5 = PPP peer (0 = ethernet). ip/mask are passed
	// NETWORK-order-as-LE (our raw wire/DHCP rep): microstk's IPInput compares ifn_ipAddr
	// DIRECTLY against the raw packet dest, and stores ipAddr as the wire source - so it
	// MUST be network order (byteswapping breaks both RX accept and the TX source addr).
	s_pIfn->ifn_IPInterfaceConfigure(s_pIfn, dwIp, dwMask, 1500, 0);
	s_pIfn->ifn_dwFlags |= IFF_UP;
	WriteDnsServers();
	s_bHaveIP = 1;
	{ // publish the bound address to the boot screen (dcwboot reads DCBOOT)
		WCHAR achIp[DCB_RESLEN];
		const BYTE *b = (const BYTE *)&dwIp;
		wsprintfW(achIp, L"%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
		DcBootSet(DCB_ADDR, DCB_OK, achIp);
	}
	// Eagerly resolve the GATEWAY MAC now. Off-link TCP routes via s_dwGw; if its ARP isn't
	// cached when the first off-link SYN goes out, OurTransmit drops that SYN and waits for a
	// TCP retransmit (~3s) - which can exceed a connect() timeout and look like "off-link
	// fails". Pre-warming the gateway entry means the first off-link SYN is sent immediately.
	if (s_dwGw && s_dwGw != s_dwMyIP)
		ArpRequest(s_dwGw);
	{
		WCHAR b[96];
		ulong dwDns = s_nOffDnsN ? s_adwOffDns[0] : 0;
		wsprintfW(b, L"netif: bound IP=%u.%u.%u.%u gw=%u.%u.%u.%u dns=%u.%u.%u.%u\r\n", dwIp & 0xff,
		          (dwIp >> 8) & 0xff, (dwIp >> 16) & 0xff, (dwIp >> 24) & 0xff, dwGw & 0xff,
		          (dwGw >> 8) & 0xff, (dwGw >> 16) & 0xff, (dwGw >> 24) & 0xff, dwDns & 0xff,
		          (dwDns >> 8) & 0xff, (dwDns >> 16) & 0xff, (dwDns >> 24) & 0xff);
		OutputDebugStringW(b);
	}
	return 1;
}

// ---- raw DHCP client (pre-IP; configures microstk on lease) ---------------------
static DWORD s_dwXid = 0xDC0DC0DCu;
static int s_nDhcpState; // 0=discover, 1=requested, 2=bound
static ulong s_dwOffIP, s_dwOffMask, s_dwOffGw, s_dwOffSrv;

static void Be16(unsigned char *pb, ushort wVal)
{
	pb[0] = (unsigned char)(wVal >> 8);
	pb[1] = (unsigned char)wVal;
}
static ushort Ipck(const unsigned char *pb, int nLen)
{
	DWORD dwSum = 0;
	int i;
	for (i = 0; i + 1 < nLen; i += 2)
		dwSum += (pb[i] << 8) | pb[i + 1];
	if (i < nLen)
		dwSum += pb[i] << 8;
	while (dwSum >> 16)
		dwSum = (dwSum & 0xffff) + (dwSum >> 16);
	return (ushort)~dwSum;
}

// Build a DHCP frame (msgType 1=DISCOVER, 3=REQUEST). All IPs are raw network-order
// bytes (memcpy'd), kept consistent everywhere so masking/compare just works.
static int DhcpBuild(unsigned char *pbOut, int nMsgType, ulong dwReqIP, ulong dwSrv)
{
	unsigned char *eth = pbOut, *ip = pbOut + 14, *udp = pbOut + 34, *bp = pbOut + 42, *o;
	int nDataLen, i;
	memset(pbOut, 0, 400);
	for (i = 0; i < 6; i++)
		eth[i] = 0xff;
	memcpy(eth + 6, s_abMac, 6);
	Be16(eth + 12, 0x0800);
	bp[0] = 1;
	bp[1] = 1;
	bp[2] = 6;
	bp[4] = (unsigned char)(s_dwXid >> 24);
	bp[5] = (unsigned char)(s_dwXid >> 16);
	bp[6] = (unsigned char)(s_dwXid >> 8);
	bp[7] = (unsigned char)s_dwXid;
	Be16(bp + 10, 0x8000);
	memcpy(bp + 28, s_abMac, 6);
	bp[236] = 0x63;
	bp[237] = 0x82;
	bp[238] = 0x53;
	bp[239] = 0x63;
	o = bp + 240;
	*o++ = 53;
	*o++ = 1;
	*o++ = (unsigned char)nMsgType;
	if (nMsgType == 3)
	{
		*o++ = 50;
		*o++ = 4;
		memcpy(o, &dwReqIP, 4);
		o += 4;
		*o++ = 54;
		*o++ = 4;
		memcpy(o, &dwSrv, 4);
		o += 4;
	}
	*o++ = 55;
	*o++ = 4;
	*o++ = 1;
	*o++ = 3;
	*o++ = 6;
	*o++ = 15;
	*o++ = 255;
	nDataLen = (int)(o - bp);
	if (nDataLen < 250)
		nDataLen = 250;
	Be16(udp + 0, 68);
	Be16(udp + 2, 67);
	Be16(udp + 4, (ushort)(8 + nDataLen));
	Be16(udp + 6, 0);
	ip[0] = 0x45;
	Be16(ip + 2, (ushort)(20 + 8 + nDataLen));
	ip[8] = 128;
	ip[9] = 17;
	for (i = 0; i < 4; i++)
		ip[16 + i] = 0xff;
	Be16(ip + 10, Ipck(ip, 20));
	return 14 + 20 + 8 + nDataLen;
}

static void DhcpStep(void)
{
	unsigned char abFrame[400];
	int nLen;
	if (s_nDhcpState == 0)
		nLen = DhcpBuild(abFrame, 1, 0, 0); // DISCOVER
	else if (s_nDhcpState == 1)
		nLen = DhcpBuild(abFrame, 3, s_dwOffIP, s_dwOffSrv); // (re)REQUEST
	else
		return;
	if (s_pLink && s_pLink->tx)
		s_pLink->tx(abFrame, nLen);
}

static void DhcpRx(const unsigned char *pbFrame, int nLen)
{
	const unsigned char *ip = pbFrame + 14, *bp, *o, *end;
	int ihl = (ip[0] & 0x0f) * 4; // honor IP options (IHL>5)
	int nMsgType = 0, nDnsN = 0, k;
	ulong adwDns[5];
	bp = pbFrame + 14 + ihl + 8; // BOOTP = after Eth(14)+IP(ihl)+UDP(8)
	if (nLen < (int)(bp - pbFrame) + 240 + 6 || bp[0] != 2)
		return;                     // BOOTREPLY
	memcpy(&s_dwOffIP, bp + 16, 4); // yiaddr
	s_dwOffMask = 0;
	s_dwOffGw = 0;
	s_dwOffSrv = 0; // (DNS parsed into a local; see commit below)
	for (o = bp + 240, end = pbFrame + nLen; o < end && *o != 255;)
	{
		int nOpt = *o++, nOptLen;
		if (nOpt == 0)
			continue;
		nOptLen = *o++;
		if (nOpt == 53 && nOptLen == 1)
			nMsgType = o[0];
		else if (nOpt == 1 && nOptLen == 4)
			memcpy(&s_dwOffMask, o, 4);
		else if (nOpt == 3 && nOptLen >= 4)
			memcpy(&s_dwOffGw, o, 4);
		else if (nOpt == 54 && nOptLen == 4)
			memcpy(&s_dwOffSrv, o, 4);
		else if (nOpt == 6 && nOptLen >= 4) // DNS servers (option 6)
			for (k = 0; k + 4 <= nOptLen && nDnsN < 5; k += 4)
				memcpy(&adwDns[nDnsN++], o + k, 4);
		o += nOptLen;
	}
	// Commit DNS only when this reply actually carried option 6 - a later reply WITHOUT it
	// (renew, relayed dup) must not wipe a previously-parsed good set (agent B2).
	if (nDnsN > 0)
	{
		for (k = 0; k < nDnsN; k++)
			s_adwOffDns[k] = adwDns[k];
		s_nOffDnsN = nDnsN;
	}
	SysLog(L"dhcp: reply type=%d ihl=%d yi=%u.%u.%u.%u dnsN=%d", nMsgType, ihl,
	       ((BYTE *)&s_dwOffIP)[0], ((BYTE *)&s_dwOffIP)[1], ((BYTE *)&s_dwOffIP)[2],
	       ((BYTE *)&s_dwOffIP)[3], s_nOffDnsN);
	if (!s_dwOffMask)
		s_dwOffMask = 0x00ffffff; // /24 (network bytes 255.255.255.0)
	if (!s_dwOffGw)
		s_dwOffGw = (s_dwOffIP & s_dwOffMask) | (1u << 24); // guess .1
	if (nMsgType == 2 && s_nDhcpState == 0)
	{
		s_nDhcpState = 1;
		DhcpStep();
	} // OFFER -> REQUEST
	else if (nMsgType == 5 || s_nDhcpState == 1)
	{ // ACK (or accept)
		if (NetifOnLease(s_dwOffIP, s_dwOffMask, s_dwOffGw))
			s_nDhcpState = 2; // bound
		// else microstk not back-filled yet: stay at state 1, worker re-REQUESTs
	}
}

// Worker: poll the link for frames + drive DHCP until bound.
static DWORD WINAPI NetWorker(LPVOID pvParam)
{
	unsigned char abBuf[1600];
	DWORD dwLastDhcp = 0;
	(void)pvParam;
	for (;;)
	{
		int n = s_pLink->poll ? s_pLink->poll(abBuf, sizeof(abBuf)) : 0;
		if (n > 0)
		{
			s_dwRxBytes += (DWORD)n;
			RxFrame(abBuf, n);
		}
		if (!s_bHaveIP && GetTickCount() - dwLastDhcp > 1500)
		{
			DhcpStep();
			dwLastDhcp = GetTickCount();
		}
		if (GetTickCount() - s_dwStatTick >
		    2000) // throughput tick: only while bytes are moving, so
		{         // a stalled stream's last line shows where rx/tx froze
			s_dwStatTick = GetTickCount();
			if (s_dwRxBytes != s_dwRxPrev || s_dwTxBytes != s_dwTxPrev)
			{
				SysLog(L"netif: rx=%u tx=%u bytes", s_dwRxBytes, s_dwTxBytes);
				s_dwRxPrev = s_dwRxBytes;
				s_dwTxPrev = s_dwTxBytes;
			}
		}
		if (n <= 0)
			Sleep(2);
	}
}

// ---- the export microstk calls -------------------------------------------------
// microstk deliberately calls this TWICE (two "PPP" slots). All our backend state is
// process-global (one BBA), so we bring the hardware up + start the worker exactly ONCE,
// and return a fresh distinct ifnet per call (microstk needs distinct pointers). Only
// the FIRST ifnet becomes the live RX-delivery + DHCP target (s_pIfn); later ifnets stay
// inert/DOWN so microstk routes through the one that gets an IP.
static int s_bHwUp;

int InterfaceInitialize(unsigned short *name, ifnet **ppIf)
{
	ifnet *ifn;
	int i;

	if (g_nUseDial)
		return s_pfnDialIfInit(name, ppIf); // modem mode: every call -> original driver
	if (!s_bHwUp)                           // first call: detect + bring up hardware
	{
		SysLog(L"netif: InterfaceInitialize");
		s_pLink = 0;
		SysLog(L"netif: probing W5500");
		if (s_w5500.probe())
			s_pLink = &s_w5500; // 1st: dedicated W5500 NIC if present
		else
		{
			SysLog(L"netif: W5500 absent, probing BBA");
			if (s_bba.probe())
				s_pLink = &s_bba; // 2nd: Broadband Adapter (RTL8139)
			else if (LoadDialDriver())
				g_nUseDial = 1; // 3rd: dial-up modem -> original PPP driver
			else
			{
				s_pLink = &s_null;
				SysLog(L"netif: no adapter, loopback only");
			}
		}
		s_bHwUp = 1;
		// Modem path: the original mpppdial.dll owns the whole link (its own ifnet + PPP). Hand
		// every InterfaceInitialize call straight to it; our ethernet setup below is skipped.
		if (g_nUseDial)
			return s_pfnDialIfInit(name, ppIf);
		SysLog(L"netif: link chosen, bringing up");
		// Backend probed PRESENT but bring-up failed. This is the real-HW killer: on silicon
		// BbaInit/W5500Init can fail in ways an idealized model never does (GAPS EEPROM handshake,
		// reset timing, no PHY/link). We must NOT return 0 here - microstk then wedges on a NULL
		// ifnet and the whole boot hangs/resets before the shell (same wedge the s_null probe
		// fallback below avoids). So on init failure, fall back to the no-op null link: the
		// system still boots to the desktop (network down) and the failure is logged, instead
		// of bricking the boot. This makes "BBA-only" and "W5500-only" boot-survivable.
		if (!s_pLink->init(s_abMac))
		{
			SysLog(L"netif: %S init FAILED, null fallback", s_pLink->name);
			s_pLink = &s_null;
			s_pLink->init(s_abMac); // NullInit cannot fail
		}
		else
			SysLog(L"netif: %S init OK MAC %02x:%02x:%02x:%02x:%02x:%02x", s_pLink->name,
			       s_abMac[0], s_abMac[1], s_abMac[2], s_abMac[3], s_abMac[4], s_abMac[5]);
		s_bHwUp = 1;
		SysLog(L"netif: hardware up, worker starting");
	}

	ifn = (ifnet *)LocalAlloc(LPTR, sizeof(ifnet));
	if (!ifn)
		return 0;
	for (i = 0; i < 15 && name && name[i]; i++)
		ifn->ifn_szName[i] = name[i];
	ifn->ifn_Ioctl = OurIoctl;
	ifn->ifn_IPOutput = OurTransmit;
	ifn->ifn_dwMTU = 1500;
	ifn->ifn_dwFlags = IFF_BROADCAST; // ethernet: route by netmask, start DOWN (no IFF_UP)
	*ppIf = ifn;

	if (!s_pIfn) // first interface = the live one
	{
		s_pIfn = ifn;
		ReadRevivalConfig();    // revival DNS + game-master DNAT config
		if (s_pLink != &s_null) // no worker/DHCP when there's no real NIC
			CloseHandle(CreateThread(0, 0, NetWorker, 0, 0, 0));
	}
	return 1;
}

// DLL entry point AND export @16 (mppp names it "dllentry"); /entry:dllentry skips
// the CRT entry - fine here (no C++ statics; corelibc fns need no init).
BOOL WINAPI dllentry(HANDLE h, DWORD reason, LPVOID r)
{
	(void)h;
	(void)reason;
	(void)r;
	return TRUE;
}
