//
// dcwcalc.c - windowed Calculator (its own process), built on the DCWin client
// library. Demonstrates a compositor client in ~40 lines.
//
#include "dcwlib.h"

#define CW 180
#define CH 92

static WCHAR g_disp[24] = L"0";

static void HandleKey(DWORD key)
{
    int len = lstrlenW(g_disp);
    if (key >= '0' && key <= '9')
    {
        if (g_disp[0] == L'0' && len == 1)
            g_disp[0] = (WCHAR)key;
        else if (len < 18)
        {
            g_disp[len] = (WCHAR)key;
            g_disp[len + 1] = 0;
        }
    }
    else if (key == 'C' || key == VK_BACK)
    {
        g_disp[0] = L'0';
        g_disp[1] = 0;
    }
}

static void Draw(DCWin *w)
{
    int cw = CW, ch = CH;
    DCWinClientSize(w, &cw, &ch);
    DCWinBeginFrame(w);
    DCWinFillBg(w, RGB(192, 192, 192));             // background fills the window
    DCWinFill(w, 8, 8, cw - 16, 26, RGB(255, 255, 255));   // display spans the width
    DCWinText(w, 12, 13, RGB(0, 0, 0), RGB(255, 255, 255), g_disp);
    DCWinText(w, 10, 48, RGB(0, 0, 0), RGB(192, 192, 192), L"0-9 enter digits");
    DCWinText(w, 10, 64, RGB(0, 0, 0), RGB(192, 192, 192), L"C clears, Esc closes");
    DCWinIcon(w, cw - 24, 48, ICON_APP);            // client-drawn icon (DCOP_ICON)
    DCWinEndFrame(w);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key;

    w = DCWinOpen(210, 120, CW, CH, L"Calculator", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWCALC: DCWinOpen failed\r\n"); return 1; }
    Draw(w);

    for (;;)
    {
        int changed = 0;
        while (DCWinPollKey(w, &key)) { HandleKey(key); changed = 1; }
        if (DCWinResized(w)) changed = 1;               // shell resized us -> redraw to fit
        if (changed) Draw(w);
        if (DCWinShouldClose(w)) break;
        Sleep(20);
    }

    DCWinClose(w);
    return 0;
}
