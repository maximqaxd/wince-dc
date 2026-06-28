//
// syslog.h - a tiny system-wide log, viewable in the DCWin "System Log" window. Any
// process (the shell, the dcw* apps, even a WDM driver running in wdevice.exe) can append
// lines; they land in a named shared section and the dcwlog app renders them. Replaces the
// framebuffer FbLog trail - works with a console-less retail image and is readable on-screen.
//
#ifndef SYSLOG_H
#define SYSLOG_H

#include <windows.h>

#define SYSLOG_SECTION  L"DCSYSLOG"
#define SYSLOG_MAGIC    0x44434C47u     // 'DCLG'
#define SYSLOG_LINES    256             // ring capacity (power-of-two-ish; wraps)
#define SYSLOG_LINELEN  80

typedef struct
{
    DWORD        magic;
    volatile LONG head;                 // total lines ever written (monotonic); line = head-1 newest
    WCHAR        line[SYSLOG_LINES][SYSLOG_LINELEN];
} SysLogShared;

void          SysLogW(const WCHAR *s);          // append one line
void          SysLog(const WCHAR *fmt, ...);    // printf-style (wsprintf formatting)
SysLogShared *SysLogMap(int create);            // map the section (viewer passes create=0)

#endif // SYSLOG_H
