//
// dcshell.c - Dreamcast CE hybrid desktop shell (NT 4.0 style).
//
// Boots to a DESKTOP (teal, shortcut icons, taskbar). Opening "My Dreamcast" (or
// a Start-menu item) switches to the EXPLORER view (NT4 chrome: title / menu /
// address / column list / status). Esc closes the Explorer back to the desktop;
// Tab toggles the Start menu in either view.
//
// All drawing goes to a persistent back buffer (dcgfx) then GfxPresent blits it to
// the volatile DC primary every frame. Fills/bevels are DirectDraw COLORFILL; text
// is GDI Arial. Rendering is dirty-flag driven (one Render per loop) so WM_PAINT
// never storms the input queue.
//
#include "dcgfx.h"
#include "dcwin.h"

#define MAX_ENT  256

#define VIEW_DESKTOP   0
#define VIEW_EXPLORER  1

// Explorer chrome metrics
#define TITLE_H   18
#define MENU_H    18
#define ADDR_H    22
#define COLH_H    18
#define STATUS_H  18
#define TASK_H    26
#define ROW_H     18

#define ADDR_Y    (TITLE_H + MENU_H)
#define COLH_Y    (ADDR_Y + ADDR_H)
#define LIST_TOP  (COLH_Y + COLH_H)
#define LIST_BOT  (SCREEN_H - TASK_H - STATUS_H)
#define STATUS_Y  LIST_BOT
#define TASK_Y    (SCREEN_H - TASK_H)
#define VIS       ((LIST_BOT - LIST_TOP) / ROW_H)

#define COL_NAME_X  28
#define COL_SIZE_X  330
#define COL_TYPE_X  450

#define CL_DESKTOP  RGB(0, 128, 128)
#define CL_FACE     RGB(192, 192, 192)
#define CL_TITLE    RGB(0, 0, 128)
#define CL_WINDOW   RGB(255, 255, 255)
#define CL_SEL      RGB(0, 0, 128)
#define CL_TEXT     RGB(0, 0, 0)
#define CL_DIR      RGB(0, 0, 160)
#define CL_WHITE    RGB(255, 255, 255)

typedef struct
{
    WCHAR name[128];
    DWORD attr;
    DWORD size;
} ENTRY;

typedef struct
{
    const WCHAR *label;
    const WCHAR *path;     // explorer path, or NULL
    const WCHAR *exe;      // app to launch, or NULL
} SHORTCUT;

static const SHORTCUT s_desk[] =
{
    { L"My Dreamcast", L"\\",        NULL },
    { L"CD-ROM",       L"\\CD-ROM",  NULL },
    { L"Calculator",   NULL,         L"dcwcalc.exe" },
    { L"Clock",        NULL,         L"dcwclock.exe" },
};
#define DESK_N (sizeof(s_desk) / sizeof(s_desk[0]))

static const SHORTCUT s_start[] =
{
    { L"My Dreamcast", L"\\",        NULL },
    { L"Windows",      L"\\Windows", NULL },
    { L"CD-ROM",       L"\\CD-ROM",  NULL },
    { L"Shut Down...", NULL,         NULL },
};
#define START_N (sizeof(s_start) / sizeof(s_start[0]))
#define MENU_W  168

static ENTRY s_ent[MAX_ENT];
static int   s_count = 0;
static int   s_sel   = 0;
static int   s_top   = 0;
static WCHAR s_dir[MAX_PATH] = L"\\";

static int   s_view     = VIEW_DESKTOP;
static int   s_deskSel  = 0;
static int   s_menuOpen = 0;
static int   s_menuSel  = 0;
static int   s_dirty    = 1;
static int   s_focus    = -1;   // focused window index, or -1 = desktop
static int   s_wasInUse[DCWIN_MAXWIN];

static HWND  s_hwnd = NULL;

static void DbgStr(const WCHAR *s) { OutputDebugStringW(s); }

//
// DCWin compositor: a shared section holds a window registry; client apps (their
// own processes) publish draw-command lists, the shell composites them as windows
// and routes input. Poll-based (CE maps the named section at the same VA in every
// process, proven 2026-06-26).
//
static DcShared *s_shared    = NULL;
static HANDLE    s_sharedMap = NULL;

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

static void LaunchApp(const WCHAR *exe)
{
    PROCESS_INFORMATION pi;
    if (!s_shared || !exe)
        return;
    if (CreateProcessW(exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);    // detached; we poll the shared section each frame
        DbgStr(L"DCSHELL: launched app\r\n");
    }
    else DbgStr(L"DCSHELL: CreateProcess(app) FAILED\r\n");
}

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
// Helpers
//
static void CopyN(WCHAR *dst, const WCHAR *src, int n)
{
    int i;
    for (i = 0; i < n - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

static BOOL IsRoot(void)            { return (s_dir[0] == L'\\' && s_dir[1] == 0); }
static BOOL IsDir(const ENTRY *e)   { return (e->attr & FILE_ATTRIBUTE_DIRECTORY) != 0; }

static BOOL EndsWithExe(const WCHAR *s)
{
    int n = lstrlenW(s);
    if (n < 4 || s[n - 4] != L'.')
        return FALSE;
    return (s[n - 3] | 32) == 'e' && (s[n - 2] | 32) == 'x' && (s[n - 1] | 32) == 'e';
}

static void JoinPath(WCHAR *out, const WCHAR *dir, const WCHAR *name)
{
    if (dir[1] == 0)
        wsprintfW(out, L"\\%s", name);
    else
        wsprintfW(out, L"%s\\%s", dir, name);
}

//
// File model
//
static void ScanDir(void)
{
    WCHAR            pat[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE           h;

    s_count = 0;
    s_sel   = 0;
    s_top   = 0;

    if (!IsRoot())
    {
        lstrcpyW(s_ent[s_count].name, L"..");
        s_ent[s_count].attr = FILE_ATTRIBUTE_DIRECTORY;
        s_ent[s_count].size = 0;
        s_count++;
    }

    if (IsRoot())
        lstrcpyW(pat, L"\\*.*");
    else
        wsprintfW(pat, L"%s\\*.*", s_dir);

    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (s_count >= MAX_ENT)
                break;
            if (fd.cFileName[0] == L'.' &&
                (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
                continue;
            CopyN(s_ent[s_count].name, fd.cFileName, 128);
            s_ent[s_count].attr = fd.dwFileAttributes;
            s_ent[s_count].size = fd.nFileSizeLow;
            s_count++;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
}

static void GoParent(void)
{
    int i, last = -1;

    if (IsRoot())
        return;
    for (i = 0; s_dir[i]; i++)
        if (s_dir[i] == L'\\')
            last = i;
    if (last <= 0)
        lstrcpyW(s_dir, L"\\");
    else
        s_dir[last] = 0;
    ScanDir();
}

static void OpenExplorer(const WCHAR *path)
{
    if (!path)
        return;
    lstrcpyW(s_dir, path);
    ScanDir();
    s_view = VIEW_EXPLORER;
}

//
// Rendering - PASS 1 = COLORFILL/bevel, PASS 2 = text (no fills while DC locked).
//
static COLORREF IconColor(const ENTRY *e)
{
    if (IsDir(e))               return RGB(240, 208, 96);   // folder
    if (EndsWithExe(e->name))   return RGB(72, 112, 216);   // app
    return RGB(208, 208, 208);                              // file
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
            bx += 116;
        }
    }

    if (s_menuOpen)
    {
        int h = (int)START_N * ROW_H + 8;
        SetRect(&rc, 4, TASK_Y - h, 4 + MENU_W, TASK_Y);
        GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);
        if (s_menuSel >= 0 && s_menuSel < (int)START_N)
            GfxFill(8, TASK_Y - h + 4 + s_menuSel * ROW_H, MENU_W, TASK_Y - h + 4 + (s_menuSel + 1) * ROW_H, CL_SEL);
    }
}

static void RenderDesktopFills(void)
{
    int i, y;

    GfxFill(0, 0, SCREEN_W, TASK_Y, CL_DESKTOP);
    for (i = 0; i < (int)DESK_N; i++)
    {
        y = 24 + i * 76;
        if (i == s_deskSel && !s_menuOpen)
            GfxFill(14, y + 36, 110, y + 52, CL_TITLE);     // label highlight
        GfxFill(40, y, 72, y + 32, RGB(208, 208, 208));     // icon body
        { RECT ic; SetRect(&ic, 40, y, 72, y + 32); GfxBevel(&ic, TRUE); }
    }
}

static void RenderExplorerFills(void)
{
    RECT rc;
    int  i, y;

    GfxFill(0, 0, SCREEN_W, TITLE_H, CL_TITLE);
    GfxFill(0, TITLE_H, SCREEN_W, ADDR_Y, CL_FACE);
    GfxFill(0, ADDR_Y, SCREEN_W, COLH_Y, CL_FACE);

    SetRect(&rc, 76, ADDR_Y + 3, SCREEN_W - 8, COLH_Y - 3);
    GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_WINDOW); GfxBevel(&rc, FALSE);

    GfxFill(0, COLH_Y, SCREEN_W, LIST_TOP, CL_FACE);
    SetRect(&rc, 2, COLH_Y + 1, COL_SIZE_X - 4, LIST_TOP - 1); GfxBevel(&rc, TRUE);
    SetRect(&rc, COL_SIZE_X - 4, COLH_Y + 1, COL_TYPE_X - 4, LIST_TOP - 1); GfxBevel(&rc, TRUE);
    SetRect(&rc, COL_TYPE_X - 4, COLH_Y + 1, SCREEN_W - 2, LIST_TOP - 1); GfxBevel(&rc, TRUE);

    GfxFill(0, LIST_TOP, SCREEN_W, LIST_BOT, CL_WINDOW);

    for (i = s_top; i < s_count && i < s_top + VIS; i++)
    {
        y = LIST_TOP + (i - s_top) * ROW_H;
        if (i == s_sel && !s_menuOpen)
            GfxFill(2, y, SCREEN_W - 2, y + ROW_H, CL_SEL);
        GfxFill(6, y + 2, 20, y + 15, IconColor(&s_ent[i]));
    }

    GfxFill(0, STATUS_Y, SCREEN_W, TASK_Y, CL_FACE);
    SetRect(&rc, 2, STATUS_Y + 1, SCREEN_W - 2, TASK_Y - 1); GfxBevel(&rc, FALSE);

    // caption buttons
    SetRect(&rc, SCREEN_W - 54, 3, SCREEN_W - 38, TITLE_H - 3); GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);
    SetRect(&rc, SCREEN_W - 36, 3, SCREEN_W - 20, TITLE_H - 3); GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);
    SetRect(&rc, SCREEN_W - 18, 3, SCREEN_W - 2,  TITLE_H - 3); GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE); GfxBevel(&rc, TRUE);
}

static void RenderText(HDC hdc)
{
    SYSTEMTIME t;
    WCHAR      clk[16];
    int        i, y;

    if (s_view == VIEW_DESKTOP)
    {
        for (i = 0; i < (int)DESK_N; i++)
        {
            COLORREF fg = (i == s_deskSel && !s_menuOpen) ? CL_WHITE : CL_WHITE;
            COLORREF bg = (i == s_deskSel && !s_menuOpen) ? CL_TITLE : CL_DESKTOP;
            y = 24 + i * 76;
            GfxText(hdc, 18, y + 37, fg, bg, g_FontUI, s_desk[i].label);
        }
    }
    else
    {
        GfxText(hdc, 6, 1, CL_WHITE, CL_TITLE, g_FontTitle, L"Dreamcast CE \x2014 Explorer");
        GfxText(hdc, SCREEN_W - 51, 3, CL_TEXT, CL_FACE, g_FontUI, L"_");
        GfxText(hdc, SCREEN_W - 33, 3, CL_TEXT, CL_FACE, g_FontUI, L"\x25A1");
        GfxText(hdc, SCREEN_W - 15, 3, CL_TEXT, CL_FACE, g_FontUI, L"X");
        GfxText(hdc, 8, TITLE_H + 3, CL_TEXT, CL_FACE, g_FontUI, L"File    Edit    View    Help");
        GfxText(hdc, 8, ADDR_Y + 4, CL_TEXT, CL_FACE, g_FontUI, L"Address");
        GfxText(hdc, 80, ADDR_Y + 5, CL_TEXT, CL_WINDOW, g_FontUI, s_dir);
        GfxText(hdc, COL_NAME_X, COLH_Y + 2, CL_TEXT, CL_FACE, g_FontBold, L"Name");
        GfxText(hdc, COL_SIZE_X, COLH_Y + 2, CL_TEXT, CL_FACE, g_FontBold, L"Size");
        GfxText(hdc, COL_TYPE_X, COLH_Y + 2, CL_TEXT, CL_FACE, g_FontBold, L"Type");

        for (i = s_top; i < s_count && i < s_top + VIS; i++)
        {
            ENTRY   *e   = &s_ent[i];
            BOOL     dir = IsDir(e);
            BOOL     hot = (i == s_sel && !s_menuOpen);
            COLORREF fg  = hot ? CL_WHITE : (dir ? CL_DIR : CL_TEXT);
            COLORREF bg  = hot ? CL_SEL : CL_WINDOW;
            const WCHAR *type = dir ? L"Folder" : (EndsWithExe(e->name) ? L"Application" : L"File");

            y = LIST_TOP + (i - s_top) * ROW_H;
            GfxText(hdc, COL_NAME_X, y + 2, fg, bg, g_FontUI, e->name);
            if (!dir)
            {
                WCHAR sz[24];
                wsprintfW(sz, L"%u", e->size);
                GfxText(hdc, COL_SIZE_X, y + 2, fg, bg, g_FontUI, sz);
            }
            GfxText(hdc, COL_TYPE_X, y + 2, fg, bg, g_FontUI, type);
        }

        {
            WCHAR st[64];
            wsprintfW(st, L"%d object(s)", s_count);
            GfxText(hdc, 8, STATUS_Y + 2, CL_TEXT, CL_FACE, g_FontUI, st);
        }
    }

    // taskbar (both views)
    GfxText(hdc, 12, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontBold, L"Start");
    GetLocalTime(&t);
    wsprintfW(clk, L"%02d:%02d", t.wHour, t.wMinute);
    GfxText(hdc, SCREEN_W - 44, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, clk);

    if (s_shared)                       // window button labels
    {
        int bx = 80, k;
        for (k = 0; k < DCWIN_MAXWIN; k++)
        {
            if (!s_shared->win[k].inUse)
                continue;
            GfxText(hdc, bx + 6, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, s_shared->win[k].title);
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
            GfxText(hdc, 12, my + i * ROW_H + 1, fg, bg, g_FontUI, s_start[i].label);
        }
    }
}

static void DrawWinFills(DcWindow *w, BOOL active)
{
    RECT fr;
    int  i;

    SetRect(&fr, w->x - 2, w->y - 18, w->x + w->w + 2, w->y + w->h + 2);       // frame
    GfxFill(fr.left, fr.top, fr.right, fr.bottom, CL_FACE);
    GfxBevel(&fr, TRUE);
    GfxFill(w->x - 2, w->y - 18, w->x + w->w + 2, w->y - 2, active ? CL_TITLE : RGB(112,112,112));
    GfxFill(w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3, CL_FACE);  // close box
    { RECT cb; SetRect(&cb, w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3); GfxBevel(&cb, TRUE); }

    for (i = 0; i < (int)w->cmdCount && i < DCWIN_MAXCMD; i++)
    {
        DcCmd *c = &w->cmd[i];
        if (c->op == DCOP_FILL)
            GfxFill(w->x + c->x, w->y + c->y, w->x + c->x + c->w, w->y + c->y + c->h, c->color);
    }
}

static void DrawWinText(HDC hdc, DcWindow *w, BOOL active)
{
    int i;

    GfxText(hdc, w->x, w->y - 16, CL_WHITE, active ? CL_TITLE : RGB(112,112,112), g_FontBold, w->title);
    GfxText(hdc, w->x + w->w - 11, w->y - 16, CL_TEXT, CL_FACE, g_FontUI, L"X");
    for (i = 0; i < (int)w->cmdCount && i < DCWIN_MAXCMD; i++)
    {
        DcCmd *c = &w->cmd[i];
        if (c->op == DCOP_TEXT)
            GfxText(hdc, w->x + c->x, w->y + c->y, c->color, c->color2, g_FontUI, c->text);
    }
}

// non-focused windows first, focused window last (drawn on top)
static void RenderWindowsFills(void)
{
    int i;
    if (!s_shared)
        return;
    for (i = 0; i < DCWIN_MAXWIN; i++)
        if (s_shared->win[i].inUse && i != s_focus)
            DrawWinFills(&s_shared->win[i], FALSE);
    if (s_focus >= 0 && s_shared->win[s_focus].inUse)
        DrawWinFills(&s_shared->win[s_focus], TRUE);
}

static void RenderWindowsText(HDC hdc)
{
    int i;
    if (!s_shared)
        return;
    for (i = 0; i < DCWIN_MAXWIN; i++)
        if (s_shared->win[i].inUse && i != s_focus)
            DrawWinText(hdc, &s_shared->win[i], FALSE);
    if (s_focus >= 0 && s_shared->win[s_focus].inUse)
        DrawWinText(hdc, &s_shared->win[s_focus], TRUE);
}

static void Render(void)
{
    HDC hdc;

    if (s_sel < s_top)            s_top = s_sel;
    if (s_sel >= s_top + VIS)     s_top = s_sel - VIS + 1;

    // PASS 1: fills
    if (s_view == VIEW_DESKTOP)
        RenderDesktopFills();
    else
        RenderExplorerFills();
    RenderWindowsFills();           // composited app windows over the view
    RenderTaskbarFills();

    // PASS 2: text
    hdc = GfxLockDC();
    if (hdc)
    {
        RenderText(hdc);
        RenderWindowsText(hdc);
        GfxUnlockDC(hdc);
    }
}

//
// Input
//
static void ActivateStartItem(void)
{
    const SHORTCUT *s = &s_start[s_menuSel];
    s_menuOpen = 0;
    if (s->path)
        OpenExplorer(s->path);
    else if (s->exe)
        LaunchApp(s->exe);
}

static void ActivateExplorerItem(void)
{
    ENTRY *e;
    WCHAR  full[MAX_PATH];

    if (s_sel < 0 || s_sel >= s_count)
        return;
    e = &s_ent[s_sel];
    if (lstrcmpW(e->name, L"..") == 0)
    {
        GoParent();
        return;
    }
    if (IsDir(e))
    {
        JoinPath(full, s_dir, e->name);
        lstrcpyW(s_dir, full);
        ScanDir();
        return;
    }
    if (EndsWithExe(e->name))
    {
        JoinPath(full, s_dir, e->name);
        DbgStr(L"DCSHELL: launching\r\n");
        GfxLaunch(full);          // hands the display off and back
        ScanDir();
    }
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
        if (wp == VK_UP   && s_menuSel > 0)             s_menuSel--;
        if (wp == VK_DOWN && s_menuSel < (int)START_N-1) s_menuSel++;
        if (wp == VK_RETURN)                            ActivateStartItem();
        if (wp == VK_ESCAPE)                            s_menuOpen = 0;
        return;
    }

    if (s_view == VIEW_DESKTOP)
    {
        if (wp == VK_UP   && s_deskSel > 0)             s_deskSel--;
        if (wp == VK_DOWN && s_deskSel < (int)DESK_N-1) s_deskSel++;
        if (wp == VK_RETURN)
        {
            if (s_desk[s_deskSel].path)
                OpenExplorer(s_desk[s_deskSel].path);
            else
                LaunchApp(s_desk[s_deskSel].exe);
        }
        return;
    }

    // VIEW_EXPLORER
    if (wp == VK_UP   && s_sel > 0)             s_sel--;
    if (wp == VK_DOWN && s_sel < s_count - 1)   s_sel++;
    if (wp == VK_RETURN)                        ActivateExplorerItem();
    if (wp == VK_BACK)                          GoParent();
    if (wp == VK_ESCAPE)                        s_view = VIEW_DESKTOP;   // close Explorer
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;                       // we own the surface

    case WM_PAINT:
        ValidateRect(hwnd, NULL);       // continuous GfxPresent shows the frame; don't storm
        return 0;

    case WM_KEYDOWN:
        OnKey(wp);
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

    DbgStr(L"DCSHELL: WinMain enter (hybrid desktop)\r\n");

    InitShared();                   // DCWin compositor shared section

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

    ScanDir();
    DbgStr(L"DCSHELL: desktop up\r\n");

    next = GetTickCount() + 1000;
    for (;;)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                GfxShutdown();
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        FixupFocus();                   // auto-focus new windows, drop closed ones
        if (GetTickCount() >= next)     // clock tick -> repaint
        {
            s_dirty = 1;
            next += 1000;
        }
        if (CountWindows() > 0)         // an app window is live -> keep compositing
            s_dirty = 1;
        if (s_dirty)
        {
            Render();
            s_dirty = 0;
        }
        GfxPresent();
        Sleep(33);
    }
}
