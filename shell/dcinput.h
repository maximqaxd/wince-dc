//
// dcinput.h - DirectInput layer for the shell: polled keyboard (low-latency,
// replaces WM_KEYDOWN) + a pointer from either a mouse (DC Maple / host mouse,
// relative deltas) or the controller (analog stick moves the cursor, buttons =
// click).
//
#ifndef DCINPUT_H
#define DCINPUT_H

#include <windows.h>

BOOL DInInit(HWND hwnd);     // TRUE if a DI keyboard was acquired (use it, not WM_KEYDOWN)
void DInShutdown(void);
void DInUpdate(void);        // poll all devices; call once per loop

int  DInNextKey(DWORD *vk);  // 1 + VK for each queued key-down (edge / auto-repeat)
int  DInHasPointer(void);    // TRUE if a mouse or controller pointer is active
void DInCursor(int *x, int *y);
int  DInTookClick(void);     // TRUE once per mouse click (cursor paradigm)
int  DInTookActivate(void);  // TRUE once per controller face-button press (-> Enter)

// GWES WM_MOUSE* fallback feed (the shell forwards window mouse messages here in
// case the DC mouse reaches WinCE through the window queue, not DirectInput).
void DInSetCursor(int x, int y);   // absolute client coords
void DInPostClick(void);

#endif // DCINPUT_H
