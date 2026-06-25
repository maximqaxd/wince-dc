//
// dcwcalc.c - a windowed Calculator running as its OWN process, drawn by the
// dcshell compositor (DCWin protocol). Proves real windowed multitasking on the
// Dreamcast: separate process, its own window, opened and quit from the shell.
//
// It claims a window slot in the shared section, publishes a draw-command list
// (background, display field, digits), and reads key events the shell routes to
// it. The shell composites the window frame + commands onto the desktop.
//
#include "dcwin.h"

static WCHAR g_disp[24] = L"0";

static void Build(DcWindow *w)
{
    int n = 0;

    w->cmd[n].op = DCOP_FILL; w->cmd[n].x = 0; w->cmd[n].y = 0; w->cmd[n].w = w->w; w->cmd[n].h = w->h;
    w->cmd[n].color = RGB(192, 192, 192); n++;                                   // face background

    w->cmd[n].op = DCOP_FILL; w->cmd[n].x = 8; w->cmd[n].y = 8; w->cmd[n].w = w->w - 16; w->cmd[n].h = 26;
    w->cmd[n].color = RGB(255, 255, 255); n++;                                   // display field

    w->cmd[n].op = DCOP_TEXT; w->cmd[n].x = w->w - 14; w->cmd[n].y = 13;         // right-aligned-ish
    w->cmd[n].color = RGB(0, 0, 0); w->cmd[n].color2 = RGB(255, 255, 255);
    lstrcpyW(w->cmd[n].text, g_disp); n++;

    w->cmd[n].op = DCOP_TEXT; w->cmd[n].x = 10; w->cmd[n].y = 48;
    w->cmd[n].color = RGB(0, 0, 0); w->cmd[n].color2 = RGB(192, 192, 192);
    lstrcpyW(w->cmd[n].text, L"0-9 enter digits"); n++;

    w->cmd[n].op = DCOP_TEXT; w->cmd[n].x = 10; w->cmd[n].y = 64;
    w->cmd[n].color = RGB(0, 0, 0); w->cmd[n].color2 = RGB(192, 192, 192);
    lstrcpyW(w->cmd[n].text, L"C clears, Esc closes"); n++;

    w->cmdCount = n;
    w->gen++;
}

static void HandleKey(DWORD key)
{
    int len = lstrlenW(g_disp);

    if (key >= '0' && key <= '9')
    {
        if (g_disp[0] == L'0' && len == 1)
            g_disp[0] = (WCHAR)key;          // replace leading zero
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    HANDLE     hMap;
    DcShared  *sh;
    DcWindow  *w = NULL;
    int        i, idx = -1;

    OutputDebugStringW(L"DCWCALC: start\r\n");
    hMap = CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DcShared), DCWIN_SECTION);
    if (!hMap) { OutputDebugStringW(L"DCWCALC: CreateFileMapping FAILED\r\n"); return 1; }
    sh = (DcShared *)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(DcShared));
    if (!sh) { OutputDebugStringW(L"DCWCALC: MapViewOfFile FAILED\r\n"); CloseHandle(hMap); return 1; }

    for (i = 0; i < DCWIN_MAXWIN; i++)
        if (!sh->win[i].inUse) { idx = i; break; }
    if (idx < 0) { OutputDebugStringW(L"DCWCALC: no free window slot\r\n"); return 1; }

    w = &sh->win[idx];
    memset(w, 0, sizeof(*w));
    w->ownerPid = GetCurrentProcessId();
    w->x = 210; w->y = 130; w->w = 180; w->h = 92;
    lstrcpyW(w->title, L"Calculator");
    Build(w);
    w->inUse = 1;                            // publish only once fully set up
    OutputDebugStringW(L"DCWCALC: window up\r\n");

    for (;;)
    {
        while (w->inTail != w->inHead)
        {
            DcInput ev = w->in[w->inTail % DCWIN_MAXIN];
            w->inTail++;
            if (ev.type == 1)
            {
                HandleKey(ev.key);
                Build(w);
            }
        }
        if (w->wantClose)
        {
            w->inUse = 0;                     // release the slot
            break;
        }
        Sleep(33);
    }

    OutputDebugStringW(L"DCWCALC: closing\r\n");
    UnmapViewOfFile(sh);
    CloseHandle(hMap);
    return 0;
}
