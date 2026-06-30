//
// dcspi.c - Dreamcast SPI transport (SCI hardware sync-serial + SCIF bit-bang).
// Ported from the reference SCI + SCIF-SPI drivers. Exposed as a reusable
// exporting DLL (see dcspi.h / dcspi.def). All P4 control-register access is wrapped
// in SetKMode(TRUE) so it works from any caller (driver thread, microstk thread).
//
#include <windows.h>
#include "dcspi.h"

DWORD SetKMode(DWORD dwMode); // coredll export (P4 control regs need kernel mode)

// ---- SCIF (bit-bang on SCSPTR2) -------------------------------------------------
#define R8(a)   (*(volatile BYTE *)(a))
#define R16(a)  (*(volatile WORD *)(a))
#define R32(a)  (*(volatile DWORD *)(a))

#define SCSMR2  R16(0xFFE80000)
#define SCBRR2  R8(0xFFE80004)
#define SCSCR2  R16(0xFFE80008)
#define SCFSR2  R16(0xFFE80010)
#define SCFCR2  R16(0xFFE80018)
#define SCSPTR2 R16(0xFFE80020)
#define SCLSR2  R16(0xFFE80024)
#define SCFCR_MCE                                                                                  \
	0x08 // SCFCR2 modem-control-enable: HW auto-drives RTS. Clear it to use
	     // RTS as a plain GPIO chip-select (required on Naomi SCI-SPI).
#define SYSMODE                                                                                    \
	R32(0xA05F74B0) // SH-4 system-mode reg: type=(v>>4)&0xF (0=retail DC,9=Set5,0xA=Naomi)
#define P2_RTSIO  0x80
#define P2_RTSDT  0x40
#define P2_CTSIO  0x20
#define P2_CTSDT  0x10
#define P2_SPB2IO 0x02
#define P2_SPB2DT 0x01

// ---- SCI (SH-4 clocked-synchronous = hardware SPI) ------------------------------
#define SCSMR1    R8(0xFFE00000)
#define SCBRR1    R8(0xFFE00004)
#define SCSCR1    R8(0xFFE00008)
#define SCTDR1    R8(0xFFE0000C)
#define SCSSR1    R8(0xFFE00010)
#define SCRDR1    R8(0xFFE00014)
#define SCSPTR1   R8(0xFFE00018)
#define STBCR     R8(0xFFC00004)
#define PCTRA     R32(0xFF80002C)
#define PDTRA     R16(0xFF800030)
#define SC_TDRE   0x80
#define SC_RDRF   0x40
#define SC_ORER   0x20
#define SCSSR_FER 0x10 // SCSSR1 framing error (distinct from SCSCR1 RE=0x10)
#define SCSSR_PER 0x08 // SCSSR1 parity error
#define SC_TE     0x20
#define SC_RE     0x10
#define SC_CA     0x80 // SCSMR1: synchronous (clocked) mode
#define STBCR_SCI 0x01 // SCI module standby bit
#define PA7_BIT   7
#define SCI_BRR                                                                                    \
	1 // n=0 -> 6.25 MHz (50MHz/(4*(BRR+1))); =3 was 3.125, =0 is 12.5
	  // 6.25 MHz halves per-frame SPI time (W5500 handles >25 MHz) so
	  // full-size RX frames drain before the 16K W5500 ring overflows.
#define SCI_WAIT 500000

static int s_nScifUp, s_nSciUp, s_nSciCs;
static WORD s_wScsptr2; // shadow of SCSPTR2 (SCIF)

static BYTE Bitrev8(BYTE b)
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
static WORD s_wSavSmr, s_wSavScr, s_wSavFcr, s_wSavPtr, s_wSavLsr;
static BYTE s_bSavBrr;
static int s_nScifSaved;
static void ScifSaveConsole(void)
{
	s_wSavSmr = SCSMR2;
	s_bSavBrr = SCBRR2;
	s_wSavScr = SCSCR2;
	s_wSavFcr = SCFCR2;
	s_wSavPtr = SCSPTR2;
	s_wSavLsr = SCLSR2;
	s_nScifSaved = 1;
}
static void ScifRestoreConsole(void)
{
	if (!s_nScifSaved)
		return;
	SCSCR2 = 0; // stop TX/RX before reprogramming
	SCFCR2 = 0x06;
	SCFCR2 = s_wSavFcr; // reset then restore the FIFOs
	SCSMR2 = s_wSavSmr;
	SCBRR2 = s_bSavBrr;
	SCSPTR2 = s_wSavPtr;
	SCLSR2 = s_wSavLsr;
	SCSCR2 = s_wSavScr; // re-enable TX (console resumes)
	s_nScifSaved = 0;
}

// ---- SCIF bit-bang (clock=CTSDT, MOSI/MISO=SPB2DT, CS=RTSDT), SPI mode 0 --------
static void ScifInitRaw(void)
{
	ScifSaveConsole(); // so a failed probe can resurrect the console
	SCSCR2 = 0;
	SCFCR2 = 0x06; // flush FIFOs
	SCFCR2 = 0;
	SCSMR2 = 0;
	SCFSR2 = 0;
	SCLSR2 = 0;
	s_wScsptr2 = P2_RTSIO | P2_RTSDT | P2_CTSIO | P2_SPB2IO; // RTS/CTS out, CS high, data out
	SCSPTR2 = s_wScsptr2;
}
static void ScifSetcsRaw(int nActive) // CS active-low: assert -> RTSDT 0
{
	if (nActive)
		s_wScsptr2 &= ~P2_RTSDT;
	else
		s_wScsptr2 |= P2_RTSDT;
	SCSPTR2 = s_wScsptr2;
}
// Inter-edge settle for the SCIF bit-bang. KOS ships scif_spi_slow_rw_byte (1.5us/edge)
// because back-to-back SCSPTR2 stores clock the bus faster than some W5500 wiring can meet
// MISO setup/hold: a short VERSIONR read survives but a long MACRAW burst sporadically
// samples MISO before the W5500 has driven the new bit -> garbled frames. s_nSpiSettle is the
// per-half-bit spin count (volatile so the empty loop isn't optimized away); tune on HW,
// 0 restores the old zero-delay fast path.
volatile int s_nSpiSettle = 32;
static void SpiSettle(void)
{
	volatile int n = s_nSpiSettle;
	while (n-- > 0)
	{
	}
}

// Tune the bit-bang clock period. SD cards require <=400 kHz during init (CMD0..ACMD41) then
// tolerate fast clocks; the SD driver calls this to run init slow (large n) and block I/O fast.
// Applies to the SCIF bit-bang path (W5500/SD); the SCI hardware path uses SCBRR instead.
void SpiSetSettle(int n)
{
	s_nSpiSettle = (n < 0) ? 0 : n;
}

static BYTE ScifRwRaw(BYTE b)
{
	WORD wTmp = (WORD)(s_wScsptr2 & ~P2_CTSDT & ~P2_SPB2DT);
	BYTE bBit, bRv = 0;
	int i;
	for (i = 7; i >= 0; i--)
	{
		bBit = (BYTE)((b >> i) & 1);
		SCSPTR2 = (WORD)(wTmp | bBit);            // data out, clock low
		SpiSettle();                              // MOSI setup before the rising edge
		SCSPTR2 = (WORD)(wTmp | bBit | P2_CTSDT); // clock high (rising edge)
		SpiSettle();                              // let the W5500 drive MISO before we sample
		bRv = (BYTE)((bRv << 1) | (SCSPTR2 & P2_SPB2DT));
	}
	SCSPTR2 = wTmp; // leave SCK idle LOW (SPI mode 0) so the
	                // next byte's first rising edge isn't a glitch
	return bRv;
}

// ---- SCI hardware sync mode (MSB-first SPI via internal bit-reverse) ------------
static void SciSetcsRaw(int nActive)
{
	if (s_nSciCs == DCSPI_CS_GPIO)
	{
		if (nActive)
			PDTRA &= ~(1 << PA7_BIT); // CS low
		else
			PDTRA |= (1 << PA7_BIT); // CS high
	}
	else // CS via SCIF RTS
	{
		if (nActive)
			SCSPTR2 &= ~P2_RTSDT;
		else
			SCSPTR2 |= P2_RTSDT;
	}
}
static int SciInitRaw(int nCsmode)
{
	DWORD dwT = 0;
	BYTE bD;
	if (STBCR & STBCR_SCI)
	{
		STBCR &= ~STBCR_SCI;
		Sleep(1);
	} // wake the SCI module
	SCSCR1 = 0;
	SCSPTR1 = 0;
	if (nCsmode == DCSPI_CS_AUTO) // resolve CS source by board (KOS parity):
		nCsmode =
		    (((SYSMODE >> 4) & 0x0F) == DCSPI_HW_RETAIL) // retail DC has PA7; Naomi/Set5 don't
		        ? DCSPI_CS_GPIO
		        : DCSPI_CS_RTS;
	s_nSciCs = nCsmode;
	if (nCsmode == DCSPI_CS_GPIO) // PA7 as output, CS idle high (retail Dreamcast)
	{
		PCTRA = (PCTRA & ~(3u << (PA7_BIT * 2))) | (1u << (PA7_BIT * 2));
		PDTRA |= (1 << PA7_BIT);
	}
	else // CS on SCIF RTS (Naomi: CN1 exposes no GPIO)
	{
		SCFCR2 &= ~SCFCR_MCE;             // RTS as GPIO output, not HW modem control
		SCSPTR2 |= (P2_RTSIO | P2_RTSDT); // RTS output, idle high
	}
	SCSMR1 = SC_CA; // 8-bit synchronous, CKS=0 (n=0)
	SCBRR1 = SCI_BRR;
	Sleep(1);
	SCSSR1 &= ~(SC_ORER | SCSSR_FER | SCSSR_PER); // clear all RX errors
	if (SCSSR1 & SC_RDRF)
	{
		bD = SCRDR1;
		(void)bD;
	} // flush
	SCSCR1 = SC_TE | SC_RE; // internal clock, full-duplex
	do
	{
		if (++dwT > SCI_WAIT)
			return -1;
	} while (!(SCSSR1 & SC_TDRE));
	return 0;
}
static BYTE SciRwRaw(BYTE b)
{
	DWORD dwT = 0;
	BYTE bV = Bitrev8(b); // SCI shifts LSB-first; reverse for MSB-first SPI
	while (!(SCSSR1 & SC_TDRE))
		if (++dwT > SCI_WAIT)
			return 0xFF;
	SCTDR1 = bV;
	SCSSR1 &= ~SC_TDRE; // start the 8-bit clocked exchange
	dwT = 0;
	while (!(SCSSR1 & SC_RDRF))
	{
		if (SCSSR1 & SC_ORER)
			SCSSR1 &= ~SC_ORER;
		if (++dwT > SCI_WAIT)
			return 0xFF;
	}
	bV = SCRDR1;
	SCSSR1 &= ~SC_RDRF;
	return Bitrev8(bV);
}

// ---- public API (SetKMode-wrapped) ---------------------------------------------
// Hardware type from the SH-4 system-mode register (0xA05F74B0), same source KOS reads:
// returns the type nibble - 0=retail Dreamcast, 9=Set5 devkit, 0xA=Naomi. Used to pick the
// SCI-SPI chip-select source (PA7 GPIO on DC, SCIF RTS on Naomi/Set5).
int DcspiHwType(void)
{
	DWORD dwPrev = SetKMode(TRUE);
	int nTy = DCSPI_HW_RETAIL;
	__try
	{
		nTy = (int)((SYSMODE >> 4) & 0x0F);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nTy = DCSPI_HW_RETAIL;
	}
	SetKMode(dwPrev);
	return nTy;
}

int SpiInit(int nBus, int nCsmode)
{
	DWORD dwPrev = SetKMode(TRUE);
	int nRc = -1;
	__try
	{
		if (nBus == DCSPI_BUS_SCIF)
		{
			ScifInitRaw();
			s_nScifUp = 1;
			nRc = 0;
		}
		else if (nBus == DCSPI_BUS_SCI)
		{
			nRc = SciInitRaw(nCsmode);
			if (nRc == 0)
				s_nSciUp = 1;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nRc = -1;
	}
	SetKMode(dwPrev);
	return nRc;
}
void SpiShutdown(int nBus)
{
	DWORD dwPrev = SetKMode(TRUE);
	__try
	{
		if (nBus == DCSPI_BUS_SCI && s_nSciUp)
		{
			SCSCR1 = 0;
			STBCR |= STBCR_SCI;
			s_nSciUp = 0;
		}
		else if (nBus == DCSPI_BUS_SCIF)
		{
			ScifRestoreConsole();
			s_nScifUp = 0;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	SetKMode(dwPrev);
}
void SpiSetCS(int nBus, int nActive)
{
	DWORD dwPrev = SetKMode(TRUE);
	__try
	{
		if (nBus == DCSPI_BUS_SCIF)
			ScifSetcsRaw(nActive);
		else
			SciSetcsRaw(nActive);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	SetKMode(dwPrev);
}
unsigned char SpiRwByte(int nBus, unsigned char bTx)
{
	DWORD dwPrev = SetKMode(TRUE);
	BYTE bRv = 0xFF;
	__try
	{
		bRv = (nBus == DCSPI_BUS_SCIF) ? ScifRwRaw(bTx) : SciRwRaw(bTx);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bRv = 0xFF;
	}
	SetKMode(dwPrev);
	return bRv;
}
void SpiRwData(int nBus, const unsigned char *pbTx, unsigned char *pbRx, int nLen)
{
	DWORD dwPrev = SetKMode(TRUE);
	int i;
	__try
	{
		if (nBus == DCSPI_BUS_SCIF)
			for (i = 0; i < nLen; i++)
			{
				BYTE bR = ScifRwRaw(pbTx ? pbTx[i] : 0xFF);
				if (pbRx)
					pbRx[i] = bR;
			}
		else
			for (i = 0; i < nLen; i++)
			{
				BYTE bR = SciRwRaw(pbTx ? pbTx[i] : 0xFF);
				if (pbRx)
					pbRx[i] = bR;
			}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	SetKMode(dwPrev);
}

BOOL WINAPI DllMain(HANDLE h, DWORD dwReason, LPVOID pvReserved)
{
	(void)h;
	(void)dwReason;
	(void)pvReserved;
	return TRUE;
}
