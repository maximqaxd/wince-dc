/*
 * dcshell.c - minimal desktop shell / launcher for Dreamcast Windows CE 2.12.
 *
 * Full-screen window that lists *.exe in \Windows and \, lets you pick one with
 * Up/Down (or tap) and launches it with CreateProcess (= the kernel's existing
 * multitasking). Drawn entirely with the DC GDI *subset*: the stock coredll has
 * NO FillRect/Rectangle/pens/brushes, so rectangles are filled via the standard
 * ExtTextOutW + ETO_OPAQUE idiom (fills the clip rect with the current bg color).
 *
 * Build: shell\build-dcshell.bat  (SH-4, links the DC SDK coredll.lib).
 * Install: add to image .bib + a HKLM\init LaunchNN entry (see shell\README.md).
 */
#include <windows.h>

#define MAX_APPS 128
#define ROW_H    20
#define LEFT     16
#define TOP      44

static WCHAR g_apps[MAX_APPS][MAX_PATH];
static int   g_count = 0;
static int   g_sel   = 0;

static void AddExe(const WCHAR *dir)
{
    WCHAR pat[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE h;

    wsprintfW(pat, L"%s\\*.exe", dir);
    h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (g_count >= MAX_APPS) break;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        wsprintfW(g_apps[g_count++], L"%s\\%s", dir, fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static void ScanApps(void)
{
    g_count = 0;
    AddExe(L"\\Windows");
    AddExe(L"\\");
}

static void Launch(int i)
{
    PROCESS_INFORMATION pi;
    if (i < 0 || i >= g_count) return;
    if (CreateProcessW(g_apps[i], NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

/* Fill a rect with the current bg color (no FillRect in the DC GDI subset). */
static void FillBox(HDC hdc, const RECT *r)
{
    ExtTextOutW(hdc, r->left, r->top, ETO_OPAQUE, r, L"", 0, NULL);
}

static void Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc, row;
    SYSTEMTIME st;
    WCHAR clk[32];
    int i, y;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);

    /* background */
    SetBkColor(hdc, RGB(0, 0, 64));
    FillBox(hdc, &rc);

    /* title bar + clock */
    SetTextColor(hdc, RGB(255, 255, 255));
    ExtTextOutW(hdc, LEFT, 10, 0, NULL, L"Dreamcast Windows CE \x2014 Shell", 28, NULL);
    GetLocalTime(&st);
    wsprintfW(clk, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    ExtTextOutW(hdc, rc.right - 84, 10, 0, NULL, clk, lstrlenW(clk), NULL);

    /* app list */
    for (i = 0; i < g_count; i++) {
        y = TOP + i * ROW_H;
        SetRect(&row, LEFT - 6, y, rc.right - 8, y + ROW_H);
        if (i == g_sel) { SetBkColor(hdc, RGB(0, 0, 200)); SetTextColor(hdc, RGB(255, 255, 0)); }
        else            { SetBkColor(hdc, RGB(0, 0, 64));  SetTextColor(hdc, RGB(210, 210, 210)); }
        ExtTextOutW(hdc, LEFT, y + 2, ETO_OPAQUE, &row, g_apps[i], lstrlenW(g_apps[i]), NULL);
    }
    if (g_count == 0) {
        SetBkColor(hdc, RGB(0, 0, 64)); SetTextColor(hdc, RGB(255, 128, 128));
        ExtTextOutW(hdc, LEFT, TOP, 0, NULL, L"(no .exe found in \\ or \\Windows)", 31, NULL);
    }

    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:   ScanApps(); SetTimer(hwnd, 1, 1000, NULL); return 0;
    case WM_TIMER:    InvalidateRect(hwnd, NULL, FALSE); return 0;
    case WM_PAINT:    Paint(hwnd); return 0;
    case WM_KEYDOWN:
        if (wp == VK_UP   && g_sel > 0)         { g_sel--; InvalidateRect(hwnd, NULL, FALSE); }
        if (wp == VK_DOWN && g_sel < g_count-1) { g_sel++; InvalidateRect(hwnd, NULL, FALSE); }
        if (wp == VK_RETURN)                    Launch(g_sel);
        return 0;
    case WM_LBUTTONDOWN: {
        int i = ((int)(short)HIWORD(lp) - TOP) / ROW_H;
        if (i >= 0 && i < g_count) { g_sel = i; InvalidateRect(hwnd, NULL, FALSE); Launch(i); }
        return 0;
    }
    case WM_DESTROY:  KillTimer(hwnd, 1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    WNDCLASSW wc;
    HWND hwnd;
    MSG msg;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"DCSHELL";
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(0, L"DCSHELL", L"DCShell", WS_VISIBLE,
                           0, 0,
                           GetSystemMetrics(SM_CXSCREEN),
                           GetSystemMetrics(SM_CYSCREEN),
                           NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
