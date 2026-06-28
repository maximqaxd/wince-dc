//
// dcwclock.c - a live digital clock (its own DCWin process): a 7-segment HH:MM with a
// blinking colon, a seconds bar, and the full date. Redraws once a second.
//
#include "dcwlib.h"

#define CW 240
#define CH 150

// 7-segment masks for 0-9.  bit: a=1 b=2 c=4 d=8 e=16 f=32 g=64
//   aaa
//  f   b
//   ggg
//  e   c
//   ddd
static const int SEG[10] = { 0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F };

static const WCHAR *DOW[7] = { L"Sunday",L"Monday",L"Tuesday",L"Wednesday",L"Thursday",L"Friday",L"Saturday" };
static const WCHAR *MON[12] = { L"January",L"February",L"March",L"April",L"May",L"June",
                                L"July",L"August",L"September",L"October",L"November",L"December" };

// Draw one 7-segment digit in a (dw x dh) cell at (x,y); only lit segments are filled.
static void Draw7(DCWin *w, int x, int y, int dw, int dh, int digit, COLORREF on)
{
    int t = dw / 6, vlen, m;
    if (t < 3) t = 3;
    vlen = (dh - 3 * t) / 2; if (vlen < 1) vlen = 1;
    m = (digit >= 0 && digit <= 9) ? SEG[digit] : 0;
    if (m & 0x01) DCWinFill(w, x + t,        y,                dw - 2 * t, t,    on);  // a
    if (m & 0x20) DCWinFill(w, x,            y + t,            t,          vlen, on);  // f
    if (m & 0x02) DCWinFill(w, x + dw - t,   y + t,            t,          vlen, on);  // b
    if (m & 0x40) DCWinFill(w, x + t,        y + t + vlen,     dw - 2 * t, t,    on);  // g
    if (m & 0x10) DCWinFill(w, x,            y + 2 * t + vlen, t,          vlen, on);  // e
    if (m & 0x04) DCWinFill(w, x + dw - t,   y + 2 * t + vlen, t,          vlen, on);  // c
    if (m & 0x08) DCWinFill(w, x + t,        y + dh - t,       dw - 2 * t, t,    on);  // d
}

static void DrawClock(DCWin *w, SYSTEMTIME *t)
{
    const COLORREF BG = RGB(8, 10, 18), ON = RGB(0, 240, 140);
    const COLORREF BARBG = RGB(24, 30, 44), BARFG = RGB(0, 180, 220), DATEFG = RGB(200, 210, 230);
    int  cw = CW, ch = CH, dh, dw, gap, colw, totw, x0, y0, x, by, bh, len, tx;
    int  hh = t->wHour, mm = t->wMinute, ss = t->wSecond;
    WCHAR line[64];

    DCWinClientSize(w, &cw, &ch);
    dh = (ch * 42) / 100; if (dh < 24) dh = 24;
    dw = (dh * 60) / 100; if (dw < 14) dw = 14;
    gap = dw / 5; if (gap < 2) gap = 2;
    colw = dw / 2;
    totw = 4 * dw + 3 * gap + colw + 2 * gap;
    x0 = (cw - totw) / 2; if (x0 < 4) x0 = 4;
    y0 = (ch > 120) ? 12 : 6;

    DCWinBeginFrame(w);
    DCWinFillBg(w, BG);

    x = x0;
    Draw7(w, x, y0, dw, dh, hh / 10, ON); x += dw + gap;
    Draw7(w, x, y0, dw, dh, hh % 10, ON); x += dw + gap;
    if (ss & 1)                                     // blinking colon (on for odd seconds)
    {
        int d = dw / 6; if (d < 3) d = 3;
        DCWinFill(w, x + (colw - d) / 2, y0 + dh / 3 - d / 2,     d, d, ON);
        DCWinFill(w, x + (colw - d) / 2, y0 + 2 * dh / 3 - d / 2, d, d, ON);
    }
    x += colw + 2 * gap;
    Draw7(w, x, y0, dw, dh, mm / 10, ON); x += dw + gap;
    Draw7(w, x, y0, dw, dh, mm % 10, ON);

    // seconds bar (0..59 -> 0..full)
    by = y0 + dh + 10; bh = 6;
    if (by + bh < ch - 22)
    {
        DCWinFill(w, x0, by, totw, bh, BARBG);
        DCWinFill(w, x0, by, (totw * ss) / 59, bh, BARFG);
    }

    // date line, centred
    wsprintfW(line, L"%s, %s %d, %d", DOW[t->wDayOfWeek % 7], MON[(t->wMonth + 11) % 12], t->wDay, t->wYear);
    len = lstrlenW(line); tx = (cw - len * 6) / 2; if (tx < 2) tx = 2;
    DCWinText(w, tx, ch - 18, DATEFG, BG, line);

    DCWinEndFrame(w);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin     *w;
    SYSTEMTIME t;
    DWORD      key;
    int        lastSec = -1;

    w = DCWinOpen(70, 300, CW, CH, L"Clock", ICON_CLOCK);
    if (!w) { OutputDebugStringW(L"DCWCLOCK: DCWinOpen failed\r\n"); return 1; }

    for (;;)
    {
        int changed = 0;
        GetLocalTime(&t);
        if ((int)t.wSecond != lastSec) { lastSec = (int)t.wSecond; changed = 1; }
        if (DCWinResized(w)) changed = 1;
        if (changed) DrawClock(w, &t);

        while (DCWinPollKey(w, &key)) { /* clock ignores keys */ }
        if (DCWinShouldClose(w)) break;
        Sleep(100);
    }

    DCWinClose(w);
    return 0;
}
