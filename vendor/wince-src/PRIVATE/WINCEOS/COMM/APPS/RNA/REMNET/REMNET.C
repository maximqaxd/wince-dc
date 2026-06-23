/******************************************************************************
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

remnet.c : Remote networking

******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "string.h"
#include "memory.h"
#include "commctrl.h"
#include "wcommctl.h"
//#include "shlobj.h"
//#include "wceshell.h"
//#include "aygshell.h"
#ifdef USE_SIP
#	include <sipapi.h>
#endif

#include "resource.h"
#include "remnet.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("RemoteNet"), {
        TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),
        TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),
        TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),TEXT("Misc"),
        TEXT("Alloc"),TEXT("Function"),TEXT("Warning"),TEXT("Error") },
    0x0000C000
    //0xFFFF
}; 
#endif	// DEBUG

ITEMINFO	EditItem;
HINSTANCE	v_hInst;
HWND		v_hMainWnd;
HWND		v_hListWnd;
HWND		v_hCmdBar;
HWND		v_hDialogWnd;
HFONT		v_hfont;
RECT		v_ClientRect;
RECT		v_ListRect;
DWORD		v_WizDialog;
DWORD       v_PrevWizDialog;
BOOL		v_fInRename;
BOOL		v_fPortrait;

#ifdef USE_SIP
LPFNSIP		  g_pSipGetInfo, g_pSipSetInfo;
LPFNSIPSTATUS g_pSipStatus;
HINSTANCE	  g_hSipLib;
#endif

extern DWORD v_EnteredAsAWizard;

DWORD		v_DialogPages[DLG_NUMDLGS] = {
	IDD_RAS_WIZ_1,
	IDD_RAS_WIZ_2,
	IDD_RAS_WIZ_3,
	IDD_RAS_WIZ_4,
	IDD_RAS_WIZ_5,
	IDD_RAS_TCPIP_GEN,
	IDD_RAS_TCPIP_NAME_SERV
};

const TCHAR szAppName[] = TEXT("RemoteNet");
TCHAR szTitle[36];

const static TBBUTTON tbButton[] = {
	{0,             0,        TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0, -1},
  {STD_DELETE+12,ID_FILE_DELETE,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
  {STD_PROPERTIES+12,ID_FILE_PROPERTIES,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
  {0, 0,              TBSTATE_ENABLED, TBSTYLE_SEP,               0, 0, 0, -1},
  {VIEW_LARGEICONS, ID_VIEW_LARGEICON,TBSTATE_ENABLED,TBSTYLE_CHECKGROUP,0,0,0,-1},
  {VIEW_SMALLICONS, ID_VIEW_SMALLICON,TBSTATE_ENABLED,TBSTYLE_CHECKGROUP,0,0,0,-1},
  {VIEW_DETAILS, ID_VIEW_DETAILS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0, 0, -1},
};    

static TBBUTTON tbButtonPortrait[] = {
	{0, 0,        TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0, -1},
	{0,ID_CONN_CONN,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
	{0, 0,        TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0, -1},
//	{0, ID_CONN_NEW,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
	{STD_PROPERTIES+12,ID_CONN_EDIT,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
	{STD_DELETE+12,ID_CONN_DELETE,TBSTATE_ENABLED,TBSTYLE_BUTTON,0,0,0,-1},
};    

TCHAR	nonStr[10];
TCHAR	deleteStr[30];
TCHAR	propertiesStr[30];
TCHAR	largeIconStr[30];
TCHAR	smallIconStr[30];
TCHAR	detailsStr[30];

const LPTSTR ToolTipsTbl[] = {
	nonStr,
	deleteStr,
	propertiesStr,
	largeIconStr,
	smallIconStr,
	detailsStr
};

const LPTSTR ToolTipsTblPortrait[] = {
	nonStr,
	largeIconStr,
	smallIconStr,
	propertiesStr,
	deleteStr,
};

// Declare the CommCtrl stuff.
DECLARE_COMMCTRL_TABLE;


void  FAR  PASCAL RegisterIPClass(HINSTANCE);
void  FAR  PASCAL UnregisterIPClass(HINSTANCE);

void PositionSIP(int nSipState)
{
#ifdef USE_SIP
    // Do we have the sip function?
    if (g_pSipGetInfo && g_pSipSetInfo)
    {
	    SIPINFO si;

        // See whether the SIP is up or down
        memset(&si, 0, sizeof(SIPINFO));
        si.cbSize = sizeof(SIPINFO);
        if ((*g_pSipGetInfo)(&si))
        {
            // Has the SIP state changed?
            if ((!(si.fdwFlags & SIPF_ON) && SIP_UP == nSipState) ||
                (si.fdwFlags & SIPF_ON && !(SIP_UP == nSipState)))
            {
                si.fdwFlags ^= SIPF_ON;
		        (*g_pSipSetInfo)(&si);
            }
        }
    }
#endif
}

BOOL
CreateShortcut(TCHAR *pEntryName)
{
    TCHAR   szFmtStr[256];
    TCHAR   szPath[256];
    TCHAR   szFile[256];
    char    szAnsiTarget[128];
	HANDLE  hFile;
    int     nLinks = 1;
    int     Len;
	
	LoadString(v_hInst, IDS_DESKTOP_PATH, szFmtStr,
			   sizeof(szFmtStr)/sizeof(TCHAR));
    
    wsprintf(szPath, szFmtStr, pEntryName);
    
	hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_NEW,
					   FILE_ATTRIBUTE_NORMAL, NULL);

    while (hFile == INVALID_HANDLE_VALUE)
    {
        if (nLinks++ > 20)
            return FALSE;

		LoadString(v_hInst, IDS_DESKTOP_PATH2, szFmtStr,
				   sizeof(szFmtStr)/sizeof(TCHAR));
    
        wsprintf(szPath, szFmtStr,
                 pEntryName, nLinks);

        hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);        
    }

    
	LoadString(v_hInst, IDS_SHORTCUT_STRING, szFmtStr,
			   sizeof(szFmtStr)/sizeof(TCHAR));
    
    wsprintf(szFile, szFmtStr, pEntryName);

    Len = _tcslen(szFile);

    wsprintf(szPath, TEXT("%d#%s"), Len, szFile);
    
    Len = WideCharToMultiByte(CP_ACP, 0, szPath, -1, szAnsiTarget, 128, NULL, NULL);

    WriteFile(hFile, szAnsiTarget, Len, &Len, NULL);

    CloseHandle(hFile);
    
    return TRUE;
}
VOID
UpdateView (DWORD	What)
{
	HMENU	hMenu;
	
	hMenu = CallCommCtrlFunc(CommandBar_GetMenu)(v_hCmdBar, 0);

    if (ID_VIEW_LARGEICON == What)
         CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICON,
                                   ID_VIEW_DETAILS,
                                   ID_VIEW_LARGEICON,
                                   MF_BYCOMMAND);
    if (ID_VIEW_SMALLICON == What)
         CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICON,
                                   ID_VIEW_DETAILS,
                                   ID_VIEW_SMALLICON,
                                   MF_BYCOMMAND);
    if (ID_VIEW_DETAILS == What)
         CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICON,
                                   ID_VIEW_DETAILS,
                                   ID_VIEW_DETAILS,
                                   MF_BYCOMMAND);

	SendMessage (v_hCmdBar, TB_CHECKBUTTON, ID_VIEW_LARGEICON,
				 MAKELONG((ID_VIEW_LARGEICON == What) ? TRUE : FALSE, 0));
	SendMessage (v_hCmdBar, TB_CHECKBUTTON, ID_VIEW_SMALLICON,
				 MAKELONG((ID_VIEW_SMALLICON == What) ? TRUE : FALSE, 0));
	SendMessage (v_hCmdBar, TB_CHECKBUTTON, ID_VIEW_DETAILS,
				 MAKELONG((ID_VIEW_DETAILS == What) ? TRUE : FALSE, 0));
}

VOID
SetMenu()
{
	HMENU	hMenu;
	DWORD		    cb;
	LV_ITEM		    lvi;
    MENUITEMINFO    MenuItemInfo;
	TCHAR		    szTemp[128];
	
	cb = ListView_GetSelectedCount (v_hListWnd);
	lvi.iItem = ListView_GetNextItem(v_hListWnd, -1, LVNI_SELECTED);
	hMenu = CallCommCtrlFunc(CommandBar_GetMenu)(v_hCmdBar, 0);
	// Set the File->Connect/Create menu to connect
	MenuItemInfo.cbSize = sizeof(MENUITEMINFO);
	MenuItemInfo.fMask = MIIM_TYPE;
	MenuItemInfo.dwTypeData = szTemp;
	MenuItemInfo.cch = sizeof(szTemp)/sizeof(TCHAR);

	GetMenuItemInfo(hMenu, ID_FILE_CONNECTCREATE, FALSE,
					&MenuItemInfo);
	MenuItemInfo.fMask = MIIM_TYPE;                        
	MenuItemInfo.cch = LoadString(v_hInst, IDS_CONNECT,
								  MenuItemInfo.dwTypeData,
								  sizeof(szTemp)/sizeof(TCHAR));
	SetMenuItemInfo(hMenu, ID_FILE_CONNECTCREATE, FALSE,
					&MenuItemInfo);

	EnableMenuItem (hMenu, ID_FILE_CONNECTCREATE, MF_GRAYED);
	EnableMenuItem (hMenu, ID_FILE_CREATESHORTCUT, MF_GRAYED);
	EnableMenuItem (hMenu, ID_FILE_DELETE, MF_GRAYED);
	EnableMenuItem (hMenu, ID_FILE_RENAME, MF_GRAYED);
	EnableMenuItem (hMenu, ID_FILE_PROPERTIES, MF_GRAYED);
	EnableMenuItem (hMenu, ID_EDIT_CREATECOPY, MF_GRAYED);

	// Portrait mode menu
	EnableMenuItem (hMenu, ID_CONN_CONN, MF_GRAYED);
	EnableMenuItem (hMenu, ID_CONN_EDIT, MF_GRAYED);
	EnableMenuItem (hMenu, ID_CONN_DELETE, MF_GRAYED);
	EnableMenuItem (hMenu, ID_CONN_RENAME, MF_GRAYED);

	SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_EDIT,
				 MAKELONG(FALSE, 0));
	SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_CONN,
				 MAKELONG(FALSE, 0));
	SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_DELETE,
				 MAKELONG(FALSE, 0));
	SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_FILE_DELETE,
				 MAKELONG(FALSE, 0));
	SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_FILE_PROPERTIES,
				 MAKELONG(FALSE, 0));

	if (cb == 1) {
		if (lvi.iItem == 0) {
			// Make new connection

			// Set File->Connect/Create menu to create and enable
			MenuItemInfo.fMask = MIIM_TYPE;                        
			MenuItemInfo.cch = LoadString(v_hInst, IDS_CREATE,
										  MenuItemInfo.dwTypeData,
										  sizeof(szTemp)/sizeof(TCHAR));
			SetMenuItemInfo(hMenu, ID_FILE_CONNECTCREATE, FALSE,
							&MenuItemInfo);
			EnableMenuItem (hMenu, ID_FILE_CONNECTCREATE,
							MF_ENABLED);
		} else {
			EnableMenuItem (hMenu, ID_FILE_CONNECTCREATE,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_CREATESHORTCUT,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_DELETE,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_RENAME,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_PROPERTIES,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_EDIT_CREATECOPY,
							MF_ENABLED);

			EnableMenuItem (hMenu, ID_CONN_CONN, MF_ENABLED);
			EnableMenuItem (hMenu, ID_CONN_EDIT, MF_ENABLED);
			EnableMenuItem (hMenu, ID_CONN_DELETE, MF_ENABLED);
			EnableMenuItem (hMenu, ID_CONN_RENAME, MF_ENABLED);

			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_EDIT,
						 MAKELONG(TRUE, 0));
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_CONN,
						 MAKELONG(TRUE, 0));
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_DELETE,
						 MAKELONG(TRUE, 0));

			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_FILE_DELETE,
						 MAKELONG(TRUE, 0));
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON,
						 ID_FILE_PROPERTIES,
						 MAKELONG(TRUE, 0));
		}
	} else {
		if (cb != 0) {
			// allow shortcuts for multiple items
			if (lvi.iItem > 0) {
				EnableMenuItem (hMenu, ID_EDIT_CREATECOPY,
								MF_ENABLED);
			}
			EnableMenuItem (hMenu, ID_FILE_CONNECTCREATE,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_CREATESHORTCUT,
							MF_ENABLED);
			EnableMenuItem (hMenu, ID_FILE_DELETE,
							MF_ENABLED);
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_CONN,
						 MAKELONG(TRUE, 0));
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_CONN_DELETE,
						 MAKELONG(TRUE, 0));
			SendMessage (v_hCmdBar, TB_ENABLEBUTTON, ID_FILE_DELETE,
						 MAKELONG(TRUE, 0));

			EnableMenuItem (hMenu, ID_CONN_DELETE, MF_ENABLED);
		}
	}
}

// ----------------------------------------------------------------
//
// NotifyHandler ()
//
// Handle Notification events for the ListView
//
// ----------------------------------------------------------------

LRESULT
NotifyHandler (HWND hWnd, NM_LISTVIEW *pnm, LV_DISPINFO *plvdi)
{
	LV_ITEM		    lvi;
	PITEMINFO		pItemInfo;
	HWND			hwndEdit;
	DWORD		    dwTemp;
    int             nFormatId;
	TCHAR		    szTemp[128];
	TCHAR			szFmtStr[128];

	DEBUGMSG (ZONE_MISC, (TEXT("Got WM_NOTIFY hdr.code=%d\r\n"),
						  pnm->hdr.code));

	switch (pnm->hdr.code) {
	case LVN_DELETEALLITEMS :
		DEBUGMSG (ZONE_MISC, (TEXT("Got LVN_DELETEALLITEMS\r\n")));
		return FALSE;
	case LVN_DELETEITEM :
		if (0 != pnm->iItem) {
			DEBUGMSG (ZONE_MISC, (TEXT("Got LVN_DELETEITEM %d\r\n"), pnm->iItem));
			LocalFree ((HLOCAL)pnm->lParam);
		}
		break;
	case LVN_GETDISPINFO :
		DEBUGMSG (ZONE_MISC, (TEXT("LVN_GETDISPINFO\r\n")));
		if (!(plvdi->item.mask & LVIF_TEXT)) {
			DEBUGMSG (ZONE_MISC, (TEXT("LVN_GETDISPINFO: Not Text? Mask=0x%X\r\n"),
								  plvdi->item.mask));
			break;
		}
		SetMenu();

		if (0 == plvdi->item.iItem) {
			if (0 == plvdi->item.iSubItem) {
				LoadString(v_hInst, IDS_MAKE_NEW,
						   plvdi->item.pszText,
						   plvdi->item.cchTextMax);
			}
			break;
		}
		pItemInfo = (PITEMINFO)plvdi->item.lParam;
		switch (plvdi->item.iSubItem) {
		case 0 :
			_tcsncpy (plvdi->item.pszText,
					  pItemInfo->EntryName,
					  plvdi->item.cchTextMax);
			break;
		case 1 :
			_tcsncpy (plvdi->item.pszText,
					  pItemInfo->szPhone,
					  plvdi->item.cchTextMax);
			break;
		case 2 :
			_tcsncpy (plvdi->item.pszText,
					  ((PITEMINFO)plvdi->item.lParam)->Entry.szDeviceName,
					  plvdi->item.cchTextMax);
			break;
		}
		DEBUGMSG (ZONE_MISC, (TEXT("LVN_GETDISPINFO: Returning string '%s'\r\n"),
							  plvdi->item.pszText));
		break;
	case LVN_COLUMNCLICK:
		
		break;

	case LVN_BEGINLABELEDIT:
		DEBUGMSG (ZONE_MISC, (TEXT("BeginEdit\r\n")));

		if (plvdi->item.iItem == 0) {
			DEBUGMSG (ZONE_MISC, (TEXT("Can't edit the 'Make New' icon\r\n")));
			return TRUE; // Can't edit the Make New icon
		}
		v_fInRename = TRUE;
		PositionSIP(SIP_UP);
		ListView_EnsureVisible(v_hListWnd, plvdi->item.iItem, FALSE);
		if (hwndEdit = ListView_GetEditControl(v_hListWnd)) {
			SendMessage(hwndEdit, EM_LIMITTEXT,
						RAS_MaxEntryName, 0);
		}
		return FALSE; // Return FALSE to let the user edit

	case LVN_ENDLABELEDIT:
		v_fInRename = FALSE;
		PositionSIP(SIP_DOWN);
		if (plvdi->item.pszText == NULL) {
			DEBUGMSG (ZONE_MISC|ZONE_ERROR,
					  (TEXT("ENDLABELEIDT: Text is NULL\r\n")));
			break;
		}

		// if it's the same as before then it's a no-op.
		if (!_tcsicmp (plvdi->item.pszText, ((PITEMINFO)plvdi->item.lParam)->EntryName)) {
		}

		if (dwTemp = RasValidateEntryName(NULL, plvdi->item.pszText)) {
			if (dwTemp == ERROR_ALREADY_EXISTS) {
				nFormatId = IDS_ALREADY_EXISTS;
			} else if (*(plvdi->item.pszText)) {
				nFormatId = IDS_BADNAME;
			} else {
				nFormatId = IDS_NULLNAME;
			}
			LoadString(v_hInst, nFormatId, szFmtStr, sizeof(szFmtStr)/sizeof(TCHAR));
			LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET,
					   szTemp, sizeof(szTemp)/sizeof(TCHAR));
			MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
			PostMessage (v_hListWnd, LVM_EDITLABEL,
						 (WPARAM)(int)plvdi->item.iItem, 0);
			break;
		}

		RasRenameEntry(NULL, ((PITEMINFO)plvdi->item.lParam)->EntryName,
					   plvdi->item.pszText);
		dwTemp = plvdi->item.iItem;
		InitListViewItems(v_hListWnd);
		ListView_SetItemState (v_hListWnd, 0, 0, LVIS_SELECTED);
		ListView_SetItemState (v_hListWnd, dwTemp,
							   LVIS_SELECTED | LVIS_FOCUSED,
							   LVIS_SELECTED | LVIS_FOCUSED);
		SetFocus (v_hListWnd);
		break;
	case NM_RCLICK:
		PostMessage(v_hMainWnd, WM_COMMAND, ID_SHORTCUT, 0);
		break;
	case NM_CLICK:
		if (GetKeyState(VK_MENU) < 0) {
			PostMessage(v_hMainWnd, WM_COMMAND, ID_SHORTCUT, 0);
		}

		break;
	case NM_DBLCLK:
		if (GetKeyState(VK_MENU) < 0) {
			PostMessage(v_hMainWnd, WM_COMMAND, ID_SHORTCUT, 0);
			break;
		}

		lvi.mask  = LVIF_PARAM;
		lvi.iSubItem = 0;
		lvi.iItem = pnm->iItem;


		if (lvi.iItem == 0) {
			// Allow them to make a new item.
			PostMessage(v_hMainWnd, WM_COMMAND, 
						ID_CONNECTIONS_MAKENEWCONNECTION, 0);
		} else if (lvi.iItem != -1) {
			ListView_SetItemState (v_hListWnd, lvi.iItem,
								   LVIS_SELECTED, LVIS_SELECTED);

			// They want to connect.
			SendMessage(v_hMainWnd, WM_COMMAND, 
						ID_FILE_CONNECTCREATE, 0);
		}
		break;

	case LVN_KEYDOWN:
		switch (((LV_KEYDOWN *) plvdi)->wVKey) {
		case VK_RETURN :
			lvi.iItem = ListView_GetNextItem(v_hListWnd, -1,
											 LVNI_SELECTED);
			if (lvi.iItem == 0) {
				PostMessage(v_hMainWnd, WM_COMMAND, 
							ID_CONNECTIONS_MAKENEWCONNECTION, 0);
			} else if (lvi.iItem != -1) {
				ListView_SetItemState (v_hListWnd, lvi.iItem,
									   LVIS_SELECTED, LVIS_SELECTED);

				// They want to connect.
				SendMessage(v_hMainWnd, WM_COMMAND, 
							ID_FILE_CONNECTCREATE, 0);
			}
			break;
		case VK_CONTROL :
			if (GetAsyncKeyState(VK_MENU)) {
				PostMessage(v_hMainWnd, WM_COMMAND,
							ID_SHORTCUT, 0);
			}
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

// ----------------------------------------------------------------
//
// WndProc
//
// Main window Proc
//
// ----------------------------------------------------------------
LRESULT CALLBACK
WndProc (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
	NM_LISTVIEW	    *pnm;
    LV_DISPINFO     *plvdi;
	LV_ITEM		    lvi;
	TCHAR		    szTemp[128];
	TCHAR			szFmtStr[128];
	DWORD		    dwTemp;
	DWORD		    cb;
	RECT		    rc;
	HMENU		    hMenu, hMenuTrack;
	POINT		    pt;
    int             nFormatId;
    TCHAR		    TmpEntryName[2*RAS_MaxEntryName];
	DWORD			dwSize, dwDevSize;
	PBYTE			pDevConfig;
	BOOL			bRefresh;
	int				iCopyNum;
	DWORD			i;

    switch (message) {
	case WM_HELP:
		RETAILMSG (1, (TEXT("PegHelp file:rnetw.htm#Main_Contents\r\n")));
		CreateProcess(TEXT("peghelp"),TEXT("file:rnetw.htm#Main_Contents"),
					  NULL,NULL,FALSE,0,NULL,NULL,NULL,NULL);
		break;

	case WM_SIZE :
		GetClientRect (hWnd, &rc);
		rc.top += CallCommCtrlFunc(CommandBar_Height)(v_hCmdBar);
		MoveWindow (v_hListWnd, rc.left, rc.top, rc.right-rc.left,
					rc.bottom-rc.top, TRUE); 
		break;
          
	case WM_NOTIFY :
		pnm = (NM_LISTVIEW *)lParam;
		plvdi  = (LV_DISPINFO *)lParam;

		if (pnm->hdr.idFrom != IDD_LISTVIEW) {
			break;
		}
		return NotifyHandler (hWnd, pnm, plvdi);
		
		break;
	case WM_COMMAND :
        
		switch (LOWORD(wParam)) {
		case ID_HELP :
			SendMessage (v_hMainWnd, WM_HELP, 0, 0);
			break;
		case ID_CONN_CONN :
		case ID_FILE_CONNECTCREATE :
			// Start RNAApp for each item selected
			for (lvi.iItem = -1; (-1 != (lvi.iItem =
				ListView_GetNextItem(v_hListWnd, lvi.iItem,
									 LVNI_SELECTED)));) {

				// First item is special, Invoke the Make New wizard
				if (0 == lvi.iItem) {
					PostMessage(v_hMainWnd, WM_COMMAND, 
								ID_CONNECTIONS_MAKENEWCONNECTION, 0);
					continue;
				}
				
				lvi.iSubItem = 0;
				lvi.mask  = LVIF_PARAM;
				ListView_GetItem(v_hListWnd, &lvi);
				
				// Actually start the process
				wsprintf (szTemp, TEXT("-e\"%s\""),
						  ((PITEMINFO)lvi.lParam)->EntryName);
				DEBUGMSG (ZONE_MISC,
						  (TEXT("Starting RNAAPP '%s'\r\n"), szTemp));
				if (FALSE == CreateProcess (TEXT("rnaapp"), szTemp, NULL, NULL,
											FALSE, 0, NULL, NULL, NULL, NULL)) {
					LoadString (v_hInst, IDS_CREATE_PROC_ERR, szFmtStr,
								sizeof(szFmtStr)/sizeof(TCHAR));
					LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET,
							   szTemp, sizeof(szTemp)/sizeof(TCHAR));
					MessageBox (hWnd, szFmtStr, szTemp, MB_APPLMODAL |
								MB_OK | MB_ICONEXCLAMATION);
				}
			}
			break;

		case ID_FILE_CREATESHORTCUT :
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			if ((cb = ListView_GetSelectedCount (v_hListWnd)) == 0)
				break;

			for (lvi.iItem = -1; (-1 != (lvi.iItem =
				ListView_GetNextItem(v_hListWnd, lvi.iItem,
									 LVNI_SELECTED)));) {

				if (0 == lvi.iItem) {
					LoadString(v_hInst, IDS_NOSHORTCUT, szFmtStr,
							   sizeof(szFmtStr)/sizeof(TCHAR));
					LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET,
							   szTemp, sizeof(szTemp)/sizeof(TCHAR));
					MessageBox (hWnd, szFmtStr, szTemp, MB_APPLMODAL |
								MB_OK | MB_ICONEXCLAMATION);
					continue;
				}
                        
				lvi.iSubItem = 0;
				lvi.mask  = LVIF_PARAM;
				ListView_GetItem(v_hListWnd, &lvi);                    

				if (!CreateShortcut(((PITEMINFO)lvi.lParam)->EntryName)) {
					LoadString(v_hInst, IDS_SHORTCUTFAILED, szFmtStr,
							   sizeof(szFmtStr)/sizeof(TCHAR));
					LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET,
							   szTemp, sizeof(szTemp)/sizeof(TCHAR));
					MessageBox (hWnd, szFmtStr, szTemp, MB_APPLMODAL |
								MB_OK | MB_ICONEXCLAMATION);
				}
			}
			break;
                    
		case ID_CONN_DELETE :
		case ID_FILE_DELETE :
			dwTemp = ListView_GetSelectedCount (v_hListWnd);
			if (dwTemp == 0) {
				break;
			}

			lvi.iItem = ListView_GetNextItem(v_hListWnd, -1,
											 LVNI_SELECTED);

			if (dwTemp == 1 && (lvi.iItem == 0)) {
				break;
			}
                            
			if (dwTemp == 1) {
                        LoadString(v_hInst, IDS_DELCON1, szFmtStr,
								   sizeof(szFmtStr)/sizeof(TCHAR));
			} else {
				LoadString(v_hInst, IDS_DELCON2, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
			}
                    
			LoadString(v_hInst, IDS_CONFDEL, szTemp,
					   sizeof(szTemp)/sizeof(TCHAR));
                    
			if (IDYES != MessageBox (hWnd, szFmtStr, szTemp,
									MB_DEFBUTTON2 | MB_YESNO | MB_ICONWARNING)) {
				break;
			}

			do {
				// Can't delete to the MakeNew Item
				if (0 == lvi.iItem)  {
					continue;
				}

				lvi.iSubItem = 0;
				lvi.mask  = LVIF_PARAM;
				ListView_GetItem(v_hListWnd, &lvi);
				RasDeleteEntry (NULL, ((PITEMINFO)lvi.lParam)->EntryName);
			} while ((lvi.iItem = ListView_GetNextItem(v_hListWnd,
				lvi.iItem, LVNI_SELECTED)) != -1);
                    

			// Refresh the list?
			InitListViewItems(v_hListWnd);
			SetFocus (v_hListWnd);
			break;
		case ID_FILE_RENAME :
		case ID_CONN_RENAME :
			DEBUGMSG (ZONE_MISC, (TEXT("ID_????_RENAME\r\n")));
			
			if (1 != ListView_GetSelectedCount (v_hListWnd)) {
				LoadString(v_hInst, IDS_TOOMANYENT, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
				LoadString(v_hInst, IDS_ERROR, szTemp,
						   sizeof(szTemp)/sizeof(TCHAR));
				MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
				break;
			}
			lvi.iItem = ListView_GetNextItem(v_hListWnd, -1,
											 LVNI_SELECTED);
                    
			if (lvi.iItem == 0) {
				LoadString(v_hInst, IDS_RENAMENEW, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
				LoadString(v_hInst, IDS_ERROR, szTemp,
						   sizeof(szTemp)/sizeof(TCHAR));
				MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
				break;
			}
			// Does it need focus?
			SetFocus (v_hListWnd);
			ListView_EditLabel(v_hListWnd, lvi.iItem);
			break;
                    
		case ID_CONN_EDIT :
		case ID_FILE_PROPERTIES:
			if (v_hDialogWnd) {
				SetForegroundWindow(v_hDialogWnd);
				break;
			}			
			
			// Which item is selected?
			if ((cb = ListView_GetSelectedCount(v_hListWnd)) == 0) {
				break;
			}
			if (1 != cb) {
				DEBUGMSG (ZONE_MISC,
						  (TEXT("ID_FILE_PROPERTIES %d items selected\r\n"),
						   ListView_GetSelectedCount(v_hListWnd)));

				LoadString(v_hInst, IDS_TOOMANYENT, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
				LoadString(v_hInst, IDS_ERROR, szTemp,
						   sizeof(szTemp)/sizeof(TCHAR));
				MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
				break;
			}
			// Get the selected item
			lvi.iItem = ListView_GetNextItem(v_hListWnd, -1, LVNI_SELECTED);

			// Can't get properites for the MakeNew Item
			if (0 == lvi.iItem) {
				LoadString(v_hInst, IDS_PLEASE, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
				LoadString(v_hInst, IDS_ERROR, szTemp,
						   sizeof(szTemp)/sizeof(TCHAR));
				MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
				break;
			}

			lvi.iSubItem = 0;
			lvi.mask  = LVIF_PARAM;
			ListView_GetItem(v_hListWnd, &lvi);

			memcpy(&EditItem, (PITEMINFO)lvi.lParam, sizeof(ITEMINFO));

			// Direct or Dial-up
			v_WizDialog = _tcscmp(EditItem.Entry.szDeviceType,
								  RASDT_Direct) ? v_DialogPages[DLG_PG_2] :
						  		  v_DialogPages[DLG_PG_4];

			v_EnteredAsAWizard = FALSE;
			v_hDialogWnd = CreateDialog (v_hInst,
										 MAKEINTRESOURCE(v_WizDialog), hWnd,
										 ConnWizDlgProc);

			if (NULL == v_hDialogWnd) {
				ERRORMSG (1, (TEXT("Unable to CreateDialog\r\n")));
				SetFocus (v_hListWnd);
			}
			InitListViewItems(v_hListWnd);
			break;
			
                    
		case ID_FILE_EXIT :
		case ID_CONN_EXIT :
			DestroyWindow(hWnd);
			break;
		case ID_EDIT_CREATECOPY:
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			bRefresh = FALSE;
			lvi.iItem = -1;
			for(;;){
				lvi.iItem = ListView_GetNextItem(v_hListWnd, lvi.iItem, LVNI_SELECTED);

				if (lvi.iItem <= 0)
					break;

				lvi.iSubItem = 0;
				lvi.mask = LVIF_PARAM;
				ListView_GetItem(v_hListWnd, &lvi);

				memcpy(&EditItem, (PITEMINFO)lvi.lParam, sizeof(ITEMINFO));

				LoadString(v_hInst, IDS_COPYOF, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));

				wsprintf(TmpEntryName, szFmtStr, EditItem.EntryName);
				// Truncate it
				if(RAS_MaxEntryName <= _tcslen(TmpEntryName)){
					RETAILMSG (1, (TEXT("Name '%s' was too long, truncating\r\n"), TmpEntryName));
					TmpEntryName[RAS_MaxEntryName] = TEXT('\0');
				}

				if (dwTemp = RasValidateEntryName(NULL, TmpEntryName)) {
					if (dwTemp == ERROR_ALREADY_EXISTS) {
						RETAILMSG (1, (TEXT("Duplicate name found, trying again\r\n")));
						iCopyNum = 2;
						LoadString(v_hInst, IDS_COPY_NUM_OF, szFmtStr,
								   sizeof(szFmtStr)/sizeof(TCHAR));
						for(;;){
							if (iCopyNum > 100) {
								// Terminate loop at some point
								break;
							}
							

							if(_tcsstr(szFmtStr, TEXT("%d")) < _tcsstr(szFmtStr, TEXT("%s"))){
								wsprintf(TmpEntryName, szFmtStr, iCopyNum, EditItem.EntryName);
							} else {
								wsprintf(TmpEntryName, szFmtStr, EditItem.EntryName, iCopyNum);
							}

							// Truncate it
							TmpEntryName[RAS_MaxEntryName] = TEXT('\0');
							if (dwTemp = RasValidateEntryName(NULL, TmpEntryName)) {
								if (dwTemp == ERROR_ALREADY_EXISTS) {
									iCopyNum++;
									continue;
								} else {
									break;
								}
							} else {
								break;
							}
						}
					}
				}

				RETAILMSG (1, (TEXT("Final name is '%s'\r\n"), TmpEntryName));
				if (dwTemp = RasValidateEntryName(NULL, TmpEntryName)) {
					if (dwTemp == ERROR_ALREADY_EXISTS) {
	                    nFormatId = IDS_ALREADY_EXISTS;
					} else if (*TmpEntryName) {
	                    nFormatId = IDS_BADNAME;
	                } else {
	                    nFormatId = IDS_NULLNAME;
	                }
					LoadString(v_hInst, nFormatId, szFmtStr, sizeof(szFmtStr)/sizeof(TCHAR));
					LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET,
							   szTemp, sizeof(szTemp)/sizeof(TCHAR));
					MessageBox (hWnd, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
				} else {
					pDevConfig = NULL;
					
					dwSize = sizeof(EditItem.Entry);
					dwDevSize = 0;
					dwTemp = RasGetEntryProperties (NULL, EditItem.EntryName, &(EditItem.Entry),
										   &dwSize, NULL,
										   &dwDevSize);

					if (dwDevSize && (ERROR_BUFFER_TOO_SMALL == dwTemp)) {
						pDevConfig = (PBYTE)LocalAlloc (LPTR, dwDevSize);
						if (pDevConfig) {
							dwTemp = RasGetEntryProperties (NULL, EditItem.EntryName,
								&(EditItem.Entry), &dwSize, pDevConfig,
								&dwDevSize);
							DEBUGMSG (dwTemp, (TEXT("Error %d from RasGetEntryProperties\r\n")));
							ASSERT (dwTemp == 0);
						}
					}

					_tcscpy(EditItem.EntryName, TmpEntryName);
					RasSetEntryProperties (NULL, EditItem.EntryName,
										   &(EditItem.Entry),
										   sizeof(EditItem.Entry),
										   pDevConfig, dwDevSize);
					if (pDevConfig) {
						LocalFree (pDevConfig);
					}
					bRefresh = TRUE;
				}
			}
			// Refresh the list?
			if(bRefresh){
				InitListViewItems(v_hListWnd);
			}
			break;
                    
		case ID_EDIT_SELECTALL:
		    dwTemp = ListView_GetItemCount (v_hListWnd);

		    // Get the selected item
		    ListView_SetItemState(v_hListWnd, 0, 0, LVIS_SELECTED);

		    for (i=1; i<dwTemp; i++)  {
		        ListView_SetItemState(v_hListWnd, i, LVIS_SELECTED, LVIS_SELECTED);
		    }
			break;
		case ID_VIEW_LARGEICON :
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			UpdateView (ID_VIEW_LARGEICON);

			SetWindowLong (v_hListWnd, GWL_STYLE,
						   (GetWindowLong (v_hListWnd, GWL_STYLE) &
							~LVS_TYPEMASK) | LVS_ICON);
			break;
		case ID_VIEW_SMALLICON :
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			UpdateView (ID_VIEW_SMALLICON);

			SetWindowLong (v_hListWnd, GWL_STYLE,
						   (GetWindowLong (v_hListWnd, GWL_STYLE) &
							~LVS_TYPEMASK) | LVS_SMALLICON);
			break;

		case ID_VIEW_DETAILS :
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			UpdateView (ID_VIEW_DETAILS);

			SetWindowLong (v_hListWnd, GWL_STYLE,
						   (GetWindowLong (v_hListWnd, GWL_STYLE) &
							~LVS_TYPEMASK) | LVS_REPORT);
			break;
		case ID_VIEW_REFRESH:
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			InitListViewItems(v_hListWnd);
			break;
		case ID_HELP_ABOUT :
			if (v_fPortrait) {
				RETAILMSG (1, (TEXT("Command %d for landscape mode only\r\n"),
							   LOWORD(wParam)));
				break;
			}

			LoadString(v_hInst, IDS_COPYRIGHT, szFmtStr,
					   sizeof(szFmtStr)/sizeof(TCHAR));
			LoadString(v_hInst, IDS_HELPABOUT, szTemp,
					   sizeof(szTemp)/sizeof(TCHAR));
			MessageBox (hWnd, szFmtStr, szTemp, MB_OK);
			break;
		case ID_CONN_NEW :
		case ID_CONNECTIONS_MAKENEWCONNECTION :
			if (v_hDialogWnd) {
				SetForegroundWindow(v_hDialogWnd);
				break;
			}
			
			// Initialize the EntryInfo
			memset ((char *)&EditItem, 0, sizeof(EditItem));

			// Loop over calls to RasValidateEntryName until
			// we get a unique name.
			cb = 1;
			do {
				LoadString(v_hInst, IDS_MYCONN1, szFmtStr,
						   sizeof(szFmtStr)/sizeof(TCHAR));
				LoadString(v_hInst, IDS_MYCONN2, szTemp,
						   sizeof(szTemp)/sizeof(TCHAR));
				if (1 == cb) {
					_tcscpy (EditItem.EntryName, szFmtStr);
				} else {
					wsprintf (EditItem.EntryName, szTemp, cb);
				}
				cb++;
			} while (RasValidateEntryName(NULL, EditItem.EntryName));


			EditItem.Entry.dwSize = sizeof(RASENTRY);

			// Get a defailt RasEntry
			cb = sizeof(RASENTRY);
			RasGetEntryProperties (NULL, TEXT(""),
								   &(EditItem.Entry),
								   &cb,
								   NULL, NULL);

			v_PrevWizDialog = 0;
			v_WizDialog = v_DialogPages[DLG_PG_1];
			v_EnteredAsAWizard = TRUE;
			v_hDialogWnd = CreateDialog (v_hInst,
										 MAKEINTRESOURCE(v_WizDialog), hWnd,
										 ConnWizDlgProc);

            // The dialog is entered, bring up the SIP, if we can
            if (v_hDialogWnd)
            {
                // Bring the SIP up
                PositionSIP(SIP_UP);
            }
			break;

		case ID_SHORTCUT :
			// Get first item selected.
			lvi.iItem = ListView_GetNextItem(v_hListWnd, -1,
											 LVNI_SELECTED | LVIS_FOCUSED);
			// Get number selected
			cb = ListView_GetSelectedCount (v_hListWnd);

			if (lvi.iItem == -1) {
				// BackGround menu
				break;
			} else {
				// Get location of item
				ListView_GetItemRect (v_hListWnd, lvi.iItem, &rc, LVIR_BOUNDS);
				hMenu = LoadMenu (v_hInst,
								  MAKEINTRESOURCE(IDR_CONTEXT_MENU));

				// Is it the Make New icon?
				if (lvi.iItem == 0) {
					hMenuTrack = GetSubMenu(hMenu, 0);
				} else {
					hMenuTrack = GetSubMenu(hMenu, 1);
					EnableMenuItem (hMenuTrack, ID_FILE_CONNECTCREATE,
									(cb >= 1) ? MF_ENABLED : MF_GRAYED);
					EnableMenuItem (hMenuTrack, ID_FILE_RENAME,
									(cb == 1) ? MF_ENABLED : MF_GRAYED);
					EnableMenuItem (hMenuTrack, ID_FILE_PROPERTIES,
									(cb == 1) ? MF_ENABLED : MF_GRAYED);
				}
			}

			dwTemp = GetMessagePos();
			pt.x = LOWORD(dwTemp);
			pt.y = HIWORD(dwTemp);

			TrackPopupMenu(hMenuTrack, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
			DestroyMenu(hMenu);

			break;
		default :
			DEBUGMSG (ZONE_MISC, (TEXT("Got WM_COMMAND=%d\r\n"), LOWORD(wParam)));
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
		break;

    case WM_WININICHANGE:
#ifdef USE_SIP    
        if (g_pSipGetInfo && (wParam ==  SPI_SETSIPINFO))
        {
            // SIP most likely either was raised or lowered
            SIPINFO si;

            memset(&si, 0, sizeof(SIPINFO));
    		si.cbSize = sizeof(si);
    		if ((*g_pSipGetInfo)(&si) )
    		{
                // Resize the main window and the listview
                SetWindowPos(hWnd, NULL, si.rcVisibleDesktop.left, si.rcVisibleDesktop.top,
                                si.rcVisibleDesktop.right - si.rcVisibleDesktop.left,
                                si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top,
                                SWP_NOACTIVATE | SWP_NOZORDER);

				// And the listview--must convert to client coordinates here
                si.rcVisibleDesktop.top += CallCommCtrlFunc(CommandBar_Height)(v_hCmdBar);
                MapWindowRect(NULL, GetParent(v_hListWnd), &si.rcVisibleDesktop);
                SetWindowPos(v_hListWnd, NULL, si.rcVisibleDesktop.left, si.rcVisibleDesktop.top,
                                si.rcVisibleDesktop.right - si.rcVisibleDesktop.left,
                                si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top,
                                SWP_NOACTIVATE | SWP_NOZORDER);
     		}
        }
#endif     
        break;

	case WM_DESTROY:
		// Clean up the image list
		ImageList_Destroy (ListView_GetImageList(v_hListWnd, LVSIL_NORMAL));
		ImageList_Destroy (ListView_GetImageList(v_hListWnd, LVSIL_SMALL));
		PostQuitMessage(0);
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE) {
			SetFocus (v_hListWnd);
		}
		break;

	default:
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}

return (0);
}


BOOL
InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

    // Register IP class window
    RegisterIPClass(hInstance);
    
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = szAppName;
    return(RegisterClass(&wc));
}

// InitListViewImageList - creates image lists for a list view.
// Returns TRUE if successful or FALSE otherwise.
// hwndLV - handle of the list view control
BOOL WINAPI
InitListViewImageLists(HWND hwndLV)
{
	HICON hiconItem;        // icon for list view items
	HIMAGELIST himlLarge;   // image list for icon view
	HIMAGELIST himlSmall;   // image list for other views

	// Create the full-sized and small icon image lists.
	himlLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
								 GetSystemMetrics(SM_CYICON), TRUE, 1, 1);
	himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								 GetSystemMetrics(SM_CYSMICON), TRUE, 1, 1);

	// Add an icon to each image list.
	hiconItem = LoadIcon(v_hInst, MAKEINTRESOURCE(IDI_NEWCONN));
	ImageList_AddIcon(himlLarge, hiconItem);
    hiconItem = (HICON)LoadImage(v_hInst, MAKEINTRESOURCE(IDI_NEWCONN), 
                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	ImageList_AddIcon(himlSmall, hiconItem);
	DeleteObject(hiconItem);
	hiconItem = LoadIcon(v_hInst, MAKEINTRESOURCE((v_fPortrait ? IDI_RNA2 : IDI_RNA)));
	ImageList_AddIcon(himlLarge, hiconItem);
    hiconItem = (HICON)LoadImage(v_hInst, MAKEINTRESOURCE((v_fPortrait ? IDI_RNA2 : IDI_RNA)), 
                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);    
	ImageList_AddIcon(himlSmall, hiconItem);
	DeleteObject(hiconItem);
	hiconItem = LoadIcon(v_hInst, MAKEINTRESOURCE(IDI_DIRECTCC));
	ImageList_AddIcon(himlLarge, hiconItem);
    hiconItem = (HICON)LoadImage(v_hInst, MAKEINTRESOURCE(IDI_DIRECTCC), 
                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);    
	ImageList_AddIcon(himlSmall, hiconItem);
	DeleteObject(hiconItem);

	// Assign the image lists to the list view control.
	ListView_SetImageList(hwndLV, himlLarge, LVSIL_NORMAL);
	ListView_SetImageList(hwndLV, himlSmall, LVSIL_SMALL);
	return TRUE;
}

// InitListViewColumns - adds columns to a list view control.
// Returns TRUE if successful or FALSE otherwise.
// hwndLV - handle of the list view control
BOOL WINAPI
InitListViewColumns(HWND hwndLV)
{
	TCHAR szTemp[256];     // temporary buffer
	LV_COLUMN lvc;
	int iCol;

	// Initialize the LV_COLUMN structure.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 100;
	lvc.pszText = szTemp;

	// Add the columns.
	for (iCol = 0; iCol < NUM_LV_COLUMNS; iCol++) {
		lvc.iSubItem = iCol;
        switch (iCol)
        {
            case 0 :
                lvc.cx = 150;
                break;
            case 2 :
                lvc.cx = 225;
                break;
            default:
                lvc.cx = 100;
        }
            
		LoadString(v_hInst, IDS_ENTRYCOL + iCol, szTemp,
				   sizeof(szTemp));
		if (ListView_InsertColumn(hwndLV, iCol, &lvc) == -1)
			return FALSE;
	}
	return TRUE;
}

// InitListViewItems - adds items and subitems to a list view.
// Returns TRUE if successful or FALSE otherwise.
// hwndLV - handle of the list view control
// pfData - text file containing list view items with columns
//          separated by semicolons
BOOL WINAPI
InitListViewItems(HWND hwndLV)
{
	int			iItem;			// Item we're working on
	LV_ITEM		lvi;			// Current Item structure
	DWORD		cb;				// Number of bytes in RasEntryName list.
	LPRASENTRYNAME lpRasEntries;	// Pointer to the RasEntries.
	DWORD		cEntries;		// Number of Entries found
	DWORD		Index;			
	PITEMINFO	pItemInfo;

	ListView_DeleteAllItems (hwndLV);
	
	// Initialize LV_ITEM members that are common to all items.
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;   // app. maintains text
	lvi.iImage = 0;                     // image list index

	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.lParam = (LPARAM) 0;    // item data
	
	// Add the item.
	ListView_InsertItem(hwndLV, &lvi);
		
	lvi.state = 0;		// The rest of the items are not selected.

	// Now get the current RasEntryNames
	RasEnumEntries (NULL, NULL, NULL, &cb, NULL);

	if (cb) {
		// Now we have to do something with the list.
		lpRasEntries = (LPRASENTRYNAME)LocalAlloc (LPTR, cb);
		if (!RasEnumEntries (NULL, NULL, lpRasEntries, &cb, &cEntries)) {
			// Walk the list
			iItem = 1;
			for ( Index = 0; Index < cEntries; Index++) {
				if (lpRasEntries[Index].szEntryName[0] == TEXT('`')) {
					DEBUGMSG (1, (TEXT("Skipping entry \"%s\"\r\n"),
								   lpRasEntries[Index].szEntryName));
					continue;
				}

				pItemInfo = (PITEMINFO)LocalAlloc (LPTR, sizeof(ITEMINFO));
				memset ((char *)pItemInfo, 0, sizeof(ITEMINFO));

				_tcscpy (pItemInfo->EntryName,
						 lpRasEntries[Index].szEntryName);

				// Get the Entry properties.
				cb = sizeof(pItemInfo->Entry);
				pItemInfo->Entry.dwSize = sizeof(RASENTRY);
				if (RasGetEntryProperties (NULL, pItemInfo->EntryName,
										   &(pItemInfo->Entry),
										   &cb,
										   NULL, NULL)) {
					
					LocalFree (pItemInfo);
					continue;
				}

				// This should use lineTranslateAddress to get a pretty version
				// But should handle VPN and Direct entries nicely.
				_tcsncpy (pItemInfo->szPhone, 
						  pItemInfo->Entry.szLocalPhoneNumber,
						  sizeof(pItemInfo->szPhone)/sizeof(TCHAR));
				
				// Initialize item-specific LV_ITEM members.
				lvi.iItem = iItem++;
				lvi.iSubItem = 0;
				lvi.lParam = (LPARAM) pItemInfo;    // save item data
				lvi.iImage = _tcscmp (pItemInfo->Entry.szDeviceType,
									  RASDT_Direct) ? 1 : 2;
		
				// Add the item.
				ListView_InsertItem(hwndLV, &lvi);
				lvi.state = 0;		// The rest of the items are not selected.
			}
		}
		// Free the RasEntry info
		LocalFree ((HLOCAL)lpRasEntries);
	}

	return TRUE;
}


												

BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	RECT		rc;
    HICON       hIcon;
	HMENU	hMenu = NULL;
	int			rv, nWidth, nHeight, x, y;

    v_hInst = hInstance;

    // OK, create the window, taking the position of the SIP into account
	nWidth = CW_USEDEFAULT;
	nHeight = CW_USEDEFAULT;
	x = y = CW_USEDEFAULT;
#ifdef USE_SIP
    if (g_pSipGetInfo)
    {
        SIPINFO si;

        memset(&si, 0, sizeof(SIPINFO));
    	si.cbSize = sizeof(si);
    	if((*g_pSipGetInfo)(&si) )
    	{
    	    nWidth = si.rcVisibleDesktop.right - si.rcVisibleDesktop.left;
            nHeight = si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top;
            x = si.rcVisibleDesktop.left;
            y = si.rcVisibleDesktop.top;
     	}
    }
#endif

    v_hMainWnd = CreateWindowEx(0, szAppName, szTitle,
								WS_CLIPCHILDREN, x, y,
								nWidth, nHeight,
								NULL, hMenu, hInstance, NULL);
	
    if ( v_hMainWnd == 0 ) {
        return (FALSE);
    }
    // Set remnets icon for the task bar
    hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_REMOTENW),
							 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	SendMessage(v_hMainWnd, WM_SETICON, FALSE, (WPARAM)hIcon);    

	DEBUGMSG (ZONE_MISC, (TEXT("After CreateWindowEx\r\n")));
	GetClientRect (v_hMainWnd, &rc);

	v_hCmdBar = CallCommCtrlFunc(CommandBar_Create) (hInstance, v_hMainWnd, 1);
	if (v_fPortrait) {
		CallCommCtrlFunc(CommandBar_InsertMenubar)(v_hCmdBar, hInstance,
			IDR_PORTRAIT_MENU, 0);
	} else {
		CallCommCtrlFunc(CommandBar_InsertMenubar)(v_hCmdBar, hInstance,
			IDR_MAIN_MENU, 0);
	}

	// NOTE: Add the command buttons
	CallCommCtrlFunc(CommandBar_AddBitmap)(v_hCmdBar, HINST_COMMCTRL,
										   IDB_VIEW_SMALL_COLOR,
										   12, 16, 16);
	CallCommCtrlFunc(CommandBar_AddBitmap)(v_hCmdBar, HINST_COMMCTRL,
										   IDB_STD_SMALL_COLOR,
										   15, 16, 16);
	if (!v_fPortrait) {
    
		CommandBar_AddButtons(v_hCmdBar, sizeof(tbButton)/sizeof(TBBUTTON),
							  tbButton);

		LoadString(v_hInst, IDS_TLTP_NON, nonStr, sizeof(nonStr));
		LoadString(v_hInst, IDS_TLTP_DELETE, deleteStr,
				   sizeof(deleteStr));
		LoadString(v_hInst, IDS_TLTP_PRPTY, propertiesStr,
				   sizeof(propertiesStr));
		LoadString(v_hInst, IDS_TLTP_LARGE, largeIconStr,
				   sizeof(largeIconStr));
		LoadString(v_hInst, IDS_TLTP_SMOLL, smallIconStr,
				   sizeof(smallIconStr));
		LoadString(v_hInst, IDS_TLTP_DETAILS, detailsStr,
				   sizeof(detailsStr));

		CommandBar_AddToolTips(v_hCmdBar, 6, (LPARAM)ToolTipsTbl);

		CallCommCtrlFunc(CommandBar_AddAdornments)(v_hCmdBar, STD_HELP,
			ID_FILE_EXIT);
		SendMessage (v_hCmdBar, TB_CHECKBUTTON, ID_VIEW_LARGEICON,
					 MAKELONG(TRUE, 0));

		// Set the default view checkbox
		hMenu = CallCommCtrlFunc(CommandBar_GetMenu)(v_hCmdBar, 0);
		CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICON,
						   ID_VIEW_DETAILS,
						   ID_VIEW_LARGEICON,
						   MF_BYCOMMAND);
	
	} else {
		rv = CallCommCtrlFunc(CommandBar_AddBitmap)(v_hCmdBar, v_hInst,
			IDB_TOOLBAR, 2, 16, 16);
		
		tbButtonPortrait[1].iBitmap = rv+1;
//		tbButtonPortrait[3].iBitmap = rv;

		 
		CommandBar_AddButtons(v_hCmdBar,
							  sizeof(tbButtonPortrait)/sizeof(TBBUTTON),
							  tbButtonPortrait);
	
		LoadString(v_hInst, IDS_TLTP_NON, nonStr, sizeof(nonStr));
		LoadString(v_hInst, IDS_TLTP_CONNECT, largeIconStr,
				   sizeof(largeIconStr));
		LoadString(v_hInst, IDS_TLTP_EDIT, smallIconStr,
				   sizeof(smallIconStr));
		LoadString(v_hInst, IDS_TLTP_DELETEP, propertiesStr,
				   sizeof(propertiesStr));


		CommandBar_AddToolTips(v_hCmdBar, 4, (LPARAM)ToolTipsTblPortrait);

		CallCommCtrlFunc(CommandBar_AddAdornments)(v_hCmdBar, STD_HELP, ID_CONN_EXIT);
	}
	
	rc.top += CallCommCtrlFunc(CommandBar_Height)(v_hCmdBar);

	
	v_hListWnd = CreateWindow (WC_LISTVIEW, TEXT(""),
                               WS_VISIBLE|WS_CHILD|WS_VSCROLL|
                               LVS_SHOWSELALWAYS|LVS_AUTOARRANGE|
                               0x0040|LVS_EDITLABELS|
							   LVS_ICON,
                               rc.left, rc.top,  rc.right - rc.left,
                               rc.bottom - rc.top, v_hMainWnd,
                               (HMENU)IDD_LISTVIEW,
                               v_hInst, NULL);

    if ( v_hListWnd == 0 ) {
        return (FALSE);
    }


    if (!InitListViewImageLists(v_hListWnd) ||
		!InitListViewColumns(v_hListWnd) ||
		!InitListViewItems(v_hListWnd)) {
		DestroyWindow(v_hListWnd);
		return FALSE;
	}

    ShowWindow(v_hMainWnd, SW_SHOW);
    UpdateWindow(v_hMainWnd);

	// Focus in on the list view.
	SetFocus (v_hListWnd);
    return TRUE;
}

void LoadSIP(void)
{
#ifdef USE_SIP
    if( (g_hSipLib = LoadLibrary( TEXT("coredll.dll") )) &&
        (g_pSipStatus = (LPFNSIPSTATUS)GetProcAddress( g_hSipLib, TEXT("SipStatus"))) &&
	    (g_pSipGetInfo = (LPFNSIP)GetProcAddress( g_hSipLib, TEXT("SipGetInfo"))) &&
	    (g_pSipSetInfo = (LPFNSIP)GetProcAddress( g_hSipLib, TEXT("SipSetInfo"))) &&
	    (SIP_STATUS_AVAILABLE == g_pSipStatus()) )
	{
		DEBUGMSG(1, (L"REMNET: Using SIP!\r\n"));
	}
	else
	{
        g_pSipStatus = NULL; g_pSipGetInfo = NULL; g_pSipSetInfo = NULL;
    	if(g_hSipLib) FreeLibrary(g_hSipLib);
    }
#endif
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPWSTR pszCmdLine,
        int nCmdShow)
{
    MSG		msg;
	HWND	hPrevWind;
	HANDLE	hAccelTable;

	DEBUGREGISTER(0);

	if (GetSystemMetrics(SM_CXSCREEN) < 480) {
		v_fPortrait = TRUE;
	}

	// Check if SIP is present
	LoadSIP();

#ifdef DEBUG
	// Scan command line
	{
		while (*pszCmdLine) {
			// Skip leading blanks
			while (' ' == *pszCmdLine) {
				pszCmdLine++;
			}
			if ('\0' == *pszCmdLine) {
				break;
			}
			if ((TEXT('-') == *pszCmdLine) || (TEXT('/') == *pszCmdLine)) {
				pszCmdLine++;

				while ((TEXT(' ') != *pszCmdLine) && (TEXT('\0') != *pszCmdLine)) {
					switch (*pszCmdLine) {
					case 'p' :
					case 'P' :
						v_fPortrait = TRUE;
						break;
					case 'l' :
					case 'L' :
						v_fPortrait = FALSE;
						break;
					case 'd' :
					case 'D' :
						// Get debug flag
						dpCurSettings.ulZoneMask = 0;
						pszCmdLine++;
						while ((TEXT('0') <= *pszCmdLine) && (TEXT('9') >= *pszCmdLine)) {
							dpCurSettings.ulZoneMask *= 10;
							dpCurSettings.ulZoneMask += *pszCmdLine - TEXT('0');
							pszCmdLine++;
						}
						pszCmdLine--;
						DEBUGMSG (1, (TEXT("New ulZoneMask=%d\r\n"),
									  dpCurSettings.ulZoneMask));
						break;
					default :
						DEBUGMSG (1, (TEXT("Unknown switch '%s'\r\n"), pszCmdLine));
						break;
					}
					pszCmdLine++;
				}
			} else {
				DEBUGMSG (1, (TEXT("Bad commandline parm '%s'\r\n"), pszCmdLine));
				break;
			}
		}
	}
#endif
	

	LoadString(hInstance, (v_fPortrait) ? IDS_CONNECTIONS : IDS_TITLE,
			   szTitle, sizeof(szTitle) / sizeof(TCHAR));

	if (hPrevWind = FindWindow (szAppName, szTitle)) {
        SetForegroundWindow((HWND) ((ULONG) hPrevWind | 0x00000001));
		return FALSE;
	}
	
	if (!InitCommCtrlTable()) {
		DEBUGMSG (ZONE_ERROR, (TEXT("Unable to load commctrl.dll\r\n")));
		return FALSE;
	}

	CallCommCtrlFunc(InitCommonControls)();

	{ INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC  = ICC_TOOLTIP_CLASSES|ICC_CAPEDIT_CLASS;
	CallCommCtrlFunc(InitCommonControlsEx)(&icce); }

	if ( hInstPrev != 0 ) {
        return FALSE;
    }

    DEBUGMSG (ZONE_MISC, (TEXT("About to InitApplication\r\n")));
    if ( InitApplication(hInstance) == FALSE ) {
        return(FALSE);
    }

	DEBUGMSG (ZONE_MISC, (TEXT("About to InitInstance\r\n")));
    if ( InitInstance(hInstance, nCmdShow) == FALSE ) {
		// Unregister IP class window
		UnregisterIPClass(hInstance);
        return(FALSE);
    }
	
	if (v_fPortrait) {
		// Update dialogs ID table with small version.
		v_DialogPages[DLG_PG_1] = IDD_RAS_WIZ_1P;
		v_DialogPages[DLG_PG_2] = IDD_RAS_WIZ_2P;
		v_DialogPages[DLG_PG_3] = IDD_RAS_WIZ_3P;
		v_DialogPages[DLG_PG_4] = IDD_RAS_WIZ_4P;
		v_DialogPages[DLG_PG_5] = IDD_RAS_WIZ_5P;
		v_DialogPages[DLG_TCP_GEN] = IDD_RAS_TCPIP_GENP;
		v_DialogPages[DLG_TCP_NS] = IDD_RAS_TCPIP_NAME_SERVP;
	}
	

	hAccelTable = LoadAccelerators (hInstance,TEXT("REMNET_ACCEL"));

	SetMenu();

	DEBUGMSG (ZONE_MISC, (TEXT("About to enter GetMessage Loop\r\n")));
    while ( GetMessage(&msg, NULL, 0, 0) != FALSE ) {
		if (v_hDialogWnd && IsDialogMessage (v_hDialogWnd, &msg)) {
			continue;
		}
		if (v_fInRename ||
			!TranslateAccelerator (v_hMainWnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }

    // Unregister IP class window
    UnregisterIPClass(hInstance);
	
	DEBUGMSG (ZONE_MISC, (TEXT("Exiting WinMain\r\n")));
    return(msg.wParam);
}



