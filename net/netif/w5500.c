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
#include "dcspi.h"               // DCSPI_BUS_* / DCSPI_CS_* constants only
#include "syslog.h"
#include "dcboot.h"

// dcspi.dll entry points, bound on demand (no static import).
typedef int           (*PFN_Init)(int, int);
typedef void          (*PFN_Shutdown)(int);
typedef void          (*PFN_SetCS)(int, int);
typedef void          (*PFN_RwData)(int, const unsigned char *, unsigned char *, int);
typedef int           (*PFN_HwType)(void);
static PFN_Init     pInit;
static PFN_Shutdown pShutdown;
static PFN_SetCS    pSetCS;
static PFN_RwData   pRw;
static PFN_HwType   pHwType;

// Serialize every W5500 SPI transaction: the NetWorker RX-poll thread and microstk's TX path
// (connect/send -> OurTransmit -> W5500Tx) hit the chip from DIFFERENT threads. Without this
// their SCIF bit-bang interleaves at the CS/clock level and corrupts frames - a probabilistic
// race that lets DNS + the fast/close 1.1.1.1 survive but drops slower multi-round-trip
// connects. Mirrors KOS's w5500_spi_mutex and our own BBA s_g2cs. (CE critical sections are
// recursive, so nested w5_* calls are safe.)
static CRITICAL_SECTION s_w5cs;
static int              s_w5csInit;
#define W5LOCK()   EnterCriticalSection(&s_w5cs)
#define W5UNLOCK() LeaveCriticalSection(&s_w5cs)

static int load_dcspi(void)
{
    HMODULE h;
    if (!s_w5csInit) { InitializeCriticalSection(&s_w5cs); s_w5csInit = 1; }   // before any w5_* access
    if (pInit) return 1;
    h = LoadLibraryW(L"dcspi.dll");
    if (!h) return 0;
    pInit     = (PFN_Init)    GetProcAddress(h, L"SpiInit");
    pShutdown = (PFN_Shutdown)GetProcAddress(h, L"SpiShutdown");
    pSetCS    = (PFN_SetCS)   GetProcAddress(h, L"SpiSetCS");
    pRw       = (PFN_RwData)  GetProcAddress(h, L"SpiRwData");
    pHwType   = (PFN_HwType)  GetProcAddress(h, L"DcspiHwType");   // optional (Naomi vs DC)
    return (pInit && pSetCS && pRw) ? 1 : 0;
}

// ---- W5500 block-select (BSB) + control byte -----------------------------------
#define BSB_COMMON  0x00
#define BSB_S0_REG  0x01
#define BSB_S0_TX   0x02
#define BSB_S0_RX   0x03
#define CTRL(bsb, wr) ((BYTE)(((bsb) << 3) | ((wr) ? 0x04 : 0x00)))   // OM=00 (VDM)

#define MR        0x0000
#define SHAR      0x0009
#define PHYCFGR   0x002E         // common: PHY config (link/auto-neg)
#define VERSIONR  0x0039
#define PHYCFG_RST_AN 0xB8       // RST(no)|OPMD|OPMDC=AllAuto = software-override auto-neg
#define PHY_LINK_UP   0x01       // PHYCFGR bit0: link present
#define Sn_MR     0x0000
#define Sn_CR     0x0001
#define Sn_SR     0x0003
#define Sn_RXBUF  0x001E
#define Sn_TXBUF  0x001F
#define Sn_TX_FSR 0x0020
#define Sn_TX_WR  0x0024
#define Sn_RX_RSR 0x0026
#define Sn_RX_RD  0x0028
#define CMD_OPEN  0x01
#define CMD_SEND  0x20
#define CMD_RECV  0x40
#define SOCK_MACRAW 0x42
#define MR_MACRAW_MF 0x84

static unsigned char g_w5mac[6] = { 0x02, 0xDC, 0x00, 0x00, 0x00, 0x01 };
static int g_bus = -1, g_cs;

// ---- raw register access via dcspi ----------------------------------------------
static void w5_write(BYTE bsb, WORD addr, const BYTE *data, int len)
{
    BYTE hdr[3];
    hdr[0] = (BYTE)(addr >> 8); hdr[1] = (BYTE)addr; hdr[2] = CTRL(bsb, 1);
    W5LOCK();
    pSetCS(g_bus, 1);
    pRw(g_bus, hdr, 0, 3);
    if (len) pRw(g_bus, data, 0, len);
    pSetCS(g_bus, 0);
    W5UNLOCK();
}
static void w5_read(BYTE bsb, WORD addr, BYTE *data, int len)
{
    BYTE hdr[3];
    hdr[0] = (BYTE)(addr >> 8); hdr[1] = (BYTE)addr; hdr[2] = CTRL(bsb, 0);
    W5LOCK();
    pSetCS(g_bus, 1);
    pRw(g_bus, hdr, 0, 3);
    if (len) pRw(g_bus, 0, data, len);
    pSetCS(g_bus, 0);
    W5UNLOCK();
}
static void w5_w1(BYTE bsb, WORD a, BYTE v)  { w5_write(bsb, a, &v, 1); }
static BYTE w5_r1(BYTE bsb, WORD a)          { BYTE v = 0; w5_read(bsb, a, &v, 1); return v; }
static void w5_w16(BYTE bsb, WORD a, WORD v) { BYTE b[2]; b[0]=(BYTE)(v>>8); b[1]=(BYTE)v; w5_write(bsb, a, b, 2); }
static WORD w5_r16(BYTE bsb, WORD a)         { BYTE b[2]; w5_read(bsb, a, b, 2); return (WORD)((b[0]<<8)|b[1]); }
// Stable 16-bit read for the chip-updated pointers (Sn_TX_FSR / Sn_RX_RSR): the W5500's
// MAC engine updates these asynchronously, so a single SPI read can latch a high byte from
// before an update and a low byte from after (a torn value). Loop until two CONSECUTIVE
// reads agree (a safe 16-bit register read). The old code gave up after 4 tries and returned
// a possibly-unstable value -> bogus FSR/RSR -> TX ring overrun / RX ring desync on real HW
// (idealized models update atomically, so it never bit there). Bounded at 16 to never hang.
static WORD w5_r16s(BYTE bsb, WORD a)
{
    WORD x, y = w5_r16(bsb, a);
    int  i;
    for (i = 0; i < 16; i++) { x = y; y = w5_r16(bsb, a); if (x == y) return y; }
    return y;
}

// ---- LinkOps ---------------------------------------------------------------------
static int w5_try(int bus, int cs)
{
    BYTE ver;
    SysLog(L"w5500: probe bus=%d cs=%d", bus, cs);        // (SCI init drives Port A; SCIF reconfigs SCIF)
    if (pInit(bus, cs) != 0) { SysLog(L"w5500: SpiInit FAILED bus=%d", bus); return 0; }
    g_bus = bus; g_cs = cs;
    ver = w5_r1(BSB_COMMON, VERSIONR);
    SysLog(L"w5500: VERSIONR=0x%02x bus=%d (expect 0x04)", ver, bus);
    if (ver == 0x04)
    {
        SysLog(L"w5500: DETECTED on bus %d", bus);
        DcBootSet(DCB_NET, DCB_OK, bus == DCSPI_BUS_SCIF ? L"W5500 (SCIF)" : L"W5500 (SCI)");
        return 1;
    }
    if (pShutdown) pShutdown(bus);
    g_bus = -1;
    return 0;
}

// Read the configured bus. HKLM\Comm\Netif : "W5500Bus" (REG_DWORD):
//   0 / absent = disabled (safe default)
//   1 = SCI  (bus 1, hardware sync-SPI, CS on PA7 GPIO)
//   2 = SCIF (bus 2, bit-bang SPI on SCSPTR2, CS on RTS)
//   3 = AUTO (probe SCI first, then SCIF)
static DWORD cfg_bus(void)
{
    HKEY  h;
    DWORD v = 0, type, sz = sizeof(v);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm\\Netif", 0, KEY_QUERY_VALUE, &h) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(h, L"W5500Bus", 0, &type, (BYTE *)&v, &sz) != ERROR_SUCCESS) v = 0;
        RegCloseKey(h);
    }
    return v;
}

int W5500Probe(void)
{
    DWORD bus = cfg_bus();
    SysLog(L"w5500: W5500Bus=%d (0=off 1=SCI 2=SCIF 3=auto)", bus);
    if (bus == 0) return 0;                               // not configured -> skip (safe default)
    if (!load_dcspi()) { SysLog(L"w5500: dcspi.dll load FAILED"); return 0; }
    if (pHwType) { int ty = pHwType();                    // log board: Naomi uses SCIF-RTS CS on SCI
        SysLog(L"w5500: hw=%s type=0x%x", ty == 0xA ? L"NAOMI" : ty == 0x9 ? L"Set5" : L"Dreamcast", ty); }
    // SCI CS source is AUTO: PA7 GPIO on retail DC, SCIF RTS on Naomi/Set5 (resolved in dcspi).
    if (bus == 1) return w5_try(DCSPI_BUS_SCI,  DCSPI_CS_AUTO);
    if (bus == 2) return w5_try(DCSPI_BUS_SCIF, DCSPI_CS_RTS);
    if (bus == 3)                                         // AUTO-detect which bus the chip is on
    {
        // SCI FIRST: probing SCI only touches the SCI + PA7/RTS, never the SCIF data pins, so the
        // nkscifkd debug console (which lives on the SCIF) survives if the chip is on SCI.
        SysLog(L"w5500: auto-probe SCI first...");
        if (w5_try(DCSPI_BUS_SCI, DCSPI_CS_AUTO)) return 1;
        // SCIF LAST: scif_init_raw reconfigures the SCIF to GPIO/SPI, which TAKES OVER the
        // SCIF and kills the kernel text console. Only do this if SCI had no chip.
        SysLog(L"w5500: auto-probe SCIF (takes over SCIF console)...");
        if (w5_try(DCSPI_BUS_SCIF, DCSPI_CS_RTS)) return 1;
        SysLog(L"w5500: not found on SCI or SCIF");
    }
    return 0;
}

int W5500Init(unsigned char mac[6])
{
    int t, i;
    if (g_bus < 0) return 0;

    // Soft reset, then WAIT for MR.RST to clear with a real delay (PLL/PHY settle). The old
    // tight 1000-spin (no delay) could fall through before reset finished on real silicon and
    // then configure a chip mid-reset (writes lost). Bail if it never clears.
    w5_w1(BSB_COMMON, MR, 0x80);
    for (t = 0; t < 100 && (w5_r1(BSB_COMMON, MR) & 0x80); t++) Sleep(1);
    if (w5_r1(BSB_COMMON, MR) & 0x80) { OutputDebugStringW(L"w5500: soft-reset stuck\r\n"); return 0; }

    w5_write(BSB_COMMON, SHAR, g_w5mac, 6);              // our MAC

    // Force the PHY to software-override auto-negotiation. On real boards the PHY power-up
    // mode is pin-strapped; without this it may never link. (0x00 then 0xB8 per datasheet.)
    w5_w1(BSB_COMMON, PHYCFGR, 0x00); Sleep(1);
    w5_w1(BSB_COMMON, PHYCFGR, PHYCFG_RST_AN);

    // Zero ALL 8 sockets' RX/TX buffer sizes (power-on default is 2K each = 16K used) BEFORE
    // giving socket 0 the full 16K. Setting S0=16 without zeroing the rest requests 16+7*2=30K
    // > the chip's 16K -> overlapping buffers, corrupt MACRAW reads/writes (the audit found
    // this; masked by an idealized buffer model). Block N reg-block = 1 + 4*N.
    for (i = 0; i < 8; i++) { BYTE blk = (BYTE)(1 + 4 * i);
        w5_w1(blk, Sn_RXBUF, 0); w5_w1(blk, Sn_TXBUF, 0); }
    w5_w1(BSB_S0_REG, Sn_RXBUF, 16);
    w5_w1(BSB_S0_REG, Sn_TXBUF, 16);

    w5_w1(BSB_S0_REG, Sn_MR, MR_MACRAW_MF);             // MACRAW + MAC filter
    w5_w1(BSB_S0_REG, Sn_CR, CMD_OPEN);
    for (t = 0; t < 1000 && w5_r1(BSB_S0_REG, Sn_CR); t++) ;
    if (w5_r1(BSB_S0_REG, Sn_SR) != SOCK_MACRAW) { OutputDebugStringW(L"w5500: MACRAW open failed\r\n"); return 0; }
    memcpy(mac, g_w5mac, 6);

    // Best-effort: note the PHY link bit (don't block boot on it - the netif DHCP worker
    // retries, so the lease lands once the link comes up).
    { WCHAR b[80]; int up = (w5_r1(BSB_COMMON, PHYCFGR) & PHY_LINK_UP) ? 1 : 0;
      wsprintfW(b, L"w5500: up on bus %d link=%d MAC=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
        g_bus, up, g_w5mac[0],g_w5mac[1],g_w5mac[2],g_w5mac[3],g_w5mac[4],g_w5mac[5]); OutputDebugStringW(b); }
    return 1;
}

int W5500Tx(const unsigned char *frame, int len)
{
    WORD fsr, wr;
    int  t = 0;
    if (len <= 0 || len > 1514) return 0;
    do { fsr = w5_r16s(BSB_S0_REG, Sn_TX_FSR); } while (fsr < (WORD)len && ++t < 20000);
    if (fsr < (WORD)len) return 0;
    wr = w5_r16(BSB_S0_REG, Sn_TX_WR);
    w5_write(BSB_S0_TX, wr, frame, len);                  // chip wraps the ring internally
    w5_w16(BSB_S0_REG, Sn_TX_WR, (WORD)(wr + len));
    w5_w1(BSB_S0_REG, Sn_CR, CMD_SEND);
    for (t = 0; t < 100000 && w5_r1(BSB_S0_REG, Sn_CR); t++) ;
    if (w5_r1(BSB_S0_REG, Sn_CR)) return 0;               // SEND never accepted -> report failure
    return 1;
}

int W5500RxPoll(unsigned char *buf, int max)
{
    WORD rsr, rd, plen;
    int  flen;
    BYTE hdr[2];
    rsr = w5_r16s(BSB_S0_REG, Sn_RX_RSR);
    if (rsr < 2) return 0;
    rd = w5_r16(BSB_S0_REG, Sn_RX_RD);
    w5_read(BSB_S0_RX, rd, hdr, 2);                       // MACRAW per-packet 2-byte length (incl header)
    plen = (WORD)((hdr[0] << 8) | hdr[1]);
    if (plen < 3 || plen > rsr)                           // ring desync -> resync to write ptr
    {
        w5_w16(BSB_S0_REG, Sn_RX_RD, (WORD)(rd + rsr));
        w5_w1(BSB_S0_REG, Sn_CR, CMD_RECV);
        return -1;
    }
    flen = plen - 2;
    if (flen > max) flen = max;
    w5_read(BSB_S0_RX, (WORD)(rd + 2), buf, flen);
    w5_w16(BSB_S0_REG, Sn_RX_RD, (WORD)(rd + plen));      // advance past the whole packet
    w5_w1(BSB_S0_REG, Sn_CR, CMD_RECV);
    return flen;
}
