/*****************************************************************************\
*                                                                             *
* commctrl.h - - Interface for the Windows Common Controls                    *
*                                                                             *
* Version 1.2                                                                 *
*                                                                             *
* Copyright (c) 1991-2000 Microsoft Corporation.  All rights reserved.
*                                                                             *
\*****************************************************************************/

#ifndef _INC_COMCTRLP
#define _INC_COMCTRLP
#ifndef NOUSER

#ifndef UNDER_CE
#ifdef _WIN32
#include <pshpack1.h>
#endif
#endif // !UNDER_CE

#ifdef __cplusplus
extern "C" {
#endif

//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NOBTNLIST    A control which is a list of bitmap buttons.
//
//=============================================================================

#define ICC_ALL_CLASSES      0x0000FFFF //

#ifdef UNDER_CE // Moved from commctrl.h. The following are not supported on WinCE
//++++++++++++++++++++++++++ NOT SUPPORTED ON WINCE ++++++++++++++++++++ 
#define ICC_HOTKEY_CLASS     0x00000040 // hotkey
#define ICC_ANIMATE_CLASS    0x00000080 // animate
#define ICC_USEREX_CLASSES   0x00000200 // comboex
#if (_WIN32_IE >= 0x0400)
#define ICC_INTERNET_CLASSES 0x00000800
#endif // (_WIN32_IE >= 0x0400)

#define CBEN_FIRST              (0U-800U)       // combo box ex
#define CBEN_LAST               (0U-830U)

#if (_WIN32_IE >= 0x0400)
#define IPN_FIRST               (0U-860U)       // internet address
#define IPN_LAST                (0U-879U)       // internet address
#endif // (_WIN32_IE >= 0x0400)

#define MSGF_COMMCTRL_TOOLBARCUST   0x4203

#if (_WIN32_IE >= 0x0400)
// Generic structure to request an object of a specific type.

typedef struct tagNMOBJECTNOTIFY {
    NMHDR   hdr;
    int     iItem;
#ifdef __IID_DEFINED__
    const IID *piid;
#else
    const void *piid;
#endif
    void *pObject;
    HRESULT hResult;
    DWORD dwFlags;    // control specific flags (hints as to where in iItem it hit)
} NMOBJECTNOTIFY, *LPNMOBJECTNOTIFY;

#endif           // _WIN32_IE >= 0x0400

//====== IMAGE APIS ===========================================================
#ifndef NOIMAGEAPIS
#define ILC_COLOR4              0x0004
#define ILC_COLOR8              0x0008
#define ILC_COLOR16             0x0010
#define ILC_COLOR24             0x0018
#define ILC_COLOR32             0x0020

#ifdef __IStream_INTERFACE_DEFINED__
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Read(LPSTREAM pstm);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Write(HIMAGELIST himl, LPSTREAM pstm);
#endif

#endif // NOIMAGEAPIS

//====== HEADER CONTROL =======================================================
#ifndef NOHEADER
#if (_WIN32_IE >= 0x0300)
#define HDS_HOTTRACK            0x0004
#endif

#endif      // NOHEADER


//====== TOOLBAR CONTROL ======================================================
#ifndef NOTOOLBAR

typedef struct _COLORMAP {
    COLORREF from;
    COLORREF to;
} COLORMAP, FAR* LPCOLORMAP;

WINCOMMCTRLAPI HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, int idBitmap,
                                  UINT wFlags, LPCOLORMAP lpColorMap,
                                  int iNumMaps);

#define CMB_MASKED              0x02

#if (_WIN32_IE >= 0x0400)
#define TBSTYLE_REGISTERDROP    0x4000
#endif // (_WIN32_IE >= 0x0400)

#ifdef _WIN32

typedef struct tagTBSAVEPARAMSA {
    HKEY hkr;
    LPCSTR pszSubKey;
    LPCSTR pszValueName;
} TBSAVEPARAMSA, FAR* LPTBSAVEPARAMSA;

typedef struct tagTBSAVEPARAMSW {
    HKEY hkr;
    LPCWSTR pszSubKey;
    LPCWSTR pszValueName;
} TBSAVEPARAMSW, FAR *LPTBSAVEPARAMW;

#ifdef UNICODE
#define TBSAVEPARAMS            TBSAVEPARAMSW
#define LPTBSAVEPARAMS          LPTBSAVEPARAMSW
#else
#define TBSAVEPARAMS            TBSAVEPARAMSA
#define LPTBSAVEPARAMS          LPTBSAVEPARAMSA
#endif

#endif  // _WIN32

#define TB_SAVERESTOREA         (WM_USER + 26)
#define TB_SAVERESTOREW         (WM_USER + 76)
#define TB_CUSTOMIZE            (WM_USER + 27)

#if (_WIN32_IE >= 0x0300)
#define TB_SETHOTIMAGELIST      (WM_USER + 52)
#define TB_GETHOTIMAGELIST      (WM_USER + 53)
#endif      // _WIN32_IE >= 0x0300

#if (_WIN32_IE >= 0x0400)
#define TB_GETOBJECT            (WM_USER + 62)  // wParam == IID, lParam void **ppv
#define TB_GETHOTITEM           (WM_USER + 71)  
#define TB_SETHOTITEM           (WM_USER + 72)  // wParam == iHotItem
#define TB_SETANCHORHIGHLIGHT   (WM_USER + 73)  // wParam == TRUE/FALSE
#define TB_GETANCHORHIGHLIGHT   (WM_USER + 74)
#endif  // _WIN32_IE >= 0x0400

#define TBN_BEGINADJUST         (TBN_FIRST-3)
#define TBN_ENDADJUST           (TBN_FIRST-4)
#define TBN_RESET               (TBN_FIRST-5)
#define TBN_QUERYINSERT         (TBN_FIRST-6)
#define TBN_QUERYDELETE         (TBN_FIRST-7)
#define TBN_TOOLBARCHANGE       (TBN_FIRST-8)
#define TBN_CUSTHELP            (TBN_FIRST-9)

#if (_WIN32_IE >= 0x0400)
#define TBN_GETOBJECT           (TBN_FIRST - 12)
typedef struct tagNMTBHOTITEM
{
    NMHDR   hdr;
    int     iHotOld;
    int     iHotNew;
} NMTBHOTITEM, * LPNMTBHOTITEM;

#define TBN_SETHOTITEM          (TBN_FIRST - 13)
#endif // (_WIN32_IE >= 0x0400)

#if (_WIN32_IE >= 0x0300)
#define IDB_HIST_SMALL_COLOR    8
#define IDB_HIST_LARGE_COLOR    9
#endif

#if (_WIN32_IE >= 0x0300)
#define HIST_BACK               0
#define HIST_FORWARD            1
#define HIST_FAVORITES          2
#define HIST_ADDTOFAVORITES     3
#define HIST_VIEWTREE           4
#endif

#if (_WIN32_IE >= 0x0400)
#define VIEW_VIEWMENU           12
#endif

#endif  // NOTOOLBAR

//====== REBAR CONTROL ========================================================

#if (_WIN32_IE >= 0x0300) // Rebar control
#ifndef NOREBAR

#if (_WIN32_IE >= 0x0400)
#define RBS_TOOLTIPS        0x0100
#else
#define RBS_TOOLTIPS        0x00000100
#endif      // _WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0400)
#define RB_GETDROPTARGET (WM_USER + 32)
#define RBN_GETOBJECT       (RBN_FIRST - 1)
#endif      // _WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0400)
#define RB_GETTOOLTIPS  (WM_USER +  17)
#define RB_SETTOOLTIPS  (WM_USER +  18)
#endif      // _WIN32_IE >= 0x0400

#endif      // NOREBAR
#endif      // _WIN32_IE >= 0x0300

//====== TOOLTIPS CONTROL =====================================================

#ifndef NOTOOLTIPS

#ifdef _WIN32

#define TOOLTIPS_CLASSW         L"tooltips_class32"
#define TOOLTIPS_CLASSA         "tooltips_class32"

#ifdef UNICODE
#define TOOLTIPS_CLASS          TOOLTIPS_CLASSW
#else
#define TOOLTIPS_CLASS          TOOLTIPS_CLASSA
#endif

#else
#define TOOLTIPS_CLASS          "tooltips_class"
#endif // _WIN32

#if (_WIN32_IE >= 0x0300)
#define LPTOOLINFOA   LPTTTOOLINFOA
#define LPTOOLINFOW   LPTTTOOLINFOW
#define TOOLINFOA       TTTOOLINFOA
#define TOOLINFOW       TTTOOLINFOW
#else
#define   TTTOOLINFOA   TOOLINFOA
#define LPTTTOOLINFOA LPTOOLINFOA
#define   TTTOOLINFOW   TOOLINFOW
#define LPTTTOOLINFOW LPTOOLINFOW
#endif // (_WIN32_IE >= 0x0300)

#define LPTOOLINFO    LPTTTOOLINFO
#define TOOLINFO        TTTOOLINFO

typedef struct tagTOOLINFOA {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
#if (_WIN32_IE >= 0x0300)
    LPARAM lParam;
#endif
} TTTOOLINFOA, NEAR *PTOOLINFOA, FAR *LPTTTOOLINFOA;

typedef struct tagTOOLINFOW {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPWSTR lpszText;
#if (_WIN32_IE >= 0x0300)
    LPARAM lParam;
#endif
} TTTOOLINFOW, NEAR *PTOOLINFOW, FAR* LPTTTOOLINFOW;

#ifdef UNICODE
#define TTTOOLINFO              TTTOOLINFOW
#define PTOOLINFO               PTOOLINFOW
#define LPTTTOOLINFO            LPTTTOOLINFOW
#else
#define PTOOLINFO               PTOOLINFOA
#define TTTOOLINFO              TTTOOLINFOA
#define LPTTTOOLINFO            LPTTTOOLINFOA
#endif

// begin_r_commctrl

#define TTS_ALWAYSTIP           0x01
#define TTS_NOPREFIX            0x02

// end_r_commctrl

#define TTF_IDISHWND            0x0001

// Use this to center around trackpoint in trackmode
// -OR- to center around tool in normal mode.
// Use TTF_ABSOLUTE to place the tip exactly at the track coords when
// in tracking mode.  TTF_ABSOLUTE can be used in conjunction with TTF_CENTERTIP
// to center the tip absolutely about the track point.

#define TTF_CENTERTIP           0x0002
#define TTF_RTLREADING          0x0004
#if (_WIN32_IE >= 0x0300)
#define TTF_SUBCLASS            0x0010
#define TTF_TRACK               0x0020
#define TTF_ABSOLUTE            0x0080
#define TTF_TRANSPARENT         0x0100
#if (_WIN32_IE >= 0x0400)
#define TTF_USEHITTEST          0x0400
#endif
#define TTF_DI_SETITEM          0x8000       // valid only on the TTN_NEEDTEXT callback
#endif      // _WIN32_IE >= 0x0300

#define TTDT_AUTOMATIC          0
#define TTDT_RESHOW             1
#define TTDT_AUTOPOP            2
#define TTDT_INITIAL            3

#define TTM_ACTIVATE            (WM_USER + 1)
#define TTM_SETDELAYTIME        (WM_USER + 3)
#define TTM_ADDTOOLA            (WM_USER + 4)
#define TTM_ADDTOOLW            (WM_USER + 50)
#define TTM_DELTOOLA            (WM_USER + 5)
#define TTM_DELTOOLW            (WM_USER + 51)
#define TTM_NEWTOOLRECTA        (WM_USER + 6)
#define TTM_NEWTOOLRECTW        (WM_USER + 52)
#define TTM_RELAYEVENT          (WM_USER + 7)

#define TTM_GETTOOLINFOA        (WM_USER + 8)
#define TTM_GETTOOLINFOW        (WM_USER + 53)

#define TTM_SETTOOLINFOA        (WM_USER + 9)
#define TTM_SETTOOLINFOW        (WM_USER + 54)

#define TTM_HITTESTA            (WM_USER +10)
#define TTM_HITTESTW            (WM_USER +55)
#define TTM_GETTEXTA            (WM_USER +11)
#define TTM_GETTEXTW            (WM_USER +56)
#define TTM_UPDATETIPTEXTA      (WM_USER +12)
#define TTM_UPDATETIPTEXTW      (WM_USER +57)
#define TTM_GETTOOLCOUNT        (WM_USER +13)
#define TTM_ENUMTOOLSA          (WM_USER +14)
#define TTM_ENUMTOOLSW          (WM_USER +58)
#define TTM_GETCURRENTTOOLA     (WM_USER + 15)
#define TTM_GETCURRENTTOOLW     (WM_USER + 59)
#define TTM_WINDOWFROMPOINT     (WM_USER + 16)
#if (_WIN32_IE >= 0x0300)
#define TTM_TRACKACTIVATE       (WM_USER + 17)  // wParam = TRUE/FALSE start end  lparam = LPTOOLINFO
#define TTM_TRACKPOSITION       (WM_USER + 18)  // lParam = dwPos
#define TTM_SETTIPBKCOLOR       (WM_USER + 19)
#define TTM_SETTIPTEXTCOLOR     (WM_USER + 20)
#define TTM_GETDELAYTIME        (WM_USER + 21)
#define TTM_GETTIPBKCOLOR       (WM_USER + 22)
#define TTM_GETTIPTEXTCOLOR     (WM_USER + 23)
#define TTM_SETMAXTIPWIDTH      (WM_USER + 24)
#define TTM_GETMAXTIPWIDTH      (WM_USER + 25)
#define TTM_SETMARGIN           (WM_USER + 26)  // lParam = lprc
#define TTM_GETMARGIN           (WM_USER + 27)  // lParam = lprc
#define TTM_POP                 (WM_USER + 28)
#endif
#if (_WIN32_IE >= 0x0400)
#define TTM_UPDATE              (WM_USER + 29)
#endif


#ifdef UNICODE
#define TTM_ADDTOOL             TTM_ADDTOOLW
#define TTM_DELTOOL             TTM_DELTOOLW
#define TTM_NEWTOOLRECT         TTM_NEWTOOLRECTW
#define TTM_GETTOOLINFO         TTM_GETTOOLINFOW
#define TTM_SETTOOLINFO         TTM_SETTOOLINFOW
#define TTM_HITTEST             TTM_HITTESTW
#define TTM_GETTEXT             TTM_GETTEXTW
#define TTM_UPDATETIPTEXT       TTM_UPDATETIPTEXTW
#define TTM_ENUMTOOLS           TTM_ENUMTOOLSW
#define TTM_GETCURRENTTOOL      TTM_GETCURRENTTOOLW
#else
#define TTM_ADDTOOL             TTM_ADDTOOLA
#define TTM_DELTOOL             TTM_DELTOOLA
#define TTM_NEWTOOLRECT         TTM_NEWTOOLRECTA
#define TTM_GETTOOLINFO         TTM_GETTOOLINFOA
#define TTM_SETTOOLINFO         TTM_SETTOOLINFOA
#define TTM_HITTEST             TTM_HITTESTA
#define TTM_GETTEXT             TTM_GETTEXTA
#define TTM_UPDATETIPTEXT       TTM_UPDATETIPTEXTA
#define TTM_ENUMTOOLS           TTM_ENUMTOOLSA
#define TTM_GETCURRENTTOOL      TTM_GETCURRENTTOOLA
#endif


#if (_WIN32_IE >= 0x0300)
#define LPHITTESTINFOW    LPTTHITTESTINFOW
#define LPHITTESTINFOA    LPTTHITTESTINFOA
#else
#define LPTTHITTESTINFOA  LPHITTESTINFOA
#define LPTTHITTESTINFOW  LPHITTESTINFOW
#endif

#define LPHITTESTINFO     LPTTHITTESTINFO

typedef struct _TT_HITTESTINFOA {
    HWND hwnd;
    POINT pt;
    TTTOOLINFOA ti;
} TTHITTESTINFOA, FAR * LPTTHITTESTINFOA;

typedef struct _TT_HITTESTINFOW {
    HWND hwnd;
    POINT pt;
    TTTOOLINFOW ti;
} TTHITTESTINFOW, FAR * LPTTHITTESTINFOW;

#ifdef UNICODE
#define TTHITTESTINFO           TTHITTESTINFOW
#define LPTTHITTESTINFO         LPTTHITTESTINFOW
#else
#define TTHITTESTINFO           TTHITTESTINFOA
#define LPTTHITTESTINFO         LPTTHITTESTINFOA
#endif

#define TTN_GETDISPINFOA        (TTN_FIRST - 0)
#define TTN_GETDISPINFOW        (TTN_FIRST - 10)
#define TTN_SHOW                (TTN_FIRST - 1)
#define TTN_POP                 (TTN_FIRST - 2)

#ifdef UNICODE
#define TTN_GETDISPINFO         TTN_GETDISPINFOW
#else
#define TTN_GETDISPINFO         TTN_GETDISPINFOA
#endif

#define TTN_NEEDTEXT            TTN_GETDISPINFO
#define TTN_NEEDTEXTA           TTN_GETDISPINFOA
#define TTN_NEEDTEXTW           TTN_GETDISPINFOW

#if (_WIN32_IE >= 0x0300)
#define TOOLTIPTEXTW NMTTDISPINFOW
#define TOOLTIPTEXTA NMTTDISPINFOA
#define LPTOOLTIPTEXTA LPNMTTDISPINFOA
#define LPTOOLTIPTEXTW LPNMTTDISPINFOW
#else
#define tagNMTTDISPINFOA  tagTOOLTIPTEXTA
#define NMTTDISPINFOA     TOOLTIPTEXTA
#define LPNMTTDISPINFOA   LPTOOLTIPTEXTA
#define tagNMTTDISPINFOW  tagTOOLTIPTEXTW
#define NMTTDISPINFOW     TOOLTIPTEXTW
#define LPNMTTDISPINFOW   LPTOOLTIPTEXTW
#endif

#define TOOLTIPTEXT    NMTTDISPINFO
#define LPTOOLTIPTEXT  LPNMTTDISPINFO

typedef struct tagNMTTDISPIFNOA {
    NMHDR hdr;
    LPSTR lpszText;
    char szText[80];
    HINSTANCE hinst;
    UINT uFlags;
#if (_WIN32_IE >= 0x0300)
    LPARAM lParam;
#endif
} NMTTDISPINFOA, FAR *LPNMTTDISPINFOA;

typedef struct tagNMTTDISPINFOW {
    NMHDR hdr;
    LPWSTR lpszText;
    WCHAR szText[80];
    HINSTANCE hinst;
    UINT uFlags;
#if (_WIN32_IE >= 0x0300)
    LPARAM lParam;
#endif
} NMTTDISPINFOW, FAR *LPNMTTDISPINFOW;

#ifdef UNICODE
#define NMTTDISPINFO            NMTTDISPINFOW
#define LPNMTTDISPINFO          LPNMTTDISPINFOW
#else
#define NMTTDISPINFO            NMTTDISPINFOA
#define LPNMTTDISPINFO          LPNMTTDISPINFOA
#endif

#endif      // NOTOOLTIPS

//====== STATUS BAR CONTROL ===================================================

#ifndef NOSTATUSBAR // Status Bar
#define SBARS_SIZEGRIP          0x0100
#endif      // NOSTATUSBAR

//====== MENU HELP ============================================================

#ifndef NOMENUHELP

WINCOMMCTRLAPI void WINAPI MenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, UINT FAR *lpwIDs);
WINCOMMCTRLAPI BOOL WINAPI ShowHideMenuCtl(HWND hWnd, UINT uFlags, LPINT lpInfo);
WINCOMMCTRLAPI void WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo);

#define MINSYSCOMMAND   SC_SIZE

#endif // NOMENUHELP

//====== TRACKBAR CONTROL =====================================================

#ifndef NOTRACKBAR

#if (_WIN32_IE >= 0x0300)
#define TBS_TOOLTIPS            0x0100
#endif

#if (_WIN32_IE >= 0x0300)
#define TBM_SETTOOLTIPS         (WM_USER+29)
#define TBM_GETTOOLTIPS         (WM_USER+30)
#define TBM_SETTIPSIDE          (WM_USER+31)
// TrackBar Tip Side flags
#define TBTS_TOP                0
#define TBTS_LEFT               1
#define TBTS_BOTTOM             2
#define TBTS_RIGHT              3
#endif

#endif // NOTRACKBAR

//====== DRAG LIST CONTROL ====================================================

#ifndef NODRAGLIST

typedef struct tagDRAGLISTINFO {
    UINT uNotification;
    HWND hWnd;
    POINT ptCursor;
} DRAGLISTINFO, FAR *LPDRAGLISTINFO;

#define DL_BEGINDRAG            (WM_USER+133)
#define DL_DRAGGING             (WM_USER+134)
#define DL_DROPPED              (WM_USER+135)
#define DL_CANCELDRAG           (WM_USER+136)

#define DL_CURSORSET            0
#define DL_STOPCURSOR           1
#define DL_COPYCURSOR           2
#define DL_MOVECURSOR           3

#define DRAGLISTMSGSTRING       TEXT("commctrl_DragListMsg")

WINCOMMCTRLAPI BOOL WINAPI MakeDragList(HWND hLB);
WINCOMMCTRLAPI void WINAPI DrawInsert(HWND handParent, HWND hLB, int nItem);
WINCOMMCTRLAPI int WINAPI LBItemFromPt(HWND hLB, POINT pt, BOOL bAutoScroll);

#endif // NODRAGLIST

//====== HOTKEY CONTROL =======================================================

#ifndef NOHOTKEY

#define HOTKEYF_SHIFT           0x01
#define HOTKEYF_CONTROL         0x02
#define HOTKEYF_ALT             0x04
#define HOTKEYF_EXT             0x08

#define HKCOMB_NONE             0x0001
#define HKCOMB_S                0x0002
#define HKCOMB_C                0x0004
#define HKCOMB_A                0x0008
#define HKCOMB_SC               0x0010
#define HKCOMB_SA               0x0020
#define HKCOMB_CA               0x0040
#define HKCOMB_SCA              0x0080


#define HKM_SETHOTKEY           (WM_USER+1)
#define HKM_GETHOTKEY           (WM_USER+2)
#define HKM_SETRULES            (WM_USER+3)

#ifdef _WIN32

#define HOTKEY_CLASSA           "msctls_hotkey32"
#define HOTKEY_CLASSW           L"msctls_hotkey32"

#ifdef UNICODE
#define HOTKEY_CLASS            HOTKEY_CLASSW
#else
#define HOTKEY_CLASS            HOTKEY_CLASSA
#endif

#else
#define HOTKEY_CLASS            "msctls_hotkey"
#endif

#endif  // NOHOTKEY

//====== LISTVIEW CONTROL =====================================================

#ifndef NOLISTVIEW

#define LVS_EX_ONECLICKACTIVATE 0x00000040
#define LVS_EX_TWOCLICKACTIVATE 0x00000080

#if (_WIN32_IE >= 0x0400)
#define LVS_EX_FLATSB           0x00000100 // cannot be cleared
#define LVS_EX_REGIONAL         0x00000200
#define LVS_EX_INFOTIP          0x00000400 // listview does InfoTips for you
#endif

#if (_WIN32_IE >= 0x0300)
#define LVM_SETHOTITEM  (LVM_FIRST + 60)
#define ListView_SetHotItem(hwnd, i) \
        (int)SNDMSG((hwnd), LVM_SETHOTITEM, (WPARAM)i, 0)

#define LVM_GETHOTITEM  (LVM_FIRST + 61)
#define ListView_GetHotItem(hwnd) \
        (int)SNDMSG((hwnd), LVM_GETHOTITEM, 0, 0)

#define LVM_SETHOTCURSOR  (LVM_FIRST + 62)
#define ListView_SetHotCursor(hwnd, hcur) \
        (HCURSOR)SNDMSG((hwnd), LVM_SETHOTCURSOR, 0, (LPARAM)hcur)

#define LVM_GETHOTCURSOR  (LVM_FIRST + 63)
#define ListView_GetHotCursor(hwnd) \
        (HCURSOR)SNDMSG((hwnd), LVM_GETHOTCURSOR, 0, 0)
#endif      // _WIN32_IE >= 0x0300

#if (_WIN32_IE >= 0x0400)

#define LVM_SETWORKAREAS         (LVM_FIRST + 65)
#define ListView_SetWorkAreas(hwnd, nWorkAreas, prc) \
    (BOOL)SNDMSG((hwnd), LVM_SETWORKAREAS, (WPARAM)(int)nWorkAreas, (LPARAM)(RECT FAR*)(prc))

#define LVM_GETWORKAREAS        (LVM_FIRST + 70)
#define ListView_GetWorkAreas(hwnd, nWorkAreas, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETWORKAREAS, (WPARAM)(int)nWorkAreas, (LPARAM)(RECT FAR*)(prc))


#define LVM_GETNUMBEROFWORKAREAS  (LVM_FIRST + 73)
#define ListView_GetNumberOfWorkAreas(hwnd, pnWorkAreas) \
    (BOOL)SNDMSG((hwnd), LVM_GETNUMBEROFWORKAREAS, 0, (LPARAM)(UINT *)(pnWorkAreas))

#define LVM_SETHOVERTIME        (LVM_FIRST + 71)
#define ListView_SetHoverTime(hwndLV, dwHoverTimeMs)\
        (DWORD)SendMessage((hwndLV), LVM_SETHOVERTIME, 0, dwHoverTimeMs)

#define LVM_GETHOVERTIME        (LVM_FIRST + 72)
#define ListView_GetHoverTime(hwndLV)\
        (DWORD)SendMessage((hwndLV), LVM_GETHOVERTIME, 0, 0)

typedef struct tagLVBKIMAGEA
{
    ULONG ulFlags;              // LVBKIF_*
    HBITMAP hbm;
    LPSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
} LVBKIMAGEA, FAR *LPLVBKIMAGEA;
typedef struct tagLVBKIMAGEW
{
    ULONG ulFlags;              // LVBKIF_*
    HBITMAP hbm;
    LPWSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
} LVBKIMAGEW, FAR *LPLVBKIMAGEW;

#define LVBKIF_SOURCE_NONE      0x00000000
#define LVBKIF_SOURCE_HBITMAP   0x00000001
#define LVBKIF_SOURCE_URL       0x00000002
#define LVBKIF_SOURCE_MASK      0x00000003
#define LVBKIF_STYLE_NORMAL     0x00000000
#define LVBKIF_STYLE_TILE       0x00000010
#define LVBKIF_STYLE_MASK       0x00000010

#define LVM_SETBKIMAGEA         (LVM_FIRST + 68)
#define LVM_SETBKIMAGEW         (LVM_FIRST + 138)
#define LVM_GETBKIMAGEA         (LVM_FIRST + 69)
#define LVM_GETBKIMAGEW         (LVM_FIRST + 139)

#ifdef UNICODE
#define LVBKIMAGE               LVBKIMAGEW
#define LPLVBKIMAGE             LPLVBKIMAGEW
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEW
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEW
#else
#define LVBKIMAGE               LVBKIMAGEA
#define LPLVBKIMAGE             LPLVBKIMAGEA
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEA
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEA
#endif

#define ListView_SetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_SETBKIMAGE, 0, (LPARAM)plvbki)

#define ListView_GetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_GETBKIMAGE, 0, (LPARAM)plvbki)

#endif      // _WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0400)
#define LVN_HOTTRACK            (LVN_FIRST-21)
#endif

#if (_WIN32_IE >= 0x0400)
typedef struct tagNMLVGETINFOTIPA
{
    NMHDR hdr;
    DWORD dwFlags;
    LPSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPA, *LPNMLVGETINFOTIPA;

typedef struct tagNMLVGETINFOTIPW
{
    NMHDR hdr;
    DWORD dwFlags;
    LPWSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPW, *LPNMLVGETINFOTIPW;

// NMLVGETINFOTIPA.dwFlag values

#define LVGIT_UNFOLDED  0x0001

#define LVN_GETINFOTIPA          (LVN_FIRST-57)
#define LVN_GETINFOTIPW          (LVN_FIRST-58)

#ifdef UNICODE
#define LVN_GETINFOTIP          LVN_GETINFOTIPW
#define NMLVGETINFOTIP          NMLVGETINFOTIPW
#define LPNMLVGETINFOTIP        LPNMLVGETINFOTIPW
#else
#define LVN_GETINFOTIP          LVN_GETINFOTIPA
#define NMLVGETINFOTIP          NMLVGETINFOTIPA
#define LPNMLVGETINFOTIP        LPNMLVGETINFOTIPA
#endif

#endif      // _WIN32_IE >= 0x0400
#endif // NOLISTVIEW

//====== TREEVIEW CONTROL =====================================================

#ifndef NOTREEVIEW
#if (_WIN32_IE >= 0x0400)
#define TVS_INFOTIP             0x0800
#endif

#if (_WIN32_IE >= 0x0300)
#define TVM_SETTOOLTIPS         (TV_FIRST + 24)
#define TVM_GETTOOLTIPS         (TV_FIRST + 25)
#endif

#if (_WIN32_IE >= 0x0400)
#define TVN_GETINFOTIPA         (TVN_FIRST-13)
#define TVN_GETINFOTIPW         (TVN_FIRST-14)

typedef struct tagNMTVGETINFOTIPA
{
    NMHDR hdr;
    LPSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPA, *LPNMTVGETINFOTIPA;

typedef struct tagNMTVGETINFOTIPW
{
    NMHDR hdr;
    LPWSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPW, *LPNMTVGETINFOTIPW;


#ifdef UNICODE
#define TVN_GETINFOTIP          TVN_GETINFOTIPW
#define NMTVGETINFOTIP          NMTVGETINFOTIPW
#define LPNMTVGETINFOTIP        LPNMTVGETINFOTIPW
#else
#define TVN_GETINFOTIP          TVN_GETINFOTIPA
#define NMTVGETINFOTIP          NMTVGETINFOTIPA
#define LPNMTVGETINFOTIP        LPNMTVGETINFOTIPA
#endif

#endif
#endif // NOTREEVIEW

////////////////////  ComboBoxEx ////////////////////////////////
#if (_WIN32_IE >= 0x0300)

#ifndef NOUSEREXCONTROLS

#define WC_COMBOBOXEXW         L"ComboBoxEx32"
#define WC_COMBOBOXEXA         "ComboBoxEx32"

#ifdef UNICODE
#define WC_COMBOBOXEX          WC_COMBOBOXEXW
#else
#define WC_COMBOBOXEX          WC_COMBOBOXEXA
#endif


#define CBEIF_TEXT              0x00000001
#define CBEIF_IMAGE             0x00000002
#define CBEIF_SELECTEDIMAGE     0x00000004
#define CBEIF_OVERLAY           0x00000008
#define CBEIF_INDENT            0x00000010
#define CBEIF_LPARAM            0x00000020

#define CBEIF_DI_SETITEM        0x10000000

typedef struct tagCOMBOBOXEXITEMA
{
    UINT mask;
    int iItem;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMA, *PCOMBOBOXEXITEMA;
typedef COMBOBOXEXITEMA CONST *PCCOMBOEXITEMA;


typedef struct tagCOMBOBOXEXITEMW
{
    UINT mask;
    int iItem;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMW, *PCOMBOBOXEXITEMW;
typedef COMBOBOXEXITEMW CONST *PCCOMBOEXITEMW;

#ifdef UNICODE
#define COMBOBOXEXITEM            COMBOBOXEXITEMW
#define PCOMBOBOXEXITEM           PCOMBOBOXEXITEMW
#define PCCOMBOBOXEXITEM          PCCOMBOBOXEXITEMW
#else
#define COMBOBOXEXITEM            COMBOBOXEXITEMA
#define PCOMBOBOXEXITEM           PCOMBOBOXEXITEMA
#define PCCOMBOBOXEXITEM          PCCOMBOBOXEXITEMA
#endif

#define CBEM_INSERTITEMA        (WM_USER + 1)
#define CBEM_SETIMAGELIST       (WM_USER + 2)
#define CBEM_GETIMAGELIST       (WM_USER + 3)
#define CBEM_GETITEMA           (WM_USER + 4)
#define CBEM_SETITEMA           (WM_USER + 5)
#define CBEM_DELETEITEM         CB_DELETESTRING
#define CBEM_GETCOMBOCONTROL    (WM_USER + 6)
#define CBEM_GETEDITCONTROL     (WM_USER + 7)
#if (_WIN32_IE >= 0x0400)
#define CBEM_SETEXSTYLE         (WM_USER + 8)  // use  SETEXTENDEDSTYLE instead
#define CBEM_SETEXTENDEDSTYLE   (WM_USER + 14)   // lparam == new style, wParam (optional) == mask
#define CBEM_GETEXSTYLE         (WM_USER + 9) // use GETEXTENDEDSTYLE instead
#define CBEM_GETEXTENDEDSTYLE   (WM_USER + 9)
#else
#define CBEM_SETEXSTYLE         (WM_USER + 8)
#define CBEM_GETEXSTYLE         (WM_USER + 9)
#endif
#define CBEM_HASEDITCHANGED     (WM_USER + 10)
#define CBEM_INSERTITEMW        (WM_USER + 11)
#define CBEM_SETITEMW           (WM_USER + 12)
#define CBEM_GETITEMW           (WM_USER + 13)

#ifdef UNICODE
#define CBEM_INSERTITEM         CBEM_INSERTITEMW
#define CBEM_SETITEM            CBEM_SETITEMW
#define CBEM_GETITEM            CBEM_GETITEMW
#else
#define CBEM_INSERTITEM         CBEM_INSERTITEMA
#define CBEM_SETITEM            CBEM_SETITEMA
#define CBEM_GETITEM            CBEM_GETITEMA
#endif

#define CBES_EX_NOEDITIMAGE          0x00000001
#define CBES_EX_NOEDITIMAGEINDENT    0x00000002
#define CBES_EX_PATHWORDBREAKPROC    0x00000004
#if (_WIN32_IE >= 0x0400)
#define CBES_EX_NOSIZELIMIT          0x00000008
#define CBES_EX_CASESENSITIVE        0x00000010

typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMA ceItem;
} NMCOMBOBOXEXA, *PNMCOMBOBOXEXA;

typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMW ceItem;
} NMCOMBOBOXEXW, *PNMCOMBOBOXEXW;

#ifdef UNICODE
#define NMCOMBOBOXEX            NMCOMBOBOXEXW
#define PNMCOMBOBOXEX           PNMCOMBOBOXEXW
#define CBEN_GETDISPINFO        CBEN_GETDISPINFOW
#else
#define NMCOMBOBOXEX            NMCOMBOBOXEXA
#define PNMCOMBOBOXEX           PNMCOMBOBOXEXA
#define CBEN_GETDISPINFO        CBEN_GETDISPINFOA
#endif

#else
typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEM ceItem;
} NMCOMBOBOXEX, *PNMCOMBOBOXEX;

#define CBEN_GETDISPINFO         (CBEN_FIRST - 0)

#endif      // _WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0400)
#define CBEN_GETDISPINFOA        (CBEN_FIRST - 0)
#endif
#define CBEN_INSERTITEM          (CBEN_FIRST - 1)
#define CBEN_DELETEITEM          (CBEN_FIRST - 2)
#define CBEN_BEGINEDIT           (CBEN_FIRST - 4)
#define CBEN_ENDEDITA            (CBEN_FIRST - 5)
#define CBEN_ENDEDITW            (CBEN_FIRST - 6)
#if (_WIN32_IE >= 0x0400)
#define CBEN_GETDISPINFOW        (CBEN_FIRST - 7)
#define CBEN_DRAGBEGINA          (CBEN_FIRST - 8)
#define CBEN_DRAGBEGINW          (CBEN_FIRST - 9)

#ifdef UNICODE
#define CBEN_DRAGBEGIN CBEN_DRAGBEGINW
#else
#define CBEN_DRAGBEGIN CBEN_DRAGBEGINA
#endif
#endif      // _WIN32_IE >= 0x0400

// lParam specifies why the endedit is happening
#ifdef UNICODE
#define CBEN_ENDEDIT CBEN_ENDEDITW
#else
#define CBEN_ENDEDIT CBEN_ENDEDITA
#endif

#define CBENF_KILLFOCUS         1
#define CBENF_RETURN            2
#define CBENF_ESCAPE            3
#define CBENF_DROPDOWN          4

#define CBEMAXSTRLEN 260


#if (_WIN32_IE >= 0x0400)
// CBEN_DRAGBEGIN sends this information ...

typedef struct {
    NMHDR hdr;
    int   iItemid;
    WCHAR szText[CBEMAXSTRLEN];
}NMCBEDRAGBEGINW, *PNMCBEDRAGBEGINW;


typedef struct {
    NMHDR hdr;
    int   iItemid;
    char szText[CBEMAXSTRLEN];
}NMCBEDRAGBEGINA, *PNMCBEDRAGBEGINA;

#ifdef UNICODE
#define  NMCBEDRAGBEGIN NMCBEDRAGBEGINW
#define  PNMCBEDRAGBEGIN PNMCBEDRAGBEGINW
#else
#define  NMCBEDRAGBEGIN NMCBEDRAGBEGINA
#define  PNMCBEDRAGBEGIN PNMCBEENDEDITA
#endif
#endif      // _WIN32_IE >= 0x0400


// CBEN_ENDEDIT sends this information...
// fChanged if the user actually did anything
// iNewSelection gives what would be the new selection unless the notify is failed
//                      iNewSelection may be CB_ERR if there's no match
typedef struct {
        NMHDR hdr;
        BOOL fChanged;
        int iNewSelection;
        WCHAR szText[CBEMAXSTRLEN];
        int iWhy;
} NMCBEENDEDITW, *PNMCBEENDEDITW;

typedef struct {
        NMHDR hdr;
        BOOL fChanged;
        int iNewSelection;
        char szText[CBEMAXSTRLEN];
        int iWhy;
} NMCBEENDEDITA, *PNMCBEENDEDITA;

#ifdef UNICODE
#define  NMCBEENDEDIT NMCBEENDEDITW
#define  PNMCBEENDEDIT PNMCBEENDEDITW
#else
#define  NMCBEENDEDIT NMCBEENDEDITA
#define  PNMCBEENDEDIT PNMCBEENDEDITA
#endif

#endif

#endif      // _WIN32_IE >= 0x0300

//====== Flat Scrollbar APIs=========================================
#if (_WIN32_IE >= 0x0400)
#ifndef NOFLATSBAPIS

#define WSB_PROP_CYVSCROLL  0x00000001L
#define WSB_PROP_CXHSCROLL  0x00000002L
#define WSB_PROP_CYHSCROLL  0x00000004L
#define WSB_PROP_CXVSCROLL  0x00000008L
#define WSB_PROP_CXHTHUMB   0x00000010L
#define WSB_PROP_CYVTHUMB   0x00000020L
#define WSB_PROP_VBKGCOLOR  0x00000040L
#define WSB_PROP_HBKGCOLOR  0x00000080L
#define WSB_PROP_VSTYLE     0x00000100L
#define WSB_PROP_HSTYLE     0x00000200L
#define WSB_PROP_WINSTYLE   0x00000400L
#define WSB_PROP_PALETTE    0x00000800L
#define WSB_PROP_MASK       0x00000FFFL

#define FSB_FLAT_MODE           2
#define FSB_ENCARTA_MODE        1
#define FSB_REGULAR_MODE        0

WINCOMMCTRLAPI BOOL WINAPI FlatSB_EnableScrollBar(HWND, int, UINT);
WINCOMMCTRLAPI BOOL WINAPI FlatSB_ShowScrollBar(HWND, int code, BOOL);

WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollRange(HWND, int code, LPINT, LPINT);
WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollInfo(HWND, int code, LPSCROLLINFO);
WINCOMMCTRLAPI int WINAPI FlatSB_GetScrollPos(HWND, int code);
WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollProp(HWND, int propIndex, LPINT);

WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollPos(HWND, int code, int pos, BOOL fRedraw);
WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollInfo(HWND, int code, LPSCROLLINFO, BOOL fRedraw);
WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollRange(HWND, int code, int min, int max, BOOL fRedraw);
WINCOMMCTRLAPI BOOL WINAPI FlatSB_SetScrollProp(HWND, UINT index, int newValue, BOOL);

WINCOMMCTRLAPI BOOL WINAPI InitializeFlatSB(HWND);
WINCOMMCTRLAPI HRESULT WINAPI UninitializeFlatSB(HWND);

#endif  //  NOFLATSBAPIS
#endif      // _WIN32_IE >= 0x0400

//====== TAB CONTROL ==========================================================
#ifndef NOTABCONTROL

#define TCS_TOOLTIPS            0x4000

#if (_WIN32_IE >= 0x0400)
#define TCS_EX_REGISTERDROP     0x00000002
#endif

#define TCM_GETTOOLTIPS         (TCM_FIRST + 45)
#define TabCtrl_GetToolTips(hwnd) \
        (HWND)SNDMSG((hwnd), TCM_GETTOOLTIPS, 0, 0L)


#define TCM_SETTOOLTIPS         (TCM_FIRST + 46)
#define TabCtrl_SetToolTips(hwnd, hwndTT) \
        (void)SNDMSG((hwnd), TCM_SETTOOLTIPS, (WPARAM)hwndTT, 0L)

#if (_WIN32_IE >= 0x0400)
#define TCN_GETOBJECT           (TCN_FIRST - 3)
#endif      // _WIN32_IE >= 0x0400

#endif  //  NOTABCONTROL

//====== ANIMATE CONTROL ======================================================

#ifndef NOANIMATE

#ifdef _WIN32

#define ANIMATE_CLASSW          L"SysAnimate32"
#define ANIMATE_CLASSA          "SysAnimate32"

#ifdef UNICODE
#define ANIMATE_CLASS           ANIMATE_CLASSW
#else
#define ANIMATE_CLASS           ANIMATE_CLASSA
#endif

// begin_r_commctrl

#define ACS_CENTER              0x0001
#define ACS_TRANSPARENT         0x0002
#define ACS_AUTOPLAY            0x0004
#if (_WIN32_IE >= 0x0300)
#define ACS_TIMER               0x0008  // don't use threads... use timers
#endif

// end_r_commctrl

#define ACM_OPENA               (WM_USER+100)
#define ACM_OPENW               (WM_USER+103)

#ifdef UNICODE
#define ACM_OPEN                ACM_OPENW
#else
#define ACM_OPEN                ACM_OPENA
#endif

#define ACM_PLAY                (WM_USER+101)
#define ACM_STOP                (WM_USER+102)


#define ACN_START               1
#define ACN_STOP                2


#define Animate_Create(hwndP, id, dwStyle, hInstance)   \
            CreateWindow(ANIMATE_CLASS, NULL,           \
                dwStyle, 0, 0, 0, 0, hwndP, (HMENU)(id), hInstance, NULL)

#define Animate_Open(hwnd, szName)          (BOOL)SNDMSG(hwnd, ACM_OPEN, 0, (LPARAM)(LPTSTR)(szName))
#define Animate_Play(hwnd, from, to, rep)   (BOOL)SNDMSG(hwnd, ACM_PLAY, (WPARAM)(UINT)(rep), (LPARAM)MAKELONG(from, to))
#define Animate_Stop(hwnd)                  (BOOL)SNDMSG(hwnd, ACM_STOP, 0, 0)
#define Animate_Close(hwnd)                 Animate_Open(hwnd, NULL)
#define Animate_Seek(hwnd, frame)           Animate_Play(hwnd, frame, frame, 1)
#endif

#endif      // NOANIMATE


//////////////////////////    IP Address edit control ////////////////////////

#if (_WIN32_IE >= 0x0400)

#ifndef NOIPADDRESS

// Messages sent to IPAddress controls

#define IPM_CLEARADDRESS (WM_USER+100) // no parameters
#define IPM_SETADDRESS   (WM_USER+101) // lparam = TCP/IP address
#define IPM_GETADDRESS   (WM_USER+102) // lresult = # of non black fields.  lparam = LPDWORD for TCP/IP address
#define IPM_SETRANGE (WM_USER+103) // wparam = field, lparam = range
#define IPM_SETFOCUS (WM_USER+104) // wparam = field
#define IPM_ISBLANK  (WM_USER+105) // no parameters

#define WC_IPADDRESSW           L"SysIPAddress32"
#define WC_IPADDRESSA           "SysIPAddress32"

#ifdef UNICODE
#define WC_IPADDRESS          WC_IPADDRESSW
#else
#define WC_IPADDRESS          WC_IPADDRESSA
#endif

#define IPN_FIELDCHANGED                (IPN_FIRST - 0)
typedef struct tagNMIPADDRESS
{
        NMHDR hdr;
        int iField;
        int iValue;
} NMIPADDRESS, *LPNMIPADDRESS;

// The following is a useful macro for passing the range values in the
// IPM_SETRANGE message.

#define MAKEIPRANGE(low, high)    ((LPARAM)(WORD)(((BYTE)(high) << 8) + (BYTE)(low)))

// And this is a useful macro for making the IP Address to be passed
// as a LPARAM.

#define MAKEIPADDRESS(b1,b2,b3,b4)  ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

// Get individual number
#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)


#endif // NOIPADDRESS

#endif      // _WIN32_IE >= 0x0400

//====== TrackMouseEvent  =====================================================

#ifdef _WIN32

#ifndef NOTRACKMOUSEEVENT

//
// If the messages for TrackMouseEvent have not been defined then define them
// now.
//
#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSELEAVE                   0x02A3
#endif

//
// If the TRACKMOUSEEVENT structure and associated flags havent been declared
// then declare them now.
//
#ifndef TME_HOVER

#define TME_HOVER       0x00000001
#define TME_LEAVE       0x00000002
#define TME_QUERY       0x40000000
#define TME_CANCEL      0x80000000


#define HOVER_DEFAULT   0xFFFFFFFF

typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

#endif // !TME_HOVER

//
// Declare _TrackMouseEvent.  This API tries to use the window manager's
// implementation of TrackMouseEvent if it is present, otherwise it emulates.
//
WINCOMMCTRLAPI
BOOL
WINAPI
_TrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack);

#endif // !NOTRACKMOUSEEVENT

#endif /* _WIN32 */

//++++++++++++++++++++++++++ NOT SUPPORTED ON WINCE ++++++++++++++++++++ 
#endif // UNDER_CE

#ifndef UNDER_CE
WINCOMMCTRLAPI LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr);
WINCOMMCTRLAPI LRESULT WINAPI SendNotifyEx(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr, BOOL bUnicode);
#endif // UNDER_CE

#define NM_STARTWAIT            (NM_FIRST-9)
#define NM_ENDWAIT              (NM_FIRST-10)
#define NM_BTNCLK               (NM_FIRST-11)
// Message Filter Proc codes - These are defined above MSGF_USER
/////                           0x00000001  // don't use because some apps return 1 for all notifies
#define CDRF_NOTIFYITEMERASE    0x00000080   //
#define CDRF_VALIDFLAGS         0x000000F6   //


#define SSI_DEFAULT ((UINT)-1)


#define SSIF_SCROLLPROC    0x0001
#define SSIF_MAXSCROLLTIME 0x0002
#define SSIF_MINSCROLL     0x0003

typedef int (CALLBACK *PFNSMOOTHSCROLLPROC)(    HWND hWnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip ,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags);


typedef struct tagSSWInfo{
    UINT cbSize;
    DWORD fMask;
    HWND hwnd;
    int dx;
    int dy;
    LPCRECT lprcSrc;
    LPCRECT lprcClip;
    HRGN hrgnUpdate;
    LPRECT lprcUpdate;
    UINT fuScroll;

    UINT uMaxScrollTime;
    UINT cxMinScroll;
    UINT cyMinScroll;

    PFNSMOOTHSCROLLPROC pfnScrollProc;  // we'll call this back instead
} SMOOTHSCROLLINFO, *PSMOOTHSCROLLINFO;

WINCOMMCTRLAPI INT  WINAPI SmoothScrollWindow(PSMOOTHSCROLLINFO pssi);

#define SSW_EX_NOTIMELIMIT      0x00010000
#define SSW_EX_IMMEDIATE        0x00020000
#define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll



// ================ READER MODE ================

typedef BOOL (CALLBACK *PFNREADERSCROLL)(struct tagReaderModeInfo*, int, int);
typedef BOOL (CALLBACK *PFNREADERTRANSLATEDISPATCH)(LPMSG);
typedef struct tagReaderModeInfo
{
    UINT cbSize;
    HWND hwnd;
    DWORD fFlags;
    LPRECT prc;
    PFNREADERSCROLL pfnScroll;
    PFNREADERTRANSLATEDISPATCH pfnTranslateDispatch;

    LPARAM lParam;
} READERMODEINFO, *PREADERMODEINFO;

#define RMF_ZEROCURSOR          0x00000001
#define RMF_VERTICALONLY        0x00000002
#define RMF_HORIZONTALONLY      0x00000004

#define RM_SCROLLUNIT 20

WINCOMMCTRLAPI void WINAPI DoReaderMode(PREADERMODEINFO prmi);

// Cursors and Bitmaps used by ReaderMode
#ifdef RC_INVOKED
#define IDC_VERTICALONLY        109
#define IDC_HORIZONTALONLY      110
#define IDC_MOVE2D              111
#define IDC_NORTH               112
#define IDC_SOUTH               113
#define IDC_EAST                114
#define IDC_WEST                115
#define IDC_NORTHEAST           116
#define IDC_NORTHWEST           117
#define IDC_SOUTHEAST           118
#define IDC_SOUTHWEST           119

#define IDB_2DSCROLL    132
#define IDB_VSCROLL     133
#define IDB_HSCROLL     134
#else
#define IDC_VERTICALONLY        MAKEINTRESOURCE(109)
#define IDC_HORIZONTALONLY      MAKEINTRESOURCE(110)
#define IDC_MOVE2D              MAKEINTRESOURCE(111)
#define IDC_NORTH               MAKEINTRESOURCE(112)
#define IDC_SOUTH               MAKEINTRESOURCE(113)
#define IDC_EAST                MAKEINTRESOURCE(114)
#define IDC_WEST                MAKEINTRESOURCE(115)
#define IDC_NORTHEAST           MAKEINTRESOURCE(116)
#define IDC_NORTHWEST           MAKEINTRESOURCE(117)
#define IDC_SOUTHEAST           MAKEINTRESOURCE(118)
#define IDC_SOUTHWEST           MAKEINTRESOURCE(119)

#define IDB_2DSCROLL    MAKEINTRESOURCE(132)
#define IDB_VSCROLL     MAKEINTRESOURCE(133)
#define IDB_HSCROLL     MAKEINTRESOURCE(134)
#endif
#define ILC_COLORMASK           0x00FE
#define ILC_SHARED              0x0100      // this is a shareable image list
#define ILC_LARGESMALL          0x0200      // contains both large and small images
#define ILC_UNIQUE              0x0400      // makes sure no dup. image exists in list
#define ILC_VIRTUAL             0x8000      // enables ImageList_SetFilter
#define ILC_VALID   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE | ILC_VIRTUAL)   // legal implemented flags
#define ILD_BLENDMASK           0x000E
#define ILD_BLEND75             0x0008   // not implemented
#define OVERLAYMASKTOINDEX(i)   ((((i) >> 8) & (ILD_OVERLAYMASK >> 8))-1)
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetIconSize(HIMAGELIST himl, int FAR *cx, int FAR *cy);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetImageRect(HIMAGELIST himl, int i, RECT FAR* prcImage);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_Remove(HIMAGELIST himl, int i);
#define ILCF_VALID  (ILCF_SWAP)
typedef BOOL (CALLBACK *PFNIMLFILTER)(HIMAGELIST *, int *, LPARAM, BOOL);
WINCOMMCTRLAPI BOOL WINAPI ImageList_SetFilter(HIMAGELIST himl, PFNIMLFILTER pfnFilter, LPARAM lParamFilter);
#define HDS_VERT                0x0001
#define HDS_SHAREDIMAGELISTS    0x0000
#define HDS_PRIVATEIMAGELISTS   0x0010
#define HDS_OWNERDATA           0x0020
#define HDI_ALL                 0x00Bf
#define CMB_DISCARDABLE         0x01
/* Messages up to WM_USER+8 are reserved until we define more state bits */
/* Messages up to WM_USER+16 are reserved until we define more state bits */
#define IDB_STD_SMALL_MONO      2       /*  not supported yet */
#define IDB_STD_LARGE_MONO      3       /*  not supported yet */
#define IDB_VIEW_SMALL_MONO     6       /*  not supported yet */
#define IDB_VIEW_LARGE_MONO     7       /*  not supported yet */
#define STD_LAST                (STD_PRINT)     //
#define STD_MAX                 (STD_LAST + 1)  //
#define VIEW_LAST               (VIEW_VIEWMENU) //
#define VIEW_MAX                (VIEW_LAST + 1) //
#define HIST_LAST               (HIST_VIEWTREE) //
#define HIST_MAX                (HIST_LAST + 1) //
#define TB_SETBUTTONTYPE        (WM_USER + 34)
#ifdef _WIN32
#define TB_ADDBITMAP32          (WM_USER + 38)
#endif
#define TBBF_MONO               0x0002  /* not supported yet */
// since we don't have these for all the toolbar api's, we shouldn't expose any

#define ToolBar_SetButtonInfo(hwnd, idBtn, lptbbi)  \
    (BOOL)SNDMSG((hwnd), TB_SETBUTTONINFO, (WPARAM)(idBtn), (LPARAM)(lptbbi))

#define ToolBar_GetButtonInfo(hwnd, idBtn, lptbbi)  \
    (BOOL)SNDMSG((hwnd), TB_GETBUTTONINFO, (WPARAM)(idBtn), (LPARAM)(lptbbi))

#define ToolBar_SetStyle(hwnd, dwStyle)  \
    SNDMSG((hwnd), TB_SETSTYLE, 0, (LPARAM)(dwStyle))

#define ToolBar_GetStyle(hwnd)  \
    (DWORD)SNDMSG((hwnd), TB_GETSTYLE, 0, 0L)

#define ToolBar_GetHotItem(hwnd)  \
    (int)SNDMSG((hwnd), TB_GETHOTITEM, 0, 0L)

#define ToolBar_SetHotItem(hwnd, iHot)  \
    (int)SNDMSG((hwnd), TB_SETHOTITEM, (WPARAM)(iHot), 0L)

#define ToolBar_GetAnchorHighlight(hwnd)  \
    (BOOL)SNDMSG((hwnd), TB_GETANCHORHIGHLIGHT, 0, 0L)

#define ToolBar_SetAnchorHighlight(hwnd, bSet)  \
    SNDMSG((hwnd), TB_SETANCHORHIGHLIGHT, (WPARAM)(bSet), 0L)
// since we don't have these for all the toolbar api's, we shouldn't expose any
#define ToolBar_HitTest(hwnd, lppoint)  \
    (int)SNDMSG((hwnd), TB_HITTEST, 0, (LPARAM)(lppoint))
#define TBN_CLOSEUP             (TBN_FIRST - 11)  //
#ifndef _WIN32
// for compatibility with the old 16 bit WM_COMMAND hacks
typedef struct _ADJUSTINFO {
    TBBUTTON tbButton;
    char szDescription[1];
} ADJUSTINFO, NEAR* PADJUSTINFO, FAR* LPADJUSTINFO;
#define TBN_BEGINDRAG           0x0201
#define TBN_ENDDRAG             0x0203
#define TBN_BEGINADJUST         0x0204
#define TBN_ADJUSTINFO          0x0205
#define TBN_ENDADJUST           0x0206
#define TBN_RESET               0x0207
#define TBN_QUERYINSERT         0x0208
#define TBN_QUERYDELETE         0x0209
#define TBN_TOOLBARCHANGE       0x020a
#define TBN_CUSTHELP            0x020b
#endif

#ifndef UNDER_CE
#define RBS_VALID       (RBS_AUTOSIZE | RBS_TOOLTIPS | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP)
#else
#define RBS_VALID       (RBS_AUTOSIZE | RBS_TOOLTIPS | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_SMARTLABELS)
#endif // UNDER_CE
#define RBBS_DRAGBREAK      0x80000000  //
#define RB_GETBANDINFOOLD (WM_USER +  5)  //
#define RB_GETOBJECT    (WM_USER +  15) //
#define RB_PRIV_RESIZE   (WM_USER + 33)   //
#define RBN_BANDHEIGHTCHANGE (RBN_FIRST - 20) // send when the rebar auto changes the height of a variableheight band
#define TTF_STRIPACCELS         0x0008       // (this is implicit now)
#define TTF_UNICODE             0x0040       // Unicode Notify's
#define TTF_MEMALLOCED          0x0200
// SBS_* styles need to not overlap with CCS_* values

#define SB_SETBORDERS           (WM_USER+5)
// Warning +11-+13 are used in the unicode stuff above!
/*REVIEW: is this internal? */
/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOBTNLIST

/*REVIEW: should be BUTTONLIST_CLASS */
#define BUTTONLISTBOX           "ButtonListBox"

/* Button List Box Styles */
#define BLS_NUMBUTTONS          0x00FF
#define BLS_VERTICAL            0x0100
#define BLS_NOSCROLL            0x0200

/* Button List Box Messages */
#define BL_ADDBUTTON            (WM_USER+1)
#define BL_DELETEBUTTON         (WM_USER+2)
#define BL_GETCARETINDEX        (WM_USER+3)
#define BL_GETCOUNT             (WM_USER+4)
#define BL_GETCURSEL            (WM_USER+5)
#define BL_GETITEMDATA          (WM_USER+6)
#define BL_GETITEMRECT          (WM_USER+7)
#define BL_GETTEXT              (WM_USER+8)
#define BL_GETTEXTLEN           (WM_USER+9)
#define BL_GETTOPINDEX          (WM_USER+10)
#define BL_INSERTBUTTON         (WM_USER+11)
#define BL_RESETCONTENT         (WM_USER+12)
#define BL_SETCARETINDEX        (WM_USER+13)
#define BL_SETCURSEL            (WM_USER+14)
#define BL_SETITEMDATA          (WM_USER+15)
#define BL_SETTOPINDEX          (WM_USER+16)
#define BL_MSGMAX               (WM_USER+17)

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE            (-2)
#define BLN_SELCHANGE           1
#define BLN_CLICKED             2
#define BLN_SELCANCEL           3
#define BLN_SETFOCUS            4
#define BLN_KILLFOCUS           5

/* Message return values */
#define BL_OKAY                 0
#define BL_ERR                  (-1)
#define BL_ERRSPACE             (-2)

/* Create structure for                    */
/* BL_ADDBUTTON and                        */
/* BL_INSERTBUTTON                         */
/*   lpCLB = (LPCREATELISTBUTTON)lParam    */
typedef struct tagCREATELISTBUTTON {
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   /* button text */

} CREATELISTBUTTON, FAR* LPCREATELISTBUTTON;

#endif /* NOBTNLIST */
//=============================================================================

//
// Unnecessary to create a A and W version
// of this string since it is only passed
// to RegisterWindowMessage.
//
#define PBS_SHOWPERCENT         0x01
#define PBS_SHOWPOS             0x02


#define CCS_NOHILITE            0x00000010L
#define LVS_PRIVATEIMAGELISTS   0x0000
#define LVS_ALIGNBOTTOM         0x0400
#define LVS_ALIGNRIGHT          0x0c00
#define LVIF_ALL                0x001f
#define LVIF_VALID              0x081f
#define LVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff
#define LVIS_DISABLED           0x0010
#define LVIS_LINK               0x0040
#define LVIS_USERMASK           LVIS_STATEIMAGEMASK
#ifndef UNDER_CE
#define LVIS_ALL                0xFFFF
#endif //!UNDER_CE
#define STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
#define LVNI_PREVIOUS           0x0020
#define LVFI_SUBSTRING          0x0004
#define LVFI_NOCASE             0x0010
// the following #define's must be packed sequentially.
#define LVIR_MAX                4
#define LVA_ALIGNRIGHT          0x0003
#define LVA_ALIGNBOTTOM         0x0004
#define LVA_ALIGNMASK           0x0007
#define LVA_SORTASCENDING       0x0100
#define LVA_SORTDESCENDING      0x0200
#define LVCF_ALL                0x003f
#define LV_MAX_WORKAREAS         16
#define LVN_ENDDRAG             (LVN_FIRST-10)
#define LVN_ENDRDRAG            (LVN_FIRST-12)
#ifdef PW2
#define LVN_PEN                 (LVN_FIRST-20)
#endif
#define LVN_GETEMPTYTEXTA          (LVN_FIRST-60)
#define LVN_GETEMPTYTEXTW          (LVN_FIRST-61)

#ifdef UNICODE
#define LVN_GETEMPTYTEXT           LVN_GETEMPTYTEXTW
#else
#define LVN_GETEMPTYTEXT           LVN_GETEMPTYTEXTA
#endif
#define TVS_SHAREDIMAGELISTS    0x0000  //
#define TVS_PRIVATEIMAGELISTS   0x0400  //
#define TVIF_ALL                0x007F
#define TVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff
#define TVIS_FOCUSED            0x0001
#define TVIS_DISABLED           0
#define TVIS_ALL                0xFF7E
#define TVE_ACTIONMASK          0x0003      //  (TVE_COLLAPSE | TVE_EXPAND | TVE_TOGGLE)
#define TV_FINDITEM             (TV_FIRST + 3)
#define CBEN_ITEMCHANGED         (CBEN_FIRST - 3)  //
#define TCS_SHAREIMAGELISTS     0x0000
#define TCS_PRIVATEIMAGELISTS   0x0000
#define TCM_GETBKCOLOR          (TCM_FIRST + 0)
#define TabCtrl_GetBkColor(hwnd)  (COLORREF)SNDMSG((hwnd), TCM_GETBKCOLOR, 0, 0L)
#define TCM_SETBKCOLOR          (TCM_FIRST + 1)
#define TabCtrl_SetBkColor(hwnd, clrBk)  (BOOL)SNDMSG((hwnd), TCM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))
#define TCIF_ALL                0x001f
#define TCM_GETOBJECT           (TCM_FIRST + 54)
#define TabCtrl_GetObject(hwnd, piid, ppv) \
        (DWORD)SNDMSG((hwnd), TCM_GETOBJECT, (WPARAM)piid, (LRESULT)ppv)
#define MCSC_COLORCOUNT   6   //
// NOTE: this was MCN_FIRST + 2 but I changed it when I changed the structre //
#define MCS_VALIDBITS       0x001F          //
#define MCS_INVALIDBITS     ((~MCS_VALIDBITS) & 0x0000FFFF) //
#define DTS_VALIDBITS       0x003F //
#define DTS_INVALIDBITS     ((~DTS_VALIDBITS) & 0x0000FFFF) //

#ifndef NO_COMMCTRL_DA
//====== Dynamic Array routines ==========================================
#define DA_LAST (0x7FFFFFFF)

// Dynamic structure array
typedef struct _DSA FAR* HDSA;

typedef int (CALLBACK *PFNDPAENUMCALLBACK)(LPVOID p, LPVOID pData);
typedef int (CALLBACK *PFNDSAENUMCALLBACK)(LPVOID p, LPVOID pData);

WINCOMMCTRLAPI HDSA   WINAPI DSA_Create(int cbItem, int cItemGrow);
WINCOMMCTRLAPI BOOL   WINAPI DSA_Destroy(HDSA hdsa);
WINCOMMCTRLAPI BOOL   WINAPI DSA_GetItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI LPVOID WINAPI DSA_GetItemPtr(HDSA hdsa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DSA_SetItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI int    WINAPI DSA_InsertItem(HDSA hdsa, int i, void FAR* pitem);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteItem(HDSA hdsa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteAllItems(HDSA hdsa);
WINCOMMCTRLAPI void   WINAPI DSA_EnumCallback(HDSA hdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData);
WINCOMMCTRLAPI void   WINAPI DSA_DestroyCallback(HDSA hdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData);
#define     DSA_GetItemCount(hdsa)      (*(int FAR*)(hdsa))
#define     DSA_AppendItem(hdsa, pitem) DSA_InsertItem(hdsa, DA_LAST, pitem)

// Dynamic pointer array
typedef struct _DPA FAR* HDPA;

WINCOMMCTRLAPI HDPA   WINAPI DPA_Create(int cItemGrow);
WINCOMMCTRLAPI HDPA   WINAPI DPA_CreateEx(int cpGrow, HANDLE hheap);
WINCOMMCTRLAPI BOOL   WINAPI DPA_Destroy(HDPA hdpa);
WINCOMMCTRLAPI HDPA   WINAPI DPA_Clone(HDPA hdpa, HDPA hdpaNew);
WINCOMMCTRLAPI LPVOID WINAPI DPA_GetPtr(HDPA hdpa, int i);
WINCOMMCTRLAPI int    WINAPI DPA_GetPtrIndex(HDPA hdpa, LPVOID p);
WINCOMMCTRLAPI BOOL   WINAPI DPA_Grow(HDPA pdpa, int cp);
WINCOMMCTRLAPI BOOL   WINAPI DPA_SetPtr(HDPA hdpa, int i, LPVOID p);
WINCOMMCTRLAPI int    WINAPI DPA_InsertPtr(HDPA hdpa, int i, LPVOID p);
WINCOMMCTRLAPI LPVOID WINAPI DPA_DeletePtr(HDPA hdpa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DPA_DeleteAllPtrs(HDPA hdpa);
WINCOMMCTRLAPI void   WINAPI DPA_EnumCallback(HDPA hdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData);
WINCOMMCTRLAPI void   WINAPI DPA_DestroyCallback(HDPA hdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData);
#define     DPA_GetPtrCount(hdpa)       (*(int FAR*)(hdpa))
#define     DPA_GetPtrPtr(hdpa)         (*((LPVOID FAR* FAR*)((BYTE FAR*)(hdpa) + sizeof(int))))
#define     DPA_FastGetPtr(hdpa, i)     (DPA_GetPtrPtr(hdpa)[i])
#define     DPA_AppendPtr(hdpa, pitem)  DPA_InsertPtr(hdpa, DA_LAST, pitem)

#ifdef __IStream_INTERFACE_DEFINED__
// Save to and load from a stream.  The stream callback gets a pointer to
// a DPASTREAMINFO structure.
//
// For DPA_SaveStream, the callback is responsible for writing the pvItem
// info to the stream.  (It's not necessary to write the iPos to the
// stream.)  Return S_OK if the element was saved, S_FALSE if it wasn't
// but continue anyway, or some failure.
//
// For DPA_LoadStream, the callback is responsible for allocating an
// item and setting the pvItem field to the new pointer.  Return S_OK
// if the element was loaded, S_FALSE it it wasn't but continue anyway,
// or some failure.
//

typedef struct _DPASTREAMINFO
{
    int    iPos;        // Index of item
    LPVOID pvItem;
} DPASTREAMINFO;

typedef HRESULT (CALLBACK *PFNDPASTREAM)(DPASTREAMINFO * pinfo, IStream * pstream, LPVOID pvInstData);

WINCOMMCTRLAPI HRESULT WINAPI DPA_LoadStream(HDPA * phdpa, PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData);
WINCOMMCTRLAPI HRESULT WINAPI DPA_SaveStream(HDPA hdpa, PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData);
#endif

typedef int (CALLBACK *PFNDPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);

WINCOMMCTRLAPI BOOL   WINAPI DPA_Sort(HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam);

// Merge two DPAs.  This takes two (optionally) presorted arrays and merges
// the source array into the dest.  DPA_Merge uses the provided callbacks
// to perform comparison and merge operations.  The merge callback is
// called when two elements (one in each list) match according to the
// compare function.  This allows portions of an element in one list to
// be merged with the respective element in the second list.
//
// The first DPA (hdpaDest) is the output array.
//
// Merge options:
//
//    DPAM_SORTED       The arrays are already sorted; don't sort
//    DPAM_UNION        The resulting array is the union of all elements
//                      in both arrays (DPAMM_INSERT may be sent for
//                      this merge option.)
//    DPAM_INTERSECT    Only elements in the source array that intersect
//                      with the dest array are merged.  (DPAMM_DELETE
//                      may be sent for this merge option.)
//    DPAM_NORMAL       Like DPAM_INTERSECT except the dest array
//                      also maintains its original, additional elements.
//
#define DPAM_SORTED             0x00000001
#define DPAM_NORMAL             0x00000002
#define DPAM_UNION              0x00000004
#define DPAM_INTERSECT          0x00000008

// The merge callback should merge contents of the two items and return
// the pointer of the merged item.  It's okay to simply use pvDest
// as the returned pointer.
//
typedef LPVOID (CALLBACK *PFNDPAMERGE)(UINT uMsg, LPVOID pvDest, LPVOID pvSrc, LPARAM lParam);

// Messages for merge callback
#define DPAMM_MERGE     1
#define DPAMM_DELETE    2
#define DPAMM_INSERT    3

WINCOMMCTRLAPI BOOL WINAPI DPA_Merge(HDPA hdpaDest, HDPA hdpaSrc, DWORD dwFlags, PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge, LPARAM lParam);


// Search array.  If DPAS_SORTED, then array is assumed to be sorted
// according to pfnCompare, and binary search algorithm is used.
// Otherwise, linear search is used.
//
// Searching starts at iStart (0 to start search at beginning).
//
// DPAS_INSERTBEFORE/AFTER govern what happens if an exact match is not
// found.  If neither are specified, this function returns -1 if no exact
// match is found.  Otherwise, the index of the item before or after the
// closest (including exact) match is returned.
//
// Search option flags
//
#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004

WINCOMMCTRLAPI int WINAPI DPA_Search(HDPA hdpa, LPVOID pFind, int iStart,
                      PFNDPACOMPARE pfnCompare,
                      LPARAM lParam, UINT options);

//======================================================================
// String management helper routines

WINCOMMCTRLAPI int  WINAPI Str_GetPtrA(LPCSTR psz, LPSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI int  WINAPI Str_GetPtrW(LPCWSTR psz, LPWSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrA(CHAR FAR* UNALIGNED * ppsz, LPCSTR psz);
WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrW(WCHAR FAR* UNALIGNED * ppsz, LPCWSTR psz);

#ifdef UNICODE
#define Str_GetPtr              Str_GetPtrW
#define Str_SetPtr              Str_SetPtrW
#else
#define Str_GetPtr              Str_GetPtrA
#define Str_SetPtr              Str_SetPtrA
#endif

#endif // NO_COMMCTRL_DA

#ifndef NO_COMMCTRL_ALLOCFCNS
//====== Memory allocation functions ===================

#ifdef _WIN32
#define _huge
#endif

WINCOMMCTRLAPI void _huge* WINAPI Alloc(long cb);
WINCOMMCTRLAPI void _huge* WINAPI ReAlloc(void _huge* pb, long cb);
WINCOMMCTRLAPI BOOL        WINAPI Free(void _huge* pb);
WINCOMMCTRLAPI DWORD       WINAPI GetSize(void _huge* pb);

#endif


#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifdef _WIN32
//===================================================================
typedef int (CALLBACK *MRUCMPPROCA)(LPCSTR, LPCSTR);
typedef int (CALLBACK *MRUCMPPROCW)(LPCWSTR, LPCWSTR);

#ifdef UNICODE
#define MRUCMPPROC              MRUCMPPROCW
#else
#define MRUCMPPROC              MRUCMPPROCA
#endif

// This is cdecl - to be compatible with the crts.
typedef int (cdecl FAR *MRUCMPDATAPROC)(const void FAR *, const void FAR *,
                                        size_t);



typedef struct _MRUINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPPROCA lpfnCompare;
} MRUINFOA, FAR *LPMRUINFOA;

typedef struct _MRUINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPPROCW lpfnCompare;
} MRUINFOW, FAR *LPMRUINFOW;

typedef struct _MRUDATAINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOA, FAR *LPMRUDATAINFOA;

typedef struct _MRUDATAINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOW, FAR *LPMRUDATAINFOW;


#ifdef UNICODE
#define MRUINFO                 MRUINFOW
#define LPMRUINFO               LPMRUINFOW
#define MRUDATAINFO             MRUDATAINFOW
#define LPMRUDATAINFO           LPMRUDATAINFOW
#else
#define MRUINFO                 MRUINFOA
#define LPMRUINFO               LPMRUINFOA
#define MRUDATAINFO             MRUDATAINFOA
#define LPMRUDATAINFO           LPMRUDATAINFOA
#endif

#define MRU_BINARY              0x0001
#define MRU_CACHEWRITE          0x0002
#define MRU_ANSI                0x0004


WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListA(LPMRUINFOA lpmi);
WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListW(LPMRUINFOW lpmi);
WINCOMMCTRLAPI void   WINAPI FreeMRUList(HANDLE hMRU);
WINCOMMCTRLAPI int    WINAPI AddMRUStringA(HANDLE hMRU, LPCSTR szString);
WINCOMMCTRLAPI int    WINAPI AddMRUStringW(HANDLE hMRU, LPCWSTR szString);
WINCOMMCTRLAPI int    WINAPI DelMRUString(HANDLE hMRU, int nItem);
WINCOMMCTRLAPI int    WINAPI FindMRUStringA(HANDLE hMRU, LPCSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI FindMRUStringW(HANDLE hMRU, LPCWSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI EnumMRUListA(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen);
WINCOMMCTRLAPI int    WINAPI EnumMRUListW(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen);

WINCOMMCTRLAPI int    WINAPI AddMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData);
WINCOMMCTRLAPI int    WINAPI FindMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData,
                          LPINT lpiSlot);

#ifdef UNICODE
#define CreateMRUList           CreateMRUListW
#define AddMRUString            AddMRUStringW
#define FindMRUString           FindMRUStringW
#define EnumMRUList             EnumMRUListW
#else
#define CreateMRUList           CreateMRUListA
#define AddMRUString            AddMRUStringA
#define FindMRUString           FindMRUStringA
#define EnumMRUList             EnumMRUListA
#endif

#endif

//=========================================================================

#ifdef _WIN32
#define DPA_CreateORD           328
#define DPA_DestroyORD          329
#define DPA_GrowORD             330
#define DPA_CloneORD            331
#define DPA_GetPtrORD           332
#define DPA_GetPtrIndexORD      333
#define DPA_InsertPtrORD        334
#define DPA_SetPtrORD           335
#define DPA_DeletePtrORD        336
#define DPA_DeleteAllPtrsORD    337
#define DPA_SortORD             338
#define DPA_SearchORD           339
#define DPA_CreateExORD         340
#define SendNotifyORD           341
#define CreatePageORD           163
#define CreateProxyPageORD      164
#endif
#define WM_TRACKMOUSEEVENT_FIRST        0x02A0
#define WM_TRACKMOUSEEVENT_LAST         0x02AF
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL) //
//====== SetPathWordBreakProc  ======================================
void WINAPI SetPathWordBreakProc(HWND hwndEdit, BOOL fSet);

#ifdef __cplusplus
}
#endif

#ifndef UNDER_CE
#ifdef _WIN32
#include <poppack.h>
#endif
#endif // UNDER_CE

#endif

#endif  // _INC_COMMCTRLP
