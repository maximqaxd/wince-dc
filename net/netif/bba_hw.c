//
// bba_hw.c - RTL8139C (Dreamcast Broadband Adapter, GAPS bridge on the G2 bus)
// hardware behind the netif.c LinkOps. Factored from drivers/bba/bba.c (the WDM
// driver); here it is plain code linked into the microstk link DLL (mppp.dll), with
// SetKMode(TRUE) around every G2 access so it works from any caller thread.
//
#include <windows.h>

DWORD SetKMode(DWORD fMode);          // coredll export (P2/G2 needs kernel mode)

#define G2_FIFO_STATUS  (*(volatile DWORD *)0xA05F688C)
#define FIFO_AICA       0x0001
#define FIFO_G2         0x0010
static void g2_fifo_wait(void) { int i; for (i = 0; i < 0x1800; i++) if (!(G2_FIFO_STATUS & (FIFO_AICA | FIFO_G2))) break; }
static BYTE  g2_read_8 (DWORD a) { g2_fifo_wait(); return *(volatile BYTE  *)a; }
static WORD  g2_read_16(DWORD a) { g2_fifo_wait(); return *(volatile WORD  *)a; }
static DWORD g2_read_32(DWORD a) { g2_fifo_wait(); return *(volatile DWORD *)a; }
static void  g2_write_8 (DWORD a, BYTE  v) { g2_fifo_wait(); *(volatile BYTE  *)a = v; }
static void  g2_write_16(DWORD a, WORD  v) { g2_fifo_wait(); *(volatile WORD  *)a = v; }
static void  g2_write_32(DWORD a, DWORD v) { g2_fifo_wait(); *(volatile DWORD *)a = v; }
static void g2_read_block(BYTE *d, DWORD a, int n)  { int i; for (i = 0; i < n; i++) { if (!(i & 31)) g2_fifo_wait(); d[i] = *(volatile BYTE *)(a + i); } }
static void g2_write_block(DWORD a, const BYTE *s, int n) { int i; for (i = 0; i < n; i++) { if (!(i & 31)) g2_fifo_wait(); *(volatile BYTE *)(a + i) = s[i]; } }

#define GAPS_BASE   0xA1000000
#define RTL_DMA     0x01840000
#define RTL_CPU     0xA1840000
#define NIC(reg)    (GAPS_BASE + 0x1700 + (reg))
#define RT_IDR0 0x00
#define RT_MAR0 0x08
#define RT_MAR4 0x0c
#define RT_TXSTATUS0 0x10
#define RT_TXADDR0 0x20
#define RT_RXBUF 0x30
#define RT_CHIPCMD 0x37
#define RT_RXBUFTAIL 0x38
#define RT_INTRMASK 0x3c
#define RT_INTRSTATUS 0x3e
#define RT_TXCONFIG 0x40
#define RT_RXCONFIG 0x44
#define RT_CMD_RESET 0x10
#define RT_CMD_RX_ENABLE 0x08
#define RT_CMD_TX_ENABLE 0x04
#define RT_CMD_RX_BUF_EMPTY 0x01
#define RT_TX_HOST_OWNS 0x00002000
#define RXC_RBLEN_16K (1 << 11)
#define RXC_MXDMA_1K (6 << 8)
#define RXC_WRAP 0x80
#define RXC_AB 0x08
#define RXC_APM 0x02
#define RX_CONFIG (RXC_RBLEN_16K | RXC_MXDMA_1K | RXC_WRAP)
#define TX_CONFIG (6 << 8)
#define RX_BUF_LEN 0x4000
#define TX_OFF (RX_BUF_LEN + 0x2000)
#define TX_LEN 0x800
#define TX_N 4

static BYTE  s_mac[6];
static DWORD g_curtx, g_currx;
static CRITICAL_SECTION s_g2cs;          // serialize G2 + ring state (TX thread vs RX worker)
static int   s_g2csInit;

static int gaps_init(void)
{
    char str[16]; int i;
    g2_read_block((BYTE *)str, GAPS_BASE + 0x1400, 16);
    if (memcmp(str, "GAPSPCI_BRIDGE_2", 16) != 0) return -1;
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
static void rtl_reset(void) { int i; g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RESET); for (i = 1000; i > 0 && (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET); i--) ; }
static void rtl_read_mac(void)
{
    DWORD lo = g2_read_32(NIC(RT_IDR0)), hi = g2_read_32(NIC(RT_IDR0 + 4));
    s_mac[0]=(BYTE)lo; s_mac[1]=(BYTE)(lo>>8); s_mac[2]=(BYTE)(lo>>16); s_mac[3]=(BYTE)(lo>>24);
    s_mac[4]=(BYTE)hi; s_mac[5]=(BYTE)(hi>>8);
}
static void rtl_start(void)
{
    int i;
    g2_write_32(NIC(RT_RXBUF), RTL_DMA);
    for (i = 0; i < TX_N; i++) g2_write_32(NIC(RT_TXADDR0 + i * 4), RTL_DMA + i * TX_LEN + TX_OFF);
    g2_write_16(NIC(RT_INTRMASK), 0);
    g2_write_8 (NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
    g2_write_32(NIC(RT_RXCONFIG), RX_CONFIG | RXC_APM | RXC_AB);
    g2_write_32(NIC(RT_TXCONFIG), TX_CONFIG);
    g2_write_8 (NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
    g2_write_32(NIC(RT_MAR0), 0xffffffff);
    g2_write_32(NIC(RT_MAR4), 0xffffffff);
    g2_write_16(NIC(RT_INTRSTATUS), 0xffff);
    g_curtx = 0; g_currx = 0;
    g2_write_16(NIC(RT_RXBUFTAIL), (WORD)(0 - 16));
}
static BOOL tx(const BYTE *pkt, int len)
{
    DWORD tsd = NIC(RT_TXSTATUS0 + 4 * g_curtx), buf = RTL_CPU + g_curtx * TX_LEN + TX_OFF;
    int i;
    if (len <= 0 || len > 1514) return FALSE;
    g2_write_block(buf, pkt, len);
    if (len < 60) { for (i = len; i < 60; i++) g2_write_8(buf + i, 0); len = 60; }
    g2_write_32(tsd, (DWORD)len);
    g_curtx = (g_curtx + 1) & (TX_N - 1);
    return TRUE;
}
static int rx(BYTE *buf, int maxlen)
{
    DWORD status, off; int size, pkt;
    if (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RX_BUF_EMPTY) return 0;
    off = g_currx % RX_BUF_LEN;
    status = g2_read_32(RTL_CPU + off);
    size = (int)((status >> 16) & 0xffff);
    if (size == 0xfff0) return 0;
    pkt = size - 4;
    if (!(status & 1) || pkt < 14 || pkt > 1514) { rtl_reset(); rtl_start(); return -1; }
    if (pkt > maxlen) pkt = maxlen;
    g2_read_block(buf, RTL_CPU + off + 4, pkt);
    g_currx = (g_currx + size + 4 + 3) & ~3;
    g2_write_16(NIC(RT_RXBUFTAIL), (WORD)((g_currx - 16) & (RX_BUF_LEN - 1)));
    return pkt;
}

// ---- LinkOps hooks (SetKMode-wrapped) ------------------------------------------
int BbaProbe(void)
{
    DWORD prev = SetKMode(TRUE);
    char  str[16];
    int   ok = 0;
    __try { g2_read_block((BYTE *)str, GAPS_BASE + 0x1400, 16); ok = (memcmp(str, "GAPSPCI_BRIDGE_2", 16) == 0); }
    __except (EXCEPTION_EXECUTE_HANDLER) { ok = 0; }
    SetKMode(prev);
    return ok;
}
int BbaInit(unsigned char mac[6])
{
    DWORD prev;
    int   ok = 0;
    if (!s_g2csInit) { InitializeCriticalSection(&s_g2cs); s_g2csInit = 1; }   // before any TX/RX
    prev = SetKMode(TRUE);
    __try { if (gaps_init() == 0) { rtl_reset(); rtl_read_mac(); rtl_start(); memcpy(mac, s_mac, 6); ok = 1; } }
    __except (EXCEPTION_EXECUTE_HANDLER) { ok = 0; }
    SetKMode(prev);
    return ok;
}
int BbaTx(const unsigned char *frame, int len)
{
    DWORD prev;
    int   ok;
    EnterCriticalSection(&s_g2cs);
    prev = SetKMode(TRUE);
    ok = tx((const BYTE *)frame, len);
    SetKMode(prev);
    LeaveCriticalSection(&s_g2cs);
    return ok;
}
int BbaRxPoll(unsigned char *buf, int max)
{
    DWORD prev;
    int   n;
    EnterCriticalSection(&s_g2cs);
    prev = SetKMode(TRUE);
    n = rx((BYTE *)buf, max);
    SetKMode(prev);
    LeaveCriticalSection(&s_g2cs);
    return n;
}
