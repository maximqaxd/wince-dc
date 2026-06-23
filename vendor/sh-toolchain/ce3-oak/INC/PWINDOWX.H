/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pwindowx.h

Abstract:

Private portion of windowx.h - Macro APIs, window message crackers, and
control APIs

Notes: 


--*/
    
    #ifndef _PRIV_INC_WINDOWSX
    #define _PRIV_INC_WINDOWSX

    
    #ifdef __cplusplus
    extern "C" {            /* Assume C declarations for C++ */
    #endif /* __cplusplus */
    
    
 #define     GlobalAllocPtr(flags, cb)        \
                 (GlobalAlloc((flags), (cb)))
 #define     GlobalReAllocPtr(lp, cbNew, flags)       \
                 (GlobalReAlloc(lp , (cbNew), (flags)))
 
    
 #define     GetWindowInstance(hwnd) ((HMODULE)GetWindowLong(hwnd, GWL_HINSTANCE))
 
    
 #define     GetFirstChild(hwnd)     GetTopWindow(hwnd)
    
 #define     SetWindowRedraw(hwnd, fRedraw)  \
                     ((void)SendMessage(hwnd, WM_SETREDRAW, (WPARAM)(BOOL)(fRedraw), 0L))
 
    
 #define     IsMinimized(hwnd)        IsIconic(hwnd)
 #define     IsMaximized(hwnd)        IsZoomed(hwnd)
 #define     IsRestored(hwnd)    ((GetWindowStyle(hwnd) & (WS_MINIMIZE | WS_MAXIMIZE)) == 0L)
 
    
 #define     DefDlgProcEx(hwnd, msg, wParam, lParam, pfRecursion) \
     (*(pfRecursion) = TRUE, DefDlgProc(hwnd, msg, wParam, lParam))
 
 #define     CheckDefDlgRecursion(pfRecursion) \
     if (*(pfRecursion)) { *(pfRecursion) = FALSE; return FALSE; }
 
    /****** Message crackers ****************************************************/
    
    
 /* void Cls_OnCompacting(HWND hwnd, UINT compactRatio) */
 #define HANDLE_WM_COMPACTING(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (UINT)(wParam)), 0L)
 #define FORWARD_WM_COMPACTING(hwnd, compactRatio, fn) \
     (void)(fn)((hwnd), WM_COMPACTING, (WPARAM)(UINT)(compactRatio), 0L)
 
 /* void Cls_OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName) */
 #define HANDLE_WM_WININICHANGE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (LPCTSTR)(lParam)), 0L)
 #define FORWARD_WM_WININICHANGE(hwnd, lpszSectionName, fn) \
     (void)(fn)((hwnd), WM_WININICHANGE, 0L, (LPARAM)(LPCTSTR)(lpszSectionName))
 
 /* void Cls_OnSysColorChange(HWND hwnd) */
 #define HANDLE_WM_SYSCOLORCHANGE(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_SYSCOLORCHANGE(hwnd, fn) \
     (void)(fn)((hwnd), WM_SYSCOLORCHANGE, 0L, 0L)
 
 /* BOOL Cls_OnQueryNewPalette(HWND hwnd) */
 #define HANDLE_WM_QUERYNEWPALETTE(hwnd, wParam, lParam, fn) \
     MAKELRESULT((BOOL)(fn)(hwnd), 0L)
 #define FORWARD_WM_QUERYNEWPALETTE(hwnd, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_QUERYNEWPALETTE, 0L, 0L)
 
 /* void Cls_OnPaletteIsChanging(HWND hwnd, HWND hwndPaletteChange) */
 #define HANDLE_WM_PALETTEISCHANGING(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam)), 0L)
 #define FORWARD_WM_PALETTEISCHANGING(hwnd, hwndPaletteChange, fn) \
     (void)(fn)((hwnd), WM_PALETTEISCHANGING, (WPARAM)(HWND)(hwndPaletteChange), 0L)
 
 /* void Cls_OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange) */
 #define HANDLE_WM_PALETTECHANGED(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam)), 0L)
 #define FORWARD_WM_PALETTECHANGED(hwnd, hwndPaletteChange, fn) \
     (void)(fn)((hwnd), WM_PALETTECHANGED, (WPARAM)(HWND)(hwndPaletteChange), 0L)
 
    
 /* void Cls_OnSpoolerStatus(HWND hwnd, UINT status, int cJobInQueue) */
 #define HANDLE_WM_SPOOLERSTATUS(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (UINT)(wParam), (int)(short)LOWORD(lParam)), 0L)
 #define FORWARD_WM_SPOOLERSTATUS(hwnd, status, cJobInQueue, fn) \
     (void)(fn)((hwnd), WM_SPOOLERSTATUS, (WPARAM)(status), MAKELPARAM((cJobInQueue), 0))
 
 /* void Cls_OnDevModeChange(HWND hwnd, LPCTSTR lpszDeviceName) */
 #define HANDLE_WM_DEVMODECHANGE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (LPCTSTR)(lParam)), 0L)
 #define FORWARD_WM_DEVMODECHANGE(hwnd, lpszDeviceName, fn) \
     (void)(fn)((hwnd), WM_DEVMODECHANGE, 0L, (LPARAM)(LPCTSTR)(lpszDeviceName))
 
 /* void Cls_OnTimeChange(HWND hwnd) */
 #define HANDLE_WM_TIMECHANGE(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_TIMECHANGE(hwnd, fn) \
     (void)(fn)((hwnd), WM_TIMECHANGE, 0L, 0L)
 
 /* void Cls_OnPower(HWND hwnd, int code) */
 #define HANDLE_WM_POWER(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(wParam)), 0L)
 #define FORWARD_WM_POWER(hwnd, code, fn) \
     (void)(fn)((hwnd), WM_POWER, (WPARAM)(int)(code), 0L)
 
 /* BOOL Cls_OnQueryEndSession(HWND hwnd) */
 #define HANDLE_WM_QUERYENDSESSION(hwnd, wParam, lParam, fn) \
     MAKELRESULT((BOOL)(fn)(hwnd), 0L)
 #define FORWARD_WM_QUERYENDSESSION(hwnd, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_QUERYENDSESSION, 0L, 0L)
 
 /* void Cls_OnEndSession(HWND hwnd, BOOL fEnding) */
 #define HANDLE_WM_ENDSESSION(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (BOOL)(wParam)), 0L)
 #define FORWARD_WM_ENDSESSION(hwnd, fEnding, fn) \
     (void)(fn)((hwnd), WM_ENDSESSION, (WPARAM)(BOOL)(fEnding), 0L)
 
    
 /* This message is in Windows 3.1 only */
 /* void Cls_OnSystemError(HWND hwnd, int errCode) */
 #define HANDLE_WM_SYSTEMERROR(hwnd, wParam, lParam, fn) 0L
 #define FORWARD_WM_SYSTEMERROR(hwnd, errCode, fn) 0L
 
    
 /* BOOL Cls_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) */
 #define HANDLE_WM_NCCREATE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (LPCREATESTRUCT)(lParam))
 #define FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_NCCREATE, 0L, (LPARAM)(LPCREATESTRUCT)(lpCreateStruct))
 
    
 /* void Cls_OnNCDestroy(HWND hwnd) */
 #define HANDLE_WM_NCDESTROY(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_NCDESTROY(hwnd, fn) \
     (void)(fn)((hwnd), WM_NCDESTROY, 0L, 0L)
 
 /* void Cls_OnShowWindow(HWND hwnd, BOOL fShow, UINT status) */
 #define HANDLE_WM_SHOWWINDOW(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (BOOL)(wParam), (UINT)(lParam)), 0L)
 #define FORWARD_WM_SHOWWINDOW(hwnd, fShow, status, fn) \
     (void)(fn)((hwnd), WM_SHOWWINDOW, (WPARAM)(BOOL)(fShow), (LPARAM)(UINT)(status))
 
    
    
 /* BOOL Cls_OnWindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos) */
 #define HANDLE_WM_WINDOWPOSCHANGING(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (LPWINDOWPOS)(lParam))
 #define FORWARD_WM_WINDOWPOSCHANGING(hwnd, lpwpos, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_WINDOWPOSCHANGING, 0L, (LPARAM)(LPWINDOWPOS)(lpwpos))
 
    
 /* void Cls_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) */
 #define HANDLE_WM_GETMINMAXINFO(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (LPMINMAXINFO)(lParam)), 0L)
 #define FORWARD_WM_GETMINMAXINFO(hwnd, lpMinMaxInfo, fn) \
     (void)(fn)((hwnd), WM_GETMINMAXINFO, 0L, (LPARAM)(LPMINMAXINFO)(lpMinMaxInfo))
 
    
 /* BOOL Cls_OnIconEraseBkgnd(HWND hwnd, HDC hdc) */
 #define HANDLE_WM_ICONERASEBKGND(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (HDC)(wParam))
 #define FORWARD_WM_ICONERASEBKGND(hwnd, hdc, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_ICONERASEBKGND, (WPARAM)(HDC)(hdc), 0L)
 
 /* void Cls_OnNCPaint(HWND hwnd, HRGN hrgn) */
 #define HANDLE_WM_NCPAINT(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HRGN)(wParam)), 0L)
 #define FORWARD_WM_NCPAINT(hwnd, hrgn, fn) \
     (void)(fn)((hwnd), WM_NCPAINT, (WPARAM)(HRGN)(hrgn), 0L)
 
 /* UINT Cls_OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp) */
 #define HANDLE_WM_NCCALCSIZE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)((hwnd), (BOOL)(0), (NCCALCSIZE_PARAMS *)(lParam))
 #define FORWARD_WM_NCCALCSIZE(hwnd, fCalcValidRects, lpcsp, fn) \
     (UINT)(DWORD)(fn)((hwnd), WM_NCCALCSIZE, 0L, (LPARAM)(NCCALCSIZE_PARAMS *)(lpcsp))
 
 /* UINT Cls_OnNCHitTest(HWND hwnd, int x, int y) */
 #define HANDLE_WM_NCHITTEST(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam))
 #define FORWARD_WM_NCHITTEST(hwnd, x, y, fn) \
     (UINT)(DWORD)(fn)((hwnd), WM_NCHITTEST, 0L, MAKELPARAM((x), (y)))
 
 /* HICON Cls_OnQueryDragIcon(HWND hwnd) */
 #define HANDLE_WM_QUERYDRAGICON(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)(hwnd)
 #define FORWARD_WM_QUERYDRAGICON(hwnd, fn) \
     (HICON)(UINT)(DWORD)(fn)((hwnd), WM_QUERYDRAGICON, 0L, 0L)
 
 #ifdef _INC_SHELLAPI
 /* void Cls_OnDropFiles(HWND hwnd, HDROP hdrop) */
 #define HANDLE_WM_DROPFILES(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HDROP)(wParam)), 0L)
 #define FORWARD_WM_DROPFILES(hwnd, hdrop, fn) \
     (void)(fn)((hwnd), WM_DROPFILES, (WPARAM)(HDROP)(hdrop), 0L)
 #endif  /* _INC_SHELLAPI */
 
    
 /* void Cls_OnActivateApp(HWND hwnd, BOOL fActivate, DWORD dwThreadId) */
 #define HANDLE_WM_ACTIVATEAPP(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (BOOL)(wParam), (DWORD)(lParam)), 0L)
 #define FORWARD_WM_ACTIVATEAPP(hwnd, fActivate, dwThreadId, fn) \
     (void)(fn)((hwnd), WM_ACTIVATEAPP, (WPARAM)(BOOL)(fActivate), (LPARAM)(dwThreadId))
 
  
    
 /* void Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) */
 #define HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_RBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, fn) \
     (void)(fn)((hwnd), (fDoubleClick) ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))
 
 /* void Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) */
 #define HANDLE_WM_RBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 
 /* void Cls_OnRButtonUp(HWND hwnd, int x, int y, UINT flags) */
 #define HANDLE_WM_RBUTTONUP(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, fn) \
     (void)(fn)((hwnd), WM_RBUTTONUP, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))
 
 /* void Cls_OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) */
 #define HANDLE_WM_MBUTTONDOWN(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_MBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, fn) \
     (void)(fn)((hwnd), (fDoubleClick) ? WM_MBUTTONDBLCLK : WM_MBUTTONDOWN, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))
 
 /* void Cls_OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) */
 #define HANDLE_WM_MBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd),)(wParam)), 0L)
 
 /* void Cls_OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCLBUTTONUP(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_NCLBUTTONUP(hwnd, x, y, codeHitTest, fn) \
     (void)(fn)((hwnd), WM_NCLBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)))
 
 /* void Cls_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCRBUTTONDOWN(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_NCRBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, fn) \
     (void)(fn)((hwnd), (fDoubleClick) ? WM_NCRBUTTONDBLCLK : WM_NCRBUTTONDOWN, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )
 
 /* void Cls_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCRBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 
 /* void Cls_OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCRBUTTONUP(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_NCRBUTTONUP(hwnd, x, y, codeHitTest, fn) \
     (void)(fn)((hwnd), WM_NCRBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )
 
 /* void Cls_OnNCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCMBUTTONDOWN(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_NCMBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, fn) \
     (void)(fn)((hwnd), (fDoubleClick) ? WM_NCMBUTTONDBLCLK : WM_NCMBUTTONDOWN, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )
 
 /* void Cls_OnNCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCMBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 
 /* void Cls_OnNCMButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) */
 #define HANDLE_WM_NCMBUTTONUP(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
 #define FORWARD_WM_NCMBUTTONUP(hwnd, x, y, codeHitTest, fn) \
     (void)(fn)((hwnd), WM_NCMBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )
 
 /* int Cls_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg) */
 #define HANDLE_WM_MOUSEACTIVATE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(int)(fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))
 #define FORWARD_WM_MOUSEACTIVATE(hwnd, hwndTopLevel, codeHitTest, msg, fn) \
     (int)(DWORD)(fn)((hwnd), WM_MOUSEACTIVATE, (WPARAM)(HWND)(hwndTopLevel), MAKELPARAM((codeHitTest), (msg)))
 
  
 /* void Cls_OnInitMenu(HWND hwnd, HMENU hMenu) */
 #define HANDLE_WM_INITMENU(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HMENU)(wParam)), 0L)
 #define FORWARD_WM_INITMENU(hwnd, hMenu, fn) \
     (void)(fn)((hwnd), WM_INITMENU, (WPARAM)(HMENU)(hMenu), 0L)
 
    
 /* void Cls_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags) */
 #define HANDLE_WM_MENUSELECT(hwnd, wParam, lParam, fn)                  \
     ((fn)((hwnd), (HMENU)(lParam),  \
     (HIWORD(wParam) & MF_POPUP) ? 0L : (int)(LOWORD(wParam)),           \
     (HIWORD(wParam) & MF_POPUP) ? GetSubMenu((HMENU)lParam, LOWORD(wParam)) : 0L, \
     (UINT)(((short)HIWORD(wParam) == -1) ? 0xFFFFFFFF : HIWORD(wParam))), 0L)
 #define FORWARD_WM_MENUSELECT(hwnd, hmenu, item, hmenuPopup, flags, fn) \
     (void)(fn)((hwnd), WM_MENUSELECT, MAKEWPARAM((item), (flags)), (LPARAM)(HMENU)((hmenu) ? (hmenu) : (hmenuPopup)))
 
    //  (void)(fn)((hwnd), WM_VSCROLL, MAKEWPARAM((UINT)(int)(code), (UINT)(int)(pos)), (LPARAM)(HWND)(hwndCtl))
    
      
 /* void Cls_OnDrawClipboard(HWND hwnd) */
 #define HANDLE_WM_DRAWCLIPBOARD(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_DRAWCLIPBOARD(hwnd, fn) \
     (void)(fn)((hwnd), WM_DRAWCLIPBOARD, 0L, 0L)
 
 /* void Cls_OnPaintClipboard(HWND hwnd, HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct) */
 #define HANDLE_WM_PAINTCLIPBOARD(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam), (const LPPAINTSTRUCT)((HGLOBAL)(lParam))), ((HGLOBAL)(lParam)), 0L)
 #define FORWARD_WM_PAINTCLIPBOARD(hwnd, hwndCBViewer, lpPaintStruct, fn) \
     (void)(fn)((hwnd), WM_PAINTCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), (LPARAM)(PPAINTSTRUCT)(lpPaintStruct))
 
 /* void Cls_OnSizeClipboard(HWND hwnd, HWND hwndCBViewer, const LPRECT lprc) */
 #define HANDLE_WM_SIZECLIPBOARD(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam), (const LPRECT)((HGLOBAL)(lParam))), (HGLOBAL)(lParam)), 0L)
 #define FORWARD_WM_SIZECLIPBOARD(hwnd, hwndCBViewer, lprc, fn) \
     (void)(fn)((hwnd), WM_SIZECLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), (LPARAM)(PRECT)(lprc))
 
 /* void Cls_OnVScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos) */
 #define HANDLE_WM_VSCROLLCLIPBOARD(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)
 #define FORWARD_WM_VSCROLLCLIPBOARD(hwnd, hwndCBViewer, code, pos, fn) \
     (void)(fn)((hwnd), WM_VSCROLLCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), MAKELPARAM((code), (pos)))
 
 /* void Cls_OnHScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos) */
 #define HANDLE_WM_HSCROLLCLIPBOARD(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)
 #define FORWARD_WM_HSCROLLCLIPBOARD(hwnd, hwndCBViewer, code, pos, fn) \
     (void)(fn)((hwnd), WM_HSCROLLCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), MAKELPARAM((code), (pos)))
 
 /* void Cls_OnAskCBFormatName(HWND hwnd, int cchMax, LPTSTR rgchName) */
 #define HANDLE_WM_ASKCBFORMATNAME(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(wParam), (LPTSTR)(lParam)), 0L)
 #define FORWARD_WM_ASKCBFORMATNAME(hwnd, cchMax, rgchName, fn) \
     (void)(fn)((hwnd), WM_ASKCBFORMATNAME, (WPARAM)(int)(cchMax), (LPARAM)(rgchName))
 
 /* void Cls_OnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext) */
 #define HANDLE_WM_CHANGECBCHAIN(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam), (HWND)(lParam)), 0L)
 #define FORWARD_WM_CHANGECBCHAIN(hwnd, hwndRemove, hwndNext, fn) \
     (void)(fn)((hwnd), WM_CHANGECBCHAIN, (WPARAM)(HWND)(hwndRemove), (LPARAM)(HWND)(hwndNext))
 
 /* BOOL Cls_OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg) */
 #define HANDLE_WM_SETCURSOR(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))
 #define FORWARD_WM_SETCURSOR(hwnd, hwndCursor, codeHitTest, msg, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_SETCURSOR, (WPARAM)(HWND)(hwndCursor), MAKELPARAM((codeHitTest), (msg)))
 
   
 /* HWND Cls_MDICreate(HWND hwnd, const LPMDICREATESTRUCT lpmcs) */
 #define HANDLE_WM_MDICREATE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)((hwnd), (LPMDICREATESTRUCT)(lParam))
 #define FORWARD_WM_MDICREATE(hwnd, lpmcs, fn) \
     (HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDICREATE, 0L, (LPARAM)(LPMDICREATESTRUCT)(lpmcs))
 
 /* void Cls_MDIDestroy(HWND hwnd, HWND hwndDestroy) */
 #define HANDLE_WM_MDIDESTROY(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam)), 0L)
 #define FORWARD_WM_MDIDESTROY(hwnd, hwndDestroy, fn) \
     (void)(fn)((hwnd), WM_MDIDESTROY, (WPARAM)(hwndDestroy), 0L)
 
 /* NOTE: Usable only by MDI client windows */
 /* void Cls_MDIActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate) */
 #define HANDLE_WM_MDIACTIVATE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (BOOL)(lParam == (LPARAM)hwnd), (HWND)(lParam), (HWND)(wParam)), 0L)
 #define FORWARD_WM_MDIACTIVATE(hwnd, fActive, hwndActivate, hwndDeactivate, fn) \
     (void)(fn)(hwnd, WM_MDIACTIVATE, (WPARAM)(hwndDeactivate), (LPARAM)(hwndActivate))
 
 /* void Cls_MDIRestore(HWND hwnd, HWND hwndRestore) */
 #define HANDLE_WM_MDIRESTORE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam)), 0L)
 #define FORWARD_WM_MDIRESTORE(hwnd, hwndRestore, fn) \
     (void)(fn)((hwnd), WM_MDIRESTORE, (WPARAM)(hwndRestore), 0L)
 
 /* HWND Cls_MDINext(HWND hwnd, HWND hwndCur, BOOL fPrev) */
 #define HANDLE_WM_MDINEXT(hwnd, wParam, lParam, fn) \
     (LRESULT)(HWND)(fn)((hwnd), (HWND)(wParam), (BOOL)lParam)
 #define FORWARD_WM_MDINEXT(hwnd, hwndCur, fPrev, fn) \
     (HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDINEXT, (WPARAM)(hwndCur), (LPARAM)(fPrev))
 
 /* void Cls_MDIMaximize(HWND hwnd, HWND hwndMaximize) */
 #define HANDLE_WM_MDIMAXIMIZE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (HWND)(wParam)), 0L)
 #define FORWARD_WM_MDIMAXIMIZE(hwnd, hwndMaximize, fn) \
     (void)(fn)((hwnd), WM_MDIMAXIMIZE, (WPARAM)(hwndMaximize), 0L)
 
 /* BOOL Cls_MDITile(HWND hwnd, UINT cmd) */
 #define HANDLE_WM_MDITILE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam))
 #define FORWARD_WM_MDITILE(hwnd, cmd, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_MDITILE, (WPARAM)(cmd), 0L)
 
 /* BOOL Cls_MDICascade(HWND hwnd, UINT cmd) */
 #define HANDLE_WM_MDICASCADE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam))
 #define FORWARD_WM_MDICASCADE(hwnd, cmd, fn) \
     (BOOL)(DWORD)(fn)((hwnd), WM_MDICASCADE, (WPARAM)(cmd), 0L)
 
 /* void Cls_MDIIconArrange(HWND hwnd) */
 #define HANDLE_WM_MDIICONARRANGE(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_MDIICONARRANGE(hwnd, fn) \
     (void)(fn)((hwnd), WM_MDIICONARRANGE, 0L, 0L)
 
 /* HWND Cls_MDIGetActive(HWND hwnd) */
 #define HANDLE_WM_MDIGETACTIVE(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)(hwnd)
 #define FORWARD_WM_MDIGETACTIVE(hwnd, fn) \
     (HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDIGETACTIVE, 0L, 0L)
 
 /* HMENU Cls_MDISetMenu(HWND hwnd, BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow) */
 #define HANDLE_WM_MDISETMENU(hwnd, wParam, lParam, fn) \
     (LRESULT)(DWORD)(UINT)(fn)((hwnd), (BOOL)(wParam), (HMENU)(wParam), (HMENU)(lParam))
 #define FORWARD_WM_MDISETMENU(hwnd, fRefresh, hmenuFrame, hmenuWindow, fn) \
     (HMENU)(UINT)(DWORD)(fn)((hwnd), WM_MDISETMENU, (WPARAM)((fRefresh) ? (hmenuFrame) : 0), (LPARAM)(hmenuWindow))
 
 /* void Cls_OnChildActivate(HWND hwnd) */
 #define HANDLE_WM_CHILDACTIVATE(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_CHILDACTIVATE(hwnd, fn) \
     (void)(fn)((hwnd), WM_CHILDACTIVATE, 0L, 0L)
 
    
 /* void Cls_OnParentNotify(HWND hwnd, UINT msg, HWND hwndChild, int idChild) */
 #define HANDLE_WM_PARENTNOTIFY(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (UINT)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L)
 #define FORWARD_WM_PARENTNOTIFY(hwnd, msg, hwndChild, idChild, fn) \
     (void)(fn)((hwnd), WM_PARENTNOTIFY, MAKEWPARAM(msg, idChild), (LPARAM)(hwndChild))
 
 /* void Cls_OnEnterIdle(HWND hwnd, UINT source, HWND hwndSource) */
 #define HANDLE_WM_ENTERIDLE(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (UINT)(wParam), (HWND)(lParam)), 0L)
 #define FORWARD_WM_ENTERIDLE(hwnd, source, hwndSource, fn) \
     (void)(fn)((hwnd), WM_ENTERIDLE, (WPARAM)(UINT)(source), (LPARAM)(HWND)(hwndSource))
 

   
 /* void Cls_OnQueueSync(HWND hwnd) */
 #define HANDLE_WM_QUEUESYNC(hwnd, wParam, lParam, fn) \
     ((fn)(hwnd), 0L)
 #define FORWARD_WM_QUEUESYNC(hwnd, fn) \
     (void)(fn)((hwnd), WM_QUEUESYNC, 0L, 0L)
 #if (WINVER >= 0x030a)
 /* void Cls_OnCommNotify(HWND hwnd, int cid, UINT flags) */
 #define HANDLE_WM_COMMNOTIFY(hwnd, wParam, lParam, fn) \
     ((fn)((hwnd), (int)(wParam), (UINT)LOWORD(lParam)), 0L)
 #define FORWARD_WM_COMMNOTIFY(hwnd, cid, flags, fn) \
     (void)(fn)((hwnd), WM_COMMNOTIFY, (WPARAM)(cid), MAKELPARAM((flags), 0))
 #endif
    /****** Static control message APIs ******************************************/
    
    
 #define Static_SetIcon(hwndCtl, hIcon)          ((HICON)(UINT)(DWORD)SendMessage((hwndCtl), STM_SETICON, (WPARAM)(HICON)(hIcon), 0L))
 #define Static_GetIcon(hwndCtl, hIcon)          ((HICON)(UINT)(DWORD)SendMessage((hwndCtl), STM_GETICON, 0L, 0L))
 
    

  

    /****** Alternate porting layer macros ****************************************/
    
    /* USER MESSAGES: */
    
    
 #define GET_WM_MENUSELECT_CMD(wp, lp)               LOWORD(wp)
 #define GET_WM_MENUSELECT_FLAGS(wp, lp)             (UINT)(int)(short)HIWORD(wp)
 #define GET_WM_MENUSELECT_HMENU(wp, lp)             (HMENU)(lp)
 #define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)  \
         (WPARAM)MAKELONG(cmd, f), (LONG)(hmenu)
 
 /* Note: the following are for interpreting MDIclient to MDI child messages. */
 #define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (lp == (LONG)hwnd)
 #define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)(wp)
 #define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)(lp)
 /* Note: the following is for sending to the MDI client window. */
 #define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA)\
         (WPARAM)(hwndA), 0
 
 #define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW) (WPARAM)hmenuF, (LONG)hmenuW
 
    
 #define GET_WM_PARENTNOTIFY_MSG(wp, lp)             LOWORD(wp)
 #define GET_WM_PARENTNOTIFY_ID(wp, lp)              HIWORD(wp)
 #define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)       (HWND)(lp)
 #define GET_WM_PARENTNOTIFY_X(wp, lp)               (int)(short)LOWORD(lp)
 #define GET_WM_PARENTNOTIFY_Y(wp, lp)               (int)(short)HIWORD(lp)
 #define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd) \
         (WPARAM)MAKELONG(id, msg), (LONG)(hwnd)
 #define GET_WM_PARENTNOTIFY2_MPS(msg, x, y) \
         (WPARAM)MAKELONG(0, msg), MAKELONG(x, y)
 

 #define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)       (HWND)(lp)
 
   

    
    #ifdef __cplusplus
    }                       /* End of extern "C" { */
    #endif       /* __cplusplus */
    
    #endif  /* _PRIV_INC_WINDOWSX*/
