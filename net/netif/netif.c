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
#include <ras.h>            // RASENTRY (the dial-config DNS the user set lives in ipaddrDns)

// ---- link backend abstraction --------------------------------------------------
typedef struct {
    const char *name;
    int   (*probe)(void);                       // 1 if the adapter is present
    int   (*init)(unsigned char mac[6]);        // bring HW up, return MAC
    int   (*tx)(const unsigned char *frame, int len);   // send a raw ethernet frame
    int   (*poll)(unsigned char *buf, int max); // get one rx frame, return len or 0
} LinkOps;

// BBA backend (RTL8139 on the G2 bus) - hardware reused from drivers/bba/bba.c.
extern int  BbaProbe(void);                     // GAPS magic 0x5a14a501 present?
extern int  BbaInit(unsigned char mac[6]);
extern int  BbaTx(const unsigned char *frame, int len);
extern int  BbaRxPoll(unsigned char *buf, int max);
static LinkOps s_bba = { "BBA", BbaProbe, BbaInit, BbaTx, BbaRxPoll };

// W5500 backend (MACRAW over dcspi.dll: SCI hardware-SPI / SCIF bit-bang) - see w5500.c.
extern int W5500Probe(void);
extern int W5500Init(unsigned char mac[6]);
extern int W5500Tx(const unsigned char *frame, int len);
extern int W5500RxPoll(unsigned char *buf, int max);
static LinkOps s_w5500 = { "W5500", W5500Probe, W5500Init, W5500Tx, W5500RxPoll };

// Modem backend: rather than re-implement PPP, DELEGATE to the original SDK dial-up driver
// (mpppdial.dll = the stock mppp.dll, vendored under a non-recursive name). When no ethernet
// adapter is present we LoadLibrary it and forward InterfaceInitialize + the 13 AfdRas* exports;
// it does the whole serial(COM6)+AT-dial+LCP/PAP-CHAP/IPCP+HDLC path (config from HKLM\Modem\
// Sega-DreamcastBuiltIn). g_useDial/g_dialRas are read by ras.c to forward the RAS exports too.
int     g_useDial = 0;                           // 1 = modem path: forward everything to mpppdial
FARPROC g_dialRas[14];                            // [1..13] = original AfdRas* by ordinal
static int (*g_dialIfInit)(unsigned short *, ifnet **);

static int LoadDialDriver(void)
{
    HINSTANCE h;
    int i;
    h = LoadLibraryW(L"mpppdial.dll");
    if (!h) { SysLog(L"netif: mpppdial.dll load FAILED"); return 0; }
    g_dialIfInit = (int (*)(unsigned short *, ifnet **))GetProcAddress(h, (LPCWSTR)15);  // InterfaceInitialize
    for (i = 1; i <= 13; i++) g_dialRas[i] = (FARPROC)GetProcAddress(h, (LPCWSTR)(DWORD)i);
    if (!g_dialIfInit) { SysLog(L"netif: mpppdial InterfaceInitialize missing"); return 0; }
    SysLog(L"netif: no ethernet -> delegating modem/PPP to original mpppdial.dll");
    return 1;
}

// Null backend - no NIC present. A no-op link so microstk STILL gets a valid ifnet and
// initialises (the stack comes up with just loopback); without it InterfaceInitialize
// returns 0, microstk wedges on a NULL ifnet, and the whole boot hangs before the shell.
// The interface stays DOWN (no IP); enable a real link to actually network.
static int NullInit(unsigned char mac[6])
{ mac[0]=0x02; mac[1]=0xDC; mac[2]=mac[3]=mac[4]=0; mac[5]=0xFF; return 1; }
static int NullTx(const unsigned char *f, int n)   { (void)f; (void)n; return 0; }   // drop
static int NullRxPoll(unsigned char *b, int max)   { (void)b; (void)max; return 0; } // nothing
static LinkOps s_null = { "NONE", 0, NullInit, NullTx, NullRxPoll };

static LinkOps *s_link;                          // the chosen backend
static ifnet   *g_ifn;                           // our interface (microstk filled the callbacks)
static unsigned char g_mac[6];
static int      g_haveIP;
static void DhcpRx(const unsigned char *f, int n);   // fwd: raw DHCP reply handler

// ---- tiny ARP cache (ethernet backends) ----------------------------------------
#define ARP_N 16
static struct { ulong ip; unsigned char mac[6]; int used; } s_arp[ARP_N];
static ulong g_myIP, g_mask, g_gw;       // kept in network-order-as-LE (wire/ARP rep)
static ulong g_offDns[5];                // DHCP option-6 DNS servers (network order)
static int   g_offDnsN;
static int   s_txLog, s_rxLog;           // limit TX/RX diagnostics
static DWORD g_rxBytes, g_txBytes;       // cumulative link RX (delivered) / TX (microstk produced)
static DWORD g_statTick, g_rxPrev, g_txPrev;   // periodic throughput log (spot where a stream stalls)

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
#define AFO_ORIG_IP  0x83f2fb3fUL         // 63.251.242.131  Alien Front Online  (wire bytes in mem)
#define IGP_ORIG_IP  0xef2bd2ccUL         // 204.210.43.239  Internet Game Pack
static ulong g_revivalDns;               // primary DNS to prepend (0 = off)
static ulong g_revivalMaster;            // DNAT target for the hardcoded-IP games (0 = off)
static ulong g_natOrig, g_natTarget;     // active 1-entry reverse-NAT (one online game at a time)
static int   g_revivalRead;              // config read yet?
static void  TcpFixChecksum(unsigned char *ip);   // used by both RxFrame (above) and OurTransmit
static void  IpFixChecksum(unsigned char *ip);
static void  UdpFixChecksum(unsigned char *ip);
static ulong RedirectDest(ulong dst);

static int ArpLookup(ulong ip, unsigned char mac[6])
{
    int i;
    for (i = 0; i < ARP_N; i++)
        if (s_arp[i].used && s_arp[i].ip == ip) { memcpy(mac, s_arp[i].mac, 6); return 1; }
    return 0;
}
static void ArpInsert(ulong ip, const unsigned char *mac)
{
    int i, slot = 0;
    for (i = 0; i < ARP_N; i++) { if (s_arp[i].used && s_arp[i].ip == ip) { slot = i; break; }
                                  if (!s_arp[i].used) slot = i; }
    s_arp[slot].ip = ip; memcpy(s_arp[slot].mac, mac, 6); s_arp[slot].used = 1;
}

static void EthSend(const unsigned char dst[6], ushort type, const unsigned char *pl, int len)
{
    unsigned char f[1600];
    if (len > 1500) return;
    memcpy(f, dst, 6); memcpy(f + 6, g_mac, 6);
    f[12] = (unsigned char)(type >> 8); f[13] = (unsigned char)type;
    memcpy(f + 14, pl, len);
    s_link->tx(f, len + 14);
}

static void ArpRequest(ulong ip)
{
    unsigned char a[28], bcast[6];
    memset(bcast, 0xff, 6);
    a[0]=0;a[1]=1; a[2]=8;a[3]=0; a[4]=6; a[5]=4; a[6]=0;a[7]=1;     // eth/ip, req
    memcpy(a+8, g_mac, 6); memcpy(a+14, &g_myIP, 4);
    memset(a+18, 0, 6);    memcpy(a+24, &ip, 4);
    EthSend(bcast, 0x0806, a, 28);
}

// Skip one DNS name (labels until a 0 length byte, or a 0xC0 compression pointer).
static const unsigned char *DnsSkipName(const unsigned char *p, const unsigned char *end)
{
    while (p < end)
    {
        if ((*p & 0xC0) == 0xC0) return p + 2;       // compression pointer (2 bytes)
        if (*p == 0)             return p + 1;       // root label = end of name
        p += *p + 1;                                 // ordinary label
    }
    return end;
}

// 16-bit ones-complement checksum over a network-order byte range (folded), with a running seed.
static unsigned short Csum16(const unsigned char *p, int n, unsigned long seed)
{
    unsigned long sum = seed;
    int i;
    for (i = 0; i + 1 < n; i += 2) sum += (unsigned long)((p[i] << 8) | p[i + 1]);
    if (n & 1) sum += (unsigned long)(p[n - 1] << 8);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (unsigned short)(~sum & 0xFFFF);
}

// Flatten a DNS reply so coredll's broken DnsQueryAddress (which takes answer[0].RDATA without
// checking TYPE) returns the right address. We do exactly what KOS's getaddrinfo RR-walk does -
// scan every answer RR, pick the TYPE==A record, skip CNAME(5)/others - then rebuild the packet
// with a SINGLE A answer named via a compression pointer to the question. This mirrors what
// Flycast achieves by handing the guest a flattening DNS server (dns.flyca.st), done in-flight.
// Returns the new IP-datagram length in `out` (>0) when it rewrote, else 0 (deliver original).
// Only rewrites CNAME-bearing replies that DO carry an A - plain A-only replies pass through.
static int DnsFlatten(const unsigned char *ip, int iplen, unsigned char *out)
{
    int ihl = (ip[0] & 0x0f) * 4;
    const unsigned char *udp = ip + ihl, *dns = udp + 8, *end = ip + iplen, *p;
    int qd, an, i, qlen, dnslen, udplen, total, haveA = 0, cname = 0;
    unsigned char aip[4] = { 0, 0, 0, 0 };
    unsigned char *od, *a;
    if (dns + 12 > end) return 0;
    qd = (dns[4] << 8) | dns[5];                      // QDCOUNT
    an = (dns[6] << 8) | dns[7];                      // ANCOUNT
    p  = dns + 12;
    for (i = 0; i < qd && p < end; i++) { p = DnsSkipName(p, end); p += 4; }   // skip questions
    qlen = (int)(p - (dns + 12));                     // question-section length (to copy verbatim)
    for (i = 0; i < an && p < end; i++)               // KOS-style answer walk
    {
        int ty, rdl;
        p = DnsSkipName(p, end);
        if (p + 10 > end) break;
        ty  = (p[0] << 8) | p[1];                     // TYPE (1=A, 5=CNAME, 28=AAAA, ...)
        rdl = (p[8] << 8) | p[9];                     // RDLENGTH
        if (ty == 5) cname = 1;
        if (ty == 1 && rdl == 4 && p + 14 <= end && !haveA) { memcpy(aip, p + 10, 4); haveA = 1; }
        p += 10 + rdl;
    }
    SysLog(L"dns: an=%d cname=%d haveA=%d ip=%u.%u.%u.%u", an, cname, haveA, aip[0], aip[1], aip[2], aip[3]);
    if (!cname || !haveA) return 0;                   // only fix CNAME-confused replies that carry an A

    memcpy(out, ip, ihl);                             // IP header (length+csum fixed below)
    memcpy(out + ihl, udp, 8);                        // UDP header (length+csum fixed below)
    od = out + ihl + 8;
    memcpy(od, dns, 12);                              // DNS header
    od[6] = 0; od[7] = 1;                             // ANCOUNT = 1
    od[8] = od[9] = od[10] = od[11] = 0;              // NSCOUNT = ARCOUNT = 0
    memcpy(od + 12, dns + 12, qlen);                  // copy question verbatim
    a = od + 12 + qlen;
    a[0] = 0xC0; a[1] = 0x0C;                         // NAME -> pointer to question name (offset 12)
    a[2] = 0x00; a[3] = 0x01;                         // TYPE  = A
    a[4] = 0x00; a[5] = 0x01;                         // CLASS = IN
    a[6] = a[7] = a[8] = 0; a[9] = 60;                // TTL   = 60s
    a[10] = 0x00; a[11] = 0x04;                       // RDLENGTH = 4
    memcpy(a + 12, aip, 4);                           // RDATA = the A address
    dnslen = 12 + qlen + 16;
    udplen = 8 + dnslen;
    out[ihl + 4] = (unsigned char)(udplen >> 8); out[ihl + 5] = (unsigned char)udplen;
    out[ihl + 6] = out[ihl + 7] = 0;                  // zero UDP checksum before recompute
    total = ihl + udplen;
    out[2] = (unsigned char)(total >> 8); out[3] = (unsigned char)total;       // IP total length
    out[10] = out[11] = 0;                            // zero IP header checksum before recompute
    { unsigned short c = Csum16(out, ihl, 0); out[10] = (unsigned char)(c >> 8); out[11] = (unsigned char)c; }
    {   // UDP checksum over the pseudo-header (src+dst IP, proto=17, udplen) + UDP segment
        unsigned long seed = 17 + udplen;
        unsigned short c;
        for (i = 12; i < 20; i += 2) seed += (out[i] << 8) | out[i + 1];       // src + dst IP
        c = Csum16(out + ihl, udplen, seed);
        if (c == 0) c = 0xFFFF;
        out[ihl + 6] = (unsigned char)(c >> 8); out[ihl + 7] = (unsigned char)c;
    }
    return total;
}

// Build an NDIS packet for an inbound IP datagram and hand it to microstk (the exact
// sequence from mppp's Receive @ 1000ab5c).
static void DeliverIP(const unsigned char *ip, int len)
{
    NDIS_PACKET *pkt;
    NDIS_BUFFER *buf, *last;
    unsigned char *mem = 0;
    if (!g_ifn || !g_ifn->ifn_AllocatePacket || !g_ifn->ifn_IPInput) return;
    pkt = g_ifn->ifn_AllocatePacket();
    if (!pkt) return;
    buf = g_ifn->ifn_AllocateBufferWithMemory((ulong)len, &mem);
    if (!buf || !mem) { g_ifn->ifn_FreePacket(pkt); return; }
    memcpy(mem, ip, len);
    for (last = buf; last->Next; last = last->Next) ;
    if (pkt->Private.Head == 0) pkt->Private.Tail = last;
    last->Next = pkt->Private.Head;
    pkt->Private.Head = buf;
    pkt->Private.ValidCounts = 0;
    g_ifn->ifn_IPInput(g_ifn, pkt);
}

// Handle one raw ethernet frame from the link (ARP ourselves, IP -> microstk).
static void RxFrame(const unsigned char *f, int len)
{
    ushort type;
    if (len < 14) return;
    type = (ushort)((f[12] << 8) | f[13]);
    if (type == 0x0806 && len >= 42)            // ARP
    {
        const unsigned char *a = f + 14;
        ulong sip; memcpy(&sip, a + 14, 4);
        ArpInsert(sip, a + 8);                  // cache sender
        if (a[6] == 0 && a[7] == 1)             // request -> reply if for us
        {
            ulong tip; memcpy(&tip, a + 24, 4);
            if (tip == g_myIP && g_myIP)
            {
                unsigned char r[28];
                memcpy(r, a, 28);
                r[7] = 2;                                   // opcode = reply
                memcpy(r + 8, g_mac, 6); memcpy(r + 14, &g_myIP, 4);
                memcpy(r + 18, a + 8, 6);   memcpy(r + 24, a + 14, 4);
                EthSend(f + 6, 0x0806, r, 28);
            }
        }
    }
    else if (type == 0x0800)                    // IPv4
    {
        const unsigned char *ip = f + 14;
        int ihl = (ip[0] & 0x0f) * 4;                   // IP header length (IHL*4); NOT always 20
        const unsigned char *udp;                       // UDP starts after the (variable) IP header
        unsigned char natbuf[1600];
        // (B) reverse the game-master DNAT: replies arrive FROM the revival IP, but the guest's
        // stack expects them from the original (dead) IP it dialled. Restore src into a copy.
        if (g_natTarget && len - 14 >= 20 && len - 14 <= (int)sizeof(natbuf))
        {
            ulong sip; memcpy(&sip, ip + 12, 4);
            if (sip == g_natTarget)
            {
                memcpy(natbuf, ip, len - 14);
                memcpy(natbuf + 12, &g_natOrig, 4);     // src: revival -> original
                IpFixChecksum(natbuf);
                if (natbuf[9] == 6)       TcpFixChecksum(natbuf);
                else if (natbuf[9] == 17) UdpFixChecksum(natbuf);
                ip = natbuf;
            }
        }
        udp = ip + ihl;
        if (ip[9] == 17 && len >= 14 + ihl + 8 &&
            udp[2] == 0 && udp[3] == 68)                // UDP dest port 68 = DHCP reply
            DhcpRx(f, len);
        else
        {
            if (s_rxLog < 8)
            {
                WCHAR w[96];
                wsprintfW(w, L"netif RX[%d]: src=%u.%u.%u.%u proto=%u len=%d\r\n", s_rxLog,
                          ip[12], ip[13], ip[14], ip[15], ip[9], len - 14);
                OutputDebugStringW(w); s_rxLog++;
            }
            if (ip[9] == 6)                     // TCP: log every inbound handshake/control segment
            {
                const unsigned char *tcp = ip + ihl; BYTE fl = tcp[13];
                if (fl & 0x07)                  // SYN(-ACK=0x12) / FIN / RST(0x04)
                    SysLog(L"tcp RX %u.%u.%u.%u:%u fl=%02x", ip[12], ip[13], ip[14], ip[15],
                           (tcp[0] << 8) | tcp[1], fl);
            }
            // DNS reply (UDP src port 53)? Flatten CNAME chains so coredll's resolver returns the
            // real A record (see DnsFlatten). Otherwise hand the datagram up unchanged.
            if (ip[9] == 17 && len >= 14 + ihl + 8 && udp[0] == 0 && udp[1] == 53)
            {
                unsigned char flat[600];
                int nl = DnsFlatten(ip, len - 14, flat);
                if (nl > 0) { DeliverIP(flat, nl); return; }
            }
            DeliverIP(ip, len - 14);            // everything else -> microstk
        }
    }
}

// Recompute the TCP checksum over the pseudo-header (src/dst IP, proto 6, TCP length) + segment,
// after we normalize a SYN's flags. Reuses Csum16 (the same routine the DNS-flatten path uses).
static void TcpFixChecksum(unsigned char *ip)
{
    int ihl = (ip[0] & 0x0f) * 4;
    int tcplen = ((ip[2] << 8) | ip[3]) - ihl;
    unsigned char *tcp = ip + ihl;
    unsigned long seed = 6 + (unsigned long)tcplen;
    unsigned short c;
    int i;
    if (tcplen < 20) return;
    tcp[16] = tcp[17] = 0;
    for (i = 12; i < 20; i += 2) seed += (unsigned long)((ip[i] << 8) | ip[i + 1]);   // src + dst IP
    c = Csum16(tcp, tcplen, seed);
    tcp[16] = (unsigned char)(c >> 8); tcp[17] = (unsigned char)c;
}

// ---- microstk TX/Ioctl (the ifnet vtable) --------------------------------------
// ifn_IPOutput: gather the NDIS buffer chain into one IP datagram, resolve the
// next hop (ARP), prepend ethernet, send; ALWAYS FreePacket; return 0 = ok.
static ulong OurTransmit(ifnet *ifn, NDIS_PACKET *pkt, ulong nextHop)
{
    unsigned char ipbuf[1600], mac[6];
    NDIS_BUFFER *b;
    int off = 0, hit;
    ulong dst;
    (void)nextHop;                              // microstk has no routes; use the IP header dest
    for (b = pkt->Private.Head; b && off + (int)b->BufferLength <= (int)sizeof(ipbuf); b = b->Next)
    { memcpy(ipbuf + off, b->VirtualAddress, b->BufferLength); off += (int)b->BufferLength; }
    g_txBytes += (DWORD)off;                    // microstk's TX intent (ACKs/data) - even if dropped below

    memcpy(&dst, ipbuf + 16, 4);                // IP dest (network order = our g_* rep)
    {                                           // (B) game-master DNAT: dead hardcoded IP -> revival
        ulong nd = RedirectDest(dst);
        if (nd != dst)
        {
            memcpy(ipbuf + 16, &nd, 4);
            IpFixChecksum(ipbuf);
            if (ipbuf[9] == 6)       TcpFixChecksum(ipbuf);
            else if (ipbuf[9] == 17) UdpFixChecksum(ipbuf);
            SysLog(L"netif: game master %u.%u.%u.%u -> revival %u.%u.%u.%u",
                   ((BYTE*)&dst)[0],((BYTE*)&dst)[1],((BYTE*)&dst)[2],((BYTE*)&dst)[3],
                   ((BYTE*)&nd)[0],((BYTE*)&nd)[1],((BYTE*)&nd)[2],((BYTE*)&nd)[3]);
            dst = nd;
        }
    }
    if (g_mask && (dst & g_mask) != (g_myIP & g_mask)) dst = g_gw;   // off-link -> gateway

    hit = ArpLookup(dst, mac);
    if (s_txLog < 6)
    {
        WCHAR w[96]; ulong d; memcpy(&d, ipbuf + 16, 4);
        wsprintfW(w, L"netif TX[%d]: dst=%u.%u.%u.%u via=%u.%u.%u.%u %s\r\n", s_txLog,
                  d&0xff,(d>>8)&0xff,(d>>16)&0xff,(d>>24)&0xff,
                  dst&0xff,(dst>>8)&0xff,(dst>>16)&0xff,(dst>>24)&0xff,
                  hit ? L"(arp hit)" : L"(arp miss->req)");
        OutputDebugStringW(w); s_txLog++;
    }

    if (ipbuf[9] == 6)                          // TCP
    {
        int ih = (ipbuf[0] & 0x0f) * 4; unsigned char *tcp = ipbuf + ih; BYTE fl = tcp[13];
        // microstk sets PSH on its SYNs (fl=0x0a). Cloudflare/Google tolerate it, but a stricter
        // firewall (Akamai-fronted hosts, the game master) can drop a SYN+PSH as malformed -> the
        // handshake never starts. Normalize a PURE SYN (SYN set, ACK clear) to a clean 0x02 and
        // recompute the checksum so it's a well-formed SYN every stack accepts.
        if ((fl & 0x02) && !(fl & 0x10) && (fl & 0x08))
        { tcp[13] = (BYTE)(fl & ~0x08); TcpFixChecksum(ipbuf); fl = tcp[13]; }
        if (fl & 0x07)                          // SYN / FIN / RST
            SysLog(L"tcp TX %u.%u.%u.%u:%u fl=%02x %s", ipbuf[16], ipbuf[17], ipbuf[18], ipbuf[19],
                   (tcp[2] << 8) | tcp[3], fl, hit ? L"sent" : L"DROP(arp)");
    }
    if (hit) EthSend(mac, 0x0800, ipbuf, off);
    else ArpRequest(dst);                       // drop this one; ARP for next time (TCP retransmits)

    ifn->ifn_FreePacket(pkt);                   // contract: we own the packet
    return 0;
}

// Recompute the IPv4 header checksum after we rewrite an address (game-master DNAT).
static void IpFixChecksum(unsigned char *ip)
{
    int ihl = (ip[0] & 0x0f) * 4; unsigned short c;
    ip[10] = ip[11] = 0;
    c = Csum16(ip, ihl, 0);
    ip[10] = (unsigned char)(c >> 8); ip[11] = (unsigned char)c;
}

// Recompute the UDP checksum (pseudo-header src/dst/proto17/len + datagram) after a DNAT.
static void UdpFixChecksum(unsigned char *ip)
{
    int ihl = (ip[0] & 0x0f) * 4; unsigned char *udp = ip + ihl;
    int ulen = (udp[4] << 8) | udp[5];
    unsigned long seed = 17 + (unsigned long)ulen;
    unsigned short c; int i;
    if (ulen < 8) return;
    udp[6] = udp[7] = 0;
    for (i = 12; i < 20; i += 2) seed += (unsigned long)((ip[i] << 8) | ip[i + 1]);  // src + dst
    c = Csum16(udp, ulen, seed);
    if (c == 0) c = 0xffff;                  // UDP: a 0 checksum means "none"; send 0xffff instead
    udp[6] = (unsigned char)(c >> 8); udp[7] = (unsigned char)c;
}

// Map a dead hardcoded master-server IP to the configured revival master (game-master DNAT, TX).
// Records the pair so RxFrame can restore the original src on replies (one online game at a time).
static ulong RedirectDest(ulong dst)
{
    if (g_revivalMaster && (dst == AFO_ORIG_IP || dst == IGP_ORIG_IP))
    {
        g_natOrig = dst; g_natTarget = g_revivalMaster;
        return g_revivalMaster;
    }
    return dst;
}

static int OurIoctl(ifnet *ifn, ulong cmd, void *arg) { (void)ifn; (void)cmd; (void)arg; return 0; }

// Write the DHCP-supplied DNS server(s) where the winsock resolver reads them: HKLM\Comm
// value "DnsServers", REG_BINARY = [count][ip...] (network order) - exactly the layout
// mppp's IPCP RegAddData uses. Without this gethostbyname has no resolver (microstk has
// no DNS code of its own).
extern int FlashromGetDns(unsigned long dns[2]);   // DC system-flash ISP DNS (flashrom.c)

// Pack a dotted IP into the network-order-as-LE rep g_offDns uses (wire bytes in memory).
static ulong dns_ip(BYTE a, BYTE b, BYTE c, BYTE d)
{ return (ulong)a | ((ulong)b << 8) | ((ulong)c << 16) | ((ulong)d << 24); }

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
static int ReadRasDns(ulong dns[2])
{
    HKEY  book, e;
    DWORD i = 0;
    int   n = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\RasBook", 0, KEY_READ, &book) != ERROR_SUCCESS)
        return 0;
    for (;;)
    {
        WCHAR    nm[64], path[128];
        DWORD    nl = 64, t, cb;
        RASENTRY re;
        if (RegEnumKeyExW(book, i++, nm, &nl, 0, 0, 0, 0) != ERROR_SUCCESS) break;
        lstrcpyW(path, L"Comm\\RasBook\\"); lstrcatW(path, nm);
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &e) != ERROR_SUCCESS) continue;
        cb = sizeof(re);
        if (RegQueryValueExW(e, L"Entry", 0, &t, (BYTE *)&re, &cb) == ERROR_SUCCESS &&
            cb >= (DWORD)((BYTE *)&re.ipaddrDnsAlt - (BYTE *)&re) + sizeof(re.ipaddrDnsAlt))
        {
            ulong d1 = 0, d2 = 0;
            memcpy(&d1, &re.ipaddrDns, 4);
            memcpy(&d2, &re.ipaddrDnsAlt, 4);
            if (d1) { dns[n++] = d1; if (d2 && n < 2) dns[n++] = d2; }
        }
        RegCloseKey(e);
        if (n > 0) break;
    }
    RegCloseKey(book);
    return n;
}

// Parse a dotted "a.b.c.d" into our wire-bytes-in-memory rep (0 if malformed / "0.0.0.0").
static ulong ParseIpStr(const WCHAR *s)
{
    ulong v = 0; int part = 0, n = 0, seen = 0;
    for (;; s++)
    {
        if (*s >= L'0' && *s <= L'9') { n = n * 10 + (*s - L'0'); seen = 1; }
        else
        {
            if (seen && part < 4) ((BYTE *)&v)[part++] = (BYTE)n;
            n = 0; seen = 0;
            if (!*s) break;
        }
    }
    return part == 4 ? v : 0;
}

// Load the revival config (HKLM\Comm\Netif) once. Defaults to dns.flyca.st (178.156.255.64), the
// CURRENT Flycast/DCNet revival resolver; the legacy 46.101.91.123 is stale (it fails to resolve
// e.g. master*.4x4evolution.com, which public DNS resolves fine). IP-DNAT off unless configured.
static void ReadRevivalConfig(void)
{
    HKEY h; WCHAR s[32]; DWORD t, cb;
    if (g_revivalRead) return;
    g_revivalRead   = 1;
    g_revivalDns    = dns_ip(178, 156, 255, 64);   // dns.flyca.st == dcnet.flyca.st == DCNet's resolver
                                                   // (set RevivalDns in HKLM\Comm\Netif to override)
    g_revivalMaster = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\Netif", 0, KEY_READ, &h) == ERROR_SUCCESS)
    {
        cb = sizeof(s); if (RegQueryValueExW(h, L"RevivalDns",    0, &t, (BYTE *)s, &cb) == ERROR_SUCCESS && t == REG_SZ) g_revivalDns    = ParseIpStr(s);
        cb = sizeof(s); if (RegQueryValueExW(h, L"RevivalMaster", 0, &t, (BYTE *)s, &cb) == ERROR_SUCCESS && t == REG_SZ) g_revivalMaster = ParseIpStr(s);
        RegCloseKey(h);
    }
}

static void WriteDnsServers(void)
{
    HKEY  h;
    ulong buf[6];
    int   i;
    LONG  rc1, rc2 = -1;
    ulong rasdns[2];
    int   rn = ReadRasDns(rasdns);
    if (rn > 0)                                         // user's dial-config DNS overrides DHCP/flash
    {
        for (i = 0; i < rn; i++) g_offDns[i] = rasdns[i];
        g_offDnsN = rn;
        OutputDebugStringW(L"netif: DNS: using RAS-entry (user-configured) DNS\r\n");
        SysLog(L"netif: DNS = RAS entry %u.%u.%u.%u", ((BYTE*)&rasdns[0])[0], ((BYTE*)&rasdns[0])[1],
               ((BYTE*)&rasdns[0])[2], ((BYTE*)&rasdns[0])[3]);
    }
    if (g_offDnsN <= 0)
    {
        unsigned long fdns[2] = { 0, 0 };
        int fn = FlashromGetDns(fdns);                 // 2nd: DC system flash (DreamPassport/PlanetWeb)
        if (fn > 0)
        {
            for (i = 0; i < fn; i++) g_offDns[i] = fdns[i];
            g_offDnsN = fn;
            OutputDebugStringW(L"netif: DNS: using flashrom (ISP) DNS servers\r\n");
        }
        else                                           // 3rd: public resolver (last resort)
        {
            g_offDns[0] = dns_ip(8, 8, 4, 4);          // Google public DNS
            g_offDns[1] = dns_ip(8, 8, 8, 8);
            g_offDnsN   = 2;
            OutputDebugStringW(L"netif: DNS: no option-6/flashrom - using public 8.8.4.4/8.8.8.8\r\n");
        }
    }
    ReadRevivalConfig();
    // (A) Prepend the revival DNS as PRIMARY so game master-server hostnames resolve to revival
    // IPs - UNLESS the user set an explicit per-game RAS-entry DNS (rn>0), which we honor as-is.
    if (g_revivalDns && rn <= 0 && g_offDns[0] != g_revivalDns)
    {
        int j;
        if (g_offDnsN > 4) g_offDnsN = 4;
        for (j = g_offDnsN; j > 0; j--) g_offDns[j] = g_offDns[j - 1];   // shift the chain down
        g_offDns[0] = g_revivalDns;
        g_offDnsN++;
        SysLog(L"netif: DNS = revival %u.%u.%u.%u (primary)", ((BYTE*)&g_revivalDns)[0],
               ((BYTE*)&g_revivalDns)[1], ((BYTE*)&g_revivalDns)[2], ((BYTE*)&g_revivalDns)[3]);
    }
    buf[0] = (ulong)g_offDnsN;
    for (i = 0; i < g_offDnsN && i < 5; i++) buf[1 + i] = g_offDns[i];   // network order, as received
    h = 0;
    rc1 = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, KEY_ALL_ACCESS, &h);   // [HKLM\Comm] preexists
    if (rc1 != ERROR_SUCCESS)
        rc1 = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, 0, 0, 0, 0, &h, 0);
    if (rc1 == ERROR_SUCCESS)
    {
        rc2 = RegSetValueExW(h, L"DnsServers", 0, REG_BINARY, (const BYTE *)buf, (DWORD)(g_offDnsN * 4 + 4));
        RegCloseKey(h);
    }
    { WCHAR b[96]; wsprintfW(b, L"netif: DNS write n=%d openOrCreate.rc=%d setVal.rc=%d\r\n",
        g_offDnsN, (int)rc1, (int)rc2); OutputDebugStringW(b); }
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
static int NetifOnLease(ulong ip, ulong mask, ulong gw)
{
    if (g_haveIP) return 1;                          // already bound (ignore duplicate ACK)
    g_myIP = ip; g_mask = mask; g_gw = gw;          // routing is ours, network-order rep
    if (!g_ifn || !g_ifn->ifn_IPInterfaceConfigure) return 0;
    // arg4 = MTU (NOT a gateway), arg5 = PPP peer (0 = ethernet). ip/mask are passed
    // NETWORK-order-as-LE (our raw wire/DHCP rep): microstk's IPInput compares ifn_ipAddr
    // DIRECTLY against the raw packet dest, and stores ipAddr as the wire source - so it
    // MUST be network order (byteswapping breaks both RX accept and the TX source addr).
    g_ifn->ifn_IPInterfaceConfigure(g_ifn, ip, mask, 1500, 0);
    g_ifn->ifn_dwFlags |= IFF_UP;
    WriteDnsServers();
    g_haveIP = 1;
    {   // publish the bound address to the boot screen (dcwboot reads DCBOOT)
        WCHAR ips[DCB_RESLEN]; const BYTE *b = (const BYTE *)&ip;
        wsprintfW(ips, L"%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
        DcBootSet(DCB_ADDR, DCB_OK, ips);
    }
    // Eagerly resolve the GATEWAY MAC now. Off-link TCP routes via g_gw; if its ARP isn't
    // cached when the first off-link SYN goes out, OurTransmit drops that SYN and waits for a
    // TCP retransmit (~3s) - which can exceed a connect() timeout and look like "off-link
    // fails". Pre-warming the gateway entry means the first off-link SYN is sent immediately.
    if (g_gw && g_gw != g_myIP) ArpRequest(g_gw);
    { WCHAR b[96]; ulong dns = g_offDnsN ? g_offDns[0] : 0;
      wsprintfW(b, L"netif: bound IP=%u.%u.%u.%u gw=%u.%u.%u.%u dns=%u.%u.%u.%u\r\n",
        ip&0xff,(ip>>8)&0xff,(ip>>16)&0xff,(ip>>24)&0xff,
        gw&0xff,(gw>>8)&0xff,(gw>>16)&0xff,(gw>>24)&0xff,
        dns&0xff,(dns>>8)&0xff,(dns>>16)&0xff,(dns>>24)&0xff); OutputDebugStringW(b); }
    return 1;
}

// ---- raw DHCP client (pre-IP; configures microstk on lease) ---------------------
static DWORD g_xid = 0xDC0DC0DCu;
static int   g_dhcpState;                       // 0=discover, 1=requested, 2=bound
static ulong g_offIP, g_offMask, g_offGw, g_offSrv;

static void be16(unsigned char *p, ushort v) { p[0] = (unsigned char)(v >> 8); p[1] = (unsigned char)v; }
static ushort ipck(const unsigned char *p, int n)
{ DWORD s = 0; int i; for (i = 0; i + 1 < n; i += 2) s += (p[i] << 8) | p[i+1]; if (i < n) s += p[i] << 8;
  while (s >> 16) s = (s & 0xffff) + (s >> 16); return (ushort)~s; }

// Build a DHCP frame (msgType 1=DISCOVER, 3=REQUEST). All IPs are raw network-order
// bytes (memcpy'd), kept consistent everywhere so masking/compare just works.
static int DhcpBuild(unsigned char *out, int msgType, ulong reqIP, ulong srv)
{
    unsigned char *eth = out, *ip = out + 14, *udp = out + 34, *bp = out + 42, *o;
    int dlen, i;
    memset(out, 0, 400);
    for (i = 0; i < 6; i++) eth[i] = 0xff;
    memcpy(eth + 6, g_mac, 6); be16(eth + 12, 0x0800);
    bp[0] = 1; bp[1] = 1; bp[2] = 6;
    bp[4]=(unsigned char)(g_xid>>24); bp[5]=(unsigned char)(g_xid>>16); bp[6]=(unsigned char)(g_xid>>8); bp[7]=(unsigned char)g_xid;
    be16(bp + 10, 0x8000);
    memcpy(bp + 28, g_mac, 6);
    bp[236]=0x63; bp[237]=0x82; bp[238]=0x53; bp[239]=0x63;
    o = bp + 240;
    *o++ = 53; *o++ = 1; *o++ = (unsigned char)msgType;
    if (msgType == 3) { *o++ = 50; *o++ = 4; memcpy(o, &reqIP, 4); o += 4;
                        *o++ = 54; *o++ = 4; memcpy(o, &srv, 4);   o += 4; }
    *o++ = 55; *o++ = 4; *o++ = 1; *o++ = 3; *o++ = 6; *o++ = 15;
    *o++ = 255;
    dlen = (int)(o - bp); if (dlen < 250) dlen = 250;
    be16(udp + 0, 68); be16(udp + 2, 67); be16(udp + 4, (ushort)(8 + dlen)); be16(udp + 6, 0);
    ip[0] = 0x45; be16(ip + 2, (ushort)(20 + 8 + dlen)); ip[8] = 128; ip[9] = 17;
    for (i = 0; i < 4; i++) ip[16 + i] = 0xff;
    be16(ip + 10, ipck(ip, 20));
    return 14 + 20 + 8 + dlen;
}

static void DhcpStep(void)
{
    unsigned char f[400];
    int len;
    if (g_dhcpState == 0)      len = DhcpBuild(f, 1, 0, 0);                 // DISCOVER
    else if (g_dhcpState == 1) len = DhcpBuild(f, 3, g_offIP, g_offSrv);   // (re)REQUEST
    else return;
    if (s_link && s_link->tx) s_link->tx(f, len);
}

static void DhcpRx(const unsigned char *f, int n)
{
    const unsigned char *ip = f + 14, *bp, *o, *end;
    int ihl = (ip[0] & 0x0f) * 4;                           // honor IP options (IHL>5)
    int msgType = 0, dnsN = 0, k;
    ulong dns[5];
    bp = f + 14 + ihl + 8;                                  // BOOTP = after Eth(14)+IP(ihl)+UDP(8)
    if (n < (int)(bp - f) + 240 + 6 || bp[0] != 2) return;  // BOOTREPLY
    memcpy(&g_offIP, bp + 16, 4);                           // yiaddr
    g_offMask = 0; g_offGw = 0; g_offSrv = 0;               // (DNS parsed into a local; see commit below)
    for (o = bp + 240, end = f + n; o < end && *o != 255; ) {
        int op = *o++, ln; if (op == 0) continue; ln = *o++;
        if      (op == 53 && ln == 1) msgType = o[0];
        else if (op == 1  && ln == 4) memcpy(&g_offMask, o, 4);
        else if (op == 3  && ln >= 4) memcpy(&g_offGw, o, 4);
        else if (op == 54 && ln == 4) memcpy(&g_offSrv, o, 4);
        else if (op == 6  && ln >= 4)                                // DNS servers (option 6)
            for (k = 0; k + 4 <= ln && dnsN < 5; k += 4) memcpy(&dns[dnsN++], o + k, 4);
        o += ln;
    }
    // Commit DNS only when this reply actually carried option 6 - a later reply WITHOUT it
    // (renew, relayed dup) must not wipe a previously-parsed good set (agent B2).
    if (dnsN > 0) { for (k = 0; k < dnsN; k++) g_offDns[k] = dns[k]; g_offDnsN = dnsN; }
    SysLog(L"dhcp: reply type=%d ihl=%d yi=%u.%u.%u.%u dnsN=%d", msgType, ihl,
           ((BYTE*)&g_offIP)[0], ((BYTE*)&g_offIP)[1], ((BYTE*)&g_offIP)[2], ((BYTE*)&g_offIP)[3], g_offDnsN);
    if (!g_offMask) g_offMask = 0x00ffffff;                 // /24 (network bytes 255.255.255.0)
    if (!g_offGw)   g_offGw = (g_offIP & g_offMask) | (1u << 24);  // guess .1
    if (msgType == 2 && g_dhcpState == 0) { g_dhcpState = 1; DhcpStep(); }      // OFFER -> REQUEST
    else if (msgType == 5 || g_dhcpState == 1) {                                 // ACK (or accept)
        if (NetifOnLease(g_offIP, g_offMask, g_offGw)) g_dhcpState = 2;          // bound
        // else microstk not back-filled yet: stay at state 1, worker re-REQUESTs
    }
}

// Worker: poll the link for frames + drive DHCP until bound.
static DWORD WINAPI NetWorker(LPVOID p)
{
    unsigned char buf[1600];
    DWORD lastDhcp = 0;
    (void)p;
    for (;;)
    {
        int n = s_link->poll ? s_link->poll(buf, sizeof(buf)) : 0;
        if (n > 0) { g_rxBytes += (DWORD)n; RxFrame(buf, n); }
        if (!g_haveIP && GetTickCount() - lastDhcp > 1500) { DhcpStep(); lastDhcp = GetTickCount(); }
        if (GetTickCount() - g_statTick > 2000)     // throughput tick: only while bytes are moving, so
        {                                           // a stalled stream's last line shows where rx/tx froze
            g_statTick = GetTickCount();
            if (g_rxBytes != g_rxPrev || g_txBytes != g_txPrev)
            { SysLog(L"netif: rx=%u tx=%u bytes", g_rxBytes, g_txBytes); g_rxPrev = g_rxBytes; g_txPrev = g_txBytes; }
        }
        if (n <= 0) Sleep(2);
    }
}

// ---- the export microstk calls -------------------------------------------------
// microstk deliberately calls this TWICE (two "PPP" slots). All our backend state is
// process-global (one BBA), so we bring the hardware up + start the worker exactly ONCE,
// and return a fresh distinct ifnet per call (microstk needs distinct pointers). Only
// the FIRST ifnet becomes the live RX-delivery + DHCP target (g_ifn); later ifnets stay
// inert/DOWN so microstk routes through the one that gets an IP.
static int g_hwUp;

int InterfaceInitialize(unsigned short *name, ifnet **ppIf)
{
    ifnet *ifn;
    int    i;

    if (g_useDial) return g_dialIfInit(name, ppIf);   // modem mode: every call -> original driver
    if (!g_hwUp)                                 // first call: detect + bring up hardware
    {
        SysLog(L"netif: InterfaceInitialize");
        s_link = 0;
        SysLog(L"netif: probing W5500");
        if (s_w5500.probe())      s_link = &s_w5500;   // 1st: dedicated W5500 NIC if present
        else
        {
            SysLog(L"netif: W5500 absent, probing BBA");
            if (s_bba.probe())        s_link = &s_bba;     // 2nd: Broadband Adapter (RTL8139)
            else if (LoadDialDriver()) g_useDial = 1;      // 3rd: dial-up modem -> original PPP driver
            else { s_link = &s_null; SysLog(L"netif: no adapter, loopback only"); }
        }
        g_hwUp = 1;
        // Modem path: the original mpppdial.dll owns the whole link (its own ifnet + PPP). Hand
        // every InterfaceInitialize call straight to it; our ethernet setup below is skipped.
        if (g_useDial) return g_dialIfInit(name, ppIf);
        SysLog(L"netif: link chosen, bringing up");
        // Backend probed PRESENT but bring-up failed. This is the real-HW killer: on silicon
        // BbaInit/W5500Init can fail in ways an idealized model never does (GAPS EEPROM handshake, reset
        // timing, no PHY/link). We must NOT return 0 here - microstk then wedges on a NULL
        // ifnet and the whole boot hangs/resets before the shell (same wedge the s_null probe
        // fallback below avoids). So on init failure, fall back to the no-op null link: the
        // system still boots to the desktop (network down) and the failure is logged, instead
        // of bricking the boot. This makes "BBA-only" and "W5500-only" boot-survivable.
        if (!s_link->init(g_mac))
        {
            SysLog(L"netif: %S init FAILED, null fallback", s_link->name);
            s_link = &s_null;
            s_link->init(g_mac);                 // NullInit cannot fail
        }
        else SysLog(L"netif: %S init OK MAC %02x:%02x:%02x:%02x:%02x:%02x", s_link->name,
                    g_mac[0],g_mac[1],g_mac[2],g_mac[3],g_mac[4],g_mac[5]);
        g_hwUp = 1;
        SysLog(L"netif: hardware up, worker starting");
    }

    ifn = (ifnet *)LocalAlloc(LPTR, sizeof(ifnet));
    if (!ifn) return 0;
    for (i = 0; i < 15 && name && name[i]; i++) ifn->ifn_szName[i] = name[i];
    ifn->ifn_Ioctl    = OurIoctl;
    ifn->ifn_IPOutput = OurTransmit;
    ifn->ifn_dwMTU    = 1500;
    ifn->ifn_dwFlags  = IFF_BROADCAST;          // ethernet: route by netmask, start DOWN (no IFF_UP)
    *ppIf = ifn;

    if (!g_ifn)                                 // first interface = the live one
    {
        g_ifn = ifn;
        ReadRevivalConfig();                    // revival DNS + game-master DNAT config
        if (s_link != &s_null)                  // no worker/DHCP when there's no real NIC
            CloseHandle(CreateThread(0, 0, NetWorker, 0, 0, 0));
    }
    return 1;
}

// DLL entry point AND export @16 (mppp names it "dllentry"); /entry:dllentry skips
// the CRT entry - fine here (no C++ statics; corelibc fns need no init).
BOOL WINAPI dllentry(HANDLE h, DWORD reason, LPVOID r) { (void)h; (void)reason; (void)r; return TRUE; }
