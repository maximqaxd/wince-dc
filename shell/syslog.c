//
// syslog.c - see syslog.h. Lazily creates/maps the DCSYSLOG shared section and appends
// lines into its ring. Self-contained (just coredll) so it compiles into any module.
//
#include <stdarg.h>
#include "syslog.h"

static SysLogShared *g_sl;
static HANDLE        g_map;

SysLogShared *SysLogMap(int create)
{
    if (g_sl) return g_sl;
    // CreateFileMapping on the same name returns the one global section (zero-filled on first
    // create), so every process - and a WDM driver in wdevice.exe - shares the same ring.
    g_map = CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(SysLogShared), SYSLOG_SECTION);
    if (!g_map) return NULL;
    g_sl = (SysLogShared *)MapViewOfFile(g_map, FILE_MAP_WRITE, 0, 0, sizeof(SysLogShared));
    if (g_sl && create && g_sl->magic != SYSLOG_MAGIC) g_sl->magic = SYSLOG_MAGIC;  // first writer stamps it
    return g_sl;
}

void SysLogW(const WCHAR *s)
{
    SysLogShared *sl = SysLogMap(1);
    LONG  idx;
    int   i;
    WCHAR *d;
    if (!sl || !s) return;
    idx = InterlockedIncrement(&sl->head) - 1;          // claim a slot
    d = sl->line[(DWORD)idx % SYSLOG_LINES];
    for (i = 0; i < SYSLOG_LINELEN - 1 && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

void SysLog(const WCHAR *fmt, ...)
{
    WCHAR   buf[SYSLOG_LINELEN];
    va_list ap;
    va_start(ap, fmt);
    wvsprintfW(buf, fmt, ap);
    va_end(ap);
    SysLogW(buf);
}
