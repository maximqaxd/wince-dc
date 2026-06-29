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

// Ordinal 14: PPP FCS-16 table. Unused on the ethernet path; exported (DATA) as a
// zeroed table so any importer resolves.
WORD FCSTable[256];

// Tracks whether the title has an active "connection" (dialed and not hung up), so
// status/hangup behave consistently across a dial->use->hangup->redial cycle. The
// underlying ethernet link is always up; this is just the RAS-level connection state.
static int g_dialed = 0;

// Ordinal 2: the one that matters - make the IE-style dial complete.
DWORD AfdRasDial(void *ext, const WCHAR *phonebook, void *params,
                 DWORD notifierType, void *notifier, HRASCONN *phConn)
{
    (void)ext; (void)phonebook; (void)params;
    g_dialed = 1;
    NetifApplyDns();                                      // honor the entry's Primary/Secondary DNS
    if (phConn) *phConn = (HRASCONN)1;
    if (notifierType == 0xFFFFFFFF && notifier)           // notifier is a window handle
        PostMessageW((HWND)notifier, WM_RASDIALEVENT, RASCS_Connected, 0);
    return 0;                                             // SUCCESS
}

// No modem to hang up - we keep the ethernet link up - but clear the RAS state so the
// title sees a clean disconnect (and re-dials cleanly next time).
DWORD AfdRasHangUp(HRASCONN h) { (void)h; g_dialed = 0; return 0; }

// IsConnect() path: report the live connection if dialed (so the app reuses it), else
// none (so it dials, which then "connects" via AfdRasDial).
DWORD AfdRasEnumConnections(void *conns, DWORD *cb, DWORD *count)
{ (void)conns; (void)cb; if (count) *count = g_dialed ? 1 : 0; return 0; }

DWORD AfdRasGetConnectStatus(HRASCONN h, RASCONNSTATUS *st)
{ (void)h; if (st) { st->rasconnstate = g_dialed ? RASCS_Connected : RASCS_Disconnected; st->dwError = 0; } return 0; }

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
{ return RegOpenKeyExW(HKEY_LOCAL_MACHINE, RASBOOK, 0, sam, ph); }

static LONG OpenEntry(const WCHAR *name, BOOL create, HKEY *ph)
{
    WCHAR path[96];
    DWORD disp;
    lstrcpyW(path, RASBOOK); lstrcatW(path, L"\\");
    lstrcatW(path, name ? name : L"Desktop");
    if (create)
        return RegCreateKeyExW(HKEY_LOCAL_MACHINE, path, 0, NULL, 0, 0, NULL, ph, &disp);
    return RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_ALL_ACCESS, ph);
}

// Stock AfdRasMakeDefault: if the book has no entries, create "Desktop" so the title
// always finds a configured connection. (The user can then edit it in-game.)
static void EnsureDefaultEntry(void)
{
    HKEY  h, e;
    DWORD have = 0;
    if (OpenBook(KEY_READ, &h) == ERROR_SUCCESS)
    {
        WCHAR nm[64]; DWORD nl = 64;
        if (RegEnumKeyExW(h, 0, nm, &nl, 0, 0, 0, 0) == ERROR_SUCCESS) have = 1;
        RegCloseKey(h);
    }
    if (!have && OpenEntry(L"Desktop", TRUE, &e) == ERROR_SUCCESS) RegCloseKey(e);
}

static void ReadVal(HKEY h, const WCHAR *v, void *out, int cb)
{ DWORD t, n = cb; ((BYTE *)out)[0] = 0; RegQueryValueExW(h, v, 0, &t, (BYTE *)out, &n); }

static void CopyN(WCHAR *d, const WCHAR *s, int n)   // bounded WCHAR copy (no lstrcpynW on CE2.12)
{ int i; for (i = 0; i < n - 1 && s[i]; i++) d[i] = s[i]; d[i] = 0; }

DWORD AfdRasEnumEntries(void *a, void *b, RASENTRYNAME *entries, DWORD *cb, DWORD *count)
{
    HKEY  h;
    DWORD i = 0, got = 0;
    (void)a; (void)b;
    if (count) *count = 0;
    EnsureDefaultEntry();
    if (OpenBook(KEY_READ, &h) != ERROR_SUCCESS) { if (cb) *cb = 0; return 0; }
    for (;;)
    {
        WCHAR nm[64]; DWORD nl = 64;
        if (RegEnumKeyExW(h, i, nm, &nl, 0, 0, 0, 0) != ERROR_SUCCESS) break;
        if (entries && cb && (got + 1) * sizeof(RASENTRYNAME) <= *cb)
        {
            entries[got].dwSize = sizeof(RASENTRYNAME);   // 0x30
            CopyN(entries[got].szEntryName, nm, RAS_MaxEntryName + 1);
            got++;
        }
        i++;
    }
    RegCloseKey(h);
    if (count) *count = i;
    if (cb)    *cb = i * sizeof(RASENTRYNAME);
    return 0;
}

DWORD AfdRasGetEntryDialParams(void *reserved, RASDIALPARAMS *p, BOOL *pw)
{
    HKEY h;
    (void)reserved;
    if (pw) *pw = FALSE;
    if (!p) return ERROR_INVALID_PARAMETER;
    p->szUserName[0] = p->szPassword[0] = p->szDomain[0] = 0;
    if (OpenEntry(p->szEntryName, FALSE, &h) == ERROR_SUCCESS)
    {
        ReadVal(h, L"User",     p->szUserName, (UNLEN + 1) * sizeof(WCHAR));
        ReadVal(h, L"Domain",   p->szDomain,   (DNLEN + 1) * sizeof(WCHAR));
        ReadVal(h, L"Password", p->szPassword, (PWLEN + 1) * sizeof(WCHAR));
        if (pw) *pw = (p->szPassword[0] != 0);
        RegCloseKey(h);
    }
    return 0;
}

DWORD AfdRasSetEntryDialParams(void *reserved, RASDIALPARAMS *p, BOOL del)
{
    HKEY h;
    (void)reserved; (void)del;
    if (!p) return ERROR_INVALID_PARAMETER;
    if (OpenEntry(p->szEntryName, TRUE, &h) != ERROR_SUCCESS) return ERROR_ACCESS_DENIED;
    RegSetValueExW(h, L"User",     0, REG_SZ,     (const BYTE *)p->szUserName, (lstrlenW(p->szUserName) + 1) * sizeof(WCHAR));
    RegSetValueExW(h, L"Domain",   0, REG_SZ,     (const BYTE *)p->szDomain,   (lstrlenW(p->szDomain)   + 1) * sizeof(WCHAR));
    RegSetValueExW(h, L"Password", 0, REG_BINARY, (const BYTE *)p->szPassword, (lstrlenW(p->szPassword) + 1) * sizeof(WCHAR));
    RegCloseKey(h);
    return 0;
}

DWORD AfdRasSetEntryProperties(void *reserved, const WCHAR *e, void *props, DWORD cb, void *dev, DWORD dcb)
{
    HKEY h;
    (void)reserved; (void)dev; (void)dcb;
    if (OpenEntry(e, TRUE, &h) != ERROR_SUCCESS) return ERROR_ACCESS_DENIED;
    if (props && cb) RegSetValueExW(h, L"Entry", 0, REG_BINARY, (const BYTE *)props, cb);
    RegCloseKey(h);
    return 0;
}

DWORD AfdRasGetEntryProperties(void *reserved, const WCHAR *e, void *props, DWORD *cb, void *dev, DWORD *dcb)
{
    HKEY  h; DWORD t;
    (void)reserved; (void)dev;
    if (dcb) *dcb = 0;
    if (OpenEntry(e, FALSE, &h) != ERROR_SUCCESS) { if (cb) *cb = 0; return 0; }
    if (cb && props) RegQueryValueExW(h, L"Entry", 0, &t, (BYTE *)props, cb);
    RegCloseKey(h);
    return 0;
}

DWORD AfdRasValidateEntryName(void *reserved, const WCHAR *e)
{
    HKEY h;
    (void)reserved;
    if (OpenEntry(e, FALSE, &h) == ERROR_SUCCESS) { RegCloseKey(h); return 183; } // ERROR_ALREADY_EXISTS
    return 0;
}

DWORD AfdRasDeleteEntry(void *reserved, const WCHAR *e)
{
    WCHAR path[96];
    (void)reserved;
    lstrcpyW(path, RASBOOK); lstrcatW(path, L"\\"); lstrcatW(path, e ? e : L"");
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, path);
    return 0;
}

DWORD AfdRasRenameEntry(void *reserved, const WCHAR *o, const WCHAR *n) { (void)reserved; (void)o; (void)n; return 0; }
DWORD AfdRasIOControl(HRASCONN h, DWORD code, void *in, DWORD inl, void *out, DWORD *outl)
{ (void)h; (void)code; (void)in; (void)inl; (void)out; (void)outl; return 0; }
