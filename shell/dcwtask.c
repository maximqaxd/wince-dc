//
// dcwtask.c - Task Manager: a DCWin client (own process) that lists the real CE
// processes via toolhelp, shows free RAM, and ends a selected task.
//
// Process enumeration uses toolhelp.dll (dynamically loaded so a missing DLL
// degrades gracefully instead of failing to start). GlobalMemoryStatus and
// OpenProcess/TerminateProcess are coredll APIs; the kill path is SEH-guarded.
//
// Keys (forwarded by the shell to the focused window): Up/Down select,
// Delete or Enter = end task, Esc = close (shell handles it).
//
#include "dcwlib.h"
#include <tlhelp32.h>

#define CW    300
#define CH    222
#define ROWH  14
#define LISTY 40
#define ROWS  11          // visible process rows
#define MAXP  48

#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE 0x0001
#endif

typedef HANDLE (WINAPI *PFN_Snap)(DWORD, DWORD);
typedef BOOL   (WINAPI *PFN_P32)(HANDLE, LPPROCESSENTRY32);
typedef BOOL   (WINAPI *PFN_Close)(HANDLE);
typedef BOOL   (WINAPI *PFN_HL)(HANDLE, LPHEAPLIST32);
typedef BOOL   (WINAPI *PFN_HF)(HANDLE, LPHEAPENTRY32, DWORD, DWORD);
typedef BOOL   (WINAPI *PFN_HN)(HANDLE, LPHEAPENTRY32);

static PFN_Snap  pSnap;
static PFN_P32   pFirst, pNext;
static PFN_Close pClose;
static PFN_HL    pHeapListFirst, pHeapListNext;
static PFN_HF    pHeapFirst;
static PFN_HN    pHeapNext;

static DWORD g_pid[MAXP];
static WCHAR g_name[MAXP][32];
static DWORD g_mem[MAXP];          // heap bytes per process (0 = n/a)
static int   g_n, g_sel, g_top;
static WCHAR g_status[44] = L"";

static const WCHAR *Base(const WCHAR *p)
{
    const WCHAR *b = p;
    for (; *p; p++) if (*p == L'\\') b = p + 1;
    return b;
}

static int IEq(const WCHAR *a, const WCHAR *b)
{
    for (; *a && *b; a++, b++)
        if ((*a | 32) != (*b | 32)) return 0;
    return *a == *b;
}

// Critical processes we refuse to terminate (killing them freezes the system).
static int Protected(const WCHAR *n)
{
    return IEq(n, L"nk.exe")    || IEq(n, L"gwes.exe") || IEq(n, L"filesys.exe") ||
           IEq(n, L"device.exe")|| IEq(n, L"dcshell.exe");
}

static void LoadToolhelp(void)
{
    HINSTANCE h = LoadLibraryW(L"toolhelp.dll");
    if (!h) { OutputDebugStringW(L"DCWTASK: toolhelp.dll load failed\r\n"); return; }
    pSnap  = (PFN_Snap) GetProcAddress(h, L"CreateToolhelp32Snapshot");
    pFirst = (PFN_P32)  GetProcAddress(h, L"Process32First");
    pNext  = (PFN_P32)  GetProcAddress(h, L"Process32Next");
    pClose = (PFN_Close)GetProcAddress(h, L"CloseToolhelp32Snapshot");
    pHeapListFirst = (PFN_HL)GetProcAddress(h, L"Heap32ListFirst");
    pHeapListNext  = (PFN_HL)GetProcAddress(h, L"Heap32ListNext");
    pHeapFirst     = (PFN_HF)GetProcAddress(h, L"Heap32First");
    pHeapNext      = (PFN_HN)GetProcAddress(h, L"Heap32Next");
    if (!pSnap || !pFirst || !pNext)
        OutputDebugStringW(L"DCWTASK: toolhelp procs missing\r\n");
}

// Sum the non-free heap blocks of one process = a usable per-process memory
// estimate (CE 2.12 has no working-set API). Heavy, so call only for the selected
// row. SEH-guarded in case this toolhelp build doesn't implement heap walking.
static DWORD MemUsage(DWORD pid)
{
    DWORD total = 0;
    if (!pSnap || !pHeapListFirst || !pHeapFirst || !pHeapNext)
        return 0;
    __try
    {
        HANDLE      snap = pSnap(TH32CS_SNAPHEAPLIST, pid);
        HEAPLIST32  hl;
        HEAPENTRY32 he;
        if (!snap || snap == INVALID_HANDLE_VALUE)
            return 0;
        memset(&hl, 0, sizeof(hl)); hl.dwSize = sizeof(hl);
        if (pHeapListFirst(snap, &hl))
        {
            do
            {
                memset(&he, 0, sizeof(he)); he.dwSize = sizeof(he);
                if (pHeapFirst(snap, &he, hl.th32ProcessID, hl.th32HeapID))
                    do { if (!(he.dwFlags & LF32_FREE)) total += he.dwBlockSize; }
                    while (pHeapNext(snap, &he));
            } while (pHeapListNext && pHeapListNext(snap, &hl));
        }
        if (pClose) pClose(snap);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { total = 0; }
    return total;
}


static void Scan(void)
{
    HANDLE         snap;
    PROCESSENTRY32 pe;
    g_n = 0;
    if (!pSnap || !pFirst || !pNext)
        return;
    snap = pSnap(TH32CS_SNAPPROCESS, 0);
    if (!snap || snap == INVALID_HANDLE_VALUE)
        return;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    if (pFirst(snap, &pe))
    {
        do
        {
            const WCHAR *nm = Base(pe.szExeFile);
            int k;
            if (g_n >= MAXP) break;
            for (k = 0; k < 31 && nm[k]; k++) g_name[g_n][k] = nm[k];
            g_name[g_n][k] = 0;
            g_pid[g_n] = pe.th32ProcessID;
            g_n++;
        } while (pNext(snap, &pe));
    }
    if (pClose) pClose(snap);
    if (g_sel >= g_n) g_sel = g_n ? g_n - 1 : 0;

    {
        int i;
        for (i = 0; i < g_n; i++) g_mem[i] = MemUsage(g_pid[i]);   // per-process heap bytes
    }
}

static void EndTask(void)
{
    if (g_sel < 0 || g_sel >= g_n)
        return;
    if (Protected(g_name[g_sel]))
    {
        lstrcpyW(g_status, L"protected - won't end");
        return;
    }
    __try
    {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, g_pid[g_sel]);
        if (h)
        {
            TerminateProcess(h, 0);
            CloseHandle(h);
            wsprintfW(g_status, L"ended %s", g_name[g_sel]);
        }
        else lstrcpyW(g_status, L"can't open process");
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { lstrcpyW(g_status, L"end-task unavailable"); }
}

static void RamLine(WCHAR *out)
{
    MEMORYSTATUS ms;
    memset(&ms, 0, sizeof(ms));
    ms.dwLength = sizeof(ms);
    __try
    {
        GlobalMemoryStatus(&ms);
        wsprintfW(out, L"RAM  %u K free / %u K  (%u%% used)",
                  (unsigned)(ms.dwAvailPhys / 1024), (unsigned)(ms.dwTotalPhys / 1024),
                  (unsigned)ms.dwMemoryLoad);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { lstrcpyW(out, L"RAM  (unavailable)"); }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key, nextScan;

    w = DCWinOpen(150, 70, CW, CH, L"Task Manager", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWTASK: DCWinOpen failed\r\n"); return 1; }

    LoadToolhelp();
    Scan();
    nextScan = GetTickCount() + 1000;

    for (;;)
    {
        WCHAR ram[64], row[64], mem[12];
        int   i, y;

        while (DCWinPollKey(w, &key))
        {
            if      (key == VK_UP   && g_sel > 0)        g_sel--;
            else if (key == VK_DOWN && g_sel < g_n - 1)  g_sel++;
            else if (key == VK_DELETE || key == VK_RETURN) { EndTask(); Scan(); }
            if (g_sel < g_top)          g_top = g_sel;
            if (g_sel >= g_top + ROWS)  g_top = g_sel - ROWS + 1;
        }
        if (GetTickCount() >= nextScan) { Scan(); nextScan += 1000; }

        DCWinBeginFrame(w);
        DCWinFill(w, 0, 0, CW, CH, RGB(192, 192, 192));
        RamLine(ram);
        DCWinText(w, 8, 6, RGB(0, 0, 0), RGB(192, 192, 192), ram);
        DCWinFill(w, 6, 22, CW - 12, ROWH, RGB(0, 0, 128));
        DCWinText(w, 10, 23, RGB(255, 255, 255), RGB(0, 0, 128), L"PID    Mem    Image");

        for (i = 0; i < ROWS && g_top + i < g_n; i++)
        {
            int      idx = g_top + i;
            COLORREF bg  = (idx == g_sel) ? RGB(0, 0, 160) : RGB(192, 192, 192);
            COLORREF fg  = (idx == g_sel) ? RGB(255, 255, 255) : RGB(0, 0, 0);
            y = LISTY + i * ROWH;
            if (idx == g_sel)
                DCWinFill(w, 6, y, CW - 12, ROWH, bg);
            if (g_mem[idx]) wsprintfW(mem, L"%5uK", (unsigned)(g_mem[idx] / 1024));
            else            lstrcpyW(mem, L"   --");
            wsprintfW(row, L"%5u %s %s", (unsigned)g_pid[idx], mem, g_name[idx]);
            DCWinText(w, 10, y + 1, fg, bg, row);
        }

        y = LISTY + ROWS * ROWH + 4;
        wsprintfW(row, L"%d procs   Del/Enter: end task", g_n);
        DCWinText(w, 8, y, RGB(0, 0, 0), RGB(192, 192, 192), row);
        if (g_status[0])
            DCWinText(w, 8, y + ROWH, RGB(128, 0, 0), RGB(192, 192, 192), g_status);
        DCWinEndFrame(w);

        if (DCWinShouldClose(w)) break;
        Sleep(60);
    }

    DCWinClose(w);
    return 0;
}
