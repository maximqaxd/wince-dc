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

// Ordinal 14: PPP FCS-16 table. Unused on the ethernet path; exported (DATA) as a
// zeroed table so any importer resolves.
WORD FCSTable[256];

// Ordinal 2: the one that matters - make the IE-style dial complete.
DWORD AfdRasDial(void *ext, const WCHAR *phonebook, void *params,
                 DWORD notifierType, void *notifier, HRASCONN *phConn)
{
    (void)ext; (void)phonebook; (void)params;
    if (phConn) *phConn = (HRASCONN)1;
    if (notifierType == 0xFFFFFFFF && notifier)           // notifier is a window handle
        PostMessageW((HWND)notifier, WM_RASDIALEVENT, RASCS_Connected, 0);
    return 0;                                             // SUCCESS
}

DWORD AfdRasHangUp(HRASCONN h) { (void)h; return 0; }

// IsConnect() path: report no enumerated connections (so the app dials, which then
// "connects" via AfdRasDial), but report Connected if asked about a handle.
DWORD AfdRasEnumConnections(void *conns, DWORD *cb, DWORD *count)
{ (void)conns; (void)cb; if (count) *count = 0; return 0; }

DWORD AfdRasGetConnectStatus(HRASCONN h, RASCONNSTATUS *st)
{ (void)h; if (st) { st->rasconnstate = RASCS_Connected; st->dwError = 0; } return 0; }

// Phonebook/entry management - succeed silently (we ignore the modem entry).
DWORD AfdRasEnumEntries(void *a, void *b, void *entries, DWORD *cb, DWORD *count)
{ (void)a; (void)b; (void)entries; (void)cb; if (count) *count = 0; return 0; }
DWORD AfdRasGetEntryDialParams(const WCHAR *pb, void *p, BOOL *pw)
{ (void)pb; (void)p; if (pw) *pw = FALSE; return 0; }
DWORD AfdRasSetEntryDialParams(const WCHAR *pb, void *p, BOOL del) { (void)pb; (void)p; (void)del; return 0; }
DWORD AfdRasSetEntryProperties(const WCHAR *pb, const WCHAR *e, void *p, DWORD cb, void *dev, DWORD dcb)
{ (void)pb; (void)e; (void)p; (void)cb; (void)dev; (void)dcb; return 0; }
DWORD AfdRasGetEntryProperties(const WCHAR *pb, const WCHAR *e, void *p, DWORD *cb, void *dev, DWORD *dcb)
{ (void)pb; (void)e; (void)p; (void)cb; (void)dev; (void)dcb; return 0; }
DWORD AfdRasValidateEntryName(const WCHAR *pb, const WCHAR *e) { (void)pb; (void)e; return 0; }
DWORD AfdRasDeleteEntry(const WCHAR *pb, const WCHAR *e) { (void)pb; (void)e; return 0; }
DWORD AfdRasRenameEntry(const WCHAR *pb, const WCHAR *o, const WCHAR *n) { (void)pb; (void)o; (void)n; return 0; }
DWORD AfdRasIOControl(HRASCONN h, DWORD code, void *in, DWORD inl, void *out, DWORD *outl)
{ (void)h; (void)code; (void)in; (void)inl; (void)out; (void)outl; return 0; }
