//
// dcwnet.c - Network test: a DCWin client that exercises the winsock -> microstk
// path end to end, so we can validate the universal link shim (modem/BBA/W5500)
// without a full browser. It is the "backport" of the IE sample's networking down
// to its essentials: the IE app brings the link up via RAS then lets the WebBrowser2
// control fetch over winsock - here we skip the HTML control and hit winsock直接:
//   WSAStartup -> gethostbyname (DNS) -> socket/connect (TCP) -> HTTP GET -> recv.
//
// Each step's result is shown in the window. With no bound link the connect fails
// fast (proving the stack is there but no route); once the shim binds the BBA + DHCP,
// the same calls succeed - that is the go signal for every winsock title.
//
// Keys: Enter / A = run the test.  Esc = close (shell handles it).
//
#include "dcwlib.h"
#include <winsock.h>

#define CW   324
#define CH   210
#define HOST  "www.sega.com"       // narrow: gethostbyname + HTTP request
#define HOSTW L"www.sega.com"      // wide: on-screen display
#define PORT  80

static WCHAR g_line[8][56];        // result lines
static int   g_nLine;
static int   g_busy;               // a test is running
static int   g_done;               // a test finished (publish once)

static void Log(const WCHAR *s)
{
    int i;
    if (g_nLine >= 8) return;
    for (i = 0; i < 54 && s[i]; i++) g_line[g_nLine][i] = s[i];
    g_line[g_nLine][i] = 0;
    g_nLine++;
}

static void LogF(const WCHAR *fmt, DWORD a)
{
    WCHAR b[56];
    wsprintfW(b, fmt, a);
    Log(b);
}

// Run the whole winsock probe synchronously (brief block during connect/recv is fine
// for a manual test). SEH-guarded so a missing winsock export degrades gracefully.
static void RunTest(void)
{
    WSADATA        wsa;
    struct hostent *he;
    SOCKET         s;
    SOCKADDR_IN    sa;
    unsigned long  ip = 0;
    int            n;
    char           buf[128];
    static const char req[] = "GET / HTTP/1.0\r\nHost: " HOST "\r\n\r\n";

    g_nLine = 0;
    __try
    {
        if (WSAStartup(MAKEWORD(1, 1), &wsa) != 0) { Log(L"WSAStartup FAILED"); return; }
        Log(L"WSAStartup ok");

        // 1) DNS
        he = gethostbyname(HOST);
        if (he && he->h_addr_list[0])
        {
            ip = *(unsigned long *)he->h_addr_list[0];
            Log(L"DNS ok");
            { WCHAR b[56]; unsigned char *p = (unsigned char *)&ip;
              wsprintfW(b, L"  %s = %u.%u.%u.%u", HOSTW, p[0], p[1], p[2], p[3]); Log(b); }
        }
        else
        {
            Log(L"DNS failed - direct IP 1.1.1.1");
            ip = inet_addr("1.1.1.1");                      // off-subnet: tests BBA+gateway+NAT, not loopback
        }

        // 2) TCP connect
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) { LogF(L"socket() err %u", (DWORD)WSAGetLastError()); return; }
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port   = htons(PORT);
        sa.sin_addr.s_addr = ip;
        if (connect(s, (SOCKADDR *)&sa, sizeof(sa)) != 0)
        {
            LogF(L"connect() FAILED err %u", (DWORD)WSAGetLastError());
            Log(L"(no link bound? run the shim)");
            closesocket(s);
            return;
        }
        Log(L"TCP connect ok");

        // 3) HTTP GET
        if (send(s, req, sizeof(req) - 1, 0) <= 0) { Log(L"send() FAILED"); closesocket(s); return; }
        n = recv(s, buf, sizeof(buf) - 1, 0);
        if (n > 0)
        {
            WCHAR w[56]; int i, j;
            buf[n] = 0;
            for (i = 0, j = 0; j < 54 && buf[i] && buf[i] != '\r' && buf[i] != '\n'; i++, j++)
                w[j] = (WCHAR)(unsigned char)buf[i];
            w[j] = 0;
            LogF(L"recv %u bytes:", (DWORD)n);
            Log(w);                                         // first response line
        }
        else Log(L"recv: no data");
        closesocket(s);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { Log(L"winsock faulted (export missing)"); }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *w;
    DWORD  key;
    int    drawnOnce = 0;

    w = DCWinOpen(110, 60, CW, CH, L"Network Test", ICON_APP);
    if (!w) { OutputDebugStringW(L"DCWNET: DCWinOpen failed\r\n"); return 1; }
    Log(L"Enter / A  =  test winsock -> " HOSTW);

    for (;;)
    {
        int changed = 0;

        while (DCWinPollKey(w, &key))
        {
            if ((key == VK_RETURN || key == VK_SPACE) && !g_busy)
            {
                g_busy = 1;
                Log(L"testing...");
                changed = 1;
            }
            else changed = 1;
        }

        if (g_busy)                       // run after we've shown "testing..."
        {
            RunTest();
            g_busy = 0;
            g_done = 1;
            changed = 1;
        }

        if (changed || !drawnOnce)
        {
            int i, y;
            drawnOnce = 1;
            DCWinBeginFrame(w);
            DCWinFill(w, 0, 0, CW, CH, RGB(192, 192, 192));
            DCWinFill(w, 6, 6, CW - 12, 16, RGB(0, 0, 128));
            DCWinText(w, 10, 7, RGB(255, 255, 255), RGB(0, 0, 128), L"winsock / microstk probe");
            for (i = 0, y = 30; i < g_nLine; i++, y += 16)
                DCWinText(w, 10, y, RGB(0, 0, 0), RGB(192, 192, 192), g_line[i]);
            DCWinEndFrame(w);
        }

        if (DCWinShouldClose(w)) break;
        Sleep(20);
    }

    WSACleanup();
    DCWinClose(w);
    return 0;
}
