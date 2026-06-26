//
// bba.c - Dreamcast Broadband Adapter (HIT-0400, RTL8139C on the G2 bus) as a
// Dreamcast WinCE WDM driver. Provides NT-style DriverEntry, links wdm.lib (which
// supplies the DllMain that calls InitWDMDriver -> our DriverEntry), registered
// under HKLM\WDMDrivers\BuiltIn\BBA like maple.dll. NO own DllMain, no exports.
//
// Stage 2: GAPS bridge + RTL8139C init, MAC read, RX ring + TX descriptors, an RX
// worker thread that logs received frames, an ARP TX probe, and SEND/RECV IOCTLs.
// Polled (no interrupt yet). Hardware sequence ported from KallistiOS
// (broadband_adapter.c, g2bus.c, fifo.h).
//
#include <windows.h>
#include <wdm.h>

DWORD SetKMode(DWORD fMode);    // coredll export, not in the SDK public headers

#ifndef CTL_CODE
#define CTL_CODE(t,f,m,a) (((t) << 16) | ((a) << 14) | ((f) << 2) | (m))
#define METHOD_BUFFERED   0
#define FILE_ANY_ACCESS   0
#endif
#define IOCTL_BBA_GET_MAC      CTL_CODE(FILE_DEVICE_NETWORK, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_GET_PRESENT  CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_SEND         CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_RECV         CTL_CODE(FILE_DEVICE_NETWORK, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ---------------------------------------------------------------------------
// G2 bus access (P2 / uncached; drain the 32-byte G2 write FIFO before each).
// ---------------------------------------------------------------------------
#define G2_FIFO_STATUS  (*(volatile DWORD *)0xA05F688C)
#define FIFO_AICA       0x0001
#define FIFO_G2         0x0010

static void g2_fifo_wait(void)
{
    int i;
    for (i = 0; i < 0x1800; i++)
        if (!(G2_FIFO_STATUS & (FIFO_AICA | FIFO_G2)))
            break;
}

static BYTE  g2_read_8 (DWORD a) { g2_fifo_wait(); return *(volatile BYTE  *)a; }
static WORD  g2_read_16(DWORD a) { g2_fifo_wait(); return *(volatile WORD  *)a; }
static DWORD g2_read_32(DWORD a) { g2_fifo_wait(); return *(volatile DWORD *)a; }
static void  g2_write_8 (DWORD a, BYTE  v) { g2_fifo_wait(); *(volatile BYTE  *)a = v; }
static void  g2_write_16(DWORD a, WORD  v) { g2_fifo_wait(); *(volatile WORD  *)a = v; }
static void  g2_write_32(DWORD a, DWORD v) { g2_fifo_wait(); *(volatile DWORD *)a = v; }

static void g2_read_block(BYTE *dst, DWORD addr, int n)
{
    int i;
    for (i = 0; i < n; i++) { if (!(i & 31)) g2_fifo_wait(); dst[i] = *(volatile BYTE *)(addr + i); }
}
static void g2_write_block(DWORD addr, const BYTE *src, int n)
{
    int i;
    for (i = 0; i < n; i++) { if (!(i & 31)) g2_fifo_wait(); *(volatile BYTE *)(addr + i) = src[i]; }
}

// ---------------------------------------------------------------------------
// GAPS bridge + RTL8139C (from KOS broadband_adapter). RTL_DMA = chip-side DMA
// offset (GAPS-relative); RTL_CPU = the same memory in the P2 window.
// ---------------------------------------------------------------------------
#define GAPS_BASE   0xA1000000
#define RTL_DMA     0x01840000
#define RTL_CPU     0xA1840000
#define NIC(reg)    (GAPS_BASE + 0x1700 + (reg))

#define RT_IDR0      0x00
#define RT_MAR0      0x08
#define RT_MAR4      0x0c
#define RT_TXSTATUS0 0x10
#define RT_TXADDR0   0x20
#define RT_RXBUF     0x30
#define RT_CHIPCMD   0x37
#define RT_RXBUFTAIL 0x38   // CAPR
#define RT_RXBUFHEAD 0x3a   // CBR
#define RT_INTRMASK  0x3c
#define RT_INTRSTATUS 0x3e
#define RT_TXCONFIG  0x40
#define RT_RXCONFIG  0x44

#define RT_CMD_RESET     0x10
#define RT_CMD_RX_ENABLE 0x08
#define RT_CMD_TX_ENABLE 0x04
#define RT_CMD_RX_BUF_EMPTY 0x01
#define RT_TX_HOST_OWNS  0x00002000

#define RXC_RBLEN_16K (1 << 11)
#define RXC_MXDMA_1K  (6 << 8)
#define RXC_WRAP      0x80
#define RXC_AB        0x08      // accept broadcast
#define RXC_APM       0x02      // accept physical-match
#define RX_CONFIG     (RXC_RBLEN_16K | RXC_MXDMA_1K | RXC_WRAP)
#define TX_CONFIG     (6 << 8)

#define RX_BUF_LEN    0x4000    // 16K (RBLEN=1)
#define TX_OFF        (RX_BUF_LEN + 0x2000)   // 0x6000: RX(16K) + 8K wrap pad
#define TX_LEN        0x800     // 2K per TX buffer
#define TX_N          4

static BYTE  g_mac[6];
static BOOL  g_present = FALSE;
static DWORD g_curtx = 0, g_currx = 0;

static int gaps_init(void)
{
    char str[16];
    int  i;
    g2_read_block((BYTE *)str, GAPS_BASE + 0x1400, 16);
    if (memcmp(str, "GAPSPCI_BRIDGE_2", 16) != 0)
        return -1;
    g2_write_32(GAPS_BASE + 0x1418, 0x5a14a501);
    for (i = 10000; i > 0 && !(g2_read_32(GAPS_BASE + 0x1418) & 1); i--) ;
    if (!(g2_read_32(GAPS_BASE + 0x1418) & 1)) return -2;
    g2_write_32(GAPS_BASE + 0x1420, 0x01000000);
    g2_write_32(GAPS_BASE + 0x1424, 0x01000000);
    g2_write_32(GAPS_BASE + 0x1428, RTL_DMA);
    g2_write_32(GAPS_BASE + 0x142c, RTL_DMA + 32 * 1024);
    g2_write_32(GAPS_BASE + 0x1414, 0x00000001);
    g2_write_32(GAPS_BASE + 0x1434, 0x00000001);
    g2_write_16(GAPS_BASE + 0x1606, 0xf900);
    g2_write_32(GAPS_BASE + 0x1630, 0x00000000);
    g2_write_8 (GAPS_BASE + 0x163c, 0x00);
    g2_write_8 (GAPS_BASE + 0x160d, 0xf0);
    g2_write_16(GAPS_BASE + 0x1604, g2_read_16(GAPS_BASE + 0x1604) | 0x6);
    g2_write_32(GAPS_BASE + 0x1614, 0x01000000);
    if (g2_read_8(GAPS_BASE + 0x1650) & 0x1)
        g2_write_16(GAPS_BASE + 0x1654, (g2_read_16(GAPS_BASE + 0x1654) & 0xfffc) | 0x8000);
    g2_write_32(GAPS_BASE + 0x1414, 0x00000001);
    return 0;
}

static void rtl_reset(void)
{
    int i;
    g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RESET);
    for (i = 1000; i > 0 && (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET); i--) ;
}

static void rtl_read_mac(void)
{
    DWORD lo = g2_read_32(NIC(RT_IDR0));
    DWORD hi = g2_read_32(NIC(RT_IDR0 + 4));
    g_mac[0] = (BYTE)lo;        g_mac[1] = (BYTE)(lo >> 8);
    g_mac[2] = (BYTE)(lo >> 16); g_mac[3] = (BYTE)(lo >> 24);
    g_mac[4] = (BYTE)hi;        g_mac[5] = (BYTE)(hi >> 8);
}

// Bring up the RX ring + TX descriptors and enable RX/TX (KOS bba_hw_init).
static void rtl_start(void)
{
    int i;
    g2_write_32(NIC(RT_RXBUF), RTL_DMA);
    for (i = 0; i < TX_N; i++)
        g2_write_32(NIC(RT_TXADDR0 + i * 4), RTL_DMA + i * TX_LEN + TX_OFF);
    g2_write_16(NIC(RT_INTRMASK), 0);                  // polled: no interrupts
    g2_write_8 (NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
    g2_write_32(NIC(RT_RXCONFIG), RX_CONFIG | RXC_APM | RXC_AB);
    g2_write_32(NIC(RT_TXCONFIG), TX_CONFIG);
    g2_write_8 (NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
    g2_write_32(NIC(RT_MAR0), 0xffffffff);             // accept all multicast
    g2_write_32(NIC(RT_MAR4), 0xffffffff);
    g2_write_16(NIC(RT_INTRSTATUS), 0xffff);           // clear any latched status
    g_curtx = 0;
    g_currx = 0;
    g2_write_16(NIC(RT_RXBUFTAIL), (WORD)(0 - 16));
}

// Transmit one frame (<=1514 bytes). Pads runts to 60. Polled OWN wait.
static BOOL bba_tx(const BYTE *pkt, int len)
{
    DWORD tsd = NIC(RT_TXSTATUS0 + 4 * g_curtx);
    DWORD buf = RTL_CPU + g_curtx * TX_LEN + TX_OFF;
    int   i;
    if (len <= 0 || len > 1514)
        return FALSE;
    for (i = 0; i < 1000 && !(g2_read_32(tsd) & RT_TX_HOST_OWNS) && i == 0; i++)
        ;                                              // first use: OWN not yet meaningful
    g2_write_block(buf, pkt, len);
    if (len < 60) { for (i = len; i < 60; i++) g2_write_8(buf + i, 0); len = 60; }
    g2_write_32(tsd, (DWORD)len);                      // size + OWN=0 -> start TX
    g_curtx = (g_curtx + 1) & (TX_N - 1);
    return TRUE;
}

// Poll one received frame into buf. Returns payload length, 0 if none, -1 on error.
static int bba_rx_poll(BYTE *buf, int maxlen)
{
    DWORD status, off;
    int   size, pkt;
    if (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RX_BUF_EMPTY)
        return 0;
    off    = g_currx % RX_BUF_LEN;
    status = g2_read_32(RTL_CPU + off);
    size   = (int)((status >> 16) & 0xffff);
    if (size == 0xfff0)                                // early-receive: not ready
        return 0;
    pkt = size - 4;                                    // drop the 4-byte CRC
    if (!(status & 1) || pkt < 14 || pkt > 1514)
    {
        rtl_reset(); rtl_start();                      // ring desync -> re-init
        return -1;
    }
    if (pkt > maxlen) pkt = maxlen;
    g2_read_block(buf, RTL_CPU + off + 4, pkt);
    g_currx = (g_currx + size + 4 + 3) & ~3;
    g2_write_16(NIC(RT_RXBUFTAIL), (WORD)((g_currx - 16) & (RX_BUF_LEN - 1)));
    return pkt;
}

// --- big-endian helper + IPv4 header checksum (for the DHCP self-test) ---
static void be16(BYTE *p, WORD v) { p[0] = (BYTE)(v >> 8); p[1] = (BYTE)v; }
static WORD ip_csum(const BYTE *p, int n)
{
    DWORD s = 0;
    int   i;
    for (i = 0; i + 1 < n; i += 2) s += (DWORD)((p[i] << 8) | p[i + 1]);
    if (i < n) s += (DWORD)(p[i] << 8);
    while (s >> 16) s = (s & 0xffff) + (s >> 16);
    return (WORD)~s;
}

// Build a broadcast DHCP DISCOVER (returns length). Subnet-agnostic, so it works
// in Flycast's built-in BBA DHCP/NAT mode regardless of the host's subnet.
static int build_dhcp_discover(BYTE *out, DWORD xid)
{
    BYTE *eth = out, *ip = out + 14, *udp = out + 34, *bp = out + 42;
    int   dhcplen = 250, i;
    memset(out, 0, 400);
    for (i = 0; i < 6; i++) eth[i] = 0xff;                 // dst broadcast
    for (i = 0; i < 6; i++) eth[6 + i] = g_mac[i];
    be16(eth + 12, 0x0800);
    bp[0] = 1; bp[1] = 1; bp[2] = 6;                       // op=BOOTREQUEST htype hlen
    bp[4] = (BYTE)(xid >> 24); bp[5] = (BYTE)(xid >> 16); bp[6] = (BYTE)(xid >> 8); bp[7] = (BYTE)xid;
    be16(bp + 10, 0x8000);                                 // flags = broadcast
    for (i = 0; i < 6; i++) bp[28 + i] = g_mac[i];         // chaddr
    bp[236] = 0x63; bp[237] = 0x82; bp[238] = 0x53; bp[239] = 0x63;  // magic cookie
    bp[240] = 53; bp[241] = 1; bp[242] = 1;                // option 53: DHCP DISCOVER
    bp[243] = 55; bp[244] = 4; bp[245] = 1; bp[246] = 3; bp[247] = 6; bp[248] = 15;  // param req
    bp[249] = 255;                                         // end
    be16(udp + 0, 68); be16(udp + 2, 67);                  // UDP src 68 -> dst 67
    be16(udp + 4, (WORD)(8 + dhcplen)); be16(udp + 6, 0);  // len, csum 0 (legal for IPv4)
    ip[0] = 0x45; be16(ip + 2, (WORD)(20 + 8 + dhcplen));  // ver/ihl, total len
    ip[8] = 128; ip[9] = 17;                               // ttl, proto UDP
    for (i = 0; i < 4; i++) ip[16 + i] = 0xff;             // dst 255.255.255.255 (src stays 0)
    be16(ip + 10, ip_csum(ip, 20));
    return 14 + 20 + 8 + dhcplen;
}

// If frame is a DHCP reply (UDP->68, BOOTREPLY), return yiaddr ("your" IP), else 0.
static DWORD parse_dhcp_offer(const BYTE *f, int n)
{
    if (n < 62) return 0;
    if (f[12] != 0x08 || f[13] != 0x00) return 0;          // IPv4
    if (f[23] != 17) return 0;                             // UDP
    if (f[36] != 0 || f[37] != 68) return 0;               // UDP dst port 68
    if (f[42] != 2) return 0;                              // BOOTREPLY
    return ((DWORD)f[58] << 24) | ((DWORD)f[59] << 16) | ((DWORD)f[60] << 8) | f[61];  // yiaddr
}

// RX worker: poll the ring, log the first frames seen (proof of RX on Flycast's
// bridged network), and periodically send an ARP probe (proof of TX).
static DWORD WINAPI BbaRxThread(LPVOID param)
{
    static BYTE frame[1600];
    static BYTE disc[400];
    int   logged = 0, poll = 0, dlen, gotip = 0;
    DWORD total = 0;

    SetKMode(TRUE);
    dlen = build_dhcp_discover(disc, 0xDC0DC0DCu);

    for (;;)
    {
        int n = bba_rx_poll(frame, sizeof(frame));
        if (n > 14)
        {
            DWORD yi;
            total++;
            if (logged < 8)
            {
                WCHAR b[112];
                wsprintfW(b, L"BBA RX[%u]: len=%d src=%02x:%02x:%02x:%02x:%02x:%02x type=%04x\r\n",
                          (unsigned)total, n, frame[6], frame[7], frame[8], frame[9], frame[10], frame[11],
                          (frame[12] << 8) | frame[13]);
                OutputDebugStringW(b);
                if (++logged == 8) OutputDebugStringW(L"BBA RX: (further frames counted silently)\r\n");
            }
            yi = parse_dhcp_offer(frame, n);
            if (yi && !gotip)
            {
                WCHAR b[72];
                wsprintfW(b, L"BBA: *** DHCP reply - your IP = %u.%u.%u.%u ***\r\n",
                          (unsigned)((yi >> 24) & 0xff), (unsigned)((yi >> 16) & 0xff),
                          (unsigned)((yi >> 8) & 0xff), (unsigned)(yi & 0xff));
                OutputDebugStringW(b);
                gotip = 1;                              // round-trip proven; stop probing
            }
        }
        else
        {
            if (!gotip && (poll++ % 200) == 0)          // ~every 2s until we get a reply
            {
                bba_tx(disc, dlen);
                OutputDebugStringW(L"BBA TX: DHCP DISCOVER\r\n");
            }
            Sleep(10);
        }
    }
    return 0;
}

static BOOL bba_bringup(void)
{
    DWORD prev = SetKMode(TRUE);
    BOOL  ok = FALSE;
    WCHAR b[80];
    __try
    {
        if (gaps_init() == 0)
        {
            rtl_reset();
            rtl_read_mac();
            rtl_start();
            ok = TRUE;
            wsprintfW(b, L"BBA: up, MAC %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                      g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
            OutputDebugStringW(b);
        }
        else
            OutputDebugStringW(L"BBA: no GAPS bridge / init failed (adapter present?)\r\n");
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        OutputDebugStringW(L"BBA: probe faulted (G2 access) - skipping\r\n");
    }
    SetKMode(prev);
    return ok;
}

// ---------------------------------------------------------------------------
// WDM dispatch (buffered I/O).
// ---------------------------------------------------------------------------
static NTSTATUS BbaDispatch(PDEVICE_OBJECT dev, PIRP irp)
{
    PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS st = STATUS_SUCCESS;
    ULONG    info = 0;

    if (sp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
    {
        ULONG code   = sp->Parameters.DeviceIoControl.IoControlCode;
        ULONG inlen  = sp->Parameters.DeviceIoControl.InputBufferLength;
        ULONG outlen = sp->Parameters.DeviceIoControl.OutputBufferLength;
        PVOID buf    = irp->AssociatedIrp.SystemBuffer;
        DWORD prev;

        if (code == IOCTL_BBA_GET_MAC && buf && outlen >= 6)
        { memcpy(buf, g_mac, 6); info = 6; }
        else if (code == IOCTL_BBA_GET_PRESENT && buf && outlen >= 4)
        { *(DWORD *)buf = g_present ? 1 : 0; info = 4; }
        else if (code == IOCTL_BBA_SEND && buf && inlen >= 14 && g_present)
        { prev = SetKMode(TRUE); st = bba_tx((BYTE *)buf, (int)inlen) ? STATUS_SUCCESS : STATUS_IO_DEVICE_ERROR; SetKMode(prev); }
        else if (code == IOCTL_BBA_RECV && buf && outlen >= 14 && g_present)
        { int n; prev = SetKMode(TRUE); n = bba_rx_poll((BYTE *)buf, (int)outlen); SetKMode(prev); info = (n > 0) ? (ULONG)n : 0; }
        else
            st = STATUS_NOT_IMPLEMENTED;
    }

    irp->IoStatus.Status = st;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, 0);
    return st;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING regpath)
{
    UNICODE_STRING name;
    PDEVICE_OBJECT dev = NULL;
    NTSTATUS       st;
    int            i;

    OutputDebugStringW(L"BBA DriverEntry\r\n");
    g_present = bba_bringup();

    RtlInitUnicodeString(&name, L"\\Device\\BBA1");
    st = IoCreateDevice(drv, 0, &name, FILE_DEVICE_NETWORK, 0, FALSE, &dev);
    if (!NT_SUCCESS(st)) { OutputDebugStringW(L"BBA: IoCreateDevice failed\r\n"); return st; }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        drv->MajorFunction[i] = BbaDispatch;

    if (g_present)
    {
        HANDLE h = CreateThread(NULL, 0, BbaRxThread, NULL, 0, NULL);
        if (h) CloseHandle(h);
        OutputDebugStringW(L"BBA: RX worker started\r\n");
    }
    OutputDebugStringW(L"BBA: device created (DriverEntry ok)\r\n");
    return STATUS_SUCCESS;
}

// No DllMain: wdm.lib provides it (it calls InitWDMDriver -> DriverEntry).
