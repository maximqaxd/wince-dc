//
// dcwlog.c - the "System Log" window: renders the DCSYSLOG ring (see syslog.h) as a
// green-on-black console. Auto-follows the tail; Up/Down scroll back through history.
//
#include "dcwlib.h"
#include "syslog.h"

#define LW  380
#define LH  240
#define ROW 13

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin        *w;
    SysLogShared *sl;
    DWORD         key;
    LONG          lastHead = -1;
    int           scroll = 0;

    w = DCWinOpen(60, 60, LW, LH, L"System Log", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWLOG: DCWinOpen failed\r\n"); return 1; }
    sl = SysLogMap(0);                                   // map (creating if needed)

    for (;;)
    {
        int  changed = 0, cw = LW, ch = LH, vis, i, y;
        LONG head = sl ? sl->head : 0;

        DCWinClientSize(w, &cw, &ch);
        while (DCWinPollKey(w, &key))
        {
            if (key == VK_UP)               { scroll++; changed = 1; }
            else if (key == VK_DOWN && scroll > 0) { scroll--; changed = 1; }
            else if (key == VK_NEXT || key == VK_HOME) { scroll = 0; changed = 1; }  // jump to tail
        }
        if (head != lastHead) { lastHead = head; if (scroll == 0) changed = 1; }     // new lines (following tail)
        if (DCWinResized(w)) changed = 1;

        if (changed)
        {
            vis = (ch - 2) / ROW; if (vis < 1) vis = 1; if (vis > 44) vis = 44;       // command-budget cap
            if (scroll > head - vis) scroll = head > vis ? (int)(head - vis) : 0;
            DCWinBeginFrame(w);
            DCWinFillBg(w, RGB(0, 0, 0));
            for (i = 0; i < vis; i++)
            {
                LONG idx = head - vis - scroll + i;                                  // bottom line = newest-scroll
                if (idx < 0 || (head - idx) > SYSLOG_LINES) continue;                // before start / rolled off
                y = 1 + i * ROW;
                DCWinText(w, 4, y, RGB(0, 255, 80), RGB(0, 0, 0), sl->line[(DWORD)idx % SYSLOG_LINES]);
            }
            DCWinEndFrame(w);
        }
        if (DCWinShouldClose(w)) break;
        Sleep(120);
    }

    DCWinClose(w);
    return 0;
}
