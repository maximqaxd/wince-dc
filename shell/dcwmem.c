//
// dcwmem.c - Memory / MMU test: a DCWin client that proves RAM is real (not an
// aliased mirror) and exercises the MMU through VirtualAlloc.
//
// It commits one large block (as much as the 32MB process slot / free RAM allows)
// and runs three two-pass tests:
//   1. address-in-address - every 32-bit cell is written with its own virtual
//      address, then the whole block is read back and verified. Because the block
//      is FILLED in full before it is CHECKED, and the block is far larger than the
//      SH-4 cache (16KB), every line is evicted between write and read - so the
//      verify reads physical RAM, not a cached copy. If the upper 16MB is a mirror
//      of the lower (a 32MB image on a 16MB box), two virtual pages alias one
//      physical page: the second write clobbers the first, and the verify catches
//      the mismatch. A genuine 32MB machine passes.
//   2/3. 0x55555555 / 0xAAAAAAAA pattern passes - catch stuck data bits.
//
// The work is done a chunk per loop so the window stays responsive and closable.
// Press Enter (or the A button) to run; results show MB tested, pass/fail, and the
// first bad address. GlobalMemoryStatus' total is the headline 16-vs-32MB number.
//
#include "dcwlib.h"

#define CW       324
#define CH       208
#define CHUNK    (512*1024/4)      // 512K words = 2MB processed per loop pass
#define MAXTEST  (24*1024*1024)    // cap one VirtualAlloc (CE gives a 32MB slot/proc)
#define MARGIN   (2*1024*1024)     // leave headroom so the system stays alive

enum { ST_IDLE, ST_FILLA, ST_CHKA, ST_FILL5, ST_CHK5, ST_FILLB, ST_CHKB, ST_DONE, ST_FAIL };

static DWORD *g_blk;               // committed test block (NULL = none)
static DWORD  g_bytes;             // block size in bytes
static DWORD  g_words;             // block size in words
static DWORD  g_pos;               // cursor within the current phase (words)
static int    g_state = ST_IDLE;
static DWORD  g_failAddr, g_failGot, g_failExp;
static WCHAR  g_msg[48] = L"";

// Allocate the biggest block we reasonably can: aim for availPhys-MARGIN, capped,
// and step down by 1MB until a commit succeeds (VA fragmentation can bite the top).
static DWORD *AllocBig(DWORD *gotBytes)
{
    MEMORYSTATUS ms;
    DWORD want, b;
    void *p;
    memset(&ms, 0, sizeof(ms));
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    want = (ms.dwAvailPhys > MARGIN) ? ms.dwAvailPhys - MARGIN : 0;
    if (want > MAXTEST) want = MAXTEST;
    want &= ~0xFFFFu;                                  // 64K align
    for (b = want; b >= 0x100000; b -= 0x100000)
    {
        p = VirtualAlloc(NULL, b, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (p) { *gotBytes = b; return (DWORD *)p; }
    }
    *gotBytes = 0;
    return NULL;
}

static void Fail(DWORD i, DWORD got, DWORD exp)
{
    g_failAddr = (DWORD)(g_blk + i);
    g_failGot  = got;
    g_failExp  = exp;
    g_state    = ST_FAIL;
    lstrcpyW(g_msg, L"FAIL - memory/MMU error");
}

// Process one CHUNK of the current phase; advance phase at end of block.
static void StepTest(void)
{
    DWORD end, i;
    if (g_state < ST_FILLA || g_state > ST_CHKB)
        return;
    end = g_pos + CHUNK;
    if (end > g_words) end = g_words;

    switch (g_state)
    {
    case ST_FILLA:                                     // cell = its own address
        for (i = g_pos; i < end; i++) g_blk[i] = (DWORD)(g_blk + i);
        break;
    case ST_CHKA:
        for (i = g_pos; i < end; i++)
            if (g_blk[i] != (DWORD)(g_blk + i)) { Fail(i, g_blk[i], (DWORD)(g_blk + i)); return; }
        break;
    case ST_FILL5:
        for (i = g_pos; i < end; i++) g_blk[i] = 0x55555555;
        break;
    case ST_CHK5:
        for (i = g_pos; i < end; i++)
            if (g_blk[i] != 0x55555555) { Fail(i, g_blk[i], 0x55555555); return; }
        break;
    case ST_FILLB:
        for (i = g_pos; i < end; i++) g_blk[i] = 0xAAAAAAAA;
        break;
    case ST_CHKB:
        for (i = g_pos; i < end; i++)
            if (g_blk[i] != 0xAAAAAAAA) { Fail(i, g_blk[i], 0xAAAAAAAA); return; }
        break;
    }

    g_pos = end;
    if (g_pos >= g_words)                               // phase complete -> next
    {
        g_pos = 0;
        g_state++;                                      // ST_FILLA..ST_CHKB..ST_DONE
        if (g_state == ST_DONE)
            wsprintfW(g_msg, L"PASS - %u MB ok, 0 errors", (unsigned)(g_bytes >> 20));
    }
}

static const WCHAR *PhaseName(void)
{
    switch (g_state)
    {
    case ST_FILLA: return L"addr-in-addr: writing";
    case ST_CHKA:  return L"addr-in-addr: verifying";
    case ST_FILL5: return L"pattern 0x55: writing";
    case ST_CHK5:  return L"pattern 0x55: verifying";
    case ST_FILLB: return L"pattern 0xAA: writing";
    case ST_CHKB:  return L"pattern 0xAA: verifying";
    }
    return L"";
}

// 0..100 across all 6 phases.
static int Percent(void)
{
    int ph;
    if (g_state < ST_FILLA) return 0;
    if (g_state > ST_CHKB)  return 100;
    ph = g_state - ST_FILLA;                            // 0..5
    if (!g_words) return 0;
    return (int)(((DWORD)ph * 1000 + (g_pos * 1000 / g_words)) / 60);
}

static void StartTest(void)
{
    if (g_blk) { VirtualFree(g_blk, 0, MEM_RELEASE); g_blk = NULL; }
    g_blk = AllocBig(&g_bytes);
    if (!g_blk)
    {
        g_words = 0; g_state = ST_FAIL;
        lstrcpyW(g_msg, L"VirtualAlloc failed (low RAM)");
        return;
    }
    g_words = g_bytes / 4;
    g_pos   = 0;
    g_state = ST_FILLA;
    g_msg[0] = 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key;
    int    drawnOnce = 0;

    w = DCWinOpen(120, 70, CW, CH, L"Memory / MMU Test", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWMEM: DCWinOpen failed\r\n"); return 1; }

    for (;;)
    {
        MEMORYSTATUS ms;
        WCHAR        ram[72], row[72];
        int          busy, changed = 0, cw = CW, ch = CH;
        changed |= DCWinClientSize(w, &cw, &ch);   // resize/maximize -> redraw to fit

        while (DCWinPollKey(w, &key))
        {
            if ((key == VK_RETURN || key == VK_SPACE) &&
                (g_state == ST_IDLE || g_state == ST_DONE || g_state == ST_FAIL))
                StartTest();
            changed = 1;
        }

        busy = (g_state >= ST_FILLA && g_state <= ST_CHKB);
        if (busy) { StepTest(); changed = 1; }          // animate progress while testing
        if ((g_state == ST_DONE || g_state == ST_FAIL) && g_blk)
        {                                               // free as soon as the run ends
            VirtualFree(g_blk, 0, MEM_RELEASE);
            g_blk = NULL;
        }

        if (changed || !drawnOnce)
        {
            int pc = Percent(), bw = cw - 24;             // progress bar spans the width
            drawnOnce = 1;
            DCWinBeginFrame(w);
            DCWinFillBg(w, RGB(192, 192, 192));            // background fills the window

            memset(&ms, 0, sizeof(ms)); ms.dwLength = sizeof(ms);
            GlobalMemoryStatus(&ms);
            wsprintfW(ram, L"Total %u K   Free %u K",
                      (unsigned)(ms.dwTotalPhys / 1024), (unsigned)(ms.dwAvailPhys / 1024));
            DCWinText(w, 10, 8, RGB(0, 0, 0), RGB(192, 192, 192), ram);
            DCWinText(w, 10, 24, RGB(0, 0, 96), RGB(192, 192, 192),
                      L"Enter / A  =  run memory + MMU test");

            // progress bar
            DCWinFill(w, 12, 52, bw, 16, RGB(128, 128, 128));
            if (pc > 0) DCWinFill(w, 12, 52, bw * pc / 100, 16, RGB(0, 0, 160));
            wsprintfW(row, L"%s  %d%%", busy ? PhaseName() : L"", pc);
            DCWinText(w, 14, 74, RGB(0, 0, 0), RGB(192, 192, 192), row);

            if (g_blk)
            {
                wsprintfW(row, L"block %u MB  @ %08X..%08X",
                          (unsigned)(g_bytes >> 20), (unsigned)g_blk,
                          (unsigned)((DWORD)g_blk + g_bytes));
                DCWinText(w, 10, 96, RGB(0, 0, 0), RGB(192, 192, 192), row);
            }

            if (g_msg[0])
            {
                COLORREF c = (g_state == ST_DONE) ? RGB(0, 110, 0) : RGB(170, 0, 0);
                DCWinText(w, 10, 120, c, RGB(192, 192, 192), g_msg);
                if (g_state == ST_FAIL && g_failAddr)
                {
                    wsprintfW(row, L"@ %08X  got %08X  exp %08X",
                              (unsigned)g_failAddr, (unsigned)g_failGot, (unsigned)g_failExp);
                    DCWinText(w, 10, 136, RGB(170, 0, 0), RGB(192, 192, 192), row);
                    DCWinText(w, 10, 152, RGB(80, 0, 0), RGB(192, 192, 192),
                              L"(aliased/mirrored RAM - not real 32MB)");
                }
            }
            DCWinEndFrame(w);
        }

        if (DCWinShouldClose(w)) break;
        Sleep(busy ? 8 : 30);
    }

    if (g_blk) VirtualFree(g_blk, 0, MEM_RELEASE);
    DCWinClose(w);
    return 0;
}
