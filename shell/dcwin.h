//
// dcwin.h - DCWin compositor protocol, shared between the shell (server) and
// client apps. Lives in a named shared section ("DCWIN") that CE maps at the
// same virtual address in every process. Poll-based (no cross-process events):
// clients publish a draw-command list per window; the shell composites them and
// pushes input events back into each window's input ring.
//
#ifndef DCWIN_H
#define DCWIN_H

#include <windows.h>

#define DCWIN_SECTION  L"DCWIN"
#define DCWIN_MAGIC    0x44435731   // 'DCW1'
#define DCWIN_MAXWIN   4
#define DCWIN_MAXCMD   48
#define DCWIN_MAXIN    16

#define DCOP_NONE  0
#define DCOP_FILL  1   // x,y,w,h + color
#define DCOP_TEXT  2   // x,y + color (fg) + color2 (bg) + text
#define DCOP_ICON  3   // x,y + color = icon id (ICON_* below)

// shell icon ids (shared so clients can reference them via DCOP_ICON / DCWinIcon)
#define ICON_COMPUTER  0
#define ICON_DRIVE     1
#define ICON_FOLDER    2
#define ICON_APP       3
#define ICON_CLOCK     4
#define ICON_FILE      5
#define ICON_SWIRL     6   // Dreamcast swirl (My Dreamcast)
#define ICON_COUNT     7

typedef struct
{
    DWORD op;
    LONG  x, y, w, h;
    DWORD color;
    DWORD color2;
    WCHAR text[40];
} DcCmd;

typedef struct
{
    DWORD type;     // 1 = key down
    DWORD key;      // VK code
} DcInput;

typedef struct
{
    DWORD inUse;            // 0 = free slot
    DWORD ownerPid;
    LONG  x, y, w, h;       // client-area position + size on the desktop
    WCHAR title[40];
    DWORD icon;             // ICON_* for the title bar / taskbar
    DWORD wantClose;        // shell -> client: please close
    DWORD gen;              // client bumps after writing a full frame of commands
    DWORD cmdCount;
    DcCmd cmd[DCWIN_MAXCMD];
    DWORD inHead;           // shell writes here
    DWORD inTail;           // client reads here
    DcInput in[DCWIN_MAXIN];
} DcWindow;

typedef struct
{
    DWORD    magic;
    DWORD    execSeq;          // client bumps to ask the shell to launch execPath
    WCHAR    execPath[260];
    DcWindow win[DCWIN_MAXWIN];
} DcShared;

#endif // DCWIN_H
