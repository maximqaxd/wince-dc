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

// Read the first DNS server the shim wrote to HKLM\Comm "DnsServers"
// (REG_BINARY = [count][ip...], network order). 0 if none.
static unsigned long ReadDnsServer(void)
{
    HKEY h; DWORD t, n; unsigned long buf[6], ip = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, KEY_QUERY_VALUE, &h) == ERROR_SUCCESS)
    {
        n = sizeof(buf);
        if (RegQueryValueExW(h, L"DnsServers", 0, &t, (BYTE *)buf, &n) == ERROR_SUCCESS && n >= 8 && buf[0] >= 1)
            ip = buf[1];
        RegCloseKey(h);
    }
    return ip;
}

// Bounded TCP connect: non-blocking connect + select() with a few-second timeout, so an
// unreachable target returns in ~secs instead of blocking the UI thread for the full ~15-20s
// CE connect timeout. Two back-to-back full-length blocking connects froze the app long
// enough to be killed/faulted - this keeps each probe short. Returns 1 = connected.
static int TryConnect(SOCKET s, SOCKADDR_IN *sa, int secs)
{
    unsigned long nb = 1;
    struct timeval tv;
    fd_set wf, ef;
    int r;
    ioctlsocket(s, FIONBIO, &nb);
    if (connect(s, (SOCKADDR *)sa, sizeof(*sa)) == 0) return 1;     // immediate
    if (WSAGetLastError() != WSAEWOULDBLOCK) return 0;             // hard error (refused)
    FD_ZERO(&wf); FD_SET(s, &wf);
    FD_ZERO(&ef); FD_SET(s, &ef);
    tv.tv_sec = secs; tv.tv_usec = 0;
    r = select(0, 0, &wf, &ef, &tv);                              // writable = connected
    return (r > 0 && FD_ISSET(s, &wf)) ? 1 : 0;
}

// One TCP connect probe -> log "label ip:port OK/FAIL". Routing discriminator: the DNS
// server is known reachable (DNS resolved through it), so if dnssrv:53 connects but the web
// host:80 doesn't, the stack+gateway routing are fine and the failure is the remote
// target/NAT - not our shim. If even dnssrv:53 fails, TCP isn't getting through (vs the UDP
// DNS that did) -> a stack/link problem.
static void Probe(unsigned long ip, int port, const WCHAR *label)
{
    SOCKET s; SOCKADDR_IN sa; WCHAR b[56];
    unsigned char *p = (unsigned char *)&ip;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { Log(L"  socket() err"); return; }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET; sa.sin_port = htons((u_short)port); sa.sin_addr.s_addr = ip;
    wsprintfW(b, L"%s %u.%u.%u.%u:%d %s", label, p[0], p[1], p[2], p[3], port,
              TryConnect(s, &sa, 4) ? L"OK" : L"FAIL");
    Log(b);
    closesocket(s);
}

// Run the whole winsock probe synchronously (brief block during connect/recv is fine
// for a manual test). SEH-guarded so a missing winsock export degrades gracefully.
static void RunTest(void)
{
    WSADATA        wsa;
    struct hostent *he;
    unsigned long  ip = 0, dns;

    g_nLine = 0;
    __try
    {
        if (WSAStartup(MAKEWORD(1, 1), &wsa) != 0) { Log(L"WSAStartup FAILED"); return; }
        Log(L"WSAStartup ok");

        // Configured DNS server (from DHCP option-6 or flashrom, via the shim).
        dns = ReadDnsServer();
        if (dns) { WCHAR b[56]; unsigned char *p = (unsigned char *)&dns;
                   wsprintfW(b, L"DNS srv = %u.%u.%u.%u", p[0], p[1], p[2], p[3]); Log(b); }
        else Log(L"DNS srv = (none)");

        // 1) DNS resolve
        he = gethostbyname(HOST);
        if (he && he->h_addr_list[0])
        {
            WCHAR b[56]; unsigned char *p;
            ip = *(unsigned long *)he->h_addr_list[0];
            p = (unsigned char *)&ip;
            wsprintfW(b, L"DNS ok: %s=%u.%u.%u.%u", HOSTW, p[0], p[1], p[2], p[3]); Log(b);
        }
        else { Log(L"DNS failed"); ip = inet_addr("1.1.1.1"); }

        // Routing discriminator (see Probe()): on-link DNS server (TCP 53), then a
        // KNOWN-LIVE off-link host 1.1.1.1:80 (Cloudflare - definitely up, so unlike the
        // DNS-resolved host this tells us whether OFF-LINK/internet TCP works at all), then
        // the resolved web host. dnssrv OK + cf FAIL = off-link/routing is the problem.
        if (dns) Probe(dns, 53, L"dnssrv");
        Probe(inet_addr("1.1.1.1"), 80, L"cf");
        Probe(ip, PORT, L"host");
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
        int cw = CW, ch = CH;
        if (DCWinClientSize(w, &cw, &ch)) changed = 1;   // shell resized/maximized -> republish to fit

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
            DCWinFill(w, 0, 0, cw, ch, RGB(192, 192, 192));        // background fills the window
            DCWinFill(w, 6, 6, cw - 12, 16, RGB(0, 0, 128));       // header spans the full width
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
