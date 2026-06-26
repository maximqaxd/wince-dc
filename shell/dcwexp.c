//
// dcwexp.c - windowed file Explorer, running as its own process on the DCWin
// client library. Browses \, \Windows, \CD-ROM; Enter opens a folder or launches
// an app (which itself appears as a window), Backspace = parent. Start path may be
// passed on the command line. Everything is a window now - the shell is just the
// desktop + compositor.
//
#include "dcwlib.h"

#define EW     320
#define EH     216
#define EROW   20
#define EVIS   9
#define MAXENT 256

typedef struct { WCHAR name[128]; DWORD attr; DWORD size; } ENT;

static ENT   g_ent[MAXENT];
static int   g_count, g_sel, g_top;
static WCHAR g_dir[MAX_PATH] = L"\\";

static void CopyN(WCHAR *d, const WCHAR *s, int n) { int i; for (i = 0; i < n-1 && s[i]; i++) d[i] = s[i]; d[i] = 0; }
static int  IsRoot(void)        { return g_dir[0] == L'\\' && g_dir[1] == 0; }
static int  IsDir(ENT *e)       { return (e->attr & FILE_ATTRIBUTE_DIRECTORY) != 0; }
static int  EndsWithExe(const WCHAR *s)
{
    int n = lstrlenW(s);
    if (n < 4 || s[n-4] != L'.') return 0;
    return (s[n-3]|32) == 'e' && (s[n-2]|32) == 'x' && (s[n-1]|32) == 'e';
}
static void JoinPath(WCHAR *o, const WCHAR *d, const WCHAR *nm)
{
    if (d[1] == 0) wsprintfW(o, L"\\%s", nm); else wsprintfW(o, L"%s\\%s", d, nm);
}
static int FileIcon(ENT *e) { return IsDir(e) ? ICON_FOLDER : (EndsWithExe(e->name) ? ICON_APP : ICON_FILE); }

static void ScanDir(void)
{
    WCHAR pat[MAX_PATH]; WIN32_FIND_DATAW fd; HANDLE h;
    g_count = 0; g_sel = 0; g_top = 0;
    if (!IsRoot()) { lstrcpyW(g_ent[g_count].name, L".."); g_ent[g_count].attr = FILE_ATTRIBUTE_DIRECTORY; g_ent[g_count].size = 0; g_count++; }
    if (IsRoot()) lstrcpyW(pat, L"\\*.*"); else wsprintfW(pat, L"%s\\*.*", g_dir);
    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do {
            if (g_count >= MAXENT) break;
            if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0))) continue;
            CopyN(g_ent[g_count].name, fd.cFileName, 128);
            g_ent[g_count].attr = fd.dwFileAttributes; g_ent[g_count].size = fd.nFileSizeLow; g_count++;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
}

static void GoParent(void)
{
    int i, last = -1;
    if (IsRoot()) return;
    for (i = 0; g_dir[i]; i++) if (g_dir[i] == L'\\') last = i;
    if (last <= 0) lstrcpyW(g_dir, L"\\"); else g_dir[last] = 0;
    ScanDir();
}

static DCWin *g_w;

static void EnterSel(void)
{
    ENT *e; WCHAR full[MAX_PATH];
    if (g_sel < 0 || g_sel >= g_count) return;
    e = &g_ent[g_sel];
    if (lstrcmpW(e->name, L"..") == 0) { GoParent(); return; }
    if (IsDir(e)) { JoinPath(full, g_dir, e->name); lstrcpyW(g_dir, full); ScanDir(); return; }
    if (EndsWithExe(e->name))
    {
        JoinPath(full, g_dir, e->name);
        DCWinExec(g_w, full);    // shell launches it (windowed dcw* or fullscreen hand-off)
    }
}

static void HandleKey(DWORD k)
{
    if (k == VK_UP   && g_sel > 0)           g_sel--;
    if (k == VK_DOWN && g_sel < g_count - 1) g_sel++;
    if (k == VK_RETURN)                      EnterSel();
    if (k == VK_BACK)                        GoParent();
}

static void Draw(DCWin *w)
{
    int i, y;
    DCWinBeginFrame(w);
    DCWinFill(w, 0, 0, EW, EH, RGB(255, 255, 255));         // list background
    DCWinFill(w, 0, 0, EW, 16, RGB(192, 192, 192));         // path bar
    DCWinText(w, 4, 1, RGB(0, 0, 0), RGB(192, 192, 192), g_dir);

    if (g_sel < g_top)            g_top = g_sel;
    if (g_sel >= g_top + EVIS)    g_top = g_sel - EVIS + 1;

    for (i = g_top; i < g_count && i < g_top + EVIS; i++)
    {
        ENT     *e   = &g_ent[i];
        int      sel = (i == g_sel);
        COLORREF bg  = sel ? RGB(0, 0, 128) : RGB(255, 255, 255);
        COLORREF fg  = sel ? RGB(255, 255, 255) : RGB(0, 0, 0);
        y = 18 + (i - g_top) * EROW;
        if (sel) DCWinFill(w, 2, y, EW - 4, EROW, RGB(0, 0, 128));   /* (x,y,WIDTH,HEIGHT) */
        DCWinIcon(w, 4, y + 2, FileIcon(e));
        DCWinText(w, 24, y + 3, fg, bg, e->name);
        if (!IsDir(e)) { WCHAR sz[20]; wsprintfW(sz, L"%u", e->size); DCWinText(w, EW - 70, y + 3, fg, bg, sz); }
    }
    DCWinEndFrame(w);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key;

    if (lpCmd && lpCmd[0])
        CopyN(g_dir, lpCmd, MAX_PATH);          // start path from the command line

    w = DCWinOpen(120, 70, EW, EH, L"Explorer", ICON_FOLDER);
    if (!w) { OutputDebugStringW(L"DCWEXP: DCWinOpen failed\r\n"); return 1; }
    g_w = w;
    ScanDir();
    Draw(w);

    for (;;)
    {
        int changed = 0;
        while (DCWinPollKey(w, &key)) { HandleKey(key); changed = 1; }
        if (changed) Draw(w);
        if (DCWinShouldClose(w)) break;
        Sleep(20);
    }

    DCWinClose(w);
    return 0;
}
