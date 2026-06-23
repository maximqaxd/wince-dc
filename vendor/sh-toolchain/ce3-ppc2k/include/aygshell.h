/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    aygshell.h

Abstract:

    Shell defines.

--*/

#ifndef __AYGSHELL_H__
#define __AYGSHELL_H__


#include <sipapi.h>


#ifdef __cplusplus
extern "C" {
#endif


//
// flags in the fdwFlags field of SIPINFO.
// some of these are defined in sipapi.h in the OS.
//
#define SIPF_DISABLECOMPLETION      0x08


//
// Supported system parameters.
//
#ifndef SPI_SETSIPINFO
#define SPI_SETSIPINFO          224
#endif
#define SPI_GETSIPINFO          225
#define SPI_SETCURRENTIM        226
#define SPI_GETCURRENTIM        227
#define SPI_SETCOMPLETIONINFO   223
#define SPI_APPBUTTONCHANGE     228


//Gryphon special controls
#define WC_SIPPREF    L"SIPPREF"

#define CEM_UPCASEALLWORDS    (WM_USER + 1)
#define CEM_ENABLEUPCASE      (WM_USER + 2)


//
// SHSipInfo function.
//
WINSHELLAPI
BOOL
SHSipInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni );

BYTE SHGetAppKeyAssoc( LPCTSTR ptszApp );
BOOL SHSetAppKeyWndAssoc( BYTE bVk, HWND hwnd );
BOOL SHInitExtraControls(void);
BOOL SHCloseApps( DWORD dwMemSought );


//++++++
//
// SHInitDialog 
//

typedef struct tagSHINITDLGINFO
{
    DWORD dwMask;
    HWND  hDlg;
    DWORD dwFlags;
} SHINITDLGINFO, *PSHINITDLGINFO;


//
// The functions
//

BOOL SHInitDialog(PSHINITDLGINFO pshidi);

//
// Valid mask values
//

#define SHIDIM_FLAGS                0x0001

//
// Valid flags
//

#define SHIDIF_DONEBUTTON           0x0001
#define SHIDIF_SIZEDLG              0x0002
#define SHIDIF_SIZEDLGFULLSCREEN    0x0004
#define SHIDIF_SIPDOWN              0x0008
#define SHIDIF_FULLSCREENNOMENUBAR  0x0010

//
// End SHInitDialog
//
//------


//++++++
//
// Shell Menubar support
//

#define NOMENU 0xFFFF
#define IDC_COMMANDBANDS    100

// These defines MUST be < 100.  This is so apps can use these defines
// to get strings from the shell.
#define IDS_SHNEW           1
#define IDS_SHEDIT          2
#define IDS_SHTOOLS         3
#define IDS_SHVIEW          4
#define IDS_SHFILE          5
#define IDS_SHGO            6
#define IDS_SHFAVORITES     7

//
// Shared New menu support
//
#define  IDM_SHAREDNEW        10
#define  IDM_SHAREDNEWDEFAULT 11

//
// Valid dwFlags
//
#define SHCMBF_EMPTYBAR      0x0001
#define SHCMBF_HIDDEN        0x0002 // create it hidden
#define SHCMBF_HIDESIPBUTTON 0x0004

typedef struct tagSHMENUBARINFO
{
    DWORD cbSize;               // IN  - Indicates which members of struct are valid
	HWND hwndParent;            // IN
    DWORD dwFlags;              // IN  - Some features we want
    UINT nToolBarId;            // IN  - Which toolbar are we using
    HINSTANCE hInstRes;         // IN  - Instance that owns the resources
    int nBmpId;
    int cBmpImages;             // IN  - Count of bitmap images
	HWND hwndMB;                // OUT
} SHMENUBARINFO, *PSHMENUBARINFO;

WINSHELLAPI BOOL  SHCreateMenuBar(SHMENUBARINFO *pmbi);

#define SHCMBM_SETSUBMENU   (WM_USER + 400)
#define SHCMBM_GETSUBMENU   (WM_USER + 401) // lParam == ID
#define SHCMBM_GETMENU      (WM_USER + 402) // get the owning hmenu (as specified in the load resource)

// wparam == id of button, lParam == hmenu
// return is old hmenu

//
// End Shell Menubar support
//
//------


//++++++
//
// SHHandleWMActivate and SHHandleWMSettingChange fun
//

typedef struct
{
    DWORD cbSize;
    HWND hwndLastFocus;
    UINT fSipUp :1;
    UINT fSipOnDeactivation :1;
    UINT fActive :1;
    UINT fReserved :29;
} SHACTIVATEINFO, *PSHACTIVATEINFO;

#define SHA_INPUTDIALOG 0x00000001

WINSHELLAPI BOOL SHHandleWMActivate(HWND hwnd, WPARAM wParam, LPARAM lParam, SHACTIVATEINFO* psai, DWORD dwFlags);
WINSHELLAPI BOOL SHHandleWMSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam, SHACTIVATEINFO* psai);

//
// End SHHandleWMActivate and SHHandleWMSettingChange fun
//
//------


//++++++
//
// SHSipPreference
//

typedef enum tagSIPSTATE
{
    SIP_UP = 0,
    SIP_DOWN,
	SIP_FORCEDOWN,
    SIP_UNCHANGED,
    SIP_INPUTDIALOG,
} SIPSTATE;

BOOL SHSipPreference(HWND hwnd, SIPSTATE st);

//
// End SHSipPreference
//
//------


//++++++
//
// New menu notifications
//

// get the application specific reg key for "new" menu items
#define  NMN_GETAPPREGKEY       1101
// Sent to app before shared new menu is destroyed.
#define  NMN_NEWMENUDESTROY     1102
// Sent to app before COM object is instantiated.
#define  NMN_INVOKECOMMAND      1103
// Sent to app when new button style changes
#define  NMN_NEWBUTTONUPDATED   1104

typedef struct tagNMNEWMENU 
{
    NMHDR hdr;
    TCHAR szReg[80];
    HMENU hMenu;
    CLSID clsid;
} NMNEWMENU, *PNMNEWMENU;

// For application added menu items.
#define IDM_NEWMENUMAX      3000

//
// End New menu notifications
//
//------


//++++++
//
// SHRecognizeGesture structs
//

typedef struct tagSHRGI {
    DWORD cbSize;
    HWND hwndClient;
    POINT ptDown;
    DWORD dwFlags;
} SHRGINFO, *PSHRGINFO;


//
// Gesture notifications
//
#define  GN_CONTEXTMENU       1000


//
// Gesture flags
//
#define  SHRG_RETURNCMD       0x00000001
#define  SHRG_NOTIFYPARENT    0x00000002
// use the longer (mixed ink) delay timer
// useful for cases where you might click down first, verify you're
// got the right spot, then start dragging... and it's not clear
// you wanted a context menu
#define  SHRG_LONGDELAY       0x00000008 

//
// Struct sent through WM_NOTIFY when SHRG_NOTIFYPARENT is used
//
typedef struct tagNMRGINFO 
{
    NMHDR hdr;
    POINT ptAction;
    DWORD dwItemSpec;
} NMRGINFO, *PNMRGINFO;

WINSHELLAPI DWORD SHRecognizeGesture(SHRGINFO *shrg);

//
// End SHRecognizeGesture
//
//------


//++++++
//
// SHFullScreen
//

BOOL SHFullScreen(HWND hwndRequester, DWORD dwState);


//
// Valid states
//

#define SHFS_SHOWTASKBAR            0x0001
#define SHFS_HIDETASKBAR            0x0002
#define SHFS_SHOWSIPBUTTON          0x0004
#define SHFS_HIDESIPBUTTON          0x0008
#define SHFS_SHOWSTARTICON          0x0010
#define SHFS_HIDESTARTICON          0x0020

//
// End SHFullScreen
//
//------


//++++++
//
// SHDoneButton
//

BOOL SHDoneButton(HWND hwndRequester, DWORD dwState);


//
// Valid states
//

#define SHDB_SHOW                   0x0001
#define SHDB_HIDE                   0x0002

//
// End SHDoneButton
//
//------


//++++++
//
// SHGetAutoRunPath
//	pAutoRunPath must be at least MAX_PATH long

BOOL SHGetAutoRunPath( LPTSTR pAutoRunPath );

//
// End SHGetAutoRunPath
//
//------

//++++++
//
// SHSetNavBarText
//

BOOL SHSetNavBarText(HWND hwndRequester, LPCTSTR pszText);

//
// End SHSetNavBarText
//
//------

//++++++
//
// SHInputDialog
//

void SHInputDialog(HWND hwnd, UINT uMsg, WPARAM wParam);

//
// End SHInputDialog
//
//------

#ifdef __cplusplus
}
#endif


#endif // __AYGSHELL_H__
