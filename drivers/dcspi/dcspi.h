//
// dcspi.h - Dreamcast SPI transport (SH-4 SCI hardware sync-serial + SCIF bit-bang)
// as a reusable driver DLL. Exposed as exported functions so any consumer links it:
//   - the W5500 ethernet backend (net/netif/w5500.c) NOW,
//   - SD / CF block drivers + a FAT FSD LATER.
//
// Two buses (a board wires its SPI device to one of them):
//   DCSPI_BUS_SCIF  software bit-bang on SCSPTR2 (shares the debug serial port!)
//   DCSPI_BUS_SCI   SH-4 SCI clocked-synchronous mode = true hardware SPI (faster)
//
// Ported from the reference SCIF-SPI + SCI drivers. SPI mode 0, MSB-first
// on the wire (the SCI sends LSB-first, so dcspi bit-reverses internally).
//
#ifndef DCSPI_H
#define DCSPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define DCSPI_BUS_SCIF   0      // bit-bang via SCSPTR2 (CTSDT=clk, SPB2DT=data, RTSDT=CS)
#define DCSPI_BUS_SCI    1      // SH-4 SCI sync mode; CS via PA7 GPIO (retail) or SCIF-RTS

// CS source for the chosen bus.
#define DCSPI_CS_RTS     0      // CS on SCIF RTS line (SCSPTR2 bit6) - default for SCIF bus
#define DCSPI_CS_GPIO    1      // CS on PA7 GPIO (PCTRA/PDTRA)       - default for SCI bus (retail DC)

// Bring a bus up for SPI. csmode selects the chip-select source. Returns 0 on success.
// SetKMode-wrapped internally (touches P4 control registers).
int  SpiInit(int bus, int csmode);

// Release the bus (SCIF reverts toward serial-console use).
void SpiShutdown(int bus);

// Assert (active=1, CS low) / deassert (active=0, CS high) the chip select.
void SpiSetCS(int bus, int active);

// Full-duplex transfer of len bytes. tx may be NULL (sends 0xFF), rx may be NULL
// (discards). For half-duplex use one or the other.
void SpiRwData(int bus, const unsigned char *tx, unsigned char *rx, int len);

// Single byte full-duplex.
unsigned char SpiRwByte(int bus, unsigned char tx);

#ifdef __cplusplus
}
#endif

#endif // DCSPI_H
