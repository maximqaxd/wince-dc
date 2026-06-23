/******************************************************************************
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

tcpprop.c : Remote networking tcp/ip properties

******************************************************************************/
#include <windows.h>
#include <tchar.h>
#include "string.h"
#include "memory.h"
#include "commctrl.h"
#include "remnet.h"
#include "resource.h"
#include "ipaddr.h"
#include "wcommctl.h"


extern BOOL		v_fPortrait;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
LRESULT CALLBACK
TCPGenDlgProc (
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
	PROPSHEETPAGE *psp;
	NMHDR		*header;
	PITEMINFO	pItem;
	DWORD		Flag;
	
	DEBUGMSG (ZONE_MISC, (TEXT("+TCPGenDlgProc(0x%X, 0x%X, 0x%X, 0x%X)\r\n"),
				   hDlg, message, wParam, lParam));
	
	switch (message) {

	case WM_NOTIFY :
		header = (NMHDR*) lParam;

		if (PSN_APPLY != header->code) {
			break;
		}
		DEBUGMSG (1, (TEXT("Apply Changes....\r\n")));
		
		pItem = (PITEMINFO) GetWindowLong (hDlg, GWL_USERDATA);
		
		// Server assigned IP addr check box
		if (SendMessage(GetDlgItem(hDlg, IDC_SERVIPADDR),
						BM_GETCHECK, 0, 0)) {
			// Server assigned IP addr
			pItem->Entry.dwfOptions &= ~RASEO_SpecificIpAddr;
		} else {
			pItem->Entry.dwfOptions |= RASEO_SpecificIpAddr;
		}

		// Even if they want server assigned save the IP addr...
		SendMessage(GetDlgItem(hDlg, IDC_IP_ADDR), IP_GETADDRESS, 0,
					(LPARAM) ((int *) &pItem->Entry.ipaddr));

		// The check boxes
		if (SendMessage(GetDlgItem(hDlg, IDC_USESLIP), BM_GETCHECK, 0, 0)) {
			pItem->Entry.dwFramingProtocol = RASFP_Slip;
		} else {
			pItem->Entry.dwFramingProtocol = RASFP_Ppp;
		}
				
		if (SendMessage(GetDlgItem(hDlg, IDC_SOFTCOMPCHK), BM_GETCHECK, 0,0)) {
			pItem->Entry.dwfOptions |= RASEO_SwCompression;
		} else {
			pItem->Entry.dwfOptions &= ~RASEO_SwCompression;
		}
				
		if (SendMessage(GetDlgItem(hDlg, IDC_IPCOMP), BM_GETCHECK, 0, 0)) {
			pItem->Entry.dwfOptions |= RASEO_IpHeaderCompression;
		} else {
			pItem->Entry.dwfOptions &= ~RASEO_IpHeaderCompression;
		}
		
		break;
	case WM_COMMAND :
		if (LOWORD(wParam) == IDC_SERVIPADDR) {
			pItem = (PITEMINFO) GetWindowLong (hDlg, GWL_USERDATA);
			
			if (SendMessage(GetDlgItem(hDlg, IDC_SERVIPADDR),
							   BM_GETCHECK, 0, 0)) {
				Flag = FALSE;
			} else {
				Flag = TRUE;
			}
			EnableWindow(GetDlgItem(hDlg, IDC_IP_ADDR), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_IPADDRLABEL), Flag);
			if (Flag || *(int *) &pItem->Entry.ipaddr) {
				SendMessage (GetDlgItem(hDlg, IDC_IP_ADDR), IP_SETADDRESS, 0,
							 (LPARAM) *(int *) &pItem->Entry.ipaddr);
			} else {
				SendMessage(GetDlgItem(hDlg, IDC_IP_ADDR),
							IP_CLEARADDRESS, 0, 0);
			}
		}
		break;
	case WM_INITDIALOG :
		psp = (PROPSHEETPAGE *)lParam;
		
		SetWindowLong (hDlg, GWL_USERDATA, psp->lParam);
		pItem = (PITEMINFO)psp->lParam;

		if (_tcscmp (pItem->Entry.szDeviceType, RASDT_Direct)) {
			// It's a dial-up, change the icon
			HICON hIcon;
			hIcon = LoadIcon(v_hInst, MAKEINTRESOURCE((v_fPortrait ? IDI_RNA2 : IDI_RNA)));
			SendMessage (GetDlgItem (hDlg, IDC_MYICON), STM_SETIMAGE,
						 (WPARAM)IMAGE_ICON, (LPARAM)hIcon);
			DeleteObject(hIcon);
		}
		
		SetWindowLong (GetParent(hDlg), GWL_EXSTYLE,
					   GetWindowLong(GetParent(hDlg), GWL_EXSTYLE) |
					   WS_EX_NODRAG);
		
		SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
					   pItem->EntryName);

		if (pItem->Entry.dwfOptions & RASEO_SpecificIpAddr) {
			Flag = 1;
		} else {
			Flag = 0;
		}
		
		// Set if non-zero of enabled
		if (Flag || *(int *) &pItem->Entry.ipaddr) {
			SendMessage (GetDlgItem(hDlg, IDC_IP_ADDR), IP_SETADDRESS, 0,
						 (LPARAM) *(int *) &pItem->Entry.ipaddr);
		}
			
		SendMessage(GetDlgItem(hDlg, IDC_SERVIPADDR), BM_SETCHECK, !Flag, 0);
		EnableWindow(GetDlgItem(hDlg, IDC_IP_ADDR), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_IPADDRLABEL), Flag);
		
		if (pItem->Entry.dwFramingProtocol == RASFP_Slip) {
			SendMessage(GetDlgItem(hDlg, IDC_USESLIP), BM_SETCHECK,
						1, 0);
		}
		if (pItem->Entry.dwfOptions & RASEO_SwCompression) {
			SendMessage(GetDlgItem(hDlg, IDC_SOFTCOMPCHK), BM_SETCHECK,
						1, 0);
		}
		if (pItem->Entry.dwfOptions & RASEO_IpHeaderCompression) {
			SendMessage(GetDlgItem(hDlg, IDC_IPCOMP), BM_SETCHECK, 1, 0);
		}

		// If it's a VPN type then disable the SLIP option
		if (_tcscmp (pItem->Entry.szDeviceType, RASDT_Vpn)) {
			// Not VPN type, enable the control
			EnableWindow (GetDlgItem (hDlg, IDC_USESLIP), TRUE);
		} else {
			// VPN type, disable slip
			EnableWindow (GetDlgItem (hDlg, IDC_USESLIP), FALSE);
		}
		
		break;
	default :
		DEBUGMSG (1, (TEXT("-TCPGenDlgProc: (default) Return FALSE\r\n")));
		return (FALSE);
	}
	DEBUGMSG (ZONE_MISC, (TEXT("-TCPGenDlgProc: Return TRUE\r\n")));
	return TRUE;
		
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
LRESULT CALLBACK
TCPNameServDlgProc (
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
	PROPSHEETPAGE *psp;
	NMHDR		*header;
	PITEMINFO	pItem;
	BOOL		Flag;
	
	DEBUGMSG (ZONE_MISC, (TEXT("+TCPNameServDlgProc(0x%X, 0x%X, 0x%X, 0x%X)\r\n"),
				   hDlg, message, wParam, lParam));
	
	switch (message) {

	case WM_NOTIFY :
		header = (NMHDR*) lParam;

		if (PSN_APPLY != header->code) {
			break;
		}
		DEBUGMSG (1, (TEXT("Apply Changes....\r\n")));
		pItem = (PITEMINFO) GetWindowLong (hDlg, GWL_USERDATA);
		
		// Server assigned name server address
		if (SendMessage(GetDlgItem(hDlg, IDC_NAMESERVADDR),
						BM_GETCHECK, 0, 0)) {
			pItem->Entry.dwfOptions &= ~RASEO_SpecificNameServers;
			memset ((char *)&(pItem->Entry.ipaddrDns), 0, sizeof(RASIPADDR));
			memset ((char *)&(pItem->Entry.ipaddrDnsAlt), 0,
					sizeof(RASIPADDR));
			memset ((char *)&(pItem->Entry.ipaddrWins), 0, sizeof(RASIPADDR));
			memset ((char *)&(pItem->Entry.ipaddrWinsAlt), 0,
					sizeof(RASIPADDR));
		} else {
			pItem->Entry.dwfOptions |= RASEO_SpecificNameServers;
					
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ADDR), IP_GETADDRESS, 0,
						(LPARAM) ((int *) &pItem->Entry.ipaddrDns));
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ALTADDR), IP_GETADDRESS, 0,
						(LPARAM) ((int *) &pItem->Entry.ipaddrDnsAlt));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ADDR), IP_GETADDRESS, 0,
						(LPARAM) ((int *) &pItem->Entry.ipaddrWins));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ALTADDR), IP_GETADDRESS, 0,
						(LPARAM) ((int *) &pItem->Entry.ipaddrWinsAlt));
		}
		break;
	case WM_COMMAND :
		if (LOWORD(wParam) == IDC_NAMESERVADDR) {
			pItem = (PITEMINFO) GetWindowLong (hDlg, GWL_USERDATA);
			Flag = !SendMessage(GetDlgItem(hDlg, IDC_NAMESERVADDR),
							   BM_GETCHECK, 0, 0);

			EnableWindow(GetDlgItem(hDlg, IDC_DNS_ADDR), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_DNS_ALTADDR), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_WINS_ADDR), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_WINS_ALTADDR), Flag);
			
			EnableWindow(GetDlgItem(hDlg, IDC_DNS_LABEL), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_DNS_ALTLABEL), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_WINS_LABEL), Flag);
			EnableWindow(GetDlgItem(hDlg, IDC_WINS_ALTLABEL), Flag);

			// If enabled then show the addresses.
			if (Flag) {
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrDns));
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ALTADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrDnsAlt));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrWins));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ALTADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrWinsAlt));
			} else {
				SendMessage(GetDlgItem(hDlg, IDC_DNS_ADDR),
							IP_CLEARADDRESS, 0, 0);
				SendMessage(GetDlgItem(hDlg, IDC_DNS_ALTADDR),
							IP_CLEARADDRESS, 0, 0);
				SendMessage(GetDlgItem(hDlg, IDC_WINS_ADDR),
							IP_CLEARADDRESS, 0, 0);
				SendMessage(GetDlgItem(hDlg, IDC_WINS_ALTADDR),
							IP_CLEARADDRESS, 0, 0);
			}
		}
		break;
	case WM_INITDIALOG :
		psp = (PROPSHEETPAGE *)lParam;
		
		SetWindowLong (hDlg, GWL_USERDATA, psp->lParam);
		pItem = (PITEMINFO)psp->lParam;

		if (_tcscmp (pItem->Entry.szDeviceType, RASDT_Direct)) {
			// It's a dial-up, change the icon
			HICON hIcon;
			hIcon = LoadIcon(v_hInst, MAKEINTRESOURCE((v_fPortrait ? IDI_RNA2 : IDI_RNA)));
			SendMessage (GetDlgItem (hDlg, IDC_MYICON), STM_SETIMAGE,
						 (WPARAM)IMAGE_ICON, (LPARAM)hIcon);
			DeleteObject(hIcon);
		}
		
		SetWindowText (GetDlgItem (hDlg, IDC_CONNLABEL),
					   pItem->EntryName);

		// Name server address
		if (pItem->Entry.dwfOptions & RASEO_SpecificNameServers) {
			Flag = 1;
		} else {
			Flag = 0;
		}

		// Invert flag for this.
		SendMessage(GetDlgItem(hDlg, IDC_NAMESERVADDR), BM_SETCHECK,
					!Flag, 0);                        

		// Server assigned name server address, so disable
		EnableWindow(GetDlgItem(hDlg, IDC_DNS_ADDR), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_DNS_ALTADDR), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_WINS_ADDR), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_WINS_ALTADDR), Flag);
			
		EnableWindow(GetDlgItem(hDlg, IDC_DNS_LABEL), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_DNS_ALTLABEL), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_WINS_LABEL), Flag);
		EnableWindow(GetDlgItem(hDlg, IDC_WINS_ALTLABEL), Flag);

		if (Flag) {
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrDns));
			SendMessage(GetDlgItem(hDlg, IDC_DNS_ALTADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrDnsAlt));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrWins));
			SendMessage(GetDlgItem(hDlg, IDC_WINS_ALTADDR),
						IP_SETADDRESS, 0,
						(LPARAM) *((int *) &pItem->Entry.ipaddrWinsAlt));
		} 
		break;
	default :
		DEBUGMSG (1, (TEXT("-TCPNameServDlgProc: (default) Return FALSE\r\n")));
		return (FALSE);
	}
	DEBUGMSG (ZONE_MISC, (TEXT("-TCPNameServDlgProc: Return TRUE\r\n")));
	return TRUE;
		
}

int CALLBACK 
PropSheetProc(
   HWND hwndDlg,  // handle to the property sheet dialog box
   UINT uMsg,     // message identifier
   LPARAM lParam  // message parameter
   )
{
	DEBUGMSG (1, (TEXT("PropSheetProc(0x%X, 0x%X, 0x%X)\r\n"),
				   hwndDlg, uMsg, lParam));
	return 0;
}   



BOOL
TCP_IP_Properties(HWND hWndOwner, PITEMINFO pItem)
{
	PROPSHEETPAGE psp[2];
	PROPSHEETHEADER psh;
	TCHAR szCaption1[30];
	TCHAR szCaption2[30];
	TCHAR szTitle[30];
	

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = v_hInst;
	psp[0].pszTemplate = MAKEINTRESOURCE(v_DialogPages[DLG_TCP_GEN]);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = TCPGenDlgProc;
	LoadString(v_hInst, IDS_TCPIP_GENERAL, szCaption1,
			   sizeof(szCaption1)/sizeof(szCaption1[0]));
	psp[0].pszTitle = szCaption1;
	psp[0].lParam = (LPARAM)pItem;
	psp[0].pfnCallback = NULL;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance = v_hInst;
	psp[1].pszTemplate = MAKEINTRESOURCE(v_DialogPages[DLG_TCP_NS]);
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = TCPNameServDlgProc;
	LoadString(v_hInst, IDS_TCPIP_NAME_SERV, szCaption2,
			   sizeof(szCaption2)/sizeof(szCaption2[0]));
	psp[1].pszTitle = szCaption2;
	psp[1].lParam = (LPARAM)pItem;
	psp[1].pfnCallback = NULL;


	psh.dwSize     = sizeof(PROPSHEETHEADER);
	psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
	psh.hwndParent = hWndOwner;
	psh.hInstance  = v_hInst;
	psh.pszIcon    = NULL;
	LoadString(v_hInst, IDS_TCPIP_SETTINGS, szTitle,
			   sizeof(szTitle)/sizeof(szTitle[0]));
	psh.pszCaption = (LPTSTR) szTitle;
	psh.nPages     = 2;
	psh.nStartPage = 0;       // we will always jump to the first page
	psh.ppsp       = (LPCPROPSHEETPAGE) &psp;
	psh.pfnCallback = PropSheetProc;

	return CallCommCtrlFunc(PropertySheetW)(&psh);
}
