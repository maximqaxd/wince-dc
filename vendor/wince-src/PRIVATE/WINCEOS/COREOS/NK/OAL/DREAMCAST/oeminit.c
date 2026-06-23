/*
 * oeminit.c - Dreamcast OAL master init (was hal:cfwkatan.obj).
 *
 * RECONSTRUCTED from release\debug\nknodbg.exe (OEMInit @ 0x8C03CAFC). Faithful
 * transcription of the decompiled boot sequence; see OAL-NOTES.md.
 */
#include <windows.h>
#include "dc_hw.h"

/* --- kernel / OAL imports (provided by nkmain.lib or sibling OAL objs) --- */
/* NKDbgPrintfW is declared by the CE private headers (pulled via windows.h). */
extern BOOL  HookInterrupt(int idInt, FARPROC pfn);
extern DWORD pTOC;                      /* ROMHDR* (kernel ROM table of contents) */

extern void  InitClock(void);
extern void  OEMParallelPortInit(void);
extern void  SerialInit(void);
extern int   OEMGetPlatformVersion(void *pv);
extern DWORD OEMPlatformVersion;        /* set by OEMGetPlatformVersion: 4 or 5 */

/* Holly IRL demuxers (intr.c) and per-source ISRs */
extern void KatanaISR2(void), KatanaISR4(void), KatanaISR6(void);
extern void JTAGISR(void);
extern void DMAC0ISR(void), DMAC1ISR(void), DMAC2ISR(void), DMAC3ISR(void);

static const WCHAR szSet5[] = L"Set 5 is detected.\r\n";
static const WCHAR szSet4[] = L"Set 4 is detected.\r\n";

void OEMInit(void)
{
    DWORD *pRamFixup;

    /* 1. Mask every interrupt source: SH-4 INTC priorities + all 9 Holly masks. */
    VUINT16(SH4_INTC_IPRA) = 0;
    VUINT16(SH4_INTC_IPRB) = 0;
    VUINT16(SH4_INTC_IPRC) = 0;
    VUINT32(SB_IML2NRM) = 0; VUINT32(SB_IML2EXT) = 0; VUINT32(SB_IML2ERR) = 0;
    VUINT32(SB_IML4NRM) = 0; VUINT32(SB_IML4EXT) = 0; VUINT32(SB_IML4ERR) = 0;
    VUINT32(SB_IML6NRM) = 0; VUINT32(SB_IML6EXT) = 0; VUINT32(SB_IML6ERR) = 0;

    /* 2. Detect Katana board revision (Set 4 / Set 5 = DC dev hardware). */
    OEMGetPlatformVersion(NULL);

    /* 3-4. Bring up the TMU tick and the debug parallel port. */
    InitClock();
    OEMParallelPortInit();

    if (OEMPlatformVersion == 5)
        NKDbgPrintfW(szSet5);
    else if (OEMPlatformVersion == 4)
        NKDbgPrintfW(szSet4);

    /* 5. INTC vector -> ISR map. */
    HookInterrupt(INTRVEC_HOLLY2, (FARPROC)KatanaISR2);   /* 0x0D : IRL level 2 */
    HookInterrupt(INTRVEC_HOLLY4, (FARPROC)KatanaISR4);   /* 0x0B : IRL level 4 */
    HookInterrupt(INTRVEC_HOLLY6, (FARPROC)KatanaISR6);   /* 0x09 : IRL level 6 */
    HookInterrupt(INTRVEC_JTAG,   (FARPROC)JTAGISR);      /* 0x20 */
    HookInterrupt(INTRVEC_DMAC0+0,(FARPROC)DMAC0ISR);     /* 0x22..0x25 */
    HookInterrupt(INTRVEC_DMAC0+1,(FARPROC)DMAC1ISR);
    HookInterrupt(INTRVEC_DMAC0+2,(FARPROC)DMAC2ISR);
    HookInterrupt(INTRVEC_DMAC0+3,(FARPROC)DMAC3ISR);

    /* 6. SCIF serial interrupts. */
    SerialInit();

    /* 7. Clear a 3-DWORD RAM scratch via the P2 uncached alias (ROMHDR+0x18),
     *    then enable the DMAC. */
    pRamFixup = (DWORD *)((*(DWORD *)((char *)pTOC + 0x18)) | P2_UNCACHED);
    pRamFixup[0] = 0;
    pRamFixup[1] = 0;
    pRamFixup[2] = 0;

    VUINT32(SH4_DMAC_DMAOR) = SH4_DMAOR_INIT;
}
