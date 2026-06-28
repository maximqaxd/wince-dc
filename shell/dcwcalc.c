//
// dcwcalc.c - a real windowed Calculator (its own DCWin process): a right-aligned
// display + a 4x5 button grid (digits / operators / C, CE, backspace, +/-, .),
// immediate-execution arithmetic. Driven by the keyboard (digit + operator keys),
// and by arrow keys + Enter/Space to move and press the highlighted button (so the
// controller D-pad works too).
//
#include "dcwlib.h"

#ifndef VK_OEM_PLUS                 // CE 2.12 headers may not define the OEM VKs
#define VK_OEM_PLUS   0xBB
#define VK_OEM_MINUS  0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2      0xBF
#endif

#define CW 200
#define CH 250
#define MAXLEN 14                   // display entry cap

static WCHAR  g_disp[24] = L"0";
static double g_acc   = 0.0;        // accumulator
static int    g_pend  = 0;          // pending op: 0 none, else '+','-','*','/'
static int    g_fresh = 1;          // next digit starts a new number
static int    g_err   = 0;          // error latched (e.g. divide by zero)
static int    g_sel   = 4;          // highlighted button (start on '7')

// 4 columns x 5 rows. Indices drive Press().
static const WCHAR *BTN[20] = {
    L"C",  L"CE", L"<-", L"/",
    L"7",  L"8",  L"9",  L"*",
    L"4",  L"5",  L"6",  L"-",
    L"1",  L"2",  L"3",  L"+",
    L"+/-",L"0",  L".",  L"="
};

// ---- numeric core ------------------------------------------------------------------
static double ParseNum(const WCHAR *s)
{
    double v = 0.0, scale = 1.0; int i = 0, neg = 0, frac = 0;
    if (s[i] == L'-') { neg = 1; i++; }
    for (; s[i]; i++)
    {
        if (s[i] == L'.') { frac = 1; continue; }
        if (s[i] < L'0' || s[i] > L'9') break;
        if (!frac) v = v * 10.0 + (double)(s[i] - L'0');
        else { scale *= 0.1; v += (double)(s[i] - L'0') * scale; }
    }
    return neg ? -v : v;
}

static void FmtNum(double v, WCHAR *out)
{
    WCHAR ipbuf[16], rev[16], f[8];
    int neg = 0, n = 0, k, fdig;
    unsigned long ip, frac, t;

    if (v < 0.0) { neg = 1; v = -v; }
    if (v >= 1.0e9) { lstrcpyW(out, L"Overflow"); return; }
    ip = (unsigned long)v;
    frac = (unsigned long)((v - (double)ip) * 1000000.0 + 0.5);   // 6 decimals, rounded
    if (frac >= 1000000UL) { frac -= 1000000UL; ip++; }

    t = ip;
    if (t == 0) rev[n++] = L'0';
    while (t) { rev[n++] = (WCHAR)(L'0' + t % 10); t /= 10; }
    for (k = 0; k < n; k++) ipbuf[k] = rev[n - 1 - k];
    ipbuf[n] = 0;

    for (k = 5; k >= 0; k--) { f[k] = (WCHAR)(L'0' + frac % 10); frac /= 10; }
    f[6] = 0;
    fdig = 6; while (fdig > 0 && f[fdig - 1] == L'0') fdig--; f[fdig] = 0;

    out[0] = 0;
    if (neg && (ip != 0 || fdig > 0)) lstrcatW(out, L"-");
    lstrcatW(out, ipbuf);
    if (fdig > 0) { lstrcatW(out, L"."); lstrcatW(out, f); }
}

static double Apply(double a, int op, double b)
{
    switch (op)
    {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/': if (b == 0.0) { g_err = 1; return 0.0; } return a / b;
    }
    return b;
}

// ---- editing ops -------------------------------------------------------------------
static void ClearAll(void) { lstrcpyW(g_disp, L"0"); g_acc = 0.0; g_pend = 0; g_fresh = 1; g_err = 0; }

static void InputDigit(WCHAR d)
{
    int n;
    if (g_err) ClearAll();
    if (g_fresh) { g_disp[0] = d; g_disp[1] = 0; g_fresh = 0; return; }
    n = lstrlenW(g_disp);
    if (n == 1 && g_disp[0] == L'0') { g_disp[0] = d; return; }   // replace lone leading zero
    if (n < MAXLEN) { g_disp[n] = d; g_disp[n + 1] = 0; }
}

static void InputDot(void)
{
    int i, n;
    if (g_err) ClearAll();
    if (g_fresh) { lstrcpyW(g_disp, L"0."); g_fresh = 0; return; }
    for (i = 0; g_disp[i]; i++) if (g_disp[i] == L'.') return;    // one dot only
    n = lstrlenW(g_disp);
    if (n < MAXLEN) { g_disp[n] = L'.'; g_disp[n + 1] = 0; }
}

static void Backspace(void)
{
    int n;
    if (g_err) { ClearAll(); return; }
    n = lstrlenW(g_disp);
    if (n <= 1 || (n == 2 && g_disp[0] == L'-')) { lstrcpyW(g_disp, L"0"); g_fresh = 1; return; }
    g_disp[n - 1] = 0;
}

static void Negate(void)
{
    int i, n;
    if (g_err) return;
    if (g_disp[0] == L'-') { for (i = 0; g_disp[i]; i++) g_disp[i] = g_disp[i + 1]; }
    else if (!(g_disp[0] == L'0' && g_disp[1] == 0))
    {
        n = lstrlenW(g_disp);
        for (i = n; i >= 0; i--) g_disp[i + 1] = g_disp[i];
        g_disp[0] = L'-';
    }
}

static void DoOp(int op)
{
    double cur;
    if (g_err) return;
    cur = ParseNum(g_disp);
    if (g_pend && !g_fresh)
    {
        g_acc = Apply(g_acc, g_pend, cur);
        if (g_err) { lstrcpyW(g_disp, L"Error"); return; }
        FmtNum(g_acc, g_disp);
    }
    else g_acc = cur;
    g_pend = op; g_fresh = 1;
}

static void Equals(void)
{
    double cur;
    if (g_err || !g_pend) return;
    cur = ParseNum(g_disp);
    g_acc = Apply(g_acc, g_pend, cur);
    if (g_err) { lstrcpyW(g_disp, L"Error"); g_pend = 0; return; }
    FmtNum(g_acc, g_disp);
    g_pend = 0; g_fresh = 1;
}

static void Press(int idx)
{
    switch (idx)
    {
    case 0:  ClearAll(); break;                                  // C
    case 1:  if (g_err) ClearAll(); else { lstrcpyW(g_disp, L"0"); g_fresh = 1; } break;  // CE
    case 2:  Backspace(); break;                                 // <-
    case 3:  DoOp('/'); break;
    case 4:  InputDigit(L'7'); break;
    case 5:  InputDigit(L'8'); break;
    case 6:  InputDigit(L'9'); break;
    case 7:  DoOp('*'); break;
    case 8:  InputDigit(L'4'); break;
    case 9:  InputDigit(L'5'); break;
    case 10: InputDigit(L'6'); break;
    case 11: DoOp('-'); break;
    case 12: InputDigit(L'1'); break;
    case 13: InputDigit(L'2'); break;
    case 14: InputDigit(L'3'); break;
    case 15: DoOp('+'); break;
    case 16: Negate(); break;                                    // +/-
    case 17: InputDigit(L'0'); break;
    case 18: InputDot(); break;
    case 19: Equals(); break;
    }
}

static int HandleKey(DWORD k)
{
    if (k >= L'0' && k <= L'9')           { InputDigit((WCHAR)k); return 1; }
    if (k >= VK_NUMPAD0 && k <= VK_NUMPAD9) { InputDigit((WCHAR)(L'0' + (k - VK_NUMPAD0))); return 1; }
    switch (k)
    {
    case VK_ADD:                       DoOp('+');     return 1;
    case VK_SUBTRACT: case VK_OEM_MINUS: DoOp('-');   return 1;
    case VK_MULTIPLY:                  DoOp('*');     return 1;
    case VK_DIVIDE:   case VK_OEM_2:   DoOp('/');     return 1;
    case VK_DECIMAL:  case VK_OEM_PERIOD: InputDot(); return 1;
    case VK_OEM_PLUS:                  Equals();      return 1;  // the '=' key (unshifted)
    case VK_BACK:                      Backspace();   return 1;
    case 'C':                          ClearAll();    return 1;
    case VK_RETURN: case VK_SPACE:     Press(g_sel);  return 1;  // press highlighted button
    case VK_UP:    if (g_sel >= 4)  g_sel -= 4; return 1;
    case VK_DOWN:  if (g_sel < 16)  g_sel += 4; return 1;
    case VK_LEFT:  if (g_sel % 4)   g_sel--;    return 1;
    case VK_RIGHT: if (g_sel % 4 != 3) g_sel++; return 1;
    }
    return 0;
}

// ---- drawing -----------------------------------------------------------------------
static void Draw(DCWin *w)
{
    const COLORREF BG = RGB(212, 208, 200), DISPBG = RGB(255, 255, 255), DISPFG = RGB(0, 0, 0);
    const COLORREF BTNBG = RGB(236, 233, 227), ACTBG = RGB(226, 222, 214), OPBG = RGB(214, 224, 240);
    const COLORREF OPFG = RGB(0, 0, 128), SELBG = RGB(0, 84, 170), SELFG = RGB(255, 255, 255), FG = RGB(0, 0, 0);
    int cw = CW, ch = CH, dispH = 34, gap = 4, gx, gy, gw, gh, cellW, cellH, i, len, tx;

    DCWinClientSize(w, &cw, &ch);
    DCWinBeginFrame(w);
    DCWinFillBg(w, BG);

    // display, right-aligned
    DCWinFill(w, 6, 6, cw - 12, dispH, DISPBG);
    len = lstrlenW(g_disp); tx = cw - 12 - len * 7; if (tx < 10) tx = 10;
    DCWinText(w, tx, 6 + (dispH - 12) / 2, DISPFG, DISPBG, g_disp);

    // button grid
    gx = 6; gy = 6 + dispH + 6; gw = cw - 12; gh = ch - gy - 6;
    cellW = (gw - 3 * gap) / 4; cellH = (gh - 4 * gap) / 5;
    if (cellW < 8) cellW = 8; if (cellH < 8) cellH = 8;
    for (i = 0; i < 20; i++)
    {
        int isOp  = (i == 3 || i == 7 || i == 11 || i == 15 || i == 19);  // / * - + =
        int isAct = (i == 0 || i == 1 || i == 2 || i == 16);              // C CE <- +/-
        COLORREF bg = (i == g_sel) ? SELBG : (isOp ? OPBG : (isAct ? ACTBG : BTNBG));
        COLORREF fg = (i == g_sel) ? SELFG : (isOp ? OPFG : FG);
        int r = i / 4, c = i % 4;
        int bx = gx + c * (cellW + gap), by = gy + r * (cellH + gap), bl, btx;
        DCWinFill(w, bx, by, cellW, cellH, bg);
        bl = lstrlenW(BTN[i]); btx = bx + (cellW - bl * 7) / 2; if (btx < bx + 2) btx = bx + 2;
        DCWinText(w, btx, by + (cellH - 12) / 2, fg, bg, BTN[i]);
    }

    DCWinEndFrame(w);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key;

    w = DCWinOpen(210, 90, CW, CH, L"Calculator", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWCALC: DCWinOpen failed\r\n"); return 1; }
    Draw(w);

    for (;;)
    {
        int changed = 0;
        while (DCWinPollKey(w, &key)) { if (HandleKey(key)) changed = 1; }
        if (DCWinResized(w)) changed = 1;
        if (changed) Draw(w);
        if (DCWinShouldClose(w)) break;
        Sleep(20);
    }

    DCWinClose(w);
    return 0;
}
