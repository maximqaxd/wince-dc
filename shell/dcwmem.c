//
// dcwmem.c - Memory / MMU test: a DCWin client that proves RAM is real (not an
// aliased mirror) and exercises the MMU through VirtualAlloc.
//
// It commits one large block (as much as the 32MB process slot / free RAM allows)
// and runs three two-pass tests:
//   1. address-in-address - every 32-bit cell is written with its own virtual
//      address, then the whole block is read back and verified. Because the block
//      is FILLED in full before it is CHECKED, and the block is far larger than the
//      SH-4 cache (16KB), every line is evicted between write and read - so the
//      verify reads physical RAM, not a cached copy. If the upper 16MB is a mirror
//      of the lower (a 32MB image on a 16MB box), two virtual pages alias one
//      physical page: the second write clobbers the first, and the verify catches
//      the mismatch. A genuine 32MB machine passes.
//   2/3. 0x55555555 / 0xAAAAAAAA pattern passes - catch stuck data bits.
//
// The work is done a chunk per loop so the window stays responsive and closable.
// Press Enter (or the A button) to run; results show MB tested, pass/fail, and the
// first bad address. GlobalMemoryStatus' total is the headline 16-vs-32MB number.
//
#include "dcwlib.h"

#define CW      324
#define CH      208
#define CHUNK   (512 * 1024 / 4)   // 512K words = 2MB processed per loop pass
#define MAXTEST (24 * 1024 * 1024) // cap one VirtualAlloc (CE gives a 32MB slot/proc)
#define MARGIN  (2 * 1024 * 1024)  // leave headroom so the system stays alive

enum
{
	ST_IDLE,
	ST_FILLA,
	ST_CHKA,
	ST_FILL5,
	ST_CHK5,
	ST_FILLB,
	ST_CHKB,
	ST_DONE,
	ST_FAIL
};

static DWORD *g_pdwBlk; // committed test block (NULL = none)
static DWORD g_dwBytes; // block size in bytes
static DWORD g_dwWords; // block size in words
static DWORD g_dwPos;   // cursor within the current phase (words)
static int g_nState = ST_IDLE;
static DWORD g_dwFailAddr, g_dwFailGot, g_dwFailExp;
static WCHAR g_awchMsg[48] = L"";

// Allocate the biggest block we reasonably can: aim for availPhys-MARGIN, capped,
// and step down by 1MB until a commit succeeds (VA fragmentation can bite the top).
static DWORD *AllocBig(DWORD *pdwGotBytes)
{
	MEMORYSTATUS ms;
	DWORD dwWant, dwB;
	void *pv;
	memset(&ms, 0, sizeof(ms));
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatus(&ms);
	dwWant = (ms.dwAvailPhys > MARGIN) ? ms.dwAvailPhys - MARGIN : 0;
	if (dwWant > MAXTEST)
		dwWant = MAXTEST;
	dwWant &= ~0xFFFFu; // 64K align
	for (dwB = dwWant; dwB >= 0x100000; dwB -= 0x100000)
	{
		pv = VirtualAlloc(NULL, dwB, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (pv)
		{
			*pdwGotBytes = dwB;
			return (DWORD *)pv;
		}
	}
	*pdwGotBytes = 0;
	return NULL;
}

static void Fail(DWORD i, DWORD dwGot, DWORD dwExp)
{
	g_dwFailAddr = (DWORD)(g_pdwBlk + i);
	g_dwFailGot = dwGot;
	g_dwFailExp = dwExp;
	g_nState = ST_FAIL;
	lstrcpyW(g_awchMsg, L"FAIL - memory/MMU error");
}

// Process one CHUNK of the current phase; advance phase at end of block.
static void StepTest(void)
{
	DWORD dwEnd, i;
	if (g_nState < ST_FILLA || g_nState > ST_CHKB)
		return;
	dwEnd = g_dwPos + CHUNK;
	if (dwEnd > g_dwWords)
		dwEnd = g_dwWords;

	switch (g_nState)
	{
		case ST_FILLA: // cell = its own address
			for (i = g_dwPos; i < dwEnd; i++)
				g_pdwBlk[i] = (DWORD)(g_pdwBlk + i);
			break;
		case ST_CHKA:
			for (i = g_dwPos; i < dwEnd; i++)
				if (g_pdwBlk[i] != (DWORD)(g_pdwBlk + i))
				{
					Fail(i, g_pdwBlk[i], (DWORD)(g_pdwBlk + i));
					return;
				}
			break;
		case ST_FILL5:
			for (i = g_dwPos; i < dwEnd; i++)
				g_pdwBlk[i] = 0x55555555;
			break;
		case ST_CHK5:
			for (i = g_dwPos; i < dwEnd; i++)
				if (g_pdwBlk[i] != 0x55555555)
				{
					Fail(i, g_pdwBlk[i], 0x55555555);
					return;
				}
			break;
		case ST_FILLB:
			for (i = g_dwPos; i < dwEnd; i++)
				g_pdwBlk[i] = 0xAAAAAAAA;
			break;
		case ST_CHKB:
			for (i = g_dwPos; i < dwEnd; i++)
				if (g_pdwBlk[i] != 0xAAAAAAAA)
				{
					Fail(i, g_pdwBlk[i], 0xAAAAAAAA);
					return;
				}
			break;
	}

	g_dwPos = dwEnd;
	if (g_dwPos >= g_dwWords) // phase complete -> next
	{
		g_dwPos = 0;
		g_nState++; // ST_FILLA..ST_CHKB..ST_DONE
		if (g_nState == ST_DONE)
			wsprintfW(g_awchMsg, L"PASS - %u MB ok, 0 errors", (unsigned)(g_dwBytes >> 20));
	}
}

static const WCHAR *PhaseName(void)
{
	switch (g_nState)
	{
		case ST_FILLA:
			return L"addr-in-addr: writing";
		case ST_CHKA:
			return L"addr-in-addr: verifying";
		case ST_FILL5:
			return L"pattern 0x55: writing";
		case ST_CHK5:
			return L"pattern 0x55: verifying";
		case ST_FILLB:
			return L"pattern 0xAA: writing";
		case ST_CHKB:
			return L"pattern 0xAA: verifying";
	}
	return L"";
}

// 0..100 across all 6 phases.
static int Percent(void)
{
	int nPh;
	if (g_nState < ST_FILLA)
		return 0;
	if (g_nState > ST_CHKB)
		return 100;
	nPh = g_nState - ST_FILLA; // 0..5
	if (!g_dwWords)
		return 0;
	return (int)(((DWORD)nPh * 1000 + (g_dwPos * 1000 / g_dwWords)) / 60);
}

static void StartTest(void)
{
	if (g_pdwBlk)
	{
		VirtualFree(g_pdwBlk, 0, MEM_RELEASE);
		g_pdwBlk = NULL;
	}
	g_pdwBlk = AllocBig(&g_dwBytes);
	if (!g_pdwBlk)
	{
		g_dwWords = 0;
		g_nState = ST_FAIL;
		lstrcpyW(g_awchMsg, L"VirtualAlloc failed (low RAM)");
		return;
	}
	g_dwWords = g_dwBytes / 4;
	g_dwPos = 0;
	g_nState = ST_FILLA;
	g_awchMsg[0] = 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *w;
	DWORD dwKey;
	int nDrawnOnce = 0;

	w = DCWinOpen(120, 70, CW, CH, L"Memory / MMU Test", ICON_APP);
	if (!w)
	{
		OutputDebugStringW(L"DCWMEM: DCWinOpen failed\r\n");
		return 1;
	}

	for (;;)
	{
		MEMORYSTATUS ms;
		WCHAR awchRam[72], awchRow[72];
		int nBusy, nChanged = 0, nCw = CW, nCh = CH;
		nChanged |= DCWinClientSize(w, &nCw, &nCh); // resize/maximize -> redraw to fit

		while (DCWinPollKey(w, &dwKey))
		{
			if ((dwKey == VK_RETURN || dwKey == VK_SPACE) &&
			    (g_nState == ST_IDLE || g_nState == ST_DONE || g_nState == ST_FAIL))
				StartTest();
			nChanged = 1;
		}

		nBusy = (g_nState >= ST_FILLA && g_nState <= ST_CHKB);
		if (nBusy)
		{
			StepTest();
			nChanged = 1;
		} // animate progress while testing
		if ((g_nState == ST_DONE || g_nState == ST_FAIL) && g_pdwBlk)
		{ // free as soon as the run ends
			VirtualFree(g_pdwBlk, 0, MEM_RELEASE);
			g_pdwBlk = NULL;
		}

		if (nChanged || !nDrawnOnce)
		{
			int nPc = Percent(), nBw = nCw - 24; // progress bar spans the width
			nDrawnOnce = 1;
			DCWinBeginFrame(w);
			DCWinFillBg(w, RGB(192, 192, 192)); // background fills the window

			memset(&ms, 0, sizeof(ms));
			ms.dwLength = sizeof(ms);
			GlobalMemoryStatus(&ms);
			wsprintfW(awchRam, L"Total %u K   Free %u K", (unsigned)(ms.dwTotalPhys / 1024),
			          (unsigned)(ms.dwAvailPhys / 1024));
			DCWinText(w, 10, 8, RGB(0, 0, 0), RGB(192, 192, 192), awchRam);
			DCWinText(w, 10, 24, RGB(0, 0, 96), RGB(192, 192, 192),
			          L"Enter / A  =  run memory + MMU test");

			// progress bar
			DCWinFill(w, 12, 52, nBw, 16, RGB(128, 128, 128));
			if (nPc > 0)
				DCWinFill(w, 12, 52, nBw * nPc / 100, 16, RGB(0, 0, 160));
			wsprintfW(awchRow, L"%s  %d%%", nBusy ? PhaseName() : L"", nPc);
			DCWinText(w, 14, 74, RGB(0, 0, 0), RGB(192, 192, 192), awchRow);

			if (g_pdwBlk)
			{
				wsprintfW(awchRow, L"block %u MB  @ %08X..%08X", (unsigned)(g_dwBytes >> 20),
				          (unsigned)g_pdwBlk, (unsigned)((DWORD)g_pdwBlk + g_dwBytes));
				DCWinText(w, 10, 96, RGB(0, 0, 0), RGB(192, 192, 192), awchRow);
			}

			if (g_awchMsg[0])
			{
				COLORREF c = (g_nState == ST_DONE) ? RGB(0, 110, 0) : RGB(170, 0, 0);
				DCWinText(w, 10, 120, c, RGB(192, 192, 192), g_awchMsg);
				if (g_nState == ST_FAIL && g_dwFailAddr)
				{
					wsprintfW(awchRow, L"@ %08X  got %08X  exp %08X", (unsigned)g_dwFailAddr,
					          (unsigned)g_dwFailGot, (unsigned)g_dwFailExp);
					DCWinText(w, 10, 136, RGB(170, 0, 0), RGB(192, 192, 192), awchRow);
					DCWinText(w, 10, 152, RGB(80, 0, 0), RGB(192, 192, 192),
					          L"(aliased/mirrored RAM - not real 32MB)");
				}
			}
			DCWinEndFrame(w);
		}

		if (DCWinShouldClose(w))
			break;
		Sleep(nBusy ? 8 : 30);
	}

	if (g_pdwBlk)
		VirtualFree(g_pdwBlk, 0, MEM_RELEASE);
	DCWinClose(w);
	return 0;
}
