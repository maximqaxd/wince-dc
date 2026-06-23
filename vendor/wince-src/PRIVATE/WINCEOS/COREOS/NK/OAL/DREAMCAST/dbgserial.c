/*
 * dbgserial.c - Dreamcast OAL polled SCIF debug console.
 *
 * OURS (no shipped equivalent). The SDK kernel's OEMWriteDebug* route through the
 * Sega ASE BIOS / Debug Adapter (asedbg), NOT the UART. For first-boot bring-up
 * we want plain text out the SH-4 SCIF (the DC "serial"/coder's cable) with NO
 * KITL / WinDbg protocol - just polled, byte-at-a-time output a terminal can read.
 *
 * 8N1, default 57600 baud, internal clock (Pck = 50 MHz). Reference: SH7750
 * SCIF + KallistiOS dbgio/scif. See OAL-NOTES.md.
 */
#include <windows.h>
#include "dc_hw.h"

#ifndef DBG_SCIF_BAUD
#define DBG_SCIF_BAUD   57600
#endif

/* Initialise the SCIF for polled 8N1 output. No interrupts enabled (TIE/RIE off)
 * so this never collides with an interrupt-driven serial driver or KITL. */
/* volatile so the compiler can't optimize the settle loop away */
static void scif_delay(void)
{
    volatile int i;
    for (i = 0; i < 800000; i++)
        ;
}

void OEMInitDebugSerial(void)
{
    VUINT16(SH4_SCSCR2) = 0x0000;                       /* TX/RX off, internal clock */
    VUINT16(SH4_SCFCR2) = SCFCR2_TFRST | SCFCR2_RFRST;  /* reset both FIFOs (0x06) */
    VUINT16(SH4_SCSMR2) = 0x0000;                        /* async, 8N1, Pck/1 */
    VUINT8 (SH4_SCBRR2) = (BYTE)SCIF_BRR(DBG_SCIF_BAUD); /* bit rate (57600 -> 26) */
    scif_delay();
    VUINT16(SH4_SCFCR2) = 0x0040;                        /* unreset, trigger on 8 (KOS) */
    VUINT16(SH4_SCSPTR2)= 0x0000;                        /* no manual pin control */
    VUINT16(SH4_SCFSR2) = 0x0060;                        /* clear TEND|TDFE */
    VUINT16(SH4_SCLSR2) = 0x0000;                        /* clear ORER */
    VUINT16(SH4_SCSCR2) = SCSCR2_TE | SCSCR2_RE;         /* enable TX + RX (polled) */
    scif_delay();
}

/* Write one byte, blocking until the TX FIFO has room. */
void OEMWriteDebugByte(BYTE ch)
{
    while ((VUINT16(SH4_SCFSR2) & SCFSR2_TDFE) == 0)     /* wait for FIFO space */
        ;
    VUINT8(SH4_SCFTDR2) = ch;
    /* clear TDFE + TEND (write 0 to the status bits) so the next poll is fresh */
    VUINT16(SH4_SCFSR2) &= (USHORT)~(SCFSR2_TDFE | SCFSR2_TEND);
}

/* Kernel debug strings are Unicode; emit the low byte, LF -> CRLF. */
void OEMWriteDebugString(LPCWSTR psz)
{
    if (!psz)
        return;
    while (*psz) {
        if (*psz == L'\n')
            OEMWriteDebugByte('\r');
        OEMWriteDebugByte((BYTE)(*psz & 0xFF));
        psz++;
    }
}

/* Polled read; returns the byte, or -1 if none available. */
int OEMReadDebugByte(void)
{
    if (VUINT16(SH4_SCFSR2) & (SCFSR2_RDF | SCFSR2_DR)) {
        BYTE ch = VUINT8(SH4_SCFRDR2);
        VUINT16(SH4_SCFSR2) &= (USHORT)~(SCFSR2_RDF | SCFSR2_DR);   /* ack */
        return (int)ch;
    }
    return -1;   /* OEM_DEBUG_READ_NODATA */
}
