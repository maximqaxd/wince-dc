/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

pwinuser.h

Abstract:

Private portion of winuser.h

Notes:


--*/


#ifndef _PRIV_WINUSER_
#define _PRIV_WINUSER_

#ifdef __cplusplus
extern "C"  {
#endif

#include <keybd.h>

//	Non-Standard CE internal messages.
#define WM_QM_ACTIVATE		0x60
#define WM_HOOK_DO_CALLBACK	0x61
#define WM_SYSCOPYDATA		0x62


//	wparam values for WM_SYSCOPYDATA
#define WMSCD_CHAR_RANKED_ALTS	0x01



/*
 * Extended Window Styles
 */
#if(WINVER >= 0x0400)

#endif /* WINVER >= 0x0400 */

//	Marker for end of accelerator tables.
#define FLASTKEY  0x80

//	SetWindowPos internal flags
#define SWP_SYSTEMCAPTURE			0x80000000
#define SWP_FINDNEWFOREGROUND		0x40000000
#define SWP_MAXMIN					0x20000000


//	Internal values for hwnd for use in broadcasting messages
//	Must be odd numbered.
#define HWND_TOPLEVELVISIBLEORPRE30		((HWND)0xfffd)	// top level windows that are visible on screen or ver < 3.0
#define HWND_TOPLEVELCHILD				((HWND)0xfffb)	// top level windows with the WS_CHILD bit set

/*
 * Predefined Clipboard Formats
 */
#define CF_METAFILEPICT     3
#define CF_ENHMETAFILE      14
#define IMAGE_ENHMETAFILE   3

void WINAPI SystemIdleTimerReset( void );

HWND WINAPI SetKeyboardTarget( HWND hWnd );
HWND WINAPI GetKeyboardTarget( VOID );
KEY_STATE_FLAGS WINAPI GetAsyncShiftFlags(UINT VKey);

void AudioUpdateFromRegistry(void);

/*	Values for NotifyWinUserSystem */
#define NWUS_DOUBLE_TAP_CHANGED	1
#define NWUS_KEYBD_REPEAT_CHANGED 2
#define NWUS_MAX_IDLE_TIME_CHANGED 3

#define SPI_SETNOSEHFORVALIDATION	250

#define SPI_PROJECT1	224
#define SPI_PROJECT2	225
#define SPI_PROJECT3	226
#define SPI_PROJECT4	227
#define SPI_PROJECT5	228
#define SPI_PROJECT6	229
#define SPI_PROJECT7	230
#define SPI_PROJECT8	231
#define SPI_PROJECT9	232
#define SPI_PROJECT10	233
#define SPI_PROJECT11	234
#define SPI_PROJECT12	235
#define SPI_PROJECT13	236
#define SPI_PROJECT14	237
#define SPI_PROJECT15	238
#define SPI_PROJECT16	239

#define VK_APP_LAUNCH1	0xC1
#define VK_APP_LAUNCH2	0xC2
#define VK_APP_LAUNCH3	0xC3
#define VK_APP_LAUNCH4	0xC4
#define VK_APP_LAUNCH5	0xC5
#define VK_APP_LAUNCH6	0xC6
#define VK_APP_LAUNCH7	0xC7
#define VK_APP_LAUNCH8	0xC8
#define VK_APP_LAUNCH9	0xC9
#define VK_APP_LAUNCH10	0xCA
#define VK_APP_LAUNCH11	0xCB
#define VK_APP_LAUNCH12	0xCC
#define VK_APP_LAUNCH13	0xCD
#define VK_APP_LAUNCH14	0xCE
#define VK_APP_LAUNCH15	0xCF




void WINAPI NotifyWinUserSystem( UINT	uEvent );
BOOL WINAPI NLedGetDeviceInfo( UINT	nInfoId, void	*pOutput );
BOOL WINAPI NLedSetDevice( UINT	nDeviceId, void	*pInput );

BOOL WINAPI RegisterTaskBar (HWND hwndTaskbar);

WINUSERAPI BOOL WINAPI RectangleAnimation ( HWND hWnd, LPCRECT prc, BOOL fMaximize ) ;
void WINAPI ShellModalEnd( void );

//#define ExitWindows(dwReserved, Code) ExitWindowsEx(EWX_LOGOFF, 0xFFFFFFFF)
//WINUSERAPI BOOL WINAPI ExitWindowsEx( UINT uFlags,DWORD dwReserved);
void GwesPowerOffSystem (void);

void WINAPI BatteryGetLifeTimeInfo ( LPSYSTEMTIME pstLastChange, DWORD * pcmsCpuUsage, DWORD * pcmsPreviousCpuUsage );
LONG WINAPI BatteryDrvrGetLevels ( void );
BOOL WINAPI BatteryDrvrSupportsChangeNotification ( void );
void WINAPI BatteryNotifyOfTimeChange ( BOOL fForward, FILETIME * pftDelta );

// A system class built in to GWE for use by everyone
#define CS_SYSTEMCLASS 0x80000000

/* flags for DrawFrameControl */
//#define DFC_CAPTION             1
#define DFC_MENU                2
//#define DFC_SCROLL              3
//#define DFC_BUTTON              4

//#define DFCS_CAPTIONCLOSE       0x0000
//#define DFCS_CAPTIONMIN         0x0001
//#define DFCS_CAPTIONMAX         0x0002
//#define DFCS_CAPTIONRESTORE     0x0003
//#define DFCS_CAPTIONHELP        0x0004

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002
#define DFCS_MENUARROWRIGHT     0x0004

//#define DFCS_SCROLLUP           0x0000
//#define DFCS_SCROLLDOWN         0x0001
//#define DFCS_SCROLLLEFT         0x0002
//#define DFCS_SCROLLRIGHT        0x0003
//#define DFCS_SCROLLCOMBOBOX     0x0005
#define DFCS_SCROLLSIZEGRIP     0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

//#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002
//#define DFCS_BUTTONRADIO        0x0004
//#define DFCS_BUTTON3STATE       0x0008
//#define DFCS_BUTTONPUSH         0x0010

//#define DFCS_INACTIVE           0x0100
//#define DFCS_PUSHED             0x0200
//#define DFCS_CHECKED            0x0400
#define DFCS_ADJUSTRECT         0x2000
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000

//WINUSERAPI BOOL    WINAPI DrawFrameControl(HDC, LPRECT, UINT, UINT);

// NC messages
#define WM_NCCREATE                     0x0081
#define WM_NCDESTROY                    0x0082
#define WM_NCCALCSIZE                   0x0083
#define WM_NCHITTEST                    0x0084
#define WM_NCPAINT                      0x0085
#define WM_NCACTIVATE                   0x0086

#define WM_NCMOUSEFIRST					0x00A0
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCRBUTTONDBLCLK              0x00A6
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_NCMBUTTONDBLCLK              0x00A9
#define WM_NCMOUSELAST					0x00A9

#ifdef FAREAST
/* Class style for IME UI window */
#define CS_IME      0x00010000
#endif // FAREAST

#ifdef _WIN32_WCE_EMULATION
#ifdef CS_IME
#undef CS_IME
#endif
#define CS_IME      0x00010000
#endif

/* For internal APIs "exposed" to other internal pieces. */
#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif /*__cplusplus*/
#define INTAPI  EXTERN_C LONG __stdcall
#define INTAPI_(type) EXTERN_C type __stdcall

BOOL KeybdGetDeviceInfo( INT iIndex, LPVOID  lpOutput );
BOOL KeybdInitStates( KEY_STATE KeyState, void  *pKeybdDriverState );

UINT32 KeybdVKeyToUnicode(
    UINT32          VKey,
    KEY_STATE_FLAGS KeyEventFlags,
    KEY_STATE       KeyState,
    void            *KeybdDriverState,
    UINT32          cBufferSize,
    UINT32          *pcCharacters,
    KEY_STATE_FLAGS *pShiftStateBuffer,
    UINT32          *pCharacterBuffer
    );


#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD_LL      20


/*
 * Hook Codes
 */
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREMOVE         3
#define HC_NOREM            HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

typedef LRESULT (CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);

/*
 * Message Structure used in Journaling
 */
typedef struct tagEVENTMSG {
    UINT    message;
    UINT    paramL;
    UINT    paramH;
    DWORD    time;
    HWND     hwnd;
} EVENTMSG, *PEVENTMSGMSG, NEAR *NPEVENTMSGMSG, FAR *LPEVENTMSGMSG;

typedef struct tagEVENTMSG *PEVENTMSG, NEAR *NPEVENTMSG, FAR *LPEVENTMSG;


HHOOK
WINAPI
QASetWindowsJournalHook(
	int			nFilterType,
	HOOKPROC	pfnFilterProc,
	EVENTMSG	*pfnEventMsg
	);


BOOL
WINAPI
QAUnhookWindowsJournalHook(
	int			nFilterType
	);


BOOL
WINAPI
GetMessageWNoWait(
    PMSG    pMsg,
    HWND    hWnd,
    UINT    wMsgFilterMin,
    UINT    wMsgFilterMax
    );

BOOL
WINAPI
TouchCalibrate(
	void
	);

int
WINAPI
DisableCaretSystemWide(
	void
	);


int
WINAPI
EnableCaretSystemWide(
	void
	);


BOOL
WINAPI
EnableHardwareKeyboard(
	BOOL fEnable
	);

BOOL RegisterSIPanel (HWND hwndSIPanel);


void WINAPI SetAssociatedMenu (
	HWND hwnd,
	HMENU hmenu
	);

HMENU WINAPI GetAssociatedMenu (
	HWND hwnd
	);


BOOL
WINAPI
UnregisterFunc1(
	UINT	fsModifiers,
	UINT	vk
	);







typedef struct tagKBDLLHOOKSTRUCT {
	DWORD vkCode;		// virtual key code
	DWORD scanCode;		// scan code    DWORD flags;       // flags
	DWORD time;			// time stamp for this message
	DWORD dwExtraInfo;	// extra info from the driver or keybd_event
} KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

HHOOK
WINAPI
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId);
#define SetWindowsHookEx  SetWindowsHookExW


BOOL
WINAPI
UnhookWindowsHookEx(
    HHOOK hhk);

LRESULT
WINAPI
CallNextHookEx(
    HHOOK hhk,
    int nCode,
    WPARAM wParam,
    LPARAM lParam);

typedef struct tagGetForegroundInfo
	{
	HWND	hwndActive;
	HWND	hwndFocus;
	HWND	hwndMenu;
	HWND	hwndKeyboardDest;
	DWORD	fdwConversion;
	DWORD	fdwSentence;
	BOOL	fOpen;
	DWORD	dwCompStrLen;
	} GET_FOREGROUND_INFO;

BOOL
WINAPI
GetForegroundInfo(
	GET_FOREGROUND_INFO	*pgfi
	);

HWND GetForegroundKeyboardTarget(
	void
	);


BOOL
WINAPI
ImmGetConversionStatusForeground(
	DWORD	*pfdwConversion,
	DWORD	*pfdwSentence
	);



//	This is taken directly from pbt.h.  Not all values are used or supported.

#ifndef WM_POWERBROADCAST
#define WM_POWERBROADCAST               0x218
#endif

#define PBT_APMQUERYSUSPEND             0x0000
#define PBT_APMQUERYSTANDBY             0x0001

#define PBT_APMQUERYSUSPENDFAILED       0x0002
#define PBT_APMQUERYSTANDBYFAILED       0x0003

#define PBT_APMSUSPEND                  0x0004
#define PBT_APMSTANDBY                  0x0005

#define PBT_APMRESUMECRITICAL           0x0006
#define PBT_APMRESUMESUSPEND            0x0007
#define PBT_APMRESUMESTANDBY            0x0008

#define PBTF_APMRESUMEFROMFAILURE       0x00000001

#define PBT_APMBATTERYLOW               0x0009
#define PBT_APMPOWERSTATUSCHANGE        0x000A

#define PBT_APMOEMEVENT                 0x000B

#define PBT_APMUSERIDLE					0x000C
#define PBT_APMUSERACTIVE				0x000D



#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* _PRIV_WINUSER_ */
