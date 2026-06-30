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

typedef struct
{
	WCHAR name[128];
	DWORD attr;
	DWORD size;
} ENT;

static ENT g_aEnt[MAXENT];
static int g_nCount, g_nSel, g_nTop;
static WCHAR g_szDir[MAX_PATH] = L"\\";

static void CopyN(WCHAR *pszDst, const WCHAR *psz, int n)
{
	int i;
	for (i = 0; i < n - 1 && psz[i]; i++)
		pszDst[i] = psz[i];
	pszDst[i] = 0;
}
static int IsRoot(void)
{
	return g_szDir[0] == L'\\' && g_szDir[1] == 0;
}
static int IsDir(ENT *pEnt)
{
	return (pEnt->attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
static int EndsWithExe(const WCHAR *psz)
{
	int n = lstrlenW(psz);
	if (n < 4 || psz[n - 4] != L'.')
		return 0;
	return (psz[n - 3] | 32) == 'e' && (psz[n - 2] | 32) == 'x' && (psz[n - 1] | 32) == 'e';
}
static int EndsWithExt3(const WCHAR *psz, WCHAR a, WCHAR b, WCHAR c)
{
	int n = lstrlenW(psz);
	if (n < 4 || psz[n - 4] != L'.')
		return 0;
	return (psz[n - 3] | 32) == a && (psz[n - 2] | 32) == b && (psz[n - 1] | 32) == c;
}
static int IsAudio(const WCHAR *psz)
{
	return EndsWithExt3(psz, 'm', 'p', '3') || EndsWithExt3(psz, 'w', 'a', 'v');
}
static void JoinPath(WCHAR *pszOut, const WCHAR *pszDir, const WCHAR *pszName)
{
	if (pszDir[1] == 0)
		wsprintfW(pszOut, L"\\%s", pszName);
	else
		wsprintfW(pszOut, L"%s\\%s", pszDir, pszName);
}
static int FileIcon(ENT *pEnt)
{
	return IsDir(pEnt) ? ICON_FOLDER : (EndsWithExe(pEnt->name) ? ICON_APP : ICON_FILE);
}

static void ScanDir(void)
{
	WCHAR aszPat[MAX_PATH];
	WIN32_FIND_DATAW fd;
	HANDLE h;
	g_nCount = 0;
	g_nSel = 0;
	g_nTop = 0;
	if (!IsRoot())
	{
		lstrcpyW(g_aEnt[g_nCount].name, L"..");
		g_aEnt[g_nCount].attr = FILE_ATTRIBUTE_DIRECTORY;
		g_aEnt[g_nCount].size = 0;
		g_nCount++;
	}
	if (IsRoot())
		lstrcpyW(aszPat, L"\\*.*");
	else
		wsprintfW(aszPat, L"%s\\*.*", g_szDir);
	h = FindFirstFileW(aszPat, &fd);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (g_nCount >= MAXENT)
				break;
			if (fd.cFileName[0] == L'.' &&
			    (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
				continue;
			CopyN(g_aEnt[g_nCount].name, fd.cFileName, 128);
			g_aEnt[g_nCount].attr = fd.dwFileAttributes;
			g_aEnt[g_nCount].size = fd.nFileSizeLow;
			g_nCount++;
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}
}

static void GoParent(void)
{
	int i, nLast = -1;
	if (IsRoot())
		return;
	for (i = 0; g_szDir[i]; i++)
		if (g_szDir[i] == L'\\')
			nLast = i;
	if (nLast <= 0)
		lstrcpyW(g_szDir, L"\\");
	else
		g_szDir[nLast] = 0;
	ScanDir();
}

static DCWin *g_pWin;

static void EnterSel(void)
{
	ENT *pEnt;
	WCHAR aszFull[MAX_PATH];
	if (g_nSel < 0 || g_nSel >= g_nCount)
		return;
	pEnt = &g_aEnt[g_nSel];
	if (lstrcmpW(pEnt->name, L"..") == 0)
	{
		GoParent();
		return;
	}
	if (IsDir(pEnt))
	{
		JoinPath(aszFull, g_szDir, pEnt->name);
		lstrcpyW(g_szDir, aszFull);
		ScanDir();
		return;
	}
	if (EndsWithExe(pEnt->name))
	{
		JoinPath(aszFull, g_szDir, pEnt->name);
		DCWinExec(g_pWin, aszFull); // shell launches it (windowed dcw* or fullscreen hand-off)
	}
	else if (IsAudio(pEnt->name)) // .mp3/.wav -> open in the music player with the path as its arg
	{
		WCHAR aszCmd[MAX_PATH + 16];
		JoinPath(aszFull, g_szDir, pEnt->name);
		lstrcpyW(aszCmd, L"dcwplay.exe ");
		lstrcatW(aszCmd, aszFull);
		DCWinExec(g_pWin, aszCmd); // shell splits "dcwplay.exe <path>" into exe + arg
	}
}

static void HandleKey(DWORD dwKey)
{
	if (dwKey == VK_UP && g_nSel > 0)
		g_nSel--;
	if (dwKey == VK_DOWN && g_nSel < g_nCount - 1)
		g_nSel++;
	if (dwKey == VK_RETURN)
		EnterSel();
	if (dwKey == VK_BACK)
		GoParent();
}

static void Draw(DCWin *pWin)
{
	int i, y, nCw = EW, nCh = EH, nVis;
	DCWinClientSize(pWin, &nCw, &nCh); // current size (shell may have resized us)
	nVis = (nCh - 18) / EROW;
	if (nVis < 1)
		nVis = 1; // rows that fit -> more files when taller
	DCWinBeginFrame(pWin);
	DCWinFillBg(pWin, RGB(255, 255, 255));              // list background fills the window
	DCWinFill(pWin, 0, 0, nCw, 16, RGB(192, 192, 192)); // path bar spans the width
	DCWinText(pWin, 4, 1, RGB(0, 0, 0), RGB(192, 192, 192), g_szDir);

	if (g_nSel < g_nTop)
		g_nTop = g_nSel;
	if (g_nSel >= g_nTop + nVis)
		g_nTop = g_nSel - nVis + 1;

	for (i = g_nTop; i < g_nCount && i < g_nTop + nVis; i++)
	{
		ENT *pEnt = &g_aEnt[i];
		int bSel = (i == g_nSel);
		COLORREF bg = bSel ? RGB(0, 0, 128) : RGB(255, 255, 255);
		COLORREF fg = bSel ? RGB(255, 255, 255) : RGB(0, 0, 0);
		y = 18 + (i - g_nTop) * EROW;
		if (bSel)
			DCWinFill(pWin, 2, y, nCw - 4, EROW, RGB(0, 0, 128)); /* (x,y,WIDTH,HEIGHT) */
		DCWinIcon(pWin, 4, y + 2, FileIcon(pEnt));
		DCWinText(pWin, 24, y + 3, fg, bg, pEnt->name);
		if (!IsDir(pEnt))
		{
			WCHAR aszSz[20];
			wsprintfW(aszSz, L"%u", pEnt->size);
			DCWinText(pWin, nCw - 70, y + 3, fg, bg, aszSz);
		}
	}
	DCWinEndFrame(pWin);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *pWin;
	DWORD dwKey;

	if (lpCmd && lpCmd[0])
		CopyN(g_szDir, lpCmd, MAX_PATH); // start path from the command line

	pWin = DCWinOpen(120, 70, EW, EH, L"Explorer", ICON_FOLDER);
	if (!pWin)
	{
		OutputDebugStringW(L"DCWEXP: DCWinOpen failed\r\n");
		return 1;
	}
	g_pWin = pWin;
	ScanDir();
	Draw(pWin);

	for (;;)
	{
		int bChanged = 0;
		while (DCWinPollKey(pWin, &dwKey))
		{
			HandleKey(dwKey);
			bChanged = 1;
		}
		if (DCWinResized(pWin))
			bChanged = 1; // shell resized us -> redraw to fit
		if (bChanged)
			Draw(pWin);
		if (DCWinShouldClose(pWin))
			break;
		Sleep(20);
	}

	DCWinClose(pWin);
	return 0;
}
