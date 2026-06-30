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
#include "vmustore.h"

#define TASK_H     26
#define ROW_H      18 // Start-menu row height
#define TASK_Y     (SCREEN_H - TASK_H)
#define MENU_W     168

#define CL_DESKTOP RGB(0, 128, 128)
#define CL_FACE    RGB(192, 192, 192)
#define CL_TITLE   RGB(0, 0, 128)
#define CL_SEL     RGB(0, 0, 128)
#define CL_TEXT    RGB(0, 0, 0)
#define CL_WHITE   RGB(255, 255, 255)

typedef struct
{
	const WCHAR *label;
	const WCHAR *path; // open Explorer here, or NULL
	const WCHAR *exe;  // launch this app, or NULL
	int icon;          // ICON_*
} SHORTCUT;

// Built-in desktop shortcuts. The live desktop is a mutable copy (s_aDesk) so the user can
// drag items off the Start menu to add their own; user-added entries are persisted to the
// VMU. String pointers always point at const tables (defaults here / s_start below), so the
// live array stores no allocations.
static const SHORTCUT s_deskDefault[] = {
    {L"My Dreamcast", L"\\", NULL, ICON_SWIRL},
    {L"CD-ROM", L"\\CD-ROM", NULL, ICON_DRIVE},
    {L"Calculator", NULL, L"dcwcalc.exe", ICON_APP},
    {L"Clock", NULL, L"dcwclock.exe", ICON_CLOCK},
    {L"Task Manager", NULL, L"dcwtask.exe", ICON_APP},
    {L"Network", NULL, L"dcwnet.exe", ICON_APP},
    {L"System Log", NULL, L"dcwlog.exe", ICON_APP},
};
#define DEFAULT_N (sizeof(s_deskDefault) / sizeof(s_deskDefault[0]))
#define MAXDESK   24
static SHORTCUT s_aDesk[MAXDESK]; // live desktop shortcuts
static int s_anDeskUser[MAXDESK]; // for user-added: the s_start[] index it came from; -1 = built-in
static int s_cDesk = 0;

static const SHORTCUT s_start[] = {
    {L"My Dreamcast", L"\\", NULL, ICON_SWIRL}, {L"Windows", L"\\Windows", NULL, ICON_FOLDER},
    {L"CD-ROM", L"\\CD-ROM", NULL, ICON_DRIVE}, {L"Task Manager", NULL, L"dcwtask.exe", ICON_APP},
    {L"Shut Down...", NULL, NULL, ICON_FILE},
};
#define START_N (sizeof(s_start) / sizeof(s_start[0]))

static int s_nDeskSel = 0;
static int s_bMenuOpen = 0;
static int s_nMenuSel = 0;
static int s_bDirty = 1;
static int s_bDeskDirty = 1;   // static desktop-layer cache needs a (re)build
static int s_nCachedSel = -1;  // s_nDeskSel baked into the desktop cache
static int s_nCachedMenu = -1; // s_bMenuOpen baked into the desktop cache
static int s_nFocus = -1;      // focused window index, or -1 = desktop
static int s_abWasInUse[DCWIN_MAXWIN];
static DWORD s_adwLastGen[DCWIN_MAXWIN]; // last composited gen per window
static DWORD s_dwLastExec = 0;           // last processed exec request
static HWND s_hwnd = NULL;
static BOOL s_bDiKbd = FALSE;  // DI keyboard acquired (else fall back to WM_KEYDOWN)
static int s_bWmMouseSeen = 0; // logged once when GWES mouse messages arrive
static int s_nCx = SCREEN_W / 2, s_nCy = SCREEN_H / 2; // pointer position

// Desktop icon cell positions (top-left of the 96x54 clickable cell). Mutable so icons can
// be dragged. Built-ins are laid out in a column-major grid by DeskPosInit() (flow top-to-
// bottom, then wrap into the next column so they never run under the taskbar); user-added
// icons keep their dropped / persisted position.
static POINT s_aDeskPos[MAXDESK];
static int s_bDeskPosInit = 0;
static int s_nDeskRows = 6; // built-in icons per column (computed from the usable height)
#define ICELL_W 96
#define ICELL_H 54
#define DESK_X0 14
#define DESK_Y0 16
#define DESK_DY 70             // row pitch
#define DESK_DX (ICELL_W + 12) // column pitch
static void DeskPosInit(void)
{
	int i, nSlot = 0;
	s_nDeskRows = (TASK_Y - DESK_Y0) / DESK_DY;
	if (s_nDeskRows < 1)
		s_nDeskRows = 1;
	for (i = 0; i < s_cDesk; i++)
	{
		if (s_anDeskUser[i] >= 0)
			continue; // user icons keep their saved / dropped pos
		s_aDeskPos[i].x = DESK_X0 + (nSlot / s_nDeskRows) * DESK_DX;
		s_aDeskPos[i].y = DESK_Y0 + (nSlot % s_nDeskRows) * DESK_DY;
		nSlot++;
	}
	s_bDeskPosInit = 1;
}

// --- dynamic shortcuts + VMU persistence --------------------------------------------
// Persistence blob (one VMU block): 'DCW1' magic, a count, then count records of the
// originating s_start[] index + the icon's desktop position.
#define SHC_MAGIC 0x31574344u // 'D','C','W','1'
typedef struct
{
	short idx, x, y;
} ShcEntry;

static void SaveShortcuts(void)
{
	BYTE abBuf[512];
	DWORD *pdwHdr = (DWORD *)abBuf;
	ShcEntry *pEntry = (ShcEntry *)(abBuf + 8);
	int i, cEntry = 0, cCap = (int)((sizeof(abBuf) - 8) / sizeof(ShcEntry));
	memset(abBuf, 0, sizeof(abBuf));
	for (i = 0; i < s_cDesk && cEntry < cCap; i++)
	{
		if (s_anDeskUser[i] < 0)
			continue;
		pEntry[cEntry].idx = (short)s_anDeskUser[i];
		pEntry[cEntry].x = (short)s_aDeskPos[i].x;
		pEntry[cEntry].y = (short)s_aDeskPos[i].y;
		cEntry++;
	}
	pdwHdr[0] = SHC_MAGIC;
	pdwHdr[1] = (DWORD)cEntry;
	VmuSave(abBuf, 8 + cEntry * (int)sizeof(ShcEntry));
}

static void LoadShortcuts(void)
{
	BYTE abBuf[512];
	DWORD *pdwHdr = (DWORD *)abBuf;
	ShcEntry *pEntry = (ShcEntry *)(abBuf + 8);
	int nGot = 0, i, cEntry;
	if (!VmuLoad(abBuf, sizeof(abBuf), &nGot) || nGot < 8 || pdwHdr[0] != SHC_MAGIC)
		return;
	cEntry = (int)pdwHdr[1];
	for (i = 0; i < cEntry && s_cDesk < MAXDESK; i++)
	{
		int nIdx = pEntry[i].idx;
		if (nIdx < 0 || nIdx >= (int)START_N)
			continue;
		if (!s_start[nIdx].exe && !s_start[nIdx].path)
			continue;
		s_aDesk[s_cDesk] = s_start[nIdx];
		s_anDeskUser[s_cDesk] = nIdx;
		s_aDeskPos[s_cDesk].x = pEntry[i].x;
		s_aDeskPos[s_cDesk].y = pEntry[i].y;
		s_cDesk++;
	}
}

// Seed the live desktop from the built-ins, append persisted user shortcuts, lay out.
static void DeskInit(void)
{
	int i;
	for (i = 0; i < (int)DEFAULT_N; i++)
	{
		s_aDesk[i] = s_deskDefault[i];
		s_anDeskUser[i] = -1;
	}
	s_cDesk = (int)DEFAULT_N;
	LoadShortcuts();
	DeskPosInit();
}

// Add a Start-menu item to the desktop at (x,y) and persist. Returns 1 if added.
static int AddDesktopShortcut(int nStartIdx, int x, int y)
{
	if (s_cDesk >= MAXDESK || nStartIdx < 0 || nStartIdx >= (int)START_N)
		return 0;
	if (!s_start[nStartIdx].exe && !s_start[nStartIdx].path)
		return 0; // e.g. "Shut Down..."
	x -= ICELL_W / 2;
	y -= 16;
	if (x < 0)
		x = 0;
	if (x > SCREEN_W - ICELL_W)
		x = SCREEN_W - ICELL_W;
	if (y < 0)
		y = 0;
	if (y > TASK_Y - ICELL_H)
		y = TASK_Y - ICELL_H;
	s_aDesk[s_cDesk] = s_start[nStartIdx];
	s_anDeskUser[s_cDesk] = nStartIdx;
	s_aDeskPos[s_cDesk].x = x;
	s_aDeskPos[s_cDesk].y = y;
	s_nDeskSel = s_cDesk;
	s_cDesk++;
	SaveShortcuts();
	s_bDeskDirty = 1;
	return 1;
}

// Remove a user-added shortcut (built-ins can't be removed) and persist.
static void RemoveShortcut(int i)
{
	int k;
	if (i < 0 || i >= s_cDesk || s_anDeskUser[i] < 0)
		return;
	for (k = i; k < s_cDesk - 1; k++)
	{
		s_aDesk[k] = s_aDesk[k + 1];
		s_anDeskUser[k] = s_anDeskUser[k + 1];
		s_aDeskPos[k] = s_aDeskPos[k + 1];
	}
	s_cDesk--;
	if (s_nDeskSel >= s_cDesk)
		s_nDeskSel = s_cDesk - 1;
	if (s_nDeskSel < 0)
		s_nDeskSel = 0;
	SaveShortcuts();
	s_bDeskDirty = 1;
}

// Pointer drag state machine (mouse-L or controller-A held). Lets the user move/resize
// windows and move desktop shortcuts; a press with no drag is treated as a click on release.
#define DRAG_NONE      0
#define DRAG_WMOVE     1 // moving a window (grabbed its title bar)
#define DRAG_WSIZE     2 // resizing a window (grabbed its bottom-right corner)
#define DRAG_ICON      3 // moving a desktop shortcut
#define DRAG_STARTITEM 4 // dragging a Start-menu item out onto the desktop (creates a shortcut)
#define DRAG_THRESH    3 // px of motion before a press becomes a drag (vs a click)
#define WIN_MINW       80
#define WIN_MINH       40
static int s_nDragKind = DRAG_NONE, s_nDragTarget = -1;
static int s_nDragOffX = 0, s_nDragOffY = 0; // cursor - target origin, at grab time
static int s_bDragMoved = 0, s_bPtrWas = 0, s_nDownX = 0, s_nDownY = 0;

// Window maximize/restore + minimize (shell-side, keyed by window slot). s_awinRestore =
// {x,y,w,h} saved before maximizing. Both reset when a slot frees so a reused slot is normal.
static int s_abWinMax[DCWIN_MAXWIN];
static int s_awinRestore[DCWIN_MAXWIN][4];
static int s_abWinMin[DCWIN_MAXWIN]; // window minimized (hidden; only its taskbar button shows)

static DcShared *s_pShared = NULL;
static HANDLE s_hSharedMap = NULL;

// seqlock snapshot of each window's command list (avoids reading a half-written frame)
static DcCmd s_aSnap[DCWIN_MAXWIN][DCWIN_MAXCMD];
static int s_anSnapN[DCWIN_MAXWIN];

static void DbgStr(const WCHAR *psz)
{
	OutputDebugStringW(psz);
}

//
// Compositor shared section + launching
//
static void InitShared(void)
{
	s_hSharedMap =
	    CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DcShared), DCWIN_SECTION);
	if (!s_hSharedMap)
	{
		DbgStr(L"DCSHELL: shared section create FAILED\r\n");
		return;
	}
	s_pShared = (DcShared *)MapViewOfFile(s_hSharedMap, FILE_MAP_WRITE, 0, 0, sizeof(DcShared));
	if (!s_pShared)
	{
		DbgStr(L"DCSHELL: shared section map FAILED\r\n");
		return;
	}
	memset(s_pShared, 0, sizeof(DcShared));
	s_pShared->magic = DCWIN_MAGIC;
	DbgStr(L"DCSHELL: compositor ready\r\n");
}

static void LaunchApp(const WCHAR *pszExe, const WCHAR *pszArgs)
{
	PROCESS_INFORMATION pi;
	WCHAR szCl[MAX_PATH];
	if (!s_pShared || !pszExe)
		return;
	if (pszArgs)
		lstrcpyW(szCl, pszArgs);
	if (CreateProcessW(pszExe, pszArgs ? szCl : NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess); // detached; we poll the shared section each frame
		DbgStr(L"DCSHELL: launched app\r\n");
	}
	else
		DbgStr(L"DCSHELL: CreateProcess(app) FAILED\r\n");
}

// dcw*.exe are windowed DCWin apps (composited); anything else gets the display.
static BOOL IsDcwApp(const WCHAR *pszPath)
{
	const WCHAR *pszBase = pszPath, *p;
	for (p = pszPath; *p; p++)
		if (*p == L'\\')
			pszBase = p + 1;
	return (pszBase[0] | 32) == 'd' && (pszBase[1] | 32) == 'c' && (pszBase[2] | 32) == 'w';
}

static void ShellLaunch(const WCHAR *pszCmd)
{
	WCHAR szExe[MAX_PATH];
	const WCHAR *pszArgs = NULL;
	int i;
	for (i = 0; pszCmd[i] && pszCmd[i] != L' ' && i < MAX_PATH - 1; i++)
		szExe[i] = pszCmd[i]; // split exe arg
	szExe[i] = 0;
	if (pszCmd[i] == L' ')
		pszArgs = pszCmd + i + 1; // e.g. "dcwplay.exe \CD-ROM\song.mp3"
	if (IsDcwApp(szExe))
		LaunchApp(szExe, pszArgs); // windowed, composited (+ optional file argument)
	else
	{
		DbgStr(L"DCSHELL: fullscreen launch (display + input hand-off)\r\n");
		DInRelease();     // hand input to the app (it may grab DI exclusively)
		GfxLaunch(szExe); // release display -> run (blocks) -> reclaim
		DInReacquire();   // app exited -> take input back
		s_bDirty = 1;
		s_bDeskDirty = 1; // surfaces/icons rebuilt -> recache desktop
	}
}

//
// Window focus management
//
static int CountWindows(void)
{
	int i, n = 0;
	if (!s_pShared)
		return 0;
	for (i = 0; i < DCWIN_MAXWIN; i++)
		if (s_pShared->win[i].inUse)
			n++;
	return n;
}

// Reap windows whose owner process died without DCWinClose (e.g. a Task Manager
// "end task", or a crash) - otherwise the slot stays inUse and ghosts forever.
// Liveness is an OpenProcess(ownerPid) probe; s_dwReapAccess is the access flag that
// worked on our own (live) pid at startup, or 0 if OpenProcess is unusable here.
static DWORD s_dwReapAccess = 0;

static void ProbeReap(void)
{
	static const DWORD adwTryAccess[] = {0x0400 /*PROCESS_QUERY_INFORMATION*/, PROCESS_ALL_ACCESS,
	                                     0};
	int i;
	s_dwReapAccess = 0;
	__try
	{
		for (i = 0; i < (int)(sizeof(adwTryAccess) / sizeof(adwTryAccess[0])); i++)
		{
			HANDLE h = OpenProcess(adwTryAccess[i], FALSE, GetCurrentProcessId());
			if (h)
			{
				CloseHandle(h);
				s_dwReapAccess = adwTryAccess[i];
				break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		s_dwReapAccess = 0;
	}
	DbgStr(s_dwReapAccess ? L"DCSHELL: dead-window reaper active\r\n"
	                      : L"DCSHELL: OpenProcess unusable, reaper off\r\n");
}

static void ReapDeadWindows(void)
{
	int i;
	if (!s_pShared || !s_dwReapAccess)
		return;
	for (i = 0; i < DCWIN_MAXWIN; i++)
	{
		HANDLE h;
		if (!s_pShared->win[i].inUse || !s_pShared->win[i].ownerPid)
			continue;
		h = OpenProcess(s_dwReapAccess, FALSE, s_pShared->win[i].ownerPid);
		if (h)
		{
			CloseHandle(h);
			continue;
		} // owner alive
		s_pShared->win[i].inUse = 0; // owner gone -> free the ghost slot
		s_bDirty = 1;
	}
}

// auto-focus newly-appeared windows; drop focus when the focused one closes
static void FixupFocus(void)
{
	int i;
	if (!s_pShared)
		return;
	if (s_nFocus >= 0 && !s_pShared->win[s_nFocus].inUse)
		s_nFocus = -1;
	for (i = 0; i < DCWIN_MAXWIN; i++)
	{
		int bNow = s_pShared->win[i].inUse ? 1 : 0;
		if (bNow != s_abWasInUse[i])
			s_bDirty = 1; // a window opened/closed -> recompose
		if (bNow && !s_abWasInUse[i])
			s_nFocus = i;
		s_abWasInUse[i] = bNow;
	}
}

// cycle focus: current -> next in-use window -> ... -> desktop (-1) -> wrap
static void CycleFocus(void)
{
	int i, nCur = s_nFocus;
	if (!s_pShared)
		return;
	for (i = 0; i < DCWIN_MAXWIN; i++)
	{
		nCur++;
		if (nCur >= DCWIN_MAXWIN)
		{
			s_nFocus = -1;
			return;
		}
		if (s_pShared->win[nCur].inUse)
		{
			s_nFocus = nCur;
			return;
		}
	}
	s_nFocus = -1;
}

//
// Rendering. Each layer: COLORFILL/icon fills (DC unlocked) then a GetDC text pass.
//
static void RenderDesktopFills(void)
{
	int i, x, y;

	GfxFill(0, 0, SCREEN_W, TASK_Y, CL_DESKTOP);
	for (i = 0; i < s_cDesk; i++)
	{
		int nLabelW = GfxTextWidth(g_FontUI, s_aDesk[i].label), nLabelX;
		x = s_aDeskPos[i].x;
		y = s_aDeskPos[i].y;
		nLabelX = x + ICELL_W / 2 - nLabelW / 2; // label centred under the icon
		if (i == s_nDeskSel && !s_bMenuOpen)
			GfxFill(nLabelX - 3, y + 36, nLabelX + nLabelW + 3, y + 52,
			        CL_TITLE);                  // selection box hugs the label
		GfxIconBig(s_aDesk[i].icon, x + 32, y); // 32x32 icon (centred in the 96px cell)
	}
}

static void RenderDesktopText(HDC hdc)
{
	int i, x, y;
	for (i = 0; i < s_cDesk; i++)
	{
		COLORREF bg = (i == s_nDeskSel && !s_bMenuOpen) ? CL_TITLE : CL_DESKTOP;
		int nLabelW = GfxTextWidth(g_FontUI, s_aDesk[i].label), nLabelX;
		x = s_aDeskPos[i].x;
		y = s_aDeskPos[i].y;
		nLabelX = x + ICELL_W / 2 - nLabelW / 2;
		GfxText(hdc, nLabelX, y + 37, CL_WHITE, bg, g_FontUI, s_aDesk[i].label);
	}
}

static void RenderTaskbarFills(void)
{
	RECT rc;

	GfxFill(0, TASK_Y, SCREEN_W, SCREEN_H, CL_FACE);
	SetRect(&rc, 0, TASK_Y, SCREEN_W, SCREEN_H);
	GfxBevel(&rc, TRUE);
	SetRect(&rc, 4, TASK_Y + 3, 72, SCREEN_H - 3);
	GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE);
	GfxBevel(&rc, TRUE);

	if (s_pShared) // one button per open window (focused = pressed)
	{
		int bx = 80, k;
		for (k = 0; k < DCWIN_MAXWIN; k++)
		{
			if (!s_pShared->win[k].inUse)
				continue;
			SetRect(&rc, bx, TASK_Y + 3, bx + 110, SCREEN_H - 3);
			GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE);
			GfxBevel(&rc, (k == s_nFocus) ? FALSE : TRUE);
			GfxIcon((int)s_pShared->win[k].icon, bx + 4, TASK_Y + 6);
			bx += 116;
		}
	}

	if (s_bMenuOpen)
	{
		int h = (int)START_N * ROW_H + 8;
		int my = TASK_Y - h + 4, i;
		SetRect(&rc, 4, TASK_Y - h, 4 + MENU_W, TASK_Y);
		GfxFill(rc.left, rc.top, rc.right, rc.bottom, CL_FACE);
		GfxBevel(&rc, TRUE);
		if (s_nMenuSel >= 0 && s_nMenuSel < (int)START_N)
			GfxFill(8, my + s_nMenuSel * ROW_H, MENU_W, my + (s_nMenuSel + 1) * ROW_H, CL_SEL);
		for (i = 0; i < (int)START_N; i++)
			GfxIcon(s_start[i].icon, 10, my + i * ROW_H + 1);
	}
}

static void RenderBarsText(HDC hdc)
{
	SYSTEMTIME t;
	WCHAR szClk[16];
	int i;

	GfxText(hdc, 12, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontBold, L"Start");
	GetLocalTime(&t);
	wsprintfW(szClk, L"%02d:%02d", t.wHour, t.wMinute);
	GfxText(hdc, SCREEN_W - 44, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, szClk);

	if (s_pShared)
	{
		int bx = 80, k;
		for (k = 0; k < DCWIN_MAXWIN; k++)
		{
			if (!s_pShared->win[k].inUse)
				continue;
			GfxText(hdc, bx + 24, TASK_Y + 5, CL_TEXT, CL_FACE, g_FontUI, s_pShared->win[k].title);
			bx += 116;
		}
	}

	if (s_bMenuOpen)
	{
		int h = (int)START_N * ROW_H + 8;
		int my = TASK_Y - h + 4;
		for (i = 0; i < (int)START_N; i++)
		{
			COLORREF fg = (i == s_nMenuSel) ? CL_WHITE : CL_TEXT;
			COLORREF bg = (i == s_nMenuSel) ? CL_SEL : CL_FACE;
			GfxText(hdc, 30, my + i * ROW_H + 1, fg, bg, g_FontUI, s_start[i].label);
		}
	}
}

static void SnapWindows(void)
{
	int wi, i, nTries;

	if (!s_pShared)
		return;
	for (wi = 0; wi < DCWIN_MAXWIN; wi++)
	{
		DcWindow *w = &s_pShared->win[wi];
		s_anSnapN[wi] = 0;
		if (!w->inUse)
			continue;
		for (nTries = 0; nTries < 8; nTries++)
		{
			DWORD dwGen1 = w->gen, dwGen2;
			int cCmd;
			if (dwGen1 & 1)
			{
				Sleep(1);
				continue;
			} // client mid-write (Sleep(1):
			//            we run ABOVE_NORMAL, so Sleep(0) would never yield to the
			//            lower-priority client writing the frame -> it could never finish
			cCmd = (int)w->cmdCount;
			if (cCmd > DCWIN_MAXCMD)
				cCmd = DCWIN_MAXCMD;
			for (i = 0; i < cCmd; i++)
				s_aSnap[wi][i] = w->cmd[i];
			dwGen2 = w->gen;
			if (dwGen1 == dwGen2)
			{
				s_anSnapN[wi] = cCmd;
				break;
			} // consistent
		}
	}
}

static void DrawWinFills(DcWindow *w, int wi, BOOL bActive)
{
	RECT rcFrame;
	int i;

	SetRect(&rcFrame, w->x - 2, w->y - 18, w->x + w->w + 2, w->y + w->h + 2); // frame
	GfxFill(rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom, CL_FACE);
	GfxBevel(&rcFrame, TRUE);
	GfxFill(w->x - 2, w->y - 18, w->x + w->w + 2, w->y - 2,
	        bActive ? CL_TITLE : RGB(112, 112, 112));
	GfxIcon((int)w->icon, w->x, w->y - 18);                                   // title-bar icon
	GfxFill(w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3, CL_FACE); // close box
	{
		RECT rcClose;
		SetRect(&rcClose, w->x + w->w - 14, w->y - 16, w->x + w->w + 1, w->y - 3);
		GfxBevel(&rcClose, TRUE);
	}
	{ // maximize/restore box (left of close) + its glyph (a little window w/ a title bar)
		int nMaxX0 = w->x + w->w - 30, nMaxY0 = w->y - 16;
		RECT rcMax;
		SetRect(&rcMax, nMaxX0, nMaxY0, w->x + w->w - 15, w->y - 3);
		GfxFill(rcMax.left, rcMax.top, rcMax.right, rcMax.bottom, CL_FACE);
		GfxBevel(&rcMax, TRUE);
		GfxFill(nMaxX0 + 3, nMaxY0 + 3, nMaxX0 + 12, nMaxY0 + 10, CL_TEXT); // outline
		GfxFill(nMaxX0 + 4, nMaxY0 + 5, nMaxX0 + 11, nMaxY0 + 9,
		        CL_FACE); // interior (leaves a top "title bar")
	}
	{ // minimize box (left of maximize) + its glyph (a low bar)
		int nMinX0 = w->x + w->w - 46, nMinY0 = w->y - 16;
		RECT rcMin;
		SetRect(&rcMin, nMinX0, nMinY0, w->x + w->w - 31, w->y - 3);
		GfxFill(rcMin.left, rcMin.top, rcMin.right, rcMin.bottom, CL_FACE);
		GfxBevel(&rcMin, TRUE);
		GfxFill(nMinX0 + 3, nMinY0 + 8, nMinX0 + 12, nMinY0 + 10, CL_TEXT); // the "minimize" bar
	}

	GfxSetClip(w->x, w->y, w->x + w->w, w->y + w->h); // clip content to the client area
	for (i = 0; i < s_anSnapN[wi]; i++)
	{
		DcCmd *c = &s_aSnap[wi][i];
		if (c->op == DCOP_FILL)
			GfxFill(w->x + c->x, w->y + c->y, w->x + c->x + c->w, w->y + c->y + c->h, c->color);
		else if (c->op == DCOP_ICON)
			GfxIcon((int)c->color, w->x + c->x, w->y + c->y);
	}
	GfxClearClip();
}

static void DrawWinText(HDC hdc, DcWindow *w, int wi, BOOL bActive)
{
	int i;

	// Clip title-bar text to the bar: a glyph cell is GH(16)px tall but the bar is only
	// 16px, so without this the opaque text bg quad spills ~2px below the bar's bottom edge
	// (the blue "dip" under the caption). The glyph itself fits, only the overhang is cut.
	GfxSetClip(w->x - 2, w->y - 18, w->x + w->w + 2, w->y - 2);
	GfxText(hdc, w->x + 18, w->y - 16, CL_WHITE, bActive ? CL_TITLE : RGB(112, 112, 112),
	        g_FontBold, w->title);
	GfxText(hdc, w->x + w->w - 11, w->y - 16, CL_TEXT, CL_FACE, g_FontUI, L"X");
	GfxClearClip();
	GfxSetClip(w->x, w->y, w->x + w->w, w->y + w->h); // clip content text to the client area
	for (i = 0; i < s_anSnapN[wi]; i++)
	{
		DcCmd *c = &s_aSnap[wi][i];
		if (c->op == DCOP_TEXT)
			GfxText(hdc, w->x + c->x, w->y + c->y, c->color, c->color2, g_FontUI, c->text);
	}
	GfxClearClip();
}

// Composite ONE window completely (fills, then text) so an overlapping window's
// text never lands on a window above it.
static void RenderWindow(int wi, BOOL bActive)
{
	HDC hdc;
	DcWindow *w = &s_pShared->win[wi];

	DrawWinFills(w, wi, bActive);
	hdc = GfxLockDC();
	if (hdc)
	{
		DrawWinText(hdc, w, wi, bActive);
		GfxUnlockDC(hdc);
	}
}

// Repaint the static desktop layer (bg + icons + labels) into the dcgfx desktop
// cache. Called only when its visual inputs (selection, menu-open) change - never
// for clock ticks, window republishes, or focus changes.
static void RebuildDesktopCache(void)
{
	GfxBeginDesktopCache();
	RenderDesktopFills();
	RenderDesktopText((HDC)1); // GfxText ignores the HDC; target is s_lockPix
	GfxEndDesktopCache();
	s_bDeskDirty = 0;
	s_nCachedSel = s_nDeskSel;
	s_nCachedMenu = s_bMenuOpen;
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
	if (s_bDeskDirty || s_nDeskSel != s_nCachedSel || s_bMenuOpen != s_nCachedMenu)
		RebuildDesktopCache();
	GfxBlitDesktopCache();

	// layers 1..N: app windows, back-to-front, each composited fully. The quad arrays grow on
	// demand (dcgfx), so a text-heavy window can't starve the taskbar/cursor drawn after it.
	if (s_pShared)
	{
		for (i = 0; i < DCWIN_MAXWIN; i++)
			if (s_pShared->win[i].inUse && !s_abWinMin[i] && i != s_nFocus)
				RenderWindow(i, FALSE);
		if (s_nFocus >= 0 && s_pShared->win[s_nFocus].inUse && !s_abWinMin[s_nFocus])
			RenderWindow(s_nFocus, TRUE);
	}

	// top layer: taskbar + Start menu
	RenderTaskbarFills();
	hdc = GfxLockDC();
	if (hdc)
	{
		RenderBarsText(hdc);
		GfxUnlockDC(hdc);
	}
	// The cursor is NOT in the scene here - GfxPresent appends it as the last quad.
}

//
// Input
//
static void ActivateStartItem(void)
{
	const SHORTCUT *pShc = &s_start[s_nMenuSel];
	s_bMenuOpen = 0;
	if (pShc->path)
		LaunchApp(L"dcwexp.exe", pShc->path); // Explorer window at this path
	else if (pShc->exe)
		ShellLaunch(pShc->exe); // dcw* -> composited; others -> display hand-off
}

static void OnKey(WPARAM wParam)
{
	if (wParam == VK_TAB) // task-switch; or Start menu when no windows open
	{
		if (CountWindows() == 0)
		{
			s_bMenuOpen = !s_bMenuOpen;
			s_nMenuSel = 0;
		}
		else
			CycleFocus();
		s_bDirty = 1;
		return;
	}

	// A focused window captures input: forward the key to its ring and return
	// WITHOUT marking the scene dirty. The shell must NOT recomposite here - the
	// window content hasn't changed yet; when the client processes the key and
	// republishes (gen bump) the loop's gen-change check triggers exactly one
	// render. (Marking dirty here recomposited stale frames every keypress and
	// kept the shell busy, starving the client -> the in-window nav lag.)
	if (s_nFocus >= 0 && s_pShared && s_pShared->win[s_nFocus].inUse)
	{
		DcWindow *w = &s_pShared->win[s_nFocus];
		if (wParam == VK_ESCAPE)
		{
			w->wantClose = 1;
			return;
		} // FixupFocus redraws on close
		w->in[w->inHead % DCWIN_MAXIN].type = 1;
		w->in[w->inHead % DCWIN_MAXIN].key = (DWORD)wParam;
		w->inHead++;
		return;
	}

	if (s_bMenuOpen)
	{
		if (wParam == VK_UP && s_nMenuSel > 0)
			s_nMenuSel--;
		if (wParam == VK_DOWN && s_nMenuSel < (int)START_N - 1)
			s_nMenuSel++;
		if (wParam == VK_RETURN)
			ActivateStartItem();
		if (wParam == VK_ESCAPE)
			s_bMenuOpen = 0;
		s_bDirty = 1;
		return;
	}

	// desktop - grid navigation (column-major: index = col*rows + row)
	s_bDirty = 1;
	{
		int nRow = s_nDeskSel % s_nDeskRows;
		if (wParam == VK_UP && nRow > 0)
			s_nDeskSel--;
		if (wParam == VK_DOWN && nRow < s_nDeskRows - 1 && s_nDeskSel + 1 < s_cDesk)
			s_nDeskSel++;
		if (wParam == VK_LEFT && s_nDeskSel - s_nDeskRows >= 0)
			s_nDeskSel -= s_nDeskRows;
		if (wParam == VK_RIGHT && s_nDeskSel + s_nDeskRows < s_cDesk)
			s_nDeskSel += s_nDeskRows;
	}
	if (wParam == VK_DELETE)
	{
		RemoveShortcut(s_nDeskSel);
		return;
	} // remove a user-added shortcut
	if (wParam == VK_RETURN)
	{
		if (s_aDesk[s_nDeskSel].path)
			LaunchApp(L"dcwexp.exe", s_aDesk[s_nDeskSel].path); // Explorer window
		else
			ShellLaunch(s_aDesk[s_nDeskSel].exe); // hand off the display for non-dcw apps
	}
}

// Maximize/restore a window (toggle). Full-screen = the desktop area above the taskbar,
// leaving room for the title bar. Saves the pre-maximize geometry to restore.
static void ToggleMaximize(int wi)
{
	DcWindow *w;
	if (!s_pShared || wi < 0 || wi >= DCWIN_MAXWIN || !s_pShared->win[wi].inUse)
		return;
	w = &s_pShared->win[wi];
	if (s_abWinMax[wi])
	{
		w->x = s_awinRestore[wi][0];
		w->y = s_awinRestore[wi][1];
		w->w = s_awinRestore[wi][2];
		w->h = s_awinRestore[wi][3];
		s_abWinMax[wi] = 0;
	}
	else
	{
		s_awinRestore[wi][0] = w->x;
		s_awinRestore[wi][1] = w->y;
		s_awinRestore[wi][2] = w->w;
		s_awinRestore[wi][3] = w->h;
		w->x = 2;
		w->y = 20; // title bar sits at y=2..20
		w->w = SCREEN_W - 4;
		w->h = TASK_Y - 22; // fill down to the taskbar
		s_abWinMax[wi] = 1;
	}
	s_bDirty = 1;
}

// Cursor hit-test (top-down): Start menu -> Start button -> taskbar buttons ->
// windows (close box / maximize box / body) -> desktop icons. Returns 1 if the click was
// consumed by a target (close box, taskbar/start, desktop icon), 0 if it landed
// on a window body (focus it, but let an Enter through to the window) or empty
// space. The controller's "A" uses the return: consumed -> done; else -> Enter.
static int HandleClick(int x, int y)
{
	int k;
	s_bDirty = 1;

	if (s_bMenuOpen)
	{
		int h = (int)START_N * ROW_H + 8, my = TASK_Y - h + 4;
		if (x >= 4 && x < 4 + MENU_W && y >= my && y < my + (int)START_N * ROW_H)
		{
			s_nMenuSel = (y - my) / ROW_H;
			ActivateStartItem();
			return 1;
		}
		s_bMenuOpen = 0; // click elsewhere closes the menu
		return 1;
	}
	if (y >= TASK_Y && x >= 4 && x < 72) // Start button
	{
		s_bMenuOpen = !s_bMenuOpen;
		s_nMenuSel = 0;
		return 1;
	}
	if (y >= TASK_Y && s_pShared) // taskbar window buttons
	{
		int bx = 80;
		for (k = 0; k < DCWIN_MAXWIN; k++)
		{
			if (!s_pShared->win[k].inUse)
				continue;
			if (x >= bx && x < bx + 110) // restore a minimized window + focus it
			{
				if (s_abWinMin[k])
					s_abWinMin[k] = 0;
				s_nFocus = k;
				s_bDirty = 1;
				return 1;
			}
			bx += 116;
		}
	}
	if (s_pShared) // windows: topmost (focused) first
	{
		int anOrder[DCWIN_MAXWIN + 1], n = 0, j;
		if (s_nFocus >= 0 && s_pShared->win[s_nFocus].inUse)
			anOrder[n++] = s_nFocus;
		for (k = 0; k < DCWIN_MAXWIN; k++)
			if (s_pShared->win[k].inUse && k != s_nFocus)
				anOrder[n++] = k;
		for (j = 0; j < n; j++)
		{
			DcWindow *w = &s_pShared->win[anOrder[j]];
			if (s_abWinMin[anOrder[j]])
				continue; // minimized: not on the desktop
			if (x >= w->x - 2 && x < w->x + w->w + 2 && y >= w->y - 18 && y < w->y + w->h + 2)
			{
				if (x >= w->x + w->w - 14 && x <= w->x + w->w + 1 && y >= w->y - 16 &&
				    y <= w->y - 3)
				{
					w->wantClose = 1;
					return 1;
				} // close box -> consumed
				if (x >= w->x + w->w - 30 && x <= w->x + w->w - 15 && y >= w->y - 16 &&
				    y <= w->y - 3)
				{
					ToggleMaximize(anOrder[j]);
					return 1;
				} // maximize/restore box
				if (x >= w->x + w->w - 46 && x <= w->x + w->w - 31 && y >= w->y - 16 &&
				    y <= w->y - 3)
				{
					s_abWinMin[anOrder[j]] = 1;
					if (s_nFocus == anOrder[j])
						s_nFocus = -1;
					s_bDirty = 1;
					return 1;
				} // minimize
				s_nFocus = anOrder[j]; // body -> focus it, fall through to Enter
				return 0;
			}
		}
	}
	for (k = 0; k < s_cDesk; k++) // desktop icons (mutable cell positions)
	{
		int nIconX = s_aDeskPos[k].x, nIconY = s_aDeskPos[k].y;
		if (x >= nIconX && x < nIconX + ICELL_W && y >= nIconY && y < nIconY + ICELL_H)
		{
			s_nDeskSel = k;
			if (s_aDesk[k].path)
				LaunchApp(L"dcwexp.exe", s_aDesk[k].path);
			else
				ShellLaunch(s_aDesk[k].exe);
			return 1;
		}
	}
	return 0; // empty space
}

// ---- pointer drag: move/resize windows, move desktop shortcuts --------------------
// Hit-test at a pointer-DOWN to decide what (if anything) a drag would grab.
static void DragHitTest(int x, int y)
{
	int j, k;
	s_nDragKind = DRAG_NONE;
	s_nDragTarget = -1;

	if (s_bMenuOpen) // grabbing a Start-menu row -> drag-to-desktop
	{
		int h = (int)START_N * ROW_H + 8, my = TASK_Y - h + 4, nRow;
		if (x >= 4 && x < 4 + MENU_W && y >= my && y < my + (int)START_N * ROW_H)
		{
			nRow = (y - my) / ROW_H;
			if (nRow >= 0 && nRow < (int)START_N && (s_start[nRow].exe || s_start[nRow].path))
			{
				s_nDragKind = DRAG_STARTITEM;
				s_nDragTarget = nRow;
			}
		}
		return; // menu is modal: never grab windows/icons under it
	}
	if (s_pShared && !s_bMenuOpen) // windows, top-most (focused) first
	{
		int anOrder[DCWIN_MAXWIN + 1], n = 0;
		if (s_nFocus >= 0 && s_pShared->win[s_nFocus].inUse)
			anOrder[n++] = s_nFocus;
		for (k = 0; k < DCWIN_MAXWIN; k++)
			if (s_pShared->win[k].inUse && k != s_nFocus)
				anOrder[n++] = k;
		for (j = 0; j < n; j++)
		{
			DcWindow *w = &s_pShared->win[anOrder[j]];
			if (s_abWinMin[anOrder[j]])
				continue; // minimized windows aren't on the desktop
			// bottom-right corner (12x12) -> resize
			if (x >= w->x + w->w - 12 && x <= w->x + w->w + 2 && y >= w->y + w->h - 12 &&
			    y <= w->y + w->h + 2)
			{
				s_nDragKind = DRAG_WSIZE;
				s_nDragTarget = anOrder[j];
				s_nDragOffX = x - (w->x + w->w);
				s_nDragOffY = y - (w->y + w->h);
				s_nFocus = anOrder[j];
				return;
			}
			// title bar (excluding the min/max/close boxes) -> move
			if (x >= w->x - 2 && x < w->x + w->w - 46 && y >= w->y - 18 && y < w->y - 2)
			{
				s_nDragKind = DRAG_WMOVE;
				s_nDragTarget = anOrder[j];
				s_nDragOffX = x - w->x;
				s_nDragOffY = y - w->y;
				s_nFocus = anOrder[j];
				return;
			}
		}
	}
	for (k = 0; k < s_cDesk; k++) // desktop icon -> move
	{
		int nIconX = s_aDeskPos[k].x, nIconY = s_aDeskPos[k].y;
		if (x >= nIconX && x < nIconX + ICELL_W && y >= nIconY && y < nIconY + ICELL_H)
		{
			s_nDragKind = DRAG_ICON;
			s_nDragTarget = k;
			s_nDragOffX = x - nIconX;
			s_nDragOffY = y - nIconY;
			s_nDeskSel = k;
			return;
		}
	}
}

// Drop a desktop icon to (x,y): clamp into the desktop and persist if it's a user shortcut.
static void DropIcon(int nTarget, int x, int y)
{
	int nNewX = x - s_nDragOffX, nNewY = y - s_nDragOffY;
	if (nNewX < 0)
		nNewX = 0;
	if (nNewX > SCREEN_W - ICELL_W)
		nNewX = SCREEN_W - ICELL_W;
	if (nNewY < 0)
		nNewY = 0;
	if (nNewY > TASK_Y - ICELL_H)
		nNewY = TASK_Y - ICELL_H;
	s_aDeskPos[nTarget].x = nNewX;
	s_aDeskPos[nTarget].y = nNewY;
	s_bDeskDirty = 1; // icons live in the cached desktop layer
	if (nTarget < s_cDesk && s_anDeskUser[nTarget] >= 0)
		SaveShortcuts();
}

// Apply a drag while the pointer is held + has moved past the threshold.
static void DragApply(int x, int y)
{
	if (s_nDragKind == DRAG_ICON)
	{
		// Windows-style: the original icon stays put; a translucent ghost (drawn by GfxPresent
		// at the cursor) tracks the drag, and the icon commits to the new spot on drop.
	}
	else if (s_pShared && s_nDragTarget >= 0 && s_pShared->win[s_nDragTarget].inUse)
	{
		DcWindow *w = &s_pShared->win[s_nDragTarget];
		if (s_nDragKind == DRAG_WMOVE)
		{
			int nNewX = x - s_nDragOffX, nNewY = y - s_nDragOffY;
			if (nNewY < 18)
				nNewY = 18; // keep the title bar on-screen
			if (nNewX < -(w->w - 40))
				nNewX = -(w->w - 40);
			if (nNewX > SCREEN_W - 40)
				nNewX = SCREEN_W - 40;
			if (nNewY > TASK_Y - 1)
				nNewY = TASK_Y - 1;
			w->x = nNewX;
			w->y = nNewY;
		}
		else if (s_nDragKind == DRAG_WSIZE)
		{
			int nNewW = x - s_nDragOffX - w->x, nNewH = y - s_nDragOffY - w->y;
			if (nNewW < WIN_MINW)
				nNewW = WIN_MINW;
			if (nNewW > SCREEN_W)
				nNewW = SCREEN_W;
			if (nNewH < WIN_MINH)
				nNewH = WIN_MINH;
			if (nNewH > SCREEN_H)
				nNewH = SCREEN_H;
			w->w = nNewW;
			w->h = nNewH;
		}
	}
	s_bDirty = 1;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_ERASEBKGND:
			return 1; // we own the surface
		case WM_PAINT:
			ValidateRect(hWnd, NULL); // continuous present shows the frame; don't storm
			return 0;
		case WM_KEYDOWN:
			if (!s_bDiKbd)
				OnKey(wParam); // fallback when DI keyboard not available
			return 0;
		case WM_MOUSEMOVE: // GWES mouse fallback (if DC mouse isn't on DI)
			DInSetCursor((short)LOWORD(lParam), (short)HIWORD(lParam));
			if (!s_bWmMouseSeen)
			{
				s_bWmMouseSeen = 1;
				DbgStr(L"DCSHELL: WM mouse active\r\n");
			}
			return 0;
		case WM_LBUTTONDOWN:
			DInSetCursor((short)LOWORD(lParam), (short)HIWORD(lParam));
			DInPostClick();
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// Forward the analog-stick cursor to the window directly under it (client-relative), so client
// apps can hit-test their own controls (buttons, sliders, seek bars). Topmost-under-cursor wins;
// every other window gets ptrX = -1 (not over). Level-based - clients edge-detect btn for clicks.
static void PublishPointer(int nCursorX, int nCursorY, int bDown)
{
	int anOrder[DCWIN_MAXWIN], n = 0, nTop = -1, i, k;
	if (!s_pShared)
		return;
	if (s_nFocus >= 0 && s_pShared->win[s_nFocus].inUse && !s_abWinMin[s_nFocus])
		anOrder[n++] = s_nFocus;
	for (k = 0; k < DCWIN_MAXWIN; k++)
		if (s_pShared->win[k].inUse && !s_abWinMin[k] && k != s_nFocus)
			anOrder[n++] = k;
	for (i = 0; i < n; i++)
	{
		DcWindow *w = &s_pShared->win[anOrder[i]];
		if (nCursorX >= w->x && nCursorX < w->x + w->w && nCursorY >= w->y &&
		    nCursorY < w->y + w->h)
		{
			nTop = anOrder[i];
			break;
		}
	}
	for (k = 0; k < DCWIN_MAXWIN; k++)
	{
		DcWindow *w = &s_pShared->win[k];
		if (!w->inUse)
			continue;
		if ((int)k == nTop)
		{
			w->ptrX = nCursorX - w->x;
			w->ptrY = nCursorY - w->y;
			w->ptrBtn = (DWORD)(bDown ? 1 : 0);
		}
		else
			w->ptrX = -1;
	}
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpszCmd, int nShow)
{
	WNDCLASSW wc;
	MSG msg;
	DWORD dwNext, dwNextPresent = 0, dwFrameStart;
	int i, bMoved;

	DbgStr(L"DCSHELL: WinMain enter (hybrid desktop)\r\n");

	InitShared();

	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hbrBackground = NULL;
	wc.lpszClassName = L"DCSHELL";
	RegisterClassW(&wc);
	s_hwnd = CreateWindowExW(0, L"DCSHELL", L"DCShell", WS_VISIBLE, 0, 0, SCREEN_W, SCREEN_H, NULL,
	                         NULL, hInst, NULL);

	if (!GfxInit(s_hwnd))
	{
		DbgStr(L"DCSHELL: GfxInit failed\r\n");
		return 1;
	}
	DbgStr(L"DCSHELL: desktop up\r\n");
	DeskInit(); // seed shortcuts (built-ins + VMU-persisted) + lay out

	// Run a notch above client apps. We are the window server: input polling and the
	// cursor present must preempt a CPU-bound or fast-republishing client, or the
	// pointer stalls for a whole scheduler quantum while that client holds the SH-4.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	s_bDiKbd = DInInit(s_hwnd); // polled keyboard (low-latency) + controller pointer
	DbgStr(s_bDiKbd ? L"DCSHELL: DI keyboard active\r\n"
	                : L"DCSHELL: DI keyboard absent, WM fallback\r\n");
	ProbeReap(); // is OpenProcess usable for dead-window reaping?

	dwNext = GetTickCount() + 1000;
	for (;;)
	{
		dwFrameStart = GetTickCount(); // frame start (for the limiter + input-poll cost)
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

		DInUpdate(); // poll DI devices once per frame
		{
			DWORD dwVk;
			while (DInNextKey(&dwVk))
				OnKey(dwVk);
		}
		bMoved = 0;
		if (DInHasPointer())
		{
			int nOldX = s_nCx, nOldY = s_nCy;
			DInCursor(&s_nCx, &s_nCy);
			if (s_nCx != nOldX || s_nCy != nOldY)
				bMoved = 1; // cursor moved -> needs a present
		}
		// Pointer press / drag / release. A press that doesn't drag is a CLICK on release
		// (preserving the old click + controller-activate behaviour); a press that grabs a
		// title bar / window corner / desktop icon and then moves becomes a drag.
		{
			int bPtr = DInPointerDown();
			if (bPtr && !s_bPtrWas) // DOWN: arm a possible drag
			{
				s_nDownX = s_nCx;
				s_nDownY = s_nCy;
				s_bDragMoved = 0;
				DragHitTest(s_nCx, s_nCy);
				s_bDirty = 1;
			}
			else if (bPtr && s_bPtrWas) // HELD: drag once past the threshold
			{
				if (!s_bDragMoved &&
				    (abs(s_nCx - s_nDownX) > DRAG_THRESH || abs(s_nCy - s_nDownY) > DRAG_THRESH))
					s_bDragMoved = 1;
				if (s_bDragMoved && s_nDragKind != DRAG_NONE)
					DragApply(s_nCx, s_nCy);
			}
			else if (!bPtr && s_bPtrWas) // UP: drop, or click if it never moved
			{
				if (!s_bDragMoved)
				{
					if (!HandleClick(s_nCx, s_nCy))
						OnKey(VK_RETURN);
				}
				else if (s_nDragKind == DRAG_STARTITEM && s_nDragTarget >= 0)
				{
					int h = (int)START_N * ROW_H + 8, my = TASK_Y - h + 4;
					if (s_nCy < TASK_Y &&
					    (s_nCy < my || s_nCx >= 4 + MENU_W)) // dropped on the desktop
						AddDesktopShortcut(s_nDragTarget, s_nCx, s_nCy);
					s_bMenuOpen = 0;
				}
				else if (s_nDragKind == DRAG_ICON && s_nDragTarget >= 0 && s_nDragTarget < s_cDesk)
					DropIcon(s_nDragTarget, s_nCx, s_nCy); // commit the icon to the drop point
				s_nDragKind = DRAG_NONE;
				s_nDragTarget = -1;
				s_bDragMoved = 0;
				s_bDirty = 1;
			}
			s_bPtrWas = bPtr;
			PublishPointer(s_nCx, s_nCy, bPtr); // deliver the cursor to the window under it
		}
		// GWES WM-tap click path (DInPostClick) - independent of the held-button drag model.
		if (DInTookClick())
			HandleClick(s_nCx, s_nCy);

		FixupFocus(); // auto-focus new windows, drop closed ones
		if (s_pShared && s_pShared->execSeq != s_dwLastExec) // a window asked to launch an app
		{
			s_dwLastExec = s_pShared->execSeq;
			ShellLaunch(s_pShared->execPath);
		}
		if (GetTickCount() >= dwNext) // clock tick -> repaint + reap dead windows
		{
			s_bDirty = 1;
			dwNext += 1000;
			ReapDeadWindows(); // free slots whose owner process is gone
		}
		// re-render only when a window published a new frame (digit typed, clock tick)
		if (s_pShared)
			for (i = 0; i < DCWIN_MAXWIN; i++)
			{
				DWORD dwGen = s_pShared->win[i].inUse ? s_pShared->win[i].gen : 0;
				if (dwGen != s_adwLastGen[i])
				{
					s_bDirty = 1;
					s_adwLastGen[i] = dwGen;
				}
				if (!s_pShared->win[i].inUse)
				{
					s_abWinMax[i] = 0;
					s_abWinMin[i] = 0;
				} // freed slot -> normal
			}
		// Drag ghost: a translucent icon trails the cursor while a drag is in flight.
		if (s_bDragMoved && s_nDragKind == DRAG_ICON && s_nDragTarget >= 0 &&
		    s_nDragTarget < s_cDesk)
			GfxSetDragGhost(s_aDesk[s_nDragTarget].icon);
		else if (s_bDragMoved && s_nDragKind == DRAG_STARTITEM && s_nDragTarget >= 0 &&
		         s_nDragTarget < (int)START_N)
			GfxSetDragGhost(s_start[s_nDragTarget].icon);
		else
			GfxSetDragGhost(-1);
		// The scene is a PVR2 quad list that GfxPresent consumes + clears, so we MUST
		// Render() (rebuild quads) before every present - a bare present would submit
		// an empty scene. Rebuild is cheap (no rasterization), so recompose+present on
		// any change, cursor move, or keepalive.
		if (s_bDirty || bMoved || GetTickCount() >= dwNextPresent)
		{
			Render();
			s_bDirty = 0;
			if (GfxPresent(s_nCx, s_nCy, DInHasPointer()))
				s_bDirty = 1; // surface lost -> rebuild next loop
			dwNextPresent = GetTickCount() + 100;
		}
		// Pace to the PVR vblank (~60 Hz) instead of Sleep, which rounds up to the
		// ~25 ms CE tick + a 25 ms guard (= the 20 fps cap). WaitForVerticalBlank
		// yields during the blank at the real refresh rate; if it no-ops (returns
		// non-OK, or the loop ran <8 ms so it clearly didn't pace), fall back to Sleep.
		{
			HRESULT hrVb = GfxWaitVBlank();
			DWORD dwElapsed = GetTickCount() - dwFrameStart;
			if (hrVb != 0 /*DD_OK*/ || dwElapsed < 8)
				Sleep(dwElapsed < 16 ? 16 - dwElapsed : 1);
		}
	}
}
