/******************************************************************************
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

remwiz.c : Remote networking wizard dialogs

******************************************************************************/

// ----------------------------------------------------------------
//
//	IDD_RAS_WIZ_1
//  ( Enter Name and Dial/Direct/Vpn)
//    |
//    +-------Dial?-------> IDD_RAS_WIZ_2  
//    |                     (Enter Modem Name, have Config+TCP settings)
//    |                           |
//    |                           |
//    |                     IDD_RAS_WIZ_3
//    | 	                (Enter Phone Num, Fin)
//    | 
//    |
//    |
//    +-------Direct?-----> IDD_RAS_WIZ_4
//    |                     (Enter DevName, Fin)
//    |                     (Config + TCP Settings)
//    |
//    |
//    |
//    +-------VPN?--------> IDD_RAS_WIZ_5
//						    (Enter DevName, Fin)
//						    (Config + TCP Settings)
//
//
//
//
//
// ----------------------------------------------------------------


#include <windows.h>
#include <tchar.h>
#include "string.h"
#include "memory.h"
#include "commctrl.h"
#include "remnet.h"
#include "resource.h"
#include "ipaddr.h"

#include "tapi.h"
#include "unimodem.h"
#include "dbt.h"

PBYTE	v_pDevConfig;
DWORD	v_dwDevConfigSize;

DWORD Device;
BOOL  v_EnteredAsAWizard;
extern BOOL		v_fPortrait;
extern const TCHAR szAppName[];
extern HINSTANCE	v_hInst;

#define COUNTRY_CODE_SIZE   16

WNDPROC g_pEditProc;

// 
// ----------------------------------------------------------------
//
// GetRasDevices
//
// Call's RasEnumDevices() to get an array of the Ras Device names
// and types.
//
// Returns the number of devices found.
//
// NOTE: Caller is responsible for freeing the pRasDevInfo data.
//
// Example usage:
//
//			dwRasDevices = GetRasDevices (&pRasDevInfo);
//			for (i=0; i < dwRasDevices; i++) {
//				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Modem)) {
//					_tcscpy (EditItem.Entry.szDeviceName, pRasDevInfo[0].szDeviceName);
//					break;
//				}
//			}
//			if (pRasDevInfo) {
//				LocalFree (pRasDevInfo);
//			}
//
// ----------------------------------------------------------------			

DWORD
GetRasDevices (LPRASDEVINFO *pRasDevInfo)
{
	DWORD	cBytes, cNumDev, dwRetVal;

	*pRasDevInfo = NULL;
	cBytes = 0;
	cNumDev = 0;
	dwRetVal = RasEnumDevices (NULL, &cBytes, &cNumDev);
	if (0 == dwRetVal) {
		// Allocate a buffer
		*pRasDevInfo = (LPRASDEVINFO) LocalAlloc (LPTR, cBytes);

		if (pRasDevInfo) {
			dwRetVal = RasEnumDevices (*pRasDevInfo, &cBytes, &cNumDev);
		} else {
			DEBUGMSG (ZONE_ERROR, (TEXT("Error %d doing LocalAlloc of RasDev's\r\n"),
								   GetLastError()));
			LocalFree (*pRasDevInfo);
			*pRasDevInfo = NULL;
			cNumDev = 0;
		}
		
	} else {
		DEBUGMSG (ZONE_ERROR, (TEXT("Error %d from RasEnumDevices()\r\n"), dwRetVal));
		cNumDev = 0;
	}

#ifdef DEBUG
	DEBUGMSG (1, (TEXT("GetRasDevices: Found %d devices\r\n"), cNumDev));
	for (cBytes=0; cBytes < cNumDev; cBytes++) {
		DEBUGMSG (1, (TEXT("\tDev[%d] : Name='%s' Type='%s'\r\n"),
					  cBytes,
					  (*pRasDevInfo)[cBytes].szDeviceName,
					  (*pRasDevInfo)[cBytes].szDeviceType));
	}
#endif
	
	return cNumDev;
}
	
LONG EditFullWidthConversionSubclassProc(
    HWND hwnd,
    UINT nMsg,
    WPARAM wparam,
    LPARAM lparam)
{
    TCHAR ch, chHW;

    ASSERT (g_pEditProc);

    switch(nMsg)
    {
    case WM_CHAR:
        ch = (TCHAR)wparam;
        LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH,
                   &ch, 1, &chHW, 1);
        wparam = (WPARAM)chHW;
        break;
    }
    return CallWindowProc(g_pEditProc, hwnd, nMsg, wparam, lparam);
}

BOOL
GetDialUpSettings(HWND hDlg, TCHAR CountryCode[], BOOL Validate)
{
    TCHAR           *pGet, *pPut;
	TCHAR		    szTemp[128];
    UINT            szTempSize = sizeof(szTemp) / sizeof(TCHAR);
	TCHAR			szFmtStr[128];
    UINT            cFmtStrSize = sizeof(szFmtStr) / sizeof(TCHAR);
    
    // Area code
    GetWindowText (GetDlgItem (hDlg, IDC_AREA_CODE),
                   EditItem.Entry.szAreaCode,
                   RAS_MaxAreaCode+1);

    // Phone Number
    GetWindowText (GetDlgItem (hDlg, IDC_PHONE_NUM),
                   EditItem.Entry.szLocalPhoneNumber,
                   RAS_MaxPhoneNumber+1);
    
    // trim white space in phone number
    pGet = pPut = EditItem.Entry.szLocalPhoneNumber;
    while (*pPut != TEXT('\0'))
    {
        if (*pGet != TEXT(' '))
            *pPut++ = *pGet;
        pGet++;
    }
    
    if (Validate)
    {
        // Validate Phone...
        if (TEXT('\0') == EditItem.Entry.szLocalPhoneNumber[0]) {
            LoadString(v_hInst, IDS_PHONEREQ, szFmtStr, cFmtStrSize);
            LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET, szTemp, szTempSize);
            MessageBox (hDlg, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
            // Will this make it blink?
            SetFocus(GetDlgItem(hDlg, IDC_PHONE_NUM));
            return FALSE;
        }
    }

    // Country Code
    GetWindowText (GetDlgItem (hDlg, IDC_COUNTRY),
                   CountryCode,
                   COUNTRY_CODE_SIZE);                
    if (CountryCode[0] == TEXT('\0'))
        EditItem.Entry.dwCountryCode = 0;
    else
        EditItem.Entry.dwCountryCode = My_atoi(CountryCode);

    // Country code check box
    if (SendMessage(GetDlgItem(hDlg, IDC_FORCELD),
					BM_GETCHECK, 0, 0)) {
		EditItem.Entry.dwfOptions |= RASEO_UseCountryAndAreaCodes;
	} else {
        EditItem.Entry.dwfOptions &= ~(RASEO_UseCountryAndAreaCodes);
	}

    if (SendMessage(GetDlgItem(hDlg, IDC_FORCELOCAL),
                    BM_GETCHECK, 0, 0)) {
        EditItem.Entry.dwfOptions |= RASEO_DialAsLocalCall;
	} else {
        EditItem.Entry.dwfOptions &= ~(RASEO_DialAsLocalCall);
	}

    // -- if they have checked the force local checkbox, make sure they have entered an area code
    if (Validate)
    {
        if ((EditItem.Entry.dwfOptions & RASEO_DialAsLocalCall) && 
            (TEXT('\0') == EditItem.Entry.szAreaCode[0]))
        {
            LoadString(v_hInst, IDS_AREAREQ, szFmtStr, cFmtStrSize);
            LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET, szTemp, szTempSize);
            MessageBox (hDlg, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
            // Will this make it blink?
            SetFocus(GetDlgItem(hDlg, IDC_AREA_CODE));
            return FALSE;
        }
    }

    return TRUE;
}


void
FixButtons(HWND hDlg, DWORD Dialog_IDD)
{
    LONG    dwStyle;
    TCHAR	szFmtStr[128];
    UINT    cFmtStrSize = sizeof(szFmtStr) / sizeof(TCHAR);
    
    if (!v_EnteredAsAWizard)
    {
        dwStyle = GetWindowLong(hDlg, GWL_STYLE);
        SetWindowLong(hDlg, GWL_STYLE, dwStyle | WS_SYSMENU);

        dwStyle = GetWindowLong(hDlg, GWL_EXSTYLE);
        SetWindowLong(hDlg, GWL_EXSTYLE, dwStyle | WS_EX_CAPTIONOKBTN);
        
        // Not a wizard, so hide buttons 
        ShowWindow(GetDlgItem(hDlg, IDBACK), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDFINISH), SW_HIDE);

        // Rename dialog title
        if (Dialog_IDD == v_DialogPages[DLG_PG_2])
        {
            LoadString(v_hInst, IDS_DIALUPTITLE, szFmtStr, cFmtStrSize);
            SetWindowText(hDlg, szFmtStr);
        }
        if (Dialog_IDD == v_DialogPages[DLG_PG_4])
        {
            LoadString(v_hInst, IDS_DIRECTTITLE, szFmtStr, cFmtStrSize);
            SetWindowText(hDlg, szFmtStr);
        }                        
        if (Dialog_IDD == v_DialogPages[DLG_PG_5])
        {
            LoadString(v_hInst, IDS_VPNTITLE, szFmtStr, cFmtStrSize);
            SetWindowText(hDlg, szFmtStr);
        }                        
    }
}
VOID
NextPage (HWND hDlg, DWORD NextPage)
{
	LV_ITEM		    lvi;

	if (0 == NextPage) {
		RasSetEntryProperties (NULL, EditItem.EntryName,
							   &(EditItem.Entry),
							   sizeof(EditItem.Entry), v_pDevConfig, v_dwDevConfigSize);

		// Refresh the list?
		InitListViewItems(v_hListWnd);

		// Set this one as the selected one.
		ListView_SetItemState (v_hListWnd, 0, 0, LVIS_SELECTED|LVIS_FOCUSED);
							
		for (lvi.iItem = -1;
			 (-1 != (lvi.iItem = ListView_GetNextItem(v_hListWnd,
				 lvi.iItem, LVNI_ALL)));) {

			if (0 == lvi.iItem) {
				continue;
			}
			lvi.iSubItem = 0;
			lvi.mask = LVIF_PARAM;
			ListView_GetItem(v_hListWnd, &lvi);
			if (!_tcscmp (EditItem.EntryName,
						  ((PITEMINFO)lvi.lParam)->EntryName)) {
				ListView_SetItemState (v_hListWnd,
									   lvi.iItem,
									   LVIS_SELECTED|LVIS_FOCUSED,
									   LVIS_SELECTED|LVIS_FOCUSED);
				break;
			}
		}
							
		// Peform Abort cleanup.
		NextPage = (DWORD)-1;
	}
	if ((DWORD)-1 == NextPage) {
		// Cancel...
		// Free the Device config info
		if (v_pDevConfig) {
			LocalFree (v_pDevConfig);
			v_pDevConfig = NULL;
			v_dwDevConfigSize = 0;
		}

        // Bring the SIP down
        PositionSIP(SIP_DOWN);
		DestroyWindow (hDlg);
		SetFocus (v_hListWnd);
		v_hDialogWnd = NULL;
		return;
	}
	// Must have a new page to display
	v_hDialogWnd = CreateDialog (v_hInst,
								 MAKEINTRESOURCE(NextPage), v_hMainWnd,
								 ConnWizDlgProc);
	DestroyWindow (hDlg);
	
}

VOID
DoNext (HWND hDlg)
{
	DWORD	i;
	TCHAR   szTemp[128];
	TCHAR	szFmtStr[128];
	int     nFormatId;
	LPRASDEVINFO	pRasDevInfo;
	DWORD	dwRasDevices;

	if (v_WizDialog == v_DialogPages[DLG_PG_1]) {
		// Validate the input.                            
		GetWindowText (GetDlgItem (hDlg, IDC_REMNAME),
					   EditItem.EntryName, RAS_MaxEntryName+1);

		if (i = RasValidateEntryName(NULL, EditItem.EntryName)) {
			if (i == ERROR_ALREADY_EXISTS) {
                nFormatId = IDS_ALREADY_EXISTS;
			} else if (*EditItem.EntryName) {
                nFormatId = IDS_BADNAME;
            } else {
                nFormatId = IDS_NULLNAME;
            }
			SetFocus (GetDlgItem(hDlg, IDC_REMNAME));
            LoadString(v_hInst, nFormatId, szFmtStr, sizeof(szFmtStr) / sizeof(TCHAR));
			LoadString(v_hInst, (v_fPortrait) ? IDS_CONNECTIONS : IDS_REMNET, szTemp,
					   sizeof(szTemp) / sizeof(TCHAR));
			MessageBox (hDlg, szFmtStr, szTemp, MB_OK | MB_ICONWARNING);
			// Will this make it blink?
			return;
		}

		// Validate the input.
		if (SendMessage (GetDlgItem (hDlg, IDC_DIALUPCONN),
						 BM_GETCHECK, 0, 0)) {
			_tcscpy (EditItem.Entry.szDeviceType,
					 RASDT_Modem);

			dwRasDevices = GetRasDevices (&pRasDevInfo);
			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Modem)) {
					_tcscpy (EditItem.Entry.szDeviceName, pRasDevInfo[0].szDeviceName);
					break;
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}
			
			// Follow Dial-Up path.
			v_WizDialog = v_DialogPages[DLG_PG_2];
		} else if (SendMessage (GetDlgItem (hDlg, IDC_DIRECTCONN),
								BM_GETCHECK, 0, 0)) {
			// They want a direct connection.
			_tcscpy (EditItem.Entry.szDeviceType,
					 RASDT_Direct);

			// Set the IP address for Replication
			EditItem.Entry.ipaddr.d = 192;
			EditItem.Entry.ipaddr.c = 168;
			EditItem.Entry.ipaddr.b = 55;
			EditItem.Entry.ipaddr.a = 100;

			// Follow Direct-Connect path
			v_WizDialog = v_DialogPages[DLG_PG_4];
		} else if (SendMessage (GetDlgItem (hDlg, IDC_VPNCONN),
								BM_GETCHECK, 0, 0)) {
			// They want a direct connection.
			_tcscpy (EditItem.Entry.szDeviceType,
					 RASDT_Vpn);
			v_WizDialog = v_DialogPages[DLG_PG_5];
		} else {
			ASSERT(0);
			DEBUGMSG (1, (TEXT("Wrong page?\r\n")));
		}
	} else if (v_WizDialog == v_DialogPages[DLG_PG_2]) {

		GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
					   EditItem.Entry.szDeviceName,
					   RAS_MaxDeviceName+1);
		v_WizDialog = v_DialogPages[DLG_PG_3];
	}
	NextPage (hDlg, v_WizDialog);
}
void CALLBACK 
lineCallbackFunc(DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
				 DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	// NULL function.  I can't do a lineInitialize without this function.
	// Since I never dial I don't actually care about any of the state changes
	return;
}
LPLINETRANSLATECAPS
GetTranslateCaps ()
{
	LPLINETRANSLATECAPS pLineTranCaps = NULL;
	DWORD				dwNeededSize;
	long				lReturn;
	HLINEAPP			hLineApp;
	DWORD				dwVersion = TAPI_CURRENT_VERSION;
	DWORD				dwNumDevs;

	DEBUGMSG (ZONE_FUNCTION,
			  (TEXT("+GetTranslateCaps()\r\n")));

	lReturn = lineInitialize (&hLineApp, v_hInst, lineCallbackFunc, szAppName, &dwNumDevs);

	if (lReturn) {
		DEBUGMSG (ZONE_FUNCTION | ZONE_ERROR,
				  (TEXT("-GetTranslateCaps(): Error %d from lineInitialize\r\n"),
				   lReturn));
		return NULL;
	}
	
	dwNeededSize = sizeof(LINETRANSLATECAPS);

	while (1) {
		pLineTranCaps = LocalAlloc (LPTR, dwNeededSize);
		if (NULL == pLineTranCaps) {
			DEBUGMSG (ZONE_ERROR|ZONE_ALLOC,
					 (TEXT("!GetTranslateCaps: Alloc failed\r\n")));
			break;
		}
		pLineTranCaps->dwTotalSize = dwNeededSize;
		lReturn = lineGetTranslateCaps (hLineApp, dwVersion, pLineTranCaps);

		if (lReturn) {
			DEBUGMSG (ZONE_ERROR, (TEXT(" GetTranslateCaps:lineGetTranslateCaps returned 0x%X\r\n"),
								   lReturn));
			LocalFree (pLineTranCaps);
			pLineTranCaps = NULL;
			break;
		}
		if (pLineTranCaps->dwNeededSize > pLineTranCaps->dwTotalSize) {
			dwNeededSize = pLineTranCaps->dwNeededSize;
			LocalFree (pLineTranCaps);
			pLineTranCaps = NULL;
			continue;
		}
		break;
	}

	lineShutdown (hLineApp);

	DEBUGMSG (ZONE_FUNCTION, (TEXT("-TapiLineGetTranslateCaps Returning 0x%X\r\n"), 
							  pLineTranCaps));
	return pLineTranCaps;
	
}
DWORD
GetDefaultCountryCode ()
{
	LPLINETRANSLATECAPS pCaps;
	LPLINELOCATIONENTRY	pLocEntry;
	int	i;
	DWORD	CountryCode = 0;

	pCaps = GetTranslateCaps();
	
	if (pCaps) {
        if( pCaps->dwLocationListSize ) {
            pLocEntry = (LPLINELOCATIONENTRY) ((LPBYTE)pCaps + pCaps->dwLocationListOffset);
            for( i=0; i < (int)pCaps->dwNumLocations; i++ ) {
                if( pLocEntry[i].dwPermanentLocationID ==
					pCaps->dwCurrentLocationID ) {
					CountryCode = pLocEntry[i].dwCountryCode;
				}
			}
		}
		LocalFree (pCaps);
	}
	return CountryCode;
}
VOID
GetDefaultAreaCode (PTCHAR szAreaCode, DWORD cMaxLen)
{
	LPLINETRANSLATECAPS pCaps;
	LPLINELOCATIONENTRY	pLocEntry;
	int	i;
	DWORD	CountryCode = 0;

	szAreaCode[0] = TEXT('\0');
	
	pCaps = GetTranslateCaps();
	
	if (pCaps) {
        if( pCaps->dwLocationListSize ) {
            pLocEntry = (LPLINELOCATIONENTRY) ((LPBYTE)pCaps + pCaps->dwLocationListOffset);
            for( i=0; i < (int)pCaps->dwNumLocations; i++ ) {
                if( pLocEntry[i].dwPermanentLocationID ==
					pCaps->dwCurrentLocationID ) {
					_tcsncpy (szAreaCode, (LPTSTR)((LPBYTE)pCaps +
						pLocEntry[i].dwCityCodeOffset),
							  cMaxLen);
				}
			}
		}
		LocalFree (pCaps);
	}
	return;
}

DWORD WINAPI
DevConfigThread (LPVOID pvarg)
{
	DWORD	dwNeededSize;
	LPVARSTRING	pVarString;
	HWND	hDlg = (HWND)pvarg;
	DWORD	RetVal;
	
	dwNeededSize = sizeof(VARSTRING);
	while (1) {
		pVarString = (LPVARSTRING)LocalAlloc (LPTR, dwNeededSize);
		pVarString->dwTotalSize = dwNeededSize;
		if (NULL == v_pDevConfig) {
			v_dwDevConfigSize = 0;
		}

		RetVal = RasDevConfigDialogEdit (EditItem.Entry.szDeviceName,
										 EditItem.Entry.szDeviceType,
										 hDlg, v_pDevConfig, v_dwDevConfigSize,
										 pVarString);
		if (STATUS_SUCCESS != RetVal) {
			DEBUGMSG (ZONE_ERROR, (TEXT("RemNet: Error 0x%X(%d) from RasDevConfigDialogEdit\r\n"),
								   RetVal, RetVal));
			LocalFree (pVarString);
			break;
		} else if (pVarString->dwNeededSize > pVarString->dwTotalSize) {
			// Structure not large enough.  Get new size and free original
			dwNeededSize = pVarString->dwNeededSize;
			LocalFree (pVarString);
		} else {
			// Free the original.
			if (v_pDevConfig) {
				v_dwDevConfigSize = 0;
				LocalFree (v_pDevConfig);
			}
			v_pDevConfig = (PBYTE)LocalAlloc (LMEM_FIXED, pVarString->dwStringSize);
			if (NULL == v_pDevConfig) {
				DEBUGMSG (ZONE_ERROR, (TEXT("RemNet: Unable to allocate v_pDevConfig\r\n")));
			} else {
				memcpy (v_pDevConfig, (LPBYTE)pVarString + pVarString->dwStringOffset, pVarString->dwStringSize);
				v_dwDevConfigSize = pVarString->dwStringSize;
			}
			LocalFree (pVarString);
			break;
		}
	}
	return 0;
}

LRESULT CALLBACK
ConnWizDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR   CountryCode[COUNTRY_CODE_SIZE];
	DWORD	dwStyle;
	DWORD	i;
    int     nDefFocus;
	long	lResult;
	TCHAR   szTemp[128];
    UINT    cTempStrSize = sizeof(szTemp) / sizeof(TCHAR);
	LPRASDEVINFO	pRasDevInfo;
	DWORD	dwRasDevices;
	DWORD	RetVal;

    
    switch (message) {
	case WM_HELP :
		RETAILMSG (1, (TEXT("PegHelp file:rnetw.htm#Main_Contents\r\n")));
		CreateProcess(TEXT("peghelp"),TEXT("file:rnetw.htm#Main_Contents"),
					  NULL,NULL,FALSE,0,NULL,NULL,NULL,NULL);
		break;
		
	case WM_DEVICECHANGE :

		// Are we on Wiz2?
		if (v_WizDialog == v_DialogPages[DLG_PG_2]) {
			GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
						   EditItem.Entry.szDeviceName,
						   RAS_MaxDeviceName+1);

			// Clear the list...
			SendMessage(GetDlgItem (hDlg, IDC_MODEM),
						CB_RESETCONTENT, 0, 0);

			dwRasDevices = GetRasDevices (&pRasDevInfo);

			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Modem)) {
					SendMessage(GetDlgItem (hDlg, IDC_MODEM),
								CB_ADDSTRING,
								0, (LPARAM)pRasDevInfo[i].szDeviceName);
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}
			
			// Try to find the modem in the list.
			lResult = SendMessage (GetDlgItem (hDlg, IDC_MODEM),
								   CB_FINDSTRINGEXACT,
								   0,
								   (LPARAM)EditItem.Entry.szDeviceName);

			if (lResult == CB_ERR) {
				// Couldn't find it, just set to first entry
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, 0, 0);
			} else {
				// Set to the correct device.
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, (WPARAM)lResult, 0);
			}
		} else if (v_WizDialog == v_DialogPages[DLG_PG_4]) {
			// Save the users current value
			GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
						   EditItem.Entry.szDeviceName,
						   RAS_MaxDeviceName+1);

			// Clear the list...
			SendMessage(GetDlgItem (hDlg, IDC_MODEM),
						CB_RESETCONTENT, 0, 0);

			dwRasDevices = GetRasDevices (&pRasDevInfo);

			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Direct)) {
					SendMessage(GetDlgItem (hDlg, IDC_MODEM),
								CB_ADDSTRING,
								0, (LPARAM)pRasDevInfo[i].szDeviceName);
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}

			// Try to find the modem in the list.
			lResult = SendMessage (GetDlgItem (hDlg, IDC_MODEM),
								   CB_FINDSTRINGEXACT, 0,
								   (LPARAM)EditItem.Entry.szDeviceName);

			if (lResult == CB_ERR) {
				// Couldn't find it, just set to first entry
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, 0, 0);
			} else {
				// Set to the correct device.
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, (WPARAM)lResult, 0);
			}
		}
		break;
	case WM_INITDIALOG :
		dwStyle = GetWindowLong (hDlg, GWL_EXSTYLE);
        if (!v_fPortrait) {
            dwStyle |= WS_EX_CONTEXTHELP;
        }
		SetWindowLong (hDlg, GWL_EXSTYLE, dwStyle | WS_EX_NODRAG |
					   WS_EX_WINDOWEDGE | WS_EX_CAPTIONOKBTN);
		
		if (NULL == v_pDevConfig) {
			RASENTRY	RasEntry;
			DWORD		dwSize;

			// Read in the previous DevConfig if any
			dwSize = sizeof(RasEntry);
			RasEntry.dwSize = dwSize;
			v_dwDevConfigSize = 0;
			RetVal = RasGetEntryProperties (NULL, EditItem.EntryName, &(RasEntry),
											&dwSize, NULL,
											&v_dwDevConfigSize);

			if (v_dwDevConfigSize && (ERROR_BUFFER_TOO_SMALL == RetVal)) {
				v_pDevConfig = (PBYTE)LocalAlloc (LPTR, v_dwDevConfigSize);
				if (v_pDevConfig) {
					RetVal = RasGetEntryProperties (NULL, EditItem.EntryName,
						&(RasEntry), &dwSize, v_pDevConfig,
						&v_dwDevConfigSize);
					DEBUGMSG (RetVal, (TEXT("Error %d from RasGetEntryProperties\r\n")));
					ASSERT (RetVal == 0);
				}
			}
		}
			
		if (v_WizDialog == v_DialogPages[DLG_PG_1]) {

			// Set the connection name.
			SetWindowText (GetDlgItem (hDlg, IDC_REMNAME),EditItem.EntryName);

			// Restrict them to 20 characters
			SendMessage (GetDlgItem (hDlg, IDC_REMNAME), EM_LIMITTEXT,
						 RAS_MaxEntryName, 0);
            SendMessage (GetDlgItem (hDlg, IDC_REMNAME), EM_SETSEL, 0, -1);

			
			
			// Indicate that we didn't find the current device type
			CheckRadioButton (hDlg, IDC_DIALUPCONN, IDC_VPNCONN,
							  IDC_DIRECTCONN);

			// Start by disabling all of the windows.
			EnableWindow (GetDlgItem (hDlg, IDC_DIALUPCONN), FALSE);
			EnableWindow (GetDlgItem (hDlg, IDC_DIRECTCONN), FALSE);
			EnableWindow (GetDlgItem (hDlg, IDC_VPNCONN), FALSE);
							  
			dwRasDevices = GetRasDevices (&pRasDevInfo);
			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Modem)) {
					EnableWindow (GetDlgItem (hDlg, IDC_DIALUPCONN), TRUE);
					if (!_tcscmp (EditItem.Entry.szDeviceType, RASDT_Modem)) {
						CheckRadioButton (hDlg, IDC_DIALUPCONN, IDC_VPNCONN,
										  IDC_DIALUPCONN);
					}
				} else if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Direct)) {
					EnableWindow (GetDlgItem (hDlg, IDC_DIRECTCONN), TRUE);
					if (!_tcscmp (EditItem.Entry.szDeviceType, RASDT_Direct)) {
						CheckRadioButton (hDlg, IDC_DIALUPCONN, IDC_VPNCONN,
										  IDC_DIRECTCONN);
					}
				} else if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Vpn)) {
					EnableWindow (GetDlgItem (hDlg, IDC_VPNCONN), TRUE);
					if (!_tcscmp (EditItem.Entry.szDeviceType, RASDT_Vpn)) {
						CheckRadioButton (hDlg, IDC_DIALUPCONN, IDC_VPNCONN,
										  IDC_VPNCONN);
					}
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}
			

			// Edit box gets focus by default
            nDefFocus = IDC_REMNAME;

		} else if (v_WizDialog == v_DialogPages[DLG_PG_2]) {
			
			// Changes buttons if this is a wizard dialog or not
			FixButtons(hDlg, v_DialogPages[DLG_PG_2]);
			
			// Set the connection name.
			SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
						   EditItem.EntryName);

			dwRasDevices = GetRasDevices (&pRasDevInfo);
			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Modem)) {
					SendMessage(GetDlgItem (hDlg, IDC_MODEM),
								CB_ADDSTRING,
								0, (LPARAM)pRasDevInfo[i].szDeviceName);
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}
			
			// Try to find the modem in the list.
			lResult = SendMessage (GetDlgItem (hDlg, IDC_MODEM),
								   CB_FINDSTRINGEXACT,
								   0,
								   (LPARAM)EditItem.Entry.szDeviceName);

			if (lResult == CB_ERR) {
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, 0, 0);
				GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
							   EditItem.Entry.szDeviceName,
							   RAS_MaxDeviceName+1);
				
			} else {
				// Set to the correct device.
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, (WPARAM)lResult, 0);
			}

            // Modem combo box gets focus by default
            nDefFocus = IDC_MODEM;

		} else if (v_WizDialog == v_DialogPages[DLG_PG_3]) {
			
			if (v_EnteredAsAWizard)
            {
				if (EditItem.Entry.dwCountryCode == 0) {
					EditItem.Entry.dwCountryCode = GetDefaultCountryCode ();
				}
				if (EditItem.Entry.szAreaCode[0] == TEXT('\0')) {
					GetDefaultAreaCode (EditItem.Entry.szAreaCode, RAS_MaxAreaCode);
				}
            } else {
                LoadString(v_hInst, IDS_DIALUPTITLE, szTemp, cTempStrSize);
                SetWindowText(hDlg, szTemp);
            }
						
			// Set the connection name.
			SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
						   EditItem.EntryName);
			
			// Set Phone Number
			SetWindowText (GetDlgItem (hDlg, IDC_AREA_CODE),
						   EditItem.Entry.szAreaCode);
			SetWindowText (GetDlgItem (hDlg, IDC_PHONE_NUM),
						   EditItem.Entry.szLocalPhoneNumber) ;
			
			// Initialize Country code list box.                    
			CountryCode[0] = TEXT('\0');
			if (EditItem.Entry.dwCountryCode)
				wsprintf(CountryCode, TEXT("%d"),
						 EditItem.Entry.dwCountryCode);
			SetWindowText (GetDlgItem (hDlg, IDC_COUNTRY),
						   CountryCode);

			// Initialize the use country check box
			if (EditItem.Entry.dwfOptions & RASEO_UseCountryAndAreaCodes) {
				SendMessage (GetDlgItem(hDlg, IDC_FORCELD), BM_SETCHECK,
							 1, 0);
			}
			if (EditItem.Entry.dwfOptions & RASEO_DialAsLocalCall) {
				SendMessage (GetDlgItem(hDlg, IDC_FORCELOCAL), BM_SETCHECK,
							 1, 0);
			}

            // Phone number gets focus by default
            nDefFocus = IDC_PHONE_NUM;

            g_pEditProc = (WNDPROC)SetWindowLong(GetDlgItem(hDlg, IDC_AREA_CODE),
                                                 GWL_WNDPROC,
                                                 (LONG)EditFullWidthConversionSubclassProc);
            SetWindowLong(GetDlgItem(hDlg, IDC_PHONE_NUM),
                          GWL_WNDPROC,
                          (LONG)EditFullWidthConversionSubclassProc);
            SetWindowLong(GetDlgItem(hDlg, IDC_COUNTRY),
                          GWL_WNDPROC,
                          (LONG)EditFullWidthConversionSubclassProc);

		} else if (v_WizDialog == v_DialogPages[DLG_PG_4]) {
			// Changes buttons if this is a wizard dialog or not
			FixButtons(hDlg, v_DialogPages[DLG_PG_4]);
			
			// Set the connection name.
			SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
						   EditItem.EntryName);

			dwRasDevices = GetRasDevices (&pRasDevInfo);

			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Direct)) {
					SendMessage(GetDlgItem (hDlg, IDC_MODEM),
								CB_ADDSTRING,
								0, (LPARAM)pRasDevInfo[i].szDeviceName);
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}

			// Try to find the modem in the list.
			lResult = SendMessage (GetDlgItem (hDlg, IDC_MODEM),
								   CB_FINDSTRINGEXACT, 0,
								   (LPARAM)EditItem.Entry.szDeviceName);

			if (lResult == CB_ERR) {
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, 0, 0);
				GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
							   EditItem.Entry.szDeviceName,
							   RAS_MaxDeviceName+1);
			} else {
				// Set to the correct device.
				SendMessage (GetDlgItem (hDlg, IDC_MODEM),
							 CB_SETCURSEL, (WPARAM)lResult, 0);
			}
			
            // gets focus by default
            nDefFocus = IDC_MODEM;
		} else if (v_WizDialog == v_DialogPages[DLG_PG_5]) {
			// The VPN config page
			FixButtons(hDlg, v_DialogPages[DLG_PG_5]);
			
			dwRasDevices = GetRasDevices (&pRasDevInfo);

			for (i=0; i < dwRasDevices; i++) {
				if (!_tcscmp (pRasDevInfo[i].szDeviceType, RASDT_Vpn)) {
					// Just copy the first device to the entry.
					_tcsncpy (EditItem.Entry.szDeviceType, RASDT_Vpn, RAS_MaxDeviceType);
					_tcsncpy (EditItem.Entry.szDeviceName, pRasDevInfo[i].szDeviceName,
							 RAS_MaxDeviceName);
					break;
				}
			}
			if (pRasDevInfo) {
				LocalFree (pRasDevInfo);
			}
			
			// Set the connection name.
			SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
						   EditItem.EntryName);

			SetWindowText (GetDlgItem (hDlg, IDC_HOSTNAME),
						   EditItem.Entry.szLocalPhoneNumber);
			
			SendMessage (GetDlgItem (hDlg, IDC_HOSTNAME), EM_LIMITTEXT,
						 RAS_MaxPhoneNumber, 0);
			
            nDefFocus = IDC_HOSTNAME;
		} else {
			
			ASSERT (0);
		}

        // Set focus
        SetFocus(GetDlgItem(hDlg, nDefFocus));
		return FALSE;
	case WM_COMMAND :
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			DEBUGMSG (ZONE_WARN,
					  (TEXT("Got CBN_SELCHANGE message\r\n")));
			// Make sure that it's the Modem
			if (LOWORD(wParam) == IDC_MODEM) {
				// Release any Device Config info they had edited.
				if (v_pDevConfig) {
					LocalFree (v_pDevConfig);
					v_pDevConfig = NULL;
					v_dwDevConfigSize = 0;
				}
			}
			break;
		}
		switch (LOWORD(wParam)) {
        case IDCANCEL :
			NextPage (hDlg, (DWORD)-1);
			break;

        case IDFINISH:
		case IDOK:
			if ((v_WizDialog == v_DialogPages[DLG_PG_1]) ||
				(v_WizDialog == v_DialogPages[DLG_PG_2])) {
				DoNext (hDlg);
			} else if (v_WizDialog == v_DialogPages[DLG_PG_3]) {
				// Validate the input.
				if (GetDialUpSettings(hDlg, CountryCode, TRUE))
                {
                    v_WizDialog = 0;
					NextPage (hDlg, v_WizDialog);
                }
			} else if (v_WizDialog == v_DialogPages[DLG_PG_4]) {
				GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
							   EditItem.Entry.szDeviceName,
							   RAS_MaxDeviceName+1);
				v_WizDialog = 0;
				NextPage (hDlg, v_WizDialog);
			} else if (v_WizDialog == v_DialogPages[DLG_PG_5]) {
				// Should we validate the string???
				
				GetWindowText (GetDlgItem (hDlg, IDC_HOSTNAME),
							   EditItem.Entry.szLocalPhoneNumber,
							   RAS_MaxPhoneNumber+1);
				v_WizDialog = 0;
				NextPage (hDlg, v_WizDialog);
			}            
			break;
			
		case IDNEXT :
			DoNext (hDlg);
			break;
			
		case IDBACK :
			if (v_WizDialog == v_DialogPages[DLG_PG_2]) {
				// Save the modem name.
				GetWindowText (GetDlgItem (hDlg, IDC_MODEM),
							   EditItem.Entry.szDeviceName,
							   RAS_MaxDeviceName+1);
				v_WizDialog = v_DialogPages[DLG_PG_1];
			} else if (v_WizDialog == v_DialogPages[DLG_PG_3]) {
				(void) GetDialUpSettings(hDlg, CountryCode, FALSE);
                v_WizDialog = v_DialogPages[DLG_PG_2];
			} else if (v_WizDialog == v_DialogPages[DLG_PG_4]) {
				v_WizDialog = v_DialogPages[DLG_PG_1];
				Device = SendMessage (GetDlgItem (hDlg, IDC_MODEM),
									  CB_GETCURSEL, 0, 0);
			} else if (v_WizDialog == v_DialogPages[DLG_PG_5]) {
				v_WizDialog = v_DialogPages[DLG_PG_1];
			}
			NextPage (hDlg, v_WizDialog);
			break;

		case IDC_TCPSETTINGS :
			TCP_IP_Properties (hDlg, &(EditItem));
			break;

		case IDC_FORCELOCAL :
			if (SendMessage(GetDlgItem(hDlg, IDC_FORCELOCAL),
							BM_GETCHECK, 0, 0)) {
				// Clear the FORCELD checkbox.
				SendMessage (GetDlgItem(hDlg, IDC_FORCELD), BM_SETCHECK,
							 0, 0);
			}
			break;

		case IDC_FORCELD :
			if (SendMessage(GetDlgItem(hDlg, IDC_FORCELD),
							BM_GETCHECK, 0, 0)) {
				SendMessage (GetDlgItem(hDlg, IDC_FORCELOCAL), BM_SETCHECK,
							 0, 0);
			}
			break;

		case IDC_CONFIG :
		{
			static HANDLE	hThread = NULL;
			DWORD	ThreadID;
			DWORD	RetVal;
			MSG		NewMsg;

			if (hThread != NULL) {
				RETAILMSG (1, (TEXT("Recursing, only one config allowed at a time\r\n")));
				ASSERT(1);
				break;
			}
			
			hThread = CreateThread (NULL, 0, DevConfigThread, (LPVOID)hDlg, 0, &ThreadID);

			while (1) {
				RetVal = MsgWaitForMultipleObjectsEx (1, &hThread, INFINITE, QS_PAINT, 0);
				if (WAIT_OBJECT_0 == RetVal) {
					// The thread died.  Break out of this loop
					break;
				} else if ((WAIT_OBJECT_0+1) == RetVal) {
					if (PeekMessage (&NewMsg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE)) {
						if (!IsDialogMessage(hDlg, &NewMsg)) {
							TranslateMessage(&NewMsg);
							DispatchMessage(&NewMsg);
						}
					}
				}
			}

			CloseHandle (hThread);
			hThread = NULL;
		}
			
			break;
		default :
			// Unused WM_COMMAND?
			break;
		}
	case WM_DESTROY:
		return TRUE;
		break;

	default:
		return (FALSE);
    }
    return TRUE;
}


