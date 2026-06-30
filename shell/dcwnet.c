//
// dcwnet.c - Network diagnostics: a DCWin client that exercises the whole stack the way a
// real winsock title does - including DIALING. The old version only did gethostbyname/connect,
// which works on ethernet (the link is already up via boot-time DHCP) but NEVER brings a modem
// up: dial-up only connects when something calls RasDial. So this version dials first (RasDial
// -> our mppp shim -> ethernet instant-connect OR, in modem mode, the delegated original PPP
// driver), shows the connection + IP + DNS, then resolves + connects to a user-typed target.
//
// All controls are clickable with the analog-stick pointer (the shell delivers it via
// DCWinGetPointer): an on-screen keyboard edits the target host/IP, and Dial / Test / Hang Up
// buttons drive the test. Results are a colour-coded log (green OK / red FAIL / blue info).
//
#include "dcwlib.h"
#include <winsock.h>
#include <ras.h>

#define CW 472
#define CH 372

// ---- colours ----
#define C_BG    RGB(192, 192, 192)
#define C_HDR   RGB(0, 0, 128)
#define C_WHITE RGB(255, 255, 255)
#define C_BLACK RGB(0, 0, 0)
#define C_KEY   RGB(176, 176, 176)
#define C_KEYF  RGB(224, 224, 224)
#define C_BTN   RGB(0, 0, 128)
#define C_OK    RGB(0, 112, 0)
#define C_FAIL  RGB(176, 0, 0)
#define C_INFO  RGB(0, 0, 160)
#define C_MUTE  RGB(96, 96, 96)

// ---- target host/IP (optionally host/path) being edited ----
// Default to a known plain-HTTP test file (exactly 1,048,576 bytes, Content-Length set) so the
// Get download test works out of the box - we have no TLS, so it must be http:// (port 80).
static WCHAR g_pszTarget[80] = L"speedtest.tele2.net/1MB.zip";

// ---- connection state ----
enum
{
	CN_IDLE,
	CN_DIALING,
	CN_UP,
	CN_FAILED
};
static int g_nConn = CN_IDLE;
static HRASCONN g_hConn;
static DWORD g_dwDialStart;
static WCHAR g_pszIpStr[24] = L"-";
static WCHAR g_pszDnsStr[24] = L"-";

// ---- colour-coded result log ----
typedef struct
{
	COLORREF c;
	WCHAR s[58];
} LogLine;
static LogLine g_aLog[7];
static int g_nLog;

static void LogC(COLORREF c, const WCHAR *psz)
{
	int i;
	if (g_nLog >= 7)
	{
		for (i = 0; i < 6; i++)
			g_aLog[i] = g_aLog[i + 1];
		g_nLog = 6;
	} // scroll
	g_aLog[g_nLog].c = c;
	for (i = 0; i < 56 && psz[i]; i++)
		g_aLog[g_nLog].s[i] = psz[i];
	g_aLog[g_nLog].s[i] = 0;
	g_nLog++;
}
static void LogF(COLORREF c, const WCHAR *pszFmt, DWORD dwArg)
{
	WCHAR achBuf[58];
	wsprintfW(achBuf, pszFmt, dwArg);
	LogC(c, achBuf);
}

// ---- registry DNS (the shim writes HKLM\Comm "DnsServers" = [count][ip...] net order) ----
static unsigned long ReadDns(void)
{
	HKEY h;
	DWORD dwType, cb;
	unsigned long aulBuf[6], ulIp = 0;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Comm", 0, KEY_QUERY_VALUE, &h) == ERROR_SUCCESS)
	{
		cb = sizeof(aulBuf);
		if (RegQueryValueExW(h, L"DnsServers", 0, &dwType, (BYTE *)aulBuf, &cb) == ERROR_SUCCESS &&
		    cb >= 8 && aulBuf[0] >= 1)
			ulIp = aulBuf[1];
		RegCloseKey(h);
	}
	return ulIp;
}

static void IpToStr(unsigned long ulIp, WCHAR *pszOut)
{
	unsigned char *pb = (unsigned char *)&ulIp;
	wsprintfW(pszOut, L"%u.%u.%u.%u", pb[0], pb[1], pb[2], pb[3]);
}

// Local bound address: a UDP socket "connected" to the DNS server (no packets sent) lets
// getsockname report the source IP the stack would use - works on ethernet and PPP alike.
static void RefreshLocalIp(void)
{
	unsigned long ulDns = ReadDns();
	SOCKET sock;
	SOCKADDR_IN sa, me;
	int cbMe = sizeof(me);
	if (ulDns)
		IpToStr(ulDns, g_pszDnsStr);
	else
		lstrcpyW(g_pszDnsStr, L"(none)");
	lstrcpyW(g_pszIpStr, L"-");
	if (!ulDns)
		return;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
		return;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(53);
	sa.sin_addr.s_addr = ulDns;
	if (connect(sock, (SOCKADDR *)&sa, sizeof(sa)) == 0 &&
	    getsockname(sock, (SOCKADDR *)&me, &cbMe) == 0)
		IpToStr(me.sin_addr.s_addr, g_pszIpStr);
	closesocket(sock);
}

// ---- dialing (RasDial -> our shim -> modem PPP / ethernet instant-connect) ----
// Ensure a dial entry exists pointing at the DC built-in modem, so RasDial has something to
// dial. On ethernet our shim ignores the device and instant-connects; in modem mode the
// delegated original driver opens HKLM\Modem\Sega-DreamcastBuiltIn (COM6) and runs PPP.
static void EnsureEntry(void)
{
	RASENTRY re;
	memset(&re, 0, sizeof(re));
	re.dwSize = sizeof(re);
	re.dwfOptions = RASEO_IpHeaderCompression;
	re.dwCountryID = 1;
	lstrcpyW(re.szDeviceType, RASDT_Modem);
	lstrcpyW(re.szDeviceName, L"Sega-DreamcastBuiltIn");
	lstrcpyW(re.szLocalPhoneNumber, L"0118999"); // Flycast/null-modem ignore the number
	RasSetEntryProperties(NULL, L"DC Modem", &re, sizeof(re), NULL, 0);
}

static void DoDial(void)
{
	RASDIALPARAMS dp;
	DWORD dwRc;
	if (g_nConn == CN_DIALING || g_nConn == CN_UP)
		return;
	g_nLog = 0;
	LogC(C_INFO, L"Dialing (RasDial)...");
	EnsureEntry();
	memset(&dp, 0, sizeof(dp));
	dp.dwSize = sizeof(dp);
	lstrcpyW(dp.szEntryName, L"DC Modem");
	g_hConn = 0;
	dwRc = RasDial(NULL, NULL, &dp, 0, NULL, &g_hConn);
	if (dwRc != 0)
	{
		LogF(C_FAIL, L"RasDial err %u", dwRc);
		g_nConn = CN_FAILED;
		return;
	}
	g_nConn = CN_DIALING;
	g_dwDialStart = GetTickCount();
}

// Non-blocking dial progress; called each frame while CN_DIALING. Returns 1 if state changed.
static int PollDial(void)
{
	RASCONNSTATUS st;
	if (g_nConn != CN_DIALING)
		return 0;
	memset(&st, 0, sizeof(st));
	st.dwSize = sizeof(st);
	RasGetConnectStatus(g_hConn, &st);
	if (st.rasconnstate == RASCS_Connected)
	{
		WCHAR achBuf[58];
		g_nConn = CN_UP;
		RefreshLocalIp();
		LogC(C_OK, L"Connected.");
		wsprintfW(achBuf, L"  IP  %s", g_pszIpStr);
		LogC(C_INFO, achBuf);
		wsprintfW(achBuf, L"  DNS %s", g_pszDnsStr);
		LogC(C_INFO, achBuf);
		return 1;
	}
	if (st.dwError != 0)
	{
		LogF(C_FAIL, L"Dial failed, err %u", st.dwError);
		g_nConn = CN_FAILED;
		return 1;
	}
	if (GetTickCount() - g_dwDialStart > 45000)
	{
		LogC(C_FAIL, L"Dial timeout (45s)");
		g_nConn = CN_FAILED;
		return 1;
	}
	return 0;
}

static void DoHangup(void)
{
	if (g_hConn)
		RasHangUp(g_hConn);
	g_hConn = 0;
	g_nConn = CN_IDLE;
	lstrcpyW(g_pszIpStr, L"-");
	LogC(C_MUTE, L"Hung up.");
}

// ---- the actual reachability test (DNS -> TCP -> HTTP), on g_pszTarget ----
static int TryConnect(SOCKET sock, SOCKADDR_IN *psa, int nSecs)
{
	unsigned long ulNonBlock = 1;
	struct timeval tv;
	fd_set wf, ef;
	int nRet;
	ioctlsocket(sock, FIONBIO, &ulNonBlock);
	if (connect(sock, (SOCKADDR *)psa, sizeof(*psa)) == 0)
		return 1;
	if (WSAGetLastError() != WSAEWOULDBLOCK)
		return 0;
	FD_ZERO(&wf);
	FD_SET(sock, &wf);
	FD_ZERO(&ef);
	FD_SET(sock, &ef);
	tv.tv_sec = nSecs;
	tv.tv_usec = 0;
	nRet = select(0, 0, &wf, &ef, &tv);
	return (nRet > 0 && FD_ISSET(sock, &wf)) ? 1 : 0;
}

// Split g_pszTarget ("host" or "host/path") into an ANSI host + path (path defaults to "/").
static void SplitTarget(char *pszHostA, int nHCap, char *pszPathA, int nPCap)
{
	WCHAR achHost[80];
	const WCHAR *pszSlash;
	int i, nHostLen = 0;
	for (pszSlash = g_pszTarget; *pszSlash && *pszSlash != L'/'; pszSlash++)
		;
	for (i = 0; g_pszTarget + i < pszSlash && nHostLen < 78; i++)
		achHost[nHostLen++] = g_pszTarget[i];
	achHost[nHostLen] = 0;
	WideCharToMultiByte(CP_ACP, 0, achHost, -1, pszHostA, nHCap, 0, 0);
	if (*pszSlash)
		WideCharToMultiByte(CP_ACP, 0, pszSlash, -1, pszPathA, nPCap, 0, 0);
	else
	{
		pszPathA[0] = '/';
		pszPathA[1] = 0;
	}
}

static void DoTest(void)
{
	char achHostA[80], achPathA[96];
	struct hostent *phe;
	unsigned long ulIp;
	SOCKET sock;
	SOCKADDR_IN sa;
	int bIsIp;

	g_nLog = 0;
	if (g_nConn != CN_UP)
		LogC(C_MUTE, L"(not dialed - testing anyway)");
	SplitTarget(achHostA, sizeof(achHostA), achPathA, sizeof(achPathA));

	// numeric IP or hostname?
	ulIp = inet_addr(achHostA);
	bIsIp = (ulIp != INADDR_NONE);
	if (bIsIp)
	{
		WCHAR achBuf[40];
		wsprintfW(achBuf, L"target is literal IP");
		LogC(C_INFO, achBuf);
	}
	else
	{
		__try
		{
			phe = gethostbyname(achHostA);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			phe = 0;
		}
		if (phe && phe->h_addr_list[0])
		{
			WCHAR achBuf[58], achIps[24];
			ulIp = *(unsigned long *)phe->h_addr_list[0];
			IpToStr(ulIp, achIps);
			wsprintfW(achBuf, L"DNS OK -> %s", achIps);
			LogC(C_OK, achBuf);
		}
		else
		{
			LogC(C_FAIL, L"DNS resolve FAILED");
			return;
		}
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		LogC(C_FAIL, L"socket() failed");
		return;
	}
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	sa.sin_addr.s_addr = ulIp;
	if (!TryConnect(sock, &sa, 6))
	{
		LogC(C_FAIL, L"TCP connect :80 FAILED");
		closesocket(sock);
		return;
	}
	LogC(C_OK, L"TCP connect :80 OK");

	{ // minimal HTTP GET to prove data flows
		char achReq[200], achBuf[512];
		int nReqLen = 0, nRecv = -1;
		fd_set rf;
		struct timeval tv;
		int nRet;
		const char *pszG = "GET ";
		while (*pszG)
			achReq[nReqLen++] = *pszG++;
		{
			int i;
			for (i = 0; achPathA[i] && nReqLen < 150; i++)
				achReq[nReqLen++] = achPathA[i];
		}
		{
			const char *pszH = " HTTP/1.0\r\nHost: ";
			while (*pszH)
				achReq[nReqLen++] = *pszH++;
		}
		{
			int i;
			for (i = 0; achHostA[i] && nReqLen < 180; i++)
				achReq[nReqLen++] = achHostA[i];
		}
		{
			const char *pszE = "\r\nConnection: close\r\n\r\n";
			while (*pszE)
				achReq[nReqLen++] = *pszE++;
		}
		send(sock, achReq, nReqLen, 0);
		// sock is still NON-BLOCKING (from TryConnect) - select() for readability before recv,
		// else recv returns WSAEWOULDBLOCK instantly (looks like "no reply" but data's en route).
		FD_ZERO(&rf);
		FD_SET(sock, &rf);
		tv.tv_sec = 6;
		tv.tv_usec = 0;
		nRet = select(0, &rf, 0, 0, &tv);
		if (nRet > 0 && FD_ISSET(sock, &rf))
			nRecv = recv(sock, achBuf, sizeof(achBuf) - 1, 0);
		if (nRecv > 0)
		{
			WCHAR achLine[58];
			int i, j = 0;
			achBuf[nRecv] = 0; // show the HTTP status line
			for (i = 0; i < nRecv && j < 54 && achBuf[i] != '\r' && achBuf[i] != '\n'; i++)
				achLine[j++] = (WCHAR)(unsigned char)achBuf[i];
			achLine[j] = 0;
			LogC(C_OK, achLine);
			LogF(C_INFO, L"(%u bytes received)", (DWORD)nRecv);
		}
		else if (nRecv == 0)
			LogC(C_FAIL, L"connected but server sent no data");
		else
			LogC(C_FAIL, L"no HTTP reply (6s timeout)");
	}
	closesocket(sock);
}

// ---- download-to-RAM test: GET the whole body into a RAM buffer, verify the byte count, show
// progress. The buffer lives only in RAM and is freed when the app closes or a new Get starts
// (the DC filesystem is read-only anyway). Streams across frames so the UI + progress bar update.
enum
{
	DL_IDLE,
	DL_ACTIVE,
	DL_DONE,
	DL_FAIL
};
static int g_nDl = DL_IDLE;
static SOCKET g_sockDl = INVALID_SOCKET;
static BYTE *g_pbDlBuf;    // RAM "file" (grown as data arrives, capped at DL_MAX)
static DWORD g_dwDlCap;    // allocated bytes
static DWORD g_dwDlStored; // bytes actually held in g_pbDlBuf (<= DL_MAX)
static DWORD g_dwDlRaw;    // total bytes received (headers+body), even past the cap
static int g_bDlHdrDone;
static DWORD g_dwDlBodyOff;          // offset of the body within the response stream
static DWORD g_dwDlTotal;            // Content-Length (0 = unknown)
static DWORD g_dwDlLastData;         // tick of last recv (stall timeout)
static DWORD g_dwDlStart, g_dwDlEnd; // ticks: download begin / finish (for speed + elapsed)
#define DL_INIT (64 * 1024)
#define DL_MAX  (4 * 1024 * 1024)

// Format a rate (in KB/s) as "123 KB/s" or "1.2 MB/s" (integer math, no float).
static void SpeedStr(DWORD dwKbps, WCHAR *pszOut)
{
	if (dwKbps >= 1024)
	{
		DWORD dwM10 = dwKbps * 10 / 1024;
		wsprintfW(pszOut, L"%u.%u MB/s", dwM10 / 10, dwM10 % 10);
	}
	else
		wsprintfW(pszOut, L"%u KB/s", dwKbps);
}

// Average rate so far, in KB/s (elapsed = now while active, else the finished span).
static DWORD DownloadKbps(DWORD dwBody)
{
	DWORD dwEl = (g_nDl == DL_ACTIVE) ? (GetTickCount() - g_dwDlStart)
	                                  : (g_dwDlEnd >= g_dwDlStart ? g_dwDlEnd - g_dwDlStart : 0);
	return dwEl ? (dwBody / 1024) * 1000 / dwEl : 0; // (KB) * 1000ms / ms = KB/s, overflow-safe
}

static void DownloadFree(void)
{
	if (g_sockDl != INVALID_SOCKET)
	{
		closesocket(g_sockDl);
		g_sockDl = INVALID_SOCKET;
	}
	if (g_pbDlBuf)
	{
		LocalFree(g_pbDlBuf);
		g_pbDlBuf = 0;
	}
	g_dwDlCap = g_dwDlStored = g_dwDlRaw = g_dwDlBodyOff = g_dwDlTotal = 0;
	g_dwDlStart = g_dwDlEnd = 0;
	g_bDlHdrDone = 0;
}

static DWORD DownloadBody(void)
{
	return (g_dwDlRaw >= g_dwDlBodyOff) ? g_dwDlRaw - g_dwDlBodyOff : 0;
}

// Once the header block is buffered, find the body offset (\r\n\r\n) and Content-Length.
static void DownloadParseHeaders(void)
{
	DWORD i;
	if (g_bDlHdrDone || g_dwDlStored < 4)
		return;
	for (i = 0; i + 3 < g_dwDlStored; i++)
		if (g_pbDlBuf[i] == '\r' && g_pbDlBuf[i + 1] == '\n' && g_pbDlBuf[i + 2] == '\r' &&
		    g_pbDlBuf[i + 3] == '\n')
		{
			g_dwDlBodyOff = i + 4;
			g_bDlHdrDone = 1;
			break;
		}
	if (!g_bDlHdrDone)
		return;
	for (i = 0; i + 16 < g_dwDlBodyOff; i++) // case-insensitive "content-length:"
	{
		static const char szCl[] = "content-length:";
		int j;
		char ch;
		for (j = 0; szCl[j]; j++)
		{
			ch = (char)g_pbDlBuf[i + j];
			if (ch >= 'A' && ch <= 'Z')
				ch = (char)(ch + 32);
			if (ch != szCl[j])
				break;
		}
		if (!szCl[j])
		{
			DWORD dwVal = 0, k = i + 15;
			while (k < g_dwDlBodyOff && (g_pbDlBuf[k] == ' ' || g_pbDlBuf[k] == '\t'))
				k++;
			while (k < g_dwDlBodyOff && g_pbDlBuf[k] >= '0' && g_pbDlBuf[k] <= '9')
				dwVal = dwVal * 10 + (g_pbDlBuf[k++] - '0');
			g_dwDlTotal = dwVal;
			break;
		}
	}
}

// Append a recv'd chunk: count it (always) and store it into the RAM buffer up to DL_MAX (manual
// grow: alloc bigger + copy + free, avoiding LocalReAlloc handle semantics).
static void DownloadAppend(const char *pData, int n)
{
	g_dwDlRaw += (DWORD)n;
	if (g_dwDlStored < DL_MAX)
	{
		DWORD dwNeed = g_dwDlStored + (DWORD)n;
		if (dwNeed > DL_MAX)
			dwNeed = DL_MAX;
		if (dwNeed > g_dwDlCap)
		{
			DWORD dwNewCap = g_dwDlCap ? g_dwDlCap : DL_INIT;
			BYTE *pbNew;
			while (dwNewCap < dwNeed)
				dwNewCap *= 2;
			if (dwNewCap > DL_MAX)
				dwNewCap = DL_MAX;
			pbNew = (BYTE *)LocalAlloc(LPTR, dwNewCap);
			if (pbNew)
			{
				if (g_pbDlBuf)
				{
					memcpy(pbNew, g_pbDlBuf, g_dwDlStored);
					LocalFree(g_pbDlBuf);
				}
				g_pbDlBuf = pbNew;
				g_dwDlCap = dwNewCap;
			}
		}
		if (g_pbDlBuf)
		{
			DWORD dwRoom = (g_dwDlCap > g_dwDlStored) ? g_dwDlCap - g_dwDlStored : 0;
			DWORD dwCpy = ((DWORD)n < dwRoom) ? (DWORD)n : dwRoom;
			memcpy(g_pbDlBuf + g_dwDlStored, pData, dwCpy);
			g_dwDlStored += dwCpy;
		}
	}
	if (!g_bDlHdrDone)
		DownloadParseHeaders();
}

static void DownloadFinish(void)
{
	DWORD dwBody = DownloadBody(), dwEl, dwKbps;
	if (g_sockDl != INVALID_SOCKET)
	{
		closesocket(g_sockDl);
		g_sockDl = INVALID_SOCKET;
	}
	g_dwDlEnd = GetTickCount();
	g_nDl = DL_DONE;
	LogF(C_INFO, L"received %u body bytes", dwBody);
	dwEl = (g_dwDlEnd >= g_dwDlStart) ? g_dwDlEnd - g_dwDlStart : 0;
	dwKbps = DownloadKbps(dwBody);
	{
		WCHAR achSp[24], achBuf[58];
		SpeedStr(dwKbps, achSp);
		wsprintfW(achBuf, L"%u.%us  avg %s", dwEl / 1000, (dwEl % 1000) / 100, achSp);
		LogC(C_INFO, achBuf);
	}
	if (g_dwDlTotal)
	{
		WCHAR achBuf[58];
		if (dwBody == g_dwDlTotal)
		{
			wsprintfW(achBuf, L"COMPLETE: all %u bytes", g_dwDlTotal);
			LogC(C_OK, achBuf);
		}
		else
		{
			wsprintfW(achBuf, L"INCOMPLETE %u / %u bytes", dwBody, g_dwDlTotal);
			LogC(C_FAIL, achBuf);
		}
	}
	else
		LogC(C_OK, L"done (server sent no Content-Length)");
	if (g_dwDlRaw > g_dwDlStored)
		LogF(C_MUTE, L"(buffered first %u in RAM)", g_dwDlStored);
}

// Start a download of g_pszTarget (host or host/path) into RAM. Non-blocking; PollDownload finishes
// it.
static void DoDownloadStart(void)
{
	char achHostA[80], achPathA[96], achReq[220];
	struct hostent *phe;
	unsigned long ulIp;
	SOCKET sock;
	SOCKADDR_IN sa;
	const char *p;
	int nReqLen = 0, i;

	if (g_nDl == DL_ACTIVE)
		return;
	DownloadFree(); // drop the previous RAM file
	g_nLog = 0;
	SplitTarget(achHostA, sizeof(achHostA), achPathA, sizeof(achPathA));

	ulIp = inet_addr(achHostA);
	if (ulIp == INADDR_NONE)
	{
		__try
		{
			phe = gethostbyname(achHostA);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			phe = 0;
		}
		if (!phe || !phe->h_addr_list[0])
		{
			LogC(C_FAIL, L"DNS resolve FAILED");
			g_nDl = DL_FAIL;
			return;
		}
		ulIp = *(unsigned long *)phe->h_addr_list[0];
	}
	{
		WCHAR achBuf[58], achIps[24];
		IpToStr(ulIp, achIps);
		wsprintfW(achBuf, L"GET from %s", achIps);
		LogC(C_INFO, achBuf);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		LogC(C_FAIL, L"socket() failed");
		g_nDl = DL_FAIL;
		return;
	}
	{
		int nRcvBuf = 32 * 1024;
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&nRcvBuf, sizeof(nRcvBuf));
	} // bigger RX window
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	sa.sin_addr.s_addr = ulIp;
	if (!TryConnect(sock, &sa, 6))
	{
		LogC(C_FAIL, L"TCP connect :80 FAILED");
		closesocket(sock);
		g_nDl = DL_FAIL;
		return;
	}
	LogC(C_OK, L"connected - downloading...");

	p = "GET ";
	while (*p)
		achReq[nReqLen++] = *p++;
	for (i = 0; achPathA[i] && nReqLen < 160; i++)
		achReq[nReqLen++] = achPathA[i];
	p = " HTTP/1.0\r\nHost: ";
	while (*p)
		achReq[nReqLen++] = *p++;
	for (i = 0; achHostA[i] && nReqLen < 200; i++)
		achReq[nReqLen++] = achHostA[i];
	p = "\r\nConnection: close\r\n\r\n";
	while (*p)
		achReq[nReqLen++] = *p++;
	send(sock, achReq, nReqLen, 0); // socket is non-blocking (TryConnect set FIONBIO)

	g_sockDl = sock;
	g_nDl = DL_ACTIVE;
	g_dwDlStart = g_dwDlLastData = GetTickCount();
}

// Drive the active download: drain readable bytes (budgeted per frame so the UI stays live).
// recv==0 means the server closed (Connection: close) = complete. Returns 1 to force a redraw.
static int PollDownload(void)
{
	char achTmp[8192];
	int n;
	DWORD dwBudget = 0;
	if (g_nDl != DL_ACTIVE)
		return 0;
	for (;;) // drain readable bytes; the socket is non-blocking
	{
		n = recv(g_sockDl, achTmp, sizeof(achTmp), 0); // recv directly (no select readability poll)
		if (n == 0)
		{
			DownloadFinish();
			return 1;
		}
		if (n < 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;
			LogC(C_FAIL, L"recv error");
			g_nDl = DL_FAIL;
			g_dwDlEnd = GetTickCount();
			closesocket(g_sockDl);
			g_sockDl = INVALID_SOCKET;
			return 1;
		}
		DownloadAppend(achTmp, n);
		g_dwDlLastData = GetTickCount();
		if (g_dwDlTotal && DownloadBody() >= g_dwDlTotal)
		{
			DownloadFinish();
			return 1;
		} // got it all
		dwBudget += (DWORD)n;
		if (dwBudget >= 262144)
			break; // yield to the UI; resume next frame
	}
	if (GetTickCount() - g_dwDlLastData > 15000)
	{
		LogC(C_FAIL, L"download stalled (15s)");
		g_nDl = DL_FAIL;
		g_dwDlEnd = GetTickCount();
		if (g_sockDl != INVALID_SOCKET)
		{
			closesocket(g_sockDl);
			g_sockDl = INVALID_SOCKET;
		}
		return 1;
	}
	return 1; // active -> redraw progress each frame
}

// ---- on-screen keyboard + buttons (clickable via the analog-stick pointer) ----
static const WCHAR *kbRows[4] = {L"1234567890", L"qwertyuiop", L"asdfghjkl-", L"zxcvbnm.:/"};
#define KCOLS 10
#define KW    30
#define KH    22
#define KOX   8
#define KOY   76
#define KGAP  2

// action buttons: id, label, x, w (all on one row at BTN_Y)
#define BTN_Y 196
enum
{
	B_SPACE,
	B_DOTCOM,
	B_BKSP,
	B_CLEAR,
	B_DIAL,
	B_TEST,
	B_GET,
	B_HANG,
	B_COUNT
};
typedef struct
{
	int x, w;
	const WCHAR *label;
	COLORREF c;
} Btn;
static const Btn s_btn[B_COUNT] = {
    {8, 48, L"space", C_KEY},   {60, 48, L".com", C_KEY},    {112, 44, L"Bksp", C_KEY},
    {160, 48, L"Clear", C_KEY}, {222, 52, L"Dial", C_BTN},   {278, 44, L"Test", C_BTN},
    {326, 44, L"Get", C_BTN},   {374, 86, L"HangUp", C_BTN},
};

static void EditAppendCh(WCHAR ch)
{
	int n = lstrlenW(g_pszTarget);
	if (n < 78)
	{
		g_pszTarget[n] = ch;
		g_pszTarget[n + 1] = 0;
	}
}
static void EditAppendStr(const WCHAR *psz)
{
	int n = lstrlenW(g_pszTarget), i;
	for (i = 0; psz[i] && n < 78; i++)
		g_pszTarget[n++] = psz[i];
	g_pszTarget[n] = 0;
}
static void EditBksp(void)
{
	int n = lstrlenW(g_pszTarget);
	if (n)
		g_pszTarget[n - 1] = 0;
}

// Hit-test a click at client (x,y). Returns 1 if it changed something needing a redraw.
static int HandleClick(int x, int y)
{
	int r, c, i;
	for (r = 0; r < 4; r++) // OSK char keys
		for (c = 0; c < KCOLS; c++)
		{
			int kx = KOX + c * (KW + KGAP), ky = KOY + r * (KH + KGAP);
			if (x >= kx && x < kx + KW && y >= ky && y < ky + KH)
			{
				EditAppendCh(kbRows[r][c]);
				return 1;
			}
		}
	for (i = 0; i < B_COUNT; i++) // action buttons
	{
		if (x >= s_btn[i].x && x < s_btn[i].x + s_btn[i].w && y >= BTN_Y && y < BTN_Y + KH)
		{
			switch (i)
			{
				case B_SPACE:
					EditAppendCh(L' ');
					break;
				case B_DOTCOM:
					EditAppendStr(L".com");
					break;
				case B_BKSP:
					EditBksp();
					break;
				case B_CLEAR:
					g_pszTarget[0] = 0;
					break;
				case B_DIAL:
					DoDial();
					break;
				case B_TEST:
					DoTest();
					break;
				case B_GET:
					DoDownloadStart();
					break;
				case B_HANG:
					DoHangup();
					break;
			}
			return 1;
		}
	}
	return 0;
}

static const WCHAR *ConnText(void)
{
	switch (g_nConn)
	{
		case CN_DIALING:
			return L"dialing...";
		case CN_UP:
			return L"CONNECTED";
		case CN_FAILED:
			return L"failed";
		default:
			return L"idle";
	}
}

static void Draw(DCWin *w, int cw, int ch)
{
	int r, c, i, y;
	WCHAR achLine[64];

	DCWinFillBg(w, C_BG);
	DCWinFill(w, 6, 4, cw - 12, 16, C_HDR);
	DCWinText(w, 10, 5, C_WHITE, C_HDR, L"Network Diagnostics");

	// status: connection / IP / DNS
	wsprintfW(achLine, L"Conn: %s    IP: %s    DNS: %s", ConnText(), g_pszIpStr, g_pszDnsStr);
	DCWinText(w, 8, 24, (g_nConn == CN_UP) ? C_OK : (g_nConn == CN_FAILED ? C_FAIL : C_BLACK), C_BG,
	          achLine);

	// target field
	DCWinText(w, 8, 44, C_BLACK, C_BG, L"Target:");
	DCWinFill(w, 58, 42, cw - 66, 18, C_WHITE);
	DCWinText(w, 62, 44, C_BLACK, C_WHITE, g_pszTarget[0] ? g_pszTarget : L"(type a host or IP)");

	// OSK
	for (r = 0; r < 4; r++)
		for (c = 0; c < KCOLS; c++)
		{
			WCHAR k[2];
			int kx = KOX + c * (KW + KGAP), ky = KOY + r * (KH + KGAP);
			k[0] = kbRows[r][c];
			k[1] = 0;
			DCWinFill(w, kx, ky, KW, KH, C_KEYF);
			DCWinText(w, kx + 11, ky + 3, C_BLACK, C_KEYF, k);
		}
	// buttons
	for (i = 0; i < B_COUNT; i++)
	{
		COLORREF bg = s_btn[i].c, fg = (bg == C_BTN) ? C_WHITE : C_BLACK;
		DCWinFill(w, s_btn[i].x, BTN_Y, s_btn[i].w, KH, bg);
		DCWinText(w, s_btn[i].x + 5, BTN_Y + 3, fg, bg, s_btn[i].label);
	}

	// download progress bar (shown once a Get has run)
	if (g_nDl != DL_IDLE)
	{
		DWORD dwBody = DownloadBody();
		DWORD dwPct = g_dwDlTotal ? (dwBody / 1024 * 100) / (g_dwDlTotal / 1024 + 1) : 0;
		int barw = cw - 16, fill;
		COLORREF pc = (g_nDl == DL_FAIL) ? C_FAIL : (g_nDl == DL_DONE ? C_OK : C_HDR);
		const WCHAR *pszSt = (g_nDl == DL_ACTIVE) ? L"Downloading"
		                     : (g_nDl == DL_DONE) ? L"Downloaded"
		                                          : L"Download failed";
		WCHAR achProgLine[96], achSp[24];
		SpeedStr(DownloadKbps(dwBody), achSp);
		if (g_nDl == DL_DONE && g_dwDlTotal && dwBody >= g_dwDlTotal)
			dwPct = 100;
		fill = g_dwDlTotal ? (int)((DWORD)barw * dwPct / 100) : (g_nDl == DL_DONE ? barw : 0);
		if (g_dwDlTotal)
			wsprintfW(achProgLine, L"%s  %u / %u bytes (%u%%)  %s", pszSt, dwBody, g_dwDlTotal,
			          dwPct, achSp);
		else
			wsprintfW(achProgLine, L"%s  %u bytes  %s", pszSt, dwBody, achSp);
		DCWinText(w, 8, 224, C_BLACK, C_BG, achProgLine);
		DCWinFill(w, 8, 238, barw, 12, C_WHITE); // track
		if (fill > 0)
			DCWinFill(w, 8, 238, fill, 12, pc); // fill
	}

	// result log
	DCWinFill(w, 6, 256, cw - 12, 1, C_MUTE);
	for (i = 0, y = 262; i < g_nLog; i++, y += 15)
		DCWinText(w, 10, y, g_aLog[i].c, C_BG, g_aLog[i].s);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
	DCWin *w;
	WSADATA wsa;
	int cw = CW, ch = CH, bDirty = 1, nPrevBtn = 0;
	DWORD dwKey;

	w = DCWinOpen(70, 40, CW, CH, L"Network Diagnostics", ICON_APP);
	if (!w)
	{
		OutputDebugStringW(L"DCWNET: DCWinOpen failed\r\n");
		return 1;
	}
	WSAStartup(MAKEWORD(1, 1), &wsa);
	RefreshLocalIp();
	LogC(C_INFO, L"Point + A on Dial, then Test.");

	for (;;)
	{
		int px, py, nBtn;
		if (DCWinClientSize(w, &cw, &ch))
			bDirty = 1;

		while (DCWinPollKey(w, &dwKey)) // keyboard edits (NOT Enter: the shell
		{                               // synthesizes VK_RETURN on every body click,
			if (dwKey == VK_BACK)
			{
				EditBksp();
				bDirty = 1;
			} // so binding it to a connect made
		} // every OSK keypress fire a blocking Test)

		if (DCWinGetPointer(w, &px, &py, &nBtn)) // analog-stick cursor over our window
		{
			if (nBtn && !nPrevBtn)
			{
				if (HandleClick(px, py))
					bDirty = 1;
			} // click edge
			nPrevBtn = nBtn;
		}
		else
			nPrevBtn = 0;

		if (PollDial())
			bDirty = 1; // advance a modem dial in progress
		if (PollDownload())
			bDirty = 1; // drain an in-progress download

		if (bDirty)
		{
			DCWinBeginFrame(w);
			Draw(w, cw, ch);
			DCWinEndFrame(w);
			bDirty = 0;
		}
		if (g_nConn == CN_DIALING)
			bDirty = 1; // keep polling/redrawing while dialing
		if (g_nDl == DL_ACTIVE)
			bDirty = 1; // keep draining/redrawing while downloading
		if (DCWinShouldClose(w))
			break;
		Sleep(20);
	}

	DownloadFree(); // the RAM "file" is destroyed on close
	if (g_hConn)
		RasHangUp(g_hConn);
	WSACleanup();
	DCWinClose(w);
	return 0;
}
