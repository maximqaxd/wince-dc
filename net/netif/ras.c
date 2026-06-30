//
// ras.c - the RAS backend exports of mppp.dll (ordinals 1-13), stubbed for the
// universal link shim. The link is already up (DHCP via netif.c InterfaceInitialize),
// so "dialing" succeeds instantly: AfdRasDial posts RASCS_Connected to the caller's
// notifier window and returns 0; status queries report connected; the rest succeed.
// (mras.lib thunks RasDial/RasHangUp/etc. -> these AfdRas* by ordinal.)
//
// On SH-4 CE the caller cleans the stack, so stubs that ignore extra args are safe.
//
#include <windows.h>
#include <ras.h>

// netif.c: re-apply DnsServers (picks up the user's Primary/Secondary DNS from this entry).
extern void NetifApplyDns(void);

// Modem delegation (set in netif.c when no ethernet is found): forward every RAS export to the
// original dial-up driver (mpppdial.dll) by ordinal, so real modem dialing/phonebook works. On
// the ethernet path g_nUseDial is 0 and the instant-connect stubs below run instead.
extern int g_nUseDial;
extern FARPROC g_apfnDialRas[14];
// One typedef per AfdRas* signature, so each export can forward to g_apfnDialRas[ordinal].
typedef DWORD (*FnDial)(void *, const WCHAR *, void *, DWORD, void *, HRASCONN *);
typedef DWORD (*FnHangUp)(HRASCONN);
typedef DWORD (*FnEnumConn)(void *, DWORD *, DWORD *);
typedef DWORD (*FnConnStat)(HRASCONN, RASCONNSTATUS *);
typedef DWORD (*FnEnumEnt)(void *, void *, RASENTRYNAME *, DWORD *, DWORD *);
typedef DWORD (*FnGetDial)(void *, RASDIALPARAMS *, BOOL *);
typedef DWORD (*FnSetDial)(void *, RASDIALPARAMS *, BOOL);
typedef DWORD (*FnSetProp)(void *, const WCHAR *, void *, DWORD, void *, DWORD);
typedef DWORD (*FnGetProp)(void *, const WCHAR *, void *, DWORD *, void *, DWORD *);
typedef DWORD (*FnValidate)(void *, const WCHAR *);
typedef DWORD (*FnDelete)(void *, const WCHAR *);
typedef DWORD (*FnRename)(void *, const WCHAR *, const WCHAR *);
typedef DWORD (*FnIoctl)(HRASCONN, DWORD, void *, DWORD, void *, DWORD *);

// Ordinal 14: PPP FCS-16 table. Unused on the ethernet path; exported (DATA) as a
// zeroed table so any importer resolves.
WORD FCSTable[256];

// Tracks whether the title has an active "connection" (dialed and not hung up), so
// status/hangup behave consistently across a dial->use->hangup->redial cycle. The
// underlying ethernet link is always up; this is just the RAS-level connection state.
static int s_bDialed = 0;

// Ordinal 2: the one that matters - make the IE-style dial complete.
DWORD AfdRasDial(void *pvExt, const WCHAR *pszPhonebook, void *pvParams, DWORD dwNotifierType,
                 void *pvNotifier, HRASCONN *phConn)
{
	if (g_nUseDial && g_apfnDialRas[2])
	{
		// Modem: the original driver dials + runs PPP. The DC-era IPCP doesn't carry DNS (it
		// came from the system-flash ISP config), and our InterfaceInitialize is delegated so
		// NetifOnLease/WriteDnsServers never ran - so write a resolver here (RAS-entry DNS ->
		// flashrom -> public fallback) into HKLM\Comm so gethostbyname works over the PPP link.
		DWORD dwRc = ((FnDial)g_apfnDialRas[2])(pvExt, pszPhonebook, pvParams, dwNotifierType,
		                                        pvNotifier, phConn);
		NetifApplyDns();
		return dwRc;
	}
	(void)pvExt;
	(void)pszPhonebook;
	(void)pvParams;
	s_bDialed = 1;
	NetifApplyDns(); // honor the entry's Primary/Secondary DNS
	if (phConn)
		*phConn = (HRASCONN)1;
	if (dwNotifierType == 0xFFFFFFFF && pvNotifier) // notifier is a window handle
		PostMessageW((HWND)pvNotifier, WM_RASDIALEVENT, RASCS_Connected, 0);
	return 0; // SUCCESS
}

// No modem to hang up - we keep the ethernet link up - but clear the RAS state so the
// title sees a clean disconnect (and re-dials cleanly next time).
DWORD AfdRasHangUp(HRASCONN h)
{
	if (g_nUseDial && g_apfnDialRas[8])
		return ((FnHangUp)g_apfnDialRas[8])(h);
	(void)h;
	s_bDialed = 0;
	return 0;
}

// IsConnect() path: report the live connection if dialed (so the app reuses it), else
// none (so it dials, which then "connects" via AfdRasDial).
DWORD AfdRasEnumConnections(void *pvConns, DWORD *pcb, DWORD *pcCount)
{
	if (g_nUseDial && g_apfnDialRas[3])
		return ((FnEnumConn)g_apfnDialRas[3])(pvConns, pcb, pcCount);
	(void)pvConns;
	(void)pcb;
	if (pcCount)
		*pcCount = s_bDialed ? 1 : 0;
	return 0;
}

DWORD AfdRasGetConnectStatus(HRASCONN h, RASCONNSTATUS *pStatus)
{
	if (g_nUseDial && g_apfnDialRas[5])
		return ((FnConnStat)g_apfnDialRas[5])(h, pStatus);
	(void)h;
	if (pStatus)
	{
		pStatus->rasconnstate = s_bDialed ? RASCS_Connected : RASCS_Disconnected;
		pStatus->dwError = 0;
	}
	return 0;
}

// ---- RAS phonebook: registry-backed (HKLM\Comm\RasBook\<entry>), matching the stock
// mppp.dll (rasreg.c, reversed in Ghidra). The original stores dial params as values
// User/Domain/Password and the RASENTRY blob as "Entry", and AfdRasEnumEntries
// auto-creates a default "Desktop" entry when the book is empty (AfdRasMakeDefault).
// Our earlier stubs returned 0 entries, so a dial-era title (e.g. 4x4 Evolution) saw
// "No dialup properties have been setup". We replicate the registry behaviour; the
// actual dial is still AfdRasDial's instant-connect (the link is already up), so we
// pass the password through verbatim (no de/encryption needed).
#define RASBOOK L"Comm\\RasBook"

static LONG OpenBook(REGSAM sam, HKEY *ph)
{
	return RegOpenKeyExW(HKEY_LOCAL_MACHINE, RASBOOK, 0, sam, ph);
}

static LONG OpenEntry(const WCHAR *pszName, BOOL bCreate, HKEY *ph)
{
	WCHAR achPath[96];
	DWORD dwDisp;
	lstrcpyW(achPath, RASBOOK);
	lstrcatW(achPath, L"\\");
	lstrcatW(achPath, pszName ? pszName : L"Desktop");
	if (bCreate)
		return RegCreateKeyExW(HKEY_LOCAL_MACHINE, achPath, 0, NULL, 0, 0, NULL, ph, &dwDisp);
	return RegOpenKeyExW(HKEY_LOCAL_MACHINE, achPath, 0, KEY_ALL_ACCESS, ph);
}

// Stock AfdRasMakeDefault: if the book has no entries, create "Desktop" so the title
// always finds a configured connection. (The user can then edit it in-game.)
static void EnsureDefaultEntry(void)
{
	HKEY h, e;
	DWORD bHave = 0;
	if (OpenBook(KEY_READ, &h) == ERROR_SUCCESS)
	{
		WCHAR achName[64];
		DWORD nl = 64;
		if (RegEnumKeyExW(h, 0, achName, &nl, 0, 0, 0, 0) == ERROR_SUCCESS)
			bHave = 1;
		RegCloseKey(h);
	}
	if (!bHave && OpenEntry(L"Desktop", TRUE, &e) == ERROR_SUCCESS)
		RegCloseKey(e);
}

static void ReadVal(HKEY h, const WCHAR *pszVal, void *pvOut, int cb)
{
	DWORD t, n = cb;
	((BYTE *)pvOut)[0] = 0;
	RegQueryValueExW(h, pszVal, 0, &t, (BYTE *)pvOut, &n);
}

static void CopyN(WCHAR *pszDst, const WCHAR *pszSrc,
                  int n) // bounded WCHAR copy (no lstrcpynW on CE2.12)
{
	int i;
	for (i = 0; i < n - 1 && pszSrc[i]; i++)
		pszDst[i] = pszSrc[i];
	pszDst[i] = 0;
}

DWORD AfdRasEnumEntries(void *pvA, void *pvB, RASENTRYNAME *pEntries, DWORD *pcb, DWORD *pcCount)
{
	HKEY h;
	DWORD i = 0, nGot = 0;
	if (g_nUseDial && g_apfnDialRas[4])
		return ((FnEnumEnt)g_apfnDialRas[4])(pvA, pvB, pEntries, pcb, pcCount);
	(void)pvA;
	(void)pvB;
	if (pcCount)
		*pcCount = 0;
	EnsureDefaultEntry();
	if (OpenBook(KEY_READ, &h) != ERROR_SUCCESS)
	{
		if (pcb)
			*pcb = 0;
		return 0;
	}
	for (;;)
	{
		WCHAR achName[64];
		DWORD nl = 64;
		if (RegEnumKeyExW(h, i, achName, &nl, 0, 0, 0, 0) != ERROR_SUCCESS)
			break;
		if (pEntries && pcb && (nGot + 1) * sizeof(RASENTRYNAME) <= *pcb)
		{
			pEntries[nGot].dwSize = sizeof(RASENTRYNAME); // 0x30
			CopyN(pEntries[nGot].szEntryName, achName, RAS_MaxEntryName + 1);
			nGot++;
		}
		i++;
	}
	RegCloseKey(h);
	if (pcCount)
		*pcCount = i;
	if (pcb)
		*pcb = i * sizeof(RASENTRYNAME);
	return 0;
}

DWORD AfdRasGetEntryDialParams(void *pvReserved, RASDIALPARAMS *pParams, BOOL *pbPw)
{
	HKEY h;
	if (g_nUseDial && g_apfnDialRas[6])
		return ((FnGetDial)g_apfnDialRas[6])(pvReserved, pParams, pbPw);
	(void)pvReserved;
	if (pbPw)
		*pbPw = FALSE;
	if (!pParams)
		return ERROR_INVALID_PARAMETER;
	pParams->szUserName[0] = pParams->szPassword[0] = pParams->szDomain[0] = 0;
	if (OpenEntry(pParams->szEntryName, FALSE, &h) == ERROR_SUCCESS)
	{
		ReadVal(h, L"User", pParams->szUserName, (UNLEN + 1) * sizeof(WCHAR));
		ReadVal(h, L"Domain", pParams->szDomain, (DNLEN + 1) * sizeof(WCHAR));
		ReadVal(h, L"Password", pParams->szPassword, (PWLEN + 1) * sizeof(WCHAR));
		if (pbPw)
			*pbPw = (pParams->szPassword[0] != 0);
		RegCloseKey(h);
	}
	return 0;
}

DWORD AfdRasSetEntryDialParams(void *pvReserved, RASDIALPARAMS *pParams, BOOL bDel)
{
	HKEY h;
	if (g_nUseDial && g_apfnDialRas[11])
		return ((FnSetDial)g_apfnDialRas[11])(pvReserved, pParams, bDel);
	(void)pvReserved;
	(void)bDel;
	if (!pParams)
		return ERROR_INVALID_PARAMETER;
	if (OpenEntry(pParams->szEntryName, TRUE, &h) != ERROR_SUCCESS)
		return ERROR_ACCESS_DENIED;
	RegSetValueExW(h, L"User", 0, REG_SZ, (const BYTE *)pParams->szUserName,
	               (lstrlenW(pParams->szUserName) + 1) * sizeof(WCHAR));
	RegSetValueExW(h, L"Domain", 0, REG_SZ, (const BYTE *)pParams->szDomain,
	               (lstrlenW(pParams->szDomain) + 1) * sizeof(WCHAR));
	RegSetValueExW(h, L"Password", 0, REG_BINARY, (const BYTE *)pParams->szPassword,
	               (lstrlenW(pParams->szPassword) + 1) * sizeof(WCHAR));
	RegCloseKey(h);
	return 0;
}

DWORD AfdRasSetEntryProperties(void *pvReserved, const WCHAR *pszEntry, void *pvProps, DWORD cb,
                               void *pvDev, DWORD dcb)
{
	HKEY h;
	if (g_nUseDial && g_apfnDialRas[12])
		return ((FnSetProp)g_apfnDialRas[12])(pvReserved, pszEntry, pvProps, cb, pvDev, dcb);
	(void)pvReserved;
	(void)pvDev;
	(void)dcb;
	if (OpenEntry(pszEntry, TRUE, &h) != ERROR_SUCCESS)
		return ERROR_ACCESS_DENIED;
	if (pvProps && cb)
		RegSetValueExW(h, L"Entry", 0, REG_BINARY, (const BYTE *)pvProps, cb);
	RegCloseKey(h);
	return 0;
}

DWORD AfdRasGetEntryProperties(void *pvReserved, const WCHAR *pszEntry, void *pvProps, DWORD *pcb,
                               void *pvDev, DWORD *pdcb)
{
	HKEY h;
	DWORD t;
	if (g_nUseDial && g_apfnDialRas[7])
		return ((FnGetProp)g_apfnDialRas[7])(pvReserved, pszEntry, pvProps, pcb, pvDev, pdcb);
	(void)pvReserved;
	(void)pvDev;
	if (pdcb)
		*pdcb = 0;
	if (OpenEntry(pszEntry, FALSE, &h) != ERROR_SUCCESS)
	{
		if (pcb)
			*pcb = 0;
		return 0;
	}
	if (pcb && pvProps)
		RegQueryValueExW(h, L"Entry", 0, &t, (BYTE *)pvProps, pcb);
	RegCloseKey(h);
	return 0;
}

DWORD AfdRasValidateEntryName(void *pvReserved, const WCHAR *pszEntry)
{
	HKEY h;
	if (g_nUseDial && g_apfnDialRas[13])
		return ((FnValidate)g_apfnDialRas[13])(pvReserved, pszEntry);
	(void)pvReserved;
	if (OpenEntry(pszEntry, FALSE, &h) == ERROR_SUCCESS)
	{
		RegCloseKey(h);
		return 183;
	} // ERROR_ALREADY_EXISTS
	return 0;
}

DWORD AfdRasDeleteEntry(void *pvReserved, const WCHAR *pszEntry)
{
	WCHAR achPath[96];
	if (g_nUseDial && g_apfnDialRas[1])
		return ((FnDelete)g_apfnDialRas[1])(pvReserved, pszEntry);
	(void)pvReserved;
	lstrcpyW(achPath, RASBOOK);
	lstrcatW(achPath, L"\\");
	lstrcatW(achPath, pszEntry ? pszEntry : L"");
	RegDeleteKeyW(HKEY_LOCAL_MACHINE, achPath);
	return 0;
}

DWORD AfdRasRenameEntry(void *pvReserved, const WCHAR *pszOld, const WCHAR *pszNew)
{
	if (g_nUseDial && g_apfnDialRas[10])
		return ((FnRename)g_apfnDialRas[10])(pvReserved, pszOld, pszNew);
	(void)pvReserved;
	(void)pszOld;
	(void)pszNew;
	return 0;
}
DWORD AfdRasIOControl(HRASCONN h, DWORD dwCode, void *pvIn, DWORD dwInLen, void *pvOut,
                      DWORD *pdwOutLen)
{
	if (g_nUseDial && g_apfnDialRas[9])
		return ((FnIoctl)g_apfnDialRas[9])(h, dwCode, pvIn, dwInLen, pvOut, pdwOutLen);
	(void)h;
	(void)dwCode;
	(void)pvIn;
	(void)dwInLen;
	(void)pvOut;
	(void)pdwOutLen;
	return 0;
}
