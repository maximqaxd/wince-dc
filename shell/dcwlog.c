//
// dcwlog.c - the "System Log" window: renders the DCSYSLOG ring (see syslog.h) as a
// green-on-black console. Auto-follows the tail; Up/Down scroll back through history.
//
#include "dcwlib.h"
#include "syslog.h"

#define LW  560 // wide enough for the longest netif/dns/tcp lines (~59 chars) without clipping
#define LH  300
#define ROW 13

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *w;
	SysLogShared *sl;
	DWORD dwKey;
	LONG lLastHead = -1;
	int nScroll = 0;

	w = DCWinOpen(60, 60, LW, LH, L"System Log", ICON_APP);
	if (!w)
	{
		OutputDebugStringW(L"DCWLOG: DCWinOpen failed\r\n");
		return 1;
	}
	sl = SysLogMap(0); // map (creating if needed)

	for (;;)
	{
		int nChanged = 0, nCw = LW, nCh = LH, nVis, i, y;
		LONG lHead = sl ? sl->head : 0;

		DCWinClientSize(w, &nCw, &nCh);
		while (DCWinPollKey(w, &dwKey))
		{
			if (dwKey == VK_UP)
			{
				nScroll++;
				nChanged = 1;
			}
			else if (dwKey == VK_DOWN && nScroll > 0)
			{
				nScroll--;
				nChanged = 1;
			}
			else if (dwKey == VK_NEXT || dwKey == VK_HOME)
			{
				nScroll = 0;
				nChanged = 1;
			} // jump to tail
		}
		if (lHead != lLastHead)
		{
			lLastHead = lHead;
			if (nScroll == 0)
				nChanged = 1;
		} // new lines (following tail)
		if (DCWinResized(w))
			nChanged = 1;

		if (nChanged)
		{
			nVis = (nCh - 2) / ROW;
			if (nVis < 1)
				nVis = 1;
			if (nVis > 44)
				nVis = 44; // command-budget cap
			if (nScroll > lHead - nVis)
				nScroll = lHead > nVis ? (int)(lHead - nVis) : 0;
			DCWinBeginFrame(w);
			DCWinFillBg(w, RGB(0, 0, 0));
			for (i = 0; i < nVis; i++)
			{
				LONG lIdx = lHead - nVis - nScroll + i; // bottom line = newest-scroll
				if (lIdx < 0 || (lHead - lIdx) > SYSLOG_LINES)
					continue; // before start / rolled off
				y = 1 + i * ROW;
				DCWinText(w, 4, y, RGB(0, 255, 80), RGB(0, 0, 0),
				          sl->line[(DWORD)lIdx % SYSLOG_LINES]);
			}
			DCWinEndFrame(w);
		}
		if (DCWinShouldClose(w))
			break;
		Sleep(120);
	}

	DCWinClose(w);
	return 0;
}
