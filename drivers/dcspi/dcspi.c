//
// dcspi.c - Dreamcast SPI transport (SCI hardware sync-serial + SCIF bit-bang).
// Ported from the reference SCI + SCIF-SPI drivers. Exposed as a reusable
// exporting DLL (see dcspi.h / dcspi.def). All P4 control-register access is wrapped
// in SetKMode(TRUE) so it works from any caller (driver thread, microstk thread).
//
#include <windows.h>
#include "dcspi.h"

DWORD SetKMode(DWORD fMode);            // coredll export (P4 control regs need kernel mode)

// ---- SCIF (bit-bang on SCSPTR2) -------------------------------------------------
#define R8(a)  (*(volatile BYTE  *)(a))
#define R16(a) (*(volatile WORD  *)(a))
#define R32(a) (*(volatile DWORD *)(a))

#define SCSMR2  R16(0xFFE80000)
#define SCBRR2  R8 (0xFFE80004)
#define SCSCR2  R16(0xFFE80008)
#define SCFSR2  R16(0xFFE80010)
#define SCFCR2  R16(0xFFE80018)
#define SCSPTR2 R16(0xFFE80020)
#define SCLSR2  R16(0xFFE80024)
#define SCFCR_MCE 0x08          // SCFCR2 modem-control-enable: HW auto-drives RTS. Clear it to use
                                // RTS as a plain GPIO chip-select (required on Naomi SCI-SPI).
#define SYSMODE R32(0xA05F74B0) // SH-4 system-mode reg: type=(v>>4)&0xF (0=retail DC,9=Set5,0xA=Naomi)
#define P2_RTSIO  0x80
#define P2_RTSDT  0x40
#define P2_CTSIO  0x20
#define P2_CTSDT  0x10
#define P2_SPB2IO 0x02
#define P2_SPB2DT 0x01

// ---- SCI (SH-4 clocked-synchronous = hardware SPI) ------------------------------
#define SCSMR1  R8(0xFFE00000)
#define SCBRR1  R8(0xFFE00004)
#define SCSCR1  R8(0xFFE00008)
#define SCTDR1  R8(0xFFE0000C)
#define SCSSR1  R8(0xFFE00010)
#define SCRDR1  R8(0xFFE00014)
#define SCSPTR1 R8(0xFFE00018)
#define STBCR   R8(0xFFC00004)
#define PCTRA   R32(0xFF80002C)
#define PDTRA   R16(0xFF800030)
#define SC_TDRE 0x80
#define SC_RDRF 0x40
#define SC_ORER 0x20
#define SCSSR_FER 0x10          // SCSSR1 framing error (distinct from SCSCR1 RE=0x10)
#define SCSSR_PER 0x08          // SCSSR1 parity error
#define SC_TE   0x20
#define SC_RE   0x10
#define SC_CA   0x80            // SCSMR1: synchronous (clocked) mode
#define STBCR_SCI 0x01          // SCI module standby bit
#define PA7_BIT   7
#define SCI_BRR   1             // n=0 -> 6.25 MHz (50MHz/(4*(BRR+1))); =3 was 3.125, =0 is 12.5
                                // 6.25 MHz halves per-frame SPI time (W5500 handles >25 MHz) so
                                // full-size RX frames drain before the 16K W5500 ring overflows.
#define SCI_WAIT  500000

static int s_scifUp, s_sciUp, s_sciCs;
static WORD s_scsptr2;          // shadow of SCSPTR2 (SCIF)

static BYTE bitrev8(BYTE b)
{
    b = (BYTE)((b & 0xF0) >> 4 | (b & 0x0F) << 4);
    b = (BYTE)((b & 0xCC) >> 2 | (b & 0x33) << 2);
    b = (BYTE)((b & 0xAA) >> 1 | (b & 0x55) << 1);
    return b;
}

// The SCIF is shared with the kernel debug console (nkscifkd logs text via SCFTDR2).
// Switching it to GPIO/SPI kills that console, so snapshot the UART regs on init and
// restore them on shutdown: a FAILED SCIF probe (no chip) then leaves the console intact.
// (If a chip IS found we keep SPI mode until shutdown -- the W5500 owns the SCIF.)
static WORD s_savSmr, s_savScr, s_savFcr, s_savPtr, s_savLsr;
static BYTE s_savBrr;
static int  s_scifSaved;
static void scif_save_console(void)
{
    s_savSmr = SCSMR2; s_savBrr = SCBRR2; s_savScr = SCSCR2;
    s_savFcr = SCFCR2; s_savPtr = SCSPTR2; s_savLsr = SCLSR2; s_scifSaved = 1;
}
static void scif_restore_console(void)
{
    if (!s_scifSaved) return;
    SCSCR2 = 0;                             // stop TX/RX before reprogramming
    SCFCR2 = 0x06; SCFCR2 = s_savFcr;       // reset then restore the FIFOs
    SCSMR2 = s_savSmr; SCBRR2 = s_savBrr; SCSPTR2 = s_savPtr; SCLSR2 = s_savLsr;
    SCSCR2 = s_savScr;                      // re-enable TX (console resumes)
    s_scifSaved = 0;
}

// ---- SCIF bit-bang (clock=CTSDT, MOSI/MISO=SPB2DT, CS=RTSDT), SPI mode 0 --------
static void scif_init_raw(void)
{
    scif_save_console();                    // so a failed probe can resurrect the console
    SCSCR2 = 0;
    SCFCR2 = 0x06;                          // flush FIFOs
    SCFCR2 = 0;
    SCSMR2 = 0;
    SCFSR2 = 0;
    SCLSR2 = 0;
    s_scsptr2 = P2_RTSIO | P2_RTSDT | P2_CTSIO | P2_SPB2IO;   // RTS/CTS out, CS high, data out
    SCSPTR2 = s_scsptr2;
}
static void scif_setcs_raw(int active)      // CS active-low: assert -> RTSDT 0
{
    if (active) s_scsptr2 &= ~P2_RTSDT;
    else        s_scsptr2 |=  P2_RTSDT;
    SCSPTR2 = s_scsptr2;
}
// Inter-edge settle for the SCIF bit-bang. KOS ships scif_spi_slow_rw_byte (1.5us/edge)
// because back-to-back SCSPTR2 stores clock the bus faster than some W5500 wiring can meet
// MISO setup/hold: a short VERSIONR read survives but a long MACRAW burst sporadically
// samples MISO before the W5500 has driven the new bit -> garbled frames. s_spiSettle is the
// per-half-bit spin count (volatile so the empty loop isn't optimized away); tune on HW,
// 0 restores the old zero-delay fast path.
volatile int s_spiSettle = 32;
static void spi_settle(void) { volatile int n = s_spiSettle; while (n-- > 0) { } }

// Tune the bit-bang clock period. SD cards require <=400 kHz during init (CMD0..ACMD41) then
// tolerate fast clocks; the SD driver calls this to run init slow (large n) and block I/O fast.
// Applies to the SCIF bit-bang path (W5500/SD); the SCI hardware path uses SCBRR instead.
void SpiSetSettle(int n) { s_spiSettle = (n < 0) ? 0 : n; }

static BYTE scif_rw_raw(BYTE b)
{
    WORD tmp = (WORD)(s_scsptr2 & ~P2_CTSDT & ~P2_SPB2DT);
    BYTE bit, rv = 0;
    int  i;
    for (i = 7; i >= 0; i--)
    {
        bit = (BYTE)((b >> i) & 1);
        SCSPTR2 = (WORD)(tmp | bit);                 // data out, clock low
        spi_settle();                                // MOSI setup before the rising edge
        SCSPTR2 = (WORD)(tmp | bit | P2_CTSDT);      // clock high (rising edge)
        spi_settle();                                // let the W5500 drive MISO before we sample
        rv = (BYTE)((rv << 1) | (SCSPTR2 & P2_SPB2DT));
    }
    SCSPTR2 = tmp;                                   // leave SCK idle LOW (SPI mode 0) so the
                                                     // next byte's first rising edge isn't a glitch
    return rv;
}

// ---- SCI hardware sync mode (MSB-first SPI via internal bit-reverse) ------------
static void sci_setcs_raw(int active)
{
    if (s_sciCs == DCSPI_CS_GPIO)
    {
        if (active) PDTRA &= ~(1 << PA7_BIT);        // CS low
        else        PDTRA |=  (1 << PA7_BIT);        // CS high
    }
    else                                             // CS via SCIF RTS
    {
        if (active) SCSPTR2 &= ~P2_RTSDT;
        else        SCSPTR2 |=  P2_RTSDT;
    }
}
static int sci_init_raw(int csmode)
{
    DWORD t = 0;
    BYTE  d;
    if (STBCR & STBCR_SCI) { STBCR &= ~STBCR_SCI; Sleep(1); }   // wake the SCI module
    SCSCR1 = 0;
    SCSPTR1 = 0;
    if (csmode == DCSPI_CS_AUTO)                     // resolve CS source by board (KOS parity):
        csmode = (((SYSMODE >> 4) & 0x0F) == DCSPI_HW_RETAIL)  // retail DC has PA7; Naomi/Set5 don't
                 ? DCSPI_CS_GPIO : DCSPI_CS_RTS;
    s_sciCs = csmode;
    if (csmode == DCSPI_CS_GPIO)                     // PA7 as output, CS idle high (retail Dreamcast)
    {
        PCTRA = (PCTRA & ~(3u << (PA7_BIT * 2))) | (1u << (PA7_BIT * 2));
        PDTRA |= (1 << PA7_BIT);
    }
    else                                             // CS on SCIF RTS (Naomi: CN1 exposes no GPIO)
    {
        SCFCR2 &= ~SCFCR_MCE;                        // RTS as GPIO output, not HW modem control
        SCSPTR2 |= (P2_RTSIO | P2_RTSDT);            // RTS output, idle high
    }
    SCSMR1 = SC_CA;                                  // 8-bit synchronous, CKS=0 (n=0)
    SCBRR1 = SCI_BRR;
    Sleep(1);
    SCSSR1 &= ~(SC_ORER | SCSSR_FER | SCSSR_PER);    // clear all RX errors
    if (SCSSR1 & SC_RDRF) { d = SCRDR1; (void)d; }   // flush
    SCSCR1 = SC_TE | SC_RE;                          // internal clock, full-duplex
    do { if (++t > SCI_WAIT) return -1; } while (!(SCSSR1 & SC_TDRE));
    return 0;
}
static BYTE sci_rw_raw(BYTE b)
{
    DWORD t = 0;
    BYTE  v = bitrev8(b);                            // SCI shifts LSB-first; reverse for MSB-first SPI
    while (!(SCSSR1 & SC_TDRE)) if (++t > SCI_WAIT) return 0xFF;
    SCTDR1 = v;
    SCSSR1 &= ~SC_TDRE;                              // start the 8-bit clocked exchange
    t = 0;
    while (!(SCSSR1 & SC_RDRF)) { if (SCSSR1 & SC_ORER) SCSSR1 &= ~SC_ORER; if (++t > SCI_WAIT) return 0xFF; }
    v = SCRDR1;
    SCSSR1 &= ~SC_RDRF;
    return bitrev8(v);
}

// ---- public API (SetKMode-wrapped) ---------------------------------------------
// Hardware type from the SH-4 system-mode register (0xA05F74B0), same source KOS reads:
// returns the type nibble - 0=retail Dreamcast, 9=Set5 devkit, 0xA=Naomi. Used to pick the
// SCI-SPI chip-select source (PA7 GPIO on DC, SCIF RTS on Naomi/Set5).
int DcspiHwType(void)
{
    DWORD prev = SetKMode(TRUE);
    int   ty = DCSPI_HW_RETAIL;
    __try { ty = (int)((SYSMODE >> 4) & 0x0F); } __except (EXCEPTION_EXECUTE_HANDLER) { ty = DCSPI_HW_RETAIL; }
    SetKMode(prev);
    return ty;
}

int SpiInit(int bus, int csmode)
{
    DWORD prev = SetKMode(TRUE);
    int   rc = -1;
    __try {
        if (bus == DCSPI_BUS_SCIF)     { scif_init_raw(); s_scifUp = 1; rc = 0; }
        else if (bus == DCSPI_BUS_SCI) { rc = sci_init_raw(csmode); if (rc == 0) s_sciUp = 1; }
    } __except (EXCEPTION_EXECUTE_HANDLER) { rc = -1; }
    SetKMode(prev);
    return rc;
}
void SpiShutdown(int bus)
{
    DWORD prev = SetKMode(TRUE);
    __try {
        if (bus == DCSPI_BUS_SCI && s_sciUp) { SCSCR1 = 0; STBCR |= STBCR_SCI; s_sciUp = 0; }
        else if (bus == DCSPI_BUS_SCIF) { scif_restore_console(); s_scifUp = 0; }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    SetKMode(prev);
}
void SpiSetCS(int bus, int active)
{
    DWORD prev = SetKMode(TRUE);
    __try {
        if (bus == DCSPI_BUS_SCIF) scif_setcs_raw(active);
        else                       sci_setcs_raw(active);
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    SetKMode(prev);
}
unsigned char SpiRwByte(int bus, unsigned char tx)
{
    DWORD prev = SetKMode(TRUE);
    BYTE  rv = 0xFF;
    __try { rv = (bus == DCSPI_BUS_SCIF) ? scif_rw_raw(tx) : sci_rw_raw(tx); }
    __except (EXCEPTION_EXECUTE_HANDLER) { rv = 0xFF; }
    SetKMode(prev);
    return rv;
}
void SpiRwData(int bus, const unsigned char *tx, unsigned char *rx, int len)
{
    DWORD prev = SetKMode(TRUE);
    int   i;
    __try {
        if (bus == DCSPI_BUS_SCIF)
            for (i = 0; i < len; i++) { BYTE r = scif_rw_raw(tx ? tx[i] : 0xFF); if (rx) rx[i] = r; }
        else
            for (i = 0; i < len; i++) { BYTE r = sci_rw_raw(tx ? tx[i] : 0xFF); if (rx) rx[i] = r; }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    SetKMode(prev);
}

BOOL WINAPI DllMain(HANDLE h, DWORD reason, LPVOID r) { (void)h; (void)reason; (void)r; return TRUE; }
