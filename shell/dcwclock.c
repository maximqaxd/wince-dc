//
// dcwclock.c - a live digital clock (its own DCWin process): a 7-segment HH:MM with a
// blinking colon, a seconds bar, and the full date. Redraws once a second.
//
#include "dcwlib.h"

#define CW 240
#define CH 150

// 7-segment masks for 0-9.  bit: a=1 b=2 c=4 d=8 e=16 f=32 g=64
//   aaa
//  f   b
//   ggg
//  e   c
//   ddd
static const int SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

static const WCHAR *DOW[7] = {L"Sunday",   L"Monday", L"Tuesday", L"Wednesday",
                              L"Thursday", L"Friday", L"Saturday"};
static const WCHAR *MON[12] = {L"January",   L"February", L"March",    L"April",
                               L"May",       L"June",     L"July",     L"August",
                               L"September", L"October",  L"November", L"December"};

// Draw one 7-segment digit in a (nDw x nDh) cell at (x,y); only lit segments are filled.
static void Draw7(DCWin *pWin, int x, int y, int nDw, int nDh, int nDigit, COLORREF crOn)
{
	int nT = nDw / 6, nVlen, nMask;
	if (nT < 3)
		nT = 3;
	nVlen = (nDh - 3 * nT) / 2;
	if (nVlen < 1)
		nVlen = 1;
	nMask = (nDigit >= 0 && nDigit <= 9) ? SEG[nDigit] : 0;
	if (nMask & 0x01)
		DCWinFill(pWin, x + nT, y, nDw - 2 * nT, nT, crOn); // a
	if (nMask & 0x20)
		DCWinFill(pWin, x, y + nT, nT, nVlen, crOn); // f
	if (nMask & 0x02)
		DCWinFill(pWin, x + nDw - nT, y + nT, nT, nVlen, crOn); // b
	if (nMask & 0x40)
		DCWinFill(pWin, x + nT, y + nT + nVlen, nDw - 2 * nT, nT, crOn); // g
	if (nMask & 0x10)
		DCWinFill(pWin, x, y + 2 * nT + nVlen, nT, nVlen, crOn); // e
	if (nMask & 0x04)
		DCWinFill(pWin, x + nDw - nT, y + 2 * nT + nVlen, nT, nVlen, crOn); // c
	if (nMask & 0x08)
		DCWinFill(pWin, x + nT, y + nDh - nT, nDw - 2 * nT, nT, crOn); // d
}

static void DrawClock(DCWin *pWin, SYSTEMTIME *pst)
{
	const COLORREF BG = RGB(8, 10, 18), ON = RGB(0, 240, 140);
	const COLORREF BARBG = RGB(24, 30, 44), BARFG = RGB(0, 180, 220), DATEFG = RGB(200, 210, 230);
	int nCw = CW, nCh = CH, nDh, nDw, nGap, nColw, nTotw, nX0, nY0, x, nBy, nBh, nLen, nTx;
	int nHh = pst->wHour, nMm = pst->wMinute, nSs = pst->wSecond;
	WCHAR aszLine[64];

	DCWinClientSize(pWin, &nCw, &nCh);
	nDh = (nCh * 42) / 100;
	if (nDh < 24)
		nDh = 24;
	nDw = (nDh * 60) / 100;
	if (nDw < 14)
		nDw = 14;
	nGap = nDw / 5;
	if (nGap < 2)
		nGap = 2;
	nColw = nDw / 2;
	nTotw = 4 * nDw + 3 * nGap + nColw + 2 * nGap;
	nX0 = (nCw - nTotw) / 2;
	if (nX0 < 4)
		nX0 = 4;
	nY0 = (nCh > 120) ? 12 : 6;

	DCWinBeginFrame(pWin);
	DCWinFillBg(pWin, BG);

	x = nX0;
	Draw7(pWin, x, nY0, nDw, nDh, nHh / 10, ON);
	x += nDw + nGap;
	Draw7(pWin, x, nY0, nDw, nDh, nHh % 10, ON);
	x += nDw + nGap;
	if (nSs & 1) // blinking colon (on for odd seconds)
	{
		int nD = nDw / 6;
		if (nD < 3)
			nD = 3;
		DCWinFill(pWin, x + (nColw - nD) / 2, nY0 + nDh / 3 - nD / 2, nD, nD, ON);
		DCWinFill(pWin, x + (nColw - nD) / 2, nY0 + 2 * nDh / 3 - nD / 2, nD, nD, ON);
	}
	x += nColw + 2 * nGap;
	Draw7(pWin, x, nY0, nDw, nDh, nMm / 10, ON);
	x += nDw + nGap;
	Draw7(pWin, x, nY0, nDw, nDh, nMm % 10, ON);

	// seconds bar (0..59 -> 0..full)
	nBy = nY0 + nDh + 10;
	nBh = 6;
	if (nBy + nBh < nCh - 22)
	{
		DCWinFill(pWin, nX0, nBy, nTotw, nBh, BARBG);
		DCWinFill(pWin, nX0, nBy, (nTotw * nSs) / 59, nBh, BARFG);
	}

	// date line, centred
	wsprintfW(aszLine, L"%s, %s %d, %d", DOW[pst->wDayOfWeek % 7], MON[(pst->wMonth + 11) % 12],
	          pst->wDay, pst->wYear);
	nLen = lstrlenW(aszLine);
	nTx = (nCw - nLen * 6) / 2;
	if (nTx < 2)
		nTx = 2;
	DCWinText(pWin, nTx, nCh - 18, DATEFG, BG, aszLine);

	DCWinEndFrame(pWin);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *pWin;
	SYSTEMTIME st;
	DWORD dwKey;
	int nLastSec = -1;

	pWin = DCWinOpen(70, 300, CW, CH, L"Clock", ICON_CLOCK);
	if (!pWin)
	{
		OutputDebugStringW(L"DCWCLOCK: DCWinOpen failed\r\n");
		return 1;
	}

	for (;;)
	{
		int bChanged = 0;
		GetLocalTime(&st);
		if ((int)st.wSecond != nLastSec)
		{
			nLastSec = (int)st.wSecond;
			bChanged = 1;
		}
		if (DCWinResized(pWin))
			bChanged = 1;
		if (bChanged)
			DrawClock(pWin, &st);

		while (DCWinPollKey(pWin, &dwKey))
		{ /* clock ignores keys */
		}
		if (DCWinShouldClose(pWin))
			break;
		Sleep(100);
	}

	DCWinClose(pWin);
	return 0;
}
