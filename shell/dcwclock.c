//
// dcwclock.c - a second windowed app (its own process): a live clock. Second
// DCWin client, proving multi-window compositing.
//
#include "dcwlib.h"

#define CW 140
#define CH 56

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin     *w;
    SYSTEMTIME t;
    WCHAR      s[16];
    DWORD      key;

    w = DCWinOpen(70, 300, CW, CH, L"Clock");
    if (!w) { OutputDebugStringW(L"DCWCLOCK: DCWinOpen failed\r\n"); return 1; }

    for (;;)
    {
        GetLocalTime(&t);
        wsprintfW(s, L"%02d:%02d:%02d", t.wHour, t.wMinute, t.wSecond);

        DCWinBeginFrame(w);
        DCWinFill(w, 0, 0, CW, CH, RGB(192, 192, 192));
        DCWinFill(w, 8, 8, CW - 16, 32, RGB(0, 0, 64));
        DCWinText(w, 30, 16, RGB(0, 255, 128), RGB(0, 0, 64), s);
        DCWinEndFrame(w);

        while (DCWinPollKey(w, &key)) { /* clock ignores keys */ }
        if (DCWinShouldClose(w)) break;
        Sleep(250);
    }

    DCWinClose(w);
    return 0;
}
