//
// dcshell.c - Dreamcast CE hybrid desktop shell (DCWin compositor).
//
// A desktop (teal background, shortcut icons, taskbar, Start menu). Everything
// else is a separate windowed process drawn by the compositor: Explorer
// (dcwexp.exe), Calculator, Clock. Desktop/Start shortcuts with a path launch the
// Explorer at that path; shortcuts with an exe launch that app.
//
// Drawing: the whole frame is composited into a back buffer (dcgfx), presented to
// the volatile DC primary each loop. Fills/bevels are DirectDraw COLORFILL, text
// is GDI Arial, icons are color-keyed surfaces. Each layer (desktop -> windows
// back-to-front -> taskbar/Start) does its fills then a GetDC text pass, so
// overlapping windows clip correctly.
//
#include "dcgfx.h"
#include "dcwin.h"
#include "dcinput.h"

#define TASK_H    26
#define ROW_H     18            // Start-menu row height
#define TASK_Y    (SCREEN_H - TASK_H)
#define MENU_W    168

#define CL_DESKTOP  RGB(0, 128, 128)
#define CL_FACE     RGB(192, 192, 192)
#define CL_TITLE    RGB(0, 0, 128)
#define CL_SEL      RGB(0, 0, 128)
#define CL_TEXT     RGB(0, 0, 0)
#define CL_WHITE    RGB(255, 255, 255)

typedef struct
{
    const WCHAR *label;
    const WCHAR *path;     // open Explorer here, or NULL
    const WCHAR *exe;      // launch this app, or NULL
    int          icon;     // ICON_*
} SHORTCUT;

static const SHORTCUT s_desk[] =
{
    { L"My Dreamcast", L"\\",        NULL,            ICON_SWIRL },
    { L"CD-ROM",       L"\\CD-ROM",  NULL,            ICON_DRIVE },
    { L"Calculator",   NULL,         L"dcwcalc.exe",  ICON_APP },
    { L"Clock",        NULL,         L"dcwclock.exe", ICON_CLOCK },
    { L"Task Manager", NULL,         L"dcwtask.exe",  ICON_APP },
};
#define DESK_N (sizeof(s_desk) / sizeof(s_desk[0]))

static const SHORTCUT s_start[] =
{
    { L"My Dreamcast", L"\\",        NULL,            ICON_SWIRL },
    { L"Windows",      L"\\Windows", NULL,            ICON_FOLDER },
    { L"CD-ROM",       L"\\CD-ROM",  NULL,            ICON_DRIVE },
    { L"Task Manager", NULL,         L"dcwtask.exe",  ICON_APP },
    { L"Shut Down...", NULL,         NULL,            ICON_FILE },
};
#define START_N (sizeof(s_start) / sizeof(s_start[0]))

static int   s_deskSel  = 0;
static int   s_menuOpen = 0;
static int   s_menuSel  = 0;
static int   s_dirty    = 1;
static int   s_focus    = -1;   // focused window index, or -1 = desktop
static int   s_wasInUse[DCWIN_MAXWIN];
static DWORD s_lastGen[DCWIN_MAXWIN];   // last composited gen per window
static DWORD s_lastExec = 0;    // last processed exec request
static HWND  s_hwnd     = NULL;
static BOOL  s_diKbd    = FALSE; // DI keyboard acquired (else fall back to WM_KEYDOWN)
static int   s_wmMouseSeen = 0;  // logged once when GWES mouse messages arrive
static int   s_cx = SCREEN_W / 2, s_cy = SCREEN_H / 2;   // pointer position

static DcShared *s_shared    = NULL;
static HANDLE    s_sharedMap = NULL;

// seqlock snapshot of each window's command list (avoids reading a half-written frame)
static DcCmd s_snap[DCWIN_MAXWIN][DCWIN_MAXCMD];
static int   s_snapN[DCWIN_MAXWIN];

static void DbgStr(const WCHAR *s) { OutputDebugStringW(s); }

//
// Compositor shared section + launching
//
static void InitShared(void)
{
    s_sharedMap = CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DcShared), DCWIN_SECTION);
    if (!s_sharedMap) { DbgStr(L"DCSHELL: shared section create FAILED\r\n"); return; }
    s_shared = (DcShared *)MapViewOfFile(s_sharedMap, FILE_MAP_WRITE, 0, 0, sizeof(DcShared));
    if (!s_shared) { DbgStr(L"DCSHELL: shared section map FAILED\r\n"); return; }
    memset(s_shared, 0, sizeof(DcShared));
    s_shared->magic = DCWIN_MAGIC;
    DbgStr(L"DCSHELL: compositor ready\r\n");
}

static void LaunchApp(const WCHAR *exe, const WCHAR *args)
{
    PROCESS_INFORMATION pi;
    WCHAR               cl[MAX_PATH];
    if (!s_shared || !exe)
        return;
    if (args)
        lstrcpyW(cl, args);
    if (CreateProcessW(exe, args ? cl : NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);    // detached; we poll the shared section each frame
        DbgStr(L"DCSHELL: launched app\r\n");
    }
    else DbgStr(L"DCSHELL: CreateProcess(app) FAILED\r\n");
}

// dcw*.exe are windowed DCWin apps (composited); anything else gets the display.
static BOOL IsDcwApp(const WCHAR *path)
{
    const WCHAR *b = path, *p;
    for (p = path; *p; p++) if (*p == L'\\') b = p + 1;
    return (b[0]|32) == 'd' && (b[1]|32) == 'c' && (b[2]|32) == 'w';
}

static void ShellLaunch(const WCHAR *exe)
{
    if (IsDcwApp(exe))
        LaunchApp(exe, NULL);                 // windowed, composited
    else
    {
        DbgStr(L"DCSHELL: fullscreen launch (display hand-off)\r\n");
        GfxLaunch(exe);                       // release display -> run -> reclaim
        s_dirty = 1;
    }
}

//
// Window focus management
//
static int CountWindows(void)
{
    int i, n = 0;
    if (!s_shared)
        return 0;
    for (i = 0; i < DCWIN_MAXWIN; i++)
        if (s_shared->win[i].inUse)
            n++;
    return n;
}

// Reap windows whose owner process died without DCWinClose (e.g. a Task Manager
// "end task", or a crash) - otherwise the slot stays inUse and ghosts forever.
// Liveness is an OpenProcess(ownerPid) probe; s_reapAccess is the access flag that
// worked on our own (live) pid at startup, or 0 if OpenProcess is unusable here.
static DWORD s_reapAccess = 0;

static void ProbeReap(void)
{
    static const DWORD tryAccess[] = { 0x0400 /*PROCESS_QUERY_INFORMATION*/, PROCESS_ALL_ACCESS, 0 };
    int i;
    s_reapAccess = 0;
    __try
    {
        for (i = 0; i < (int)(sizeof(tryAccess) / sizeof(tryAccess[0])); i++)
        {
            HANDLE h = OpenProcess(tryAccess[i], FALSE, GetCurrentProcessId());
            if (h) { CloseHandle(h); s_reapAccess = tryAccess[i]; break; }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { s_reapAccess = 0; }
    DbgStr(s_reapAccess ? L"DCSHELL: dead-window reaper active\r\n"
                        : L"DCSHELL: OpenProcess unusable, reaper off\r\n");
}

static void ReapDeadWindows(void)
{
    int i;
    if (!s_shared || !s_reapAccess)
        return;
    for (i = 0; i < DCWIN_MAXWIN; i++)
    {
        HANDLE h;
        if (!s_shared->win[i].inUse || !s_shared->win[i].ownerPid)
            continue;
        h = OpenProcess(s_reapAccess, FALSE, s_shared->win[i].ownerPid);
        if (h) { CloseHandle(h); continue; }     // owner alive
        s_shared->win[i].inUse = 0;              // owner gone -> free the ghost slot
        s_dirty = 1;
    }
}

// auto-focus newly-appeared windows; drop focus when the focused one closes
static void FixupFocus(void)
{
    int i;
    if (!s_shared)
        return;
    if (s_focus >= 0 && !s_shared->win[s_focus].inUse)
        s_focus = -1;
    for (i = 0; i < DCWIN_MAXWIN; i++)
    {
        int now = s_shared->win[i].inUse ? 1 : 0;
        if (now != s_wasInUse[i])
            s_dirty = 1;                    // a window opened/closed -> recompose
        if (now && !s_wasInUse[i])
            s_focus = i;
        s_wasInUse[i] = now;
    }
}

// cycle focus: current -> next in-use window -> ... -> desktop (-1) -> wrap
static void CycleFocus(void)
{
    int i, cur = s_focus;
    if (!s_shared)
        return;
    for (i = 0; i < DCWIN_MAXWIN; i++)
    {
        cur++;
        if (cur >= DCWIN_MAXWIN) { s_focus = -1; return; }
        if (s_shared->win[cur].inUse) { s_focus = cur; return; }
    }
    s_focus = -1;
}

//
// Rendering. Each layer: COLORFILL/icon fills (DC unlocked) then a GetDC text pass.
//
static void RenderDesktopFills(void)
{
    int i, y;

    GfxFill(0, 0, SCREEN_W, TASK_Y, CL_DESKTOP);
    for (i = 0; i < (int)DESK_N; i++)
    {
        y = 24 + i * 76;
        if (i == s_deskSel && !s_menuOpen)
            GfxFill(14, y + 36, 110, y + 52, CL_TITLE);     // label highlight
        GfxIconBig(s_desk[i].icon, 46, y);                  // 32x32 icon
    }
}

static void RenderDesktopText(HDC hdc)
{
    int i, y;
    for (i = 0; i < (int)DESK_N; i++)
    {
        COLORREF bg = (i == s_deskSel && !s_menuOpen) ? CL_TITLE : CL_DESKTOP;
        y = 24 + i * 76;
        GfxText(hdc, 18, y + 37, CL_WHITE, bg, g_FontUI, s_desk[i].label);
    }
}

static void RenderTaskbarFills(void)
{
    RECT rc;

    GfxFill(0, TASK_Y, SCREEN_W, SCREEN_H, CL_FACE);
    SetRect(&rc, 0, TASK_Y, SCREEN_W, SCREEN_H); GfxBevel(&rc, TRUE);
    SetRect(&rc, 4, TASK_Y + 3, 72, SCREEN_H - 3);
    GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);

    if (s_shared)                       // one button per open window (focused = pressed)
    {
        int bx = 80, k;
        for (k = 0; k < DCWIN_MAXWIN; k++)
        {
            if (!s_shared->win[k].inUse)
                continue;
            SetRect(&rc, bx, TASK_Y + 3, bx + 110, SCREEN_H - 3);
            GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE);
            GfxBevel(&rc, (k == s_focus) ? FALSE : TRUE);
            GfxIcon((int)s_shared->win[k].icon, bx + 4, TASK_Y + 6);
            bx += 116;
        }
    }

    if (s_menuOpen)
    {
        int h = (int)START_N * ROW_H + 8;
        int my = TASK_Y - h + 4, i;
        SetRect(&rc, 4, TASK_Y - h, 4 + MENU_W, TASK_Y);
        GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);
        if (s_menuSel >= 0 && s_menuSel < (int)START_N)
            GfxFill(8, my + s_menuSel * ROW_H, MENU_W, my + (s_menuSel + 1) * ROW_H, CL_SEL);
        for (i = 0; i < (int)START_N; i++)
            GfxIcon(s_start[i].icon, 10, my + i * ROW_H + 1);
    }
}

static void RenderBarsText(HDC hdc)
{
    SYSTEMTIME t;
    WCHAR      clk[16];
    int        i;

    GfxText(hdc, 12, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontBold, L"Start");
    GetLocalTime(&t);
    wsprintfW(clk, L"%02d:%02d", t.wHour, t.wMinute);
    GfxText(hdc, SCREEN_W - 44, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, clk);

    if (s_shared)
    {
        int bx = 80, k;
        for (k = 0; k < DCWIN_MAXWIN; k++)
        {
            if (!s_shared->win[k].inUse)
                continue;
            GfxText(hdc, bx + 24, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, s_shared->win[k].title);
            bx += 116;
        }
    }

    if (s_menuOpen)
    {
        int h  = (int)START_N * ROW_H + 8;
        int my = TASK_Y - h + 4;
        for (i = 0; i < (int)START_N; i++)
        {
            COLORREF fg = (i == s_menuSel) ? CL_WHITE : CL_TEXT;
            COLORREF bg = (i == s_menuSel) ? CL_SEL : CL_FACE;
            GfxText(hdc, 30, my + i * ROW_H + 1, fg, bg, g_FontUI, s_start[i].label);
        }
    }
}

static void SnapWindows(void)
{
    int wi, i, tries;

    if (!s_shared)
        return;
    for (wi = 0; wi < DCWIN_MAXWIN; wi++)
    {
        DcWindow *w = &s_shared->win[wi];
        s_snapN[wi] = 0;
        if (!w->inUse)
            continue;
        for (tries = 0; tries < 8; tries++)
        {
            DWORD g1 = w->gen, g2;
            int   cnt;
            if (g1 & 1) { Sleep(0); continue; }              // client mid-write
            cnt = (int)w->cmdCount;
            if (cnt > DCWIN_MAXCMD) cnt = DCWIN_MAXCMD;
            for (i = 0; i < cnt; i++) s_snap[wi][i] = w->cmd[i];
            g2 = w->gen;
            if (g1 == g2) { s_snapN[wi] = cnt; break; }      // consistent
        }
    }
}

static void DrawWinFills(DcWindow *w, int wi, BOOL active)
{
    RECT fr;
    int  i;

    SetRect(&fr, w->x - 2, w->y - 18, w->x + w->w + 2, w->y + w->h + 2);       // frame
    GfxFill(fr.left, fr.top, fr.right, fr.bottom, CL_FACE);
    GfxBevel(&fr, TRUE);
    GfxFill(w->x - 2, w->y - 18, w->x + w->w + 2, w->y - 2, active ? CL_TITLE : RGB(112,112,112));
    GfxIcon((int)w->icon, w->x, w->y - 18);                                    // title-bar icon
    GfxFill(w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3, CL_FACE);  // close box
    { RECT cb; SetRect(&cb, w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3); GfxBevel(&cb, TRUE); }

    for (i = 0; i < s_snapN[wi]; i++)
    {
        DcCmd *c = &s_snap[wi][i];
        if (c->op == DCOP_FILL)
            GfxFill(w->x + c->x, w->y + c->y, w->x + c->x + c->w, w->y + c->y + c->h, c->color);
        else if (c->op == DCOP_ICON)
            GfxIcon((int)c->color, w->x + c->x, w->y + c->y);
    }
}

static void DrawWinText(HDC hdc, DcWindow *w, int wi, BOOL active)
{
    int i;

    GfxText(hdc, w->x + 18, w->y - 16, CL_WHITE, active ? CL_TITLE : RGB(112,112,112), g_FontBold, w->title);
    GfxText(hdc, w->x + w->w - 11, w->y - 16, CL_TEXT, CL_FACE, g_FontUI, L"X");
    for (i = 0; i < s_snapN[wi]; i++)
    {
        DcCmd *c = &s_snap[wi][i];
        if (c->op == DCOP_TEXT)
            GfxText(hdc, w->x + c->x, w->y + c->y, c->color, c->color2, g_FontUI, c->text);
    }
}

// Composite ONE window completely (fills, then text) so an overlapping window's
// text never lands on a window above it.
static void RenderWindow(int wi, BOOL active)
{
    HDC       hdc;
    DcWindow *w = &s_shared->win[wi];

    DrawWinFills(w, wi, active);
    hdc = GfxLockDC();
    if (hdc) { DrawWinText(hdc, w, wi, active); GfxUnlockDC(hdc); }
}

static void Render(void)
{
    HDC hdc;
    int i;

    SnapWindows();

    // layer 0: desktop
    RenderDesktopFills();
    hdc = GfxLockDC();
    if (hdc) { RenderDesktopText(hdc); GfxUnlockDC(hdc); }

    // layers 1..N: app windows, back-to-front, each composited fully
    if (s_shared)
    {
        for (i = 0; i < DCWIN_MAXWIN; i++)
            if (s_shared->win[i].inUse && i != s_focus)
                RenderWindow(i, FALSE);
        if (s_focus >= 0 && s_shared->win[s_focus].inUse)
            RenderWindow(s_focus, TRUE);
    }

    // top layer: taskbar + Start menu
    RenderTaskbarFills();
    hdc = GfxLockDC();
    if (hdc) { RenderBarsText(hdc); GfxUnlockDC(hdc); }
    // NB: the pointer is NOT drawn here - it is overlaid on the primary in
    // GfxPresent so moving it never triggers this recomposite.
}

//
// Input
//
static void ActivateStartItem(void)
{
    const SHORTCUT *s = &s_start[s_menuSel];
    s_menuOpen = 0;
    if (s->path)
        LaunchApp(L"dcwexp.exe", s->path);     // Explorer window at this path
    else if (s->exe)
        LaunchApp(s->exe, NULL);
}

static void OnKey(WPARAM wp)
{
    s_dirty = 1;

    if (wp == VK_TAB)               // task-switch; or Start menu when no windows open
    {
        if (CountWindows() == 0) { s_menuOpen = !s_menuOpen; s_menuSel = 0; }
        else CycleFocus();
        return;
    }

    if (s_focus >= 0 && s_shared && s_shared->win[s_focus].inUse)
    {
        DcWindow *w = &s_shared->win[s_focus];     // focused window captures input
        if (wp == VK_ESCAPE) { w->wantClose = 1; return; }
        w->in[w->inHead % DCWIN_MAXIN].type = 1;
        w->in[w->inHead % DCWIN_MAXIN].key  = (DWORD)wp;
        w->inHead++;
        return;
    }

    if (s_menuOpen)
    {
        if (wp == VK_UP   && s_menuSel > 0)              s_menuSel--;
        if (wp == VK_DOWN && s_menuSel < (int)START_N-1) s_menuSel++;
        if (wp == VK_RETURN)                             ActivateStartItem();
        if (wp == VK_ESCAPE)                             s_menuOpen = 0;
        return;
    }

    // desktop
    if (wp == VK_UP   && s_deskSel > 0)             s_deskSel--;
    if (wp == VK_DOWN && s_deskSel < (int)DESK_N-1) s_deskSel++;
    if (wp == VK_RETURN)
    {
        if (s_desk[s_deskSel].path)
            LaunchApp(L"dcwexp.exe", s_desk[s_deskSel].path);   // Explorer window
        else
            LaunchApp(s_desk[s_deskSel].exe, NULL);
    }
}

// Cursor hit-test (top-down): Start menu -> Start button -> taskbar buttons ->
// windows (close box / body) -> desktop icons. Returns 1 if the click was
// consumed by a target (close box, taskbar/start, desktop icon), 0 if it landed
// on a window body (focus it, but let an Enter through to the window) or empty
// space. The controller's "A" uses the return: consumed -> done; else -> Enter.
static int HandleClick(int x, int y)
{
    int k;
    s_dirty = 1;

    if (s_menuOpen)
    {
        int h = (int)START_N * ROW_H + 8, my = TASK_Y - h + 4;
        if (x >= 4 && x < 4 + MENU_W && y >= my && y < my + (int)START_N * ROW_H)
        { s_menuSel = (y - my) / ROW_H; ActivateStartItem(); return 1; }
        s_menuOpen = 0;                 // click elsewhere closes the menu
        return 1;
    }
    if (y >= TASK_Y && x >= 4 && x < 72)            // Start button
    { s_menuOpen = !s_menuOpen; s_menuSel = 0; return 1; }
    if (y >= TASK_Y && s_shared)                    // taskbar window buttons
    {
        int bx = 80;
        for (k = 0; k < DCWIN_MAXWIN; k++)
        { if (!s_shared->win[k].inUse) continue; if (x >= bx && x < bx + 110) { s_focus = k; return 1; } bx += 116; }
    }
    if (s_shared)                                   // windows: topmost (focused) first
    {
        int order[DCWIN_MAXWIN + 1], n = 0, j;
        if (s_focus >= 0 && s_shared->win[s_focus].inUse) order[n++] = s_focus;
        for (k = 0; k < DCWIN_MAXWIN; k++) if (s_shared->win[k].inUse && k != s_focus) order[n++] = k;
        for (j = 0; j < n; j++)
        {
            DcWindow *w = &s_shared->win[order[j]];
            if (x >= w->x - 2 && x < w->x + w->w + 2 && y >= w->y - 18 && y < w->y + w->h + 2)
            {
                if (x >= w->x + w->w - 14 && x <= w->x + w->w + 1 && y >= w->y - 16 && y <= w->y - 3)
                { w->wantClose = 1; return 1; }     // close box -> consumed
                s_focus = order[j];                 // body -> focus it, fall through to Enter
                return 0;
            }
        }
    }
    for (k = 0; k < (int)DESK_N; k++)               // desktop icons
    {
        int iy = 24 + k * 76;
        if (x >= 14 && x < 110 && y >= iy && y < iy + 54)
        {
            s_deskSel = k;
            if (s_desk[k].path) LaunchApp(L"dcwexp.exe", s_desk[k].path);
            else                LaunchApp(s_desk[k].exe, NULL);
            return 1;
        }
    }
    return 0;                                        // empty space
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;                       // we own the surface
    case WM_PAINT:
        ValidateRect(hwnd, NULL);       // continuous present shows the frame; don't storm
        return 0;
    case WM_KEYDOWN:
        if (!s_diKbd) OnKey(wp);        // fallback when DI keyboard not available
        return 0;
    case WM_MOUSEMOVE:                  // GWES mouse fallback (if DC mouse isn't on DI)
        DInSetCursor((short)LOWORD(lp), (short)HIWORD(lp));
        if (!s_wmMouseSeen) { s_wmMouseSeen = 1; DbgStr(L"DCSHELL: WM mouse active\r\n"); }
        return 0;
    case WM_LBUTTONDOWN:
        DInSetCursor((short)LOWORD(lp), (short)HIWORD(lp));
        DInPostClick();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    WNDCLASSW wc;
    MSG       msg;
    DWORD     next;
    int       i;
    BOOL      recomposited;

    DbgStr(L"DCSHELL: WinMain enter (hybrid desktop)\r\n");

    InitShared();

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"DCSHELL";
    RegisterClassW(&wc);
    s_hwnd = CreateWindowExW(0, L"DCSHELL", L"DCShell", WS_VISIBLE, 0, 0, SCREEN_W, SCREEN_H,
                             NULL, NULL, hInst, NULL);

    if (!GfxInit(s_hwnd))
    {
        DbgStr(L"DCSHELL: GfxInit failed\r\n");
        return 1;
    }
    DbgStr(L"DCSHELL: desktop up\r\n");

    s_diKbd = DInInit(s_hwnd);          // polled keyboard (low-latency) + controller pointer
    DbgStr(s_diKbd ? L"DCSHELL: DI keyboard active\r\n" : L"DCSHELL: DI keyboard absent, WM fallback\r\n");
    ProbeReap();                        // is OpenProcess usable for dead-window reaping?

    next = GetTickCount() + 1000;
    for (;;)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                DInShutdown();
                GfxShutdown();
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        DInUpdate();                    // poll DI devices once per frame
        {
            DWORD vk;
            while (DInNextKey(&vk)) OnKey(vk);
        }
        if (DInHasPointer())
            DInCursor(&s_cx, &s_cy);    // pointer rides the present overlay; no recomposite
        if (DInTookClick())             // mouse: pure cursor click at the pointer
            HandleClick(s_cx, s_cy);
        if (DInTookActivate())          // controller A: click the cursor target, else
            if (!HandleClick(s_cx, s_cy)) OnKey(VK_RETURN);   // activate the selection

        FixupFocus();                   // auto-focus new windows, drop closed ones
        if (s_shared && s_shared->execSeq != s_lastExec)   // a window asked to launch an app
        {
            s_lastExec = s_shared->execSeq;
            ShellLaunch(s_shared->execPath);
        }
        if (GetTickCount() >= next)     // clock tick -> repaint + reap dead windows
        {
            s_dirty = 1;
            next += 1000;
            ReapDeadWindows();          // free slots whose owner process is gone
        }
        // re-render only when a window published a new frame (digit typed, clock tick)
        if (s_shared)
            for (i = 0; i < DCWIN_MAXWIN; i++)
            {
                DWORD g = s_shared->win[i].inUse ? s_shared->win[i].gen : 0;
                if (g != s_lastGen[i]) { s_dirty = 1; s_lastGen[i] = g; }
            }
        recomposited = FALSE;
        if (s_dirty)
        {
            Render();
            s_dirty = 0;
            recomposited = TRUE;        // tells GfxPresent the saved cursor region is stale
        }
        // present every frame (primary is volatile); the pointer is composited into
        // the back buffer (save-under) and sent in one blit -> no flicker, and
        // cursor motion costs only a 16x16 save-under, not a recomposite
        if (GfxPresent(s_cx, s_cy, DInHasPointer(), recomposited))
            s_dirty = 1;                // VRAM surface lost+restored -> redraw next frame
        Sleep(8);                       // ~120 Hz poll: low input latency; present stays cheap
    }
}
