/******************************************************************************
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

remnet.h

******************************************************************************/

#ifndef _REMNET_H_
#define _REMNET_H_

#include "ras.h"
#include "raserror.h"


// Function prototypes
#ifdef USE_SIP
#include <sipapi.h>
typedef BOOL (WINAPI* LPFNSIP)(SIPINFO*);
typedef DWORD (WINAPI* LPFNSIPSTATUS)();
#endif

typedef BOOL     (*INITFUNC)(void);

#define	NUM_LV_COLUMNS	3

// The data in each list item.
typedef struct _ITEMINFO {
	TCHAR		EntryName[RAS_MaxEntryName+1];
	RASENTRY	Entry;			// Ras Entry
	TCHAR		szPhone[256];
} ITEMINFO, *PITEMINFO;


#define MIN(x,y) ((x < y) ? x : y)


// --------------------------------------------------------------------
//	
// NT DEBUG SUPPORT
//
// --------------------------------------------------------------------
#ifndef UNDER_CE
#ifdef DEBUG
_inline int
DbgPrint (TCHAR *pFormat, ...)
{
	va_list ArgList;
	TCHAR	Buffer[256];
	int		RetVal;

	va_start (ArgList, pFormat);

	RetVal = wvsprintf (Buffer, pFormat, ArgList);

	OutputDebugString (Buffer);

	return RetVal;
}
#define DEBUGMSG(exp,arg)	((exp) ? DbgPrint arg,1 : (0))
#define ASSERT(exp)	((exp) ? 1 : (DbgPrint (TEXT("ASSERT: %s %s %s"),#exp, __FILE__, __LINE__), DebugBreak()))
#define DEBUGZONE(b)    (DebugFlag&(0x00000001<<b))

extern DWORD	DebugFlag;

#else // DEBUG

// Retail defines
#define DEBUGMSG(exp,arg)	(0)
#define ASSERT(exp)			(0)

#endif // DEBUG
#endif // ndef UNDER_CE

// Debug zone definitions (for both CE and NT).
//#define ZONE_???		DEBUGZONE(1)
#define ZONE_MISC		DEBUGZONE(11)
#define ZONE_ALLOC		DEBUGZONE(12)
#define ZONE_FUNCTION	DEBUGZONE(13)
#define ZONE_WARN		DEBUGZONE(14)
#define ZONE_ERROR		DEBUGZONE(15)

#define DLG_PG_1	0
#define DLG_PG_2	1
#define DLG_PG_3	2
#define DLG_PG_4	3
#define DLG_PG_5	4
#define DLG_TCP_GEN	5
#define DLG_TCP_NS	6
#define DLG_NUMDLGS 7

#define SIP_UP      0
#define SIP_DOWN    1

// ----------------------------------------------------------------
//
// Global variables
//
// ----------------------------------------------------------------
extern HINSTANCE	v_hInst;
extern HWND			v_hMainWnd;
extern HWND			v_hListWnd;
extern HWND			v_hCmdBar;
extern HWND			v_hDialogWnd;
extern HFONT		v_hfont;
extern RECT			v_ClientRect;
extern RECT			v_ListRect;
extern DWORD		v_WizDialog;
extern DWORD		v_PrevWizDialog;
extern ITEMINFO		EditItem;	// The temp item to edit.
extern DWORD		v_DialogPages[];



// ----------------------------------------------------------------
//
// Function Declarations
//
// ----------------------------------------------------------------

// remnet.c
LRESULT CALLBACK
WndProc (HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam);
BOOL
InitApplication(HINSTANCE hInstance);
BOOL WINAPI
InitListViewImageLists(HWND hwndLV);
BOOL WINAPI
InitListViewColumns(HWND hwndLV);
BOOL WINAPI
InitListViewItems(HWND hwndLV);
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow);
void 
PositionSIP(int nSipState);


// remwiz.c
LRESULT CALLBACK
ConnWizDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


void CALLBACK 
lineCallbackFunc(DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
				 DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

int
My_atoi(LPTSTR);

BOOL
TCP_IP_Properties(HWND hWndOwner, PITEMINFO pItem);

#endif // ndef _REMNET_H_
