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
    { L"Network",      NULL,         L"dcwnet.exe",   ICON_APP },
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
static int   s_deskDirty = 1;    // static desktop-layer cache needs a (re)build
static int   s_cachedSel = -1;   // s_deskSel baked into the desktop cache
static int   s_cachedMenu = -1;  // s_menuOpen baked into the desktop cache
static int   s_focus    = -1;   // focused window index, or -1 = desktop
static int   s_wasInUse[DCWIN_MAXWIN];
static DWORD s_lastGen[DCWIN_MAXWIN];   // last composited gen per window
static DWORD s_lastExec = 0;    // last processed exec request
static HWND  s_hwnd     = NULL;
static BOOL  s_diKbd    = FALSE; // DI keyboard acquired (else fall back to WM_KEYDOWN)
static int   s_wmMouseSeen = 0;  // logged once when GWES mouse messages arrive
static int   s_cx = SCREEN_W / 2, s_cy = SCREEN_H / 2;   // pointer position

// Desktop icon cell positions (top-left of the 96x54 clickable cell). Mutable so icons can
// be dragged. Initialised to the default left column by DeskPosInit().
static POINT s_deskPos[DESK_N];
static int   s_deskPosInit = 0;
#define ICELL_W 96
#define ICELL_H 54
static void DeskPosInit(void)
{
    int i;
    for (i = 0; i < (int)DESK_N; i++) { s_deskPos[i].x = 14; s_deskPos[i].y = 24 + i * 76; }
    s_deskPosInit = 1;
}

// Pointer drag state machine (mouse-L or controller-A held). Lets the user move/resize
// windows and move desktop shortcuts; a press with no drag is treated as a click on release.
#define DRAG_NONE   0
#define DRAG_WMOVE  1    // moving a window (grabbed its title bar)
#define DRAG_WSIZE  2    // resizing a window (grabbed its bottom-right corner)
#define DRAG_ICON   3    // moving a desktop shortcut
#define DRAG_THRESH 3    // px of motion before a press becomes a drag (vs a click)
#define WIN_MINW    80
#define WIN_MINH    40
static int s_dragKind = DRAG_NONE, s_dragTarget = -1;
static int s_dragOffX = 0, s_dragOffY = 0;     // cursor - target origin, at grab time
static int s_dragMoved = 0, s_ptrWas = 0, s_downX = 0, s_downY = 0;

// Window maximize/restore + minimize (shell-side, keyed by window slot). s_winRestore =
// {x,y,w,h} saved before maximizing. Both reset when a slot frees so a reused slot is normal.
static int s_winMax[DCWIN_MAXWIN];
static int s_winRestore[DCWIN_MAXWIN][4];
static int s_winMin[DCWIN_MAXWIN];        // window minimized (hidden; only its taskbar button shows)

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
        DbgStr(L"DCSHELL: fullscreen launch (display + input hand-off)\r\n");
        DInRelease();                         // hand input to the app (it may grab DI exclusively)
        GfxLaunch(exe);                       // release display -> run (blocks) -> reclaim
        DInReacquire();                       // app exited -> take input back
        s_dirty = 1;
        s_deskDirty = 1;                      // surfaces/icons rebuilt -> recache desktop
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
    int i, x, y;

    GfxFill(0, 0, SCREEN_W, TASK_Y, CL_DESKTOP);
    for (i = 0; i < (int)DESK_N; i++)
    {
        x = s_deskPos[i].x; y = s_deskPos[i].y;
        if (i == s_deskSel && !s_menuOpen)
            GfxFill(x, y + 36, x + ICELL_W, y + 52, CL_TITLE);   // label highlight
        GfxIconBig(s_desk[i].icon, x + 32, y);                   // 32x32 icon
    }
}

static void RenderDesktopText(HDC hdc)
{
    int i, x, y;
    for (i = 0; i < (int)DESK_N; i++)
    {
        COLORREF bg = (i == s_deskSel && !s_menuOpen) ? CL_TITLE : CL_DESKTOP;
        x = s_deskPos[i].x; y = s_deskPos[i].y;
        GfxText(hdc, x + 4, y + 37, CL_WHITE, bg, g_FontUI, s_desk[i].label);
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
            if (g1 & 1) { Sleep(1); continue; }              // client mid-write (Sleep(1):
            //            we run ABOVE_NORMAL, so Sleep(0) would never yield to the
            //            lower-priority client writing the frame -> it could never finish
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
    {   // maximize/restore box (left of close) + its glyph (a little window w/ a title bar)
        int mx0 = w->x + w->w - 30, my0 = w->y - 16;
        RECT mb; SetRect(&mb, mx0, my0, w->x + w->w - 15, w->y - 3);
        GfxFill(mb.left, mb.top, mb.right, mb.bottom, CL_FACE); GfxBevel(&mb, TRUE);
        GfxFill(mx0 + 3, my0 + 3, mx0 + 12, my0 + 10, CL_TEXT);    // outline
        GfxFill(mx0 + 4, my0 + 5, mx0 + 11, my0 + 9,  CL_FACE);    // interior (leaves a top "title bar")
    }
    {   // minimize box (left of maximize) + its glyph (a low bar)
        int nx0 = w->x + w->w - 46, ny0 = w->y - 16;
        RECT nb; SetRect(&nb, nx0, ny0, w->x + w->w - 31, w->y - 3);
        GfxFill(nb.left, nb.top, nb.right, nb.bottom, CL_FACE); GfxBevel(&nb, TRUE);
        GfxFill(nx0 + 3, ny0 + 8, nx0 + 12, ny0 + 10, CL_TEXT);    // the "minimize" bar
    }

    GfxSetClip(w->x, w->y, w->x + w->w, w->y + w->h);             // clip content to the client area
    for (i = 0; i < s_snapN[wi]; i++)
    {
        DcCmd *c = &s_snap[wi][i];
        if (c->op == DCOP_FILL)
            GfxFill(w->x + c->x, w->y + c->y, w->x + c->x + c->w, w->y + c->y + c->h, c->color);
        else if (c->op == DCOP_ICON)
            GfxIcon((int)c->color, w->x + c->x, w->y + c->y);
    }
    GfxClearClip();
}

static void DrawWinText(HDC hdc, DcWindow *w, int wi, BOOL active)
{
    int i;

    GfxText(hdc, w->x + 18, w->y - 16, CL_WHITE, active ? CL_TITLE : RGB(112,112,112), g_FontBold, w->title);
    GfxText(hdc, w->x + w->w - 11, w->y - 16, CL_TEXT, CL_FACE, g_FontUI, L"X");
    GfxSetClip(w->x, w->y, w->x + w->w, w->y + w->h);            // clip content text to the client area
    for (i = 0; i < s_snapN[wi]; i++)
    {
        DcCmd *c = &s_snap[wi][i];
        if (c->op == DCOP_TEXT)
            GfxText(hdc, w->x + c->x, w->y + c->y, c->color, c->color2, g_FontUI, c->text);
    }
    GfxClearClip();
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

// Repaint the static desktop layer (bg + icons + labels) into the dcgfx desktop
// cache. Called only when its visual inputs (selection, menu-open) change - never
// for clock ticks, window republishes, or focus changes.
static void RebuildDesktopCache(void)
{
    GfxBeginDesktopCache();
    RenderDesktopFills();
    RenderDesktopText((HDC)1);     // GfxText ignores the HDC; target is s_lockPix
    GfxEndDesktopCache();
    s_deskDirty  = 0;
    s_cachedSel  = s_deskSel;
    s_cachedMenu = s_menuOpen;
}

// Rebuild the scene: a PVR2 quad list, back-to-front (desktop -> windows -> taskbar)
// = submit order = painter's Z. The desktop layer is a cached vertex sub-list,
// rebuilt only when its inputs (selection / menu) change. GfxPresent consumes it.
static void Render(void)
{
    HDC hdc;
    int i;

    SnapWindows();

    // layer 0: desktop (cached vertex sub-list; rebuild only on layout change)
    if (s_deskDirty || s_deskSel != s_cachedSel || s_menuOpen != s_cachedMenu)
        RebuildDesktopCache();
    GfxBlitDesktopCache();

    // layers 1..N: app windows, back-to-front, each composited fully
    if (s_shared)
    {
        for (i = 0; i < DCWIN_MAXWIN; i++)
            if (s_shared->win[i].inUse && !s_winMin[i] && i != s_focus)
                RenderWindow(i, FALSE);
        if (s_focus >= 0 && s_shared->win[s_focus].inUse && !s_winMin[s_focus])
            RenderWindow(s_focus, TRUE);
    }

    // top layer: taskbar + Start menu
    RenderTaskbarFills();
    hdc = GfxLockDC();
    if (hdc) { RenderBarsText(hdc); GfxUnlockDC(hdc); }
    // The cursor is NOT in the scene here - GfxPresent appends it as the last quad.
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
    if (wp == VK_TAB)               // task-switch; or Start menu when no windows open
    {
        if (CountWindows() == 0) { s_menuOpen = !s_menuOpen; s_menuSel = 0; }
        else CycleFocus();
        s_dirty = 1;
        return;
    }

    // A focused window captures input: forward the key to its ring and return
    // WITHOUT marking the scene dirty. The shell must NOT recomposite here - the
    // window content hasn't changed yet; when the client processes the key and
    // republishes (gen bump) the loop's gen-change check triggers exactly one
    // render. (Marking dirty here recomposited stale frames every keypress and
    // kept the shell busy, starving the client -> the in-window nav lag.)
    if (s_focus >= 0 && s_shared && s_shared->win[s_focus].inUse)
    {
        DcWindow *w = &s_shared->win[s_focus];
        if (wp == VK_ESCAPE) { w->wantClose = 1; return; }   // FixupFocus redraws on close
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
        s_dirty = 1;
        return;
    }

    // desktop
    s_dirty = 1;
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

// Maximize/restore a window (toggle). Full-screen = the desktop area above the taskbar,
// leaving room for the title bar. Saves the pre-maximize geometry to restore.
static void ToggleMaximize(int wi)
{
    DcWindow *w;
    if (!s_shared || wi < 0 || wi >= DCWIN_MAXWIN || !s_shared->win[wi].inUse) return;
    w = &s_shared->win[wi];
    if (s_winMax[wi])
    {
        w->x = s_winRestore[wi][0]; w->y = s_winRestore[wi][1];
        w->w = s_winRestore[wi][2]; w->h = s_winRestore[wi][3];
        s_winMax[wi] = 0;
    }
    else
    {
        s_winRestore[wi][0] = w->x; s_winRestore[wi][1] = w->y;
        s_winRestore[wi][2] = w->w; s_winRestore[wi][3] = w->h;
        w->x = 2; w->y = 20;                              // title bar sits at y=2..20
        w->w = SCREEN_W - 4; w->h = TASK_Y - 22;          // fill down to the taskbar
        s_winMax[wi] = 1;
    }
    s_dirty = 1;
}

// Cursor hit-test (top-down): Start menu -> Start button -> taskbar buttons ->
// windows (close box / maximize box / body) -> desktop icons. Returns 1 if the click was
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
        { if (!s_shared->win[k].inUse) continue;
          if (x >= bx && x < bx + 110)            // restore a minimized window + focus it
          { if (s_winMin[k]) s_winMin[k] = 0; s_focus = k; s_dirty = 1; return 1; }
          bx += 116; }
    }
    if (s_shared)                                   // windows: topmost (focused) first
    {
        int order[DCWIN_MAXWIN + 1], n = 0, j;
        if (s_focus >= 0 && s_shared->win[s_focus].inUse) order[n++] = s_focus;
        for (k = 0; k < DCWIN_MAXWIN; k++) if (s_shared->win[k].inUse && k != s_focus) order[n++] = k;
        for (j = 0; j < n; j++)
        {
            DcWindow *w = &s_shared->win[order[j]];
            if (s_winMin[order[j]]) continue;        // minimized: not on the desktop
            if (x >= w->x - 2 && x < w->x + w->w + 2 && y >= w->y - 18 && y < w->y + w->h + 2)
            {
                if (x >= w->x + w->w - 14 && x <= w->x + w->w + 1 && y >= w->y - 16 && y <= w->y - 3)
                { w->wantClose = 1; return 1; }     // close box -> consumed
                if (x >= w->x + w->w - 30 && x <= w->x + w->w - 15 && y >= w->y - 16 && y <= w->y - 3)
                { ToggleMaximize(order[j]); return 1; }   // maximize/restore box
                if (x >= w->x + w->w - 46 && x <= w->x + w->w - 31 && y >= w->y - 16 && y <= w->y - 3)
                { s_winMin[order[j]] = 1; if (s_focus == order[j]) s_focus = -1; s_dirty = 1; return 1; }  // minimize
                s_focus = order[j];                 // body -> focus it, fall through to Enter
                return 0;
            }
        }
    }
    for (k = 0; k < (int)DESK_N; k++)               // desktop icons (mutable cell positions)
    {
        int ix = s_deskPos[k].x, iy = s_deskPos[k].y;
        if (x >= ix && x < ix + ICELL_W && y >= iy && y < iy + ICELL_H)
        {
            s_deskSel = k;
            if (s_desk[k].path) LaunchApp(L"dcwexp.exe", s_desk[k].path);
            else                LaunchApp(s_desk[k].exe, NULL);
            return 1;
        }
    }
    return 0;                                        // empty space
}

// ---- pointer drag: move/resize windows, move desktop shortcuts --------------------
// Hit-test at a pointer-DOWN to decide what (if anything) a drag would grab.
static void DragHitTest(int x, int y)
{
    int j, k;
    s_dragKind = DRAG_NONE; s_dragTarget = -1;

    if (s_shared && !s_menuOpen)                    // windows, top-most (focused) first
    {
        int order[DCWIN_MAXWIN + 1], n = 0;
        if (s_focus >= 0 && s_shared->win[s_focus].inUse) order[n++] = s_focus;
        for (k = 0; k < DCWIN_MAXWIN; k++) if (s_shared->win[k].inUse && k != s_focus) order[n++] = k;
        for (j = 0; j < n; j++)
        {
            DcWindow *w = &s_shared->win[order[j]];
            if (s_winMin[order[j]]) continue;          // minimized windows aren't on the desktop
            // bottom-right corner (12x12) -> resize
            if (x >= w->x + w->w - 12 && x <= w->x + w->w + 2 && y >= w->y + w->h - 12 && y <= w->y + w->h + 2)
            { s_dragKind = DRAG_WSIZE; s_dragTarget = order[j]; s_dragOffX = x - (w->x + w->w); s_dragOffY = y - (w->y + w->h); s_focus = order[j]; return; }
            // title bar (excluding the min/max/close boxes) -> move
            if (x >= w->x - 2 && x < w->x + w->w - 46 && y >= w->y - 18 && y < w->y - 2)
            { s_dragKind = DRAG_WMOVE; s_dragTarget = order[j]; s_dragOffX = x - w->x; s_dragOffY = y - w->y; s_focus = order[j]; return; }
        }
    }
    for (k = 0; k < (int)DESK_N; k++)               // desktop icon -> move
    {
        int ix = s_deskPos[k].x, iy = s_deskPos[k].y;
        if (x >= ix && x < ix + ICELL_W && y >= iy && y < iy + ICELL_H)
        { s_dragKind = DRAG_ICON; s_dragTarget = k; s_dragOffX = x - ix; s_dragOffY = y - iy; s_deskSel = k; return; }
    }
}

// Apply a drag while the pointer is held + has moved past the threshold.
static void DragApply(int x, int y)
{
    if (s_dragKind == DRAG_ICON && s_dragTarget >= 0)
    {
        int nx = x - s_dragOffX, ny = y - s_dragOffY;
        if (nx < 0) nx = 0; if (nx > SCREEN_W - ICELL_W) nx = SCREEN_W - ICELL_W;
        if (ny < 0) ny = 0; if (ny > TASK_Y - ICELL_H)   ny = TASK_Y - ICELL_H;
        s_deskPos[s_dragTarget].x = nx; s_deskPos[s_dragTarget].y = ny;
        s_deskDirty = 1;                            // icons live in the cached desktop layer
    }
    else if (s_shared && s_dragTarget >= 0 && s_shared->win[s_dragTarget].inUse)
    {
        DcWindow *w = &s_shared->win[s_dragTarget];
        if (s_dragKind == DRAG_WMOVE)
        {
            int nx = x - s_dragOffX, ny = y - s_dragOffY;
            if (ny < 18) ny = 18;                              // keep the title bar on-screen
            if (nx < -(w->w - 40)) nx = -(w->w - 40);
            if (nx > SCREEN_W - 40) nx = SCREEN_W - 40;
            if (ny > TASK_Y - 1) ny = TASK_Y - 1;
            w->x = nx; w->y = ny;
        }
        else if (s_dragKind == DRAG_WSIZE)
        {
            int nw = x - s_dragOffX - w->x, nh = y - s_dragOffY - w->y;
            if (nw < WIN_MINW) nw = WIN_MINW; if (nw > SCREEN_W) nw = SCREEN_W;
            if (nh < WIN_MINH) nh = WIN_MINH; if (nh > SCREEN_H) nh = SCREEN_H;
            w->w = nw; w->h = nh;
        }
    }
    s_dirty = 1;
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
    DWORD     next, nextPresent = 0, lt0;
    int       i, moved;

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
    DeskPosInit();                      // default desktop-icon positions (draggable thereafter)

    // Run a notch above client apps. We are the window server: input polling and the
    // cursor present must preempt a CPU-bound or fast-republishing client, or the
    // pointer stalls for a whole scheduler quantum while that client holds the SH-4.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    s_diKbd = DInInit(s_hwnd);          // polled keyboard (low-latency) + controller pointer
    DbgStr(s_diKbd ? L"DCSHELL: DI keyboard active\r\n" : L"DCSHELL: DI keyboard absent, WM fallback\r\n");
    ProbeReap();                        // is OpenProcess usable for dead-window reaping?

    next = GetTickCount() + 1000;
    for (;;)
    {
        lt0 = GetTickCount();           // frame start (for the limiter + input-poll cost)
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
        moved = 0;
        if (DInHasPointer())
        {
            int ox = s_cx, oy = s_cy;
            DInCursor(&s_cx, &s_cy);
            if (s_cx != ox || s_cy != oy) moved = 1;   // cursor moved -> needs a present
        }
        // Pointer press / drag / release. A press that doesn't drag is a CLICK on release
        // (preserving the old click + controller-activate behaviour); a press that grabs a
        // title bar / window corner / desktop icon and then moves becomes a drag.
        {
            int ptr = DInPointerDown();
            if (ptr && !s_ptrWas)                          // DOWN: arm a possible drag
            { s_downX = s_cx; s_downY = s_cy; s_dragMoved = 0; DragHitTest(s_cx, s_cy); s_dirty = 1; }
            else if (ptr && s_ptrWas)                      // HELD: drag once past the threshold
            {
                if (!s_dragMoved && (abs(s_cx - s_downX) > DRAG_THRESH || abs(s_cy - s_downY) > DRAG_THRESH))
                    s_dragMoved = 1;
                if (s_dragMoved && s_dragKind != DRAG_NONE) DragApply(s_cx, s_cy);
            }
            else if (!ptr && s_ptrWas)                     // UP: drop, or click if it never moved
            {
                if (!s_dragMoved) { if (!HandleClick(s_cx, s_cy)) OnKey(VK_RETURN); }
                s_dragKind = DRAG_NONE; s_dragTarget = -1; s_dragMoved = 0; s_dirty = 1;
            }
            s_ptrWas = ptr;
        }
        // GWES WM-tap click path (DInPostClick) - independent of the held-button drag model.
        if (DInTookClick())
            HandleClick(s_cx, s_cy);

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
                if (!s_shared->win[i].inUse) { s_winMax[i] = 0; s_winMin[i] = 0; }   // freed slot -> normal
            }
        // The scene is a PVR2 quad list that GfxPresent consumes + clears, so we MUST
        // Render() (rebuild quads) before every present - a bare present would submit
        // an empty scene. Rebuild is cheap (no rasterization), so recompose+present on
        // any change, cursor move, or keepalive.
        if (s_dirty || moved || GetTickCount() >= nextPresent)
        {
            Render();
            s_dirty = 0;
            if (GfxPresent(s_cx, s_cy, DInHasPointer()))
                s_dirty = 1;            // surface lost -> rebuild next loop
            nextPresent = GetTickCount() + 100;
        }
        // Pace to the PVR vblank (~60 Hz) instead of Sleep, which rounds up to the
        // ~25 ms CE tick + a 25 ms guard (= the 20 fps cap). WaitForVerticalBlank
        // yields during the blank at the real refresh rate; if it no-ops (returns
        // non-OK, or the loop ran <8 ms so it clearly didn't pace), fall back to Sleep.
        {
            HRESULT vb = GfxWaitVBlank();
            DWORD   el = GetTickCount() - lt0;
            if (vb != 0 /*DD_OK*/ || el < 8)
                Sleep(el < 16 ? 16 - el : 1);
        }
    }
}
