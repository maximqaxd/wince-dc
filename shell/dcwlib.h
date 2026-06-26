//
// dcwlib.h - DCWin client library. An app links this to become a windowed program
// the dcshell compositor draws and routes input to. Typical use:
//
//     DCWin *w = DCWinOpen(x, y, cw, ch, L"Title");
//     for (;;) {
//         DWORD key;
//         while (DCWinPollKey(w, &key)) { ...handle key... }
//         DCWinBeginFrame(w);
//         DCWinFill(w, 0,0, cw,ch, RGB(192,192,192));
//         DCWinText(w, 8,8, RGB(0,0,0), RGB(192,192,192), L"hello");
//         DCWinEndFrame(w);
//         if (DCWinShouldClose(w)) break;
//         Sleep(33);
//     }
//     DCWinClose(w);
//
#ifndef DCWLIB_H
#define DCWLIB_H

#include "dcwin.h"

typedef struct DCWin DCWin;

DCWin *DCWinOpen(int x, int y, int w, int h, const WCHAR *title, int iconId);  // NULL on failure
void   DCWinBeginFrame(DCWin *win);
void   DCWinFill(DCWin *win, int x, int y, int w, int h, COLORREF color);
void   DCWinText(DCWin *win, int x, int y, COLORREF fg, COLORREF bg, const WCHAR *text);
void   DCWinIcon(DCWin *win, int x, int y, int iconId);
void   DCWinEndFrame(DCWin *win);          // publishes the frame atomically
int    DCWinPollKey(DCWin *win, DWORD *key);   // returns 1 and a VK if one was queued
int    DCWinShouldClose(DCWin *win);       // shell asked us to close
void   DCWinExec(DCWin *win, const WCHAR *path);  // ask the shell to launch an app
void   DCWinClose(DCWin *win);

#endif // DCWLIB_H
