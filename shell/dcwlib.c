//
// dcwlib.c - DCWin client library (see dcwlib.h). Maps the shared section, claims
// a window slot, and marshals draw commands / input. One window per client.
//
#include "dcwlib.h"

struct DCWin
{
    HANDLE    hMap;
    DcShared *sh;
    DcWindow *w;
    DcCmd     build[DCWIN_MAXCMD];
    int       buildN;
    int       lastW, lastH;   // last size seen by DCWinClientSize (change detection)
};

static struct DCWin g_win;   // single window per client process

DCWin *DCWinOpen(int x, int y, int w, int h, const WCHAR *title, int iconId)
{
    int i, idx = -1, tries;

    g_win.hMap = CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DcShared), DCWIN_SECTION);
    if (!g_win.hMap)
        return NULL;
    g_win.sh = (DcShared *)MapViewOfFile(g_win.hMap, FILE_MAP_WRITE, 0, 0, sizeof(DcShared));
    if (!g_win.sh)
        return NULL;

    for (tries = 0; tries < 50 && g_win.sh->magic != DCWIN_MAGIC; tries++)
        Sleep(20);                       // wait for the shell to initialise the section
    if (g_win.sh->magic != DCWIN_MAGIC)
        return NULL;

    for (i = 0; i < DCWIN_MAXWIN; i++)
        if (!g_win.sh->win[i].inUse) { idx = i; break; }
    if (idx < 0)
        return NULL;

    g_win.w = &g_win.sh->win[idx];
    memset(g_win.w, 0, sizeof(DcWindow));
    g_win.w->ownerPid = GetCurrentProcessId();
    g_win.w->x = x; g_win.w->y = y; g_win.w->w = w; g_win.w->h = h;
    lstrcpyW(g_win.w->title, title);
    g_win.w->icon     = (DWORD)iconId;
    g_win.w->cmdCount = 0;
    g_win.w->gen      = 0;            // even = stable (seqlock)
    g_win.buildN      = 0;
    g_win.lastW       = w; g_win.lastH = h;
    g_win.w->inUse    = 1;               // publish only once fully initialised
    return &g_win;
}

// Current client size (the shell may have resized us). Returns 1 if it changed since last call.
int DCWinClientSize(DCWin *win, int *cw, int *ch)
{
    int w = (int)win->w->w, h = (int)win->w->h, changed;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    changed = (w != win->lastW || h != win->lastH);
    win->lastW = w; win->lastH = h;
    if (cw) *cw = w;
    if (ch) *ch = h;
    return changed;
}

// 1 if the shell resized us since last call (consumes the change). OR into your dirty flag.
int DCWinResized(DCWin *win) { return DCWinClientSize(win, 0, 0); }

// Fill the whole current client area with one colour - the standard first draw of a frame, so
// the window always fills after a resize/maximize without the app tracking dimensions.
void DCWinFillBg(DCWin *win, COLORREF color)
{
    int w = (int)win->w->w, h = (int)win->w->h;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    DCWinFill(win, 0, 0, w, h, color);
}

void DCWinBeginFrame(DCWin *win)
{
    win->buildN = 0;
}

void DCWinFill(DCWin *win, int x, int y, int w, int h, COLORREF color)
{
    DcCmd *c;
    if (win->buildN >= DCWIN_MAXCMD)
        return;
    c = &win->build[win->buildN++];
    c->op = DCOP_FILL; c->x = x; c->y = y; c->w = w; c->h = h; c->color = color;
}

void DCWinText(DCWin *win, int x, int y, COLORREF fg, COLORREF bg, const WCHAR *text)
{
    DcCmd *c;
    int    k;
    if (win->buildN >= DCWIN_MAXCMD)
        return;
    c = &win->build[win->buildN++];
    c->op = DCOP_TEXT; c->x = x; c->y = y; c->color = fg; c->color2 = bg;
    for (k = 0; k < 39 && text[k]; k++)
        c->text[k] = text[k];
    c->text[k] = 0;
}

void DCWinIcon(DCWin *win, int x, int y, int iconId)
{
    DcCmd *c;
    if (win->buildN >= DCWIN_MAXCMD)
        return;
    c = &win->build[win->buildN++];
    c->op = DCOP_ICON; c->x = x; c->y = y; c->color = (DWORD)iconId;
}

void DCWinEndFrame(DCWin *win)
{
    int i;
    win->w->gen++;                       // odd = writing (seqlock)
    for (i = 0; i < win->buildN; i++)
        win->w->cmd[i] = win->build[i];
    win->w->cmdCount = win->buildN;
    win->w->gen++;                       // even = stable
}

int DCWinPollKey(DCWin *win, DWORD *key)
{
    if (win->w->inTail == win->w->inHead)
        return 0;
    *key = win->w->in[win->w->inTail % DCWIN_MAXIN].key;
    win->w->inTail++;
    return 1;
}

int DCWinShouldClose(DCWin *win)
{
    return win->w->wantClose != 0;
}

void DCWinExec(DCWin *win, const WCHAR *path)
{
    lstrcpyW(win->sh->execPath, path);
    win->sh->execSeq++;          // shell polls execSeq and launches execPath
}

void DCWinClose(DCWin *win)
{
    win->w->inUse = 0;
    if (win->sh)   UnmapViewOfFile(win->sh);
    if (win->hMap) CloseHandle(win->hMap);
}
