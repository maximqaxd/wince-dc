/*
 * dc_hw.h - Sega Dreamcast / SH7091 hardware register map for the CE 3.0 OAL.
 *
 * RECONSTRUCTED from the shipped SDK kernel (release\debug\nknodbg.exe). Values
 * verified against OEMInit/InitClock literal pools. See OAL-NOTES.md.
 *
 * All MMIO is accessed uncached: SH-4 on-chip control regs live in P4
 * (0xFFxxxxxx); Holly/system-bus regs are addressed in the P2 uncached window
 * (0xA05Fxxxx). volatile pointers, never cached.
 */
#ifndef _DC_HW_H_
#define _DC_HW_H_

#define VUINT32(a)      (*(volatile unsigned long  *)(a))
#define VUINT16(a)      (*(volatile unsigned short *)(a))
#define VUINT8(a)       (*(volatile unsigned char  *)(a))

/* ---- SH-4 INTC (interrupt controller) -- P4 ---------------------------------*/
#define SH4_INTC_ICR    0xFFD00000      /* 16-bit */
#define SH4_INTC_IPRA   0xFFD00004      /* 16-bit: TMU0/TMU1/... priority nibbles */
#define SH4_INTC_IPRB   0xFFD00008      /* 16-bit */
#define SH4_INTC_IPRC   0xFFD0000C      /* 16-bit */

/* ---- SH-4 TMU (timer unit) -- P4 --------------------------------------------*/
#define SH4_TMU_BASE    0xFFD80000
#define SH4_TMU_TOCR    (SH4_TMU_BASE+0x00)     /* 8-bit  output control */
#define SH4_TMU_TSTR    (SH4_TMU_BASE+0x04)     /* 8-bit  start (bit n = TMUn) */
#define SH4_TMU_TCOR0   (SH4_TMU_BASE+0x08)     /* 32-bit constant */
#define SH4_TMU_TCNT0   (SH4_TMU_BASE+0x0C)     /* 32-bit counter */
#define SH4_TMU_TCR0    (SH4_TMU_BASE+0x10)     /* 16-bit control (UNIE=0x20) */
#define SH4_TMU_TCOR1   (SH4_TMU_BASE+0x14)
#define SH4_TMU_TCNT1   (SH4_TMU_BASE+0x18)
#define SH4_TMU_TCR1    (SH4_TMU_BASE+0x1C)
#define SH4_TMU_TCOR2   (SH4_TMU_BASE+0x20)
#define SH4_TMU_TCNT2   (SH4_TMU_BASE+0x24)
#define SH4_TMU_TCR2    (SH4_TMU_BASE+0x28)
#define SH4_TMU_TCR_UNIE        0x0020          /* underflow interrupt enable */
#define SH4_TSTR_STR0           0x01
#define SH4_TSTR_STR1           0x02
#define SH4_TSTR_STR2           0x04

/* ---- SH-4 DMAC -- P4 --------------------------------------------------------*/
#define SH4_DMAC_BASE   0xFFA00000
#define SH4_DMAC_DMAOR  (SH4_DMAC_BASE+0x40)    /* operation reg */
#define SH4_DMAOR_INIT  0x00008201              /* DDT|PR|DME : enable on-demand DMA */

/* ---- SH-4 SCIF (serial w/ FIFO) -- P4 ; the DC "serial"/coder's-cable UART ---*/
#define SH4_SCIF_BASE   0xFFE80000
#define SH4_SCSMR2      (SH4_SCIF_BASE+0x00)    /* 16: serial mode (8N1 = 0x0000) */
#define SH4_SCBRR2      (SH4_SCIF_BASE+0x04)    /*  8: bit-rate divisor */
#define SH4_SCSCR2      (SH4_SCIF_BASE+0x08)    /* 16: serial control (TE=0x20,RE=0x10) */
#define SH4_SCFTDR2     (SH4_SCIF_BASE+0x0C)    /*  8: TX FIFO data */
#define SH4_SCFSR2      (SH4_SCIF_BASE+0x10)    /* 16: FIFO status */
#define SH4_SCFRDR2     (SH4_SCIF_BASE+0x14)    /*  8: RX FIFO data */
#define SH4_SCFCR2      (SH4_SCIF_BASE+0x18)    /* 16: FIFO control */
#define SH4_SCFDR2      (SH4_SCIF_BASE+0x1C)    /* 16: FIFO data count */
#define SH4_SCSPTR2     (SH4_SCIF_BASE+0x20)    /* 16: serial port reg */
#define SH4_SCLSR2      (SH4_SCIF_BASE+0x24)    /* 16: line status */
/* SCSCR2 bits */
#define SCSCR2_TE       0x0020                  /* transmit enable */
#define SCSCR2_RE       0x0010                  /* receive enable */
/* SCFSR2 status bits */
#define SCFSR2_ER       0x0080                  /* receive error */
#define SCFSR2_TEND     0x0040                  /* transmit end */
#define SCFSR2_TDFE     0x0020                  /* TX FIFO data empty (room to write) */
#define SCFSR2_BRK      0x0010                  /* break */
#define SCFSR2_RDF      0x0002                  /* RX FIFO full (>= trigger) */
#define SCFSR2_DR       0x0001                  /* RX data ready */
/* SCFCR2 bits */
#define SCFCR2_TFRST    0x0004                  /* reset TX FIFO */
#define SCFCR2_RFRST    0x0002                  /* reset RX FIFO */
/* DC peripheral clock = 50 MHz (CPU/4). divisor N = Pck/(32*baud) - 1. */
#define DC_PCK          50000000
#define SCIF_BRR(baud)  ((DC_PCK / (32 * (baud))) - 1)   /* 57600 -> 26 ; 115200 -> 13 */

/* ---- SR bits ----------------------------------------------------------------*/
#define SH4_SR_BL       0x10000000              /* block exceptions */

/* P1->P2 (cached->uncached) alias offset */
#define P2_UNCACHED     0x20000000

/* ---- Holly / system bus interrupt regs -- P2 uncached -----------------------*/
/* Status (read in the IRL ISRs), W1C on ack */
#define SB_ISTNRM       0xA05F6900      /* normal int status */
#define SB_ISTEXT       0xA05F6904      /* external int status (Maple, GD, BBA...) */
#define SB_ISTERR       0xA05F6908      /* error int status */
/* Per-IRL masks (OEMInit zeroes all 9 at boot) */
#define SB_IML2NRM      0xA05F6910
#define SB_IML2EXT      0xA05F6914
#define SB_IML2ERR      0xA05F6918
#define SB_IML4NRM      0xA05F6920
#define SB_IML4EXT      0xA05F6924
#define SB_IML4ERR      0xA05F6928
#define SB_IML6NRM      0xA05F6930
#define SB_IML6EXT      0xA05F6934
#define SB_IML6ERR      0xA05F6938

/* ---- CE INTC vector numbers passed to HookInterrupt() (from OEMInit) --------*/
#define INTRVEC_TMU0    0x10            /* system tick */
#define INTRVEC_TMU1    0x11
#define INTRVEC_HOLLY6  0x09            /* SH-4 IRL level 6 -> KatanaISR6 */
#define INTRVEC_HOLLY4  0x0B            /* IRL level 4 -> KatanaISR4 */
#define INTRVEC_HOLLY2  0x0D            /* IRL level 2 -> KatanaISR2 */
#define INTRVEC_JTAG    0x20
#define INTRVEC_DMAC0   0x22            /* 0x22..0x25 = DMAC ch0..3 */
#define INTRVEC_SCIF    0x28            /* 0x28..0x2B = ERI/RXI/BRI/TXI */

#endif /* _DC_HW_H_ */
