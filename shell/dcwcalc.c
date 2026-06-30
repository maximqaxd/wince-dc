//
// dcwcalc.c - a real windowed Calculator (its own DCWin process): a right-aligned
// display + a 4x5 button grid (digits / operators / C, CE, backspace, +/-, .),
// immediate-execution arithmetic. Driven by the keyboard (digit + operator keys),
// and by arrow keys + Enter/Space to move and press the highlighted button (so the
// controller D-pad works too).
//
#include "dcwlib.h"

#ifndef VK_OEM_PLUS // CE 2.12 headers may not define the OEM VKs
#define VK_OEM_PLUS   0xBB
#define VK_OEM_MINUS  0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2      0xBF
#endif

#define CW     200
#define CH     250
#define MAXLEN 14 // display entry cap

static WCHAR g_aszDisp[24] = L"0";
static double g_dAcc = 0.0; // accumulator
static int g_nPend = 0;     // pending op: 0 none, else '+','-','*','/'
static int g_bFresh = 1;    // next digit starts a new number
static int g_bErr = 0;      // error latched (e.g. divide by zero)
static int g_nSel = 4;      // highlighted button (start on '7')

// 4 columns x 5 rows. Indices drive Press().
static const WCHAR *BTN[20] = {L"C", L"CE", L"<-", L"/", L"7", L"8", L"9",   L"*", L"4", L"5",
                               L"6", L"-",  L"1",  L"2", L"3", L"+", L"+/-", L"0", L".", L"="};

// ---- numeric core ------------------------------------------------------------------
static double ParseNum(const WCHAR *psz)
{
	double dVal = 0.0, dScale = 1.0;
	int i = 0, bNeg = 0, bFrac = 0;
	if (psz[i] == L'-')
	{
		bNeg = 1;
		i++;
	}
	for (; psz[i]; i++)
	{
		if (psz[i] == L'.')
		{
			bFrac = 1;
			continue;
		}
		if (psz[i] < L'0' || psz[i] > L'9')
			break;
		if (!bFrac)
			dVal = dVal * 10.0 + (double)(psz[i] - L'0');
		else
		{
			dScale *= 0.1;
			dVal += (double)(psz[i] - L'0') * dScale;
		}
	}
	return bNeg ? -dVal : dVal;
}

static void FmtNum(double dVal, WCHAR *pszOut)
{
	WCHAR aszIp[16], aszRev[16], aszF[8];
	int bNeg = 0, n = 0, k, nFdig;
	unsigned long dwIp, dwFrac, dwT;

	if (dVal < 0.0)
	{
		bNeg = 1;
		dVal = -dVal;
	}
	if (dVal >= 1.0e9)
	{
		lstrcpyW(pszOut, L"Overflow");
		return;
	}
	dwIp = (unsigned long)dVal;
	dwFrac = (unsigned long)((dVal - (double)dwIp) * 1000000.0 + 0.5); // 6 decimals, rounded
	if (dwFrac >= 1000000UL)
	{
		dwFrac -= 1000000UL;
		dwIp++;
	}

	dwT = dwIp;
	if (dwT == 0)
		aszRev[n++] = L'0';
	while (dwT)
	{
		aszRev[n++] = (WCHAR)(L'0' + dwT % 10);
		dwT /= 10;
	}
	for (k = 0; k < n; k++)
		aszIp[k] = aszRev[n - 1 - k];
	aszIp[n] = 0;

	for (k = 5; k >= 0; k--)
	{
		aszF[k] = (WCHAR)(L'0' + dwFrac % 10);
		dwFrac /= 10;
	}
	aszF[6] = 0;
	nFdig = 6;
	while (nFdig > 0 && aszF[nFdig - 1] == L'0')
		nFdig--;
	aszF[nFdig] = 0;

	pszOut[0] = 0;
	if (bNeg && (dwIp != 0 || nFdig > 0))
		lstrcatW(pszOut, L"-");
	lstrcatW(pszOut, aszIp);
	if (nFdig > 0)
	{
		lstrcatW(pszOut, L".");
		lstrcatW(pszOut, aszF);
	}
}

static double Apply(double dA, int nOp, double dB)
{
	switch (nOp)
	{
		case '+':
			return dA + dB;
		case '-':
			return dA - dB;
		case '*':
			return dA * dB;
		case '/':
			if (dB == 0.0)
			{
				g_bErr = 1;
				return 0.0;
			}
			return dA / dB;
	}
	return dB;
}

// ---- editing ops -------------------------------------------------------------------
static void ClearAll(void)
{
	lstrcpyW(g_aszDisp, L"0");
	g_dAcc = 0.0;
	g_nPend = 0;
	g_bFresh = 1;
	g_bErr = 0;
}

static void InputDigit(WCHAR wchDigit)
{
	int n;
	if (g_bErr)
		ClearAll();
	if (g_bFresh)
	{
		g_aszDisp[0] = wchDigit;
		g_aszDisp[1] = 0;
		g_bFresh = 0;
		return;
	}
	n = lstrlenW(g_aszDisp);
	if (n == 1 && g_aszDisp[0] == L'0')
	{
		g_aszDisp[0] = wchDigit;
		return;
	} // replace lone leading zero
	if (n < MAXLEN)
	{
		g_aszDisp[n] = wchDigit;
		g_aszDisp[n + 1] = 0;
	}
}

static void InputDot(void)
{
	int i, n;
	if (g_bErr)
		ClearAll();
	if (g_bFresh)
	{
		lstrcpyW(g_aszDisp, L"0.");
		g_bFresh = 0;
		return;
	}
	for (i = 0; g_aszDisp[i]; i++)
		if (g_aszDisp[i] == L'.')
			return; // one dot only
	n = lstrlenW(g_aszDisp);
	if (n < MAXLEN)
	{
		g_aszDisp[n] = L'.';
		g_aszDisp[n + 1] = 0;
	}
}

static void Backspace(void)
{
	int n;
	if (g_bErr)
	{
		ClearAll();
		return;
	}
	n = lstrlenW(g_aszDisp);
	if (n <= 1 || (n == 2 && g_aszDisp[0] == L'-'))
	{
		lstrcpyW(g_aszDisp, L"0");
		g_bFresh = 1;
		return;
	}
	g_aszDisp[n - 1] = 0;
}

static void Negate(void)
{
	int i, n;
	if (g_bErr)
		return;
	if (g_aszDisp[0] == L'-')
	{
		for (i = 0; g_aszDisp[i]; i++)
			g_aszDisp[i] = g_aszDisp[i + 1];
	}
	else if (!(g_aszDisp[0] == L'0' && g_aszDisp[1] == 0))
	{
		n = lstrlenW(g_aszDisp);
		for (i = n; i >= 0; i--)
			g_aszDisp[i + 1] = g_aszDisp[i];
		g_aszDisp[0] = L'-';
	}
}

static void DoOp(int nOp)
{
	double dCur;
	if (g_bErr)
		return;
	dCur = ParseNum(g_aszDisp);
	if (g_nPend && !g_bFresh)
	{
		g_dAcc = Apply(g_dAcc, g_nPend, dCur);
		if (g_bErr)
		{
			lstrcpyW(g_aszDisp, L"Error");
			return;
		}
		FmtNum(g_dAcc, g_aszDisp);
	}
	else
		g_dAcc = dCur;
	g_nPend = nOp;
	g_bFresh = 1;
}

static void Equals(void)
{
	double dCur;
	if (g_bErr || !g_nPend)
		return;
	dCur = ParseNum(g_aszDisp);
	g_dAcc = Apply(g_dAcc, g_nPend, dCur);
	if (g_bErr)
	{
		lstrcpyW(g_aszDisp, L"Error");
		g_nPend = 0;
		return;
	}
	FmtNum(g_dAcc, g_aszDisp);
	g_nPend = 0;
	g_bFresh = 1;
}

static void Press(int nIdx)
{
	switch (nIdx)
	{
		case 0:
			ClearAll();
			break; // C
		case 1:
			if (g_bErr)
				ClearAll();
			else
			{
				lstrcpyW(g_aszDisp, L"0");
				g_bFresh = 1;
			}
			break; // CE
		case 2:
			Backspace();
			break; // <-
		case 3:
			DoOp('/');
			break;
		case 4:
			InputDigit(L'7');
			break;
		case 5:
			InputDigit(L'8');
			break;
		case 6:
			InputDigit(L'9');
			break;
		case 7:
			DoOp('*');
			break;
		case 8:
			InputDigit(L'4');
			break;
		case 9:
			InputDigit(L'5');
			break;
		case 10:
			InputDigit(L'6');
			break;
		case 11:
			DoOp('-');
			break;
		case 12:
			InputDigit(L'1');
			break;
		case 13:
			InputDigit(L'2');
			break;
		case 14:
			InputDigit(L'3');
			break;
		case 15:
			DoOp('+');
			break;
		case 16:
			Negate();
			break; // +/-
		case 17:
			InputDigit(L'0');
			break;
		case 18:
			InputDot();
			break;
		case 19:
			Equals();
			break;
	}
}

static int HandleKey(DWORD dwKey)
{
	if (dwKey >= L'0' && dwKey <= L'9')
	{
		InputDigit((WCHAR)dwKey);
		return 1;
	}
	if (dwKey >= VK_NUMPAD0 && dwKey <= VK_NUMPAD9)
	{
		InputDigit((WCHAR)(L'0' + (dwKey - VK_NUMPAD0)));
		return 1;
	}
	switch (dwKey)
	{
		case VK_ADD:
			DoOp('+');
			return 1;
		case VK_SUBTRACT:
		case VK_OEM_MINUS:
			DoOp('-');
			return 1;
		case VK_MULTIPLY:
			DoOp('*');
			return 1;
		case VK_DIVIDE:
		case VK_OEM_2:
			DoOp('/');
			return 1;
		case VK_DECIMAL:
		case VK_OEM_PERIOD:
			InputDot();
			return 1;
		case VK_OEM_PLUS:
			Equals();
			return 1; // the '=' key (unshifted)
		case VK_BACK:
			Backspace();
			return 1;
		case 'C':
			ClearAll();
			return 1;
		case VK_RETURN:
		case VK_SPACE:
			Press(g_nSel);
			return 1; // press highlighted button
		case VK_UP:
			if (g_nSel >= 4)
				g_nSel -= 4;
			return 1;
		case VK_DOWN:
			if (g_nSel < 16)
				g_nSel += 4;
			return 1;
		case VK_LEFT:
			if (g_nSel % 4)
				g_nSel--;
			return 1;
		case VK_RIGHT:
			if (g_nSel % 4 != 3)
				g_nSel++;
			return 1;
	}
	return 0;
}

// ---- drawing -----------------------------------------------------------------------
static void Draw(DCWin *pWin)
{
	const COLORREF BG = RGB(212, 208, 200), DISPBG = RGB(255, 255, 255), DISPFG = RGB(0, 0, 0);
	const COLORREF BTNBG = RGB(236, 233, 227), ACTBG = RGB(226, 222, 214),
	               OPBG = RGB(214, 224, 240);
	const COLORREF OPFG = RGB(0, 0, 128), SELBG = RGB(0, 84, 170), SELFG = RGB(255, 255, 255),
	               FG = RGB(0, 0, 0);
	int nCw = CW, nCh = CH, nDispH = 34, nGap = 4, nGx, nGy, nGw, nGh, nCellW, nCellH, i, nLen, nTx;

	DCWinClientSize(pWin, &nCw, &nCh);
	DCWinBeginFrame(pWin);
	DCWinFillBg(pWin, BG);

	// display, right-aligned
	DCWinFill(pWin, 6, 6, nCw - 12, nDispH, DISPBG);
	nLen = lstrlenW(g_aszDisp);
	nTx = nCw - 12 - nLen * 7;
	if (nTx < 10)
		nTx = 10;
	DCWinText(pWin, nTx, 6 + (nDispH - 12) / 2, DISPFG, DISPBG, g_aszDisp);

	// button grid
	nGx = 6;
	nGy = 6 + nDispH + 6;
	nGw = nCw - 12;
	nGh = nCh - nGy - 6;
	nCellW = (nGw - 3 * nGap) / 4;
	nCellH = (nGh - 4 * nGap) / 5;
	if (nCellW < 8)
		nCellW = 8;
	if (nCellH < 8)
		nCellH = 8;
	for (i = 0; i < 20; i++)
	{
		int bIsOp = (i == 3 || i == 7 || i == 11 || i == 15 || i == 19); // / * - + =
		int bIsAct = (i == 0 || i == 1 || i == 2 || i == 16);            // C CE <- +/-
		COLORREF bg = (i == g_nSel) ? SELBG : (bIsOp ? OPBG : (bIsAct ? ACTBG : BTNBG));
		COLORREF fg = (i == g_nSel) ? SELFG : (bIsOp ? OPFG : FG);
		int nR = i / 4, nC = i % 4;
		int nBx = nGx + nC * (nCellW + nGap), nBy = nGy + nR * (nCellH + nGap), nBl, nBtx;
		DCWinFill(pWin, nBx, nBy, nCellW, nCellH, bg);
		nBl = lstrlenW(BTN[i]);
		nBtx = nBx + (nCellW - nBl * 7) / 2;
		if (nBtx < nBx + 2)
			nBtx = nBx + 2;
		DCWinText(pWin, nBtx, nBy + (nCellH - 12) / 2, fg, bg, BTN[i]);
	}

	DCWinEndFrame(pWin);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *pWin;
	DWORD dwKey;

	pWin = DCWinOpen(210, 90, CW, CH, L"Calculator", ICON_APP);
	if (!pWin)
	{
		OutputDebugStringW(L"DCWCALC: DCWinOpen failed\r\n");
		return 1;
	}
	Draw(pWin);

	for (;;)
	{
		int bChanged = 0;
		while (DCWinPollKey(pWin, &dwKey))
		{
			if (HandleKey(dwKey))
				bChanged = 1;
		}
		if (DCWinResized(pWin))
			bChanged = 1;
		if (bChanged)
			Draw(pWin);
		if (DCWinShouldClose(pWin))
			break;
		Sleep(20);
	}

	DCWinClose(pWin);
	return 0;
}
